// A stand-in for libctru's <3ds.h> with just what platform3ds.c uses, so the 3DS platform file can be compiled and tested on a PC (pc_3ds.c). Not for the real build.
#ifndef STUB_3DS_H
#define STUB_3DS_H
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t Result;
#define R_SUCCEEDED(x) ((x)>=0)
enum { KEY_A=1<<0, KEY_B=1<<1, KEY_SELECT=1<<2, KEY_START=1<<3, KEY_DRIGHT=1<<4, KEY_DLEFT=1<<5, KEY_DUP=1<<6, KEY_DDOWN=1<<7, KEY_R=1<<8, KEY_L=1<<9, KEY_X=1<<10, KEY_Y=1<<11, KEY_TOUCH=1<<20,
       KEY_CPAD_UP=1<<28, KEY_CPAD_DOWN=1<<29, KEY_CPAD_LEFT=1<<30, KEY_CPAD_RIGHT=1u<<31 };
typedef struct { u16 px, py; } touchPosition;
typedef enum { GFX_TOP=0, GFX_BOTTOM=1 } gfxScreen_t; typedef enum { GFX_LEFT=0, GFX_RIGHT=1 } gfx3dSide_t;
enum { NDSP_OUTPUT_STEREO=1, NDSP_INTERP_LINEAR=1, NDSP_FORMAT_STEREO_PCM16=1, NDSP_WBUF_FREE=0, NDSP_WBUF_QUEUED=1, NDSP_WBUF_PLAYING=2, NDSP_WBUF_DONE=3 };
typedef struct { void*data_vaddr; u32 nsamples; int status; } ndspWaveBuf;
// the model the test looks at
extern u8 stubFb[2][2][400*240*3]; extern int stubCur[2], stubShown[2], stubSwaps, stubFlushes, stubSpeedup, stubQueued, stubNextExpect; extern u32 stubKeys; extern touchPosition stubTouch; extern short*stubQueue[64]; extern ndspWaveBuf*stubWb[64];
static inline void osSetSpeedupEnable(int on){ extern int stubSpeedup; stubSpeedup=on; }
static inline void gfxInitDefault(void){} static inline void gfxSet3D(int b){ (void)b; } static inline void gfxSetDoubleBuffering(int s,int b){ (void)s;(void)b; } static inline void gfxExit(void){}
extern u8 stubFbR[2][400*240*3]; extern float stubSlider;
static inline u8*gfxGetFramebuffer(int s,int side,u16*w,u16*h){ (void)w;(void)h; if(s==GFX_TOP&&side==GFX_RIGHT) return stubFbR[stubCur[0]]; return stubFb[s][stubCur[s]]; }
static inline float osGet3DSliderState(void){ return stubSlider; }
static inline void gfxFlushBuffers(void){ extern int stubFlushes; stubFlushes++; }
static inline void gfxSwapBuffers(void){ extern int stubSwaps; for(int s=0;s<2;s++){ stubShown[s]=stubCur[s]; stubCur[s]^=1; } stubSwaps++; }
static inline void gspWaitForVBlank(void){}
static inline Result ndspInit(void){ return 0; } static inline void ndspExit(void){} static inline void ndspSetOutputMode(int m){ (void)m; } static inline void ndspChnSetInterp(int c,int i){ (void)c;(void)i; } static inline void ndspChnSetRate(int c,float r){ (void)c;(void)r; } static inline void ndspChnSetFormat(int c,int f){ (void)c;(void)f; }
static inline void*linearAlloc(size_t n){ return malloc(n); } static inline void DSP_FlushDataCache(const void*p,u32 n){ (void)p;(void)n; }
static inline void ndspChnWaveBufAdd(int c,ndspWaveBuf*w){ (void)c; extern int stubQueued; extern short*stubQueue[64]; extern ndspWaveBuf*stubWb[64]; w->status=NDSP_WBUF_QUEUED; if(stubQueued<64){ stubQueue[stubQueued]=(short*)w->data_vaddr; stubWb[stubQueued]=w; } stubQueued++; }
static inline void hidScanInput(void){} static inline u32 hidKeysHeld(void){ extern u32 stubKeys; return stubKeys; } static inline void hidTouchRead(touchPosition*t){ extern touchPosition stubTouch; *t=stubTouch; }
static inline int aptMainLoop(void){ return 1; }
#endif
