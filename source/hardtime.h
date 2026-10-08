// hardtime.h - HARD TIME: the prison made to play like MDickie's Hard Time (inmate stats, reputation, gangs, a daily regime, work, smokes, talks).
//   Needs prison.h (prIn, prShown, prDays, prTot, prTrouble, prGuardsNear, prSave) and skills.h (skGain, skLvl, skAt, hmNeed, SK_BODY, SK_LOGIC).
//
// INMATE      STRENGTH = the BODY skill, INTELLIGENCE = the LOGIC skill (the treadmill and the library train them), AGILITY = htAgi (the treadmill
//             trains it in the prison too). REPUTATION 0..100 (htRep) starts by how bad your crime was: a scuffle starts you at 10, a long sentence at 55.
// REGIME      07:00 WAKE UP, 12:00 CHOW TIME, 22:00 LOCKDOWN until 07:00. Out of your cell for half an hour in lockdown and a warden comes: days added.
// WORK        PAUSE > PRISON > WORK AND GANG. SWEEP FLOORS $12 a day (and a little rep lost), KITCHEN $20 (LOGIC 1), WORKSHOP $28 (BODY 2). Paid at midnight.
// GANGS       THE PEAKS (clean living), THE POWERS (the brains), THE GLADIATORS (the muscle), THE SMOKE (dealers). Each asks for something and gives a perk.
// TABLES      R next to a canteen table: CHAT (a conversation, two answers: a tough answer earns rep, giving in costs it), BUY / SELL / SMOKE.
//             A low rep makes you a target: bullies take cash at midnight. Fights in the prison raise your rep.
// SAVED       10 bytes at JB_OFF+68: 'H' 'T', rep, gang, agility, smokes, job, checksum, grudge, warden goal (the jukebox block's last spare bytes). About 16 bytes of RAM.
// COURT       hardcourt.h: a warden who catches you (a fight in sight, dealing, out of your cell in lockdown, a cell search) beats you and drags you before the judge.
// WARDENS     a goal now and then (read, work out, work, keep clean): meet it by midnight for 2 days off, at a cost in rep.
#define HT_OFF (JB_OFF+68)
static u8 htRep EWRAM_BSS, htGang EWRAM_BSS, htAgi EWRAM_BSS, htCig EWRAM_BSS, htJob EWRAM_BSS, htInit EWRAM_BSS;
static u8 htTalks EWRAM_BSS, htDeals EWRAM_BSS, htOut EWRAM_BSS, htWarned EWRAM_BSS; static u16 htLastMin EWRAM_BSS;
static u8 htPend EWRAM_BSS, htGoal EWRAM_BSS, htGrudge EWRAM_BSS;   // the charge waiting for court (1 FIGHTING, 2 DEALING, 3 OUT AFTER LOCKDOWN, 4 CONTRABAND), the warden's goal, days the warden holds a grudge
static void htCourt(void);   // hardcourt.h
static const char* const htGoalNm[5]={"","READ 2 BOOKS","WORK OUT TWICE","DO YOUR JOB","STAY OUT OF TROUBLE"};
static char htMsg[40] EWRAM_BSS, htQb[3][40] EWRAM_BSS; static u8 htQi EWRAM_BSS, htQn EWRAM_BSS;
static void htQ(const char*a,const char*b,int n){ if(htQn>=3) return; htQn++; char*e=slCat(htQb[htQi],a); if(n>=0) e=slNum(e,n); if(b) slCat(e,b); simQPush(htQb[htQi]); htQi=(u8)((htQi+1)%3); }   // a note after the ones waiting
static const char* const htGangNm[5]={"NO GANG","THE PEAKS","THE POWERS","THE GLADIATORS","THE SMOKE"};
static const char* const htGangAsk[5]={"","A CLEAN WEEK  NO TROUBLE","LOGIC 1  PAYS DUES $5","BODY 1  REP 30  DUES $5","REP 25  SMOKES ARE CHEAP"};
static const char* const htJobNm[4]={"NO JOB","SWEEP FLOORS","KITCHEN","WORKSHOP"};
static const u8 htPay[4]={0,12,20,28};

