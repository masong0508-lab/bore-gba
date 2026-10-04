// loading.h - the LOADING screen: a message, a progress bar and a percent, so a long wait never looks like a freeze.
//   ldShow("WHAT IS HAPPENING", done, total);   draws the whole screen once and presents it (call it between the steps of a slow job)
//   ldEnd();                                    when the slow job is over: puts the game's display mode back (see below)
// Needs before it: REG_DISPCNT, rect, text, tw, box, present, numStr, GOLD, SW, SH. Costs one frame (vsync) per call.
// The game's display mode has WINDOW 0 on (house.h): outside the window only what WINOUT allows shows, and before the first game
// frame sets those registers that is NOTHING, so a loading screen drawn then stays invisible (pure black). ldShow switches the
// window off (plain mode 3 bitmap) and remembers the old mode; ldEnd puts it back (with a full-screen window, so no black gap).
static u16 ldKeep;   // the display mode ldShow found with the window on (0 = nothing to put back)
static void ldShow(const char*msg,int done,int total){
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
}
static void winFull(void){   // window 0 over the whole screen with the game's settings, so turning the game's mode on never blacks out the
    *(volatile u16*)0x04000040=240; *(volatile u16*)0x04000044=160;   // picture (the first game frame narrows it to the room view, house.h)
    *(volatile u16*)0x04000048=0x34; *(volatile u16*)0x0400004A=0x04;
}
static void ldEnd(void){ if(ldKeep){ winFull(); REG_DISPCNT=ldKeep; ldKeep=0; } }   // the loading screen stays up until the game's first frame replaces it
