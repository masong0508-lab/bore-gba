// test_kinggiants.c - PC test of step 5e: the four giants' stats (kinghand.h khStatT) and the builder's rebate. From the repo root:   sh tools/test_kinggiants.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
#include "../source/kinghand.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<25){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static void fresh(int giant){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=1; R.genVer=KG_GENVER; R.seed=77; R.rng=5; R.giant=(u8)giant; kgGen(cells,77,1); CHECK(ksInit(&S,&R,B,cells)==KSE_OK,"init");
    int d=S.dim, m=d/2-12; for(int y=m;y<m+24;y++) for(int x=m;x<m+24;x++) S.cells[y*d+x]=KT_GRASS; }
int main(void){
    printf("== kinggiants: the stats\n");
    // the table: every giant inside the small range, none best, the roadmap's up and down
    int tot[5]; for(int g=0;g<5;g++){ tot[g]=0; for(int c=0;c<4;c++) tot[g]+=khStat(g,c); CHECK(khStride(g)>=2&&khStride(g)<=4&&khLoad(0)==5&&khYield(g)>=1&&khYield(g)<=3&&khBuild(g)>=1&&khBuild(g)<=3,"range %d",g); }
    CHECK(khStride(1)>khStride(0)&&khLoad(1)<khLoad(0)&&khYield(1)==khYield(0)&&khBuild(1)==khBuild(0),"B: faster, carries less, otherwise as A");
    CHECK(khStride(2)<khStride(0)&&khLoad(2)>khLoad(0)&&khYield(2)==khYield(0)&&khBuild(2)==khBuild(0),"C: slower, carries more, otherwise as A");
    CHECK(khStride(3)==khStride(0)&&khLoad(3)==khLoad(0)&&khYield(3)<khYield(0)&&khBuild(3)>khBuild(0),"D: gathers less, builds better, otherwise as A");
    CHECK(tot[4]==tot[0],"your own Bore starts average");
    for(int g=1;g<4;g++) for(int h=0;h<4;h++) if(g!=h){ int better=0; for(int c=0;c<4;c++) better|=khStat(g,c)>khStat(h,c); CHECK(better,"giant %d is not just worse than %d",h,g); }
    CHECK(khStat(9,0)==khStat(0,0)&&khStat(-1,3)==khStat(0,3)&&khBuild(7)==khBuild(0),"a bad giant number falls back to the first");
    CHECK(khRun(1)<8&&khRun(0)<8&&khRun(2)<8,"running never skips a cell");
    // the builder's rebate: a site finished by hand gives back build-1 goods; others get none; the books balance throughout
    for(int g=0;g<5;g++){
        fresh(g); int d=S.dim, m=d/2-12; ksGift(&S,G_WOOD,30);
        int st=ksPlace(&S,KB_STORE,m+2,m+2,1), hut=ksPlace(&S,KB_HUT,m+8,m+2,0); CHECK(st>=0&&hut>=0,"placed"); ksCensus(&S);
        int cg=0,cn=0; ksCost(&S,&S.B[hut],&cg,&cn); long before=R.goods[cg];
        KHand H; H.mode=KHM_HAND; CHECK(khAct(&H,&S,m+2,m+2)==KHR_TAKE,"take (giant %d)",g);
        int took=S.hd.n; CHECK(took==cn||took==khLoad(g),"load %d",took);
        int r=khAct(&H,&S,m+8,m+2); CHECK(r==KHR_GIVE,"give (giant %d)",g);
        long expect=before-cn+(khBuild(g)-1);   // finished by one load only when the load covers the cost
        if(took>=cn){ CHECK(R.goods[cg]==expect,"giant %d: pile %ld, want %ld",g,(long)R.goods[cg],expect); CHECK(ksIn(&S.B[hut])==cn,"site full"); }
        CHECK(ksCheck(&S)==0,"books (giant %d: %d)",g,ksCheck(&S));
        for(int k=0;k<6;k++) ksStep(&S); CHECK(S.B[hut].lvl==1&&ksCheck(&S)==0,"the hut is raised, books balanced");
        printf("  giant %d: stride %d carry %d work %d build %d | hut cost %d %s, pile %ld -> %ld\n",g,khStride(g),khLoad(g),khYield(g),khBuild(g),cn,took>=cn?"paid in one load":"needs two loads",before,(long)R.goods[cg]);
    }
    // a partial delivery (not finishing the site) gives nothing back, even to the builder
    fresh(3); { int d=S.dim, m=d/2-12; ksGift(&S,G_WOOD,30); ksPlace(&S,KB_STORE,m+2,m+2,1); int sw=ksPlace(&S,KB_SAWMILL,m+8,m+2,0); ksCensus(&S);
      int cg=0,cn=0; ksCost(&S,&S.B[sw],&cg,&cn); KHand H; H.mode=KHM_HAND; khAct(&H,&S,m+2,m+2); CHECK(S.hd.k==KHD_GOODS&&S.hd.g==cg&&S.hd.n>1,"a load of the sawmill's good"); ksPool(&S,cg,S.hd.n-1); S.hd.n=1; long pile=R.goods[cg];
      CHECK(cn>1,"the sawmill costs more than 1 (%d)",cn); CHECK(khAct(&H,&S,m+8,m+2)==KHR_GIVE&&(long)R.goods[cg]==pile&&ksCheck(&S)==0,"a part delivery gives no rebate"); }
    printf("kinggiants: %s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}
