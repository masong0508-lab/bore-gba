// family.h - FAMILY LIFE: weddings, babies (and twins), and a household that grows up around you.
//   PROPOSE         (social, house.h) adults going steady: a yes marries them. Kin says it from then on: WIFE / HUSBAND (or SPOUSE for a
//                   nonbinary Sim) both ways, and the one asked takes the asker's last name.
//   TRY FOR A BABY  (social) a married or steady couple of adults who live here, room in the house, no baby on the way: a yes means a baby
//                   arrives 3 days later (at midnight), a mix of both parents (stMixLook) with a gender of its own (sexRoll). Kin: the parents
//                   are its MOTHER / FATHER / PARENT, it is their DAUGHTER / SON / CHILD, and the parents' other children are its siblings.
//   TWINS           OPTIONS > SIM > BORES > TWINS: NEVER / SOMETIMES (1 in 6) / OFTEN (1 in 3) / ALWAYS, only with room for two. Half identical
//                   (one look, one gender), half fraternal; twins are SISTER / BROTHER / SIBLING and start out close.
//   GROWING UP      every member ages at midnight as you do (ageTick): the days on OPTIONS > TIME > AGES, scaled by AGING.
//   NOTICES         a birth or a birthday stops the game for a Sims dialog with the new faces (after the sprites are baked).
//   FAMILY          pause menu > HOUSEHOLD > FAMILY: a card per Sim (face, age, gender, a ring or a heart), the picked one described below.
// Saved in the household block ('H@', house.h: famAge, famDue, famPa, famPb), so room slots and the household bank carry it.
// Needs before it: house.h, sims.h, story.h (stMixLook, stAddSim, stRel), the UI kit and s2rr / s2grad / s2pill / s3Box / s3Pill (main.c).

static void famForget(int u){   // u moved out: their days go, and a baby they were expecting comes with the other parent (or not at all)
    if(u<0||u>=HU_N) return; famAge[u]=0;
    if(famPa==u) famPa=255; if(famPb==u) famPb=255; if(famPa==255&&famPb==255) famDue=0;
}
static const char* famLastOf(int u){ int m=hhMemOf(u); return m<0?hhPLast:hhM[m].last; }
static void famWed(int a,int b){   // a proposed and b said yes
    static const u8 wr[SX_N]={KN_WIFE,KN_HUSBAND,KN_SPOUSE};
    int x=uSex(a), y=uSex(b); kin[a][b]=wr[x<SX_N?x:SX_NB]; kin[b][a]=wr[y<SX_N?y:SX_NB];
    relF[a][b]|=RF_STEADY; relF[b][a]|=RF_STEADY;
    { const char*l=famLastOf(a); char t[HH_NM]; int k=0; for(;l[k]&&k<HH_NM-1;k++) t[k]=l[k]; t[k]=0;   // b takes a's last name
      int m=hhMemOf(b); char*d=m<0?hhPLast:hhM[m].last; if(t[0]) for(int i=0;i<HH_NM;i++) d[i]=t[i]; }
    if(a==hhPUid||b==hhPUid){ simEvent(SE_STEADY); simQueue("JUST MARRIED"); }
    else { int m=hhMemOf(a); if(m>=0) hhNote(&hhM[m]," GOT MARRIED"); }
}
static void famTry(int a,int b){   // a couple said yes to a baby: it comes in 3 days
    if(famDue) return;
    famDue=3; famPa=(u8)a; famPb=(u8)b;
    if(a==hhPUid||b==hhPUid) simQueue("A BABY IS ON THE WAY");
    else { int m=hhMemOf(a); if(m>=0) hhNote(&hhM[m]," IS EXPECTING"); }
}

