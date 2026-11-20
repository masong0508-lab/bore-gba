// main3ds.c - Bore: GodGuy on the 3DS. The game (source/main.c, DS user interface and all) is compiled unchanged against compat/ndsshim.h; this file supplies the keys, the screens and the sound
// through platform3ds.c. The touch screen shows the world and the toolbar, the top screen the status screen, as on the DS. The 3DS globe has extra atmospheric lighting and adaptive detail.
#define SAVE_PATH "sdmc:/godguy.sav"
#define GG_THEATRE 1
#define main ds_main
#include "../source/main.c"
#undef main
#include "frame_clock.h"
extern unsigned long long plat_ms(void); extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth); extern void plat_exit(void); extern int plat_isNew3DS(void);
static unsigned char topBuf[400*240*3], botBuf[320*240*3];
static short sndBuf[2800*2];
static Gg3dsFrameClock frameClock;
void shimInput(int frame,int*held,int*tx,int*ty){ (void)frame; *held=plat_keys(tx,ty); }
void shimPresent(void){
    // Preserve wall-clock time across short rendering stalls instead of
    // discarding everything above 80 ms, which dragged music and starved
    // the sound buffer. Unmixed samples and sequencer ticks are bounded
    // but carried into subsequent frames.
    int n,k,quality;
    gg3dsClockFrame(&frameClock,plat_ms(),&n,&k,&quality);
    shimMusicTicks=k;
    gg3dsGlobeQuality=quality;  // New 3DS detail yields to audio under load
    shimMix(sndBuf,n);
    int tc,bc;
    shimRenderChanged(topBuf,botBuf,&tc,&bc);
    plat_present(topBuf,botBuf,tc,bc,sndBuf,n,shimSwap?0:shimDepth);
}
int main(void){
    plat_init();
    int new3ds=plat_isNew3DS();
    gg3dsClockInit(&frameClock,new3ds);
    gg3dsGlobeQuality=new3ds;
#ifdef GG_HQVOX
    gmVoxWarm();     // predecode vocals, never decode in the frame mixer
#endif
#ifdef SHIM_HQ
    shimHqOn=new3ds; // retain the lighter mixer on Old 3DS
#endif
    ds_main(); plat_exit(); return 0;
}
