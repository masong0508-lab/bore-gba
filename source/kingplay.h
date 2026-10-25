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
#include "kinggiant.h"  // Phase 4g: the walking giant (pure C: movement, camera, OAM words)
#include "kingfx.h"     // Phase 6: the life on the map (carriers, workers, smoke, the cursor)
#include "kingadv.h"    // the advisor: what is stuck, what comes next (the GOALS screen)
#include "kingover.h"   // Phase 4i: the overview (the whole realm, a pixel a cell)
#define KH_BLANK (KV_TILES+KH_TILES)   // the see-through tile, after the text tiles (tile 492 of 512: the map's tiles, the text's, this one)
_Static_assert(KA_NT+KH_TILES+1<=512,"the map's tiles, the text strip and the blank tile must fit one charblock (the map's screenblock 8 starts after it)");
static int kpGiant=0;   // the realm's giant (0..4), for the sprite's colours
static u16 kpGiantPal[16];   // the giant's OBJ palette (bank 0): one of the four giants', or the player's own Bore
static u8 kpHudNew=1;   // 1: the text tiles are not in VRAM (the map was just shown): draw them whatever the text says
static int kpOamN=0;   // sprites the plan put in the OAM last frame
// the sprites of the realm: the plan of the life on the map (kingfx.h), the giant in it, is copied into the OAM (the entries after it are switched off)
static void kpFxShow(const u16*pl,int n){ volatile u16*o=(volatile u16*)0x07000000; for(int i=0;i<n;i++){ o[i*4]=pl[i*3]; o[i*4+1]=pl[i*3+1]; o[i*4+2]=pl[i*3+2]; } for(int i=n;i<kpOamN;i++) o[i*4]=0x200; kpOamN=n; }
// the light of the day: every palette scaled by a tint that follows the realm's day (kvDayTint). Bank 15 (the text) and the cursor are left alone.
static void kpTint(const KSim*S){
    int tr,tg,tb; kvDayTint((unsigned)(S->t%KS_DAYT),&tr,&tg,&tb);
    volatile u16*pal=(volatile u16*)0x05000000; for(int b=0;b<15;b++) for(int i=0;i<16;i++) pal[b*16+i]=kvTintC(kaPal[b][i],tr,tg,tb);
    volatile u16*op=(volatile u16*)0x05000200; for(int i=0;i<16;i++) op[i]=kvTintC(kpGiantPal[i],tr,tg,tb);
    for(int b=1;b<7;b++) for(int i=0;i<16;i++) op[b*16+i]=kvTintC(kaObjPal[b][i],tr,tg,tb);
    for(int i=0;i<16;i++) op[8*16+i]=kvTintC(kaObjPal[8][i],tr,tg,tb);                // (bank 8: the crowds)
}
static void kpWater(int frame){ volatile u16*tg=(volatile u16*)(0x06000000+KA_WATER*32); for(int t=0;t<32;t++){ const u8*g=frame?kaWaterB[t]:kaTile[KA_WATER+t]; for(int i=0;i<16;i++) tg[t*16+i]=(u16)(g[2*i]|(g[2*i+1]<<8)); } }   // the second frame of the waves
// ---- YOUR OWN BORE as the giant: the player's baked sprite (spr4: four views, spr4s: the same legs mid-stride; 32 x 60, feet on row SPF) made into the giant's 32 x 64 OBJ frames. The sprite has up to 255 colours, an OBJ
// has 15: the 14 most used are kept (every other colour goes to the nearest of them) and the 15th is a dark OUTLINE round the whole figure, so the Bore stands out on any ground. The sprite is SCALED UP (up to twice,
// in quarters) as far as it fits the 32 x 64 frame, so the giant is as big as it can be. The player is baked in four isometric corners: 0 and 3 face the viewer to the left and to the right, 1 and 2 face away (kpViewOf:
// down, sideways facing right, up). Frames per view: stand, step, step (the second step is the first one mirrored, except sideways, where the face would flip). Writes straight into OBJ VRAM; fills kpGiantPal;
// returns 0 (and writes nothing) if there is no sprite to use. ----
static const u8 kpViewOf[3]={ 0, 3, 1 };
static u32 kpBk;   // the bake key of the Bore that sprBuf holds for this realm (0: not baked here yet)
static inline int kpFl(int a,int b){ return a>=0?a/b:-((-a+b-1)/b); }   // floor division, b > 0
static inline int kpGp(const u8*src,const u8*map,int n,int x,int y){   // the colour (0 clear, 1..14) of pixel (x,y) of a 32 x 64 frame, the sprite scaled by n quarters about its feet
    if(x<0||x>31||y<0||y>63) return 0;
    int sx=16+kpFl((x-16)*4+2,n), sy=SPF+kpFl((y-60)*4+2,n); if(sx<0||sx>=SPW||sy<0||sy>=SPH) return 0;
    return map[src[sy*SPW+sx]];
}
static int kpBoreGiant(void){
    u32 pk=bakeKey();
    if(!(sprKey&&pk==sprKey)&&kpBk!=pk){ buildLook(); setColors(); bakeInto(spr4); strideK=1; bakeInto(spr4s); strideK=0; kpBk=pk; sprKey=0; }   // (buildLook first: the menus drew their own scenes in the voxel grid; sprKey 0: the life game bakes the player again on the way back)
    unsigned short cnt[256]; for(int i=0;i<256;i++) cnt[i]=0;
    int ymin=SPH, xlo=SPW, xhi=0;
    for(int d=0;d<3;d++) for(int f=0;f<2;f++){ const u8*a=sprBuf[(f?4:0)+kpViewOf[d]];
        for(int y=0;y<SPH;y++) for(int x=0;x<SPW;x++){ int c=a[y*SPW+x]; if(!c) continue; if(cnt[c]<65535) cnt[c]++; if(y<ymin) ymin=y; if(x<xlo) xlo=x; if(x>xhi) xhi=x; } }
    if(ymin>=SPH) return 0;
    u8 pick[14]; int np=0; for(;np<14;np++){ int best=-1; for(int i=1;i<256;i++){ int used=0; for(int k=0;k<np;k++) used|=pick[k]==i; if(!used&&cnt[i]&&(best<0||cnt[i]>cnt[best])) best=i; } if(best<0) break; pick[np]=(u8)best; }
    u8 map[256]; for(int i=0;i<256;i++) map[i]=0;
    for(int i=1;i<256;i++) if(cnt[i]){ int bd=0x7FFFFFFF, bk=0; u16 c=sprPal[i];
        for(int k=0;k<np;k++){ u16 e=sprPal[pick[k]]; int dr=(c&31)-(e&31), dg=((c>>5)&31)-((e>>5)&31), db=((c>>10)&31)-((e>>10)&31); int d2=dr*dr+dg*dg+db*db; if(d2<bd){ bd=d2; bk=k; } }
        map[i]=(u8)(bk+1); }
    int h=SPF-ymin+1, hw=16-xlo>xhi-16+1?16-xlo:xhi-16+1, n=8; while(n>4&&(h*n/4>58||hw*n/4>15)) n--;   // the biggest scale (in quarters, 4..8) that fits
    for(int i=0;i<16;i++) kpGiantPal[i]=0; for(int k=0;k<np;k++) kpGiantPal[k+1]=sprPal[pick[k]]; kpGiantPal[15]=RGB(2,3,8);
    volatile u16*ot=(volatile u16*)0x06010000;
    for(int f=0;f<KG_FRAMES;f++){ int d=f/3, w=f%3, mir=(w==2&&d!=1); const u8*src=w?sprBuf[4+kpViewOf[d]]:sprBuf[kpViewOf[d]];
        for(int q=0;q<KG_FTILES*16;q++){ int tile=q>>4, r=(q>>1)&7, half=q&1, x0=(tile&3)*8+half*4, y=(tile>>2)*8+r; unsigned v=0;
            for(int c=0;c<4;c++){ int x=x0+c, X=mir?31-x:x; int px=kpGp(src,map,n,X,y);
                if(!px){ int XL=mir?X+1:X-1, XR=mir?X-1:X+1; if(kpGp(src,map,n,XL,y)||kpGp(src,map,n,XR,y)||kpGp(src,map,n,X,y-1)||kpGp(src,map,n,X,y+1)) px=15; }
                v|=(unsigned)px<<(c*4); }
            ot[f*KG_FTILES*16+q]=(u16)v; } }
    return 1;
}
// ---- the hardware: mode 0, BG0 = the realm (tiles in charblock 0, the 32 x 32 map in screenblock 8), BG1 = the text strip, sprites. All of it lives in the bitmap's VRAM, so leaving mode 0 repaints the bitmap from fb. ----
static void kvScroll(const KView*V){ *(volatile u16*)0x04000010=(u16)(V->x&255); *(volatile u16*)0x04000012=(u16)(V->y&255); }
static void kvHwEnter(KView*V,const KSim*S){
    vsync(); objHideAll(); kpOamN=0; kpHudNew=1; REG_DISPCNT=0x3340;                           // mode 0, BG0 and BG1 (the text), sprites on (1D tiles), window 0 on (it darkens the text strip)
    { volatile u16*tg=(volatile u16*)0x06000000; for(int t=0;t<KA_NT;t++) for(int i=0;i<16;i++) tg[t*16+i]=(u16)(kaTile[t][2*i]|(kaTile[t][2*i+1]<<8)); }
    { u16 gp[16]; if(kpGiant<4){ for(int i=0;i<16;i++) kpGiantPal[i]=kaGiantPal[kpGiant][i]; (void)gp; } }
    if(kpGiant>=4&&!kpBoreGiant()){ kpGiant=0; for(int i=0;i<16;i++) kpGiantPal[i]=kaGiantPal[0][i]; }   // (no sprite to use: the first giant)
    if(kpGiant<4){ volatile u16*ot=(volatile u16*)0x06010000; const u8*g=kaGiants[kpGiant]; for(int i=0;i<KG_FRAMES*KG_FTILES*16;i++) ot[i]=(u16)(g[2*i]|(g[2*i+1]<<8)); }
    { volatile u16*ot=(volatile u16*)(0x06010000+288*32);                          // the Nubbins, the goods, the smoke, the cursor, the crowds: OBJ tiles 288 on
      for(int t=0;t<12;t++) for(int i=0;i<16;i++) ot[t*16+i]=(u16)(kaNub[t][2*i]|(kaNub[t][2*i+1]<<8)); ot+=12*16;
      for(int t=0;t<8;t++) for(int i=0;i<16;i++) ot[t*16+i]=(u16)(kaGood[t][2*i]|(kaGood[t][2*i+1]<<8)); ot+=8*16;
      for(int t=0;t<4;t++) for(int i=0;i<16;i++) ot[t*16+i]=(u16)(kaSmoke[t][2*i]|(kaSmoke[t][2*i+1]<<8)); ot+=4*16;
      for(int t=0;t<4;t++) for(int i=0;i<16;i++) ot[t*16+i]=(u16)(kaCursor[t][2*i]|(kaCursor[t][2*i+1]<<8)); ot+=4*16;
      for(int t=0;t<56;t++) for(int i=0;i<16;i++) ot[t*16+i]=(u16)(kaCrowd[t][2*i]|(kaCrowd[t][2*i+1]<<8)); }   // the crowds: OBJ tiles 316..371
    kpTint(S); { volatile u16*op=(volatile u16*)0x05000200; for(int i=0;i<16;i++) op[7*16+i]=kaObjPal[7][i]; }   // (the cursor is never tinted)
    { volatile u16*pal=(volatile u16*)0x05000000; pal[15*16+1]=0x7FFF; pal[15*16+2]=RGB(1,1,3); pal[15*16+3]=RGB(31,26,6); pal[15*16+4]=RGB(19,19,22); pal[15*16+5]=RGB(21,16,3); }   // bank 15: the text: white, a dark outline, gold, soft white, soft gold
    *(volatile u16*)0x0400000A=0x0900;                                             // BG1CNT: priority 0 (above the map), charblock 0, screenblock 9
    { volatile u16*z=(volatile u16*)(0x06000000+KH_BLANK*32); for(int i=0;i<16;i++) z[i]=0;   // a see-through tile fills the whole text map but the two text rows
      volatile u16*mp=(volatile u16*)0x06004800; for(int i=0;i<1024;i++) mp[i]=(u16)(KH_BLANK|(15<<12));
      for(int i=0;i<KH_TILES;i++) mp[18*32+(i/30)*32+(i%30)]=(u16)((KV_TILES+i)|(15<<12)); }
    *(volatile u16*)0x04000014=0; *(volatile u16*)0x04000016=0;                    // BG1 never scrolls
    *(volatile u16*)0x04000008=0x0801;                                             // BG0CNT: priority 1, charblock 0, screenblock 8, 16 colours, 32 x 32
    *(volatile u16*)0x04000040=0x00F0; *(volatile u16*)0x04000044=(u16)((144<<8)|160);   // window 0: the bottom 16 rows (the text strip)
    *(volatile u16*)0x04000048=0x0033; *(volatile u16*)0x0400004A=0x0013;            // inside it the map may be darkened (BG0, BG1, sprites, effects); outside, no effects
    *(volatile u16*)0x04000050=0x00C1; *(volatile u16*)0x04000054=7;                // darken BG0 by 7/16: the text strip reads on any ground
    V->ok=0; kvSync(V); kvScroll(V);
}
static void kvHwLeave(void){ vsync(); objHideAll(); *(volatile u16*)0x04000050=0; *(volatile u16*)0x04000054=0; *(volatile u16*)0x04000048=0; *(volatile u16*)0x0400004A=0; REG_DISPCNT=0x0403; dmaRows(fb,VRAM_ADDR,0,ROW_W,0,SH); }   // mode 3 again, the bitmap painted back from fb
static int kpSame(const char*a,const char*b){ for(int i=0;i<64;i++){ if(a[i]!=b[i]) return 0; if(!a[i]) return 1; } return 1; }
static void kpHud(const KSim*S,const KRealm*R,const KHand*h,int cx,int cy,int spd,const char*msg){   // draws the two lines in the bitmap (not shown in mode 0), makes tiles of them; nothing is done if the words are the same as the last time
    char l1[64], l2[64], hb[24]; khHeld(S,hb); khStatus2(l1,h->mode,khDay(R->day),S->pop,spd,hb); if(msg) khCat(l2,msg); else khInfo(S,cx,cy,l2);
    static char o1[64], o2[64]; if(!kpHudNew&&kpSame(l1,o1)&&kpSame(l2,o2)) return;
    for(int i=0;i<64;i++){ o1[i]=l1[i]; if(!l1[i]) break; } for(int i=0;i<64;i++){ o2[i]=l2[i]; if(!l2[i]) break; } kpHudNew=0;
    rect(0,0,240,16,0); text(2,1,l1,WHITE,1); text(2,9,l2,WHITE,1);
    static u16 t[KH_TILES*16]; khStrip(fb,0,t); volatile u16*vt=(volatile u16*)(0x06000000+KV_TILES*32); for(int i=0;i<KH_TILES*16;i++) vt[i]=t[i]; }
