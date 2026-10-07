// mysim.h - MY SIM (PAUSE > MY SIM): the whole Sim on one panel, in the style of The Sims 2's job, skills, needs and relationships panels.
//   L / R (or LEFT / RIGHT) change the tab, and the game remembers the last one:  SIM  WANTS  JOB  SKILLS  PEOPLE  STUFF  MORE
//   SIM     name, money, life stage (and the day of it), mood, aspiration and its meter, the eight needs, the five personality traits, the five
//           abilities, the zodiac sign and the DNA points.
//   WANTS   the aspiration meter, the lifetime want, the wants (a bar when a want has a number: cash, combos) and fears (A locks a want, SELECT opens the REWARD SHOP).
//   JOB     the title, track and level, the pay of a shift, the hours, the work days of the week (today is marked), how this shift is going (work done
//           against the quota), good and bad shifts in a row, and what the next promotion needs (the skill, a friend). A changes track.
//   SKILLS  the same bars as the old SKILLS screen, and what each skill does.
//   PEOPLE  how you feel about each Sim in the house and how they feel about you (daily on top, lifetime below), a heart for a steady partner.
//   STUFF   the REWARD SHOP (what each reward costs, which one you own, which one would do something now; A buys) and the creator's slider PACKS you own.
//   MORE    VIEW TRICKS, VIEW GOALS and the TIMED RUN.
// Nothing is saved and no new RAM is used but one byte (the tab you were on): it only reads what the game already keeps.
// Needs before it: story.h (stBack), skills.h (skRow), career.h, house.h (relBar, hhInvite), hud.h (hudNeedNm, hudLvlCol, hudMoodCol), sims.h (simBuy), main.c (slkUl, slkNm, slkCost).
static char* msHour(char*d,int h){ int h12=h%12; if(!h12) h12=12; d=simCatN(d,h12); return simCat(d,(h%24)<12?" AM":" PM"); }   // 17 -> "5 PM"
static void msTab(int x,int w,const char*nm,int on){
    if(on){ s2rr(x,19,w,13,GOLD); s2rr(x+1,20,w-2,11,RGB(6,18,10)); text(x+(w-tw(nm,1))/2,22,nm,WHITE,1); }
    else { s2rr(x,20,w,12,RGB(10,20,30)); s2grad(x+1,21,w-2,10,6,15,25,3,9,17); text(x+(w-tw(nm,1))/2,23,nm,RGB(17,24,29),1); }
}
static void msPanel(int y,int h){ s2rr(6,y,228,h,RGB(10,20,30)); s2rr(7,y+1,226,h-2,RGB(2,6,13)); s2grad(8,y+2,224,h-4,3,9,19,1,4,10); }
static void msCareer(void){
    const JobTr*t=jobT(); char b[56], *e; msPanel(35,110);
    if(!ojob()){
        text(14,42,stage<AG_TEEN?"TOO YOUNG FOR A JOB":stage>=AG_ELDER?"RETIRED":"JOBS ARE OFF",GOLD,1);
        text(14,56,stage<AG_TEEN?"KIDS GO TO SCHOOL ON WEEKDAYS":stage>=AG_ELDER?"ENJOY THE REST":"TURN THEM ON IN OPTIONS  GAMEPLAY",WHITE,1);
        return; }
    text(14,147,"A  CHANGE TRACK",RGB(12,14,16),1);
    text(14,39,jobTitle(),GOLD,2);
    e=simCat(b,t->nm); e=simCat(e,"  LEVEL "); e=simCatN(e,jobLvl+1); e=simCat(e," OF "); simCatN(e,t->top+1); text(14,58,b,RGB(17,29,31),1);
    e=b; *e++=(char)0xC2; *e++=(char)0xA7; e=simCatN(e,jobPayOf(jobTrack,jobLvl,jobBr)); simCat(e," A SHIFT"); text(14,71,b,RGB(14,30,14),1);
    e=msHour(b,t->from); e=simCat(e," TO "); msHour(e,t->to); text(14,82,b,WHITE,1);
    { static const char dl[7]={'M','T','W','T','F','S','S'}; int td=simDay%7;   // the week: bright = a work day, the bar under a letter = today
      for(int d=0;d<7;d++){ int x=14+d*12; char s[2]={dl[d],0}; text(x,93,s,d<t->days?RGB(17,29,31):RGB(10,13,16),1); if(d==td) rect(x,103,5,2,GOLD); } }
    text(124,39,"THIS SHIFT",GOLD,1);
    { int q=simQuota(), p=shiftPts, w=q>0?p*98/q:0; if(w>98) w=98; rect(123,49,100,7,RGB(14,26,31)); rect(124,50,98,5,RGB(3,5,9)); if(w>0) rect(124,50,w,5,p>=q?RGB(8,26,8):GOLD);
      e=simCatN(b,p); e=simCat(e," OF "); e=simCatN(e,q); simCat(e,jobTrack==JT_SKATE?" POINTS":" MINUTES"); text(124,59,b,WHITE,1); }
    { const char*st; u16 sc=RGB(17,24,29);
      if(simInShift()){ st="ON THE CLOCK"; sc=RGB(10,28,12); }
      else if(!simJobDay()) st="DAY OFF TODAY";
      else if(simMin<simJobFrom()){ e=simCat(b,"STARTS AT "); msHour(e,t->from); st=b; }
      else st="SHIFT OVER";
      text(124,70,st,sc,1); }
    text(124,82,"GOOD",RGB(17,24,29),1); for(int i=0;i<t->good;i++) rect(152+i*9,84,7,5,i<jobGood?RGB(8,26,8):RGB(7,14,22));
    text(124,93,"BAD",RGB(17,24,29),1);  for(int i=0;i<t->bad;i++)  rect(152+i*9,95,7,5,i<jobBad?RGB(28,8,6):RGB(7,14,22));
    rect(12,108,216,1,RGB(14,26,31));
    if(jobLvl>=t->top){ text(14,113,"TOP OF THE TRACK",GOLD,1); text(14,124,"NOTHING LEFT TO CLIMB",RGB(17,24,29),1); return; }
    e=simCat(b,"NEXT  "); simCat(e,jobTitleOf(jobTrack,jobLvl+1,jobBr)); text(14,112,b,GOLD,1);
    { int need=jobNeedSkill(jobLvl), have=jobSkLvl(jobTrack);
      if(need){ e=simCat(b,jobSkName(jobTrack)); e=simCat(e,"  LEVEL "); e=simCatN(e,have); e=simCat(e," OF "); simCatN(e,need); }
      else simCat(b,"NO SKILL NEEDED");
      text(14,123,b,have>=need?RGB(10,28,12):RGB(30,12,8),1); }
    if(jobNeedFriend(jobLvl)){ int f=jobFriends(); text(14,133,f>0?"A FRIEND IN THE HOUSE  YES":"A FRIEND IN THE HOUSE  NOT YET",f>0?RGB(10,28,12):RGB(30,12,8),1); }
    else text(14,133,"NO FRIEND NEEDED YET",RGB(17,24,29),1);
}
static void msSkills(int sel){
    static const u8 ord[SK_N+1]={SK_COOK,SK_LOGIC,SK_BODY,SK_CHARM,SK_CREAT,SK_N,SK_GRIND,SK_AIR,SK_BAL};   // top to bottom; SK_N stands for SKATING
    text(10,34,"LIFE SKILLS",GOLD,1); text(10,89,"SKATER SKILLS",GOLD,1);
    for(int r=0;r<=SK_N;r++){ int id=ord[r], y=r<SK_LIFE?44+r*9:99+(r-SK_LIFE)*9;
        if(id==SK_N) skRow(y,r==sel,"SKATING",skillPts,skillLvl,simSkillAt); else skRow(y,r==sel,skNm[id],skPts[id],skLvl(id),skAt); }
    rect(8,135,224,1,RGB(14,26,31)); text(10,138,skFx[ord[sel]],WHITE,1); text(10,147,skHow[ord[sel]],RGB(17,29,31),1);
}
// ---- the SIM tab: who you are right now ----
static void msStat(int x,int y,const char*nm,int v,u16 pip){   // a label and ten pips, like the ability bars of the creator
    x=text(x,y,nm,RGB(20,26,30),1)+3; for(int q=0;q<10;q++) rect(x+q*2,y+1,1,4,q<v?pip:RGB(4,6,12));
}
static void msNeed(int x,int y,const char*nm,int v){   // a need: the HUD's name and colours, a longer bar
    int w=58, f; u16 c=hudLvlCol(v); if(v<0) v=0; if(v>100) v=100; f=v*(w-2)/100;
    text(x,y,nm,RGB(20,26,30),1); rect(x+34,y+1,w,6,RGB(10,20,30)); rect(x+35,y+2,w-2,4,RGB(3,5,9)); if(f>0) rect(x+35,y+2,f,4,c);
}
static void msSim(void){
    char b[40], *e; int st=moodState(); msPanel(35,121);
    e=simCat(b,hhPName); if(hhPLast[0]){ *e++=' '; simCat(e,hhPLast); } text(14,39,b,GOLD,1);
    e=b; *e++=(char)0xC2; *e++=(char)0xA7; simCatN(e,simMoney); text(228-tw(b,1),39,b,RGB(14,30,14),1);
    e=simCat(b,stageNm[stage]);
    if(stage<AG_ELDER&&xo[XO_AGING]&&oStageDays(stage)){ static const u8 pct[4]={0,200,100,50}; int need=oStageDays(stage)*pct[xo[XO_AGING]]/100; if(need<1) need=1;
        e=simCat(e,"  DAY "); e=simCatN(e,ageDays+1); e=simCat(e," OF "); simCatN(e,need); }
    text(14,49,b,RGB(17,29,31),1); text(228-tw(moodStName[st],1),49,moodStName[st],hudMoodCol(st),1);
    if(simWishes()){ int a=aspNow(); const char*z=simZoneNm[simZone]; simIcon(14,59,simAspIcon[a],GOLD); text(24,59,aspNm[a],GOLD,1); text(228-tw(z,1),59,z,simZoneCol(simZone),1); simMeterBar(14,69,214,5); }
    else text(14,59,"BABIES HAVE NO WANTS",DIMC,1);
    rect(12,77,216,1,RGB(14,26,31));
    { int v[8]={lfood,sNrg,sHyg,sCom,100-lbl,moodFunPct(),sRoom,sSoc};
      for(int i=0;i<8;i++) msNeed(i<4?14:122,80+(i&3)*9,hudNeedNm[i],v[i]); }
    rect(12,117,216,1,RGB(14,26,31));
    for(int i=0;i<TR_N;i++) msStat(14+(i%3)*74,120+(i/3)*9,trNm[i],trOf(i),GOLD);
    for(int i=0;i<AB_N;i++) msStat(14+(i%3)*74,138+(i/3)*9,abNm[i],abOf10(i),RGB(12,30,24));
    text(162,129,signNm[signOf()],RGB(20,22,30),1);
    { int x=text(162,147,"DNA",DIMC,1)+3; numStr(b,pDna); text(x,147,b,RGB(12,30,24),1); }
}
// ---- the WANTS tab: the aspiration panel, on a tab ----
static void msWants(int cur){
    char b[24]; msPanel(35,121);
    if(!simWishes()){ text(14,40,"BABIES HAVE NO WANTS",DIMC,1); text(14,52,"GROW UP TO WISH FOR THINGS",RGB(17,24,29),1); return; }
    { int a=aspNow(); const char*z=simZoneNm[simZone]; simIcon(14,39,simAspIcon[a],GOLD); text(24,39,aspNm[a],GOLD,1); text(228-tw(z,1),39,z,simZoneCol(simZone),1); simMeterBar(14,49,214,5); }
    { const SimLtw*L=simLtw(); const char*s; text(14,58,"LIFETIME",DIMC,1); text(60,58,L->name,stage<AG_TEEN?DIMC:WHITE,1);
      if(simFlags&SF_LTW) s="MET"; else if(stage<AG_TEEN) s="AS A TEEN"; else { int v=simLtwVal(), g=L->goal; char*e=b; e+=numStr(e,v>g?g:v); *e++='/'; numStr(e,g); s=b; }
      text(228-tw(s,1),58,s,(simFlags&SF_LTW)?RGB(24,30,31):GOLD,1); }
    for(int s=0;s<SIM_WS;s++){ int y=69+s*10, on=simW[s]>=0, lk=simLock>>s&1, f=s==cur;
        if(f){ rect(8,y-2,224,11,RGB(6,16,8)); rect(8,y-2,2,11,GOLD); }
        simCell(14,y-1,on?simWants[simW[s]].icon:0,0,lk,on);
        if(on){ char p[8]; p[0]='+'; numStr(p+1,simWants[simW[s]].pts); int x=228-tw(p,1); text(28,y,simWantName(s),f?WHITE:RGB(22,28,22),1); text(x,y,p,RGB(12,30,12),1);
            { int c, g; if(simWantProg(s,&c,&g)){ int w=c*44/g; rect(114,y+2,44,4,RGB(3,5,9)); if(w>0) rect(114,y+2,w,4,c>=g?GOLD:RGB(10,28,12)); } }   // a want with a number: how far along
            if(lk) text(x-6-tw("LOCKED",1),y,"LOCKED",GOLD,1); }
        else text(28,y,"...",DIMC,1); }
    for(int s=0;s<SIM_FS;s++){ int y=111+s*10, on=simF[s]>=0;
        simCell(14,y-1,on?simFears[simF[s]].icon:0,1,0,on);
        if(on){ char p[8]; p[0]='-'; numStr(p+1,simFears[simF[s]].pts); text(28,y,simFearName(s),RGB(30,18,16),1); text(228-tw(p,1),y,p,RGB(30,10,8),1); }
        else text(28,y,"...",DIMC,1); }
    text(14,145,"A LOCK  SELECT SHOP",RGB(12,14,16),1);
    { int x; numStr(b,simAsp); x=228-tw(b,1); text(x,145,b,GOLD,1); text(x-3-tw("POINTS",1),145,"POINTS",DIMC,1); }
}
// ---- the STUFF tab: the reward shop and the packs you own ----
static const char* msRew(int r,char*nb,u16*col,int*ok){   // what a reward is worth to you right now (the same checks as simBuy in sims.h)
    char*e; *ok=0; *col=RGB(26,16,10);
    if(r==RW_TREE&&(simFlags&SF_TREE)){ e=simCat(nb,"OWNED  +"); e=simCatN(e,SIM_TREE_PAY); simCat(e," A DAY"); *col=RGB(10,28,12); return nb; }
    if(simAsp<simRewCost[r]){ e=simCat(nb,"NEED "); simCatN(e,simRewCost[r]-simAsp); return nb; }
    *col=RGB(17,24,29);
    if(r==RW_ENERGIZER&&sNrg>=95) return "NOT TIRED";
    if(r==RW_CAP&&skillLvl>=5) return "SKILL MAXED";
    if(r==RW_ELIXIR){ if(stage>=AG_ELDER||!xo[XO_AGING]||!oStageDays(stage)) return "NOT AGING"; if(!ageDays) return "ALREADY YOUNG"; }
    *ok=1; *col=RGB(10,28,12); return "READY";
}
static void msStuff(int cur){
    static char nb[24] EWRAM_BSS; char b[16]; msPanel(35,121);
    text(14,39,"REWARD SHOP",GOLD,1); { int x; numStr(b,simAsp); x=228-tw(b,1); text(x,39,b,GOLD,1); text(x-3-tw("POINTS",1),39,"POINTS",DIMC,1); }
    if(!simWishes()){ text(14,52,"BABIES HAVE NO REWARD POINTS",DIMC,1); }
    else for(int r=0;r<RW_N;r++){ int y=49+r*11, ok; u16 c; const char*st=msRew(r,nb,&c,&ok);
        if(r==cur){ rect(8,y-2,224,11,RGB(6,16,8)); rect(8,y-2,2,11,GOLD); }
        text(14,y,simRewNm[r],r==cur?WHITE:RGB(22,28,22),1); numStr(b,simRewCost[r]); text(100,y,b,RGB(17,24,29),1); text(228-tw(st,1),y,st,c,1); }
    rect(12,95,216,1,RGB(14,26,31));
    text(14,99,"CREATOR PACKS",GOLD,1); { int x; numStr(b,pDna); x=228-tw(b,1); text(x,99,b,RGB(12,30,24),1); text(x-3-tw("DNA",1),99,"DNA",DIMC,1); }
    for(int i=0;i<NSLK;i++){ int x=i<3?14:122, y=110+(i%3)*9, own=sUnlock||(slkUl>>i&1);
        text(x,y,slkNm[i],own?RGB(22,28,22):RGB(17,24,29),1);
        if(own) text(x+98-tw("OWNED",1),y,"OWNED",RGB(10,28,12),1); else { numStr(b,slkCost[i]); text(x+98-tw(b,1),y,b,pDna>=slkCost[i]?RGB(22,25,28):RGB(26,16,10),1); } }
    text(14,147,"A BUY  PACKS COME FROM THE CREATOR",RGB(12,14,16),1);
}
static void msPeople(int top){
    text(90,34,"YOU TO THEM",DIMC,1); text(158,34,"THEM TO YOU",DIMC,1);
    if(!hhN){ text(10,50,"NO ONE ELSE LIVES HERE",WHITE,1); text(10,62,"INVITE SOMEONE OVER ON THE PHONE",DIMC,1); return; }
    for(int m=top;m<hhN&&m<top+6;m++){ int y=44+(m-top)*16, b=hhM[m].uid, a=hhPUid;
        text(10,y,hhM[m].name,WHITE,1);
        { char q[40]; const char*w=relWord(a,b); int kr=kin[b][a];
          if(kr){ char*e=simCat(q,kinNm[kr]); e=simCat(e,"  "); simCat(e,w); if(tw(q,1)<=76) w=q; else w=kinNm[kr]; }
          text(10,y+8,w,(relF[a][b]&(RF_LOVE|RF_STEADY|RF_CRUSH))?RGB(31,14,20):relD[a][b]<=-20?RGB(30,10,8):RGB(16,26,16),1); }
        relBar(90,y+1,relD[a][b]); relBar(90,y+8,relL[a][b]); relBar(158,y+1,relD[b][a]); relBar(158,y+8,relL[b][a]);
        if(relF[a][b]&RF_STEADY) simIcon(224,y+2,IC_HEART,RGB(31,14,20)); }
    text(10,147,hhN>6?"TOP DAILY  LOW LIFETIME  UP DOWN MORE":dbgOn?"TOP DAILY  LOW LIFETIME  SELECT ADD":"TOP BAR DAILY  LOW BAR LIFETIME",RGB(12,14,16),1);
}
#define MY_MORE 4
static void mySimScreen(void){
    static const char* const tn[7]={"SIM","WANTS","JOB","SKILLS","PEOPLE","STUFF","MORE"};
    static u8 keep EWRAM_BSS;   // the tab you were on last time
    slkLoad();   // (the packs you own: a byte of the save chip)
    int tab=keep<7?keep:0, sel=0, top=0, ms=0, dirty=1; u16 prev=keyNow(); u32 cnt=0, lt=~0u;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)) return;
        if(pr&(K_R|K_RIGHT)){ tab=(tab+1)%7; sel=top=ms=0; dirty=1; keep=(u8)tab; }
        if(pr&(K_L|K_LEFT)){ tab=(tab+6)%7; sel=top=ms=0; dirty=1; keep=(u8)tab; }
        if(tab==1){ if(simWishes()){ if(pr&K_DOWN){ sel=(sel+1)%SIM_WS; dirty=1; } if(pr&K_UP){ sel=(sel+SIM_WS-1)%SIM_WS; dirty=1; }
                if((pr&K_A)&&simW[sel]>=0){ simLock=(simLock>>sel&1)?0:(1<<sel); dirty=1; }   // one lock: locking another want moves it
                if(pr&K_SEL){ aspRewards(); prev=keyNow(); dirty=1; } } }
        if(tab==2&&(pr&K_A)&&ojob()){ careerScreen(); prev=keyNow(); dirty=1; }
        if(tab==3){ if(pr&K_DOWN){ sel=(sel+1)%(SK_N+1); dirty=1; } if(pr&K_UP){ sel=(sel+SK_N)%(SK_N+1); dirty=1; } }
        if(tab==4){ if((pr&K_DOWN)&&top+6<hhN){ top++; dirty=1; } if((pr&K_UP)&&top>0){ top--; dirty=1; }
            if((pr&K_SEL)&&dbgOn){ hhInvite(); prev=keyNow(); if(top+6>hhN) top=hhN>6?hhN-6:0; dirty=1; } }
        if(tab==5){ if(pr&K_DOWN){ sel=(sel+1)%RW_N; dirty=1; } if(pr&K_UP){ sel=(sel+RW_N-1)%RW_N; dirty=1; }
            if((pr&K_A)&&simWishes()){ char nb[24]; u16 c; int ok; msRew(sel,nb,&c,&ok);
                if(ok){ static char q[32] EWRAM_BSS; static const char* const yn[2]={"YES  BUY IT","NO"}; char*e=simCat(q,simRewNm[sel]); e=simCat(e,"  "); e=simCatN(e,simRewCost[sel]); simCat(e," POINTS?");
                    if(menu(q,yn,2)==0) toast(simBuy(sel)); }
                else toast(simBuy(sel));
                prev=keyNow(); dirty=1; } }
        if(tab==6){ if(pr&K_DOWN){ ms=(ms+1)%MY_MORE; dirty=1; } if(pr&K_UP){ ms=(ms+MY_MORE-1)%MY_MORE; dirty=1; }
            if(pr&K_A){ if(ms==0) tricksScreen(); else if(ms==1) goalsScreen(); else if(ms==2) trnPick(); else statsScreen();
                prev=keyNow(); dirty=1; } }
        if(!dirty&&(cnt>>3)==lt){ vsync(); continue; }   // idle: the picture on the screen is still right
        dirty=0; lt=cnt>>3;
        stBack("MY SIM",(int)cnt);
        { const char*h="L R TAB  B BACK"; text(236-tw(h,1),7,h,RGB(17,29,31),1); }
        { int x=6; for(int i=0;i<7;i++){ int w=tw(tn[i],1)+7; msTab(x,w,tn[i],i==tab); x+=w+2; } }
        if(tab==0) msSim();
        else if(tab==1) msWants(sel);
        else if(tab==2) msCareer();
        else if(tab==3){ msPanel(35,121); msSkills(sel); }
        else if(tab==4){ msPanel(35,121); msPeople(top); }
        else if(tab==5) msStuff(sel);
        else { static const char* const ds[MY_MORE]={"THE SKATE CONTROLS ON ONE PAGE","THE GOALS OF THIS LOT AND THE TOWN","A 2 MINUTE TRICK SCORE ATTACK","HOURS PLAYED, LIFETIME SCORE AND MORE"};
            msPanel(35,121); text(10,40,"MORE ABOUT YOU",GOLD,1);
            for(int i=0;i<MY_MORE;i++){ const char*nm=i==0?"VIEW TRICKS":i==1?"VIEW GOALS":i==2?trnLabel():"LIFETIME STATS"; int y=52+i*15, on=i==ms;
                s2rr(10,y,220,13,on?GOLD:RGB(10,20,30)); s2rr(11,y+1,218,11,on?RGB(6,18,10):RGB(4,9,18)); text(16,y+3,nm,on?WHITE:RGB(20,26,30),1); }
            rect(8,131,224,1,RGB(14,26,31)); text(10,136,ds[ms],RGB(17,29,31),1); text(10,147,"A OPEN",RGB(12,14,16),1); }
        present();
    }
}
