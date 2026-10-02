// sims.h - BORE life-sim layer, in the spirit of The Sims 2 on a handheld. Integer only, no allocation, no art files.
// Include AFTER mood.h and the life globals (lfood, lbl, lstun, lsp, lgrind, lnote, lnoteT, lnear, lifeMap) and BEFORE lifeStep.
//
//  NEEDS     ENERGY, HYGIENE, COMFORT (new) next to FOOD and WC (existing). 0..100, they drain by themselves; furniture refills them:
//            BED (S) sleeps, SHOWER (H) washes, SOFA (C) sits. Stand next to it and press R; press B or A to get up early.
//            Hunger and bladder, FUN and HAPPY stay in main.c / mood.h. Everything feeds the HAPPY meter (mood.h -> simsComfort()).
//  WANTS     two wants and one fear float above the skater like a Sims 2 thought bubble. Meeting a want pays aspiration points and
//            a mood lift; a fear coming true costs points. A want is only offered if the room has what it needs (no bed, no nap want).
//  ASPIRATION  points -> BRONZE / SILVER / GOLD / PLATINUM. Each level slows the needs down (platinum: half speed).
//  PLUMBOB   green = fine, yellow = so-so, red = in trouble. Bobs over the head.
//  BUBBLE    thought bubble: an urgent need if there is one, otherwise a want.
//
// HOW TO ADD A WANT: add a SE_ name, a row in simWants (name, event, aspiration points, required furniture mask), and call
//   simEvent(SE_X) where it happens. A FEAR is the same in simFears. HOW TO ADD A NEED: a variable, a rate in simsTick,
//   a use in simBegin, a bar in simsHud.
// Not saved to SRAM yet (see simsReset).
//
// TUNING
#define SIM_RATE_NRG   7      // energy lost per step, 1/1024 pt (7 = 1 pt per 2.4 s, 100 to 0 in ~4 minutes)
#define SIM_RATE_HYG   5
#define SIM_RATE_COM   6
#define SIM_GAIN_SLEEP 175    // energy gained per step while sleeping, 1/1024 pt (~10 s for a full night)
#define SIM_GAIN_WASH  400    // hygiene per step in the shower (~4 s)
#define SIM_GAIN_SIT   300    // comfort per step on the sofa (~5.5 s)
#define SIM_LOW        20     // a need below this shows in the thought bubble
#define SIM_SLEEPY_TOP 20     // % of top speed lost when ENERGY is empty-ish (< SIM_LOW)
#define SIM_WANT_GAP   240    // steps before an empty want slot is refilled
static const short simLvlAt[4]={40,120,260,450};          // aspiration points for BRONZE, SILVER, GOLD, PLATINUM
static const char* const simLvlNm[5]={"NONE","BRONZE","SILVER","GOLD","PLATINUM"};
static const unsigned char simDecayPct[5]={100,90,80,70,50}; // need decay at each aspiration level

// furniture the room has (simsScan) and what a want needs
enum { SR_FRIDGE=1, SR_TOILET=2, SR_BED=4, SR_SHOWER=8, SR_SOFA=16, SR_RAIL=32, SR_RAMP=64 };
// things that happen (wants and fears are both made of these)
enum { SE_EAT, SE_PEE, SE_SLEEP, SE_SHOWER, SE_SOFA, SE_TRICK, SE_COMBO, SE_GRIND, SE_AIR, SE_STOKED,
       SE_BAIL, SE_HURT, SE_ACCIDENT, SE_FAINT, SE_PASSOUT, SE_N };
typedef struct { const char* name; unsigned char ev, pts, req; } SimWish;
static const SimWish simWants[]={
    {"HAVE A SNACK", SE_EAT,   10, SR_FRIDGE},
    {"USE THE WC",    SE_PEE,    8, SR_TOILET},
    {"TAKE A NAP",    SE_SLEEP, 15, SR_BED},
    {"GET CLEAN", SE_SHOWER,12, SR_SHOWER},
    {"SIT ON SOFA", SE_SOFA,  10, SR_SOFA},
    {"LAND A TRICK",  SE_TRICK, 10, 0},
    {"TRICK COMBO", SE_COMBO, 25, 0},
    {"GRIND A RAIL",  SE_GRIND, 15, SR_RAIL},
    {"GET AIR",SE_AIR,   15, SR_RAMP},
    {"FEEL STOKED",   SE_STOKED,20, 0},
};
static const SimWish simFears[]={
    {"BAILING",       SE_BAIL,     10, 0},
    {"AN ACCIDENT",   SE_ACCIDENT, 15, 0},
    {"PASSING OUT",   SE_PASSOUT,  15, 0},
    {"GETTING HURT",  SE_HURT,     10, 0},
    {"FAINTING",      SE_FAINT,    15, 0},
};
#define SIM_NW ((int)(sizeof(simWants)/sizeof(simWants[0])))
#define SIM_NF ((int)(sizeof(simFears)/sizeof(simFears[0])))