// ---- PORTRAITS: a Sim's head and shoulders in a round frame (the pie's hub, FAMILY, the notices), drawn at full resolution ----
// The face is the HUD portrait's own drawing (hudface.h faceDrawL: the creator's eye and mouth art, brows, glasses, nose, cheeks, hair, every face slider) for
// anyone's look, with an expression for how they feel; under it their neck and their top's colour across the shoulders. What falls outside the circle is put
// back as it was (the pixels under the frame are kept first), then the rim goes round it.
static u16 famUnder[556] EWRAM_BSS;   // the pixels under the frame that are put back: only the corners outside the rim (556 at most, r up to 21; was the whole 47 x 47 square)
static void famBg(int sx,u16*a,u16*b){   // a portrait's backdrop: pink, blue or mint by gender
    if(sx==SX_FEMALE){ *a=RGB(31,25,28); *b=RGB(25,13,21); } else if(sx==SX_MALE){ *a=RGB(22,28,31); *b=RGB(9,17,30); } else { *a=RGB(24,31,25); *b=RGB(10,23,15); } }
static int famSt(int u,int stg){   // their face for the moment: SAD BORED OK HAPPY STOKED (hudExpr's rows)
    if(u==hhPUid) return moodState();
    if(stg==AG_BABY) return 3;
    int v=uMood(u); return v<25?0:v<40?1:v<60?2:v<80?3:4; }
static void famFaceL(int cx,int cy,int r,const u8*lk,int stg,int sx,const u8*e){   // a portrait of any look (e: the expression, hudExpr's rows)
    if(r>21) r=21;
    int R2=(r+2)*(r+2), R1=(r+1)*(r+1), R0=r*r, bw=2*r+5, bx=cx-r-2, by=cy-r-2; u16 b0,b1; famBg(sx,&b0,&b1);
    { int q=0; for(int j=0;j<bw;j++) for(int i=0;i<bw;i++){ int dx=i-r-2, dy=j-r-2; if(dx*dx+dy*dy<=R2) continue; int X=bx+i, Y=by+j; famUnder[q++]=((unsigned)X<SW&&(unsigned)Y<SH)?fb[Y*SW+X]:0; } }   // (the same order the corners are put back in below)
    int ox0=cX0, oy0=cY0; unsigned ow=cW, oh=cH;
    clipSet(bx<0?0:bx,by<0?0:by,bx+bw>SW?SW:bx+bw,by+bw>SH?SH:by+bw);
    for(int dy=-r;dy<=r;dy++) rect(cx-r,cy+dy,2*r+1,1,s3Mix(b0,b1,dy+r,2*r+1));   // the backdrop (the corners are put back below)
    u16 skin=toneBy(skinTones[lk[LK_SKIN]],slideEffS(lk[LK_TONE])), hair=toneBy(hairTones[lk[LK_HCOL]%NSW],slideEffS(lk[LK_HTONE]));
    u16 top=toneBy(topTones[lk[LK_TOP]%NSW],slideEffS(lk[LK_TTONE]));
    int up=r>=19?13:r-3; if(up>13) up=13;
    int fx=cx-11, fy=cy-up, sh=fy+(r>=19?25:23);   // the face's top left, the top of the shoulders (a bigger frame shows more neck)
    rect(cx-3,fy+19,7,sh-fy-18,shade(skin,13)); rect(cx-3,fy+19,1,sh-fy-18,shade(skin,11)); rect(cx-3,fy+22,7,1,shade(skin,11));   // the neck, the jaw's shadow on it
    for(int y=sh;y<=cy+r;y++){ int k=y-sh, w=7; while((w-6)*(w-6)<k*36&&w<2*r) w++;   // round shoulders in their top's colour, light along the top
        rect(cx-w,y,2*w+1,1,k==0?lite(top,19):top); rect(cx-w,y,2,1,shade(top,12)); rect(cx+w-1,y,2,1,shade(top,12)); }
    if(sh<=cy+r){ rect(cx-2,sh,5,1,skin); if(sh+1<=cy+r) rect(cx-1,sh+1,3,1,skin); }   // the neckline
    { u16 ed=shade(skin,12); rect(fx-1,fy+9,2,5,skin); rect(fx-1,fy+10,1,3,ed); rect(fx+21,fy+9,2,5,skin); rect(fx+22,fy+10,1,3,ed); }   // ears (the hair, if it is long, falls over them)
    { int hs=lk[LK_HSTYLE]%NHAIR; if(hudHairRows[hs]) rect(fx+5,fy-1,12,1,hair); if(hs==5){ rect(fx+2,fy-3,18,2,hair); rect(fx,fy-1,22,2,hair); } }   // a little hair above the crown (an AFRO a lot)
    faceDrawL(fx,fy,lk,stg,e,-1,-1);
    cX0=ox0; cY0=oy0; cW=ow; cH=oh;
    { int q=0; for(int j=0;j<bw;j++) for(int i=0;i<bw;i++){ int dx=i-r-2, dy=j-r-2, d=dx*dx+dy*dy; if(d<=R0) continue;   // outside the circle: as it was; then the rim and its dark edge
        u16 c=d>R2?famUnder[q++]:d>R1?RGB(2,5,11):RGB(26,30,31); px(bx+i,by+j,c); } }
}
static void famFace(int cx,int cy,int r,int u){   // the portrait of Sim u, with their face for the moment
    int m=hhMemOf(u); const u8*lk=m<0?look:hhM[m].look; int stg=m<0?stage:hhM[m].stage, st=famSt(u,stg);
    famFaceL(cx,cy,r,lk,stg,uSex(u),(u==hhPUid)?hudEx(st):hudExpr[st]);
}
static void famBob(int cx,int y,int mood){   // the plumbob over a Sim: green when fine, yellow when so-so, red when miserable
    static const u16 col[3][4]={ {RGB(3,20,6),RGB(2,14,4),RGB(14,31,16),RGB(8,26,10)}, {RGB(22,18,2),RGB(16,12,1),RGB(31,29,10),RGB(28,24,4)}, {RGB(22,4,3),RGB(15,2,2),RGB(31,14,12),RGB(28,8,6)} };
    const u16*c=col[mood<0?0:mood>2?2:mood];
    for(int i=0;i<7;i++){ int hw=i<4?i:6-i; rect(cx-hw,y+i*2,hw+1,2,i<3?c[0]:c[1]); rect(cx+1,y+i*2,hw,2,i<3?c[2]:c[3]); }
    rect(cx+1,y+3,1,2,RGB(28,31,28)); }
