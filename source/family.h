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
// ---- NOTICES: a birth or a birthday stops the game for a Sims dialog: a confetti backdrop, a glass panel with the plumbob in its
// title bar, the Sims it is about in round portraits (pink, blue or mint behind them by gender) with their names, two lines, an OK.
// It shows after the new sprites are baked, so the portraits are the new faces. ----
static void famNotice(const char*title,const char*l1,const char*l2,const int*us,int n){
    u16 prev=keyNow(); u32 fr=0; if(n>4) n=4;
    objHideAll(); suBackdrop();
    { static const u16 cf[6]={RGB(31,26,10),RGB(31,16,24),RGB(14,28,31),RGB(16,31,14),RGB(31,20,10),RGB(24,18,31)};   // confetti
      for(int i=0;i<70;i++){ int x=4+rnd8()*232/256, y=4+rnd8()*152/256; rect(x,y,2+(i&1),2,cf[i%6]); } }
    s3Box(16,20,208,124,10,RGB(15,24,31),RGB(8,15,26)); s3Box(17,21,206,122,9,RGB(5,11,22),RGB(2,5,13));
    int gap=n>1?184/n:0, x0=120-gap*(n-1)/2;
    for(int k=0;k<n;k++){ int u=us[k], cx=x0+k*gap, mm=hhMemOf(u); u16 a,b; sexBg(uSex(u),&a,&b);
        simPortrait(cx,64,n>2?17:21,u,a,b,RGB(29,31,31));
        const char*nm=mm<0?hhPName:hhM[mm].name; int w=tw(nm,1)+14; s3Pill(cx-w/2,n>2?84:88,w,11,0,nm); }
    text(120-tw(l1,1)/2,105,l1,WHITE,1); if(l2&&l2[0]) text(120-tw(l2,1)/2,115,l2,RGB(17,29,31),1);
    s3Pill(96,127,48,13,1,"OK");
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; fr++;
        if(pr&(K_A|K_B|K_START)) break;
        suTitleBar(18,22,204,title,fr);
        present();
    }
    while((~REG_KEYINPUT)&0x3FF) vsync();   // (the game must not see the A)
}
static char famT[24] EWRAM_BSS, famL1[48] EWRAM_BSS, famL2[40] EWRAM_BSS;
static int famBorn[2], famBornN, famBornSame, famBornMine;   // who was just born (uids), whether they are identical twins, whether you are a parent
static int famBirth(void){   // the due day: one baby, or two. 1 = someone was born
    int pa=famPa, pb=famPb; famDue=famPa=famPb=255; famBornN=0;
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
    famBorn[famBornN++]=hhM[m1].uid; famBornSame=same&&m2>=0;
    if(m2>=0){ int a=hhM[m1].uid, b=hhM[m2].uid; stRel(a,b,60,60,RF_FRIEND); famBorn[famBornN++]=b; }   // twins: close from the start
    if(m2>=0){ simCat(famT,same?"IDENTICAL TWINS!":"TWINS!"); char*e=simCat(famL1,hhM[m1].name); e=simCat(e," AND "); e=simCat(e,hhM[m2].name); simCat(e," ARE BORN"); }
    else { int sx=hhM[m1].look[LK_SEX]; simCat(famT,sx==SX_FEMALE?"IT'S A GIRL!":sx==SX_MALE?"IT'S A BOY!":"A NEW BABY!"); simCat(simCat(famL1,hhM[m1].name)," IS BORN"); }
    { char*e=simCat(famL2,"WELCOME TO THE "); e=simCat(e,famLastOf(pa!=255?pa:pb)[0]?famLastOf(pa!=255?pa:pb):"FAMILY"); if(famLastOf(pa!=255?pa:pb)[0]) simCat(e," FAMILY"); }
    famBornMine=pa==hhPUid||pb==hhPUid; if(famBornMine) moodEvent(M_WANT);
    return 1;
}
static const char* const famGrewW[AG_N]={"","IS A CHILD NOW","IS A TEEN NOW","IS ALL GROWN UP","IS AN ELDER NOW"};
static const char* const famGrewL2[AG_N]={"","OFF TO SCHOOL ON WEEKDAYS","OLD ENOUGH FOR AN ASPIRATION","OFF TO WORK ON WEEKDAYS","A LIFE WELL LIVED"};
static void famGrow(HhSim*s){   // a member's birthday: the next stage, the look fitted to it
    u8 sl[LK_N], ss=stage; for(int i=0;i<LK_N;i++){ sl[i]=look[i]; look[i]=s->look[i]; }
    stage=(u8)(s->stage+1); fixLook(); for(int i=0;i<LK_N;i++) s->look[i]=look[i]; s->stage=stage;
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=ss;
    if(s->stage>=AG_TEEN&&s->asp>=AS_PICK) s->asp=(u8)(rnd8()%AS_PICK);   // old enough for an aspiration of their own
}
static void famDay(void){   // midnight (simMinute, after your own birthday): everyone else grows, and the baby may come
    static const u8 pct[4]={0,200,100,50};
    int grew=0, born=0, gu[HH_MAX];
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; int u=s->uid;
        if(s->stage>=AG_ELDER||!xo[XO_AGING]||!oStageDays(s->stage)){ famAge[u]=0; continue; }   // OFF or FOREVER: nobody grows
        int need=oStageDays(s->stage)*pct[xo[XO_AGING]]/100; if(need<1) need=1;
        if(famAge[u]<255) famAge[u]++;
        if(famAge[u]<need) continue;
        famAge[u]=0; famGrow(s); gu[grew++]=u; }
    if(famDue!=255){ if(famDue>0) famDue--; if(!famDue) born=famBirth(); }
    if(grew||born){
        for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
        hhBakeAll();   // (the new faces first: the notices show them)
        if(born){ if(famBornMine) voxPlay(V_yahoo); famNotice(famT,famL1,famL2,famBorn,famBornN); }   // (your voice: a whoop)
        if(grew){ int m=hhMemOf(gu[0]);
            if(grew==1&&m>=0){ char*e=simCat(famL1,hhM[m].name); *e++=' '; simCat(e,famGrewW[hhM[m].stage]); famNotice("HAPPY BIRTHDAY!",famL1,famGrewL2[hhM[m].stage],gu,1); }
            else famNotice("HAPPY BIRTHDAYS!","THE FAMILY GREW UP","A YEAR OLDER  A NEW LOOK",gu,grew); }
        hhSlotsFree(); liveInvalidate();
    }
    hhSave();   // (and the family block with it)
}
// ---- pause menu > HOUSEHOLD > FAMILY: The Sims 2's family panel. A card for each Sim (their face, name and age, a ring when married,
// a heart when going steady, YOU on yours), the picked one described below with how you two get on, and the family's news ----
static int famDaysLeft(int u){   // game days until u grows up (-1: never)
    static const u8 pct[4]={0,200,100,50}; int st=uStage(u);
    if(st>=AG_ELDER||!xo[XO_AGING]||!oStageDays(st)) return -1;
    int need=oStageDays(st)*pct[xo[XO_AGING]]/100; if(need<1) need=1; int had=u==hhPUid?ageDays:famAge[u]; return need>had?need-had:1; }
