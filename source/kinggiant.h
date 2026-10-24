// kinggiant.h - EP6 THE LAND BOREFORE TIME, Phase 4 step g: THE WALKING GIANT. PURE C (needs kingdom.h, kingsim.h, kingview.h and kinghand.h before it; no hardware).
// The giant is a point on the map in pixels (its feet) that the D-pad moves, `stride` pixels a frame (kinghand.h: khStride, and khRun while B is held). The camera follows it
// (kgCamera), the cell under its feet is the cell A acts on, and it is drawn as ONE hardware sprite (16 x 16, 4 bits a pixel, 2 walk frames) over the map.
// WHERE IT WALKS: everywhere inside the land except deep water (and a 1 cell border); it walks over buildings (it is a giant), and a step into water is simply refused (it slides along the shore).
// NOTE (step 4c): main.c #defines W, D and H. No single capital letters as names here.
#ifndef KINGGIANT_H
#define KINGGIANT_H
typedef struct { int x, y; int face; int step; int moving; } KGiant;   // x, y: the feet in world pixels; face: 0 looking right, 1 left; step: counts the frames walked
static inline int kgcWalk(const KSim*S,int px,int py){   // may the feet stand on pixel (px,py)?
    int d=S->dim; if(px<8||py<8||px>=d*8-8||py>=d*8-8) return 0;
    int c=S->cells[(py>>3)*d+(px>>3)]; return (c&KC_BLD)||(c&7)!=KT_WATER;
}
static inline int kgCx(const KGiant*G){ return G->x>>3; }   // the cell A acts on
static inline int kgCy(const KGiant*G){ return G->y>>3; }
// put the giant on the cell nearest the middle of the land that it may stand on (rings outward); returns 1 if it found one, 0 (and stands in the middle) if the whole land is water
static inline int kgSpawn(KGiant*G,const KSim*S){
    int d=S->dim, c=d/2; G->face=0; G->step=0; G->moving=0; G->x=c*8+4; G->y=c*8+4;
    for(int r=0;r<d/2;r++) for(int y=c-r;y<=c+r;y++) for(int x=c-r;x<=c+r;x++){
        if(r&&x>c-r&&x<c+r&&y>c-r&&y<c+r) continue;   // (the ring only)
        if(kgcWalk(S,x*8+4,y*8+4)){ G->x=x*8+4; G->y=y*8+4; return 1; } }
    return 0;
}
// put the giant back on the cell (cx,cy) a save left it on (step 4j): 1 if it stands there; 0 if it cannot (no saved position, off the land, the border or water), and it then starts as kgSpawn starts it
static inline int kgPlace(KGiant*G,const KSim*S,int cx,int cy){
    int d=S->dim; if(cx<0||cy<0||cx>=d||cy>=d||!kgcWalk(S,cx*8+4,cy*8+4)){ kgSpawn(G,S); return 0; }
    G->face=0; G->step=0; G->moving=0; G->x=cx*8+4; G->y=cy*8+4; return 1;
}
// one frame of walking: (dx,dy) are -1, 0 or 1 from the D-pad, sp pixels (under 8) from khStride or khRun. Each axis on its own, so the giant slides along water and the edge. Returns 1 if it moved.
static inline int kgMove(KGiant*G,const KSim*S,int dx,int dy,int sp){
    int lim=S->dim*8, moved=0; G->moving=0;
    if(sp>7) sp=7;
    if(dx){ G->face=dx<0; int nx=G->x+dx*sp; if(nx<8) nx=8; if(nx>lim-9) nx=lim-9; if(nx!=G->x&&kgcWalk(S,nx,G->y)){ G->x=nx; moved=1; } }
    if(dy){ int ny=G->y+dy*sp; if(ny<8) ny=8; if(ny>lim-9) ny=lim-9; if(ny!=G->y&&kgcWalk(S,G->x,ny)){ G->y=ny; moved=1; } }
    if(moved){ G->moving=1; G->step++; }
    return moved;
}
// the camera on the giant (then clamped to the land, so near an edge the giant leaves the middle of the screen)
static inline void kgCamera(KView*V,const KGiant*G){ V->x=G->x-120; V->y=G->y-80; kvClamp(V); }
// ---- the sprite: 16 x 16, tiles in the 1D order (top left, top right, bottom left, bottom right); frame 0 stands, frame 1 steps. Placeholder art (Phase 6). ----
// colours of the 16 colour palette: 0 see-through, 1 outline, 2 skin, 3 shirt, 4 hair, 5 (spare), 6 trousers
#define KG_TILES 8   // 2 frames of 4 tiles
static inline void kgGfx(int frame,u8*o128){   // one frame: 4 tiles of 32 bytes (128 bytes)
    u8 p[256]; for(int i=0;i<256;i++) p[i]=0;
    #define KGP(x,y,c) do{ if((x)>=0&&(x)<16&&(y)>=0&&(y)<16) p[(y)*16+(x)]=(u8)(c); }while(0)
    for(int y=8;y<=11;y++) for(int x=4;x<=11;x++) KGP(x,y,3);                    // the shirt
    for(int y=8;y<=10;y++){ KGP(3,y,2); KGP(12,y,2); }                            // the arms
    if(!frame){ for(int y=12;y<=15;y++){ KGP(5,y,6); KGP(6,y,6); KGP(9,y,6); KGP(10,y,6); } }
    else { for(int y=12;y<=15;y++){ KGP(4,y,6); KGP(5,y,6); } for(int y=12;y<=14;y++){ KGP(10,y,6); KGP(11,y,6); } }   // a stride: one leg forward, one lifted
    for(int y=0;y<=7;y++) for(int x=4;x<=11;x++){ int dx=2*x-15, dy=2*y-7; if(dx*dx+dy*dy<=40) KGP(x,y,y<=1?4:2); }   // the head, hair on top
    KGP(7,3,1); KGP(9,3,1);                                                       // the eyes
    #undef KGP
    u8 q[256]; for(int i=0;i<256;i++) q[i]=p[i];                                    // the outline: any empty pixel beside a filled one
    for(int y=0;y<16;y++) for(int x=0;x<16;x++) if(!p[y*16+x]){ int n=(x>0&&p[y*16+x-1])||(x<15&&p[y*16+x+1])||(y>0&&p[(y-1)*16+x])||(y<15&&p[(y+1)*16+x]); if(n) q[y*16+x]=1; }
    for(int i=0;i<128;i++) o128[i]=0;
    for(int y=0;y<16;y++) for(int x=0;x<16;x++){ int t=(y>>3)*2+(x>>3); int off=t*32+(y&7)*4+((x&7)>>1); o128[off]=(u8)(o128[off]|(q[y*16+x]<<(((x&7)&1)*4))); }
}
static inline void kgGfxAll(u8*o256){ kgGfx(0,o256); kgGfx(1,o256+128); }   // both frames, 256 bytes: tiles 0..3 stand, 4..7 step
// the giant's colours by the giant (the four, then your own Bore): shirt and hair
static const u16 kgShirt[5]={ KV_RGB(27,6,5), KV_RGB(5,22,8), KV_RGB(28,17,3), KV_RGB(17,8,24), KV_RGB(6,22,28) };   // red, green, orange, purple, cyan
static const u16 kgHair[5]={ KV_RGB(7,4,2), KV_RGB(27,24,6), KV_RGB(20,20,20), KV_RGB(2,2,3), KV_RGB(12,8,3) };
static inline void kgPal(int giant,u16*o16){
    int g=giant>=0&&giant<5?giant:0; for(int i=0;i<16;i++) o16[i]=0;
    o16[1]=(u16)(2|(2<<5)|(4<<10)); o16[2]=(u16)(28|(22<<5)|(17<<10)); o16[3]=kgShirt[g]; o16[4]=kgHair[g]; o16[5]=0x7FFF; o16[6]=(u16)(8|(10<<5)|(22<<10));
}
// the sprite's three OAM words for this camera: the feet are at the bottom middle of the 16 x 16. Off the screen the sprite is switched off (attribute 0 = 0x200).
// Priority 1: above the map (BG0 is priority 1 and sprites win a tie), below the text layer (BG1, priority 0).
static inline void kgOam(const KGiant*G,const KView*V,unsigned*a0,unsigned*a1,unsigned*a2){
    int sx=G->x-V->x, sy=G->y-V->y, x=sx-8, y=sy-15, fr=G->moving?((G->step>>3)&1):0;
    if(x<=-16||x>=240||y<=-16||y>=160){ *a0=0x200; *a1=0; *a2=0; return; }
    *a0=(unsigned)(y&255); *a1=(unsigned)(x&511)|(G->face?0x1000u:0u)|0x4000u; *a2=(unsigned)(fr*4)|(1u<<10);
}
#endif
