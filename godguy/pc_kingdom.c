// PC test of the kingdoms: let several worlds run for a long time and print who rose, fought, allied and fell
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(int argc,char**argv){
    int seeds=argc>1?atoi(argv[1]):3, ticks=argc>2?atoi(argv[2]):60000, wars=0, vass=0, emp=0, cap=0, ally=0;
    for(int s=0;s<seeds;s++){ planetSize=192; rs=777u*2654435761u+s*31337u; makeSpecies(); makeItems(); newWorld(); monOn=1; for(int t=1;t<=ticks;t++){ tick=t; year=t/1200; simulate(); }
        int nk=0; for(int k=0;k<KN_N;k++) nk+=kd[k].on; printf("world %d: year %d, %d villages, %d kingdoms\n",s,year,nVilOn,nk);
        for(int k=0;k<KN_N;k++) if(kd[k].on){ char n[24], r[40]; int c,p,w; kdName(k,n); kdRuler(k,r); kdStats(k,&c,&p,&w); printf("  %-8s %s %-18s %-8s cities %d pop %3d power %3d gold %4d tech %d%s%s war %02x ally %02x\n",n,kd[k].empire?"EMP":"kng",r,rtrN[kd[k].rtr],c,p,w,kd[k].gold,kd[k].tech,kd[k].lord?" VASSAL":"",kd[k].trade?" TRADE":"",kd[k].war,kd[k].allies); }
        for(int i=0;i<jrN;i++){ printf("    y%-2d %s\n",jrYear[i],jr[i]); if(strstr(jr[i],"DECLARES")) wars++; if(strstr(jr[i],"VASSAL")) vass++; if(strstr(jr[i],"EMPIRE")) emp++; if(strstr(jr[i],"TAKES")) cap++; if(strstr(jr[i],"ALLY")) ally++; } }
    printf("totals: wars %d, captures %d, vassals %d, empires %d, alliances %d\n",wars,cap,vass,emp,ally); return 0; }
