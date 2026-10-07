// lifestats.h - LIFETIME STATS: totals that survive new lives (lifetime score, hours played, tricks, best combo ...).
// PER CHARACTER: one set for every Sim of the household, kept by uid, and the set that counts is the one of the Sim you control now (hhPUid, so SELECT
//   switching changes whose stats grow). Kept in RAM (lsAll[uid][k]; lsc[k] is the controlled Sim's row), mirrored to SRAM at LS_OFF and written into
//   the player's save file as a 'T' chunk (savegame.h, all rows).
// GAME TOTALS: one more set for the whole cart (hours with the game, steps taken ...), SRAM at LG_OFF, shown on the last page of the screen.
// Saved with the life (simsSaveNow) and once a minute. Include BEFORE sims.h and AFTER mood.h; the screen is in statscreen.h.
// Time counts game steps (60 a second) from lifeStep, so menus and loading screens do not count.
#define LS_CH 8          // characters (HU_N: the uids of a household)
#define LS_N 18          // numbers per character
#define LS_OFF 7610      // LS_OFF+3+4*LS_N*LS_CH must stay <= 8192; the asserts are in statscreen.h
#define LG_N 11
#define LG_OFF 7560      // LG_OFF+3+4*LG_N <= LS_OFF
enum { LS_SCORE, LS_SECS, LS_TRICKS, LS_BESTCOMBO, LS_BESTTRICK, LS_BAILS, LS_DEATHS, LS_DAYS, LS_LIVES, LS_COMBOS,
       LS_WANTS, LS_PROMOS, LS_AIRS, LS_GRINDS, LS_MEALS, LS_BOARDSECS, LS_GRINDSECS, LS_STEPS };   // (add new ones at the END and raise LS_N)
