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
enum { STY_NONE, STY_ROOM, STY_WED, STY_PARENT, STY_SKATE, STY_HOUSE, STY_FRIEND, STY_RAGS, STY_CLIMB, STY_TOWN, STY_SECOND, STY_TVSHOW, STY_N };   // (new stories go at the END: the saved story number stays valid)
enum { SG_FRIEND, SG_LOVE, SG_STEADY, SG_JOB, SG_MONEY, SG_KID, SG_KIDFRIEND, SG_GUEST, SG_END,
    SG_SKILL, SG_TRICKS, SG_WANTS, SG_HOUSE, SG_FRIENDS, SG_BFF, SG_DAYS, SG_SCRIPT, SG_MULTI };   // (SG_SCRIPT: no number to count: only a scripted cutscene ends the chapter, with stComplete)
//   TV SHOW & TELL  drama: a fallen TV judge wins her fans back in five chapters (Dancing with the Bars, Pull Yourself a Sweater, The Winner Takes It All, The Loser Has To Fall, While She's Dancing with the Stars)   // (the last seven: skill level, tricks landed, wants fulfilled, Sims in the house, friends in the house, a best friend, days since the chapter began)
typedef struct { const char* nm; u8 goal; u32 arg; } StCh;
static const StCh stRoom[]={ {"BECOME FRIENDS WITH YOUR ROOMMATE",SG_FRIEND,0}, {"FALL IN LOVE",SG_LOVE,0}, {"GO STEADY",SG_STEADY,0},
    {"GET A PROMOTION",SG_JOB,1}, {"A CHILD COMES HOME",SG_KID,0}, {"THE END  A FAMILY OF YOUR OWN",SG_END,0} };
static const StCh stWed[]={ {"GET A PROMOTION",SG_JOB,1}, {"SAVE 40000 SIMOLEONS",SG_MONEY,40000}, {"A CHILD COMES HOME",SG_KID,0},
    {"BECOME YOUR KID'S FRIEND",SG_KIDFRIEND,0}, {"SAVE 100000 SIMOLEONS",SG_MONEY,100000}, {"THE END  HAPPY EVER AFTER",SG_END,0} };
static const StCh stPar[]={ {"GET A PROMOTION",SG_JOB,1}, {"BECOME YOUR KID'S FRIEND",SG_KIDFRIEND,0}, {"SAVE 32000 SIMOLEONS",SG_MONEY,32000},
    {"HAVE A NEIGHBOR OVER",SG_GUEST,0}, {"GET ANOTHER PROMOTION",SG_JOB,2}, {"THE END  YOU MADE IT WORK",SG_END,0} };
static const StCh stSkate[]={ {"LAND 20 TRICKS",SG_TRICKS,20}, {"REACH SKILL LEVEL 2",SG_SKILL,2}, {"GET A PROMOTION",SG_JOB,1},
    {"LAND 150 TRICKS",SG_TRICKS,150}, {"REACH SKILL LEVEL 4",SG_SKILL,4}, {"THE END  A TRUE SKATER",SG_END,0} };
static const StCh stHouse[]={ {"BECOME FRIENDS WITH A HOUSEMATE",SG_FRIEND,0}, {"MAKE 3 FRIENDS",SG_FRIENDS,3}, {"FILL THE HOUSE WITH 4 SIMS",SG_HOUSE,4},
    {"GET A PROMOTION",SG_JOB,1}, {"SAVE 60000 SIMOLEONS",SG_MONEY,60000}, {"THE END  A HOUSE FULL OF LIFE",SG_END,0} };
static const StCh stFriend[]={ {"MAKE A BEST FRIEND",SG_BFF,0}, {"MAKE 2 FRIENDS",SG_FRIENDS,2}, {"HAVE A NEIGHBOR OVER",SG_GUEST,0},
    {"FULFIL 5 WANTS",SG_WANTS,5}, {"SAVE 20000 SIMOLEONS",SG_MONEY,20000}, {"THE END  FRIENDS FOR LIFE",SG_END,0} };
static const StCh stRags[]={ {"SAVE 12000 SIMOLEONS",SG_MONEY,12000}, {"GET A PROMOTION",SG_JOB,1}, {"SAVE 40000 SIMOLEONS",SG_MONEY,40000},
    {"GET ANOTHER PROMOTION",SG_JOB,2}, {"SAVE 120000 SIMOLEONS",SG_MONEY,120000}, {"THE END  RICH AT LAST",SG_END,0} };
static const StCh stClimb[]={ {"GET A PROMOTION",SG_JOB,1}, {"SAVE 24000 SIMOLEONS",SG_MONEY,24000}, {"FULFIL 5 WANTS",SG_WANTS,5},
    {"GET ANOTHER PROMOTION",SG_JOB,2}, {"REACH SKILL LEVEL 3",SG_SKILL,3}, {"THE END  TOP OF THE LADDER",SG_END,0} };
static const StCh stTown[]={ {"BECOME FRIENDS WITH A HOUSEMATE",SG_FRIEND,0}, {"MAKE 2 FRIENDS",SG_FRIENDS,2}, {"HAVE A NEIGHBOR OVER",SG_GUEST,0},
    {"FILL THE HOUSE WITH 3 SIMS",SG_HOUSE,3}, {"FULFIL 8 WANTS",SG_WANTS,8}, {"THE END  YOU BELONG HERE",SG_END,0} };
static const StCh stSecond[]={ {"BECOME FRIENDS AGAIN",SG_FRIEND,0}, {"FALL BACK IN LOVE",SG_LOVE,0}, {"GO STEADY AGAIN",SG_STEADY,0},
    {"SURVIVE 14 MORE DAYS",SG_DAYS,14}, {"SAVE 32000 SIMOLEONS",SG_MONEY,32000}, {"THE END  WORTH FIXING",SG_END,0} };
static const StCh stTv[]={ {"DANCING WITH THE BARS",SG_MULTI,0}, {"PULL YOURSELF A SWEATER",SG_MULTI,0}, {"THE WINNER TAKES IT ALL...",SG_MULTI,0},
    {"...THE LOSER HAS TO FALL",SG_MULTI,0}, {"WHILE SHE'S DANCING WITH THE STARS",SG_MULTI,0}, {"THE END  HERE TODAY",SG_END,0} };
_Static_assert(sizeof(stTv)/sizeof(stTv[0])==6,"a story has five chapters and the END row");
// the two lines under a TV SHOW & TELL chapter on its card (what the chapter is about; nothing is counted)
static const char* const stTvBrief[5][2]={ {"HUNGOVER AND UNSTEADY  PULL YOURSELF","TOGETHER AND JUDGE THE TALENT SHOW"},
    {"TWO MONTHS ON  YOUR FANS HAVE GONE COLD","GO A WEEK WITHOUT DRINKING  I DARE YOU"},
    {"THE PAPARAZZI ARE EVERYWHERE  BUILD","A PRIVATE HOUSE WITH TIGHT SECURITY"},
    {"YOU ARE MAMESY NOW  KEEP HOPE ALIVE","AND STOP THE PLUG BEING PULLED"},
    {"YOUR COMEBACK SHOW IS READY  NOTHING","CAN GO WRONG  RIGHT"} };
