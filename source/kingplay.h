// kingplay.h - EP6 THE LAND BOREFORE TIME, Phase 4 step a: a realm can be PLAYED. The wiring of kingmem.h into the game, and a plain status screen to watch it on.
// WHAT IS HERE: kpOpen (stgClaim, lay the realm out in hhPool, load it, start the simulation), kpSave (save it, the realm goes on running), kpSettle (the first
// settlement of a brand new realm) and kpRun (KINGDOM MODE > PLAY: runs the economy at three speeds, shows day / population / goods, saves or leaves from a menu).
// STEP b: the MAP. PLAY opens on the realm drawn with the hardware tile background (kingview.h); the D-pad scrolls it, A switches to the status screen and back.
// STEP g: the GIANT (kinggiant.h): the D-pad walks it (B held runs), the camera follows, A acts on the cell under its feet (HAND gathers, the other modes build), SELECT shows the status screen.
// STEP h: PICK UP AND PUT DOWN (kinghold.h): HAND carries goods, MOVE lifts buildings, NUBBIN lifts a resident or a worker. SELECT cycles map, OVERVIEW (kingover.h, step i), status. The giant's cell is saved with the realm (step j: KRealm gx, gy, kgPlace).
// Needs before it: kingdom.h, kingslot.h, kingsim.h, kinglog.h, kingmem.h, house.h (hhPool, stgClaim, stgOk) and, unless KP_NOSCREEN, kingscreen.h's helpers.
#define KP_OK      0
#define KP_E_READ (-1)    // the realm could not be read from its slot
#define KP_E_FIT  (-2)    // it does not fit the pool (never happens for the three sizes: tools/test_kingmem.c)
#define KP_E_LOAD (-3)    // the save did not load (damaged, or newer than this game)
#define KP_E_SIM  (-4)    // the simulation refused it (more than KS_MAXB buildings, too many road cells)
#define KP_E_HELD (-40)   // (kpSave, step 4h) the giant has a building lifted: put it down before saving
static const char* kpWhy(int e){ return e==KP_E_READ?"THIS REALM CANNOT BE READ":e==KP_E_FIT?"NOT ENOUGH MEMORY":e==KP_E_LOAD?"THIS REALM DID NOT LOAD":"THIS REALM IS TOO BIG TO RUN"; }
// OPEN: the realm in `slot` is loaded and running-ready in the pool. Leaves the household's sprites let go (they bake again by themselves, house.h) whatever the result.
static int kpOpen(KmMem*m,int slot){
    int t=0,g=0,c=0; u32 d=0;
    stgClaim(); stgOk=0;                                   // the pool is ours: every baked household sprite is let go, their keys cleared
    if(!kgsPeek(slot,&t,&g,&c,&d)||t<0||t>=KG_TIERS) return KP_E_READ;
    if(kmLayout(hhPool,SP_POOL,t,m)) return KP_E_FIT;
    if(kgsLoad(slot,m->R,m->B,m->cells,m->lscr,m->lN)!=SLE_OK) return KP_E_LOAD;
    if(ksInit(m->S,m->R,m->B,m->cells)!=KSE_OK) return KP_E_SIM;
    return KP_OK;
}
// SAVE: a fresh copy in the flash (kingslot.h; the old copy goes only when the new one reads back). ksPrepSave first: the carriers' goods go home and the maps are let go,
// which is what frees the cache for the scratch. The realm keeps running afterwards. Returns the slot it now lives in (it MOVES), or a negative SLE_ error with the realm untouched.
static int kpSave(KmMem*m,int slot){
    if(ksHoldHome(m->S)) return KP_E_HELD;   // (step 4h: a load of goods goes back on the pile, a Nubbin home; a lifted building stops the save)
    int id=slI[slot].pid; ksPrepSave(m->S);
    return kgsSave(id,0,m->R,m->B,m->cells,m->sscr,m->sN,&id);
}
// THE FIRST SETTLEMENT [ASSUMED, tunable]: a new realm (no buildings, day 0) gets a storehouse already standing, two hut sites and a few road cells in the middle of the land,
// on a cleared 8 x 8 patch (so it works on any land). The founding goods (ksFound: 10 of each raw good) pay for the huts. Returns 1 if it settled, 0 if the realm was not new.
static int kpSettle(KmMem*m){
    KSim*S=m->S; KRealm*R=m->R; if(R->nB||R->day) return 0;
    int d=S->dim, c=d/2;
    for(int y=c-3;y<c+5;y++) for(int x=c-3;x<c+5;x++) m->cells[y*d+x]=KT_GRASS;
    ksPlace(S,KB_STORE,c-1,c-1,1); ksPlace(S,KB_HUT,c+2,c-1,0); ksPlace(S,KB_HUT,c-1,c+2,0);
    ksRoadSet(S,c+1,c-1,1); ksRoadSet(S,c+1,c,1); ksRoadSet(S,c+1,c+1,1); ksRoadSet(S,c,c+1,1); ksRoadSet(S,c-1,c+1,1); ksRoadSet(S,c+2,c+1,1);
    return 1;
}
#ifndef KP_NOSCREEN
#ifndef KV_PUT
#define KV_PUT(sx,sy,e) (*(volatile u16*)(0x06004000u+(u32)((sy)*32+(sx))*2u)=(u16)(e))   // BG0's screenblock 8
#endif
#include "kingview.h"
#include "kinghand.h"   // Phase 4d: building by hand (A acts on the cell under the giant's feet)
#include "kinggiant.h"  // Phase 4g: the walking giant (pure C: movement, camera, sprite art, OAM words)
#include "kingover.h"   // Phase 4i: the overview (the whole realm, a pixel a cell)
#define KH_BLANK (KV_TILES+KH_TILES)   // the see-through tile, after the text tiles
#define KH_SHADOW RGB(1,1,3)
static int kpGiant=0;   // the realm's giant (0..4), for the sprite's colours
// the giant's sprite: OAM entry 0
static void kgShow(const KGiant*G,const KView*V){ unsigned a0,a1,a2; kgOam(G,V,&a0,&a1,&a2); volatile u16*o=(volatile u16*)0x07000000; o[0]=(u16)a0; o[1]=(u16)a1; o[2]=(u16)a2; }
// ---- the hardware: mode 0, BG0 = the realm (tiles in charblock 0, the 32 x 32 map in screenblock 8). Both live in the bitmap's VRAM, so leaving mode 0 repaints the bitmap from fb. ----
static void kvScroll(const KView*V){ *(volatile u16*)0x04000010=(u16)(V->x&255); *(volatile u16*)0x04000012=(u16)(V->y&255); }
static void kvHwEnter(KView*V){
    vsync(); objHideAll(); REG_DISPCNT=0x1340;                                     // mode 0, BG0 and BG1 (the text), sprites on (1D tiles), no windows
    volatile u16*pal=(volatile u16*)0x05000000; for(int b=0;b<=KS_NTYPES;b++) for(int i=0;i<16;i++) pal[b*16+i]=kvPalAt(b,i);
    for(int i=0;i<16;i++) pal[15*16+i]=khPalCur(kvPalAt(0,i));                       // bank 15: the cursor cell, blinking
    volatile u16*tg=(volatile u16*)0x06000000; for(int t=0;t<KV_TILES;t++){ u8 g[32]; kvTileGfx(t,g); for(int i=0;i<16;i++) tg[t*16+i]=(u16)(g[2*i]|(g[2*i+1]<<8)); }
    *(volatile u16*)0x0400000A=0x0900;                                             // BG1CNT: priority 0 (above the map), charblock 0, screenblock 9
    pal[14*16+1]=0x7FFF; pal[14*16+2]=RGB(1,1,3);                                  // bank 14: the text, white with a dark shadow
    { volatile u16*z=(volatile u16*)(0x06000000+KH_BLANK*32); for(int i=0;i<16;i++) z[i]=0;   // a see-through tile fills the whole text map but the two text rows
      volatile u16*mp=(volatile u16*)0x06004800; for(int i=0;i<1024;i++) mp[i]=(u16)(KH_BLANK|(14<<12));
      for(int i=0;i<KH_TILES;i++) mp[18*32+(i/30)*32+(i%30)]=(u16)((KV_TILES+i)|(14<<12)); }
    { u8 g[256]; kgGfxAll(g); volatile u16*ot=(volatile u16*)0x06010000; for(int i=0;i<128;i++) ot[i]=(u16)(g[2*i]|(g[2*i+1]<<8)); }   // the giant: OBJ tiles 0..7 (mode 0: 0x06010000; the bitmap repaints on leaving)
    { u16 gp[16]; kgPal(kpGiant,gp); volatile u16*op=(volatile u16*)0x05000200; for(int i=0;i<16;i++) op[i]=gp[i]; }                      // OBJ palette bank 0
    *(volatile u16*)0x04000014=0; *(volatile u16*)0x04000016=0;                    // BG1 never scrolls
    *(volatile u16*)0x04000008=0x0801;                                             // BG0CNT: charblock 0, screenblock 8, 16 colours, 32 x 32
    V->ok=0; kvSync(V); kvScroll(V);
}
static void kvHwLeave(void){ vsync(); objHideAll(); REG_DISPCNT=0x0403; dmaRows(fb,VRAM_ADDR,0,ROW_W,0,SH); }   // mode 3 again, the bitmap painted back from fb
static void khShow(const KView*V,int cx,int cy,int on){   // the cursor cell: its tile in bank 15 while it blinks on, its own tile otherwise
    if(!V->ok||cx<V->tx0||cy<V->ty0||cx>=V->tx0+KV_W||cy>=V->ty0+KV_H) return;
    KV_PUT(cx&31,cy&31,on?(u16)(kvEntry(V,cx,cy)|0xF000):kvEntry(V,cx,cy)); }
