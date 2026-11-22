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
static int ggHybridWalkable(const Cr *actor,int x,int y){
    if((unsigned)x>=WW||(unsigned)y>=WH)return 0;
    unsigned terrain=world[y][x];
    return terrain<T_N&&((arch[sp[actor->sp].a].mask|trMask(actor))>>terrain&1);
}
#include "hybrid3d.h"
#include "outdoor3d.h"
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
#include "field_ui3d.h"
extern void plat_setCinema(int on);
extern unsigned long long plat_ms(void); extern int plat_init(void); extern int plat_keys(int*tx,int*ty); extern void plat_present(const unsigned char*top,const unsigned char*bot,int topChanged,int botChanged,const short*snd,int frames,const signed char*depth); extern void plat_exit(void); extern int plat_isNew3DS(void); extern void plat_new3dsRequired(void);
static unsigned char topBuf[400*240*3], botBuf[320*240*3];
static short sndBuf[2800*2];
static Gg3dsFrameClock frameClock;
// Two modes share ONE simulation and ONE selection. Select a living
// sentient creature via INSPECT/POSSESS or the explicit hybrid picker.
// Then SELECT+R toggles its 2D tile/3D POV. SELECT+X opens the picker.
// Persistent IDs avoid accidentally controlling another creature if the
// simulation compacts its array after a save.
static int ggHybridPicking,ggPickAndEnter,ggWalkHelp;
static u32 ggHybridSelectedId;
static int ggHybridSelectedIndex=-1;
static int ggExpRenderValid;
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
static int ggFieldMode(void){
    if(shimSwap||sceneClip||(ggChron.active&&(ggOrigin.active||ggSaga.mode)))
        return GG_FIELD_HIDDEN;
    if(ggWalkHelp)return GG_FIELD_GUIDE;
    if(ggChron.active&&(ggQuest.talk||ggReign.menu))return GG_FIELD_MODAL;
    if(ggChron.active)return GG_FIELD_REIGN;
    if(ggExplore.active)return GG_FIELD_WALK;
    if(!ggExpCanEnter())return GG_FIELD_HIDDEN;
    if(ggHybridPicking)return GG_FIELD_PICK;
    return ggChronReady()?GG_FIELD_MAP_READY:GG_FIELD_MAP_LOCKED;
}
static int ggExpEnterPerson(int index){
    if(!ggHybridBegin(index,paused))return 0;
    ggExpRemember(index);
    ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
    paused=1;
    ggHybridPicking=0;
    ggOutdoorReset();
    ggExpRenderValid=0;
    return 1;
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
    ggWalkHelp=0;
    ggOutdoorReset();
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
    int pressedView=((raw&(KEY_SELECT|KEY_L|KEY_R))==(KEY_SELECT|KEY_R))&&
                    ((previousRaw&(KEY_SELECT|KEY_R))!=(KEY_SELECT|KEY_R));
    int pressedWalk=((raw&(KEY_SELECT|KEY_L|KEY_R))==(KEY_SELECT|KEY_L))&&
                    ((previousRaw&(KEY_SELECT|KEY_L))!=(KEY_SELECT|KEY_L));
    int pressedField=((raw&(KEY_L|KEY_A))==(KEY_L|KEY_A))&&
                     ((previousRaw&(KEY_L|KEY_A))!=(KEY_L|KEY_A));
    int pressedPick=((raw&(KEY_SELECT|KEY_X))==(KEY_SELECT|KEY_X))&&
                    ((previousRaw&(KEY_SELECT|KEY_X))!=(KEY_SELECT|KEY_X));
    int pressedChron=((raw&(KEY_SELECT|KEY_Y))==(KEY_SELECT|KEY_Y))&&
                     ((previousRaw&(KEY_SELECT|KEY_Y))!=(KEY_SELECT|KEY_Y));
    int down=raw&~previousRaw;
    previousRaw=raw;
    int fieldMode=ggFieldMode();
    int button=(down&GG_3DS_SHORTCUT_TOUCH)?ggFieldButtonAt(fieldMode,*tx,*ty):-1;
    if(ggWalkHelp){
        if(button==1||(down&(KEY_A|KEY_B|KEY_START|KEY_SELECT))){
            ggWalkHelp=0;
            ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
            ggExpRenderValid=0;
        }
        *held=0;*tx=*ty=0;return;
    }
    if(button>=0){
        if(fieldMode==GG_FIELD_MAP_LOCKED||fieldMode==GG_FIELD_MAP_READY){
            pressedWalk=button==0;pressedPick=button==1;pressedChron=button==2;
        }else if(fieldMode==GG_FIELD_PICK){
            ggHybridPicking=0;
            ggExpMessage("CHOOSE A 3D VIEW ON THE MAP");
            *held=0;*tx=*ty=0;return;
        }else if(fieldMode==GG_FIELD_MODAL){
            raw|=KEY_B;
        }else if(button==1){
            ggWalkHelp=1;
            if(ggChron.active)ggChronCombatStow();
            ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
            ggExpRenderValid=0;
            *held=0;*tx=*ty=0;return;
        }else if(fieldMode==GG_FIELD_REIGN){
            raw|=button==0?KEY_START:KEY_SELECT;
        }else{
            pressedWalk=button==0;pressedField=button==2;
        }
    }
    raw&=~GG_3DS_SHORTCUT_TOUCH;
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
        int sagaPaused=ggSaga.paused;
        int sagaAction=ggSagaStep(raw);
        if(sagaAction){
            ggChronCombatStow();
            if(sagaAction==2){
                ggReignSave();ggChron.active=0;ggExpFinish();
                ggExpMessage("REIGN SAVED: RETURNED TO THE ORIGINAL STORY");
            }else if(sagaMode!=ggSaga.mode||sagaChoice!=ggSaga.choice||
                     sagaPaused!=ggSaga.paused)
                ggExpRenderValid=0;
            ggChron.lastKeys=raw;ggReign.lastKeys=raw;ggQuest.lastKeys=raw;
            *held=0;*tx=*ty=0;return;
        }
        // Talks and travel orders use A in safe towns. Nearby enemies
        // leave the attack control available.
        if(ggQuestStep(raw)){
            ggChronCombatStow();
            ggChron.lastKeys=raw;
            ggReign.lastKeys=raw;
            ggExpRenderValid=0;
            *held=0;*tx=*ty=0;return;
        }
        if(ggReignStep(raw)){
            ggChronCombatStow();
            ggChron.lastKeys=raw;
            ggExpRenderValid=0;
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
    if(pressedWalk){
        if(ggExplore.active){
            ggExpFinish();
            ggExpMessage("BACK TO THE MAP");
        }else if(ggExpCanEnter()){
            int x=(camX+128)>>3,y=(camY+96)>>3;
            ggExplore.actorId=0;
            if(ggExpBegin(x,y,paused)){
                ggModeInputTransition(&gg3dModeInput,gg3dPhysicalKeys);
                paused=1;
                ggHybridPicking=0;
                ggOutdoorReset();
                ggExpRenderValid=0;
                ggExpMessage("FREE WALK: L+A EXPLORES PLACES");
            }else ggExpMessage("WALK FROM A LAND REGION");
        }
        *held=0;*tx=*ty=0;return;
    }
    if(pressedPick){
        if(ggExplore.active)ggExpFinish();
        if(ggExpCanEnter()){
            ggHybridPicking=1;
            ggPickAndEnter=button==1;
            ggExpMessage("TAP A PERSON ON THE MAP");
        }
        *held=0;*tx=*ty=0;return;
    }
    if(pressedView){
        if(ggExplore.active){
            ggExpFinish();
            ggExpMessage("HYBRID: BACK TO 2D; SELECT+R FOR 3D");
        }else if(ggExpCanEnter()){
            int i=ggExpSelected();
            if(i>=0)ggExpEnterPerson(i);
            else{
                // No playable mind selected: stay in 2D, prompt a
                // deliberate choice instead of possessing a random animal.
                ggHybridPicking=1;
                ggPickAndEnter=1;
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
                if(!ggPickAndEnter||!ggExpEnterPerson(i)){
                    ggExpRemember(i);
                    ggHybridPicking=0;
                    char nm[48];crName(&cr[i],nm);
                    snprintf(info,sizeof info,"FOLLOW %.36s WITH SELECT+R",nm);
                    toastT=420;
                    shimExternalInvalidate=1;
                }
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
                ggOutdoorReset();
                ggExpRenderValid=0;
            }
        }
        if(!(raw&KEY_A))ggOutdoorCaptureA=0;
        if(pressedField){
            ggOutdoorCaptureA=(raw&KEY_A)!=0;
            ggOutdoorUse();
            ggExpRenderValid=0;
        }
        if(ggOutdoorCaptureA)raw&=~(KEY_A|KEY_L);
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
    *held=raw&~(GG_3DS_ZL|GG_3DS_ZR);
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
    // Fixed scenic LOD budgets follow this existing hardware/frame-clock
    // decision; never steal audio time for decorative foliage.
    ggExpScenicQuality=gg3dsGlobeQuality?2:1;
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
    static int lastFieldMode;
    int fieldMode=ggFieldMode();
    if(!shimFilmFullscreen&&(fieldMode||lastFieldMode)&&(bc||fieldMode!=lastFieldMode)){
        ggFieldStripDraw(botBuf,fieldMode);
        bc=1;
    }
    lastFieldMode=fieldMode;
    if(ggExplore.active){
        // Render on actual camera/world changes, or every eighth frame
        // for subtle clouds/feedback. Static scenes reuse the existing
        // RGB output; there is no second 400x240 framebuffer or hash scan.
        static int lastX,lastY,lastA,lastEdits,lastFx,lastSky,lastMaterial;
        static int lastMeshQuality=-1;
        int sky=ggExplore.frame/8;
        // During authored 3D movie shots we render at a steady, bounded
        // 30fps-like cadence (every 2nd 60Hz tick), rather than paying the
        // full terrain ray-caster cost each simulation tick. The camera
        // still interpolates from real-world landmarks each rendered frame.
        // A stale ggChron.dirty flag must NOT force every movie frame.
        int movie=ggChron.active&&ggSaga.mode;
        // A quality transition requires a fresh real-3D projection,
        // even when the camera is still and no sky tick is due.
        int dirty=!ggExpRenderValid||
                  ggExpScenicQuality!=lastMeshQuality||
                  (movie ? (ggSaga.mode==GG_SAGA_CHOICE||ggSaga.paused?
                            0:!(ggSaga.frame&1)) :
                   tc||ggExplore.px!=lastX||ggExplore.py!=lastY||
                   ggExplore.angle!=lastA||ggExplore.edits!=lastEdits||
                   ggExplore.material!=lastMaterial||
                   ggExplore.feedback!=lastFx||sky!=lastSky||
                   (ggChron.active&&!ggWalkHelp&&(ggOrigin.active||ggChron.dirty||
                                      ggReign.menu||ggReign.noticeT||
                                      !(ggChron.frame&3))));
        if(dirty){
            if(ggChron.active&&ggOrigin.active)
                ggOriginDraw(topBuf);
            else if(ggChron.active&&ggSaga.mode)
                ggSagaDraw(topBuf);
            else{
                ggExpBuildPreview=!ggChron.active;
                ggExpRender(topBuf);
                if(ggChron.active){
                    int modal=ggWalkHelp||ggReign.menu||ggQuest.talk;
                    char hint[64];
                    const char *context=modal?NULL:ggReign.noticeT?ggReign.notice:
                                        ggQuestFieldHint(hint,sizeof hint);
                    ggReignSceneDraw(topBuf);
                    ggChronDraw(topBuf,context,modal);
                    ggReignDraw(topBuf);
                    ggQuestDraw(topBuf);
                }else ggOutdoorHud(topBuf);
                if(ggWalkHelp)ggFieldGuideDraw(topBuf,ggChron.active,ggExplore.actorId!=0);
            }
            ggExpRenderValid=1;
            lastX=ggExplore.px;lastY=ggExplore.py;
            lastA=ggExplore.angle;lastEdits=ggExplore.edits;
            lastMaterial=ggExplore.material;
            lastFx=ggExplore.feedback;lastSky=sky;
            lastMeshQuality=ggExpScenicQuality;
            tc=1;
        }
    }
    // While exploring, the left-eye RGB image now contains real
    // perspective polygons with physical depth; the 2D globe's
    // unrelated disparity map must never warp these world surfaces.
    plat_present(topBuf,botBuf,tc,bc,sndBuf,n,
                 ggExplore.active?0:(shimSwap?0:shimDepth));
}
#ifdef GG_TEST_NATIVE_ORIGIN
#define main ggNativeFirmwareMain
#endif

int main(void){
    plat_init();
    int new3ds=plat_isNew3DS();
#ifdef GG_NEW3DS_ONLY
    if(!new3ds){
        plat_new3dsRequired();
        plat_exit();
        return 0;
    }
#endif
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

#ifdef GG_TEST_NATIVE_ORIGIN
#undef main
#endif
