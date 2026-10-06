// story.h - STORY MODE (NEW GAME > STORY MODE): a life with chapters, so the game is more than a sandbox. Three stories, each a start
// (who you live with and how you feel about them) and a run of chapter goals the game checks by itself (stTick, every second):
//   ROOMMATES      romance: you and a roommate you barely know: become friends, fall in love, go steady, get promoted, a child comes home
//   NEWLYWEDS      romance and family: you and your love: get promoted, save up, a child comes home, become your kid's friend, save more
//   SINGLE PARENT  family: you and your kid: get promoted, become your kid's friend, save up, have a neighbor over, get promoted again
// A finished chapter pays §250 and 25 jenes and the next one starts. The child that comes home looks like both of you (stMixLook).
// Saved at STORY_OFF (8 bytes): 'S' 'Y', story, chapter (bit 7: a neighbor came over), your partner's uid, your kid's uid, the day the
// promised child comes home, checksum (255 = nobody / no day).
// The household bank (households.h) keeps it with the household, so every household has its own story.
// Needs before it: house.h (hhM, hhAdd, relF), sims.h (simMoney, jobLvl, simDay), households.h, lookTrueRandom, the UI kit.
_Static_assert(OPT_OFF+3+XO_N+1<=STORY_OFF&&STORY_OFF+8<=SLOT_DIR,"the story block overlaps the options or the slot directory");
enum { STY_NONE, STY_ROOM, STY_WED, STY_PARENT, STY_SKATE, STY_HOUSE, STY_FRIEND, STY_RAGS, STY_N };   // (new stories go at the END: the saved story number stays valid)
enum { SG_FRIEND, SG_LOVE, SG_STEADY, SG_JOB, SG_MONEY, SG_KID, SG_KIDFRIEND, SG_GUEST, SG_END,
    SG_SKILL, SG_TRICKS, SG_WANTS, SG_HOUSE, SG_FRIENDS, SG_BFF };   // (the last six: skill level, tricks landed, wants fulfilled, Sims in the house, friends in the house, a best friend)
typedef struct { const char* nm; u8 goal; u16 arg; } StCh;
static const StCh stRoom[]={ {"BECOME FRIENDS WITH YOUR ROOMMATE",SG_FRIEND,0}, {"FALL IN LOVE",SG_LOVE,0}, {"GO STEADY",SG_STEADY,0},
    {"GET A PROMOTION",SG_JOB,1}, {"A CHILD COMES HOME",SG_KID,0}, {"THE END  A FAMILY OF YOUR OWN",SG_END,0} };
static const StCh stWed[]={ {"GET A PROMOTION",SG_JOB,1}, {"SAVE 1000 SIMOLEONS",SG_MONEY,1000}, {"A CHILD COMES HOME",SG_KID,0},
    {"BECOME YOUR KID'S FRIEND",SG_KIDFRIEND,0}, {"SAVE 2500 SIMOLEONS",SG_MONEY,2500}, {"THE END  HAPPY EVER AFTER",SG_END,0} };
static const StCh stPar[]={ {"GET A PROMOTION",SG_JOB,1}, {"BECOME YOUR KID'S FRIEND",SG_KIDFRIEND,0}, {"SAVE 800 SIMOLEONS",SG_MONEY,800},
    {"HAVE A NEIGHBOR OVER",SG_GUEST,0}, {"GET ANOTHER PROMOTION",SG_JOB,2}, {"THE END  YOU MADE IT WORK",SG_END,0} };
static const StCh stSkate[]={ {"LAND 20 TRICKS",SG_TRICKS,20}, {"REACH SKILL LEVEL 2",SG_SKILL,2}, {"GET A PROMOTION",SG_JOB,1},
    {"LAND 150 TRICKS",SG_TRICKS,150}, {"REACH SKILL LEVEL 4",SG_SKILL,4}, {"THE END  A TRUE SKATER",SG_END,0} };
static const StCh stHouse[]={ {"BECOME FRIENDS WITH A HOUSEMATE",SG_FRIEND,0}, {"MAKE 3 FRIENDS",SG_FRIENDS,3}, {"FILL THE HOUSE WITH 4 SIMS",SG_HOUSE,4},
    {"GET A PROMOTION",SG_JOB,1}, {"SAVE 1500 SIMOLEONS",SG_MONEY,1500}, {"THE END  A HOUSE FULL OF LIFE",SG_END,0} };