static u8 htSum(const volatile u8*m){ return (u8)(0x3C^m[2]^(m[3]<<1)^(m[4]<<2)^(m[5]<<3)^(m[6]<<4)); }
static void htSave(void){ if(!htInit) return; volatile u8*m=SRAM_BASE+HT_OFF; m[0]='H'; m[1]='T'; m[2]=htRep; m[3]=htGang; m[4]=htAgi; m[5]=htCig; m[6]=htJob; m[7]=htSum(m); m[8]=htGrudge; m[9]=htGoal; }
static void htLoad(void){
    volatile u8*m=SRAM_BASE+HT_OFF; htInit=1;
    if(m[0]=='H'&&m[1]=='T'&&m[7]==htSum(m)){ htRep=m[2]>100?100:m[2]; htGang=m[3]>4?0:m[3]; htAgi=m[4]; htCig=m[5]>9?9:m[5]; htJob=m[6]>3?0:m[6]; htGrudge=m[8]>3?0:m[8]; htGoal=m[9]>4?0:m[9]; }
    else { htRep=15; htGang=htAgi=htCig=htJob=htGrudge=htGoal=0; }
    htTalks=htDeals=htOut=htWarned=htPend=0; htLastMin=0xFFFF;
}
static int htAgiLvl(void){ int l=0; for(int i=0;i<5;i++) if(htAgi>=skAt[i]) l=i+1; return l; }
static void htRepAdd(int d){ int v=htRep+d; htRep=(u8)(v<0?0:v>100?100:v); }
static const char* htRepNm(void){ return htRep<15?"PUSHOVER":htRep<30?"NEWBIE":htRep<50?"KNOWN":htRep<70?"HARD":"LEGEND"; }
static char* htNote(const char*a,int d,const char*b){ char*e=slCat(htMsg,a); if(d){ *e++=d<0?'-':'+'; e=slNum(e,d<0?-d:d); *e=0; } if(b) e=slCat(e,b); lnote=htMsg; lnoteT=90; return e; }
static void htAddDays(int a){ if(prDays!=PR_LIFE){ prDays=(u16)(prDays+a>60000?60000:prDays+a); if(prTot<prDays) prTot=prDays; } prTrouble=1; prSave(); }

