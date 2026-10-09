// hardcourt.h - COURT: the warden catches you, beats you and drags you before the judge (the way Hard Time does it).
//   A charge waits in htPend (hardtime.h) until you are on your feet and awake: then the warden's beating, the court screen, your plea and the verdict.
//   GUILTY PLEA    half the days, and the cons respect an honest man (rep +2).
//   NOT GUILTY     the odds rise with your LOGIC and CHARISMA, the Powers' lawyers and a good reputation, and fall with your convictions and the warden's grudge.
//                  Won: no days, the warden is reprimanded (and holds it against you for 2 days), but the cons look down on an innocent man (rep -3).
//                  Lost: base days and a half (you wasted the court's time), rep +3.
//   A conviction counts against you the next time you are sentenced (prCon). Needs hardtime.h and prison.h.
static const char* const htChNm[5]={"","FIGHTING","DEALING","OUT AFTER LOCKDOWN","CONTRABAND"};
static const u8 htChDays[5]={0,6,5,3,4};
static const char* const htWard[4]={"WARDEN KRUPP","WARDEN STONE","WARDEN BELL","WARDEN VANCE"};
static void htCourtHead(const char*t){ box(14,10,212,136); rect(15,11,210,14,RGB(5,12,24)); rect(15,25,210,1,GOLD); text(120-tw(t,2)/2,11,t,GOLD,2); }
static void htCourt(void){
    int ch=htPend; htPend=0; if(ch<1||ch>4) return;
    int base=htChDays[ch]; if(prTot>=140) base+=base/2;
    const char*w=htWard[rnd8()&3];
    lsp=0; lgrind=0; lstun=90; if(lhp>25) lhp-=15; else if(lhp>10) lhp=10;   // the beating
    moodEvent(M_HURT_BIG); sfxPlay(SFX_HIT);
    htCourtHead("COURT");
    char b[32]; int y=32;
    htLine(y,"CHARGE",htChNm[ch],GOLD); y+=11; htLine(y,"OFFICER",w,WHITE); y+=11; htLine(y,"JUDGE","HARDCASTLE",WHITE); y+=11;
    slNum(b,prCon); htLine(y,"CONVICTIONS",b,WHITE); y+=15;
    text(20,y,"THE WARDEN BEAT YOU AND DRAGGED",DIMC,1); y+=9; text(20,y,"YOU BEFORE THE JUDGE.",DIMC,1); y+=14;
    text(20,y,"THE ROOM IS WAITING FOR YOUR PLEA.",RGB(14,26,31),1);
    text(176,134,"A OK",DIMC,1);
    present(); prWait();
    const char*it[2]={"GUILTY  HALF THE DAYS","NOT GUILTY  TAKE YOUR CHANCE"};
    int c=menu("HOW DO YOU PLEAD",it,2);
    int days, dr; const char*v;
    if(c!=1){ days=base/2<1?1:base/2; dr=2; v="GUILTY"; }
    else {
        int p=25+skLvl(SK_LOGIC)*10+skLvl(SK_CHARM)*5+(htGang==2?10:0)+(htRep>=50?5:0)-(int)prCon*2-(htGrudge?10:0); if(p<10) p=10; if(p>80) p=80;
        if((int)(rnd8()*100/256)<p){ days=0; dr=-3; v="NOT GUILTY"; htGrudge=2; }
        else { days=base+base/2; dr=3; v="GUILTY  YOU WASTED THE COURT'S TIME"; }
    }
    htCourtHead("VERDICT");
    y=34; text(20,y,v,days?RGB(31,10,10):RGB(14,30,14),1); y+=16;
    if(days&&prDays!=PR_LIFE){ char*e=slNum(b,days); slCat(e,days==1?" DAY ADDED":" DAYS ADDED"); text(20,y,b,GOLD,1); }
    else if(days) text(20,y,"A LIFER LOSES NOTHING MORE",DIMC,1);
    else text(20,y,"THE WARDEN IS REPRIMANDED",GOLD,1);
    y+=14; { char*e=slCat(b,"REPUTATION "); if(dr<0){ *e++='-'; slNum(e,-dr); } else { *e++='+'; slNum(e,dr); } text(20,y,b,dr<0?RGB(31,10,10):RGB(14,30,14),1); y+=14; }
    if(days) text(20,y,"A CONVICTION GOES ON YOUR RECORD.",DIMC,1);
    else text(20,y,"THE CONS THINK YOU SOFT. THE WARDEN",DIMC,1), text(20,y+9,"WILL REMEMBER THIS.",DIMC,1);
    text(176,134,"A OK",DIMC,1);
    htRepAdd(dr); if(days){ htAddDays(days); if(prCon<60) prCon++; if(htGang==1) htGang=0; }
    htSave(); prSave();
    present(); prWait();
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
}
