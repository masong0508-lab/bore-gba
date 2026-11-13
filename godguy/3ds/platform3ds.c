// platform3ds.c - the 3DS side of the port (libctru): keys, touch, the two screens and the sound. It is kept apart from the game's translation unit because libctru's KEY_ names clash
// with the shim's. The game itself (source/main.c) runs unchanged through compat/ndsshim.h: see main3ds.c.
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#define SR 32768
#define NBUF 8       // audio buffers in flight (about 140 ms): a slow frame no longer drops sound
static ndspWaveBuf wb[NBUF]; static short*abuf[NBUF]; static int audioOk;
int plat_init(void){
    osSetSpeedupEnable(true);     // New 3DS: 804 MHz and the bigger cache (ignored on the old 3DS)
    gfxInitDefault(); gfxSet3D(false); gfxSetDoubleBuffering(GFX_TOP,true); gfxSetDoubleBuffering(GFX_BOTTOM,true);
    if(R_SUCCEEDED(ndspInit())){ ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspChnSetInterp(0,NDSP_INTERP_LINEAR); ndspChnSetRate(0,SR); ndspChnSetFormat(0,NDSP_FORMAT_STEREO_PCM16);
        for(int i=0;i<NBUF;i++){ abuf[i]=(short*)linearAlloc(700*4); memset(&wb[i],0,sizeof wb[i]); wb[i].data_vaddr=abuf[i]; wb[i].status=NDSP_WBUF_DONE; } audioOk=1; }
    return 1; }
// keys in the shim's layout (A B SELECT START RIGHT LEFT UP DOWN R L X Y, bit 12 = touch)
int plat_keys(int*tx,int*ty){
    hidScanInput(); u32 k=hidKeysHeld(); int h=(int)(k&0xFFF); if(k&KEY_CPAD_UP) h|=1<<6; if(k&KEY_CPAD_DOWN) h|=1<<7; if(k&KEY_CPAD_LEFT) h|=1<<5; if(k&KEY_CPAD_RIGHT) h|=1<<4;
    if(k&KEY_TOUCH){ touchPosition t; hidTouchRead(&t); int x=t.px-32, y=t.py-24; if(x>=0&&x<256&&y>=0&&y<192){ *tx=x; *ty=y; h|=1<<12; } }
    if(!aptMainLoop()) exit(0); return h; }
// RGB rows -> the 3DS framebuffer (BGR, columns first, bottom-up), in 8 x 8 tiles so the reads and the writes both stay in the small data cache
static void swz(u8*fb,const unsigned char*src,int w){
    for(int x0=0;x0<w;x0+=8) for(int y0=0;y0<240;y0+=8) for(int x=x0;x<x0+8;x++){ u8*d=&fb[(x*240+(239-y0))*3]; const unsigned char*s=&src[(y0*w+x)*3];
        for(int y=0;y<8;y++,s+=w*3,d-=3){ d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; } } }
static int dirtyT, dirtyB;      // the framebuffers are double buffered: a changed screen is copied twice, an unchanged one not at all
void plat_present(const unsigned char*top,const unsigned char*bot,const short*snd,int frames,int changed){
    if(changed&1) dirtyT=2; if(changed&2) dirtyB=2;
    if(dirtyT){ swz(gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL),top,400); dirtyT--; }       // 400 x 240, BGR8, columns first
    if(dirtyB){ swz(gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,NULL,NULL),bot,320); dirtyB--; }
    gfxFlushBuffers(); gfxSwapBuffers();
    if(audioOk&&frames>0&&frames<=700){ for(int i=0;i<NBUF;i++) if(wb[i].status==NDSP_WBUF_DONE||wb[i].status==NDSP_WBUF_FREE){ memcpy(abuf[i],snd,frames*4); wb[i].nsamples=frames; DSP_FlushDataCache(abuf[i],frames*4); ndspChnWaveBufAdd(0,&wb[i]); break; } }
    gspWaitForVBlank(); }
void plat_exit(void){ if(audioOk) ndspExit(); gfxExit(); }
