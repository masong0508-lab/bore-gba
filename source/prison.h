// prison.h - PRISON: when the cops catch you, the sentence depends on how bad you were, and you serve it in a prison of your own.
//
// THE RAP SHEET  Every time you hurt a Sim (copCrime, npc.h) your RECORD grows (prNote). When the cops BUST you (copArrest) the game adds up how bad you were:
//                record + 6 per WANTED star + the heat + 4 per earlier conviction (prCon), and looks the days up (prSentence): from 2 days for a scuffle, through
//                14, 45, 140 and 365 days, to LIFE. Being busted wipes the record and the stars (the old rule) but every conviction counts against you next time.
// THE PRISON     A community lot of the town (type PRISON, CT_PRISON): a sealed compound with a cell block, showers, a day room with a TV and books, a canteen
//                with a chow line, a gym with treadmills and a phone. prLotGet finds it, or builds it on free land (or turns a never built free lot into it).
//                prisonBuild (called by nbTemplate, neighborhood.h) lays it out. It is a normal community lot: nobody can build in it, it is saved with the town.
// DOING TIME     The Sim who was busted lives in the prison (the game moves you there, prTransfer, and back when you are released). Everyone else of your
//                household is OUT while you are in there, the prisoner is OUT while you are at home: SELECT (or SWITCH TO A SIM) trades one place for the other,
//                so you can look after the family at home and go back to the cell whenever you like. Needs, the clock and the days go on everywhere.
//                Midnight takes a day off the sentence (prDay); a clean week earns a day off for good behavior; fighting in prison adds days (no cops come).
//                Pause menu > PRISON (the STORY tile): the record, SERVE TIME (skip 1, 7 or 30 days) and the story journal. A LIFE sentence never ends.
// SAVED          12 bytes in the jukebox block's spare room (JB_OFF+56): 'P' 'R', days left (65535 = life), who (uid + 1), the sentence, convictions, record, checksum.
//                The prison map itself is saved like any lot. Nothing here is EWRAM that matters: about 20 bytes.
// BAIL          Pause menu > PRISON > PAY BAIL (in the cell, not for LIFE): PR_BAIL simoleons a day, 1, 7 or 30 days. A missed shift in the cell counts as a bad shift (sims.h simShiftEnd) and may get you FIRED (simFire: back to the bottom of the track); so may the arrest itself (prBook), the longer the sentence the likelier.
// ONE PRISONER   Only one Sim of the household can be inside at a time; if a second one is busted meanwhile the old 10 second hold happens ("the cells are full").
#define PR_OFF  (JB_OFF+56)
#define PR_LIFE 0xFFFF
#define PR_BAIL 40   // simoleons per day of sentence bought off (PAY BAIL)
static u16 prDays EWRAM_BSS, prTot EWRAM_BSS, prRec EWRAM_BSS;   // days left (PR_LIFE = life), the sentence as given, the record not yet punished
static HhSim prVis EWRAM_BSS; static u8 prVisSt EWRAM_BSS, prVisRp EWRAM_BSS, prVisM EWRAM_BSS, prVisX EWRAM_BSS, prVisY EWRAM_BSS; static u16 prVisT EWRAM_BSS;   // the visitor: 0 none, 1 walking in, 2 with you, 3 walking out
static u8 prVisit EWRAM_BSS, prGTry EWRAM_BSS;   // a family visit is due today; the guards were tried this stay
static u8 prW1 EWRAM_BSS, prCon EWRAM_BSS, prGood EWRAM_BSS, prTrouble EWRAM_BSS, prCardOn EWRAM_BSS, prInit EWRAM_BSS;   // who is inside (uid + 1, 0 = nobody), convictions, clean days in a row, trouble today, show the booking card

static u8 prSum(const volatile u8*m){ u8 x=0xA7; for(int i=2;i<11;i++) x^=m[i]; return x; }
static u8 prNorm(void){   // who is inside as a place in this household, which is what a load gives back (hhLoad hands the uids out again: you 1, the members 2, 3, ...). 0 = nobody
    if(!prDays||!prW1) return 0;
    if(hhPUid==prW1-1) return 1;
    for(int m=0;m<hhN;m++) if(hhM[m].uid==prW1-1) return (u8)(m+2);
    return 0;
}
static void prSave(void){ if(!prInit) return;   // (before the first prLoad the RAM holds nothing: the saved sentence must not be wiped)
    volatile u8*m=SRAM_BASE+PR_OFF; m[0]='P'; m[1]='R'; m[2]=(u8)prDays; m[3]=(u8)(prDays>>8); m[4]=prNorm(); m[5]=(u8)prTot; m[6]=(u8)(prTot>>8); m[7]=prCon; m[8]=prGood; m[9]=(u8)prRec; m[10]=(u8)(prRec>>8); m[11]=prSum(m); }
