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
static void hmNeed(int*v,int d);   // skills.h
static u16 jbCDay; static u8 jbCN[6];   // what you did at the computer today (each has a daily limit, so it is not a free skill farm)
static char jbCM[36] EWRAM_BSS;
static int jbCLimit(int k,int lim){ if(jbCDay!=(u16)(simDay+1)){ jbCDay=(u16)(simDay+1); for(int i=0;i<6;i++) jbCN[i]=0; } if(jbCN[k]>=lim){ lnote="ENOUGH FOR TODAY"; lnoteT=50; return 1; } jbCN[k]++; return 0; }
static void jbComp(int k){   // the computer's things to do
    lsp=0; lgrind=0;
    if(sNrg<10&&k!=0){ lnote="TOO TIRED"; lnoteT=50; return; }
    if(k==0){ if(jbCLimit(0,6)) return; lstun=90; hmNeed(&sCom,10); hmNeed(&sNrg,-5); moodEvent(M_SOFA); simEvent(SE_LAUGH); sfxPlay(SFX_TV); lnote="PLAYED A GAME  FUN"; lnoteT=60; if(simMin>=SIM_NIGHT_FROM||simMin<SIM_NIGHT_TO) hmNeed(&sNrg,-6); }
    else if(k==1){ if(jbCLimit(1,3)) return; lstun=100; hmNeed(&sCom,5); skGain(SK_LOGIC,1);
        if((rnd8()&7)==0){ lstun=260; skGain(SK_LOGIC,2); hmNeed(&sNrg,-6); lnote="YOU FELL DOWN A WIKI HOLE"; } else lnote="SURFED THE WEB"; lnoteT=70; sfxPlay(SFX_PAGE); }
    else if(k==2){ if(jbCLimit(2,3)) return; lstun=140; hmNeed(&sNrg,-4); skGain(SK_LOGIC,2); simEvent(SE_READ); sfxPlay(SFX_PAGE); lnote="STUDIED ONLINE  LOGIC UP"; lnoteT=60; }
    else if(k==3){ if(jbCLimit(3,3)) return; lstun=140; hmNeed(&sNrg,-4); hmNeed(&sCom,4); skGain(SK_CREAT,2); moodEvent(M_SOFA); sfxPlay(SFX_POP); lnote="MADE DIGITAL ART  CREATIVITY UP"; lnoteT=70; }
    else if(k==4){ if(jbCLimit(4,3)) return; lstun=100; hmNeed(&sSoc,12); skGain(SK_CHARM,1); hmNeed(&sCom,4); moodEvent(M_SOFA); sfxPlay(SFX_POP); lnote="CHATTED ONLINE  SOCIAL UP"; lnoteT=60; }
    else if(k==5){ if(sNrg<25){ lnote="TOO TIRED FOR A GIG"; lnoteT=50; return; } if(jbCLimit(5,2)) return;
        int pay=35+(skLvl(SK_LOGIC)+skLvl(SK_CREAT))*6; lstun=160; hmNeed(&sNrg,-15); simMoneyAdd(pay); moodEvent(M_PAY); sfxPlay(SFX_POP);
        char*e=simCat(jbCM,"FREELANCE GIG  +\xC2\xA7"); simCatN(e,pay); lnote=jbCM; lnoteT=70; }
}
static int jbUse(void){   // R next to a COMPUTER. 1 = handled
    if(!ojobOn()&&stage<AG_CHILD) return 0;
    if(!jbCompNear()) return 0;
    static char tt[32] EWRAM_BSS; char*e=simCat(tt,"COMPUTER  \xC2\xA7"); simCatMoney(e,simMoney,1);
    const char* it[8]; int id[8], n=0;
    if(ojobOn()){ it[n]="JOB BOARD  ALL JOBS"; id[n++]=10; }
    it[n]="PLAY A GAME"; id[n++]=0; it[n]="SURF THE WEB"; id[n++]=1; it[n]="STUDY ONLINE"; id[n++]=2; it[n]="MAKE DIGITAL ART"; id[n++]=3; it[n]="CHAT ONLINE"; id[n++]=4;
    if(stage>=AG_TEEN){ it[n]="FREELANCE GIG  PAYS"; id[n++]=5; }
    if(ojobOn()){ it[n]="VIEW MY CAREER"; id[n++]=11; }
    int c=menu(tt,it,n);
    liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]==10){ int al[JT_N], na=jbAllowedList(al); if(na>0) jbBoard("JOB BOARD",al,na); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync(); }
    else if(id[c]==11){ careerScreen(); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync(); }
    else jbComp(id[c]);
    return 1;
}
static const char* jbHint(void){ return (ojobOn()&&jbCompNear())?"R USE THE COMPUTER":0; }   // hud.h
// ---- THE COMMUTE: every job has a workplace lot and the shift only counts while you are there ----
static const u8 jbWk[JT_N]={CT_CINEMA,CT_PARK,CT_GYM,CT_BANK,CT_CASINO,CT_ARMS,CT_LIBRARY,CT_TACO,CT_SKATE};   // the workplace of each track (enum order: ENT SLACK ATH BUS CRIM MIL EDU FOOD SKATE)
static int jbWorkOk(void){ return !nbOk||prIn()||trvHere(jbWk[jobTrack]); }   // no town: work anywhere, as before
static void jbVan(void){   // the CARPOOL: a very cheap van, and very large for what it is: seven passengers and you
    static const u16 hc[7]={RGB(26,14,10),RGB(10,22,12),RGB(10,14,28),RGB(28,24,8),RGB(24,10,24),RGB(8,24,24),RGB(28,16,8)};
    u16 pk=keyNow(); sfxPlay(SFX_CASH);
    for(int f=0;f<150;f++){
        u16 k=keyNow(); if(k&~pk&(K_A|K_B|K_START)) break; pk=k;
        int x; if(f<50){ int r=50-f; x=40-r*r*210/2500; } else if(f<90) x=40; else { int r=f-90; x=40+r*r*220/3600; }
        rect(0,52,SW,56,RGB(13,22,30)); rect(0,108,SW,34,RGB(5,5,7)); rect(0,108,SW,2,RGB(14,14,16));
        for(int d=-24;d<SW;d+=24) rect(d+24-((f*4)%24),124,12,2,RGB(24,24,10));
        if(f>=45&&f<80){ int px=14+(f-45>25?25:f-45)*26/25; rect(px,94,8,12,RGB(8,26,14)); rect(px+1,96,2,2,RGB(2,2,2)); rect(px+5,96,2,2,RGB(2,2,2)); }   // you walk up to the door
        s2rr(x,78,160,28,RGB(28,24,4)); s2rr(x+6,64,118,18,RGB(28,24,4)); rect(x,92,160,3,RGB(26,6,6));
        for(int i=0;i<7;i++){ rect(x+12+i*16,68,14,11,RGB(8,14,22)); rect(x+15+i*16,71,8,8,hc[i]); rect(x+16+i*16,73,1,2,RGB(2,2,2)); rect(x+21+i*16,73,1,2,RGB(2,2,2)); }
        rect(x+126,70,30,12,RGB(8,14,22)); if(f>=80){ rect(x+132,72,8,8,RGB(8,26,14)); rect(x+133,74,1,2,RGB(2,2,2)); rect(x+138,74,1,2,RGB(2,2,2)); }
        rect(x+156,92,4,5,RGB(31,31,18)); rect(x+3,103,154,4,RGB(14,14,16));
        rect(x+20,100,18,14,RGB(3,3,4)); rect(x+25,105,8,5,RGB(14,14,16)); rect(x+112,100,18,14,RGB(3,3,4)); rect(x+117,105,8,5,RGB(14,14,16));
        if(x>=2&&x+110<=SW) text(x+28,83,"CARPOOL",RGB(3,3,3),1);
        if(f<50||f>=90) rect(x-6-(f&7),100-(f&3),5,4,RGB(20,20,22));
        present(); }
    sfxPlay(SFX_POP);
}
static void jbGoTrip(int lot,int mode){   // mode 0: on your own bore (free, tires you), 1: the carpool (5, social)
    if(mode==1&&simMoney<5){ toast("NOT ENOUGH FOR THE FARE"); return; }
    if(mode==0&&sNrg<15){ toast("TOO TIRED TO GET THERE"); return; }
    if(!trvGo(lot)) return;
    if(mode==1){ simMoneyAdd(-5); jbVan(); hmNeed(&sSoc,8); moodEvent(M_SOFA); toast("CARPOOL  PACKED BUT CHEAP"); }
    else { hmNeed(&sNrg,lhave?-6:-12); toast(lhave?"RODE YOUR BOARD THERE":"WALKED THERE"); }
}
static void jbCommute(void){   // the phone and YOUR ACTIONS: to work, or home
    if(!ojob()||!nbOk||prIn()){ toast("NO COMMUTE RIGHT NOW"); return; }
    int wct=jbWk[jobTrack], at=jbWorkOk(), n=0, id[2]; const char* it[2]; static char a[32] EWRAM_BSS;
    if(!at){ char*e=simCat(a,"GO TO WORK  "); simCat(e,ctNm[wct]); it[n]=a; id[n++]=0; }
    if(nbT.home<NB_LOTS&&nbT.home!=nbT.cur){ it[n]="GO HOME"; id[n++]=1; }
    if(!n){ toast("YOU ARE AT WORK  KEEP AT IT"); return; }
    int c=menu("COMMUTE",it,n); if(c<0) return;
    int lot=id[c]==0?trvPlace(wct):nbT.home; if(lot<0){ toast("NO WORKPLACE IN TOWN"); return; }
    static const char* const md[2]={"ON MY OWN BORE  FREE","CARPOOL  \xC2\xA7" "5  BIG VAN"};
    int m=menu("HOW TO GET THERE",md,2); if(m<0) return;
    jbGoTrip(lot,m);
}
static void jbDeliver(void){   // simMinute: 6:00, the paper lands on the step
    if(!ojobOn()||simQ) return;
    if(jobNone) simQueue((simDay&1)?"THE PAPERGIRL CAME  CHECK THE ADS":"THE PAPERBOY CAME  CHECK THE ADS");
    else simQueue((simDay&1)?"THE PAPERGIRL CAME":"THE PAPERBOY CAME");
}
// JBEND