static int famMood(int u){ int v=uMood(u); return v>=55?0:v>=30?1:2; }
static void pieHub(int cx,int cy,int r,int u){ famFace(cx,cy,r,u); famBob(cx,cy-r-17,famMood(u)); }   // pie.h: the face of the one you talk to, in the hub

// ---- NOTICES: a birth or a birthday stops the game for a Sims dialog: confetti, a panel, the new faces with their names, two lines, an OK ----
static void famBack(void){
    objHideAll(); s2grad(0,0,SW,SH,1,4,10,2,9,17);
    for(int y=0;y<SH;y+=8) for(int x=(y&8)?4:0;x<SW;x+=8) rect(x,y,1,1,RGB(3,9,17));
    s2rr(1,1,238,158,RGB(10,20,30)); s2rr(2,2,236,156,RGB(2,6,13)); }
static void famBar(int x,int y,int w,const char*t){ s2grad(x,y,w,13,9,19,29,3,10,20); rect(x,y+13,w,1,RGB(15,26,31)); famBob(x+9,y,0); text(x+18,y+4,t,WHITE,1); }
static void famNotice(const char*title,const char*l1,const char*l2,const int*us,int n){
    u16 prev=keyNow(); if(n>4) n=4;
    famBack();
    { static const u16 cf[6]={RGB(31,26,10),RGB(31,16,24),RGB(14,28,31),RGB(16,31,14),RGB(31,20,10),RGB(24,18,31)};   // confetti
      for(int i=0;i<70;i++){ int x=4+rnd8()*232/256, y=4+rnd8()*152/256; rect(x,y,2+(i&1),2,cf[i%6]); } }
    s3Box(16,20,208,124,10,RGB(15,24,31),RGB(8,15,26)); s3Box(17,21,206,122,9,RGB(5,11,22),RGB(2,5,13));
    famBar(18,22,204,title);
    int gap=n>1?184/n:0, x0=120-gap*(n-1)/2;
    for(int k=0;k<n;k++){ int u=us[k], cx=x0+k*gap; famFace(cx,64,n>2?17:21,u);
        const char*nm=uName(u); int w=tw(nm,1)+14; s3Pill(cx-w/2,n>2?84:88,w,11,0,nm); }
    text(120-tw(l1,1)/2,105,l1,WHITE,1); if(l2&&l2[0]) text(120-tw(l2,1)/2,115,l2,RGB(17,29,31),1);
    s3Pill(96,127,48,13,1,"OK");
    present();
    for(;;){ u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) break; vsync(); }
    while((~REG_KEYINPUT)&0x3FF) vsync();   // (the game must not see the A)
}

