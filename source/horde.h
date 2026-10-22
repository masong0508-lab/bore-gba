// horde.h - HORDE MODE: the BOREZIES! pack's endless mode (opens when all five of its missions are done; expand.h).
//
// WAVES   Borezies pour in wave after wave while you stand your ground on your own lot or an arena (the MAP row, arena.h): 100 WAVES (win at the hundredth) or ENDLESS (until you are
//         bored stiff). Each wave is a count of enemies (hzCount: grows with the wave, at most 48), let in a few at a time up to the HORDE SIZE
//         (hzCap: 3, 4 or 6 on the lot at once, never more than the household leaves free places for: the fewer Sims you live with, the bigger the
//         crowd). Between waves a breather: a supply drop (a little FUN and HEALTH back, the clock for a snack) and every five waves a PERK to pick.
// ENEMIES BOREZIE (the shambler), FIDGETY BOREZIE (a sprinter, from wave 3), HEAVY BOREZIE (a brute, from wave 6), PATIENT ZERO (a boss every ten waves).
//         They bite FUN (expand.h: a wind-up groan, then the bite; block it with L, parry it by raising the guard as the groan starts). Fists and fun
//         both work: a knock-out is a kill, a joke / hug / high five / compliment cures one (and cheers you up). The score counts both.
// LIVES   Fun at zero costs a life (the crowd is shoved back, your Fun comes back to 60); out of lives, the run ends. ENDLESS lives = practice (no record).
// OPTIONS fourteen mutators on the set-up screen (hzoNm): horde size, pace, toughness, bite, lives, how a fight is won, the clock, the special meter,
//         boss waves, supply drops, needs, who comes, how fast the waves grow, which wave to start at. They are remembered (HZ block).
// PERKS   eight, one of three offered every five waves, kept for the run (hzPerkNm).
// SAVED   HZ_OFF: 'H' 'Z', version, flags (bit 0: the hundred waves beaten), best wave and best score for each mode, the options, a sum. 21 bytes in the
//         free end of the handheld's block (gotchi.h keeps its 20 at GO_OFF; the board block starts at BD_OFF). Not part of the player save chunks.
// Needs before it: expand.h (the hostile visitors, hzOn and the forward declarations), house.h (twLim, the visitor places), mood.h, the UI kit.
#define HZ_OFF 7524
#define HZ_LEN 21
_Static_assert(HZ_OFF>=GO_OFF+GO_LEN&&HZ_OFF+HZ_LEN<=BD_OFF,"the HORDE record block must sit in the free end of the handheld's block");
_Static_assert(TK_OFF>=HZ_OFF+HZ_LEN&&TK_OFF+TK_LEN<=BD_OFF,"the ticket block must sit after the HORDE record and before the board block");
enum { HZO_DENS, HZO_SPEED, HZO_TOUGH, HZO_BITE, HZO_LIVES, HZO_CURE, HZO_TIME, HZO_SPECIAL, HZO_BOSS, HZO_DROP, HZO_NEEDS, HZO_KIND, HZO_RAMP, HZO_START, HZO_N };
static const char* const hzoNm[HZO_N]={"HORDE SIZE","ENEMY PACE","ENEMY TOUGHNESS","BITE STRENGTH","LIVES","HOW A FIGHT IS WON","TIME OF DAY","SPECIAL METER","BOSS WAVES","SUPPLY DROPS","NEEDS","WHO COMES","WAVES GROW","START AT WAVE"};
static const char* const hzoCh[HZO_N][4]={
    {"SPARSE","NORMAL","SWARM",0}, {"SLOW","NORMAL","FAST","FRANTIC"}, {"SOFT","NORMAL","TOUGH","BRUTAL"}, {"LIGHT","NORMAL","HEAVY","DEADLY"},
    {"ONE","THREE","FIVE","ENDLESS"}, {"FISTS OR FUN","FUN ONLY","FISTS ONLY",0}, {"AS IT COMES","ALWAYS DAY","ALWAYS NIGHT",0}, {"NORMAL","ALWAYS FULL","NONE",0},
    {"ON","OFF",0,0}, {"ON","OFF",0,0}, {"FROZEN","DRAINING",0,0}, {"EVERYONE","SHAMBLERS","SPRINTERS","BRUTES"}, {"GENTLE","NORMAL","STEEP",0}, {"WAVE 1","WAVE 10","WAVE 25","WAVE 50"}};
