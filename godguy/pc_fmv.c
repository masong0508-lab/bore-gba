// Exercise the chapter interludes through the shared game loop.
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main

enum {
    TITLE, OPENING, RUN_OPENING, OPENING_DIALOGUE, NEXT_CHAPTER, CHAPTER_FILM,
    CHAPTER_DIALOGUE, TOUCH_SKIP_DIALOGUE, EXTRAS_BREAK, EPILOGUE, FINALE, ENDING_CARD, SHOW_CREDITS, SKIP_CREDITS, DONE,
};
static int phase, clipStart, handoffStart, expectedChapter=1;
static unsigned firstFlush;
static int firstWorldTick;
static u32 firstPicture;

static void check(int condition,const char *message){
    if(!condition){
        fprintf(stderr,"FMV FAIL: %s (chapter %d)\n",message,chapter);
        exit(1);
    }
}

static u32 pictureHash(void){
    u32 hash=2166136261u;
    for(int i=24*256;i<168*256;i++) hash=(hash^uiBuf[i])*16777619u;
    return hash;
}

static void checkFullPlayback(void){
    for(unsigned i=0;i<sizeof fmvClips/sizeof *fmvClips;i++){
        const FmvClip*clip=&fmvClips[i];
        u32 previous=0;
        int changes=0, still=0;
        unsigned flush=uiFlushes;
        for(int f=0;f<clip->frames;f++){
            fxQ=-1;
            fmvDecode(clip,f);
            uiFlushRaw();
            int want=-1;
            for(int c=0;c<2;c++) if(clip->cue[c].effect&&clip->cue[c].frame==f) want=clip->cue[c].effect;
            check(fxQ==want,"sound cue did not land on its frame");
            check(fmvRead<=fmvBlockEnd,"frame ran past its delta block");
            u32 now=pictureHash();
            changes+=f>0&&previous!=now;
            still=previous==now?still+1:0;
            check(still<3,"a frame stayed frozen for too long");
            previous=now;
        }
        check(changes>=clip->frames/4,"film held the same picture too long");
        check(uiFlushes-flush==(unsigned)clip->frames,"film dropped a frame");
        check(fmvRead==fmvBlockEnd,"film left unread delta commands");
    }
    fxQ=-1;
}

// Film-direction contract: lock the lens after choosing a shot, give every
// actor a consistent screen side, and never enlarge Wisp to human scale.
static void checkDirectedBlocking(void){
    for(unsigned i=0;i<sizeof filmReels/sizeof *filmReels;i++){
        const FilmReel *reel=&filmReels[i];
        for(int n=0;n<reel->count;n++){
            const FilmBeat *beat=&reel->beats[n];
            FilmFraming first=filmFraming(beat,0),last=filmFraming(beat,256);
            check(first.dx==last.dx&&first.dy==last.dy,
                  "a scene zoomed while the characters were speaking");
            check(first.y==last.y,"a shot drifted vertically");
            if(beat->camera==F_TRACK){
                check(last.x>first.x&&last.x-first.x<=6*256,
                      "camera tracking traveled too far");
            } else check(first.x==last.x,"a locked camera moved");
            // Each 34-pixel cast cel stays within 1.5x native scale.
            // A 20-pixel prop insert can never become a 50-pixel zoom.
            check(first.dx>=FMV_W*FILM_PLATE_SCALE-4*FILM_PLATE_SCALE,
                  "a scene cropped too far into the backdrop");
            if(beat->camera!=F_TRACK)
                check(first.dx==FMV_W*FILM_PLATE_SCALE,
                      "a close-up digitally magnified the set");
            FilmBlocking start=filmBlocking(beat,0);
            FilmBlocking finish=filmBlocking(beat,250);
            check(start.leadHeight<=FILM_CEL_H*3/2&&
                  start.otherHeight<=FILM_CEL_H*3/2,
                  "a close-up enlarged an actor beyond the sharp source art");
            if(beat->lead==F_WISP) check(start.leadHeight<=36,"Wisp was scaled to human height");
            if(beat->other==F_WISP) check(start.otherHeight<=36,"Wisp was scaled to human height");
            if(beat->camera!=F_TRACK) check(start.leadX==finish.leadX,
                   "a stationary conversation moved the lead actor");
            if(beat->lead<F_ACTOR_N&&beat->other<F_ACTOR_N){
                check(start.leadX!=start.otherX,"speakers overlap one another");
                for(int j=n+1;j<reel->count;j++){
                    const FilmBeat *next=&reel->beats[j];
                    if(beat->lead==next->other&&beat->other==next->lead){
                        FilmBlocking converse=filmBlocking(next,250);
                        check((finish.leadX<finish.otherX)==
                              (converse.otherX<converse.leadX),
                              "a speaker switched screen sides between cuts");
                    }
                }
            }
        }
    }
    puts("FMV direction OK: natural cel scale, stable speaker sides, fixed lenses, modest tracking and small Wisp");
}