// ---- THE FAMILY BIN (The Sims 2): one screen to start a pre-made family. UP / DOWN go through the families, their faces in a row; LEFT / RIGHT pick
// who of them you play, described below; A plays them. (From a lot the family is the one living there: only the member is picked.)
static int famPick(int*fp,int lock){   // the member you become (*fp: the family, changed with UP / DOWN unless lock); -1 = back
    int f=*fp, sel=0, dirty=1; u16 prev=keyNow();
    for(;;){
        const HhFam*F=&hhFams[f]; int n=F->n>4?4:F->n; char last[HH_NM]; famLast(F,last);
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(!lock&&(pr&(K_DOWN|K_UP))){ f=(f+((pr&K_DOWN)?1:HH_NFAM-1))%HH_NFAM; sel=0; dirty=1; sfxPlay(SFX_TICK); continue; }
        if(n>1&&(pr&(K_RIGHT|K_R))){ sel=(sel+1)%n; dirty=1; sfxPlay(SFX_TICK); }
        if(n>1&&(pr&(K_LEFT|K_L))){ sel=(sel+n-1)%n; dirty=1; sfxPlay(SFX_TICK); }
        if(pr&K_A){ sfxPlay(SFX_POP); while((~REG_KEYINPUT)&0x3FF) vsync(); *fp=f; return sel; }   // (the next screen must not see the A)
        if(pr&(K_B|K_START)){ while((~REG_KEYINPUT)&0x3FF) vsync(); return -1; }
        if(!dirty){ vsync(); continue; }
        dirty=0;
        famBack();
        s3Box(12,14,216,132,10,RGB(15,24,31),RGB(8,15,26)); s3Box(13,15,214,130,9,RGB(5,11,22),RGB(2,5,13));
        famBar(14,16,212,F->fam);
        if(!lock){ char c[8]; char*e=simCatN(c,f+1); *e++='/'; simCatN(e,HH_NFAM); text(222-tw(c,1),20,c,RGB(17,29,31),1); }   // 3/12
        text(120-tw("WHO DO YOU PLAY?",1)/2,34,"WHO DO YOU PLAY?",RGB(17,29,31),1);
        int gap=n>1?196/n:0, x0=120-gap*(n-1)/2;
        for(int i=0;i<n;i++){ const HhPre*p=&F->m[i]; int cx=x0+i*gap, on=i==sel;
            famFaceL(cx,68,on?21:16,p->look,p->stage,p->sex,hudExpr[on?4:2]);
            int w=tw(p->name,1)+14; s3Pill(cx-w/2,92,w,11,on,p->name); }
        { const HhPre*p=&F->m[sel]; char b[40]; char*e=simCat(b,p->name); if(last[0]){ e=simCat(e," "); simCat(e,last); }
          text(120-tw(b,1)/2,108,b,WHITE,1);
          e=simCat(b,stageNm[p->stage<AG_N?p->stage:AG_ADULT]); e=simCat(e,"  "); simCat(e,sexNm[p->sex<SX_N?p->sex:SX_NB]);
          text(120-tw(b,1)/2,118,b,RGB(17,29,31),1);
          e=simCat(b,"ASPIRES TO "); simCat(e,aspNm[p->asp<AS_N?p->asp:0]);
          text(120-tw(b,1)/2,128,b,GOLD,1); }
        { const char*h=lock?"< > WHO   A PLAY   B BACK":"UP DOWN FAMILY   < > WHO   A PLAY   B BACK"; text(120-tw(h,1)/2,150,h,RGB(12,18,26),1); }
        present();
    }
}
static int famWho(const HhFam*F){ int f=(int)(F-hhFams); return famPick(&f,1); }   // a lot's pre-made family: which of them you play