// the monarch's title and name: made from the realm's seed (the same for ever, nothing to save), from the game's own list of names
static void kpMonarch(const KRealm*R,char*b){ u32 h=kgHash(R->seed,7,7); char*e=khCat(b,(h&0x100u)?"KING ":"QUEEN "); khCat(e,hhNames[(h>>9)%24u]); }
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
    if(R->castle>=KG_CASTLE){ char mb[24]; kpMonarch(R,mb); e=slCat(b,"MONARCH  "); slCat(e,mb); text(14,136,b,GOLD,1); }
    else { int beds=0, jobs=0; for(int i=0;i<R->nB;i++){ const KBld*q=&m->B[i]; if(!q->lvl) continue; const KsType*T=ksT(q->type); if(T->kind==KK_HOME) beds+=T->house; else if(T->kind==KK_GATHER||T->kind==KK_PROC) jobs++; }
        e=slCat(b,"BEDS "); e=slNum(e,beds); e=slCat(e,"   JOBS "); e=slNum(e,S->emp); e=slCat(e," OF "); slNum(e,jobs); text(14,136,b,RGB(17,29,31),1); }
    { const char*a="SEL MAP", *b="START MENU"; int x=5; s2pill(x,147,tw(a,1)+10,a); x+=tw(a,1)+14; s2pill(x,147,tw(b,1)+10,b); }
    present(); return 0;
}
// ---- GOALS: the open mission (its words and its tasks, live) and what the advisor says is stuck; HOW TO PLAY: two pages. Each is a screen of its own: A or B leaves. ----
#define KP_NMIS ((int)(sizeof(xmKing)/sizeof(xmKing[0])))   // the missions of the pack
static void kpTipLine(const KSim*S,const KAdv*a,char*b){ char t[48]; kaText(S,a,t); b[0]='-'; b[1]=' '; int i=0; for(;t[i]&&i<44;i++) b[2+i]=t[i]; b[2+i]=0; }
static void kpGoals(const KmMem*m){
    u32 cnt=0; u16 prev=keyNow();
    for(;;){ u16 k=keyNow(), pr=k&~prev; prev=k; cnt++; if(pr&(K_A|K_B|K_START|K_SEL)) return;
        if(cnt&7){ vsync(); continue; }
        stBack("GOALS",(int)cnt); const KSim*S=m->S; int y=22; const XMs*x=exP==XP_KING+1?exMs():0;
        if(x){ ktFeed(S,exKv); text(14,y,x->nm,GOLD,1); vsDraw(14,y+11,212,3,WHITE,exBrT(x),cnt); exTkList(x,14,y+42,10,1);
            { char b[24]; char*e=slCat(b,"PAYS  "); e=slNum(e,x->pay*100); slCat(e," SIMOLEONS"); text(14,y+74,b,RGB(17,29,31),1); } }
        else { int nx=0; while(nx<KP_NMIS&&kdGet(nx)) nx++;
            if(nx<KP_NMIS){ text(14,y,"NO MISSION OPEN",GOLD,1); { char b[48]; char*e=slCat(b,"NEXT  "); slCat(e,xmKing[nx].nm); text(14,y+11,b,WHITE,1); } vsDraw(14,y+22,212,3,DIMC,"Start it from the expansion shelf in your own life. Everything you build here still counts.",cnt); }
            else { text(14,y,"EVERY MISSION IS DONE",GOLD,1); vsDraw(14,y+11,212,3,WHITE,"The realm never ends. Raise more of everything, fill the land with Nubbins and watch the crowds grow.",cnt); } }
        text(14,104,"ADVISOR",GOLD,1);
        { KAdv ad[KA_MAX]; int n=kaAdvise(S,ad,3); if(!n) text(14,114,"- ALL WELL  KEEP BUILDING",RGB(10,28,12),1); for(int i=0;i<n;i++){ char b[52]; kpTipLine(S,&ad[i],b); text(14,114+i*9,b,WHITE,1); } }
        s2pill(5,147,tw("A BACK",1)+10,"A BACK"); present(); }
}
static void kpHelp(void){
    static const char* const pg[3]={
      "D PAD walks the giant, hold B to run. A does what the MODE in the text strip says. L and R change the mode: HAND, ROAD, a building, DEMOLISH, MOVE or NUBBIN. HAND gathers from the ground under your feet and picks up goods: A again puts them down. SELECT: overview, status, map. START: menu.",
      "A site is a building waiting for goods. The Nubbins carry them along ROADS from the storehouse, so build a STOREHOUSE first and lay road to every door. Roads are the realm: a building with no road beside it is never served.",
      "Homes bring Nubbins, Nubbins take jobs, jobs make goods. A well and a tavern near a workplace make it work faster. Build the castle in four stages and crown a monarch. The realm never ends: GOALS shows what is next and the ADVISOR what is stuck." };
    int p=0; u32 cnt=0; u16 prev=keyNow();
    for(;;){ u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_B) return; if(pr&(K_A|K_RIGHT|K_START)){ if(p==2) return; p++; cnt=0; } if(pr&K_LEFT&&p) p--;
        if(cnt&7){ vsync(); continue; }
        stBack("HOW TO PLAY",(int)cnt); vsDraw(14,24,212,8,WHITE,pg[p],cnt);
        { char b[16]; b[0]='P'; b[1]='A'; b[2]='G'; b[3]='E'; b[4]=' '; b[5]=(char)('1'+p); b[6]=' '; b[7]='O'; b[8]='F'; b[9]=' '; b[10]='3'; b[11]=0; text(176,138,b,DIMC,1); }
        s2pill(5,147,tw(p==2?"A BACK":"A NEXT",1)+10,p==2?"A BACK":"A NEXT"); present(); }
}
// the hand's sounds, by what A did (0: none): a road is a tick, a site a pop, a load handed over a till, a refusal a bonk
static const u8 kpSfx[KHR_N]={ 0, SFX_TICK, SFX_TICK, SFX_POP, SFX_BONK, SFX_TICK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK,
    SFX_POP, SFX_CASH, SFX_POP, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK,
    SFX_POP, SFX_LAND,
    SFX_POP, SFX_LAND, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK, SFX_BONK };
