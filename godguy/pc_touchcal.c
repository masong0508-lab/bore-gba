// PC test of the touch screen filter (hw/touchfilter.h, the code that runs on the ARM7): calibration, noise, outliers, bounce, release
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hw/touchfilter.h"
static int fails;
#define CHECK(c,msg,...) do{ if(!(c)){ fails++; printf("  FAIL: " msg "\n",##__VA_ARGS__); } }while(0)
int main(){
    // a firmware-like block: adc (0x2D0,0x3D0) for pixel (32,32) and (0xD20,0xBD0) for pixel (224,160): the usual DS numbers
    unsigned char b[0x100]; memset(b,0,sizeof b); b[0x58]=0xD0; b[0x59]=0x02; b[0x5A]=0xD0; b[0x5B]=0x03; b[0x5C]=32; b[0x5D]=32; b[0x5E]=0x20; b[0x5F]=0x0D; b[0x60]=0xD0; b[0x61]=0x0B; b[0x62]=224; b[0x63]=160;
    TouchCal c; tcLoad(&c,b); printf("calibration loaded: ok %d, adc (%d,%d)-(%d,%d) -> pixels (%d,%d)-(%d,%d)\n",c.ok,c.ax1,c.ay1,c.ax2,c.ay2,c.sx1,c.sy1,c.sx2,c.sy2); CHECK(c.ok,"valid calibration refused");
    int px,py; tcMap(&c,c.ax1,c.ay1,&px,&py); CHECK(px==31&&py==31,"point 1 maps to %d,%d (want 31,31: the pixels are 1 based)",px,py);
    tcMap(&c,c.ax2,c.ay2,&px,&py); CHECK(px==223&&py==159,"point 2 maps to %d,%d (want 223,159)",px,py);
    tcMap(&c,0,0,&px,&py); CHECK(px==0&&py==0,"far corner not clamped: %d,%d",px,py); tcMap(&c,4095,4095,&px,&py); CHECK(px==255&&py==191,"far corner not clamped: %d,%d",px,py);
    int last=-1; for(int r=0;r<4096;r+=16){ tcMap(&c,r,r,&px,&py); CHECK(px>=last,"not monotonic at %d",r); last=px; }
    // bad firmware data -> the plain linear fallback
    unsigned char bad[0x100]; memset(bad,0xFF,sizeof bad); TouchCal cb; tcLoad(&cb,bad); CHECK(!cb.ok,"garbage calibration accepted"); tcMap(&cb,2048,2048,&px,&py); CHECK(px==128&&py==96,"fallback maps the middle to %d,%d",px,py);
    // a steady touch with noise: the reported position stays within 1 pixel of the truth
    TouchState st; memset(&st,0,sizeof st); srand(5); int rx=1700, ry=1500, tx,ty; tcMap(&c,rx,ry,&tx,&ty); int maxerr=0, reported=0, firstRep=-1;
    for(int f=0;f<60;f++){ int xs[5], ys[5]; for(int k=0;k<5;k++){ xs[k]=rx+rand()%17-8; ys[k]=ry+rand()%17-8; } if(f%7==3){ xs[2]+=900; ys[4]-=700; }      // an outlier now and then
        int pen=tcFrame(&st,&c,1,xs,ys,5,&px,&py); if(pen){ reported++; if(firstRep<0) firstRep=f; int e=abs(px-tx)>abs(py-ty)?abs(px-tx):abs(py-ty); if(e>maxerr) maxerr=e; } }
    printf("steady noisy touch: reported %d of 60 frames from frame %d, worst error %d px\n",reported,firstRep,maxerr); CHECK(maxerr<=1,"error %d px",maxerr); CHECK(firstRep==1,"first frame (the bounce) was reported or the touch was late: %d",firstRep);
    // bounce: a wild first frame followed by the real position is reported at the real position, never at the wild one
    memset(&st,0,sizeof st); { int xs[5]={300,310,305,2200,300}, ys[5]={300,305,310,2900,300}; int pen=tcFrame(&st,&c,1,xs,ys,5,&px,&py); CHECK(!pen,"a bouncing first frame was reported"); }
    { int xs[5]={rx,rx+2,rx-2,rx+1,rx}, ys[5]={ry,ry+1,ry-1,ry+2,ry}; tcFrame(&st,&c,1,xs,ys,5,&px,&py); int pen=tcFrame(&st,&c,1,xs,ys,5,&px,&py); CHECK(pen&&abs(px-tx)<=1&&abs(py-ty)<=1,"second frame: pen %d at %d,%d (want %d,%d)",pen,px,py,tx,ty); }
    // one empty frame inside a drag does not release; two do
    { int xs[5]={rx,rx,rx,rx,rx}, ys[5]={ry,ry,ry,ry,ry}; int p1=tcFrame(&st,&c,0,xs,ys,5,&px,&py); CHECK(p1&&abs(px-tx)<=1,"a single empty frame released the touch"); int p2=tcFrame(&st,&c,0,xs,ys,5,&px,&py); CHECK(!p2,"two empty frames did not release"); }
    // a fast swipe (samples that disagree a lot) is not trusted and never reported as a random point
    memset(&st,0,sizeof st); { int rep=0; for(int f=0;f<10;f++){ int xs[5]={500+f*300,900+f*300,300+f*300,1500+f*300,700+f*300}, ys[5]={400,2000,900,3000,1500}; rep+=tcFrame(&st,&c,1,xs,ys,5,&px,&py); } CHECK(rep==0,"incoherent samples were reported %d times",rep); }
    // smoothing: jitter within a pixel is ignored, a real move is followed
    int sx=0, sy=0, have=0; tcSmooth(1,&sx,&sy,&have,100,100); tcSmooth(1,&sx,&sy,&have,101,100); CHECK(sx==100,"1 px jitter moved the point: %d",sx); tcSmooth(1,&sx,&sy,&have,140,100); CHECK(sx>=130&&sx<=140,"a real move was not followed: %d",sx); tcSmooth(0,&sx,&sy,&have,0,0); CHECK(!have,"smoothing kept state after release");
    printf("%s\n",fails?"TOUCHCAL FAIL":"TOUCHCAL OK"); return fails?1:0; }
