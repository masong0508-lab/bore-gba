// ndsshim.h - a small stand-in for the parts of libnds that Bore: GodGuy uses, so the SAME game (source/main.c, DS user interface and all) runs on a PC test harness and on the 3DS.
// It models: the main-engine tile background (64 x 64 map of 8-bit tiles, scrolled) with 128 sprites, the sub-engine bitmap background and text console, the 16 sound channels
// (looped / one-shot 8-bit samples, PSG pulse, noise; linear pan), the keys and the stylus. The 3D engine is not modelled: the 3D globe is switched off in this build.
// The caller supplies   void shimInput(int frame,int*held,int*tx,int*ty)   (keys and stylus for this frame) and   void shimPresent(void)   (one frame is finished).
#ifndef NDSSHIM_H
#define NDSSHIM_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <stdarg.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16; typedef int32_t s32;
#define NDSSHIM 1
#define iprintf shimPrintf
#define siprintf sprintf
#define RGB15(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
// ---- keys
#define KEY_A (1<<0)
#define KEY_B (1<<1)
#define KEY_SELECT (1<<2)
#define KEY_START (1<<3)
#define KEY_RIGHT (1<<4)
#define KEY_LEFT (1<<5)
#define KEY_UP (1<<6)
#define KEY_DOWN (1<<7)
#define KEY_R (1<<8)
#define KEY_L (1<<9)
#define KEY_X (1<<10)
#define KEY_Y (1<<11)
#define KEY_TOUCH (1<<12)
typedef struct { int px, py; } touchPosition;
// ---- video constants (values do not matter)
enum { MODE_0_2D=1, MODE_0_3D, MODE_5_2D, DISPLAY_BG0_ACTIVE=0x100, DISPLAY_BG1_ACTIVE=0x2000, DISPLAY_BG3_ACTIVE=0x200, DISPLAY_SPR_ACTIVE=0x400, DISPLAY_SPR_1D_LAYOUT=0x800, DISPLAY_SPR_1D_SIZE_32=0x1000,
       VRAM_A_MAIN_BG=1, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG, VRAM_D_TEXTURE, BgType_Text8bpp=1, BgType_Text4bpp, BgType_Bmp8, BgSize_T_512x512=1, BgSize_T_256x256, BgSize_B8_256x256,
       SpriteMapping_1D_32=1, SpriteSize_8x8=1, SpriteColorFormat_16Color=1, SoundFormat_8Bit=1, DutyCycle_50=4 };
typedef enum { DutyCycle_12=1, DutyCycle_25=2, DutyCycle_75=6 } ShimDuty;
#define DutyCycle int
// ---- memory the game writes to
static u16 BG_PALETTE[256], BG_PALETTE_SUB[256], SPRITE_PALETTE[256];
static u8 shimTiles[1024*64];
static u16 shimMap[4096];
static u8 shimBmp[256*256];
#define BG_TILE_RAM(n) ((void*)shimTiles)
static u16 shimMap1[1024];
static void*bgGetMapPtr(int id){ return id==1?(void*)shimMap1:(void*)shimMap; }
static void*bgGetGfxPtr(int id){ (void)id; return shimBmp; }
static int shimScrollX, shimScrollY;
static inline void videoSetMode(int m){ (void)m; } static inline void videoSetModeSub(int m){ (void)m; }
static inline void vramSetBankA(int b){ (void)b; } static inline void vramSetBankB(int b){ (void)b; } static inline void vramSetBankC(int b){ (void)b; } static inline void vramSetBankD(int b){ (void)b; }
static int shimSwap;
static inline void lcdMainOnBottom(void){ shimSwap=0; } static inline void lcdMainOnTop(void){ shimSwap=1; }
static inline int bgInit(int l,int t,int s,int m,int tb){ (void)t;(void)s;(void)m;(void)tb; return l; }
static inline void bgSetPriority(int id,int p){ (void)id;(void)p; }
static inline int bgInitSub(int l,int t,int s,int m,int tb){ (void)l;(void)t;(void)s;(void)m;(void)tb; return 3; }
static inline void bgSetScroll(int id,int x,int y){ (void)id; shimScrollX=x; shimScrollY=y; }
static inline void bgUpdate(void){}
static inline void DC_FlushRange(const void*p,u32 n){ (void)p; (void)n; }
#define REG_VCOUNT 0
// ---- sprites
typedef struct { int dummy; } OamState; static OamState oamMain;
typedef struct { int on, x, y, pal, hflip; const u8*gfx; } ShimSpr; static ShimSpr shimSpr[128];
static inline void oamInit(OamState*o,int m,bool e){ (void)o;(void)m;(void)e; memset(shimSpr,0,sizeof shimSpr); }
static u16*oamAllocateGfx(OamState*o,int sz,int fmt){ (void)o;(void)sz;(void)fmt; return (u16*)calloc(1,32); }
static inline void oamSet(OamState*o,int id,int x,int y,int prio,int pal,int sz,int fmt,const void*gfx,int aff,bool dbl,bool hide,bool hflip,bool vflip,bool mosaic){
    (void)o;(void)prio;(void)sz;(void)fmt;(void)aff;(void)dbl;(void)vflip;(void)mosaic; ShimSpr*s=&shimSpr[id]; s->on=!hide; s->x=x; s->y=y; s->pal=pal; s->gfx=(const u8*)gfx; s->hflip=hflip; }
