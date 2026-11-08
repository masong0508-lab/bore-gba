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
#include "kingamb.h"    // the seasons and the ambient life (snow, leaves, butterflies, birds, fireflies, bats)
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
    int sn,nx,sk; kaSeason(S,&sn,&nx,&sk);   // the season: the land's own colours (bank 0) first, then the light of the day over everything
    volatile u16*pal=(volatile u16*)0x05000000; for(int b=0;b<15;b++) for(int i=0;i<16;i++) pal[b*16+i]=kvTintC((b==0||(b>=2&&i>=1&&i<=3))?kaSeasonMix(kaPal[b][i],sn,nx,sk):kaPal[b][i],tr,tg,tb);   // (the land, and the three greens every building's own picture keeps for the ground round it)
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
    { volatile u16*at=(volatile u16*)(0x06010000+KA_AMB*32); u16 tl[16]; for(int t=0;t<KA_AMB_N;t++){ kaAmbTile(t,tl); for(int i=0;i<16;i++) at[t*16+i]=tl[i]; }   // the weather's and the wildlife's pictures: OBJ tiles KA_AMB.., bank 9
      volatile u16*ap=(volatile u16*)(0x05000200+KA_AMBPAL*32); for(int i=0;i<16;i++) ap[i]=kaAmbPal[i]; }
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
      "D pad walks the giant and B runs. A does whatever the MODE in the text strip says; L and R change it: HAND, ROAD, a building, DEMOLISH, MOVE or NUBBIN. The HAND lifts goods off the ground under your feet, and A again puts them down. SELECT: overview, status, map. START: menu.",
      "A site is a building waiting for goods. The Nubbins carry them along ROADS from the storehouse, so build a STOREHOUSE first and lay road to every door. A building with no road beside it never gets served. Roads are the realm.",
      "Homes bring Nubbins, Nubbins take jobs, jobs make goods. A well and a tavern near a workplace speed it up. Build the castle in four stages and a monarch is crowned. The realm never ends: GOALS shows what is next, and the ADVISOR tells you what is stuck." };
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
// THE CARAVAN's counter (kingamb.h): with a load in the hands, A sells it; with empty hands a list of every good in the stockpile opens, A sells up to ten of the one picked (the money is the life game's:
// simMoneyAdd), B leaves. The books stay true: what is sold leaves the stockpile and S->made together, like a bridge's stone.
static void kpTrade(KmMem*m,KView*V,const char**say){
    KSim*S=m->S; static char sb[44]; KHold*h=&S->hd;
    if(h->k==KHD_GOODS&&h->n>0){ int g=h->g, n=h->n, v=kaPay(S,g,n); S->made[g]-=n; h->k=KHD_NONE; h->n=0; simMoneyAdd((money_t)v);
        char*e=khCat(sb,"SOLD "); e=khNum(e,n); e=khCat(e," "); e=khCat(e,kaGoodNm[g&7]); e=khCat(e,"  +"); e=khNum(e,v); khCat(e," SIMOLEONS"); *say=sb; sfxPlay(SFX_CASH); return; }
    if(h->k){ *say="PUT THAT DOWN FIRST"; sfxPlay(SFX_BONK); return; }
    kvHwLeave(); int sold=0, total=0, cur=0;
    for(;;){
        char bl[KG_GOODS][40]; const char*bi[KG_GOODS]; char ttl[40];
        for(int g=0;g<KG_GOODS;g++){ char*e=khCat(bl[g],kaGoodNm[g]); e=khCat(e,"  "); e=khNum(e,kaPrice[g]); e=khCat(e," EACH  HAVE "); e=khNum(e,m->R->goods[g]); if(g==kaWanted(S)) khCat(e,"  WANTED"); bi[g]=bl[g]; }
        { char*e=khCat(ttl,"THE CARAVAN WANTS "); khCat(e,kaGoodNm[kaWanted(S)]); }
        rect(0,0,SW,SH,RGB(2,5,12)); rect(0,0,SW,13,RGB(5,12,22)); text(8,3,"THE LAND BOREFORE TIME",GOLD,1);
        int c=menu(ttl,bi,KG_GOODS); if(c<0) break; cur=c;
        int n=m->R->goods[c]; if(n>10) n=10; if(n<=0){ toast("NONE IN THE STOREHOUSE"); continue; }
        int v=kaPay(S,c,n); m->R->goods[c]=(u16)(m->R->goods[c]-n); S->made[c]-=n; simMoneyAdd((money_t)v); sold+=n; total+=v; sfxPlay(SFX_CASH);
        { char*e=khCat(sb,"SOLD "); e=khNum(e,n); e=khCat(e," "); e=khCat(e,kaGoodNm[c]); e=khCat(e,"  +"); e=khNum(e,v); khCat(e," SIMOLEONS"); toast(sb); } }
    (void)cur; kvHwEnter(V,S);
    if(sold){ char*e=khCat(sb,"SOLD "); e=khNum(e,sold); e=khCat(e," GOODS  +"); e=khNum(e,total); khCat(e," SIMOLEONS"); *say=sb; } else *say="THE TRADER SHRUGS";
}
// THE CEREMONY: a castle stage done (1 foundation, 2 walls, 3 towers) or the crown (4). A real-time scene of its own (the map's hardware mode is left and entered again by the caller): the castle as far as it
// has come, the Nubbins at its foot, and for the crown a night of fireworks, bells and flags. It runs on the vblank clock, so it takes the same time at any picture rate; A skips it once it has had its moment.
static void kpRoof(int cx,int top,int half,int h,u16 c,u16 d){ for(int r=0;r<h;r++){ int w=half*(r+1)/h; rect(cx-w,top+r,2*w,1,(r&1)?d:c); } }
static void kpCeremony(int stage,const char*title,const char*sub){
    int crown=stage>=KG_CASTLE, T=crown?480:130, skipAt=crown?110:36; u32 vb0=vblN, fired=0; u16 prev=keyNow();
    static const u16 fwc[4]={ RGB(31,26,6), RGB(31,10,8), RGB(12,26,31), RGB(22,31,10) };
#define CY(v) ((v)+14)
    for(;;){
        int t=(int)(vblN-vb0); if(t>=T) break;
        u16 k=keyNow(), pr=k&~prev; prev=k; if(t>skipAt&&(pr&(K_A|K_B|K_START))) break;
        for(int y=0;y<SH;y+=2) rect(0,y,SW,2,crown?exMix(RGB(0,0,5),RGB(8,4,14),y*16/SH):exMix(RGB(3,6,16),RGB(31,17,8),y*16/SH));   // the sky: a night for the crown, a dusk for a stage
        for(int i=0;i<(crown?46:22);i++){ unsigned h=kvHash(i,7); int x=(int)(h%240u), y=(int)((h>>9)%100u); px(x,y,(((t>>3)+i)&7)<6?RGB(28,28,31):RGB(12,12,18)); }
        disc(34,44,8,RGB(29,29,26)); disc(37,42,7,crown?RGB(3,2,9):RGB(11,9,15));   // the moon, thin
        { int hy=CY(120); for(int x=0;x<SW;x+=6){ int hh=8+exSn(x*2+11,150)/20; rect(x,hy-hh,6,hh+2,RGB(2,7,5)); } rect(0,hy,SW,SH-hy,RGB(2,6,4));
          for(int y=hy;y<SH;y++){ int hw=6+(y-hy)/2; rect(120-hw,y,2*hw,1,RGB(9,6,3)); } }   // the hills, and the road widening towards us
        u16 st=RGB(17,17,20), sh=RGB(11,11,13), hl=RGB(23,23,25);
        rect(62,CY(112),116,8,sh); rect(62,CY(112),116,1,hl);   // the footing
        if(stage==1){ for(int x=70;x<=166;x+=24){ rect(x,CY(84),2,28,RGB(14,9,4)); } rect(70,CY(84),98,2,RGB(14,9,4)); rect(70,CY(98),98,2,RGB(14,9,4)); }   // scaffolding
        if(stage>=2){ rect(68,CY(84),104,28,st); rect(68,CY(84),2,28,hl); for(int y=90;y<112;y+=6) rect(68,CY(y),104,1,sh); for(int x=68;x<172;x+=8) rect(x,CY(80),5,4,st); rect(110,CY(96),20,16,RGB(2,2,4)); disc(120,CY(98),10,RGB(2,2,4)); rect(110,CY(96),20,1,RGB(2,2,4)); }
        if(stage>=3){ for(int q=0;q<2;q++){ int tx=q?160:56; rect(tx,CY(54),24,58,st); rect(tx,CY(54),2,58,hl); for(int y=60;y<112;y+=6) rect(tx,CY(y),24,1,sh); for(int x=tx;x<tx+24;x+=8) rect(x,CY(50),5,4,st); kpRoof(tx+12,CY(34),15,16,RGB(26,5,4),RGB(18,3,3)); rect(tx+10,CY(66),4,8,(((t>>4)+q)&3)?RGB(31,26,6):RGB(12,9,2)); }
            rect(96,CY(40),48,46,st); rect(96,CY(40),2,46,hl); for(int y=46;y<84;y+=6) rect(96,CY(y),48,1,sh); kpRoof(120,CY(16),28,24,RGB(26,5,4),RGB(18,3,3)); rect(116,CY(52),8,12,RGB(31,26,6)); rect(117,CY(54),2,8,RGB(12,9,2)); }
        if(crown){ for(int q=0;q<3;q++){ int fx=q==0?66:q==1?170:117, fy=q==2?CY(0):CY(18); rect(fx+13,fy,1,16,RGB(8,6,6)); for(int i=0;i<14;i++) rect(fx+14+i,fy+exSn(t*2+i*7,34)/50,1,7-i/3,RGB(31,8,6)); }
            for(int q=0;q<4;q++){ int tx=q<2?(q?150:92):(q==2?58:166); rect(tx,CY(102),2,3,RGB(31,22,4)); if(((t>>2)+q)&1) px(tx,CY(101),RGB(31,31,12)); } }
        // the crowd at the foot: a head, a body, arms up and jumping once the crown is on
        for(int i=0;i<26;i++){ unsigned h=kvHash(i,31); int x=8+i*9+(int)(h%4u), y=CY(132)+(int)((h>>5)%9u), j=(crown&&exSn(t*3+i*17,24)>50)?-3:0; u16 c=((h>>9)&3)==0?RGB(28,8,6):((h>>9)&3)==1?RGB(8,14,28):((h>>9)&3)==2?RGB(10,24,10):RGB(28,24,6);
            rect(x,y+j,4,5,c); rect(x,y-3+j,4,3,RGB(27,20,15)); if(crown||((t>>5)+i)&1){ px(x-1,y-2+j,RGB(27,20,15)); px(x+4,y-2+j,RGB(27,20,15)); } }
        if(crown){ for(int b=0;b<7;b++){ int tb=30+b*58, age=t-tb; if(age<0||age>110) continue; unsigned h=kvHash(b,3); int bx=40+(int)(h%160u), by=36+(int)((h>>9)%34u); u16 c=fwc[(b+(int)(h>>20))&3];
                if(!(fired&(1u<<b))){ fired|=1u<<b; sfxPlay((b&1)?SFX_THUNDER:SFX_POP); }
                if(age<6){ int fr=age<3?8+age*6:26-age*3; disc(bx,by,fr,exMix(c,WHITE,8)); }   // the flash
                for(int p=0;p<36;p++){ int a=(p*64/36+b*5)&63, r=(50+(p&1)*14)*age/(age+14), x=bx+exS64[(a+16)&63]*r/127, y=by+exS64[a]*r/127+age*age/120; u16 f=exMix(c,RGB(0,0,4),age>40?(age-40)*16/70:0);
                    rect(x,y,3,3,f); if(age<70&&(p&1)) rect(x-exS64[(a+16)&63]/30,y-exS64[a]/30,2,2,exMix(f,RGB(0,0,4),8)); } }
            if((t&63)==2) sfxPlay(SFX_BELL); }
        // the words on a dark plate
        exDarken(0,0,SW,crown?30:26,0); rect(0,crown?30:26,SW,1,GOLD);
        text((SW-tw(title,2))/2,3,title,crown?GOLD:WHITE,2); text((SW-tw(sub,1))/2,crown?19:17,sub,RGB(20,26,31),1);
        if(t>skipAt) text(SW-tw("A CONTINUE",1)-4,SH-10,"A CONTINUE",DIMC,1);
        present();
    }
#undef CY
}
static int kpRun(int slot){   // returns 0: the KINGDOM list should rescan (the realm's slot moved)
    KmMem m; int e=kpOpen(&m,slot); if(e){ toast(kpWhy(e)); return 0; }
    char nm[SLOT_NAME+1]; { int i=0; for(;i<SLOT_NAME&&slI[slot].name[i];i++) nm[i]=slI[slot].name[i]; nm[i]=0; if(!i){ nm[0]='R'; nm[1]='E'; nm[2]='A'; nm[3]='L'; nm[4]='M'; nm[5]=0; } }
    kpSettle(&m);
    static const int per[3]={1,4,16}; int spd=0, mapOn=1, ov=0, hudT=0, hudD=1; const char*hmsg=0; u32 cnt=0; u16 prev=keyNow(); KHand hand; khInit(&hand); int hcx=-1, hcy=-1, lrx=-1, lry=-1;   // lrx, lry: the last cell a road was laid on by holding A
    KGiant G; kgPlace(&G,m.S,m.R->gx,m.R->gy); kpGiant=m.R->giant; kpBk=0;
    KView V; kvInit(&V,m.cells,m.S->dim,m.B,m.R->nB); V.castle=m.R->castle; kgCamera(&V,&G); kvHwEnter(&V,m.S);
    u32 vbPrev=vblN; int stCap=16, stOk=0;
    KFx fx; kfInit(&fx); ksSeasonsOn=1; int lastCv=kaCaravanHere(m.S); int lastSn=(int)((m.R->day>>3)&3u); u16 plan[128*3]; int water=0, lastCastle=m.R->castle, mil=0; static char tipB[48];
    while(mil<8&&m.S->pop>=kpMil[mil]) mil++;   // (the ones it is past already are not said again)   // (the plan of the sprites: 720 bytes, once; the carriers' tracking: 200)
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&K_SEL||(!mapOn&&pr&K_B)){ if(mapOn){ kvHwLeave(); mapOn=0; ov=1; kpOver(&m,&G,cnt); } else if(ov&&pr&K_SEL){ ov=0; kpDraw(&m,nm,spd,cnt); } else { mapOn=1; ov=0; kvHwEnter(&V,m.S); hudD=1; } }   // SELECT: map, overview, status, map (B goes back to the map from either screen)
        if(pr&K_START){
            static char sp[24]; static const char* const mn[8]={"GOALS","HOW TO PLAY","BUILD MENU",sp,"SAVE AND LEAVE","SAVE AND KEEP PLAYING","LEAVE WITHOUT SAVING","KEEP PLAYING"};
            { char*e=khCat(sp,"SPEED  X"); khNum(e,spd==0?1:spd==1?4:16); }
            if(mapOn){ kvHwLeave(); rect(0,0,SW,SH,RGB(2,5,12)); rect(0,0,SW,13,RGB(5,12,22)); text(8,3,"THE LAND BOREFORE TIME",GOLD,1); }   // (a plain backdrop: the last screen the bitmap held, the realm list, showed through the menu)
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
            { int n=kfPlan(plan,&fx,m.S,&V,&G,(unsigned)cnt,spd==0); n=kaCaravan(plan,n,m.S,&V,(unsigned)cnt); n=kaAmbient(plan,n,m.S,(unsigned)cnt); kpFxShow(plan,n); }   // carriers, workers, smoke, the cursor
            if(pr&(K_L|K_R)){ khCycle(&hand,pr&K_R?1:-1); hudD=1; }
            else if((pr&K_A)&&hand.mode==KHM_HAND&&kaNearWagon(m.S,cx,cy)){ const char*say=0; kpTrade(&m,&V,&say); prev=keyNow(); hmsg=say; hudT=200; kpHud(m.S,m.R,&hand,cx,cy,spd,hmsg); hudD=0; }   // (the caravan's counter: HAND + A beside the wagon)
            else if((pr&K_A)||(hand.mode==KHM_ROAD&&(k&K_A)&&(cx!=lrx||cy!=lry)&&!ksIsRoad(m.S,cy*m.S->dim+cx))){ lrx=cx; lry=cy; int r=khAct(&hand,m.S,cx,cy); if(kpSfx[r<KHR_N?r:0]) sfxPlay(kpSfx[r<KHR_N?r:0]); if(r==KHR_GOT) kvPoke(&V,cx,cy); else if(khRedraw(r)){ V.nB=m.R->nB; kvFull(&V); } hmsg=khMsg[r]; hudT=75; hudD=1; }   // (said in the text layer for 1.25 seconds)
            if(hudT&&!--hudT){ hmsg=0; hudD=1; }
            if(((cx!=hcx||cy!=hcy)&&!(cnt&3))||!(cnt&63)){ hcx=cx; hcy=cy; hudD=1; }   // (walking: the text strip follows the cell every fourth frame, it is the costliest thing the map draws)
            if(hudD){ hudD=0; kpHud(m.S,m.R,&hand,cx,cy,spd,hmsg); }
        } else if(cnt&7) vsync(); else if(ov) kpOver(&m,&G,cnt); else kpDraw(&m,nm,spd,cnt);
        { int late=(int)(vblN-vbPrev); vbPrev=vblN;   // the realm runs as fast as the picture allows: a frame that came late means fewer steps next time (X16 is "as fast as it can" in a big realm; the map keeps its 60 pictures a second), and the allowance creeps back up while frames are on time
          if(late>1&&late<8){ stCap=stCap*3/4; if(stCap<2) stCap=2; stOk=0; } else if(late<=1&&++stOk>=20){ stOk=0; if(stCap<16) stCap++; }
          int want=per[spd]; if(want>stCap) want=stCap; for(int i=0;i<want;i++) ksStep(m.S); }
        kfTrack(&fx,m.S); if(m.R->castle!=lastCastle){ lastCastle=m.R->castle; V.castle=lastCastle; if(mapOn) kvFull(&V); }   // (a castle stage finished: the castle looks different)
        if(!(cnt%60)){ int cv=kaCaravanHere(m.S); if(cv!=lastCv){ lastCv=cv; if(cv){ static char cvB[44]; char*e=khCat(cvB,"CARAVAN AT THE STORE  WANTS "); khCat(e,kaGoodNm[kaWanted(m.S)]); hmsg=cvB; sfxPlay(SFX_BELL); } else hmsg="THE CARAVAN HAS MOVED ON"; hudT=300; hudD=1; } }
        if(!(cnt%60)){ int sn=(int)((m.R->day>>3)&3u); if(sn!=lastSn){ lastSn=sn; hmsg=kaSeasonMsg[sn]; hudT=300; hudD=1; sfxPlay(SFX_BELL); } }   // (a season turns: said in the text strip)
        if(!(cnt%60)){ if(exKingTick(m.S)){ hmsg=exB; hudT=150; hudD=1; } }   // (Phase 5c: the open mission's tasks, checked against this realm)
        if(m.S->ev){ int ev=m.S->ev; m.S->ev=0; if(ev>=1&&ev<=KG_CASTLE){ hmsg=kpStageMsg[ev-1]; if(ev==KG_CASTLE){ static char crB[40]; char mb[24]; kpMonarch(m.R,mb); char*e=khCat(crB,mb); khCat(e," IS CROWNED"); hmsg=crB; sfxPlay(SFX_CASH); } hudT=240; hudD=1; sfxPlay(SFX_BELL);
            if(mapOn){ char tt[40], sb[44]; const char*sub; if(ev==KG_CASTLE){ char mb[24]; kpMonarch(m.R,mb); khCat(sb,mb); khCat(tt,"CROWNED"); sub=sb; } else { khCat(tt,kpStageMsg[ev-1]); char*e=khCat(sb,"CASTLE  STAGE "); e=khNum(e,ev); khCat(e," OF 4"); sub=sb; }
                kvHwLeave(); kpCeremony(ev,tt,sub); prev=keyNow(); kvHwEnter(&V,m.S); kpHud(m.S,m.R,&hand,kgCx(&G),kgCy(&G),spd,hmsg); hudD=0; vbPrev=vblN; } } }   // (the text strip is drawn at once: its tiles are empty until it is)
        if(!(cnt%60)&&mil<8&&m.S->pop>=kpMil[mil]){ static char milB[40]; char*e=khNum(milB,kpMil[mil]); khCat(e," NUBBINS LIVE HERE"); hmsg=milB; hudT=200; hudD=1; mil++; sfxPlay(SFX_POP); }   // (a crowd to be proud of)
        if(cnt%5400==2700&&!hmsg){ KAdv ad; if(kaAdvise(m.S,&ad,1)){ char t[48]; kaText(m.S,&ad,t); char*e=khCat(tipB,"> "); khCat(e,t); hmsg=tipB; hudT=270; hudD=1; } }   // (every 90 seconds: the most pressing thing that is stuck)
    }
}
#endif