static const u8 hzoN[HZO_N]={3,4,4,4,4,3,3,3,2,2,2,4,3,4};
static const u8 hzoDef[HZO_N]={1,1,1,1,1,0,0,0,0,0,0,0,1,0};
static const char* const hzoDesc[HZO_N]={"HOW MANY ON THE LOT AT ONCE","HOW FAST THEY SHAMBLE","HOW MUCH PUNISHMENT THEY SOAK","HOW MUCH FUN A BITE TAKES","A BORED STIFF SIM COSTS A LIFE","A PUNCH OR A JOKE  OR ONLY ONE","FREEZE THE CLOCK AT NOON OR MIDNIGHT","THE SPECIAL METER AS YOU LIKE IT","PATIENT ZERO EVERY TEN WAVES","A BREATHER AND A SNACK BETWEEN WAVES","HUNGER AND SLEEP WAIT  OR DO NOT","THE WHOLE ZOO  OR ONE KIND","HOW FAST THE CROWD GETS BIGGER","SKIP AHEAD  FOR THE BRAVE"};
static u8 hzOpt[HZO_N];                        // the chosen option of each mutator
static u8 hzMode;                              // set-up: 0 a hundred waves, 1 endless
static u8 hzMap;                               // set-up: where it is fought: 0 your own lot, 1..AR_N an arena (arena.h)
static const u8 hzStartW[4]={1,10,25,50};
// ---- the run ----
enum { HZP_SKIN, HZP_FISTS, HZP_CHEER, HZP_WIND, HZP_SLOW, HZP_CHARGE, HZP_HEART, HZP_KING, HZP_N };
static const char* const hzPerkNm[HZP_N]={"THICK SKIN","IRON FISTS","CHEERFUL","SECOND WIND","SLOW-MO FOES","FULL CHARGE","BIG HEART","COMBO KING"};
static const char* const hzPerkDs[HZP_N]={"BITES TAKE 30% LESS FUN","YOUR BLOWS LAND 25% HARDER","CURING ONE CHEERS YOU UP MORE","ONE EXTRA LIFE  AT ONCE","NEW ENEMIES 15% SLOWER","SPECIAL FULL AT ONCE  FILLS FAST","FUN BACK TO FULL  REGROWS FASTER","COMBOS LAST AND SCORE 50% MORE"};
static u16 hzSpawnLeft, hzKills, hzCures, hzT, hzSpT, hzComboT;
static u8 hzWave, hzPhase, hzLives, hzCombo, hzPerks, hzBossLeft, hzBites, hzLot, hzDeadSeen, hzStartWave;
static u8 hzPerkOpt[3], hzFlags, hzBestW[2];
static u32 hzBestS[2];
static u16 hzClock0;
// the last run's result card (exModal 6)
static u8 hzRes, hzRWave, hzRNew, hzRMode; static u32 hzRScore; static u16 hzRKills, hzRCures;
static int hzCount(int w){ static const u8 g[3]={3,4,6}; int n=3+w*g[hzOpt[HZO_RAMP]]/2; if(w%10==0&&hzOpt[HZO_BOSS]==0) n=n*3/5; return n>48?48:n; }   // enemies in wave w
static int hzField(void){ int n=0; for(int k=0;k<TW_N;k++) if(exHT[k]&&twOn[k]&&!exHDie[k]) n++; return n; }
static int hzPending(void){ int n=0; for(int k=0;k<TW_N;k++) if(exHPend[k]&&!twOn[k]) n++; return n; }
static int hzLeft(void){ return hzSpawnLeft+hzPending()+hzField(); }
static int hzCapMax(void){ static const u8 t[3]={3,4,6}; int c=t[hzOpt[HZO_DENS]], fr=HH_MAX-hhN; if(c>fr) c=fr; if(c>TW_N) c=TW_N; return c<1?1:c; }   // the most the horde size allows (and the places there are)
static int hzCap(void){ int c=hzCapMax(), w=2+hzWave/3; return c>w?w:c; }   // how many at once right now: two to start with, one more every three waves