static void prLoad(void){
    volatile u8*m=SRAM_BASE+PR_OFF; prInit=1;
    if(m[0]=='P'&&m[1]=='R'&&m[11]==prSum(m)){ prDays=(u16)(m[2]|m[3]<<8); prW1=m[4]; prTot=(u16)(m[5]|m[6]<<8); prCon=m[7]; prGood=m[8]>7?0:m[8]; prRec=(u16)(m[9]|m[10]<<8); if(prW1>HU_N) prW1=0; if(!prW1) prDays=0; }
    else { prDays=prTot=prRec=0; prW1=prCon=prGood=0; }
}
static u16 prSentence(int s){   // how bad you were (points) -> days
    static const u8 lim[11]={3,6,10,15,22,32,45,60,80,105,140};
    static const u16 dy[11]={2,3,5,8,14,25,45,80,140,240,365};
    for(int i=0;i<11;i++) if(s<=lim[i]) return dy[i];
    return PR_LIFE;
}
static int prHere(void){ return nbOk&&nbT.cur<NB_LOTS&&nbT.lot[nbT.cur].on&&nbT.lot[nbT.cur].kind==LKIND_COMM&&nbT.lot[nbT.cur].type==CT_PRISON; }
static int prIn(void){ return prDays&&prW1&&hhPUid==prW1-1&&prHere(); }   // you are the one doing time, and you are in the prison
static int prTotDays(void){ return prTot==PR_LIFE?60000:prTot; }   // (sims.h: how long the sentence is)
static int prShown(void){ return prDays&&prW1; }
static int prHeld(const HhSim*s){   // house.h hhSched: a Sim who is not where they should be today is OUT: the prisoner while you are at home, everyone else while you are at the prison
    if(!prDays||!prW1) return 0;
    return (s->uid==prW1-1)?!prHere():prHere();
}
static const char* prName(void){   // the name of the one doing time
    if(hhPUid==prW1-1) return hhPName;
    for(int m=0;m<hhN;m++) if(hhM[m].uid==prW1-1) return hhM[m].name;
    return "SOMEONE";
}
static char* prDaysTxt(char*d,u16 n){ if(n==PR_LIFE) return slCat(d,"LIFE"); d=slNum(d,n); return slCat(d,n==1?" DAY":" DAYS"); }