static void kpHud(const KSim*S,const KRealm*R,const KHand*h,int cx,int cy,int spd,const char*msg){   // draws the two lines in the bitmap (not shown in mode 0), makes tiles of them
    char l1[64], l2[64], hb[24]; khHeld(S,hb); khStatus2(l1,h->mode,(long)R->day,S->pop,spd,hb); if(msg) khCat(l2,msg); else khInfo(S,cx,cy,l2);
    rect(0,0,240,16,0); text(1,1,l1,KH_SHADOW,1); text(0,0,l1,GOLD,1); text(1,9,l2,KH_SHADOW,1); text(0,8,l2,WHITE,1);
    static u16 t[KH_TILES*16]; khStrip(fb,KH_SHADOW,t); volatile u16*vt=(volatile u16*)(0x06000000+KV_TILES*32); for(int i=0;i<KH_TILES*16;i++) vt[i]=t[i]; }
static int kpDraw(const KmMem*m,const char*nm,int spd,u32 cnt){   // the status screen
    const KSim*S=m->S; const KRealm*R=m->R; char b[40]; char*e; int built=0; for(int i=0;i<R->nB;i++) built+=m->B[i].lvl;
    stBack("KINGDOM MODE",(int)cnt);
    text(14,22,nm,GOLD,1); text(14,31,ksTier[R->tier],RGB(20,26,31),1);
    e=slCat(b,"DAY "); e=slNum(e,(int)(R->day>99999?99999:R->day)); e=slCat(e,"   NUBBINS "); slNum(e,S->pop); text(14,46,b,RGB(17,29,31),1);
    e=slNum(b,built); e=slCat(e," BUILT OF "); e=slNum(e,R->nB); slCat(e," BUILDINGS"); text(14,55,b,RGB(17,29,31),1);
    e=slCat(b,"WOOD "); e=slNum(e,R->goods[G_WOOD]); e=slCat(e,"   STONE "); slNum(e,R->goods[G_STONE]); text(14,70,b,WHITE,1);
    e=slCat(b,"PLANKS "); e=slNum(e,R->goods[G_PLANKS]); e=slCat(e,"   CUT STONE "); slNum(e,R->goods[G_CUT]); text(14,79,b,WHITE,1);
    e=slCat(b,"WOOL "); e=slNum(e,R->goods[G_WOOL]); e=slCat(e,"   CRYSTAL "); slNum(e,R->goods[G_CRYSTAL]); text(14,88,b,DIMC,1);
    e=slCat(b,"CLOTH "); e=slNum(e,R->goods[G_CLOTH]); e=slCat(e,"   POLISHED "); slNum(e,R->goods[G_POLISH]); text(14,97,b,DIMC,1);
    e=slCat(b,"DELIVERED "); slNum(e,(int)S->delivered); text(14,109,b,DIMC,1);
    e=slCat(b,"SPEED  X"); slNum(e,spd==0?1:spd==1?4:16); text(14,118,b,GOLD,1);
    e=slCat(b,"CASTLE  "); slCat(e,ksCastle[R->castle<=KG_CASTLE?R->castle:0]); text(14,127,b,RGB(17,29,31),1);
    { const char*a="SEL MAP", *b="START MENU"; int x=5; s2pill(x,147,tw(a,1)+10,a); x+=tw(a,1)+14; s2pill(x,147,tw(b,1)+10,b); }
    present(); return 0;
}
static void kpOver(const KmMem*m,const KGiant*G,u32 cnt){   // the overview screen (mode 3): the whole realm, the giant a blinking dot
    rect(0,0,240,160,RGB(0,0,2)); koDraw(fb,m->S,kgCx(G),kgCy(G),(int)((cnt>>3)&1)); text(2,1,"OVERVIEW",GOLD,1);
    { const char*a="SEL STATUS"; s2pill(5,147,tw(a,1)+10,a); } present(); }
