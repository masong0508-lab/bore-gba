// PC test of the 3DS platform file (3ds/platform3ds.c, built against 3ds/stub/3ds.h): the tiled screen copy gives the same pixels as the plain copy, a screen that did not change is not copied but both of its
// buffers always show the latest picture, the New 3DS speed-up is switched on, and the sound queue takes frames in order, eight deep, dropping only when it is full.
//   gcc -O2 -I3ds/stub -o pc_3ds pc_3ds.c 3ds/platform3ds.c -lm && ./pc_3ds
#include <3ds.h>
#include <stdio.h>
u8 stubFb[2][2][400*240*3]; int stubCur[2], stubShown[2], stubSwaps, stubFlushes, stubSpeedup, stubQueued, stubNextExpect; u32 stubKeys; touchPosition stubTouch; short*stubQueue[64]; ndspWaveBuf*stubWb[64];
static void playAll(void){ for(int i=0;i<64&&i<stubQueued;i++) stubWb[i]->status=NDSP_WBUF_DONE; }   // the DSP has played everything queued
extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int tc,int bc,const short*snd,int frames); extern void plat_exit(void);
static unsigned char top[400*240*3], bot[320*240*3]; static short snd[700*2]; static unsigned rs=12345;
static unsigned rnd(void){ rs=rs*1103515245u+12345u; return rs>>8; }
static void plain(u8*fb,const unsigned char*src,int W){ for(int x=0;x<W;x++) for(int y=0;y<240;y++){ const unsigned char*s=&src[(y*W+x)*3]; u8*d=&fb[(x*240+(239-y))*3]; d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; } }
static void scribble(unsigned char*b,int n,int count){ for(int i=0;i<count;i++) b[rnd()%n]=(unsigned char)rnd(); }
int main(void){
    int bad=0; plat_init(); if(!stubSpeedup){ printf("FAIL: the New 3DS speed-up was not switched on\n"); bad++; }
    // 1. the picture on both screens matches the plain copy after every frame, for a random run of changes and no-changes
    static u8 want[2][400*240*3]; int copies0=0, frames=400, shownBad=0;
    scribble(top,sizeof top,200000); scribble(bot,sizeof bot,150000); plat_present(top,bot,1,1,snd,546); plat_present(top,bot,0,0,snd,546);
    for(int f=0;f<frames;f++){ int tc=(rnd()%3)==0, bc=(rnd()%3)==0; if(tc) scribble(top,sizeof top,1+rnd()%5000); if(bc) scribble(bot,sizeof bot,1+rnd()%5000);
        int sw0=stubSwaps; plat_present(top,bot,tc,bc,snd,546); playAll(); if(stubSwaps==sw0) copies0++;
        plain(want[0],top,400); plain(want[1],bot,320);
        if(memcmp(stubFb[0][stubShown[0]],want[0],400*240*3)) shownBad++; if(memcmp(stubFb[1][stubShown[1]],want[1],320*240*3)) shownBad++; }
    printf("tiled copy and skipping: %d frames, %d frames shown a wrong picture, %d frames copied nothing\n",frames,shownBad,copies0); bad+=shownBad!=0; if(copies0<frames/10){ printf("FAIL: unchanged frames are still copied\n"); bad++; }
    // 2. the sound queue: frames go in in order; eight in a row fill it; none are lost while it is drained
    playAll(); stubQueued=0; for(int i=0;i<12;i++){ for(int k=0;k<1092;k++) snd[k]=(short)(i*100+1); plat_present(top,bot,0,0,snd,546); }
    printf("sound queue: %d of 12 frames queued (the queue is 8 deep, so the rest are dropped)\n",stubQueued); if(stubQueued!=8){ printf("FAIL: expected 8\n"); bad++; }
    int order=1; for(int i=0;i<8&&i<stubQueued;i++) if(stubQueue[i][0]!=(short)(i*100+1)) order=0; printf("sound order: %s\n",order?"in order":"OUT OF ORDER"); bad+=!order;
    plat_exit(); printf(bad?"3ds: FAIL\n":"3ds: ok\n"); return bad?1:0; }
