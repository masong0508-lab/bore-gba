// main3ds.c - Bore: GodGuy on the 3DS. The game (source/main.c, DS user interface and all) is compiled unchanged against compat/ndsshim.h; this file supplies the keys, the screens and the sound
// through platform3ds.c. The touch screen shows the world and the toolbar, the top screen the status screen, as on the DS. The 3D globe is DS-only.
#define SAVE_PATH "sdmc:/godguy.sav"
#define main ds_main
#include "../source/main.c"
#undef main
extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,const short*snd,int frames); extern void plat_exit(void);
static unsigned char topBuf[400*240*3], botBuf[320*240*3]; static short sndBuf[700*2]; static double carry;
void shimInput(int frame,int*held,int*tx,int*ty){ (void)frame; *held=plat_keys(tx,ty); }
void shimPresent(void){ double want=SHIM_SR/60.0+carry; int n=(int)want; carry=want-n; shimMix(sndBuf,n); shimRender(topBuf,botBuf); plat_present(topBuf,botBuf,sndBuf,n); }
int main(void){ plat_init(); ds_main(); plat_exit(); return 0; }
