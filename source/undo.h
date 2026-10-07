// undo.h - UNDO / REDO for the room builder (BUILD ROOM, the pause menu's EDIT MAP, and a community lot built from the town view).
//
// HOW IT WORKS   Every tile the builder is about to change is recorded first (udRec: the tile's three bytes - item char, floor, wallpaper -
//                before the change). One ACTION is everything done between pressing A (or B) and letting go: a whole ROOM, a WALL line, a
//                FLOOR fill, or a held-down stroke of items. UNDO swaps the recorded tiles back with the live ones, so the same records
//                serve REDO (swap again). Because only the recorded tiles are touched, undo is safe after a play test.
// MONEY          what an action cost (or paid back, when it sold something) is kept with it: UNDO refunds it, REDO charges it again. If the
//                purse cannot cover it, the step is refused ("NOT ENOUGH CASH TO UNDO").
// FLOORS         an action remembers its floor. UNDO / REDO on another floor goes there first (flGo), like SELECT + UP / DOWN.
// LIMITS         UD_ACT steps and UD_REC tiles in all (4 bytes a tile). The oldest steps fall off the back. One step bigger than UD_REC
//                (a whole 40 x 40 fill) cannot be undone: it clears the history instead and says so.
// KEYS           SELECT + B = UNDO, SELECT + START = REDO, and the MAP MENU (START) lists both with the number of steps left.
// MEMORY         about 4.2 KB of EWRAM (udR 4000 B + udA 136 B). IWRAM is untouched. Lower UD_REC if EWRAM gets tight (make size).
// WHERE          included by main.c just before the map editor; mapPlace / edPay (earlier in main.c) call udRec / count the cash through
//                the forward declarations at the top, edSet / eApply and mapEditor use the rest.
#define UD_REC 1000
#define UD_ACT 3   // you can undo three times (and redo them again); raise it for a longer history
_Static_assert(NFL<=16&&NWALL<=256&&MW*MH<=2048,"undo.h packs a tile into 4 bytes: 11 bits of position, 4 of floor, 8 of wallpaper, 8 of item");
#define UD_PACK(i,f,w,c) ((u32)(i)|((u32)(f)<<11)|((u32)(w)<<15)|((u32)(u8)(c)<<23))
typedef struct { u16 s,n; s16 cash; u8 fl,pad; } UdAct;   // a step: its first record, how many, what it cost (negative: it sold), and its floor
static u32 udR[UD_REC] EWRAM_BSS;                          // the recorded tiles
static UdAct udA[UD_ACT+1] EWRAM_BSS;                      // the steps (one spare slot: the step being drawn)
static u16 udNR EWRAM_BSS;                                 // records in use
static u8 udNA EWRAM_BSS, udCur EWRAM_BSS, udOpen EWRAM_BSS, udOver EWRAM_BSS, udBig EWRAM_BSS;   // steps kept, steps applied (the rest can be redone), one is being drawn, it outgrew the history, tell the player

static void udClear(void){ udNA=udCur=udOpen=udOver=udBig=0; udNR=0; udCashD=0; }
static void udDrop(void){   // forget the oldest step to make room. With nothing to forget the step being drawn is too big: the whole history goes
    if(!udNA){ udOver=1; udNR=0; udCur=0; return; }
    int d=udA[0].n;
    for(int i=d;i<udNR;i++) udR[i-d]=udR[i];
    udNR=(u16)(udNR-d);
    for(int i=0;i<udNA;i++){ udA[i]=udA[i+1]; udA[i].s=(u16)(udA[i].s-d); }   // (the step being drawn, if any, moves down with the rest)
    udNA--; udCur--;
}
static void udBegin(void){   // a new step: whatever could have been redone is gone
    udNA=udCur;
    udNR=udNA?(u16)(udA[udNA-1].s+udA[udNA-1].n):0;
    while(udNA>=UD_ACT) udDrop();
    udA[udNA].s=udNR; udA[udNA].n=0; udA[udNA].cash=0; udA[udNA].fl=(u8)curFl; udOpen=1;
}
static void udRec(int x,int y){   // call BEFORE changing tile (x, y)
    if(!udOn||udOver) return;
    if(!udOpen) udBegin();
    if(udNR>=UD_REC){ udDrop(); if(udOver) return; }
    udR[udNR++]=UD_PACK(y*MW+x,floorMap[y][x],wallMap[y][x],lifeMap[y][x]);
    udA[udNA].n++;
}
static int udDiffers(int k){ u32 r=udR[k]; int i=(int)(r&2047), y=i/MW, x=i-y*MW;
    return floorMap[y][x]!=(u8)((r>>11)&15)||wallMap[y][x]!=(u8)((r>>15)&255)||(u8)lifeMap[y][x]!=(u8)((r>>23)&255); }
