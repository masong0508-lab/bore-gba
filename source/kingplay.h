// kingplay.h - EP6 THE LAND BOREFORE TIME, Phase 4 step a: a realm can be PLAYED. The wiring of kingmem.h into the game, and a plain status screen to watch it on.
// WHAT IS HERE: kpOpen (stgClaim, lay the realm out in hhPool, load it, start the simulation), kpSave (save it, the realm goes on running), kpSettle (the first
// settlement of a brand new realm) and kpRun (KINGDOM MODE > PLAY: runs the economy at three speeds, shows day / population / goods, saves or leaves from a menu).
// STEP b: the MAP. PLAY opens on the realm drawn with the hardware tile background (kingview.h); the D-pad scrolls it, A switches to the status screen and back.
// WHAT IS NOT: no giant, no building by hand yet. Those are the next steps of Phase 4 (the giant, the build menu).
// Needs before it: kingdom.h, kingslot.h, kingsim.h, kinglog.h, kingmem.h, house.h (hhPool, stgClaim, stgOk) and, unless KP_NOSCREEN, kingscreen.h's helpers.
#define KP_OK      0
#define KP_E_READ (-1)    // the realm could not be read from its slot
#define KP_E_FIT  (-2)    // it does not fit the pool (never happens for the three sizes: tools/test_kingmem.c)
#define KP_E_LOAD (-3)    // the save did not load (damaged, or newer than this game)
#define KP_E_SIM  (-4)    // the simulation refused it (more than KS_MAXB buildings, too many road cells)
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
// ---- the hardware: mode 0, BG0 = the realm (tiles in charblock 0, the 32 x 32 map in screenblock 8). Both live in the bitmap's VRAM, so leaving mode 0 repaints the bitmap from fb. ----
static void kvScroll(const KView*V){ *(volatile u16*)0x04000010=(u16)(V->x&255); *(volatile u16*)0x04000012=(u16)(V->y&255); }
static void kvHwEnter(KView*V){
    vsync(); REG_DISPCNT=0x0100;                                                   // mode 0, BG0 only (no sprites, no windows)
    volatile u16*pal=(volatile u16*)0x05000000; for(int b=0;b<8;b++) for(int i=0;i<16;i++) pal[b*16+i]=kvPalAt(b,i);
    volatile u16*tg=(volatile u16*)0x06000000; for(int t=0;t<KV_TILES;t++){ u8 g[32]; kvTileGfx(t,g); for(int i=0;i<16;i++) tg[t*16+i]=(u16)(g[2*i]|(g[2*i+1]<<8)); }
    *(volatile u16*)0x04000008=0x0800;                                             // BG0CNT: charblock 0, screenblock 8, 16 colours, 32 x 32
    V->ok=0; kvSync(V); kvScroll(V);
}
static void kvHwLeave(void){ vsync(); REG_DISPCNT=0x0403; dmaRows(fb,VRAM_ADDR,0,ROW_W,0,SH); }   // mode 3 again, the bitmap painted back from fb
static int kpDraw(const KmMem*m,const char*nm,int spd,u32 cnt){   // the status screen
    const KSim*S=m->S; const KRealm*R=m->R; char b[40]; char*e; int built=0; for(int i=0;i<R->nB;i++) built+=m->B[i].lvl;
    stBack("KINGDOM MODE",(int)cnt);
    text(14,22,nm,GOLD,1); text(14,31,ksTier[R->tier],RGB(20,26,31),1);
    e=slCat(b,"DAY "); e=slNum(e,(int)(R->day>99999?99999:R->day)); e=slCat(e,"   NUBBINS "); slNum(e,S->pop); text(14,46,b,RGB(17,29,31),1);
    e=slNum(b,built); e=slCat(e," BUILT OF "); e=slNum(e,R->nB); slCat(e," BUILDINGS"); text(14,55,b,RGB(17,29,31),1);
    e=slCat(b,"WOOD "); e=slNum(e,R->goods[G_WOOD]); e=slCat(e,"   STONE "); slNum(e,R->goods[G_STONE]); text(14,70,b,WHITE,1);
    e=slCat(b,"PLANKS "); e=slNum(e,R->goods[G_PLANKS]); e=slCat(e,"   CUT STONE "); slNum(e,R->goods[G_CUT]); text(14,79,b,WHITE,1);
    e=slCat(b,"WOOL "); e=slNum(e,R->goods[G_WOOL]); e=slCat(e,"   CRYSTAL "); slNum(e,R->goods[G_CRYSTAL]); text(14,88,b,DIMC,1);
    e=slCat(b,"DELIVERED "); slNum(e,(int)S->delivered); text(14,103,b,DIMC,1);
    e=slCat(b,"SPEED  X"); slNum(e,spd==0?1:spd==1?4:16); text(14,112,b,GOLD,1);
    { const char*a="A MAP", *b="L R SPEED", *c="B MENU"; int x=5; s2pill(x,147,tw(a,1)+10,a); x+=tw(a,1)+14; s2pill(x,147,tw(b,1)+10,b); x+=tw(b,1)+14; s2pill(x,147,tw(c,1)+10,c); }
    present(); return 0;
}
static int kpRun(int slot){   // returns 0: the KINGDOM list should rescan (the realm's slot moved)
    KmMem m; int e=kpOpen(&m,slot); if(e){ toast(kpWhy(e)); return 0; }
    char nm[SLOT_NAME+1]; { int i=0; for(;i<SLOT_NAME&&slI[slot].name[i];i++) nm[i]=slI[slot].name[i]; nm[i]=0; if(!i){ nm[0]='R'; nm[1]='E'; nm[2]='A'; nm[3]='L'; nm[4]='M'; nm[5]=0; } }
    kpSettle(&m);
    static const int per[3]={1,4,16}; int spd=0, mapOn=1; u32 cnt=0; u16 prev=keyNow();
    KView V; kvInit(&V,m.cells,m.S->dim,m.B,m.R->nB); kvCenter(&V,m.S->dim/2,m.S->dim/2); kvHwEnter(&V);
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_L&&spd>0) spd--; if(pr&K_R&&spd<2) spd++;
        if(pr&K_A){ if(mapOn){ kvHwLeave(); mapOn=0; kpDraw(&m,nm,spd,cnt); } else { mapOn=1; kvHwEnter(&V); } }
        if(pr&(K_B|K_START)){
            static const char* const mn[4]={"SAVE AND LEAVE","SAVE AND KEEP PLAYING","LEAVE WITHOUT SAVING","KEEP PLAYING"};
            if(mapOn) kvHwLeave();
            int ch=menu(nm,mn,4);
            if(ch==0||ch==1){ int s=kpSave(&m,slot); if(s>=0){ slot=s; toast("KINGDOM SAVED"); if(ch==0) return 0; } else toast(s==SLE_NOROOM?"NO ROOM  FREE A SLOT":slErrMsg(s)); }   // (a failed save never leaves: the realm in play is the only copy of its progress)
            else if(ch==2){ static const char* const yn[2]={"YES  LEAVE","NO  KEEP PLAYING"}; if(menu("EVERYTHING SINCE THE LAST SAVE IS LOST",yn,2)==0) return 0; }
            prev=keyNow(); if(mapOn) kvHwEnter(&V); else kpDraw(&m,nm,spd,cnt); continue; }
        if(mapOn){                                                                 // the map: scroll 3 pixels a frame, write what came into view, refresh one row
            vsync();
            V.x+=(k&K_RIGHT?3:0)-(k&K_LEFT?3:0); V.y+=(k&K_DOWN?3:0)-(k&K_UP?3:0); kvClamp(&V); V.nB=m.R->nB;
            kvSync(&V); kvRefreshRow(&V); kvScroll(&V);
        } else if(cnt&7) vsync(); else kpDraw(&m,nm,spd,cnt);
        for(int i=0;i<per[spd];i++) ksStep(m.S);
    }
}
#endif
