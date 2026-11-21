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
// Minimal host simulation of the *same* creature records, identity
// compaction, occupancy and possession rules used by the full 3DS port.
#define ARCH_N 8
#define FL_SAPIENT 0x0400
#define ST_MINDLESS 0x80
#define HAS_STATUS(c,f) (((c)->sick&(f))!=0)
#define DISEASE(c) ((c)->sick&3)
typedef struct {int a;} Sp;
typedef struct {int diet;} Arch;
typedef struct {int sp,x,y,alive,sick;} Cr;
static Cr cr[8];
static unsigned crId[8];
static unsigned short crFl[8];
static int nCr=5,nSp=8;
static Sp sp[8];
static Arch arch[8];
static int possessed=-1,inspected=-1,inspKind,inspIdx,inspT;
static void crName(const Cr*c,char*out){
    snprintf(out,40,"CITIZEN %d",c->sp);
}
static int stepOk(const Sp*species,int*nx,int*ny){
    (void)species;
    return *nx>0&&*ny>0&&*nx<WW-1&&*ny<WH-1&&
           world[*ny][*nx]>T_WATER&&
           world[*ny][*nx]!=T_LAVA&&!occ[*ny][*nx];
}
static void moveTo(Cr*c,int i,int nx,int ny){
    occ[c->y][c->x]=0;
    c->x=nx;c->y=ny;
    occ[ny][nx]=(unsigned short)(i+1);
}
#include "3ds/hybrid3d.h"
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
    // Every playable mind comes from the actual game's diet or
    // FL_SAPIENT classification; a common animal is not a human brain.
    sp[0].a=0;arch[0].diet=4;  // person
    sp[1].a=1;arch[1].diet=1;  // unevolved animal
    cr[0]=(Cr){0,40,41,1,0};crId[0]=1001;
    cr[1]=(Cr){1,43,41,1,0};crId[1]=1002;
    cr[2]=(Cr){1,47,41,1,0};crId[2]=1003;
    occ[41][40]=1;occ[41][43]=2;occ[41][47]=3;
    check(ggHybridEligible(0)&&!ggHybridEligible(1),
          "hybrid mind filter confused people with ordinary animals");
    check(ggHybridPick(43,41)==-1,
          "hybrid selection possessed a nonsentient animal");
    crFl[2]=FL_SAPIENT;
    check(ggHybridEligible(2)&&ggHybridPick(47,41)==2,
          "uplifted/sapient animal cannot become a selected hybrid mind");
    cr[2].sick=ST_MINDLESS;
    check(!ggHybridEligible(2),"mindless creature can enter first-person POV");
    cr[2].sick=3;
    check(!ggHybridEligible(2),"infected mind can enter hybrid POV");
    cr[2].sick=0;
    check(ggHybridPick(40,41)==0,
          "2D tile select did not use the authoritative occupant grid");
    check(ggHybridBegin(0,0)&&ggExplore.actorId==1001&&
          ggExplore.px>>8==40&&ggExplore.py>>8==41,
          "hybrid camera failed to attach to the chosen 2D person");
    check(possessed==0&&inspected==0,
          "2D Inspect/Possess selection was lost on entering 3D");
    ggExplore.angle=0;
    ggExplore.px=40*256+244;ggExplore.py=41*256+128;
    int ox2=ggExplore.px,oy2=ggExplore.py;
    ggExpStep(KEY_UP,0,0);
    check(ggHybridMove(ox2,oy2)&&cr[0].x==41&&cr[0].y==41&&
          occ[41][41]==1&&occ[41][40]==0,
          "3D walking did not move that same creature on the 2D map");
    ggExpStep(0,0,0);
    bld[41][42]=B_HOUSE;
    ggExplore.px=41*256+249;ggExplore.py=41*256+128;
    ox2=ggExplore.px;oy2=ggExplore.py;
    ggExpStep(KEY_UP,0,0);
    check(ggHybridMove(ox2,oy2)&&cr[0].x==41,
          "hybrid character phased through a city building");
    bld[41][42]=0;
    // Saving compacts the creature array. The hybrid camera follows
    // crId, not a stale index that may now point to somebody else.
    cr[4]=cr[0];crId[4]=crId[0];crFl[4]=crFl[0];
    cr[0].alive=0;crId[0]=9999;
    occ[41][41]=5;
    check(ggHybridResolve()==4,
          "save compaction rebound the camera to a different person");
    ggHybridAfterSave();
    check(possessed==4&&inspected==4&&inspIdx==4,
          "2D selection no longer tracks the same mind after save");
    cr[4].alive=0;
    check(ggHybridResolve()<0,
          "dead or invalid creature remains possessed in 3D");
    // Actor can begin in a house and leave it, instead of having its POV
    // teleported to a fake grassy spawn to satisfy generic walk checks.
    cr[2].x=47;cr[2].y=41;cr[2].alive=1;crId[2]=1003;
    bld[41][47]=B_HUT;
    check(ggHybridBegin(2,0)&&ggExplore.px>>8==47,
          "a sentient household occupant was displaced at scene entry");
    check(ggExpWalkable(47,41,10,0),
          "first-person control locked a resident inside their own home");
    bld[41][47]=0;
    double elapsed=1000.0*(clock()-started)/CLOCKS_PER_SEC;
    printf("EXPLORE3D OK: hybrid minds, stable IDs, selection, 2D/3D "
           "actor movement, protected homes, safe frame buffers; "
           "4 host renders %.1f ms total\n",elapsed);
    return 0;
}
