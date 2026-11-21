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
#include "chronicles3d.h"
#include "reign3d.h"
#include "origin3d.h"
#include "reign_scenery.h"
#include "reign_quests.h"
#include "reign_saga3d.h"
#include "reign_ambient3d.h"
#include "reign_map3d.h"
#include "mode_input3d.h"
#include "world_music3d.h"
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
static int ggChronHintShown;
static GgModeInputGuard gg3dModeInput;
static int gg3dPhysicalKeys;
static void ggExpRemember(int i){
    if(!ggHybridEligible(i))return;
    ggHybridSelectedId=crId[i];
    ggHybridSelectedIndex=i;
}
static int ggExpSelected(void){
    // INSPECT and POSSESS are active choices. If the player taps a new
    // sentient mind on the 2D map, that newer subject must replace the
    // previously bound hybrid camera instead of silently keeping the old
    // person. Explicit possession takes priority over an older card.
    if(ggHybridSelectedId){
        if(ggHybridEligible(possessed)&&
           crId[possessed]!=ggHybridSelectedId){
            ggExpRemember(possessed);return possessed;
        }
        if(inspKind==1&&ggHybridEligible(inspIdx)&&
           crId[inspIdx]!=ggHybridSelectedId){
            ggExpRemember(inspIdx);return inspIdx;
        }
    }
    // Check the fast slot first; search only after compaction.
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
    // The exit button must NOT trigger another action when control
    // resumes in the 2D game (e.g. START opening its pause menu).
    ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
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
    gg3dPhysicalKeys=plat_keys(tx,ty);
    int raw=ggModeInputFilter(&gg3dModeInput,gg3dPhysicalKeys);
    static int previousRaw;
    int pressedTouch=(raw&KEY_TOUCH)&&!(previousRaw&KEY_TOUCH);
    int pressedView=((raw&(KEY_SELECT|KEY_R))==(KEY_SELECT|KEY_R))&&
                    ((previousRaw&(KEY_SELECT|KEY_R))!=(KEY_SELECT|KEY_R));
    int pressedPick=((raw&(KEY_SELECT|KEY_X))==(KEY_SELECT|KEY_X))&&
                    ((previousRaw&(KEY_SELECT|KEY_X))!=(KEY_SELECT|KEY_X));
    int pressedChron=((raw&(KEY_SELECT|KEY_Y))==(KEY_SELECT|KEY_Y))&&
                     ((previousRaw&(KEY_SELECT|KEY_Y))!=(KEY_SELECT|KEY_Y));
    previousRaw=raw;
    // A wholly separate, unlockable 3DS story. The main story stays on
    // the original 2D tile UI and neither story's save format is changed.
    if(ggChron.active){
        // First entry begins the life-story opening and adult creator.
        // Neither the battle nor the career council advances until the
        // character discovers their hidden nature at age twenty-two.
        if(ggOrigin.active){
            int before=ggOrigin.active;
            if(ggOriginStep(raw)){
                ggChron.active=0;
                ggExpFinish();
                ggExpMessage("OPENING EXIT: ORIGINAL 2D STORY RESTORED");
            }else if(before&&!ggOrigin.active)
                ggExpRenderValid=0; // redraw real terrain after the reel
            *held=0;*tx=*ty=0;return;
        }
        // The act/ending film director has first claim on inputs, so a
        // meaningful choice never turns into an accidental combat strike.
        int sagaMode=ggSaga.mode,sagaChoice=ggSaga.choice;
        int sagaAction=ggSagaStep(raw);
        if(sagaAction){
            if(sagaAction==2){
                ggReignSave();ggChron.active=0;ggExpFinish();
                ggExpMessage("REIGN SAVED: RETURNED TO THE ORIGINAL STORY");
            }else if(sagaMode!=ggSaga.mode||sagaChoice!=ggSaga.choice)
                ggExpRenderValid=0;
            ggChron.lastKeys=raw;ggReign.lastKeys=raw;ggQuest.lastKeys=raw;
            *held=0;*tx=*ty=0;return;
        }
        // Talk and travel orders are contextual: a safe town's A
        // interaction precedes melee, but enemies always take priority.
        if(ggQuestStep(raw)){
            ggChron.lastKeys=raw;
            ggReign.lastKeys=raw;
            ggExpRenderValid=0;
            *held=0;*tx=*ty=0;return;
        }
        if(ggReignStep(raw)){
            *held=0;*tx=*ty=0;return;
        }
        int priorSaga=ggSaga.mode;
        if(ggChronStep(raw)){
            ggReignSave();
            ggChron.active=0;
            ggExpFinish();
            ggExpMessage("UNWRITTEN REIGN: 2D STORY RESTORED");
        }else{
            // The final-post event queues a full-screen decision. Force
            // its very first picture even before directional input.
            if(priorSaga!=ggSaga.mode)ggExpRenderValid=0;
            ggReignSquadTick();
            if(ggAmbientTick())ggExpRenderValid=0;
            camX=ggExpClamp((ggExplore.px>>8)*8-128,0,WW*8-256);
            camY=ggExpClamp((ggExplore.py>>8)*8-96,0,WH*8-192);
        }
        *held=0;*tx=*ty=0;return;
    }
    if(pressedChron&&!ggExplore.active&&!ggHybridPicking){
        if(ggExpCanEnter()){
            int cx=ggExpClamp((camX+128)>>3,1,WW-2);
            int cy=ggExpClamp((camY+96)>>3,1,WH-2);
            int entered=ggChronBegin(cx,cy,paused);
            if(entered>0){
                // Suppress the launch SELECT+Y until each key is
                // released: it cannot issue a command to the new mode.
                ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
                paused=1;ggExpRenderValid=0;
                ggReignBegin();ggReign.lastKeys=raw;
                ggQuestBegin();ggQuest.lastKeys=raw;
                ggSagaBegin();ggSaga.lastKeys=raw;
                ggAmbientBegin();
                ggOriginBegin();ggOrigin.lastKeys=raw;
                ggExpMessage(ggOrigin.active?
                    "UNWRITTEN REIGN: AN ORDINARY LIFE BEGINS":
                    "THE UNWRITTEN REIGN: RETURN TO YOUR LIFE");
            }else if(entered==-2)
                ggExpMessage("NO SAFE EXPEDITION ROUTE: CHOOSE A LAND REGION");
            else if(entered<0)
                ggExpMessage("NEXT ACT LOCKED: ADVANCE THE 2D STORY");
            else ggExpMessage("UNWRITTEN REIGN UNLOCKS AT CHAPTER 3");
        }
        *held=0;*tx=*ty=0;return;
    }
    if(!ggChronHintShown&&ggChronReady()&&!ggExplore.active&&
       ggExpCanEnter()&&!toastT){
        ggChronHintShown=1;
        ggExpMessage("UNWRITTEN REIGN: SELECT+Y TO ENTER");
    }
    if(pressedPick){
        if(ggExplore.active)ggExpFinish();
        if(ggExpCanEnter()){
            ggHybridPicking=1;
            ggExpMessage("HYBRID PICK: TAP A SAPIENT; B CANCEL");
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
                    ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
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
            int i=(*ty>=0&&*ty<TB_Y)?ggHybridPick(x,y):-1;
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
            int i=(*ty>=0&&*ty<TB_Y)?ggHybridPick(x,y):-1;
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
                // doSave reports SAVED, SAVE FAILED or NO SD CARD into
                // info. Do not overwrite that result with a misleading
                // "requested" toast. Show outcome in the 3D HUD too.
                ggExplore.feedback=!strcmp(info,"SAVED")?4:5;
                ggExplore.feedbackT=150;
                toastT=150;
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
    // The 3D top screen completely replaces the old sub-engine bitmap.
    // Keep rendering and caching the live 2D map on the bottom LCD.
    shimExploreTopOverride=ggExplore.active;
    // Force a lower-screen redraw only when the objective changes. The
    // map's usual camera/terrain invalidation still handles walking.
    static int lastObjectiveSignature=-1;
    int objectiveSignature=ggMapObjectiveSignature();
    if(objectiveSignature!=lastObjectiveSignature){
        shimExternalInvalidate=1;
        lastObjectiveSignature=objectiveSignature;
    }
    shimRenderChanged(topBuf,botBuf,&tc,&bc);
    if(bc)ggMapObjectivesDraw(botBuf);
    if(ggExplore.active){
        // Render on actual camera/world changes, or every eighth frame
        // for subtle clouds/feedback. Static scenes reuse the existing
        // RGB output; there is no second 400x240 framebuffer or hash scan.
        static int lastX,lastY,lastA,lastEdits,lastFx,lastSky;
        int sky=ggExplore.frame/8;
        // During authored 3D movie shots we render at a steady, bounded
        // 30fps-like cadence (every 2nd 60Hz tick), rather than paying the
        // full terrain ray-caster cost each simulation tick. The camera
        // still interpolates from real-world landmarks each rendered frame.
        // A stale ggChron.dirty flag must NOT force every movie frame.
        int movie=ggChron.active&&ggSaga.mode;
        int dirty=!ggExpRenderValid||
                  (movie ? (ggSaga.mode==GG_SAGA_CHOICE?
                            0:!(ggSaga.frame&1)) :
                   tc||ggExplore.px!=lastX||ggExplore.py!=lastY||
                   ggExplore.angle!=lastA||ggExplore.edits!=lastEdits||
                   ggExplore.feedback!=lastFx||sky!=lastSky||
                   (ggChron.active&&(ggOrigin.active||ggChron.dirty||
                                      ggReign.menu||ggReign.noticeT||
                                      !(ggChron.frame&3))));
        if(dirty){
            if(ggChron.active&&ggOrigin.active)
                ggOriginDraw(topBuf);
            else if(ggChron.active&&ggSaga.mode)
                ggSagaDraw(topBuf);
            else{
                ggExpRender(topBuf);
                if(ggChron.active){
                    ggReignSceneDraw(topBuf);
                    ggChronDraw(topBuf);
                    ggReignDraw(topBuf);
                    ggQuestDraw(topBuf);
                }
            }
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
    gg3dMusicTick=ggWorldMusicTick;
#ifdef GG_HQVOX
    gmVoxWarm();     // predecode vocals, never decode in the frame mixer
#endif
#ifdef SHIM_HQ
    shimHqOn=new3ds; // retain the lighter mixer on Old 3DS
#endif
    ds_main(); plat_exit(); return 0;
}
