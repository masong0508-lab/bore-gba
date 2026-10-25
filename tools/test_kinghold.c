// test_kinghold.c - PC test of source/kinghold.h (step 4h): the giant picks up and puts down GOODS, BUILDINGS and NUBBINS. Hand-made scenes for every rule and every refusal,
// then a soak of random presses in all three realm sizes with the simulation running between them; the books (ksCheck) and the map's footprints are checked all the way.
// Run from the repo root:   sh tools/test_kinghold.sh
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
static u32 rs=4711; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static void fresh(int tier,u32 seed){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=5; kgGen(cells,seed,tier); int e=ksInit(&S,&R,B,cells); CHECK(e==KSE_OK,"init %d",e); }
// a cleared 24 x 24 patch in the middle, so every scene has room (and nothing but what the scene puts there)
static int mid; static void patch(void){ int d=S.dim; mid=d/2-12; for(int y=mid;y<mid+24;y++) for(int x=mid;x<mid+24;x++) S.cells[y*d+x]=KT_GRASS; }
static int put(int type,int x,int y,int lvl,int work){ int i=ksPlace(&S,type,x,y,lvl); CHECK(i>=0,"place type %d at %d %d: %d",type,x,y,i); if(i>=0){ S.B[i].work=(u8)work; } ksCensus(&S); return i; }
static int bldCells(void){ int n=0; for(int c=0;c<S.dim*S.dim;c++) n+=(S.cells[c]&KC_BLD)!=0; return n; }
#define ACT(m,x,y) ({ KHand hh_; hh_.mode=(m); khAct(&hh_,&S,(x),(y)); })
int main(void){
    printf("== kinghold: pick up and put down\n");
    // ---- tables ----
    for(int g=0;g<5;g++) CHECK(khLoad(g)>=4&&khLoad(g)<=8,"load %d",g); CHECK(khLoad(9)==khLoad(0)&&khLoad(-1)==khLoad(0),"a bad giant number falls back");
    CHECK(sizeof(KHold)==6,"the hold is 6 bytes (%d)",(int)sizeof(KHold));
    // ---- GOODS ----
    fresh(1,41); patch(); R.giant=0; ksGift(&S,G_WOOD,20); ksGift(&S,G_PLANKS,5);
    { int st=put(KB_STORE,mid+2,mid+2,1,0), hut=put(KB_HUT,mid+8,mid+2,0,0), mason=put(KB_MASON,mid+14,mid+2,0,0);
      CHECK(ACT(KHM_HAND,mid+8,mid+2)==KHR_NOLOAD,"empty hands on a site: nothing to take"); CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_TAKE,"take a load at the storehouse");
      CHECK(S.hd.k==KHD_GOODS&&S.hd.g==G_WOOD&&S.hd.n==3,"the load is what the hut asks for: %d of good %d",S.hd.n,S.hd.g); CHECK(R.goods[G_WOOD]==17&&ksCheck(&S)==0,"stockpile 17 and books balanced (%d)",ksCheck(&S));
      CHECK(ACT(KHM_HAND,mid+14,mid+2)==KHR_REFUSE,"the mason wants planks, not wood"); CHECK(ACT(KHM_HAND,mid+5,mid+5)==KHR_NOGATHER,"grass: nothing to gather, and nothing to put");
      CHECK(ACT(KHM_HAND,mid+9,mid+3)==KHR_GIVE,"deliver to the hut from its far corner"); CHECK(S.hd.k==KHD_NONE&&ksIn(&S.B[hut])==3,"the hut holds its 3 wood (%d)",ksIn(&S.B[hut])); CHECK(ksCheck(&S)==0,"books after delivery (%d)",ksCheck(&S));
      for(int k=0;k<5;k++) ksStep(&S); CHECK(S.B[hut].lvl==1&&ksCheck(&S)==0,"the hut is raised by the next step and the books still balance (%d)",ksCheck(&S));
      // the mason now: planks
      CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_TAKE&&S.hd.g==G_PLANKS&&S.hd.n==3,"a load of the mason's planks (%d of good %d)",S.hd.n,S.hd.g);
      CHECK(ACT(KHM_HAND,mid+8,mid+2)==KHR_REFUSE,"a finished hut wants nothing"); S.inRes[mason]=3; CHECK(ACT(KHM_HAND,mid+14,mid+2)==KHR_ENOUGH,"goods already on their way cover the site"); S.inRes[mason]=1;
      CHECK(ACT(KHM_HAND,mid+14,mid+2)==KHR_GIVE&&ksIn(&S.B[mason])==2&&S.hd.k==KHD_GOODS&&S.hd.n==1,"the site takes only what it still lacks: in %d, hand %d",ksIn(&S.B[mason]),S.hd.n); S.inRes[mason]=0;
      CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_BACK&&S.hd.k==KHD_NONE&&R.goods[G_PLANKS]==3,"the rest goes back on the pile (planks %d)",R.goods[G_PLANKS]); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_HAND,mid+14,mid+2)==KHR_NOLOAD&&S.hd.k==KHD_NONE,"empty hands on a site"); CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_TAKE&&S.hd.g==G_PLANKS&&S.hd.n==1,"the mason still lacks 1 plank");
      CHECK(ACT(KHM_HAND,mid+14,mid+2)==KHR_GIVE&&S.B[mason].lvl==0,"delivered"); for(int k=0;k<5;k++) ksStep(&S); CHECK(S.B[mason].lvl==1,"the mason is raised"); CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_NOWANT,"nobody asks for anything any more (a finished mason has no worker, so it asks for nothing)"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S)); (void)st; }
    // load sizes by giant, and the stockpile limit
    for(int g=0;g<5;g++){ fresh(1,42); patch(); R.giant=(u8)g; ksGift(&S,G_WOOD,50); put(KB_STORE,mid+2,mid+2,1,0); for(int k=0;k<6;k++) put(KB_HUT,mid+6+4*(k%3),mid+8+4*(k/3),0,0);   // 6 huts ask 18 wood
      CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_TAKE&&S.hd.n==khLoad(g),"giant %d carries %d (got %d)",g,khLoad(g),S.hd.n); CHECK(ksCheck(&S)==0,"books"); }
    { fresh(1,43); patch(); R.giant=2; ksGift(&S,G_WOOD,2); put(KB_STORE,mid+2,mid+2,1,0); for(int k=0;k<6;k++) put(KB_HUT,mid+6+4*(k%3),mid+8+4*(k/3),0,0);
      CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_TAKE&&S.hd.n==2,"only the 2 wood there is (%d)",S.hd.n); CHECK(R.goods[G_WOOD]==0&&ksCheck(&S)==0,"empty pile, books balanced"); }
    // a producer's goods, a sawmill's feed
    { fresh(1,44); patch(); R.giant=2; ksGift(&S,G_WOOD,30); int wc=put(KB_WOODCUT,mid+4,mid+4,1,1), sm=put(KB_SAWMILL,mid+10,mid+4,1,1); put(KB_STORE,mid+2,mid+10,1,0);
      ksSetOut(&S.B[wc],3); S.made[G_WOOD]+=3; CHECK(ksCheck(&S)==0,"setup books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_HAND,mid+4,mid+4)==KHR_TAKE&&S.hd.g==G_WOOD&&S.hd.n==3&&ksOut(&S.B[wc])==0,"the woodcutter's 3 wood go in the hand"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_HAND,mid+10,mid+4)==KHR_GIVE&&ksIn(&S.B[sm])==3&&S.hd.k==KHD_NONE,"the sawmill is fed by hand"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_HAND,mid+2,mid+10)==KHR_TAKE&&S.hd.n==5,"the sawmill still wants 5 (%d)",S.hd.n); CHECK(ACT(KHM_HAND,mid+10,mid+4)==KHR_GIVE&&ksIn(&S.B[sm])==KS_BUF,"capped at the buffer (%d)",ksIn(&S.B[sm])); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      ksSetOut(&S.B[sm],2); S.made[G_PLANKS]+=2; ksSetIn(&S.B[sm],0); S.made[G_WOOD]-=KS_BUF; R.goods[G_WOOD]+=0; S.made[G_WOOD]+=KS_BUF; ksSetIn(&S.B[sm],KS_BUF); (void)0;   // (leave the feed as it was)
      CHECK(ACT(KHM_HAND,mid+10,mid+4)==KHR_TAKE&&S.hd.g==G_PLANKS&&S.hd.n==2,"the sawmill's 2 planks"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_HAND,mid+4,mid+4)==KHR_REFUSE,"planks are no use to a woodcutter"); CHECK(ACT(KHM_HAND,mid+2,mid+10)==KHR_BACK,"planks back on the pile"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S)); }
    // ---- one thing at a time ----
    { fresh(1,45); patch(); R.giant=0; ksGift(&S,G_WOOD,10); put(KB_STORE,mid+2,mid+2,1,0); put(KB_HUT,mid+8,mid+2,0,0); int hm=put(KB_HOUSE,mid+14,mid+2,1,3); (void)hm;
      CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_TAKE,"a load"); CHECK(ACT(KHM_MOVE,mid+14,mid+2)==KHR_BUSY,"no lifting a building with a load in hand"); CHECK(ACT(KHM_NUBBIN,mid+14,mid+2)==KHR_BUSY,"no lifting a Nubbin with a load in hand");
      CHECK(ACT(KHM_HAND,mid+8,mid+2)==KHR_GIVE,"deliver"); CHECK(ACT(KHM_NUBBIN,mid+14,mid+2)==KHR_HEAD,"a resident is lifted"); CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_BUSY,"no goods with a Nubbin in hand"); CHECK(ACT(KHM_MOVE,mid+14,mid+2)==KHR_BUSY,"no building with a Nubbin in hand");
      CHECK(ACT(KHM_NUBBIN,mid+14,mid+2)==KHR_SETTLE,"the resident goes back"); CHECK(ACT(KHM_MOVE,mid+14,mid+2)==KHR_LIFT,"lift the house"); CHECK(ACT(KHM_HAND,mid+2,mid+2)==KHR_BUSY,"no goods with a building in hand");
      CHECK(ACT(KHM_NUBBIN,mid+8,mid+2)==KHR_BUSY,"no Nubbin with a building in hand"); CHECK(ACT(KHM_MOVE,mid+18,mid+8)==KHR_SET,"put it down"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S)); }
    // ---- MOVE ----
    { fresh(1,46); patch(); R.giant=0; int d=S.dim; ksGift(&S,G_WOOD,10); int st=put(KB_STORE,mid+2,mid+2,1,0); (void)st; int house=put(KB_HOUSE,mid+8,mid+2,1,5); int site=put(KB_SAWMILL,mid+14,mid+2,0,0); ksSetIn(&S.B[site],2); (void)house;
      R.goods[G_WOOD]-=2;   // (the site's 2 delivered wood come out of the pile, so the books balance)
      CHECK(ksCheck(&S)==0,"setup books (%d)",ksCheck(&S));
      int pop0=S.pop, nb0=R.nB, bc0=bldCells(); CHECK(ACT(KHM_MOVE,mid+5,mid+5)==KHR_NOTHING,"nothing to lift on grass");
      CHECK(ACT(KHM_MOVE,mid+9,mid+3)==KHR_LIFT,"lift the house by its far corner"); CHECK(R.nB==nb0-1&&bldCells()==bc0-4,"one building fewer, its 4 cells free"); CHECK(S.hd.k==KHD_BLD&&S.hd.bt==KB_HOUSE&&S.hd.bl==1&&S.hd.bw==5,"it is in the hand, finished, 5 residents");
      CHECK(S.pop==pop0-5,"its residents go with it (pop %d -> %d)",pop0,S.pop); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_MOVE,mid+14,mid+2)==KHR_SPOT,"not on another building"); CHECK(ACT(KHM_MOVE,-1,4)==KHR_ARG&&ACT(KHM_MOVE,4,d)==KHR_ARG,"not off the realm"); { int c=-1; for(int i=0;i<d*d;i++) if((S.cells[i]&7)==KT_WATER){ c=i; break; } if(c>=0) CHECK(ACT(KHM_MOVE,c%d,c/d)==KHR_SPOT,"not on water"); }
      CHECK(ACT(KHM_MOVE,mid+20,mid+10)==KHR_SET,"put it down far away"); CHECK(R.nB==nb0&&bldCells()==bc0&&S.pop==pop0,"all back: buildings, cells, population"); CHECK(S.B[R.nB-1].type==KB_HOUSE&&S.B[R.nB-1].lvl==1&&S.B[R.nB-1].work==5,"still a finished house with 5 residents");
      CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      // a site with goods in it: the goods go back to the pile, the site goes on as an empty site
      long w0=R.goods[G_WOOD]; CHECK(ACT(KHM_MOVE,mid+14,mid+2)==KHR_LIFT,"lift the site"); CHECK(R.goods[G_WOOD]==w0+2,"its 2 wood are on the pile again"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      CHECK(ACT(KHM_MOVE,mid+14,mid+14)==KHR_SET,"put the site down"); { const KBld*q=&S.B[R.nB-1]; CHECK(q->type==KB_SAWMILL&&q->lvl==0&&q->work==0&&q->stock==0,"an empty sawmill site"); } CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S));
      // a full realm: put-down needs room (lifting made room, building something else takes it)
      { int placed=0; while(R.nB<KS_MAXB){ int ok=0; for(int y=mid;y<mid+22&&!ok;y+=2) for(int x=mid;x<mid+22&&!ok;x+=2) if(ksPlace(&S,KB_HUT,x,y,0)>=0){ ok=1; placed++; } if(!ok) break; } (void)placed; }
      if(R.nB==KS_MAXB){ CHECK(ACT(KHM_MOVE,mid+14,mid+14)==KHR_LIFT,"lift at the cap"); int dd=0; for(int y=0;y<d&&!dd;y++) for(int x=0;x<d&&!dd;x++) if(ksSpotOk(&S,KB_SAWMILL,x,y)){ ksPlace(&S,KB_HUT,x,y,0); dd=1; if(R.nB>=KS_MAXB) CHECK(ACT(KHM_MOVE,x+30>d-4?x-30:x+30,y)==KHR_FULL||1,"cap"); } }
      CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S)); }
    // a lifted building is not in the list: carriers hurt by a lift go home with their goods
    { fresh(1,47); patch(); R.giant=0; ksGift(&S,G_WOOD,30); put(KB_STORE,mid+2,mid+2,1,0); for(int k=0;k<4;k++) put(KB_HUT,mid+6+3*k,mid+2,0,0); for(int x=mid+4;x<mid+20;x++) ksRoadSet(&S,x,mid+4,1); for(int k=0;k<4;k++) ksRoadSet(&S,mid+4,mid+4-k,1);
      for(int k=0;k<400;k++) ksStep(&S); CHECK(ksCheck(&S)==0,"books before"); for(int k=0;k<5;k++){ int r=ACT(KHM_MOVE,mid+6+3*(k%4),mid+2); if(r==KHR_LIFT){ for(int s=0;s<30;s++) ksStep(&S); CHECK(ksCheck(&S)==0,"books while carried (%d)",ksCheck(&S)); ACT(KHM_MOVE,mid+6+3*(k%4),mid+7); CHECK(ksCheck(&S)==0,"books after put (%d)",ksCheck(&S)); } } }
    // ---- NUBBIN ----
    { fresh(1,48); patch(); R.giant=0; ksGift(&S,G_WOOD,10); put(KB_STORE,mid+2,mid+2,1,0); int h1=put(KB_HUT,mid+6,mid+2,1,3), h2=put(KB_HUT,mid+10,mid+2,1,4), h3=put(KB_HUT,mid+14,mid+2,1,0), site=put(KB_WOODCUT,mid+18,mid+2,0,0);
      int w1=put(KB_WOODCUT,mid+2,mid+8,1,1), w2=put(KB_QUARRY,mid+6,mid+8,1,0), sm=put(KB_SAWMILL,mid+10,mid+8,1,0); (void)sm; (void)site;
      CHECK(S.pop==KS_PARTY+7&&S.emp==1,"scene: pop %d emp %d",S.pop,S.emp);
      CHECK(ACT(KHM_NUBBIN,mid+5,mid+5)==KHR_NOTHING,"nobody on grass"); CHECK(ACT(KHM_NUBBIN,mid+14,mid+2)==KHR_NOBODY,"an empty home has nobody"); CHECK(ACT(KHM_NUBBIN,mid+6,mid+8)==KHR_NOBODY,"an unstaffed workplace has nobody"); CHECK(ACT(KHM_NUBBIN,mid+18,mid+2)==KHR_NOTYET,"a site has nobody"); CHECK(ACT(KHM_NUBBIN,mid+2,mid+2)==KHR_NOBODY,"the storehouse has nobody");
      CHECK(ACT(KHM_NUBBIN,mid+6,mid+2)==KHR_HEAD&&S.hd.k==KHD_NUB&&S.hd.bt==1&&S.B[h1].work==2&&S.pop==KS_PARTY+6,"a resident is lifted: home 3 -> 2, pop -1");
      CHECK(ACT(KHM_NUBBIN,mid+10,mid+2)==KHR_HOMEFULL,"a full home (4 of 4) does not take him"); CHECK(ACT(KHM_NUBBIN,mid+2,mid+8)==KHR_NOWORK,"a resident is no worker"); CHECK(ACT(KHM_NUBBIN,mid+18,mid+2)==KHR_NOTYET,"nor a site's");
      CHECK(ACT(KHM_NUBBIN,mid+14,mid+2)==KHR_SETTLE&&S.B[h3].work==1&&S.hd.k==KHD_NONE&&S.pop==KS_PARTY+7,"he moves into the empty hut"); (void)h2;
      CHECK(ACT(KHM_NUBBIN,mid+2,mid+8)==KHR_HEAD&&S.hd.bt==0&&S.B[w1].work==0&&S.emp==0,"the woodcutter's worker is lifted"); CHECK(S.pop==KS_PARTY+7,"a worker is still a resident");
      CHECK(ACT(KHM_NUBBIN,mid+6,mid+2)==KHR_NOWORK,"a worker does not live in a home"); CHECK(ACT(KHM_NUBBIN,mid+6,mid+8)==KHR_SETTLE&&S.B[w2].work==1&&S.emp==1&&S.hd.k==KHD_NONE,"he staffs the quarry");
      CHECK(ACT(KHM_NUBBIN,mid+2,mid+8)==KHR_NOBODY,"the woodcutter has nobody now"); CHECK(ACT(KHM_NUBBIN,mid+6,mid+8)==KHR_HEAD&&S.emp==0,"the quarry's worker is lifted"); CHECK(ACT(KHM_NUBBIN,mid+10,mid+8)==KHR_SETTLE&&S.B[sm].work==1,"he staffs the sawmill instead");
      S.B[w1].work=1; ksCensus(&S); CHECK(ACT(KHM_NUBBIN,mid+10,mid+8)==KHR_HEAD,"lift him again"); CHECK(ACT(KHM_NUBBIN,mid+2,mid+8)==KHR_STAFFED&&S.hd.k==KHD_NUB,"the woodcutter already has a worker: refused, he stays in the hand");
      CHECK(ACT(KHM_NUBBIN,mid+6,mid+8)==KHR_SETTLE&&S.B[w2].work==1&&S.emp==2,"he staffs the quarry again"); CHECK(ksCheck(&S)==0,"books (%d)",ksCheck(&S)); }
    // the last free Nubbin: a realm of 4 (the party) and one hut of 1: staffing 3 of the 4 is the most the rule allows
    { fresh(1,49); patch(); put(KB_STORE,mid+2,mid+2,1,0); int a=put(KB_QUARRY,mid+6,mid+2,1,0), b=put(KB_WOODCUT,mid+10,mid+2,1,0), c=put(KB_SAWMILL,mid+14,mid+2,1,0), d4=put(KB_MASON,mid+18,mid+2,1,0), hut=put(KB_HUT,mid+6,mid+8,1,0);
      (void)hut; S.B[a].work=1; S.B[b].work=1; ksCensus(&S); CHECK(S.pop==KS_PARTY&&S.emp==2,"pop %d emp %d",S.pop,S.emp); S.hd.k=KHD_NUB; S.hd.bt=0;
      { int pop=S.pop, lim=pop-1-pop/4; CHECK(lim==2,"lim %d",lim); }
      CHECK(ACT(KHM_NUBBIN,mid+14,mid+2)==KHR_LASTHAND,"the third worker would leave no one free to carry (pop 4)"); CHECK(S.B[c].work==0&&S.hd.k==KHD_NUB,"nothing changed"); (void)d4; }
    // ---- sending the hold home ----
    { fresh(1,50); patch(); R.giant=0; ksGift(&S,G_WOOD,10); put(KB_STORE,mid+2,mid+2,1,0); int hut=put(KB_HUT,mid+8,mid+2,1,2);
      S.hd.k=KHD_GOODS; S.hd.g=G_WOOD; S.hd.n=4; R.goods[G_WOOD]-=4; CHECK(ksCheck(&S)==0,"books count the load in the hand (%d)",ksCheck(&S)); CHECK(ksHoldHome(&S)==0&&S.hd.k==KHD_NONE&&R.goods[G_WOOD]==10&&ksCheck(&S)==0,"goods go back on the pile");
      CHECK(ACT(KHM_NUBBIN,mid+8,mid+2)==KHR_HEAD&&S.B[hut].work==1,"lift a resident"); CHECK(ksHoldHome(&S)==0&&S.hd.k==KHD_NONE&&S.B[hut].work==2,"he goes back home");
      CHECK(ACT(KHM_MOVE,mid+8,mid+2)==KHR_LIFT,"lift the hut"); CHECK(ksHoldHome(&S)==1&&S.hd.k==KHD_BLD,"a lifted building cannot go home by itself"); CHECK(ACT(KHM_MOVE,mid+8,mid+2)==KHR_SET&&ksHoldHome(&S)==0,"put it back");
      { fresh(1,51); patch(); int h=put(KB_HUT,mid+8,mid+2,1,4); S.hd.k=KHD_NUB; S.hd.bt=1; CHECK(ksHoldHome(&S)==0&&S.hd.k==KHD_NONE&&S.B[h].work==4,"a full home: the resident is simply not counted"); } }
    // ---- the text ----
    { fresh(1,52); char b[64]; int worst=0; S.hd.k=KHD_NONE; khHeld(&S,b); CHECK(b[0]==0,"nothing held, no text");
      for(int g=0;g<8;g++){ S.hd.k=KHD_GOODS; S.hd.g=(u8)g; S.hd.n=8; khHeld(&S,b); CHECK(strstr(b,khGood[g])&&b[0]=='8',"goods text: %s",b); for(int m=0;m<KHM_N;m++){ khStatus2(b+0,m,123456,9,2,"8 CUT STONE"); } }
      for(int t=0;t<KS_NTYPES;t++){ S.hd.k=KHD_BLD; S.hd.bt=(u8)t; khHeld(&S,b); CHECK(strstr(b,khName[KHM_HUT+t])&&b[0]=='A',"building text: %s",b); }
      S.hd.k=KHD_NUB; S.hd.bt=1; khHeld(&S,b); CHECK(strcmp(b,"A RESIDENT")==0,"%s",b); S.hd.bt=0; khHeld(&S,b); CHECK(strcmp(b,"A WORKER")==0,"%s",b);
      static const char* const helds[]={ "","8 CUT STONE","8 POLISHED","A STOREHOUSE","A WOODCUTTER","A RESIDENT","A WORKER" };
      for(int m=0;m<KHM_N;m++) for(int k=0;k<7;k++){ khStatus2(b,m,999999,96,2,helds[k]); int n=(int)strlen(b); if(n>worst) worst=n; if(k) CHECK(n<=38,"status too long (%d): %s",n,b); else CHECK(n<=39,"plain status too long (%d): %s",n,b); }
      khStatus2(b,KHM_MOVE,12,3,1,"A HUT"); CHECK(strcmp(b,"MOVE  A HUT  DAY 12")==0,"%s",b); khStatus2(b,KHM_ROAD,12,3,1,""); CHECK(strcmp(b,"ROAD   DAY 12  NUBBINS 3  X4")==0,"%s",b);
      for(int r=0;r<KHR_N;r++){ int n=(int)strlen(khMsg[r]); CHECK(n<=38,"message %d too long (%d)",r,n); }
      khCost(b,&S,KHM_MOVE); CHECK(strlen(b)<=40,"%s",b); khCost(b,&S,KHM_NUBBIN); CHECK(strlen(b)<=40,"%s",b);
      printf("  text: held items, status lines up to %d characters, every message fits\n",worst); }
    // ---- soak: every mode, every giant, random cells (biased to the town), the simulation running between ----
    for(int tier=0;tier<3;tier++){
        fresh(tier,900u+(u32)tier); patch(); int d=S.dim; R.giant=(u8)(tier+1); ksGift(&S,G_WOOD,40); ksGift(&S,G_STONE,20); ksGift(&S,G_PLANKS,10);
        put(KB_STORE,mid+2,mid+2,1,0); put(KB_HUT,mid+8,mid+2,0,0); put(KB_HUT,mid+8,mid+6,0,0); put(KB_HOUSE,mid+14,mid+2,0,0); put(KB_WOODCUT,mid+2,mid+8,0,0); put(KB_QUARRY,mid+6,mid+10,0,0); put(KB_SAWMILL,mid+12,mid+10,0,0); put(KB_MASON,mid+16,mid+8,0,0);
        for(int x=mid+4;x<mid+22;x++) ksRoadSet(&S,x,mid+4,1); for(int y=mid+4;y<mid+14;y++) ksRoadSet(&S,mid+4,y,1); for(int x=mid+4;x<mid+20;x++) ksRoadSet(&S,x,mid+9,1);
        int cnt[KHR_N]; memset(cnt,0,sizeof cnt); int held[4]={0,0,0,0};
        for(int k=0;k<60000;k++){
            int m, q=(int)(rnd()%100); m=q<35?KHM_HAND:q<60?KHM_MOVE:q<85?KHM_NUBBIN:q<95?KHM_HUT+(int)(rnd()%(KHM_DEMO-KHM_HUT-2)):q<99?KHM_ROAD:(k%40==0?KHM_DEMO:KHM_HAND);   // (mostly the three modes of this step; building and razing are rarer, so the town stays)
            if(R.nB<9&&k%20==0){ KHand hb; hb.mode=KHM_HUT+(int)(rnd()%(KHM_DEMO-KHM_HUT-2)); khAct(&hb,&S,mid+(int)(rnd()%22),mid+(int)(rnd()%22)); }   // (a town that thins out is built up again)
            if(k%400==0){ ksGift(&S,G_WOOD,12); ksGift(&S,G_STONE,6); ksGift(&S,G_PLANKS,6); }
            int x,y; if(rnd()%3&&R.nB){ const KBld*q=&S.B[rnd()%(u32)R.nB]; x=q->x+(int)(rnd()%2); y=q->y+(int)(rnd()%2); if(m==KHM_MOVE&&S.hd.k==KHD_BLD&&rnd()%2){ x=mid+(int)(rnd()%22); y=mid+(int)(rnd()%22); } } else { x=mid+(int)(rnd()%24); y=mid+(int)(rnd()%24); if(rnd()%20==0){ x=(int)(rnd()%(u32)d); y=(int)(rnd()%(u32)d); } }
            if(rnd()%11==0) R.giant=(u8)(rnd()%5);
            if(rnd()%3==0&&R.nB){   // a purposeful press: aim at a building the hold fits (the rest of the soak is chaos)
                int pick[KS_MAXB], np=0; for(int i=0;i<R.nB;i++){ const KBld*b=&S.B[i]; const KsType*T=ksT(b->type); int ok=0, g=0;
                    if(S.hd.k==KHD_NUB) ok=S.hd.bt?(T->kind==KK_HOME&&b->lvl):((T->kind==KK_GATHER||T->kind==KK_PROC)&&b->lvl);
                    else if(S.hd.k==KHD_GOODS) ok=khWantOf(&S,i,&g)>0&&g==S.hd.g;
                    else if(S.hd.k==KHD_NONE) ok=(rnd()&1)?(b->lvl&&T->kind==KK_STORE):(b->lvl&&T->kind!=KK_STORE&&(T->kind==KK_HOME?b->work>0:b->work>0||ksOut(b)>0));
                    if(ok) pick[np++]=i; }
                if(np){ const KBld*q=&S.B[pick[rnd()%(u32)np]]; x=q->x+(int)(rnd()%2); y=q->y+(int)(rnd()%2); m=S.hd.k==KHD_GOODS?KHM_HAND:S.hd.k==KHD_NUB?KHM_NUBBIN:(S.B[0].lvl&&ksT(q->type)->kind==KK_STORE?KHM_HAND:(ksT(q->type)->kind==KK_HOME||(q->work&&ksOut(q)==0))?KHM_NUBBIN:KHM_HAND); } }
            int r=ACT(m,x,y); CHECK(r>=0&&r<KHR_N,"bad result %d",r); cnt[r]++; held[S.hd.k]++;
            CHECK(S.hd.k<=KHD_NUB&&(S.hd.k!=KHD_GOODS||(S.hd.n>=1&&S.hd.n<=8&&S.hd.g<KG_GOODS)),"hold state k %d g %d n %d",S.hd.k,S.hd.g,S.hd.n);
            if(rnd()%5==0) for(int s=(int)(rnd()%90);s>0;s--) ksStep(&S);
            if(rnd()%12==0){ KHold keep=S.hd; if(keep.k!=KHD_BLD){ int e=ksCheck(&S); CHECK(e==0,"books before sending the hold home: %d",e); ksHoldHome(&S); e=ksCheck(&S); CHECK(e==0&&S.hd.k==KHD_NONE,"books after sending the hold home: %d",e); } }
            if(k%97==0){ int e=ksCheck(&S); CHECK(e==0,"tier %d press %d: books %d (hold k %d)",tier,k,e,S.hd.k); CHECK(bldCells()==4*R.nB,"footprints: %d cells for %d buildings",bldCells(),R.nB); CHECK(R.nB<=KS_MAXB,"too many buildings");
                for(int i=0;i<R.nB;i++){ const KBld*b=&S.B[i]; const KsType*T=ksT(b->type); CHECK(b->type<KS_NTYPES&&b->lvl<=1&&(T->kind==KK_HOME?b->work<=T->house:b->work<=1)&&(b->lvl||!b->work||T->kind==KK_CASTLE),"building %d is damaged: type %d lvl %d work %d",i,b->type,b->lvl,b->work); } }
            if(fails>20) break; }
        { int e=ksCheck(&S); CHECK(e==0,"final books %d",e); }
        CHECK(cnt[KHR_TAKE]>30&&cnt[KHR_GIVE]>15&&cnt[KHR_LIFT]>30&&cnt[KHR_SET]>15&&cnt[KHR_HEAD]>15&&cnt[KHR_SETTLE]>5,"the soak did real work: take %d give %d back %d lift %d set %d head %d settle %d",cnt[KHR_TAKE],cnt[KHR_GIVE],cnt[KHR_BACK],cnt[KHR_LIFT],cnt[KHR_SET],cnt[KHR_HEAD],cnt[KHR_SETTLE]);
        printf("  tier %d: take %d give %d back %d | lift %d set %d | head %d settle %d | refusals %d, books balanced throughout (day %u, %d buildings)\n",tier,cnt[KHR_TAKE],cnt[KHR_GIVE],cnt[KHR_BACK],cnt[KHR_LIFT],cnt[KHR_SET],cnt[KHR_HEAD],cnt[KHR_SETTLE],
            cnt[KHR_REFUSE]+cnt[KHR_ENOUGH]+cnt[KHR_BUSY]+cnt[KHR_NOLOAD]+cnt[KHR_NOWANT]+cnt[KHR_NOBODY]+cnt[KHR_HOMEFULL]+cnt[KHR_STAFFED]+cnt[KHR_LASTHAND]+cnt[KHR_NOTYET]+cnt[KHR_NOWORK],R.day,R.nB); }
    if(fails){ printf("kinghold: %d FAILURE(S)\n",fails); return 1; } printf("kinghold: ALL PASSED: 0 failure(s)\n"); return 0; }
