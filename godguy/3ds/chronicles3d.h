// GodGuy 3DS - The Unwritten Reign: unlockable companion story.
// A bounded near-real-time tactical/survival campaign on the EXISTING
// world[][] / bld[][] map. The god-game remains 2D-led and unchanged.
// No terrain editing, duplicated world, heap allocations or per-frame scans.
// Included ONLY by main3ds.c, after explore3d.h and population3d.h.
#ifndef GG_CHRONICLES3D_H
#define GG_CHRONICLES3D_H

#include "platform_keys3d.h"

enum { GG_CHRON_ACTS=4, GG_CHRON_POSTS=3, GG_CHRON_FOES=9,
       GG_CHRON_GRID=33, GG_CHRON_CELLS=GG_CHRON_GRID*GG_CHRON_GRID };
enum { GG_CHRON_BLADE, GG_CHRON_BOW, GG_CHRON_EMBER, GG_CHRON_STYLES };
enum { GG_CHRON_SHOTS=4 };
typedef struct { short x,y; signed char hp,post,wait; } GgChronFoe;
typedef struct {
    int x,y,z,vx,vy,vz;
    unsigned char life,style,damage,stagger;
} GgChronShot;
typedef struct {
    int active,loaded,act,cleared,frame,lastKeys;
    int hp,stamina,food,rations,morale,score,attackWait,attackFlash;
    int post,won,lost,briefTime,day,dirty;
    int routeOx,routeOy; // mission's already-validated 33x33 walking region
    short px[GG_CHRON_POSTS],py[GG_CHRON_POSTS];
    GgChronFoe foe[GG_CHRON_FOES];
    unsigned char foeWindup[GG_CHRON_FOES],foeAlert[GG_CHRON_FOES];
    signed char foeGuard[GG_CHRON_FOES];
    GgChronShot shots[GG_CHRON_SHOTS];
    int style,focus,charge,swing,heavy,guardTicks;
    int combatNotice,combatNoticeT;
    int foeN;
} GgChronicles;
static GgChronicles ggChron;
// Bounded cache, built only when a mission or objective changes; this
// is never allocated again or computed on an individual 3D draw call.
static unsigned char ggChronNavDistance[GG_CHRON_CELLS];
static const char *const ggChronActName[GG_CHRON_ACTS]={
    "THE FORD'S LAST LIGHT","THE BROKEN WATCH",
    "THE BORDER OF NAMES","THE HOLLOW MARCH"
};
static const char *const ggChronPrologue[GG_CHRON_ACTS]={
    "MIRA: A GODBORN SOUL CHOOSES ITS OWN PATH.",
    "THORGRIM: EVEN DIVINITY CANNOT BUY TRUST.",
    "SERIN: THESE NAMES BELONG TO REAL TOWNS.",
    "ELOWEN: NO CROWN CAN MAKE THE WORLD OBEY."
};
static const char *const ggChronPostName[GG_CHRON_POSTS]={
    "THE OUTER CAMP","THE WATCH POST","THE LAST STANDARD"
};
static const int ggChronRequires[GG_CHRON_ACTS]={3,5,8,14};

// Resolved by the 3DS-only saga director after the player secures a final post.
static void ggSagaStartChoice(void);
static int ggSagaParley(int post);
static int ggReignNearestGuard(int x,int y,int *gx,int *gy);
static int ggReignGuardPosition(int index,int *x,int *y);
static void ggReignGuardDamage(int index,int damage);
static void ggReignBlockGuardCells(unsigned char *dist,int width,int ox,int oy);
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
static int ggChronRoute(void){
    unsigned char dist[GG_CHRON_CELLS];
    unsigned short queue[GG_CHRON_CELLS];
    const int sx=ggExplore.px>>8,sy=ggExplore.py>>8;
    const int ox=sx-16,oy=sy-16;
    ggChron.routeOx=ox;ggChron.routeOy=oy;
    int front=0,back=0;
    memset(dist,255,sizeof dist);
    dist[16*GG_CHRON_GRID+16]=0;
    queue[back++]=16*GG_CHRON_GRID+16;
    while(front<back){
        int pos=queue[front++],x=pos%GG_CHRON_GRID,y=pos/GG_CHRON_GRID;
        if(dist[pos]>=254)continue; // no unsigned-distance wrap in a maze
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
        if(best==0x7fffffff)return 0; // no reachable distinct site
    }
    return 1;
}