static void udEnd(void){   // the step is finished (A / B let go, a menu, a floor change, leaving): keep it, unless nothing really changed
    if(udOver){ udOver=0; udOpen=0; udNA=udCur=0; udNR=0; udBig=1; udCashD=0; return; }
    if(!udOpen){ udCashD=0; return; }
    udOpen=0;
    UdAct*a=&udA[udNA]; int ch=0;
    for(int k=a->s;k<a->s+a->n&&!ch;k++) ch=udDiffers(k);
    if(!ch){ udNR=a->s; udCashD=0; return; }   // (a tile put back as it was: not a step)
    a->cash=(s16)udCashD; udCashD=0; udNA++; udCur=udNA;
}
static void udSwap(int k){   // record k <-> the live tile
    u32 r=udR[k]; int i=(int)(r&2047), y=i/MW, x=i-y*MW;
    udR[k]=UD_PACK(i,floorMap[y][x],wallMap[y][x],lifeMap[y][x]);
    lifeMap[y][x]=(char)((r>>23)&255); floorMap[y][x]=(u8)((r>>11)&15); wallMap[y][x]=(u8)((r>>15)&255);
}
static const char* udUndo(void){
    udEnd();
    if(!udCur) return "NOTHING TO UNDO";
    UdAct*a=&udA[udCur-1];
    if(a->cash&&edCharged()&&simMoney+a->cash<0) return "NOT ENOUGH CASH TO UNDO";   // (undoing a sale buys the things back)
    if(a->fl!=curFl&&!flGo(a->fl)) return "TOO MUCH BUILT TO CHANGE FLOOR";
    for(int k=a->s+a->n-1;k>=a->s;k--) udSwap(k);
    if(a->cash&&edCharged()){ int m=simMoney+a->cash; if(m>9999) m=9999; simMoney=m; edCashDirty=1; }
    udCur--; wDirty=1; return "UNDONE";
}
static const char* udRedo(void){
    udEnd();
    if(udCur>=udNA) return "NOTHING TO REDO";
    UdAct*a=&udA[udCur];
    if(a->cash&&edCharged()&&simMoney-a->cash<0) return "NOT ENOUGH CASH TO REDO";
    if(a->fl!=curFl&&!flGo(a->fl)) return "TOO MUCH BUILT TO CHANGE FLOOR";
    for(int k=a->s;k<a->s+a->n;k++) udSwap(k);
    if(a->cash&&edCharged()){ int m=simMoney-a->cash; if(m>9999) m=9999; simMoney=m; edCashDirty=1; }
    udCur++; wDirty=1; return "REDONE";
}
static void edSet(int x,int y,int l,int f,int w){   // change a tile and record it. l / f / w below 0 = leave that part as it is. A tile that would not change is not touched
    char nl=l<0?lifeMap[y][x]:(char)l; int nf=f<0?floorMap[y][x]:f, nw=w<0?wallMap[y][x]:w;
    if(nl==lifeMap[y][x]&&nf==floorMap[y][x]&&nw==wallMap[y][x]) return;
    udRec(x,y); lifeMap[y][x]=nl; floorMap[y][x]=(u8)nf; wallMap[y][x]=(u8)nw;
}
