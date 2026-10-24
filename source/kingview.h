// kingview.h - EP6 THE LAND BOREFORE TIME, Phase 4 step b: the map as the GBA's hardware tile background (Plan B, roadmap 5.3). PURE C: the only hardware is the macro KV_PUT.
// WHAT IT DOES: one cell of the realm is one 8 x 8 tile. A 32 x 32 tile screenblock (256 x 256 pixels) is a ring: world tile (x,y) lives at (x&31, y&31), and the hardware scroll
// registers (camera x&255, y&255) show the right part of it. The screen needs 31 x 21 tiles; when the camera crosses a tile edge only the newly exposed column / row is written
// (at most 21 + 31 tiles a frame at 8 pixels a frame), so scrolling costs about 3,000 cycles (roadmap 5.3), not a redraw.
// TILES: 16 of them, made here in code (placeholder art, Phase 6 makes the real art): 0 grass 1 forest 2 rock 3 water 4 crystal 5 meadow 6 road 7 deep water (off the map)
//        8..11 a building's four quarters (top left, top right, bottom left, bottom right)  12..15 the same quarters as a building SITE (scaffolding, lvl 0).
// PALETTES: bank 0 is the land; banks 1..7 are the same colours with the roof recoloured for building type 0..6 (hut, house, woodcutter, quarry, sawmill, mason, storehouse).
// WHAT KEEPS IT TRUE: kvSync (after the camera moved) writes what scrolled into view; kvRefreshRow (every frame) rewrites one row of the window, cycling, so a felled tree, a new road
// or a finished building shows within 21 frames; kvPoke(x,y) rewrites one tile at once (for edits the player makes). Needs kingdom.h (KT_*, KC_BLD, KBld, kgDim), kingsim.h (KS_NTYPES).
// KV_PUT(sx,sy,entry) writes screenblock entry (sx,sy), both 0..31: the game defines it as a 16 bit VRAM write, the PC test as an array write.
#define KV_W 31            // tiles across the screen window (240 pixels + one for the scroll offset)
#define KV_H 21            // tiles down
#define KV_TILES 16
#define KV_DEEP 7
typedef struct { int x, y; int tx0, ty0; int ok, rr; int dim; const u8*cells; const KBld*B; int nB; } KView;   // x, y: the camera, in pixels, top left of the screen
static inline void kvInit(KView*V,const u8*cells,int dim,const KBld*B,int nB){ V->cells=cells; V->dim=dim; V->B=B; V->nB=nB; V->x=V->y=0; V->tx0=V->ty0=0; V->ok=0; V->rr=0; }
static inline int kvMaxX(const KView*V){ return V->dim*8-240; }
static inline int kvMaxY(const KView*V){ return V->dim*8-160; }
static inline void kvClamp(KView*V){ int mx=kvMaxX(V), my=kvMaxY(V); if(V->x>mx) V->x=mx; if(V->y>my) V->y=my; if(V->x<0) V->x=0; if(V->y<0) V->y=0; }   // (the smallest land, 48 x 8 = 384 pixels, is bigger than the screen)
static inline void kvCenter(KView*V,int cx,int cy){ V->x=cx*8+4-120; V->y=cy*8+4-80; kvClamp(V); }   // the camera on a cell
// the screenblock entry for world cell (x,y): tile | palette bank << 12
static inline u16 kvEntry(const KView*V,int x,int y){
    if(x<0||y<0||x>=V->dim||y>=V->dim) return KV_DEEP;
    int v=V->cells[y*V->dim+x];
    if(v&KC_BLD){
        for(int i=0;i<V->nB;i++){ const KBld*b=&V->B[i]; int w=kgFootW(b->type), h=kgFootH(b->type);
            if(x>=b->x&&x<b->x+w&&y>=b->y&&y<b->y+h){ int q=(y-b->y>0?2:0)+(x-b->x>0?1:0); return (u16)(b->lvl?(8+q)|((1+(b->type<7?b->type:6))<<12):12+q); } }
        return 3;   // (bit 7 with no building under it: a damaged map; water is the least harmful thing to show)
    }
    v&=7; return (u16)(v<KT_N?v:0);
}
static inline void kvPoke(const KView*V,int x,int y){ if(!V->ok) return; if(x<V->tx0||y<V->ty0||x>=V->tx0+KV_W||y>=V->ty0+KV_H) return; KV_PUT(x&31,y&31,kvEntry(V,x,y)); }
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
// ---- the art: 4 bits a pixel, 8 pixels a row, low nibble first (the GBA's order) ----
#define KV_RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
static const u16 kvPal[16]={ KV_RGB(2,3,8), KV_RGB(6,17,5), KV_RGB(9,22,7), KV_RGB(3,11,4), KV_RGB(5,16,6), KV_RGB(14,9,4), KV_RGB(12,12,13), KV_RGB(19,19,20), KV_RGB(4,10,24), KV_RGB(8,16,28), KV_RGB(12,28,30), KV_RGB(17,24,8), KV_RGB(23,19,11), KV_RGB(17,13,7), KV_RGB(26,8,6), KV_RGB(29,27,22) };
static const u16 kvRoof[7]={ KV_RGB(20,12,6), KV_RGB(26,8,6), KV_RGB(6,20,6), KV_RGB(14,14,16), KV_RGB(28,16,4), KV_RGB(6,12,26), KV_RGB(18,8,24) };   // roof colour by building type
static inline u16 kvPalAt(int bank,int i){ return (bank>=1&&bank<=7&&i==14)?kvRoof[bank-1]:kvPal[i]; }
static inline void kvTileGfx(int t,u8*o32){   // fills 32 bytes
    u8 px[64]; for(int i=0;i<64;i++){ unsigned h=(unsigned)(i*2654435761u+(unsigned)t*40503u); h^=h>>13; unsigned r=h&7; int x=i&7, y=i>>3; int c=0;
        switch(t){
        case 0: c=(r<5)?1:2; break;                                         // grass
        case 1: c=((x+y)&3)==0?5:(r<3?3:4); break;                          // forest
        case 2: c=(r<2)?7:6; if(y>5&&x>2) c=6; break;                       // rock
        case 3: c=((x+2*y)%8<2)?9:8; break;                                 // water
        case 4: c=((x==y||x+y==7)&&x>1&&x<6)?10:6; if(x>2&&x<5&&y>2&&y<5) c=10; break;   // crystal
        case 5: c=(r==0)?15:(r<2?11:2); break;                              // meadow
        case 6: c=(r<2)?13:12; break;                                       // road
        case 7: c=0; if(r==0) c=3; break;                                   // deep water, off the map
        case 8: case 9: { int e=(t==8)?x:7-x; c=(y<2&&e<y+2)?0:(y<7?14:13); if(t==8&&x==0) c=0; if(t==9&&x==7) c=0; if(y==0) c=13; break; }   // roof
        case 10: case 11: c=(y==7)?13:15; if(y>=2&&y<=4&&x>=2&&x<=4&&t==10) c=8; if(t==11&&x>=3&&x<=4&&y>=3) c=13; if((t==10&&x==0)||(t==11&&x==7)) c=13; break;   // walls, a window and a door
        default: { int q=t-12; c=(((x+y)&3)==0||x==0||y==0||x==7||y==7)?5:0; if(q==0&&(x==0||y==0)) c=5; if(r==0) c=12; break; }   // a site: bare ground, a frame of timbers
        }
        px[i]=(u8)c; }
    for(int i=0;i<32;i++) o32[i]=(u8)(px[2*i]|(px[2*i+1]<<4));
}
