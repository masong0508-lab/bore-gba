// mysim.h - MY SIM (PAUSE > MY SIM): the job, the skills and the people on one panel, in the style of The Sims 2's job and skills panels.
//   L / R (or LEFT / RIGHT) change the tab:  CAREER   SKILLS   PEOPLE   MORE      UP / DOWN pick a skill, scroll the people, pick a MORE entry
//   CAREER  the title, track and level, the pay of a shift, the hours, the work days of the week (today is marked), how this shift is going (work done
//           against the quota), good and bad shifts in a row, and what the next promotion needs (the skill, a friend).
//   SKILLS  the same bars as the old SKILLS screen, and what each skill does.
//   PEOPLE  how you feel about each Sim in the house and how they feel about you (daily on top, lifetime below), as in the RELATIONSHIPS screen.
//   MORE    the screens that used to be in PAUSE > WANTS: WANTS FEARS REWARDS, VIEW TRICKS, VIEW GOALS, TIMED RUN, and the CAREER screen (change track).
// Nothing is saved and no new RAM is used: it only reads what the game already keeps. Needs before it: story.h (stBack), skills.h (skRow), career.h, house.h (relBar).
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
    if(jobLvl>=t->top){ text(14,113,"TOP OF THE TRACK",GOLD,1); text(14,124,"MORE TAB  CAREER  TO CHANGE TRACK",RGB(17,24,29),1); return; }
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
static void msPeople(int top){
    text(96,34,"YOU TO THEM",DIMC,1); text(166,34,"THEM TO YOU",DIMC,1);
    if(!hhN){ text(10,50,"NO ONE ELSE LIVES HERE",WHITE,1); text(10,62,"INVITE SOMEONE OVER ON THE PHONE",DIMC,1); return; }
    for(int m=top;m<hhN&&m<top+6;m++){ int y=44+(m-top)*16, b=hhM[m].uid, a=hhPUid;
        text(10,y,hhM[m].name,WHITE,1);
        { char q[40]; const char*w=relWord(a,b); int kr=kin[b][a];
          if(kr){ char*e=simCat(q,kinNm[kr]); e=simCat(e,"  "); simCat(e,w); if(tw(q,1)<=82) w=q; else w=kinNm[kr]; }
          text(10,y+8,w,(relF[a][b]&(RF_LOVE|RF_STEADY|RF_CRUSH))?RGB(31,14,20):relD[a][b]<=-20?RGB(30,10,8):RGB(16,26,16),1); }
        relBar(96,y+1,relD[a][b]); relBar(96,y+8,relL[a][b]); relBar(166,y+1,relD[b][a]); relBar(166,y+8,relL[b][a]); }
    text(10,147,hhN>6?"TOP DAILY  LOW LIFETIME  UP DOWN MORE":"TOP BAR DAILY  LOW BAR LIFETIME",RGB(12,14,16),1);
}
#define MS_MORE 5
static void mySimScreen(void){
    static const char* const tn[4]={"CAREER","SKILLS","PEOPLE","MORE"};
    int tab=0, sel=0, top=0, ms=0, dirty=1; u16 prev=keyNow(); u32 cnt=0, lt=~0u;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)) return;
        if(pr&(K_R|K_RIGHT)){ tab=(tab+1)&3; sel=top=ms=0; dirty=1; }
        if(pr&(K_L|K_LEFT)){ tab=(tab+3)&3; sel=top=ms=0; dirty=1; }
        if(tab==1){ if(pr&K_DOWN){ sel=(sel+1)%(SK_N+1); dirty=1; } if(pr&K_UP){ sel=(sel+SK_N)%(SK_N+1); dirty=1; } }
        if(tab==2){ if((pr&K_DOWN)&&top+6<hhN){ top++; dirty=1; } if((pr&K_UP)&&top>0){ top--; dirty=1; } }
        if(tab==3){ if(pr&K_DOWN){ ms=(ms+1)%MS_MORE; dirty=1; } if(pr&K_UP){ ms=(ms+MS_MORE-1)%MS_MORE; dirty=1; }
            if(pr&K_A){ if(ms==0) aspPanel(); else if(ms==1) tricksScreen(); else if(ms==2) goalsScreen(); else if(ms==3) trnPick(); else careerScreen();
                prev=keyNow(); dirty=1; } }
        if(!dirty&&(cnt>>3)==lt){ vsync(); continue; }   // idle: the picture on the screen is still right
        dirty=0; lt=cnt>>3;
        stBack("MY SIM",(int)cnt);
        { const char*h="L R TAB  B BACK"; text(236-tw(h,1),7,h,RGB(17,29,31),1); }
        { int x=6; for(int i=0;i<4;i++){ int w=tw(tn[i],1)+14; msTab(x,w,tn[i],i==tab); x+=w+3; } }
        if(tab==0) msCareer();
        else if(tab==1){ msPanel(35,121); msSkills(sel); }
        else if(tab==2){ msPanel(35,121); msPeople(top); }
        else { static const char* const ds[MS_MORE]={"THE ASPIRATION METER AND REWARD SHOP","THE SKATE CONTROLS ON ONE PAGE","THE GOALS OF THIS LOT AND THE TOWN","A 2 MINUTE TRICK SCORE ATTACK","CHANGE YOUR TRACK  PICK A BRANCH"};
            msPanel(35,121); text(10,40,"MORE ABOUT YOU",GOLD,1);
            for(int i=0;i<MS_MORE;i++){ const char*nm=i==0?"WANTS  FEARS  REWARDS":i==1?"VIEW TRICKS":i==2?"VIEW GOALS":i==3?trnLabel():"CAREER  CHANGE TRACK"; int y=52+i*15, on=i==ms;
                s2rr(10,y,220,13,on?GOLD:RGB(10,20,30)); s2rr(11,y+1,218,11,on?RGB(6,18,10):RGB(4,9,18)); text(16,y+3,nm,on?WHITE:RGB(20,26,30),1); }
            rect(8,131,224,1,RGB(14,26,31)); text(10,136,ds[ms],RGB(17,29,31),1); text(10,147,"A OPEN",RGB(12,14,16),1); }
        present();
    }
}
