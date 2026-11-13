// PC test: villages raise temples (a mood lift, they stop sickness) and statues to their heroes
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(){ planetSize=128; rs=31u; makeSpecies(); makeItems(); newWorld(); monOn=1; for(int t=1;t<=40000;t++){ tick=t; year=t/1200; simulate(); }
  int tm=0,st=0,vt=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){ tm+=bld[y][x]==B_TEMPLE; st+=bld[y][x]==B_STATUE; } for(int v=0;v<VIL_N;v++) vt+=vil[v].on;
  int w=0,wo=0,nw=0,no=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ if(vilBf[v]&1){ w+=vilMood(v); nw++; } else { wo+=vilMood(v); no++; } }
  printf("villages %d, temples %d, statues %d; mean mood with a temple %d (%d), without %d (%d)\n",vt,tm,st,nw?w/nw:0,nw,no?wo/no:0,no);
  int ok=saveGame("/tmp/gg_t.sav"); int f0=0; for(int v=0;v<VIL_N;v++) f0+=vil[v].on?vilBf[v]:0; memset(vilBf,0,sizeof vilBf); loadGame("/tmp/gg_t.sav"); int f1=0; for(int v=0;v<VIL_N;v++) f1+=vil[v].on?vilBf[v]:0; printf("flags kept by load: %s (%d vs %d); folk of level 2+: %d\n",f0==f1?"yes":"NO",f0,f1,({int c=0; for(int i=0;i<nCr;i++) c+=cr[i].alive&&cr[i].dir>=1&&arch[sp[cr[i].sp].a].diet==4; c;}));
  printf("%s\n",(tm>0&&st>0&&f0==f1)?"TEMPLES OK":"TEMPLES WEAK"); return 0; }
