// The Unwritten Reign: distinctive settlement decorations in live 3D.
// Every object here belongs to an EXISTING bld[][] tile. No Minecraft block
// inventory, no fabricated scenery, no texture uploads, no heap allocation.
// Projected painter's decals belong to the fallback renderer. The native
// renderer already models the same buildings with physical depth.
#ifndef GG_REIGN_SCENERY_H
#define GG_REIGN_SCENERY_H
enum { GG_REIGN_SCENE_RADIUS=8, GG_REIGN_SCENE_CAP=16 };

static void ggReignSceneHouse(u8 *out,int x,int y,int u,int hall){
    // Warm timber door, lintel, glass windows, pitched roof and eaves.
    int w=(hall?12:9)*u;
    int h=(hall?11:8)*u;
    ggExpRect(out,x-w/2,y-h,w,h,148,120,92);
    for(int i=0;i<=6;i++){
        int rw=w+2*u-i*u;
        ggExpRect(out,x-rw/2,y-h-(7-i)*u,rw,u,87+i*4,66+i*3,63);
    }
    ggExpRect(out,x-2*u,y-6*u,4*u,6*u,69,53,55);
    ggExpRect(out,x+u,y-3*u,u,u,204,181,113);
    ggExpRect(out,x-w/2+u,y-h+3*u,2*u,2*u,225,199,129);
    ggExpRect(out,x-w/2+u,y-h+4*u,2*u,u,69,111,131);
    ggExpRect(out,x+w/2-3*u,y-h+3*u,2*u,2*u,225,199,129);
    ggExpRect(out,x+w/2-3*u,y-h+4*u,2*u,u,69,111,131);
    if(hall){
        ggExpRect(out,x+w/2-2*u,y-h-6*u,u,8*u,76,54,61);
        ggExpRect(out,x+w/2-u,y-h-6*u,4*u,3*u,164,89,58);
        ggExpRect(out,x+w/2,y-h-5*u,2*u,u,236,193,112);
    }
}
static void ggReignSceneFarm(u8 *out,int x,int y,int u,int f){
    // Real crops move a few pixels in the wind, not a flat green cube.
    ggExpRect(out,x-9*u,y-2*u,18*u,3*u,73,62,43);
    for(int k=0;k<7;k++){
        int ax=x+(k-3)*2*u,bob=((f/16+k)&1)?-u:0;
        ggExpRect(out,ax,y-5*u+bob,u,5*u,81,119,62);
        ggExpRect(out,ax-2*u,y-6*u+bob,3*u,2*u,145,157,68);
        ggExpRect(out,ax+u,y-4*u+bob,2*u,2*u,161,169,76);
        if((k&1)==0)ggExpRect(out,ax,y-u,u*2,u,173,110,59);
    }
}
static void ggReignSceneTower(u8 *out,int x,int y,int u,int f){
    ggExpRect(out,x-4*u,y-15*u,8*u,15*u,136,139,136);
    ggExpRect(out,x-5*u,y-17*u,10*u,3*u,92,101,106);
    for(int k=0;k<5;k++)
        ggExpRect(out,x+(k*2-5)*u,y-19*u,2*u,3*u,133,142,146);
    ggExpRect(out,x-1*u,y-9*u,2*u,4*u,55,63,76);
    ggExpRect(out,x+4*u,y-23*u,u,8*u,104,88,80);
    ggExpRect(out,x+5*u,y-23*u,5*u,3*u,149,54+(f/16%3)*4,60);
}
static void ggReignSceneTemple(u8 *out,int x,int y,int u,int f){
    ggExpRect(out,x-8*u,y-10*u,16*u,10*u,173,166,150);
    for(int i=0;i<5;i++)
        ggExpRect(out,x-(9-i)*u,y-(15-i)*u,(18-2*i)*u,u,
                  150+i*3,148+i*2,143+i*2);
    ggExpRect(out,x-3*u,y-8*u,6*u,8*u,86,75,86);
    ggExpRect(out,x-2*u,y-7*u,4*u,6*u,112,86,103);
    ggExpRect(out,x-2*u,y-14*u,4*u,3*u,207,174,92);
    ggExpRect(out,x-u,y-13*u,2*u,u,88,99,134);
    if((f/25)&1)ggExpRect(out,x-u,y-13*u,2*u,u,121,166,186);
}
static void ggReignSceneDock(u8 *out,int x,int y,int u){
    ggExpRect(out,x-9*u,y-2*u,19*u,3*u,118,87,66);
    for(int k=0;k<5;k++)
        ggExpRect(out,x+(k*4-8)*u,y-u,u,3*u,82,64,55);
    ggExpRect(out,x+u,y-15*u,u,16*u,131,116,93);
    for(int k=0;k<7;k++)ggExpRect(out,x+2*u,y-(14-k)*u,
                                 (7-k)*u,u,194,187,155);
}
static void ggReignSceneRuin(u8 *out,int x,int y,int u){
    ggExpRect(out,x-7*u,y-5*u,4*u,5*u,114,110,108);
    ggExpRect(out,x+2*u,y-9*u,4*u,9*u,129,123,115);
    ggExpRect(out,x-7*u,y-10*u,12*u,2*u,115,109,104);
    ggExpRect(out,x-2*u,y-7*u,4*u,2*u,39,42,50);
    ggExpRect(out,x-4*u,y-u,3*u,u,138,123,112);
}
static void ggReignSceneLibrary(u8 *out,int x,int y,int u){
    ggReignSceneHouse(out,x,y,u,0);
    ggExpRect(out,x+4*u,y-10*u,5*u,6*u,79,69,72);
    ggExpRect(out,x+5*u,y-9*u,2*u,4*u,215,195,137);
    ggExpRect(out,x+7*u,y-9*u,u,4*u,117,166,170);
}
static void ggReignSceneStatue(u8 *out,int x,int y,int u){
    ggExpRect(out,x-5*u,y-2*u,10*u,2*u,127,130,127);
    ggExpRect(out,x-3*u,y-11*u,6*u,9*u,137,140,133);
    ggExpRect(out,x-3*u,y-13*u,6*u,5*u,171,157,126);
    ggExpRect(out,x-4*u,y-5*u,8*u,2*u,168,156,128);
    ggExpRect(out,x-2*u,y-17*u,4*u,4*u,157,158,153);
    ggExpRect(out,x-3*u,y-19*u,6*u,u,223,178,90);
}
static void ggReignSceneAt(u8*out,int build,int x,int y,int u,int f){
    switch(build){
    case B_HUT:case B_HOUSE:ggReignSceneHouse(out,x,y,u,0);break;
    case B_HALL:ggReignSceneHouse(out,x,y,u,1);break;
    case B_FARM:ggReignSceneFarm(out,x,y,u,f);break;
    case B_TOWER:ggReignSceneTower(out,x,y,u,f);break;
    case B_TEMPLE:ggReignSceneTemple(out,x,y,u,f);break;
    case B_DOCK:ggReignSceneDock(out,x,y,u);break;
    case B_LIBRARY:ggReignSceneLibrary(out,x,y,u);break;
    case B_STATUE:ggReignSceneStatue(out,x,y,u);break;
    case B_RUIN:ggReignSceneRuin(out,x,y,u);break;
    default:break;
    }
}
static int ggReignSceneDrawn;
static void ggReignSceneDraw(u8*out){
    ggReignSceneDrawn=0;
    if(!ggChron.active||ggExpMeshActive)return;
    int px=ggExplore.px>>8,py=ggExplore.py>>8;
    // Choose the closest *visible* real buildings first. The old
    // far-first cap spent its 16 decals on distant huts, omitted nearby
    // landmarks and could waste occlusion tests on invisible districts.
    // This stack list is < 256 bytes; no heap, texture or framebuffer.
    struct { short sx,foot; unsigned char build,u; } visible[GG_REIGN_SCENE_CAP];
    for(int d=1;d<=GG_REIGN_SCENE_RADIUS;d++){
        for(int dy=-d;dy<=d;dy++){
            int y=py+dy;
            if((unsigned)y>=WH)continue;
            for(int dx=-d;dx<=d;dx++){
                if(abs(dx)+abs(dy)!=d)continue;
                int x=px+dx;
                if((unsigned)x>=WW||!bld[y][x])continue;
                int sx,foot,size;
                if(!ggChronProject(x,y,&sx,&foot,&size))continue;
                int i=ggReignSceneDrawn++;
                visible[i].sx=(short)sx;visible[i].foot=(short)foot;
                visible[i].build=(unsigned char)bld[y][x];
                visible[i].u=(unsigned char)ggExpClamp(24/(d+3),1,5);
                if(ggReignSceneDrawn>=GG_REIGN_SCENE_CAP)goto render;
            }
        }
    }
render:
    // Painter's order is still far-to-near, despite near-first selection.
    for(int i=ggReignSceneDrawn-1;i>=0;i--)
        ggReignSceneAt(out,visible[i].build,visible[i].sx,visible[i].foot,
                       visible[i].u,ggChron.frame);
}
#endif
