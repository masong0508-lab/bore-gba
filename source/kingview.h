// kingview.h - EP6 THE LAND BOREFORE TIME, the map as the GBA's hardware tile background (Plan B, roadmap 5.3). PURE C: the only hardware is the macro KV_PUT.
// WHAT IT DOES: a realm cell is 16 x 16 pixels = 2 x 2 tiles of 8 x 8. A 32 x 32 tile screenblock (256 x 256 pixels) is a ring: world tile (x,y) lives at (x&31, y&31), and the hardware scroll
// registers (camera x&255, y&255) show the right part of it. The screen needs 31 x 21 tiles; when the camera crosses a tile edge only the newly exposed column / row is written
// (at most 21 + 31 tiles a frame at 8 pixels a frame), so scrolling costs about 3,000 cycles (roadmap 5.3), not a redraw.
// THE ART (tools/make_king_art.py -> kingart.h, Phase 6): every kind of ground has several looks picked by a hash of where it is, so no two meadows are the same and nothing repeats in a grid:
//   GRASS and MEADOW: eight 8 x 8 tiles each.  FOREST: four trees (round, pine, a pair, a copse).  ROCK: three crags.  CRYSTAL: two clusters.  Off the map: deep water.
//   WATER is AUTOTILED by quarters: each 8 x 8 quarter of a water cell looks at the two cells it touches and the one on its corner, and draws a beach, foam and the shallows where they are land.
//   ROAD is AUTOTILED the same way: each quarter knows whether the road goes on across its two edges (a cell with a building counts: a road leads to its door). A BRIDGE lies along the road.
//   BUILDINGS are 32 x 32 pictures of their own (a 4 x 4 grid of tiles), one for each of the thirteen kinds, a building SITE (scaffolding) and five stages of the CASTLE, each with its own palette bank.
// WHAT KEEPS IT TRUE: kvSync (after the camera moved) writes what scrolled into view; kvRefreshRow (every frame) rewrites one tile row of the window, cycling, so a felled tree, a new road
// or a finished building shows within 21 frames; kvPoke(x,y) rewrites a cell and its eight neighbours at once (the autotiles look at them). Needs kingdom.h (KT_*, KC_BLD, KBld, kgDim), kingsim.h (KS_NTYPES).
// KV_PUT(sx,sy,entry) writes screenblock entry (sx,sy), both 0..31: the game defines it as a 16 bit VRAM write, the PC test as an array write.
#include "kingart.h"
#define KV_CELL 16         // pixels in a realm cell
#define KV_W 31            // tiles across the screen window (240 pixels + one for the scroll offset)
#define KV_H 21            // tiles down
#define KV_TILES KA_NT     // the map's tiles (the text tiles come after them)
#define KV_DEEP 7          // (the terrain number of the deep water off the map, kept for the older tests)
typedef struct { int x, y; int tx0, ty0; int ok, rr; int dim; const u8*cells; const KBld*B; int nB; int castle; } KView;   // x, y: the camera, in pixels, top left of the screen; castle: stages of the castle done (what it looks like)
static inline void kvInit(KView*V,const u8*cells,int dim,const KBld*B,int nB){ V->cells=cells; V->dim=dim; V->B=B; V->nB=nB; V->x=V->y=0; V->tx0=V->ty0=0; V->ok=0; V->rr=0; V->castle=0; }
static inline int kvMaxX(const KView*V){ return V->dim*KV_CELL-240; }
static inline int kvMaxY(const KView*V){ return V->dim*KV_CELL-160; }
static inline void kvClamp(KView*V){ int mx=kvMaxX(V), my=kvMaxY(V); if(V->x>mx) V->x=mx; if(V->y>my) V->y=my; if(V->x<0) V->x=0; if(V->y<0) V->y=0; }   // (the smallest land, 48 x 16 = 768 pixels, is bigger than the screen)
static inline void kvCenter(KView*V,int cx,int cy){ V->x=cx*KV_CELL+KV_CELL/2-120; V->y=cy*KV_CELL+KV_CELL/2-80; kvClamp(V); }   // the camera on a cell
static inline unsigned kvHash(int x,int y){ unsigned h=(unsigned)x*2654435761u^((unsigned)y*2246822519u+0x9E3779B9u); h^=h>>15; h*=0x85EBCA6Bu; h^=h>>13; return h; }
// the raw cell at (x,y): -1 off the land
static inline int kvRaw(const KView*V,int x,int y){ if(x<0||y<0||x>=V->dim||y>=V->dim) return -1; return V->cells[y*V->dim+x]; }
static inline int kvLandy(int v){ return v>=0&&((v&KC_BLD)||((v&7)!=KT_WATER&&(v&7)!=KT_BRIDGE)); }   // does a beach lie here? (a building's cell is land; a bridge is over water)
static inline int kvRoady(int v){ return v>=0&&((v&KC_BLD)||(v&7)==KT_ROAD||(v&7)==KT_BRIDGE); }    // does a road go on here? (a road leads to a building's door)
static inline int kvBldAt(const KView*V,int x,int y){ for(int i=0;i<V->nB;i++){ const KBld*b=&V->B[i]; if(x>=b->x&&x<b->x+kgFootW(b->type)&&y>=b->y&&y<b->y+kgFootH(b->type)) return i; } return -1; }
// which of the castle's five looks (KA_CASTLE0..4) a castle with this many stages done shows; -1: the site (nothing done yet)
static inline int kvCastleLook(int done,int crowned){ return crowned?4:done<=0?-1:done==1?0:done==2?1:3; }
// the screenblock entry for world tile (tx,ty): tile | palette bank << 12. (tx, ty >= 0; the cell is tx>>1, ty>>1 and the quarter of it is tx&1, ty&1)
static inline u16 kvEntry(const KView*V,int tx,int ty){
    int cx=tx>>1, cy=ty>>1, sx=tx&1, sy=ty&1, q=sy*2+sx;
    int v=kvRaw(V,cx,cy);
    if(v<0) return (u16)((KA_DEEP+(int)(kvHash(tx,ty)&3))|(1<<12));
    if(v&KC_BLD){
        int i=kvBldAt(V,cx,cy); if(i<0) return (u16)(KA_DEEP|(1<<12));   // (bit 7 with no building under it: a damaged map; deep water is the least harmful thing to show)
        const KBld*b=&V->B[i]; int X=tx-b->x*2, Y=ty-b->y*2; if(X<0||X>3||Y<0||Y>3) { X&=3; Y&=3; }
        int base, bank;
        if(b->type==KB_CASTLE){ int lk=kvCastleLook(V->castle,b->lvl); if(lk<0){ base=KA_SITE; bank=0; } else { base=KA_CASTLE0+lk*16; bank=kaBldBank[KB_CASTLE]; } }
        else if(!b->lvl){ base=KA_SITE; bank=0; }
        else { int t=b->type<14?b->type:0; base=kaBldTile[t]; bank=kaBldBank[t]; }
        return (u16)((base+Y*4+X)|(bank<<12));
    }
    int dx=sx?1:-1, dy=sy?1:-1;
    switch(v&7){
    case KT_GRASS:   return (u16)(KA_GRASS+(int)(kvHash(tx,ty)&7));
    case KT_MEADOW:  return (u16)(KA_MEADOW+(int)(kvHash(tx,ty)&7));
    case KT_FOREST:  return (u16)(KA_FOREST+(int)(kvHash(cx,cy)&3)*4+q);
    case KT_ROCK:    return (u16)((KA_ROCK+(int)(kvHash(cx,cy)%3u)*4+q)|(1<<12));
    case KT_CRYSTAL: return (u16)((KA_CRYSTAL+(int)(kvHash(cx,cy)&1)*4+q)|(1<<12));
    case KT_WATER: { int m=kvLandy(kvRaw(V,cx+dx,cy))|(kvLandy(kvRaw(V,cx,cy+dy))<<1)|(kvLandy(kvRaw(V,cx+dx,cy+dy))<<2); return (u16)((KA_WATER+q*8+m)|(1<<12)); }
    case KT_ROAD:  { int hv=kvRoady(kvRaw(V,cx+dx,cy))|(kvRoady(kvRaw(V,cx,cy+dy))<<1); return (u16)(KA_ROAD+((q*4+hv)*2+(int)(kvHash(cx,cy)&1))); }
    case KT_BRIDGE:{ int along=kvRoady(kvRaw(V,cx-1,cy))||kvRoady(kvRaw(V,cx+1,cy)); return (u16)((KA_BRIDGE+(along?0:4)+q)|(1<<12)); }   // along the road: planks across it
    default: return (u16)(KA_GRASS+(int)(kvHash(tx,ty)&7));
    }
}
// one cell and the eight round it (the autotiles look at their neighbours)
static inline void kvPoke(const KView*V,int x,int y){ if(!V->ok) return;
    for(int ty=(y-1)*2;ty<(y+2)*2+0;ty++) for(int tx=(x-1)*2;tx<(x+2)*2+0;tx++){ if(tx<V->tx0||ty<V->ty0||tx>=V->tx0+KV_W||ty>=V->ty0+KV_H) continue; KV_PUT(tx&31,ty&31,kvEntry(V,tx,ty)); } }
