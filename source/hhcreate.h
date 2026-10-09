// hhcreate.h - CREATE-A-HOUSEHOLD: the creator's DONE tab > HOUSEHOLD. Make the whole household in the creator: add Sims, say who is whose mother,
// sister, roommate, wife ... and see it all on one screen.
//
// THE HUB (HOUSEHOLD  n OF 8)
//   ADD THIS SIM      the look on screen (and its persona) joins the household, you name it, then say who it is related to. Change the look
//                     and add the next one. The Sim left on screen is the one you play.
//   SET A RELATION    pick two Sims, then what the first IS to the second (ROOMMATE, MOTHER, FATHER, DAUGHTER, SON, SISTER, BROTHER, WIFE, HUSBAND,
//                     PARTNER, MORE KIN: grandparents, grandchildren, aunt, uncle, niece, nephew, cousin, step family, PARENT / CHILD / SIBLING).
//                     When the other side could be two or three things (a mother's child is a DAUGHTER or a SON) you pick that too.
//   WHO IS WHO        one Sim's household at a glance; LEFT RIGHT change whose.
//   EDIT OR MOVE OUT  main.c's famMenu: play as one of them in the creator, or move them out.
//   SAME LAST NAME    everyone gets your last name.
//
// WHAT A RELATION DOES
//   It is kept in kin[a][b] (house.h): what a is TO b, saved in the household block ('H?'), so room slots, the household bank and every
//   other save of a household carry it. Family (everything from KN_MOTHER up) can never romance each other (socAllowed). Setting a relation
//   also lifts how well the two know each other to something that fits (family 60 / 70, partners and spouses are a couple, roommates as they are):
//   it only ever raises the scores, so a household that has lived together keeps its history. Ages are checked (hcStageOk): a baby cannot be
//   a mother, a child cannot be a wife.
//
// Needs before it: house.h (hhM, hhN, hhAdd, kin, kinNm, kinInv, hhSave / hhLoad, uName, uStage, ageBand), main.c's nameEdit, famMenu and the UI kit.

#define HC_GATE 0   // 1: the HOUSEHOLD row works only with the debug code (the way ADD TO FAMILY and FAMILY used to); 0: always