static void famLookOf(int u,u8*out){ const u8*l=fkLook(u); for(int i=0;i<LK_N;i++) out[i]=l[i]; }
static int famKidOf(int c,int p){ int r=kin[c][p]; return p!=255&&(r==KN_DAUGHTER||r==KN_SON||r==KN_CHILD); }   // is c a child of p?
static int famBaby(const u8*lk,int pa,int pb){   // one baby moves in: its place, -1 = no room
    int m=stAddSim(lk,AG_BABY,famLastOf(pa!=255?pa:pb)); if(m<0) return -1;
    int u=hhM[m].uid; famAge[u]=0; hhM[m].asp=AS_GROW;
    for(int k=0;k<hhN;k++){ int c=hhM[k].uid; if(c!=u&&(famKidOf(c,pa)||famKidOf(c,pb))) kinSib(c,u); }   // the parents' other children: its sisters and brothers
    if(hhPUid!=u&&(famKidOf(hhPUid,pa)||famKidOf(hhPUid,pb))) kinSib(hhPUid,u);
    if(pa!=255){ stRel(pa,u,60,50,RF_FRIEND); kinParent(pa,u); }
    if(pb!=255){ stRel(pb,u,60,50,RF_FRIEND); kinParent(pb,u); }
    return m;
}
static char famT[24] EWRAM_BSS, famL1[48] EWRAM_BSS, famL2[44] EWRAM_BSS;
static int famBorn[2] EWRAM_BSS, famBornN EWRAM_BSS;
static int famBirth(void){   // the due day: one baby, or two. 1 = someone was born
    int pa=famPa, pb=famPb; famDue=0; famPa=famPb=255; famBornN=0;
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
    famBorn[famBornN++]=hhM[m1].uid;
    if(m2>=0){ int a=hhM[m1].uid, b=hhM[m2].uid; stRel(a,b,60,60,RF_FRIEND); kinSib(a,b); famBorn[famBornN++]=b; }   // twins: close from the start
    if(m2>=0){ simCat(famT,same?"IDENTICAL TWINS!":"TWINS!"); char*e=simCat(famL1,hhM[m1].name); e=simCat(e," AND "); e=simCat(e,hhM[m2].name); simCat(e," ARE BORN"); }
    else { int sx=hhM[m1].look[LK_SEX]; simCat(famT,sx==SX_FEMALE?"IT'S A GIRL!":sx==SX_MALE?"IT'S A BOY!":"A NEW BABY!"); simCat(simCat(famL1,hhM[m1].name)," IS BORN"); }
    { const char*l=famLastOf(pa!=255?pa:pb); char*e=simCat(famL2,"WELCOME TO THE "); if(l[0]){ e=simCat(e,l); simCat(e," FAMILY"); } else simCat(e,"FAMILY"); }
    if(pa==hhPUid||pb==hhPUid) moodEvent(M_WANT);
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
static int famNeed(int st){ static const u8 pct[4]={0,200,100,50}; if(st>=AG_ELDER||!xo[XO_AGING]||!oStageDays(st)) return 0; int n=oStageDays(st)*pct[xo[XO_AGING]]/100; return n<1?1:n; }   // 0: never grows
static void famDay(void){   // midnight (simMinute, after your own birthday): everyone else grows, and the baby may come
    int grew=0, born=0, gu[HH_MAX];
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; int u=s->uid, need=famNeed(s->stage);
        if(!need){ famAge[u]=0; continue; }   // OFF or FOREVER: nobody grows
        if(famAge[u]<255) famAge[u]++;
        if(famAge[u]<need) continue;
        famAge[u]=0; famGrow(s); gu[grew++]=u; }
    if(famDue){ famDue--; if(!famDue) born=famBirth(); }
    if(grew||born){
        for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
        hhBakeAll();   // (the new faces first: the notices show them)
        if(born) famNotice(famT,famL1,famL2,famBorn,famBornN);
        if(grew){ int m=hhMemOf(gu[0]);
            if(grew==1&&m>=0){ char*e=simCat(famL1,hhM[m].name); *e++=' '; simCat(e,famGrewW[hhM[m].stage]); famNotice("HAPPY BIRTHDAY!",famL1,famGrewL2[hhM[m].stage],gu,1); }
            else famNotice("HAPPY BIRTHDAYS!","THE FAMILY GREW UP","A YEAR OLDER  A NEW LOOK",gu,grew); }
        hhSlotsFree(); liveInvalidate();
    }
    hhSave();   // (the family tail goes with it)
}