// ---------- the prison lot ----------
static int prLotGet(void){   // the prison's lot, made if there is none (-1: no room in the town)
    if(!nbOk) return -1;
    for(int i=0;i<NB_LOTS;i++){ const NbLot*L=&nbT.lot[i]; if(L->on&&L->kind==LKIND_COMM&&L->type==CT_PRISON) return i; }
    int fi=-1; for(int i=0;i<NB_LOTS&&fi<0;i++) if(!nbT.lot[i].on) fi=i;
    if(fi>=0) for(int sz=6;sz>=4;sz--) for(int y=0;y+sz<=NB_H;y++) for(int x=0;x+sz<=NB_W;x++){
        int ok=1; for(int yy=y;yy<y+sz&&ok;yy++) for(int xx=x;xx<x+sz;xx++) if(nbAt(xx,yy)>=0||NB_GR(nbT.cell[yy][xx])==NT_WATER){ ok=0; break; }
        if(!ok) continue;
        nbLotAdd(fi,"PRISON",x,y,sz,sz,LKIND_COMM,CT_PRISON);
        for(int yy=y;yy<y+sz;yy++) for(int xx=x;xx<x+sz;xx++) nbT.cell[yy][xx]=(u8)NT_PLAZA;
        nbSave(); return fi; }
    for(int i=0;i<NB_LOTS;i++){ NbLot*L=&nbT.lot[i];   // no free land: a lot nobody built on and nobody lives on becomes the prison
        if(L->on&&L->kind==LKIND_RES&&i!=nbT.home&&i!=nbT.cur&&L->slot<0&&!nbLives(i)){
            L->kind=LKIND_COMM; L->type=CT_PRISON; const char*nm="PRISON"; int k=0; for(;nm[k]&&k<NB_NAME;k++) L->name[k]=nm[k]; L->name[k]=0; nbSave(); return i; } }
    return -1;
}
static void prisonBuild(int x0,int y0,int x1,int y1){   // nbTemplate: the compound fills the lot's rectangle; you arrive at the front desk (the bottom middle)
    int wp=NWP+57, cx=(x0+x1)/2, hy=y0+6;
    gBox(x0,y0,x1,y1,12);
    gLine(x0,y0,x1,y0,'W',wp); gLine(x0,y1,x1,y1,'W',wp); gLine(x0,y0,x0,y1,'W',wp); gLine(x1,y0,x1,y1,'W',wp);   // the fence: sealed, there is no way out
    for(int c=x0;c+4<=x1;c+=4){ gRoom(c,y0,c+4,y0+5,3,wp); gPut(c+2,y0+5,'D'); gPut(c+1,y0+2,'S'); gPut(c+3,y0+2,'T'); }   // the cell block: a bunk and a toilet in each
    gFree(x0+1,hy+1,'H'); gFree(x0+1,hy+3,'H');                                               // showers
    gFree(x0+4,hy+2,'C'); gFree(x0+6,hy+2,'C'); gFree(x0+5,hy+1,'v');                          // the day room: sofas and a TV
    gFree(x0+9,hy+1,'b'); gFree(x0+10,hy+1,'b');                                              // the library
    gFree(x1-5,hy+2,'K'); gFree(x1-5,hy+5,'K'); gFree(x1-1,hy+1,'F'); gFree(x1-1,hy+2,'F'); gFree(x1-1,hy+4,'Y');   // the canteen: tables, the chow line, a bin
    gFree(x0+3,y1-5,'m'); gFree(x0+5,y1-5,'m'); gFree(x0+7,y1-5,'m'); gFree(x0+10,y1-5,'N'); gFree(x0+12,y1-4,'#');   // the gym
    gFree(x1-3,y1-5,'I');                                                                     // the phone
    gFree(cx+1,y1-1,'Z');                                                                     // a plant at the desk (and no skateboard: nbTemplate puts one beside you when it can)
}

// ---------- the record, the booking, the days ----------
static int prNote(int n){   // npc.h copCrime: you hurt someone. 1 = no cops (in the prison); the days of a fight in there are added to the sentence
    if(prHere()){
        if(prIn()){ if(prDays!=PR_LIFE){ int a=n*3; prDays=(u16)(prDays+a>60000?60000:prDays+a); if(prTot<prDays) prTot=prDays; static char t[24] EWRAM_BSS; char*e=slCat(t,"TROUBLE  +"); e=slNum(e,a); slCat(e," DAYS"); lnote=t; lnoteT=90; }
            else { lnote="TROUBLE  YOU ARE IN FOR LIFE"; lnoteT=90; }
            prTrouble=1; prSave(); }
        return 1; }
    prRec=(u16)(prRec+n>400?400:prRec+n); prSave();
    return 0;
}
static int prBook(void){   // npc.h copArrest, before the stars are cleared: sentence the Sim you control. 1 = off to the prison
    if(prEd||!nbOk||prDays||hhPUid<0||hhPUid>=HU_N) return 0;
    int s=prRec+copWant*6+copHeat+prCon*4;
    if(prLotGet()<0) return 0;
    u16 d=prSentence(s); prDays=d; prTot=d; prW1=(u8)(hhPUid+1); if(prCon<60) prCon++; prRec=0; prGood=0; prTrouble=0; prCardOn=1; prSave();
    { int ch=d==PR_LIFE?230:25+d; if(ch>230) ch=230;   // the boss hears of it: 10 percent for a scuffle, about 2 in 3 for 140 days, almost sure for a long one
      if(rnd8()<ch) simFire("FIRED  YOUR BOSS HEARD ABOUT THE ARREST"); }
    prGo=1; return 1;
}
static void prCancel(void){ if(prCon) prCon--; prDays=prTot=0; prW1=0; prCardOn=0; prSave(); }
static void prRelease(void){   // the sentence is served
    static char t[40] EWRAM_BSS; char*e=slCat(t,prName()); slCat(e,"  RELEASED");
    int was=prIn(); prDays=prTot=0; prW1=0; prGood=0; prTrouble=0; prSave();
    copCool=3600; copHeat=0; copWant=0; lnote=t; lnoteT=140;
    if(was) prGo=2;   // you were inside: you go home
}
static void prDay(void){   // sims.h, every midnight
    if(!prDays||!prW1) return;
    if(!prNorm()){ prDays=prTot=0; prW1=0; prGood=0; prSave(); return; }   // (the one doing time moved out: nobody to keep inside)
    if(prDays!=PR_LIFE){
        prDays--; if(hhN>0&&(rnd8()%3)==0) prVisit=1; else prVisit=0;
        if(prTrouble) prGood=0; else if(prTot>=14&&++prGood>=7&&prDays>1){ prGood=0; prDays--; }   // a clean week earns a day off for good behavior
        prTrouble=0;
        if(!prDays){ prRelease(); return; }
    }
    prSave();
}

