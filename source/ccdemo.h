// ccdemo.h - THE BORE CREATOR DEMO (make creator -> bore-creator.gba, built with -DCC_DEMO): just the creature creator, like Spore's own
// Creature Creator. Every part and slider is free; there is no life, town or map. What you make goes to MY CREATIONS (a gallery on the save
// chip) and every creation has a SHARE CODE (sharecode.h) that the full game's creator takes too (DONE tab: SHARE CODE / ENTER A CODE).
//   THE HUB     your Bore on the left, the menu on the right: CREATE A NEW BORE, MY CREATIONS, ENTER A SHARE CODE, ABOUT.
//   GALLERY     flip through your creations (LEFT RIGHT): A edit, START share code, SELECT delete, B back.
//   SAVING      DONE tab: SAVE CREATION (asks for a name the first time). Editing a creation saves over it; MAIN MENU asks when unsaved.
// The gallery: from SLOT_BASE, CC_N records of CC_REC bytes: 'B' 'C' looks-count stage asp ltw traits(5) first(12) last(12) looks sum.
#define CC_OFF  SLOT_BASE
#define CC_REC  192
#define CC_MAX  48
#define CC_HDR  (6+TR_N+2*HH_NM)
_Static_assert(CC_HDR+LK_N+1<=CC_REC,"a creation must fit its record");
static int ccEdit=-1;     // the creation being edited (saved over), -1 = a new one
static u8 ccSnap[LK_N+TR_N+3] EWRAM_BSS;
static void ccSnapTake(u8*s){ for(int i=0;i<LK_N;i++) s[i]=look[i]; s[LK_N]=stage; s[LK_N+1]=pAsp; s[LK_N+2]=pLtw; for(int i=0;i<TR_N;i++) s[LK_N+3+i]=pTr[i]; }
static int ccUnsaved(void){ u8 s[LK_N+TR_N+3]; ccSnapTake(s); for(int i=0;i<(int)sizeof s;i++) if(s[i]!=ccSnap[i]) return 1; return 0; }
static int ccN(void){ int n=(int)((svSlotEnd()-CC_OFF)/CC_REC); return n>CC_MAX?CC_MAX:n; }
static u32 ccAt(int i){ return CC_OFF+(u32)i*CC_REC; }
static int ccOk(int i){
    u32 o=ccAt(i); if(svRd(o)!='B'||svRd(o+1)!='C') return 0;
    int nl=svRd(o+2); if(nl<1||nl>LK_N) return 0;   // (a later demo with more looks: the extra ones are left out here)
    u8 s=0x43; for(int k=2;k<CC_HDR+nl;k++) s=(u8)(s*5+svRd(o+k));
    return s==svRd(o+CC_HDR+nl);
}
static void ccName(int i,char*d){ u32 o=ccAt(i)+6+TR_N; int k=0; for(;k<HH_NM-1;k++){ char c=(char)svRd(o+k); if(!c) break; d[k]=c; } d[k]=0; }
static int ccCount(void){ int c=0; for(int i=0;i<ccN();i++) c+=ccOk(i); return c; }
static int ccLoad(int i){   // creation i comes into the creator
    if(!ccOk(i)) return 0;
    u32 o=ccAt(i); int nl=svRd(o+2); u8 tr[TR_N]; for(int k=0;k<TR_N;k++) tr[k]=svRd(o+6+k);
    if(svRd(o+3)>=AG_N||!persValid(svRd(o+4),svRd(o+5),tr)) return 0;
    stage=svRd(o+3); pAsp=svRd(o+4); pLtw=svRd(o+5); for(int k=0;k<TR_N;k++) pTr[k]=tr[k];
    for(int k=0;k<HH_NM;k++){ hhPName[k]=(char)svRd(o+6+TR_N+k); hhPLast[k]=(char)svRd(o+6+TR_N+HH_NM+k); } hhPName[HH_NM-1]=hhPLast[HH_NM-1]=0;
    for(int k=0;k<LK_N;k++) look[k]=k<nl?svRd(o+CC_HDR+k):0;
    if(look[LK_SEX]>=SX_N) look[LK_SEX]=SX_NB;
    fixLook(); custom=0; buildLook(); setColors();
    return 1;
}
static int ccWrite(int i){   // the creature in the creator into record i. 1 = done
    u32 o=ccAt(i); svErr=0; svErase(o,CC_REC);
    u8 h[CC_HDR]; h[0]='B'; h[1]='C'; h[2]=LK_N; h[3]=stage; h[4]=pAsp; h[5]=pLtw; for(int k=0;k<TR_N;k++) h[6+k]=pTr[k];
    for(int k=0;k<HH_NM;k++){ h[6+TR_N+k]=(u8)hhPName[k]; h[6+TR_N+HH_NM+k]=(u8)hhPLast[k]; }
    u8 s=0x43; for(int k=2;k<CC_HDR;k++){ s=(u8)(s*5+h[k]); svWr(o+k,h[k]); }
    for(int k=0;k<LK_N;k++){ s=(u8)(s*5+look[k]); svWr(o+CC_HDR+k,look[k]); }
    svWr(o+CC_HDR+LK_N,s); svWr(o+1,'C'); svWr(o,'B');   // the magic last: a cut write leaves no half creation
    return !svErr&&ccOk(i);
}
static int ccNameEmpty(void){ return !hhPName[0]||(hhPName[0]=='Y'&&hhPName[1]=='O'&&hhPName[2]=='U'&&!hhPName[3]); }
static int ccSave(void){   // DONE tab: SAVE CREATION
    if(ccNameEmpty()&&!nameEdit(hhPName,HH_NM-1,"NAME YOUR BORE",0)) return 0;
    int i=ccEdit; if(i<0||i>=ccN()){ i=-1; for(int k=0;k<ccN()&&i<0;k++) if(!ccOk(k)) i=k; }
    if(i<0){ toast("MY CREATIONS IS FULL  DELETE ONE"); return 0; }
    if(!ccWrite(i)){ toast("COULD NOT SAVE"); return 0; }
    ccEdit=i; ccSnapTake(ccSnap); sfxPlay(SFX_CASH); toast("SAVED TO MY CREATIONS"); return 1;
}
static int ccLeave(void){   // DONE tab: MAIN MENU. 1 = leave the creator
    if(!ccUnsaved()) return 1;
    static const char* const it[3]={"SAVE AND LEAVE","LEAVE  DO NOT SAVE","KEEP CREATING"};
    int c=menu("SAVE YOUR BORE FIRST?",it,3);
    if(c==0) return ccSave();
    return c==1;
}
static void ccFresh(void){   // CREATE A NEW BORE: the plain starter Bore, adult, nameless
    for(int i=0;i<LK_N;i++) look[i]=0;
    stage=AG_ADULT; pAsp=AS_FORTUNE; pLtw=0; setSign(0);
    look[LK_SEX]=sexRoll(); hhPName[0]=0; hhPLast[0]=0; fixLook(); custom=0; buildLook(); setColors();
}
static void ccCreate(void){ ccSnapTake(ccSnap); creatureEditor(); menuMusStart(); }
// ---- the hub and the gallery: the creature on the left (the creator's own stage), a panel on the right ----
static void ccPanel(const char*title,const char*sub){
    fillCols(SCENE_W,ROW_W,PANEL);
    text(130,6,"BORE",RGB(31,26,6),2); text(130,20,"CREATURE CREATOR",RGB(14,30,12),1);
    rect(128,29,108,1,GOLD2);
    if(title) text(130,34,title,GOLD,1);
    if(sub) text(130,43,sub,DIMC,1);
}
static void ccScene(const char*cap){
    stageOn=1; drawScene(0); stageOn=0;
    if(cap){ text(7,6,cap,RGB(3,3,6),1); text(6,5,cap,GOLD,1); }
}
static void ccGallery(void){
    int n=ccN(), cur=-1; for(int i=0;i<n&&cur<0;i++) if(ccOk(i)) cur=i;
    if(cur<0){ toast("NOTHING SAVED YET  CREATE ONE"); return; }
    u16 prev=keyNow(); int dirty=3;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        int d=(pr&(K_RIGHT|K_R))?1:(pr&(K_LEFT|K_L))?-1:0;
        if(d){ int j=cur; for(int t=0;t<n;t++){ j=(j+d+n)%n; if(ccOk(j)) break; } if(j!=cur){ cur=j; dirty=3; sfxPlay(SFX_POP); } }
        if(dirty&1){ if(!ccLoad(cur)){ toast("THIS ONE IS BROKEN"); } }
        if(pr&K_B) return;
        if(pr&K_A){ ccEdit=cur; ccCreate(); return; }
        if(pr&K_START){ scShow(); dirty=3; prev=keyNow(); }
        if(pr&K_SEL){ static const char* const yn[2]={"YES  DELETE IT","NO"};
            if(menu("DELETE THIS BORE?",yn,2)==0){ svErase(ccAt(cur),CC_REC); toast("DELETED");
                int j=-1; for(int t=1;t<=n&&j<0;t++){ int q=(cur+t)%n; if(ccOk(q)) j=q; } if(j<0) return; cur=j; }
            dirty=3; prev=keyNow(); }
        if(dirty){
            if(dirty&1) ccScene("MY CREATIONS");
            char nm[2*HH_NM+2]; ccName(cur,nm); char*e=nm; while(*e) e++;
            { char ln[HH_NM]; u32 o=ccAt(cur)+6+TR_N+HH_NM; int q=0; for(;q<HH_NM-1;q++){ char c=(char)svRd(o+q); if(!c) break; ln[q]=c; } ln[q]=0; if(q){ *e++=' '; for(int z=0;z<=q;z++) e[z]=ln[z]; } }
            int pos=0, tot=0; for(int i=0;i<n;i++) if(ccOk(i)){ tot++; if(i<=cur) pos++; }
            char ps[16]; char*p=ps; p+=numStr(p,pos); *p++=' '; *p++='O'; *p++='F'; *p++=' '; p+=numStr(p,tot); *p=0;
            ccPanel(nm,ps);
            text(130,56,stageNm[stage<AG_N?stage:AG_ADULT],WHITE,1);
            text(130,66,aspNm[pAsp<AS_N?pAsp:0],RGB(14,30,12),1);
            static const char* const keys[5]={"LEFT RIGHT  FLIP","A  EDIT","START  SHARE CODE","SELECT  DELETE","B  BACK"};
            for(int i=0;i<5;i++) text(130,90+i*10,keys[i],i==0?GOLD:DIMC,1);
            present(); dirty=0;
        } else vsync();
    }
}
static void ccAbout(void){
    static const char* const ln[]={">THE BORE CREATOR DEMO","EVERY PART, SLIDER AND COLOUR IS FREE.","MAKE A BORE, SAVE IT TO MY CREATIONS,","AND GIVE ITS SHARE CODE TO A FRIEND.","",
        ">IN THE FULL GAME","YOUR BORE MOVES INTO A TOWN, FINDS A JOB,","FALLS IN LOVE, SKATES, GETS ARRESTED ...","TYPE ITS SHARE CODE IN THE FULL GAME'S","CREATOR (DONE TAB: ENTER A CODE).","",
        ">KEYS","L R TABS   UP DOWN ROWS   LEFT RIGHT CHANGE","SELECT TURNS YOUR BORE   START DONE TAB"};
    helpScreen("ABOUT",ln,(int)(sizeof ln/sizeof ln[0]));
}
static void ccMain(void){   // the demo's whole life after the title screen
    static const char* const it[5]={"CREATE A NEW BORE","MY CREATIONS","ENTER A SHARE CODE","ABOUT THE DEMO","CONTINUE THIS ONE"};
    int sel=0, dirty=3, have=0; u16 prev=keyNow();
    ccFresh(); menuMusStart();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        int nIt=have?5:4;
        if(pr&K_DOWN){ sel=(sel+1)%nIt; dirty|=2; } if(pr&K_UP){ sel=(sel+nIt-1)%nIt; dirty|=2; }
        if(pr&K_SEL){ view=(view+1)&3; dirty=3; }
        if(pr&(K_A|K_START)){
            switch(sel){
                case 0: ccFresh(); ccEdit=-1; ccCreate(); have=1; break;
                case 1: ccGallery(); have=1; break;
                case 2: if(scEnter()){ ccEdit=-1; hhPName[0]=0; hhPLast[0]=0; ccCreate(); have=1; } break;
                case 3: ccAbout(); break;
                case 4: ccCreate(); break;
            }
            prev=keyNow(); dirty=3;
        }
        if(dirty){
            if(dirty&1) ccScene(0);
            ccPanel(0,0);
            { char t[24]; char*e=t; e+=numStr(e,ccCount()); e=simCat(e," SAVED"); text(130,34,t,DIMC,1); }
            for(int i=0;i<nIt;i++){ int y=50+i*16, f=i==sel; rect(128,y-3,108,13,f?RGB(6,16,8):RGB(5,7,14)); if(f) rect(128,y+9,108,1,GOLD); text(134,y,it[i],f?GOLD:WHITE,1); }
            text(130,140,"A CHOOSE   SELECT TURN",DIMC,1);
            present(); dirty=0;
        } else vsync();
    }
}