static void ggChronNavBuild(int post){
    // Reverse breadth-first walking distances from the CURRENT objective.
    // The flow field uses the SAME map cells and climb rules as the 3D
    // player's feet. Render/UI lookups are constant-time and cheap.
    memset(ggChronNavDistance,255,sizeof ggChronNavDistance);
    if(post<0||post>=GG_CHRON_POSTS)return;
    int xx=ggChron.px[post]-ggChron.routeOx;
    int yy=ggChron.py[post]-ggChron.routeOy;
    if((unsigned)xx>=GG_CHRON_GRID||(unsigned)yy>=GG_CHRON_GRID)
        return;
    unsigned short queue[GG_CHRON_CELLS];
    int front=0,back=0,origin=yy*GG_CHRON_GRID+xx;
    queue[back++]=(unsigned short)origin;
    ggChronNavDistance[origin]=0;
    static const signed char dx[4]={1,0,-1,0};
    static const signed char dy[4]={0,1,0,-1};
    while(front<back){
        int pos=queue[front++],x=pos%GG_CHRON_GRID,y=pos/GG_CHRON_GRID;
        if(ggChronNavDistance[pos]>=254)continue;
        int toX=ggChron.routeOx+x,toY=ggChron.routeOy+y;
        for(int k=0;k<4;k++){
            int nx=x+dx[k],ny=y+dy[k];
            if((unsigned)nx>=GG_CHRON_GRID||
               (unsigned)ny>=GG_CHRON_GRID)continue;
            int q=ny*GG_CHRON_GRID+nx;
            if(ggChronNavDistance[q]!=255)continue;
            int fromX=ggChron.routeOx+nx,fromY=ggChron.routeOy+ny;
            if(!ggChronLand(fromX,fromY))continue;
            // Reverse traversal: a player at the neighbor must be able
            // to climb toward the current cell, not the other way round.
            if(ggExpGround(world[toY][toX])-
               ggExpGround(world[fromY][fromX])>19)continue;
            ggChronNavDistance[q]=ggChronNavDistance[pos]+1;
            queue[back++]=(unsigned short)q;
        }
    }
}
static int ggChronNavNext(int x,int y,int *nx,int *ny,int *steps){
    int lx=x-ggChron.routeOx,ly=y-ggChron.routeOy;
    if((unsigned)lx>=GG_CHRON_GRID||(unsigned)ly>=GG_CHRON_GRID)
        return 0;
    int pos=ly*GG_CHRON_GRID+lx;
    unsigned d=ggChronNavDistance[pos];
    if(d==255)return 0;
    *steps=(int)d;*nx=x;*ny=y;
    if(d==0)return 1;
    static const signed char dx[4]={1,0,-1,0};
    static const signed char dy[4]={0,1,0,-1};
    for(int k=0;k<4;k++){
        int tx=lx+dx[k],ty=ly+dy[k];
        if((unsigned)tx>=GG_CHRON_GRID||
           (unsigned)ty>=GG_CHRON_GRID)continue;
        int q=ty*GG_CHRON_GRID+tx;
        if(ggChronNavDistance[q]<d){
            *nx=ggChron.routeOx+tx;
            *ny=ggChron.routeOy+ty;
            return 1;
        }
    }
    return 0;
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
            // Last-act captains are actual bosses, not identical guards:
            // heavier health, bounded enemy count and the same combat AI.
            f->hp=(signed char)(ggChron.act==GG_CHRON_ACTS-1&&
                 post==GG_CHRON_POSTS-1&&k==0?105:
                 20+ggChron.act*6+post*3);
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
    ggExplore.actorId=0; // do not inherit a former hybrid POV walk rule
    if(!ggExpBegin(x,y,oldPause))return -2;
    ggChron.active=1;
    ggChron.act=act;
    ggChron.frame=ggChron.lastKeys=0;
    ggChron.hp=100;ggChron.stamina=100;ggChron.food=100;
    ggChron.rations=3;ggChron.morale=70;ggChron.score=0;
    ggChron.post=0;ggChron.won=ggChron.lost=0;
    ggChron.attackWait=ggChron.attackFlash=0;
    ggChron.style=GG_CHRON_BLADE;ggChron.focus=100;
    ggChron.charge=ggChron.swing=ggChron.heavy=ggChron.guardTicks=0;
    ggChron.combatNotice=ggChron.combatNoticeT=0;
    memset(ggChron.foeWindup,0,sizeof ggChron.foeWindup);
    memset(ggChron.foeAlert,0,sizeof ggChron.foeAlert);
    memset(ggChron.foeGuard,-1,sizeof ggChron.foeGuard);
    memset(ggChron.shots,0,sizeof ggChron.shots);
    ggChron.briefTime=165;ggChron.day=0;ggChron.dirty=1;
    // The 3D role is the expedition's commander, NOT possession of a
    // random sapient citizen from the original story.
    ggExplore.actorId=0;
    ggExplore.actorIndex=-1;
    if(!ggChronRoute()){
        ggChron.active=0;
        ggExplore.active=0;
        return -2; // no traversable route: never create a soft-locked act
    }
    ggChronSpawn();
    ggChronNavBuild(0);
    return 1;
}
static int ggChronEnemiesAt(int post){
    int n=0;
    for(int i=0;i<ggChron.foeN;i++)
        if(ggChron.foe[i].hp>0&&ggChron.foe[i].post==post)n++;
    return n;
}
#include "chronicle_combat3d.h"