static const u16 kpMil[8]={ 10, 25, 50, 100, 150, 200, 250, 320 };   // the crowd's milestones: said in the text strip the first time the realm reaches them
static void kpOver(const KmMem*m,const KGiant*G,u32 cnt){   // the overview screen (mode 3): the whole realm, the giant a blinking dot
    rect(0,0,240,160,RGB(0,0,2)); koDraw(fb,m->S,kgCx(G),kgCy(G),(int)((cnt>>3)&1)); text(2,1,"OVERVIEW",GOLD,1);
    { const char*a="SEL STATUS"; s2pill(5,147,tw(a,1)+10,a); } present(); }
static const char* const kpStageMsg[KG_CASTLE]={ "FOUNDATION LAID", "WALLS STAND", "TOWERS RISE", "CASTLE DONE  A MONARCH IS CROWNED" };   // (step 5b) said in the text layer when a castle stage is finished
static int kpRun(int slot){   // returns 0: the KINGDOM list should rescan (the realm's slot moved)
    KmMem m; int e=kpOpen(&m,slot); if(e){ toast(kpWhy(e)); return 0; }
    char nm[SLOT_NAME+1]; { int i=0; for(;i<SLOT_NAME&&slI[slot].name[i];i++) nm[i]=slI[slot].name[i]; nm[i]=0; if(!i){ nm[0]='R'; nm[1]='E'; nm[2]='A'; nm[3]='L'; nm[4]='M'; nm[5]=0; } }
    kpSettle(&m);
    static const int per[3]={1,4,16}; int spd=0, mapOn=1, ov=0, hudT=0, hudD=1; const char*hmsg=0; u32 cnt=0; u16 prev=keyNow(); KHand hand; khInit(&hand); int hcx=-1, hcy=-1, lrx=-1, lry=-1;   // lrx, lry: the last cell a road was laid on by holding A
    KGiant G; kgPlace(&G,m.S,m.R->gx,m.R->gy); kpGiant=m.R->giant; kpBk=0;
    KView V; kvInit(&V,m.cells,m.S->dim,m.B,m.R->nB); V.castle=m.R->castle; kgCamera(&V,&G); kvHwEnter(&V,m.S);
    KFx fx; kfInit(&fx); u16 plan[KF_MAX*3]; int water=0, lastCastle=m.R->castle, mil=0; static char tipB[48];
    while(mil<8&&m.S->pop>=kpMil[mil]) mil++;   // (the ones it is past already are not said again)   // (the plan of the sprites: 720 bytes, once; the carriers' tracking: 200)
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_SEL||(!mapOn&&pr&K_B)){ if(mapOn){ kvHwLeave(); mapOn=0; ov=1; kpOver(&m,&G,cnt); } else if(ov&&pr&K_SEL){ ov=0; kpDraw(&m,nm,spd,cnt); } else { mapOn=1; ov=0; kvHwEnter(&V,m.S); hudD=1; } }   // SELECT: map, overview, status, map (B goes back to the map from either screen)
        if(pr&K_START){
            static char sp[24]; static const char* const mn[8]={"GOALS","HOW TO PLAY","BUILD MENU",sp,"SAVE AND LEAVE","SAVE AND KEEP PLAYING","LEAVE WITHOUT SAVING","KEEP PLAYING"};
            { char*e=khCat(sp,"SPEED  X"); khNum(e,spd==0?1:spd==1?4:16); }
            if(mapOn) kvHwLeave();
            int ch=menu(nm,mn,8);
            if(ch==0) kpGoals(&m);                                                 // the open mission, live, and what the advisor says is stuck
            else if(ch==1) kpHelp();
            else if(ch==2){ char bl[KHM_N][48]; const char*bi[KHM_N]; for(int i=0;i<KHM_N;i++){ khCost(bl[i],m.S,i); bi[i]=bl[i]; } int c=menu("BUILD",bi,KHM_N); if(c>=0) hand.mode=c; }   // (A sets the mode, B backs out)
            else if(ch==3) spd=(spd+1)%3;                                          // (the speed moved here from SELECT: B is now RUN)
            else if(ch==4||ch==5){ m.R->gx=(u8)kgCx(&G); m.R->gy=(u8)kgCy(&G); int s=kpSave(&m,slot); if(s>=0){ slot=s; toast("KINGDOM SAVED"); if(ch==4) return 0; } else toast(s==KP_E_HELD?"PUT THE BUILDING DOWN FIRST":s==SLE_NOROOM?"NO ROOM  FREE A SLOT":slErrMsg(s)); }   // (a failed save never leaves: the realm in play is the only copy of its progress)
            else if(ch==6){ static const char* const yn[2]={"YES  LEAVE","NO  KEEP PLAYING"}; if(menu("EVERYTHING SINCE THE LAST SAVE IS LOST",yn,2)==0) return 0; }
            prev=keyNow(); if(mapOn){ kvHwEnter(&V,m.S); hudD=1; } else if(ov) kpOver(&m,&G,cnt); else kpDraw(&m,nm,spd,cnt); continue; }
        if(mapOn){                                                                 // the map: the giant walks, the camera follows, what came into view is written, one row is refreshed
            vsync();
            { int dx=(k&K_RIGHT?1:0)-(k&K_LEFT?1:0), dy=(k&K_DOWN?1:0)-(k&K_UP?1:0); padTrack(pr); padFour(&dx,&dy); if(dx||dy) kgMove(&G,m.S,dx,dy,k&K_B?khRun(m.R->giant):khStride(m.R->giant)); else G.moving=0; }
            kgCamera(&V,&G); V.nB=m.R->nB;
            kvSync(&V); kvRefreshRow(&V); kvScroll(&V);
            int cx=kgCx(&G), cy=kgCy(&G);
            if(!(cnt&23)){ water^=1; kpWater(water); }                              // the waves
            if(!(cnt&7)) kpTint(m.S);                                               // the light of the day (eight frames: 75 small steps a day)
            { int n=kfPlan(plan,&fx,m.S,&V,&G,(unsigned)cnt,spd==0); kpFxShow(plan,n); }   // carriers, workers, smoke, the cursor
            if(pr&(K_L|K_R)){ khCycle(&hand,pr&K_R?1:-1); hudD=1; }
            else if((pr&K_A)||(hand.mode==KHM_ROAD&&(k&K_A)&&(cx!=lrx||cy!=lry)&&!ksIsRoad(m.S,cy*m.S->dim+cx))){ lrx=cx; lry=cy; int r=khAct(&hand,m.S,cx,cy); if(kpSfx[r<KHR_N?r:0]) sfxPlay(kpSfx[r<KHR_N?r:0]); if(r==KHR_GOT) kvPoke(&V,cx,cy); else if(khRedraw(r)){ V.nB=m.R->nB; kvFull(&V); } hmsg=khMsg[r]; hudT=75; hudD=1; }   // (said in the text layer for 1.25 seconds)
            if(hudT&&!--hudT){ hmsg=0; hudD=1; }
            if(cx!=hcx||cy!=hcy||!(cnt&63)){ hcx=cx; hcy=cy; hudD=1; }
            if(hudD){ hudD=0; kpHud(m.S,m.R,&hand,cx,cy,spd,hmsg); }
        } else if(cnt&7) vsync(); else if(ov) kpOver(&m,&G,cnt); else kpDraw(&m,nm,spd,cnt);
        for(int i=0;i<per[spd];i++) ksStep(m.S);
        kfTrack(&fx,m.S); if(m.R->castle!=lastCastle){ lastCastle=m.R->castle; V.castle=lastCastle; if(mapOn) kvFull(&V); }   // (a castle stage finished: the castle looks different)
        if(!(cnt%60)){ if(exKingTick(m.S)){ hmsg=exB; hudT=150; hudD=1; } }   // (Phase 5c: the open mission's tasks, checked against this realm)
        if(m.S->ev){ int ev=m.S->ev; m.S->ev=0; if(ev>=1&&ev<=KG_CASTLE){ hmsg=kpStageMsg[ev-1]; if(ev==KG_CASTLE){ static char crB[40]; char mb[24]; kpMonarch(m.R,mb); char*e=khCat(crB,mb); khCat(e," IS CROWNED"); hmsg=crB; sfxPlay(SFX_CASH); } hudT=240; hudD=1; sfxPlay(SFX_BELL); } }
        if(!(cnt%60)&&mil<8&&m.S->pop>=kpMil[mil]){ static char milB[40]; char*e=khNum(milB,kpMil[mil]); khCat(e," NUBBINS LIVE HERE"); hmsg=milB; hudT=200; hudD=1; mil++; sfxPlay(SFX_POP); }   // (a crowd to be proud of)
        if(cnt%5400==2700&&!hmsg){ KAdv ad; if(kaAdvise(m.S,&ad,1)){ char t[48]; kaText(m.S,&ad,t); char*e=khCat(tipB,"> "); khCat(e,t); hmsg=tipB; hudT=270; hudD=1; } }   // (every 90 seconds: the most pressing thing that is stuck)
    }
}
#endif
