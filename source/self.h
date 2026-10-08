// self.h - YOUR ACTIONS: the Sims-style pie menu for the Sim you control (interactions with yourself, like clicking your own Sim).
//
// OPEN IT   R with nothing around you (no furniture, no Sim in reach, on foot), or "YOUR ACTIONS" in the pie of any Sim you are next to (USE ring).
// TWO LEVELS  like the social pie: the ring of CATEGORIES first (BODY / MIND / FUN / CARE), then the actions of the one you picked. B steps back up a level.
//   BODY  STRETCH, PUSH-UPS, FLEX          comfort, the BODY skill, showing off
//   MIND  MEDITATE, PRACTICE SPEECH, DOODLE, DAYDREAM     LOGIC, CHARISMA and CREATIVITY skills
//   FUN   DANCE, SING, AIR GUITAR, HAVE A CRY      happy-making (mood events), a little tiring
//   CARE  FRESHEN UP, CAT NAP, DEEP BREATH      hygiene, energy, health
// Every action costs or gives needs (energy, hygiene, comfort, food), keeps you busy a moment (lstun) and then needs a cooldown in game minutes
// (TOO SOON otherwise), so it is a choice, not a way to farm skills. Too hungry or too tired for the hard ones.
// MEMORY  64 bytes of EWRAM (the cooldowns, not saved). No IWRAM.
enum { SL_STRETCH, SL_PUSHUP, SL_FLEX, SL_MEDIT, SL_SPEECH, SL_DOODLE, SL_DREAM, SL_DANCE, SL_SING, SL_GUITAR, SL_CRY, SL_FRESH, SL_NAP, SL_BREATH, SL_N };
typedef struct { const char*nm; u8 cat, cd, busy; } SlfAct;   // cd: game minutes before it can be done again; busy: steps you are occupied
static const SlfAct slfT[SL_N]={
    {"STRETCH",         0, 30,  60},
    {"PUSH-UPS",        0, 45, 120},
    {"FLEX",            0, 30,  70},
    {"MEDITATE",        1, 90, 180},
    {"PRACTICE SPEECH", 1, 60, 130},
    {"DOODLE",          1, 60, 130},
    {"DAYDREAM",        1, 60, 110},
    {"DANCE",           2, 60, 140},
    {"SING",            2, 60, 120},
    {"AIR GUITAR",      2, 45, 110},
    {"HAVE A CRY",      2,120, 130},
    {"FRESHEN UP",      3, 40,  80},
    {"CAT NAP",         3,120, 200},
    {"DEEP BREATH",     3, 30,  50},
};
static const char* const slfCatNm[4]={"BODY","MIND","FUN","CARE"};
static const u16 slfCatCol[4]={RGB(8,24,10),RGB(9,17,29),RGB(28,22,5),RGB(28,10,17)};
static u32 slfAt[SL_N] EWRAM_BSS;   // the game minute (+1) each action is ready again; 0 = ready
static u32 slfNow(void){ return (u32)simDay*1440u+(u32)simMin+1u; }
static void slfAdd(int*v,int d){ *v+=d; if(*v>100) *v=100; if(*v<0) *v=0; }
static void slfDo(int a){
    const SlfAct*d=&slfT[a]; u32 now=slfNow();
    if(slfAt[a]&&now<slfAt[a]){ lnote="TOO SOON"; lnoteT=40; return; }
    // what each one asks for
    if((a==SL_PUSHUP||a==SL_DANCE)&&sNrg<25){ lnote="TOO TIRED"; lnoteT=40; return; }
    if(a==SL_PUSHUP&&lfood<15){ lnote="TOO HUNGRY"; lnoteT=40; return; }
    if(a==SL_NAP&&sNrg>=85){ lnote="NOT TIRED"; lnoteT=40; return; }
    if(a==SL_FRESH&&sHyg>=95){ lnote="ALREADY CLEAN"; lnoteT=40; return; }
    switch(a){
        case SL_STRETCH: slfAdd(&sCom,8); slfAdd(&sNrg,-2); skGain(SK_BODY,1); lnote="STRETCH  AHH"; break;
        case SL_PUSHUP:  slfAdd(&sNrg,-7); slfAdd(&sHyg,-4); slfAdd(&lfood,-4); skGain(SK_BODY,2); hpHeal(3); lnote="PUSH-UPS  PUMPED"; break;
        case SL_FLEX:    simEvent(SE_SHOWOFF); skGain(SK_CHARM,1); if(hhOthers()) slfAdd(&sSoc,3); lnote="FLEX  LOOK AT THESE"; break;
        case SL_MEDIT:   slfAdd(&sCom,6); slfAdd(&sNrg,2); skGain(SK_LOGIC,2); moodEvent(M_CHILL); lnote="MEDITATE  CALM"; break;
        case SL_SPEECH:  slfAdd(&sNrg,-3); skGain(SK_CHARM,2); simEvent(SE_PRACTICE); lnote="SPEECH  SMOOTH"; break;
        case SL_DOODLE:  slfAdd(&sNrg,-2); skGain(SK_CREAT,2); simEvent(SE_PRACTICE); lnote="DOODLE  NICE ONE"; break;
        case SL_DREAM:   slfAdd(&sCom,4); moodEvent(M_SOFA); voxPlay(V_reading_or_thinking); lnote="DAYDREAM"; break;
        case SL_DANCE:   slfAdd(&sNrg,-5); slfAdd(&lfood,-3); slfAdd(&sHyg,-2); moodEvent(M_COMBO); skGain(SK_BODY,1); simEvent(SE_SHOWOFF); voxPlay(V_yeha); lnote="DANCE  GROOVY"; break;
        case SL_SING:    slfAdd(&sNrg,-2); moodEvent(M_TRICK); skGain(SK_CREAT,1); voxPlay(V_serenade_good); lnote="SING  LA LA"; break;
        case SL_GUITAR:  slfAdd(&sNrg,-3); moodEventN(M_TRICK,2); voxPlay(V_yahoo); lnote="AIR GUITAR  ROCK ON"; break;
        case SL_CRY:     slfAdd(&sNrg,-4); moodEvent(M_SOFA); voxPlay(V_cry); lnote="SNIFF  FEELING BETTER"; break;
        case SL_FRESH:   slfAdd(&sHyg,12); lnote="SPLASH  FRESH"; break;
        case SL_NAP:     slfAdd(&sNrg,14); slfAdd(&sCom,-3); voxPlay(V_snoore); lnote="ZZZ"; break;
        case SL_BREATH:  hpHeal(6); slfAdd(&sCom,2); lnote="DEEP BREATH"; break;
    }
    lnoteT=60; lstun=d->busy; lsp=0; lgrind=0;
    slfAt[a]=now+d->cd;
}
// the two-level pie. Returns 1 (the key was used, whatever was picked)
static int slfMenu(void){
    static char ft[44] EWRAM_BSS; static char tl[28] EWRAM_BSS;
    { char*e=simCat(tl,hhPName); simCat(e,"  YOU"); }
    { char*e=simCat(ft,"ENERGY "); e=simCatN(e,sNrg); e=simCat(e,"  CLEAN "); e=simCatN(e,sHyg); e=simCat(e,"  COMFY "); simCatN(e,sCom); }
    int cs=0, pick=-1;
    for(;;){
        int c=pieMenu(tl,ft,slfCatNm,slfCatCol,4,cs); if(c<0) break;
        cs=c; const char* l[8]; u16 col[8]; int id[8], q=0;
        for(int i=0;i<SL_N&&q<8;i++) if(slfT[i].cat==c){ l[q]=slfT[i].nm; col[q]=slfCatCol[c]; id[q++]=i; }
        int r=pieMenu(tl,slfCatNm[c],l,col,q,0);
        if(r>=0){ pick=id[r]; break; }   // B: back up to the categories
    }
    liveInvalidate();
    while((~REG_KEYINPUT)&0x3FF) vsync();
    if(pick>=0) slfDo(pick);
    return 1;
}
