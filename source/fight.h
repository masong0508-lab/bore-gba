// fight.h - STREET FIGHTER FEEL (module 1: moves and blocking). Real-time fists for the Sim you control, against household Sims.
//
// THE BUTTONS (on foot, nothing in your hand, teen or older, not in the prison):
//   HOLD L   GUARD. Stand your ground and block: a blow from in front gets through at a quarter strength and does not stun you.
//            Raise the guard just as a blow lands (the first 10 steps) and it is a PARRY: no damage, the attacker reels, and your next hit is half again as hard.
//            D-pad while guarding turns you to face (you do not walk). A TAP of L still swaps board / on foot (it fires when you let go).
//   L + R    take a fight stance: R attacks. The stance lasts 5 s after your last blow, after a blow you took or after a social PUNCH (fightHurt sets it).
//   R        in the stance: JAB, then CROSS, then HOOK (the finisher: hits hardest and knocks them back; your creator parts name it HEADBUTT, PINCH ...).
//            Press the next R while the last blow is still finishing and it is buffered. Keep the beat and the combo grows (x2, x3, ...). A miss drops the chain.
//   A + R    KICK: longer reach, slower, ends the chain.
//   D-pad    aims the blow (8 ways); with the pad let go you turn to the nearest Sim in reach (aim assist).
//
// THE OTHER SIDE  A Sim you hit is stunned (less each hit of a combo). When the combo stops it may hit back (grouchy Sims often, nice ones rarely):
//   it winds up for about a quarter of a second (a HIYAH balloon and a tick, that is your cue to guard), then swings once. It only reaches 1.9 tiles.
//   Damage, dodging, armour and critical hits are the same as before (fightHit in house.h): this module only scales the blow (fgMul) and decides when.
//
// HEALTH BAR  the Sim you last hit gets a small bar floating over its head for about 2.5 s (house.h: hhBarHit / hhBarDraw), with a pale ghost chunk that drains to show what the blow took.
//
// MEMORY  about 20 bytes of EWRAM. Nothing is saved. The hooks: fgPre (first thing in lifeStep), fgDraw (main.c next to wpDraw), fgMode/fgMul (main.c).
#define FG_HOLD 6     // steps L is held before it is a guard (shorter = a tap = the board swap)
#define FG_PARRY 10   // steps after the guard goes up in which a blow is a PARRY
enum { FG_JAB, FG_CROSS, FG_HOOK, FG_KICK, FG_N };
static const u8 fgReach[FG_N]={17,19,20,26};   // 1/16 tiles
static const u8 fgPct[FG_N]  ={70,100,160,135}; // percent of a normal blow
static const u8 fgCdT[FG_N]  ={9,12,20,22};     // steps before the next blow (you stand still meanwhile)
static const u8 fgStunT[FG_N]={24,28,50,40};    // steps the one hit stands stunned
static const u8 fgKb[FG_N]   ={0x14,0x20,0x70,0x50};  // knock back, 1/256 tiles
static const char* const fgNm[FG_N]={"JAB","CROSS","HOOK","KICK"};
static u8 fgLH EWRAM_BSS, fgLR EWRAM_BSS, fgGT EWRAM_BSS, fgG EWRAM_BSS;       // steps L has been held, L was pressed with R held (a weapon cycle), steps the guard has been up, guard is up
static u8 fgCd EWRAM_BSS, fgBuf EWRAM_BSS, fgC EWRAM_BSS, fgCW EWRAM_BSS, fgCh EWRAM_BSS;   // blow cooldown, buffered R, next blow in the chain, steps left to continue it, hits in this combo
static u8 fgSw EWRAM_BSS, fgSwX EWRAM_BSS, fgSwY EWRAM_BSS, fgMv EWRAM_BSS, fgPar EWRAM_BSS;   // swing timer and direction (+1), the blow, steps the parry bonus lasts
static u8 fgFoe EWRAM_BSS, fgFoeT EWRAM_BSS;                                   // the Sim (uid) that is winding up a counter, steps to go (0 = nobody)
static char fgB[28] EWRAM_BSS;
static const signed char fgDX[8]={1,1,0,-1,-1,-1,0,1}, fgDY[8]={0,1,1,1,0,-1,-1,-1};   // the same 8 ways as wpAim
static int fgOk(void){ return wpSel<0&&!lskate&&stage>=AG_TEEN&&!ldead&&!prIn()&&!lcamF&&!simAct&&lz<=(s32)(surfH(lfx,lfy)<<8); }
static void fgFace(int ax,int ay){ for(int i=0;i<8;i++) if(fgDX[i]==ax&&fgDY[i]==ay){ lhd=i*2; return; } }
static int fgLive(int m){ return wpLive(m)&&!(hhM[m].act==HA_SOC&&hhM[m].t>=500); }   // in this room and not knocked out
// the nearest live Sim in reach R (1/256 tiles) in front of (ax,ay): its index or -1
static int fgFind(int R,int ax,int ay){
    int best=-1, bd=0x7FFFFFFF;
    for(int m=0;m<hhN;m++){ if(!fgLive(m)) continue;
        int dx=(int)(hhM[m].fx-lfx), dy=(int)(hhM[m].fy-lfy), d=dx*dx+dy*dy;
        if(d>R*R||dx*ax+dy*ay<=0||d>=bd) continue;
        best=m; bd=d; }
    return best;
}
// which way a blow goes: the pad, else toward the nearest Sim within 2 tiles, else the way you face
static void fgAim(u16 k,int*ax,int*ay){
    if(k&(K_UP|K_DOWN|K_LEFT|K_RIGHT)){ wpAim(k,ax,ay); return; }
    int best=-1, bd=0x7FFFFFFF;
    for(int m=0;m<hhN;m++){ if(!fgLive(m)) continue;
        int dx=(int)(hhM[m].fx-lfx), dy=(int)(hhM[m].fy-lfy), d=dx*dx+dy*dy;
        if(d>512*512||d>=bd) continue;
        best=m; bd=d; }
    if(best<0){ wpAim(0,ax,ay); return; }
    int dx=(int)(hhM[best].fx-lfx), dy=(int)(hhM[best].fy-lfy), adx=dx<0?-dx:dx, ady=dy<0?-dy:dy;
    *ax=(adx*2>=ady)?(dx>0?1:-1):0; *ay=(ady*2>=adx)?(dy>0?1:-1):0;
    if(!*ax&&!*ay) wpAim(0,ax,ay);
}
static void fgKnock(int m,int ax,int ay,int amt){   // shove a Sim back, never into a wall or up a step
    HhSim*t=&hhM[m]; s32 nx=t->fx+ax*amt, ny=t->fy+ay*amt; int h0=surfH(t->fx,t->fy);
    if(surfH(nx,ny)<=h0&&surfH(nx,t->fy)<=h0&&surfH(t->fx,ny)<=h0){ t->fx=nx; t->fy=ny; }
}
static void fgAttack(u16 k){
    int kick=(k&K_A)!=0, mv=kick?FG_KICK:(fgC>FG_HOOK?FG_JAB:fgC), ax, ay;
    fgAim(k,&ax,&ay); fgFace(ax,ay);
    fgCd=fgCdT[mv]; fgSw=8; fgSwX=(u8)(ax+1); fgSwY=(u8)(ay+1); fgMv=(u8)mv; sfxPlay(SFX_POP);
    if(!fgMode) simEvent(SE_FIGHT);
    fgMode=300;
    int m=fgFind(fgReach[mv]*16,ax,ay);
    if(m<0){ fgC=0; fgCW=0; fgCh=0; fgCd+=6; return; }   // whiffed: the chain drops and you pay for it
    HhSim*t=&hhM[m]; int b=t->uid, hp0=t->hp;
    int mul=fgPct[mv]+fgCh*5; if(fgPar){ mul=mul*3/2; fgPar=0; }
    fgMul=mul; fightHit(hhPUid,b); fgMul=0;
    if(t->t==600&&t->act==HA_SOC){   // knocked out
        hhBarHit(m,hp0); fgC=0; fgCW=0; fgCh=0; fgFoeT=0; fgMode=60; lnote="K.O."; lnoteT=60; return; }
    int dealt=hp0-(int)t->hp;
    if(dealt<=0){ fgC=0; fgCW=0; return; }   // dodged
    hhBarHit(m,hp0);   // the health bar floats over this foe
    if(hhPUid>=0&&hhPUid<HU_N&&b<HU_N){ relD[b][hhPUid]=(signed char)clampR(relD[b][hhPUid]-5); relL[b][hhPUid]=(signed char)clampR(relL[b][hhPUid]-2); }
    fgCh++; if(fgCh>9) fgCh=9;
    int st=fgStunT[mv]-fgCh*3; if(st<10) st=10;
    hhFreeze(b,st); fgKnock(m,ax,ay,fgKb[mv]); sfxPlay(SFX_HIT);
    fgC=(mv==FG_KICK||mv==FG_HOOK)?0:(u8)(mv+1); fgCW=(u8)(fgCdT[mv]+16);
    { const char*nm=mv==FG_HOOK?fkMove(hhPUid,fgNm[mv]):fgNm[mv]; char*e=simCat(fgB,nm); *e++=' '; e=simCatN(e,dealt);
      if(fgCh>1){ e=simCat(e,"  x"); e=simCatN(e,fgCh); }
      e=simCat(e,"  HP "); simCatN(e,t->hp); lnote=fgB; lnoteT=40; }
    // it may hit back once the combo stops (every landed blow cancels the one it was winding up)
    fgFoeT=0;
    if((rnd8()*100>>8)<70-uTr(b,TR_NICE)*5+fgCh*4){ fgFoe=(u8)b; fgFoeT=(u8)(st+12+(rnd8()&15)); hhFreeze(b,fgFoeT+6); }
    if(mv==FG_HOOK||mv==FG_KICK) fgCh=0;   // the finisher closes the combo
}
static void fgCounter(void){   // the Sim that was winding up swings
    int m=hhMemOf(fgFoe); if(m<0||!fgLive(m)) return;
    int dx=(int)(hhM[m].fx-lfx), dy=(int)(hhM[m].fy-lfy), ax, ay;
    if(dx*dx+dy*dy>(30*16)*(30*16)){ hhSay(fgFoe,IC_BAIL,"MISS"); return; }
    wpAim(0,&ax,&ay);
    int blocking=fgG&&fgCd==0&&(dx*ax+dy*ay>=0);
    fgMode=300;
    if(blocking&&fgGT<=FG_PARRY){   // raised just in time
        hhFreeze(fgFoe,70); fgPar=50; sfxPlay(SFX_HIT); hhSay(fgFoe,IC_BAIL,"WHOA"); lnote="PARRY"; lnoteT=50; return; }
    fgMul=blocking?25:100; fightHit(fgFoe,hhPUid); fgMul=0;
    if(blocking&&lhp>0){ lstun=0; lnote="BLOCKED"; lnoteT=40; }
    else { simEvent(SE_SLAPPED); moodEventN(M_FEAR,2); }
}
// First thing in every life step (before wpPre). Reads the pad, may rewrite the keys the rest of the step sees.
static void fgPre(u16*kp,u16*pp){
    u16 k=*kp, pr=*pp, k0=k;
    hhBarTick();
    if(fgCd) fgCd--;
    if(fgCW&&!--fgCW){ fgC=0; fgCh=0; }
    if(fgSw) fgSw--;
    if(fgPar) fgPar--;
    if(fgBuf) fgBuf--;
    if(fgMode) fgMode--;
    if(fgFoeT){ if(!fgOk()||ldead) fgFoeT=0; else if(!--fgFoeT) fgCounter(); else if(fgFoeT==14){ hhSay(fgFoe,IC_HURT,"HIYAH"); sfxPlay(SFX_TICK); } }
    if(!fgOk()||(k&K_SEL)){ fgLH=0; fgGT=0; fgG=0; fgBuf=0; if(!fgOk()) fgMode=0; return; }
    // L: a tap is the board swap (held back until it is let go), a hold is the guard
    int Lh=(k&K_L)!=0;
    if(pr&K_L) fgLR=(k&K_R)!=0;   // L pressed with R down: that is the weapon cycle, leave it alone
    if(Lh){ if(fgLH<255) fgLH++; }
    else { if(fgLH&&fgLH<FG_HOLD&&!fgLR) pr|=K_L; fgLH=0; fgLR=0; }
    if((pr&K_L)&&!fgLR&&Lh) pr&=(u16)~K_L;
    int guard=Lh&&!fgLR&&fgLH>=FG_HOLD;
    if(guard){ if(fgGT<255) fgGT++; } else fgGT=0;
    fgG=(u8)guard;
    // R: with the stance up (or L held) it is an attack
    if((pr&K_R)&&(fgMode>0||(Lh&&!fgLR))){ pr&=(u16)~K_R; fgBuf=6; if(!fgMode) fgMode=300; }
    // facing: the pad turns you while you guard or recover
    if((guard||fgCd>0)&&(k&(K_UP|K_DOWN|K_LEFT|K_RIGHT))){ int ax,ay; wpAim(k,&ax,&ay); fgFace(ax,ay); }
    if(fgBuf&&fgCd==0&&lstun<=0&&fgMode>0){ fgBuf=0; fgAttack(k0); }
    // standing your ground: no walking while guarding or swinging
    if(guard||fgCd>0){ const u16 m=(u16)(K_UP|K_DOWN|K_LEFT|K_RIGHT|K_A|K_B); k&=(u16)~m; pr&=(u16)~m; }
    *kp=k; *pp=pr;
}
// the swing trail (like wpDraw) and the guard
static void fgDraw(void){
    if(fgSw){ int ax=(int)fgSwX-1, ay=(int)fgSwY-1, R=fgReach[fgMv]*16, n=fgSw>>1;
        for(int i=0;i<=5;i++){ int sx,sy; int r=R*(i+1)/6; wpScr(lfx+ax*r,lfy+ay*r,plFh+8+(i&1)*2,&sx,&sy);
            u16 c=(i>=4-n/2)?(fgMv==FG_HOOK?RGB(31,12,4):RGB(31,26,8)):RGB(28,28,28); rect(sx-1,sy-1,i>=4?3:2,i>=4?3:2,c); } }
    if(fgG&&fgCd==0){ int ax,ay; wpAim(0,&ax,&ay); u16 c=fgGT<=FG_PARRY?RGB(31,31,31):RGB(8,18,31);
        for(int i=-1;i<=1;i++){ int sx,sy; wpScr(lfx+ax*110-ay*i*60,lfy+ay*110+ax*i*60,plFh+8,&sx,&sy); rect(sx-1,sy-3,3,6,c); } }
}
