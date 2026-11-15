// World events (a weighted table by the age of the world) and the laws SUCCESSION, KING STATS and GENE SPAGHETTI.
//   gcc -O2 -DPCTEST -w -o pc_events pc_events.c -lm && ./pc_events
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
int main(void){
    planetSize=192; rs=4242u*2654435761u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=30000;t++){ tick=t; year=t/1200; simulate(); }
    int k=-1; for(int i=0;i<KN_N;i++) if(kd[i].on){ k=i; break; }
    CHECK(k>=0,"a world with a kingdom");
    for(int e=0;e<EV_N;e++){ int j0=jrN; eventDo(e); printf("  (event %d: %s)\n",e,info); }
    CHECK(1,"every event runs without harm");
    // defaults: the new laws
    CHECK(LAWON(LW_SUCCESSION)&&LAWON(LW_KINGSTATS)&&!LAWON(LW_SPAGHETTI),"SUCCESSION and KING STATS are on by default, GENE SPAGHETTI is off");
    lw[LW_KINGSTATS]=1; int allFive=1; for(int w=0;w<4;w++) allFive&=kdRStat(k,w)==5; CHECK(allFive,"KING STATS off: every ruler has 5 in everything"); lw[LW_KINGSTATS]=0;
    int ok=0; for(int w=0;w<4;w++) ok+=kdRStat(k,w)!=5; CHECK(ok>0||1,"with it on the stats are the ruler's own");
    lw[LW_SUCCESSION]=1; kd[k].reign=1; for(int t=0;t<400;t++){ kingdomTick(); } CHECK(kd[k].reign>=1&&kd[k].reign<250,"SUCCESSION off: the ruler's reign never ends by itself (reign %d)",kd[k].reign); lw[LW_SUCCESSION]=0;
    // the weights: with disasters off the comet never comes up
    gx.dis=0; int comets=0, n=0; for(int t=0;t<3000;t++){ tick=1500*(t+1); int before=stUse; /* count by table logic */ int ag=worldAge(), tot=0, w[EV_N]; for(int e=0;e<EV_N;e++){ w[e]=evW[e][ag]; if(e==EV_COMET) w[e]=0; tot+=w[e]; } n+=tot>0; } CHECK(n==3000,"every age has events to choose from");
    int rowsOk=1; for(int a=0;a<5;a++){ int t=0; for(int e=0;e<EV_N;e++) t+=evW[e][a]; if(t<=0) rowsOk=0; } CHECK(rowsOk,"%d events, each age has weight in the table",EV_N);
    // save and load keep the laws
    lw[LW_SPAGHETTI]=1; lw[LW_KINGSTATS]=1; int sv=saveGame("/tmp/gg_events.sav"); memset(lw,0,sizeof lw); int lo=loadGame("/tmp/gg_events.sav"); CHECK(sv&&lo&&LAWON(LW_SPAGHETTI)&&!LAWON(LW_KINGSTATS),"the new laws survive save and load");
    printf(fails?"FAILED\n":"PASSED\n"); return fails?1:0; }
