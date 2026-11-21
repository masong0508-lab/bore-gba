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
static u8 shimBmp[256*256]; static signed char shimDepth[256*192];
#ifdef __3DS__
// Cinema is originally authored at 256x192. Display it on the 3DS at
// exactly 320x240 (5:4 nearest-neighbour scaling), centered on the wide
// 400x240 top display and full-screen on the 320x240 touch display.
static int shimFilmFullscreen;
// Externally composited top-LCD views (3DS walkable voxel mode) must
// invalidate the cached ordinary HUD image when returning to normal play.
static int shimExternalInvalidate;
#endif      // shimDepth: a level for each pixel of the top picture, for the 3DS's 3D screen (see compat/stereo.h)
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
#ifdef SHIM_DISTORT
// PC model of a panel whose readings do not map linearly onto the screen (DraStic: the bottom reads about 14 px too high until calibrated)
#define HW_CAL 1
static int hwCalOn, hwCal[8], hwRx, hwRy;
#define HWCAL_MAGIC 0x4C414347u   /* "GCAL" */
static int hwUiScale;
static void hwCalPack(u8*o){ u32 r[11]; r[0]=HWCAL_MAGIC; u32 c=0xA5A5A5A5u; for(int i=0;i<8;i++){ r[1+i]=hwCalOn?(u32)hwCal[i]:0u; c=c*31u+r[1+i]; } r[9]=(u32)hwUiScale|(hwCalOn?0x100u:0u); c=c*31u+r[9]; r[10]=c; memcpy(o,r,44); }
static int hwCalUnpack(const u8*in){ u32 r[11]; memcpy(r,in,44); if(r[0]!=HWCAL_MAGIC) return 0; u32 c=0xA5A5A5A5u; for(int i=0;i<8;i++) c=c*31u+r[1+i]; u32 c0=c; c=c*31u+r[9];
    if(c0==r[9]){ r[9]=0x100u; }   // a record written before UI SCALE existed: ten words, calibration only
    else if(c!=r[10]) return 0;
    int v[8]; for(int i=0;i<8;i++) v[i]=(int)r[1+i]; int dx=v[4]-v[0], dy=v[5]-v[1]; hwUiScale=(int)(r[9]&3)>2?0:(int)(r[9]&3);
    if(r[9]&0x100u){ if(dx<2200||dx>4800||dy<2000||dy>4800) return 0; for(int i=0;i<8;i++) hwCal[i]=v[i]; hwCalOn=1; } return 1; }