static int sNrg, sHyg, sCom;                 // the three new needs, 0..100
static int simCrN, simCrH, simCrC;           // fractional need changes, 1/1024 pt
static int simAct, simActT;                  // activity: 0 none, 1 sleep, 2 wash, 3 sit; steps left
static int simAsp, simLvl, simDone;          // aspiration points, level 0..4, wants met
static int simW[2], simF;                    // slot contents (index into the tables, -1 empty)
static int simSlotT[3];                      // refill countdown per slot (w0, w1, fear)
static int simHave, simPrevStoked;           // SR_ mask of furniture in the room, was STOKED last step
static unsigned simRng=12345u;
static const char* simQ; static int simQT;   // a note waiting for the note line to be free
static int simT;                             // steps since reset (drives the bubble)

static int simRnd(void){ simRng=simRng*1664525u+1013904223u; return (int)(simRng>>24); }
static void simQueue(const char* s){ simQ=s; simQT=240; }

static void simsScan(void){   // what does this room have?
    simHave=0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ char c=lifeMap[y][x];
        if(c=='F') simHave|=SR_FRIDGE; else if(c=='T') simHave|=SR_TOILET; else if(c=='S') simHave|=SR_BED;
        else if(c=='H') simHave|=SR_SHOWER; else if(c=='C') simHave|=SR_SOFA;
        else if(c=='='||c=='L'||c=='N') simHave|=SR_RAIL; else if((c>='1'&&c<='8')) simHave|=SR_RAMP; }
}
static int simPick(const SimWish* tab,int n,int avoid1,int avoid2){   // a random wish the room can satisfy, not already on show
    for(int t=0;t<40;t++){ int i=simRnd()%n; if(i==avoid1||i==avoid2) continue; if((tab[i].req&simHave)!=tab[i].req) continue; return i; }
    return -1;
}
static void simFill(void){
    if(simW[0]<0&&simSlotT[0]<=0) simW[0]=simPick(simWants,SIM_NW,simW[1],-1);
    if(simW[1]<0&&simSlotT[1]<=0) simW[1]=simPick(simWants,SIM_NW,simW[0],-1);
    if(simF<0&&simSlotT[2]<=0)    simF=simPick(simFears,SIM_NF,-1,-1);
}
static void simsReset(void){
    sNrg=100; sHyg=100; sCom=80; simCrN=simCrH=simCrC=0; simAct=simActT=0; simAsp=0; simLvl=0; simDone=0;
    simW[0]=simW[1]=simF=-1; simSlotT[0]=simSlotT[1]=simSlotT[2]=0; simPrevStoked=0; simQ=0; simQT=0; simT=0;
    simsScan(); simFill();
}
// mood.h calls this from every moodEvent: the game events that wants and fears care about
static void simEvent(int ev);
static void simsMood(int ev,int n){
    switch(ev){
        case M_TRICK: simEvent(SE_TRICK); break;       case M_COMBO: if(n>=2) simEvent(SE_COMBO); break;   // n = tricks - 1
        case M_GRIND_ON: simEvent(SE_GRIND); break;    case M_LAUNCH: simEvent(SE_AIR); break;
        case M_EAT: simEvent(SE_EAT); break;           case M_RELIEVE: simEvent(SE_PEE); break;
        case M_BAIL: simEvent(SE_BAIL); break;         case M_HURT: case M_HURT_BIG: simEvent(SE_HURT); break;
        case M_ACCIDENT: simEvent(SE_ACCIDENT); break; case M_FAINT: simEvent(SE_FAINT); break;
        case M_PASSOUT: simEvent(SE_PASSOUT); break;
        default: break;   // M_SLEEP/M_SHOWER/M_SOFA/M_WANT/M_FEAR are raised by sims.h itself
    }
}
static void simSetLvl(void){
    int l=0; for(int i=0;i<4;i++) if(simAsp>=simLvlAt[i]) l=i+1;
    if(l>simLvl){ simQueue("ASPIRATION UP"); moodEvent(M_WANT); }
    simLvl=l;
}
// something happened: pay a want, or let a fear come true
static void simEvent(int ev){
    for(int s=0;s<2;s++) if(simW[s]>=0&&simWants[simW[s]].ev==ev){
        simAsp+=simWants[simW[s]].pts; simDone++; simW[s]=-1; simSlotT[s]=SIM_WANT_GAP; moodEvent(M_WANT); simQueue("WANT MET"); simSetLvl(); }
    if(simF>=0&&simFears[simF].ev==ev){
        simAsp-=simFears[simF].pts; if(simAsp<0) simAsp=0; simF=-1; simSlotT[2]=SIM_WANT_GAP*2; moodEvent(M_FEAR); simQueue("FEAR CAME TRUE");
        int l=0; for(int i=0;i<4;i++) if(simAsp>=simLvlAt[i]) l=i+1; simLvl=l; }
}
static inline int simMin3(int a,int b,int c){ return a<b?(a<c?a:c):(b<c?b:c); }
static int simsWorst(void){ return simMin3(sNrg,sHyg,sCom); }                 // lowest of the three new needs
static int simsComfort(void){ return (sNrg*40+sHyg*30+sCom*30)/100; }          // blended, used by the HAPPY target in mood.h
static int simsTop(int top){ return sNrg<SIM_LOW?top-top*SIM_SLEEPY_TOP/100:top; }   // too tired: slower
// plumbob colour: 0 green, 1 yellow, 2 red
static int simsPlumb(void){
    int h=moodHapPct(), w=simMin3(simsWorst(),lfood,100-lbl);
    if(h<MOOD_SAD||w<10) return 2;
    if(h<60||w<SIM_LOW) return 1;
    return 0;
}
static const char* simsAlert(void){   // most urgent need, or 0
    if(lbl>80) return "WC";
    if(lfood<SIM_LOW) return "EAT";
    if(sNrg<SIM_LOW) return "ZZZ";
    if(sHyg<SIM_LOW) return "STINKY";
    if(sCom<SIM_LOW) return "SIT";
    return 0;
}

