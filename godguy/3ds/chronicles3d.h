// GodGuy 3DS - The Ashen Chronicles: unlockable companion story.
// A bounded near-real-time tactical/survival campaign on the EXISTING
// world[][] / bld[][] map. The god-game remains 2D-led and unchanged.
// No terrain editing, duplicated world, heap allocations or per-frame scans.
// Included ONLY by main3ds.c, after explore3d.h and population3d.h.
#ifndef GG_CHRONICLES3D_H
#define GG_CHRONICLES3D_H

enum { GG_CHRON_ACTS=4, GG_CHRON_POSTS=3, GG_CHRON_FOES=9,
       GG_CHRON_GRID=33, GG_CHRON_CELLS=GG_CHRON_GRID*GG_CHRON_GRID };
typedef struct { short x,y; signed char hp,post,wait; } GgChronFoe;
typedef struct {
    int active,loaded,act,cleared,frame,lastKeys;
    int hp,stamina,food,rations,morale,score,attackWait,attackFlash;
    int post,won,lost,briefTime,day,dirty;
    short px[GG_CHRON_POSTS],py[GG_CHRON_POSTS];
    GgChronFoe foe[GG_CHRON_FOES];
    int foeN;
} GgChronicles;
static GgChronicles ggChron;
static const char *const ggChronActName[GG_CHRON_ACTS]={
    "THE FORD'S LAST LIGHT","THE BROKEN WATCH",
    "THE BORDER OF NAMES","THE HOLLOW MARCH"
};
static const char *const ggChronPrologue[GG_CHRON_ACTS]={
    "MIRA: BRING THE SUPPLY COLUMN THROUGH.",
    "THORGRIM: RESTORE THE BEACON ROAD.",
    "SERIN: THESE LOST NAMES ARE REAL PLACES.",
    "ELOWEN: HOLD THE ROAD AGAINST THE HOLLOW."
};
static const char *const ggChronPostName[GG_CHRON_POSTS]={
    "THE OUTER CAMP","THE WATCH POST","THE LAST STANDARD"
};
static const int ggChronRequires[GG_CHRON_ACTS]={3,5,8,14};

