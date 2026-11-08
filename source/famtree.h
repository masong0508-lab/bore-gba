// famtree.h - THE FAMILY TREE: pause menu > HOUSEHOLD > FAMILY TREE (and SELECT on the FAMILY cards). Everyone who lives in the house, drawn as a tree:
// generations in rows (grandparents at the top), a ring or a heart between a couple, a line down from the parents to each child. D-pad walks the tree
// (it goes to the nearest face in that direction); the panel below says who the picked one is to YOU (their kin role), who their parents, partner and
// children are, and how close you are. FAMILY BOND, top right, is how well you get on with your kin overall.
// The living are worked out from the kin table (house.h) every time it opens. THE LINEAGE (below) is the one thing saved: everyone who moved out or passed away stays on the tree as a
// grey memorial portrait, linked to the family by the roles they had (recorded by name), so a family's tree keeps growing across generations.
// Also here: kids inherit their parents' personality (famInherit: each trait lands between the parents' two, give or take one), and a family that lives
// under one roof grows closer a little every night (famWarmth, called by famDay).
// Needs before it: house.h, family.h. NOTE: no single capital letters as names (main.c #defines W, D, H).
#define FT_MAX 16
static inline int ftIsPar(int r){ return r==KN_MOTHER||r==KN_FATHER||r==KN_PARENT; }
static inline int ftIsKid(int r){ return r==KN_DAUGHTER||r==KN_SON||r==KN_CHILD; }
// the rows a role puts a (kin[a][b] = what a is TO b) from b: a's generation minus b's; 99 = the role says nothing
static int ftGenD(int r){
    switch(r){
    case KN_MOTHER: case KN_FATHER: case KN_PARENT: case KN_AUNT: case KN_UNCLE: case KN_STEPMOM: case KN_STEPDAD: return -1;
    case KN_DAUGHTER: case KN_SON: case KN_CHILD: case KN_NIECE: case KN_NEPHEW: case KN_STEPDAU: case KN_STEPSON: return 1;
    case KN_GRANDMA: case KN_GRANDPA: return -2;
    case KN_GRANDDAU: case KN_GRANDSON: return 2;
    case KN_SISTER: case KN_BROTHER: case KN_SIBLING: case KN_COUSIN: case KN_WIFE: case KN_HUSBAND: case KN_SPOUSE: case KN_PARTNER: case KN_ROOMMATE: return 0;
    default: return 99; }
}
// FAMILY BOND: your lifetime score with every kin (a role from MOTHER up, or a spouse), -100..100 -> 0..100. -1: you have no kin here
static int ftBond(void){
    int sum=0, n=0; for(int m=0;m<hhN;m++){ int v=hhM[m].uid, r=kin[v][hhPUid]; if(r>=KN_MOTHER||kinWed(r)){ sum+=relL[hhPUid][v]; n++; } }
    return n?(sum/n+100)/2:-1;
}
// a baby's personality: every trait lands between its parents' two, give or take one (pb 255: one parent only)
static void famInherit(HhSim*s,int pa,int pb){
    for(int i=0;i<TR_N;i++){ int a=pa!=255?uTr(pa,i):rnd8()%11, b=pb!=255?uTr(pb,i):a, v=(a+b+1)/2+(int)(rnd8()%3)-1; s->tr[i]=(u8)(v<0?0:v>10?10:v); }
}
// every night, family under one roof (a parent and child, siblings, grandparents, a spouse) grow a little closer: +1 lifetime, up to 60 (past that it takes real time together)
static void famWarmth(void){
    for(int a=0;a<hhN+1;a++) for(int b=0;b<hhN+1;b++){ if(a==b) continue; int ua=a?hhM[a-1].uid:hhPUid, ub=b?hhM[b-1].uid:hhPUid, r=kin[ua][ub];
        if(r>=KN_MOTHER||kinWed(r)){ if(relL[ua][ub]<60) relL[ua][ub]++; } }
}