static const StCh* const stChs[STY_N]={0,stRoom,stWed,stPar,stSkate,stHouse,stFriend,stRags,stClimb,stTown,stSecond,stTv};
static const u8 stLen[STY_N]={0,6,6,6,6,6,6,6,6,6,6,6};   // (6 each: the story card and the journal have room for six rows)
static const char* const stNm[STY_N]={"","ROOMMATES","NEWLYWEDS","SINGLE PARENT","SKATE LIFE","HOUSEFULL","BEST FRIENDS","RAGS TO RICHES","CAREER CLIMBER","NEW IN TOWN","SECOND CHANCE","TV SHOW & TELL"};
static int stRew(int ch){ return 5000+ch*1000; }   // the pay of a chapter rises with the story: 5000, 6000, 7000 ...
static const char* const stAbout[STY_N]={"","A NEW ROOMMATE  AND MAYBE MORE","JUST MARRIED  A FAMILY TO START","YOU AND YOUR KID  ON YOUR OWN"};
static const char* const stTag[STY_N]={"","ROMANCE","ROMANCE AND FAMILY","FAMILY","SKILL AND CAREER","FRIENDSHIP AND HOME","FRIENDSHIP","MONEY AND CAREER","CAREER AND SKILL","FRIENDSHIP AND TOWN","ROMANCE AND REPAIR","FAME AND RECOVERY"};
static const char* const stBlurb[STY_N][3]={{0,0,0},{"YOU MOVE IN WITH SOMEONE","YOU BARELY KNOW  FRIENDS","FIRST  THEN MAYBE LOVE"},
    {"JUST MARRIED AND IN LOVE","SAVE UP  CLIMB THE CAREER","AND START A FAMILY"},{"YOU AND YOUR KID ON YOUR","OWN  MAKE THE MONEY WORK","AND LET THE NEIGHBORS IN"},
    {"NOBODY HERE BUT YOU AND","A BOARD  LAND TRICKS  LEARN","THE SKILL  MAKE IT PAY"},{"A BUSY SHARED HOUSE  MAKE","FRIENDS  FILL THE ROOMS","AND KEEP THE BILLS PAID"},
    {"TWO NEW HOUSEMATES  ONE","TRUE BEST FRIEND  AND A","NEIGHBOR TO HAVE OVER"},{"YOU START WITH ALMOST","NOTHING  WORK  SAVE  GET","PROMOTED  AND GET RICH"},
    {"A JOB WITH A FUTURE  WORK","HARD  LEARN A SKILL  AND","CLIMB THE LADDER"},{"YOU JUST MOVED IN  KNOW","NOBODY  MEET THE NEIGHBORS","AND MAKE THE PLACE YOURS"},
    {"YOU WERE CLOSE ONCE  NOW","THEY BARELY LOOK AT YOU","FIX WHAT YOU BROKE"},
    {"A FALLEN TV JUDGE  WIN BACK","HER FANS  AND SHAKE THE URGE","FOR ONE MORE MARTINI"}};
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
    for(int i=0;i<LK_N;i++) look[i]=sl[i];
    stage=ss;
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
    int m=stAddSim(lk,AG_CHILD,hhPLast); if(m<0){ toast("The house is full. No room for a kid."); return; }
    stRel(hhPUid,hhM[m].uid,50,40,RF_FRIEND); if(p>=0) stRel(hhM[p].uid,hhM[m].uid,50,40,RF_FRIEND);
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    toast("One moment. Your kid is on the way."); hhBakeAll(); hhSave(); liveInvalidate();
    static char t[40] EWRAM_BSS; char*e=simCat(t,hhM[m].name); simCat(e," IS HOME"); toast(t);
    stKid=hhM[m].uid;   // (the kid goals are about this child)
}
// TV SHOW & TELL: every chapter is SEVERAL goals at once and ALL of them must hold at the same moment (no skating in this story: it is about getting a life back).
// The chapter clock lives in stKidDay (no kid in this story): the day the chapter began, or the day of the last RELAPSE (a puff) or COLLAPSE (faint / pass out) in the chapters that watch for them.
static const char* const stqNm[9]={"DAYS CLEAN","DAYS HOLDING ON","FRIENDS","SIMOLEONS","SIMS LIVING HERE","HAVE A NEIGHBOR OVER","FEEL HAPPY RIGHT NOW","SECURITY PIECES","PLUG PRESSURE"};
enum { TQ_CLEAN, TQ_DAYS, TQ_FRIENDS, TQ_MONEY, TQ_HOUSE, TQ_GUEST, TQ_HAPPY, TQ_SECURE, TQ_PLUG };
// TV SHOW & TELL step 4: the PLUG. Chapter 4 only. Missy lies unconscious and the hospital (Dr. Okafor) wants to pull the plug: plugV is its PRESSURE, 0 to 100.
// You are Mamesy and you hold it off by keeping the chapter going: it creeps up every 10 seconds by 3, less 1 for each of: 2 friends, feeling happy, 12000 simoleons.
// Every new day Dr. Okafor calls: the life-support bill is paid from your cash (PLUG_BILL; if you cannot, the pressure jumps). A new friend, a best friend or a promotion eases it.
// At 75 and 90 you are warned. At 100 the hospital gives you ONE MORE NIGHT: the days-holding-on clock starts over, a deposit is taken and the pressure drops to 50. The chapter also
// asks for the pressure to be 60 or less at the moment you finish. Nothing is saved: the pressure starts again at PLUG_START when the game starts and the chapter is running.
#define PLUG_START 20
#define PLUG_BILL 400
#define PLUG_OK 60
static u8 plugV=PLUG_START; static u16 plugDay=0xFFFF; static u8 plugSec, plugTen, plugNight;   // plugNight: the ONE MORE NIGHT was used (the next time the pressure hits 100 Missy dies)
typedef struct { u8 k; u32 n; } StQ;
static const StQ stqT[5][5]={
    { {TQ_GUEST,1}, {TQ_FRIENDS,1}, {TQ_MONEY,4000} },                                                     // 1 hungover: have somebody over, a friend, a little cash
    { {TQ_CLEAN,7}, {TQ_FRIENDS,2}, {TQ_MONEY,6000}, {TQ_HAPPY,1} },                                       // 2 a week clean (a puff or a faint starts it again)
    { {TQ_HOUSE,3}, {TQ_CLEAN,4}, {TQ_MONEY,20000}, {TQ_FRIENDS,2}, {TQ_SECURE,4} },                                      // 3 a private house with a crew around you
    { {TQ_DAYS,5}, {TQ_FRIENDS,2}, {TQ_MONEY,12000}, {TQ_HAPPY,1}, {TQ_PLUG,1} },                          // 4 you are Mamesy: hold on, and keep the hospital from pulling the plug
    { {TQ_CLEAN,10}, {TQ_FRIENDS,4}, {TQ_HOUSE,4}, {TQ_MONEY,40000}, {TQ_HAPPY,1} } };                     // 5 the comeback: all of it, together
