// ramps.h - kicker ramps and quarter pipes: which tile chars they are and how high the surface is at any point inside the tile.
// Kicker = '1'..'4', quarter pipe = '5'..'8'; the digit is the side the ramp FACES (its low entry side): 0 S(+y) 1 E(+x) 2 N(-y) 3 W(-x),
// the same numbering as itemFacing(). The lip is on the opposite side. Include BEFORE tileH().
// RAMP TUNING (see lifeStep): F_RAMP_BOOST turns the climb speed (8.8 px/step) into launch speed, F_RAMP_MAX caps it.
#include "rampdata.h"
#define F_RAMP_BOOST 4      // launch vz = (smoothed px climbed per step, 8.8) * this
#define F_RAMP_MAX   0x2A0  // never launch harder than this: ~14 px above the lip (an ollie is ~0x388). Keep lip + this under 34 px or landings scream (launch ramp: 18 + 14 = 32). It was 0x300 with 8 and 12 px lips
#define F_RAMP_LONG 3       // launch boost of a LONG RAMP in halves: its slope is 8/12 (kicker) or 12/18 (launch) of the single ramp, so 3/2 gives the same air per tile of speed
#define F_RAMP_TOL   9      // px a ramp surface may rise between two steps before it counts as a wall (flat ground uses 3). The quarter pipe's last eighth of a tile climbs 7 px, so this was 6 with the old 14 px pipe
static inline int isKicker(char c){ return c>='1'&&c<='4'; }
static inline int isQPipe(char c){ return c>='5'&&c<='8'; }
static inline int isLaunch(char c){ return c>='9'&&c<='<'; }   // launch ramp (pack 2): '9' ':' ';' '<' = faces S E N W
static inline int isRamp(char c){ return (c>='1'&&c<='8')||isLaunch(c); }
// LONG RAMPS: kickers (or launch ramps) placed in a row, all facing the same way, join into one longer ramp. The tile at the low end is chain index 0,
// the next one 1 ... A tile is part of a chain when a tile of the same kind and the same way lies on its low side (index > 0) or on its lip side (up).
// Each tile of a chain climbs KICKER_SEG / LAUNCH_SEG px (a gentler slope than the single ramp), and after KICKER_SEGS / LAUNCH_SEGS tiles the next one is a flat deck.
// The sprites are V_KSEG / V_LSEG (items.h); the heights come from rampdata.h so the surface always matches the art.
static int rampChain(int tx,int ty,char c,int*up){   // returns the chain index (0 = the low end, capped one past the last climbing tile); *up = a tile of the same ramp lies on the lip side
    static const signed char dx[4]={0,1,0,-1}, dy[4]={1,0,-1,0};
    int d=isKicker(c)?c-'1':c-'9', n=isKicker(c)?KICKER_SEGS:LAUNCH_SEGS, i=0, x=tx+dx[d], y=ty+dy[d];
    while(i<n&&x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]==c){ i++; x+=dx[d]; y+=dy[d]; }
    x=tx-dx[d]; y=ty-dy[d]; *up=(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]==c);
    return i;
}
static int rampTop(int tx,int ty,char c){   // the highest point of a ramp tile (tileH)
    int up, i; if(isQPipe(c)) return qpH[7];
    i=rampChain(tx,ty,c,&up);
    if(!i&&!up) return isKicker(c)?KICKER_H:LAUNCH_H;
    return isKicker(c)?KICKER_SEG*(i<KICKER_SEGS?i+1:KICKER_SEGS):LAUNCH_SEG*(i<LAUNCH_SEGS?i+1:LAUNCH_SEGS);
}
static int rampH(char c,int fx,int fy){   // surface height (px) at a position inside the tile (fx,fy in 1/256 tile)
    int d=isKicker(c)?c-'1':isQPipe(c)?c-'5':c-'9', t;      // t 0..255 = how far from the low (entry) edge towards the lip
    switch(d){ case 0: t=255-(fy&255); break; case 1: t=255-(fx&255); break; case 2: t=fy&255; break; default: t=fx&255; }
    if(isQPipe(c)) return qpH[t>>5];
    { int up, i=rampChain(fx>>8,fy>>8,c,&up);
      if(i||up){ int sg=isKicker(c)?KICKER_SEG:LAUNCH_SEG, n=isKicker(c)?KICKER_SEGS:LAUNCH_SEGS;
          return i>=n?sg*n:sg*i+((t*(sg+1))>>8); } }
    return isKicker(c)?(t*(KICKER_H+1))>>8:(t*(LAUNCH_H+1))>>8;
}
