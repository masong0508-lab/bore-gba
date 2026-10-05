// family.h - FAMILY LIFE: weddings, babies (and twins), and a household that grows up around you.
//   PROPOSE         (social, house.h) adults going steady: a yes marries them (RF_MARRIED: WIFE / HUSBAND / SPOUSE), the one asked takes
//                   the asker's last name
//   TRY FOR A BABY  (social) a married or steady couple of adults, room in the house, no baby on the way already: a yes means a baby
//                   arrives 3 days later (at midnight), a mix of both parents (stMixLook) with a gender of its own (sexRoll)
//   TWINS           OPTIONS > SIM > SIMS > TWINS: NEVER / SOMETIMES (1 in 6) / OFTEN (1 in 3) / ALWAYS. Only when the house has room for two.
//                   Half the twins are identical (one look, one gender), half fraternal (each their own mix and gender)
//   GROWING UP      every member ages at midnight as you do (ageTick): the days on OPTIONS > TIME > AGES, scaled by AGING. Babies are looked
//                   after (their needs never sink below 60) and only crawl about and coo
// Saved at FAM_OFF (16 bytes): 'F' 'Y', the days each member has lived in their stage (by place in the household, as hhSave writes them),
// the days until the baby comes (255: none), its parents (0 = you, 1.. = members by place, 255 = nobody), the household block's checksum
// and size it belongs to, a spare byte, checksum. hhSave writes it, hhLoad reads it: a household from a slot or the bank that does not
// match starts with everyone fresh in their stage. The household bank (households.h) keeps it with the household.
// Needs before it: house.h (hhM, relF, famAge..), sims.h (simDay, oStageDays), story.h (stMixLook, stRel), the UI kit.
_Static_assert(STORY_OFF+8<=FAM_OFF&&FAM_OFF+16<=SLOT_DIR,"the family block overlaps the story block or the slot directory");
_Static_assert(HH_MAX<=7,"the family block keeps 7 members' ages");
static u8 famSum(volatile u8*m){ u8 s=0x46; for(int i=2;i<15;i++) s=(u8)(s*3+m[i]); return s; }
static int famPlace(int u){ if(u==hhPUid) return 0; int m=hhMemOf(u); return m<0?255:m+1; }   // a uid as the block keeps it
static int famUid(int p){ return p==0?hhPUid:p<=hhN?hhM[p-1].uid:255; }
static void famReset(void){ for(int u=0;u<HU_N;u++) famAge[u]=0; famDue=famPa=famPb=255; }
static void famSave(u8 hsum){   // (from hhSave, with the checksum of the household it just wrote)
    volatile u8*m=SRAM_BASE+FAM_OFF;
    for(int i=0;i<7;i++) m[2+i]=i<hhN?famAge[hhM[i].uid]:0;
    m[9]=famDue; m[10]=(u8)(famDue==255?255:famPlace(famPa)); m[11]=(u8)(famDue==255?255:famPlace(famPb));
    m[12]=hsum; m[13]=(u8)hhN; m[14]=0; m[0]='F'; m[1]='Y'; m[15]=famSum(m);
}
static void famLoad(int hsum){   // (from hhLoad: hsum is the checksum of the household it read, -1 = none)
    volatile u8*m=SRAM_BASE+FAM_OFF; famReset();
    if(hsum<0||m[0]!='F'||m[1]!='Y'||m[15]!=famSum(m)||m[12]!=(u8)hsum||m[13]!=hhN) return;   // not this household's
    for(int i=0;i<hhN;i++) famAge[hhM[i].uid]=m[2+i];
    if(m[9]!=255&&m[9]<=9){ famDue=m[9]; famPa=(u8)famUid(m[10]); famPb=(u8)famUid(m[11]); if(famPa==255&&famPb==255) famDue=255; }
}
static void famForget(int u){   // u moved out: their days go, and a baby they were expecting comes with the other parent (or not at all)
    if(u<0||u>=HU_N) return; famAge[u]=0;
    if(famPa==u) famPa=255; if(famPb==u) famPb=255; if(famPa==255&&famPb==255) famDue=255;
}
static const char* famLastOf(int u){ int m=hhMemOf(u); return m<0?hhPLast:hhM[m].last; }
static void famWed(int a,int b){   // a proposed and b said yes
    relF[a][b]|=RF_MARRIED|RF_STEADY; relF[b][a]|=RF_MARRIED|RF_STEADY;
    { const char*l=famLastOf(a); char t[HH_NM]; int k=0; for(;l[k]&&k<HH_NM-1;k++) t[k]=l[k]; t[k]=0;   // b takes a's last name
      int m=hhMemOf(b); char*d=m<0?hhPLast:hhM[m].last; if(t[0]) for(int i=0;i<HH_NM;i++) d[i]=t[i]; }
    if(a==hhPUid||b==hhPUid){ simEvent(SE_STEADY); simQueue("JUST MARRIED"); }
    else { int m=hhMemOf(a); if(m>=0) hhNote(&hhM[m]," GOT MARRIED"); }
}
static void famTry(int a,int b){   // a couple said yes to a baby: it comes in 3 days
    if(famDue!=255) return;
    famDue=3; famPa=(u8)a; famPb=(u8)b;
    if(a==hhPUid||b==hhPUid) simQueue("A BABY IS ON THE WAY");
    else { int m=hhMemOf(a); if(m>=0) hhNote(&hhM[m]," IS EXPECTING"); }
}
static void famLookOf(int u,u8*out){ const u8*l=fkLook(u); for(int i=0;i<LK_N;i++) out[i]=l[i]; }
static int famBaby(const u8*lk,int pa,int pb){   // one baby moves in: its place, -1 = no room
    int m=stAddSim(lk,AG_BABY,famLastOf(pa!=255?pa:pb)); if(m<0) return -1;
    int u=hhM[m].uid; famAge[u]=0; hhM[m].asp=AS_GROW;
    if(pa!=255) stRel(pa,u,60,50,RF_FRIEND); if(pb!=255) stRel(pb,u,60,50,RF_FRIEND);   // and the parents love them
    return m;
}
static char famMsg[48] EWRAM_BSS;
static int famBirth(void){   // the due day: one baby, or two. 1 = someone was born
    int pa=famPa, pb=famPb; famDue=famPa=famPb=255;
    if(pa==255&&pb==255) return 0;
    u8 la[LK_N], lb[LK_N], lk[LK_N];
    famLookOf(pa!=255?pa:pb,la); famLookOf(pb!=255?pb:pa,lb);
    static const u8 tw[4]={0,43,85,255};   // TWINS: NEVER, SOMETIMES (1 in 6), OFTEN (1 in 3), ALWAYS
    int two=hhN+2<=HH_MAX&&xo[XO_TWINS]&&rnd8()<tw[xo[XO_TWINS]], same=two&&(rnd8()&1);
    stMixLook(lk,la,lb,AG_BABY); lk[LK_SEX]=sexRoll();
    int m1=famBaby(lk,pa,pb); if(m1<0){ toast("THE HOUSE IS FULL  NO ROOM FOR A BABY"); return 0; }
    int m2=-1;
    if(two){ if(!same){ stMixLook(lk,la,lb,AG_BABY); lk[LK_SEX]=sexRoll(); }   // fraternal: a mix of their own; identical: the same look and gender
        m2=famBaby(lk,pa,pb); }
    if(m2>=0){ int a=hhM[m1].uid, b=hhM[m2].uid; stRel(a,b,60,60,RF_FRIEND); }   // twins: close from the start
    char*e=famMsg;
    if(m2>=0){ e=simCat(e,same?"IDENTICAL TWINS  ":"TWINS  "); e=simCat(e,hhM[m1].name); e=simCat(e," AND "); e=simCat(e,hhM[m2].name); }
    else { int sx=hhM[m1].look[LK_SEX]; e=simCat(e,sx==SX_FEMALE?"IT'S A GIRL  ":sx==SX_MALE?"IT'S A BOY  ":"A NEW BABY  "); e=simCat(e,hhM[m1].name); e=simCat(e," IS BORN"); }
    if(pa==hhPUid||pb==hhPUid) moodEvent(M_WANT);
    return 1;
}
static const char* const famGrewW[AG_N]={"","IS A CHILD NOW","IS A TEEN NOW","IS ALL GROWN UP","IS AN ELDER NOW"};
static char famGrowB[40] EWRAM_BSS;
static void famGrow(HhSim*s){   // a member's birthday: the next stage, the look fitted to it
    u8 sl[LK_N], ss=stage; for(int i=0;i<LK_N;i++){ sl[i]=look[i]; look[i]=s->look[i]; }
    stage=(u8)(s->stage+1); fixLook(); for(int i=0;i<LK_N;i++) s->look[i]=look[i]; s->stage=stage;
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=ss;
    if(s->stage>=AG_TEEN&&s->asp>=AS_PICK) s->asp=(u8)(rnd8()%AS_PICK);   // old enough for an aspiration of their own
    char*e=simCat(famGrowB,s->name); *e++=' '; simCat(e,famGrewW[s->stage]);
}
static void famDay(void){   // midnight (simMinute, after your own birthday): everyone else grows, and the baby may come
    static const u8 pct[4]={0,200,100,50};
    int grew=0, born=0;
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; int u=s->uid;
        if(s->stage>=AG_ELDER||!xo[XO_AGING]||!oStageDays(s->stage)){ famAge[u]=0; continue; }   // OFF or FOREVER: nobody grows
        int need=oStageDays(s->stage)*pct[xo[XO_AGING]]/100; if(need<1) need=1;
        if(famAge[u]<255) famAge[u]++;
        if(famAge[u]<need) continue;
        famAge[u]=0; famGrow(s); grew++; }
    if(famDue!=255){ if(famDue>0) famDue--; if(!famDue) born=famBirth(); }
    if(grew||born){
        if(born) toast(famMsg); if(grew) toast(grew==1?famGrowB:"BIRTHDAYS  THE FAMILY GREW UP");   // (before the bake, over the game: after it they would land on the loading screen)
        for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
        hhBakeAll(); liveInvalidate();
    }
    hhSave();   // (and the family block with it)
}
static void famScreen(void){   // pause menu > HOUSEHOLD > FAMILY: who is married, the baby on the way, everyone's age
    static char ln[12][40] EWRAM_BSS; const char* L[12]; int n=0;
    { char*e=slCat(ln[n],">"); slCat(e,hhPLast[0]?hhPLast:"YOUR FAMILY"); L[n]=ln[n]; n++; }
    for(int m=-1;m<hhN&&n<10;m++){ int u=m<0?hhPUid:hhM[m].uid, st=uStage(u), sx=uSex(u);
        char*e=slCat(ln[n],uName(u)); e=slCat(e,"  "); e=slCat(e,whoWord(st,sx));
        for(int v=0;v<HU_N;v++) if(v!=u&&(relF[u][v]&RF_MARRIED)&&(v==hhPUid||hhMemOf(v)>=0)){ e=slCat(e,"  MARRIED TO "); slCat(e,uName(v)); break; }
        L[n]=ln[n]; n++; }
    if(famDue!=255&&n<12){ char*e=slCat(ln[n],"A BABY IN "); e=slNum(e,famDue); slCat(e,famDue==1?" DAY":" DAYS"); L[n]=ln[n]; n++; }
    if(n<12){ static const char* const twn[4]={"TWINS NEVER","TWINS SOMETIMES","TWINS OFTEN","TWINS ALWAYS"}; L[n++]=twn[xo[XO_TWINS]]; }
    helpScreen("FAMILY",L,n);
}
