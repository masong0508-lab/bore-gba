// platform3ds.c - the 3DS side of the port (libctru): keys, touch, the two screens and the sound. It is kept apart from the game's translation unit because libctru's KEY_ names clash
// with the shim's. The game itself (source/main.c) runs unchanged through compat/ndsshim.h: see main3ds.c.
// Speed: the New 3DS's faster clock is switched on; a screen is copied only when the game's picture changed (and for the two frames its two buffers need), and the copy walks 8 x 8 blocks,
// because the 3DS screens are stored column by column and a plain row-by-row copy jumps through memory for every pixel. The sound queue is eight frames deep.
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include "../compat/stereo.h"
#define SR 32768
#define NWB 8       // sound buffers queued ahead (one is a frame of 546 samples: about 130 ms in all)
static ndspWaveBuf wb[NWB]; static short*abuf[NWB]; static int audioOk, wbHead;
int plat_init(void){
    osSetSpeedupEnable(true);       // New 3DS: 804 MHz and the extra cache (nothing happens on an old 3DS)
    gfxInitDefault(); gfxSet3D(false); gfxSetDoubleBuffering(GFX_TOP,true); gfxSetDoubleBuffering(GFX_BOTTOM,true);
    if(R_SUCCEEDED(ndspInit())){ ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspChnSetInterp(0,NDSP_INTERP_LINEAR); ndspChnSetRate(0,SR); ndspChnSetFormat(0,NDSP_FORMAT_STEREO_PCM16);
        for(int i=0;i<NWB;i++){ abuf[i]=(short*)linearAlloc(700*4); memset(&wb[i],0,sizeof wb[i]); wb[i].data_vaddr=abuf[i]; wb[i].status=NDSP_WBUF_DONE; } audioOk=1; }
    return 1; }
// keys in the shim's layout (A B SELECT START RIGHT LEFT UP DOWN R L X Y, bit 12 = touch)
int plat_keys(int*tx,int*ty){
    hidScanInput(); u32 k=hidKeysHeld(); int h=(int)(k&0xFFF); if(k&KEY_CPAD_UP) h|=1<<6; if(k&KEY_CPAD_DOWN) h|=1<<7; if(k&KEY_CPAD_LEFT) h|=1<<5; if(k&KEY_CPAD_RIGHT) h|=1<<4;
    if(k&KEY_TOUCH){ touchPosition t; hidTouchRead(&t); int x=t.px-32, y=t.py-24; if(x>=0&&x<256&&y>=0&&y<192){ *tx=x; *ty=y; h|=1<<12; } }
    if(!aptMainLoop()) exit(0); return h; }
// src: W x 240 pixels, RGB, row by row.  fb: the 3DS screen, BGR, column by column, the bottom row first.
static void copyScreen(u8*fb,const unsigned char*src,int W){
    for(int by=0;by<240;by+=8) for(int bx=0;bx<W;bx+=8) for(int x=bx;x<bx+8;x++){ u8*d=&fb[(x*240+(239-by))*3]; const unsigned char*s=&src[(by*W+x)*3];
        for(int y=0;y<8;y++){ d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; d-=3; s+=W*3; } } }
static int topPend, botPend;       // frames still to copy: two after a change (the screen has two buffers)
static int stereoOn; static unsigned char eyeL[400*240*3], eyeR[400*240*3]; static int frameNo, lastStrength=-1;
void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth){
    // the 3D slider: 0 is a flat screen (and the 3D hardware is switched off, as before); above it the top screen is drawn twice, once for each eye (compat/stereo.h)
    float sl=osGet3DSliderState(); int want=depth&&sl>0.03f; if(want!=stereoOn){ stereoOn=want; gfxSet3D(want?true:false); topChanged=1; }
    int strength=want?(int)(sl*24.f+0.5f):0; if(strength!=lastStrength){ lastStrength=strength; topChanged=1; }
    frameNo++; if(want&&(frameNo&3)==0) topChanged=1;                 // the stars around the picture drift, so a still screen is redrawn every few frames
    if(topChanged) topPend=2; if(botChanged) botPend=2; int any=0;
    if(topPend>0){ if(want){ if(topPend==2) stereoTop(eyeL,eyeR,top,depth,frameNo,strength); copyScreen(gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL),eyeL,400); copyScreen(gfxGetFramebuffer(GFX_TOP,GFX_RIGHT,NULL,NULL),eyeR,400); }
        else copyScreen(gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL),top,400); topPend--; any=1; }
    if(botPend>0){ copyScreen(gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,NULL,NULL),bot,320); botPend--; any=1; }
    if(any){ gfxFlushBuffers(); gfxSwapBuffers(); }
    if(audioOk&&frames>0&&frames<=700){ ndspWaveBuf*w=&wb[wbHead]; if(w->status==NDSP_WBUF_DONE||w->status==NDSP_WBUF_FREE){ memcpy(abuf[wbHead],snd,frames*4); w->nsamples=frames; DSP_FlushDataCache(abuf[wbHead],frames*4); ndspChnWaveBufAdd(0,w); wbHead=(wbHead+1)%NWB; } }   // in order, or the sound would play out of turn; a full queue drops the frame's sound
    gspWaitForVBlank(); }
int plat_isNew3DS(void){ bool n=false; APT_CheckNew3DS(&n); return n; }
void plat_exit(void){ if(audioOk) ndspExit(); gfxExit(); }
