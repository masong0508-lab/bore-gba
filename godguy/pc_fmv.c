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

void shimPresent(void){}

void shimInput(int frame,int *held,int *tx,int *ty){
    *held=0;
    check(frame<1500,"story flow timed out");
    switch(phase){
    case TITLE:
        if(frame==3){
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
            clipStart=frame;
            phase=RUN_OPENING;
        }
        break;
    case RUN_OPENING:
        if(sceneT==1) firstPicture=pictureHash();
        if(sceneT==3*FMV_TICKS+1) check(pictureHash()!=firstPicture,"opening frame did not animate");
        if(!sceneClip){
            check(frame-clipStart==fmvClips[0].frames*FMV_TICKS+1,"opening ended at the wrong time");
            check(uiFlushes-firstFlush==(unsigned)fmvClips[0].frames+1,"opening missed frames or narration");
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
            stUse=20;
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
        if(sceneClip&&sceneT>=2){
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
        if(sceneClip&&sceneT>=2){
            check(sceneClip==&fmvClips[CHAP_N]&&sceneTxt==endTxt&&paused,"finale failed to start");
            *held=KEY_TOUCH;
            *tx=128;
            *ty=178;
            if(sceneT==3) phase=ENDING_CARD;
        }
        break;
    case ENDING_CARD:
        check(!sceneClip&&sceneOn&&sceneTxt==endTxt,"touch skip bypassed the ending narration");
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
        puts("FMV story flow OK (14 chapters and finale)");
        exit(0);
    }
}

int main(void){
    char dir[]="/tmp/gg_fmv.XXXXXX";
    if(!mkdtemp(dir)||chdir(dir)||rmdir(dir)) return 1;
    return ds_main();
}
