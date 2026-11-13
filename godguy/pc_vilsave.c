// PC test of saves across village counts: compile with -DGG_VILN=40 -DVS_WRITE to write a world, then without to read it (a build with another VIL_N must keep every village)
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(int argc,char**argv){
#ifdef VS_WRITE
    planetSize=192; rs=2468u; makeSpecies(); makeItems(); newWorld(); monOn=1; for(int t=1;t<=30000;t++){ tick=t; year=t/1200; simulate(); }
    for(int v=0;v<VIL_N;v++) favVil[v]=(v%3==0); int nv=0; unsigned h=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ nv++; h=h*31+vil[v].x*7+vil[v].y+vil[v].pop; }
    printf("wrote with VIL_N %d: %d villages, hash %u, fav count %d\n",VIL_N,nv,h,({int c=0; for(int v=0;v<VIL_N;v++) c+=favVil[v]!=0; c;})); saveGame("/tmp/gg_vil.sav"); return 0;
#else
    makeSpecies(); makeItems(); int ok=loadGame("/tmp/gg_vil.sav"); int nv=0; unsigned h=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ nv++; h=h*31+vil[v].x*7+vil[v].y+vil[v].pop; } int fc=0; for(int v=0;v<VIL_N;v++) fc+=favVil[v]!=0;
    printf("read with VIL_N %d: load %d, %d villages, hash %u, fav count %d\n",VIL_N,ok,nv,h,fc); return 0;
#endif
}
