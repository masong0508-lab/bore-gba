// ndshw.h - the same small slice of libnds that the game uses (see ndsshim.h), but talking straight to the DS hardware registers: NO libnds, NO calico. The ARM9 side (this file) draws; the sound channels
// and the touch screen belong to the ARM7, so they are driven through a command ring in main RAM (hw/arm7.c reads it). The 3D engine is not used.
#ifndef NDSHW_H
#define NDSHW_H
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef signed char s8; typedef short s16; typedef int s32;
typedef volatile unsigned short vu16; typedef volatile unsigned int vu32;
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#define NDSHW 1
#define siprintf sprintf
#define iprintf(...) ((void)0)
#define RGB15(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
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
enum { MODE_0_2D=1, MODE_0_3D, MODE_5_2D, DISPLAY_BG0_ACTIVE=0x100, DISPLAY_BG1_ACTIVE=0x2000, DISPLAY_BG3_ACTIVE=0x200, DISPLAY_SPR_ACTIVE=0x400, DISPLAY_SPR_1D_LAYOUT=0x800, DISPLAY_SPR_1D_SIZE_32=0x1000,
       VRAM_A_MAIN_BG=1, VRAM_B_MAIN_SPRITE, VRAM_C_SUB_BG, VRAM_D_TEXTURE, BgType_Text8bpp=1, BgType_Text4bpp, BgType_Bmp8, BgSize_T_512x512=1, BgSize_T_256x256, BgSize_B8_256x256,
       SpriteMapping_1D_32=1, SpriteSize_8x8=1, SpriteColorFormat_16Color=1, SoundFormat_8Bit=1, DutyCycle_50=4 };
#define DutyCycle int
#define HW_REG16(a) (*(volatile u16*)(a))
#define HW_REG32(a) (*(volatile u32*)(a))
#define BG_PALETTE ((u16*)0x05000000)
#define SPRITE_PALETTE ((u16*)0x05000200)
#define BG_PALETTE_SUB ((u16*)0x05000400)
#define BG_TILE_RAM(n) ((void*)0x06004000)
#define REG_VCOUNT HW_REG16(0x04000006)
// ---- video
static int hwDispMain=0x10000, hwDispSub=0x10000;
static inline void videoSetMode(int m){   // m: the shim's MODE_ + DISPLAY_ bits -> DISPCNT
    u32 d=0x10000; if(m&DISPLAY_BG0_ACTIVE) d|=0x100; if(m&DISPLAY_BG1_ACTIVE) d|=0x200; if(m&DISPLAY_SPR_ACTIVE) d|=0x1000; if(m&DISPLAY_SPR_1D_LAYOUT) d|=0x10; HW_REG32(0x04000000)=hwDispMain=(int)d; }
static inline void videoSetModeSub(int m){ (void)m; HW_REG32(0x04001000)=hwDispSub=(int)(0x10000|0x100); }   // the top screen is a plain tile layer (mode 0, BG0): see hwSubFlush
static inline void vramSetBankA(int b){ (void)b; *(volatile u8*)0x04000240=0x81; }   // A: main BG (0x06000000)
static inline void vramSetBankB(int b){ (void)b; *(volatile u8*)0x04000241=0x82; }   // B: main sprites (0x06400000)
static inline void vramSetBankC(int b){ (void)b; *(volatile u8*)0x04000242=0x84; }   // C: sub BG (0x06200000)
static inline void vramSetBankD(int b){ (void)b; }
static inline void lcdMainOnBottom(void){ HW_REG16(0x04000304)=HW_REG16(0x04000304)&~0x8000; }
static inline void lcdMainOnTop(void){ HW_REG16(0x04000304)=HW_REG16(0x04000304)|0x8000; }
static inline int bgInit(int l,int t,int s,int m,int tb){ (void)t; if(l==0) HW_REG16(0x04000008)=(u16)(0x0001|(tb<<2)|(m<<8)|0x80|(s==BgSize_T_512x512?(3<<14):0)); else HW_REG16(0x0400000A)=(u16)(0x0000|(tb<<2)|(m<<8)|0x80); return l; }
static inline int bgInitSub(int l,int t,int s,int m,int tb){ (void)l;(void)t;(void)s;(void)m;(void)tb;
    HW_REG16(0x04001008)=(u16)(0x0080|(1<<2)|(0<<8));            // BG0: 8 bit tiles, tiles at 16 KB, map at 0, 256 x 256
    vu16*map=(vu16*)0x06200000; for(int i=0;i<1024;i++) map[i]=(u16)i;   // tile i = screen block i (32 wide)
    return 3; }