static const char* const kpStageMsg[KG_CASTLE]={ "FOUNDATION LAID", "WALLS STAND", "TOWERS RISE", "CASTLE DONE  A MONARCH IS CROWNED" };   // (step 5b) said in the text layer when a castle stage is finished
static int kpRun(int slot){   // returns 0: the KINGDOM list should rescan (the realm's slot moved)
    KmMem m; int e=kpOpen(&m,slot); if(e){ toast(kpWhy(e)); return 0; }
    char nm[SLOT_NAME+1]; { int i=0; for(;i<SLOT_NAME&&slI[slot].name[i];i++) nm[i]=slI[slot].name[i]; nm[i]=0; if(!i){ nm[0]='R'; nm[1]='E'; nm[2]='A'; nm[3]='L'; nm[4]='M'; nm[5]=0; } }
    kpSettle(&m);
    static const int per[3]={1,4,16}; int spd=0, mapOn=1, ov=0, ocx=-1, ocy=-1, hudT=0, hudD=1; const char*hmsg=0; u32 cnt=0; u16 prev=keyNow(); KHand hand; khInit(&hand); int hcx=-1, hcy=-1;
    KGiant G; kgPlace(&G,m.S,m.R->gx,m.R->gy); kpGiant=m.R->giant;
    KView V; kvInit(&V,m.cells,m.S->dim,m.B,m.R->nB); kgCamera(&V,&G); kvHwEnter(&V);
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_SEL||(!mapOn&&pr&K_B)){ if(mapOn){ kvHwLeave(); mapOn=0; ov=1; kpOver(&m,&G,cnt); } else if(ov&&pr&K_SEL){ ov=0; kpDraw(&m,nm,spd,cnt); } else { mapOn=1; ov=0; kvHwEnter(&V); hudD=1; ocx=-1; } }   // SELECT: map, overview, status, map (B goes back to the map from either screen)
        if(pr&K_START){
            static char sp[24]; static const char* const mn[6]={"BUILD MENU",sp,"SAVE AND LEAVE","SAVE AND KEEP PLAYING","LEAVE WITHOUT SAVING","KEEP PLAYING"};
            { char*e=khCat(sp,"SPEED  X"); khNum(e,spd==0?1:spd==1?4:16); }
            if(mapOn) kvHwLeave();
            int ch=menu(nm,mn,6);
            if(ch==0){ char bl[KHM_N][48]; const char*bi[KHM_N]; for(int i=0;i<KHM_N;i++){ khCost(bl[i],m.S,i); bi[i]=bl[i]; } int c=menu("BUILD",bi,KHM_N); if(c>=0) hand.mode=c; }   // (A sets the mode, B backs out)
            else if(ch==1) spd=(spd+1)%3;                                          // (the speed moved here from SELECT: B is now RUN)
            else if(ch==2||ch==3){ m.R->gx=(u8)kgCx(&G); m.R->gy=(u8)kgCy(&G); int s=kpSave(&m,slot); if(s>=0){ slot=s; toast("KINGDOM SAVED"); if(ch==2) return 0; } else toast(s==KP_E_HELD?"PUT THE BUILDING DOWN FIRST":s==SLE_NOROOM?"NO ROOM  FREE A SLOT":slErrMsg(s)); }   // (a failed save never leaves: the realm in play is the only copy of its progress)
            else if(ch==4){ static const char* const yn[2]={"YES  LEAVE","NO  KEEP PLAYING"}; if(menu("EVERYTHING SINCE THE LAST SAVE IS LOST",yn,2)==0) return 0; }
            prev=keyNow(); if(mapOn){ kvHwEnter(&V); hudD=1; ocx=-1; } else if(ov) kpOver(&m,&G,cnt); else kpDraw(&m,nm,spd,cnt); continue; }
        if(mapOn){                                                                 // the map: the giant walks, the camera follows, what came into view is written, one row is refreshed
            vsync();
            { int dx=(k&K_RIGHT?1:0)-(k&K_LEFT?1:0), dy=(k&K_DOWN?1:0)-(k&K_UP?1:0); if(dx||dy) kgMove(&G,m.S,dx,dy,k&K_B?khRun(m.R->giant):khStride(m.R->giant)); else G.moving=0; }
            kgCamera(&V,&G); V.nB=m.R->nB;
            kvSync(&V); kvRefreshRow(&V); kvScroll(&V); kgShow(&G,&V);
            int cx=kgCx(&G), cy=kgCy(&G); if(cx!=ocx||cy!=ocy){ khShow(&V,ocx,ocy,0); ocx=cx; ocy=cy; }
            if(pr&(K_L|K_R)){ khCycle(&hand,pr&K_R?1:-1); hudD=1; }
            else if(pr&K_A){ int r=khAct(&hand,m.S,cx,cy); if(r==KHR_GOT) kvPoke(&V,cx,cy); else if(khRedraw(r)){ V.nB=m.R->nB; kvFull(&V); } hmsg=khMsg[r]; hudT=75; hudD=1; }   // (said in the text layer for 1.25 seconds)
            if(hudT&&!--hudT){ hmsg=0; hudD=1; }
            if(cx!=hcx||cy!=hcy||!(cnt&63)){ hcx=cx; hcy=cy; hudD=1; }
            if(hudD){ hudD=0; kpHud(m.S,m.R,&hand,cx,cy,spd,hmsg); }
            khShow(&V,cx,cy,(cnt>>3)&1);
        } else if(cnt&7) vsync(); else if(ov) kpOver(&m,&G,cnt); else kpDraw(&m,nm,spd,cnt);
        for(int i=0;i<per[spd];i++) ksStep(m.S);
        if(!(cnt%60)){ if(exKingTick(m.S)){ hmsg=exB; hudT=150; hudD=1; } }   // (Phase 5c: the open mission's tasks, checked against this realm)
        if(m.S->ev){ int ev=m.S->ev; m.S->ev=0; if(ev>=1&&ev<=KG_CASTLE){ hmsg=kpStageMsg[ev-1]; hudT=240; hudD=1; } }
    }
}
#endif
