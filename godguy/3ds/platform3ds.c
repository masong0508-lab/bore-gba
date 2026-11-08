// platform3ds.c - the 3DS side of the port (libctru): keys, touch, the two screens and the sound. It is kept apart from the game's translation unit because libctru's KEY_ names clash
// with the shim's. The game itself (source/main.c) runs unchanged through compat/ndsshim.h: see main3ds.c.
#include <3ds.h>
#include <string.h>
#include <stdlib.h>
#define SR 32768
static ndspWaveBuf wb[4]; static short*abuf[4]; static int audioOk;
int plat_init(void){
    gfxInitDefault(); gfxSet3D(false); gfxSetDoubleBuffering(GFX_TOP,true); gfxSetDoubleBuffering(GFX_BOTTOM,true);
    if(R_SUCCEEDED(ndspInit())){ ndspSetOutputMode(NDSP_OUTPUT_STEREO); ndspChnSetInterp(0,NDSP_INTERP_LINEAR); ndspChnSetRate(0,SR); ndspChnSetFormat(0,NDSP_FORMAT_STEREO_PCM16);
        for(int i=0;i<4;i++){ abuf[i]=(short*)linearAlloc(700*4); memset(&wb[i],0,sizeof wb[i]); wb[i].data_vaddr=abuf[i]; wb[i].status=NDSP_WBUF_DONE; } audioOk=1; }
    return 1; }
// keys in the shim's layout (A B SELECT START RIGHT LEFT UP DOWN R L X Y, bit 12 = touch)
int plat_keys(int*tx,int*ty){
    hidScanInput(); u32 k=hidKeysHeld(); int h=(int)(k&0xFFF); if(k&KEY_CPAD_UP) h|=1<<6; if(k&KEY_CPAD_DOWN) h|=1<<7; if(k&KEY_CPAD_LEFT) h|=1<<5; if(k&KEY_CPAD_RIGHT) h|=1<<4;
    if(k&KEY_TOUCH){ touchPosition t; hidTouchRead(&t); int x=t.px-32, y=t.py-24; if(x>=0&&x<256&&y>=0&&y<192){ *tx=x; *ty=y; h|=1<<12; } }
    if(!aptMainLoop()) exit(0); return h; }
void plat_present(const unsigned char*top,const unsigned char*bot,const short*snd,int frames){
    u8*fb=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,NULL,NULL);     // 400 x 240, BGR8, columns first
    for(int x=0;x<400;x++) for(int y=0;y<240;y++){ const unsigned char*s=&top[(y*400+x)*3]; u8*d=&fb[(x*240+(239-y))*3]; d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; }
    fb=gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,NULL,NULL);
    for(int x=0;x<320;x++) for(int y=0;y<240;y++){ const unsigned char*s=&bot[(y*320+x)*3]; u8*d=&fb[(x*240+(239-y))*3]; d[0]=s[2]; d[1]=s[1]; d[2]=s[0]; }
    gfxFlushBuffers(); gfxSwapBuffers();
    if(audioOk&&frames>0&&frames<=700){ for(int i=0;i<4;i++) if(wb[i].status==NDSP_WBUF_DONE||wb[i].status==NDSP_WBUF_FREE){ memcpy(abuf[i],snd,frames*4); wb[i].nsamples=frames; DSP_FlushDataCache(abuf[i],frames*4); ndspChnWaveBufAdd(0,&wb[i]); break; } }
    gspWaitForVBlank(); }
void plat_exit(void){ if(audioOk) ndspExit(); gfxExit(); }