static inline void oamSetHidden(OamState*o,int id,bool h){ (void)o; shimSpr[id].on=!h; }
static inline void oamUpdate(OamState*o){ (void)o; }
// ---- the sub-engine text console (32 x 24)
static char shimCon[24][33]; static int shimRow, shimCol;
static inline void consoleInit(void*a,int b,int c,int d,int e,int f,bool g,bool h){ (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h; }
static inline void consoleClear(void){ for(int r=0;r<24;r++){ memset(shimCon[r],' ',32); shimCon[r][32]=0; } shimRow=shimCol=0; }
static int shimPrintf(const char*fmt,...){
    char buf[512]; va_list ap; va_start(ap,fmt); int n=vsnprintf(buf,sizeof buf,fmt,ap); va_end(ap);
    for(const char*p=buf;*p;p++){ if(*p==27&&p[1]=='['){ int r=0,c=0; p+=2; while(*p>='0'&&*p<='9') r=r*10+*p++-'0'; if(*p==';') p++; while(*p>='0'&&*p<='9') c=c*10+*p++-'0'; shimRow=r; shimCol=c; continue; }
        if(*p=='\n'){ shimRow++; shimCol=0; continue; } if(shimRow>=0&&shimRow<24&&shimCol>=0&&shimCol<32) shimCon[shimRow][shimCol]=*p; shimCol++; }
    return n; }
// ---- input
extern void shimInput(int frame,int*held,int*tx,int*ty);
static int shimFrame, shimHeld, shimPrev, shimTx, shimTy;
static inline void scanKeys(void){ shimPrev=shimHeld; int tx=0,ty=0; shimInput(shimFrame,&shimHeld,&tx,&ty); shimTx=tx; shimTy=ty; }
static inline int keysDown(void){ return shimHeld&~shimPrev; } static inline int keysHeld(void){ return shimHeld; }
static inline void touchRead(touchPosition*t){ t->px=shimTx; t->py=shimTy; }
// ---- files: the SD card is the current directory
static inline int fatInitDefault(void){ return 1; }
#ifndef SAVE_PATH
#define SAVE_PATH "godguy.sav"
#endif
// ---- the 3D engine is not modelled
static u32 shimGfxTex;
#define GFX_TEX_COORD shimGfxTex
#define TEXTURE_PACK(a,b) 0
#define NORMAL_PACK(a,b,c) 0
#define inttot16(x) 0
#define glInit(...) ((void)0)
#define glEnable(...) ((void)0)
#define glClearColor(...) ((void)0)
#define glClearPolyID(...) ((void)0)
#define glClearDepth(...) ((void)0)
#define glViewport(...) ((void)0)
#define glResetTextures(...) ((void)0)
#define glGenTextures(...) ((void)0)
#define glBindTexture(...) ((void)0)
#define glTexImage2D(...) ((void)0)
#define glMatrixMode(...) ((void)0)
#define glLoadIdentity(...) ((void)0)
#define gluPerspective(...) ((void)0)
#define glLight(...) ((void)0)
#define glMaterialf(...) ((void)0)
#define glTranslatef32(...) ((void)0)
#define glRotateX(...) ((void)0)
#define glRotateY(...) ((void)0)
#define glPolyFmt(...) ((void)0)
#define glBegin(...) ((void)0)
#define glEnd(...) ((void)0)
#define glNormal(...) ((void)0)
#define glVertex3v16(...) ((void)0)
#define glFlush(...) ((void)0)
enum { GL_ANTIALIAS=1, GL_TEXTURE_2D, GL_MODELVIEW, GL_PROJECTION, GL_QUAD, GL_RGB, GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_EMISSION, TEXGEN_TEXCOORD, GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T,
       TEXTURE_SIZE_128, POLY_CULL_NONE, POLY_FORMAT_LIGHT0 };
#define POLY_ALPHA(x) 0
// ---- sound: 16 channels mixed in software at 32768 Hz (stereo, 16 bit)
#define SHIM_SR 32768
typedef struct { int on, kind, size, vol, pan, loop, duty; const s8*data; double rate, pos, ph; unsigned lfsr; float cur; } ShimCh;   // kind 1 sample, 2 PSG, 3 noise
static ShimCh shimCh[16]; static int shimChFail;
static inline void soundEnable(void){}
static int shimAlloc(int lo,int hi){ for(int i=lo;i<=hi;i++) if(!shimCh[i].on) return i; shimChFail++; return -1; }
static int soundPlaySample(const void*data,int fmt,u32 size,u16 freq,u8 vol,u8 pan,bool loop,u16 lp){ (void)fmt;(void)lp; int i=shimAlloc(0,15); if(i<0) return -1; ShimCh*c=&shimCh[i]; memset(c,0,sizeof *c); c->on=1; c->kind=1; c->data=(const s8*)data; c->size=(int)size; c->rate=freq; c->vol=vol; c->pan=pan; c->loop=loop; return i; }
static int soundPlayPSG(int duty,u16 freq,u8 vol,u8 pan){ int i=shimAlloc(8,13); if(i<0) return -1; ShimCh*c=&shimCh[i]; memset(c,0,sizeof *c); c->on=1; c->kind=2; c->rate=freq; c->vol=vol; c->pan=pan; c->duty=duty; return i; }
static int soundPlayNoise(u16 freq,u8 vol,u8 pan){ int i=shimAlloc(14,15); if(i<0) return -1; ShimCh*c=&shimCh[i]; memset(c,0,sizeof *c); c->on=1; c->kind=3; c->rate=freq; c->vol=vol; c->pan=pan; c->lfsr=0x7FFF; return i; }
static inline void soundKill(int id){ if(id>=0&&id<16) shimCh[id].on=0; }
static inline void soundSetFreq(int id,u16 f){ if(id>=0&&id<16) shimCh[id].rate=f; }
static inline void soundSetVolume(int id,u8 v){ if(id>=0&&id<16) shimCh[id].vol=v; }
static inline void soundSetPan(int id,u8 p){ if(id>=0&&id<16) shimCh[id].pan=p; }
static double shimPeak; static long shimClip;
static void shimMix(short*out,int frames){   // interleaved stereo
    for(int n=0;n<frames;n++){ float L=0,R=0;
        for(int i=0;i<16;i++){ ShimCh*c=&shimCh[i]; if(!c->on||c->vol<=0) { if(c->on&&c->kind==1&&c->vol<=0){ c->pos+=c->rate/SHIM_SR; if(c->loop) { if(c->pos>=c->size) c->pos-=c->size*(int)(c->pos/c->size); } else if(c->pos>=c->size) c->on=0; } continue; } float s=0;
            if(c->kind==1){ c->pos+=c->rate/SHIM_SR; if(c->pos>=c->size){ if(c->loop) c->pos-=c->size*(int)(c->pos/c->size); else { c->on=0; continue; } } s=c->data[(int)c->pos]/127.f; }
            else if(c->kind==2){ c->ph+=c->rate/SHIM_SR; c->ph-=floor(c->ph); float d=c->duty==1?0.125f:c->duty==2?0.25f:c->duty==6?0.75f:0.5f; s=c->ph<d?1.f:-1.f; }
            else { c->ph+=c->rate/SHIM_SR; while(c->ph>=1){ c->ph-=1; unsigned x=((c->lfsr^(c->lfsr>>1))&1); c->lfsr=(c->lfsr>>1)|(x<<14); c->cur=(c->lfsr&1)?1.f:-1.f; } s=c->cur; }
            s*=c->vol/127.f; L+=s*(128-c->pan)/128.f; R+=s*c->pan/128.f; }
        if(fabs(L)>shimPeak) shimPeak=fabs(L); if(fabs(R)>shimPeak) shimPeak=fabs(R);
        if(L>1.f){ L=1.f; shimClip++; } if(L<-1.f){ L=-1.f; shimClip++; } if(R>1.f){ R=1.f; shimClip++; } if(R<-1.f){ R=-1.f; shimClip++; }
        out[n*2]=(short)(L*30000); out[n*2+1]=(short)(R*30000); } }
extern void shimPresent(void);
static inline void swiWaitForVBlank(void){ shimFrame++; shimPresent(); }
// ---- drawing: both screens into RGB buffers (row-major). top 400 x 240 (the sub engine: bitmap + text), bottom 320 x 240 (the main engine: tiles + sprites)
#include "font5x7.h"
static inline void shimRGB(u16 c,u8*o){ o[0]=(u8)((c&31)*255/31); o[1]=(u8)(((c>>5)&31)*255/31); o[2]=(u8)(((c>>10)&31)*255/31); }
static void shimRender(u8*top,u8*bot){
    memset(top,0,400*240*3); memset(bot,0,320*240*3);
    u8*mb=shimSwap?top:bot; int mw=shimSwap?400:320, mox=shimSwap?72:32; u8*sb=shimSwap?bot:top; int sw=shimSwap?320:400, sox=shimSwap?32:72;   // which screen shows which engine
    // bottom: BG0 + sprites at (32,24)
    for(int sy=0;sy<192;sy++) for(int sx=0;sx<256;sx++){ int wx=(sx+shimScrollX)&511, wy=(sy+shimScrollY)&511, mx=wx>>3, my=wy>>3; u16 e=shimMap[((my>=32?2:0)+(mx>=32?1:0))*1024+(my&31)*32+(mx&31)]; int idx=shimTiles[(e&0x3FF)*64+(wy&7)*8+(wx&7)];
        shimRGB(BG_PALETTE[idx],&mb[((24+sy)*mw+mox+sx)*3]); }
    for(int sy=0;sy<192;sy++) for(int sx=0;sx<256;sx++){ u16 e=shimMap1[(sy>>3)*32+(sx>>3)]; int idx=shimTiles[(e&0x3FF)*64+(sy&7)*8+(sx&7)]; if(idx) shimRGB(BG_PALETTE[idx],&mb[((24+sy)*mw+mox+sx)*3]); }   // BG1 (the toolbar) over the world, under nothing
    for(int n=127;n>=0;n--){ const ShimSpr*s=&shimSpr[n]; if(!s->on||!s->gfx) continue; for(int y=0;y<8;y++) for(int x=0;x<8;x++){ int xx=s->hflip?7-x:x; int b=s->gfx[y*4+xx/2]; int idx=(xx&1)?(b>>4):(b&15); if(!idx) continue; int px=s->x+x, py=s->y+y; if(px<0||py<0||px>=256||py>=192) continue;
            shimRGB(SPRITE_PALETTE[s->pal*16+idx],&mb[((24+py)*mw+mox+px)*3]); } }
    // top: bitmap + text at (72,24)
    for(int sy=0;sy<192;sy++) for(int sx=0;sx<256;sx++){ int idx=shimBmp[sy*256+sx]; if(idx) shimRGB(BG_PALETTE_SUB[idx],&sb[((24+sy)*sw+sox+sx)*3]); }
    for(int r=0;r<24;r++) for(int c=0;c<32;c++){ int ch=shimCon[r][c]; if(ch>='a'&&ch<='z') ch-=32; if(ch<32||ch>=96) continue; const unsigned char*g=font5x7[ch-32];
        for(int y=0;y<7;y++) for(int x=0;x<5;x++) if((g[y]>>(4-x))&1){ u8*o=&sb[((24+r*8+y)*sw+sox+c*8+x+1)*3]; o[0]=o[1]=o[2]=255; } }
}
#endif