// ---- using furniture ----
// kind: 3 bed, 4 shower, 5 sofa (the lnear values). Returns 1 if the skater started.
static int simBegin(int kind){
    if(kind==3){ if(sNrg>=95){ lnote="NOT TIRED"; lnoteT=40; return 0; } simAct=1; simActT=900; lnote="ZZZ"; lnoteT=40; }
    else if(kind==4){ if(sHyg>=95){ lnote="ALREADY CLEAN"; lnoteT=40; return 0; } simAct=2; simActT=420; lnote="SPLASH"; lnoteT=40; }
    else if(kind==5){ if(sCom>=95){ lnote="COMFY ALREADY"; lnoteT=40; return 0; } simAct=3; simActT=480; lnote="AHH SOFA"; lnoteT=40; }
    else return 0;
    lsp=0; lgrind=0; lstun=2; return 1;
}
static void simEnd(void){
    if(simAct==1){ moodEvent(M_SLEEP); simEvent(SE_SLEEP); }
    else if(simAct==2){ moodEvent(M_SHOWER); simEvent(SE_SHOWER); }
    else if(simAct==3){ moodEvent(M_SOFA); simEvent(SE_SOFA); }
    simAct=0; simActT=0;
}
// once per logic step while alive. pr = keys pressed this step.
static void simsTick(unsigned pr){
    simT++;
    int pct=simDecayPct[simLvl];
    if(simAct==0){   // needs drain only when not being refilled
        simCrN+=SIM_RATE_NRG*pct/100; simCrH+=SIM_RATE_HYG*pct/100; simCrC+=SIM_RATE_COM*pct/100;
        while(simCrN>=1024){ simCrN-=1024; if(sNrg>0) sNrg--; }
        while(simCrH>=1024){ simCrH-=1024; if(sHyg>0) sHyg--; }
        while(simCrC>=1024){ simCrC-=1024; if(sCom>0) sCom--; }
    } else {
        int *n=simAct==1?&sNrg:simAct==2?&sHyg:&sCom, *cr=simAct==1?&simCrN:simAct==2?&simCrH:&simCrC;
        int g=simAct==1?SIM_GAIN_SLEEP:simAct==2?SIM_GAIN_WASH:SIM_GAIN_SIT;
        *cr+=g; while(*cr>=1024){ *cr-=1024; if(*n<100) (*n)++; }
        lstun=lstun>2?lstun:2; lsp=0; lgrind=0;       // stay put while busy
        simActT--;
        if(*n>=100||simActT<=0||(pr&(K_A|K_B|K_R))) simEnd();
    }
    if(sNrg==0&&simAct==0){ sNrg=25; lstun=300; lsp=0; lgrind=0; lnote="PASSED OUT"; lnoteT=90; moodEvent(M_PASSOUT); }   // like the old FAINT, from tiredness
    int st=moodState()==MS_STOKED; if(st&&!simPrevStoked) simEvent(SE_STOKED); simPrevStoked=st;
    for(int s=0;s<3;s++) if(simSlotT[s]>0) simSlotT[s]--;
    simFill();
    if(simQ){ if(lnoteT<=0){ lnote=simQ; lnoteT=60; simQ=0; } else if(--simQT<=0) simQ=0; }
}