// The physical-story contract goes beyond changing pixels: cast hands
// actually reach prop grips, an object rests on the surface, locations
// establish before actors enter, and the first act has causal geography.
static void checkPhysicalStoryContinuity(void){
    const FilmReel *opening=&filmReels[0],*rain=&filmReels[1],*roof=&filmReels[2];
    check(opening->count==8&&rain->count==8&&roof->count==8,
          "opening three-chapter shot order changed unexpectedly");
    for(int i=0;i<opening->count;i++)
        check(opening->beats[i].place==(i<2?F_ORBIT:i<7?F_SHORE:F_VILLAGE),
              "the first-light story jumped back to an earlier location");
    // The baked first-rain film ends on the porch. The new edit cannot
    // jump backward outside before Mira takes and puts the same cup down.
    for(int i=0;i<rain->count;i++)
        check(rain->beats[i].place==(i<7?F_PORCH:F_VILLAGE),
              "rain edit jumped away from the baked porch scene");
    for(int i=0;i<roof->count;i++)
        check(roof->beats[i].place==(i<7?F_VILLAGE:F_PORCH),
              "construction jumped into a different building");
    check(rain->beats[3].action==F_ACT_TAKE&&
          rain->beats[5].action==F_ACT_PUT&&
          roof->beats[1].action==F_ACT_TAKE&&
          roof->beats[2].action==F_ACT_WORK,
          "script lost the visible pickup, putdown or building action");
    for(unsigned i=0;i<sizeof fmvClips/sizeof *fmvClips;i++){
        const FilmReel *reels[2]={&filmReels[i],NULL};
#ifdef __3DS__
        reels[1]=&filmExtraReels[i];
#endif
        for(int group=0;group<2;group++){
            if(!reels[group])continue;
            for(int n=0;n<reels[group]->count;n++){
                const FilmBeat *beat=&reels[group]->beats[n];
                check(beat->action<=F_ACT_RECEIVE,"unknown actor prop action");
                if(beat->action!=F_ACT_AUTO){
                    check(beat->prop<F_PROP_N&&beat->lead<F_ACTOR_N,
                          "story asked a missing character to pick up a prop");
                }
                const FilmBeat *prev=filmPreviousBeat(&fmvClips[i],beat);
                if(n>0)check(prev==beat-1,"scene edit lost its previous shot");
                if(beat->prop<F_PROP_N&&beat->prop!=F_GOAT&&
                   beat->lead<F_ACTOR_N&&beat->lead!=F_WISP){
                    FilmBlocking b=filmBlocking(beat,180);
                    int paired=beat->other<F_ACTOR_N;
                    int flip=paired&&b.leadX>b.otherX;
                    FilmPoint palm=filmHandPoint(beat->lead,3,b.leadX,
                                                b.leadFeet,b.leadHeight,flip);
                    int size=beat->camera==F_WIDE?17:19;
                    FilmPoint center=filmPropCenterForGrip(beat->prop,size,palm);
                    FilmPoint pivot=filmPropGripPoint(beat->prop);
                    int gripX=center.x-size/2+size*pivot.x/20;
                    int gripY=center.y-size+size*pivot.y/20;
                    check(gripX>=palm.x-2&&gripX<=palm.x+2&&
                          gripY>=palm.y-2&&gripY<=palm.y+2,
                          "prop handle missed the actor's real hand");
                    check(palm.y>=b.leadFeet-b.leadHeight*3/4&&
                          palm.y<=b.leadFeet,
                          "prop grip floats above character's upper body");
                }
            }
        }
    }
    check(finaleBeats[19].action==F_ACT_RECEIVE,
          "the student mistakenly handed the hammer away from the teacher");
    check(rain->beats[0].place==F_PORCH&&
          rain->beats[2].camera==F_INSERT&&
          rain->beats[3].place==rain->beats[5].place,
          "rain prologue-to-directed cup blocking lost continuity");
    FilmPoint porch=filmSurfacePoint(firstRainBeats+5,90);
    FilmPoint porchReverse=filmSurfacePoint(firstRainBeats+4,170);
    check(porch.x==porchReverse.x&&porch.y==porchReverse.y,
          "the cup's porch sill shifted between camera reverses");
    puts("FMV continuity OK: two-way handoffs, hand grips, grounded props and stable locations");
}

