// kinggiant.h - EP6 THE LAND BOREFORE TIME, Phase 4 step g: THE WALKING GIANT. PURE C (needs kingdom.h, kingsim.h, kingview.h and kinghand.h before it; no hardware).
// The giant is a point on the map in pixels (its feet) that the D-pad moves, `stride` steps a frame (kinghand.h: khStride, and khRun while B is held; a step is two pixels now that a cell is 16). The camera follows it
// (kgCamera), the cell under its feet is the cell A acts on, and it is drawn as ONE hardware sprite (32 x 64, 4 bits a pixel, nine frames: stand, step, step facing down, right, up) over the map.
// WHERE IT WALKS: everywhere inside the land except deep water (and a 1 cell border); it walks over buildings (it is a giant), and a step into water is simply refused (it slides along the shore).
// NOTE (step 4c): main.c #defines W, D and H. No single capital letters as names here.
#ifndef KINGGIANT_H
#define KINGGIANT_H
typedef struct { int x, y; int face; int step; int moving; int dir; } KGiant;   // x, y: the feet in world pixels; face: 0 looking right, 1 left; step: counts the frames walked; dir: 0 down, 1 sideways, 2 up (how it last walked)
static inline int kgcWalk(const KSim*S,int px,int py){   // may the feet stand on pixel (px,py)?
    int d=S->dim; if(px<KV_CELL||py<KV_CELL||px>=d*KV_CELL-KV_CELL||py>=d*KV_CELL-KV_CELL) return 0;
    int c=S->cells[(py/KV_CELL)*d+(px/KV_CELL)]; return (c&KC_BLD)||(c&7)!=KT_WATER;
}
static inline int kgCx(const KGiant*G){ return G->x/KV_CELL; }   // the cell A acts on
static inline int kgCy(const KGiant*G){ return G->y/KV_CELL; }
// put the giant on the cell nearest the middle of the land that it may stand on (rings outward); returns 1 if it found one, 0 (and stands in the middle) if the whole land is water
static inline int kgSpawn(KGiant*G,const KSim*S){
    int d=S->dim, c=d/2; G->face=0; G->step=0; G->moving=0; G->dir=0; G->x=c*KV_CELL+KV_CELL/2; G->y=c*KV_CELL+KV_CELL/2;
    for(int r=0;r<d/2;r++) for(int y=c-r;y<=c+r;y++) for(int x=c-r;x<=c+r;x++){
        if(r&&x>c-r&&x<c+r&&y>c-r&&y<c+r) continue;   // (the ring only)
        if(kgcWalk(S,x*KV_CELL+KV_CELL/2,y*KV_CELL+KV_CELL/2)){ G->x=x*KV_CELL+KV_CELL/2; G->y=y*KV_CELL+KV_CELL/2; return 1; } }
    return 0;
}
// put the giant back on the cell (cx,cy) a save left it on (step 4j): 1 if it stands there; 0 if it cannot (no saved position, off the land, the border or water), and it then starts as kgSpawn starts it
static inline int kgPlace(KGiant*G,const KSim*S,int cx,int cy){
    int d=S->dim; if(cx<0||cy<0||cx>=d||cy>=d||!kgcWalk(S,cx*KV_CELL+KV_CELL/2,cy*KV_CELL+KV_CELL/2)){ kgSpawn(G,S); return 0; }
    G->face=0; G->step=0; G->moving=0; G->dir=0; G->x=cx*KV_CELL+KV_CELL/2; G->y=cy*KV_CELL+KV_CELL/2; return 1;
}
// one frame of walking: (dx,dy) are -1, 0 or 1 from the D-pad, sp steps (under 8) from khStride or khRun, each worth two pixels. Each axis on its own, so the giant slides along water and the edge. Returns 1 if it moved.
static inline int kgMove(KGiant*G,const KSim*S,int dx,int dy,int sp){
    int lim=S->dim*KV_CELL, moved=0; G->moving=0;
    if(sp>7) sp=7; sp*=KV_CELL/8;
    if(dx){ G->face=dx<0; G->dir=1; int nx=G->x+dx*sp; if(nx<KV_CELL) nx=KV_CELL; if(nx>lim-KV_CELL-1) nx=lim-KV_CELL-1; if(nx!=G->x&&kgcWalk(S,nx,G->y)){ G->x=nx; moved=1; } }
    if(dy){ if(!dx) G->dir=dy>0?0:2; int ny=G->y+dy*sp; if(ny<KV_CELL) ny=KV_CELL; if(ny>lim-KV_CELL-1) ny=lim-KV_CELL-1; if(ny!=G->y&&kgcWalk(S,G->x,ny)){ G->y=ny; moved=1; } }
    if(moved){ G->moving=1; G->step++; }
    return moved;
}
// the camera on the giant (then clamped to the land, so near an edge the giant leaves the middle of the screen). The giant is tall: the camera sits a little above its feet.
static inline void kgCamera(KView*V,const KGiant*G){ V->x=G->x-120; V->y=G->y-92; kvClamp(V); }
// ---- the sprite: 32 x 64 (the art is in kingart.h: kaGiants[giant], nine frames of 32 tiles, the feet on row 60). Frames 0-2 face down, 3-5 right (flipped for left), 6-8 up; each is stand, step, step. ----
#define KG_FRAMES 9
#define KG_FTILES 32        // tiles in a frame
static inline int kgFrame(const KGiant*G){   // 0..8
    int walk=G->moving?((int[4]){0,1,0,2})[(G->step>>2)&3]:0; int v=G->dir==1?3:G->dir==2?6:0; return v+walk; }
// the sprite's three OAM words for this camera: the feet are at the bottom middle of the 32 x 64 (row 60). Off the screen the sprite is switched off (attribute 0 = 0x200).
// Priority 1: above the map (BG0 is priority 1 and sprites win a tie), below the text layer (BG1, priority 0).
static inline void kgOam(const KGiant*G,const KView*V,unsigned*a0,unsigned*a1,unsigned*a2){
    int sx=G->x-V->x, sy=G->y-V->y, x=sx-16, y=sy-60;
    if(x<=-32||x>=240||y<=-64||y>=160){ *a0=0x200; *a1=0; *a2=0; return; }
    *a0=(unsigned)(y&255)|0x8000u; *a1=(unsigned)(x&511)|(G->face&&G->dir==1?0x1000u:0u)|0xC000u; *a2=(unsigned)(kgFrame(G)*KG_FTILES)|(1u<<10);   // (tall shape, size 3: 32 x 64; palette bank 0)
}
#endif
