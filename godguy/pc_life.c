// PC test: creature levels (killers grow stronger) and village mood (fed, safe villages are happier and grow faster)
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(){ planetSize=128; rs=2024u; makeSpecies(); makeItems(); newWorld(); monOn=1; for(int t=1;t<=20000;t++){ tick=t; year=t/1200; simulate(); }
  int hist[11]={0}, top=0, over=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ if(cr[i].dir>10) over++; else hist[cr[i].dir]++; if(cr[i].dir>top&&cr[i].dir<=10) top=cr[i].dir; }
  printf("levels alive: "); for(int l=0;l<=10;l++) printf("%d:%d ",l,hist[l]); printf("\n");
  int vn=0,vs=0,mn=100,mx=-100; for(int v=0;v<VIL_N;v++) if(vil[v].on){ vn++; vs+=vilMood(v); if(vilMood(v)<mn) mn=vilMood(v); if(vilMood(v)>mx) mx=vilMood(v); }
  printf("villages %d, mood %d..%d, mean %d\n",vn,vn?mn:0,vn?mx:0,vn?vs/vn:0);
  int v0=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on){ v0=v; break; } int hi=0,lo=0; if(v0>=0){ vil[v0].stock=400; vilSick[v0]=0; hi=vilMood(v0); vil[v0].stock=0; vilSick[v0]=5; vil[v0].foe=1; lo=vilMood(v0); vil[v0].foe=0; vilSick[v0]=0; }
  printf("one village fed and at peace: mood %d; starving, sick, at war: %d\n",hi,lo);
  // a level really changes the fight: health and damage grow with it
  int hp0=0,hp10=0; { Cr c; memset(&c,0,sizeof c); c.sp=8; c.dir=0; hp0=maxHp(&c); c.dir=10; hp10=maxHp(&c); } printf("max health at level 0: %d, at level 10: %d\n",hp0,hp10);
  // the level of a creature is saved with it
  int j=-1; for(int i=0;i<nCr;i++) if(cr[i].alive){ j=i; break; } int kept=0; if(j>=0){ cr[j].dir=7; unsigned sp0=cr[j].sp, x0=cr[j].x, y0=cr[j].y; saveGame("/tmp/gg_life.sav"); memset(cr,0,sizeof(Cr)*nCr); loadGame("/tmp/gg_life.sav"); for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].sp==sp0&&cr[i].x==x0&&cr[i].y==y0&&cr[i].dir==7) kept=1; }
  printf("level kept by save: %s\n",kept?"yes":"NO");
  printf("%s\n",(top>=2&&!over&&vn>0&&hi>lo+40&&hp10>hp0&&kept)?"LIFE OK":"LIFE WEAK"); return 0; }
