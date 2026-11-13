// PC test of the atmosphere: the gases settle by themselves (forests against crowds), the greenhouse and dust powers move the temperature, heat melts the ice, and a save keeps the air
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int snow(void){ int n=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) n+=world[y][x]==T_SNOW; return n; }
static void run(int a,int b){ for(int t=a;t<=b;t++){ tick=t; simulate(); } }
int main(int argc,char**argv){
    planetSize=128; rs=31337u; makeSpecies(); makeItems(); newWorld(); monOn=0;
    printf("start: O2 %d CO2 %d dust %d warmth %d\n",ATV(atO2),ATV(atCO2),ATV(atDust),ATV(atTemp));
    int o2min=99,o2max=0,tmin=99,tmax=-99; for(int t=1;t<=24000;t++){ tick=t; simulate(); if(t%2000==0){ int o=ATV(atO2),tt=ATV(atTemp); if(o<o2min) o2min=o; if(o>o2max) o2max=o; if(tt<tmin) tmin=tt; if(tt>tmax) tmax=tt; printf("tick %5d: O2 %2d CO2 %3d dust %2d warmth %+3d  creatures %5d snow tiles %d\n",t,o,ATV(atCO2),ATV(atDust),tt,nCr,snow()); } }
    int pg=-1,pd=-1,po=-1; for(int q=0;q<PW_N;q++){ if(pw[q].kind==PK_AIR&&pw[q].arg==0) pg=q; if(pw[q].kind==PK_AIR&&pw[q].arg==2) pd=q; if(pw[q].kind==PK_AIR&&pw[q].arg==1) po=q; }
    int t0=ATV(atTemp); power=pd; brush=1; for(int k=0;k<30;k++) usePower(60,60,1); int d1=ATV(atDust); int tlow=99; for(int t=24001;t<=27000;t++){ tick=t; simulate(); if(t%64==0&&ATV(atTemp)<tlow) tlow=ATV(atTemp); }
    printf("DUST x30: dust %d, warmth %+d -> lowest %+d\n",d1,t0,tlow); run(27001,32000);
    int t1=ATV(atTemp), s0=snow(); power=pg; for(int k=0;k<40;k++) usePower(60,60,1); int c1=ATV(atCO2); int t2=-99; for(int t=32001;t<=42000;t++){ tick=t; simulate(); if(t%64==0&&ATV(atTemp)>t2) t2=ATV(atTemp); } int s1=snow();
    printf("GREENHOUSE x40: CO2 %d, warmth %+d -> peak %+d, snow tiles %d -> %d\n",c1,t1,t2,s0,s1);
    int o0=ATV(atO2); power=po; for(int k=0;k<5;k++) usePower(60,60,1); printf("OXYGEN x5: O2 %d -> %d\n",o0,ATV(atO2));
    int a0=atO2,a1=atCO2,a2=atDust,a3=atTemp; int ok=saveGame("/tmp/gg_atmo.sav"); atmoReset(); int ok2=loadGame("/tmp/gg_atmo.sav"); int same=a0==atO2&&a1==atCO2&&a2==atDust&&a3==atTemp;
    printf("save %d load %d: air kept: %s\n",ok,ok2,same?"YES":"NO");
    printf("%s\n",(o2min>=8&&o2max<=45&&tmin>=-30&&tmax<=30&&t2>t1+3&&tlow<t0-2&&same)?"ATMO OK":"ATMO WEAK"); return 0; }