static int ggChronReady(void){return chapter>=3||storyDone;}
static int ggChronLand(int x,int y){
    if(x<1||y<1||x>=WW-1||y>=WH-1)return 0;
    if(world[y][x]<=T_WATER||world[y][x]==T_LAVA)return 0;
    return !bld[y][x]&&!occ[y][x];
}
static int ggChronNear(int ax,int ay,int bx,int by){
    int dx=ax-bx,dy=ay-by;
    return abs(dx)+abs(dy);
}
// A tiny, once-per-mission flood fill chooses reachable real-world
// objectives, not arbitrary mountains, islands or impossible water crossings.
static void ggChronRoute(void){
    unsigned char dist[GG_CHRON_CELLS];
    unsigned short queue[GG_CHRON_CELLS];
    const int sx=ggExplore.px>>8,sy=ggExplore.py>>8;
    const int ox=sx-16,oy=sy-16;
    int front=0,back=0;
    memset(dist,255,sizeof dist);
    dist[16*GG_CHRON_GRID+16]=0;
    queue[back++]=16*GG_CHRON_GRID+16;
    while(front<back){
        int pos=queue[front++],x=pos%GG_CHRON_GRID,y=pos/GG_CHRON_GRID;
        static const signed char dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
        for(int k=0;k<4;k++){
            int nx=x+dx[k],ny=y+dy[k];
            if((unsigned)nx>=GG_CHRON_GRID||(unsigned)ny>=GG_CHRON_GRID)continue;
            int q=ny*GG_CHRON_GRID+nx;
            if(dist[q]!=255||!ggChronLand(ox+nx,oy+ny))continue;
            if(ggExpGround(world[oy+ny][ox+nx])-
               ggExpGround(world[oy+y][ox+x])>19)continue;
            dist[q]=dist[pos]+1;
            queue[back++]=(unsigned short)q;
        }
    }
    for(int post=0;post<GG_CHRON_POSTS;post++){
        int goal=7+post*6,choice=16*GG_CHRON_GRID+16,best=0x7fffffff;
        for(int n=1;n<back;n++){
            int p=queue[n],x=ox+p%GG_CHRON_GRID,y=oy+p/GG_CHRON_GRID;
            if(dist[p]<4)continue;
            int separated=1;
            for(int j=0;j<post;j++)
                if(ggChronNear(x,y,ggChron.px[j],ggChron.py[j])<5)
                    separated=0;
            if(!separated)continue;
            int diff=abs((int)dist[p]-goal);
            // Architectural landmarks make each route feel authored:
            // towns, ruins and towers are rewarded, never fabricated.
            int landmark=0;
            for(int yy=y-2;yy<=y+2;yy++)
                for(int xx=x-2;xx<=x+2;xx++)
                    if((unsigned)xx<WW&&(unsigned)yy<WH &&
                       (bld[yy][xx]==B_HALL||bld[yy][xx]==B_TOWER||
                        bld[yy][xx]==B_RUIN||bld[yy][xx]==B_TEMPLE))
                        landmark=1;
            int cost=diff*15-landmark*16+
                ((x*11+y*7+post*13+ggChron.act*17)&15);
            if(cost<best){best=cost;choice=p;}
        }
        ggChron.px[post]=(short)(ox+choice%GG_CHRON_GRID);
        ggChron.py[post]=(short)(oy+choice/GG_CHRON_GRID);
    }
}
static void ggChronSpawn(void){
    ggChron.foeN=0;
    for(int post=0;post<GG_CHRON_POSTS;post++){
        int n=2+(ggChron.act>=2);
        for(int k=0;k<n;k++){
            const int dx[8]={1,-1,0,0,2,-2,1,-1};
            const int dy[8]={0,0,1,-1,1,-1,-2,2};
            int x=ggChron.px[post],y=ggChron.py[post];
            int found=0;
            for(int t=0;t<8;t++){
                int z=(t+k*3+post)&7,nx=x+dx[z],ny=y+dy[z];
                if(!ggChronLand(nx,ny))continue;
                int occupied=0;
                for(int j=0;j<ggChron.foeN;j++)
                    if(ggChron.foe[j].x==nx&&ggChron.foe[j].y==ny)
                        occupied=1;
                if(!occupied){x=nx;y=ny;found=1;break;}
            }
            if(!found)continue; // a narrow bridge should not spawn stacked foes
            GgChronFoe *f=&ggChron.foe[ggChron.foeN++];
            f->x=(short)x;f->y=(short)y;
            f->post=(signed char)post;
            f->hp=(signed char)(20+ggChron.act*6);
            f->wait=(signed char)(9+post*7+k*5);
        }
    }
}
static void ggChronLoad(void){
    if(ggChron.loaded)return;
    ggChron.loaded=1;
    // Companion progress uses a separate tiny file: never alters the
    // regular GodGuy world's serialized save format.
    FILE *f=fopen("sdmc:/godguy_chronicles.dat","rb");
    if(!f)return;
    u32 data[4];
    int good=fread(data,sizeof data,1,f)==1;
    fclose(f);
    if(good&&data[0]==0x47474348u&&data[1]==1&&data[2]<=GG_CHRON_ACTS &&
       data[3]==(data[0]^data[1]^data[2]^0x5aa59669u))
        ggChron.cleared=(int)data[2];
}
static void ggChronSave(void){
    const u32 data[4]={0x47474348u,1,(u32)ggChron.cleared,
                        0x47474348u^1u^(u32)ggChron.cleared^0x5aa59669u};
    FILE *f=fopen("sdmc:/godguy_chronicles.dat","wb");
    if(!f)return;
    (void)fwrite(data,sizeof data,1,f);
    fclose(f);
}
static int ggChronUnlockedAct(int act){
    if(act<0||act>=GG_CHRON_ACTS)return 0;
    return storyDone||chapter>=ggChronRequires[act];
}
static int ggChronBegin(int x,int y,int oldPause){
    ggChronLoad();
    if(!ggChronReady())return 0;
    int act=ggChron.cleared<GG_CHRON_ACTS?ggChron.cleared:GG_CHRON_ACTS-1;
    if(!ggChronUnlockedAct(act))return -1;
    ggExpBegin(x,y,oldPause);
    ggChron.active=1;
    ggChron.act=act;
    ggChron.frame=ggChron.lastKeys=0;
    ggChron.hp=100;ggChron.stamina=100;ggChron.food=100;
    ggChron.rations=3;ggChron.morale=70;ggChron.score=0;
    ggChron.post=0;ggChron.won=ggChron.lost=0;
    ggChron.attackWait=ggChron.attackFlash=0;
    ggChron.briefTime=165;ggChron.day=0;ggChron.dirty=1;
    // The 3D role is the expedition's commander, NOT possession of a
    // random sapient citizen from the original story.
    ggExplore.actorId=0;
    ggExplore.actorIndex=-1;
    ggChronRoute();
    ggChronSpawn();
    return 1;
}
static int ggChronEnemiesAt(int post){
    int n=0;
    for(int i=0;i<ggChron.foeN;i++)
        if(ggChron.foe[i].hp>0&&ggChron.foe[i].post==post)n++;
    return n;
}
static void ggChronHit(void){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    int a=ggExplore.angle&255;
    int found=-1,nearest=9000;
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe *f=&ggChron.foe[i];
        if(f->hp<=0)continue;
        int vx=((int)f->x<<8)+128-ggExplore.px;
        int vy=((int)f->y<<8)+128-ggExplore.py;
        int forward=(vx*ggExpCos[a]+vy*ggExpSin[a])/4096;
        int side=(-vx*ggExpSin[a]+vy*ggExpCos[a])/4096;
        if(forward<10||forward>720||abs(side)>forward/2+70 ||
           !ggExpLineClear(px,py,f->x,f->y))continue;
        if(forward<nearest){found=i;nearest=forward;}
    }
    ggChron.attackWait=11;
    ggChron.attackFlash=5;
    if(found<0)return;
    GgChronFoe *f=&ggChron.foe[found];
    int damage=12+(ggChron.morale>=75?3:0);
    f->hp-=damage;
    ggChron.score+=5;
    if(f->hp<=0){ggChron.morale=ggExpClamp(ggChron.morale+3,0,100);
                 ggChron.score+=20;}
    ggChron.dirty=1;
}
static void ggChronEnemiesStep(void){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    for(int i=0;i<ggChron.foeN;i++){
        GgChronFoe *f=&ggChron.foe[i];
        if(f->hp<=0)continue;
        if(f->wait>0){f->wait--;continue;}
        int distance=ggChronNear(f->x,f->y,px,py);
        if(distance<=2){
            ggChron.hp-=4+ggChron.act;
            ggChron.morale=ggExpClamp(ggChron.morale-1,0,100);
            f->wait=(signed char)(33+(i&3)*5);
            ggChron.dirty=1;
        }else if(distance<8){
            // Bounded pursuit with axis fallback; no all-world A* search
            // and never overwrite or damage live god-game inhabitants.
            int dx=px>f->x?1:px<f->x?-1:0;
            int dy=py>f->y?1:py<f->y?-1:0;
            int nx=f->x,ny=f->y;
            if(abs(px-f->x)>=abs(py-f->y)&&ggChronLand(nx+dx,ny))
                nx+=dx;
            else if(ggChronLand(nx,ny+dy))ny+=dy;
            else if(ggChronLand(nx+dx,ny))nx+=dx;
            if(ggChronNear(nx,ny,px,py)>0){
                int collide=0;
                for(int j=0;j<ggChron.foeN;j++)if(j!=i&&
                    ggChron.foe[j].hp>0&&ggChron.foe[j].x==nx&&
                    ggChron.foe[j].y==ny){collide=1;break;}
                if(!collide){f->x=(short)nx;f->y=(short)ny;}
            }
            f->wait=(signed char)(21+(i&3)*4);
            ggChron.dirty=1;
        }else f->wait=25;
    }
}
static int ggChronStep(int raw){
    if(!ggChron.active)return 0;
    int down=raw&~ggChron.lastKeys;
    ggChron.lastKeys=raw;
    if(down&KEY_START)return 1; // Retreat to intact 2D story, no lost world
    if(ggChron.won||ggChron.lost){
        ggChron.frame++;
        return 0; // Ending remains readable until START is pressed
    }
    ggChron.frame++;
    ggChron.day=(ggChron.frame/800)%4;
    if(ggChron.attackWait)ggChron.attackWait--;
    if(ggChron.attackFlash)ggChron.attackFlash--;
    if(ggChron.briefTime)ggChron.briefTime--;
    // All existing movement and collision rules are reused, but terrain
    // placement, deletion, warping and quicksave are not combat actions.
    int moving=raw&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_L|KEY_R|KEY_B);
    if(ggChron.stamina<8)moving&=~KEY_B;
    ggExpStep(moving,0,0);
    if((moving&KEY_B)&&(moving&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT))){
        ggChron.stamina=ggExpClamp(ggChron.stamina-2,0,100);
    }else if((ggChron.frame&1)==0)
        ggChron.stamina=ggExpClamp(ggChron.stamina+1,0,100);
    if(ggChron.briefTime==0){
        if((down&KEY_A)&&!ggChron.attackWait)ggChronHit();
        if((down&KEY_Y)&&ggChron.rations>0){
            ggChron.rations--;
            ggChron.hp=ggExpClamp(ggChron.hp+25,0,100);
            ggChron.food=ggExpClamp(ggChron.food+35,0,100);
            ggChron.dirty=1;
        }
        if((down&KEY_X)&&ggChron.post<GG_CHRON_POSTS){
            int p=ggChron.post;
            if(ggChronNear(ggExplore.px>>8,ggExplore.py>>8,
                           ggChron.px[p],ggChron.py[p])<=2 &&
               !ggChronEnemiesAt(p)){
                ggChron.post++;
                ggChron.rations=ggExpClamp(ggChron.rations+1,0,9);
                ggChron.morale=ggExpClamp(ggChron.morale+12,0,100);
                ggChron.hp=ggExpClamp(ggChron.hp+12,0,100);
                ggChron.score+=100;
                ggChron.briefTime=120;
                ggChron.dirty=1;
                if(ggChron.post==GG_CHRON_POSTS){
                    ggChron.won=1;
                    if(ggChron.cleared<ggChron.act+1){
                        ggChron.cleared=ggChron.act+1;
                        ggChronSave();
                    }
                }
            }
        }
    }
    if(ggChron.frame%540==0){
        ggChron.food=ggExpClamp(ggChron.food-10,0,100);
        if(ggChron.food==0)ggChron.hp-=5;
        ggChron.dirty=1;
    }
    if(ggChron.frame%5==0&&ggChron.briefTime==0)ggChronEnemiesStep();
    if(ggChron.hp<=0){
        ggChron.hp=0;ggChron.lost=1;ggChron.dirty=1;
    }
    return 0;
}
static int ggChronProject(int x,int y,int *sx,int *foot,int *scale){
    int vx=(x<<8)+128-ggExplore.px,vy=(y<<8)+128-ggExplore.py;
    int a=ggExplore.angle&255;
    int forward=(vx*ggExpCos[a]+vy*ggExpSin[a])/4096;
    int side=(-vx*ggExpSin[a]+vy*ggExpCos[a])/4096;
    if(forward<=80||forward>16*256||abs(side)*4>forward*5)return 0;
    if(!ggExpLineClear(ggExplore.px>>8,ggExplore.py>>8,x,y))return 0;
    int d=forward/256+1;
    *sx=200+side*140/forward;
    *foot=(58-(ggExpGround(world[y][x])-
            ggExpGround(world[ggExplore.py>>8][ggExplore.px>>8]))*6/d)*2;
    *scale=ggExpClamp(2300/(d+5),6,38);
    return *sx>-50&&*sx<450&&*foot>=0&&*foot<270;
}
static void ggChronDraw(u8 *out){
    if(!ggChron.active)return;
    // Native-resolution heraldic billboards instead of Minecraft
    // blocks: guards, shields, banner cloth and grounded shadows.
    for(int i=ggChron.foeN-1;i>=0;i--){
        const GgChronFoe *f=&ggChron.foe[i];
        if(f->hp<=0)continue;
        int x,y,s;
        if(!ggChronProject(f->x,f->y,&x,&y,&s))continue;
        int w=ggExpClamp(s*2/3,4,26);
        int bob=(ggChron.frame/12+i)&1;
        ggExpRect(out,x-w,y-2,w*2,2,25,33,30);
        ggExpRect(out,x-w/2,y-s*2+bob,w,s,67,38,42);
        ggExpRect(out,x-w/2-3,y-s+bob,4,s/2,42,29,35);
        ggExpRect(out,x+w/2-1,y-s+bob,4,s/2,42,29,35);
        ggExpRect(out,x-w/3,y-s*2-s/2+bob,w*2/3,s/2,180,137,112);
        ggExpRect(out,x-w/2,y-s*2-s/2+bob,w,3,47,56,68);
        ggExpRect(out,x+w/2,y-s*2+2,ggExpClamp(s/2,3,9),
                  ggExpClamp(s,8,28),116,128,137);
        ggExpRect(out,x-w/2,y-s*2-s/2-5,w*2,3,33,26,29);
        ggExpRect(out,x-w/2,y-s*2-s/2-5,
                  w*2*ggExpClamp(f->hp,0,40)/40,3,192,69,59);
    }
    if(!ggChron.won){
        int p=ggChron.post,x,y,s;
        if(p<GG_CHRON_POSTS&&ggChronProject(ggChron.px[p],ggChron.py[p],
                                            &x,&y,&s)){
            int h=ggExpClamp(s*3,28,86);
            ggExpRect(out,x,y-h,3,h,225,201,126);
            ggExpRect(out,x+3,y-h,ggExpClamp(s,12,26),12,168,116,49);
            ggExpRect(out,x+5,y-h+3,9,3,244,218,142);
        }
    }
    // Replace the free-build UI completely. A commander must read the
    // tactical situation, not a block inventory.
    ggExpRect(out,0,0,400,37,13,23,34);
    ggExpRect(out,0,35,400,2,177,128,71);
    char line[100];
    snprintf(line,sizeof line,"ASHEN CHRONICLES %d/4  %.24s",
             ggChron.act+1,ggChronActName[ggChron.act]);
    ggExpText(out,9,5,line,248,220,165);
    snprintf(line,sizeof line,"HEALTH %3d   STAMINA %3d   FOOD %3d   RATIONS %d   MORALE %d",
             ggChron.hp,ggChron.stamina,ggChron.food,ggChron.rations,ggChron.morale);
    ggExpText(out,9,20,line,211,222,216);
    ggExpRect(out,0,195,400,45,12,22,34);
    ggExpRect(out,0,195,400,2,177,128,71);
    if(ggChron.won){
        ggExpText(out,9,204,"VICTORY - THE ROAD IS SECURE. A NEW ACT IS AVAILABLE.",244,211,146);
    }else if(ggChron.lost){
        ggExpText(out,9,204,"EXPEDITION LOST - RETREAT AND REGROUP.",248,132,116);
    }else{
        int p=ggChron.post;
        int distance=ggChronNear(ggExplore.px>>8,ggExplore.py>>8,
                               ggChron.px[p],ggChron.py[p]);
        int dx=ggChron.px[p]-(ggExplore.px>>8);
        int dy=ggChron.py[p]-(ggExplore.py>>8);
        const char *dir=abs(dx)>abs(dy)?(dx>=0?"EAST":"WEST"):
                                             (dy>=0?"SOUTH":"NORTH");
        snprintf(line,sizeof line,"OBJECTIVE %d/3: %s  %s %d TILES  GUARDS %d",
                 p+1,ggChronPostName[p],dir,distance,ggChronEnemiesAt(p));
        ggExpText(out,9,203,line,247,216,155);
    }
    ggExpText(out,9,221,"A STRIKE  X SECURE POST  Y RATION  B SPRINT  L/R LOOK  START 2D",
              201,214,224);
    if(ggChron.briefTime>0){
        ggExpRect(out,22,69,356,65,17,31,44);
        ggExpRect(out,22,69,356,2,187,139,79);
        ggExpText(out,38,82,ggChron.won?"THE ROAD BELONGS TO THE LIVING.":
            ggChron.post==0?ggChronPrologue[ggChron.act]:
            ggChron.post==1?"THE FIRST STANDARD RISES. KEEP MOVING.":
            ggChron.post==2?"THE WATCH HOLDS. THE LAST BANNER WAITS.":
                             "THE THREE STANDARDS STAND TOGETHER.",
            243,214,166);
        ggExpText(out,38,105,"REAL SETTLEMENTS. REAL ROADS. NO TERRAIN BUILDING.",
                  180,207,225);
    }
    if(ggChron.attackFlash)
        ggExpRect(out,194,114,12,2,245,217,150);
    ggChron.dirty=0;
}
#endif