static inline void hwMapRaw(int rx,int ry,int*px,int*py){ int x=hwCal[2]+(rx-hwCal[0])*(hwCal[6]-hwCal[2])/(hwCal[4]-hwCal[0]), y=hwCal[3]+(ry-hwCal[1])*(hwCal[7]-hwCal[3])/(hwCal[5]-hwCal[1]); *px=x<0?0:x>255?255:x; *py=y<0?0:y>191?191:y; }
static int hwCalLoad(void){ FILE*f=fopen("godguy.cal","rb"); if(!f) return 0; u8 b[44]; memset(b,0,44); int n=(int)fread(b,1,44,f); fclose(f); return n>=40&&hwCalUnpack(b)&&hwCalOn; }
static void hwCalStore(void){ u8 b[44]; hwCalPack(b); FILE*f=fopen("godguy.cal","wb"); if(f){ fwrite(b,1,44,f); fclose(f); } }
static inline void scanKeys(void){ shimPrev=shimHeld; int tx=0,ty=0; shimInput(shimFrame,&shimHeld,&tx,&ty); if(shimHeld&KEY_TOUCH){ hwRx=528+(tx-32)*1640/100; hwRy=352+(ty-32)*216/10; if(hwCalOn) hwMapRaw(hwRx,hwRy,&tx,&ty); else { tx=hwRx>>4; ty=hwRy*192/4096; if(tx>255) tx=255; if(ty>191) ty=191; } } shimTx=tx; shimTy=ty; }
#else
static inline void scanKeys(void){ shimPrev=shimHeld; int tx=0,ty=0; shimInput(shimFrame,&shimHeld,&tx,&ty); shimTx=tx; shimTy=ty; }
#endif
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
static int shimMusicTicks=1;      // how many 1/60 s the music should advance this frame (the 3DS sets it from the real time elapsed, so a slow frame does not slow the song or starve the sound)
#ifdef SHIM_HQ
#define SHIM_NCH 24      // the 3DS mixes 24 channels: the music, its drums and the vocal phrases each keep their own, and the effects have theirs
#define SHIM_PSG_LO 20
#define SHIM_NOI_LO 22
#define SHIM_SMP_HI 19
#else
#define SHIM_NCH 16
#define SHIM_PSG_LO 8
#define SHIM_NOI_LO 14
#define SHIM_SMP_HI 15
#endif
static int shimNextBus, shimNextKick, shimTempoBpm;      // set by the game just before it starts a sample: it is a vocal phrase (the vocal bus: echo and room of its own) / a kick (it ducks the rest: sidechain); the song's tempo
typedef struct { int bus, kick; int on, kind, size, vol, pan, loop, duty; const s8*data; double rate, pos, ph; unsigned lfsr; float cur; } ShimCh;   // kind 1 sample, 2 PSG, 3 noise
static ShimCh shimCh[SHIM_NCH]; static int shimChFail;
static inline void soundEnable(void){}
static int shimAlloc(int lo,int hi){ for(int i=lo;i<=hi;i++) if(!shimCh[i].on) return i; shimChFail++; return -1; }
static int soundPlaySample(const void*data,int fmt,u32 size,u16 freq,u8 vol,u8 pan,bool loop,u16 lp){ (void)fmt;(void)lp; int i=shimAlloc(0,SHIM_SMP_HI); if(i<0) return -1; ShimCh*c=&shimCh[i]; memset(c,0,sizeof *c); c->bus=shimNextBus; c->kick=shimNextKick; shimNextBus=shimNextKick=0; c->on=1; c->kind=1; c->data=(const s8*)data; c->size=(int)size; c->rate=freq; c->vol=vol; c->pan=pan; c->loop=loop; return i; }
static int soundPlayPSG(int duty,u16 freq,u8 vol,u8 pan){ int i=shimAlloc(SHIM_PSG_LO,SHIM_PSG_LO+1); if(i<0) return -1; ShimCh*c=&shimCh[i]; memset(c,0,sizeof *c); c->on=1; c->kind=2; c->rate=freq; c->vol=vol; c->pan=pan; c->duty=duty; return i; }
static int soundPlayNoise(u16 freq,u8 vol,u8 pan){ int i=shimAlloc(SHIM_NOI_LO,SHIM_NOI_LO+1); if(i<0) return -1; ShimCh*c=&shimCh[i]; memset(c,0,sizeof *c); c->on=1; c->kind=3; c->rate=freq; c->vol=vol; c->pan=pan; c->lfsr=0x7FFF; return i; }
static inline void soundKill(int id){ if(id>=0&&id<SHIM_NCH) shimCh[id].on=0; }
static inline void soundSetFreq(int id,u16 f){ if(id>=0&&id<SHIM_NCH) shimCh[id].rate=f; }
static inline void soundSetVolume(int id,u8 v){ if(id>=0&&id<SHIM_NCH) shimCh[id].vol=v; }
static inline void soundSetPan(int id,u8 p){ if(id>=0&&id<SHIM_NCH) shimCh[id].pan=p; }
#ifdef SHIM_HQ
// ---- the HIGH-QUALITY mixer (the 3DS build; -DSHIM_HQ): the same 16 channels, mixed the way a better machine can afford.
//   * samples and the 64-step waves are read with cubic (Hermite) interpolation instead of nearest-neighbour, so the waves lose their aliasing grit and the drums their hiss;
//   * a looped wave (a melodic voice) is played as THREE slightly detuned copies spread across the stereo field: a unison / chorus that makes every pad, lead and arp wider and fuller;
//   * volume and pan are smoothed over about 1.5 ms (no clicks at note starts or on voice changes) and panned with an equal-power law;
//   * the square wave is band-limited (PolyBLEP) and the noise is softened;
//   * a Freeverb-style stereo reverb (8 combs and 4 all-passes a side) gives the whole mix a room, and a soft-knee limiter replaces the hard clip.
static float shimPanL[128], shimPanR[128]; static int shimHqInit;
typedef struct { float sv, sp, pos2, pos3; } ShimHq; static ShimHq shimHq[SHIM_NCH];
#define SHQ_NC 8
#define SHQ_NA 4
static const int shqComb[SHQ_NC]={829,883,949,1007,1057,1108,1157,1202}, shqAp[SHQ_NA]={413,328,253,167};
static float shqCB[2][SHQ_NC][1300], shqCS[2][SHQ_NC], shqAB[2][SHQ_NA][450]; static int shqCI[2][SHQ_NC], shqAI[2][SHQ_NA];
static float shqWet=0.20f, shqFb=0.80f, shqDamp=0.30f;
static inline float shqBlep(float t,float dt){ if(t<dt){ t/=dt; return t+t-t*t-1.f; } if(t>1.f-dt){ t=(t-1.f)/dt; return t*t+t+t+1.f; } return 0.f; }
static inline float shqCubic(const s8*d,int size,int loop,float pos){
    int p=(int)pos; float f=pos-p; int i0=p-1,i1=p,i2=p+1,i3=p+2;
    if(loop){ if(!(size&(size-1))){ int m=size-1; i0&=m; i1&=m; i2&=m; i3&=m; } else { i0=(i0%size+size)%size; i1%=size; i2%=size; i3%=size; } } else { if(i0<0) i0=0; if(i2>=size) i2=size-1; if(i3>=size) i3=size-1; if(i1>=size) i1=size-1; }
    float y0=d[i0],y1=d[i1],y2=d[i2],y3=d[i3];
    float c1=0.5f*(y2-y0), c2=y0-2.5f*y1+2.f*y2-0.5f*y3, c3=0.5f*(y3-y0)+1.5f*(y1-y2);
    return ((c3*f+c2)*f+c1)*f+y1; }
