// lifestats.h - LIFETIME STATS: totals that survive new lives (lifetime score, hours played, tricks, best combo ...). Shown in PAUSE > MY SIM > MORE > LIFETIME STATS.
// One set per player: it is kept in RAM (ls[]), mirrored to SRAM at LS_OFF (the tail of the household area, see the assert in statscreen.h) and written into
// the player's save file as a 'T' chunk (savegame.h). Saved with the life (simsSaveNow). Include BEFORE mood.h; the screen is in statscreen.h.
// Time counts game steps (60 a second) from lifeStep, so menus and loading screens do not count.
#define LS_OFF 8144
#define LS_N 10
enum { LS_SCORE, LS_SECS, LS_TRICKS, LS_BESTCOMBO, LS_BESTTRICK, LS_BAILS, LS_DEATHS, LS_DAYS, LS_LIVES, LS_COMBOS };
static u32 ls[LS_N] EWRAM_BSS; static u8 lsSub EWRAM_BSS, lsInit EWRAM_BSS, lsDirty EWRAM_BSS;
static void lsLoad(void){
    volatile u8*m=SRAM_BASE+LS_OFF; unsigned sum=0x4C; lsInit=1; lsSub=0;
    for(int i=0;i<LS_N;i++) ls[i]=0;
    if(m[0]!='L'||m[1]!='S') return;
    for(int i=2;i<2+4*LS_N;i++) sum+=m[i];
    if(m[2+4*LS_N]!=(u8)sum) return;
    for(int i=0;i<LS_N;i++) ls[i]=(u32)m[2+4*i]|((u32)m[3+4*i]<<8)|((u32)m[4+4*i]<<16)|((u32)m[5+4*i]<<24);
}
static void lsEnsure(void){ if(!lsInit) lsLoad(); }
static void lsSave(void){
    if(!lsInit||!lsDirty) return; lsDirty=0;
    volatile u8*m=SRAM_BASE+LS_OFF; unsigned sum=0x4C;
    for(int i=0;i<LS_N;i++) for(int b=0;b<4;b++) m[2+4*i+b]=(u8)(ls[i]>>(8*b));
    for(int i=2;i<2+4*LS_N;i++) sum+=m[i];
    m[2+4*LS_N]=(u8)sum; m[0]='L'; m[1]='S';
}
static void lsReset(void){ lsInit=1; lsSub=0; for(int i=0;i<LS_N;i++) ls[i]=0; lsDirty=1; lsSave(); }   // a new player starts from zero
static void lsAdd(int k,u32 n){ lsEnsure(); u32 v=ls[k]+n; ls[k]=v<ls[k]?0xFFFFFFFFu:v; lsDirty=1; }
static void lsMax(int k,u32 n){ lsEnsure(); if(n>ls[k]){ ls[k]=n; lsDirty=1; } }
static void lsTick(void){ if(++lsSub>=60){ lsSub=0; lsAdd(LS_SECS,1); } }   // from lifeStep: one game step
static void lsEvent(int ev){   // from simsMood: tricks, combos, bails, deaths
    switch(ev){ case M_TRICK: lsAdd(LS_TRICKS,1); break; case M_BAIL: lsAdd(LS_BAILS,1); break; case M_DIE: lsAdd(LS_DEATHS,1); break; default: break; }
}
