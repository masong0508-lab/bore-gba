// weapons.h - WEAPONS (module 1: the core). Weapons are never free: they come only from the ARMS SHOP lot (armsshop.h) or from SECRET stashes
// hidden in the town's community lots (wpsecret.h). This module holds what you own, which one is in your hand, the swing of the melee ones,
// and the one button that works them.
//
// THE BUTTON   R. With nothing in your hand R does what it always did (talk, use furniture). With a weapon in hand R ATTACKS instead, on foot or on the
//              board (the board keeps rolling: steer with the D-pad and swing). Hold R: the UZI keeps firing. Tap L while holding R: next weapon
//              (UNARMED comes round too, then R talks to people again). A tap of R is held back 4 steps so the L tap can be told apart (nobody notices).
// AIM          the D-pad, 8 ways, as the screen shows it. With the pad let go the weapon points the way you face. Melee hits what is in front of you,
//              guns and missiles (wpshot.h) fly the way you aim.
// HURTING      a hit takes HP like a punch (hhM[].hp), 0 HP knocks the Sim out (never kills), the target likes you less and the cops hear of it (copCrime).
// SAVED        8 bytes at JB_OFF+48: 'W' 'P', weapons owned (a bit each), secrets found (a bit each), bullets, missiles, 0, checksum. About 24 bytes of RAM.
// PRISON       a busted Sim loses the lot (wpConfiscate, called by prBook); nobody is armed inside the prison.
#define WP_OFF (JB_OFF+48)
enum { WP_BAT, WP_KNIFE, WP_TASER, WP_PISTOL, WP_UZI, WP_ROCKET, WP_N };
typedef struct { const char*nm; u8 kind, dmg, cd, reach, flag; u16 price, col; } WpDef;   // kind 0 melee, 1 bullet, 2 missile; reach in 1/16 tiles (melee); flag 1 = keeps firing while R is held, 2 = freezes the target
static const WpDef wpT[WP_N]={
    {"BAT",    0, 22, 24, 28, 0, 120, RGB(22,14,6)},
    {"KNIFE",  0, 15, 10, 20, 0, 200, RGB(28,28,31)},
    {"TASER",  0,  9, 30, 22, 2, 900, RGB(10,24,31)},
    {"PISTOL", 1, 20, 16,  0, 0, 2400, RGB(31,28,8)},
    {"UZI",    1,  9,  5,  0, 1, 6000, RGB(31,22,6)},
    {"ROCKET", 2, 45, 50,  0, 0, 15000, RGB(31,12,4)},
};
#define WP_BMAX 99   // most bullets, most missiles
#define WP_MMAX 20
static u8 wpOwn EWRAM_BSS, wpSecr EWRAM_BSS, wpBul EWRAM_BSS, wpMis EWRAM_BSS, wpInit EWRAM_BSS;   // owned, secrets found, ammo
static s8 wpSel=-1;                                                                                   // in your hand (-1 = nothing)
static u8 wpCd EWRAM_BSS, wpRP EWRAM_BSS, wpRT EWRAM_BSS, wpCyc EWRAM_BSS, wpSw EWRAM_BSS, wpSwX EWRAM_BSS, wpSwY EWRAM_BSS;   // cooldown, R pending / its steps / cycled this press, swing timer and direction (+1)
static u16 wpKeys EWRAM_BSS;                                                                          // the keys held this step (the missiles read them)
static void wpShoot(int w,int ax,int ay); static void wpShotTick(void); static void wpShotDraw(void);   // wpshot.h
static int armsUse(void);   // armsshop.h
static int secUse(void); static void secDraw(void);   // wpsecret.h
/*WPFWD*/
static u8 wpSum(const volatile u8*m){ return (u8)(0x5E^m[2]^m[3]^(m[4]*3)^(m[5]*5)); }
static void wpSave(void){ volatile u8*m=SRAM_BASE+WP_OFF; m[0]='W'; m[1]='P'; m[2]=wpOwn; m[3]=wpSecr; m[4]=wpBul; m[5]=wpMis; m[6]=0; m[7]=wpSum(m); }
static void wpLoad(void){ volatile u8*m=SRAM_BASE+WP_OFF; wpInit=1;
    if(m[0]=='W'&&m[1]=='P'&&m[7]==wpSum(m)){ wpOwn=(u8)(m[2]&((1<<WP_N)-1)); wpSecr=m[3]; wpBul=m[4]>WP_BMAX?WP_BMAX:m[4]; wpMis=m[5]>WP_MMAX?WP_MMAX:m[5]; }
    else { wpOwn=wpSecr=wpBul=wpMis=0; } }
