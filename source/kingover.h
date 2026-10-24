// kingover.h - EP6 THE LAND BOREFORE TIME, Phase 4 step i: THE OVERVIEW. PURE C (needs kingdom.h, kingsim.h; kinggiant.h's KGiant is NOT needed: the giant is passed as cell x, y).
// The whole realm on one screen, one cell a pixel (the 48 land: 3 x 3 pixels a cell), the giant a blinking white dot, buildings in their roof colour, sites dimmer.
// koDraw writes into a 240-wide 15 bit bitmap (the game's fb); the caller draws the title and shows it. NOTE (step 4c): no single capital letters as names here.
#ifndef KINGOVER_H
#define KINGOVER_H
#define KO_RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
static const u16 koLand[8]={ KO_RGB(9,20,6), KO_RGB(3,12,4), KO_RGB(16,15,14), KO_RGB(3,9,22), KO_RGB(22,26,31), KO_RGB(15,23,8), KO_RGB(22,17,9), KO_RGB(27,21,11) };   // grass forest rock water crystal meadow road
static const u16 koRoof[KS_NTYPES]={ KO_RGB(28,8,6), KO_RGB(30,14,8), KO_RGB(18,10,3), KO_RGB(12,12,14), KO_RGB(24,16,4), KO_RGB(22,6,22), KO_RGB(31,28,6), KO_RGB(31,30,25), KO_RGB(4,28,26), KO_RGB(31,15,23), KO_RGB(16,26,31), KO_RGB(31,24,2) };
static inline int koScale(int dim){ int s=160/dim; return s<1?1:s>3?3:s; }
static inline int koW(int dim){ return dim*koScale(dim); }
static inline int koX0(int dim){ return (240-koW(dim))/2; }
static inline int koY0(int dim){ return (160-koW(dim))/2; }
static inline u16 koCell(const KSim*S,int x,int y){
    int v=S->cells[y*S->dim+x]; if(v&KC_BLD){ for(int i=0;i<S->R->nB;i++){ const KBld*b=&S->B[i]; if(x>=b->x&&x<b->x+kgFootW(b->type)&&y>=b->y&&y<b->y+kgFootH(b->type)){ u16 c=koRoof[b->type<KS_NTYPES?b->type:0]; if(!b->lvl) c=(u16)(((c>>1)&0x3DEF)|0x0421); return c; } } }
    v&=7; return koLand[v<8?v:0];
}
// the realm into fb (row stride 240); the giant's cell is drawn white when blink is 1
static inline void koDraw(u16*fb,const KSim*S,int gx,int gy,int blink){
    int d=S->dim, sc=koScale(d), x0=koX0(d), y0=koY0(d);
    for(int y=0;y<d;y++) for(int x=0;x<d;x++){ u16 c=koCell(S,x,y); if(blink&&x==gx&&y==gy) c=0x7FFF;
        for(int j=0;j<sc;j++) for(int k=0;k<sc;k++) fb[(y0+y*sc+j)*240+x0+x*sc+k]=c; }
}
#endif