static void checkDirectedPlayback(void){
    for(unsigned i=0;i<sizeof fmvClips/sizeof *fmvClips;i++){
        const FmvClip *clip=&fmvClips[i];
        int length=fmvLength(clip), baked=clip->frames*FMV_TICKS;
        int spotlight=i==9||i==13||i==14;
        check(length>=60*FILM_SECOND,"a chapter film is shorter than one minute");
        if(spotlight) check(length>180*FILM_SECOND,"a centerpiece runs three minutes or less");
        unsigned previous=0;
        int same=0, shots=0, frames=0;
        for(int t=baked;t<length;t+=FILM_STAGE_TICKS){
            int elapsed=t-baked;
            const FilmBeat *beat=filmBeatAt(clip,&elapsed);
            check(beat!=NULL,"directed timeline has a gap");
            if(!elapsed){
                check(!beat->line||uiTextW(beat->line,1)<=248,"caption exceeds the safe width");
                shots++;
            }
            fxQ=-1;
            check(fmvRenderAt(clip,t),"directed edit dropped a scheduled frame");
            check(fxQ==(!elapsed&&beat->sound?beat->sound:-1),"directed sound cue played outside its beat");
            unsigned hash=pictureHash();
            same=previous==hash?same+1:0;
            // Real films allow quiet holds and reaction beats; reject a
            // genuinely stuck renderer only after 1.5 seconds unchanged.
            check(same*FILM_STAGE_TICKS<90,"a directed take froze for over 1.5 seconds");
            previous=hash;
            frames++;
        }
        check(!fmvRenderAt(clip,length),"a film drew past its end");
        check(shots>=8&&frames==(length-baked)/FILM_STAGE_TICKS,"the edit skipped a take or a frame");
#ifdef __3DS__
        check(shots==filmReels[i].count+filmExtraReels[i].count,"a 3DS scene missed its additional takes");
#endif
        printf("%-27s %6.2fs  %2d directed takes\n",clip->title,length/60.0,shots);
    }
    fxQ=-1;
}

void shimPresent(void){
    if(sceneClip||dlgOn||sceneOn){
        check(shimSwap,"story appeared on the wrong LCD");
        check(!memcmp(bgGetGfxPtr(3),uiBuf,sizeof uiBuf),"presented a stale or partially drawn story frame");
    }
}