// ---------- guards, visitors and the escape (not saved) ----------
// GUARDS    While you serve time 2 guards (3 for a long sentence) walk the yard (the cop sprites, copSt 5, the visitor path code). They also lower your escape odds when close.
// VISITS    Each midnight there is a 1 in 3 chance someone of your household is due; in the afternoon they drop by: comfort and social up, a little fun.
// ESCAPE    R at the outer fence (CLIMB THE FENCE, lnear 16, prClimb): BODY helps, guards within 8 tiles hurt. Out: you are home, WANTED +2, and the cops may raid. Caught: days added and a stun.
static int prGuardsNear(void){
    int px=(int)(lfx>>8), py=(int)(lfy>>8), n=0;
    if(copSt==5) for(int i=0;i<copN;i++){ int dx=px-(int)(copS[i].fx>>8), dy=py-(int)(copS[i].fy>>8); if(fxAbs(dx)+fxAbs(dy)<=8) n++; }
    return n;
}
static void prGuardSpawn(void){
    int x0,y0,x1,y1; const NbLot*L=&nbT.lot[nbT.cur]; nbRect(L,&x0,&y0,&x1,&y1);
    int want=(prTot>=140)?3:2; if(want>COP_CAP) want=COP_CAP;
    copN=0; copSt=0;
    for(int t=0;t<200&&copN<want;t++){
        int x=x0+1+(int)(((u32)rnd8()<<8|rnd8())%(unsigned)(x1-x0-1)), y=y0+1+(int)(((u32)rnd8()<<8|rnd8())%(unsigned)(y1-y0-1));
        if(!hhWalk(x,y)||fxAbs(x-(int)(lfx>>8))+fxAbs(y-(int)(lfy>>8))<6) continue;
        HhSim*c=&copS[copN]; c->fx=x*256+128; c->fy=y*256+128; c->pn=c->pi=0; c->gok=0; c->stage=AG_ADULT; c->act=HA_WALK; c->hd=0; copRp[copN]=(u8)(20*copN); copN++; }
    if(copN) copSt=5;
}
static const HhSim* prVisSprite(void){ return (prVisSt&&prIn())?&prVis:0; }
static int prFence(int x,int y){   // a tile of the sealed outer fence, and you are the one doing time
    if(!prIn()) return 0;
    int x0,y0,x1,y1; nbRect(&nbT.lot[nbT.cur],&x0,&y0,&x1,&y1);
    return x==x0||x==x1||y==y0||y==y1;
}
static void prVisitStart(void){   // someone of the household walks in from the front desk
    prVisit=0; if(hhN<1) return;
    int x0,y0,x1,y1; nbRect(&nbT.lot[nbT.cur],&x0,&y0,&x1,&y1); int cx=(x0+x1)/2, sx=-1, sy=-1;
    for(int d=0;d<8&&sx<0;d++) for(int s=-1;s<=1&&sx<0;s+=2) for(int r=1;r<=2;r++){ int x=cx+s*d, y=y1-r; if(hhWalk(x,y)){ sx=x; sy=y; break; } }
    if(sx<0) return;
    int m=(int)(rnd8()%(unsigned)hhN); prVisM=(u8)m; prVisX=(u8)sx; prVisY=(u8)sy;
    HhSim*c=&prVis; c->fx=sx*256+128; c->fy=sy*256+128; c->pn=c->pi=0; c->gok=0; c->stage=AG_ADULT; c->act=HA_WALK; c->hd=0; c->uid=(u8)m;
    prVisSt=1; prVisRp=45; prVisT=0;
    static char t[40] EWRAM_BSS; char*e=slCat(t,hhM[m].name); slCat(e,"  IS HERE TO VISIT"); lnote=t; lnoteT=100;
}
static void prVisArrive(void){   // they reached you: a talk
    static char t[40] EWRAM_BSS; char*e=slCat(t,hhM[prVisM%hhN].name); slCat(e,"  COMFORTS YOU");
    sCom+=10; if(sCom>100) sCom=100; sSoc+=25; if(sSoc>100) sSoc=100; moodEvent(M_SOFA); simEvent(SE_TALK); skGain(SK_CHARM,1); sfxPlay(SFX_GASP);
    prVisSt=2; prVisT=480; prVis.act=HA_IDLE; prVis.pn=prVis.pi=0; lnote=t; lnoteT=110;
}
static void prVisTick(int*planned){
    HhSim*c=&prVis; int px=(int)(lfx>>8), py=(int)(lfy>>8);
    if(prVisSt==1){
        int dx=px-(int)(c->fx>>8), dy=py-(int)(c->fy>>8);
        if(dx*dx+dy*dy<=2){ prVisArrive(); return; }
        if(++prVisT>1800){ prVisSt=0; return; }
        if(c->pi>=c->pn||++prVisRp>=45){
            if(!*planned){ *planned=1; prVisRp=0; hhGX=px; hhGY=py; int r=hhPlan(c,1); if(r==1){ prVisArrive(); return; } if(r==0){ prVisSt=0; return; } } }
        if(c->pi<c->pn) hhStepAlong(c);
    } else if(prVisSt==2){
        if(prVisT>0) prVisT--;
        if(!prVisT&&!*planned){ *planned=1; hhGX=prVisX; hhGY=prVisY; int r=hhPlan(c,1); if(r<=1){ prVisSt=0; return; } c->act=HA_WALK; prVisSt=3; prVisT=0; }
    } else if(prVisSt==3){
        if(c->pi>=c->pn||++prVisT>1800){ prVisSt=0; return; }
        hhStepAlong(c);
    }
}
static int prGuardTick(int*planned){   // npc.h copTick: 1 = you are doing time, the cops stay out
    if(!prIn()){ prGTry=0; prVisSt=0; if(copSt==5){ copSt=0; copN=0; } return 0; }
    if(copSt!=5){ if(!prGTry){ prGTry=1; prGuardSpawn(); } if(copSt!=5) return 1; }
    for(int i=0;i<copN;i++){ HhSim*c=&copS[i];
        if(c->pi>=c->pn){ if(copRp[i]<60) copRp[i]++; if(copRp[i]>=60&&!*planned){ *planned=1; copRp[i]=0; hhPlan(c,0); } }
        else hhStepAlong(c); }
    if(prVisit&&!prVisSt&&simMin>=720&&simMin<1080&&!lstun) prVisitStart();
    if(prVisSt) prVisTick(planned);
    return 1;
}
static int prEscape(void){   // 1 = out (the main loop takes you home)
    int odds=25+skLvl(SK_BODY)*8-prGuardsNear()*15; if(odds<5) odds=5; if(odds>80) odds=80;
    if((int)(rnd8()*100/256)<odds){
        prDays=prTot=0; prW1=0; prGood=0; prTrouble=0; prCardOn=0; prSave();
        copWant=(u8)(copWant+2>6?6:copWant+2); copRaid=1; copCool=0; copHeat=0; skGain(SK_BODY,3);
        lnote="YOU ESCAPED  THEY WILL COME FOR YOU"; lnoteT=120; prGo=2; return 1; }
    if(prDays!=PR_LIFE){ int a=prTot/6<5?5:prTot/6; prDays=(u16)(prDays+a>60000?60000:prDays+a); if(prTot<prDays) prTot=prDays; }
    prTrouble=1; lstun=120; lsp=0; lgrind=0; moodEvent(M_HURT_BIG); sfxPlay(SFX_HIT); prSave();
    lnote="CAUGHT ON THE FENCE  DAYS ADDED"; lnoteT=110; return 0;
}

