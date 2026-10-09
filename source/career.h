// career.h - the CAREER screen (pause menu > PHONE > CAREER) and jobFriends().
// The tracks themselves (titles, pay, hours, perks) are the jobTr[] table in sims.h; this is the panel to look at them and to TRANSFER:
//   LEFT / RIGHT (or L / R)  flip through the tracks     A  transfer to the one on screen (asks first)     B  back
//   Level 3 of a track splits into BRANCH A (steady) and BRANCH B (high pay and a higher quota): the first visit after the promotion asks.
//   A transfer keeps part of your level (level 5 -> 2, 3 or 4 -> 1, else 0): the top of a track moves you on.
//   Teens may only take the part-time track (FAST FOOD). Promotions want the track's skill (jobSk: SKATING for PRO SKATER, a life skill for the rest; jobNeedSkill) and, from level 3, a friend (jobNeedFriend).
// Needs before it: sims.h (jobTr, jobTrack ...), house.h (relF, hhPUid), story.h (stBack: the backdrop).
static int jobFriends(void){ int me=hhPUid, n=0; for(int u=0;u<HU_N;u++) if(u!=me&&(relF[me][u]&RF_FRIEND)) n++; return n+nrFriends(me); }   // (and the friends you have in the town: townrel.h)
static const char* const jobPerkNm[5]={"","TRAINING","FINES","BARRACKS","STAFF MEAL"};
static const char* const jobPerkTx[5]={"","+1 SKILL EACH GOOD SHIFT","A BAD SHIFT COSTS 40","BILLS ARE HALVED","A GOOD SHIFT FILLS FOOD"};
static int jobAllowed(int t){ return stage>=AG_ADULT||jobTr[t].teen; }
static int jobStartLvl(int t){ int l=jobLvl>=5?2:jobLvl>=3?1:0; return l>jobTr[t].top?jobTr[t].top:l; }
static void jobBranchMenu(void){   // the first visit after reaching level 3: pick the branch
    static char a[28] EWRAM_BSS, b[28] EWRAM_BSS; char*e=simCat(a,jobTitleOf(jobTrack,3,0)); simCat(e,"  STEADY"); e=simCat(b,jobTitleOf(jobTrack,3,1)); simCat(e,"  HIGH PAY");
    const char* it[2]={a,b}; int c=menu("PICK A BRANCH",it,2); if(c<0) return;
    jobBr=c; jobChosen=1; simsSave(); toast(c?"BRANCH B  HIGH PAY  HIGH QUOTA":"BRANCH A  STEADY");
}
static void careerScreen(void){
    int sel=jobTrack; u16 prev=keyNow(); u32 cnt=0, lt=~0u;
    if(jobLvl>=3&&jobT()->top>=3&&!jobChosen) jobBranchMenu();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_RIGHT|K_R)) sel=(sel+1)%JT_N;
        if(pr&(K_LEFT|K_L)) sel=(sel+JT_N-1)%JT_N;
        if(pr&(K_B|K_START)) return;
        const JobTr*t=&jobTr[sel]; int me=sel==jobTrack, ok=jobAllowed(sel), nl=t->top+1;
        if(pr&K_A){
            if(me){ if(jobLvl>=3&&t->top>=3) jobBranchMenu(); }
            else if(!ok) toast("ADULTS ONLY");
            else { static char q[40] EWRAM_BSS; char*e=simCat(q,"JOIN "); simCat(e,t->nm); static const char* const yn[2]={"YES  TRANSFER","NO"};
                if(menu(q,yn,2)==0){ int nlv=jobStartLvl(sel); jobTrack=sel; jobLvl=nlv; jobBr=0; jobChosen=0; jobGood=jobBad=0; shiftPts=0; simsSave();
                    static char m[40] EWRAM_BSS; char*e2=simCat(m,"NEW JOB  "); simCat(e2,jobTitle()); toast(m); } }
            prev=keyNow(); continue;
        }
        if(!pr&&(cnt>>3)==lt){ vsync(); continue; }   // idle: the picture on the screen is still right (the whole backdrop used to be redrawn every frame, so taps landed between polls and were lost)
        lt=cnt>>3;
        stBack("CAREER",(int)cnt);
        s2rr(6,20,228,108,RGB(16,27,31)); s2rr(7,21,226,106,RGB(2,6,13)); s2grad(8,22,224,104,3,9,19,1,4,10);
        text(14,26,t->nm,GOLD,2);
        text(233-tw(t->teen?"PART TIME":"FULL TIME",1),26,t->teen?"PART TIME":"FULL TIME",RGB(17,29,31),1);
        if(me) text(233-tw("YOUR JOB",1),36,"YOUR JOB",RGB(10,28,12),1);
        { char b[28]; char*e; int y=46;
          e=slNum(slCat(b,"HOURS "),t->from); e=slCat(e," TO "); slNum(e,t->to); text(14,y,b,WHITE,1); y+=9;
          e=slNum(b,t->days); slCat(e," DAYS A WEEK"); text(14,y,b,WHITE,1); y+=9;
          e=slNum(slCat(b,"PAY "),jobPayOf(sel,0,0)); e=slCat(e," TO "); slNum(e,jobPayOf(sel,t->top,0)); text(14,y,b,WHITE,1); y+=9;
          e=slNum(slCat(b,sel==JT_SKATE?"TRICK QUOTA ":"WORK MIN "),jobQuotaOf(sel,0,0)); e=slCat(e," TO "); slNum(e,jobQuotaOf(sel,t->top,0)); text(14,y,b,WHITE,1); y+=9;
          e=slNum(slCat(b,"UP AFTER "),t->good); slCat(e," GOOD"); text(14,y,b,RGB(10,28,12),1); y+=9;
          e=slNum(slCat(b,"DOWN AFTER "),t->bad); slCat(e," BAD"); text(14,y,b,RGB(31,14,10),1); }
        for(int l=0;l<nl;l++){ int y=46+l*9, here=me&&l==jobLvl;   // the ladder (the branch you are on, or branch A)
            if(here){ rect(138,y-1,94,9,RGB(6,16,8)); text(140,y,">",WHITE,1); }
            text(148,y,jobTitleOf(sel,l,me?jobBr:0),here?WHITE:l<jobLvl&&me?RGB(10,22,12):RGB(20,26,30),1); }
        if(t->top>=3&&!me){ text(140,46+6*9,"LEVEL 3 SPLITS  A OR B",RGB(17,29,31),1); }
        rect(14,104,212,1,RGB(14,26,31));
        if(t->perk){ char b[44]; char*e=simCat(b,"PERK  "); e=simCat(e,jobPerkNm[t->perk]); e=simCat(e,"  "); simCat(e,jobPerkTx[t->perk]); text(14,108,b,GOLD,1); }
        else text(14,108,"NO PERK",RGB(12,18,24),1);
        { char b[48]; char*e;
          if(me){ if(jobLvl>=t->top) e=simCat(b,"TOP OF THE TRACK  TRANSFER FOR MORE");
              else { e=simCat(simCat(b,"NEXT LEVEL  "),jobSkName(jobTrack)); e=simCat(e," "); e=simCatN(e,jobNeedSkill(jobLvl)); if(jobNeedFriend(jobLvl)) simCat(e,"  1 FRIEND"); }
              text(14,117,b,RGB(24,27,30),1); }
          else if(!ok) text(14,117,"ONLY GROWN UPS CAN TAKE THIS",RGB(31,14,10),1);
          else { e=simCat(b,"A TRANSFER  YOU START AT "); simCat(e,jobTitleOf(sel,jobStartLvl(sel),0)); text(14,117,b,RGB(24,27,30),1); } }
        for(int i=0;i<JT_N;i++) s2rr(86+i*9,131,6,4,i==sel?GOLD:i==jobTrack?RGB(10,26,12):RGB(7,14,22));   // which track this is (green: yours)
        text(14,129,"<",GOLD,1); text(223,129,">",GOLD,1);
        { int br=me&&jobLvl>=3&&t->top>=3; const char*a=me?(br?"A BRANCH":"A OK"):"A TRANSFER"; int x=5, w=tw("LEFT RIGHT TRACK",1)+10;   // buttons as wide as their words
          s2pill(x,147,w,"LEFT RIGHT TRACK"); x+=w+4; w=tw(a,1)+10; s2pill(x,147,w,a); x+=w+4; s2pill(x,147,tw("B BACK",1)+10,"B BACK"); }
        present();
    }
}
