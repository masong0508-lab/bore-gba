// test_kinghand.c - PC test of source/kinghand.h: building by hand. Every mode on every kind of cell, the refusals, demolish, and then 20,000 random presses with the books
// (ksCheck) and the map view checked throughout. Run from the repo root:   sh tools/test_kinghand.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
static u16 sb[1024];
#define KV_PUT(sx,sy,e) do{ if((sx)<0||(sx)>31||(sy)<0||(sy)>31){ printf("FAIL: put outside the screenblock\n"); exit(1); } sb[(sy)*32+(sx)]=(u16)(e); }while(0)
#include "../source/kingview.h"
#include "../source/kinghand.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u32 rs=777; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int fresh(int tier,u32 seed){ memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=5; kgGen(cells,seed,tier); return ksInit(&S,&R,B,cells); }
static int findKind(int want,int*ox,int*oy){ int d=S.dim; for(int y=4;y<d-4;y++) for(int x=4;x<d-4;x++){ int ok=1; for(int j=0;j<2&&ok;j++) for(int k=0;k<2;k++) if((S.cells[(y+j)*d+x+k]&0xFF)!=want) ok=0; if(ok){ *ox=x; *oy=y; return 1; } } return 0; }
static int viewBad(KView*V){ kvFull(V); int bad=0; for(int j=0;j<KV_H;j++) for(int i=0;i<KV_W;i++){ int wx=(V->x>>3)+i, wy=(V->y>>3)+j; if(sb[(wy&31)*32+(wx&31)]!=kvEntry(V,wx,wy)) bad++; } return bad; }
int main(void){
    printf("== kinghand: building by hand\n");
    KHand K; khInit(&K); CHECK(K.mode==KHM_HAND,"starts on the hand");
    for(int i=0;i<KHM_N;i++) khCycle(&K,1); CHECK(K.mode==KHM_HAND,"R cycles round"); khCycle(&K,-1); CHECK(K.mode==KHM_DEMO,"L from HAND is DEMOLISH"); khCycle(&K,1);
    CHECK(khCx(0)==15&&khCy(0)==10&&khCx(8*40-1)==(8*40+119)/8,"the cell under the middle of the screen");
    CHECK(KHM_ROAD==1&&KHM_HUT==2&&KHM_STORE-KHM_HUT==KB_STORE&&KHM_DEMO==KS_NTYPES+2&&KHM_N==KS_NTYPES+3,"modes line up with the building types");
    CHECK(KHR_N==11&&KHR_GOT>KHR_RAZED&&KHR_GOT<KHR_NOTHING,"result codes: the map-changing ones come first"); for(int r=0;r<KHR_N;r++) CHECK(r==0||(khMsg[r]&&khMsg[r][0]),"message for result %d",r);
    for(int tier=0;tier<3;tier++){
        CHECK(fresh(tier,31u+(u32)tier)==KSE_OK,"init tier %d",tier); int d=S.dim; int x,y;
        KView V; kvInit(&V,S.cells,d,S.B,0); V.x=V.y=0; kvClamp(&V);
        // a road on grass, again (lifts it), on water (refused), off the edge (refused)
        CHECK(findKind(KT_GRASS,&x,&y)||findKind(KT_MEADOW,&x,&y),"no grass"); K.mode=KHM_ROAD;
        CHECK(khAct(&K,&S,x,y)==KHR_ROAD&&S.cells[y*d+x]==KT_ROAD,"road laid"); CHECK(khAct(&K,&S,x,y)==KHR_UNROAD&&S.cells[y*d+x]!=KT_ROAD,"road lifted");
        int wx,wy; if(findKind(KT_WATER,&wx,&wy)){ CHECK(khAct(&K,&S,wx,wy)==KHR_SPOT,"no road on water"); K.mode=KHM_STORE; CHECK(khAct(&K,&S,wx,wy)==KHR_SPOT,"no building on water"); K.mode=KHM_ROAD; }
        CHECK(khAct(&K,&S,-1,5)==KHR_ARG&&khAct(&K,&S,5,d)==KHR_ARG,"off the realm"); { int r0=khAct(&K,&S,0,0); CHECK(r0==KHR_ARG||r0==KHR_SPOT,"the border is refused (%d)",r0); }
        // every building type as a site; the road refuses a cell with a building; demolish takes it away again
        for(int t=KHM_HUT;t<KHM_DEMO;t++){ int n0=R.nB; K.mode=t; int r=khAct(&K,&S,x,y); CHECK(r==KHR_SITE&&R.nB==n0+1,"site of mode %d: result %d",t,r);
            CHECK(S.B[n0].lvl==0&&S.B[n0].type==t-KHM_HUT,"a site, not a finished building");
            CHECK(khBldAt(&S,x+1,y+1)==n0&&khBldAt(&S,x+2,y)<0,"footprint");
            K.mode=KHM_ROAD; CHECK(khAct(&K,&S,x,y)==KHR_SPOT,"no road under a building"); K.mode=t; CHECK(khAct(&K,&S,x,y)==KHR_SPOT,"no building on a building");
            V.nB=R.nB; kvFull(&V); CHECK(viewBad(&V)==0,"view after a site"); 
            K.mode=KHM_DEMO; CHECK(khAct(&K,&S,x+1,y+1)==KHR_RAZED&&R.nB==n0,"razed from its far corner"); CHECK(khAct(&K,&S,x,y)==KHR_NOTHING,"nothing left"); V.nB=R.nB; CHECK(viewBad(&V)==0,"view after demolish"); CHECK(ksCheck(&S)==0,"books %d",ksCheck(&S)); }
        // the cap: fill every building place, then the next is refused with KHR_FULL
        { int placed=0; K.mode=KHM_HUT; for(int yy=4;yy<d-4&&R.nB<KS_MAXB;yy+=3) for(int xx=4;xx<d-4&&R.nB<KS_MAXB;xx+=3) placed+=khAct(&K,&S,xx,yy)==KHR_SITE;
          if(R.nB==KS_MAXB){ int fullSeen=0; for(int yy=4;yy<d-4;yy+=3) for(int xx=4;xx<d-4;xx+=3) if(khAct(&K,&S,xx,yy)==KHR_FULL) fullSeen=1; CHECK(fullSeen,"the cap is reported"); } CHECK(placed>0,"placed some");
          K.mode=KHM_DEMO; for(int i=R.nB-1;i>=0;i--) CHECK(khAct(&K,&S,S.B[i].x,S.B[i].y)==KHR_RAZED,"clear"); CHECK(R.nB==0,"all razed"); }
        // random presses, with the simulation running between them
        { int roads=0,sites=0,razed=0; for(int k=0;k<20000;k++){ K.mode=(int)(rnd()%KHM_N); int cx=(int)(rnd()%(u32)d), cy=(int)(rnd()%(u32)d); int r=khAct(&K,&S,cx,cy); roads+=(r==KHR_ROAD); sites+=(r==KHR_SITE); razed+=(r==KHR_RAZED);
            if(k%7==0) for(int s=0;s<20;s++) ksStep(&S);
            if(k%500==0){ CHECK(ksCheck(&S)==0,"books after %d presses: %d",k,ksCheck(&S)); V.nB=R.nB; V.x=(int)(rnd()%(u32)(d*8)); V.y=(int)(rnd()%(u32)(d*8)); kvClamp(&V); CHECK(viewBad(&V)==0,"view after %d presses",k); } }
          CHECK(roads>100&&sites>20&&razed>5,"the random run did real work (roads %d, sites %d, razed %d)",roads,sites,razed); printf("  tier %d: %d roads, %d sites, %d demolitions, books balanced\n",tier,roads,sites,razed); }
        }
    // ---- the text layer: info text, status line, strip -> tiles ----
    { char b[64]; int worst=0; CHECK(fresh(1,9u)==KSE_OK,"init");
      for(int y=-1;y<=S.dim;y++) for(int x=-1;x<=S.dim;x++){ khInfo(&S,x,y,b); int n=(int)strlen(b); if(n>worst) worst=n; CHECK(n>0,"empty info at %d %d",x,y); }
      int x,y; K.mode=KHM_STORE; findKind(KT_GRASS,&x,&y); khAct(&K,&S,x,y); khInfo(&S,x,y,b); CHECK(strstr(b,"STOREHOUSE")&&strstr(b,"SITE")&&strstr(b,"GOODS 0 OF"),"site text: %s",b);
      khInfo(&S,x+1,y+1,b); CHECK(strstr(b,"STOREHOUSE")!=0,"far corner: %s",b); S.B[0].lvl=1; khInfo(&S,x,y,b); CHECK(strstr(b,"STOCKPILE")!=0,"store text: %s",b);
      K.mode=KHM_HUT; findKind(KT_GRASS,&x,&y); int xs=x+3; (void)xs; for(int yy=4;yy<S.dim-4;yy++) for(int xx=4;xx<S.dim-4;xx++) if(khAct(&K,&S,xx,yy)==KHR_SITE){ x=xx; y=yy; yy=S.dim; break; }
      S.B[R.nB-1].lvl=1; S.B[R.nB-1].work=3; khInfo(&S,x,y,b); CHECK(strstr(b,"HUT  NUBBINS 3 OF 4")!=0,"hut text: %s",b);
      khStatus(b,KHM_DEMO,123456,7,2); CHECK(strcmp(b,"DEMOLISH   DAY 99999  NUBBINS 7  X16")==0,"status: %s",b); khStatus(b,KHM_ROAD,12,0,0); CHECK(strcmp(b,"ROAD   DAY 12  NUBBINS 0  X1")==0,"status: %s",b);
      CHECK(worst<=24,"longest info line is %d characters",worst);
      static u16 fbs[240*16], tiles[KH_TILES*16]; memset(fbs,0,sizeof fbs); const u16 SH=0x0821, WH=0x7FFF;
      for(int yy=0;yy<16;yy++) for(int xx=0;xx<240;xx++){ u32 h=(u32)(xx*7+yy*13)%5; fbs[yy*240+xx]=h==0?SH:h==1?WH:h==2?0x1234:0; }
      khStrip(fbs,SH,tiles); int bad=0;
      for(int yy=0;yy<16;yy++) for(int xx=0;xx<240;xx++){ int tile=(yy>>3)*30+(xx>>3); int row=yy&7, col=xx&7; unsigned got=(tiles[tile*16+row*2+(col>>2)]>>((col&3)*4))&15; u16 px=fbs[yy*240+xx]; unsigned want=px==0?0:px==SH?2:1; bad+=got!=want; }
      CHECK(bad==0,"%d wrong pixels in the tiles",bad); printf("  text layer: info lines up to %d characters, strip -> tiles exact\n",worst); }
    // ---- the build menu lines ----
    { CHECK(fresh(0,5u)==KSE_OK,"init"); char b[64]; int worst=0; R.goods[G_WOOD]=12; R.goods[G_PLANKS]=0; R.goods[G_STONE]=70000;
      for(int m=0;m<KHM_N;m++){ khCost(b,&S,m); int n=(int)strlen(b); if(n>worst) worst=n; CHECK(n>0&&n<=40,"menu line %d is %d long: %s",m,n,b); }
      khCost(b,&S,KHM_HAND); CHECK(strcmp(b,"HAND  GATHERS WOOD STONE CRYSTAL")==0,"%s",b); khCost(b,&S,KHM_ROAD); CHECK(strcmp(b,"ROAD  FREE")==0,"%s",b); khCost(b,&S,KHM_DEMO); CHECK(strcmp(b,"DEMOLISH  GOODS COME BACK")==0,"%s",b);
      khCost(b,&S,KHM_SAWMILL); CHECK(strcmp(b,"SAWMILL  4 WOOD  HAVE 12")==0,"%s",b); khCost(b,&S,KHM_MASON); CHECK(strcmp(b,"MASON  3 PLANKS  HAVE 0")==0,"%s",b);
      R.goods[G_WOOD]=65000; khCost(b,&S,KHM_HUT); CHECK(strcmp(b,"HUT  3 WOOD  HAVE 65000")==0,"%s",b);
      for(int m=KHM_HUT;m<KHM_DEMO;m++){ khCost(b,&S,m); const KsType*T=ksT(m-KHM_HUT); CHECK(strstr(b,khGood[T->costG])!=0,"good named for mode %d: %s",m,b); }
      printf("  build menu: %d modes, longest line %d characters\n",KHM_N,worst); }
    // ---- the giant's hands (step 4g): stats and gathering ----
    { for(int g=0;g<5;g++){ CHECK(khStride(g)>=2&&khStride(g)<=4,"stride %d",g); CHECK(khRun(g)>khStride(g)&&khRun(g)<8,"run %d is %d",g,khRun(g)); CHECK(khYield(g)>=2,"yield %d",g); }
      CHECK(khStride(9)==khStride(0)&&khYield(-1)==khYield(0),"a bad giant number falls back to the first");
      for(int tier=0;tier<3;tier++){ CHECK(fresh(tier,70u+(u32)tier)==KSE_OK,"init"); int d=S.dim; int x,y,got=0;
        const int kinds[3]={KT_FOREST,KT_ROCK,KT_CRYSTAL}, goods[3]={G_WOOD,G_STONE,G_CRYSTAL}; int perKind[3]={0,0,0};
        for(int g=0;g<5;g++) for(int kk=0;kk<3;kk++){ R.giant=(u8)g; int found=0;
            for(int yy=2;yy<d-2&&!found;yy++) for(int xx=2;xx<d-2;xx++) if(S.cells[yy*d+xx]==kinds[kk]){ x=xx; y=yy; found=1; break; }
            if(!found) continue; long g0=R.goods[goods[kk]]; KHand H; khInit(&H);
            CHECK(khAct(&H,&S,x,y)==KHR_GOT,"gathered kind %d",kinds[kk]); CHECK(S.cells[y*d+x]==KT_GRASS,"the cell is grass now"); CHECK(R.goods[goods[kk]]==g0+khYield(g),"giant %d got %ld not %d",g,R.goods[goods[kk]]-g0,khYield(g));
            CHECK(khAct(&H,&S,x,y)==KHR_NOGATHER,"nothing left to gather"); CHECK(ksCheck(&S)==0,"books after gathering: %d",ksCheck(&S)); got++; perKind[kk]++; }
        CHECK(perKind[0]>=3&&perKind[1]>=1&&perKind[2]>=1,"tier %d gathered %d trees, %d rocks, %d crystals",tier,perKind[0],perKind[1],perKind[2]);
        if(findKind(KT_GRASS,&x,&y)){ KHand H; khInit(&H); CHECK(khAct(&H,&S,x,y)==KHR_NOGATHER,"grass gives nothing"); }
        { int wx,wy; KHand H; khInit(&H); if(findKind(KT_WATER,&wx,&wy)) CHECK(khAct(&H,&S,wx,wy)==KHR_NOGATHER,"water gives nothing"); }
        { KHand H; khInit(&H); CHECK(khAct(&H,&S,-1,3)==KHR_ARG&&khAct(&H,&S,3,d)==KHR_ARG,"off the realm"); }
        // a building's cell cannot be gathered (a site on a tree cell is impossible, but a finished woodcutter's own footprint is fine to test: bit 7 refuses)
        { KHand H; K.mode=KHM_STORE; if(findKind(KT_GRASS,&x,&y)){ khAct(&K,&S,x,y); khInit(&H); CHECK(khAct(&H,&S,x,y)==KHR_NOGATHER,"a building is not gathered"); } }
        // the stockpile ceiling: a full stockpile loses the gift and the books know
        { KHand H; khInit(&H); R.giant=0; int fx=-1,fy=-1; for(int yy=2;yy<d-2&&fx<0;yy++) for(int xx=2;xx<d-2;xx++) if(S.cells[yy*d+xx]==KT_FOREST){ fx=xx; fy=yy; break; }
          if(fx>=0){ ksGift(&S,G_WOOD,KG_GOODMAX-1-R.goods[G_WOOD]); CHECK(R.goods[G_WOOD]==KG_GOODMAX-1,"setup"); CHECK(khAct(&H,&S,fx,fy)==KHR_GOT&&R.goods[G_WOOD]==KG_GOODMAX&&ksCheck(&S)==0,"ceiling: %d books %d",R.goods[G_WOOD],ksCheck(&S)); } }
      }
      printf("  the giant's hands: stats, yields for all five giants, refusals, books balanced\n"); }
    CHECK(khPalCur(0x0000)!=0&&khPalCur(0x7FFF)==0x7FFF,"the cursor palette lightens and never overflows");
    for(int c=0;c<32768;c+=37){ u16 v=khPalCur((u16)c); if((v&31)<(c&31)||((v>>5)&31)<((c>>5)&31)||((v>>10)&31)<((c>>10)&31)||v>0x7FFF){ CHECK(0,"palette %d",c); break; } }
    if(fails){ printf("kinghand: %d FAILURE(S)\n",fails); return 1; } printf("kinghand: ALL PASSED: 0 failure(s)\n"); return 0; }