static const StCh stFriend[]={ {"MAKE A BEST FRIEND",SG_BFF,0}, {"MAKE 2 FRIENDS",SG_FRIENDS,2}, {"HAVE A NEIGHBOR OVER",SG_GUEST,0},
    {"FULFIL 5 WANTS",SG_WANTS,5}, {"SAVE 500 SIMOLEONS",SG_MONEY,500}, {"THE END  FRIENDS FOR LIFE",SG_END,0} };
static const StCh stRags[]={ {"SAVE 300 SIMOLEONS",SG_MONEY,300}, {"GET A PROMOTION",SG_JOB,1}, {"SAVE 1000 SIMOLEONS",SG_MONEY,1000},
    {"GET ANOTHER PROMOTION",SG_JOB,2}, {"SAVE 3000 SIMOLEONS",SG_MONEY,3000}, {"THE END  RICH AT LAST",SG_END,0} };
static const StCh* const stChs[STY_N]={0,stRoom,stWed,stPar,stSkate,stHouse,stFriend,stRags};
static const u8 stLen[STY_N]={0,6,6,6,6,6,6,6};   // (6 each: the story card and the journal have room for six rows)
static const char* const stNm[STY_N]={"","ROOMMATES","NEWLYWEDS","SINGLE PARENT","SKATE LIFE","HOUSEFULL","BEST FRIENDS","RAGS TO RICHES"};
static int stRew(int ch){ return 250+ch*50; }   // the pay of a chapter rises with the story: 250, 300, 350 ...
static const char* const stAbout[STY_N]={"","A NEW ROOMMATE  AND MAYBE MORE","JUST MARRIED  A FAMILY TO START","YOU AND YOUR KID  ON YOUR OWN"};
static const char* const stTag[STY_N]={"","ROMANCE","ROMANCE AND FAMILY","FAMILY","SKILL AND CAREER","FRIENDSHIP AND HOME","FRIENDSHIP","MONEY AND CAREER"};
static const char* const stBlurb[STY_N][3]={{0,0,0},{"YOU MOVE IN WITH SOMEONE","YOU BARELY KNOW  FRIENDS","FIRST  THEN MAYBE LOVE"},
    {"JUST MARRIED AND IN LOVE","SAVE UP  CLIMB THE CAREER","AND START A FAMILY"},{"YOU AND YOUR KID ON YOUR","OWN  MAKE THE MONEY WORK","AND LET THE NEIGHBORS IN"},
    {"NOBODY HERE BUT YOU AND","A BOARD  LAND TRICKS  LEARN","THE SKILL  MAKE IT PAY"},{"A BUSY SHARED HOUSE  MAKE","FRIENDS  FILL THE ROOMS","AND KEEP THE BILLS PAID"},
    {"TWO NEW HOUSEMATES  ONE","TRUE BEST FRIEND  AND A","NEIGHBOR TO HAVE OVER"},{"YOU START WITH ALMOST","NOTHING  WORK  SAVE  GET","PROMOTED  AND GET RICH"}};
static u8 stShown;   // the chapter whose card was shown last (id*16+chapter+1): a card once per chapter
static u8 stId, stCh, stPart=255, stKid=255, stKidDay=255, stGuest;   // the story, its chapter, your partner and your kid (uids), the day the promised child comes, a guest came
static u8 stSum(volatile u8*m){ return (u8)(0x53+m[2]+m[3]*3+m[4]*5+m[5]*7+m[6]*11); }
static void stSave(void){ volatile u8*m=SRAM_BASE+STORY_OFF; m[0]='S'; m[1]='Y'; m[2]=stId; m[3]=(u8)(stCh|(stGuest?0x80:0)); m[4]=stPart; m[5]=stKid; m[6]=stKidDay; m[7]=stSum(m); }
static void stLoad(void){ volatile u8*m=SRAM_BASE+STORY_OFF; stId=0; stCh=0; stPart=stKid=stKidDay=255; stGuest=0;
    if(m[0]!='S'||m[1]!='Y'||m[2]>=STY_N||m[7]!=stSum(m)) return;
    stId=m[2]; stCh=(u8)(m[3]&0x7F); stGuest=(u8)(m[3]>>7); stPart=m[4]; stKid=m[5]; stKidDay=m[6]; if(stId&&stCh>=stLen[stId]) stCh=(u8)(stLen[stId]-1); }