static int ggChronFoeAt(int x,int y){
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe *f=&ggChron.foe[i];
        if(f->hp>0&&f->x==x&&f->y==y)return 1;
    }
    return 0;
}
static void ggChronRouteBlockCell(unsigned char *dist,int width,int ox,int oy,
                                  int x,int y){
    int lx=x-ox,ly=y-oy;
    // Reserve the cell without queueing it. A real route is shorter than 254.
    if((unsigned)lx<(unsigned)width&&(unsigned)ly<(unsigned)width)
        dist[ly*width+lx]=254;
}
static void ggChronRouteBlockActors(unsigned char *dist,int width,int ox,int oy){
    ggChronRouteBlockCell(dist,width,ox,oy,ggExplore.px>>8,ggExplore.py>>8);
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe *f=&ggChron.foe[i];
        if(f->hp>0)ggChronRouteBlockCell(dist,width,ox,oy,f->x,f->y);
    }
    ggReignBlockGuardCells(dist,width,ox,oy);
}
static int ggChronEnemyRoute(int fx,int fy,int tx,int ty,int *outX,int *outY){
    // Bounded 9x9 pursuit flood fill. Runs only after enemy cooldown,
    // never per render; respects existing terrain and occupants.
    enum { R=4,W=9,N=81 };
    unsigned char dist[N],first[N],queue[N];
    memset(dist,255,sizeof dist);
    ggChronRouteBlockActors(dist,W,fx-R,fy-R);
    ggChronRouteBlockCell(dist,W,fx-R,fy-R,tx,ty);
    dist[R*W+R]=0;first[R*W+R]=255;
    int head=0,tail=0,best=R*W+R;
    int bestCost=ggChronNear(fx,fy,tx,ty)*20;
    queue[tail++]=(unsigned char)best;
    static const signed char dx[4]={1,0,-1,0},dy[4]={0,1,0,-1};
    while(head<tail){
        int p=queue[head++],lx=p%W,ly=p/W;
        int x=fx+lx-R,y=fy+ly-R;
        int cost=ggChronNear(x,y,tx,ty)*20+dist[p];
        if(dist[p]&&cost<bestCost){bestCost=cost;best=p;}
        for(int k=0;k<4;k++){
            int nx=lx+dx[k],ny=ly+dy[k];
            if((unsigned)nx>=W||(unsigned)ny>=W)continue;
            int q=ny*W+nx,wx=fx+nx-R,wy=fy+ny-R;
            if(dist[q]!=255||!ggChronLand(wx,wy))continue;
            if(ggExpGround(world[wy][wx])-
               ggExpGround(world[y][x])>19)continue;
            dist[q]=dist[p]+1;
            first[q]=dist[p]?first[p]:(unsigned char)q;
            queue[tail++]=(unsigned char)q;
        }
    }
    if(best==R*W+R||first[best]==255)return 0;
    int p=first[best];
    *outX=fx+p%W-R;*outY=fy+p/W-R;
    return *outX!=fx||*outY!=fy;
}
static int ggChronEnemyCanHit(const GgChronFoe *f,int x,int y){
    int tx=x>>8,ty=y>>8;
    if(ggChronNear(f->x,f->y,tx,ty)>2)return 0;
    int fx=ggChronFoeX(f),fy=ggChronFoeY(f);
    int dx=x-fx,dy=y-fy;
    if(dx*dx+dy*dy>512*512||!ggExpLineClear(f->x,f->y,tx,ty))return 0;
    // A centered diagonal ray can pass exactly between two solid tiles.
    if(tx!=f->x&&ty!=f->y&&bld[f->y][tx]&&bld[ty][f->x])return 0;
    int fz=ggMeshGroundAtQ8(fx,fy)+GG_WORLD_EYE_HEIGHT;
    int z=ggMeshGroundAtQ8(x,y)+GG_WORLD_EYE_HEIGHT;
    return abs(z-fz)<=8&&ggChronCombatLine(fx,fy,fz,x,y,z);
}
static void ggChronEnemiesStep(void){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    for(int i=0;i<ggChron.foeN;i++){
        GgChronFoe *f=&ggChron.foe[i];
        if(f->hp<=0){
            if(ggChron.foeWindup[i]){
                ggChron.foeWindup[i]=0;
                ggChron.foeGuard[i]=-1;
            }
            continue;
        }
        if(ggChron.foeAlert[i])ggChron.foeAlert[i]--;
        if(f->wait>0){f->wait--;continue;}
        if(ggChron.foeWindup[i]){
            if(--ggChron.foeWindup[i])continue;
            int guard=ggChron.foeGuard[i],damage=4+ggChron.act;
            ggChron.foeGuard[i]=-1;
            f->wait=(signed char)(33+(i&3)*5);
            ggChron.dirty=1;
            // A committed attack can miss if its target moves or takes cover.
            if(guard>=0){
                int x,y;
                if(ggReignGuardPosition(guard,&x,&y)&&
                   ggChronEnemyCanHit(f,x*256+128,y*256+128))
                    ggReignGuardDamage(guard,damage);
            }else if(ggChronEnemyCanHit(f,ggExplore.px,ggExplore.py))
                ggChronIncomingHit(i,damage);
            continue;
        }
        int distance=ggChronNear(f->x,f->y,px,py);
        int guardX=px,guardY=py;
        int guard=ggReignNearestGuard(f->x,f->y,&guardX,&guardY);
        int toGuard=guard<0?999:ggChronNear(f->x,f->y,guardX,guardY);
        // A closer guard can screen the commander when the attack begins.
        int atGuard=guard>=0&&toGuard<distance&&
            ggChronEnemyCanHit(f,guardX*256+128,guardY*256+128);
        if(atGuard||ggChronEnemyCanHit(f,ggExplore.px,ggExplore.py)){
            ggChron.foeGuard[i]=(signed char)(atGuard?guard:-1);
            ggChron.foeWindup[i]=4; // 20 game frames before impact
            ggChron.dirty=1;
        }else if(distance<8||toGuard<8||(distance<16&&ggChron.foeAlert[i])){
            int tx=px,ty=py,nx=f->x,ny=f->y;
            if(toGuard<distance){tx=guardX;ty=guardY;}
            if(ggChronEnemyRoute(f->x,f->y,tx,ty,&nx,&ny)){
                f->x=(short)nx;f->y=(short)ny;ggChron.dirty=1;
            }
            f->wait=(signed char)(21+(i&3)*4);
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
    if(ggChron.stamina<8||ggChron.charge||ggChron.guardTicks||ggChron.swing||
       (raw&GG_3DS_ZL))moving&=~KEY_B;
    int oldX=ggExplore.px,oldY=ggExplore.py;
    ggExpAdvance(moving,0,0,0);
    if((oldX>>8)!=(ggExplore.px>>8)||(oldY>>8)!=(ggExplore.py>>8)){
        for(int i=0;i<ggChron.foeN;i++){
            const GgChronFoe*f=&ggChron.foe[i];
            if(f->hp>0&&f->x==(ggExplore.px>>8)&&f->y==(ggExplore.py>>8)){
                ggExplore.px=oldX;ggExplore.py=oldY;ggExplore.stepPhase=0;
                break;
            }
        }
    }
    if((moving&KEY_B)&&(moving&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT))){
        ggChron.stamina=ggExpClamp(ggChron.stamina-2,0,100);
    }else if((ggChron.frame&1)==0&&!ggChron.charge&&!ggChron.guardTicks&&
             !ggChron.swing&&!(raw&GG_3DS_ZL))
        ggChron.stamina=ggExpClamp(ggChron.stamina+1,0,100);
    if(ggChron.briefTime==0){
        ggChronCombatStep(raw,down);
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
               (!ggChronEnemiesAt(p)||ggSagaParley(p))){
                ggChron.post++;
                if(ggChron.post<GG_CHRON_POSTS)
                    ggChronNavBuild(ggChron.post);
                ggChron.rations=ggExpClamp(ggChron.rations+1,0,9);
                ggChron.morale=ggExpClamp(ggChron.morale+12,0,100);
                ggChron.hp=ggExpClamp(ggChron.hp+12,0,100);
                ggChron.score+=100;
                ggChron.briefTime=120;
                ggChronCombatStow();
                memset(ggChron.shots,0,sizeof ggChron.shots);
                memset(ggChron.foeWindup,0,sizeof ggChron.foeWindup);
                ggChron.dirty=1;
                if(ggChron.post==GG_CHRON_POSTS){
                    // A chapter ends with a decision, not an automatic
                    // victory; saving that decision unlocks the next act.
                    ggSagaStartChoice();
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
        ggChronCombatStow();
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
#include "chronicle_combat_art3d.h"

static int ggChronProjectFoe(const GgChronFoe*f,int *sx,int *foot,int *height,int *width){
    if(!ggExpMeshActive){
        int size;
        if(!ggChronProject(f->x,f->y,sx,foot,&size))return 0;
        *height=ggExpClamp(size/4,1,9)*8;
        *width=*height*5/8;
        return 1;
    }
    GgMeshContext*m=&ggMeshContext;
    int x=ggChronFoeX(f),y=ggChronFoeY(f),z=ggMeshGroundAtQ8(x,y);
    GgMeshCameraVertex a=ggMeshCameraPoint(m,ggMeshV(x,y,z));
    if(a.f<m->near||a.f>15*256||abs(a.s)*5>a.f*6)return 0;
    GgMeshProjected base=ggMeshProject(m,a);
    a.z+=GG_CHRON_BODY;
    GgMeshProjected head=ggMeshProject(m,a);
    *sx=base.x*m->scale;*foot=base.y*m->scale;
    *height=(base.y-head.y)*m->scale;
    *width=*height*5/8;
    return a.f;
}
static void ggChronDrawShots(u8*out){
    if(!ggExpMeshActive)return;
    GgMeshContext*m=&ggMeshContext;
    for(int i=0;i<GG_CHRON_SHOTS;i++){
        const GgChronShot*p=&ggChron.shots[i];
        if(!p->life)continue;
        GgMeshCameraVertex c=ggMeshCameraPoint(m,ggMeshV(p->x,p->y,p->z>>8));
        if(c.f<m->near)continue;
        GgMeshProjected q=ggMeshProject(m,c);
        int x=q.x*m->scale,y=q.y*m->scale;
        ggExpFilmDrawDepth=c.f;
        if(p->style==GG_CHRON_BOW){
            ggExpRect(out,x-2,y-1,5,2,218,179,113);
            ggExpRect(out,x-1,y-2,2,3,249,238,200);
        }else{
            ggExpRect(out,x-3,y-2,7,5,207,103,47);
            ggExpRect(out,x-1,y-3,3,7,241,157,62);
            ggExpRect(out,x-1,y-1,3,3,255,232,131);
        }
    }
    ggExpFilmDrawDepth=0;
}
static void ggChronDrawCombat(u8*out){
    if(ggChron.lost||ggChron.won||ggChron.briefTime)return;
    int style=ggChron.style,phase=0,progress=0;
    if(ggChron.attackFlash){phase=2;progress=(5-ggChron.attackFlash)*51;}
    else if(ggChron.swing){phase=2;progress=(6-ggChron.swing)*42;}
    else if(ggChron.charge){
        phase=1;progress=ggExpClamp(ggChron.charge*255/36,0,255);
    }else if(ggChron.guardTicks){
        phase=3;progress=ggExpClamp(ggChron.guardTicks*38,0,255);
    }else if(ggChron.attackWait){
        phase=4;progress=ggExpClamp(255-ggChron.attackWait*8,0,255);
    }
    ggChronCombatWeaponDraw(out,style,phase,progress,ggChron.frame);
    int hit=ggChron.combatNoticeT>33&&ggChron.combatNotice==GG_COMBAT_HIT;
    ggChronCombatReticle(out,style,hit,ggChron.charge*255/36);
    static const char *const response[]={
        "","HIT","STOPPED BY COVER","OUT OF REACH","HOLD A LONGER",
        "LOW STAMINA","LOW FOCUS","BLOCKED","PARRY","WARDED"
    };
    if(ggChron.combatNoticeT>0&&ggChron.combatNotice>GG_COMBAT_NONE){
        const char *line=response[ggChron.combatNotice];
        ggExpRect(out,7,178,(int)strlen(line)*6+8,13,16,25,32);
        ggExpText(out,11,181,line,248,219,152);
    }
}
static void ggChronDraw(u8 *out){
    if(!ggChron.active)return;
    // Draw the bounded encounter from far to near.
    int order[GG_CHRON_FOES],depth[GG_CHRON_FOES],count=0;
    for(int i=0;i<ggChron.foeN;i++){
        const GgChronFoe*f=&ggChron.foe[i];
        if(f->hp<=0)continue;
        int aim=ggExplore.angle&255;
        int d=(ggChronFoeX(f)-ggExplore.px)*ggExpCos[aim]+
              (ggChronFoeY(f)-ggExplore.py)*ggExpSin[aim];
        int at=count++;
        while(at>0&&depth[at-1]<d){
            order[at]=order[at-1];depth[at]=depth[at-1];at--;
        }
        depth[at]=d;order[at]=i;
    }
    for(int n=0;n<count;n++){
        int i=order[n];
        const GgChronFoe *f=&ggChron.foe[i];
        int x,y,h,w,forward=ggChronProjectFoe(f,&x,&y,&h,&w);
        if(!forward||(ggExpMeshActive&&forward>11*256))continue;
        int bob=ggChron.foeWindup[i]>0;
        if(ggExpMeshActive){
            static const u8 coat[3][3]={{22,7,9},{13,6,9},{28,20,11}};
            int wx=ggChronFoeX(f),wy=ggChronFoeY(f),ground=ggMeshGroundAtQ8(wx,wy);
            GgMeshContext*m=&ggMeshContext;
            int dx=ggExplore.px-wx,dy=ggExplore.py-wy;
            int heading=abs(dx)>abs(dy)*2?(dx>0?0:128):
                        abs(dy)>abs(dx)*2?(dy>0?64:192):
                        dy>0?(dx>0?32:96):(dx>0?224:160);
            GgMeshCameraVertex view=ggMeshCameraPoint(m,ggMeshV(wx,wy,ground));
            ggMeshPersonDraw(m,wx,wy,ground,view.f,view.s,
                0xc5000000u+(unsigned)(ggChron.act*32+i),i&7,heading,
                ggChron.frame,bob,coat);
            ggExpFilmDrawDepth=forward;
        }else{
            int shape=(ggChron.act==3&&(i&1))?16:(i&1)?26:5;
            static const u8 enemyInk[4][3]={
                {91,47,55},{174,120,68},{45,49,64},{244,204,149}
            };
            for(int py=0;py<8;py++){
                const char *row=shpRow(shape,bob,py);
                for(int px=0;px<8;px++){
                    int color=row[px]=='b'?0:row[px]=='a'?1:
                              row[px]=='d'?2:row[px]=='e'?3:-1;
                    if(color<0)continue;
                    int left=x-w/2+px*w/8,top=y-h+py*h/8;
                    int right=x-w/2+(px+1)*w/8,bottom=y-h+(py+1)*h/8;
                    ggExpRect(out,left,top,right-left,bottom-top,
                              enemyInk[color][0],enemyInk[color][1],enemyInk[color][2]);
                }
            }
        }
        int bar=ggExpClamp(w,10,46),barY=y-h-7;
        if(ggExpFilmPixelHidden(x,y-h/2)){ggExpFilmDrawDepth=0;continue;}
        int blade=x+w/3,grip=y-h/3;
        if(bob)grip-=h/4;
        int bladeHeight=ggExpClamp(h/3,4,29);
        ggExpRect(out,blade,grip-bladeHeight,2,bladeHeight,183,200,207);
        ggExpRect(out,blade-2,grip,6,2,147,121,72);
        ggExpRect(out,x-w/3,y-h/2,ggExpClamp(w/5,3,10),
                  ggExpClamp(h/5,3,14),120,51,62);
        ggExpRect(out,x-bar/2,barY,bar,3,34,29,35);
        // Spawn places the chapter's single captain first at the last
        // post. The captain is distinctly tougher; its bar must show
        // actual damage from the first hit, not only after 65 HP vanish.
        int captain=ggChron.act==GG_CHRON_ACTS-1&&
                    f->post==GG_CHRON_POSTS-1&&
                    i>=0&&i<ggChron.foeN&&
                    (i==0||ggChron.foe[i-1].post!=f->post);
        int maxHp=captain?105:20+ggChron.act*6+f->post*3;
        if(maxHp<1)maxHp=40;
        ggExpRect(out,x-bar/2,barY,
                  bar*ggExpClamp(f->hp,0,maxHp)/maxHp,3,196,76,64);
        if(bob)ggExpRect(out,x-2,barY-6,5,4,247,175,68);
        ggExpFilmDrawDepth=0;
    }
    if(!ggChron.won){
        int p=ggChron.post;
        if(p<GG_CHRON_POSTS){
            int x,y,h,w;
            GgChronFoe banner={.x=ggChron.px[p],.y=ggChron.py[p]};
            int forward=ggChronProjectFoe(&banner,&x,&y,&h,&w);
            if(forward){
                if(ggExpMeshActive)ggExpFilmDrawDepth=forward;
                h=ggExpClamp(h*3/2,8,105);
                int cloth=ggExpClamp(w/2,4,24),drop=ggExpClamp(h/4,3,12);
                ggExpRect(out,x,y-h,2,h,225,201,126);
                ggExpRect(out,x+2,y-h,cloth,drop,168,116,49);
                ggExpRect(out,x+3,y-h+1,cloth*2/3,1,244,218,142);
                ggExpFilmDrawDepth=0;
            }
        }
    }
    ggChronDrawShots(out);
    ggChronDrawCombat(out);
    // Replace the free-build UI completely. A commander must read the
    // tactical situation, not a block inventory.
    ggExpRect(out,0,0,400,37,13,23,34);
    ggExpRect(out,0,35,400,2,177,128,71);
    char line[100];
    snprintf(line,sizeof line,"UNWRITTEN REIGN %d/4  %.24s",
             ggChron.act+1,ggChronActName[ggChron.act]);
    ggExpText(out,9,5,line,248,220,165);
    const char *styleName=ggChron.style==GG_CHRON_BOW?"BOW":
                          ggChron.style==GG_CHRON_EMBER?"EMBER":"BLADE";
    snprintf(line,sizeof line,"HP %d  STA %d  FOCUS %d  FOOD %d  RATIONS %d  %s",
             ggChron.hp,ggChron.stamina,ggChron.focus,ggChron.food,ggChron.rations,styleName);
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
        static const char *const tasks[GG_CHRON_ACTS][GG_CHRON_POSTS]={
            {"RESCUE THE CARAVAN","REOPEN THE FORD","LIGHT THE WATCH FIRE"},
            {"HEAR THE GUARDS","FIND THE BROKEN ORDER","REBUILD THE OATH"},
            {"SAVE A WITNESS","SECURE THE ARCHIVE","READ THE NAMES"},
            {"DEFEND THE HOMES","BREAK THE SIEGE","FACE THE LAST CAPTAIN"}
        };
        int nextX,nextY,routeSteps=0;
        int routed=ggChronNavNext(ggExplore.px>>8,ggExplore.py>>8,
                                  &nextX,&nextY,&routeSteps);
        if(distance<=2&&ggChronEnemiesAt(p)==0){
            snprintf(line,sizeof line,"%s - PRESS X TO SECURE",
                     tasks[ggChron.act][p]);
        }else if(distance<=2){
            snprintf(line,sizeof line,"%s - %d GUARDS: FIGHT OR NEGOTIATE X",
                     tasks[ggChron.act][p],ggChronEnemiesAt(p));
        }else if(routed&&routeSteps>0){
            int stepX=nextX-(ggExplore.px>>8);
            int stepY=nextY-(ggExplore.py>>8);
            const char *road=stepX>0?"EAST":stepX<0?"WEST":
                              stepY>0?"SOUTH":"NORTH";
            snprintf(line,sizeof line,"%s  PATH %s %d STEPS  GUARDS %d",
                     tasks[ggChron.act][p],road,routeSteps,
                     ggChronEnemiesAt(p));
        }else snprintf(line,sizeof line,"%s  SEEK %s %d TILES  GUARDS %d",
                       tasks[ggChron.act][p],dir,distance,
                       ggChronEnemiesAt(p));
        ggExpText(out,9,203,line,247,216,155);
    }
    const char *controls=ggChron.style==GG_CHRON_BOW?"HOLD A DRAW  ZL AIM":
                         ggChron.style==GG_CHRON_EMBER?"HOLD A CAST  ZL WARD":"A SWING  HOLD HEAVY  ZL GUARD";
    ggExpText(out,9,217,controls,226,216,185);
    ggExpText(out,9,229,"ZR GEAR  X SECURE  Y RATION  B RUN  SELECT COUNCIL  START 2D",201,214,224);
    if(ggChron.briefTime>0){
        ggExpRect(out,22,69,356,65,17,31,44);
        ggExpRect(out,22,69,356,2,187,139,79);
        ggExpText(out,38,82,ggChron.won?"THE ROAD BELONGS TO THE LIVING.":
            ggChron.post==0?ggChronPrologue[ggChron.act]:
            ggChron.post==1?"THE FIRST STANDARD RISES. KEEP MOVING.":
            ggChron.post==2?"THE WATCH HOLDS. THE LAST BANNER WAITS.":
                             "THE THREE STANDARDS STAND TOGETHER.",
            243,214,166);
        ggExpText(out,38,105,"ZR CHANGES GEAR. ZL GUARDS, AIMS, OR WARDS.",
                  180,207,225);
    }
    ggChron.dirty=0;
}
#endif
