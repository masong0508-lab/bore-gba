// arcgames.h - THE ARCADE'S MINI GAMES (venues.h includes this after the prize counter). The arcade menu's MINI GAMES entry lists them; one costs a
//   few simoleons, takes the screen over like the TV's clips (exTake = EXT_ARCADE, expand.h exTakeRun calls arcGameRun) and pays out TICKETS for the
//   prize counter (vTix). No pack needed: they are part of the base game.
//   BORE BREAKER   LEFT RIGHT move the paddle, A serves. Bricks pay tickets, a cleared wall pays a bonus.
//   SNACK CATCH    catch the snacks in the basket for 20 seconds, dodge the rocks (3 lives).
//   WHACK-A-BORE   D-pad picks a hole, A whacks the Bore that pops up. 30 seconds.
//   HIGH STRIKER   the meter runs up and down: A stops it. Five swings, the higher the better.
// Everything runs at 30 ticks a second (a vsync, then present). START leaves a game early (what you earned so far still pays).
typedef struct { const char*nm; const char*how1; const char*how2; u8 price; } AcG;
static const AcG acG[]={
    {"BORE BREAKER","LEFT RIGHT MOVES THE PADDLE","A SERVES THE BALL",2},
    {"SNACK CATCH","LEFT RIGHT MOVES THE BASKET","CATCH SNACKS  DODGE ROCKS",1},
    {"WHACK-A-BORE","D-PAD PICKS A HOLE","A WHACKS THE BORE",1},
    {"HIGH STRIKER","PRESS A TO STOP THE METER","AIM FOR THE BELL",1},
};
#define NACG ((int)(sizeof(acG)/sizeof(acG[0])))
static int arcSel, acPts;   // the game picked in the menu; the score a game leaves behind
static void acBg(const char*title){
    rect(0,0,SW,SH,RGB(2,2,8)); rect(0,0,SW,11,RGB(6,4,14)); rect(0,11,SW,1,RGB(4,22,24));
    text((SW-tw(title,1))/2,2,title,RGB(4,28,30),1);
}
static void acHud(const char*a,int n,const char*b,int m){   // a score on the left, another number on the right (in the top bar)
    char s[24]; char*e=simCat(s,a); e=simCat(e," "); simCatN(e,n); text(3,2,s,WHITE,1);
    e=simCat(s,b); e=simCat(e," "); simCatN(e,m); text(SW-3-tw(s,1),2,s,GOLD,1);
}
static int acIntro(int g){   // the rules card: A plays, B backs out
    acBg(acG[g].nm);
    text((SW-tw(acG[g].nm,2))/2,40,acG[g].nm,GOLD,2);
    text((SW-tw(acG[g].how1,1))/2,72,acG[g].how1,WHITE,1);
    text((SW-tw(acG[g].how2,1))/2,86,acG[g].how2,WHITE,1);
    text((SW-tw("A PLAY   B BACK",1))/2,120,"A PLAY   B BACK",RGB(20,26,31),1);
    present();
    keyNow(); u16 prev=keyNow();
    for(;;){ vsync(); u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&K_A) return 1; if(pr&(K_B|K_START)) return 0; }
}
static void acResult(int g,int pts,int tix){
    rect(40,52,160,56,RGB(2,3,8)); rect(40,52,160,1,GOLD); rect(40,107,160,1,GOLD);
    text((SW-tw("GAME OVER",2))/2,57,"GAME OVER",GOLD,2);
    { char s[32]; char*e=simCat(s,"SCORE "); simCatN(e,pts); text((SW-tw(s,1))/2,78,s,WHITE,1);
      e=simCat(s,"+"); e=simCatN(e,tix); simCat(e," TIX"); text((SW-tw(s,1))/2,91,s,RGB(10,30,12),1); }
    (void)g; present();
    { u16 p0=keyNow(); for(int i=0;i<150;i++){ vsync(); u16 k=keyNow(); if((k&~p0)&(K_A|K_B|K_START)) break; p0=k; } }
}
// ---- BORE BREAKER ----
static int acBreaker(void){
    u8 br[4][10]; int left=40, lives=3, px=102, bx=0, by=0, vx=0, vy=0, held=1, pts=0;
    static const u16 rc[4]={RGB(31,6,6),RGB(31,18,4),RGB(28,28,4),RGB(6,26,10)};
    for(int r=0;r<4;r++) for(int c=0;c<10;c++) br[r][c]=1;
    u16 prev=keyNow();
    while(lives>0&&left>0){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_START) break;
        if(k&K_LEFT) px-=5;
        if(k&K_RIGHT) px+=5;
        if(px<20) px=20; if(px>184) px=184;
        if(held){ bx=px+16; by=138; if(pr&K_A){ held=0; vx=(rnd8()&1)?2:-2; vy=-3; sfxPlay(SFX_POP); } }
        else {
            bx+=vx; by+=vy;
            if(bx<20){ bx=20; vx=-vx; }
            if(bx>216){ bx=216; vx=-vx; }
            if(by<13){ by=13; vy=-vy; }
            if(vy>0&&by>=140&&by<=146&&bx+4>=px&&bx<=px+36){
                vy=-3; vx=(bx+2-(px+18))/5; if(vx>3) vx=3; if(vx<-3) vx=-3; if(vx==0) vx=(rnd8()&1)?1:-1; by=138; sfxPlay(SFX_TICK); }
            else if(by>=SH){ lives--; held=1; sfxPlay(SFX_BONK); }
            { int c=(bx+2-20)/20, r=(by+2-24)/9;
              if(bx+2>=20&&by+2>=24&&r<4&&c<10&&br[r][c]){ br[r][c]=0; left--; pts++; vy=-vy; sfxPlay(SFX_POP); } }
        }
        acBg("BORE BREAKER"); acHud("SCORE",pts,"LIVES",lives);
        for(int r=0;r<4;r++) for(int c=0;c<10;c++) if(br[r][c]) rect(20+c*20+1,24+r*9+1,18,7,rc[r]);
        rect(px,144,36,5,RGB(4,28,30)); rect(px,144,36,1,WHITE);
        rect(bx,by,4,4,WHITE);
        if(held) text((SW-tw("A SERVES",1))/2,110,"A SERVES",RGB(20,26,31),1);
        vsync(); present();
    }
    acPts=pts;
    return pts/2+(left==0?8:0);
}
// ---- SNACK CATCH ----
static int acCatch(void){
    s16 ix[6], iy[6]; u8 ik[6], on[6]; int bx=106, lives=3, pts=0;
    for(int i=0;i<6;i++){ on[i]=0; ix[i]=0; iy[i]=0; ik[i]=0; }
    u16 prev=keyNow();
    for(int t=0;t<600&&lives>0;t++){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_START) break;
        if(k&K_LEFT) bx-=6;
        if(k&K_RIGHT) bx+=6;
        if(bx<4) bx=4; if(bx>208) bx=208;
        if(t%12==0){ for(int i=0;i<6;i++) if(!on[i]){ on[i]=1; ix[i]=(s16)(8+rnd8()%216); iy[i]=14; ik[i]=(u8)((rnd8()%4==0)?2:(rnd8()&1)); break; } }
        for(int i=0;i<6;i++) if(on[i]){
            iy[i]=(s16)(iy[i]+3+t/200);
            if(iy[i]>=134&&iy[i]<=146&&ix[i]+8>=bx&&ix[i]<=bx+28){ on[i]=0; if(ik[i]==2){ lives--; sfxPlay(SFX_BONK); } else { pts++; sfxPlay(SFX_POP); } }
            else if(iy[i]>=SH) on[i]=0;
        }
        acBg("SNACK CATCH"); acHud("SNACKS",pts,"LIVES",lives);
        { char s[12]; char*e=simCat(s,"TIME "); simCatN(e,(600-t)/30); text((SW-tw(s,1))/2,14,s,RGB(20,26,31),1); }
        for(int i=0;i<6;i++) if(on[i]){
            if(ik[i]==2) rect(ix[i],iy[i],8,8,RGB(14,14,15)); else disc(ix[i]+4,iy[i]+4,4,ik[i]?RGB(31,26,4):RGB(31,10,20)); }
        rect(bx,142,28,8,RGB(24,14,4)); rect(bx,142,28,2,RGB(31,22,8));
        vsync(); present();
    }
    acPts=pts;
    return pts/2;
}
// ---- WHACK-A-BORE ----
static int acWhack(void){
    int sel=4, mole=-1, mt=0, gap=10, pts=0, hit=0;
    u16 prev=keyNow();
    for(int t=0;t<900;t++){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_START) break;
        if((pr&K_LEFT)&&sel%3>0) sel--;
        if((pr&K_RIGHT)&&sel%3<2) sel++;
        if((pr&K_UP)&&sel>2) sel-=3;
        if((pr&K_DOWN)&&sel<6) sel+=3;
        if(mole<0){ if(--gap<=0){ mole=rnd8()%9; mt=26-(pts>12?12:pts); } }
        else if(--mt<=0){ mole=-1; gap=6+(rnd8()&7); }
        if(hit>0) hit--;
        if(pr&K_A){ if(mole==sel){ pts++; mole=-1; gap=5+(rnd8()&7); hit=6; sfxPlay(SFX_HIT); } else sfxPlay(SFX_BONK); }
        acBg("WHACK-A-BORE"); acHud("HITS",pts,"TIME",(900-t)/30);
        for(int i=0;i<9;i++){ int cx=60+(i%3)*60, cy=44+(i/3)*38;
            disc(cx,cy+6,16,RGB(1,1,2)); disc(cx,cy+4,15,RGB(5,3,2));
            if(i==mole){ disc(cx,cy-4,11,RGB(20,12,6)); rect(cx-5,cy-7,3,3,WHITE); rect(cx+3,cy-7,3,3,WHITE); rect(cx-4,cy-6,1,1,RGB(1,1,2)); rect(cx+4,cy-6,1,1,RGB(1,1,2)); rect(cx-3,cy+1,7,2,RGB(1,1,2)); }
            if(i==sel){ rect(cx-19,cy-17,38,1,GOLD); rect(cx-19,cy+19,38,1,GOLD); rect(cx-19,cy-17,1,37,GOLD); rect(cx+18,cy-17,1,37,GOLD); } }
        if(hit>0) text((SW-tw("BONK!",2))/2,142,"BONK!",GOLD,2);
        vsync(); present();
    }
    acPts=pts;
    return pts*2/3;
}
// ---- HIGH STRIKER ----
static void acStrikerDraw(int tr,int total,int pos,const char*msg){
    acBg("HIGH STRIKER"); acHud("POINTS",total,"SWING",tr+1);
    rect(106,22,28,120,RGB(6,6,10)); rect(108,24,24,116,RGB(2,2,4));
    { int h=pos*116/100; u16 c=pos>=95?RGB(31,26,4):pos>=60?RGB(31,14,4):RGB(6,24,10); rect(108,140-h,24,h,c); }
    disc(120,17,5,GOLD); rect(119,12,2,3,GOLD);
    if(msg) text((SW-tw(msg,2))/2,146,msg,WHITE,2);
}
static int acStriker(void){
    int total=0, quit=0, done=0;
    u16 prev=keyNow();
    for(int tr=0;tr<5&&!quit;tr++){
        int pos=0, dir=1, sp=4+tr*2, stop=0;
        while(!stop&&!quit){
            u16 k=keyNow(), pr=k&~prev; prev=k;
            if(pr&(K_B|K_START)){ quit=1; break; }
            if(pr&K_A){ stop=1; break; }
            pos+=dir*sp; if(pos>=100){ pos=100; dir=-1; } if(pos<=0){ pos=0; dir=1; }
            acStrikerDraw(tr,total,pos,0); vsync(); present();
        }
        if(quit) break;
        total+=pos; done++;
        sfxPlay(pos>=95?SFX_BELL:SFX_POP);
        for(int i=0;i<30;i++){ keyNow(); acStrikerDraw(tr,total,pos,pos>=95?"DING!":"NOT BAD"); vsync(); present(); }
        prev=keyNow();
    }
    acPts=total; (void)done;
    return total/45;
}
// ---- the menu, and the takeover ----
static void arcadeGames(void){   // the arcade menu's MINI GAMES: pays the price, then asks the life loop to run the game (exTake)
    for(;;){
        char tt[40]; const char* it[NACG+1]; char lb[NACG+1][32]; int n=0;
        { char*e=simCat(tt,"MINI GAMES  \xC2\xA7"); simCatMoney(e,simMoney,1); }
        for(int i=0;i<NACG;i++){ char*e=simCat(lb[n],acG[i].nm); e=simCat(e,"  \xC2\xA7"); simCatN(e,acG[i].price); it[n]=lb[n]; n++; }
        int c=menu(tt,it,n); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
        if(c<0) return;
        if(simMoney<acG[c].price){ lnote="NOT ENOUGH SIMOLEONS"; lnoteT=60; voxPlay(V_nah); continue; }
        simMoney-=acG[c].price; arcSel=c; lstun=60; lsp=0; lgrind=0;
        exTake=EXT_ARCADE; return;
    }
}
static void arcGameRun(void){   // expand.h exTakeRun: the screen is ours (bitmap mode, sprites off)
    int g=arcSel; if(g<0||g>=NACG) g=0;
    int tix=0; acPts=0; clipAll();
    if(!acIntro(g)){ simMoneyAdd(acG[g].price); return; }
    switch(g){ case 0: tix=acBreaker(); break; case 1: tix=acCatch(); break; case 2: tix=acWhack(); break; default: tix=acStriker(); break; }
    if(tix<0) tix=0;
    acResult(g,acPts,tix);
    vTix=(u16)(vTix+tix>9999?9999:vTix+tix); tkSave(); sgDirty=1;
    vAdd(&sNrg,-5); skGain(g==2?SK_BODY:SK_LOGIC,1);
    if(tix>=8){ moodEvent(M_WANT); voxPlay(V_yahoo); } else { moodEvent(M_COMBO); if(tix==0) voxPlay(V_nah); }
    { char*e=simCat(vMsg,acG[g].nm); e=simCat(e,"  +"); e=simCatN(e,tix); simCat(e," TIX"); lnote=vMsg; lnoteT=110; }
    stQEvent(QE_VACT,VA_CABINET);   // (the packs' missions: a cabinet at the arcade)
    simsSave();
}
