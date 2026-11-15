// main3ds.c - Bore: GodGuy on the 3DS. The game (source/main.c, DS user interface and all) is compiled unchanged against compat/ndsshim.h; this file supplies the keys, the screens and the sound
// through platform3ds.c. The touch screen shows the world and the toolbar, the top screen the status screen, as on the DS. The 3D globe is DS-only.
#define SAVE_PATH "sdmc:/godguy.sav"
#define GG_THEATRE 1
#define main ds_main
#include "../source/main.c"
#undef main
extern unsigned long long plat_ms(void); extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth); extern void plat_exit(void); extern int plat_isNew3DS(void);
static unsigned char topBuf[400*240*3], botBuf[320*240*3]; static short sndBuf[2800*2]; static double carry, tickAcc; static unsigned long long lastMs;
void shimInput(int frame,int*held,int*tx,int*ty){ (void)frame; *held=plat_keys(tx,ty); }
void shimPresent(void){      // the sound is mixed for the time that really passed (a slow frame gets more samples, never a gap), and the music advances by the same time
    unsigned long long now=plat_ms(); double dt=lastMs?(double)(now-lastMs):16.67; lastMs=now; if(dt<4) dt=4; if(dt>80) dt=80;
    double want=dt*SHIM_SR/1000.0+carry; int n=(int)want; carry=want-n; if(n>2800) n=2800; tickAcc+=dt*0.06; int k=(int)tickAcc; tickAcc-=k; if(k>4) k=4; shimMusicTicks=k;
    shimMix(sndBuf,n); int tc,bc; shimRenderChanged(topBuf,botBuf,&tc,&bc); plat_present(topBuf,botBuf,tc,bc,sndBuf,n,shimSwap?0:shimDepth); }   // only a screen that changed is drawn and copied
int main(void){ plat_init();
#ifdef GG_HQVOX
    gmVoxWarm();      // every vocal chop is unpacked once, now, so no strike ever has to decode in the middle of a frame
#endif
#ifdef SHIM_HQ
    shimHqOn=plat_isNew3DS();      // the rich mixer (cubic interpolation, unison voices, reverb) on a New 3DS; the plain one on an old 3DS, which needs the cycles for the game
#endif
    ds_main(); plat_exit(); return 0; }