// ---------- the prison calls these ----------
static void htBook(int d){   // prBook: a new sentence. The worse the crime, the tougher you start
    htRep=(u8)(d<=3?10:d<=25?20:d<=80?30:d<=240?40:55); htGang=0; htJob=0; htCig=0; htGoal=htGrudge=htPend=0; htTalks=htDeals=htOut=htWarned=0; htSave();
}
static int htFight(int n){   // prNote: you hurt someone in the prison: a hard man gets known. 1 = a guard saw it (court decides the days, prNote adds none)
    htRepAdd(n*(htGang==3?3:2)); htSave();
    if(prGuardsNear()>0&&!htPend&&prDays!=PR_LIFE){ htPend=1; return 1; }
    return 0;
}
static void htRelease(void){ htJob=0; htCig=0; htGang=0; htOut=0; htGoal=htGrudge=htPend=0; htSave(); }
static void htDay(void){   // prDay, every midnight of a sentence: work pays, the gang takes its due, bullies pick on the weak, the warden checks your goal and searches
    if(!prShown()) return;
    htQn=0;
    if(htJob&&prIn()&&!prTrouble){ int p=htPay[htJob]+(htGang==2?htPay[htJob]/4:0); simMoney+=p; if(simMoney>9999) simMoney=9999; htQ("PAID $",0,p); moodEvent(M_PAY); if(htJob==1) htRepAdd(-1); }
    else if(htJob) htQ("SHIFT MISSED",0,-1);
    if(htGang==2||htGang==3){ if(simMoney>=5){ simMoney-=5; htQ("GANG DUES $5",0,-1); } else { htGang=0; htQ("OUT OF THE GANG  NO DUES",0,-1); } }
    if(htGang==2) skGain(SK_LOGIC,1);
    if(htGang==3) skGain(SK_BODY,1);
    if(htGang==1&&prTrouble){ htGang=0; htQ("THROWN OUT OF THE PEAKS",0,-1); }
    if(htRep<20&&prIn()&&simMoney>0&&(rnd8()%3)==0){ int t=simMoney<15?simMoney:15; simMoney-=t; hmNeed(&sCom,-10); htQ("A BULLY TOOK $",0,t); }
    if(htGoal&&prIn()){   // the warden's goal: met = 2 days off, and the cons call you a teacher's pet
        int ok=htGoal==1?hmBook>=2:htGoal==2?hmRuns>=2:htGoal==3?(htJob&&!prTrouble):!prTrouble;
        if(ok){ if(prDays!=PR_LIFE&&prDays>3) prDays-=2; htRepAdd(-3); htQ("WARDEN  GOAL MET  2 DAYS OFF  REP -3",0,-1); }
        else htQ("WARDEN  YOU MISSED THE GOAL",0,-1);
        htGoal=0;
    }
    if(!htGoal&&prIn()&&prDays!=PR_LIFE&&prDays>6&&rnd8()<100){ htGoal=(u8)(1+rnd8()%4); htQ("WARDEN GOAL  ",htGoalNm[htGoal],-1); }
    if(htCig&&prIn()&&!htPend&&(rnd8()%100)<(htGrudge?50:25)){ htCig=0; htPend=4; htQ("CELL SEARCH  SMOKES FOUND",0,-1); }
    if(htGrudge) htGrudge--;
    htTalks=htDeals=0; htOut=0; htWarned=0;
    simsSave(); htSave();
}
static int htInCell(void){   // the cell block is the top rows of the compound
    int x0,y0,x1,y1; nbRect(&nbT.lot[nbT.cur],&x0,&y0,&x1,&y1); return (int)(lfy>>8)<=y0+4;
}
static void htTick(void){   // prGuardTick, every step while you serve: the daily regime
    if(htPend&&!simAct&&lstun<=0){ htCourt(); return; }   // hardcourt.h
    if(simMin==htLastMin) return;
    htLastMin=(u16)simMin;
    if(simMin==420){ lnote="WAKE UP  ROLL CALL"; lnoteT=90; }
    else if(simMin==720){ lnote="CHOW TIME  CANTEEN IS OPEN"; lnoteT=90; }
    else if(simMin==1320){ lnote="LOCKDOWN  BACK TO YOUR CELL"; lnoteT=110; htOut=0; htWarned=0; }
    if(simMin>=1320||simMin<420){   // lockdown
        if(htInCell()){ htOut=0; }
        else if(++htOut>=(htGrudge?15:30)&&!htWarned&&!htPend){ htWarned=1; htOut=0; htPend=3; lnote="WARDEN  OUT OF YOUR CELL"; lnoteT=90; }
    }
}
static void htUse(int k){   // skills.h homeUse: the gym trains your agility in the prison too
    if(!prIn()) return;
    if(k==15&&lfood>=20&&sNrg>=25){ int o=htAgiLvl(); int v=htAgi+2; htAgi=(u8)(v>250?250:v); if(htAgiLvl()>o){ lnote="AGILITY UP"; lnoteT=70; } htSave(); }
}

