// Regression tests for three things, run on the REAL DS program (source/main.c) headless through compat/ndsshim.h:
//   menu     the pause menu used to send two pictures to the screen every frame (the bare status screen, then the menu on top of it): it flickered. Now one picture, and only when something changed.
//   chap     a chapter goal that is reached must move the story on: with the world paused (A), and at the 2x / 4x game speeds (the tick the check needed was never simulated)
//   music    a song ends: it fades out and the next one follows in the game; played on its own in the Aural Sculptures it fades to silence and stops
//   gcc -O2 -DNDSSHIM -Icompat -Isource -include unistd.h -o pc_fix pc_fix.c -lm && ./pc_fix menu|chap|music
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static const char*mode="menu"; static int arg1;
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)

// ---- menu
static unsigned fl0; static int menuFrames, menuFl, menuMax, idleFrames, idleFl, pressFrames, pressFl;
static u32 hashBmp(void){ u32 h=2166136261u; for(int i=0;i<256*192;i++) h=(h^uiBuf[i])*16777619u; return h; }
static u32 menuHash; static int p1,p2;
// ---- chapter
static int chapAt=-1, chapSet;
// ---- music
static int songFrom, songAt=-1, songTo=-2, minFade=256, fadeAtEnd=-1, silentFrames, startF; static int firstOut;
static int musicChans(void){ int n=0; for(int i=0;i<16;i++) if(shimCh[i].on&&shimCh[i].kind==1&&shimCh[i].vol>0) n++; return n; }

void shimInput(int frame,int*held,int*tx,int*ty){
    (void)tx;(void)ty; *held=0;
    if(frame==3){ stUse=5; startWorld(1); }   // a story world, no intro scene
    if(!strcmp(mode,"menu")){
        if(frame>=10&&frame<12) *held=KEY_START;                    // open the pause menu
        if(frame>=60&&frame<62) *held=KEY_DOWN;
        if(frame>=100&&frame<102) *held=KEY_DOWN;
        if(frame==400){
            printf("pause menu, %d frames shown:\n",menuFrames);
            CHECK(menuFrames>300,"the menu opened and stayed open (%d frames)",menuFrames);
            CHECK(menuMax<=1,"never more than one picture sent in a frame (most in one frame: %d)",menuMax);
            CHECK(idleFl*8<idleFrames,"idle frames send almost nothing (%d pictures in %d idle frames)",idleFl,idleFrames);
            CHECK(p1&&p2,"each button press redraws the menu (%d, %d)",p1,p2);
            printf("  info  total pictures %d in %d frames; picture hash at the end %08x\n",menuFl,menuFrames,menuHash);
            printf(fails?"FAILED\n":"PASSED\n"); exit(fails?1:0); }
    }
    else if(!strcmp(mode,"chap")){   // arg1: 0 normal, 1 paused world, 2 = 2x, 4 = 4x ; the second argument shifts the frame the goal is reached on (the tick parity)
        int f0=10+(arg1>=100?arg1-100:0); int kind=arg1>=100?arg1/100:arg1; (void)kind;
        if(frame==f0){ chapSet=1; stUse=20; int k=arg1%100; if(k==1) paused=1; if(k==2) speedMul=2; if(k==4) speedMul=4; }
        if(chapSet&&chapAt<0&&chapter>=1) chapAt=frame-f0;
        if(frame==f0+700){
            int k=arg1%100;
            printf("chapter 1 goal reached (%s%s):\n",k==1?"world paused":k==2?"speed 2x":k==4?"speed 4x":"normal speed",arg1>=100?", odd tick":"");
            CHECK(chapter>=1,"the story moved on to chapter %d%s",chapter+1,chapAt>=0?"":"");
            if(chapAt>=0) printf("  info  after %d frames\n",chapAt);
            printf(fails?"FAILED\n":"PASSED\n"); exit(fails?1:0); }
    }
    else if(!strcmp(mode,"music")){   // arg1: 0 the game (a song ends, the next follows), 1 played on its own in the Aural Sculptures
        const int S=36;   // A THOUSAND YEARS, the shortest song that is not a jingle: 48 s
        if(frame==5){ gmStop(); musQ=-1; musHold=999999; if(arg1==1){ jukeOn=1; } gmPlay(S); songFrom=S; startF=frame; }
        if(frame>5){
            int f=gmCur>=0?gmFadeMul():256; if(gmCur==S&&f<minFade) minFade=f;
            if(gmCur!=S&&songAt<0){ songAt=frame; songTo=gmCur; if(musicChans()==0) firstOut=1; }
            if(songAt>=0&&gmCur<0&&musicChans()==0) silentFrames++;
        }
        if(frame==5+4200){
            double secs=(songAt-startF)/60.0;
            printf("music, %s:\n",arg1?"a song played on its own (Aural Sculptures)":"in the game");
            CHECK(songAt>0,"the song ended by itself (after %.1f s; it is 48 s long)",secs);
            CHECK(minFade<=8,"it faded out first (lowest volume factor %d of 256)",minFade);
            if(arg1==0){ CHECK(songTo>=0&&songTo!=S,"and the next song took over: %s",songTo>=0?gmSongs[songTo].name:"(none)"); CHECK(gmCur>=0,"music is still playing now: %s",gmCur>=0?gmSongs[gmCur].name:"-"); }
            else { CHECK(gmCur<0,"it stopped, and stays silent (song now: %d)",gmCur); CHECK(silentFrames>600,"%d silent frames after the end",silentFrames); }
            printf(fails?"FAILED\n":"PASSED\n"); exit(fails?1:0); }
    }
    if(!strcmp(mode,"menu")&&frame>=14){}
}
void shimPresent(void){
    if(!strcmp(mode,"menu")){
        static unsigned last; static int init;
        if(!init){ last=uiFlushes; init=1; }
        if(menuOn&&shimFrame>=14){ int d=(int)(uiFlushes-last); menuFrames++; menuFl+=d; if(d>menuMax) menuMax=d;
            int w1=shimFrame>=60&&shimFrame<66, w2=shimFrame>=100&&shimFrame<106; if(w1||w2){ if(d) { if(w1) p1=1; else p2=1; } } else if(shimFrame>=130){ idleFrames++; idleFl+=d; } menuHash=hashBmp(); }
        last=uiFlushes; }
}
int main(int argc,char**argv){ if(argc>1) mode=argv[1]; if(argc>2) arg1=atoi(argv[2]); if(chdir("/tmp")) return 1; return ds_main(); }
