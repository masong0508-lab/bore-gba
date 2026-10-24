// kingscreen.h - EP6 THE LAND BOREFORE TIME: the KINGDOM screen (KINGDOM MODE on the pack's page). Roadmap Phase 2, step c.
// It lists the realms in the save slots (kingslot.h), shows what the one under the cursor is, and deletes one (asked twice). No realm can be
// PLAYED yet and none can be founded yet: that is step d (found a kingdom: giant, size, name) and Phase 3 on. Rename and the quota are step e.
// It lives in the base ROM for now (a few hundred bytes of code); it moves into the pack's slot with the rest of the pack when there is
// something to play. Needs: slots.h, kingdom.h, kingslot.h, story.h (stBack), main.c's text and menu helpers.
#define KS_MINSLOTS 26   // a cart with less save room (SRAM only: 12 slots) cannot hold realms safely: the mode is off there (roadmap 5.4)
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
static void ksDetail(int r){   // the panel under the list: what the cursor is on
    s2rr(8,88,224,54,RGB(10,20,30)); s2rr(9,89,222,52,RGB(2,6,13));
    char b[48]; char*e;
    if(r<0){
        text(14,93,"FOUND A NEW KINGDOM",GOLD,1);
        text(14,102,"PICK A GIANT  A SIZE  A NAME",RGB(20,26,31),1);
        e=slNum(b,ksFree()); slCat(e," SAVE SLOTS FREE"); text(14,113,b,RGB(17,29,31),1);
        text(14,122,"COMING IN THE NEXT STEP",DIMC,1); return;
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
    { const char*a=sel?"A DELETE":"A FOUND"; s2pill(5,147,tw(a,1)+10,a); s2pill(9+tw(a,1)+10,147,tw("B BACK",1)+10,"B BACK"); }
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
            if(sel==0) toast("FOUNDING A KINGDOM COMES NEXT");
            else { static const char* const yn[2]={"DELETE THIS KINGDOM","KEEP IT"}; int s=ksL[sel-1];
                if(menu(slI[s].name[0]?slI[s].name:"THIS KINGDOM",yn,2)==0){ static const char* const yn2[2]={"YES  DELETE FOR GOOD","NO  KEEP IT"};
                    if(menu("THIS CANNOT BE UNDONE",yn2,2)==0){ kgsDelete(slI[s].pid); ksRefresh(); if(sel>ksN) sel=ksN; toast("KINGDOM DELETED"); } } }
            prev=keyNow(); dirty=1; continue; }
        if(sel<top){ top=sel; }
        if(sel>=top+6){ top=sel-5; }
        if(!dirty&&(cnt&15)){ vsync(); continue; }
        dirty=0; ksDraw(sel,top,cnt);
    }
    return 0;
}
