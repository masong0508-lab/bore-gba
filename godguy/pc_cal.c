// PC test of the touch calibration on a panel that reads the bottom of the screen ~14 px too high (-DSHIM_DISTORT).
// ./pc_cal 2: a second start (godguy.cal left by run 0): no calibration screen, the same taps work.
// ./pc_cal 0: calibrate (hold both crosses, A to keep), then tap OPTIONS and its BACK button.  ./pc_cal 1: skip with START, same taps (the BACK tap misses: the bug).
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static int mode;
typedef struct { int f0, f1, keys, tx, ty; } Ev;
static const Ev script[]={ {20,32,KEY_TOUCH,16,16}, {50,62,KEY_TOUCH,240,176}, {80,90,KEY_TOUCH,128,96}, {100,102,KEY_A,0,0},
 {130,134,KEY_TOUCH,60,119}, {160,164,KEY_TOUCH,228,6} };
static const Ev skipS[]={ {10,12,KEY_START,0,0}, {130,134,KEY_TOUCH,60,119}, {160,164,KEY_TOUCH,228,6} };
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; const Ev*s=mode==0?script:mode==1?skipS:skipS+1; int n=mode==0?6:mode==1?3:2;
    for(int i=0;i<n;i++) if(frame>=s[i].f0&&frame<s[i].f1){ *held|=s[i].keys; if(s[i].keys&KEY_TOUCH){ *tx=s[i].tx; *ty=s[i].ty; } }
    if(frame==70||frame==150){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[80]; sprintf(fn,"/tmp/gg_cal_%d_%d.ppm",mode,frame); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); }
    if(frame>=160&&frame<164) printf("f%d raw %d,%d -> %d,%d held=%d\n",frame,hwRx,hwRy,shimTx,shimTy,shimHeld); if(frame==150) printf("mode %d: after OPTIONS tap mScr=%d (want 1)\n",mode,mScr);
    if(frame>=190){ printf("mode %d: godguy.cal %s; after BACK tap mScr=%d (want 0 when calibrated) calOn=%d hwCalOn=%d\n",mode,access("godguy.cal",0)==0?"present":"absent",mScr,calOn,hwCalOn); exit(0); } }
void shimPresent(void){}
int main(int argc,char**argv){ mode=argc>1?atoi(argv[1]):0; return ds_main(); }
