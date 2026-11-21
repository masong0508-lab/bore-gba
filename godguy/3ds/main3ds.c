// main3ds.c - Bore: GodGuy on the 3DS. The game (source/main.c, DS user interface and all) is compiled unchanged against compat/ndsshim.h; this file supplies the keys, the screens and the sound
// through platform3ds.c. The touch screen shows the world and the toolbar, the top screen the status screen, as on the DS. The 3DS globe has extra atmospheric lighting and adaptive detail.
#define SAVE_PATH "sdmc:/godguy.sav"
#define GG_THEATRE 1
#define main ds_main
#include "../source/main.c"
#undef main
// Reuse the complete god-game world[][]/building/creature data in a live,
// first-person voxel renderer. It is 3DS-only, leaving DS/DSi unchanged.
#include "explore3d.h"
#include "frame_clock.h"
extern void plat_setCinema(int on);
extern unsigned long long plat_ms(void); extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth); extern void plat_exit(void); extern int plat_isNew3DS(void);
static unsigned char topBuf[400*240*3], botBuf[320*240*3];
static short sndBuf[2800*2];
static Gg3dsFrameClock frameClock;
// The same tile map is visible on the bottom screen while walking on
// the top LCD. Returning to 2D restores the previous simulation pause
// state and centres the regular map on the player's actual location.
static void ggExpFinish(void){
    ggExplore.active=0;
    paused=ggExplore.wasPaused;
    camX=(ggExplore.px>>8)*8-128;
    camY=(ggExplore.py>>8)*8-96;
    camX=ggExpClamp(camX,0,WW*8-256);
    camY=ggExpClamp(camY,0,WH*8-192);
    winX=-99;
    shimExternalInvalidate=1;
}
void shimInput(int frame,int*held,int*tx,int*ty){
    (void)frame;
    plat_setCinema(sceneClip!=0);
    int raw=plat_keys(tx,ty);
    // Intentionally requires a combo: SELECT alone retains its normal
    // 2D page-switching function. START cleanly exits the 3D walk.
    static int comboHeld;
    int combo=(raw&(KEY_SELECT|KEY_R))==(KEY_SELECT|KEY_R);
    int pressedCombo=combo&&!comboHeld;
    comboHeld=combo;
    if(pressedCombo){
        if(ggExplore.active){
            ggExpFinish();
        }else if(!titleOn&&!menuOn&&!extrasView&&!sceneOn&&!dlgOn&&
                 !sceneClip&&!view3d&&!zBmp&&dlgPend<0){
            int x=(camX+128)>>3,y=(camY+96)>>3;
            ggExpBegin(x,y,paused);
            paused=1;               // freeze creatures during editing
        }
        *held=0;
        *tx=*ty=0;
        return;
    }
    if(ggExplore.active){
        int action=ggExpStep(raw,*tx,*ty);
        if(action&GG_EXP_EXIT)ggExpFinish();
        else{
            // Keep the actual 2D map underneath the player, so tapping
            // it jumps to a visible position and exiting remains coherent.
            camX=ggExpClamp((ggExplore.px>>8)*8-128,0,WW*8-256);
            camY=ggExpClamp((ggExplore.py>>8)*8-96,0,WH*8-192);
            if(action&GG_EXP_CHANGE){
                KD_DIRTY();
                fxReq(FX_PAINT);
                siprintf(info,"3D TERRAIN CHANGED");
            }
            if(action&GG_EXP_SAVE){
                doSave();  // same persistent save format and backup path
                ggExplore.feedback=4;
                ggExplore.feedbackT=90;
            }
        }
        *held=0; *tx=*ty=0;
        return;
    }
    *held=raw;
}
void shimPresent(void){
    // Preserve wall-clock time across short rendering stalls instead of
    // discarding everything above 80 ms, which dragged music and starved
    // the sound buffer. Unmixed samples and sequencer ticks are bounded
    // but carried into subsequent frames.
    int n,k,quality;
    gg3dsClockFrame(&frameClock,plat_ms(),&n,&k,&quality);
    shimMusicTicks=k;
    gg3dsGlobeQuality=quality;  // New 3DS detail yields to audio under load
    shimMix(sndBuf,n);
    int tc,bc;
    shimFilmFullscreen=sceneClip!=0;
    shimRenderChanged(topBuf,botBuf,&tc,&bc);
    if(ggExplore.active){
        ggExpRender(topBuf);
        tc=1;  // a live 3D scene, not a cached/static menu picture
    }
    // While exploring, the left-eye RGB world is rendered as real live
    // voxel columns. The 2D globe's unrelated depth map must not warp it.
    plat_present(topBuf,botBuf,tc,bc,sndBuf,n,
                 ggExplore.active?0:(shimSwap?0:shimDepth));
}
int main(void){
    plat_init();
    int new3ds=plat_isNew3DS();
    gg3dsClockInit(&frameClock,new3ds);
    gg3dsGlobeQuality=new3ds;
#ifdef GG_HQVOX
    gmVoxWarm();     // predecode vocals, never decode in the frame mixer
#endif
#ifdef SHIM_HQ
    shimHqOn=new3ds; // retain the lighter mixer on Old 3DS
#endif
    ds_main(); plat_exit(); return 0;
}