// ---- pause menu > HOUSEHOLD > FAMILY: The Sims 2's family panel ----
static int famDaysLeft(int u){ int st=uStage(u), need=famNeed(st); if(!need) return -1; int had=u==hhPUid?ageDays:famAge[u]; return need>had?need-had:1; }   // -1: never
static int famPartner(int u){ for(int v=0;v<HU_N;v++) if(v!=u&&(v==hhPUid||hhMemOf(v)>=0)&&(kinRom(kin[u][v])||(relF[u][v]&RF_STEADY))) return v; return -1; }
static void famRing(int x,int y){ static const signed char cx[16]={0,1,2,3,3,3,2,1,0,-1,-2,-3,-3,-3,-2,-1}, cy[16]={-3,-3,-2,-1,0,1,2,3,3,3,2,1,0,-1,-2,-3};
    for(int a=0;a<16;a++) rect(x+cx[a],y+cy[a],1,1,RGB(31,27,8)); rect(x,y-4,1,1,RGB(25,31,31)); }   // married: a gold ring with a stone
static void famHeart(int x,int y){ static const u8 hb[5]={0x36,0x7F,0x3E,0x1C,0x08}; for(int j=0;j<5;j++) for(int i=0;i<7;i++) if(hb[j]>>(6-i)&1) rect(x-3+i,y-2+j,1,1,RGB(31,10,18)); }
static void famRelBar(int x,int y,int w,int v){ s2rr(x,y,w,6,RGB(9,16,26)); rect(x+1,y+1,w-2,4,RGB(2,4,9)); int c=x+w/2; rect(c,y,1,6,RGB(16,22,28));
    int f=v*(w/2-1)/100; if(f>0) rect(c+1,y+1,f,4,RGB(7,24,6)); else if(f<0) rect(c+f,y+1,-f,4,RGB(25,6,4)); }