enum { LG_SECS, LG_STEPS, LG_SCORE, LG_TRICKS, LG_COMBOS, LG_BAILS, LG_DEATHS, LG_DAYS, LG_AIRS, LG_GRINDS, LG_BOOTS };
static const signed char lsToG[LS_N]={ LG_SCORE,LG_SECS,LG_TRICKS,-1,-1,LG_BAILS,LG_DEATHS,LG_DAYS,-1,LG_COMBOS,-1,-1,LG_AIRS,LG_GRINDS,-1,-1,-1,LG_STEPS };
static int hhPUid;       // (house.h: the uid of the Sim you control)
static u32 lsAll[LS_CH][LS_N] EWRAM_BSS; static u32 lg[LG_N] EWRAM_BSS;
#define lsFlat ((u32*)lsAll)
static int lsRow(void){ return (hhPUid>=0&&hhPUid<LS_CH)?hhPUid:0; }
#define lsc (lsAll[lsRow()])
static u32 lsPrevS EWRAM_BSS, lsPrevP EWRAM_BSS; static u8 lsPrevOk EWRAM_BSS;
static u8 lsSub EWRAM_BSS, lsInit EWRAM_BSS, lsDirty EWRAM_BSS, lgInit EWRAM_BSS, lgDirty EWRAM_BSS; static int lsTx EWRAM_BSS=-1, lsTy EWRAM_BSS;
static u32 lsGet(volatile u8*m,int i){ return (u32)m[4*i]|((u32)m[4*i+1]<<8)|((u32)m[4*i+2]<<16)|((u32)m[4*i+3]<<24); }
static void lsLoad(void){
    volatile u8*m=SRAM_BASE+LS_OFF+2; unsigned sum=0x4C; int n=LS_N*LS_CH; lsInit=1; lsSub=0; lsPrevOk=0;
    for(int i=0;i<n;i++) lsFlat[i]=0;
    if(SRAM_BASE[LS_OFF]!='L'||SRAM_BASE[LS_OFF+1]!='S') return;
    for(int i=0;i<4*n;i++) sum+=m[i];
    if(m[4*n]!=(u8)sum) return;
    for(int i=0;i<n;i++) lsFlat[i]=lsGet(m,i);
}
static void lgLoad(void){
    volatile u8*m=SRAM_BASE+LG_OFF+2; unsigned sum=0x4D; lgInit=1;
    for(int i=0;i<LG_N;i++) lg[i]=0;
    if(SRAM_BASE[LG_OFF]=='L'&&SRAM_BASE[LG_OFF+1]=='G'){ for(int i=0;i<4*LG_N;i++) sum+=m[i]; if(m[4*LG_N]==(u8)sum) for(int i=0;i<LG_N;i++) lg[i]=lsGet(m,i); }
    lg[LG_BOOTS]++; lgDirty=1;   // one more time the cart was switched on and played
}
static void lsEnsure(void){ if(!lsInit) lsLoad(); if(!lgInit) lgLoad(); }
static void lsSave(void){
    if(lsInit&&lsDirty){ lsDirty=0; int n=LS_N*LS_CH; volatile u8*m=SRAM_BASE+LS_OFF+2; unsigned sum=0x4C;
        for(int i=0;i<n;i++) for(int b=0;b<4;b++) m[4*i+b]=(u8)(lsFlat[i]>>(8*b));
        for(int i=0;i<4*n;i++) sum+=m[i];
        m[4*n]=(u8)sum; SRAM_BASE[LS_OFF]='L'; SRAM_BASE[LS_OFF+1]='S'; }
    if(lgInit&&lgDirty){ lgDirty=0; volatile u8*m=SRAM_BASE+LG_OFF+2; unsigned sum=0x4D;
        for(int i=0;i<LG_N;i++) for(int b=0;b<4;b++) m[4*i+b]=(u8)(lg[i]>>(8*b));
        for(int i=0;i<4*LG_N;i++) sum+=m[i];
        m[4*LG_N]=(u8)sum; SRAM_BASE[LG_OFF]='L'; SRAM_BASE[LG_OFF+1]='G'; }
}
static void lsReset(void){ lsPrevOk=0; lsInit=1; lsSub=0; for(int i=0;i<LS_N*LS_CH;i++) lsFlat[i]=0; lsDirty=1; lsSave(); }   // a new player starts from zero (the game totals stay)
static void lsAdd(int k,u32 n){ lsEnsure(); u32 v=lsc[k]+n; lsc[k]=v<lsc[k]?0xFFFFFFFFu:v; lsDirty=1;
    int g=lsToG[k]; if(g>=0){ v=lg[g]+n; lg[g]=v<lg[g]?0xFFFFFFFFu:v; lgDirty=1; } }
static void lsMax(int k,u32 n){ lsEnsure(); if(n>lsc[k]){ lsc[k]=n; lsDirty=1; } }
static void lsNote(const char*t){ lnote=t; lnoteT=150; }

// ---- ACHIEVEMENTS: badges worked out from the counters (nothing extra is saved). src 0 = this Sim, 1 = the game totals ----
typedef struct { const char*nm; u8 src, k; u32 t; } LsAch;
#define LS_NACH 18
static const LsAch lsAch[LS_NACH]={
    {"FIRST TRICK",0,LS_TRICKS,1},{"TRICK MASTER",0,LS_TRICKS,100},{"TRICK LEGEND",0,LS_TRICKS,1000},
    {"COMBO KING  5000",0,LS_BESTCOMBO,5000},{"COMBO GOD  20000",0,LS_BESTCOMBO,20000},
    {"SCORE 10000",0,LS_SCORE,10000},{"SCORE 1 MILLION",0,LS_SCORE,1000000},
    {"RAIL RAT  50 GRINDS",0,LS_GRINDS,50},{"AIRHEAD  100 JUMPS",0,LS_AIRS,100},{"CRASH TEST  50 BAILS",0,LS_BAILS,50},
    {"WISH GRANTER  25 WANTS",0,LS_WANTS,25},{"EMPLOYEE  A PROMOTION",0,LS_PROMOS,1},{"FOODIE  100 MEALS",0,LS_MEALS,100},
    {"MARATHON  10000 STEPS",0,LS_STEPS,10000},{"100 DAYS LIVED",0,LS_DAYS,100},
    {"NINE LIVES  9 DEATHS",0,LS_DEATHS,9},{"10 HOURS WITH A SIM",0,LS_SECS,36000},{"20 HOURS IN THE GAME",1,LG_SECS,72000}};