static inline void kvRow(const KView*V,int ty){ for(int tx=V->tx0;tx<V->tx0+KV_W;tx++) KV_PUT(tx&31,ty&31,kvEntry(V,tx,ty)); }
static inline void kvFull(KView*V){ int nx=V->x>>3, ny=V->y>>3; V->tx0=nx; V->ty0=ny; V->ok=1; for(int ty=ny;ty<ny+KV_H;ty++) kvRow(V,ty); }
// call after the camera moved (and kvClamp): writes only what scrolled into view
static inline void kvSync(KView*V){
    int nx=V->x>>3, ny=V->y>>3;
    int dx=nx-V->tx0, dy=ny-V->ty0; if(dx<0) dx=-dx; if(dy<0) dy=-dy;
    if(!V->ok||dx>=KV_W||dy>=KV_H){ kvFull(V); return; }
    int ox=V->tx0, oy=V->ty0; V->tx0=nx; V->ty0=ny;
    if(nx>ox){ for(int tx=ox+KV_W;tx<nx+KV_W;tx++) for(int ty=ny;ty<ny+KV_H;ty++) KV_PUT(tx&31,ty&31,kvEntry(V,tx,ty)); }
    else if(nx<ox){ for(int tx=nx;tx<ox;tx++) for(int ty=ny;ty<ny+KV_H;ty++) KV_PUT(tx&31,ty&31,kvEntry(V,tx,ty)); }
    if(ny>oy){ for(int ty=oy+KV_H;ty<ny+KV_H;ty++) kvRow(V,ty); }
    else if(ny<oy){ for(int ty=ny;ty<oy;ty++) kvRow(V,ty); }
}
static inline void kvRefreshRow(KView*V){ if(!V->ok) return; if(V->rr>=KV_H) V->rr=0; kvRow(V,V->ty0+V->rr); V->rr++; }
// ---- the light of the day: a colour scaled by a tint (r, g, b in sixteenths), used for the palettes ----
static inline u16 kvTintC(u16 c,int tr,int tg,int tb){ int r=(c&31)*tr>>4, g=((c>>5)&31)*tg>>4, b=((c>>10)&31)*tb>>4; return (u16)((r>31?31:r)|((g>31?31:g)<<5)|((b>31?31:b)<<10)); }
// the tint for a moment of the day (phase 0..KS_DAYT-1): dawn warm, noon plain, dusk orange, night blue and dim. Writes r, g, b sixteenths.
static inline void kvDayTint(unsigned phase,int*tr,int*tg,int*tb){
    static const signed char key[5][3]={ {14,13,12}, {16,16,16}, {16,16,16}, {17,11,8}, {8,9,14} };   // dawn, morning, afternoon, dusk, night (each a fifth of the day), blended to the next
    unsigned seg=phase*5u/KS_DAYT, in=(phase*5u%KS_DAYT)*16u/KS_DAYT; int a=(int)seg%5, b=(a+1)%5;
    *tr=(key[a][0]*(16-(int)in)+key[b][0]*(int)in)>>4; *tg=(key[a][1]*(16-(int)in)+key[b][1]*(int)in)>>4; *tb=(key[a][2]*(16-(int)in)+key[b][2]*(int)in)>>4;
}