static void hcCut(char*d,const char*s){ int k=0; while(s[k]&&k<6){ d[k]=s[k]; k++; } d[k]=0; }   // a name cut to six letters, for the menu titles
static int hcSame(const char*a,const char*b){ while(*a&&*a==*b){ a++; b++; } return *a==*b; }
static int hcRoster(const char**lb,int*uid,int skip){   // you first, then the members: their names and uids (skip: a uid to leave out, -1 none). Returns how many
    static char nm[HH_MAX+1][HH_NM+8] EWRAM_BSS; int n=0;
    for(int i=0;i<=hhN;i++){
        int u=i==0?hhPUid:hhM[i-1].uid; if(u==skip) continue;
        char*e=simCat(nm[n],i==0?hhPName:hhM[i-1].name);
        if(i==0&&!hcSame(hhPName,"YOU")) simCat(e," (YOU)");
        lb[n]=nm[n]; uid[n]=u; n++;
    }
    return n;
}
static int hcStageOk(int r,int self,int other){   // may a Sim of life stage self be the r of one of stage other?
    switch(r){
        case KN_MOTHER: case KN_FATHER: case KN_PARENT: case KN_STEPMOM: case KN_STEPDAD: return self>=AG_TEEN&&self>=other;
        case KN_DAUGHTER: case KN_SON: case KN_CHILD: case KN_STEPDAU: case KN_STEPSON: return other>=AG_TEEN&&other>=self;
        case KN_GRANDMA: case KN_GRANDPA: return self>=AG_ADULT&&self>=other;
        case KN_GRANDDAU: case KN_GRANDSON: return other>=AG_ADULT&&other>=self;
        case KN_WIFE: case KN_HUSBAND: case KN_PARTNER: return self>=AG_TEEN&&other>=AG_TEEN&&ageBand(self)==ageBand(other);
        default: return 1;
    }
}
static void hcBond(int a,int b,int r){   // a is b's r: how well a knows and likes b grows to fit (never lower)
    int d=40, l=50;
    switch(r){
        case KN_PARTNER: case KN_WIFE: case KN_HUSBAND: d=70; l=80; break;
        case KN_MOTHER: case KN_FATHER: case KN_PARENT: case KN_DAUGHTER: case KN_SON: case KN_CHILD: case KN_SISTER: case KN_BROTHER: case KN_SIBLING:
        case KN_GRANDMA: case KN_GRANDPA: case KN_GRANDDAU: case KN_GRANDSON: d=60; l=70; break;
        case KN_AUNT: case KN_UNCLE: case KN_NIECE: case KN_NEPHEW: case KN_COUSIN: d=50; l=60; break;
        case KN_STEPMOM: case KN_STEPDAD: case KN_STEPDAU: case KN_STEPSON: d=45; l=55; break;
        default: break;   // ROOMMATE: as they are
    }
    if(d>relD[a][b]) relD[a][b]=(signed char)d;
    if(l>relL[a][b]) relL[a][b]=(signed char)l;
    if(relD[a][b]>=50) relF[a][b]|=RF_FRIEND;   // (already friends: no NEW FRIEND note the first day)
    if(kinRom(r)) relF[a][b]|=(u8)(RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF);   // a couple, as the pre-made ones are
}
static void hcSetKin(int a,int b,int ra,int rb){   // a is b's ra and b is a's rb (KN_NONE takes the relation away)
    int was=kinRom(kin[a][b])||kinRom(kin[b][a]);
    kin[a][b]=(u8)ra; kin[b][a]=(u8)rb;
    { int x=kinSex(ra); if(x>=0) uSetSex(a,x); x=kinSex(rb); if(x>=0) uSetSex(b,x); }   // a gendered role sets the GENDER (a MOTHER is a woman)
    if(ra) hcBond(a,b,ra);
    if(rb) hcBond(b,a,rb);
    if(was&&!kinRom(ra)){ relF[a][b]&=(u8)~(RF_CRUSH|RF_LOVE|RF_STEADY); relF[b][a]&=(u8)~(RF_CRUSH|RF_LOVE|RF_STEADY); }   // not a couple any more
    hhSave();
}
static int hcPickRole(const char*title){   // the relation picker: a KN_ role (KN_NONE: no relation), or -1 when backed out. B goes back a page
    static const u8 p0[10]={KN_ROOMMATE,KN_MOTHER,KN_FATHER,KN_DAUGHTER,KN_SON,KN_SISTER,KN_BROTHER,KN_WIFE,KN_HUSBAND,KN_PARTNER};
    static const u8 p1[9]={KN_GRANDMA,KN_GRANDPA,KN_GRANDDAU,KN_GRANDSON,KN_AUNT,KN_UNCLE,KN_NIECE,KN_NEPHEW,KN_COUSIN};
    static const u8 p2[7]={KN_STEPMOM,KN_STEPDAD,KN_STEPDAU,KN_STEPSON,KN_PARENT,KN_CHILD,KN_SIBLING};
    int page=0;
    for(;;){
        const char* it[12]; int id[12], n=0;
        const u8*t=page==0?p0:page==1?p1:p2; int cnt=page==0?10:page==1?9:7;
        for(int i=0;i<cnt;i++){ it[n]=kinNm[t[i]]; id[n]=t[i]; n++; }
        if(page==0){ it[n]="MORE KIN..."; id[n]=-2; n++; it[n]="NO RELATION"; id[n]=KN_NONE; n++; }
        else if(page==1){ it[n]="STEP AND MORE..."; id[n]=-3; n++; it[n]="BACK"; id[n]=-4; n++; }
        else { it[n]="BACK"; id[n]=-4; n++; }
        int c=menu(title,it,n);
        if(c<0){ if(page==0) return -1; page--; continue; }
        if(id[c]==-2){ page=1; continue; }
        if(id[c]==-3){ page=2; continue; }
        if(id[c]==-4){ page--; continue; }
        return id[c];
    }
}
static int hcRelate(int a,int b){   // ask what a is to b (and what b is to a), check the ages, set it. 1 = set
    char t[32], x[8], y[8]; hcCut(x,uName(a)); hcCut(y,uName(b));
    { char*e=simCat(t,x); e=simCat(e," IS "); e=simCat(e,y); simCat(e,"'S"); }
    int ra=hcPickRole(t); if(ra<0) return 0;
    int rb=KN_NONE;
    if(ra!=KN_NONE){
        const u8*iv=kinInv[ra]; const char* it[3]; int id[3], n=0;
        for(int i=0;i<3;i++) if(iv[i]){ it[n]=kinNm[iv[i]]; id[n]=iv[i]; n++; }
        if(n==1) rb=id[0];
        else if(uSex(b)<SX_NB&&iv[uSex(b)]) rb=iv[uSex(b)];   // GENDER answers it: a mother's child who is a boy is her SON
        else if(uSex(b)==SX_NB&&iv[2]) rb=iv[2];
        else { char*e=simCat(t,y); e=simCat(e," IS "); e=simCat(e,x); simCat(e,"'S"); int c=menu(t,it,n); if(c<0) return 0; rb=id[c]; }
        if(!hcStageOk(ra,uStage(a),uStage(b))||!hcStageOk(rb,uStage(b),uStage(a))){ toast("THEIR AGES DO NOT FIT"); return 0; }
    }
    hcSetKin(a,b,ra,rb);
    if(ra){ static char q[40] EWRAM_BSS; char*e=simCat(q,kinNm[ra]); e=simCat(e,"  AND  "); simCat(e,kinNm[rb]); toast(q); } else toast("NO RELATION");
    return 1;
}
static void hcAfterAdd(int nu){   // the new Sim nu: who is it related to?
    const char* lb[HH_MAX+2]; int uid[HH_MAX+2]; int n=hcRoster(lb,uid,nu), c=0;
    if(n<=0) return;
    if(n>1){ lb[n]="NO ONE"; c=menu("RELATED TO WHO?",lb,n+1); if(c<0||c==n) return; }
    hcRelate(nu,uid[c]);
}
static void hcAdd(void){   // ADD THIS SIM
    if(!xo[XO_SIMUSER]){ toast("USER-MADE SIMS ARE OFF"); return; }
    if(custom){ toast("BLOCK-BUILT BODIES STAY YOURS"); return; }
    hhLoad(); int m=hhAdd(look,stage,pAsp,pLtw,pTr);
    if(m<0){ toast("THE HOUSE IS FULL"); return; }
    hhSave();
    nameEdit(hhM[m].name,HH_NM-1,"NAME THIS SIM",0); hhSave();
    hcAfterAdd(hhM[m].uid);
    static char t[36] EWRAM_BSS; char*e=simCat(t,hhM[m].name); e=simCat(e," JOINS  "); e=simCatN(e,hhN+1); e=simCat(e," OF "); simCatN(e,HH_MAX+1); toast(t);
}
static void hcSet(void){   // SET A RELATION: two Sims, then the picker
    const char* lb[HH_MAX+1]; int uid[HH_MAX+1]; int n=hcRoster(lb,uid,-1);
    if(n<2){ toast("ADD ANOTHER SIM FIRST"); return; }
    int a=menu("WHO FIRST?",lb,n); if(a<0) return;
    int ua=uid[a];
    int n2=hcRoster(lb,uid,ua);   // (the same name buffers: the first list is no longer needed)
    int b=menu("RELATED TO WHO?",lb,n2); if(b<0) return;
    hcRelate(ua,uid[b]);
}
static void hcLast(void){   // SAME LAST NAME
    if(!hhPLast[0]){ toast("GIVE YOURSELF A LAST NAME FIRST"); return; }
    if(!hhN){ toast("ONLY YOU LIVE HERE SO FAR"); return; }
    for(int m=0;m<hhN;m++) for(int i=0;i<HH_NM;i++) hhM[m].last[i]=hhPLast[i];
    hhSave(); toast("EVERYONE HAS YOUR LAST NAME");
}
static void hcTree(void){   // WHO IS WHO: one Sim's household, LEFT RIGHT change whose
    const char* lb[HH_MAX+1]; int uid[HH_MAX+1]; int n=hcRoster(lb,uid,-1), f=0;
    if(n<2){ toast("ONLY YOU LIVE HERE SO FAR"); return; }
    u16 prev=keyNow();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_A|K_B|K_START)) return;
        if(pr&K_RIGHT) f=(f+1)%n;
        if(pr&K_LEFT) f=(f+n-1)%n;
        box(3,1,234,157);
        { char t[40]; const char*nm=uName(uid[f]); if(hcSame(nm,"YOU")) simCat(t,"YOUR HOUSEHOLD"); else simCat(simCat(t,nm),"'S HOUSEHOLD"); text(10,6,t,GOLD,1); }
        text(10,17,"SIM",DIMC,1); text(104,17,"IS THEIR",DIMC,1);
        for(int i=0,row=0;i<n;i++){
            if(i==f) continue;
            int y=27+row*16, u=uid[i], r=kin[u][uid[f]];
            rect(6,y-2,226,13,(row&1)?RGB(5,8,16):RGB(3,4,7)); row++;
            text(10,y,uName(u),WHITE,1);
            text(104,y,r?kinNm[r]:"NO RELATION SET",r?RGB(16,26,16):DIMC,1);
            if(kinRom(r)) simIcon(220,y+1,IC_HEART,RGB(31,14,20));
        }
        text(10,148,"LEFT RIGHT  PICK A SIM   A  BACK",RGB(12,14,16),1);
        present();
    }
}
static void hcMenu(void){   // the DONE tab's HOUSEHOLD row
    if(HC_GATE&&!dbgOn){ toast("NEEDS THE DEBUG CODE"); return; }
    hhLoad();
    static const char* const it[6]={"ADD THIS SIM","SET A RELATION","WHO IS WHO","EDIT OR MOVE OUT","SAME LAST NAME","BACK"};
    for(;;){
        char t[24]; { char*e=simCat(t,"HOUSEHOLD  "); e=simCatN(e,hhN+1); e=simCat(e," OF "); simCatN(e,HH_MAX+1); }
        int c=menu(t,it,6); if(c<0||c==5) return;
        if(c==0) hcAdd(); else if(c==1) hcSet(); else if(c==2) hcTree(); else if(c==3) famMenu(); else hcLast();
    }
}