// ---- THE LINEAGE: who has left the household, kept in SRAM (SL_LN_OFF, between the household block and the lifetime stats) ----
// An entry: the name, gender, stage, how they left (1 moved out, 2 passed away) and up to eight roles they had towards the people who were still living here (the other Sim's name as an 8-bit hash,
// and what the one who left was TO them). The ledger belongs to a family name: it is shown when somebody living here has it.
#define LN_N 12
#define LN_B 32
#define SL_LN_OFF 7100
typedef struct { char name[HH_NM]; u8 sex, stage, cause, nk; u8 kh[8], kr[8]; } LnE;
_Static_assert(sizeof(LnE)==LN_B,"a lineage entry is 32 bytes");
_Static_assert(HH_OFF+HH_MAXB<=SL_LN_OFF,"the household block runs into the lineage");
_Static_assert(SL_LN_OFF+5+LN_N*LN_B<=7560,"the lineage runs into the life stats");
static LnE lnT[LN_N] EWRAM_BSS; static u8 lnCnt, lnKeyV, lnLoaded;
static u8 lnNh(const char*s){ unsigned h=2166136261u; for(;*s;s++){ h^=(u8)*s; h*=16777619u; } u8 r=(u8)(h^(h>>8)^(h>>16)^(h>>24)); return r?r:1; }
static int lnLivingHas(u8 key){ if(lnNh(hhPLast)==key) return 1; for(int m=0;m<hhN;m++) if(lnNh(hhM[m].last)==key) return 1; return 0; }
static void lnSave(void){
    volatile u8*m=SRAM_BASE+SL_LN_OFF; u8 sum=0x4E; m[0]=0; m[2]=lnCnt; m[3]=lnKeyV; for(int i=0;i<lnCnt*LN_B;i++) m[4+i]=((const u8*)lnT)[i];
    for(int i=2;i<4+lnCnt*LN_B;i++) sum+=m[i]; m[4+lnCnt*LN_B]=sum; m[1]='N'; m[0]='L';   // (the magic last: a half written ledger is not read)
}
static void lnLoad(void){   // once: the ledger from SRAM (nothing if it is not there or does not add up)
    if(lnLoaded) return; lnLoaded=1; lnCnt=0; lnKeyV=0; volatile u8*m=SRAM_BASE+SL_LN_OFF;
    if(m[0]!='L'||m[1]!='N'||m[2]>LN_N) return; int n=m[2]; u8 sum=0x4E; for(int i=2;i<4+n*LN_B;i++) sum+=m[i]; if(m[4+n*LN_B]!=sum) return;
    lnCnt=(u8)n; lnKeyV=m[3]; for(int i=0;i<n*LN_B;i++) ((u8*)lnT)[i]=m[4+i];
}
static void lnClear(void){ lnLoaded=1; lnCnt=0; lnKeyV=0; SRAM_BASE[SL_LN_OFF]=0; }   // a new player: no line before them
static int lnShown(void){ lnLoad(); return lnCnt&&lnLivingHas(lnKeyV); }   // does this household's family have a lineage to show?
// somebody leaves (cause 1 moved out, 2 passed away): their place is member m, still in the household
static void linRecord(int m,int cause){
    if(m<0||m>=hhN) return; lnLoad();
    if(lnCnt&&!lnLivingHas(lnKeyV)) lnCnt=0;   // (another family's ledger: a new one starts)
    lnKeyV=lnNh(hhPLast[0]?hhPLast:hhM[m].last);
    LnE e; for(int i=0;i<LN_B;i++) ((u8*)&e)[i]=0;
    for(int i=0;i<HH_NM;i++) e.name[i]=hhM[m].name[i]; e.sex=hhM[m].look[LK_SEX]; e.stage=hhM[m].stage; e.cause=(u8)cause;
    int a=hhM[m].uid;
    for(int q=0;q<=hhN&&e.nk<8;q++){ int u=q?hhM[q-1].uid:hhPUid; if(u==a) continue; int r=kin[a][u]; if(r){ e.kh[e.nk]=lnNh(uName(u)); e.kr[e.nk++]=(u8)r; } }
    if(lnCnt==LN_N){ for(int i=1;i<LN_N;i++) lnT[i-1]=lnT[i]; lnCnt--; }
    lnT[lnCnt++]=e; lnSave();
}

// ---- THE TREE ----
typedef struct { int x, y, row; } FtNode;
static int ftNL, ftUs[FT_MAX], ftGh[FT_MAX];   // the living (uids) first, then the lineage entries that are shown
static int ftSex(int i){ int x=i<ftNL?uSex(ftUs[i]):lnT[ftGh[i]].sex; return x<3?x:2; }
static const char* ftName(int i){ return i<ftNL?uName(ftUs[i]):lnT[ftGh[i]].name; }
static int ftK(int a,int b){   // what node a is TO node b (a kin role), 0 when nothing is known
    if(a==b) return 0;
    if(a<ftNL&&b<ftNL) return kin[ftUs[a]][ftUs[b]];
    if(a>=ftNL&&b<ftNL){ const LnE*e=&lnT[ftGh[a]]; u8 h=lnNh(uName(ftUs[b])); for(int q=0;q<e->nk;q++) if(e->kh[q]==h) return e->kr[q]; return 0; }
    if(a<ftNL){ int r=ftK(b,a); if(!r) return 0; int v=kinInv[r][ftSex(a)]; return v?v:kinInv[r][0]; }
    return 0;
}
static void ftGhostFace(int x,int y,int rad,int sex){   // a memorial portrait: a grey silhouette in a dim frame, tinted a little by gender
    u16 bg=sex==0?RGB(9,6,11):sex==1?RGB(5,8,13):RGB(6,10,9);
    disc(x,y,rad+1,RGB(16,20,26)); disc(x,y,rad,bg); disc(x,y-rad/4,rad/3+1,RGB(15,19,25)); rect(x-rad/2,y+rad/4,rad,rad/2-1,RGB(15,19,25)); }