static void prClimb(void){   // R at the fence
    if(!prIn()) return;
    lnote="CLIMBING THE FENCE..."; lnoteT=40; prEscape();
}

// ---------- switching between the cell and home ----------
static void prSelect(void){ if(hhN<1){ toast("YOU LIVE ALONE  NO ONE TO SWITCH TO"); return; } hhSwitchFrom(0); prGo=2; }   // SELECT in the prison: you are the next Sim of the household, at home
static int prSwitchHook(int m){   // house.h hhSwitchTo: 1 = handled (the pause menu's SWITCH TO A SIM)
    if(!prDays||!prW1||custom||m<0||m>=hhN) return 0;
    if(prIn()){ hhSwitchFrom(m); prGo=2; return 1; }   // out of the cell, home as that Sim
    if(hhM[m].uid==prW1-1){ hhSwitchFrom(m); if(!prHere()) prGo=1; return 1; }   // back to the cell
    return 0;
}
static int prTransfer(int code){   // main.c lifeMode: 1 = to the prison, 2 = home (the live lot changes; 0 = it did not work)
    copSt=0; copN=0; copT=0;
    if(code==1){ int lot=prLotGet(); if(lot<0){ toast("NO ROOM IN TOWN FOR A PRISON"); prCancel(); return 0; }
        if(!nbGo(lot)){ toast(nbErr); prCancel(); return 0; } }
    else if(code==2){ if(!nbGo(nbT.home)){ toast(nbErr); return 0; } }
    return 1;
}
static void prCard(void);
static void prApply(int ed){   // main.c lifeModeRun, right after lifeInit: the sentence decides where you are and who is OUT
    prLoad(); prGo=0;
    if(!prDays||ed) return;
    if(!nbOk){ prDays=prTot=0; prW1=0; prSave(); return; }   // no town, no prison
    if(hhPUid==prW1-1&&!prHere()){ prGo=1; return; }          // the prisoner starts in the prison (a restart, or the Sim just changed)
    for(int m=0;m<hhN;m++) if(prHeld(&hhM[m])) hhM[m].act=HA_AWAY;
    if(prCardOn&&prIn()){ prCardOn=0; prCard(); }
}