// the 256 x 192 picture the UI draws in RAM goes to the top screen as 768 tiles of 8 x 8 pixels (32-bit stores only)
static void hwSubFlush(const u8*buf){
    for(int ty=0;ty<24;ty++) for(int tx=0;tx<32;tx++){ vu32*d=(vu32*)(0x06204000+(ty*32+tx)*64); for(int y=0;y<8;y++){ const u8*r=buf+(ty*8+y)*256+tx*8; d[y*2]=(u32)r[0]|((u32)r[1]<<8)|((u32)r[2]<<16)|((u32)r[3]<<24); d[y*2+1]=(u32)r[4]|((u32)r[5]<<8)|((u32)r[6]<<16)|((u32)r[7]<<24); } } }
static inline void bgSetPriority(int id,int p){ u32 a=id==0?0x04000008:0x0400000A; HW_REG16(a)=(u16)((HW_REG16(a)&~3)|(p&3)); }
static inline void bgSetScroll(int id,int x,int y){ HW_REG16(0x04000010+id*4)=(u16)x; HW_REG16(0x04000012+id*4)=(u16)y; }
static inline void bgUpdate(void){}
static inline void*bgGetMapPtr(int id){ return (void*)(0x06000000+(id==1?4*2048:0)); }
static inline void*bgGetGfxPtr(int id){ (void)id; return (void*)0x06200000; }
static inline void DC_FlushRange(const void*p,u32 n){ (void)p; (void)n; }
// ---- sprites
typedef struct { int dummy; } OamState; static OamState oamMain; static int hwObjTile;
static inline void oamInit(OamState*o,int m,bool e){ (void)o;(void)m;(void)e; hwObjTile=0; for(int i=0;i<128;i++){ ((vu16*)0x07000000)[i*4]=0x200; ((vu16*)0x07000000)[i*4+1]=0; ((vu16*)0x07000000)[i*4+2]=0; } }
static inline u16*oamAllocateGfx(OamState*o,int sz,int fmt){ (void)o;(void)sz;(void)fmt; u16*r=(u16*)(0x06400000+hwObjTile*32); hwObjTile++; return r; }
static inline void oamSet(OamState*o,int id,int x,int y,int prio,int pal,int sz,int fmt,const void*gfx,int aff,bool dbl,bool hide,bool hflip,bool vflip,bool mosaic){
    (void)o;(void)sz;(void)fmt;(void)aff;(void)dbl;(void)mosaic; vu16*a=(vu16*)0x07000000+id*4; int tile=(int)(((u32)gfx-0x06400000)>>5);
    a[0]=(u16)((y&0xFF)|(hide?0x200:0)); a[1]=(u16)((x&0x1FF)|(hflip?0x1000:0)|(vflip?0x2000:0)); a[2]=(u16)(tile|((prio&3)<<10)|((pal&15)<<12)); }
