// platform3ds.c - the 3DS side of the port (libctru): keys, touch, the two screens and the sound. It is kept apart from the game's translation unit because libctru's KEY_ names clash
// with the shim's. The game itself (source/main.c) runs unchanged through compat/ndsshim.h: see main3ds.c.
// Speed: the New 3DS's faster clock is switched on; a screen is copied only when the game's picture changed (and for the two frames its two buffers need), and the copy walks 8 x 8 blocks,
// because the 3DS screens are stored column by column and a plain row-by-row copy jumps through memory for every pixel. The sound queue is twelve frames deep.
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../compat/stereo.h"
#define SR 32768
#define NWB 12       // sound buffers queued ahead (one is a frame of 546 samples: about 200 ms in all)
static ndspWaveBuf wb[NWB]; static short*abuf[NWB];
static int audioOk,dspReady,wbHead,platformReady;
static int platCinema,topPend,botPend,stereoOn,frameNo,lastStrength=-1;
static void plat_audioExit(void){
    // Stop the DSP before releasing memory it may still be playing.
    if(dspReady)ndspExit();
    dspReady=audioOk=0;
    for(int i=0;i<NWB;i++){
        if(abuf[i])linearFree(abuf[i]);
        abuf[i]=NULL;
    }
    memset(wb,0,sizeof wb);wbHead=0;
}
int plat_init(void){
    if(platformReady)return 1;
    osSetSpeedupEnable(true);       // New 3DS: 804 MHz and the extra cache (nothing happens on an old 3DS)
    gfxInitDefault(); gfxSet3D(false); gfxSetDoubleBuffering(GFX_TOP,true); gfxSetDoubleBuffering(GFX_BOTTOM,true);
    platformReady=1;
    platCinema=topPend=botPend=stereoOn=frameNo=0;lastStrength=-1;
    if(!R_SUCCEEDED(ndspInit()))return 1;
    dspReady=1;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspChnSetInterp(0,NDSP_INTERP_LINEAR); ndspChnSetRate(0,SR); ndspChnSetFormat(0,NDSP_FORMAT_STEREO_PCM16);
    for(int i=0;i<NWB;i++){
        abuf[i]=(short*)linearAlloc(700*4);
        if(!abuf[i]){
            // A partial pool cannot safely queue audio. Release it and
            // keep graphics/input playable, as when DSP startup fails.
            plat_audioExit();return 1;
        }
        memset(&wb[i],0,sizeof wb[i]);
        wb[i].data_vaddr=abuf[i];wb[i].status=NDSP_WBUF_DONE;
    }
    audioOk=1;
    return 1;
}
// During cinematics the 320x240 touch display uses a sharp 5:4 scale.
// Touches map back to the same 256x192 film coordinates as the DS version.
void plat_setCinema(int on){platCinema=on!=0;}
// keys in the shim's layout (A B SELECT START RIGHT LEFT UP DOWN R L X Y, bit 12 = touch)
int plat_keys(int*tx,int*ty){
    hidScanInput(); u32 k=hidKeysHeld(); int h=(int)(k&0xFFF); if(k&KEY_CPAD_UP) h|=1<<6; if(k&KEY_CPAD_DOWN) h|=1<<7; if(k&KEY_CPAD_LEFT) h|=1<<5; if(k&KEY_CPAD_RIGHT) h|=1<<4;
    if(k&KEY_TOUCH){
        touchPosition t;hidTouchRead(&t);
        int x=platCinema?t.px*4/5:t.px-32;
        int y=platCinema?t.py*4/5:t.py-24;
        if(x>=0&&x<256&&y>=0&&y<192){ *tx=x; *ty=y; h|=1<<12; }
    }
    if(!aptMainLoop()) exit(0); return h; }
// src: W x 240 pixels, RGB, row by row.  fb: the 3DS screen, BGR, column by column, the bottom row first.
static void copyScreen(u8*fb,const unsigned char*src,int W){
    for(int by=0;by<240;by+=8) for(int bx=0;bx<W;bx+=8) for(int x=bx;x<bx+8;x++){ u8*d=&fb[(x*240+(239-by))*3]; const unsigned char*s=&src[(by*W+x)*3];
        for(int y=0;y<8;y++){ d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; d-=3; s+=W*3; } } }
// topPend/botPend track two copies after a change, one per screen buffer.
static unsigned char eyeL[400*240*3],eyeR[400*240*3];
void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth){
    if(!platformReady)return;
    // the 3D slider: 0 is a flat screen (and the 3D hardware is switched off, as before); above it the top screen is drawn twice, once for each eye (compat/stereo.h)
    float sl=osGet3DSliderState(); int want=depth&&sl>0.03f; if(want!=stereoOn){ stereoOn=want; gfxSet3D(want?true:false); topChanged=1; }
    int strength=want?(int)(sl*24.f+0.5f):0; if(strength!=lastStrength){ lastStrength=strength; topChanged=1; }
    frameNo++; if(want&&(frameNo&3)==0) topChanged=1;                 // the stars around the picture drift, so a still screen is redrawn every few frames
    if(topChanged) topPend=2; if(botChanged) botPend=2; int any=0;
    if(topPend>0){ if(want){ if(topPend==2) stereoTop(eyeL,eyeR,top,depth,frameNo,strength); copyScreen(gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL),eyeL,400); copyScreen(gfxGetFramebuffer(GFX_TOP,GFX_RIGHT,NULL,NULL),eyeR,400); }
        else copyScreen(gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL),top,400); topPend--; any=1; }
    if(botPend>0){ copyScreen(gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,NULL,NULL),bot,320); botPend--; any=1; }
    if(any){ gfxFlushBuffers(); gfxSwapBuffers(); }
    if(audioOk&&snd){ int done=0; while(done<frames){ int c=frames-done>700?700:frames-done; ndspWaveBuf*w=&wb[wbHead]; if(!(w->status==NDSP_WBUF_DONE||w->status==NDSP_WBUF_FREE)) break;       // (a full queue drops the rest: it can only be a stall)
            memcpy(abuf[wbHead],snd+done*2,c*4); w->nsamples=c; DSP_FlushDataCache(abuf[wbHead],c*4); ndspChnWaveBufAdd(0,w); wbHead=(wbHead+1)%NWB; done+=c; } }   // in order, or the sound would play out of turn
    gspWaitForVBlank(); }
unsigned long long plat_ms(void){ return osGetTime(); }
int plat_isNew3DS(void){ bool n=false; APT_CheckNew3DS(&n); return n; }
// Fail safely on an Old-series machine instead of launching an
// untested 400x240 software mesh/mixer profile and crashing. New 2DS
// XL reports the same New-series hardware capability as New 3DS.
void plat_new3dsRequired(void){
    consoleInit(GFX_BOTTOM,NULL);
    puts("GODGUY - NEW 3DS EDITION");
    puts("");
    puts("This version requires:");
    puts(" New Nintendo 3DS / 3DS XL");
    puts(" or New Nintendo 2DS XL");
    puts("");
    puts("Original 3DS / 2DS unsupported.");
    puts("Press START or B to exit.");
    while(aptMainLoop()){
        hidScanInput();
        u32 down=hidKeysDown();
        if(down&(KEY_START|KEY_B))break;
        gspWaitForVBlank();
    }
}
void plat_exit(void){
    if(!platformReady)return;
    plat_audioExit();gfxExit();platformReady=0;
}
