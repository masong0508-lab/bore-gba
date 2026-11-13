// PC test: docks get built on shores and boats carry settlers to new shores, founding villages; boats survive a save
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(){ int docks=0, launched=0, founded=0, maxv=0; planetSize=192; cShape=3; rs=77u; makeSpecies(); makeItems(); newWorld(); monOn=0;
  for(int t=1;t<=40000;t++){ tick=t; year=t/1200; int b0=0; for(int b=0;b<8;b++) b0+=boat[b].on; int v0=nVilOn; simulate(); int b1=0; for(int b=0;b<8;b++) b1+=boat[b].on; if(b1>b0) launched++; if(nVilOn>v0&&b1<b0) founded++; if(nVilOn>maxv) maxv=nVilOn; }
  for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) docks+=bld[y][x]==B_DOCK;
  int ok=0; for(int i=0;i<jrN;i++) if(strstr(jr[i],"SETTLERS SAIL")) ok++;
  printf("docks %d, boats launched %d, villages founded by boats %d, journal lines %d, villages %d\n",docks,launched,founded,ok,nVilOn);
  boat[0].on=1; boat[0].x=50; boat[0].y=60; boat[0].tx=70; boat[0].ty=80; saveGame("/tmp/gg_boat.sav"); boat[0].on=0; loadGame("/tmp/gg_boat.sav"); int kept=boat[0].on&&boat[0].tx==70;
  printf("boat kept by save: %s\n",kept?"yes":"NO"); printf("%s\n",(docks>0&&launched>0&&founded>0&&kept)?"BOATS OK":"BOATS WEAK"); return 0; }
