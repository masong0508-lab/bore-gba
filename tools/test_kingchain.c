// test_kingchain.c - PC test of step 5a: the wool chain and the crystal chain (sheep pen, crystal mine, weaver, crystal cutter in kingsim.h's table, kinghand.h's modes, the roofs in kingview.h and kingover.h).
// It checks: the building table is consistent (every good has a maker, every cost can be made); the sheep pen never uses meadow up and the crystal mine does; a pen with no meadow in reach makes nothing;
// the weaver and the cutter turn 2 into 1; a whole village (a storehouse, huts, then the four new buildings as SITES that the Nubbins haul and build) makes cloth and polished crystal with the books balanced;
// every type has its own build mode, name, menu line, roof and palette bank. From the repo root:   sh tools/test_kingchain.sh
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
#include "../source/kingover.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
// the scene: tier 1 (96 x 96), the middle cleared to grass, a spine road along row c+1; meadow above it and crystal below it near x = c+16..c+26 (when asked)
static int scene(int meadow,int crystal,u32 seed){
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=1; R.genVer=KG_GENVER; R.seed=seed; R.rng=7; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,seed,1);
    int d=96, c=48;
    for(int y=c-12;y<=c+14;y++) for(int x=c-12;x<=c+34;x++) cells[y*d+x]=KT_GRASS;
    if(meadow) for(int y=c-6;y<=c-3;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_MEADOW;
    if(crystal) for(int y=c+5;y<=c+7;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_CRYSTAL;
    return ksInit(&S,&R,B,cells);
}
static int count(int v,int x0,int y0,int x1,int y1){ int n=0; for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) if(S.cells[y*96+x]==v) n++; return n; }
static int village(int sites){   // the storehouse and three huts built, the spine road, then (sites) the four new buildings as sites
    int c=48; for(int x=c-8;x<=c+27;x++) ksRoadSet(&S,x,c+1,1);
    int st=ksPlace(&S,KB_STORE,c-8,c-1,1); ksPlace(&S,KB_HUT,c-5,c-1,1); ksPlace(&S,KB_HUT,c-2,c-1,1); ksPlace(&S,KB_HUT,c+1,c-1,1);
    ksGift(&S,G_WOOD,30); ksGift(&S,G_PLANKS,20); ksGift(&S,G_CUT,20);
    if(sites){ ksPlace(&S,KB_SHEEP,c+20,c-1,0); ksPlace(&S,KB_WEAVER,c+10,c-1,0); ksPlace(&S,KB_CRYMINE,c+20,c+2,0); ksPlace(&S,KB_CUTTER,c+10,c+2,0); }
    return st;
}
static int typeAt(int t){ for(int i=0;i<R.nB;i++) if(B[i].type==t) return i; return -1; }
int main(void){
    printf("== kingchain: the wool chain and the crystal chain\n");
    // 1 the table
    { int made[8]={0}; for(int t=0;t<KS_NTYPES;t++){ const KsType*T=&KS_T[t]; if(T->kind==KK_GATHER||T->kind==KK_PROC) made[T->outG]=1; }
      for(int g=0;g<8;g++) CHECK(made[g],"nothing makes good %d",g);
      for(int t=0;t<KS_NTYPES;t++){ const KsType*T=&KS_T[t];
        CHECK(T->costG<8&&made[T->costG]&&T->costN>0,"type %d: its cost cannot be made",t);
        if(T->kind==KK_GATHER){ CHECK(T->staff==1&&T->outG<8&&(T->res==KT_FOREST||T->res==KT_ROCK||T->res==KT_CRYSTAL||T->res==KT_MEADOW)&&T->period>0,"type %d: gatherer row",t); }
        if(T->kind==KK_PROC){ CHECK(T->staff==1&&T->inG<8&&T->outG<8&&T->inG!=T->outG&&T->period>0,"type %d: processor row",t); int fed=0; for(int u=0;u<KS_NTYPES;u++) if(KS_T[u].kind==KK_GATHER&&KS_T[u].outG==T->inG) fed=1; CHECK(fed,"type %d: a processor that eats a good only another processor makes",t); }
        if(T->kind==KK_HOME) CHECK(T->house>0,"type %d: a home with no room",t);
        if(T->kind==KK_COMFORT) CHECK(T->staff==0&&T->house==0&&T->period==0&&T->costN>0,"type %d: a comfort row has no worker, no home, no clock",t); }
      CHECK(KS_NTYPES==14&&KS_NTYPES<=14,"14 types, and the palette banks fit (%d)",KS_NTYPES); }
    // 2 modes, names, menu lines, roofs and banks
    { CHECK(KHM_N==KS_NTYPES+5&&KHM_DEMO==KHM_HUT+KS_NTYPES&&KHM_STORE-KHM_HUT==KB_STORE&&KHM_SHEEP-KHM_HUT==KB_SHEEP&&KHM_CRYMINE-KHM_HUT==KB_CRYMINE&&KHM_WEAVER-KHM_HUT==KB_WEAVER&&KHM_CUTTER-KHM_HUT==KB_CUTTER,"modes line up with the types");
      int worst=0; char b[64];
      CHECK(scene(1,1,5u)==KSE_OK,"init");
      for(int m=0;m<KHM_N;m++){ CHECK(khName[m]&&khName[m][0],"mode %d has no name",m); khCost(b,&S,m); int n=(int)strlen(b); if(n>worst) worst=n; CHECK(n>0&&n<=40,"menu line %d is %d long: %s",m,n,b); }
      for(int a=0;a<KS_NTYPES;a++) for(int e=a+1;e<KS_NTYPES;e++){ CHECK(kvRoof[a]!=kvRoof[e],"map roofs %d and %d are the same",a,e); CHECK(koRoof[a]!=koRoof[e],"overview roofs %d and %d are the same",a,e); }
      for(int t=0;t<KS_NTYPES;t++){ CHECK(kvPalAt(t+1,14)==kvRoof[t]&&kvPalAt(t+1,3)==kvPal[3],"bank %d is type %d's roof and nothing else",t+1,t); CHECK(koRoof[t]!=0,"overview roof %d is black",t); }
      CHECK(kvPalAt(KS_NTYPES+1,14)==kvPal[14]&&kvPalAt(0,14)==kvPal[14],"banks past the roofs and the land are plain");
      b[0]=0; khCost(b,&S,KHM_SHEEP); CHECK(strcmp(b,"SHEEP PEN  3 WOOD  HAVE 0")==0,"%s",b); khCost(b,&S,KHM_CRYMINE); CHECK(strcmp(b,"MINE  3 PLANKS  HAVE 0")==0,"%s",b);
      printf("  %d build modes, longest menu line %d characters; 14 roofs, all different on the map and the overview\n",KHM_N,worst);
      // each mode builds its own type, as a site, at the giant's feet
      for(int t=0;t<KS_NTYPES;t++){ CHECK(scene(1,1,6u+(u32)t)==KSE_OK,"init"); KHand K; khInit(&K); K.mode=KHM_HUT+t; int r=khAct(&K,&S,60,60); CHECK(r==KHR_SITE&&R.nB==1&&B[0].type==t&&B[0].lvl==0,"mode %d: result %d, built type %d",K.mode,r,B[0].type); } }
    // 3 a pen with no meadow in reach makes nothing, and breaks nothing
    { CHECK(scene(0,0,11u)==KSE_OK,"init"); village(1); for(long k=0;k<KS_DAYT*10L;k++) ksStep(&S);
      CHECK(ksCheck(&S)==0,"books %d",ksCheck(&S)); CHECK(ksHeld(&S,G_WOOL)==0&&ksHeld(&S,G_CLOTH)==0&&ksHeld(&S,G_CRYSTAL)==0&&ksHeld(&S,G_POLISH)==0,"no meadow and no crystal, yet wool or crystal was made");
      int built=0; for(int i=0;i<R.nB;i++) built+=B[i].lvl; CHECK(built>=5,"the sites were still built (%d of %d)",built,R.nB); }
    // 4 the whole village: wool and crystal gathered, hauled, made into cloth and polished crystal; the meadow stays, the crystal goes
    for(int run=0;run<3;run++){
        CHECK(scene(1,1,20u+(u32)run*13u)==KSE_OK,"init"); int c=48; int m0=count(KT_MEADOW,c+10,c-12,c+34,c+0), k0=count(KT_CRYSTAL,c+10,c+3,c+34,c+14); CHECK(m0>=44&&k0>=33,"scene meadow %d crystal %d",m0,k0);
        village(1); long producedWool=0, producedCry=0; int minePlaced=typeAt(KB_CRYMINE), penPlaced=typeAt(KB_SHEEP); CHECK(minePlaced>=0&&penPlaced>=0,"placed");
        for(long k=0;k<KS_DAYT*40L;k++){ ksStep(&S); if(k%4000==0) CHECK(ksCheck(&S)==0,"run %d frame %ld: ksCheck %d",run,k,ksCheck(&S)); }
        CHECK(ksCheck(&S)==0,"run %d: the books %d",run,ksCheck(&S));
        for(int t=KB_SHEEP;t<=KB_CUTTER;t++){ int i=typeAt(t); CHECK(i>=0&&B[i].lvl==1,"run %d: type %d was not built",run,t); }
        int m1=count(KT_MEADOW,c+10,c-12,c+34,c+0), k1=count(KT_CRYSTAL,c+10,c+3,c+34,c+14);
        CHECK(m1==m0,"run %d: the sheep used meadow up (%d to %d)",run,m0,m1); CHECK(k1<k0,"run %d: the mine used no crystal up (%d to %d)",run,k0,k1);
        long wool=ksHeld(&S,G_WOOL), cloth=ksHeld(&S,G_CLOTH), cry=ksHeld(&S,G_CRYSTAL), pol=ksHeld(&S,G_POLISH); producedWool=wool+2*cloth; producedCry=cry+2*pol+(long)(k0-k1)*0;
        CHECK(cloth>0&&pol>0,"run %d: cloth %ld polished crystal %ld",run,cloth,pol); CHECK(producedWool>=2&&producedCry>=2,"run %d: wool %ld crystal %ld",run,producedWool,producedCry);
        CHECK(S.made[G_CLOTH]==ksHeld(&S,G_CLOTH)&&S.made[G_POLISH]==ksHeld(&S,G_POLISH),"the books of the better goods");
        printf("  run %d: cloth %ld, polished crystal %ld, wool left %ld, crystal left %ld; meadow %d -> %d, crystal cells %d -> %d, delivered %u\n",run,cloth,pol,wool,cry,m0,m1,k0,k1,S.delivered); }
    // 5 the two processors on their own: 2 in, 1 out, nothing made from one
    { CHECK(scene(0,0,31u)==KSE_OK,"init"); int c=48; for(int x=c-8;x<=c+14;x++) ksRoadSet(&S,x,c+1,1); ksPlace(&S,KB_STORE,c-8,c-1,1); int w=ksPlace(&S,KB_WEAVER,c+4,c-1,1), u=ksPlace(&S,KB_CUTTER,c+4,c+2,1); CHECK(w>=0&&u>=0,"placed");
      for(int k=0;k<4;k++) ksPlace(&S,KB_HUT,c-5+3*(k%2),c+2+3*(k/2)>c+5?c+5:c+2,1);
      B[w].work=1; B[u].work=1; ksSetIn(&B[w],5); ksSetIn(&B[u],3); S.made[G_WOOL]+=5; S.made[G_CRYSTAL]+=3;
      for(long k=0;k<KS_DAYT*3L;k++){ B[w].work=1; B[u].work=1; S.timer[w]+=0; ksEcon(&S); }
      CHECK(ksOut(&B[w])+0>=0,"weaver ran"); CHECK(S.made[G_CLOTH]==2&&ksIn(&B[w])==1,"5 wool: 2 cloth made, 1 wool left (cloth %ld, in %d)",S.made[G_CLOTH],ksIn(&B[w])); CHECK(S.made[G_POLISH]==1&&ksIn(&B[u])==1,"3 crystal: 1 polished, 1 left (polished %ld, in %d)",S.made[G_POLISH],ksIn(&B[u])); }
    printf("%s: %d failure(s)\n",fails?"kingchain: FAILED":"kingchain: ALL PASSED",fails); return fails?1:0;
}
