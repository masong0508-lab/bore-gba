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
    KHand K; khInit(&K); CHECK(K.mode==KHM_ROAD,"starts on roads");
    for(int i=0;i<KHM_N;i++) khCycle(&K,1); CHECK(K.mode==KHM_ROAD,"R cycles round"); khCycle(&K,-1); CHECK(K.mode==KHM_DEMO,"L from ROAD is DEMOLISH"); khCycle(&K,1);
    CHECK(khCx(0)==15&&khCy(0)==10&&khCx(8*40-1)==(8*40+119)/8,"the cell under the middle of the screen");
    CHECK(KHM_HUT==1&&KHM_STORE-1==KB_STORE&&KHM_DEMO==KS_NTYPES+1,"modes line up with the building types");
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
    CHECK(khPalCur(0x0000)!=0&&khPalCur(0x7FFF)==0x7FFF,"the cursor palette lightens and never overflows");
    for(int c=0;c<32768;c+=37){ u16 v=khPalCur((u16)c); if((v&31)<(c&31)||((v>>5)&31)<((c>>5)&31)||((v>>10)&31)<((c>>10)&31)||v>0x7FFF){ CHECK(0,"palette %d",c); break; } }
    if(fails){ printf("kinghand: %d FAILURE(S)\n",fails); return 1; } printf("kinghand: ALL PASSED: 0 failure(s)\n"); return 0; }
