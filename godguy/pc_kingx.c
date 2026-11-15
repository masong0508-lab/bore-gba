// Pack A (rest): tribute and tax policies, the five newer culture traits, Shattered Crown / Equal Heirs, and the GGK1 save trailer.
//   gcc -O2 -DPCTEST -w -o pc_kingx pc_kingx.c -lm && ./pc_kingx
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
static int nKd(void){ int n=0; for(int i=0;i<KN_N;i++) n+=kd[i].on; return n; }
static int nCity(int k){ int c=0; for(int v=0;v<VIL_N;v++) c+=vil[v].on&&vilK[v]==k+1; return c; }
int main(void){
    planetSize=192; rs=4242u*2654435761u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=40000;t++){ tick=t; year=t/1200; simulate(); }
    int k=-1; for(int i=0;i<KN_N;i++) if(kd[i].on&&nCity(i)>=1){ k=i; break; } if(k<0){ printf("  FAIL  no kingdom\nFAILED\n"); return 1; }
    // policies
    int v=-1; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==k+1){ v=u; break; }
    kx[k].tax=0; int m0=vilMood(v); kx[k].tax=1; int mh=vilMood(v); kx[k].tax=2; int ml=vilMood(v); kx[k].tax=0;
    CHECK(mh==m0-8&&ml==m0+8,"tax: high lowers a city's mood by 8, low raises it by 8 (%d / %d / %d)",mh,m0,ml);
    int g0=kd[k].gold; kd[k].gold=100; kx[k].tribute=1; /* income is added in the tick */ 
    CHECK(polSgn(0)==0&&polSgn(1)==1&&polSgn(2)==-1,"policy values map to normal, high, low");
    kx[k].tribute=0;
    kx[k].ctr2&=~4; int ex=kdMaxCities(k); kx[k].ctr2|=4; int ex2=kdMaxCities(k); kx[k].ctr2&=~4; CHECK(ex2==ex+2,"EXPANSIONIST allows two more cities (%d -> %d)",ex,ex2);
    // save and load keep policies and traits; an old save (no trailer) has everything normal
    kx[k].tribute=1; kx[k].tax=2; kx[k].ctr2=0x0B; kx[k].renown=17;
    int ok=saveGame("/tmp/gg_kingx.sav"); memset(kx,0,sizeof kx); int lo=loadGame("/tmp/gg_kingx.sav");
    CHECK(ok&&lo&&kx[k].tribute==1&&kx[k].tax==2&&kx[k].ctr2==0x0B&&kx[k].renown==17,"policies, traits and renown survive save and load");
    memset(kx,0,sizeof kx); newWorld(); int allN=1; for(int i=0;i<KN_N;i++) allN&=kx[i].tribute==0&&kx[i].tax==0&&kx[i].ctr2==0; CHECK(allN,"a new world starts every kingdom normal with no new traits");
    // splitting: grow a kingdom to several cities by hand
    planetSize=192; rs=4242u*2654435761u; newWorld(); for(int t=1;t<=40000;t++){ tick=t; year=t/1200; simulate(); }
    int best=-1,bc=0; for(int i=0;i<KN_N;i++) if(kd[i].on&&nCity(i)>bc){ bc=nCity(i); best=i; }
    printf("  (largest kingdom has %d cities, %d kingdoms)\n",bc,nKd());
    { int fr=0; for(int i=0;i<KN_N&&fr<4;i++) if(kd[i].on&&i!=best){ kdDie(i); fr++; } for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]&&!kd[vilK[u]-1].on) vilK[u]=0; }
    if(bc>=2){ int k2=best, kb=nKd(), cb=nCity(k2); int fr=KN_N-kb; kdSplit(k2,1); int ka=nKd();
        CHECK(ka==kb+1,"EQUAL HEIRS with room makes exactly one new kingdom (%d -> %d)",kb,ka);
        { int cn=0; for(int i=0;i<KN_N;i++) if(kd[i].on&&i!=k2&&kd[i].truce) cn+=nCity(i); CHECK(cn>=1&&cn<=cb/2+1,"the heir gets about half the far cities (%d of %d)",cn,cb); }
        kdSplit(k2,0); CHECK(nKd()>=ka&&nKd()<=KN_N,"SHATTERED CROWN adds kingdoms only as far as slots allow (%d now)",nKd());
        int cap=kd[k2].cap-1; CHECK(cap>=0&&vilK[cap]==k2+1,"the capital stays with the kingdom");
        int tot=0; for(int i=0;i<KN_N;i++) if(kd[i].on) tot+=nCity(i); int all=0; for(int u=0;u<VIL_N;u++) all+=vil[u].on&&vilK[u]; CHECK(tot==all,"every city belongs to exactly one live kingdom"); (void)fr;(void)cb; }
    else printf("  (skipped: no kingdom with two cities at this seed)\n");
    // no free slots: nothing happens
    { int fills=0; for(int i=0;i<KN_N;i++) if(!kd[i].on&&fills<KN_N){ kd[i].on=1; fills++; } Kd save[KN_N]; int cb=0; for(int i=0;i<KN_N;i++) cb+=kd[i].on; kdSplit(best>=0?best:0,0); int ca=0; for(int i=0;i<KN_N;i++) ca+=kd[i].on; CHECK(ca==cb,"SHATTERED CROWN with no free slots changes nothing"); }
    printf(fails?"FAILED\n":"PASSED\n"); return fails?1:0; }