// ---------- the screens ----------
static void htLine(int y,const char*a,const char*b,u16 col){ text(20,y,a,DIMC,1); text(220-tw(b,1),y,b,col,1); }
static void htLvl(char*d,int l){ slNum(d,l); slCat(d," OF 5"); }
static void htCard(void){   // the inmate card
    box(14,6,212,148); rect(15,7,210,13,RGB(5,12,24)); rect(15,20,210,1,GOLD);
    text(20,10,"INMATE CARD",GOLD,1); { const char*nm=prName(); text(220-tw(nm,1),10,nm,WHITE,1); }
    char b[24]; int y=26;
    htLvl(b,skLvl(SK_BODY)); htLine(y,"STRENGTH",b,WHITE); y+=11;
    htLvl(b,htAgiLvl()); htLine(y,"AGILITY",b,WHITE); y+=11;
    htLvl(b,skLvl(SK_LOGIC)); htLine(y,"INTELLIGENCE",b,WHITE); y+=11;
    { char*e=slNum(b,htRep); e=slCat(e,"  "); slCat(e,htRepNm()); htLine(y,"REPUTATION",b,htRep>=50?RGB(14,30,14):htRep<20?RGB(31,10,10):GOLD); y+=11; }
    htLine(y,"GANG",htGangNm[htGang],htGang?GOLD:DIMC); y+=11;
    htLine(y,"WORK",htJobNm[htJob],htJob?WHITE:DIMC); y+=11;
    { char*e=slCat(b,"$"); slNum(e,simMoney); htLine(y,"CASH",b,WHITE); y+=11; }
    slNum(b,htCig); htLine(y,"SMOKES",b,WHITE); y+=11;
    prDaysTxt(b,prDays); htLine(y,"DAYS LEFT",b,prDays==PR_LIFE?RGB(31,8,8):GOLD); y+=11;
    { const char*r=simMin>=1320||simMin<420?"LOCKDOWN":simMin>=720&&simMin<780?"CHOW TIME":"FREE TIME"; htLine(y,"REGIME",r,r[0]=='L'?RGB(31,10,10):WHITE); }
    text(176,142,"A OK",DIMC,1);
    present(); prWait();
}
static void htWork(void){
    static char lb[4][28] EWRAM_BSS; const char*it[4]; int id[4], n=0;
    for(int j=1;j<4;j++){ char*e=slCat(lb[n],htJobNm[j]); e=slCat(e,"  $"); e=slNum(e,htPay[j]); if(j==2) slCat(e,"  LOGIC 1"); if(j==3) slCat(e,"  BODY 2"); it[n]=lb[n]; id[n++]=j; }
    it[n]="NO JOB"; id[n++]=0;
    int c=menu("WORK",it,n); if(c<0) return; int j=id[c];
    if(j==2&&skLvl(SK_LOGIC)<1){ toast("YOU NEED LOGIC 1 FOR THE KITCHEN"); return; }
    if(j==3&&skLvl(SK_BODY)<2){ toast("YOU NEED BODY 2 FOR THE WORKSHOP"); return; }
    htJob=(u8)j; htSave(); toast(j?"YOU HAVE A JOB  PAID AT MIDNIGHT":"NO MORE WORK");
}
static void htGangMenu(void){
    static char lb[5][36] EWRAM_BSS; const char*it[5]; int id[5], n=0;
    if(htGang){ it[n]="LEAVE THE GANG"; id[n++]=0; }
    else for(int g=1;g<5;g++){ char*e=slCat(lb[n],htGangNm[g]); e=slCat(e,"  "); slCat(e,htGangAsk[g]); it[n]=lb[n]; id[n++]=g; }
    int c=menu(htGang?htGangNm[htGang]:"JOIN A GANG",it,n); if(c<0) return; int g=id[c];
    if(!g){ htGang=0; htRepAdd(-5); htSave(); toast("YOU LEFT THE GANG  REP -5"); return; }
    if(g==1&&prGood<3){ toast("THE PEAKS WANT 3 CLEAN DAYS"); return; }
    if(g==2&&skLvl(SK_LOGIC)<1){ toast("THE POWERS WANT LOGIC 1"); return; }
    if(g==3&&(skLvl(SK_BODY)<1||htRep<30)){ toast("THE GLADIATORS WANT BODY 1 AND REP 30"); return; }
    if(g==4&&htRep<25){ toast("THE SMOKE WANTS REP 25"); return; }
    htGang=(u8)g; htRepAdd(3); htSave(); toast("YOU ARE IN  REP +3");
}
static void htLife(void){   // pause menu > PRISON > WORK AND GANG
    for(;;){ const char*it[3]={"WORK","GANG","WARDEN GOAL"}; int c=menu("PRISON LIFE",it,3); if(c<0) return;
        if(c==0) htWork(); else if(c==1) htGangMenu();
        else if(htGoal){ char*e=slCat(htMsg,"GOAL  "); e=slCat(e,htGoalNm[htGoal]); slCat(e,"  2 DAYS OFF"); toast(htMsg); }
        else toast("NO GOAL FROM THE WARDEN YET"); }
}

