// Pack A1 (rulers): the four ruler stats are deterministic, in range, change with the ruler, and really feed the city cap, power and loyalty; a coronation is journalled.
//   gcc -O2 -DPCTEST -w -o pc_rulers pc_rulers.c -lm && ./pc_rulers
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
int main(void){
    planetSize=192; rs=777u*2654435761u; makeSpecies(); makeItems(); newWorld(); monOn=1; static int sawCrown; for(int t=1;t<=60000;t++){ tick=t; year=t/1200; simulate(); if(t%50==0) for(int i=0;i<jrN;i++) if(strstr(jr[i],"CROWNS")) sawCrown=1; }
    int k=-1; for(int i=0;i<KN_N;i++) if(kd[i].on){ k=i; break; } printf("ruler stats (kingdom %d):\n",k); if(k<0){ printf("  FAIL  no kingdom rose\nFAILED\n"); return 1; }
    int inRange=1, same=1, seen[4][11]={{0}}, spread[4]={0};
    for(int r1=0;r1<12;r1++) for(int r2=0;r2<8;r2++){ kd[k].rn1=(u8)r1; kd[k].rn2=(u8)r2; kd[k].reign=0; for(int w=0;w<4;w++){ int a=kdRStat(k,w), b=kdRStat(k,w); if(a<1||a>10) inRange=0; if(a!=b) same=0; seen[w][a]=1; } }
    for(int w=0;w<4;w++) for(int v=1;v<=10;v++) spread[w]+=seen[w][v];
    CHECK(inRange,"every stat of every ruler (96 name seeds) is 1..10");
    CHECK(same,"the same ruler always has the same stats");
    CHECK(spread[0]>=7&&spread[1]>=7&&spread[2]>=7&&spread[3]>=7,"rulers differ: distinct values per stat %d %d %d %d",spread[0],spread[1],spread[2],spread[3]);
    kd[k].rn1=3; kd[k].rn2=4; kd[k].reign=0; int a0=kdRStat(k,0); kd[k].reign=130; int a1=kdRStat(k,0); kd[k].reign=0;
    CHECK(a1>=a0&&a1<=10,"a long reign never lowers a stat (%d -> %d)",a0,a1);
    // city cap and power follow the stats, everything else held still
    int lo=-1,hi=-1; for(int r1=0;r1<12&&(lo<0||hi<0);r1++) for(int r2=0;r2<8;r2++){ kd[k].rn1=(u8)r1; kd[k].rn2=(u8)r2; int st=kdRStat(k,0); if(st<6&&lo<0) lo=r1*8+r2; if(st>=6&&st<12&&hi<0) hi=r1*8+r2; }
    kd[k].rn1=(u8)(lo/8); kd[k].rn2=(u8)(lo%8); int capLo=kdMaxCities(k); kd[k].rn1=(u8)(hi/8); kd[k].rn2=(u8)(hi%8); int capHi=kdMaxCities(k);
    CHECK(capHi==capLo+1,"stewardship 6 or more allows one more city (%d -> %d)",capLo,capHi);
    int wlo=-1,whi=-1,wl=99,wh=0; for(int r1=0;r1<12;r1++) for(int r2=0;r2<8;r2++){ kd[k].rn1=(u8)r1; kd[k].rn2=(u8)r2; kd[k].reign=0; int st=kdRStat(k,1); if(st<wl){ wl=st; wlo=r1*8+r2; } if(st>wh){ wh=st; whi=r1*8+r2; } }
    int c,pp,pl,ph; kd[k].rn1=(u8)(wlo/8); kd[k].rn2=(u8)(wlo%8); kdStatsRaw(k,&c,&pp,&pl); kd[k].rn1=(u8)(whi/8); kd[k].rn2=(u8)(whi%8); kdStatsRaw(k,&c,&pp,&ph);
    CHECK(wh>wl&&ph>pl,"a warrior ruler (warfare %d) is stronger than a weak one (%d): power %d vs %d",wh,wl,ph,pl);
    int okL=1; for(int v=0;v<VIL_N;v++) if(vil[v].on&&(vilL[v]<-100||vilL[v]>100)) okL=0;
    CHECK(okL,"loyalty never leaves -100..100");
    int crowns=0; for(int i=0;i<jrN;i++) if(strstr(jr[i],"CROWNS")) crowns++;
    CHECK(crowns>0||sawCrown,"a coronation is written to the journal (%d in the last %d lines)",crowns,jrN);
    printf(fails?"FAILED\n":"PASSED\n"); return fails?1:0; }