void shimInput(int frame,int *held,int *tx,int *ty){
    *held=0;
    check(frame<fmvLength(&fmvClips[0])+fmvLength(&fmvClips[CHAP_N])+2500,"story flow timed out");
    switch(phase){
    case TITLE:
        if(frame==3){
            checkFullPlayback();
            checkDirectedBlocking();
            checkPhysicalStoryContinuity();
            checkDirectedPlayback();
            startWorld(0);
            check(!sceneClip&&!sceneOn,"free play started a cutscene");
            *held=KEY_A;   // Continue without a save starts a story.
            titleOn=1;
            hasSave=0;
            phase=OPENING;
        }
        break;
    case OPENING:
        if(sceneClip){
            check(sceneClip==&fmvClips[0]&&paused&&dlgPend==0,"opening starts before chapter dialogue");
            firstFlush=uiFlushes;
            firstWorldTick=tick;
            clipStart=frame;
            phase=RUN_OPENING;
        }
        break;
    case RUN_OPENING:
        if(sceneT==1) firstPicture=pictureHash();
        if(sceneT==6*FMV_TICKS+1) check(pictureHash()!=firstPicture,"opening frame did not animate");
        if(!sceneClip){
            const FmvClip *clip=&fmvClips[0];
            int length=fmvLength(clip), staged=(length-clip->frames*FMV_TICKS)/FILM_STAGE_TICKS;
            check(frame-clipStart==length+1,"opening ended at the wrong time");
            check(tick==firstWorldTick,"cinematic time advanced the world clock");
            check(storyScore<0,"the cinematic score escaped into gameplay");
            check(uiFlushes-firstFlush==(unsigned)(clip->frames+staged+1),"opening missed frames or narration");
            check(sceneOn&&paused&&sceneTxt==introTxt&&dlgPend==0,"opening narration was bypassed");
            *held=KEY_A;
            phase=OPENING_DIALOGUE;
        }
        break;
    case OPENING_DIALOGUE:
        if(dlgOn){
            check(dlgSet==dlgTab[0]&&paused,"chapter dialogue did not follow the opening card");
            *held=KEY_B;
            phase=NEXT_CHAPTER;
        }
        break;
    case NEXT_CHAPTER:
        check(!paused&&!dlgOn&&!sceneOn,"dialogue did not restore gameplay");
        check(chapter==expectedChapter-1,"unexpected story progress");
        check(storyGoalT==STORY_HANDOFF_TICKS,"dialogue did not show the current objective");
        if(expectedChapter==1){
            // Let a real completed objective trigger after the handoff, at 4x sim speed.
            stUse=chap[0].target;
            speedMul=4;
            handoffStart=frame;
        } else {
            if(expectedChapter==CHAP_N) paused=1;
            chapterDone();
        }
        if(expectedChapter==2){
            extrasView=1;
            handoffStart=frame;
            phase=EXTRAS_BREAK;
        } else {
            phase=expectedChapter<CHAP_N?CHAPTER_FILM:EPILOGUE;
        }
        break;
    case EXTRAS_BREAK:
        check(!sceneClip&&!dlgOn&&dlgPend==expectedChapter,"a film interrupted Extras");
        if(frame-handoffStart==6){
            extrasView=0;
            phase=CHAPTER_FILM;
        }
        break;
    case CHAPTER_FILM:
        if(expectedChapter==1&&frame-handoffStart<STORY_HANDOFF_TICKS){
            check(chapter==0&&!sceneClip,"another chapter interrupted the objective handoff");
        }
        if(sceneClip&&sceneT>=(expectedChapter==1?fmvClips[1].frames*FMV_TICKS+2:2)){
            check(sceneClip==&fmvClips[expectedChapter],"chapter picked the wrong film");
            check(dlgOn&&dlgSet==dlgTab[expectedChapter]&&!dlgIdx&&!dlgChars&&paused,
                  "chapter dialogue advanced during the film");
            *held=KEY_START;
            phase=CHAPTER_DIALOGUE;
        }
        break;
    case CHAPTER_DIALOGUE:
        check(!sceneClip&&!sceneOn&&dlgOn&&paused&&!dlgIdx&&!dlgChars,
              "skip bypassed the first line or replayed the film");
        if(expectedChapter==1){
            speedMul=1;
            *held=KEY_TOUCH;
            *tx=146;
            *ty=165;
            phase=TOUCH_SKIP_DIALOGUE;
        } else {
            *held=KEY_B;
            expectedChapter++;
            phase=NEXT_CHAPTER;
        }
        break;
    case TOUCH_SKIP_DIALOGUE:
        // Two stylus samples settle a contact on the libnds / shim path.
        *held=KEY_TOUCH;
        *tx=146;
        *ty=165;
        expectedChapter++;
        phase=NEXT_CHAPTER;
        break;
    case EPILOGUE:
        if(dlgOn){
            check(dlgSet==dlgTab[CHAP_N]&&!sceneClip,"finale bypassed the epilogue");
            *held=KEY_B;
            phase=FINALE;
        }
        break;
    case FINALE:
        if(sceneClip&&sceneT==2){
            check(sceneClip==&fmvClips[CHAP_N]&&sceneTxt==endTxt&&paused,"finale failed to start");
            clipStart=frame;
        }
        if(!sceneClip&&clipStart&&sceneTxt==endTxt){
            check(frame-clipStart==fmvLength(&fmvClips[CHAP_N])-1,"the full finale ended at the wrong time");
            phase=ENDING_CARD;
        }
        break;
    case ENDING_CARD:
        check(!sceneClip&&sceneOn&&sceneTxt==endTxt,"the finale bypassed the ending narration");
        phase=SHOW_CREDITS;
        break;
    case SHOW_CREDITS:
        *held=KEY_A;
        phase=SKIP_CREDITS;
        break;
    case SKIP_CREDITS:
        check(sceneOn&&sceneTxt==credTxt&&sceneScroll&&paused,"credits did not follow the ending card");
        *held=KEY_START;
        phase=DONE;
        break;
    case DONE:
        check(!sceneOn&&!sceneClip&&paused&&endShown,"ending did not restore the prior pause state");
        puts("FMV playback, sound cues and story flow OK (14 chapters and finale)");
        exit(0);
    }
}

int main(void){
    char dir[]="/tmp/gg_fmv.XXXXXX";
    if(!mkdtemp(dir)||chdir(dir)||rmdir(dir)) return 1;
    return ds_main();
}
