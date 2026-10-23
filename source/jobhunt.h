// jobhunt.h - THE JOB HUNT. A new life starts UNEMPLOYED (jobNone, saved in bit 7 of the strike byte: an older save has a 0 there and keeps its job).
// While you have no job there are no shifts, no pay and no bills. Two ways to get one:
//   THE NEWSPAPER  the paperboy (or paper girl) drops it every morning at 6:00. YOUR ACTIONS > MIND > READ THE NEWSPAPER (at home): the front page, then
//                  the CLASSIFIEDS: three jobs, a different three each day.
//   THE COMPUTER   the catalog COMPUTER (not the desk): R on it opens the JOB BOARD with every job you are old enough for, any time of day.
// A hire starts you at the bottom of the track (an employed Sim who switches keeps part of the level, see career.h). The phone's CAREER still shows
// the tracks, but only to look: you have to be hired from the paper or the computer. Needs career.h (jobAllowed, jobStartLvl).
static const char* const jbNews[12]={
    "SKATE PARK OPENS  TOWN DIVIDED","LOCAL BORE LANDS A KICKFLIP","PIZZA PRICES UP AGAIN","MYSTERY LIGHTS OVER THE POOL",
    "COUNCIL BANS FUN  FUN OBJECTS","RAMP BUILDER WINS AWARD","LOST CAT FOUND IN A TREE","BOARD SALE AT THE SKATE SHOP",
    "NEW CAFE HAS A LONG LINE","HOT WEEK AHEAD SAYS EXPERT","SLOW NEWS DAY  NOTHING HAPPENS","NEIGHBORS COMPLAIN ABOUT NOISE" };
static char jbLb[JT_N][40] EWRAM_BSS;
static const char* jbIt[JT_N];
static void jbLine(char*b,int t){ char*e=simCat(b,jobTr[t].nm); e=simCat(e,": "); e=simCat(e,jobTitleOf(t,0,0)); e=simCat(e,"  \xC2\xA7"); simCatN(e,jobPayOf(t,0,0)); }
static int jbHire(int t){   // asks first; 1 = hired
    static char q[40] EWRAM_BSS, m[44] EWRAM_BSS; static const char* const yn[2]={"YES  TAKE THE JOB","NO"};
    char*e=simCat(q,jobNone?"APPLY  ":"SWITCH TO "); simCat(e,jobTr[t].nm);
    if(menu(q,yn,2)!=0) return 0;
    int nlv=jobNone?0:jobStartLvl(t); jobNone=0; jobTrack=t; jobLvl=nlv; jobBr=0; jobChosen=0; jobGood=jobBad=0; shiftPts=0; simsSave();
    e=simCat(m,"HIRED  "); simCat(e,jobTitle()); toast(m); return 1;
}
static void jbBoard(const char*title,const int*tr,int n){   // a list of jobs: pick one to apply
    for(;;){ for(int i=0;i<n;i++){ jbLine(jbLb[i],tr[i]); jbIt[i]=jbLb[i]; }
        int c=menu(title,jbIt,n); if(c<0) return; if(jbHire(tr[c])) return; }
}
static int jbAllowedList(int*al){ int n=0; for(int t=0;t<JT_N;t++) if(jobAllowed(t)) al[n++]=t; return n; }
static void jbPaper(void){   // the newspaper: a headline, then three jobs
    int al[JT_N], n=jbAllowedList(al);
    if(!ojobOn()||n<=0){ toast("NOTHING IN THE PAPER FOR YOU"); return; }
    int d=simDay-(simMin<360?1:0); if(d<0) d=0;   // before 6:00 it is still yesterday's paper
    toast(jbNews[d%12]);
    int ad[3], na=0, a=(d*7+d/5)%n;
    for(int k=0;k<3;k++){ int t=al[(a+k*n/3)%n], dup=0; for(int j=0;j<na;j++) if(ad[j]==t) dup=1; if(!dup) ad[na++]=t; }
    jbBoard(simMin<360?"YESTERDAYS CLASSIFIEDS":"TODAYS CLASSIFIEDS",ad,na);
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
}
static int jbPaperOk(void){ return ojobOn()&&!prIn()&&!(nbOk&&nbT.cur!=nbT.home); }   // YOUR ACTIONS shows the paper at home
static int jbIsComp(int id){ const char*a=catTab[id-1].nm; const char*b="COMPUTER"; while(*a&&*a==*b){ a++; b++; } return !*a&&!*b; }   // "COMPUTER" itself, not the COMPUTER DESK
static int jbCompNear(void){ int tx=(int)(lfx>>8), ty=(int)(lfy>>8);
    for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=tx+dx, y=ty+dy; if(x<0||y<0||x>=MW||y>=MH||lifeMap[y][x]!='d') continue; int id=catIdAt(x,y); if(id&&jbIsComp(id)) return 1; }
    return 0; }
static int jbUse(void){   // R next to a COMPUTER. 1 = handled
    if(!ojobOn()||!jbCompNear()) return 0;
    const char* it[2]={"JOB BOARD  ALL JOBS","VIEW MY CAREER"};
    int c=menu("COMPUTER",it,2);
    if(c==0){ int al[JT_N], n=jbAllowedList(al); if(n>0) jbBoard("JOB BOARD",al,n); }
    else if(c==1) careerScreen();
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    return 1;
}
static const char* jbHint(void){ return (ojobOn()&&jbCompNear())?"R USE THE COMPUTER":0; }   // hud.h
static void jbDeliver(void){   // simMinute: 6:00, the paper lands on the step
    if(!ojobOn()||simQ) return;
    if(jobNone) simQueue((simDay&1)?"THE PAPERGIRL CAME  CHECK THE ADS":"THE PAPERBOY CAME  CHECK THE ADS");
    else simQueue((simDay&1)?"THE PAPERGIRL CAME":"THE PAPERBOY CAME");
}