static void wpEnsure(void){ if(!wpInit) wpLoad(); }
static void wpGive(int w,int bul,int mis){   // the shop and the stashes hand weapons out here
    wpEnsure(); wpOwn|=(u8)(1<<w); wpBul=(u8)(wpBul+bul>WP_BMAX?WP_BMAX:wpBul+bul); wpMis=(u8)(wpMis+mis>WP_MMAX?WP_MMAX:wpMis+mis); wpSave(); }
static void wpConfiscate(void){ wpEnsure(); if(wpOwn||wpBul||wpMis){ wpOwn=0; wpBul=wpMis=0; wpSel=-1; wpSave(); } }   // prison.h: busted
static void wpNote(void){   // what is in your hand
    static char b[28] EWRAM_BSS;
    if(wpSel<0){ lnote="UNARMED"; lnoteT=40; return; }
    char*e=simCat(b,wpT[wpSel].nm);
    if(wpT[wpSel].kind){ e=simCat(e,"  "); simCatN(e,wpT[wpSel].kind==1?wpBul:wpMis); }
    lnote=b; lnoteT=45; }
static void wpCycle(void){ int s=wpSel; for(int i=0;i<=WP_N;i++){ s++; if(s>=WP_N) s=-1; if(s<0||(wpOwn>>s&1)) break; } wpSel=(s8)s; sfxPlay(SFX_TICK); wpNote(); }
// ---- aim: the D-pad as the screen shows it (like feelWalk), else the way you face. ax, ay are -1, 0 or 1 in tile axes ----
static void wpAim(u16 k,int*ax,int*ay){
    int ux=((k&K_RIGHT)?1:0)-((k&K_LEFT)?1:0), uy=((k&K_DOWN)?1:0)-((k&K_UP)?1:0);
    if(ux||uy){ int dx=ux+uy, dy=uy-ux;
        if(cview){ int t=dx; if(cview==1){ dx=dy; dy=-t; } else if(cview==2){ dx=-dx; dy=-dy; } else { dx=-dy; dy=t; } }
        *ax=(dx>0)-(dx<0); *ay=(dy>0)-(dy<0); return; }
    static const signed char DX[8]={1,1,0,-1,-1,-1,0,1}, DY[8]={0,1,1,1,0,-1,-1,-1};
    int i=((lhd+1)>>1)&7; *ax=DX[i]; *ay=DY[i];
}
static int wpLive(int m){ const HhSim*s=&hhM[m]; return s->act!=HA_AWAY&&(xo[XO_MULTIFL]?hhFl[m]==curFl:curFl==0)&&!hhOnOtherFloor(m); }   // a household Sim in this room
static void wpHurt(int m,int dmg,int w){   // one hit on a household Sim (melee, bullets and blasts all end here)
    HhSim*t=&hhM[m]; int b=t->uid; if(dmg<1) dmg=1;
    copCrime(1);
    if(hhPUid>=0&&hhPUid<HU_N&&b<HU_N){ relD[b][hhPUid]=(signed char)clampR(relD[b][hhPUid]-18); relL[b][hhPUid]=(signed char)clampR(relL[b][hhPUid]-8); }
    sfxPlay(SFX_HIT); hhSay(b,IC_HURT,"OW");
    hhScare((int)(t->fx>>8),(int)(t->fy>>8),7,150); hhPanT[m]=0;   // PANIC: the others run from it (the one hit is stunned or down)
    if(t->hp>dmg){ t->hp=(u8)(t->hp-dmg); hhFreeze(b,w==WP_TASER?200:40);
        static char hb[20] EWRAM_BSS; char*e=simCat(hb,wpT[w].nm); *e++=' '; simCatN(e,dmg); lnote=hb; lnoteT=40; }
    else { t->hp=30; t->act=HA_SOC; t->t=600; t->bub=IC_SKULL; t->bubT=120; hhNote(t," IS KNOCKED OUT"); voxPlay(V_win_the_fight); copCrime(2); }
}
static void wpMelee(int w,int ax,int ay){
    const WpDef*d=&wpT[w]; wpSw=8; wpSwX=(u8)(ax+1); wpSwY=(u8)(ay+1); sfxPlay(SFX_POP);
    int R=d->reach*16;
    for(int m=0;m<hhN;m++){ if(!wpLive(m)) continue;
        int dx=(int)(hhM[m].fx-lfx), dy=(int)(hhM[m].fy-lfy);
        if(dx*dx+dy*dy>R*R) continue;
        if(dx*ax+dy*ay<-48) continue;   // behind you
        if(!wallClear(lfx,lfy,hhM[m].fx,hhM[m].fy)) continue;   // a wall in between
        wpHurt(m,d->dmg+(rnd8()>>5),w); }
}
static void wpFire(u16 k){
    int w=wpSel, ax, ay; const WpDef*d=&wpT[w]; wpAim(k,&ax,&ay); wpCd=d->cd;
    if(d->kind==0){ wpMelee(w,ax,ay); return; }
    u8*am=d->kind==1?&wpBul:&wpMis;
    if(!*am){ lnote="NO AMMO"; lnoteT=30; wpCd=20; sfxPlay(SFX_TICK); return; }
    (*am)--; wpSave(); wpShoot(w,ax,ay);
}
static void wpTry(u16 k){ if(wpCd==0&&lstun<=0&&!lcamF) wpFire(k); }
static int wpUseSpot(void){ int r=0; if(!r) r=armsUse(); if(!r) r=secUse(); /*WPSPOT*/ return r; }   // R next to a counter or a stash (armsshop.h, wpsecret.h). 1 = it was used
// Called first thing in every life step: the weapon button. It may rewrite which keys the rest of the step sees.
static void wpPre(u16*kp,u16*pp){
    u16 k=*kp, pr=*pp; wpEnsure(); wpKeys=k;
    if(wpCd) wpCd--;
    if(wpSw) wpSw--;
    wpShotTick();
    if(!wpOwn||ldead||stage<AG_TEEN||prIn()){ wpSel=-1; wpRP=0; if(!ldead&&wpOwn==0&&(pr&K_R)&&wpUseSpot()) *pp=(u16)(pr&~K_R); return; }
    if((k&K_R)&&(pr&K_L)){ wpCycle(); wpCyc=1; wpRP=0; *pp=(u16)(pr&~K_L); return; }   // R held + L: the next weapon (the board stays as it is)
    if(pr&K_R){ wpRP=1; wpRT=0; wpCyc=0; pr&=(u16)~K_R; }
    if(wpRP){ wpRT++;
        if(!(k&K_R)||wpRT>=4){ wpRP=0;
            if(wpUseSpot()){ wpCyc=1; }                       // a counter or a stash: not an attack
            else if(wpSel>=0) wpTry(k); else pr|=K_R; } }     // nothing in hand: R is the old R
    else if(wpSel>=0&&(k&K_R)&&!wpCyc&&(wpT[wpSel].flag&1)) wpTry(k);   // held: the UZI keeps firing
    *pp=pr;
}
// ---- drawing: the swing, then the shots (wpshot.h). Screen position of a spot on the floor, z px up ----
static void wpScr(s32 fx,s32 fy,int z,int*sx,int*sy){ s32 rx,ry; rotPos(fx,fy,&rx,&ry); *sx=LOX+(int)((rx-ry)>>5); *sy=LOY+(int)((rx+ry)>>6)-z; }
static void wpDraw(void){
    if(wpSw&&wpSel>=0){ const WpDef*d=&wpT[wpSel]; int ax=(int)wpSwX-1, ay=(int)wpSwY-1, R=d->reach*16, n=wpSw>>1;
        for(int i=0;i<=5;i++){ int sx,sy; int r=R*(i+1)/6; wpScr(lfx+ax*r,lfy+ay*r,plFh+10+(i&1),&sx,&sy);
            u16 c=(i>=4-n/2)?d->col:RGB(28,28,28); rect(sx-1,sy-1,i>=4?3:2,i>=4?3:2,c); } }
    wpShotDraw();
    secDraw();
}