static double shimPeak; static long shimClip;
static float shqReverb(int side,float in){
    float out=0;
    for(int k=0;k<SHQ_NC;k++){ int n=shqComb[k]+(side?23:0); if(n>1299) n=1299; float*b=shqCB[side][k]; int*ix=&shqCI[side][k]; float y=b[*ix]; shqCS[side][k]=y*(1.f-shqDamp)+shqCS[side][k]*shqDamp; b[*ix]=in+shqCS[side][k]*shqFb+1e-18f; if(++*ix>=n) *ix=0; out+=y; }
    for(int k=0;k<SHQ_NA;k++){ int n=shqAp[k]+(side?11:0); if(n>449) n=449; float*b=shqAB[side][k]; int*ix=&shqAI[side][k]; float y=b[*ix]; float o=-out+y; b[*ix]=out+y*0.5f+1e-18f; out=o; if(++*ix>=n) *ix=0; }
    return out; }
static float shimDuck, shimCmpE; static float shqDL[2][24000], shqDLP[2], shqVin; static int shqDI; 
#define SHQ_DL 24000
static void shimMixHQ(short*out,int frames){   // interleaved stereo
    if(!shimHqInit){ shimHqInit=1; for(int i=0;i<128;i++){ float th=i/127.f*1.5707963f; shimPanL[i]=cosf(th)*0.7071f; shimPanR[i]=sinf(th)*0.7071f; } }
    if(frames<=0) return;
    // Channel settings and tempo are fixed until the next buffer. Prepare
    // them once, retaining channel order and the same per-sample arithmetic.
    struct { int index, bass; float volume, pan, step; } active[SHIM_NCH];
    int activeCount=0;
    for(int i=0;i<SHIM_NCH;i++){
        const ShimCh*c=&shimCh[i];
        if(!c->on){ shimHq[i].sv=0; continue; }
        active[activeCount].index=i;
        active[activeCount].volume=c->vol<=0?0.f:c->vol/127.f;
        active[activeCount].pan=(float)c->pan;
        active[activeCount].step=(float)(c->rate/SHIM_SR);
        active[activeCount].bass=c->kind==1&&c->loop&&c->size<=256&&c->rate/c->size<170.0;
        activeCount++;
    }
    int dly=shimTempoBpm>0?(int)(60.0/shimTempoBpm*0.75*SHIM_SR):12288;
    if(dly>SHQ_DL-2) dly=SHQ_DL-2;
    if(dly<2000) dly=2000;
    for(int n=0;n<frames;n++){ float L=0,R=0,vL=0,vR=0;
        for(int k=0;k<activeCount;k++){ int i=active[k].index; ShimCh*c=&shimCh[i]; ShimHq*h=&shimHq[i]; if(!c->on){ h->sv=0; continue; }
            float tv=active[k].volume; h->sv+=(tv-h->sv)*0.045f; if(tv==0.f&&h->sv<1e-4f) h->sv=0.f; h->sp+=(active[k].pan-h->sp)*0.045f; float dt=active[k].step; float v=h->sv; int pi=(int)(h->sp+0.5f); if(pi<0) pi=0; if(pi>127) pi=127;
            if(c->kind==1){
                c->pos+=dt; if(c->pos>=c->size){ if(c->loop){ while(c->pos>=c->size) c->pos-=c->size; } else { c->on=0; continue; } }
                if(v<0.0005f){ if(c->loop){ h->pos2+=dt*1.0028f; h->pos3+=dt*0.9969f; } continue; }
                if(c->loop&&c->size<=256){ h->pos2+=dt*1.0028f; h->pos3+=dt*0.9969f; while(h->pos2>=c->size) h->pos2-=c->size; while(h->pos3>=c->size) h->pos3-=c->size;
                    float s1=shqCubic(c->data,c->size,1,c->pos)/127.f, s2=shqCubic(c->data,c->size,1,h->pos2)/127.f, s3=shqCubic(c->data,c->size,1,h->pos3)/127.f; int pa=pi-22<0?0:pi-22, pb=pi+22>127?127:pi+22;
                    if(active[k].bass){ s1=0.5f*s1+0.7f*tanhf(2.4f*s1); s2=0.5f*s2+0.7f*tanhf(2.4f*s2); s3=0.5f*s3+0.7f*tanhf(2.4f*s3); }      // (a bass: tape-style drive adds the harmonics a small speaker can actually play)
                    v*=1.f-0.32f*shimDuck;      // (sidechain: every kick pushes the melodic voices down and lets them swell back)
                    L+=v*(s1*0.60f*shimPanL[pi]+s2*0.30f*shimPanL[pa]+s3*0.30f*shimPanL[pb]); R+=v*(s1*0.60f*shimPanR[pi]+s2*0.30f*shimPanR[pa]+s3*0.30f*shimPanR[pb]); }
                else { if(c->kick&&c->pos<=dt*2.0+0.5) shimDuck=1.f; float s=shqCubic(c->data,c->size,c->loop,c->pos)/127.f*v; if(c->bus){ vL+=s*shimPanL[pi]; vR+=s*shimPanR[pi]; } else { L+=s*shimPanL[pi]; R+=s*shimPanR[pi]; } } }
            else if(c->kind==2){ c->ph+=dt; c->ph-=floor(c->ph); float d=c->duty==1?0.125f:c->duty==2?0.25f:c->duty==6?0.75f:0.5f; float ph=(float)c->ph; float s=ph<d?1.f:-1.f; float dtf=dt>0.45f?0.45f:dt; s+=shqBlep(ph,dtf); float p2=ph-d; if(p2<0) p2+=1.f; s-=shqBlep(p2,dtf); s*=v; L+=s*shimPanL[pi]; R+=s*shimPanR[pi]; }
            else { c->ph+=dt; while(c->ph>=1){ c->ph-=1; unsigned x=((c->lfsr^(c->lfsr>>1))&1); c->lfsr=(c->lfsr>>1)|(x<<14); c->cur=(c->lfsr&1)?1.f:-1.f; } h->pos2+=(c->cur-h->pos2)*0.45f; float s=h->pos2*v; L+=s*shimPanL[pi]; R+=s*shimPanR[pi]; } }
        shimDuck*=0.99955f;
        {   // the VOCAL BUS: a ping-pong echo of a dotted eighth at the song's tempo (it answers from the other side, darker each time) and a bigger room than the band has
            int rd=shqDI-dly; if(rd<0) rd+=SHQ_DL;
            float tl=shqDL[0][rd], tr=shqDL[1][rd], vin=(vL+vR)*0.5f; shqDLP[0]+=0.45f*(tl-shqDLP[0]); shqDLP[1]+=0.45f*(tr-shqDLP[1]);
            shqDL[0][shqDI]=vin*0.8f+shqDLP[1]*0.40f+1e-18f; shqDL[1][shqDI]=shqDLP[0]*0.40f+1e-18f; if(++shqDI>=SHQ_DL) shqDI=0;
            L+=vL*0.9f+tl*0.24f; R+=vR*0.9f+tr*0.24f; vin+=(tl+tr)*0.1f; shqVin=vin; }
        float in=(L+R)*0.5f*0.06f+shqVin*0.20f, wl=shqReverb(0,in)*shqWet, wr=shqReverb(1,in)*shqWet; L=(L+wl)*1.2f; R=(R+wr)*1.2f;
        { float pk=fabsf(L)>fabsf(R)?fabsf(L):fabsf(R); if(pk>shimCmpE) shimCmpE=pk; else shimCmpE*=0.9997f; if(shimCmpE>0.72f){ float g=0.72f/shimCmpE; L*=g; R*=g; } }      // (a master compressor: the louder the band and the vocal get together, the more it holds them back)
        if(fabsf(L)>shimPeak) shimPeak=fabsf(L); if(fabsf(R)>shimPeak) shimPeak=fabsf(R);
        float a=fabsf(L); if(a>0.7f){ float y=0.7f+0.3f*tanhf((a-0.7f)/0.3f); L=L<0?-y:y; shimClip+=(a>1.2f); } a=fabsf(R); if(a>0.7f){ float y=0.7f+0.3f*tanhf((a-0.7f)/0.3f); R=R<0?-y:y; shimClip+=(a>1.2f); }
        out[n*2]=(short)(L*30000); out[n*2+1]=(short)(R*30000); } }
