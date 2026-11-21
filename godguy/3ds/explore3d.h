// GodGuy's first playable, first-person 3DS world.
// Software voxel-column terrain at 200x120, scaled exactly 2x to the
// existing 400x240 top RGB buffer; no new framebuffer, GPU allocations,
// 3D model assets, or duplicate world. Bottom LCD retains the 2D map.
// Uses the SAME world[][], bld[][], occ[][] and setCell() as god-game play.
//
// Real-world voxel terrain, collision, first-person walking, live tile
// editing, crosshair, world location and safe return to the 2D map.
// It is a light sandbox foundation, not a claimed Minecraft block engine.
#ifndef GG_EXPLORE3D_H
#define GG_EXPLORE3D_H
enum { GG_EXP_W=200, GG_EXP_H=120, GG_EXP_MAX_DEPTH=42,
       GG_EXP_EXIT=1, GG_EXP_SAVE=2, GG_EXP_CHANGE=4 };
enum { GG_EXP_MATERIALS=8 };
static const u8 ggExpMaterials[GG_EXP_MATERIALS]={
    T_GRASS,T_DIRT,T_SAND,T_FOREST,T_HILL,T_MOUNT,T_WATER,T_SNOW
};
static const char *const ggExpMaterialNames[GG_EXP_MATERIALS]={
    "GRASS","DIRT","SAND","FOREST","HILL","MOUNTAIN","WATER","SNOW"
};
typedef struct {
    int active,wasPaused;
    // Persistent selected-creature identity. The population list can
    // compact on save; an array slot is NOT a durable identity.
    u32 actorId;
    int actorIndex;
    char actorName[40];
    int px,py;                   // tile coordinates in Q8 (1 tile = 256)
    int angle;                   // 0..255; one revolution is 256
    int material;
    int frame,rendered,stepPhase;
    int lastHeld,lastTargetX,lastTargetY;
    int edits,feedback,feedbackT;
} GgExplore3D;
static GgExplore3D ggExplore;
static short ggExpSin[256],ggExpCos[256];
static int ggExpTrigReady;