static int famPartner(int u,u8 flag){ for(int v=0;v<HU_N;v++) if(v!=u&&(relF[u][v]&flag)&&(v==hhPUid||hhMemOf(v)>=0)) return v; return -1; }
static void famScreen(void){
    int us[HU_N], n=0; us[n++]=hhPUid; for(int m=0;m<hhN;m++) us[n++]=hhM[m].uid;
    int sel=0, dirty=1; u32 fr=0; u16 prev=keyNow();
    static char t[40] EWRAM_BSS, d1[48] EWRAM_BSS, d2[40] EWRAM_BSS;
    { char*e=t; if(hhPLast[0]){ e=simCat(e,"THE "); e=simCat(e,hhPLast); simCat(e," FAMILY"); } else simCat(e,"YOUR FAMILY"); }
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; fr++;
        if(pr&(K_B|K_START|K_A)) return;
        if(pr&K_RIGHT){ sel=(sel+1)%n; dirty=1; } if(pr&K_LEFT){ sel=(sel+n-1)%n; dirty=1; }
        if((pr&K_DOWN)&&sel+4<n){ sel+=4; dirty=1; } if((pr&K_UP)&&sel>=4){ sel-=4; dirty=1; }
        if(dirty){ dirty=0; objHideAll(); suBackdrop();
            { char c[12]; char*e=c; e+=numStr(e,n); simCat(e,n==1?" SIM":" SIMS"); text(233-tw(c,1),7,c,RGB(17,29,31),1); }
            int big=n<=4, ch=big?102:51, pr=big?21:13;   // four or fewer: tall cards with bigger faces, how they feel and how you stand
            int x0=big?(SW-(n*57-2))/2:6;   // (a small family stands in the middle)
            for(int i=0;i<n;i++){ int u=us[i], x=x0+(i&3)*57, y=19+(i>>2)*54, on=i==sel, cx=x+27; u16 a,b; sexBg(uSex(u),&a,&b);
                if(on) s3Box(x-2,y-2,59,ch+4,7,RGB(28,31,18),RGB(16,29,8));   // the picked card glows green
                s3Box(x,y,55,ch,6,RGB(14,23,31),RGB(7,14,25)); s3Box(x+1,y+1,53,ch-2,5,on?RGB(7,15,27):RGB(5,11,22),on?RGB(3,8,17):RGB(2,5,13));
                if(big) suBob(cx,y+4,moodOf(u),0);
                simPortrait(cx,big?y+40:y+17,pr,u,a,b,on?RGB(25,31,16):RGB(23,29,31));
                if(famPartner(u,RF_MARRIED)>=0) suRing(x+8,y+7); else if(famPartner(u,RF_STEADY)>=0) suHeart(x+8,y+7);
                if(u==hhPUid){ s2rr(x+38,y+3,15,8,RGB(16,29,8)); text(x+39,y+4,"YOU",RGB(1,4,0),1); }
                int ty=big?y+67:y+34; const char*nm=uName(u); text(cx-tw(nm,1)/2,ty,nm,WHITE,1);
                const char*w=whoWord(uStage(u),uSex(u)); text(cx-tw(w,1)/2,ty+8,w,RGB(17,29,31),1);
                if(big){ const char*r=u==hhPUid?"THAT'S YOU":relWord(hhPUid,u); u16 rc=(u!=hhPUid&&(relF[hhPUid][u]&(RF_MARRIED|RF_STEADY|RF_LOVE|RF_CRUSH)))?RGB(31,19,26):RGB(24,28,31);
                    text(cx-tw(r,1)/2,ty+19,r,rc,1); } }
            { int u=us[sel], p=famPartner(u,RF_MARRIED), q=p<0?famPartner(u,RF_STEADY):-1, dl=famDaysLeft(u);   // the picked one, described
              s2rr(3,124,234,21,RGB(10,20,30)); s2rr(4,125,232,19,RGB(2,6,13));
              char*e=simCat(d1,uName(u));
              if(p>=0){ e=simCat(e,"  MARRIED TO "); simCat(e,p==hhPUid?"YOU":uName(p)); }
              else if(q>=0){ e=simCat(e,"  GOING STEADY WITH "); simCat(e,q==hhPUid?"YOU":uName(q)); }
              text(9,127,d1,WHITE,1);
              e=d2; if(dl<0) e=simCat(e,uStage(u)>=AG_ELDER?"AN ELDER":"NOT AGING"); else { e=simCat(e,"GROWS UP IN "); e+=numStr(e,dl); simCat(e,dl==1?" DAY":" DAYS"); }
              text(9,136,d2,RGB(17,29,31),1);
              if(u!=hhPUid){ text(140,127,"DAILY",RGB(17,29,31),1); suRelBar(172,127,60,relD[hhPUid][u]); text(140,136,"LIFE",RGB(17,29,31),1); suRelBar(172,136,60,relL[hhPUid][u]); } }
            s2pill(5,147,62,"DPAD PICK"); s2pill(70,147,46,"B BACK");
            { static const char* const twn[4]={"TWINS NEVER","TWINS SOMETIMES","TWINS OFTEN","TWINS ALWAYS"}; const char*tt=twn[xo[XO_TWINS]]; int w=tw(tt,1)+12; s2pill(235-w,147,w,tt);
              if(famDue!=255){ char bb[20]; char*e=simCat(bb,"BABY IN "); e+=numStr(e,famDue); simCat(e,famDue==1?" DAY":" DAYS");
                int w2=tw(bb,1)+12, x2=232-w-w2; s3Box(x2,147,w2,11,5,RGB(31,22,27),RGB(26,12,20)); text(x2+6,149,bb,RGB(12,1,7),1); } }
        }
        suTitleBar(3,3,234,t,fr); { char c[12]; char*e=c; e+=numStr(e,n); simCat(e,n==1?" SIM":" SIMS"); text(233-tw(c,1),7,c,RGB(17,29,31),1); }
        present();
    }
}
