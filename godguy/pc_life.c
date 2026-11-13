// PC test: creature levels (killers grow stronger) and village mood (fed, safe villages are happier and grow faster)
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(){ planetSize=128; rs=2024u; makeSpecies(); makeItems(); newWorld(); monOn=1; for(int t=1;t<=20000;t++){ tick=t; year=t/1200; simulate(); }
  int hist[11]={0}, top=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ hist[cr[i].dir]++; if(cr[i].dir>top) top=cr[i].dir; }
  printf("levels alive: "); for(int l=0;l<=10;l++) printf("%d:%d ",l,hist[l]); printf("\n");
  int vn=0,vs=0,mn=100,mx=-100; for(int v=0;v<VIL_N;v++) if(vil[v].on){ vn++; vs+=vilMood(v); if(vilMood(v)<mn) mn=vilMood(v); if(vilMood(v)>mx) mx=vilMood(v); }
  printf("villages %d, mood %d..%d, mean %d\n",vn,vn?mn:0,vn?mx:0,vn?vs/vn:0);
  int v0=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on){ v0=v; break; } int hi=0,lo=0; if(v0>=0){ vil[v0].stock=400; vilSick[v0]=0; hi=vilMood(v0); vil[v0].stock=0; vilSick[v0]=5; vil[v0].foe=1; lo=vilMood(v0); vil[v0].foe=0; }
  printf("one village fed and at peace: mood %d; starving, sick, at war: %d\n",hi,lo);
  printf("%s\n",(top>=2&&vn>0&&hi>lo+40)?"LIFE OK":"LIFE WEAK"); return 0; }
