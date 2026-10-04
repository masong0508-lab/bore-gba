// loading.h - the LOADING screen: a message, a progress bar and a percent, so a long wait never looks like a freeze.
//   ldShow("WHAT IS HAPPENING", done, total);   draws the whole screen once and presents it (call it between the steps of a slow job)
//   ldEnd();                                    when the slow job is over: puts the game's display mode back (see below)
// SOUND: the first ldShow of a job steps the song aside: it fades out completely (~0.5 s) and is then FROZEN, not decoded at all (the music
// mixer is a big part of what makes the bake slow), while a quiet tick-tock fades in under it (it loops, in the audio interrupt, so it keeps
// going however long a step takes). ldEnd() says the job is over: the tick-tock fades out and, a few frames later, the song comes back from
// the exact spot it stopped at (so a loading screen straight after another one does not bring it back in between). If the game starts or
// stops music itself in the meantime (PLAY: the game music, or silence), the stepped-aside song is dropped instead (ldDrop, main.c).
// A job that never calls ldEnd is let go of after LD_STALE normal frames without an ldShow. Without SOUND there is no tick-tock; without a song
// playing there is nothing to step aside.
// Needs before it: REG_DISPCNT, rect, text, tw, box, present, numStr, GOLD, SW, SH. Costs one frame (vsync) per call.
// The game's display mode has WINDOW 0 on (house.h): outside the window only what WINOUT allows shows, and before the first game
// frame sets those registers that is NOTHING, so a loading screen drawn then stays invisible (pure black). ldShow switches the
// window off (plain mode 3 bitmap) and remembers the old mode; ldEnd puts it back (with a full-screen window, so no black gap).
#define LD_GRACE 8     // frames between ldEnd and the song coming back (a second loading screen right after cancels it)
#define LD_STALE 90    // normal frames after the last ldShow without an ldEnd: the job is over after all
static int ldOn, ldIdle;   // ldOn: a loading job is running (its screen has been shown, ldEnd has not been called yet)
static void ldBegin(void){
    ldOn=1; ldIdle=0; ldRel=0;                            // (a song still waiting to come back from the last job stays out of the way)
    if(!ldHold&&mOn&&mPlay){ ldHold=1; }
    if(ldHold) ldGT=0;                                    // fade the song out; at 0 the mixer freezes it
    if(!sSnd) return;
    if(sfxLoop&&sfxV){ sfxFadeT=256; return; }            // the tick-tock of the last job is still fading out: bring it back up
    const u8*b=sfxTab[SFX_TICK];
    sfxV=0; ssrc=b+4; sn=*(const u32*)b; sPos=0; sFr=0; sRd=0; spred=0; sidx=0; sS0=sS1=0;
    sfxLoop=1; sfxFade=0; sfxFadeT=256; sfxOn=1; sfxV=1;  // fades in against the song fading out
    if(!mOn) audStart();
}
static void ldBack(void){   // the job is over (ldEnd, or LD_STALE): the tick-tock fades out, the song is let back in after LD_GRACE frames
    if(!ldOn) return;
    ldOn=0;
    if(ldHold) ldRel=LD_GRACE;
    if(sfxLoop&&sfxV) sfxFadeT=0;
}
static void ldTick(void){   // once per frame (vsync)
    if(ldOn&&++ldIdle>LD_STALE) ldBack();
    if(ldRel>0&&--ldRel==0&&ldHold){ ldHold=0; ldGT=256; }   // the song comes back from where it stopped
}
static u16 ldKeep;   // the display mode ldShow found with the window on (0 = nothing to put back)
static void ldShow(const char*msg,int done,int total){
    if(!ldOn) ldBegin(); else ldIdle=0;
    { u16 d=REG_DISPCNT; if(d&0x2000){ ldKeep=d; REG_DISPCNT=0x0403; } }
    if(total<1) total=1; if(done<0) done=0; if(done>total) done=total;
    int pct=done*100/total, bw=148, fill=bw*done/total;
    rect(0,0,SW,SH,RGB(3,4,8));
    box(36,48,168,60);
    text(120-tw("LOADING",1)/2,54,"LOADING",GOLD,1);
    text(120-tw(msg,1)/2,68,msg,WHITE,1);
    rect(46,82,bw+2,10,RGB(8,10,14)); rect(47,83,fill,8,GOLD);
    { char t[8]; char*e=t; e+=numStr(e,pct); *e++='%'; *e=0; text(120-tw(t,1)/2,95,t,DIMC,1); }
    text(120-tw("PLEASE WAIT",1)/2,101,"PLEASE WAIT",DIMC,1);
    present();
    ldIdle=0;
}
static void winFull(void){   // window 0 over the whole screen with the game's settings, so turning the game's mode on never blacks out the
    *(volatile u16*)0x04000040=240; *(volatile u16*)0x04000044=160;   // picture (the first game frame narrows it to the room view, house.h)
    *(volatile u16*)0x04000048=0x34; *(volatile u16*)0x0400004A=0x04;
}
static void ldEnd(void){ if(ldKeep){ winFull(); REG_DISPCNT=ldKeep; ldKeep=0; } ldBack(); }   // the loading screen stays up until the game's first frame replaces it