static int hzSpeedPct(void){ if(!hzOn) return 100; static const u8 t[4]={70,100,130,160}; int p=t[hzOpt[HZO_SPEED]], g=hzWave>1?(hzWave-1)*12/10:0; if(g>60) g=60; p=p*(100+g)/100; if(hzPerks&(1<<HZP_SLOW)) p=p*85/100; return p; }
static int hzToughPct(void){ if(!hzOn) return 100; static const u8 t[4]={50,100,200,255}; int p=t[hzOpt[HZO_TOUGH]], g=(hzWave-1)*2; if(g>150) g=150; return p*(100+g)/100; }
static int hzBiteMul(void){ if(!hzOn) return 100; static const u8 t[4]={40,70,110,200}; int p=t[hzOpt[HZO_BITE]]; if(hzPerks&(1<<HZP_SKIN)) p=p*70/100; return p; }   // (NORMAL: a bite takes 7 of the 100)
static int hzCureRule(void){ return hzOn?hzOpt[HZO_CURE]:0; }
static int hzDmgPct(void){ return hzOn&&(hzPerks&(1<<HZP_FISTS))?125:100; }
static int hzUnlocked(void){ return xpHave(XP_ZOMB)&&(sUnlock||exPackDone(XP_ZOMB)>=exPackPlay(XP_ZOMB)); }
// ---- records ----
static u8 hzSum(volatile const u8*m){ u8 s=0x6B; for(int i=2;i<HZ_LEN-1;i++) s=(u8)(s*7+m[i]); return s; }
static void hzSave(void){
    volatile u8*m=SRAM_BASE+HZ_OFF; m[0]='H'; m[1]='Z'; m[2]=1; m[3]=hzFlags; m[4]=hzBestW[0]; m[5]=hzBestW[1];
    for(int i=0;i<4;i++){ m[6+i]=(u8)(hzBestS[0]>>(8*i)); m[10+i]=(u8)(hzBestS[1]>>(8*i)); m[14+i]=0; }
    for(int o=0;o<HZO_N;o++) m[14+o/4]=(u8)(m[14+o/4]|(hzOpt[o]&3)<<(2*(o&3)));
    m[18]=hzMode; m[19]=hzMap; m[20]=hzSum(m);
}
static void hzLoad(void){
    for(int o=0;o<HZO_N;o++) hzOpt[o]=hzoDef[o];
    hzMode=0; hzMap=0; hzFlags=0; hzBestW[0]=hzBestW[1]=0; hzBestS[0]=hzBestS[1]=0;
    volatile const u8*m=SRAM_BASE+HZ_OFF; if(m[0]!='H'||m[1]!='Z'||m[2]!=1||m[20]!=hzSum(m)) return;
    hzFlags=m[3]; hzBestW[0]=m[4]; hzBestW[1]=m[5];
    for(int i=0;i<4;i++){ hzBestS[0]|=(u32)m[6+i]<<(8*i); hzBestS[1]|=(u32)m[10+i]<<(8*i); }
    for(int o=0;o<HZO_N;o++){ int v=(m[14+o/4]>>(2*(o&3)))&3; hzOpt[o]=(u8)(v<hzoN[o]?v:hzoDef[o]); }
    hzMode=m[18]&1; hzMap=m[19]<=AR_N?m[19]:0;
}
// ---- the HUD text and the score ----
static const char* hzHudText(void){   // the top bar's right end: "W12 L3 7" (wave, lives, how many are left), "W12 READY", "W12 CLEAR"; a combo of three or more tags on if it fits
    static char b[28]; char*e=b; *e++='W'; e=simCatN(e,hzWave); *e++=' ';
    if(hzPhase==0){ e=simCat(e,"READY"); }
    else if(hzPhase==2){ e=simCat(e,"CLEAR"); }
    else { if(hzLives!=255){ *e++='L'; e=simCatN(e,hzLives); *e++=' '; } e=simCatN(e,hzLeft()); if(hzCombo>=3&&e-b<10){ *e++=' '; *e++='X'; e=simCatN(e,hzCombo); } }
    *e=0; return b;
}
static void hzKillScore(int kind,int cured){   // exCure / exKO: one down. cured: it laughed (else it was knocked out)
    static const u16 pt[XH_N]={0,100,0,0,150,400,3000};
    int p=pt[kind]; if(!cured) p=p*4/5;
    int m=100+25*(hzCombo<12?hzCombo:12); if(hzPerks&(1<<HZP_KING)) m=m*3/2;
    hzScore+=(u32)p*(u32)m/100u; if(hzCombo<250) hzCombo++; hzComboT=(hzPerks&(1<<HZP_KING))?360:200;
    if(cured){ hzCures++; moodFun=moodClamp(moodFun+((hzPerks&(1<<HZP_CHEER))?10:4)*MOOD_ONE); } else hzKills++;
    if(kind==XH_BOSS){ lnote="PATIENT ZERO IS DOWN  +BIG"; lnoteT=150; sfxPlay(SFX_KO); }
}
static void hzEnd(int res);
static void hzFunOut(void){   // exBite: Fun ran out
    hzBites++; hzCombo=0; hzComboT=0;
    if(hzLives!=255&&--hzLives==0){ hzEnd(0); return; }
    moodFun=moodClamp(60*MOOD_ONE); lstun=50; exIF=140; moodEvent(M_SPOOK); sfxPlay(SFX_NEARLY);
    for(int k=0;k<TW_N;k++) if(exHT[k]&&twOn[k]){ s32 dx=hhM[TW_V(k)].fx-lfx, dy=hhM[TW_V(k)].fy-lfy; if((int)((dx*dx+dy*dy)>>8)<(800*800>>8)){ exHS[k]=90; exHA[k]=0; exHW[k]=0; } }   // (the crowd reels back for a moment)
    lnote="BORED STIFF  A LIFE LOST"; lnoteT=150;
}
// ---- starting and ending a run ----
static int hzCanStart(const char**why){
    if(!hzUnlocked()){ *why="FINISH ALL FIVE BOREZIES MISSIONS FIRST"; return 0; }
    if(stage<AG_TEEN){ *why="TOO YOUNG FOR A HORDE"; return 0; }
    if(curFl||prIn()||prHere()){ *why="NOT FROM UPSTAIRS OR A PRISON CELL"; return 0; }
    if(HH_MAX-hhN<1){ *why="NO ROOM  A SMALLER HOUSEHOLD FIRST"; return 0; }
    return 1;
}
XPF static int hzStartRun(void){   // 1 = it started
    exHClearAll(); if(exP){ exQuit(); }
    for(int k=0;k<TW_N;k++){ exHPend[k]=0; exHT[k]=0; exHDie[k]=exHDz[k]=0; }
    hzOn=(u8)(hzMode?2:1); twLim=(u8)hzCapMax(); if(twLim<2) twLim=2; twKeep=0; hhBakeAll(); ldEnd(); liveInvalidate();   // (a place for each one of the crowd: the first time in a session they are baked, about a second each; the same faces come back after that)
    { int any=0; for(int k=0;k<TW_N;k++) if(twHas[k]&&TW_V(k)>=hhN) any=1; if(!any){ hzOn=0; twLim=2; for(int k=2;k<TW_N;k++) twHas[k]=0; toast("NOBODY CAME  THE TOWN IS TOO QUIET"); return 0; } }
    for(int k=0;k<TW_N;k++){ twOn[k]=0; twWait[k]=30000; }
    hzStartWave=hzStartW[hzOpt[HZO_START]]; hzWave=hzStartWave; hzPhase=0; hzT=300; hzSpT=0; hzScore=0; hzKills=hzCures=0; hzCombo=0; hzComboT=0; hzPerks=0; hzBites=0; hzDeadSeen=0; exIF=60; hzSpawnLeft=0;
    { static const u8 lv[4]={1,3,5,255}; hzLives=lv[hzOpt[HZO_LIVES]]; }
    hzLot=nbOk?nbT.cur:0; hzClock0=(u16)simMin;
    moodFun=moodClamp(85*MOOD_ONE); lstun=0; if(lhp<60) lhp=60;
    arPend=(u8)(hzMap?hzMap+1:(arOn?1:0)); hzSave(); exModal=4; return 1;   // (the arena goes up once the first card is read)
}
static void hzCleanup(void){
    for(int k=0;k<TW_N;k++){ exHPend[k]=0; if(exHT[k]) exHGone(k); else { exHDie[k]=exHDz[k]=0; } }
    hzOn=0; twLim=2; for(int k=0;k<TW_N;k++){ if(k>=2) twHas[k]=0; twOn[k]=0; twWait[k]=(short)(400+k*500); }
    if(hzOpt[HZO_TIME]) simMin=hzClock0;
    exIF=0; if(arOn) arPend=1; liveInvalidate();   // (and the lot comes back after the result card)
}
static int hzRecOk(void){ return hzOpt[HZO_LIVES]!=3&&hzStartWave==1; }   // a practice run (endless lives, or a start past wave 1) leaves no record
static void hzEnd(int res){   // 0 bored stiff, 1 the hundred waves beaten, 2 left the lot, 3 gave up
    if(!hzOn) return;
    int mode=hzOn-1, wave=hzPhase==2?hzWave:hzWave-1; if(wave<0) wave=0;   // (the wave you were in does not count until it is cleared)
    hzRes=(u8)res; hzRWave=(u8)wave; hzRMode=(u8)mode; hzRScore=hzScore; hzRKills=hzKills; hzRCures=hzCures; hzRNew=0;
    if(hzRecOk()){
        if(wave>hzBestW[mode]){ hzBestW[mode]=(u8)wave; hzRNew|=1; }
        if(hzScore>hzBestS[mode]){ hzBestS[mode]=hzScore; hzRNew|=2; }
        if(res==1&&mode==0) hzFlags|=1;
    }
    hzSave(); hzCleanup(); exModal=6; sgDirty=1;
    if(res==1) voxPlay(V_yahoo);
}
static void hzNextWave(void){ hzWave++; hzPhase=0; hzT=300; }
XPF static void hzBegin(void){
    int w=hzWave, n=hzCount(w); hzSpawnLeft=(u16)n; hzBossLeft=0;
    if(hzOpt[HZO_BOSS]==0&&w%10==0){ hzBossLeft=(u8)(1+(w-10)/30); if(hzBossLeft>3) hzBossLeft=3; if(hzBossLeft>hzSpawnLeft) hzSpawnLeft=hzBossLeft; }
    hzPhase=1; hzSpT=20; hzBites=0;
    char*e=simCat(exB,"WAVE "); e=simCatN(e,w); if(hzBossLeft) e=simCat(e,"  PATIENT ZERO IS COMING"); else { e=simCat(e,"  "); e=simCatN(e,n); e=simCat(e," BOREZIES"); }
    lnote=exB; lnoteT=140; sfxPlay(SFX_BELL);
}
static int hzPick(void){
    int f=hzOpt[HZO_KIND], w=hzWave;
    if(hzBossLeft&&f==0){ hzBossLeft--; return XH_BOSS; }
    if(f==1) return XH_BORE; if(f==2) return XH_SPRINT; if(f==3) return XH_BRUTE;
    int r=rnd8()*100>>8, sp=w>=3?(w-2)*4:0, br=w>=6?(w-5)*3:0; if(sp>40) sp=40; if(br>25) br=25;
    if(r<br) return XH_BRUTE; if(r<br+sp) return XH_SPRINT; return XH_BORE;
}
XPF static void hzClear(void){   // the wave is over
    int bonus=250*hzWave+(hzBites==0?500:0);
    if(hzPerks&(1<<HZP_KING)) bonus=bonus*3/2; hzScore+=(u32)bonus;
    hzPhase=2; hzT=360; hzCombo=0; hzComboT=0; if(arOn) arScatter(1);   // (the breather: new barrels and canisters in new places)
    if(hzOpt[HZO_DROP]==0){ moodFun=moodClamp(moodFun+30*MOOD_ONE); hpHeal(30); sSoc=sSoc<90?sSoc+10:100; }
    char*e=simCat(exB,"WAVE "); e=simCatN(e,hzWave); e=simCat(e," CLEAR  +"); e=simCatN(e,bonus); lnote=exB; lnoteT=150; sfxPlay(SFX_CASH);
    if(hzOn==1&&hzWave>=100){ hzEnd(1); return; }
    if(hzRecOk()){ int m=hzOn-1; if(hzWave>hzBestW[m]) hzBestW[m]=hzWave; if(hzScore>hzBestS[m]) hzBestS[m]=hzScore; hzSave(); }   // (saved as each wave is cleared: a power cut keeps it)
    if(hzWave%5==0){   // a perk: three of the ones you do not have
        int c[HZP_N], n=0; for(int i=0;i<HZP_N;i++) if(!(hzPerks&(1<<i))) c[n++]=i;
        if(n){ for(int i=0;i<3;i++){ int j=n>0?rnd8()%n:0; hzPerkOpt[i]=(u8)(n>0?c[j]:255); if(n>0){ c[j]=c[--n]; } } exModal=5; }
    }
}
static void hzTick(void){   // once per logic step in the life game (main.c, next to exTick)
    if(!hzOn) return;
    if(exIF) exIF--;
    if(nbOk&&nbT.cur!=hzLot){ hzEnd(2); return; }
    if(curFl||prIn()) return;
    if(!(lfr&15)){   // the mutators that hold something still
        if(hzOpt[HZO_NEEDS]==0){ sNrg=sHyg=sCom=100; lfood=100; lbl=0; }
        if(hzOpt[HZO_SPECIAL]==1) lspec=SPEC_MAX; else if(hzOpt[HZO_SPECIAL]==2){ lspec=0; lspecOn=0; }
        if(hzOpt[HZO_TIME]&&!(lfr&63)) simMin=hzOpt[HZO_TIME]==1?720:1380;
        if((hzPerks&(1<<HZP_CHARGE))&&lspec<SPEC_MAX) lspec=lspec+30>SPEC_MAX?SPEC_MAX:lspec+30;
    }
    if(hzComboT&&!--hzComboT) hzCombo=0;
    if(!(lfr&127)&&!exIF&&moodFunPct()<100) moodFun=moodClamp(moodFun+((hzPerks&(1<<HZP_HEART))?2:1)*MOOD_ONE);   // a little Fun comes back between bites
    if(ldead){ if(!hzDeadSeen){ hzDeadSeen=1; hzFunOut(); } } else hzDeadSeen=0;
    if(!hzOn) return;
    switch(hzPhase){
    case 0: if(hzT&&!--hzT) hzBegin(); break;
    case 1:
        if(hzSpT) hzSpT--;
        else if(hzSpawnLeft>0&&!exHFull()&&hzPending()==0){   // let the next one in, through a place that is free
            for(int k=0;k<TW_N;k++){ if(!twHas[k]||TW_V(k)<hhN||twOn[k]||exHT[k]||exHPend[k]) continue;
                exHPend[k]=(u8)hzPick(); twWait[k]=0; hzSpawnLeft--; { int g=34-hzWave/3; hzSpT=(u16)(g<10?10:g); } break; }
        }
        if(hzSpawnLeft==0&&hzPending()==0&&hzField()==0) hzClear();
        break;
    case 2: if(hzT&&!--hzT) hzNextWave(); break;
    }
}
static void hzReset(void){ if(hzOn){ hzOn=0; twLim=2; } for(int k=0;k<TW_N;k++){ exHPend[k]=0; exHDie[k]=exHDz[k]=0; } exIF=0; }   // (entering the life game: a run does not survive a load)
// ---- the set-up screen ----
static void hzPill(int x,int y,const char*s,int on){ int w=tw(s,1)+8; s2rr(x,y,w,9,on?GOLD:RGB(10,20,30)); s2rr(x+1,y+1,w-2,7,on?RGB(13,18,28):RGB(3,9,17)); text(x+4,y+1,s,on?WHITE:RGB(17,29,31),1); }
XPF static int hzSetup(void){   // 1 = a run starts (the pause menu closes behind it)
    const char*why; if(!hzCanStart(&why)){ toast(why); return 0; }
    int sel=HZO_N+2, top=0, rows=HZO_N+3; u16 prev=keyNow(); u32 cnt=0; int dirty=1;   // (rows: MODE, MAP, the mutators, START)
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)){ hzSave(); return 0; }
        if(pr&K_DOWN){ sel=(sel+1)%rows; dirty=1; } if(pr&K_UP){ sel=(sel+rows-1)%rows; dirty=1; }
        if(pr&(K_LEFT|K_RIGHT)){ int d=(pr&K_RIGHT)?1:-1;
            if(sel==0){ hzMode^=1; dirty=1; }
            else if(sel==1){ hzMap=(u8)((hzMap+d+AR_N+1)%(AR_N+1)); dirty=1; }
            else if(sel<=HZO_N+1){ int o=sel-2; hzOpt[o]=(u8)((hzOpt[o]+d+hzoN[o])%hzoN[o]); dirty=1; } }
        if(pr&K_SEL){ for(int o=0;o<HZO_N;o++) hzOpt[o]=hzoDef[o]; hzMode=0; hzMap=0; dirty=1; }
        if(pr&K_A){ if(sel==HZO_N+2){ hzSave(); if(hzStartRun()) return 1; dirty=1; prev=keyNow(); continue; } if(sel==0){ hzMode^=1; dirty=1; } else if(sel==1){ hzMap=(u8)arPick(hzMap); dirty=1; prev=keyNow(); } else if(sel<=HZO_N+1){ int o=sel-2; hzOpt[o]=(u8)((hzOpt[o]+1)%hzoN[o]); dirty=1; } }
        if(!dirty&&(cnt&15)){ vsync(); continue; }
        dirty=0;
        if(sel<top) top=sel; if(sel>=top+9) top=sel-8;
        stBack("HORDE MODE",(int)cnt);
        for(int r=0;r<9&&top+r<rows;r++){ int i=top+r, y=21+r*11, on=i==sel;
            if(on) s2grad(8,y-1,224,11,6,16,26,3,9,17);
            if(i==0){ text(14,y,"MODE",on?WHITE:GOLD,1); text(229-tw(hzMode?"ENDLESS":"100 WAVES",1),y,hzMode?"ENDLESS":"100 WAVES",on?WHITE:RGB(14,30,10),1); continue; }
            if(i==1){ const char*v=hzMap?arNm[hzMap-1]:"YOUR OWN LOT"; text(14,y,"MAP",on?WHITE:RGB(20,26,31),1); int x=229-tw(v,1); text(x,y,v,on?WHITE:hzMap?RGB(31,24,8):RGB(17,29,31),1); if(on){ text(x-8,y,"<",GOLD,1); text(229+2,y,">",GOLD,1); } continue; }
            if(i==HZO_N+2){ text(14,y,"START THE HORDE",on?WHITE:GOLD,1); if(on) text(229-tw("A",1),y,"A",GOLD,1); continue; }
            int o=i-2; const char*v=hzoCh[o][hzOpt[o]]; u16 vc=hzOpt[o]==hzoDef[o]?RGB(17,29,31):RGB(31,24,8);
            text(14,y,hzoNm[o],on?WHITE:RGB(20,26,31),1);
            { int x=229-tw(v,1); text(x,y,v,on?WHITE:vc,1); if(on){ text(x-8,y,"<",GOLD,1); text(229+2,y,">",GOLD,1); } } }
        rect(8,121,224,1,RGB(14,26,31));
        if(sel>=2&&sel<=HZO_N+1) text(12,125,hzoDesc[sel-2],RGB(20,26,31),1); else if(sel==0) text(12,125,"A HUNDRED WAVES TO WIN  OR NO END AT ALL",RGB(20,26,31),1); else if(sel==1) text(12,125,hzMap?arBl[hzMap-1]:"YOUR OWN LOT  A  PICKS A MAP WITH A PICTURE",RGB(20,26,31),1); else text(12,125,"YOUR FISTS  YOUR JOKES  YOUR LOT OR NOT",RGB(20,26,31),1);
        { char b[40]; char*e=simCat(b,hzMode?"BEST ENDLESS  WAVE ":"BEST 100  WAVE "); e=simCatN(e,hzBestW[hzMode]); e=simCat(e,"  SCORE "); simCatN(e,(int)hzBestS[hzMode]); text(12,134,b,GOLD,1); }
        s2pill(5,147,tw("A START",1)+10,"A START"); s2pill(9+tw("A START",1)+10,147,tw("LEFT RIGHT CHANGE",1)+10,"LEFT RIGHT CHANGE");
        present();
    }
}
// ---- the cards: the run starts (exModal 4), a perk (5), the run is over (6) ----
XPF static void hzIntroCard(void){
    u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_A|K_B|K_START)) return;
        stBack("HORDE MODE",(int)cnt);
        s2rr(8,22,224,118,RGB(16,27,31)); s2rr(9,23,222,116,RGB(2,6,13)); s2grad(10,24,220,114,3,9,19,1,4,10);
        exIcon(XP_ZOMB,16,30,4,xPk[XP_ZOMB].b);
        text(60,30,hzOn==1?"A HUNDRED WAVES":"ENDLESS",GOLD,1); text(60,40,"BOREZIES  PUNCH OR CHEER THEM UP",RGB(17,29,31),1);
        { char b[40]; char*e=simCat(b,"START AT WAVE "); simCatN(e,hzStartWave); e=simCat(e,"  "); simCat(e,hzMap?arNm[hzMap-1]:"YOUR LOT"); text(60,52,b,WHITE,1); }
        { char b[28]; char*e=simCat(b,hzLives==255?"ENDLESS LIVES":"LIVES  "); if(hzLives!=255) simCatN(e,hzLives); text(60,62,b,WHITE,1); }
        rect(14,72,212,1,RGB(14,26,31));
        { int y=77; for(int o=0;o<HZO_N&&y<118;o++) if(hzOpt[o]!=hzoDef[o]&&o!=HZO_START&&o!=HZO_LIVES){ char b[40]; char*e=simCat(b,hzoNm[o]); e=simCat(e,"  "); simCat(e,hzoCh[o][hzOpt[o]]); text(16,y,b,RGB(31,24,8),1); y+=9; }
          if(y==77) text(16,77,"EVERYTHING AS IT COMES",RGB(20,26,31),1);
          if(hzOpt[HZO_LIVES]==3||hzStartWave!=1) text(16,121,"PRACTICE RUN  NO RECORD",RGB(31,12,8),1); else text(16,121,"HOLD L TO GUARD  GROAN MEANS BITE",RGB(20,26,31),1); }
        s2pill(5,147,40,"A GO");
        present();
    }
}
XPF static void hzPerkCard(void){
    int sel=0; u16 prev=keyNow(); u32 cnt=0; int n=0; for(int i=0;i<3;i++) if(hzPerkOpt[i]<HZP_N) n++;
    if(!n) return;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_DOWN) sel=(sel+1)%n; if(pr&K_UP) sel=(sel+n-1)%n;
        if(pr&K_A){ int p=hzPerkOpt[sel]; hzPerks|=(u8)(1<<p);
            if(p==HZP_WIND&&hzLives!=255&&hzLives<250) hzLives++; if(p==HZP_CHARGE) lspec=SPEC_MAX; if(p==HZP_HEART) moodFun=moodClamp(100*MOOD_ONE);
            sfxPlay(SFX_STICK); return; }
        stBack("A PERK",(int)cnt);
        s2rr(8,22,224,118,RGB(16,27,31)); s2rr(9,23,222,116,RGB(2,6,13)); s2grad(10,24,220,114,3,9,19,1,4,10);
        { char b[28]; char*e=simCat(b,"WAVE "); e=simCatN(e,hzWave); simCat(e," CLEAR"); text(16,30,b,GOLD,1); } text(16,40,"PICK ONE  IT STAYS FOR THE RUN",RGB(17,29,31),1);
        for(int i=0;i<n;i++){ int y=56+i*26, on=i==sel, p=hzPerkOpt[i]; if(on) s2grad(12,y-3,216,24,6,16,26,3,9,17);
            text(18,y,hzPerkNm[p],on?WHITE:GOLD,1); text(18,y+10,hzPerkDs[p],on?RGB(20,28,31):RGB(14,22,28),1); }
        s2pill(5,147,tw("A TAKE IT",1)+10,"A TAKE IT");
        present();
    }
}
XPF static void hzResultCard(void){
    u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(cnt>30&&(pr&(K_A|K_B|K_START))) return;
        stBack(hzRes==1?"HORDE BEATEN":"HORDE OVER",(int)cnt);
        s2rr(8,22,224,118,RGB(16,27,31)); s2rr(9,23,222,116,RGB(2,6,13)); s2grad(10,24,220,114,3,9,19,1,4,10);
        if(hzRes==1) stSparkle(cnt,12,26,216,100);
        exIcon(XP_ZOMB,16,30,4,xPk[XP_ZOMB].b);
        text(60,30,hzRes==1?"A HUNDRED WAVES  DONE":hzRes==0?"BORED STIFF":hzRes==2?"YOU LEFT THE LOT":"YOU GAVE UP",hzRes==1?GOLD:WHITE,1);
        { char b[28]; char*e=simCat(b,"WAVES CLEARED  "); simCatN(e,hzRWave); text(60,42,b,WHITE,2); }
        rect(14,72,212,1,RGB(14,26,31));
        { char b[40]; char*e=simCat(b,"SCORE  "); simCatN(e,(int)hzRScore); text(16,78,b,GOLD,1); if(hzRNew&2) text(229-tw("NEW BEST",1),78,"NEW BEST",RGB(10,28,12),1); }
        { char b[40]; char*e=simCat(b,"KNOCKED OUT  "); e=simCatN(e,hzRKills); e=simCat(e,"   CHEERED UP  "); simCatN(e,hzRCures); text(16,90,b,RGB(20,26,31),1); }
        { char b[40]; char*e=simCat(b,"BEST WAVE  "); e=simCatN(e,hzBestW[hzRMode]); if(hzRNew&1) text(229-tw("NEW BEST",1),102,"NEW BEST",RGB(10,28,12),1); text(16,102,b,RGB(20,26,31),1); }
        if(hzRes==1){ text(16,118,"THE TOWN IS SAFE  FOR NOW",WHITE,1); }
        else text(16,118,hzRNew?"A PERSONAL BEST":"THEY WILL BE BACK",RGB(17,29,31),1);
        s2pill(5,147,40,"A OK");
        present();
    }
}
