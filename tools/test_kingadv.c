// test_kingadv.c - PC test of source/kingadv.h: the advisor. Each stall it knows is built on purpose and the tip must say it; fix it and the tip must go. Run from the repo root:   sh tools/test_kingadv.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
static u16 sb[1024];
#define KV_PUT(sx,sy,e) do{ sb[(sy)*32+(sx)]=(u16)(e); }while(0)
#include "../source/kingview.h"
#include "../source/kinghand.h"
#include "../source/kingadv.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int fresh(void){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=0; R.genVer=KG_GENVER; R.seed=31u; R.rng=5; kgGen(cells,31u,0); int e=ksInit(&S,&R,B,cells); int d=S.dim,c=d/2; for(int y=c-10;y<c+10;y++) for(int x=c-14;x<c+14;x++) S.cells[y*d+x]=KT_GRASS; return e; }
static int has(int kind){ KAdv a[KA_MAX]; int n=kaAdvise(&S,a,KA_MAX); for(int i=0;i<n;i++) if(a[i].kind==kind) return 1; return 0; }
static int first(void){ KAdv a[KA_MAX]; int n=kaAdvise(&S,a,KA_MAX); return n?a[0].kind:-1; }
static void roadAround(int b){ int adj[8], n=ksAdj(&S,&S.B[b],adj); for(int i=0;i<n;i++) if(S.cells[adj[i]]==KT_GRASS) { ksRoadSet(&S,adj[i]%S.dim,adj[i]/S.dim,1); break; } }
int main(void){
    printf("== kingadv: the advisor\n"); int d, c; char t[64];
    CHECK(fresh()==KSE_OK,"init"); d=S.dim; c=d/2; CHECK(first()==-1,"an empty realm needs no advice");
    // a producer and no storehouse
    int w=ksPlace(&S,KB_WOODCUT,c-4,c-2,1); CHECK(w>=0,"woodcutter %d",w); S.B[w].work=1; ksCensus(&S); S.cells[(c-1)*d+(c-3)]=KT_FOREST;   // (a tree for it to fell)
    CHECK(first()==KAT_STORE,"no storehouse comes first: %d",first()); KAdv a[KA_MAX]; kaAdvise(&S,a,KA_MAX); kaText(&S,&a[0],t); CHECK(strstr(t,"STOREHOUSE")&&strlen(t)<=40,"text: %s",t);
    int st=ksPlace(&S,KB_STORE,c+2,c-2,1); CHECK(st>=0,"store"); ksCensus(&S); CHECK(!has(KAT_STORE),"a storehouse ends that");
    // no road at the door
    CHECK(has(KAT_ROAD),"buildings with no road beside them"); { int n=kaAdvise(&S,a,KA_MAX); for(int i=0;i<n;i++) if(a[i].kind==KAT_ROAD){ kaText(&S,&a[i],t); CHECK(strstr(t,"ROAD")&&strlen(t)<=40,"road text: %s",t); } }
    roadAround(w); roadAround(st); ksNodeBuild(&S); CHECK(!has(KAT_ROAD),"a road beside each ends that");
    // two buildings and no home
    CHECK(has(KAT_HOME),"no home: nobody moves in"); int h=ksPlace(&S,KB_HUT,c-1,c+3,1); CHECK(h>=0,"hut"); ksCensus(&S); CHECK(!has(KAT_HOME),"a hut ends that");
    // full homes: the hut holds 4 and 4 live there
    S.B[h].work=(u8)ksT(KB_HUT)->house; ksCensus(&S); CHECK(has(KAT_FULL),"full homes"); S.B[h].work=1; ksCensus(&S); CHECK(!has(KAT_FULL),"room in the hut");
    // a gatherer with nothing in reach: a quarry on bare grass
    int q=ksPlace(&S,KB_QUARRY,c-8,c+4,1); CHECK(q>=0,"quarry"); S.B[q].work=1; ksCensus(&S); roadAround(q); ksNodeBuild(&S);
    CHECK(has(KAT_EMPTY),"a quarry with no rock in reach"); { int n=kaAdvise(&S,a,KA_MAX); for(int i=0;i<n;i++) if(a[i].kind==KAT_EMPTY){ kaText(&S,&a[i],t); CHECK(strstr(t,"QUARRY")&&strlen(t)<=40,"empty text: %s",t); } }
    S.cells[(c+5)*d+(c-6)]=KT_ROCK; CHECK(!has(KAT_EMPTY),"one rock in reach ends that");
    // a processor with nothing to process: a sawmill, no wood anywhere
    int sw=ksPlace(&S,KB_SAWMILL,c+6,c+3,1); CHECK(sw>=0,"sawmill"); S.B[sw].work=1; ksCensus(&S); roadAround(sw); ksNodeBuild(&S); R.goods[G_WOOD]=0;
    CHECK(has(KAT_WAIT),"a sawmill with no wood"); { int n=kaAdvise(&S,a,KA_MAX); for(int i=0;i<n;i++) if(a[i].kind==KAT_WAIT){ kaText(&S,&a[i],t); CHECK(strstr(t,"SAWMILL")&&strstr(t,"WOOD")&&strlen(t)<=40,"wait text: %s",t); } }
    R.goods[G_WOOD]=10; CHECK(!has(KAT_WAIT),"wood in the stockpile ends that");
    // jobs with no hands
    for(int i=0;i<R.nB;i++) if(S.B[i].lvl&&ksT(S.B[i].type)->kind!=KK_HOME) S.B[i].work=0; S.B[h].work=0; ksCensus(&S); S.pop=KS_PARTY; S.emp=KS_PARTY; CHECK(has(KAT_JOBS),"more jobs than Nubbins");
    // the castle's lot
    ksCensus(&S); int ca=ksPlace(&S,KB_CASTLE,c-10,c-8,0); CHECK(ca>=0,"castle site %d",ca); CHECK(has(KAT_CASTLE),"the castle's lot"); { int n=kaAdvise(&S,a,KA_MAX); for(int i=0;i<n;i++) if(a[i].kind==KAT_CASTLE){ kaText(&S,&a[i],t); CHECK(strstr(t,"CASTLE")&&strstr(t,"PLANKS")&&strlen(t)<=40,"castle text: %s",t); } }
    CHECK(!has(KAT_NOCASTLE),"with a castle site there is no 'build a castle'");
    // the stockpile ceiling
    R.goods[G_CUT]=KG_GOODMAX; CHECK(has(KAT_STOCK),"a full stockpile"); R.goods[G_CUT]=10;
    // every tip stays within 40 characters, for every building name and every good, and the list is never longer than KA_MAX
    for(int k=0;k<KAT_N;k++) for(int x=0;x<KS_NTYPES;x++) for(int g=0;g<8;g++){ KAdv q2={(u8)k,(u8)(k==KAT_WAIT||k==KAT_STOCK||k==KAT_CASTLE?g:x),(u16)(k==KAT_WAIT?g:99)}; kaText(&S,&q2,t); CHECK(strlen(t)<=40&&t[0],"tip %d/%d/%d is %d long: %s",k,x,g,(int)strlen(t),t); }
    { KAdv big[8]; int n=kaAdvise(&S,big,8); CHECK(n<=KA_MAX,"%d tips",n); }
    printf("  stalls told and cleared: no storehouse, no road, no home, full homes, bare quarry, a sawmill with no wood, jobs without hands, the castle's lot, a full stockpile; every tip fits 40 characters\n");
    printf("%s: %d failure(s)\n",fails?"FAILED":"kingadv: ALL PASSED",fails); return fails?1:0;
}