#else
static double shimPeak; static long shimClip;
#endif
static void shimMixBasic(short*out,int frames){   // interleaved stereo
    for(int n=0;n<frames;n++){ float L=0,R=0;
        for(int i=0;i<SHIM_NCH;i++){ ShimCh*c=&shimCh[i]; if(!c->on||c->vol<=0) { if(c->on&&c->kind==1&&c->vol<=0){ c->pos+=c->rate/SHIM_SR; if(c->loop) { if(c->pos>=c->size) c->pos-=c->size*(int)(c->pos/c->size); } else if(c->pos>=c->size) c->on=0; } continue; } float s=0;
            if(c->kind==1){ c->pos+=c->rate/SHIM_SR; if(c->pos>=c->size){ if(c->loop) c->pos-=c->size*(int)(c->pos/c->size); else { c->on=0; continue; } } s=c->data[(int)c->pos]/127.f; }
            else if(c->kind==2){ c->ph+=c->rate/SHIM_SR; c->ph-=floor(c->ph); float d=c->duty==1?0.125f:c->duty==2?0.25f:c->duty==6?0.75f:0.5f; s=c->ph<d?1.f:-1.f; }
            else { c->ph+=c->rate/SHIM_SR; while(c->ph>=1){ c->ph-=1; unsigned x=((c->lfsr^(c->lfsr>>1))&1); c->lfsr=(c->lfsr>>1)|(x<<14); c->cur=(c->lfsr&1)?1.f:-1.f; } s=c->cur; }
            s*=c->vol/127.f; L+=s*(128-c->pan)/128.f; R+=s*c->pan/128.f; }
        if(fabs(L)>shimPeak) shimPeak=fabs(L); if(fabs(R)>shimPeak) shimPeak=fabs(R);
        if(L>1.f){ L=1.f; shimClip++; } if(L<-1.f){ L=-1.f; shimClip++; } if(R>1.f){ R=1.f; shimClip++; } if(R<-1.f){ R=-1.f; shimClip++; }
        out[n*2]=(short)(L*30000); out[n*2+1]=(short)(R*30000); } }