// ---- drawing: HUD column and the bubble over the head. x,y = top left of the column ----
static void simsBar(int x,int y,const char* nm,int v,u16 dim){
    text(x,y,nm,dim,1);
    rect(x+30,y,v/2,5,v<SIM_LOW?RGB(28,8,6):RGB(14,22,10));
}
static void simsHud(int x,int y,u16 dim,u16 gold){
    simsBar(x,y,"REST",sNrg,dim); simsBar(x,y+8,"CLEAN",sHyg,dim); simsBar(x,y+16,"COMFY",sCom,dim);
    // aspiration bar: progress to the next level
    int lo=simLvl==0?0:simLvlAt[simLvl-1], hi=simLvl>=4?simLvlAt[3]:simLvlAt[simLvl];
    int w=simLvl>=4?50:(simAsp-lo)*50/(hi-lo); if(w<0) w=0; if(w>50) w=50;
    text(x,y+27,simLvlNm[simLvl],gold,1); rect(x,y+35,50,3,RGB(4,5,8)); rect(x,y+35,w,3,gold);
    // thought bubble list: 2 wants, 1 fear
    for(int s=0;s<2;s++) if(simW[s]>=0){ rect(x,y+42+s*8,3,5,RGB(10,26,10)); text(x+6,y+42+s*8,simWants[simW[s]].name,RGB(22,28,22),1); }
    if(simF>=0){ rect(x,y+58,3,5,RGB(28,8,6)); text(x+6,y+58,simFears[simF].name,RGB(30,18,16),1); }
}
static void simsPlumbDraw(int cx,int cy){   // cx,cy = above the head; a little diamond that bobs and turns
    static const u16 col[3][2]={{RGB(8,28,8),RGB(3,16,4)},{RGB(30,27,6),RGB(20,16,2)},{RGB(30,8,6),RGB(18,3,3)}};
    int c=simsPlumb(), bob=((simT/10)&3)==1||((simT/10)&3)==2?1:0, ph=(simT/5)&7, wd=ph<4?4-ph:ph-4;   // half width 0..4
    cy-=bob; if(wd<1) wd=1;
    for(int j=-5;j<=5;j++){ int a=5-(j<0?-j:j); int hw=a*wd/5; for(int i=-hw;i<=hw;i++) px(cx+i,cy+j,(i<=0)?col[c][0]:col[c][1]); }
}
static void simsBubble(int cx,int cy,u16 ink){   // cx = centre, cy = bottom of the bubble
    const char* t=simsAlert(); u16 edge=RGB(31,31,31);
    if(!t){ int s=(simT/240)&1; if(simW[s]<0) s^=1; if(simW[s]<0) return; t=simW[s]>=0?simWants[simW[s]].name:0; if(!t) return; edge=RGB(24,31,24); }
    else edge=RGB(31,22,20);
    int w=tw(t,1)+6, x=cx-w/2; if(x<1) x=1; if(x+w>239) x=239-w;
    rect(x,cy-9,w,9,edge); rect(x+1,cy-8,w-2,7,RGB(31,31,31)); px(cx-1,cy,edge); px(cx,cy,edge); px(cx,cy+1,edge);
    text(x+3,cy-7,t,ink,1);
}