static const u8 stqN[5]={3,4,5,5,5};
static u8 stqDay(void){ u8 b=(u8)(simDay&255); return b==255?254:b; }
static int stqDays(void){ return stKidDay==255?0:((simDay&255)-stKidDay)&255; }
static int stqVal(const StQ*q){
    switch(q->k){
    case TQ_CLEAN: case TQ_DAYS: return stqDays();
    case TQ_FRIENDS: { int me=hhPUid, ex=stCh==3?(int)stKid:(int)stPart, n=0; for(int u=0;u<HU_N;u++) if(u!=me&&u!=ex&&(relF[me][u]&RF_FRIEND)) n++; return n; }   // (Missy's sister does not count once she is gone, and in chapter 4 Missy is in a coma)
    case TQ_MONEY: return (int)simMoneyI();
    case TQ_HOUSE: { int n=hhN+1; if(stCh==4&&stPart!=255&&stMember(stPart)>=0) n--; return n; }
    case TQ_SECURE: { int c=secCount('n'), g=secCount('j'); return (c>2?2:c)+(g>2?2:g); }   // chapter 3: tight security = 2 SECURITY CAMERAS and 2 SECURITY GATES on the lot you are on
    case TQ_GUEST: return stGuest?1:0;
    case TQ_HAPPY: return moodState()>=MS_HAPPY;
    case TQ_PLUG: return plugV<=PLUG_OK;   // chapter 4: the hospital is held off (60 or less)
    }
    return 0;
}
static int stqOk(const StQ*q){ return stqVal(q)>=(int)q->n; }
static int stqDone(void){ if(stCh>=5) return 0; int n=0; for(int i=0;i<stqN[stCh];i++) n+=stqOk(&stqT[stCh][i]); return n; }
static int stqAll(void){ return stCh<5&&stqDone()==stqN[stCh]; }
static void stqText(const StQ*q,char*b){   // one goal line, b at least 44 long: "+ 5 OF 7 DAYS CLEAN" / "- 1 OF 4 FRIENDS"
    int v=stqVal(q); char*e=slCat(b,stqOk(q)?"+ ":"- ");
    if(q->k==TQ_GUEST||q->k==TQ_HAPPY){ slCat(e,stqNm[q->k]); return; }
    if(q->k==TQ_PLUG){ e=slCat(e,"PLUG PRESSURE "); e=slNum(e,plugV); slCat(e," OF 60 MAX"); return; }
    if(v>(int)q->n) v=(int)q->n;
    e=slNum(e,v); e=slCat(e," OF "); e=slNum(e,(int)q->n); e=slCat(e," "); slCat(e,stqNm[q->k]);
}
static void stqList(int x,int y,int dy){ if(stCh>=5) return; for(int i=0;i<stqN[stCh];i++){ char b[48]; stqText(&stqT[stCh][i],b); text(x,y+i*dy,b,b[0]=='+'?RGB(10,28,12):WHITE,1); } }
static void stTvEvent(int ev){   // sims.h simEventV calls this for every game event: a puff or a collapse in chapters 2, 3 and 5 starts the clock again
    if(stId==STY_TVSHOW&&stCh==3&&stKidDay!=255&&(ev==SE_FRIEND||ev==SE_BFF||ev==SE_PROMO)){   // step 4: good news eases the hospital
        int d=ev==SE_BFF?15:10; plugV=(u8)(plugV>d?plugV-d:0); lnote=ev==SE_PROMO?"A raise. Okafor eases off.":"A friend helps. Okafor eases."; lnoteT=90; }
    if(stId!=STY_TVSHOW||stKidDay==255||(stCh!=1&&stCh!=2&&stCh!=4)) return;
    if(ev!=SE_PIPE&&ev!=SE_FAINT&&ev!=SE_PASSOUT) return;
    stKidDay=stqDay(); stSave(); toast(ev==SE_PIPE?"You relapsed. The clock restarts.":"You collapsed. The clock resets.");
}
static int stValue(const StCh*c){   // the number a goal counts (-1: the goal has none)
    switch(c->goal){
        case SG_MONEY: return simMoneyI();      case SG_JOB: return jobLvl;      case SG_SKILL: return skillLvl;
        case SG_TRICKS: return simTricks;    case SG_WANTS: return simDone;   case SG_HOUSE: return hhN+1;
        case SG_DAYS: return stKidDay==255?0:((simDay&255)-stKidDay)&255;   // (the day the chapter began lives in stKidDay: no story has both this goal and a child)
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
        case SG_SCRIPT: return 0;   // (ended by its cutscene: stComplete)
        case SG_MULTI: return stqAll();   // (TV SHOW & TELL: every goal of the chapter at once)
        case SG_BFF: return pu>=0&&(relF[me][pu]&RF_BFF);
        case SG_SKILL: case SG_TRICKS: case SG_WANTS: case SG_HOUSE: case SG_FRIENDS: case SG_DAYS: { int v=stValue(c); return v>=(int)c->arg; }
    }
    return 0;
}
// STORY MISSIONS (roadmap #4): every story chapter finished, in any life, is counted once for good (the same chapter again adds nothing). The END card of a story is not a mission,
// so a story has 5. Half of all of them (the missions of every story in the game) unlock CLOSER TO THE END and TREE-AGE IN ACTION.
// Lives in story.h (it needs STY_N). Saved in the jukebox block at JB_OFF+40: 'M' 'S', 8 bytes (a bit per mission: story-1 * 5 + chapter, room for 12 stories), the bytes xor 0x5A (appended; nothing moved).
#define SM_PER 5
_Static_assert((STY_N-1)*SM_PER<=64,"the story mission bits (8 bytes) are full: 12 stories at most");
static int jbStoryCount(int story){   // how many missions of a story are done (read only; 0 when the block is missing or damaged)
    if(story<1||story>=STY_N) return 0;
    volatile u8*m=SRAM_BASE+JB_OFF+40; if(m[0]!='M'||m[1]!='S') return 0;
    u8 x=0x5A; for(int i=0;i<8;i++) x^=m[2+i]; if(m[10]!=x) return 0;
    int n=0; for(int ch=0;ch<SM_PER;ch++){ int b=(story-1)*SM_PER+ch; if(b<64) n+=(m[2+(b>>3)]>>(b&7))&1; } return n;
}
static int jbStoryDone(int story,int ch){   // 1 when this mission was the one that reached half of them and songs came free
    if(story<1||story>=STY_N||ch<0||ch>=SM_PER) return 0;
    volatile u8*m=SRAM_BASE+JB_OFF+40; u8 v[8]; u8 x=0x5A;
    for(int i=0;i<8;i++){ v[i]=(m[0]=='M'&&m[1]=='S')?m[2+i]:0; x^=v[i]; }
    if(m[0]=='M'&&m[1]=='S'&&m[10]!=x) for(int i=0;i<8;i++) v[i]=0;   // a damaged block starts again
    int b=(story-1)*SM_PER+ch; if(b<64) v[b>>3]|=(u8)(1<<(b&7));
    x=0x5A; for(int i=0;i<8;i++) x^=v[i];
    m[0]='M'; m[1]='S'; for(int i=0;i<8;i++) m[2+i]=v[i]; m[10]=x;
    int n=0; for(int i=0;i<(STY_N-1)*SM_PER&&i<64;i++) n+=(v[i>>3]>>(i&7))&1;
    if(n*2<(STY_N-1)*SM_PER) return 0;   // half of ALL the missions: it grows with every story added (10 stories = 50 missions = 25 needed)
    int a=jbUnlock(UL_CLOSER), t=jbUnlock(UL_TREE); return a|t;
}
static int rwTotal(void){   // story missions done in every life (the bits jbStoryDone writes): the REWARDS category of BUY opens with this count
    volatile u8*m=SRAM_BASE+JB_OFF+40; if(m[0]!='M'||m[1]!='S') return 0;
    u8 v[8], x=0x5A; for(int i=0;i<8;i++){ v[i]=m[2+i]; x^=v[i]; }
    if(m[10]!=x) return 0;
    int n=0; for(int i=0;i<(STY_N-1)*SM_PER&&i<64;i++) n+=(v[i>>3]>>(i&7))&1;
    return n;
}
static const char* stGotP[2]; static u8 stGotN;   // what the chapter just finished unlocked (names of a REWARDS item and / or a slider pack): the CHAPTER COMPLETE card lists them
static void stGotAdd(const char*nm){ if(stGotN<2) stGotP[stGotN++]=nm; }
static void stAnnounce(void){ static char t[52] EWRAM_BSS; /* was 44: the longest banner is 44 letters plus the end mark */ char*e=slCat(t,"CHAPTER "); e=slNum(e,stCh+1); e=slCat(e,"  "); slCat(e,stChs[stId][stCh].nm); lnote=t; lnoteT=240; }
static void stComplete(void){   // the current chapter is done: pay it, open what it unlocks and start the next one (stTick calls it when the goal is met; a scripted chapter's cutscene calls it too)
    if(!stId||stCh>=stLen[stId]||stChs[stId][stCh].goal==SG_END) return;
    const StCh*c=&stChs[stId][stCh];
    simMoneyAdd(stRew(stCh)); dnaAdd(25); persSave(); simsSave();
    stGotN=0; int rwWas=rwTotal(); int slkWas=jbStoryCount(stId); if(jbStoryDone(stId,stCh)){ simQPush("MORE SCOOBY STUFF TO FIND"); simQPush("TOUCH GRASS TO FIND IT"); }   // half of all the story missions: secret songs (no names, go and look)
    if(slkWas<SM_PER&&jbStoryCount(stId)>=SM_PER){ int p=slkGift(stId); if(p>=0){ static char sg[32] EWRAM_BSS; simCat(simCat(sg,slkNm[p])," UNLOCKED"); simQPush(sg); stGotAdd(slkNm[p]); } }   // all 5 missions of this story are done: a free slider pack
    for(int j=0;j<RW_N;j++) if(rwWas<rwNeed[j]&&rwTotal()>=rwNeed[j]){ simQPush("NEW REWARD IN BUY MODE"); stGotAdd(palNm[catItems[NCAT-1][j]]); break; }
    if(c->goal==SG_DAYS||c->goal==SG_MULTI){ stKidDay=255; if(c->goal==SG_MULTI) stGuest=0; }
    stCh++; stSave(); stAnnounce(); stShown=(u8)(stId*16+stCh+1); stModal=2;   // the CHAPTER COMPLETE card (stRunModal)
}
// TV SHOW & TELL, chapter 4 (YOU ARE MAMESY NOW): the CAST ARRIVAL and the hand-over of control.
// Cast arrival = a person the chapter needs moves in as a real Sim of the household (not only a figure in a cutscene). Here: Mamesy, Missy's sister.
// She comes with the chapter and you take over her; chapter 5 hands you back to Missy. This story has no partner and no kid, so its two story bytes hold the uids:
// stPart = Mamesy, stKid = Missy. Only the game's own switch is used (hhSwitchTo, as the HOUSEHOLD menu does). stTvWant (not saved) = 1 go to Mamesy, 2 back to Missy.
static u8 stTvWant, stTvWarn;
static int stTvCan(int m){ return m>=0&&!custom&&!(hhM[m].act==HA_AWAY&&!hhOnOtherFloor(m)); }   // the switch would work right now (else we ask again in a second, silently)
static void stTvControl(void){
    if(stId!=STY_TVSHOW||!stTvWant){ stTvWant=0; return; }
    if(stTvWant==1){   // chapter 4: be Mamesy
        if(stCh!=3){ stTvWant=0; return; }
        if(stPart!=255&&hhPUid==(int)stPart){ stTvWant=0; return; }   // already her
        int m=stPart!=255?stMember(stPart):-1;
        if(m<0){   // the cast arrives: a grown-up sister (half of her looks come from Missy)
            u8 lk2[LK_N], lk[LK_N], st=AG_ADULT; lookTrueRandom(lk2,&st); stMixLook(lk,look,lk2,AG_ADULT);
            m=stAddSim(lk,AG_ADULT,hhPLast);
            if(m<0){ if(!stTvWarn){ stTvWarn=1; toast("No room for Mamesy. Make some space."); } return; }
            { const char*nm="MAMESY"; int k=0; for(;nm[k]&&k<HH_NM-1;k++) hhM[m].name[k]=nm[k]; hhM[m].name[k]=0; }
            stPart=hhM[m].uid; stRel(hhPUid,hhM[m].uid,70,70,RF_FRIEND|RF_BFF); kin[hhPUid][hhM[m].uid]=KN_SISTER; kin[hhM[m].uid][hhPUid]=KN_SISTER;
            for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
            toast("One moment. Mamesy is moving in."); hhBakeAll(); hhSave(); liveInvalidate(); stSave();
        }
        if(!stTvCan(m)) return;
        int me=hhPUid; hhSwitchTo(m);
        if(hhPUid!=(int)stPart) return;   // it did not switch: try again later
        stKid=(u8)me; stTvWant=0;
    } else {   // later chapters: Missy again
        if(stCh<4||stPart==255||stKid==255||hhPUid!=(int)stPart){ stTvWant=0; return; }
        int m=stMember(stKid); if(m<0){ stTvWant=0; return; }
        if(!stTvCan(m)) return;
        hhSwitchTo(m);
        if(hhPUid!=(int)stKid) return;
        stTvWant=0;
    }
    stTvWarn=0; lnote=hhPName; lnoteT=90; liveInvalidate(); camSnap=1; stSave(); hhSave();
}
// TV SHOW & TELL step 3: the PAPARAZZI. Only in chapter 3, on the ground floor of the lot you stand on. Up to PAP_MAX photographers walk in from a way off the lot
// (twFar) and close in on you, a step every 1 to 2 seconds. A SECURITY CAMERA scares them: within 4 tiles of one they back away (it films them). A SECURITY GATE is a wall they
// cannot cross, so a line of gates keeps them out. Any that get within 5 tiles of you (and are not near a camera) flash: THE PAPARAZZI SNAP YOU and a small mood hit,
// at most once every 6 seconds. Nothing is saved: they are made again when the chapter is running.
static u8 papT, papKey, papCool, papW[PAP_MAX] EWRAM_BSS, papCN, papCX[8] EWRAM_BSS, papCY[8] EWRAM_BSS;
static int papNear(int x,int y,int*cx,int*cy){   // the nearest security camera within 4 tiles of (x,y): 1 and where, else 0
    int best=99; for(int i=0;i<papCN;i++){ int d=fxAbs(x-papCX[i])+fxAbs(y-papCY[i]); if(d<=4&&d<best){ best=d; *cx=papCX[i]; *cy=papCY[i]; } }
    return best<99;
}
static int papFree(int x,int y,int me){ if(!hhWalk(x,y)) return 0; for(int i=0;i<papN;i++) if(i!=me&&papX[i]==x&&papY[i]==y) return 0; return !(x==(int)(lfx>>8)&&y==(int)(lfy>>8)); }
static void papStep(int p,int tx,int ty,int away){   // one tile toward (or away from) a target, the longer way first, the other way if blocked
    int x=papX[p], y=papY[p], dx=tx-x, dy=ty-y; if(away){ dx=-dx; dy=-dy; }
    int sx=dx<0?-1:dx>0?1:0, sy=dy<0?-1:dy>0?1:0, ax=fxAbs(dx), ay=fxAbs(dy), tryX=ax>=ay;
    for(int pass=0;pass<2;pass++,tryX=!tryX){
        if(tryX&&sx&&papFree(x+sx,y,p)){ papX[p]=(u8)(x+sx); return; }
        if(!tryX&&sy&&papFree(x,y+sy,p)){ papY[p]=(u8)(y+sy); return; } }
    int d=rnd8()&3; if(papFree(x+hhDx[d],y+hhDy[d],p)){ papX[p]=(u8)(x+hhDx[d]); papY[p]=(u8)(y+hhDy[d]); }   // boxed in: a sidestep
}
static void papTick(void){   // once per logic step (stTick)
    int on=stId==STY_TVSHOW&&stCh==2&&!curFl&&!ldead&&nbOk;
    u8 key=(u8)(nbOk?nbT.cur+1:0);
    if(!on||key!=papKey){ papN=0; papKey=key; papCN=0; papT=0; if(!on) return; }
    int px=(int)(lfx>>8), py=(int)(lfy>>8), i, cx=0, cy=0;
    if(papCool) papCool--;
    for(i=0;i<papN;i++) if(papFl[i]) papFl[i]--;
    if(++papT>=60){ papT=0;   // once a second: where the cameras are, and maybe one more photographer
        papCN=0; for(int y=0;y<MH&&papCN<8;y++) for(int x=0;x<MW&&papCN<8;x++) if(lifeMap[y][x]=='n'){ papCX[papCN]=(u8)x; papCY[papCN]=(u8)y; papCN++; }
        if(papN<PAP_MAX&&(rnd8()&1)){ int a=twFar();
            if(a>=0){ int x=a%MW, y=a/MW; if(papFree(x,y,-1)&&!papNear(x,y,&cx,&cy)){ papX[papN]=(u8)x; papY[papN]=(u8)y; papFl[papN]=0; papW[papN]=(u8)(30+(rnd8()&31)); papN++;
                if(papN==1){ lnote="Cameras outside again."; lnoteT=90; } } } } }
    for(i=0;i<papN;i++){
        int x=papX[i], y=papY[i], d=fxAbs(x-px)+fxAbs(y-py), scared=papNear(x,y,&cx,&cy);
        if(papW[i]) papW[i]--; else { papW[i]=(u8)(45+(rnd8()&45));
            if(scared) papStep(i,cx,cy,1); else if(d>2) papStep(i,px,py,0); }
        if(!scared&&d<=5&&!papFl[i]&&(rnd8()&63)==0){ papFl[i]=10;
            if(!papCool){ papCool=240; moodEvent(M_SPOOK); lnote="They got a shot of you. Ugh."; lnoteT=70; } } }
}
static void plugTick(void){   // once per logic step (stTick): the hospital's pressure in chapter 4
    int on=stId==STY_TVSHOW&&stCh==3&&stKidDay!=255;
    if(!on){ plugV=PLUG_START; plugDay=0xFFFF; plugSec=0; plugTen=0; plugNight=0; return; }
    if(++plugSec<60) return; plugSec=0;
    static const StQ qF={TQ_FRIENDS,2}, qH={TQ_HAPPY,1}, qM={TQ_MONEY,12000};
    int old=plugV, v=plugV;
    if(plugDay==0xFFFF) plugDay=(u16)simDay;
    if((u16)simDay!=plugDay){ plugDay=(u16)simDay;   // a new day: Dr. Okafor calls about the life support
        v+=8; { static const char* const ok3[3]={"Okafor called. Bill's paid.","Bill paid. One more day.","Paid. Okafor sounds calmer."}, * const no3[3]={"You can't cover the bill.","Okafor called. No money.","Overdue bill. Okafor noticed."}; int r=rnd8()%3; if(simMoney>=PLUG_BILL){ simMoney-=PLUG_BILL; lnote=ok3[r]; } else { v+=20; lnote=no3[r]; } } lnoteT=120; }
    if(++plugTen>=10){ plugTen=0; v+=3-stqOk(&qF)-stqOk(&qH)-stqOk(&qM); }
    if(v>100) v=100;
    if(v>=100&&plugNight){ plugV=100; if(!stModal) stModal=3; return; }   // the second time there is no more night: the hospital pulls the plug (stModal 3: stRunModal0 plays the loss, then back to your last save)
    if(v>=100){   // the plug is nearly pulled: one more night, and everything you held on to starts over
        v=50; stKidDay=stqDay(); stSave(); if(simMoney>=1000) simMoney-=1000; plugNight=1; toast("One more night. The days reset."); lnote="There won't be another one."; lnoteT=200; }
    else if(old<90&&v>=90){ static const char* const l90[3]={"Last chance. Save Missy.","She's almost out of time.","Mamesy, hurry. Please."}; lnote=l90[rnd8()%3]; lnoteT=150; }
    else if(old<75&&v>=75){ static const char* const l75[3]={"Okafor's losing patience.","Okafor won't stop hinting.","The hospital wants an answer."}; lnote=l75[rnd8()%3]; lnoteT=150; }
    plugV=(u8)v;
}
static void stTick0(void){   // once per logic step in the life game: is this chapter done?
    static u8 cnt; if(!stId||++cnt<60) return; cnt=0;
    if(stCh>=stLen[stId]) return;
    for(int k=0;k<TW_N;k++) if(twOn[k]==2) stGuest=1;   // a neighbor is staying over (HAVE A NEIGHBOR OVER)
    const StCh*c=&stChs[stId][stCh];
    if(c->goal==SG_END) return;
    if(c->goal==SG_DAYS&&stKidDay==255){ u8 b=(u8)(simDay&255); stKidDay=b==255?254:b; stSave(); return; }   // the clock starts when the chapter does
    if(c->goal==SG_MULTI&&stKidDay==255){ stKidDay=stqDay(); stGuest=0; stSave(); return; }   // (the chapter clock starts with the chapter)
    if(c->goal==SG_KID){   // a day after the chapter starts the child comes home
        if(stKidDay==255){ stKidDay=(u8)((simDay+1)&255); stSave(); return; }
        if((u8)simDay!=stKidDay) return;
        stKidHome(); stKidDay=255;
    } else if(!stDone(c)) return;
    stComplete();
}
static void stTick(void){ papTick(); plugTick(); stTick0(); if(stTvWant&&stId==STY_TVSHOW){ static u8 tc; if(++tc>=60){ tc=0; stTvControl(); } } }   // (TV SHOW & TELL: keep asking while the hand-over is waiting)
static void stEnter(void){ stLoad(); if(stId){ stAnnounce(); if(stShown!=(u8)(stId*16+stCh+1)){ stShown=(u8)(stId*16+stCh+1); stModal=1; } } }   // (a chapter card once per chapter and power on)   // entering the life game: the current goal on the top bar
// ---- the look: Sims 2 / Life Stories panels (the pieces live in main.c next to HOW TO PLAY) ----
static void s2rr(int x,int y,int w,int h,u16 c); static void s2grad(int x,int y,int w,int h,int r0,int g0,int b0,int r1,int g1,int b1);
static void s2pill(int x,int y,int w,const char*s);
static void stBack(const char*title,int cnt){   // the backdrop, the frame, the title bar
    objHideAll();
    s2grad(0,0,SW,SH,1,4,10,2,9,17);
    for(int y=0;y<SH;y+=8) for(int x=(y&8)?4:0;x<SW;x+=8) rect(x,y,1,1,RGB(3,9,17));
    s2rr(1,1,238,158,RGB(10,20,30)); s2rr(2,2,236,156,RGB(2,6,13));
    s2grad(3,3,234,14,8,18,28,3,10,19); rect(3,17,234,1,RGB(14,26,31));
    text(11,7,title,WHITE,1);
}
static void stIcon(int s,int x,int y,int sc,u16 c){   // a little picture per story: a heart, a ring, a parent with a kid (9 x 8 pixels, drawn big)
    static const char* const pic[STY_N][8]={{0},
      {".XX...XX.","XXXX.XXXX","XXXXXXXXX","XXXXXXXXX",".XXXXXXX.","..XXXXX..","...XXX...","....X...."},
      {"....X....","...XXX...","....X....","..XXXXX..",".X.....X.",".X.....X.",".X.....X.","..XXXXX.."},
      {".XX......",".XX......","XXXX.....","XXXX.XX..",".XX..XX..",".XX.XXXX.",".XX..XX..","XXXX.X.X."},
      {".........",".........","XXXXXXXXX",".XXXXXXX.",".........",".XX...XX.",".XX...XX.","........."},
      {"....X....","...XXX...","..XXXXX..",".XXXXXXX.","XXXXXXXXX","XX.XXX.XX","XX.XXX.XX","XXXXXXXXX"},
      {".XX...XX.",".XX...XX.","XXXX.XXXX","XXXX.XXXX",".XX...XX.",".XX...XX.",".X.X.X.X.",".X.X.X.X."},
      {"..XXXXX..",".XXXXXXX.","XXXX.XXXX","XXX...XXX","XXXX.XXXX","XXXXXXXXX",".XXXXXXX.","..XXXXX.."},
      {"......XX.","......XX.","....XXXX.","....XX...","..XXXX...","..XX.....","XXXX.....","XXXX....."},
      {"XXXXXXX..","XXXXXXXX.","XXXXXXX..","....X....","....X....","....X....","....X....","...XXX..."},
      {".XX...XX.","XXXX.XXXX","XXXXX.XXX","XXXX.XXXX",".XXX.XXX.","..XX.XX..","...XXX...","....X...."},
      {"X.......X",".X.....X.","XXXXXXXXX","X.......X","X...X...X","X.......X","XXXXXXXXX",".X.....X."}};   // SECOND CHANCE: a cracked heart; TV SHOW & TELL: a television
    for(int r=0;r<8;r++) for(int q=0;q<9;q++) if(pic[s][r][q]=='X') rect(x+q*sc,y+r*sc,sc,sc,c);
}
static void stSparkle(u32 cnt,int x0,int y0,int w,int h){   // a few twinkling pixels (a cheap celebration)
    for(int i=0;i<10;i++){ u32 h1=(u32)(i*2654435761u)>>8; int x=x0+(int)(h1%(u32)w), y=y0+(int)((h1>>9)%(u32)h); int ph=(int)((cnt>>2)+(u32)i*5)&15;
        if(ph<4){ u16 c=ph<2?RGB(31,30,18):RGB(24,22,8); rect(x,y,1,1,c); if(ph==1){ rect(x-1,y,3,1,c); rect(x,y-1,1,3,c); } } }
}
// the chapter goal's progress as text (only the goals that have a number)
static int stProg(const StCh*c,char*b){
    if(c->goal==SG_MULTI){ if(stCh>=5) return 0; char*e=slNum(b,stqDone()); e=slCat(e," OF "); slNum(e,stqN[stCh]); return 1; }
    int v=stValue(c), of=c->arg; if(v<0) return 0;
    if(v>of) v=of;
    char*e=slNum(b,v); e=slCat(e," OF "); slNum(e,of); return 1;
}
static int storyPick(void); static void storySetup(int s);   // (below)
static void storyJoin(void){   // floors step 10: pause menu > STORY with no story: pick one for the life you are living now (nobody is wiped; the new housemate or kid just moves in)
    int s=storyPick(); if(!s) return;
    int need=(s==STY_ROOM||s==STY_WED||s==STY_PARENT||s==STY_SECOND||s==STY_HOUSE||s==STY_TOWN)?1:s==STY_FRIEND?2:0;   // Sims the story brings in
    if(hhN+need>HH_MAX){ toast("Too many people home for that story."); return; }
    static const char* const yn[2]={"START THIS STORY","NOT NOW"}; if(menu(stNm[s],yn,2)!=0) return;
    money_t money=simMoney; storySetup(s); if(s==STY_RAGS) simMoney=money;   // (RAGS TO RICHES keeps your money here: a new life is the way to start it poor)
    stEnter();   // the first chapter card
}
static void stqPage(void){   // the journal page of TV SHOW & TELL (L R): what this chapter asks, live
    if(stCh>=5){ text(14,23,"THE END, IS HERE TODAY",GOLD,1); return; }
    text(14,23,stChs[stId][stCh].nm,GOLD,1);
    text(14,34,stTvBrief[stCh][0],RGB(20,26,31),1); text(14,43,stTvBrief[stCh][1],RGB(20,26,31),1);
    rect(14,54,212,1,RGB(14,26,31)); text(14,58,"ALL OF THESE AT THE SAME TIME",RGB(17,29,31),1);
    stqList(14,70,10);
    if(stCh==1||stCh==2||stCh==4) text(14,124,"A PUFF OR A FAINT STARTS THE CLOCK AGAIN",RGB(31,20,22),1);
    text(14,136,"L OR R  BACK TO THE STORY",RGB(12,18,24),1);
}
static void rwPage(void){   // the REWARDS page of the journal (L R): what the story missions open in BUY mode, and the creator slider packs
    int tot=rwTotal(); char b[44]; char*e=slNum(b,tot); e=slCat(e," OF "); e=slNum(e,(STY_N-1)*SM_PER); slCat(e," STORY MISSIONS DONE");
    text(14,23,b,GOLD,1); text(14,33,"MISSIONS COUNT IN EVERY LIFE",RGB(17,29,31),1);
    for(int j=0;j<RW_N;j++){ int y=46+j*14, nd=rwNeed[j], open=sUnlock||tot>=nd;
        if(open){ rect(10,y+1,10,8,RGB(5,18,7)); rect(11,y+2,8,6,RGB(8,26,10)); text(12,y+2,"+",WHITE,1); } else { rect(10,y+1,10,8,RGB(7,14,22)); rect(11,y+2,8,6,RGB(2,6,13)); }
        text(25,y+2,palNm[catItems[NCAT-1][j]],open?RGB(10,22,12):WHITE,1);
        if(open) text(231-tw("OPEN",1),y+2,"OPEN",RGB(10,22,12),1); else { char*f=slNum(b,nd); slCat(f," MISSIONS"); text(231-tw(b,1),y+2,b,GOLD,1); } }
    text(14,106,"FINISH ALL 5 MISSIONS OF A STORY",WHITE,1); text(14,115,"FOR A FREE CREATOR SLIDER PACK",WHITE,1);
    { int n=0; for(int i=0;i<NSLK;i++) n+=(sUnlock||(slkUl>>i&1))?1:0; char*f=slNum(b,n); f=slCat(f," OF "); f=slNum(f,NSLK); slCat(f," SLIDER PACKS OPEN"); text(14,125,b,RGB(17,29,31),1); }
    text(14,136,"L OR R  BACK TO THE STORY",RGB(12,18,24),1);
}
static void storyScreen(void){   // pause menu > STORY: the story journal, a chapter timeline (no story yet: A picks one for this life)
    int pg=0;   // 0 the story, 1 the REWARDS list (L R)
    u16 prev=keyNow(); u32 cnt=0, lt=~0u;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_L|K_R|K_LEFT|K_RIGHT)){ pg=(pg+1)%(stId==STY_TVSHOW?3:2); lt=~0u; } if(!stId&&!pg&&(pr&K_A)){ storyJoin(); prev=keyNow(); lt=~0u; if(stId) stModal=0; continue; }   // (the card is shown now by the journal itself: no second one when you close it)
        if(pr&(K_A|K_B|K_START)) return;
        if(!pr&&(cnt>>3)==lt){ vsync(); continue; }   // idle: the picture on the screen is still right (the whole backdrop used to be redrawn every frame, so taps landed between polls and were lost)
        lt=cnt>>3;
        stBack(pg==2?"THIS CHAPTER":pg?"REWARDS":stId?"STORY JOURNAL":"STORY",(int)cnt);
        if(pg==2) stqPage(); else if(pg) rwPage(); else
        if(!stId){
            s2rr(8,24,224,60,RGB(10,20,30)); s2rr(9,25,222,58,RGB(2,6,13));
            text(16,32,"NO STORY RIGHT NOW",GOLD,1); text(16,46,"PICK ONE FOR THIS LIFE WITH A",WHITE,1); text(16,56,"OR PLAY  NEW GAME  STORY MODE",RGB(17,29,31),1);
            text(16,70,"ELEVEN STORIES  SIX CHAPTERS EACH",RGB(20,24,28),1);
        } else {
            int n=stLen[stId]-1;   // (the END row is not listed)
            stIcon(stId,9,21,2,RGB(31,20,22));   // the story's picture, its name and kind, and how far you are
            text(32,21,stNm[stId],GOLD,1); text(32,30,stTag[stId],RGB(17,29,31),1);
            { char b[24]; char*e=slCat(b,"CHAPTER "); e=slNum(e,stCh+1>n?n:stCh+1); e=slCat(e," OF "); slNum(e,n); text(233-tw(b,1),21,b,WHITE,1);
              int bw=84, fx=233-bw; rect(fx-1,31,bw+2,7,RGB(14,26,31)); rect(fx,32,bw,5,RGB(3,5,9)); int f=bw*(stCh>n-1?n-1:stCh)/(n>1?n-1:1); if(f>bw) f=bw; if(f>0){ s2grad(fx,32,f,5,10,26,12,6,18,8); rect(fx,32,f,1,RGB(18,31,20)); } }
            for(int i=0;i<n;i++){ int y=44+i*13, st=i<stCh?2:i==stCh?1:0; const StCh*c=&stChs[stId][i];
                if(i+1<n) rect(14,y+9,2,4,st==2?RGB(8,26,10):RGB(7,14,22));   // the line down to the next chapter
                if(st==2){ rect(10,y+1,10,8,RGB(5,18,7)); rect(11,y+2,8,6,RGB(8,26,10)); text(12,y+2,"+",WHITE,1); }   // done: a green tick
                else if(st==1){ int g=(cnt>>3)&1; rect(10,y+1,10,8,g?RGB(31,26,6):RGB(24,19,3)); rect(11,y+2,8,6,RGB(3,5,9)); rect(13,y+4,4,2,GOLD); }   // now: a gold blinking ring
                else { rect(10,y+1,10,8,RGB(7,14,22)); rect(11,y+2,8,6,RGB(2,6,13)); }
                if(st==1){ s2grad(23,y,208,11,6,16,26,3,9,17); }
                text(25,y+2,c->nm,st==2?RGB(10,22,12):st==1?WHITE:RGB(12,18,24),1);
                if(st==1){ char b[16]; if(stProg(c,b)) text(231-tw(b,1),y+2,b,GOLD,1); }
                else if(c->goal!=SG_END){ char b[12]; char*e=slCat(b,"+"); slNum(e,stRew(i)); text(231-tw(b,1),y+2,b,st==2?RGB(10,22,12):RGB(12,18,24),1); } }   // what each chapter pays
        }
        if(stId) s2pill(5,147,60,"A OR B BACK"); else { int pw=tw("A PICK A STORY",1)+10; s2pill(5,147,pw,"A PICK A STORY"); s2pill(9+pw,147,tw("B BACK",1)+10,"B BACK"); }
        if(stId){ char b[34]; int tot=0; for(int i=0;i<stCh;i++) tot+=stRew(i); char*e=slCat(b,"EARNED "); e=slNum(e,tot); slCat(e," SIMOLEONS"); s2pill(69,147,tw(b,1)+10,b); }
        present();
    }
}
// CHAPTER 4 LOST: the plug pressure hit 100 with no night left. Missy dies (cutscene 6), a FAILED card says what happens, then play ends and the LAST SAVE FILE comes back:
// sgDiscard=2 makes sgLeaveSave (savegame.h) reload the file even with AUTO saving. No save file yet (sgPid 0): nothing to go back to, so the chapter starts over where you stand.
static void stPlugLose(void){
    csPlay(6); plugV=PLUG_START; plugDay=0xFFFF; plugSec=plugTen=0; plugNight=0;
    int back=sgPid!=0; u16 prev=keyNow(); u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_A|K_B|K_START)) break;
        stBack("CHAPTER 4  TOO LATE",(int)cnt);
        s2rr(8,22,224,116,RGB(16,27,31)); s2rr(9,23,222,114,RGB(2,6,13)); s2grad(10,24,220,112,3,9,19,1,4,10);
        stIcon(stId,16,32,4,RGB(31,20,22));
        text(60,30,stNm[stId],GOLD,1); text(60,40,"...THE LOSER HAS TO FALL",RGB(17,29,31),1);
        text(18,62,"She's gone.",RGB(31,8,8),2);
        text(18,84,"The hospital pulled the plug.",WHITE,1);
        text(18,98,back?"Deep breath. Back to your save.":"Deep breath. Let's try again.",RGB(20,26,31),1);
        if(back) text(18,110,"Whatever you did since is lost.",RGB(20,26,31),1);
        s2pill(5,147,40,"A OK");
        present();
    }
    if(back) sgDiscard=2; else { stKidDay=stqDay(); stGuest=0; stSave(); toast("No save to go back to. Try again."); }
}
// the chapter cards: CHAPTER n (a chapter starts) and CHAPTER COMPLETE (a chapter was done). Shown by lifeModeRun like the pause menu.
static void stRunModal0(void){
    int kind=stModal; stModal=0; if(!stId) return;
    if(kind==3){ stPlugLose(); return; }   // (chapter 4 lost)
    if(stId==STY_TVSHOW&&kind==1&&stCh==0&&stKidDay==255){ csPlay(7); stKidDay=stqDay(); stGuest=0; stSave(); }   // a brand new TV SHOW & TELL: the night it all started plays once, before chapter 1 (the chapter clock starts here, which is also what stops it playing again)
    if(stId==STY_TVSHOW&&kind==2&&stCh>=1&&stCh<=5) { csPlay(stCh==5?5:stCh-1); if(stCh==4) csPlay(4); }   // the scene that closes the chapter just finished (cutscene.h; chapter 5 closes with scene 5, its opening news is scene 4)
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
        if(kind==2&&stGotN){ static char gb[60] EWRAM_BSS; char*e=simCat(gb,"UNLOCKED  "); e=simCat(e,stGotP[0]); if(stGotN>1){ e=simCat(e,"  AND  "); simCat(e,stGotP[1]); } if(tw(gb,1)>206) simCat(gb,"UNLOCKED  2 NEW THINGS"); text(16,77,gb,GOLD,1); }   // (what this chapter opened: a BUY reward and / or a slider pack)
        rect(14,84,212,1,RGB(14,26,31));
        text(16,90,kind==2?(end?"THE END, IS HERE TODAY":"NEXT CHAPTER"):(end?"THE END, IS HERE TODAY":c->goal==SG_MULTI?"ALL OF THESE AT ONCE":"YOUR GOAL"),GOLD,1);
        if(c->goal!=SG_MULTI||end) text(16,102,end?(kind==2?"YOUR STORY GOES ON  KEEP PLAYING":"YOUR STORY GOES ON  KEEP PLAYING"):c->nm,WHITE,1);
        if(!end&&c->goal!=SG_MULTI){ char b[16]; if(stProg(c,b)) text(16,112,b,RGB(20,26,31),1); }
        if(stId==STY_TVSHOW&&!end&&stCh<5) stqList(16,99,8);   // (the goals of the chapter, ticked as they hold)   // (what this chapter is about)
        s2pill(5,147,40,"A OK");
        present();
    }
}
static void stRunModal(void){   // the card, then the TV SHOW & TELL hand-over of control (chapter 4 starts as Mamesy, chapter 5 as Missy again)
    int kind=stModal; stRunModal0(); if(kind==3) return;   // (3 = chapter 4 lost: lifeModeRun leaves play when sgDiscard is 2)
    if(stId!=STY_TVSHOW) return;
    if(stCh==3&&(stPart==255||hhPUid!=(int)stPart)&&(kind==2||stKid==255)) stTvWant=1;
    else if(stCh>=4&&stPart!=255&&stKid!=255&&hhPUid==(int)stPart&&(kind==2||stCh==4)) stTvWant=2;
    stTvControl();
}
// STORY CAST (story step 1): STORY MODE does not make you build a Sim. Every story has its own pre-made lead, who is who you play (the creator is for CREATE A BORE).
// One row per story, in the order of the STY_ enum; the last name comes from the family name like a pre-made family (a trailing S is dropped, so these end in a letter that is not S).
//   look: SHAPE SKIN EYES MOUTH EARS HSTYLE HCOL TOP BOT  TONE EARSZ EARLF
static const HhFam stLead[STY_N]={
    {"",0},
    {"THE PARKER",1,{ {"JESS", {4,3,2,1,1,2,2,6,0, 0,0,0},AG_ADULT,AS_PLEAS, 6} }},   // ROOMMATES
    {"THE KOWALSKI",1,{ {"NICK", {5,2,1,1,1,0,3,3,2, 0,0,0},AG_ADULT,AS_HOME, 1} }},   // NEWLYWEDS
    {"THE BRENNAN",1,{ {"MARA", {4,1,2,2,1,1,4,5,1, 0,0,0},AG_ADULT,AS_HOME, 2} }},   // SINGLE PARENT
    {"THE VALDEZ",1,{ {"DEX",  {6,1,6,2,1,2,6,1,3, 0,0,0},AG_ADULT,AS_POP,  4} }},   // SKATE LIFE
    {"THE OKAFOR",1,{ {"BRAM", {3,2,3,1,1,3,1,2,4, 0,0,0},AG_ADULT,AS_PLEAS,9} }},   // HOUSEFULL
    {"THE LINDQVIST",1,{ {"SAM",  {1,4,3,1,2,4,5,5,2, 0,0,0},AG_ADULT,AS_KNOW, 8} }},   // BEST FRIENDS
    {"THE QUINN",1,{ {"PENNY",{0,2,1,2,2,1,3,2,5, 0,0,0},AG_ADULT,AS_FORTUNE,9} }},   // RAGS TO RICHES
    {"THE HARLOW",1,{ {"WES",  {5,6,2,0,1,2,0,1,1, 0,0,0},AG_ADULT,AS_FORTUNE,0} }},   // CAREER CLIMBER
    {"THE MORENO",1,{ {"ROSA", {4,5,0,1,2,5,2,6,3, 0,0,0},AG_ADULT,AS_POP,  3} }},   // NEW IN TOWN
    {"THE ASHBY",1,{ {"LEO",  {5,0,2,3,1,0,1,3,1, 0,0,0},AG_ADULT,AS_HOME, 5} }},   // SECOND CHANCE
    {"THE TELLER",1,{ {"MISSY",{[LK_SHAPE]=4,[LK_SKIN]=1,[LK_EYES]=0,[LK_MOUTH]=4,[LK_EARS]=1,[LK_HSTYLE]=9,[LK_HCOL]=0,[LK_TOP]=4,[LK_BOT]=0,[LK_BROW]=1,[LK_GLASS]=1,[LK_EYECOL]=2},AG_ADULT,AS_POP,  3} }},   // TV SHOW & TELL
};
static int storyLead(int s){   // the story's lead becomes you (the same hand-over as A PRE-MADE FAMILY: they move in, you take their place, who you were leaves). 0 = it did not fit
    if(s<1||s>=STY_N) return 0;
    if(hhMoveIn(&stLead[s])>0){ hhSwap(&hhM[0]); hhRemove(0); return 1; }
    return 0;
}
// NEW GAME > STORY MODE: pick a story on a story card (LEFT RIGHT to flip through them, A to start)
static int storyPick(void){
    int sel=1; u16 prev=keyNow(); u32 cnt=0, lt=~0u;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_RIGHT|K_R|K_DOWN)) sel=sel%(STY_N-1)+1;
        if(pr&(K_LEFT|K_L|K_UP)) sel=(sel+STY_N-3)%(STY_N-1)+1;
        if(pr&K_A) return sel;
        if(pr&(K_B|K_START)) return 0;
        if(!pr&&(cnt>>3)==lt){ vsync(); continue; }   // idle: the picture on the screen is still right (the whole backdrop used to be redrawn every frame, so taps landed between polls and were lost)
        int full=lt==~0u; lt=cnt>>3;
        if(full) stBack("WHICH STORY?",(int)cnt);   // the backdrop and title bar only once: a flip redraws just the card (it paints over the old one)
        s2rr(8,21,224,126,RGB(16,27,31)); s2rr(9,22,222,124,RGB(2,6,13)); s2grad(10,23,220,122,3,9,19,1,4,10);
        s2rr(14,27,44,40,RGB(10,20,30)); s2grad(15,28,42,38,7,16,26,3,8,16); stIcon(sel,18+((cnt>>4)&1),33,4,RGB(31,20,22));   // the story's picture (it beats slowly)
        // layout (the 3 blurb lines used to run into the divider and THE CHAPTERS): title 27, kind 40, blurb 49/57/65 (ends 72), divider 75, header 78, chapters 88 + 8 a row (ends 136), dots 139
        text(64,27,stNm[sel],GOLD,2); text(64,40,stTag[sel],RGB(17,29,31),1);
        for(int i=0;i<3;i++) text(64,49+i*8,stBlurb[sel][i],WHITE,1);
        rect(14,75,212,1,RGB(14,26,31)); text(16,78,"THE CHAPTERS",RGB(17,29,31),1);
        { char b[32]; int pay=0; for(int i=0;i<stLen[sel]-1;i++) pay+=stRew(i); int dn=jbStoryCount(sel); char*e=slNum(b,dn); e=slCat(e," OF 5 DONE  PAYS "); slNum(e,pay); text(226-tw(b,1),78,b,dn>=SM_PER?GOLD:RGB(20,26,31),1); }   // missions finished in any life, and what the story pays in all
        for(int i=0;i<stLen[sel]-1;i++){ char b[44]; char*e=slNum(b,i+1); e=slCat(e,"  "); slCat(e,stChs[sel][i].nm); text(16,88+i*8,b,i==stLen[sel]-2?GOLD:RGB(24,27,30),1); }
        for(int i=1;i<STY_N;i++){ int x=(SW-((STY_N-2)*12+8))/2+(i-1)*12; s2rr(x,139,8,4,i==sel?GOLD:RGB(7,14,22)); }   // which of the stories this is
        text(14,138,"<",GOLD,1); text(223,138,">",GOLD,1);
        { static const char* const bt[3]={"LEFT RIGHT STORY","A START","B BACK"}; int x=5;   // the buttons are as wide as their words (they were fixed widths, and LEFT RIGHT STORY ran into A START)
          for(int i=0;i<3;i++){ int w=tw(bt[i],1)+10; s2pill(x,147,w,bt[i]); x+=w+4; } }
        present();
    }
}
// STORY CAST (story step 2): the people a story brings home are pre-made, named characters too (last = 0: they take your last name).
typedef struct { const char* last; HhPre p; } StCast;
//    look: SHAPE SKIN EYES MOUTH EARS HSTYLE HCOL TOP BOT  TONE EARSZ EARLF
static const StCast stCast[STY_N][2]={
    {{0}},
    {{"DUNMORE", {"CAL",   {0,2,3,1,2,4,1,5,2, 0,0,0},AG_ADULT,AS_KNOW,   8}}},   // ROOMMATES: the roommate you barely know
    {{0,         {"ELI",   {4,4,2,1,2,2,5,6,0, 0,0,0},AG_ADULT,AS_FORTUNE,6}}},   // NEWLYWEDS: your spouse
    {{0,         {"JUNIE", {0,1,1,2,2,1,4,5,5, 0,0,0},AG_CHILD,AS_GROW,   2}}},   // SINGLE PARENT: your kid
    {{0}}, // SKATE LIFE: just you
    {{"BELOV",   {"IVAN",  {6,0,4,2,1,3,0,2,6, 0,0,0},AG_ADULT,AS_KNOW,   5}}},   // HOUSEFULL: a housemate
    {{"TANAKA",  {"KIKI",  {1,3,5,3,2,6,2,5,1, 0,0,0},AG_ADULT,AS_POP,    7}},    // BEST FRIENDS: two housemates
     {"OKONKWO", {"RUSS",  {3,6,0,0,1,1,3,1,7, 0,0,0},AG_ADULT,AS_HOME,   4}}},
    {{0}}, // RAGS TO RICHES: just you
    {{0}}, // CAREER CLIMBER: just you
    {{"ABERNATHY",{"GWEN", {4,2,1,1,2,0,6,7,1, 0,0,0},AG_ADULT,AS_PLEAS,  11}}},   // NEW IN TOWN: a housemate
    {{0,         {"VAL",   {5,3,2,3,1,5,0,6,4, 0,0,0},AG_ADULT,AS_POP,    10}}},   // SECOND CHANCE: the one you fell out with
    {{0}}, // TV SHOW & TELL: the cast arrives with the chapters
};
static int stCastSim(const StCast*c){   // one of the story's people moves in (their look, name, aspiration and personality). -1 = the house is full
    int m=stAddSim(c->p.look,c->p.stage,c->last); if(m<0) return -1;
    HhSim*s=&hhM[m]; int k=0; for(;c->p.name[k]&&k<HH_NM-1;k++) s->name[k]=c->p.name[k]; s->name[k]=0;
    s->asp=(u8)(c->p.stage<AG_ADULT?AS_GROW:c->p.asp); for(int i=0;i<TR_N;i++) s->tr[i]=signTr[c->p.sign][i];
    return m;
}
static void storySetup(int s){   // after the new life is set up and the old household has gone
    stId=(u8)s; stCh=0; stPart=stKid=stKidDay=255; stGuest=0;
    int m;
    switch(s){
    case STY_ROOM: m=stCastSim(&stCast[s][0]); if(m>=0){ stRel(hhPUid,hhM[m].uid,10,0,0); stPart=hhM[m].uid; } break;
    case STY_WED: m=stCastSim(&stCast[s][0]);
        if(m>=0){ stRel(hhPUid,hhM[m].uid,70,80,RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF); stPart=hhM[m].uid; } break;
    case STY_PARENT: m=stCastSim(&stCast[s][0]); if(m>=0){ stRel(hhPUid,hhM[m].uid,40,30,0); stKid=hhM[m].uid; } break;
    case STY_SKATE: break;   // just you and a board
    case STY_RAGS: simMoney=2000; break;   // you start with almost nothing
    case STY_SECOND: m=stCastSim(&stCast[s][0]);   // someone you fell out with (they start cold)
        if(m>=0){ stRel(hhPUid,hhM[m].uid,-20,-10,0); stPart=hhM[m].uid; } break;
    case STY_CLIMB: break;   // just you and a job to climb
    case STY_TVSHOW: break;   // just you (the rest of the cast arrives with the chapters)
    case STY_HOUSE: case STY_FRIEND: case STY_TOWN:   // housemates who are not friends yet (BEST FRIENDS: two of them)
        for(int i=0;i<(s==STY_FRIEND?2:1);i++){ m=stCastSim(&stCast[s][i]); if(m>=0){ stRel(hhPUid,hhM[m].uid,20,0,0); if(!i) stPart=hhM[m].uid; } }
        break;
    }
    hhSave(); stSave();
}
