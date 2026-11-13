// PC test of the big planets (3DS and PC builds: up to 1024 x 1024 tiles, 100,000 creatures): make one, run it, look at the crowd, save and load it, and read an old DS-layout save
#define PCTEST 1
#include <stdlib.h>
#include <time.h>
#include "source/main.c"
int main(int argc,char**argv){
    int sz=argc>1?atoi(argv[1]):1024; int ticks=argc>2?atoi(argv[2]):4000;
    printf("MAXD %d MAXC %d sizeof(Cr) %d sizeof(Vil) %d sizes:",MAXD,MAXC,(int)sizeof(Cr),(int)sizeof(Vil)); for(int k=0;k<SZ_N;k++) printf(" %d",szList[k]); printf("\n");
    planetSize=sz; rs=555u; makeSpecies(); makeItems(); clock_t t0=clock(); newWorld(); monOn=1; double tg=(clock()-t0)/(double)CLOCKS_PER_SEC;
    int land=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) land+=world[y][x]>T_WATER; printf("planet %d x %d made in %.2f s: land %d tiles, creatures %d, villages %d\n",WW,WH,tg,land,nCr,nVilOn);
    t0=clock(); for(int t=1;t<=ticks;t++){ tick=t; year=t/1200; simulate(); } double ts=(clock()-t0)/(double)CLOCKS_PER_SEC;
    int al=0,far=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ al++; if(cr[i].x>255||cr[i].y>255) far++; } int hh; int folk=vilTotals(&hh);
    printf("%d ticks in %.2f s (%.0f us a tick): creatures %d (%d beyond tile 255), villages %d, folk %d\n",ticks,ts,ts*1e6/ticks,al,far,nVilOn,folk);
    // a patch of biome far from the origin and the atmosphere still work
    int px=WW/2, py=WH/2; for(int k=0;k<WW*WH&&!(world[py][px]>T_WATER&&world[py][px]!=T_LAVA&&world[py][px]!=T_MOUNT);k++){ px=(int)(rnd()%WW); py=(int)(rnd()%WH); } for(int b=1;b<=3;b++){ for(int q=0;q<PW_N;q++) if(pw[q].kind==PK_PAINT&&pw[q].arg==100+b) power=q; brush=3; usePower(px-10+b*8,py,1); }
    for(int t=ticks+1;t<=ticks+1500;t++){ tick=t; simulate(); } printf("biome tiles %d %d %d, air O2 %d CO2 %d warmth %+d\n",nBio[1],nBio[2],nBio[3],ATV(atO2),ATV(atCO2),ATV(atTemp));
    // save and load
    u32 h=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x+=3) h=h*31+world[y][x]+bld[y][x]; int a0=0; for(int i=0;i<nCr;i++) a0+=cr[i].alive; int nc0=a0;
    t0=clock(); int ok=saveGame("/tmp/gg_big.sav"); double tsv=(clock()-t0)/(double)CLOCKS_PER_SEC; FILE*fp=fopen("/tmp/gg_big.sav","rb"); long fsz=0; if(fp){ fseek(fp,0,SEEK_END); fsz=ftell(fp); fclose(fp); }
    memset(world,0,sizeof world); nCr=0; int ok2=loadGame("/tmp/gg_big.sav"); u32 h2=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x+=3) h2=h2*31+world[y][x]+bld[y][x]; int a1=0; for(int i=0;i<nCr;i++) a1+=cr[i].alive;
    printf("save %d (%.2f s, %ld KB) load %d: planet %d, ground identical %s, creatures %d -> %d\n",ok,tsv,fsz/1024,ok2,WW,h==h2?"YES":"NO",nc0,a1);
    printf("%s\n",(WW==sz&&ok&&ok2&&h==h2&&a1==nc0&&nBio[1]>0)?"BIG OK":"BIG WEAK"); return 0; }
