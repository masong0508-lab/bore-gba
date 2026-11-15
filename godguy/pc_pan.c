// B + stylus drag pans the 2D map (normal view and zoomed); nothing is painted while B is held; without B the stylus still paints.
//   gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_pan pc_pan.c -lm && ./pc_pan
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
static int cx0,cy0,cx1,cy1,zx0,zx1,use0,use1,use2,zcx0,zcx1;
void shimInput(int frame,int*held,int*tx,int*ty){ *held=0;
    if(frame==3){ stUse=5; startWorld(1); paused=1; power=1; }   // WATER (paints) in a story world
    if(frame==10){ camX=300; camY=300; cx0=camX; cy0=camY; use0=stUse; }
    if(frame>=12&&frame<=40){ *held=KEY_TOUCH|KEY_B; *tx=200-(frame-12)*2; *ty=100-(frame-12); }      // B held, drag left and up by ~56 px / 28 px: the view moves right and down
    if(frame==45){ cx1=camX; cy1=camY; use1=stUse; }
    if(frame>=50&&frame<=60){ *held=KEY_TOUCH; *tx=120; *ty=90; }                                      // no B: this paints
    if(frame==64){ use2=stUse; }
    if(frame==70){ zSet(Z_NORMAL+1); }
    if(frame==75){ zx0=camX; zcx0=zcx; }
    if(frame>=80&&frame<=100){ *held=KEY_TOUCH|KEY_B; *tx=220-(frame-80)*3; *ty=100; }
    if(frame==105){ zcx1=zcx; zx1=camX;
        printf("B + drag pan:\n");
        CHECK(cx1>cx0+40&&cy1>cy0+20,"normal view moved with the pen (%d,%d -> %d,%d)",cx0,cy0,cx1,cy1);
        CHECK(use1==use0,"nothing was used while B was held (stUse %d -> %d)",use0,use1);
        CHECK(use2>use1,"the stylus without B still uses the power (stUse %d -> %d)",use1,use2);
        CHECK(zcx1!=zcx0,"zoomed view centre moved too (%d -> %d)",zcx0,zcx1);
        printf(fails?"FAILED\n":"PASSED\n"); exit(fails?1:0); } }
void shimPresent(void){}
int main(int argc,char**argv){ char d[]="/tmp/gg_pan_run"; mkdir(d,0777); if(chdir(d)) return 1; return ds_main(); }
