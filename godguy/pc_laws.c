// PC test of the World Laws: each law switched off must really stop what it names, and the laws must survive a save and a load
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int alive(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive; return n; }
static int run(u32 off,int ticks,unsigned seed,int*wars,int*born,int*aged){ planetSize=128; rs=seed; makeSpecies(); makeItems(); newWorld(); monOn=1; lawOff=off; stWars=0; stMut=0; int a0=alive(); int old=0;
    for(int t=1;t<=ticks;t++){ tick=t; year=t/1200; simulate(); }
    for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].age>=120) old++;
    *wars=(int)stWars; *born=alive()-a0; *aged=old; return alive(); }
int main(){ int w,b,o, fails=0; unsigned seed=555u;
    run(0,40000,seed,&w,&b,&o); printf("all laws on : wars %d, population change %+d, creatures older than 120: %d, mutations %u\n",w,b,o,(unsigned)stMut); int wars0=w, old0=o; unsigned mut0=stMut;
    run(1u<<LW_DIPLOMACY,40000,seed,&w,&b,&o); printf("WARS off    : wars %d\n",w); if(w>0&&wars0>0){ fails++; printf("  FAIL: wars still happened\n"); }
    run(1u<<LW_AGE,40000,seed,&w,&b,&o); printf("OLD AGE off : creatures older than 120: %d (was %d)\n",o,old0); if(o<=old0){ fails++; printf("  FAIL: nobody lived longer\n"); }
    run((1u<<LW_CIVBABY)|(1u<<LW_ANIBABY),6000,seed,&w,&b,&o); printf("BREEDING off: population change %+d\n",b); { int w2,b2,o2; run(0,6000,seed,&w2,&b2,&o2); if(!(b<b2)){ fails++; printf("  FAIL: breeding off did not reduce growth (%+d vs %+d)\n",b,b2); } }
    run(1u<<LW_MUTATE,40000,seed,&w,&b,&o); printf("MUTATIONS off: mutations %u (was %u)\n",(unsigned)stMut,mut0); if(stMut>0&&mut0>0){ fails++; printf("  FAIL: mutations still happened\n"); }
    { int w2,b2,o2; run(1u<<LW_POP100,40000,seed,&w2,&b2,&o2); int mx=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop>mx) mx=vil[v].pop; int mx0; run(0,40000,seed,&w2,&b2,&o2); mx0=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop>mx0) mx0=vil[v].pop; printf("100 PEOPLE on: biggest village %d (default %d)\n",mx,mx0); if(mx>115&&mx0>mx){ fails++; printf("  FAIL: pop cap\n"); } }
    { int w2,b2,o2; run(1u<<LW_GAIA,6000,seed,&w2,&b2,&o2); int l0=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l0+=world[y][x]==T_LAVA; int cx=WW/2, cy=WH/2; for(int r=0;r<WW&&(world[cy][cx]<=T_WATER||world[cy][cx]==T_LAVA);r++){ int dx=(r%2?1:-1)*(r/2+1); int tx=WW/2+dx, ty=WH/2+((r*7)%11)-5; if(tx>=0&&tx<WW&&ty>=0&&ty<WH&&world[ty][tx]>T_WATER){ cx=tx; cy=ty; } } nukeAt(cx,cy); int l1=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l1+=world[y][x]==T_LAVA; printf("GAIA'S COVENANT on: bomb leaves lava %d -> %d\n",l0,l1); if(l1>l0){ fails++; printf("  FAIL: gaia\n"); } }
    // save and load
    run((1u<<LW_PLAGUE)|(1u<<LW_WEATHER)|(1u<<LW_GAIA),300,seed,&w,&b,&o); unsigned want=lawOff; if(!saveGame("/tmp/gg_laws.sav")){ fails++; printf("FAIL: save\n"); } lawOff=0; if(!loadGame("/tmp/gg_laws.sav")){ fails++; printf("FAIL: load\n"); } printf("save/load: laws %u -> %u\n",want,(unsigned)lawOff); if(lawOff!=want){ fails++; printf("  FAIL: laws not kept\n"); }
    printf("%s\n",fails?"LAWS FAIL":"LAWS OK"); return fails?1:0; }