static void stOff(void){ stId=0; stCh=0; stPart=stKid=stKidDay=255; stGuest=0; stSave(); }   // a game without a story
static int stMember(int uid){ for(int m=0;m<hhN;m++) if(hhM[m].uid==uid) return m; return -1; }
static void stMixLook(u8*out,const u8*a,const u8*b,int stg){   // a child of a and b: every pick, slider and colour from one of them, then fitted to the age
    u8 sl[LK_N], ss=stage; for(int i=0;i<LK_N;i++){ sl[i]=look[i]; look[i]=(rnd8()&1)?a[i]:b[i]; }
    stage=(u8)stg; look[LK_BEARD]=0; fixLook(); for(int i=0;i<LK_N;i++) out[i]=look[i];
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=ss;
}
static int stAddSim(const u8*lk,int stg,const char*last){   // someone moves in for the story (not the debug-code HOUSEHOLD menu): their place, -1 = full
    u8 tr[TR_N]; for(int i=0;i<TR_N;i++) tr[i]=(u8)(rnd8()%11);
    int m=hhAdd(lk,stg,stg<AG_ADULT?AS_GROW:rnd8()%AS_PICK,rnd8()&1,tr); if(m<0) return -1;
    if(last){ int k=0; for(;last[k]&&k<HH_NM-1;k++) hhM[m].last[k]=last[k]; hhM[m].last[k]=0; }
    return m;
}
static void stRel(int a,int b,int d,int l,u8 f){ relD[a][b]=relD[b][a]=(signed char)d; relL[a][b]=relL[b][a]=(signed char)l; relF[a][b]=relF[b][a]=f; }
static void stKidHome(void){   // the promised child moves in: a mix of you and your partner (or a look of their own)
    int p=stMember(stPart); u8 lk[LK_N];
    if(p>=0) stMixLook(lk,look,hhM[p].look,AG_CHILD); else { u8 st=AG_CHILD; lookTrueRandom(lk,&st); }
    int m=stAddSim(lk,AG_CHILD,hhPLast); if(m<0){ toast("THE HOUSE IS FULL  NO ROOM FOR A CHILD"); return; }
    stRel(hhPUid,hhM[m].uid,50,40,RF_FRIEND); if(p>=0) stRel(hhM[p].uid,hhM[m].uid,50,40,RF_FRIEND);
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  YOUR CHILD IS COMING HOME"); hhBakeAll(); hhSave(); liveInvalidate();
    static char t[40]; char*e=simCat(t,hhM[m].name); simCat(e," IS HOME"); toast(t);
    stKid=hhM[m].uid;   // (the kid goals are about this child)
}
static int stValue(const StCh*c){   // the number a goal counts (-1: the goal has none)
    switch(c->goal){
        case SG_MONEY: return simMoney;      case SG_JOB: return jobLvl;      case SG_SKILL: return skillLvl;
        case SG_TRICKS: return simTricks;    case SG_WANTS: return simDone;   case SG_HOUSE: return hhN+1;
        case SG_FRIENDS: { int me=hhPUid, n=0; for(int u=0;u<HU_N;u++) if(u!=me&&(relF[me][u]&RF_FRIEND)) n++; return n; }
    }
    return -1;
}
static int stDone(const StCh*c){   // is the chapter's goal met?
    int me=hhPUid, p=stMember(stPart), pu=p>=0?hhM[p].uid:-1;
    switch(c->goal){
        case SG_FRIEND: return pu>=0&&(relF[me][pu]&RF_FRIEND);
        case SG_LOVE: return pu>=0&&(relF[me][pu]&RF_LOVE);
        case SG_STEADY: return pu>=0&&(relF[me][pu]&RF_STEADY);
        case SG_JOB: return jobLvl>=c->arg;
        case SG_MONEY: return simMoney>=c->arg;
        case SG_KID: return 0;   // (an event: stTick brings the child home a day after the chapter starts)
        case SG_KIDFRIEND: { int k=stMember(stKid); return k>=0&&(relF[me][hhM[k].uid]&RF_FRIEND); }
        case SG_GUEST: return stGuest;
        case SG_BFF: return pu>=0&&(relF[me][pu]&RF_BFF);
        case SG_SKILL: case SG_TRICKS: case SG_WANTS: case SG_HOUSE: case SG_FRIENDS: { int v=stValue(c); return v>=(int)c->arg; }
    }
    return 0;
}
// STORY MISSIONS (roadmap #4): every story chapter finished, in any life, is counted once for good (the same chapter again adds nothing). The END card of a story is not a mission,
// so a story has 5. Half of all of them (the missions of every story in the game) unlock CLOSER TO THE END and TREE-AGE IN ACTION.
// Lives in story.h (it needs STY_N). Saved in the jukebox block at JB_OFF+40: 'M' 'S', 8 bytes (a bit per mission: story-1 * 5 + chapter, room for 12 stories), the bytes xor 0x5A (appended; nothing moved).
#define SM_PER 5
static int jbStoryDone(int story,int ch){   // 1 when this mission was the one that reached half of them and songs came free
    if(story<1||story>=STY_N||ch<0||ch>=SM_PER) return 0;
    volatile u8*m=SRAM_BASE+JB_OFF+40; u8 v[8]; u8 x=0x5A;
    for(int i=0;i<8;i++){ v[i]=(m[0]=='M'&&m[1]=='S')?m[2+i]:0; x^=v[i]; }
    if(m[0]=='M'&&m[1]=='S'&&m[10]!=x) for(int i=0;i<8;i++) v[i]=0;   // a damaged block starts again
    int b=(story-1)*SM_PER+ch; if(b<64) v[b>>3]|=(u8)(1<<(b&7));
    x=0x5A; for(int i=0;i<8;i++) x^=v[i];
    m[0]='M'; m[1]='S'; for(int i=0;i<8;i++) m[2+i]=v[i]; m[10]=x;
    int n=0; for(int i=0;i<(STY_N-1)*SM_PER&&i<64;i++) n+=(v[i>>3]>>(i&7))&1;
    if(n*2<(STY_N-1)*SM_PER) return 0;
    int a=jbUnlock(UL_CLOSER), t=jbUnlock(UL_TREE); return a|t;
}
static void stAnnounce(void){ static char t[44]; char*e=slCat(t,"CHAPTER "); e=slNum(e,stCh+1); e=slCat(e,"  "); slCat(e,stChs[stId][stCh].nm); lnote=t; lnoteT=240; }
static void stTick(void){   // once per logic step in the life game: is this chapter done?
    static u8 cnt; if(!stId||++cnt<60) return; cnt=0;
    if(stCh>=stLen[stId]) return;
    for(int k=0;k<TW_N;k++) if(twOn[k]==2) stGuest=1;   // a neighbor is staying over (HAVE A NEIGHBOR OVER)
    const StCh*c=&stChs[stId][stCh];
    if(c->goal==SG_END) return;
    if(c->goal==SG_KID){   // a day after the chapter starts the child comes home
        if(stKidDay==255){ stKidDay=(u8)((simDay+1)&255); stSave(); return; }
        if((u8)simDay!=stKidDay) return;
        stKidHome(); stKidDay=255;
    } else if(!stDone(c)) return;
    simMoney+=stRew(stCh); if(simMoney>9999) simMoney=9999; dnaAdd(25); persSave(); simsSave();
    if(jbStoryDone(stId,stCh)){ simQPush("MORE SCOOBY STUFF TO FIND"); simQPush("TOUCH GRASS TO FIND IT"); }   // half of all the story missions: secret songs (no names, go and look)
    stCh++; stSave(); stAnnounce(); stShown=(u8)(stId*16+stCh+1); stModal=2;   // the CHAPTER COMPLETE card (stRunModal)
}
static void stEnter(void){ stLoad(); if(stId){ stAnnounce(); if(stShown!=(u8)(stId*16+stCh+1)){ stShown=(u8)(stId*16+stCh+1); stModal=1; } } }   // (a chapter card once per chapter and power on)   // entering the life game: the current goal on the top bar
// ---- the look: Sims 2 / Life Stories panels (the pieces live in main.c next to HOW TO PLAY) ----
static void s2rr(int x,int y,int w,int h,u16 c); static void s2grad(int x,int y,int w,int h,int r0,int g0,int b0,int r1,int g1,int b1);
static void s2plumbob(int cx,int y); static void s2pill(int x,int y,int w,const char*s);
static void stBack(const char*title,int cnt){   // the backdrop, the frame, the title bar with the bobbing plumbob
    static const signed char bob[8]={0,1,2,2,1,0,-1,-1};
    objHideAll();
    s2grad(0,0,SW,SH,1,4,10,2,9,17);
    for(int y=0;y<SH;y+=8) for(int x=(y&8)?4:0;x<SW;x+=8) rect(x,y,1,1,RGB(3,9,17));
    s2rr(1,1,238,158,RGB(10,20,30)); s2rr(2,2,236,156,RGB(2,6,13));
    s2grad(3,3,234,14,8,18,28,3,10,19); rect(3,17,234,1,RGB(14,26,31));
    s2plumbob(11,2+bob[(cnt>>3)&7]); text(21,7,title,WHITE,1);
}
static void stIcon(int s,int x,int y,int sc,u16 c){   // a little picture per story: a heart, a ring, a parent with a kid (9 x 8 pixels, drawn big)
    static const char* const pic[STY_N][8]={{0},
      {".XX...XX.","XXXX.XXXX","XXXXXXXXX","XXXXXXXXX",".XXXXXXX.","..XXXXX..","...XXX...","....X...."},
      {"....X....","...XXX...","....X....","..XXXXX..",".X.....X.",".X.....X.",".X.....X.","..XXXXX.."},
      {".XX......",".XX......","XXXX.....","XXXX.XX..",".XX..XX..",".XX.XXXX.",".XX..XX..","XXXX.X.X."},
      {".........",".........","XXXXXXXXX",".XXXXXXX.",".........",".XX...XX.",".XX...XX.","........."},
      {"....X....","...XXX...","..XXXXX..",".XXXXXXX.","XXXXXXXXX","XX.XXX.XX","XX.XXX.XX","XXXXXXXXX"},
      {".XX...XX.",".XX...XX.","XXXX.XXXX","XXXX.XXXX",".XX...XX.",".XX...XX.",".X.X.X.X.",".X.X.X.X."},
      {"..XXXXX..",".XXXXXXX.","XXXX.XXXX","XXX...XXX","XXXX.XXXX","XXXXXXXXX",".XXXXXXX.","..XXXXX.."}};
    for(int r=0;r<8;r++) for(int q=0;q<9;q++) if(pic[s][r][q]=='X') rect(x+q*sc,y+r*sc,sc,sc,c);
}
static void stSparkle(u32 cnt,int x0,int y0,int w,int h){   // a few twinkling pixels (a cheap celebration)
    for(int i=0;i<10;i++){ u32 h1=(u32)(i*2654435761u)>>8; int x=x0+(int)(h1%(u32)w), y=y0+(int)((h1>>9)%(u32)h); int ph=(int)((cnt>>2)+(u32)i*5)&15;
        if(ph<4){ u16 c=ph<2?RGB(31,30,18):RGB(24,22,8); rect(x,y,1,1,c); if(ph==1){ rect(x-1,y,3,1,c); rect(x,y-1,1,3,c); } } }
}
// the chapter goal's progress as text (only the goals that have a number)
static int stProg(const StCh*c,char*b){
    int v=stValue(c), of=c->arg; if(v<0) return 0;
    if(v>of) v=of;
    char*e=slNum(b,v); e=slCat(e," OF "); slNum(e,of); return 1;
}
static void storyScreen(void){   // pause menu > STORY: the story journal, a chapter timeline
    u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_A|K_B|K_START)) return;
        stBack(stId?"STORY JOURNAL":"STORY",(int)cnt);
        if(!stId){
            s2rr(8,24,224,60,RGB(10,20,30)); s2rr(9,25,222,58,RGB(2,6,13));
            text(16,32,"NO STORY RIGHT NOW",GOLD,1); text(16,46,"START ONE FROM THE MAIN MENU",WHITE,1); text(16,56,"PLAY  NEW GAME  STORY MODE",RGB(17,29,31),1);
            text(16,70,"THREE STORIES  SIX CHAPTERS EACH",RGB(20,24,28),1);
        } else {
            int n=stLen[stId];
            stIcon(stId,9,21,2,RGB(31,20,22));   // the story's picture, its name and kind, and how far you are
            text(32,21,stNm[stId],GOLD,1); text(32,30,stTag[stId],RGB(17,29,31),1);
            { char b[24]; char*e=slCat(b,"CHAPTER "); e=slNum(e,stCh+1); e=slCat(e," OF "); slNum(e,n); text(233-tw(b,1),21,b,WHITE,1);
              int bw=84, fx=233-bw; rect(fx-1,31,bw+2,7,RGB(14,26,31)); rect(fx,32,bw,5,RGB(3,5,9)); int f=bw*stCh/(n>1?n-1:1); if(f>bw) f=bw; if(f>0){ s2grad(fx,32,f,5,10,26,12,6,18,8); rect(fx,32,f,1,RGB(18,31,20)); } }
            for(int i=0;i<n;i++){ int y=44+i*13, st=i<stCh?2:i==stCh?1:0; const StCh*c=&stChs[stId][i];
                if(i+1<n) rect(14,y+9,2,4,st==2?RGB(8,26,10):RGB(7,14,22));   // the line down to the next chapter
                if(st==2){ rect(10,y+1,10,8,RGB(5,18,7)); rect(11,y+2,8,6,RGB(8,26,10)); text(12,y+2,"+",WHITE,1); }   // done: a green tick
                else if(st==1){ int g=(cnt>>3)&1; rect(10,y+1,10,8,g?RGB(31,26,6):RGB(24,19,3)); rect(11,y+2,8,6,RGB(3,5,9)); rect(13,y+4,4,2,GOLD); }   // now: a gold blinking ring
                else { rect(10,y+1,10,8,RGB(7,14,22)); rect(11,y+2,8,6,RGB(2,6,13)); }
                if(st==1){ s2grad(23,y,208,11,6,16,26,3,9,17); }
                text(25,y+2,c->nm,st==2?RGB(10,22,12):st==1?WHITE:RGB(12,18,24),1);
                if(st==1){ char b[16]; if(stProg(c,b)) text(231-tw(b,1),y+2,b,GOLD,1); } }
        }
        s2pill(5,147,60,"A OR B BACK");
        if(stId){ char b[34]; int tot=0; for(int i=0;i<stCh;i++) tot+=stRew(i); char*e=slCat(b,"EARNED "); e=slNum(e,tot); slCat(e," SIMOLEONS"); s2pill(69,147,tw(b,1)+10,b); }
        present();
    }
}
// the chapter cards: CHAPTER n (a chapter starts) and CHAPTER COMPLETE (a chapter was done). Shown by lifeModeRun like the pause menu.
static void stRunModal(void){
    int kind=stModal; stModal=0; if(!stId) return;
    u16 prev=keyNow(); u32 cnt=0; const StCh*c=&stChs[stId][stCh];
    int end=c->goal==SG_END;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_A|K_B|K_START)) return;
        stBack(kind==2?"CHAPTER COMPLETE":"YOUR STORY",(int)cnt);
        s2rr(8,22,224,116,RGB(16,27,31)); s2rr(9,23,222,114,RGB(2,6,13)); s2grad(10,24,220,112,3,9,19,1,4,10);
        stIcon(stId,16,32,4,RGB(31,20,22));
        text(60,30,stNm[stId],GOLD,1); text(60,40,stTag[stId],RGB(17,29,31),1);
        { char b[24]; char*e=slCat(b,kind==2?"CHAPTER ":"CHAPTER "); e=slNum(e,kind==2?stCh:stCh+1); text(60,52,b,WHITE,2); }
        if(kind==2){ char b[40]; char*e=slCat(b,"DONE  +"); e=slNum(e,stRew(stCh-1)); slCat(e," SIMOLEONS  +25 JENES"); text(60,70,b,RGB(10,28,12),1); stSparkle(cnt,12,26,216,100); }
        rect(14,84,212,1,RGB(14,26,31));
        text(16,90,kind==2?(end?"THE END":"NEXT CHAPTER"):(end?"THE END":"YOUR GOAL"),GOLD,1);
        text(16,102,end?(kind==2?"YOUR STORY GOES ON  KEEP PLAYING":"YOUR STORY GOES ON  KEEP PLAYING"):c->nm,WHITE,1);
        if(!end){ char b[16]; if(stProg(c,b)) text(16,112,b,RGB(20,26,31),1); }
        s2pill(5,147,40,"A OK");
        present();
    }
}
// NEW GAME > STORY MODE: pick a story on a story card (LEFT RIGHT to flip through them, A to start)
static int storyPick(void){
    int sel=1; u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_RIGHT|K_R)) sel=sel%(STY_N-1)+1;
        if(pr&(K_LEFT|K_L)) sel=(sel+STY_N-3)%(STY_N-1)+1;
        if(pr&K_A) return sel;
        if(pr&(K_B|K_START)) return 0;
        stBack("WHICH STORY?",(int)cnt);
        s2rr(8,21,224,126,RGB(16,27,31)); s2rr(9,22,222,124,RGB(2,6,13)); s2grad(10,23,220,122,3,9,19,1,4,10);
        s2rr(14,27,44,40,RGB(10,20,30)); s2grad(15,28,42,38,7,16,26,3,8,16); stIcon(sel,18+((cnt>>4)&1),33,4,RGB(31,20,22));   // the story's picture (it beats slowly)
        text(64,28,stNm[sel],GOLD,2); text(64,46,stTag[sel],RGB(17,29,31),1);
        for(int i=0;i<3;i++) text(64,56+i*9,stBlurb[sel][i],WHITE,1);
        rect(14,72,212,1,RGB(14,26,31)); text(16,76,"THE CHAPTERS",RGB(17,29,31),1);
        for(int i=0;i<stLen[sel];i++){ char b[44]; char*e=slNum(b,i+1); e=slCat(e,"  "); slCat(e,stChs[sel][i].nm); text(16,86+i*8,b,i==stLen[sel]-1?GOLD:RGB(24,27,30),1); }
        for(int i=1;i<STY_N;i++){ int x=108+(i-1)*12; s2rr(x,136,8,4,i==sel?GOLD:RGB(7,14,22)); }   // which of the stories this is
        text(14,134,"<",GOLD,1); text(223,134,">",GOLD,1);
        s2pill(5,147,66,"LEFT RIGHT STORY"); s2pill(75,147,40,"A START"); s2pill(119,147,40,"B BACK");
        present();
    }
}
static void storySetup(int s){   // after the new life is set up and the old household has gone
    stId=(u8)s; stCh=0; stPart=stKid=stKidDay=255; stGuest=0;
    u8 lk[LK_N], st; int m;
    switch(s){
    case STY_ROOM: st=AG_ADULT; lookTrueRandom(lk,&st); { char l[HH_NM]; famLast(&hhFams[rnd8()%HH_NFAM],l);
        m=stAddSim(lk,AG_ADULT,l); if(m>=0){ stRel(hhPUid,hhM[m].uid,10,0,0); stPart=hhM[m].uid; } } break;
    case STY_WED: st=AG_ADULT; lookTrueRandom(lk,&st);
        m=stAddSim(lk,AG_ADULT,hhPLast); if(m>=0){ stRel(hhPUid,hhM[m].uid,70,80,RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF); stPart=hhM[m].uid; } break;
    case STY_PARENT: stMixLook(lk,look,look,AG_CHILD);   // your kid takes after you
        m=stAddSim(lk,AG_CHILD,hhPLast); if(m>=0){ stRel(hhPUid,hhM[m].uid,40,30,0); stKid=hhM[m].uid; } break;
    case STY_SKATE: break;   // just you and a board
    case STY_RAGS: simMoney=100; break;   // you start with almost nothing
    case STY_HOUSE: case STY_FRIEND:   // housemates who are not friends yet (BEST FRIENDS: two of them)
        for(int i=0;i<(s==STY_FRIEND?2:1);i++){ st=AG_ADULT; lookTrueRandom(lk,&st); char l[HH_NM]; famLast(&hhFams[rnd8()%HH_NFAM],l);
            m=stAddSim(lk,AG_ADULT,l); if(m>=0){ stRel(hhPUid,hhM[m].uid,20,0,0); if(!i) stPart=hhM[m].uid; } }
        break;
    }
    hhSave(); stSave();
}
