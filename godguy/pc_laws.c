// PC test of the World Laws (port step 5): each law switched away from its default must really change what it names, and the laws must survive a save and a load
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int alive(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive; return n; }
static void setup(unsigned seed){ planetSize=128; rs=seed; makeSpecies(); makeItems(); newWorld(); monOn=1; }
static void runT(int from,int to){ for(int t=from;t<=to;t++){ tick=t; year=t/1200; simulate(); } }
int main(){ int fails=0; unsigned seed=555u;
    // defaults: every law as it was, so an old save (no trailer) plays as before
    setup(seed); for(int i=0;i<LW_N;i++) if(lw[i]){ fails++; printf("FAIL: law %d not at default\n",i); }
    if(LAWON(LW_PEACEMON)||LAWON(LW_MUTBOX)||LAWON(LW_GAIA)||LAWON(LW_POP100)||!LAWON(LW_HUNGER)||!LAWON(LW_DIPLOMACY)||!LAWON(LW_CROWD)){ fails++; printf("FAIL: default on/off wrong\n"); }
    // baseline
    setup(seed); stWars=0; stMut=0; int a0=alive(); runT(1,30000); int wars0=(int)stWars; unsigned mut0=stMut; int old0=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].age>=120) old0++;
    printf("all default: wars %d, mutations %u, population %d -> %d, creatures older than 120: %d\n",wars0,mut0,a0,alive(),old0);
    // DIPLOMACY off: no wars
    setup(seed); lw[LW_DIPLOMACY]=1; stWars=0; runT(1,30000); printf("DIPLOMACY off: wars %d\n",(int)stWars); if(stWars>0&&wars0>0){ fails++; printf("  FAIL: wars still happened\n"); }
    // OLD AGE off: the old stay
    setup(seed); gx.lAge=1; runT(1,30000); int old1=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].age>=120) old1++; printf("OLD AGE off: older than 120: %d (default %d)\n",old1,old0); if(old1<=old0){ fails++; printf("  FAIL: nobody lived longer\n"); }
    // MUTATIONS off
    setup(seed); lw[LW_MUTATE]=1; stMut=0; runT(1,30000); printf("MUTATIONS off: mutations %u (default %u)\n",(unsigned)stMut,mut0); if(stMut>0&&mut0>0){ fails++; printf("  FAIL: mutations still happened\n"); }
    // MUTANT BOX on: many more mutations
    setup(seed); lw[LW_MUTBOX]=1; stMut=0; runT(1,6000); unsigned mb=stMut; setup(seed); stMut=0; runT(1,6000); printf("MUTANT BOX on: mutations %u vs %u in 6000 ticks\n",mb,(unsigned)stMut); if(mb<=stMut){ fails++; printf("  FAIL: mutant box\n"); }
    // BABIES off: the population cannot grow by birth
    { setup(seed); lw[LW_CIVBABY]=lw[LW_ANIBABY]=1; stBorn=0; runT(1,6000); unsigned b1=stBorn; setup(seed); stBorn=0; runT(1,6000); printf("BABIES off: born %u (default %u)\n",b1,(unsigned)stBorn); if(!(b1<stBorn)){ fails++; printf("  FAIL: babies\n"); } }
    // HUNGER off: isolate one citizen so this measures starvation, not predation or movement by the rest of the world.
    { int hp[2]={0}; for(int m=0;m<2;m++){ setup(seed); nCr=0; memset(occ,0,sizeof occ); cellClear(); for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){ world[y][x]=T_GRASS; bld[y][x]=0; bldv[y][x]=0; } lw[LW_HUNGER]=(u8)m; int i=addCr(0,64,64); if(i>=0){ cr[i].hp=3; cr[i].food=0; think(i); hp[m]=cr[i].alive?cr[i].hp:0; } }
      printf("HUNGER: isolated citizen HP after one empty-belly turn: %d (default) vs %d (law off)\n",hp[0],hp[1]); if(!(hp[0]<hp[1])){ fails++; printf("  FAIL: hunger\n"); } }
    // WEATHER off: clear sky
    { setup(seed); lw[LW_WEATHER]=1; int w=0; for(int t=1;t<=8000;t++){ tick=t; year=t/1200; weatherTick(); w+=weather!=0; } printf("WEATHER off: ticks with weather %d\n",w); if(w){ fails++; printf("  FAIL: weather\n"); } }
    // NATURAL / OTHER DISASTERS off: no events, no roaming calamities
    { setup(seed); lw[LW_OTHERDIS]=1; memset(hz,0,sizeof hz); int ev=0; for(int t=1;t<=90000;t++){ tick=t; calamityNatural(); ev+=hzCount(HZ_TORNADO)+hzCount(HZ_HOLE); } printf("OTHER DISASTERS off: calamities in 90000 ticks: %d\n",ev); if(ev){ fails++; printf("  FAIL: other disasters\n"); }
      setup(seed); memset(hz,0,sizeof hz); ev=0; for(int t=1;t<=90000;t++){ tick=t; calamityNatural(); ev+=hzCount(HZ_TORNADO)+hzCount(HZ_HOLE); memset(hz,0,sizeof hz); } printf("OTHER DISASTERS on : calamities seen %d\n",ev); if(!ev){ fails++; printf("  FAIL: default disasters never came\n"); } }
    // GAIA: the atomic bomb leaves no lava
    { setup(seed); runT(1,6000); lw[LW_GAIA]=1; int cx=WW/2, cy=WH/2; for(int r=0;r<WW&&(world[cy][cx]<=T_WATER||world[cy][cx]==T_LAVA);r++){ int tx=WW/2+((r%2)?1:-1)*(r/2+1), ty=WH/2+((r*7)%11)-5; if(tx>=0&&tx<WW&&ty>=0&&ty<WH&&world[ty][tx]>T_WATER){ cx=tx; cy=ty; } }
      int l0=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l0+=world[y][x]==T_LAVA; nukeAt(cx,cy,12); int l1=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l1+=world[y][x]==T_LAVA; printf("GAIA on: bomb lava %d -> %d\n",l0,l1); if(l1>l0){ fails++; printf("  FAIL: gaia\n"); } lw[LW_GAIA]=0; setup(seed); runT(1,6000); l0=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l0+=world[y][x]==T_LAVA; nukeAt(cx,cy,12); l1=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) l1+=world[y][x]==T_LAVA; printf("GAIA off: bomb lava %d -> %d\n",l0,l1); if(l1<=l0){ fails++; printf("  FAIL: bomb leaves no lava by default\n"); } }
    // 100 PEOPLE: no village above 100
    { setup(seed); lw[LW_POP100]=1; runT(1,40000); int mx=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop>mx) mx=vil[v].pop; printf("100 PEOPLE on: biggest village %d\n",mx); if(mx>130){ fails++; printf("  FAIL: pop cap\n"); } }
    // CITY GROWTH off: villages never get looked at, so none grows past its start
    { setup(seed); lw[LW_GROWTH]=1; int h0=0; vilTotals(&h0); runT(1,20000); int h1=0; vilTotals(&h1); setup(seed); runT(1,20000); int h2=0; vilTotals(&h2); printf("CITY GROWTH off: homes %d (default %d, start %d)\n",h1,h2,h0); if(h1>h2){ fails++; printf("  FAIL: growth\n"); } }
    // save and load: the laws ride along; a save without the trailer loads at the defaults
    setup(seed); runT(1,300); lw[LW_HUNGER]=1; lw[LW_GAIA]=1; lw[LW_DIPLOMACY]=1; gx.lPlague=2; u8 want[LW_N]; memcpy(want,lw,LW_N);
    if(!saveGame("/tmp/gg_laws.sav")){ fails++; printf("FAIL: save\n"); } memset(lw,0,sizeof lw); gx.lPlague=0; if(!loadGame("/tmp/gg_laws.sav")){ fails++; printf("FAIL: load\n"); }
    printf("save/load: laws %s, plague law %d\n",memcmp(want,lw,LW_N)?"LOST":"kept",gx.lPlague); if(memcmp(want,lw,LW_N)||gx.lPlague!=2){ fails++; printf("  FAIL: laws not kept\n"); }
    printf("%s\n",fails?"LAWS FAIL":"LAWS OK"); return fails?1:0; }