static inline void oamSetHidden(OamState*o,int id,bool h){ (void)o; vu16*a=(vu16*)0x07000000+id*4; a[0]=(u16)(h?(a[0]|0x200):(a[0]&~0x200)); }
static inline void oamUpdate(OamState*o){ (void)o; }
// ---- input: the d-pad and buttons are read here, X, Y and the stylus come from the ARM7 through the shared block
typedef struct { u32 op, ch, a, b, c, d; } HwCmd;
typedef struct { volatile u32 head, tail, busy, keys, pen, px, py, alive, svType, svBusy, svOK; HwCmd q[64]; volatile u32 seq, tapX, tapY, rawX, rawY, tapRawX, tapRawY, svSize; } HwIpc;
#define HWIPC ((HwIpc*)0x023F0000)
static int hwHeld, hwTx, hwTy; static u32 hwSeq;
#define HW_CAL 1
// touch calibration: the 12-bit readings from the ARM7 are turned into pixels with two reference points (rx1 ry1 sx1 sy1 rx2 ry2 sx2 sy2) tapped on the calibration screen
static int hwCalOn, hwCal[8], hwRx, hwRy;
#define HWCAL_MAGIC 0x4C414347u   /* "GCAL" */
static void hwCalPack(u8*o){ u32 r[10]; r[0]=HWCAL_MAGIC; u32 c=0xA5A5A5A5u; for(int i=0;i<8;i++){ r[1+i]=(u32)hwCal[i]; c=c*31u+r[1+i]; } r[9]=c; memcpy(o,r,40); }
static int hwCalUnpack(const u8*in){ u32 r[10]; memcpy(r,in,40); if(r[0]!=HWCAL_MAGIC) return 0; u32 c=0xA5A5A5A5u; for(int i=0;i<8;i++) c=c*31u+r[1+i]; if(c!=r[9]) return 0;
    int v[8]; for(int i=0;i<8;i++) v[i]=(int)r[1+i]; int dx=v[4]-v[0], dy=v[5]-v[1]; if(dx<2200||dx>4800||dy<2000||dy>4800) return 0; for(int i=0;i<8;i++) hwCal[i]=v[i]; hwCalOn=1; return 1; }