// ---------- the screens ----------
static void prWait(void){ u16 prev=keyNow(); for(;;){ u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) break; vsync(); } }
static void prLine(int y,const char*a,const char*b,u16 col){ text(30,y,a,DIMC,1); text(210-tw(b,1),y,b,col,1); }
static void prCard(void){   // BOOKED
    objHideAll(); REG_DISPCNT=0x0403;
    box(24,22,192,116); rect(25,23,190,15,RGB(5,12,24)); rect(25,38,190,1,GOLD);
    text(120-tw("BOOKED",2)/2,24,"BOOKED",GOLD,2);
    char b[24]; prDaysTxt(b,prTot);
    prLine(46,"WHO",prName(),WHITE); prLine(58,"SENTENCE",b,prTot==PR_LIFE?RGB(31,8,8):GOLD);
    char c[12]; slNum(c,prCon); prLine(70,"CONVICTIONS",c,WHITE);
    text(30,86,"A DAY OFF EVERY MIDNIGHT  A CLEAN WEEK",DIMC,1); text(30,95,"EARNS ONE MORE  FIGHTS ADD DAYS",DIMC,1);
    text(30,108,"SELECT  SWITCH TO ANOTHER SIM",RGB(14,26,31),1); text(30,117,"PAUSE  PRISON  SERVE TIME",RGB(14,26,31),1);
    text(170,128,"A OK",DIMC,1);
    present(); prWait();
    winFull(); REG_DISPCNT=0x3443; hudApplyLayout(); liveInvalidate(); camSnap=1;
}
static void prRecord(void){
    box(24,22,192,116); rect(25,23,190,15,RGB(5,12,24)); rect(25,38,190,1,GOLD);
    text(30,27,"PRISON RECORD",GOLD,1);
    char b[24]; prDaysTxt(b,prDays); prLine(46,"WHO",prName(),WHITE); prLine(58,"DAYS LEFT",b,prDays==PR_LIFE?RGB(31,8,8):GOLD);
    char c[24]; prDaysTxt(c,prTot); prLine(70,"SENTENCE",c,WHITE);
    char d[12]; slNum(d,prCon); prLine(82,"CONVICTIONS",d,WHITE);
    prLine(94,"WHERE",prHere()?"IN PRISON":"AT HOME",WHITE);
    char e[12]; slNum(e,prGood); slCat(e," OF 7"); prLine(106,"CLEAN DAYS",e,prTot>=14?RGB(14,30,14):DIMC);
    text(170,128,"A OK",DIMC,1);
    present(); prWait();
}
static int prBail(void){   // PAY BAIL: buy days off the sentence at PR_BAIL simoleons a day (not for LIFE). 1 = the cell door opened or the days came off
    static const u8 dn[3]={1,7,30}; static char lb[3][20] EWRAM_BSS; const char*it[3]; int ix[3], n=0;
    for(int i=0;i<3;i++){ int d=dn[i]; if(d>prDays) d=prDays; int cost=d*PR_BAIL; if(simMoney<cost) continue;
        char*e=slNum(lb[n],d); e=slCat(e,d==1?" DAY  ":" DAYS  "); e=slCat(e,"$"); slNum(e,cost); it[n]=lb[n]; ix[n++]=i;
        if(d==prDays) break; }   // (a bigger offer than the days left is the same offer)
    if(!n){ toast("NOT ENOUGH CASH FOR BAIL"); return 0; }
    int c=menu("PAY BAIL",it,n); if(c<0) return 0;
    int d=dn[ix[c]]; if(d>prDays) d=prDays; simMoney-=d*PR_BAIL; simsSave();
    if(d>=prDays){ prRelease(); return 1; }
    prDays=(u16)(prDays-d); prSave();
    static char t[32] EWRAM_BSS; char*e=prDaysTxt(t,prDays); slCat(e," LEFT"); lnote=t; lnoteT=90; return 1;
}
static void prServe(int days){   // skip days in the cell: the clock runs to midnight, the days come off, bills are not paid from the cell
    for(int d=0;d<days&&prDays&&!prGo;d++){
        box(50,60,140,30); text(60,66,"SERVING TIME...",WHITE,1);
        char b[24]; char*e=slNum(b,d+1); e=slCat(e," OF "); slNum(e,days); text(60,77,b,DIMC,1); present();
        int d0=simDay; for(int g=0;g<1500&&simDay==d0;g++) simMinute();
    }
    if(prDays&&!prGo){ static char t[32] EWRAM_BSS; char*e=prDaysTxt(t,prDays); slCat(e," LEFT"); lnote=t; lnoteT=90; }
}
static void prisonScreen(void){   // pause menu > PRISON (the STORY tile while a sentence runs)
    for(;;){
        const char*it[5]; int id[5], n=0;
        it[n]="PRISON RECORD"; id[n++]=0;
        if(prIn()){ it[n]="SERVE TIME"; id[n++]=1; }
        if(prIn()&&prDays!=PR_LIFE){ it[n]="PAY BAIL"; id[n++]=3; }
        it[n]="STORY JOURNAL"; id[n++]=2;
        int c=menu("PRISON",it,n); if(c<0) return;
        if(id[c]==0) prRecord();
        else if(id[c]==1){ static const char* const sv[3]={"SKIP 1 DAY","SKIP 7 DAYS","SKIP 30 DAYS"}; static const u8 dn[3]={1,7,30}; int s=menu("SERVE TIME",sv,3); if(s>=0){ prServe(dn[s]); return; } }
        else if(id[c]==3){ if(prBail()) return; }
        else storyScreen();
    }
}

// ---------- a new life, and the change of lot ----------
static void prClear(void){   // NEW LIFE / NEW GAME: no sentence, no record (inside the cell: you go home)
    int in=prIn(); prInit=1; prDays=prTot=prRec=0; prW1=prCon=prGood=prTrouble=0; prCardOn=0; prSave(); if(in) prGo=2;
}
static int prLifeSwitch(int code){   // main.c lifeModeRun: 1 = to the prison, 2 = home. The live lot changes and the game starts again on it (1 = it worked)
    int f=lfood, b=lbl, h=lhp, mf=moodFun, mh=moodHap;   // (lifeInit starts fresh: the Sim keeps what they have)
    simsSaveNow(); hhSave();                              // the life and the household first: lifeInit loads them again
    if(!prTransfer(code)) return 0;
    pkHome=-1; lifeInit(); prApply(0); phoneEnsure();
    lfood=f; lbl=b; lhp=h; moodFun=mf; moodHap=mh; moodSt=moodState();
    lcN=lcPts=lcT=lcBank=lcBankT=lcamPend=0; lcNmN=0; vbase=cview=0; lcamF=0;
    return 1;
}
