// tutorial.h - the TUTORIAL, in the spirit of The Sims 2's learn-to-play tutorial (a dedicated "Tutorial" neighborhood where
// nobody can die, guided pop-up lessons, one task at a time, the next lesson only when you did the task).
//
// How it works (all state is RAM only; the only saved byte is xo[XO_TUTOR]: OFFER / DONE / REPLAY):
//   * It runs on top of the normal life game and only LOOKS at the game (position, board, fridge, pause menu). It never edits the life,
//     the room or any save, so it cannot damage anything. Nobody dies while it runs (die() in main.c checks tutOn).
//   * A lesson = a pop-up card (A go, B skip the lesson, SELECT end the tutorial), then a short goal line in the top bar of the HUD.
//     When the goal is done, "GOOD JOB" and the next card. A lesson you cannot finish for 45 seconds offers a way out.
//   * First PLAY ever (XO_TUTOR = OFFER): one question "NEW HERE?". Later: OPTIONS > SIMU > BORES > TUTORIAL > REPLAY (starts at once from the pause menu).
//   * HOW TO ADD A LESSON: add a TutStep to tutSteps[] (and a goal in tutGoalDone() if it is a new kind of task).
// Included from main.c just before lifeModeRun(); the loop there calls tutTick() each frame and runs tutRunModal() when tutModal is set.
enum { TG_INFO, TG_WALK, TG_RUN, TG_HOP, TG_BOARD, TG_OLLIE, TG_FRIDGE, TG_PAUSE };
enum { TM_NONE, TM_OFFER, TM_CARD, TM_STUCK };
typedef struct { u8 goal; const char*hd; const char*ln[4]; const char*hint; } TutStep;
static const TutStep tutSteps[]={
 {TG_INFO,"WELCOME TO BORE",{"THIS SHORT TUTORIAL SHOWS THE BASICS","DO EACH TASK AND IT MOVES ON","NOBODY CAN DIE WHILE IT RUNS",0},""},
 {TG_WALK,"LESSON  WALKING",{"HOLD THE DPAD TO WALK AROUND","THE CAMERA FOLLOWS YOU","WALK ABOUT FOUR TILES",0},"WALK WITH THE DPAD"},
 {TG_RUN,"LESSON  RUNNING",{"HOLD B WHILE YOU WALK TO RUN","KEEP IT UP FOR A SECOND OR TWO",0,0},"HOLD B AND WALK"},
 {TG_HOP,"LESSON  HOPPING",{"PRESS A TO HOP","HOPS GET YOU OVER LOW THINGS",0,0},"PRESS A TO HOP"},
 {TG_BOARD,"LESSON  THE BOARD",{"PRESS L TO STEP ON THE BOARD","PRESS L AGAIN TO STEP OFF","ON THE BOARD A PUSHES  DPAD STEERS",0},"L GET ON BOARD"},
 {TG_OLLIE,"LESSON  TRICKS",{"PRESS B ON THE BOARD TO OLLIE","IN THE AIR THE DPAD SPINS","B IN THE AIR KICKFLIPS","LAND ON GREEN  RED IS A BAIL"},"B TO OLLIE"},
 {TG_INFO,"LESSON  YOUR NEEDS",{"THE BARS ON SCREEN ARE YOUR NEEDS","GREEN IS GOOD  RED IS URGENT","FOOD REST AND WC NEED LOOKING AFTER",0},""},
 {TG_FRIDGE,"LESSON  EATING",{"WALK TO THE FRIDGE","PRESS R WHEN IT SAYS R OPEN FRIDGE","THAT FILLS YOUR FOOD BAR",0},"R AT THE FRIDGE"},
 {TG_INFO,"WANTS AND WORK",{"WANTS FILL YOUR METER  FEARS DRAIN IT","PICK A CAREER ON THE PHONE","SKATER JOB PAYS FOR TRICK POINTS","R BY ANOTHER SIM TO TALK"},""},
 {TG_PAUSE,"LESSON  THE PAUSE MENU",{"PRESS START TO OPEN THE PAUSE MENU","IT HOLDS OPTIONS  HOUSEHOLD  PHONE","AND SAVES YOUR LIFE","PRESS B TO CLOSE IT AGAIN"},"PRESS START"},
 {TG_INFO,"TUTORIAL COMPLETE",{"THAT IS THE BASICS  HAVE FUN","HOW TO PLAY IN THE MAIN MENU HAS MORE","REPLAY THIS IN OPTIONS  TUTORIAL",0},""},
};
#define TUT_N ((int)(sizeof(tutSteps)/sizeof(tutSteps[0])))
static int tutI, tutModal, tutShown, tutT, tutAcc, tutDelay, tutSawPause, tutAsked; static s32 tutX0, tutY0;   // (tutOn lives in main.c: die() needs it)

static void s2rr(int x,int y,int w,int h,u16 c); static void s2grad(int x,int y,int w,int h,int r0,int g0,int b0,int r1,int g1,int b1);
static void s2plumbob(int cx,int y); static void s2pill(int x,int y,int w,const char*s);   // the Sims 2 style panel pieces (main.c, HOW TO PLAY)

static void tutEnd(void){ tutOn=0; tutModal=TM_NONE; if(xo[XO_TUTOR]!=1){ xo[XO_TUTOR]=1; optsSave(); } }
static void tutBegin(void){ tutOn=1; tutI=0; tutShown=0; tutT=0; tutAcc=0; tutDelay=0; tutSawPause=0; tutModal=TM_NONE; if(xo[XO_TUTOR]!=1){ xo[XO_TUTOR]=1; optsSave(); } }
static void tutNext(void){ int info=tutSteps[tutI].goal==TG_INFO; tutI++; tutShown=0; tutT=0; tutAcc=0; tutDelay=info?0:50; if(tutI>=TUT_N) tutEnd(); }

