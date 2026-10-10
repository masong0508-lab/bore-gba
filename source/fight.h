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
// ---- module 2: SPECIAL MOVES (D-pad motions), hit-stop and the K.O. fanfare ----
// Landing blows fills the SPECIAL meter under the score (the skating one: `lspec`, +20 a blow, +100 for a parry, +100 for a K.O.). Do a motion on the pad, then press R in the fight stance:
//   FIREBALL    down, down+side, side, R      (a quarter circle)  costs 30%   a ball that flies 8 tiles and hits the first Sim it meets (150%)
//   UPPERCUT    side, down, down+side, R      (a dragon punch)    costs 40%   220%, a big shove and a long stun, short reach
//   SPIN KICK   down, let go, down, R         (double tap)        costs 30%   everyone within 1.6 tiles (110%), shoved away
// With SPECIAL full (lspecOn) they are free. The motion must be done within about a third of a second; not enough meter = a normal blow.
// HIT-STOP: the game holds still a few frames on every landed blow (1 to 4, the hook and kick longest). A K.O. holds 8 frames, then plays in slow motion (a third speed for 75 frames) under a big K.O.! banner.
#define FG_SPCOST_FB 300
#define FG_SPCOST_DP 400
#define FG_SPCOST_SP 300
enum { FS_NONE, FS_FB, FS_DP, FS_SP };
static u8 fgHm[6] EWRAM_BSS, fgHt[6] EWRAM_BSS, fgNow EWRAM_BSS, fgHl EWRAM_BSS;   // pad history: the last 6 pad states (newest first) and when they began
static u8 fgHS EWRAM_BSS, fgSlow EWRAM_BSS, fgKoT EWRAM_BSS, fgSp EWRAM_BSS, fgSpT EWRAM_BSS, fgSpin EWRAM_BSS;   // hit-stop frames, slow motion frames, banner frames, the special drawn, its frames, spin steps
static u8 fbL EWRAM_BSS; static s32 fbX EWRAM_BSS, fbY EWRAM_BSS; static signed char fbDx EWRAM_BSS, fbDy EWRAM_BSS;   // the fireball: steps left, where, which way
static u16 fgPend EWRAM_BSS;   // R / A pressed while the game was held still: kept for the next step
static int fgOk(void){ return wpSel<0&&!lskate&&stage>=AG_TEEN&&!ldead&&!prIn()&&!lcamF&&!simAct&&lz<=(s32)(surfH(lfx,lfy)<<8); }
static void fgFace(int ax,int ay){ for(int i=0;i<8;i++) if(fgDX[i]==ax&&fgDY[i]==ay){ lhd=i*2; return; } }
static int fgLive(int m){ return wpLive(m)&&!(hhM[m].act==HA_SOC&&hhM[m].t>=500); }   // in this room and not knocked out
// the nearest live Sim in reach R (1/256 tiles) in front of (ax,ay): its index or -1
static int fgFind(int R,int ax,int ay){
    int best=-1, bd=0x7FFFFFFF;
    for(int m=0;m<hhN;m++){ if(!fgLive(m)) continue;
        int dx=(int)(hhM[m].fx-lfx), dy=(int)(hhM[m].fy-lfy), d=dx*dx+dy*dy;
        if(d>R*R||dx*ax+dy*ay<=0||d>=bd||!wallClear(lfx,lfy,hhM[m].fx,hhM[m].fy)) continue;   // (a wall in between: out of reach)
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
static void fgKO(int m){   // the K.O. fanfare
    (void)m; fgHS=8; fgSlow=75; fgKoT=84; specAdd(100); sfxPlay(SFX_HIT);
}
// a special lands on member m. 1 = hurt, 2 = knocked out, 0 = dodged
static int fgLand(int m,int mul,int stun,int kb,int ax,int ay,const char*nm){
    HhSim*t=&hhM[m]; int b=t->uid, hp0=t->hp;
    fgMul=mul; fightHit(hhPUid,b); fgMul=0;
    if(t->t==600&&t->act==HA_SOC){ hhBarHit(m,hp0); fgKnock(m,ax,ay,kb); fgKO(m); lnote="K.O."; lnoteT=60; return 2; }
    int dealt=hp0-(int)t->hp; if(dealt<=0) return 0;
    hhBarHit(m,hp0);
    if(hhPUid>=0&&hhPUid<HU_N&&b<HU_N){ relD[b][hhPUid]=(signed char)clampR(relD[b][hhPUid]-8); relL[b][hhPUid]=(signed char)clampR(relL[b][hhPUid]-3); }
    hhFreeze(b,stun); fgKnock(m,ax,ay,kb); sfxPlay(SFX_HIT); if(fgHS<4) fgHS=4;
    { char*e=simCat(fgB,nm); *e++=' '; e=simCatN(e,dealt); e=simCat(e,"  HP "); simCatN(e,t->hp); lnote=fgB; lnoteT=45; }
    return 1;
}
static int fgSeq(const u8*pat,int n){   // does the pad history end with these states (oldest first), the motion done quickly?
    for(int i=0;i<n;i++) if(fgHm[n-1-i]!=pat[i]) return 0;
    return (u8)(fgHt[0]-fgHt[n-1])<=20&&(u8)(fgNow-fgHt[0])<=40;
}
static int fgMove(void){
    for(int q=0;q<2;q++){ u8 X=q?4:8;   // 4 = LEFT, 8 = RIGHT (bits of fgHist's pad state)
        u8 fb[3]={2,(u8)(2|X),X}, dp[3]={X,2,(u8)(2|X)};
        if(fgSeq(fb,3)) return FS_FB;
        if(fgSeq(dp,3)) return FS_DP; }
    { static const u8 sp[3]={2,0,2}; if(fgSeq(sp,3)) return FS_SP; }
    return FS_NONE;
}
static int fgSpecial(int sp,u16 k){
    int cost=sp==FS_FB?FG_SPCOST_FB:sp==FS_DP?FG_SPCOST_DP:FG_SPCOST_SP;
    if(!lspecOn&&lspec<cost){ if(lnoteT<=0){ lnote="NEED MORE SPECIAL"; lnoteT=40; } return 0; }
    if(!lspecOn) lspec-=cost;
    for(int i=0;i<6;i++){ fgHm[i]=0; } fgHl=0;
    int ax, ay; if(sp==FS_FB) fgAim(k&(u16)(K_UP|K_DOWN|K_LEFT|K_RIGHT),&ax,&ay); else fgAim(0,&ax,&ay);
    fgFace(ax,ay); if(!fgMode) simEvent(SE_FIGHT); fgMode=300; fgFoeT=0; fgC=0; fgCW=0; fgCh=0; fgSp=(u8)sp; fgSpT=14;
    if(sp==FS_FB){ fgCd=16; fbX=lfx+ax*140; fbY=lfy+ay*140; fbDx=(signed char)ax; fbDy=(signed char)ay; fbL=36; sfxPlay(SFX_POP); lnote="FIREBALL"; lnoteT=30; }
    else if(sp==FS_DP){ fgCd=26; fgSpT=16; sfxPlay(SFX_POP); int m=fgFind(24*16,ax,ay); if(m>=0) fgLand(m,220,70,0x70,ax,ay,"UPPERCUT"); else { fgCd+=10; lnote="UPPERCUT"; lnoteT=30; } }
    else { fgCd=24; fgSpin=12; sfxPlay(SFX_POP); int hit=0;
        for(int m=0;m<hhN;m++){ if(!fgLive(m)) continue; int dx=(int)(hhM[m].fx-lfx), dy=(int)(hhM[m].fy-lfy); if(dx*dx+dy*dy>410*410||!wallClear(lfx,lfy,hhM[m].fx,hhM[m].fy)) continue;
            hit|=fgLand(m,110,36,0x50,dx>40?1:dx<-40?-1:0,dy>40?1:dy<-40?-1:0,"SPIN KICK"); }
        if(!hit&&lnoteT<=0){ lnote="SPIN KICK"; lnoteT=30; } }
    return 1;
}
static void fgFbStep(void){   // the fireball flies
    if(!fbL) return;
    if(ldead||!fgOk()){ fbL=0; return; }
    fbL--;
    for(int i=0;i<2;i++){ s32 nx=fbX+fbDx*28, ny=fbY+fbDy*28;
        if(surfH(nx,ny)>surfH(fbX,fbY)){ fbL=0; return; }   // a wall or a piece of furniture
        fbX=nx; fbY=ny;
        for(int m=0;m<hhN;m++){ if(!fgLive(m)) continue; int dx=(int)(hhM[m].fx-fbX), dy=(int)(hhM[m].fy-fbY);
            if(dx*dx+dy*dy<140*140){ fgLand(m,150,44,0x40,fbDx,fbDy,"FIREBALL"); fbL=0; return; } } }
}
// the pad, as the 4 bits the motions use (UP 1, DOWN 2, LEFT 4, RIGHT 8), pushed into the history whenever it changes
static void fgHist(u16 k){
    u8 mk=(u8)(((k&K_UP)?1:0)|((k&K_DOWN)?2:0)|((k&K_LEFT)?4:0)|((k&K_RIGHT)?8:0));
    fgNow++;
    if(mk!=fgHl){ for(int i=5;i>0;i--){ fgHm[i]=fgHm[i-1]; fgHt[i]=fgHt[i-1]; } fgHm[0]=mk; fgHt[0]=fgNow; fgHl=mk; }
}
// called once per game step by the main loop BEFORE lifeStep: 0 = hold still this step (hit-stop, slow motion)
static int fgGate(u16*pr){
    if(fgKoT) fgKoT--;
    int run=1;
    if(fgHS){ fgHS--; run=0; } else if(fgSlow){ fgSlow--; run=(fgSlow%3)==0; }
    if(!run){ fgPend|=(u16)(*pr&(K_R|K_A)); return 0; }
    *pr|=fgPend; fgPend=0; return 1;
}
// the box on screen that the fight overlays use (the main loop redraws it every frame while it is up and once more after): 0 = nothing is showing
static int fgRcNow(int*r){
    if(!(fgSw||fgG||fgSpT||fbL||fgKoT||fgSpin)) return 0;
    r[0]=plX-34; r[1]=plY-54; r[2]=plX+34; r[3]=plY+18;
    if(fbL){ int sx,sy; wpScr(fbX,fbY,plFh+8,&sx,&sy); if(sx-10<r[0]) r[0]=sx-10; if(sx+10>r[2]) r[2]=sx+10; if(sy-10<r[1]) r[1]=sy-10; if(sy+10>r[3]) r[3]=sy+10; }
    if(fgKoT){ if(r[0]>30) r[0]=30; if(r[2]<210) r[2]=210; if(r[1]>vpY0+6) r[1]=vpY0+6; if(r[3]<vpY0+44) r[3]=vpY0+44; }
    return 1;
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
        hhBarHit(m,hp0); fgKO(m); fgC=0; fgCW=0; fgCh=0; fgFoeT=0; fgMode=60; lnote="K.O."; lnoteT=60; return; }
    int dealt=hp0-(int)t->hp;
    if(dealt<=0){ fgC=0; fgCW=0; return; }   // dodged
    hhBarHit(m,hp0);   // the health bar floats over this foe
    fgHS=(u8)(mv==FG_HOOK?4:mv==FG_KICK?3:mv==FG_CROSS?2:1); specAdd(20+fgCh*4);   // hit-stop, and the SPECIAL meter fills
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
    if(dx*dx+dy*dy>(30*16)*(30*16)||!wallClear(hhM[m].fx,hhM[m].fy,lfx,lfy)){ hhSay(fgFoe,IC_BAIL,"MISS"); return; }   // (out of reach, or a wall in between)
    wpAim(0,&ax,&ay);
    int blocking=fgG&&fgCd==0&&(dx*ax+dy*ay>=0);
    fgMode=300;
    if(blocking&&fgGT<=FG_PARRY){   // raised just in time
        hhFreeze(fgFoe,70); fgPar=50; sfxPlay(SFX_HIT); hhSay(fgFoe,IC_BAIL,"WHOA"); lnote="PARRY"; lnoteT=50; specAdd(100); return; }
    fgMul=blocking?25:100; fightHit(fgFoe,hhPUid); fgMul=0;
    if(blocking&&lhp>0){ lstun=0; lnote="BLOCKED"; lnoteT=40; }
    else { simEvent(SE_SLAPPED); moodEventN(M_FEAR,2); }
}
// First thing in every life step (before wpPre). Reads the pad, may rewrite the keys the rest of the step sees.
static void fgPre(u16*kp,u16*pp){
    u16 k=*kp, pr=*pp, k0=k;
    hhBarTick(); fgFbStep(); if(fgSpT) fgSpT--;
    if(fgSpin){ fgSpin--; lhd=(u8)((lhd+2)&15); }
    if(fgCd) fgCd--;
    if(fgCW&&!--fgCW){ fgC=0; fgCh=0; }
    if(fgSw) fgSw--;
    if(fgPar) fgPar--;
    if(fgBuf) fgBuf--;
    if(fgMode) fgMode--;
    if(fgFoeT){ if(!fgOk()||ldead) fgFoeT=0; else if(!--fgFoeT) fgCounter(); else if(fgFoeT==14){ hhSay(fgFoe,IC_HURT,"HIYAH"); sfxPlay(SFX_TICK); } }
    if(!fgOk()||(k&K_SEL)){ fgLH=0; fgGT=0; fgG=0; fgBuf=0; if(!fgOk()) fgMode=0; return; }
    fgHist(k0);
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
    if((pr&K_R)&&(fgMode>0||(Lh&&!fgLR))){ pr&=(u16)~K_R;
        int sp=(fgCd==0&&lstun<=0)?fgMove():FS_NONE;
        if(sp&&fgSpecial(sp,k0)) fgBuf=0;   // a motion + R: the special move
        else { fgBuf=6; if(!fgMode) fgMode=300; } }
    // facing: the pad turns you while you guard or recover
    if((guard||fgCd>0)&&!fgSpin&&(k&(K_UP|K_DOWN|K_LEFT|K_RIGHT))){ int ax,ay; wpAim(k,&ax,&ay); fgFace(ax,ay); }
    if(fgBuf&&fgCd==0&&lstun<=0&&fgMode>0){ fgBuf=0; fgAttack(k0); }
    // standing your ground: no walking while guarding or swinging
    if(guard||fgCd>0){ const u16 m=(u16)(K_UP|K_DOWN|K_LEFT|K_RIGHT|K_A|K_B); k&=(u16)~m; pr&=(u16)~m; }
    *kp=k; *pp=pr;
}
static void fgDrawSp(void){   // fireball, uppercut, spin kick and the K.O.! banner
    if(fbL){ int sx,sy; wpScr(fbX,fbY,plFh+8,&sx,&sy); int f=(fbL>>1)&1;
        for(int t=3;t>=1;t--){ int tx,ty; wpScr(fbX-fbDx*40*t,fbY-fbDy*40*t,plFh+8,&tx,&ty); rect(tx-(3-t)/2,ty-(3-t)/2,4-t+1,4-t+1,t==1?RGB(31,16,3):RGB(24,6,3)); }
        rect(sx-4,sy-4,8,8,RGB(31,12,3)); rect(sx-3,sy-5,6,10,RGB(31,12,3)); rect(sx-3,sy-3,6,6,f?RGB(31,26,8):RGB(31,20,5)); rect(sx-1,sy-1,3,3,RGB(31,31,28)); }
    if(fgSpT&&fgSp==FS_DP){ int ax,ay; wpAim(0,&ax,&ay); int up=16-fgSpT;
        for(int i=0;i<3;i++){ int sx,sy; wpScr(lfx+ax*(60+i*70),lfy+ay*(60+i*70),plFh+4,&sx,&sy);
            int h=up*3+i*3; if(h>34) h=34; rect(sx-1,sy-h,3,h,RGB(31,24,6)); rect(sx,sy-h,1,h,RGB(31,31,24)); } }
    if(fgSpin){ int ph=fgSpin;
        for(int i=0;i<8;i++){ int j=(i+ph)&7, sx,sy; wpScr(lfx+fgDX[j]*300,lfy+fgDY[j]*300,plFh+6,&sx,&sy);
            rect(sx-1,sy-1,3,3,(i&1)?RGB(31,28,10):RGB(28,28,28)); } }
    if(fgKoT){ const char*t="K.O.!"; int w=tw(t,3), x=(SW-w)/2, y=vpY0+12; u16 c=((fgKoT>>2)&1)?RGB(31,8,4):RGB(31,28,6);
        text(x+1,y+1,t,RGB(4,2,2),3); text(x,y,t,c,3); }
}
// the swing trail (like wpDraw) and the guard
static void fgDraw(void){
    if(fgSw){ int ax=(int)fgSwX-1, ay=(int)fgSwY-1, R=fgReach[fgMv]*16, n=fgSw>>1;
        for(int i=0;i<=5;i++){ int sx,sy; int r=R*(i+1)/6; wpScr(lfx+ax*r,lfy+ay*r,plFh+8+(i&1)*2,&sx,&sy);
            u16 c=(i>=4-n/2)?(fgMv==FG_HOOK?RGB(31,12,4):RGB(31,26,8)):RGB(28,28,28); rect(sx-1,sy-1,i>=4?3:2,i>=4?3:2,c); } }
    fgDrawSp();
    if(fgG&&fgCd==0){ int ax,ay; wpAim(0,&ax,&ay); u16 c=fgGT<=FG_PARRY?RGB(31,31,31):RGB(8,18,31);
        for(int i=-1;i<=1;i++){ int sx,sy; wpScr(lfx+ax*110-ay*i*60,lfy+ay*110+ax*i*60,plFh+8,&sx,&sy); rect(sx-1,sy-3,3,6,c); } }
}
