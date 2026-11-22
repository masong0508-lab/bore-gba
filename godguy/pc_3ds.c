// PC test of the 3DS platform file (3ds/platform3ds.c, built against 3ds/stub/3ds.h): the tiled screen copy gives the same pixels as the plain copy, a screen that did not change is not copied but both of its
// buffers always show the latest picture, the New 3DS speed-up is switched on, and the sound queue takes frames in order, twelve deep, dropping only when it is full.
//   gcc -O2 -I3ds/stub -o pc_3ds pc_3ds.c 3ds/platform3ds.c -lm && ./pc_3ds
#include <3ds.h>
#include <stdio.h>
u8 stubFb[2][2][400*240*3]; int stubCur[2], stubShown[2], stubSwaps, stubFlushes, stubSpeedup, stubQueued, stubNextExpect; u32 stubKeys; touchPosition stubTouch; short*stubQueue[64]; ndspWaveBuf*stubWb[64];
int stubAllocFailAt=-1,stubAllocCalls,stubAllocLive,stubAllocFreed;
int stubDSPInitResult,stubDSPInits,stubDSPExits,stubDSPActive;
int stubFreeWhileDSPActive,stubGfxInits,stubGfxExits;
static void playAll(void){ for(int i=0;i<64&&i<stubQueued;i++) stubWb[i]->status=NDSP_WBUF_DONE; }   // the DSP has played everything queued
extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int tc,int bc,const short*snd,int frames,const signed char*depth); extern void plat_exit(void);
u8 stubFbR[2][400*240*3]; float stubSlider;
static unsigned char top[400*240*3], bot[320*240*3]; static short snd[700*2]; static unsigned rs=12345;
static unsigned rnd(void){ rs=rs*1103515245u+12345u; return rs>>8; }
static void plain(u8*fb,const unsigned char*src,int W){ for(int x=0;x<W;x++) for(int y=0;y<240;y++){ const unsigned char*s=&src[(y*W+x)*3]; u8*d=&fb[(x*240+(239-y))*3]; d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; } }
static void scribble(unsigned char*b,int n,int count){ for(int i=0;i<count;i++) b[rnd()%n]=(unsigned char)rnd(); }
static int expect(int good,const char *why){
    if(good)return 0;
    printf("FAIL: %s\n",why);return 1;
}
static int audioLifecycleTests(void){
    int bad=0;
    bad+=expect(stubAllocLive==0&&stubDSPActive==0&&stubAllocFreed==12,
                "normal shutdown did not release the whole audio pool");
    int exits=stubDSPExits,gfxExits=stubGfxExits;
    plat_exit();
    bad+=expect(stubDSPExits==exits&&stubGfxExits==gfxExits,
                "repeated shutdown released platform services twice");
    for(int fail=0;fail<12;fail++){
        stubAllocFailAt=fail;stubAllocCalls=stubAllocFreed=stubQueued=0;
        int inits=stubDSPInits;exits=stubDSPExits;
        int gfxInits=stubGfxInits;gfxExits=stubGfxExits;
        plat_init();plat_init();
        bad+=expect(stubDSPInits==inits+1&&stubGfxInits==gfxInits+1,
                    "repeated initialization reopened platform services");
        bad+=expect(stubAllocCalls==fail+1&&stubAllocFreed==fail&&
                    stubAllocLive==0&&stubDSPActive==0&&stubDSPExits==exits+1,
                    "partial allocation failure leaked buffers or left DSP active");
        int swaps=stubSwaps;
        plat_present(top,bot,1,1,snd,546,0);
        bad+=expect(stubQueued==0&&stubSwaps==swaps+1,
                    "audio allocation failure stopped graphics or queued invalid sound");
        plat_exit();plat_exit();
        bad+=expect(stubAllocLive==0&&stubDSPExits==exits+1&&
                    stubGfxExits==gfxExits+1,
                    "partial initialization could not shut down cleanly");
    }
    // Missing DSP support is also a valid silent startup, with no pool.
    stubAllocFailAt=-1;stubDSPInitResult=-1;
    stubAllocCalls=stubAllocFreed=stubQueued=0;exits=stubDSPExits;
    plat_init();plat_present(top,bot,1,1,snd,546,0);plat_exit();
    bad+=expect(stubAllocCalls==0&&stubAllocLive==0&&stubQueued==0&&
                stubDSPActive==0&&stubDSPExits==exits,
                "failed DSP startup allocated or shut down an unowned service");
    // Recovery after failed starts must retain stereo sample order and
    // the original 700-frame buffer boundaries, including a short tail.
    stubDSPInitResult=0;stubAllocCalls=stubAllocFreed=stubQueued=0;
    plat_init();
    bad+=expect(stubAllocLive==12&&stubDSPActive==1,
                "audio did not recover after failed initialization");
    plat_present(top,bot,0,0,NULL,546,0);
    bad+=expect(stubQueued==0,"a missing audio source was queued");
    static short samples[1405*2];
    for(int i=0;i<1405;i++){
        samples[i*2]=(short)(i+1);samples[i*2+1]=(short)(-i-1);
    }
    plat_present(top,bot,0,0,samples,1405,0);
    bad+=expect(stubQueued==3,"long sound did not retain 700/700/5 chunking");
    if(stubQueued==3){
        int done=0;
        for(int i=0;i<3;i++){
            int count=i<2?700:5;
            bad+=expect(stubWb[i]->nsamples==(u32)count&&
                        !memcmp(stubQueue[i],samples+done*2,(size_t)count*4),
                        "recovered queue changed stereo order or chunk contents");
            done+=count;
        }
    }
    plat_exit();
    bad+=expect(stubAllocLive==0&&stubAllocFreed==12&&stubDSPActive==0&&
                stubFreeWhileDSPActive==0,
                "audio memory was leaked or freed while DSP could still use it");
    printf("audio lifecycle: all 12 allocation failures, DSP failure, recovery and ordered 700/700/5 chunks: %s\n",
           bad?"FAIL":"ok");
    return bad;
}
int main(int argc,char **argv){
    if(argc==2){
        // Fault injection also makes the original NULL-buffer failure
        // reproducible independently of the normal framebuffer checks.
        stubAllocFailAt=atoi(argv[1]);
        plat_init();
        for(int i=0;i<12;i++)plat_present(top,bot,0,0,snd,546,0);
        plat_exit();
        return stubAllocLive||stubQueued||stubDSPActive;
    }
    int bad=0; plat_init(); if(!stubSpeedup){ printf("FAIL: the New 3DS speed-up was not switched on\n"); bad++; }
    // 1. the picture on both screens matches the plain copy after every frame, for a random run of changes and no-changes
    static u8 want[2][400*240*3]; int copies0=0, frames=400, shownBad=0;
    scribble(top,sizeof top,200000); scribble(bot,sizeof bot,150000); plat_present(top,bot,1,1,snd,546,0); plat_present(top,bot,0,0,snd,546,0);
    for(int f=0;f<frames;f++){ int tc=(rnd()%3)==0, bc=(rnd()%3)==0; if(tc) scribble(top,sizeof top,1+rnd()%5000); if(bc) scribble(bot,sizeof bot,1+rnd()%5000);
        int sw0=stubSwaps; plat_present(top,bot,tc,bc,snd,546,0); playAll(); if(stubSwaps==sw0) copies0++;
        plain(want[0],top,400); plain(want[1],bot,320);
        if(memcmp(stubFb[0][stubShown[0]],want[0],400*240*3)) shownBad++; if(memcmp(stubFb[1][stubShown[1]],want[1],320*240*3)) shownBad++; }
    printf("tiled copy and skipping: %d frames, %d frames shown a wrong picture, %d frames copied nothing\n",frames,shownBad,copies0); bad+=shownBad!=0; if(copies0<frames/10){ printf("FAIL: unchanged frames are still copied\n"); bad++; }
    // 2. the sound queue: frames go in in order; twelve in a row fill it; none are lost while it is drained
    playAll(); stubQueued=0; for(int i=0;i<16;i++){ for(int k=0;k<1092;k++) snd[k]=(short)(i*100+1); plat_present(top,bot,0,0,snd,546,0); }
    printf("sound queue: %d of 16 frames queued (the queue is 12 deep, so the rest are dropped)\n",stubQueued); if(stubQueued!=12){ printf("FAIL: expected 12\n"); bad++; }
    int order=1; for(int i=0;i<12&&i<stubQueued;i++) if(stubQueue[i][0]!=(short)(i*100+1)) order=0; printf("sound order: %s\n",order?"in order":"OUT OF ORDER"); bad+=!order;
    // 3. the 3D slider: at 0 the right eye is not touched; above it both eyes are made, they differ where the depth is, and the left is not the flat picture
    { static signed char dep[256*192]; for(int i=0;i<256*192;i++) dep[i]=(signed char)((i/256/16)%7-2); memset(stubFbR,0,sizeof stubFbR); stubSlider=0.f; plat_present(top,bot,1,1,snd,546,dep); plat_present(top,bot,0,0,snd,546,dep);
      int untouched=1; for(int i=0;i<400*240*3;i++) if(stubFbR[0][i]||stubFbR[1][i]){ untouched=0; break; } printf("3D slider at 0: right eye untouched: %s\n",untouched?"yes":"NO"); bad+=!untouched;
      stubSlider=0.7f; plat_present(top,bot,1,1,snd,546,dep); plat_present(top,bot,0,0,snd,546,dep); int diff=0; const u8*l=stubFb[0][stubShown[0]], *r=stubFbR[stubShown[0]]; for(int i=0;i<400*240*3;i++) diff+=l[i]!=r[i];
      static u8 flat[400*240*3]; plain(flat,top,400); int dl=0; for(int i=0;i<400*240*3;i++) dl+=l[i]!=flat[i]; printf("3D slider at 0.7: eyes differ in %d bytes, left differs from the flat picture in %d\n",diff,dl); if(diff<1000||dl<1000){ printf("FAIL: no stereo\n"); bad++; }
      stubSlider=0.f; plat_present(top,bot,1,1,snd,546,dep); plat_present(top,bot,0,0,snd,546,dep); plain(flat,top,400); int back=!memcmp(stubFb[0][stubShown[0]],flat,400*240*3); printf("slider back to 0: flat picture again: %s\n",back?"yes":"NO"); bad+=!back; }
    plat_exit();bad+=audioLifecycleTests(); printf(bad?"3ds: FAIL\n":"3ds: ok\n"); return bad?1:0; }