static int tutGoalDone(const TutStep*s,u16 k,u16 pr){
    switch(s->goal){
    case TG_INFO: return 1;
    case TG_WALK: { s32 dx=lfx-tutX0, dy=lfy-tutY0; if(dx<0)dx=-dx; if(dy<0)dy=-dy; return dx+dy>=1024; }
    case TG_RUN:  if((k&K_B)&&(k&(K_UP|K_DOWN|K_LEFT|K_RIGHT))&&!lskate) tutAcc++; return tutAcc>=40;
    case TG_HOP:  return (pr&K_A)&&!lskate&&lstun<=0&&!simAct;
    case TG_BOARD: return lskate!=0;
    case TG_OLLIE: return (pr&K_B)&&lskate;
    case TG_FRIDGE: return (pr&K_R)&&lnear==1;
    case TG_PAUSE: return tutSawPause;
    }
    return 1;
}
// once a frame, before the game's own step: decides whether a pop-up is due, and checks the goal
static void tutTick(u16 k,u16 pr){
    if(!tutOn||tutModal||ldead||stage==AG_BABY) return;   // (a baby cannot be steered: the lessons wait)
    if(tutDelay>0){ tutDelay--; return; }
    const TutStep*s=&tutSteps[tutI];
    if(!tutShown){
        if((s->goal==TG_BOARD||s->goal==TG_OLLIE)&&(!lhave||stage<AG_CHILD)){ tutNext(); return; }   // no board in this room (or too young): skip those lessons
        tutModal=TM_CARD; return;
    }
    if(tutGoalDone(s,k,pr)){ if(s->goal!=TG_INFO){ lnote="GOOD JOB"; lnoteT=45; } tutNext(); return; }
    if(++tutT>2700){ tutT=0; tutModal=TM_STUCK; return; }   // 45 seconds and still not done: offer a way out
    if(s->hint[0]&&lnoteT<=0){ lnote=s->hint; lnoteT=2; }   // the goal, in the top bar
}
static void tutCardDraw(const char*title,const char*hd,const char*const*ln,const char*f1,const char*f2,const char*f3,u32 cnt){
    static const signed char bob[8]={0,1,2,2,1,0,-1,-1};
    objHideAll();
    s2rr(6,22,228,112,RGB(10,20,30)); s2rr(7,23,226,110,RGB(2,6,13));
    s2grad(8,24,224,14,8,18,28,3,10,19); rect(8,38,224,1,RGB(14,26,31));
    s2plumbob(16,23+bob[(cnt>>3)&7]); text(26,28,title,WHITE,1);
    for(int i=0;i<TUT_N;i++) rect(232-(TUT_N-i)*7,28,5,5,i<tutI?RGB(8,28,10):i==tutI?GOLD:RGB(7,14,22));   // progress: one cell per lesson
    rect(8,41,224,11,RGB(6,16,26)); text(14,44,hd,GOLD,1);
    for(int i=0;i<4;i++) if(ln[i]) text(14,58+i*9,ln[i],RGB(27,30,31),1);
    int x=10; const char*f[3]={f1,f2,f3};
    for(int i=0;i<3;i++) if(f[i]){ int w=tw(f[i],1)+10; s2pill(x,121,w,f[i]); x+=w+4; }
}
// the pop-up loop: 0 = A (go on), 1 = B (skip this lesson), 2 = SELECT (end the tutorial)
static int tutCardRun(const char*title,const char*hd,const char*const*ln,const char*f1,const char*f2,const char*f3){
    u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_A) return 0;
        if(pr&K_B) return 1;
        if(pr&K_SEL) return 2;
        tutCardDraw(title,hd,ln,f1,f2,f3,cnt); present();
    }
}
static void tutRunModal(void){
    int m=tutModal; tutModal=TM_NONE;
    if(m==TM_OFFER){
        static const char* const it[3]={"TAKE THE TUTORIAL","MAYBE LATER","NEVER ASK AGAIN"};
        int c=menu("NEW HERE?",it,3);
        if(c==0) tutBegin(); else if(c==2){ xo[XO_TUTOR]=1; optsSave(); }
        return;
    }
    if(!tutOn) return;
    char ti[20]; { char*e=ti; const char*p="TUTORIAL "; while(*p)*e++=*p++; int n=tutI+1; if(n>=10)*e++=(char)('0'+n/10); *e++=(char)('0'+n%10); *e++='/'; *e++=(char)('0'+TUT_N/10); *e++=(char)('0'+TUT_N%10); *e=0; }
    if(m==TM_STUCK){
        static const char* const ln[4]={"THIS ONE IS TAKING A WHILE","KEEP TRYING  OR SKIP THE LESSON",0,0};
        int r=tutCardRun(ti,"STUCK",ln,"A KEEP TRYING","B SKIP LESSON","SEL END");
        if(r==1) tutNext(); else if(r==2) tutEnd();
        return;
    }
    const TutStep*s=&tutSteps[tutI];
    int info=s->goal==TG_INFO;
    int r=tutCardRun(ti,s->hd,s->ln,info?"A NEXT":"A OK","B SKIP LESSON","SEL END");
    if(r==2){ tutEnd(); return; }
    if(r==1){ tutNext(); return; }
    tutShown=1; tutT=0; tutAcc=0; tutX0=lfx; tutY0=lfy; tutSawPause=0;
}
