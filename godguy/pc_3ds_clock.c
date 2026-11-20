// Host-only regression for 3DS real-time audio clock and its adaptive
// globe-quality governor. No 3DS emulator, libraries or hardware needed.
// gcc -O2 -Wall -Wextra pc_3ds_clock.c -o /tmp/gg3clock && /tmp/gg3clock
#include <stdio.h>
#include <stdint.h>
#include "3ds/frame_clock.h"

static int bad, frames;
static unsigned long long samples, ticks;
#define CHECK(c,why) do{ if(!(c)){fprintf(stderr,"CLOCK FAIL: %s\n",why);bad++;} }while(0)
static void step(Gg3dsFrameClock*c,uint64_t now,int*quality){
    int n,k,q;
    gg3dsClockFrame(c,now,&n,&k,&q);
    CHECK(n>=0&&n<=2800,"sample output bounded to the 3DS audio buffer");
    CHECK(k>=0&&k<=12,"sequencer catch-up bounded per frame");
    CHECK(q==0||q==1,"globe detail setting bounded");
    samples+=(unsigned)n; ticks+=(unsigned)k; frames++;
    *quality=q;
}
int main(void){
    Gg3dsFrameClock c;
    gg3dsClockInit(&c,1);
    uint64_t now=10000;
    int q=0;
    for(int i=0;i<60;i++){if(i)now+=17;step(&c,now,&q);}
    CHECK(q==1,"New 3DS starts in fine-detail mode");
    CHECK(samples==(60ull*17*32768/1000),"steady 17 ms audio sample time exact");
    CHECK(ticks==(60ull*17*60/1000),"steady 60 Hz music time exact");
    // A 220ms renderer stall used to be clipped at 80ms, permanently
    // dropping 140ms of music and audio. Repay it across later frames.
    now+=220;step(&c,now,&q);
    CHECK(q==1,"one slow globe frame must not drop detail");
    CHECK(c.audioDebt>0,"large render stall retains unmixed audio samples");
    CHECK(c.tickDebt>0,"large render stall retains music tick fractions");
    for(int i=0;i<40;i++){now+=17;step(&c,now,&q);}
    unsigned long long ms=(60ull*17+220+40ull*17);
    CHECK(samples==ms*32768/1000,"220 ms audio stall fully repaid");
    CHECK(ticks==ms*60/1000,"220 ms music stall fully repaid");
    CHECK(c.audioDebt<1000u,"audio catch-up has drained to fractional remainder");
    CHECK(c.tickDebt<1000u,"music catch-up has drained to fractional remainder");
    // Three consecutive slow globe frames trigger conservative sampling.
    for(int i=0;i<3;i++){now+=33;step(&c,now,&q);}
    CHECK(q==0,"New 3DS safely falls back to lighter sampling under load");
    for(int i=0;i<119;i++){now+=16;step(&c,now,&q);}
    CHECK(q==0,"globe does not flicker to fine detail prematurely");
    now+=16;step(&c,now,&q);
    CHECK(q==1,"fine globe detail automatically returns after stable frames");
    // Sleep/resume should not attempt to process thousands of old ticks or
    // allocate an unbounded sound queue.
    now+=5000;step(&c,now,&q);
    CHECK(c.audioDebt<=8000000u,"resume audio debt bounded");
    CHECK(c.tickDebt<=30000u,"resume music debt bounded");
    for(int i=0;i<40;i++){now+=17;step(&c,now,&q);}
    CHECK(c.audioDebt<1000u,"resume audio backlog eventually drains");
    CHECK(c.tickDebt<1000u,"resume tick backlog eventually drains");
    Gg3dsFrameClock old;
    gg3dsClockInit(&old,0);
    int oldq=1;
    for(int i=0;i<200;i++)step(&old,20000+(uint64_t)i*16,&oldq);
    CHECK(oldq==0,"Old 3DS keeps the affordable 2-pixel globe");
    printf("3DS CLOCK %s: %d frame samples, %llu sound samples, %llu music ticks, quality governor and stall recovery\n",
           bad?"FAIL":"OK",frames,samples,ticks);
    return bad?1:0;
}