static inline void hwMapRaw(int rx,int ry,int*px,int*py){ int x=hwCal[2]+(rx-hwCal[0])*(hwCal[6]-hwCal[2])/(hwCal[4]-hwCal[0]), y=hwCal[3]+(ry-hwCal[1])*(hwCal[7]-hwCal[3])/(hwCal[5]-hwCal[1]); *px=x<0?0:x>255?255:x; *py=y<0?0:y>191?191:y; }
static inline void scanKeys(void){ u32 k=~HW_REG16(0x04000130)&0x3FF; int h=(int)k; if(HWIPC->keys&1) h|=KEY_X; if(HWIPC->keys&2) h|=KEY_Y; if(HWIPC->pen){ h|=KEY_TOUCH; hwTx=(int)HWIPC->px; hwTy=(int)HWIPC->py; hwRx=(int)HWIPC->rawX; hwRy=(int)HWIPC->rawY; hwSeq=HWIPC->seq; } else if(HWIPC->seq!=hwSeq){ hwSeq=HWIPC->seq; h|=KEY_TOUCH; hwTx=(int)HWIPC->tapX; hwTy=(int)HWIPC->tapY; hwRx=(int)HWIPC->tapRawX; hwRy=(int)HWIPC->tapRawY; } if((h&KEY_TOUCH)&&hwCalOn) hwMapRaw(hwRx,hwRy,&hwTx,&hwTy); hwHeld=h; }   // (a tap shorter than a frame still counts: the ARM7 counts every new contact)
static int hwPrev;
static inline int keysHeld(void){ return hwHeld; }
static inline int keysDown(void){ int d=hwHeld&~hwPrev; hwPrev=hwHeld; return d; }
static inline void touchRead(touchPosition*t){ t->px=hwTx; t->py=hwTy; }
// ---- saving: the world is run-length packed and written to the cartridge's save memory (flash or EEPROM) by the ARM7. The buffers sit in a part of RAM the data cache never touches.
#define SVBUF ((u8*)0x02340000)
#define SVCMP ((u8*)0x023A0000)
#define SVMAX (300*1024)
typedef struct { int dummy; } SV; static SV svDummy; static u32 svLen, svPos; static int svMode;
static int fatInitDefault(void){ for(int i=0;i<60;i++){ while(REG_VCOUNT!=192){} while(REG_VCOUNT==192){} if(HWIPC->svType) break; } return HWIPC->svType!=0; }
static void hwFlash(u32 op,u32 mem,u32 len,u32 off){ HWIPC->svBusy=1; HWIPC->svOK=0; HwIpc*q=HWIPC; while(((q->head+1)&63)==q->tail){} HwCmd*m=&q->q[q->head]; m->op=op; m->ch=0; m->a=mem; m->b=len; m->c=off; m->d=0; q->head=(q->head+1)&63; u32 t=0; while(HWIPC->svBusy&&++t<50000000u){} }
static u32 rleEnc(const u8*in,u32 n,u8*out){ u32 o=0,i=0; while(i<n){ u32 r=1; while(i+r<n&&in[i+r]==in[i]&&r<129) r++; if(r>=3){ out[o++]=(u8)(126+r); out[o++]=in[i]; i+=r; } else { u32 st=i; u32 l=0; while(i<n&&l<128){ u32 rr=1; while(i+rr<n&&in[i+rr]==in[i]&&rr<3) rr++; if(rr>=3) break; i++; l++; } out[o++]=(u8)(l-1); for(u32 k=0;k<l;k++) out[o++]=in[st+k]; } } return o; }
static u32 rleDec(const u8*in,u32 n,u8*out,u32 max){ u32 o=0,i=0; while(i<n){ u8 c=in[i++]; if(c<128){ u32 l=c+1; if(o+l>max||i+l>n) return 0; for(u32 k=0;k<l;k++) out[o++]=in[i++]; } else { u32 r=c-126; if(o+r>max||i>=n) return 0; for(u32 k=0;k<r;k++) out[o++]=in[i]; i++; } } return o; }
static SV*svOpen(const char*p,const char*m){ (void)p; if(!HWIPC->svType) return 0; svPos=0;
    if(m[0]=='w'){ svMode=1; svLen=0; return &svDummy; }
    hwFlash(9,(u32)SVCMP,12,0); if(!HWIPC->svOK||SVCMP[0]!='G'||SVCMP[1]!='G'||SVCMP[2]!='S'||SVCMP[3]=='\0') return 0; u32 clen=*(u32*)(SVCMP+4), raw=*(u32*)(SVCMP+8); if(clen==0||clen>SVMAX-16||raw>SVMAX) return 0;
    hwFlash(9,(u32)SVCMP,clen+12,0); if(!HWIPC->svOK) return 0; svLen=rleDec(SVCMP+12,clen,SVBUF,SVMAX); if(svLen!=raw) return 0; svMode=0; return &svDummy; }
static size_t svWrite(const void*b,size_t s,size_t n,SV*f){ (void)f; u32 t=(u32)(s*n); if(svLen+t>SVMAX) return 0; memcpy(SVBUF+svLen,b,t); svLen+=t; return n; }
static size_t svRead(void*b,size_t s,size_t n,SV*f){ (void)f; u32 t=(u32)(s*n); if(svPos+t>svLen) return 0; memcpy(b,SVBUF+svPos,t); svPos+=t; return n; }
static u32 hwSvLimit(void){ return HWIPC->svSize?HWIPC->svSize:65536u; }   // the end of the save memory (65536 when the chip does not say: the largest EEPROM)
static int svClose(SV*f){ (void)f; if(svMode==1){ u32 clen=rleEnc(SVBUF,svLen,SVCMP+12); SVCMP[0]='G'; SVCMP[1]='G'; SVCMP[2]='S'; SVCMP[3]=1; *(u32*)(SVCMP+4)=clen; *(u32*)(SVCMP+8)=svLen; u32 tot=clen+12;
    if(hwCalOn&&tot+40<=SVMAX&&tot+40<=hwSvLimit()){ hwCalPack(SVCMP+tot); tot+=40; }   // the touch calibration rides along right behind the world, in the same write
    hwFlash(8,(u32)SVCMP,tot,0); return HWIPC->svOK?0:-1; } return 0; }