static void famScreen(void){
    int us[HU_N], n=0; us[n++]=hhPUid; for(int m=0;m<hhN;m++) us[n++]=hhM[m].uid;
    int sel=0, dirty=1; u16 prev=keyNow();
    char t[40], d1[52], d2[40];
    { char*e=t; if(hhPLast[0]){ e=simCat(e,"THE "); e=simCat(e,hhPLast); simCat(e," FAMILY"); } else simCat(e,"YOUR FAMILY"); }
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_B|K_START|K_A)) return;
        if(pr&K_RIGHT){ sel=(sel+1)%n; dirty=1; } if(pr&K_LEFT){ sel=(sel+n-1)%n; dirty=1; }
        if((pr&K_DOWN)&&sel+4<n){ sel+=4; dirty=1; } if((pr&K_UP)&&sel>=4){ sel-=4; dirty=1; }
        if(!dirty){ vsync(); continue; }
        dirty=0; famBack(); famBar(3,3,234,t);
        { char c[12]; char*e=c; e+=numStr(e,n); simCat(e,n==1?" SIM":" SIMS"); text(233-tw(c,1),7,c,RGB(17,29,31),1); }
        int big=n<=4, ch=big?102:51, pr2=big?21:13, x0=big?(SW-(n*57-2))/2:6;   // four or fewer: tall cards in the middle with bigger faces
        for(int i=0;i<n;i++){ int u=us[i], x=x0+(i&3)*57, y=19+(i>>2)*54, on=i==sel, cx=x+27, p=famPartner(u);
            if(on) s3Box(x-2,y-2,59,ch+4,7,RGB(28,31,18),RGB(16,29,8));   // the picked card glows green
            s3Box(x,y,55,ch,6,RGB(14,23,31),RGB(7,14,25)); s3Box(x+1,y+1,53,ch-2,5,on?RGB(7,15,27):RGB(5,11,22),on?RGB(3,8,17):RGB(2,5,13));
            if(big) famBob(cx,y+4,famMood(u));
            famFace(cx,big?y+40:y+17,pr2,u);
            if(p>=0){ if(kinWed(kin[u][p])) famRing(x+8,y+7); else famHeart(x+8,y+7); }
            if(u==hhPUid){ s2rr(x+38,y+3,15,8,RGB(16,29,8)); text(x+39,y+4,"YOU",RGB(1,4,0),1); }
            int ty=big?y+67:y+34; const char*nm=uName(u); text(cx-tw(nm,1)/2,ty,nm,WHITE,1);
            const char*w=whoWord(uStage(u),uSex(u)); text(cx-tw(w,1)/2,ty+8,w,RGB(17,29,31),1);
            if(big){ const char*r=u==hhPUid?"THAT'S YOU":kin[u][hhPUid]?kinNm[kin[u][hhPUid]]:relWord(hhPUid,u);
                u16 rc=(u!=hhPUid&&(kinRom(kin[u][hhPUid])||(relF[hhPUid][u]&(RF_STEADY|RF_LOVE|RF_CRUSH))))?RGB(31,19,26):RGB(24,28,31);
                text(cx-tw(r,1)/2,ty+19,r,rc,1); } }
        { int u=us[sel], p=famPartner(u), dl=famDaysLeft(u);   // the picked one, described
          s2rr(3,124,234,21,RGB(10,20,30)); s2rr(4,125,232,19,RGB(2,6,13));
          char*e=simCat(d1,uName(u));
          if(p>=0){ e=simCat(e,kinWed(kin[u][p])?"  MARRIED TO ":"  GOING STEADY WITH "); simCat(e,p==hhPUid?"YOU":uName(p)); }
          text(9,127,d1,WHITE,1);
          e=d2; if(dl<0) e=simCat(e,uStage(u)>=AG_ELDER?"AN ELDER":"NOT AGING"); else { e=simCat(e,"GROWS UP IN "); e+=numStr(e,dl); simCat(e,dl==1?" DAY":" DAYS"); }
          text(9,136,d2,RGB(17,29,31),1);
          if(u!=hhPUid){ text(140,127,"DAILY",RGB(17,29,31),1); famRelBar(172,127,60,relD[hhPUid][u]); text(140,136,"LIFE",RGB(17,29,31),1); famRelBar(172,136,60,relL[hhPUid][u]); } }
        s2pill(5,147,62,"DPAD PICK"); s2pill(70,147,46,"B BACK");
        { static const char* const twn[4]={"TWINS NEVER","TWINS SOMETIMES","TWINS OFTEN","TWINS ALWAYS"}; const char*tt=twn[xo[XO_TWINS]]; int w=tw(tt,1)+12; s2pill(235-w,147,w,tt);
          if(famDue){ char bb[20]; char*e=simCat(bb,"BABY IN "); e+=numStr(e,famDue); simCat(e,famDue==1?" DAY":" DAYS");
            int w2=tw(bb,1)+12, x2=232-w-w2; s3Box(x2,147,w2,11,5,RGB(31,22,27),RGB(26,12,20)); text(x2+6,149,bb,RGB(12,1,7),1); } }
        present();
    }
}
