// kingscreen.h - EP6 THE LAND BOREFORE TIME: the KINGDOM screen (KINGDOM MODE on the pack's page). Roadmap Phase 2, step c.
// It lists the realms in the save slots (kingslot.h), shows what the one under the cursor is, founds one (step d), renames one and deletes one
// (asked twice). No realm can be PLAYED yet: that is Phase 3 on. Step e: rename, the quota and the reserve of room for players and towns.
// It lives in the base ROM for now (a few hundred bytes of code); it moves into the pack's slot with the rest of the pack when there is
// something to play. Needs: slots.h, kingdom.h, kingslot.h, story.h (stBack), main.c's text and menu helpers.
#define KS_MINSLOTS 26   // a cart with less save room (SRAM only: 12 slots) cannot hold realms safely: the mode is off there (roadmap 5.4)
static int kpRun(int slot);   // kingplay.h (PLAY THIS KINGDOM)
static const char* const ksTier[KG_TIERS]={"HAMLET 48 X 48","REALM 96 X 96","KINGDOM 128 X 128"};
static const char* const ksCastle[KG_CASTLE+1]={"NO CASTLE YET","FOUNDATION","WALLS","TOWERS","CROWNED"};
static const char* const ksGiant[KG_GIANTS]={"GIANT A","GIANT B","GIANT C","GIANT D","YOUR BORE"};   // (placeholder names, roadmap 4.3)
static int ksL[KGS_LISTMAX];   // the realms' slots, in the order shown
static int ksN;
static void ksRefresh(void){   // rescan the slots, drop leftover older copies, list the realms
    slScan();
    for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING) kgsHeal(slI[i].pid);
    slScan(); ksN=kgsList(ksL,KGS_LISTMAX);
}
static int ksFree(void){ int n=0; for(int i=0;i<SLOT_N;i++) if(slOwner[i]<0) n++; return n; }
// QUOTA AND RESERVE (roadmap 5.4). All three numbers are [ASSUMED] and tunable.
//  - KS_QUOTA: how many realms at once (a full 128 KB flash: 4; a 64 KB flash: 2).
//  - KS_RESERVE: save slots that must stay free for players and towns, whatever the realms do.
//  - a realm saves as a FRESH COPY, so one more realm's worth of room (KGS_MAXSPAN slots) must also stay free, or the next save would be refused.
//  - the small cart (under 58 slots, a 64 KB flash) only founds a HAMLET; the SRAM-only cart (12 slots) has no KINGDOM MODE at all (KS_MINSLOTS).
#define KS_RESERVE 8
#define KS_KEEP    (KS_RESERVE+KGS_MAXSPAN)
static int ksQuota(void){ return SLOT_N>=58?4:2; }
static int ksTiers(void){ return SLOT_N>=58?KG_TIERS:1; }       // how many sizes the menu offers (the first ones: hamlet, realm, kingdom)
static const char* ksWhyNot(void){   // 0: a new realm may be founded; else the toast that says why not
    if(ksN>=ksQuota()||ksN>=KGS_LISTMAX) return "KINGDOM LIMIT  DELETE ONE";
    if(ksFree()<KS_KEEP+1) return "NOT ENOUGH FREE SLOTS";
    return 0;
}
// FOUND A NEW KINGDOM: the giant, the size, the name; then the land is made from a fresh seed and saved (a new realm has no edits and no buildings: 40 bytes).
// Phase 3 on gives a new realm its first settlement; until then it is the empty island with a few goods. Returns the realm's slot, or -1 (backed out or refused).
#define KS_START 10   // [ASSUMED] each raw good a new realm starts with (a tunable constant)
static int ksFound(void){
    { const char*why=ksWhyNot(); if(why){ toast(why); return -1; } }
    static const char* const gi[KG_GIANTS]={"GIANT A  THE ALL ROUNDER","GIANT B  THE RUNNER","GIANT C  THE LIFTER","GIANT D  THE BUILDER","YOUR OWN BORE"};
    static const char* const ti[KG_TIERS]={"HAMLET  SMALL AND QUICK","REALM  THE USUAL SIZE","KINGDOM  HUGE"};
    int g=menu("CHOOSE YOUR GIANT",gi,KG_GIANTS); if(g<0) return -1;
    int t=menu("HOW BIG IS THE LAND",ti,ksTiers()); if(t<0) return -1;
    char nm[SLOT_NAME+1]; slNameN(nm,"REALM ",ksN+1);
    if(!slEditName(nm)) return -1;
    KRealm R; R.tier=(u8)t; R.giant=(u8)g; R.castle=0; R.genVer=KG_GENVER; R.flags=0; R.day=0; R.nB=0; R.gx=KG_NOPOS; R.gy=KG_NOPOS;
    lrng^=((u32)R_TM2D<<8^uiTicks)*2654435761u;                                  // (how long you sat in the menus stirs the seed)
    R.seed=0; for(int i=0;i<4;i++) R.seed=(R.seed<<8)|(u32)rnd8();
    R.rng=0; for(int i=0;i<4;i++) R.rng=(R.rng<<8)|(u32)rnd8(); if(!R.rng) R.rng=1;
    for(int i=0;i<KG_GOODS;i++) R.goods[i]=(u16)(i<4?KS_START:0);
    u8 buf[64]; int id=0; int e=kgsSave(0,nm,&R,0,0,buf,(int)sizeof buf,&id);
    if(e<0){ toast(e==SLE_NOROOM?"NO FREE SAVE SLOT":slErrMsg(e)); return -1; }
    toast("KINGDOM FOUNDED"); return e;
}
static void ksDetail(int r){   // the panel under the list: what the cursor is on
    s2rr(8,88,224,54,RGB(10,20,30)); s2rr(9,89,222,52,RGB(2,6,13));
    char b[48]; char*e;
    if(r<0){
        text(14,93,"FOUND A NEW KINGDOM",GOLD,1);
        text(14,102,"PICK A GIANT  A SIZE  A NAME",RGB(20,26,31),1);
        e=slNum(b,ksN); e=slCat(e," OF "); e=slNum(e,ksQuota()); e=slCat(e," KINGDOMS   "); e=slNum(e,ksFree()); slCat(e," SLOTS FREE"); text(14,113,b,RGB(17,29,31),1);
        { const char*why=ksWhyNot(); text(14,122,why?why:"PRESS A TO BEGIN",why?RGB(31,14,8):DIMC,1); } return;
    }
    int s=ksL[r], t=0, g=0, c=0; u32 d=0;
    if(!kgsPeek(s,&t,&g,&c,&d)||t>=KG_TIERS||g>=KG_GIANTS||c>KG_CASTLE){ text(14,93,slI[s].name,GOLD,1); text(14,102,"THIS REALM CANNOT BE READ",RGB(31,14,8),1); return; }
    text(14,93,slI[s].name[0]?slI[s].name:"NO NAME",GOLD,1);
    text(14,102,ksTier[t],RGB(20,26,31),1);
    e=slCat(b,ksGiant[g]); e=slCat(e,"   DAY "); slNum(e,(int)(d>99999?99999:d)); text(14,111,b,RGB(17,29,31),1);
    e=slCat(b,"CASTLE  "); slCat(e,ksCastle[c]); text(14,120,b,RGB(17,29,31),1);
    e=slCat(b,"SAVED "); e=slNum(e,slI[s].seq); e=slCat(e,slI[s].seq==1?" TIME  ":" TIMES  "); e=slSize(e,SLOT_HDR+slI[s].len); e=slCat(e,"  "); e=slNum(e,slI[s].span); slCat(e,slI[s].span==1?" SLOT":" SLOTS");
    text(14,129,b,DIMC,1);
}
static void ksDraw(int sel,int top,u32 cnt){
    stBack("KINGDOM MODE",(int)cnt);
    { char b[24]; char*e=slNum(b,ksN); slCat(e,ksN==1?" KINGDOM":" KINGDOMS"); text(231-tw(b,1),7,b,GOLD,1); }
    for(int rr=0;rr<6&&top+rr<ksN+1;rr++){ int i=top+rr, y=21+rr*11, on=i==sel;
        if(on) s2grad(8,y-1,224,11,6,16,26,3,9,17);
        if(i==0){ rect(11,y+1,8,7,GOLD); text(13,y,"+",RGB(2,6,13),1); text(23,y,"FOUND A NEW KINGDOM",on?WHITE:GOLD,1); continue; }
        int s=ksL[i-1], t=0, g=0, c=0; u32 d=0; int ok=kgsPeek(s,&t,&g,&c,&d)&&t<KG_TIERS&&c<=KG_CASTLE;
        rect(11,y+1,8,7,!ok?RGB(31,14,8):c>=KG_CASTLE?RGB(31,26,6):c?RGB(14,22,30):RGB(8,26,10)); rect(12,y+2,6,5,RGB(2,6,13));
        text(23,y,slI[s].name[0]?slI[s].name:"NO NAME",on?WHITE:RGB(20,26,31),1);
        if(ok){ const char*q=ksCastle[c]; text(229-tw(q,1),y,q,on?RGB(20,30,20):RGB(14,24,16),1); }
    }
    rect(8,85,224,1,RGB(14,26,31));
    ksDetail(sel-1);
    { const char*a=sel?"A OPTIONS":"A FOUND"; s2pill(5,147,tw(a,1)+10,a); s2pill(9+tw(a,1)+10,147,tw("B BACK",1)+10,"B BACK"); }
    present();
}
static int kgRun(void){   // KINGDOM MODE. Returns 0: nothing started that should close the menus.
    if(SLOT_N<KS_MINSLOTS){ toast("NEEDS A SAVE CHIP WITH MORE ROOM"); return 0; }
    int sel=0, top=0, dirty=1; u32 cnt=0; u16 prev=keyNow();
    ksRefresh();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)) break;
        if(pr&K_DOWN){ sel=(sel+1)%(ksN+1); dirty=1; }
        if(pr&K_UP){ sel=(sel+ksN)%(ksN+1); dirty=1; }
        if(pr&K_A){
            if(sel==0){ int f=ksFound(); if(f>=0){ ksRefresh(); for(int i=0;i<ksN;i++) if(ksL[i]==f) sel=i+1; } }
            else { static const char* const yn[4]={"PLAY THIS KINGDOM","RENAME THIS KINGDOM","DELETE THIS KINGDOM","LEAVE IT"}; int s=ksL[sel-1];
                int ch=menu(slI[s].name[0]?slI[s].name:"THIS KINGDOM",yn,4);
                if(ch==0){ int pid=slI[s].pid; kpRun(s); ksRefresh(); for(int j=0;j<ksN;j++) if(slI[ksL[j]].pid==pid) sel=j+1; }
                else if(ch==1){ char nm[SLOT_NAME+1]; int i=0; for(;i<SLOT_NAME&&slI[s].name[i];i++) nm[i]=slI[s].name[i]; nm[i]=0; int pid=slI[s].pid;
                    if(slEditName(nm)){ int e=kgsRename(pid,nm); ksRefresh(); for(int j=0;j<ksN;j++) if(slI[ksL[j]].pid==pid) sel=j+1;
                        toast(e>=0?"KINGDOM RENAMED":e==SLE_NOROOM?"NO ROOM  FREE A SLOT":slErrMsg(e)); } }
                else if(ch==2){ static const char* const yn2[2]={"YES  DELETE FOR GOOD","NO  KEEP IT"};
                    if(menu("THIS CANNOT BE UNDONE",yn2,2)==0){ kgsDelete(slI[s].pid); ksRefresh(); if(sel>ksN) sel=ksN; toast("KINGDOM DELETED"); } } }
            prev=keyNow(); dirty=1; continue; }
        if(sel<top){ top=sel; }
        if(sel>=top+6){ top=sel-5; }
        if(!dirty&&(cnt&15)){ vsync(); continue; }
        dirty=0; ksDraw(sel,top,cnt);
    }
    return 0;
}