// ---- the touch calibration is kept in the save memory: right behind the world data when a world is saved (so the old save layout still loads), at the very start when there is none
static u32 hwCalOff(void){ hwFlash(9,(u32)SVCMP,12,0); if(HWIPC->svOK&&SVCMP[0]=='G'&&SVCMP[1]=='G'&&SVCMP[2]=='S'&&SVCMP[3]){ u32 clen; memcpy(&clen,SVCMP+4,4); if(clen&&clen<=SVMAX-16) return 12+clen; } return 0; }
static int hwCalLoad(void){ if(!HWIPC->svType) return 0; u32 off=hwCalOff(); hwFlash(9,(u32)SVCMP,40,off); if(!HWIPC->svOK) return 0; return hwCalUnpack(SVCMP); }
static void hwCalStore(void){ if(!HWIPC->svType||!hwCalOn) return; u32 off=hwCalOff(), base=off&~255u; if(base+512>hwSvLimit()) return;   // a page or two are read, patched and written back whole, so the world around the record is not touched
    hwFlash(9,(u32)SVCMP,512,base); if(!HWIPC->svOK) return; hwCalPack(SVCMP+(off-base)); hwFlash(8,(u32)SVCMP,512,base); }
// ---- sound: channels are allocated here, played by the ARM7
static int hwUsed[16], hwAge[16]; static u32 hwKind[16];
static void hwSend(u32 op,u32 ch,u32 a,u32 b,u32 c,u32 d){ HwIpc*q=HWIPC; while(((q->head+1)&63)==q->tail){} HwCmd*m=&q->q[q->head]; m->op=op; m->ch=ch; m->a=a; m->b=b; m->c=c; m->d=d; q->head=(q->head+1)&63; }
static int hwAlloc(int lo,int hi){ for(int i=lo;i<=hi;i++) if(!hwUsed[i]){ hwUsed[i]=1; hwAge[i]=0; return i; } return -1; }
static inline void soundEnable(void){}
static int soundPlaySample(const void*data,int fmt,u32 size,u16 freq,u8 vol,u8 pan,bool loop,u16 lp){ (void)fmt;(void)lp; int i=hwAlloc(0,15); if(i<0) return -1; hwKind[i]=1; hwSend(1,(u32)i,(u32)data,size,freq,(u32)(vol|(pan<<8)|((loop?1:0)<<16))); return i; }
static int soundPlayPSG(int duty,u16 freq,u8 vol,u8 pan){ int i=hwAlloc(8,13); if(i<0) return -1; hwKind[i]=2; hwSend(2,(u32)i,0,0,freq,(u32)(vol|(pan<<8)|((u32)3<<16))); (void)duty; return i; }
static int soundPlayNoise(u16 freq,u8 vol,u8 pan){ int i=hwAlloc(14,15); if(i<0) return -1; hwKind[i]=3; hwSend(3,(u32)i,0,0,freq,(u32)(vol|(pan<<8))); return i; }
static inline void soundKill(int id){ if(id>=0&&id<16){ hwUsed[id]=0; hwSend(4,(u32)id,0,0,0,0); } }
static inline void soundSetFreq(int id,u16 f){ if(id>=0&&id<16) hwSend(5,(u32)id,0,0,f,0); }
static inline void soundSetVolume(int id,u8 v){ if(id>=0&&id<16) hwSend(6,(u32)id,0,0,v,0); }
static inline void soundSetPan(int id,u8 p){ if(id>=0&&id<16) hwSend(7,(u32)id,0,0,p,0); }
static void hwReap(void){ u32 b=HWIPC->busy; for(int i=0;i<16;i++) if(hwUsed[i]){ if(++hwAge[i]>4&&hwKind[i]==1&&!((b>>i)&1)) hwUsed[i]=0; } }
static inline void swiWaitForVBlank(void){ while(REG_VCOUNT!=192){} while(REG_VCOUNT==192){} hwReap(); }
// ---- the 3D engine is not used
static u32 hwGfxTex;
#define GFX_TEX_COORD hwGfxTex
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
#endif