// ---------- the canteen table: talks and trade ----------
typedef struct { const char*q,*a,*b; s8 ra,rb,ca,cb,ma,mb; } HtTalk;   // ra / rb: rep for answer a / b; ca / cb: comfort; ma / mb: money
static const HtTalk htTk[8]={
    {"GIVE ME YOUR DESSERT","HAND IT OVER","GET LOST",              -4, 4,  -6, -2,  0,  0},
    {"YOU LOOK LIKE NEW MEAT","I KNOW THE ROPES","PLEASE DONT",     2,-3,   3, -8,  0,  0},
    {"HEARD YOU ARE TOUGH","WANT TO TEST ME","JUST WORDS",          4,-2,  -3,  2,  0,  0},
    {"LEND ME TEN","SURE","NO",                                    -2, 2,   4, -2,-10,  0},
    {"NICE ARMS","THANKS","MIND YOUR BUSINESS",                     1, 2,   8, -3,  0,  0},
    {"WARDEN WANTS A WORD","YES SIR","NO THANKS",                  -5, 3,   0, -2,  0,  0},
    {"WATCH MY BACK","I GOT YOU","NOT MY PROBLEM",                  2,-1,   5, -2,  0,  0},
    {"YOU OWE ME TEN","HERE","TRY AND TAKE IT",                    -3, 4,  -4,  0,-10,  0}
};
static void htChat(void){
    if(htTalks>=3){ toast("EVERYONE HAS HAD ENOUGH OF YOU TODAY"); return; }
    int i=(int)(rnd8()&7); const HtTalk*t=&htTk[i]; const char*it[2]={t->a,t->b};
    int c=menu(t->q,it,2); if(c<0) return;
    int r=c?t->rb:t->ra, cm=c?t->cb:t->ca, m=c?t->mb:t->ma; htTalks++;
    if(m<0){ if(simMoney<-m){ toast("YOU CANT PAY  REP -3"); htRepAdd(-3); htSave(); return; } simMoney+=m; }
    if(i==5&&c==0&&prDays!=PR_LIFE&&prDays>1) prDays--;   // doing what the warden says shaves a day off (and costs you respect)
    htRepAdd(r); hmNeed(&sCom,cm); if(cm>=0) moodEvent(M_SOFA); else moodEvent(M_HURT_BIG); htSave();
    htNote("REP ",r,i==5&&c==0?"  A DAY OFF":0);
}
static int htBust(void){   // dealing with a guard close by: 1 = caught
    if(prGuardsNear()>0&&!htPend&&(rnd8()%100)<45){ htCig=0; htPend=2; if(htGang==1) htGang=0; lnote="CAUGHT DEALING"; lnoteT=90; htSave(); return 1; }
    return 0;
}
static int htTable(void){   // main.c lifeStep, R with nothing in reach: next to a canteen table of the prison? 1 = the menu ran
    if(!prIn()) return 0;
    int ok=0; for(int dy=-1;dy<=1&&!ok;dy++) for(int dx=-1;dx<=1;dx++){ int tx=(int)(lfx>>8)+dx, ty=(int)(lfy>>8)+dy; if(tx<0||ty<0||tx>=MW||ty>=MH) continue; if(lifeMap[ty][tx]=='K'){ ok=1; break; } }
    if(!ok) return 0;
    int buy=htGang==4?4:6;
    static char lb[4][24] EWRAM_BSS; const char*it[4]; int id[4], n=0;
    it[n]="CHAT"; id[n++]=0;
    { char*e=slCat(lb[n],"BUY A SMOKE  $"); slNum(e,buy); it[n]=lb[n]; id[n++]=1; }
    if(htCig){ char*e=slCat(lb[n],"SELL A SMOKE  $"); slNum(e,htGang==4?14:11); it[n]=lb[n]; id[n++]=2; }
    if(htCig){ it[n]="SMOKE ONE"; id[n++]=3; }
    int c=menu("CANTEEN TABLE",it,n);
    if(c>=0){
        if(id[c]==0) htChat();
        else if(id[c]==1){ if(htCig>=9) toast("YOUR POCKETS ARE FULL"); else if(simMoney<buy) toast("NOT ENOUGH CASH"); else if(!htBust()){ simMoney-=buy; htCig++; htSave(); toast("BOUGHT A SMOKE"); } }
        else if(id[c]==2){ if(htDeals>=4) toast("NO ONE IS BUYING"); else if(!htBust()){ int p=htGang==4?14:11; simMoney+=p; if(simMoney>9999) simMoney=9999; htCig--; htDeals++; if(htGang==1){ htGang=0; toast("THROWN OUT OF THE PEAKS"); } else htRepAdd(1); htSave(); toast("SOLD A SMOKE"); } }
        else { htCig--; hmNeed(&sCom,25); if(lhp>3) lhp-=3; moodEvent(M_CHILL); lstun=60; lsp=0; htSave(); lnote="A SMOKE  AHH"; lnoteT=60; }
    }
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    return 1;
}
