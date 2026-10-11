// petui.h - PETS, the screens: R next to a pet (its pie menu), PHONE > ADOPT A PET, HOUSEHOLD > PETS, the PET CREATOR and the pet's card.
// (pets.h is the pets themselves: needs, free will, sprites, saving.)
static int petAtHome(void){ return !curFl&&!prHere()&&copHome(); }   // pets stay on your own lot, on the ground floor
static int petCopAt(int*x,int*y){ if(copSt<2) return 0; for(int i=0;i<copN;i++){ if(copS[i].act==HA_AWAY) continue; *x=(int)(copS[i].fx>>8); *y=(int)(copS[i].fy>>8); return 1; } return 0; }
// a pet's frame into the picture at sc times its size (the creator, the card)
static void petBlit(const Pet*p,int view,int pose,int x0,int y0,int sc,int flip){
    u16 pal[8]; pal[0]=0; pal[1]=RGB(2,2,5); pal[2]=RGB(3,2,4); pal[3]=RGB(28,7,9); petRamp(p,pal+4);
    const u32*t=petArt[p->look][p->size][view][pose];
    for(int y=0;y<32;y++)for(int x=0;x<32;x++){ u32 w=t[((y>>3)*4+(x>>3))*8+(y&7)]; int c=(int)((w>>((x&7)*4))&15); if(!c) continue; int xx=flip?31-x:x; rect(x0+xx*sc,y0+y*sc,sc,sc,pal[c]); }
}
static void piePetHub(int cx,int cy,int i){ const Pet*p=&petM[i]; petBlit(p,0,PF_SIT,cx-16,cy-21,1,0); }   // pie.h
static void petBar(int x,int y,int w,int v){ rect(x,y,w,4,RGB(3,4,8)); int f=v*w/100; if(f>0) rect(x,y,f,4,v>=55?RGB(9,27,8):v>=28?RGB(29,25,5):RGB(30,7,6)); }
static void petRandName(Pet*p){ const char*s=petCat(p)?petNameCat[rnd8()&15]:petNameDog[rnd8()&15]; int j=0; while(s[j]&&j<HH_NM-1){ p->name[j]=s[j]; j++; } p->name[j]=0; }
static void petNew(Pet*p,int breed){   // a new pet of a breed (its colours and size), with a name and middling traits
    const PetBreed*b=&petBreed[breed]; for(int j=0;j<(int)sizeof(Pet);j++) ((u8*)p)[j]=0;
    p->look=b->look; p->size=p->adult=b->size; p->coat=b->coat; p->acc=b->acc;
    for(int j=0;j<PTR_N;j++) p->tr[j]=(u8)(3+rnd8()%5);
    p->need[PN_FOOD]=80; p->need[PN_BLAD]=90; p->need[PN_NRG]=90; p->need[PN_FUN]=70; p->love=40; petRandName(p);
}
// ---- the PET CREATOR: every row LEFT / RIGHT, A on NAME types one, L / R turn the pet, START or A on DONE keeps it, B cancels ----
enum { PE_BREED, PE_BODY, PE_SIZE, PE_YOUNG, PE_COAT, PE_ACC, PE_SMART, PE_HYPER, PE_FRIEND, PE_NAME, PE_DONE, PE_N };
static int petEdit(Pet*p,int isNew){
    static const char* const rl[PE_N]={"BREED","BODY","SIZE","AGE","COAT","ACCENT","SMART","HYPER","FRIENDLY","NAME","DONE"};
    Pet keep=*p; int row=0, br=-1, rot=0, young=p->size<p->adult, dirty=3; u16 prev=keyNow(); KRep kr[2]={{0,0},{0,0}}; u32 t0=vblN; int fr=-1;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_B){ *p=keep; return 0; }
        if((pr&K_START)||((pr&K_A)&&row==PE_DONE)){ sfxPlay(SFX_CASH); return 1; }
        if(kRep(&kr[0],k&K_DOWN,pr&K_DOWN)){ row=(row+1)%PE_N; dirty|=2; }
        if(kRep(&kr[0],k&K_UP,pr&K_UP)){ row=(row+PE_N-1)%PE_N; dirty|=2; }
        if(pr&K_R){ rot=(rot+1)&3; dirty|=1; }
        if(pr&K_L){ rot=(rot+3)&3; dirty|=1; }
        if((pr&K_A)&&row==PE_NAME){ char nm[HH_NM]; for(int j=0;j<HH_NM;j++) nm[j]=p->name[j]; if(nameEdit(nm,HH_NM-1,"NAME YOUR PET",0)){ for(int j=0;j<HH_NM;j++) p->name[j]=nm[j]; } prev=keyNow(); dirty=3; continue; }
        int dv=kRep(&kr[1],k&K_RIGHT,pr&K_RIGHT)?1:0;
        { static KRep kl; if(kRep(&kl,k&K_LEFT,pr&K_LEFT)) dv=-1; }
        if(dv){ dirty|=3;
            switch(row){
                case PE_BREED: { int was=petCat(p); br=(br<0?(dv>0?0:PET_NBR-1):(br+dv+PET_NBR)%PET_NBR); const PetBreed*b=&petBreed[br]; p->look=b->look; p->adult=b->size; p->size=young?0:b->size; p->coat=b->coat; p->acc=b->acc; if(isNew&&was!=petCat(p)) petRandName(p); break; }
                case PE_BODY: { int was=petCat(p); p->look=(u8)((p->look+dv+PET_LOOKS)%PET_LOOKS); if(isNew&&was!=petCat(p)) petRandName(p); break; }
                case PE_SIZE: p->adult=(u8)((p->adult+dv+3)%3); p->size=young?0:p->adult; break;
                case PE_YOUNG: young=!young; p->size=young?0:p->adult; if(!young) p->age=6; else p->age=0; break;
                case PE_COAT: p->coat=(u8)((p->coat+dv+PET_NC)%PET_NC); break;
                case PE_ACC: p->acc=(u8)((p->acc+dv+PET_NC)%PET_NC); break;
                case PE_SMART: case PE_HYPER: case PE_FRIEND: { int t=row-PE_SMART, v=p->tr[t]+dv; if(v<0) v=0; if(v>10) v=10; p->tr[t]=(u8)v; break; }
                case PE_NAME: petRandName(p); break;
                default: break; }
            }
        int f=(int)((vblN-t0)/24)%6; if(f!=fr){ fr=f; dirty|=1; }
        if(!dirty){ vsync(); continue; }
        if(dirty&2){ box(3,1,234,157); text(10,6,isNew?"ADOPT A PET":"RESTYLE YOUR PET",GOLD,1); }
        if(dirty&1){   // the pet, turning and moving
            static const u8 cyc[6]={PF_STAND,PF_WALK1,PF_STAND,PF_WALK2,PF_SIT,PF_HAPPY};
            rect(8,18,100,106,RGB(9,14,9)); rect(8,104,100,20,RGB(7,11,7));
            petBlit(p,rot>>1,cyc[fr],10,14,3,rot&1);
            text(12,112,petLookNm[p->look],WHITE,1); text(12,118,young?"YOUNG":"GROWN UP",DIMC,1); }
        if(dirty&2){
            for(int r=0;r<PE_N;r++){ int y=20+r*11, on=r==row; char v[24]; v[0]=0;
                switch(r){
                    case PE_BREED: simCat(v,br<0?"PICK ONE":petBreed[br].nm); break;
                    case PE_BODY: simCat(v,petLookNm[p->look]); break;
                    case PE_SIZE: simCat(v,petSizeNm[p->adult]); break;
                    case PE_YOUNG: simCat(v,young?(petCat(p)?"KITTEN":"PUPPY"):"GROWN UP"); break;
                    case PE_COAT: simCat(v,petColNm[p->coat]); break;
                    case PE_ACC: simCat(v,petColNm[p->acc]); break;
                    case PE_SMART: case PE_HYPER: case PE_FRIEND: break;
                    case PE_NAME: simCat(v,p->name); break;
                    default: simCat(v,isNew?"ADOPT  \xC2\xA7" "150":"KEEP IT"); }
                rect(112,y-1,120,10,on?RGB(6,16,8):RGB(3,4,7)); text(115,y+1,rl[r],on?GOLD:DIMC,1);
                if(r>=PE_SMART&&r<=PE_FRIEND){ int t=p->tr[r-PE_SMART]; for(int q=0;q<10;q++) rect(170+q*6,y+1,5,6,q<t?(on?GOLD:RGB(20,22,26)):RGB(6,7,10)); }
                else if(r==PE_COAT||r==PE_ACC){ rect(170,y,8,8,petCol[r==PE_COAT?p->coat:p->acc]); text(181,y+1,v,on?WHITE:DIMC,1); }
                else text(170,y+1,v,on?WHITE:DIMC,1); }
            text(10,140,"UP DN ROW  LEFT RIGHT CHANGE  L R TURN",DIMC,1);
            text(10,148,row==PE_NAME?"A TYPE A NAME  RIGHT A RANDOM ONE":"START DONE  B CANCEL",DIMC,1); }
        dirty=0; present();
    }
}
// ---- the pet's card: its needs, traits and tricks ----
static void petCard(int i){
    const Pet*p=&petM[i]; u16 prev=keyNow(); int drawn=0;
    for(;;){ u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) return;
        if(drawn){ vsync(); continue; } drawn=1;
        box(3,1,234,157); text(10,6,p->name,GOLD,1); { char t[40]; char*e=simCat(t,petSizeNm[p->size]); *e++=' '; e=simCat(e,petColNm[p->coat]); *e++=' '; simCat(e,petLookNm[p->look]); text(10,15,t,DIMC,1); }
        rect(8,26,100,100,RGB(9,14,9)); petBlit(p,0,p->need[PN_NRG]<30?PF_LIE:PF_SIT,10,22,3,0);
        static const char* const nn[PN_N+1]={"FOOD","BLADDER","ENERGY","FUN","LOVE"};
        for(int n=0;n<=PN_N;n++){ int y=28+n*11; text(116,y,nn[n],WHITE,1); petBar(170,y+1,56,n<PN_N?p->need[n]:p->love); }
        static const char* const tn[PTR_N]={"SMART","HYPER","FRIENDLY"};
        for(int t=0;t<PTR_N;t++){ int y=86+t*10; text(116,y,tn[t],DIMC,1); for(int q=0;q<10;q++) rect(170+q*6,y+1,5,6,q<p->tr[t]?RGB(20,22,26):RGB(6,7,10)); }
        { char t[48]; char*e=simCat(t,"HOUSE TRAINED "); e=simCatN(e,p->potty); simCat(e,"%"); text(10,130,t,DIMC,1); }
        { char t[64]; char*e=simCat(t,"TRICKS "); int any=0; for(int q=0;q<PET_TRK;q++) if(p->tricks>>q&1){ if(any) e=simCat(e,", "); e=simCat(e,petTrickNm[petCat(p)][q]); any=1; } if(!any) simCat(e,"NONE YET"); text(10,139,t,WHITE,1); }
        text(10,149,"A BACK",DIMC,1); present(); }
}
// ---- R next to a pet ----
static void petFaceMe(Pet*p){ petFace(p,lfx,lfy); s32 dx=p->w.fx-lfx, dy=p->w.fy-lfy; lhd=(u8)((dx<0?-dx:dx)>(dy<0?-dy:dy)?(dx>0?0:8):(dy>0?4:12)); }
static void petTricks(Pet*p,int train){
    const char* it[PET_TRK]; int id[PET_TRK], n=0;
    for(int q=0;q<PET_TRK;q++) if(((p->tricks>>q)&1)!=train){ it[n]=petTrickNm[petCat(p)][q]; id[n++]=q; }
    if(!n){ toast(train?"IT KNOWS EVERY TRICK":"NO TRICKS YET  TRAIN ONE"); return; }
    int c=menu(train?"TRAIN A TRICK":"DO A TRICK",it,n); if(c<0) return; int q=id[c];
    static const u8 tp[2][PET_TRK]={{PF_SIT,PF_HAPPY,PF_LIE,PF_SLEEP,PF_HAPPY},{PF_HAPPY,PF_SIT,PF_STAND,PF_HAPPY,PF_SIT}};
    p->act=PA_TRICK; p->go=0; p->w.pn=p->w.pi=0; p->t=90; p->tgt=tp[petCat(p)][q]; lstun=70; lsp=0;
    if(train){
        if(p->need[PN_NRG]<15){ p->act=PA_IDLE; petNote(p," IS TOO TIRED TO LEARN"); return; }
        p->need[PN_NRG]=(u8)(p->need[PN_NRG]-8);
        int pct=18+p->tr[PTR_SMART]*6+p->love/5;
        if((int)(((int)rnd8()*100>>8))<pct){ p->tricks|=(u8)(1<<q); p->love=(u8)(p->love>96?100:p->love+4); petSay(p,IC_STAR,90); char*e=simCat(petMsg,p->name); e=simCat(e," LEARNED "); simCat(e,petTrickNm[petCat(p)][q]); lnote=petMsg; lnoteT=150; moodEvent(M_SKILL); }
        else { static const char* const dogNo[4]={" STARES AT YOU BLANKLY"," EATS A SOCK INSTEAD"," CHASES ITS TAIL"," SNEEZES AND FORGETS"}, * const catNo[4]={" IGNORES YOU"," LICKS ITSELF INSTEAD"," WALKS AWAY SLOWLY"," KNOCKS A CUP OFF THE TABLE"};
            p->act=PA_SIT; petNote(p,(petCat(p)?catNo:dogNo)[rnd8()&3]); } }
    else { petSay(p,IC_HEART,80); p->love=(u8)(p->love>98?100:p->love+1); moodEvent(M_CHILL); char*e=simCat(petMsg,p->name); e=simCat(e,q==3&&!petCat(p)?" PLAYS DEAD  BANG":q==4&&!petCat(p)?" SAYS WOOF":" DOES IT  GOOD JOB"); (void)e; lnote=petMsg; lnoteT=150; }
}
static int petPie(int i){
    Pet*p=&petM[i]; int cat=petCat(p);
    static const char* it[12] EWRAM_BSS; static char tl[40] EWRAM_BSS; int id[12], ct[12], n=0;
    it[n]=cat?"SCRATCH CHIN":"PET"; ct[n]=0; id[n++]=0;
    if(p->love>=40){ it[n]="CUDDLE"; ct[n]=0; id[n++]=1; }
    it[n]=cat?"DANGLE A STRING":"PLAY FETCH"; ct[n]=1; id[n++]=2;
    if(p->tricks){ it[n]="DO A TRICK"; ct[n]=1; id[n++]=3; }
    it[n]="TRAIN A TRICK"; ct[n]=1; id[n++]=4;
    it[n]="FEED  \xC2\xA7" "5"; ct[n]=4; id[n++]=5;
    it[n]="LET IT OUT"; ct[n]=4; id[n++]=6;
    it[n]="HOUSE TRAIN"; ct[n]=4; id[n++]=7;
    it[n]="SCOLD"; ct[n]=3; id[n++]=8;
    it[n]="SHOO"; ct[n]=3; id[n++]=9;
    it[n]="ABOUT"; ct[n]=4; id[n++]=10;
    { char*e=simCat(tl,p->name); e=simCat(e,"  LOVE "); simCatN(e,p->love); }
    piePet=(u8)(i+1); int c=pieCats(tl,it,ct,n); piePet=0; liveInvalidate();
    while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    petFaceMe(p); p->w.pn=p->w.pi=0; p->go=0;
    switch(id[c]){
        case 0: if(cat&&p->tr[PTR_FRIEND]<4&&p->love<30&&(rnd8()&1)){ p->act=PA_SIT; p->t=60; petSay(p,IC_ANGRY,60); lhp=lhp>3?lhp-3:1; petNote(p," SWATS YOUR HAND"); lstun=30; lsp=0; break; }
            p->love=(u8)(p->love>96?100:p->love+4); if(p->need[PN_FUN]<95) p->need[PN_FUN]+=5; p->act=PA_SIT; p->t=80; petSay(p,IC_HEART,80);
            sSoc=sSoc>94?100:sSoc+6; moodEvent(M_CHILL); lstun=45; lsp=0; petNote(p,cat?" PURRS":" WAGS ITS TAIL"); break;
        case 1: p->love=(u8)(p->love>94?100:p->love+6); p->act=PA_LIE; p->t=120; petSay(p,IC_HEART,110);
            sSoc=sSoc>90?100:sSoc+10; sCom=sCom>92?100:sCom+8; moodEvent(M_CHILL); lstun=90; lsp=0; petNote(p," SNUGGLES UP"); break;
        case 2: if(p->need[PN_NRG]<15){ petNote(p," IS TOO TIRED TO PLAY"); break; }
            p->need[PN_FUN]=(u8)(p->need[PN_FUN]>70?100:p->need[PN_FUN]+30); p->need[PN_NRG]=(u8)(p->need[PN_NRG]-10); p->love=(u8)(p->love>97?100:p->love+3);
            moodEvent(M_CHILL); lstun=60; lsp=0; if(petPlan(p,0,0,0)) petStartAct(p,PA_ZOOM,0); petNote(p,cat?" POUNCES ON THE STRING":" BRINGS IT BACK  AGAIN"); break;
        case 3: petTricks(p,0); break;
        case 4: petTricks(p,1); break;
        case 5: if(p->need[PN_FOOD]>=90){ petNote(p," IS NOT HUNGRY"); break; } if(simMoney<5){ toast("NOT ENOUGH SIMOLEONS"); break; }
            simMoney-=5; p->need[PN_FOOD]=100; p->act=PA_EAT; p->t=120; p->love=(u8)(p->love>97?100:p->love+3); sfxPlay(SFX_MUNCH); lstun=40; lsp=0; petNote(p,cat?" EATS  THEN WANTS MORE":" INHALES IT"); break;
        case 6: if(petOutside((int)(p->w.fx>>8),(int)(p->w.fy>>8))){ p->act=PA_PEE; p->t=60; p->need[PN_BLAD]=100; break; }
            if(petPlan(p,2,0,0)){ petStartAct(p,PA_OUT,0); petNote(p," TROTS OUTSIDE"); } else petNote(p," HAS NOWHERE TO GO  ADD A DOOR"); break;
        case 7: { int g=3+p->tr[PTR_SMART]/2; p->potty=(u8)(p->potty+g>100?100:p->potty+g); p->act=PA_SIT; p->t=60; lstun=50; lsp=0; char*e=simCat(petMsg,p->name); e=simCat(e," IS "); e=simCatN(e,p->potty); simCat(e,"% HOUSE TRAINED"); lnote=petMsg; lnoteT=150; break; }
        case 8: p->love=(u8)(p->love>5?p->love-5:0); if(pudN&&p->potty<100) p->potty=(u8)(p->potty+4>100?100:p->potty+4); p->act=PA_LIE; p->t=90; petSay(p,IC_SAD,90); lstun=30; lsp=0; petNote(p,cat?" DOES NOT CARE":" LOOKS ASHAMED"); break;
        case 9: p->love=(u8)(p->love>2?p->love-2:0); if(petPlan(p,0,0,0)) petStartAct(p,PA_WANDER,0); petNote(p," WANDERS OFF"); break;
        case 10: petCard(i); break; }
    return 1;
}
static int petR(void){   // R: a puddle within reach is mopped; else the nearest pet's menu (a Sim who is nearer gets theirs)
    if(!petAtHome()) return 0;
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int k=0;k<pudN;k++){ int dx=pudX[k]-px, dy=pudY[k]-py; if(dx>=-1&&dx<=1&&dy>=-1&&dy<=1){ for(int j=k+1;j<pudN;j++){ pudX[j-1]=pudX[j]; pudY[j-1]=pudY[j]; } pudN--;
            lstun=60; lsp=0; sHyg=sHyg>5?sHyg-5:0; lnote="MOPPED IT UP  GROSS"; lnoteT=70; sfxPlay(SFX_SPLASH); return 1; } }
    int best=-1, bd=1<<30;
    for(int i=0;i<petN;i++){ s32 dx=petM[i].w.fx-lfx, dy=petM[i].w.fy-lfy; int d=(int)((dx*dx+dy*dy)>>8); if(d<bd&&d<=(420*420>>8)&&wallClear(lfx,lfy,petM[i].w.fx,petM[i].w.fy)){ bd=d; best=i; } }
    if(best<0) return 0;
    { int m=hhNearest(); if(m>=0){ s32 dx=hhM[m].fx-lfx, dy=hhM[m].fy-lfy; if((int)((dx*dx+dy*dy)>>8)<bd) return 0; } }
    return petPie(best);
}
// ---- PHONE > ADOPT A PET, HOUSEHOLD > PETS ----
#define PET_COST 150
static void petAdopt(void){
    if(petN>=PET_MAX){ toast("THREE PETS IS PLENTY"); return; }
    if(simMoney<PET_COST){ toast("NOT ENOUGH SIMOLEONS"); return; }
    Pet*p=&petM[petN]; petNew(p,rnd8()%PET_NBR);
    if(!petEdit(p,1)) return;
    simMoney-=PET_COST; simsSave(); petN++; petPlace(p,petN); p->act=PA_GREET; p->go=0; p->t=90; petSay(p,IC_HEART,120); petVramBad=1; hhSave();
    char*e=simCat(petMsg,p->name); simCat(e,petCat(p)?" MOVED IN  MEOW":" MOVED IN  WOOF"); toast(petMsg); liveInvalidate(); camSnap=1;
}
static void petScreen(void){
    for(;;){
        if(!petN){ static const char* const it0[1]={"ADOPT A PET  \xC2\xA7" "150"}; if(menu("NO PETS YET",it0,1)==0&&!prIn()) petAdopt(); return; }
        const char* it[PET_MAX]; for(int i=0;i<petN;i++) it[i]=petM[i].name;
        int c=menu("PETS",it,petN); if(c<0){ liveInvalidate(); return; }
        static const char* const op[4]={"ABOUT","RESTYLE","RENAME","GIVE AWAY"};
        int o=menu(petM[c].name,op,4);
        if(o==0) petCard(c);
        else if(o==1){ if(petEdit(&petM[c],0)){ petVramBad=1; hhSave(); } }
        else if(o==2){ char nm[HH_NM]; for(int j=0;j<HH_NM;j++) nm[j]=petM[c].name[j]; if(nameEdit(nm,HH_NM-1,"RENAME",0)){ for(int j=0;j<HH_NM;j++) petM[c].name[j]=nm[j]; hhSave(); } }
        else if(o==3){ static const char* const yn[2]={"YES  GOODBYE","NO"}; if(menu("GIVE AWAY FOR GOOD?",yn,2)==0){ for(int j=c+1;j<petN;j++) petM[j-1]=petM[j]; petN--; petVramBad=1; hhSave(); toast("THEY WENT TO A GOOD HOME"); } }
    }
}
