// SDK-free regression for GodGuy's real 3DS first-person voxel code.
// gcc -O2 -std=c11 -Wall -Wextra -Icompat -Isource pc_explore3d.c -lm -o /tmp/gg-exp && /tmp/gg-exp
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
typedef uint8_t u8;
enum { T_DEEP,T_WATER,T_SAND,T_GRASS,T_FOREST,T_HILL,T_MOUNT,
       T_SNOW,T_LAVA,T_DESERT,T_SWAMP,T_JUNGLE,T_DIRT,T_N };
enum { B_NONE,B_HUT,B_HOUSE,B_HALL,B_FARM,B_TOWER,B_RUIN,
       B_DOCK,B_TEMPLE,B_STATUE,B_LIBRARY,B_BORDER,B_N };
enum { KEY_A=1,KEY_B=2,KEY_SELECT=4,KEY_START=8,
       KEY_RIGHT=16,KEY_LEFT=32,KEY_UP=64,KEY_DOWN=128,
       KEY_R=256,KEY_L=512,KEY_X=1024,KEY_Y=2048,
       KEY_TOUCH=4096 };
#define WW 128
#define WH 128
static u8 world[WH][WW],bld[WH][WW];
static unsigned short occ[WH][WW];
static int camX,camY,editCalls;
static const u8 terrCol[T_N][3]={
    {2,5,16},{4,12,26},{28,25,14},{8,22,6},{3,13,4},
    {16,14,8},{15,15,17},{28,30,31},{31,9,2},{29,22,11},
    {7,14,10},{3,17,9},{18,12,6}
};
static void setCell(int x,int y,u8 tile){
    if(x>=0&&y>=0&&x<WW&&y<WH){world[y][x]=tile;editCalls++;}
}
#include "source/font5x7.h"
#include "3ds/explore3d.h"
static void check(int value,const char *message){
    if(!value){fprintf(stderr,"EXPLORE3D FAIL: %s\n",message);exit(1);}
}
static struct {u8 first[48],img[400*240*3],last[48];} guarded;
static unsigned long hash(void){
    unsigned long h=2166136261u;
    for(int i=0;i<400*240*3;i++)h=(h^guarded.img[i])*16777619u;
    return h;
}
int main(void){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)world[y][x]=T_GRASS;
    memset(guarded.first,0x5a,sizeof guarded.first);
    memset(guarded.last,0xc3,sizeof guarded.last);
    ggExpBegin(64,64,0);
    check(ggExplore.active&&(ggExplore.px>>8)==64&&(ggExplore.py>>8)==64,
          "did not enter the player's map at its saved camera position");
    check(ggExpTrigReady,"first-person camera trig not cached");
    int ox=ggExplore.px,oy=ggExplore.py;
    ggExplore.angle=0;
    ggExpStep(KEY_UP,0,0);
    check(ggExplore.px>ox&&ggExplore.py==oy,
          "D-pad forward does not travel along camera heading");
    ggExpStep(0,0,0);
    int yaw=ggExplore.angle;
    ggExpStep(KEY_R,0,0);
    check(ggExplore.angle!=yaw,"R shoulder does not turn the viewpoint");
    ggExplore.angle=0;ggExplore.px=64*256+128;ggExplore.py=64*256+128;
    int tx,ty;ggExpTarget(&tx,&ty);
    check(tx==67&&ty==64,"build crosshair does not target the adjacent world");
    ggExplore.material=4; // HILL
    check(ggExpEdit(tx,ty,ggExplore.material,0),"cannot place a terrain block");
    check(world[ty][tx]==T_HILL&&editCalls==1,
          "voxel edit did not update the existing 2D god-game map");
    bld[ty][tx]=B_HALL;
    check(!ggExpEdit(tx,ty,0,1)&&world[ty][tx]==T_HILL,
          "terrain edit razed a real city hall");
    bld[ty][tx]=0;occ[ty][tx]=1;
    check(!ggExpEdit(tx,ty,0,1),"terrain edit damaged an occupied creature");
    occ[ty][tx]=0;
    check(ggExpEdit(tx,ty,0,1)&&world[ty][tx]==T_GRASS,
          "removing the terrain does not restore the live world");
    ggExplore.px=63*256+248;ggExplore.py=64*256+128;
    ggExplore.angle=0;world[64][64]=T_DEEP;
    int before=ggExplore.px;
    ggExpStep(KEY_UP,0,0);
    check(ggExplore.px==before,"walking crossed unwalkable deep water");
    ggExpStep(0,0,0);
    world[64][64]=T_GRASS;bld[64][64]=B_TOWER;
    ggExpStep(KEY_UP,0,0);
    check(ggExplore.px==before,"walking crossed a built city tower");
    bld[64][64]=0;
    ggExplore.px=64*256+128;ggExplore.py=64*256+128;
    camX=64*8-128;camY=64*8-96;
    ggExpStep(0,0,0);
    ggExpStep(KEY_TOUCH,128+4*8,96+3*8);
    check((ggExplore.px>>8)==68&&(ggExplore.py>>8)==67,
          "tapping the actual bottom map cannot visit its world location");
    ggExpStep(0,0,0);
    check(ggExpStep(KEY_SELECT,0,0)==GG_EXP_SAVE,
          "SELECT cannot invoke existing save system");
    ggExpStep(0,0,0);
    check(ggExpStep(KEY_START,0,0)==GG_EXP_EXIT,
          "START cannot safely leave first-person mode");
    ggExplore.px=64*256+128;ggExplore.py=64*256+128;
    ggExplore.angle=0;
    clock_t started=clock();
    ggExpRender(guarded.img);
    unsigned long first=hash();
    ggExplore.angle=64;ggExpRender(guarded.img);
    check(first!=hash(),"camera does not render a different scene when turned");
    bld[70][70]=B_HALL;bld[69][70]=B_TOWER;
    ggExplore.angle=32;
    ggExpRender(guarded.img);unsigned long built=hash();
    bld[70][70]=0;bld[69][70]=0;
    ggExpRender(guarded.img);
    check(hash()!=built,"user-built city buildings are absent from the view");
    check(guarded.first[0]==0x5a&&guarded.first[47]==0x5a&&
          guarded.last[0]==0xc3&&guarded.last[47]==0xc3,
          "renderer overran 400x240 target framebuffer");
    double elapsed=1000.0*(clock()-started)/CLOCKS_PER_SEC;
    printf("EXPLORE3D OK: live-world collision, camera, buildings, editing, "
           "save/exit keys, screen canaries; 4 host renders %.1f ms total\n",
           elapsed);
    return 0;
}