static void ftFit(char*out,const char*s,int w){   // the name as much of it as fits w pixels
    int k=0; for(;s[k]&&k<HH_NM;k++){ out[k]=s[k]; out[k+1]=0; if(tw(out,1)>w){ out[k]=0; break; } } out[k]=0; if(!k&&s[0]){ out[0]=s[0]; out[1]=0; } }
static void famTree(void){
    int n=0; ftUs[n++]=hhPUid; for(int m=0;m<hhN&&n<8;m++) ftUs[n++]=hhM[m].uid; ftNL=n;
    int shownG=0; if(lnShown()){ for(int e=lnCnt-1;e>=0&&shownG<5&&n<FT_MAX;e--) if(lnT[e].nk){ ftGh[n]=e; n++; shownG++; } }
    // ---- generations: you are 0; every kin role moves a node up or down from the one it is stated against, until nothing changes ----
    int gen[FT_MAX]; for(int i=0;i<n;i++) gen[i]=-99; gen[0]=0;
    for(int it=0;it<n+2;it++) for(int i=0;i<n;i++) for(int j=0;j<n;j++){ if(i==j) continue; int d=ftGenD(ftK(i,j)); if(d==99) continue;
        if(gen[j]!=-99&&gen[i]==-99) gen[i]=gen[j]+d; else if(gen[i]!=-99&&gen[j]==-99) gen[j]=gen[i]-d; }
    int lo=99, hi=-99; for(int i=0;i<n;i++){ if(gen[i]==-99) gen[i]=0; if(gen[i]<lo) lo=gen[i]; if(gen[i]>hi) hi=gen[i]; }
    if(hi-lo>3) hi=lo+3;
    int rows=hi-lo+1; for(int i=0;i<n;i++){ gen[i]-=lo; if(gen[i]>=rows) gen[i]=rows-1; if(gen[i]<0) gen[i]=0; }
    int pitch=100/rows; if(pitch>40) pitch=40; int rad=(pitch-13)/2; if(rad>12) rad=12; if(rad<5) rad=5;   // (a face and its name fit one row)
    FtNode nd[FT_MAX]; int partner[FT_MAX];
    for(int i=0;i<n;i++){ partner[i]=-1; for(int j=0;j<n;j++) if(j!=i&&gen[j]==gen[i]&&(kinRom(ftK(i,j))||(i<ftNL&&j<ftNL&&(relF[ftUs[i]][ftUs[j]]&RF_STEADY)))){ partner[i]=j; break; } }
    for(int r=0;r<rows;r++){
        int ord[FT_MAX], cnt=0, done[FT_MAX]; for(int i=0;i<n;i++) done[i]=0;
        int uk[FT_MAX], ua[FT_MAX], ub[FT_MAX], un=0;   // the units of the row: a couple or one; key = the average x of the parents above, else the order they came in
        for(int i=0;i<n;i++){ if(gen[i]!=r||done[i]) continue; int p=partner[i]; done[i]=1; if(p>=0&&!done[p]&&partner[p]==i) done[p]=1; else p=-1;
            int sx=0, sc=0; for(int q=0;q<2;q++){ int c=q?p:i; if(c<0) continue; for(int j=0;j<n;j++) if(gen[j]<r&&ftIsPar(ftK(j,c))){ sx+=nd[j].x; sc++; } }
            uk[un]=sc?sx/sc:1000+i; ua[un]=i; ub[un]=p; un++; }
        for(int a=1;a<un;a++){ int k=uk[a], x=ua[a], y=ub[a], b=a-1; while(b>=0&&uk[b]>k){ uk[b+1]=uk[b]; ua[b+1]=ua[b]; ub[b+1]=ub[b]; b--; } uk[b+1]=k; ua[b+1]=x; ub[b+1]=y; }
        for(int a=0;a<un;a++){ ord[cnt++]=ua[a]; if(ub[a]>=0) ord[cnt++]=ub[a]; }
        int w=232/(cnt?cnt:1); if(w>58) w=58;
        for(int k=0;k<cnt;k++){ int i=ord[k]; nd[i].row=r; nd[i].x=120+(k*2-(cnt-1))*w/2; nd[i].y=21+r*pitch+rad+2; }
    }
    int rowN[4]={0,0,0,0}; for(int i=0;i<n;i++) rowN[nd[i].row&3]++;
    int sel=0, dirty=1; u16 prev=keyNow();
    char t[40], d1[60], d2[60], d3[60], nb[HH_NM+1];
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_B|K_START|K_SEL|K_A)) return;
        int dx=(pr&K_RIGHT?1:0)-(pr&K_LEFT?1:0), dy=(pr&K_DOWN?1:0)-(pr&K_UP?1:0);
        if(dx||dy){ int best=-1, bs=1<<30; for(int i=0;i<n;i++){ if(i==sel) continue; int ex=nd[i].x-nd[sel].x, ey=nd[i].y-nd[sel].y;
                if((dx>0&&ex<=0)||(dx<0&&ex>=0)||(dy>0&&ey<=0)||(dy<0&&ey>=0)) continue;
                int sc=dx?(ex<0?-ex:ex)+3*(ey<0?-ey:ey):(ey<0?-ey:ey)+3*(ex<0?-ex:ex); if(sc<bs){ bs=sc; best=i; } }
            if(best>=0){ sel=best; dirty=1; sfxPlay(SFX_TICK); } }
        if(!dirty){ vsync(); continue; }
        dirty=0; famBack();
        { char*e=t; if(hhPLast[0]){ e=simCat(e,"THE "); e=simCat(e,hhPLast); simCat(e," FAMILY TREE"); } else simCat(e,"FAMILY TREE"); }
        famBar(3,3,234,t);
        { int bd=ftBond(); char c[24]; char*e=c; if(bd>=0){ e=simCat(e,"BOND "); e+=numStr(e,bd); simCat(e,"%"); text(233-tw(c,1),7,c,bd>=60?RGB(18,31,16):bd>=40?RGB(31,26,10):RGB(31,14,10),1); }
          e=c; e+=numStr(e,rows); simCat(e,rows==1?" GENERATION":" GENERATIONS"); text(233-(bd>=0?tw("BOND 100%",1)+8:0)-tw(c,1),7,c,RGB(17,29,31),1); }
        // the lines first (faces go over them): a couple's bar with its ring or heart, then a drop from the parents to each child
        for(int i=0;i<n;i++){ int p=partner[i]; if(p>i&&partner[p]==i){ int x0=nd[i].x<nd[p].x?nd[i].x:nd[p].x, x1=nd[i].x<nd[p].x?nd[p].x:nd[i].x, y=nd[i].y, wed=kinWed(ftK(i,p));
                rect(x0,y-1,x1-x0,2,wed?RGB(31,26,8):RGB(31,12,20)); if(wed) famRing((x0+x1)/2,y); else famHeart((x0+x1)/2,y); } }
        for(int c=0;c<n;c++){ int pa=-1, pb=-1; for(int j=0;j<n;j++) if(j!=c&&nd[j].row<nd[c].row&&ftIsPar(ftK(j,c))){ if(pa<0) pa=j; else if(pb<0) pb=j; }
            if(pa<0) continue; int px=pb>=0?(nd[pa].x+nd[pb].x)/2:nd[pa].x, py=nd[pa].y, cy=nd[c].y, my=(py+cy)/2+rad/2; u16 lc=(c>=ftNL||pa>=ftNL)?RGB(11,16,22):RGB(20,26,31);
            rect(px,py,1,my-py,lc); { int x0=px<nd[c].x?px:nd[c].x, x1=px<nd[c].x?nd[c].x:px; rect(x0,my,x1-x0+1,1,lc); } rect(nd[c].x,my,1,cy-rad-my>0?cy-rad-my:0,lc); }
        for(int i=0;i<n;i++){ int x=nd[i].x, y=nd[i].y, on=i==sel, ghost=i>=ftNL; int slot=232/(rowN[nd[i].row&3]?rowN[nd[i].row&3]:1); if(slot>58) slot=58;
            if(on) s3Box(x-rad-3,y-rad-3,2*rad+6,2*rad+15,5,RGB(28,31,18),RGB(16,29,8));
            if(ghost) ftGhostFace(x,y,rad,ftSex(i)); else famFace(x,y,rad,ftUs[i]);
            if(i==0){ int tx=x-rad-19, ty=y-4; if(tx>=3){ s2rr(tx,ty,17,8,RGB(16,29,8)); text(tx+1,ty+1,"YOU",RGB(1,4,0),1); } rect(x-3,y-rad-5,7,1,RGB(16,29,8)); rect(x-2,y-rad-4,5,1,RGB(16,29,8)); rect(x-1,y-rad-3,3,1,RGB(16,29,8)); rect(x,y-rad-2,1,1,RGB(16,29,8)); }   // (a green arrow over your head; the YOU tag too when there is room)
            ftFit(nb,ftName(i),slot-2); text(x-tw(nb,1)/2,y+rad+2,nb,on?WHITE:ghost?RGB(11,15,21):RGB(20,26,31),1); }
        // the picked one: who they are to you, their people, how close you are
        { int r=ftK(sel,0), pa=-1, pb=-1; char*e; const LnE*g=sel>=ftNL?&lnT[ftGh[sel]]:0;
          s2rr(3,124,234,21,RGB(10,20,30)); s2rr(4,125,232,19,RGB(2,6,13));
          e=simCat(d1,ftName(sel)); e=simCat(e,"  "); if(sel==0) simCat(e,"THAT IS YOU"); else if(r) { e=simCat(e,"YOUR "); simCat(e,kinNm[r]); } else if(!g) simCat(e,relWord(hhPUid,ftUs[sel])); else simCat(e,"OF THE FAMILY");
          text(9,127,d1,g?RGB(20,24,30):WHITE,1);
          for(int j=0;j<n;j++) if(j!=sel&&ftIsPar(ftK(j,sel))){ if(pa<0) pa=j; else if(pb<0) pb=j; }
          e=d2; { int pp=partner[sel]; if(pp>=0){ e=simCat(e,kinWed(ftK(sel,pp))?"MARRIED TO ":"STEADY WITH "); e=simCat(e,ftName(pp)); } }
          if(pa>=0){ if(e!=d2) e=simCat(e,"  "); e=simCat(e,"CHILD OF "); e=simCat(e,ftName(pa)); if(pb>=0){ e=simCat(e," AND "); e=simCat(e,ftName(pb)); } }
          if(g){ if(e!=d2) e=simCat(e,"  "); simCat(e,g->cause==2?"PASSED AWAY":"MOVED OUT"); }
          else if(e==d2) simCat(e,"NO PARTNER  NO PARENTS HERE");
          text(9,136,d2,RGB(17,29,31),1);
          { int kids=0; e=d3; for(int j=0;j<n;j++) if(j!=sel&&ftIsPar(ftK(sel,j))){ if(!kids) e=simCat(e,"KIDS "); else e=simCat(e," "); e=simCat(e,ftName(j)); kids++; } if(!kids) d3[0]=0; }
          if(d3[0]) text(237-tw(d3,1)-2,127,d3,RGB(31,22,14),1);
          if(sel&&!g){ famRelBar(172,136,60,relL[hhPUid][ftUs[sel]]); text(150,136,"CLOSE",RGB(17,29,31),1); } }
        s2pill(5,147,62,"DPAD PICK"); s2pill(70,147,46,"B BACK");
        present();
    }
}

// An elder who has lived in the house for their span (22 to 30 days as an elder, with AGING on) passes away: a notice with their face, the family is sad, and they are kept in the lineage.
static void famPassCheck(void){
    if(!xo[XO_AGING]) return;
    int m=-1; for(int k=0;k<hhN;k++){ const HhSim*s=&hhM[k]; if(s->stage==AG_ELDER&&famAge[s->uid]>=22+(s->uid*5)%9){ m=k; break; } }
    if(m<0) return;
    int u=hhM[m].uid; char l1[44]; simCat(simCat(l1,hhM[m].name)," PASSED AWAY");
    famNotice("A SAD DAY",l1,"THE FAMILY WILL MISS THEM",&u,1);   // (their face first)
    for(int k=0;k<hhN;k++){ if(k==m) continue; if(kin[u][hhM[k].uid]>=KN_MOTHER||kin[hhM[k].uid][u]>=KN_MOTHER){ int v=hhM[k].need[HN_FUN]-30; hhM[k].need[HN_FUN]=(u8)(v<0?0:v); } }
    if(kin[u][hhPUid]>=KN_MOTHER) moodEvent(M_HURT_BIG);
    linRecord(m,2); hhRemove(m);
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    hhSlotsFree(); liveInvalidate();
}
