// main3ds.c - Bore: GodGuy on the 3DS. The game (source/main.c, DS user interface and all) is compiled unchanged against compat/ndsshim.h; this file supplies the keys, the screens and the sound
// through platform3ds.c. The touch screen shows the world and the toolbar, the top screen the status screen, as on the DS. The 3DS globe has extra atmospheric lighting and adaptive detail.
#define SAVE_PATH "sdmc:/godguy.sav"
#define GG_THEATRE 1
#define main ds_main
#include "../source/main.c"
#undef main
// Reuse the complete god-game world[][]/building/creature data in a live,
// first-person voxel renderer. It is 3DS-only, leaving DS/DSi unchanged.
static void ggExpRenderCreatures(u8 *top);
#include "explore3d.h"
#include "hybrid3d.h"
#include "population3d.h"
#include "frame_clock.h"
extern void plat_setCinema(int on);
extern unsigned long long plat_ms(void); extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth); extern void plat_exit(void); extern int plat_isNew3DS(void);
static unsigned char topBuf[400*240*3], botBuf[320*240*3];
static short sndBuf[2800*2];
static Gg3dsFrameClock frameClock;
// Two modes share ONE simulation and ONE selection. Select a living
// sentient creature via INSPECT/POSSESS or the explicit hybrid picker.
// Then SELECT+R toggles its 2D tile/3D POV. SELECT+X opens the picker.
// Persistent IDs avoid accidentally controlling another creature if the
// simulation compacts its array after a save.
static int ggHybridPicking;
static u32 ggHybridSelectedId;
static int ggHybridSelectedIndex=-1;
static int ggExpRenderValid;
static void ggExpRemember(int i){
    if(!ggHybridEligible(i))return;
    ggHybridSelectedId=crId[i];
    ggHybridSelectedIndex=i;
}
static int ggExpSelected(void){
    // Explicit tap selection takes precedence over the regular INSPECT
    // card. Check the fast slot first; search only after a compaction.
    if(ggHybridSelectedId){
        int i=ggHybridSelectedIndex;
        if(i>=0&&i<nCr&&crId[i]==ggHybridSelectedId)
            return ggHybridEligible(i)?i:-1;
        for(i=0;i<nCr;i++)if(crId[i]==ggHybridSelectedId){
            ggHybridSelectedIndex=i;
            return ggHybridEligible(i)?i:-1;
        }
        ggHybridSelectedId=0;
        ggHybridSelectedIndex=-1;
    }
    if(ggHybridEligible(possessed))return possessed;
    if(inspKind==1&&ggHybridEligible(inspIdx))return inspIdx;
    if(ggHybridEligible(inspected))return inspected;
    return -1;
}
static int ggExpCanEnter(void){
    return !titleOn&&!menuOn&&!extrasView&&!sceneOn&&!dlgOn&&
           !sceneClip&&!view3d&&!zBmp&&dlgPend<0;
}
static void ggExpFinish(void){
    if(ggExplore.actorId){
        int i=ggHybridResolve();
        if(i>=0){ggHybridAfterSave();ggExpRemember(i);}
    }
    ggExplore.active=0;
    paused=ggExplore.wasPaused;
    camX=ggExpClamp((ggExplore.px>>8)*8-128,0,WW*8-256);
    camY=ggExpClamp((ggExplore.py>>8)*8-96,0,WH*8-192);
    winX=-99;
    ggExpRenderValid=0;
    shimExternalInvalidate=1;
}
static void ggExpMessage(const char *m){
    snprintf(info,sizeof info,"%s",m);
    toastT=420;
}
void shimInput(int frame,int*held,int*tx,int*ty){
    (void)frame;
    plat_setCinema(sceneClip!=0);
    int raw=plat_keys(tx,ty);
    static int previousRaw;
    int pressedTouch=(raw&KEY_TOUCH)&&!(previousRaw&KEY_TOUCH);
    int pressedView=((raw&(KEY_SELECT|KEY_R))==(KEY_SELECT|KEY_R))&&
                    ((previousRaw&(KEY_SELECT|KEY_R))!=(KEY_SELECT|KEY_R));
    int pressedPick=((raw&(KEY_SELECT|KEY_X))==(KEY_SELECT|KEY_X))&&
                    ((previousRaw&(KEY_SELECT|KEY_X))!=(KEY_SELECT|KEY_X));
    previousRaw=raw;
    if(pressedPick){
        if(ggExplore.active)ggExpFinish();
        if(ggExpCanEnter()){
            ggHybridPicking=1;
            ggExpMessage("HYBRID PICK: TAP A SENTIENT CREATURE");
        }
        *held=0;*tx=*ty=0;return;
    }
    if(pressedView){
        if(ggExplore.active){
            ggExpFinish();
            ggExpMessage("HYBRID: BACK TO 2D; SELECT+R FOR 3D");
        }else if(ggExpCanEnter()){
            int i=ggExpSelected();
            if(i>=0){
                ggExpRemember(i);
                if(ggHybridBegin(i,paused)){
                    paused=1;
                    ggHybridPicking=0;
                    ggExpRenderValid=0;
                }
            }else{
                // No playable mind selected: stay in 2D, prompt a
                // deliberate choice instead of possessing a random animal.
                ggHybridPicking=1;
                ggExpMessage("TAP A SAPIENT PERSON OR UPLIFTED ANIMAL");
            }
        }
        *held=0;*tx=*ty=0;return;
    }
    if(ggHybridPicking&&!ggExplore.active){
        if(pressedTouch&&ggExpCanEnter()){
            int x=(camX+*tx)>>3,y=(camY+*ty)>>3;
            int i=ggHybridPick(x,y);
            if(i>=0){
                ggExpRemember(i);
                ggHybridPicking=0;
                char nm[48];crName(&cr[i],nm);
                snprintf(info,sizeof info,"SELECTED %.36s  SELECT+R: 3D",nm);
                toastT=420;
                shimExternalInvalidate=1;
            }else ggExpMessage("NOT A SENTIENT MIND: TAP ANOTHER");
        }else if(raw&KEY_B){
            ggHybridPicking=0;
            ggExpMessage("HYBRID SELECTION CLOSED");
        }
        *held=0;*tx=*ty=0;return;
    }
    if(ggExplore.active){
        if(ggExplore.actorId&&ggHybridResolve()<0){
            ggExpFinish();
            ggExpMessage("CREATURE UNAVAILABLE: RETURNED TO MAP");
            *held=0;*tx=*ty=0;return;
        }
        if(ggExplore.actorId&&pressedTouch){
            int x=(camX+*tx)>>3,y=(camY+*ty)>>3;
            int i=ggHybridPick(x,y);
            if(i>=0){
                int oldPause=ggExplore.wasPaused;
                ggExpRemember(i);
                ggHybridBegin(i,oldPause);
                ggExpRenderValid=0;
            }
        }
        int beforeX=ggExplore.px,beforeY=ggExplore.py;
        int action=ggExpStep(raw,*tx,*ty);
        if(action&GG_EXP_EXIT)ggExpFinish();
        else if(ggExplore.actorId&&!ggHybridMove(beforeX,beforeY)){
            ggExpFinish();
            ggExpMessage("MIND LOST: RETURNED TO 2D WORLD");
        }else{
            // Keep 2D and first-person cameras on the same live planet.
            camX=ggExpClamp((ggExplore.px>>8)*8-128,0,WW*8-256);
            camY=ggExpClamp((ggExplore.py>>8)*8-96,0,WH*8-192);
            if(action&GG_EXP_CHANGE){
                KD_DIRTY();
                fxReq(FX_PAINT);
                ggExpMessage("3D TERRAIN CHANGED");
            }
            if(action&GG_EXP_SAVE){
                doSave(); // compacts live population; rebind via crId
                ggHybridAfterSave();
                ggExpMessage("WORLD SAVE REQUESTED");
            }
        }
        *held=0;*tx=*ty=0;return;
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
        // Render on actual camera/world changes, or every eighth frame
        // for subtle clouds/feedback. Static scenes reuse the existing
        // RGB output; there is no second 400x240 framebuffer or hash scan.
        static int lastX,lastY,lastA,lastEdits,lastFx,lastSky;
        int sky=ggExplore.frame/8;
        int dirty=!ggExpRenderValid||tc||
                  ggExplore.px!=lastX||ggExplore.py!=lastY||
                  ggExplore.angle!=lastA||ggExplore.edits!=lastEdits||
                  ggExplore.feedback!=lastFx||sky!=lastSky;
        if(dirty){
            ggExpRender(topBuf);
            ggExpRenderValid=1;
            lastX=ggExplore.px;lastY=ggExplore.py;
            lastA=ggExplore.angle;lastEdits=ggExplore.edits;
            lastFx=ggExplore.feedback;lastSky=sky;
            tc=1;
        }
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
