// 3DS presentation clock: integer, allocation-free timing shared by the
// hardware port and its deterministic desktop regression. One pass per frame.
//
// A previous 80 ms delta clamp permanently discarded elapsed audio/music
// time during slow globe frames. Here short stalls accumulate bounded debts:
// render/mix at most 2800 samples at once, but repay any remaining audio
// samples on subsequent frames. Sequencer ticks also catch up (max 12/frame).
// The 250 ms cap applies only to sleep/resume-size pauses, not ordinary slow
// frames, to avoid a multi-second burst of stale samples after home-menu use.
//
// On New 3DS, repeated >28ms frames temporarily turn OFF expensive 1-pixel
// globe detail. It resumes only after 120 stable <=18ms frames. Old 3DS always
// uses the lighter 2-pixel path. No state is persisted in save games.
#ifndef GG3DS_FRAME_CLOCK_H
#define GG3DS_FRAME_CLOCK_H
#include <stdint.h>
typedef struct {
    uint64_t lastMs;
    uint32_t audioDebt;  // 1/1000 sample; 32768 units are added per millisecond
    uint32_t tickDebt;   // 1/1000 60Hz tick; 60 units are added per millisecond
    unsigned slow, steady;
    int new3ds, highDetail;
} Gg3dsFrameClock;
static void gg3dsClockInit(Gg3dsFrameClock*c,int new3ds){
    c->lastMs=0; c->audioDebt=c->tickDebt=0;
    c->slow=c->steady=0;
    c->new3ds=new3ds!=0; c->highDetail=c->new3ds;
}
static void gg3dsClockFrame(Gg3dsFrameClock*c,uint64_t now,int*frames,int*ticks,int*highDetail){
    uint64_t delta=c->lastMs && now>c->lastMs?now-c->lastMs:17;
    c->lastMs=now;
    // Treat a resumed process as a new stream; 250 ms is still enough
    // to cover a single severe rendering hitch without losing its tempo.
    unsigned ms=(unsigned)(delta>250?250:delta);
    c->audioDebt+=ms*32768u;
    if(c->audioDebt>8000000u)c->audioDebt=8000000u;
    unsigned n=c->audioDebt/1000u;
    if(n>2800)n=2800;
    c->audioDebt-=n*1000u;
    c->tickDebt+=ms*60u;
    if(c->tickDebt>30000u)c->tickDebt=30000u;
    unsigned k=c->tickDebt/1000u;
    if(k>12)k=12;
    c->tickDebt-=k*1000u;
    if(c->new3ds){
        if(ms>28){ c->slow++; c->steady=0; }
        else if(ms<=18){ c->steady++; c->slow=0; }
        else { c->slow=0; c->steady=0; }
        if(c->highDetail&&c->slow>=3){ c->highDetail=0; c->slow=0; }
        else if(!c->highDetail&&c->steady>=120){
            c->highDetail=1; c->steady=0;
        }
    }
    *frames=(int)n; *ticks=(int)k; *highDetail=c->highDetail;
}
#endif