#ifdef SHIM_HQ
static int shimHqOn=1;      // the platform may switch the rich mixer off (an old 3DS has the cycles for the game, not for it)
static void shimMix(short*out,int frames){ if(shimHqOn) shimMixHQ(out,frames); else shimMixBasic(out,frames); }
#else
static void shimMix(short*out,int frames){ shimMixBasic(out,frames); }
#endif
extern void shimPresent(void);
static inline void swiWaitForVBlank(void){ shimFrame++; shimPresent(); }
// ---- drawing: both screens into RGB buffers (row-major). top 400 x 240 (the sub engine: bitmap + text), bottom 320 x 240 (the main engine: tiles + sprites)
#include "font5x7.h"
static inline void shimRGB(u16 c,u8*o){ o[0]=(u8)((c&31)*255/31); o[1]=(u8)(((c>>5)&31)*255/31); o[2]=(u8)(((c>>10)&31)*255/31); }
static void shimRenderRef(u8*top,u8*bot){   // the plain renderer: every pixel of both screens, every time (kept as the reference the fast one is tested against)
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

// ---- the fast renderer. The same picture as shimRenderRef, byte for byte (pc_speed.c checks it), made quicker three ways: palettes are turned into RGB once (a table, rebuilt only when a palette changed),
// the map and the tile rows are looked up once per 8 pixels instead of once per pixel, and each engine's inputs (tiles, maps, palettes, sprites, scroll, the bitmap and the text) are hashed, so an engine that did not change
// is not drawn again and the caller is told which screen changed (the 3DS copies only those).
typedef struct { u32 a, b; } ShimSig;
static inline void shimSigAdd(ShimSig*g,const void*p,size_t n){ const u8*q=(const u8*)p; u32 a=g->a, b=g->b; for(;n>=8;n-=8,q+=8){ u32 x,y; memcpy(&x,q,4); memcpy(&y,q+4,4); a=(a^x)*16777619u; b=(b^y)*2246822519u; a^=a>>15; b^=b>>13; } for(;n;n--,q++){ a=(a^*q)*16777619u; } g->a=a; g->b=b; }
static inline void shimSigInt(ShimSig*g,int v){ shimSigAdd(g,&v,4); }
static u8 shimLutM[256][3], shimLutS[256][3], shimLutP[256][3];
static void shimLut(u8 (*l)[3],const u16*pal){ for(int i=0;i<256;i++) shimRGB(pal[i],l[i]); }
static void shimDrawMain(u8*mb,int mw,int mox){
    for(int sy=0;sy<192;sy++){ int wy=(sy+shimScrollY)&511, my=wy>>3, ty=wy&7; u8*o=&mb[((24+sy)*mw+mox)*3];
        for(int sx=0;sx<256;){ int wx=(sx+shimScrollX)&511, mx=wx>>3, tx0=wx&7; u16 e=shimMap[((my>=32?2:0)+(mx>=32?1:0))*1024+(my&31)*32+(mx&31)]; const u8*t=shimTiles+(e&0x3FF)*64+ty*8; int n=8-tx0; if(sx+n>256) n=256-sx;
            for(int k=0;k<n;k++){ const u8*c=shimLutM[t[tx0+k]]; o[0]=c[0]; o[1]=c[1]; o[2]=c[2]; o+=3; } sx+=n; } }
    for(int sy=0;sy<192;sy++){ u8*o=&mb[((24+sy)*mw+mox)*3]; for(int tx=0;tx<32;tx++){ u16 e=shimMap1[(sy>>3)*32+tx]; const u8*t=shimTiles+(e&0x3FF)*64+(sy&7)*8;   // BG1 (the toolbar): colour 0 is clear
            for(int k=0;k<8;k++){ int idx=t[k]; if(idx){ const u8*c=shimLutM[idx]; o[tx*24+k*3]=c[0]; o[tx*24+k*3+1]=c[1]; o[tx*24+k*3+2]=c[2]; } } } }
    for(int n=127;n>=0;n--){ const ShimSpr*s=&shimSpr[n]; if(!s->on||!s->gfx) continue; for(int y=0;y<8;y++){ int py=s->y+y; if(py<0||py>=192) continue; for(int x=0;x<8;x++){ int xx=s->hflip?7-x:x; int b=s->gfx[y*4+xx/2]; int idx=(xx&1)?(b>>4):(b&15); if(!idx) continue; int px=s->x+x; if(px<0||px>=256) continue;
            const u8*c=shimLutP[s->pal*16+idx]; u8*d=&mb[((24+py)*mw+mox+px)*3]; d[0]=c[0]; d[1]=c[1]; d[2]=c[2]; } } } }
static void shimDrawSub(u8*sb,int sw,int sox){
#ifdef __3DS__
    if(shimFilmFullscreen){
        // Exact 4:3 image: 256x192 -> 320x240. Five output pixels
        // correspond to four source pixels in each axis; no bilinear blur,
        // stretching, texture filtering, crop or extra framebuffer.
        int off=(sw-320)/2;
        for(int y=0;y<240;y++){
            const u8 *src=&shimBmp[((y*4)/5)*256];
            u8 *dst=&sb[(y*sw+off)*3];
            for(int x=0;x<320;x++){
                const u8 *c=shimLutS[src[(x*4)/5]];
                dst[0]=c[0];dst[1]=c[1];dst[2]=c[2];
                dst+=3;
            }
        }
        return; // all cinematic captions are already in the indexed film
    }
#endif
    for(int sy=0;sy<192;sy++){ const u8*in=&shimBmp[sy*256]; u8*o=&sb[((24+sy)*sw+sox)*3]; for(int sx=0;sx<256;sx++){ const u8*c=shimLutS[in[sx]]; if(in[sx]){ o[0]=c[0]; o[1]=c[1]; o[2]=c[2]; } else o[0]=o[1]=o[2]=0; o+=3; } }
    for(int r=0;r<24;r++) for(int c=0;c<32;c++){ int ch=shimCon[r][c]; if(ch>='a'&&ch<='z') ch-=32; if(ch<32||ch>=96) continue; const unsigned char*g=font5x7[ch-32];
        for(int y=0;y<7;y++) for(int x=0;x<5;x++) if((g[y]>>(4-x))&1){ u8*o=&sb[((24+r*8+y)*sw+sox+c*8+x+1)*3]; o[0]=o[1]=o[2]=255; } } }
static long shimDrawnMain, shimDrawnSub, shimSkipped;   // counters (pc_speed.c reads them)
#ifdef __3DS__
// The first-person 3DS renderer completely replaces the sub-engine
// picture on the TOP LCD. Do not convert that invisible bitmap, scan its
// text or clear its framebuffer while exploring. The real bottom map
// still follows the normal byte-exact dirty renderer.
static int shimExploreTopOverride;
#endif
// Draws what changed into top / bottom (400 x 240 and 320 x 240, RGB, row by row) and says which of the two screens changed. The buffers must be the ones passed last time (they keep their picture);
// a new buffer, or the screens swapping places, draws everything.
static void shimRenderChanged(u8*top,u8*bot,int*topChanged,int*botChanged){
    static u8*lt, *lb; static int lswap=-1; static ShimSig sm, ss; static int have;
#ifdef __3DS__
    static int lastFilm=-1;
#endif
    // These large buffers dominate the per-frame hash. Compare their bytes
    // directly, and keep a snapshot only when they change.
    static u8 oldTiles[sizeof shimTiles], oldBitmap[256*192];
    int force=!have||top!=lt||bot!=lb||shimSwap!=lswap;
    int skipSub=0;
#ifdef __3DS__
    skipSub=shimExploreTopOverride&&!shimSwap;
    if(shimExternalInvalidate){force=1;shimExternalInvalidate=0;}
    // A film starting/ending must repaint both LCDs even when the palette
    // and underlying world map are otherwise identical.
    if(lastFilm!=shimFilmFullscreen){force=1;lastFilm=shimFilmFullscreen;}
#endif
    if(force){
        if(!skipSub)memset(top,0,400*240*3);
        memset(bot,0,320*240*3);
        lt=top; lb=bot; lswap=shimSwap; have=1;
    }
    int tilesChanged=memcmp(oldTiles,shimTiles,sizeof oldTiles)!=0;
    int bitmapChanged=!skipSub&&memcmp(oldBitmap,shimBmp,sizeof oldBitmap)!=0;
    if(tilesChanged)memcpy(oldTiles,shimTiles,sizeof oldTiles);
    if(bitmapChanged)memcpy(oldBitmap,shimBmp,sizeof oldBitmap);
    ShimSig m={2166136261u,0x9E3779B9u}, b={2166136261u,0x9E3779B9u};
    shimSigAdd(&m,shimMap,sizeof shimMap); shimSigAdd(&m,shimMap1,sizeof shimMap1); shimSigAdd(&m,BG_PALETTE,sizeof BG_PALETTE); shimSigAdd(&m,SPRITE_PALETTE,sizeof SPRITE_PALETTE); shimSigInt(&m,shimScrollX); shimSigInt(&m,shimScrollY);
    for(int n=0;n<128;n++){ const ShimSpr*s=&shimSpr[n]; shimSigInt(&m,s->on); if(!s->on||!s->gfx) continue; shimSigInt(&m,s->x); shimSigInt(&m,s->y); shimSigInt(&m,s->pal); shimSigInt(&m,s->hflip); shimSigAdd(&m,s->gfx,32); }
    if(!skipSub){
        shimSigAdd(&b,BG_PALETTE_SUB,sizeof BG_PALETTE_SUB);
        shimSigAdd(&b,shimCon,sizeof shimCon);
    }
    int mc=force||tilesChanged||m.a!=sm.a||m.b!=sm.b;
    int sc=!skipSub&&(force||bitmapChanged||b.a!=ss.a||b.b!=ss.b);
    sm=m;
    if(!skipSub)ss=b;
    u8*mbuf=shimSwap?top:bot; int mw=shimSwap?400:320, mox=shimSwap?72:32; u8*sbuf=shimSwap?bot:top; int sw=shimSwap?320:400, sox=shimSwap?32:72;
    if(mc){ shimLut(shimLutM,BG_PALETTE); shimLut(shimLutP,SPRITE_PALETTE); shimDrawMain(mbuf,mw,mox); shimDrawnMain++; } else shimSkipped++;
    if(sc){ shimLut(shimLutS,BG_PALETTE_SUB); shimDrawSub(sbuf,sw,sox); shimDrawnSub++; } else shimSkipped++;
#ifdef __3DS__
    if(shimFilmFullscreen&&shimSwap&&sc){
        // Keep the bottom LCD touch/skip image intact, and put the same
        // larger film onto the top 400x240 3DS screen with 40px black
        // pillarboxes. Copy only on a changed film frame, not every VBlank.
        for(int y=0;y<240;y++){
            u8 *dst=top+y*400*3;
            const u8 *src=bot+y*320*3;
            memset(dst,0,40*3);
            memcpy(dst+40*3,src,320*3);
            memset(dst+360*3,0,40*3);
        }
        mc=1;
    }
#endif
    if(topChanged) *topChanged=shimSwap?mc:sc; if(botChanged) *botChanged=shimSwap?sc:mc; }
static void shimRender(u8*top,u8*bot){ shimRenderChanged(top,bot,0,0); }
#endif
