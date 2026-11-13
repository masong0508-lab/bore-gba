// touchfilter.h - the touch screen, done carefully. Pure integer code (no hardware access) so the same functions run on the ARM7 and in the PC test (pc_touchcal.c).
//  * calibration: the DS stores two calibration points in the firmware's user settings (ADC value -> pixel); with them the raw reading lands on the pixel that was touched,
//    including near the edges, instead of a straight 0..4095 -> 0..255 guess
//  * sampling: several conversions per frame; the extremes are thrown away, the rest averaged, and a frame whose samples disagree (pen moving fast / bouncing) is not trusted
//  * debounce: the first trusted frame of a press is the contact bounce and is never reported; a release needs two empty frames in a row, so one bad frame in the middle of a drag
//    cannot make a false release and a false new tap
#ifndef TOUCHFILTER_H
#define TOUCHFILTER_H
typedef struct { int ok, ax1, ay1, sx1, sy1, ax2, ay2, sx2, sy2; } TouchCal;
typedef struct { int run, gap, down, lx, ly; } TouchState;      // trusted frames in a row, empty frames in a row, reporting?, last reported position
// firmware user settings -> calibration (settings points at the 0x100 byte block; fields as in GBATEK: 058h adc x1,y1 u16; 05Ch scr x1,y1 u8; 05Eh adc x2,y2 u16; 062h scr x2,y2 u8)
static inline void tcLoad(TouchCal*c,const unsigned char*b){
    c->ax1=b[0x58]|(b[0x59]<<8); c->ay1=b[0x5A]|(b[0x5B]<<8); c->sx1=b[0x5C]; c->sy1=b[0x5D]; c->ax2=b[0x5E]|(b[0x5F]<<8); c->ay2=b[0x60]|(b[0x61]<<8); c->sx2=b[0x62]; c->sy2=b[0x63];
    c->ok=c->ax2>c->ax1+200&&c->ay2>c->ay1+200&&c->sx2>c->sx1+100&&c->sy2>c->sy1+80&&c->ax1<4096&&c->ax2<4096&&c->ay1<4096&&c->ay2<4096&&c->sx2<=255&&c->sy2<=191; }
static inline int tcClamp(int v,int lo,int hi){ return v<lo?lo:v>hi?hi:v; }
static inline void tcMap(const TouchCal*c,int rx,int ry,int*px,int*py){
    int x,y; if(c->ok){ x=(rx-c->ax1)*(c->sx2-c->sx1)/(c->ax2-c->ax1)+c->sx1-1; y=(ry-c->ay1)*(c->sy2-c->sy1)/(c->ay2-c->ay1)+c->sy1-1; } else { x=rx>>4; y=ry*192/4096; }
    *px=tcClamp(x,0,255); *py=tcClamp(y,0,191); }
// n samples of one axis: drop the lowest and the highest, average the rest; returns the spread of what is left
static inline int tcAxis(const int*v,int n,int*avg){ int a[16]; for(int i=0;i<n;i++) a[i]=v[i]; for(int i=1;i<n;i++){ int t=a[i], j=i-1; while(j>=0&&a[j]>t){ a[j+1]=a[j]; j--; } a[j+1]=t; }
    int lo=n>=5?1:0, hi=n>=5?n-1:n, s=0; for(int i=lo;i<hi;i++) s+=a[i]; *avg=s/(hi-lo); return a[hi-1]-a[lo]; }
// one frame: phys = the pen-down line of the hardware; xs/ys the raw conversions. Returns 1 while a touch is reported (px,py then hold the pixel position)
static inline int tcFrame(TouchState*s,const TouchCal*c,int phys,const int*xs,const int*ys,int n,int*px,int*py){
    int trusted=0, ax=0, ay=0;
    if(phys&&n>=3){ int sx=tcAxis(xs,n,&ax), sy=tcAxis(ys,n,&ay); trusted=ax>0&&ay>0&&ax<4095&&ay<4095&&sx<=48&&sy<=48; }
    if(trusted){ s->run++; s->gap=0; } else { s->run=0; if(s->down&&++s->gap>=2) s->down=0; else if(!s->down) s->gap=0; }
    if(trusted&&s->run>=2) s->down=1;                      // the first trusted frame is the bounce: report from the second one on
    if(s->down&&trusted){ tcMap(c,ax,ay,px,py); s->lx=*px; s->ly=*py; }
    else if(s->down){ *px=s->lx; *py=s->ly; }              // one empty frame inside a touch: keep the last position
    return s->down; }
// the main CPU's last step: while the pen is down a small move is jitter (kept), a bigger one is followed with a little smoothing
static inline void tcSmooth(int down,int*sx,int*sy,int*have,int px,int py){
    if(!down){ *have=0; return; } if(!*have){ *sx=px; *sy=py; *have=1; return; }
    int dx=px-*sx, dy=py-*sy; if(dx*dx+dy*dy<=2) return;           // within 1 pixel: the same point
    *sx+= (dx*3+(dx>0?2:-2))/4; *sy+= (dy*3+(dy>0?2:-2))/4; }
#endif
