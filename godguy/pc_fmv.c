// Exercise the story's opening and finale through the shared game loop.
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main

static int phase, clipStart;
static unsigned firstFlush;
static u32 firstPicture;

static void check(int condition,const char *message){
    if(!condition){
        fprintf(stderr,"FMV FAIL: %s\n",message);
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
    check(frame<1000,"story flow timed out");
    switch(phase){
    case 0:
        if(frame==3){
            startWorld(0);
            check(!sceneClip&&!sceneOn,"free play started a cutscene");
            *held=KEY_A;   // Continue without a save starts a story.
            titleOn=1;
            hasSave=0;
            phase++;
        }
        break;
    case 1:
        if(sceneClip){
            check(sceneClip==fmv_opening&&paused&&dlgPend==0,"opening starts before chapter dialogue");
            firstFlush=uiFlushes;
            clipStart=frame;
            phase++;
        }
        break;
    case 2:
        if(sceneT==1) firstPicture=pictureHash();
        if(sceneT==FMV_TICKS+1) check(pictureHash()!=firstPicture,"opening frame did not animate");
        if(!sceneClip){
            check(frame-clipStart==FMV_FRAMES*FMV_TICKS+1,"opening ended at the wrong time");
            check(uiFlushes-firstFlush==FMV_FRAMES+1,"opening failed to present each frame and its card");
            check(sceneOn&&paused&&sceneTxt==introTxt&&dlgPend==0,"opening narration was bypassed");
            *held=KEY_A;
            phase++;
        }
        break;
    case 3:
        if(dlgOn){
            check(dlgSet==dlgTab[0]&&paused,"chapter dialogue did not follow the opening card");
            *held=KEY_B;
            phase++;
        }
        break;
    case 4:
        check(!paused&&!dlgOn&&!sceneOn,"opening did not restore gameplay");
        paused=1;
        chapter=CHAP_N-1;
        chapterDone();
        phase++;
        break;
    case 5:
        if(dlgOn){
            check(dlgSet==dlgTab[CHAP_N]&&!sceneClip,"finale bypassed the epilogue");
            *held=KEY_B;
            phase++;
        }
        break;
    case 6:
        if(sceneClip&&sceneT>=2){
            check(sceneClip==fmv_finale&&sceneTxt==endTxt&&paused,"finale failed to start");
            *held=KEY_TOUCH;
            *tx=128;
            *ty=178;
            if(sceneT==3) phase++;
        }
        break;
    case 7:
        check(!sceneClip&&sceneOn&&sceneTxt==endTxt,"touch skip bypassed the ending narration");
        phase++;
        break;
    case 8:
        *held=KEY_A;
        phase++;
        break;
    case 9:
        check(sceneOn&&sceneTxt==credTxt&&sceneScroll&&paused,"credits did not follow the ending card");
        phase++;
        break;
    case 10:
        *held=KEY_START;
        phase++;
        break;
    case 11:
        check(!sceneOn&&!sceneClip&&paused&&endShown,"ending did not restore the prior pause state");
        puts("FMV story flow OK");
        exit(0);
    }
}

int main(void){
    char dir[]="/tmp/gg_fmv.XXXXXX";
    if(!mkdtemp(dir)||chdir(dir)||rmdir(dir)) return 1;
    return ds_main();
}