static int ggExpClamp(int n,int low,int high){
    return n<low?low:n>high?high:n;
}
static void ggExpTrigInit(void){
    if(ggExpTrigReady)return;
    for(int i=0;i<256;i++){
        double a=(6.2831853071795864769*i)/256.0;
        ggExpSin[i]=(short)(sin(a)*4096.0);
        ggExpCos[i]=(short)(cos(a)*4096.0);
    }
    ggExpTrigReady=1;
}
static int ggExpGround(int t){
    static const u8 heights[T_N]={
        0,1,5,10,16,25,48,31,6,8,7,18,8
    };
    return (unsigned)t<T_N?heights[t]:8;
}
static int ggExpBuilding(int b){
    switch(b){
    case B_HUT:return 18;
    case B_HOUSE:return 25;
    case B_HALL:return 37;
    case B_TOWER:return 55;
    case B_FARM:return 6;
    case B_RUIN:return 12;
    case B_DOCK:return 7;
    case B_TEMPLE:return 34;
    case B_STATUE:return 28;
    case B_LIBRARY:return 31;
    default:return 0;
    }
}
static int ggExpWalkable(int x,int y,int fromHeight,int climb){
    if(x<1||y<1||x>=WW-1||y>=WH-1)return 0;
    int t=world[y][x];
    if(t<=T_WATER||t==T_LAVA||bld[y][x])return 0;
    return ggExpGround(t)-fromHeight<=(climb?30:19);
}
static void ggExpBegin(int x,int y,int priorPause){
    ggExpTrigInit();
    x=ggExpClamp(x,1,WW-2);
    y=ggExpClamp(y,1,WH-2);
    if(!ggExpWalkable(x,y,60,1)){
        int found=0;
        for(int r=1;r<=14&&!found;r++)
            for(int dy=-r;dy<=r&&!found;dy++)
                for(int dx=-r;dx<=r&&!found;dx++){
                    int nx=x+dx,ny=y+dy;
                    if(ggExpWalkable(nx,ny,60,1)){
                        x=nx;y=ny;found=1;
                    }
                }
    }
    ggExplore.active=1;
    ggExplore.wasPaused=priorPause;
    ggExplore.actorId=0;
    ggExplore.actorIndex=-1;
    ggExplore.actorName[0]=0;
    ggExplore.px=x*256+128;
    ggExplore.py=y*256+128;
    ggExplore.angle=64; // begin looking into the map, not a random shot
    ggExplore.frame=0;
    ggExplore.rendered=0;
    ggExplore.stepPhase=0;
    ggExplore.lastHeld=0;
    ggExplore.feedback=0;
    ggExplore.feedbackT=0;
    ggExplore.lastTargetX=x;
    ggExplore.lastTargetY=y;
}
static void ggExpTarget(int *tx,int *ty){
    int dir=ggExplore.angle&255;
    // Aim a short three-tile construction ray from the central crosshair.
    *tx=(ggExplore.px+ggExpCos[dir]*768/4096)>>8;
    *ty=(ggExplore.py+ggExpSin[dir]*768/4096)>>8;
}
static int ggExpEdit(int tx,int ty,int material,int removing){
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    if(tx<1||ty<1||tx>=WW-1||ty>=WH-1||
       (tx==px&&ty==py)||bld[ty][tx]||occ[ty][tx])return 0;
    int t=removing?T_GRASS:ggExpMaterials[material%GG_EXP_MATERIALS];
    if(world[ty][tx]==t)return 0;
    // Never secretly raze a civilization or crush a living creature.
    // setCell keeps the 2D map, biome data and terrain transitions in sync.
    setCell(tx,ty,(u8)t);
    ggExplore.edits++;
    return 1;
}
static int ggExpStep(int held,int touchX,int touchY){
    if(!ggExplore.active)return 0;
    int down=held&~ggExplore.lastHeld;
    ggExplore.lastHeld=held;
    ggExplore.frame++;
    if(ggExplore.feedbackT>0)ggExplore.feedbackT--;
    if(down&KEY_START)return GG_EXP_EXIT;
    if(down&KEY_SELECT)return GG_EXP_SAVE;
    if(down&KEY_X){
        ggExplore.material=(ggExplore.material+1)%GG_EXP_MATERIALS;
        ggExplore.feedback=1;ggExplore.feedbackT=90;
    }
    if(held&KEY_L)ggExplore.angle=(ggExplore.angle-2)&255;
    if(held&KEY_R)ggExplore.angle=(ggExplore.angle+2)&255;
    int forward=!!(held&KEY_UP)-!!(held&KEY_DOWN);
    int strafe=!!(held&KEY_RIGHT)-!!(held&KEY_LEFT);
    if(forward||strafe){
        int a=ggExplore.angle;
        int speed=held&KEY_B?35:20;
        if(forward&&strafe)speed=speed*3/4;
        int dx=(ggExpCos[a]*forward-ggExpSin[a]*strafe)*speed/4096;
        int dy=(ggExpSin[a]*forward+ggExpCos[a]*strafe)*speed/4096;
        int from=ggExpGround(world[ggExplore.py>>8][ggExplore.px>>8]);
        int nx=ggExplore.px+dx,ny=ggExplore.py+dy;
        if(ggExpWalkable(nx>>8,ggExplore.py>>8,from,held&KEY_B))
            ggExplore.px=nx;
        if(ggExpWalkable(ggExplore.px>>8,ny>>8,from,held&KEY_B))
            ggExplore.py=ny;
        ggExplore.stepPhase++;
    }else ggExplore.stepPhase=0;
    // One tap on the live bottom map teleports to a WALKABLE world tile.
    // This is a fast way to revisit the user's own towns and coastlines.
    if((down&KEY_TOUCH)&&touchX>=0&&touchX<256&&
       touchY>=0&&touchY<160){
        int nx=(camX+touchX)>>3,ny=(camY+touchY)>>3;
        if(!ggExplore.actorId&&ggExpWalkable(nx,ny,60,1)){
            ggExplore.px=nx*256+128;
            ggExplore.py=ny*256+128;
        }
    }
    ggExpTarget(&ggExplore.lastTargetX,&ggExplore.lastTargetY);
    if(down&(KEY_A|KEY_Y)){
        int changed=ggExpEdit(ggExplore.lastTargetX,
                              ggExplore.lastTargetY,
                              ggExplore.material,!!(down&KEY_Y));
        ggExplore.feedback=changed?2:3;
        ggExplore.feedbackT=90;
        if(changed)return GG_EXP_CHANGE;
    }
    return 0;
}
// Write a 2x2 low-res pixel into the existing top LCD's 400x240 RGB.
// A single function limits memory traffic on Old 3DS; no temp surface.
static void ggExpDot(u8 *out,int x,int y,int r,int g,int b){
    if((unsigned)x>=GG_EXP_W||(unsigned)y>=GG_EXP_H)return;
    u8 *a=out+((y*2)*400+x*2)*3;
    u8 *z=a+400*3;
    for(int k=0;k<2;k++){
        a[k*3+0]=z[k*3+0]=(u8)r;
        a[k*3+1]=z[k*3+1]=(u8)g;
        a[k*3+2]=z[k*3+2]=(u8)b;
    }
}
static void ggExpRect(u8*out,int x,int y,int w,int h,int r,int g,int b){
    int x1=ggExpClamp(x,0,400),y1=ggExpClamp(y,0,240);
    int x2=ggExpClamp(x+w,0,400),y2=ggExpClamp(y+h,0,240);
    for(int iy=y1;iy<y2;iy++)
        for(int ix=x1;ix<x2;ix++){
            u8*p=out+(iy*400+ix)*3;
            p[0]=(u8)r;p[1]=(u8)g;p[2]=(u8)b;
        }
}
static void ggExpText(u8*out,int x,int y,const char*str,int r,int g,int b){
    for(;*str;str++,x+=6){
        int ch=(unsigned char)*str;
        if(ch>='a'&&ch<='z')ch-=32;
        if(ch<32||ch>95)continue;
        for(int ry=0;ry<7;ry++){
            int bits=font5x7[ch-32][ry];
            for(int rx=0;rx<5;rx++)if((bits>>(4-rx))&1)
                ggExpRect(out,x+rx,y+ry,1,1,r,g,b);
        }
    }
}
static void ggExpRender(u8 *top){
    int a=ggExplore.angle&255;
    int terrain=world[ggExplore.py>>8][ggExplore.px>>8];
    int eye=ggExpGround(terrain)+13;
    int walkBob=ggExplore.stepPhase?((ggExplore.stepPhase/5)&1):0;
    int horizon=58+walkBob;
    int dirX=ggExpCos[a],dirY=ggExpSin[a];
    int rightX=-dirY,rightY=dirX;
    int fogTick=ggExplore.frame/8;
    // Fast RGB background: fill the first 400px screen row directly,
    // then copy it to the matching 2x row. This eliminates 24,000
    // function calls and 96,000 individual per-pixel bounds checks.
    // It writes only into the already allocated 400x240 top RGB buffer.
    for(int y=0;y<GG_EXP_H;y++){
        int sky=y<horizon;
        int r=sky?28+y/2:67,g=sky?65+y:113,b=sky?112+y:151;
        u8 *line=top+(y*2)*400*3;
        for(int x=0;x<400;x++){
            line[x*3]=(u8)r;
            line[x*3+1]=(u8)g;
            line[x*3+2]=(u8)b;
        }
        memcpy(line+400*3,line,400*3);
    }
    // Sparse moving high-altitude clouds preserve horizon readability.
    for(int k=0;k<7;k++){
        int x=(k*67+fogTick)%260-30,y=8+(k*13)%29;
        int w=18+(k*7)%24;
        for(int cx=x;cx<x+w;cx++)for(int cy=y;cy<y+3;cy++)
            ggExpDot(top,cx,cy,122,160,184);
    }
    // Front-to-back silhouette-buffer voxel ray caster. 200 rays, max
    // 42 terrain samples each, with every cell read directly from the
    // live 3DS game world. No second mesh, z-buffer or GPU allocation.
    for(int x=0;x<GG_EXP_W;x++){
        int skew=(x-GG_EXP_W/2);
        int rayX=dirX+rightX*skew/125;
        int rayY=dirY+rightY*skew/125;
        int highest=GG_EXP_H;
        int previousX=-1,previousY=-1;
        for(int d=1;d<=GG_EXP_MAX_DEPTH;d++){
            int tx=(ggExplore.px+rayX*d*256/4096)>>8;
            int ty=(ggExplore.py+rayY*d*256/4096)>>8;
            if((unsigned)tx>=WW||(unsigned)ty>=WH)break;
            if(tx==previousX&&ty==previousY)continue;
            previousX=tx;previousY=ty;
            int type=world[ty][tx],build=bld[ty][tx];
            int h=ggExpGround(type)+ggExpBuilding(build);
            int projected=horizon-(h-eye)*68/(d+1);
            if(projected>=highest)continue;
            int from=ggExpClamp(projected,0,GG_EXP_H);
            int to=ggExpClamp(highest,0,GG_EXP_H);
            int baseR,baseG,baseB;
            if(build){
                // Stone, timber, roofs and towers are recognizable solid
                // blocks, positioned exactly at their simulation tiles.
                int wood=build==B_HOUSE||build==B_HUT||build==B_DOCK;
                baseR=wood?160:139;baseG=wood?112:135;baseB=wood?83:144;
                if(build==B_RUIN){baseR=102;baseG=102;baseB=108;}
            }else{
                baseR=terrCol[type][0]*8;
                baseG=terrCol[type][1]*8;
                baseB=terrCol[type][2]*8;
            }
            int shade=256-d*3;
            if(shade<110)shade=110;
            baseR=baseR*shade/256;
            baseG=baseG*shade/256;
            baseB=baseB*shade/256;
            int bright=build?13:6;
            // Horizontal ledges give the player legible voxel/block faces,
            // rather than a strange fuzzy texture or smooth gradient mesh.
            for(int y=from;y<to;y++){
                int edge=(y-from<2);
                int grain=((tx*17+ty*11+y/6)&3)==0?7:0;
                int r=ggExpClamp(baseR+(edge?bright:0)-grain,0,255);
                int g=ggExpClamp(baseG+(edge?bright:0)-grain,0,255);
                int b=ggExpClamp(baseB+(edge?bright:0)-grain,0,255);
                ggExpDot(top,x,y,r,g,b);
            }
            highest=from;
            if(!highest)break;
        }
    }
    // Crosshair fixed at camera centre. Changes always affect a cell a
    // few tiles ahead, never an invisible object behind the player's feet.
    ggExpRect(top,192,119,16,2,12,21,30);
    ggExpRect(top,199,112,2,16,12,21,30);
    ggExpRect(top,195,119,10,1,247,240,206);
    ggExpRect(top,200,115,1,10,247,240,206);
    char buf[96];
    ggExpRect(top,0,0,400,22,13,25,40);
    if(ggExplore.actorId){
        snprintf(buf,sizeof buf,"POV %-18.18s  %d,%d",
                 ggExplore.actorName,ggExplore.px>>8,ggExplore.py>>8);
    }else{
        snprintf(buf,sizeof buf,"GODGUY FREE WALK   %d,%d  %s",
                 ggExplore.px>>8,ggExplore.py>>8,
                 ggExpMaterialNames[ggExplore.material]);
    }
    ggExpText(top,10,7,buf,243,225,163);
    ggExpRect(top,0,202,400,38,12,22,34);
    ggExpText(top,9,207,ggExplore.actorId?
              "PAD WALK  L/R LOOK  B RUN  X MATERIAL  A BUILD  Y CLEAR":
              "PAD MOVE  L/R TURN  B RUN  X BLOCK  A PLACE  Y CLEAR",
              221,228,221);
    ggExpText(top,9,221,ggExplore.actorId?
              "TAP MAP: CHANGE MIND  SELECT SAVE  START: 2D":
              "TAP MAP WARP   SELECT SAVE   START EXIT",180,202,222);
    if(ggExplore.feedbackT){
        const char *msg=ggExplore.feedback==2?"WORLD TILE CHANGED":
                        ggExplore.feedback==3?"PROTECTED OR OUT OF REACH":
                        "MATERIAL SELECTED";
        ggExpText(top,9,191,msg,255,216,117);
    }
}
#endif
