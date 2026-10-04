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
enum { STY_NONE, STY_ROOM, STY_WED, STY_PARENT, STY_N };
enum { SG_FRIEND, SG_LOVE, SG_STEADY, SG_JOB, SG_MONEY, SG_KID, SG_KIDFRIEND, SG_GUEST, SG_END };
typedef struct { const char* nm; u8 goal; u16 arg; } StCh;
static const StCh stRoom[]={ {"BECOME FRIENDS WITH YOUR ROOMMATE",SG_FRIEND,0}, {"FALL IN LOVE",SG_LOVE,0}, {"GO STEADY",SG_STEADY,0},
    {"GET A PROMOTION",SG_JOB,1}, {"A CHILD COMES HOME",SG_KID,0}, {"THE END  A FAMILY OF YOUR OWN",SG_END,0} };
static const StCh stWed[]={ {"GET A PROMOTION",SG_JOB,1}, {"SAVE 1000 SIMOLEONS",SG_MONEY,1000}, {"A CHILD COMES HOME",SG_KID,0},
    {"BECOME YOUR KID'S FRIEND",SG_KIDFRIEND,0}, {"SAVE 2500 SIMOLEONS",SG_MONEY,2500}, {"THE END  HAPPY EVER AFTER",SG_END,0} };
static const StCh stPar[]={ {"GET A PROMOTION",SG_JOB,1}, {"BECOME YOUR KID'S FRIEND",SG_KIDFRIEND,0}, {"SAVE 800 SIMOLEONS",SG_MONEY,800},
    {"HAVE A NEIGHBOR OVER",SG_GUEST,0}, {"GET ANOTHER PROMOTION",SG_JOB,2}, {"THE END  YOU MADE IT WORK",SG_END,0} };
static const StCh* const stChs[STY_N]={0,stRoom,stWed,stPar};
static const u8 stLen[STY_N]={0,6,6,6};
static const char* const stNm[STY_N]={"","ROOMMATES","NEWLYWEDS","SINGLE PARENT"};
static const char* const stAbout[STY_N]={"","A NEW ROOMMATE  AND MAYBE MORE","JUST MARRIED  A FAMILY TO START","YOU AND YOUR KID  ON YOUR OWN"};
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
    }
    return 0;
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
    simMoney+=250; if(simMoney>9999) simMoney=9999; dnaAdd(25); persSave(); simsSave();
    { static char t[40]; char*e=slCat(t,"CHAPTER DONE  "); slCat(e,"+\xC2\xA7" "250  +25 JENES"); toast(t); }
    stCh++; stSave(); stAnnounce();
    if(stChs[stId][stCh].goal==SG_END) toast("THE END  YOUR STORY GOES ON");
}
static void stEnter(void){ stLoad(); if(stId) stAnnounce(); }   // entering the life game: the current goal on the top bar
static void storyScreen(void){   // pause menu > STORY
    static char ln[10][40] EWRAM_BSS; const char* L[10]; int n=0;   // (EWRAM: IWRAM holds the stack)
    if(!stId){ static const char* const none[3]={">NO STORY","START ONE FROM THE MAIN MENU","PLAY > NEW GAME > STORY MODE"}; helpScreen("STORY",none,3); return; }
    { char*e=slCat(ln[n],">"); slCat(e,stNm[stId]); L[n]=ln[n]; n++; }
    for(int i=0;i<stLen[stId]&&n<10;i++){ char*e=slCat(ln[n],i<stCh?"DONE  ":i==stCh?"NOW   ":"      "); slCat(e,stChs[stId][i].nm); L[n]=ln[n]; n++; }
    helpScreen("STORY",L,n);
}
// NEW GAME > STORY MODE: pick a story; the household starts as the story says (the creator opens next to make you)
static int storyPick(void){ const char* it[STY_N-1]; for(int i=1;i<STY_N;i++) it[i-1]=stNm[i]; int c=menu("WHICH STORY?",it,STY_N-1); return c<0?0:c+1; }
static void storySetup(int s){   // after the new life is set up and the old household has gone
    stId=(u8)s; stCh=0; stPart=stKid=stKidDay=255; stGuest=0;
    u8 lk[LK_N], st;
    if(s==STY_ROOM){ st=AG_ADULT; lookTrueRandom(lk,&st); char l[HH_NM]; famLast(&hhFams[rnd8()%HH_NFAM],l);
        int m=stAddSim(lk,AG_ADULT,l); if(m>=0){ stRel(hhPUid,hhM[m].uid,10,0,0); stPart=hhM[m].uid; } }
    else if(s==STY_WED){ st=AG_ADULT; lookTrueRandom(lk,&st);
        int m=stAddSim(lk,AG_ADULT,hhPLast); if(m>=0){ stRel(hhPUid,hhM[m].uid,70,80,RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF); stPart=hhM[m].uid; } }
    else { stMixLook(lk,look,look,AG_CHILD);   // your kid takes after you
        int m=stAddSim(lk,AG_CHILD,hhPLast); if(m>=0){ stRel(hhPUid,hhM[m].uid,40,30,0); stKid=hhM[m].uid; } }
    hhSave(); stSave();
}