static u32 lsAchVal(int i){ return lsAch[i].src?lg[lsAch[i].k]:lsc[lsAch[i].k]; }
static u32 lsAchMask(void){ u32 m=0; for(int i=0;i<LS_NACH;i++) if(lsAchVal(i)>=lsAch[i].t) m|=1u<<i; return m; }
static u32 lsPrevMask EWRAM_BSS; static int lsPrevRow EWRAM_BSS;
static char lsBadge[40] EWRAM_BSS;
static void lsMile(void){   // once a second: milestone notes (hours played, lifetime score) and a flush to the save chip every minute
    u32 s=lsc[LS_SECS], p=lsc[LS_SCORE];
    if(!lsPrevOk||lsPrevRow!=lsRow()){ lsPrevOk=1; lsPrevRow=lsRow(); lsPrevS=s; lsPrevP=p; lsPrevMask=lsAchMask(); return; }   // (a new Sim or a new player: look again, no banner)
    { u32 m=lsAchMask(), nw=m&~lsPrevMask; lsPrevMask=m; for(int i=0;i<LS_NACH&&nw;i++) if(nw>>i&1){ char*d=lsBadge; const char*a="BADGE  "; while(*a) *d++=*a++; a=lsAch[i].nm; while(*a&&d<lsBadge+38) *d++=*a++; *d=0; lsNote(lsBadge); break; } }
    static const u32 hr[4]={1,10,50,100}, sc[4]={10000,100000,500000,1000000};
    static const char* const hn[4]={"1 HOUR PLAYED","10 HOURS PLAYED","50 HOURS PLAYED","100 HOURS  TRUE FAN"};
    static const char* const sn[4]={"LIFETIME SCORE 10000","LIFETIME SCORE 100000","LIFETIME SCORE 500000","LIFETIME SCORE 1 MILLION"};
    for(int i=0;i<4;i++){ if(lsPrevS<hr[i]*3600&&s>=hr[i]*3600) lsNote(hn[i]); if(lsPrevP<sc[i]&&p>=sc[i]) lsNote(sn[i]); }
    lsPrevS=s; lsPrevP=p;
    if(s%60==0) lsSave();
}
static void lsTick(void){   // from lifeStep: one game step
    int tx=(int)(lfx>>8), ty=(int)(lfy>>8);   // a step: the Sim moved onto another tile (a jump of more than 2 tiles, stairs or a respawn, is not walking)
    if(lsTx>=0&&(tx!=lsTx||ty!=lsTy)){ int dx=tx-lsTx, dy=ty-lsTy; if(dx>=-2&&dx<=2&&dy>=-2&&dy<=2&&!ldead) lsAdd(LS_STEPS,1); }
    lsTx=tx; lsTy=ty;
    if(++lsSub>=60){ lsSub=0; lsAdd(LS_SECS,1); if(lskate&&!ldead) lsAdd(LS_BOARDSECS,1); if(lgrind) lsAdd(LS_GRINDSECS,1); lsMile(); }
}
static void lsEvent(int ev){   // from simsMood: tricks, combos, bails, deaths
    switch(ev){ case M_TRICK: lsAdd(LS_TRICKS,1); break; case M_BAIL: lsAdd(LS_BAILS,1); break; case M_DIE: lsAdd(LS_DEATHS,1); break;
        case M_PROMO: lsAdd(LS_PROMOS,1); break; case M_LAUNCH: lsAdd(LS_AIRS,1); break; case M_GRIND_ON: lsAdd(LS_GRINDS,1); break; case M_EAT: lsAdd(LS_MEALS,1); break; default: break; }
}
