// famtree.h - THE FAMILY TREE: pause menu > HOUSEHOLD > FAMILY TREE (and SELECT on the FAMILY cards). Everyone who lives in the house, drawn as a tree:
// generations in rows (grandparents at the top), a ring or a heart between a couple, a line down from the parents to each child. D-pad walks the tree
// (it goes to the nearest face in that direction); the panel below says who the picked one is to YOU (their kin role), who their parents, partner and
// children are, and how close you are. FAMILY BOND, top right, is how well you get on with your kin overall.
// Nothing here is saved: the tree is worked out from the kin table (house.h) every time it opens, so old saves and room slots carry it along.
// Also here: kids inherit their parents' personality (famInherit: each trait lands between the parents' two, give or take one), and a family that lives
// under one roof grows closer a little every night (famWarmth, called by famDay).
// Needs before it: house.h, family.h. NOTE: no single capital letters as names (main.c #defines W, D, H).
#define FT_MAX 8
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
typedef struct { int x, y, row; } FtNode;
static void famTree(void){
    int us[FT_MAX], n=0; us[n++]=hhPUid; for(int m=0;m<hhN&&n<FT_MAX;m++) us[n++]=hhM[m].uid;
    // ---- generations: you are 0; every kin role moves a Sim up or down from the one it is stated against, until nothing changes ----
    int gen[FT_MAX]; for(int i=0;i<n;i++) gen[i]=-99; gen[0]=0;
    for(int it=0;it<n+2;it++) for(int i=0;i<n;i++) for(int j=0;j<n;j++){ if(i==j) continue; int d=ftGenD(kin[us[i]][us[j]]); if(d==99) continue;
        if(gen[j]!=-99&&gen[i]==-99) gen[i]=gen[j]+d; else if(gen[i]!=-99&&gen[j]==-99) gen[j]=gen[i]-d; }
    int lo=99, hi=-99; for(int i=0;i<n;i++){ if(gen[i]==-99) gen[i]=0; if(gen[i]<lo) lo=gen[i]; if(gen[i]>hi) hi=gen[i]; }
    if(hi-lo>3) hi=lo+3;
    int rows=hi-lo+1; for(int i=0;i<n;i++){ gen[i]-=lo; if(gen[i]>=rows) gen[i]=rows-1; }
    // ---- a row at a time: couples side by side, units ordered by where their parents stand ----
    int pitch=100/rows; if(pitch>40) pitch=40; int rad=(pitch-13)/2; if(rad>12) rad=12; if(rad<5) rad=5;   // (a face and its name fit one row)
    FtNode nd[FT_MAX]; int partner[FT_MAX]; for(int i=0;i<n;i++){ partner[i]=-1; for(int j=0;j<n;j++) if(j!=i&&gen[j]==gen[i]&&(kinRom(kin[us[i]][us[j]])||(relF[us[i]][us[j]]&RF_STEADY))){ partner[i]=j; break; } }
    for(int r=0;r<rows;r++){
        int ord[FT_MAX], cnt=0, done[FT_MAX]; for(int i=0;i<n;i++) done[i]=0;
        // the units of this row: a couple (both in the row) or one; key = the average x of the parents above, else the order they came in
        int uk[FT_MAX], ua[FT_MAX], ub[FT_MAX], un=0;
        for(int i=0;i<n;i++){ if(gen[i]!=r||done[i]) continue; int p=partner[i]; done[i]=1; if(p>=0&&!done[p]&&partner[p]==i) done[p]=1; else p=-1;
            int sx=0, sc=0; for(int q=0;q<2;q++){ int c=q?p:i; if(c<0) continue; for(int j=0;j<n;j++) if(gen[j]<r&&ftIsPar(kin[us[j]][us[c]])){ sx+=nd[j].x; sc++; } }
            uk[un]=sc?sx/sc:1000+i; ua[un]=i; ub[un]=p; un++; }
        for(int a=1;a<un;a++){ int k=uk[a], x=ua[a], y=ub[a], b=a-1; while(b>=0&&uk[b]>k){ uk[b+1]=uk[b]; ua[b+1]=ua[b]; ub[b+1]=ub[b]; b--; } uk[b+1]=k; ua[b+1]=x; ub[b+1]=y; }
        for(int a=0;a<un;a++){ ord[cnt++]=ua[a]; if(ub[a]>=0) ord[cnt++]=ub[a]; }
        int w=232/(cnt?cnt:1); if(w>58) w=58;
        for(int k=0;k<cnt;k++){ int i=ord[k]; nd[i].row=r; nd[i].x=120+(k*2-(cnt-1))*w/2; nd[i].y=21+r*pitch+rad+2; }
    }
    int sel=0, dirty=1, frame=0; u16 prev=keyNow();
    char t[40], d1[56], d2[56], d3[56];
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; frame++;
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
        { int bd=ftBond(); if(bd>=0){ char c[16]; char*e=simCat(c,"BOND "); e+=numStr(e,bd); simCat(e,"%"); text(233-tw(c,1),7,c,bd>=60?RGB(18,31,16):bd>=40?RGB(31,26,10):RGB(31,14,10),1); } }
        // the lines first (faces go over them): a couple's bar with its ring or heart, then a drop from the parents to each child
        for(int i=0;i<n;i++){ int p=partner[i]; if(p>i&&partner[p]==i){ int x0=nd[i].x<nd[p].x?nd[i].x:nd[p].x, x1=nd[i].x<nd[p].x?nd[p].x:nd[i].x, y=nd[i].y;
                rect(x0,y-1,x1-x0,2,kinWed(kin[us[i]][us[p]])?RGB(31,26,8):RGB(31,12,20)); if(kinWed(kin[us[i]][us[p]])) famRing((x0+x1)/2,y); else famHeart((x0+x1)/2,y); } }
        for(int c=0;c<n;c++){ int pa=-1, pb=-1; for(int j=0;j<n;j++) if(j!=c&&nd[j].row<nd[c].row&&ftIsPar(kin[us[j]][us[c]])){ if(pa<0) pa=j; else if(pb<0) pb=j; }
            if(pa<0) continue; int px=pb>=0?(nd[pa].x+nd[pb].x)/2:nd[pa].x, py=nd[pa].y, cy=nd[c].y, my=(py+cy)/2+rad/2; u16 lc=RGB(20,26,31);
            rect(px,py,1,my-py,lc); { int x0=px<nd[c].x?px:nd[c].x, x1=px<nd[c].x?nd[c].x:px; rect(x0,my,x1-x0+1,1,lc); } rect(nd[c].x,my,1,cy-rad-my,lc); }
        for(int i=0;i<n;i++){ int u=us[i], x=nd[i].x, y=nd[i].y, on=i==sel;
            if(on){ s3Box(x-rad-3,y-rad-3,2*rad+6,2*rad+15,5,RGB(28,31,18),RGB(16,29,8)); }
            famFace(x,y,rad,u);
            if(u==hhPUid){ s2rr(x-rad-19,y-4,17,8,RGB(16,29,8)); text(x-rad-18,y-3,"YOU",RGB(1,4,0),1); }
            const char*nm=uName(u); text(x-tw(nm,1)/2,y+rad+2,nm,on?WHITE:RGB(20,26,31),1); }
        // the picked one: who they are to you, their people, how close you are
        { int u=us[sel], r=kin[u][hhPUid], pa=-1, pb=-1; char*e;
          s2rr(3,124,234,21,RGB(10,20,30)); s2rr(4,125,232,19,RGB(2,6,13));
          e=simCat(d1,uName(u)); e=simCat(e,"  "); if(u==hhPUid) simCat(e,"THAT IS YOU"); else if(r) { e=simCat(e,"YOUR "); simCat(e,kinNm[r]); } else simCat(e,relWord(hhPUid,u));
          text(9,127,d1,WHITE,1);
          for(int j=0;j<n;j++) if(j!=sel&&ftIsPar(kin[us[j]][u])){ if(pa<0) pa=j; else if(pb<0) pb=j; }
          e=d2; { int pp=partner[sel]; if(pp>=0){ e=simCat(e,kinWed(kin[u][us[pp]])?"MARRIED TO ":"STEADY WITH "); e=simCat(e,uName(us[pp])); } }
          if(pa>=0){ if(e!=d2) e=simCat(e,"  "); e=simCat(e,"CHILD OF "); e=simCat(e,uName(us[pa])); if(pb>=0){ e=simCat(e," AND "); e=simCat(e,uName(us[pb])); } }
          if(e==d2) simCat(e,"NO PARTNER  NO PARENTS HERE");
          text(9,136,d2,RGB(17,29,31),1);
          { int kids=0; e=d3; for(int j=0;j<n;j++) if(j!=sel&&ftIsPar(kin[u][us[j]])){ if(!kids) e=simCat(e,"KIDS "); else e=simCat(e," "); e=simCat(e,uName(us[j])); kids++; } if(!kids) d3[0]=0; }
          if(d3[0]) text(237-tw(d3,1)-2,127,d3,RGB(31,22,14),1);
          if(u!=hhPUid){ famRelBar(172,136,60,relL[hhPUid][u]); text(150,136,"CLOSE",RGB(17,29,31),1); } }
        s2pill(5,147,62,"DPAD PICK"); s2pill(70,147,46,"B BACK");
        present();
    }
}
