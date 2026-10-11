// ramps.h - kicker ramps and quarter pipes: which tile chars they are and how high the surface is at any point inside the tile.
// Kicker = '1'..'4', quarter pipe = '5'..'8'; the digit is the side the ramp FACES (its low entry side): 0 S(+y) 1 E(+x) 2 N(-y) 3 W(-x),
// the same numbering as itemFacing(). The lip is on the opposite side. Include BEFORE tileH().
// RAMP TUNING (see lifeStep): F_RAMP_BOOST turns the climb speed (8.8 px/step) into launch speed, F_RAMP_MAX caps it.
#include "rampdata.h"
#define F_RAMP_TOL   9      // px a ramp surface may rise between two steps before it counts as a wall (flat ground uses 3). The quarter pipe's last eighth of a tile climbs 7 px, so this was 6 with the old 14 px pipe
// RAMP PHYSICS (skating 2): the board keeps its ENERGY on a slope. Climbing costs speed and rolling down gives it back (speed^2 - F_SLOPE x the
// height gained, in 8.8 px), so a quarter pipe stops you near the top if you come in slow and rolls you back down, and a run down a ramp is fast.
// Leaving a kicker or launch ramp over its lip throws you up: vz = speed at the lip x the lip's slope (px per tile) x F_LIP_K / 1024, so faster
// = higher and steeper = higher. Up a quarter pipe's lip you go VERT: straight up (the push along the pipe stops), and you come down onto it
// facing back down, at the speed you went up with. An ollie on a ramp adds the board's climb to the pop.
#define F_SLOPE   11        // speed^2 a 1/256 px climb costs (24 px of quarter pipe from top speed 400 leaves about 300)
#define F_SPD_MAX 620       // the fastest a run down a slope can make you (top push speed is 400)
#define F_LIP_K   200       // the kick off a kicker / launch ramp lip: vz (8.8) = speed x lip slope x this / 1024 (a 12 px kicker at top speed: about 21 px above the lip; a launch ramp about 40)
#define F_QP_LIFT 210       // vert off a quarter pipe: vz (8.8) = speed x this / 64 (about 30 px above the lip at top speed)
#define F_LIP_DROP 512      // 8.8 px: a surface that falls away more than this under the board leaves you in the air (less: you roll down it)
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
// ---- skating 2: exact (8.8 px) ramp surfaces and which way a ramp climbs ----
static int rampDir(char c){ return isKicker(c)?c-'1':isQPipe(c)?c-'5':c-'9'; }   // the side it FACES (0 S 1 E 2 N 3 W)
static void rampUp(char c,int*ux,int*uy){ int d=rampDir(c); *ux=d==1?-1:d==3?1:0; *uy=d==0?-1:d==2?1:0; }   // the way UP the ramp (towards the lip)
static int rampT(char c,int fx,int fy){ int d=rampDir(c); return d==0?255-(fy&255):d==1?255-(fx&255):d==2?(fy&255):(fx&255); }   // 0 at the entry edge .. 255 at the lip
static int qpH8(int t){   // the quarter pipe's curve, 8.8 px: smooth through the middle of each eighth (qpH), flat for the last half eighth at the lip
    if(t<16) return (qpH[0]<<8)*t/16;
    if(t>=240) return qpH[7]<<8;
    int i=(t-16)>>5, f=(t-16)&31; return ((qpH[i]<<8)*(32-f)+(qpH[i+1]<<8)*f)>>5;
}
static int rampH8(char c,int fx,int fy){   // like rampH, in 8.8 px (no steps: the board rides it smoothly)
    int t=rampT(c,fx,fy);
    if(isQPipe(c)) return qpH8(t);
    { int up, i=rampChain(fx>>8,fy>>8,c,&up);
      if(i||up){ int sg=isKicker(c)?KICKER_SEG:LAUNCH_SEG, n=isKicker(c)?KICKER_SEGS:LAUNCH_SEGS;
          return i>=n?(sg*n)<<8:((sg*i)<<8)+t*sg; } }
    return t*(isKicker(c)?KICKER_H:LAUNCH_H);
}
static int rampLipSlope(char c,int tx,int ty){   // px a ramp tile climbs across its width (the slope you leave it with)
    int up, i; if(isQPipe(c)) return 24;
    i=rampChain(tx,ty,c,&up);
    if(i||up) return isKicker(c)?KICKER_SEG:LAUNCH_SEG;
    return isKicker(c)?KICKER_H:LAUNCH_H;
}
static int isqrtI(int v){ if(v<=0) return 0; int r=v, x=(v>>1)+1; while(x<r){ r=x; x=(x+v/x)>>1; } return r; }
