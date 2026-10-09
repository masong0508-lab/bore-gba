// memlog.h - MEMORIES: a short diary of the big moments of each Sim (friends, love, promotions, growing up, big combos ...), newest first.
// sims.h calls memNote() from simEventV for every life event of the Sim you control; only the events in memKinds are kept. The screen is in mysim.h
// (PAUSE > MY SIM > MORE > MEMORIES, with the lifetime score and rank on top). One diary per household uid (hhPUid), the last MEM_N moments each.
// About 800 bytes of EWRAM. SAVED in the player's save file as an 'M' chunk (savegame.h: sgEncMems / sgDecMems), so the diary comes back with the player.
#define MEM_N 16
typedef struct { u8 k, pad; u16 day, val; } MemE;   // which moment (memKinds), the day it happened (0 = the first day), a number to show (combo points)
static MemE memLog[LS_CH][MEM_N] EWRAM_BSS; static u8 memHead[LS_CH] EWRAM_BSS, memCnt[LS_CH] EWRAM_BSS;   // head = where the next one goes
typedef struct { u8 ev; const char*nm; } MemKind;
static const MemKind memKinds[]={
    {SE_FRIEND,"MADE A FRIEND"},{SE_BFF,"BECAME BEST FRIENDS"},{SE_KISS,"SHARED A FIRST KISS"},{SE_LOVE,"FELL IN LOVE"},{SE_STEADY,"STARTED GOING STEADY"},
    {SE_ENEMY,"MADE AN ENEMY"},{SE_FIGHT,"GOT INTO A FIGHT"},{SE_PROMO,"GOT PROMOTED"},{SE_DEMOTE,"GOT DEMOTED"},{SE_ACE,"ACED A SHIFT"},
    {SE_SPONSOR,"EARNED A SPONSOR BONUS"},{SE_SKILL,"LEARNED A SKILL"},{SE_GROWUP,"GREW UP"},{SE_OLD,"GOT OLD"},{SE_TAPE,"FOUND A HIDDEN TAPE"},
    {SE_SHOWOFF,"BANKED A BIG COMBO"},{SE_ACCIDENT,"HAD AN ACCIDENT"},{SE_FAINT,"FAINTED"},{SE_PASSOUT,"PASSED OUT"},{SE_BROKE,"WENT BROKE"},{SE_DIE,"DIED"} };
#define MEM_KN ((int)(sizeof memKinds/sizeof memKinds[0]))
static void memNote(int ev,int v){
    int u=hhPUid, k=-1; if(u<0||u>=LS_CH) return;
    for(int i=0;i<MEM_KN;i++) if(memKinds[i].ev==ev){ k=i; break; }
    if(k<0) return;
    if(ev==SE_SHOWOFF&&v<500) return;   // only a combo worth a memory
    { int last=(memHead[u]+MEM_N-1)%MEM_N; if(memCnt[u]&&memLog[u][last].k==k&&memLog[u][last].day==simDay&&ev!=SE_SHOWOFF) return; }   // the same thing twice in one day is one memory
    { MemE*m=&memLog[u][memHead[u]]; m->k=(u8)k; m->day=(u16)(simDay>65535?65535:simDay); m->val=(u16)(v>65535?65535:v); }
    memHead[u]=(u8)((memHead[u]+1)%MEM_N); if(memCnt[u]<MEM_N) memCnt[u]++;
}
static void memReset(void){ for(int u=0;u<LS_CH;u++){ memHead[u]=0; memCnt[u]=0; } }   // a new player or a loaded save file starts from an empty diary
