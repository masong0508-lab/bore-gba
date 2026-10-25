// test_kingcomfort.c - PC test of step 5g: the WELL and the TAVERN. A finished well or tavern makes the producers (gatherers and processors) whose 2 x 2 is within KS_CR cells of its own work
// faster: 1/8 shorter period for each KIND in reach (two wells are no better than one; a well and a tavern make it 2/8). It has no worker, no goods and no save field: the bonus is made by ksCensus.
// It checks: the table and the numbers (the castle keeps its number); the reach to the cell; unfinished comfort does nothing; the real period (the first output comes on the frame ksPeriod says);
// demolishing takes the bonus away at once; a saved and loaded realm has it again; a soak of random buildings of all 14 types with the books balanced and the bonus equal to an independent count;
// the info and menu texts; the roof colour and the palette bank the tavern shares with the text layer; the mission numbers. From the repo root:   sh tools/test_kingcomfort.sh
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
#include "../source/kingtask.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static u32 rs=777; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static int scene(int tier,u32 seed){   // the middle cleared to grass, a block of forest and of rock for the gatherers
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=(u8)tier; R.genVer=KG_GENVER; R.seed=seed; R.rng=7; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,seed,tier);
    int d=kgDim(tier), c=d/2;
    for(int y=c-12;y<=c+14;y++) for(int x=c-12;x<=c+34;x++) cells[y*d+x]=KT_GRASS;
    for(int y=c-5;y<=c-2;y++) for(int x=c-5;x<=c+9;x++) cells[y*d+x]=KT_FOREST;   // trees in reach of a woodcutter at (c, c), clear of every place the tests build on
    return ksInit(&S,&R,B,cells);
}
static int indep(int i){   // the bonus of building i, counted again from the list alone
    const KBld*b=&S.B[i]; if(!b->lvl) return 0; int k=ksT(b->type)->kind; if(k!=KK_GATHER&&k!=KK_PROC) return 0; int w=0,t=0;
    for(int j=0;j<R.nB;j++){ const KBld*c=&S.B[j]; if(!c->lvl) continue; int dx=c->x-b->x, dy=c->y-b->y; if(dx<0) dx=-dx; if(dy<0) dy=-dy; if(dx>8||dy>8) continue; if(c->type==KB_WELL) w=1; if(c->type==KB_TAVERN) t=1; }
    return w+t; }
static int firstOut(int i,int maxn){ S.t=1; for(int n=1;n<=maxn;n++){ ksEcon(&S); if(ksOut(&S.B[i])>0) return n; } return -1; }
int main(void){
    printf("== kingcomfort: the well and the tavern\n");
    // 1 the table
    CHECK(KS_NTYPES==14&&KB_CASTLE==11&&KB_WELL==12&&KB_TAVERN==13,"the type numbers: the castle stays 11, the well 12, the tavern 13");
    CHECK(KS_T[KB_WELL].kind==KK_COMFORT&&KS_T[KB_TAVERN].kind==KK_COMFORT&&KS_T[KB_WELL].staff==0&&KS_T[KB_TAVERN].staff==0&&KS_T[KB_WELL].costN>0&&KS_T[KB_TAVERN].costN>0,"comfort rows");
    CHECK(KHM_WELL-KHM_HUT==KB_WELL&&KHM_TAVERN-KHM_HUT==KB_TAVERN&&KHM_N==KS_NTYPES+5&&KHM_DEMO==KHM_TAVERN+1&&!strcmp(khName[KHM_WELL],"WELL")&&!strcmp(khName[KHM_TAVERN],"TAVERN"),"build modes line up");
    CHECK(EXK_NBT==KS_NTYPES&&EXK_BT+EXK_NBT==EXK_BRIDGE&&EXK_BRIDGE+1==EXK_N,"the task numbers hold every type");
    // 2 the reach, to the cell
    { CHECK(scene(1,11u)==KSE_OK,"init"); int c=48; int w=ksPlace(&S,KB_WOODCUT,c,c,1); S.B[w].work=1; ksCensus(&S); CHECK(S.cmf[w]==0,"no comfort yet");
      int wl=ksPlace(&S,KB_WELL,c+8,c,1); CHECK(wl>=0&&S.cmf[w]==1,"a well 8 cells away (the edge of the reach) helps");
      ksDemolish(&S,wl); CHECK(S.cmf[w]==0,"a demolished well helps no more, at once");
      wl=ksPlace(&S,KB_WELL,c+9,c,1); CHECK(wl>=0&&S.cmf[w]==0,"a well 9 cells away does not");
      ksDemolish(&S,wl); wl=ksPlace(&S,KB_WELL,c+8,c+8,1); CHECK(wl>=0&&S.cmf[w]==1,"the corner of the reach (8 and 8) helps");
      ksDemolish(&S,wl); wl=ksPlace(&S,KB_WELL,c-8,c-8,1); CHECK(wl>=0&&S.cmf[w]==1,"and the other corner (-8 and -8)");
      int w2=ksPlace(&S,KB_WELL,c+8,c,1); CHECK(w2>=0&&S.cmf[w]==1,"two wells are no better than one");
      int tv=ksPlace(&S,KB_TAVERN,c,c+8,1); CHECK(tv>=0&&S.cmf[w]==2,"a well and a tavern make it 2");
      CHECK(S.cmf[wl]==0&&S.cmf[tv]==0&&S.cmf[w2]==0,"a well and a tavern have no bonus themselves");
      ksDemolish(&S,tv); CHECK(S.cmf[w]==1,"the tavern goes, one kind is left");
      int site=ksPlace(&S,KB_TAVERN,c+4,c+4,0); CHECK(site>=0&&S.cmf[w]==1,"a tavern that is only a site does nothing");
      CHECK(ksCheck(&S)==0,"books after all that (%d)",ksCheck(&S)); }
    // 3 the real clock: the first output comes on the frame ksPeriod says, for a gatherer and a processor
    { CHECK(scene(1,12u)==KSE_OK,"init"); int c=48; int w=ksPlace(&S,KB_WOODCUT,c,c,1); S.B[w].work=1; ksCensus(&S);
      int p0=(int)ksPeriod(&S,w); CHECK(p0==KS_T[KB_WOODCUT].period,"no comfort: the period itself (%d)",p0); int f0=firstOut(w,400); CHECK(f0==p0,"woodcutter alone: first wood on frame %d, period %d",f0,p0);
      ksSetOut(&S.B[w],0); S.timer[w]=0; ksPlace(&S,KB_WELL,c+6,c,1); int p1=(int)ksPeriod(&S,w); int f1=firstOut(w,400);
      CHECK(p1==90-90/8&&f1==p1,"woodcutter with a well: period %d (wanted 79), first wood on frame %d",p1,f1);
      ksSetOut(&S.B[w],0); S.timer[w]=0; ksPlace(&S,KB_TAVERN,c+6,c+4,1); int p2=(int)ksPeriod(&S,w); int f2=firstOut(w,400);
      CHECK(p2==90-90*2/8&&f2==p2&&p2<p1&&p1<p0,"woodcutter with both: period %d (wanted 68), first wood on frame %d",p2,f2);
      int m=ksPlace(&S,KB_MASON,c-2,c+10,1); S.B[m].work=1; ksSetIn(&S.B[m],2); S.made[G_STONE]+=2; ksCensus(&S); int pm=(int)ksPeriod(&S,m); int fm=firstOut(m,400);
      CHECK(pm==150-150*S.cmf[m]/8&&fm==pm,"mason: period %d, first output on frame %d (comfort %d)",pm,fm,S.cmf[m]);
      for(int t=0;t<KS_NTYPES;t++){ const KsType*T=&KS_T[t]; if(T->kind!=KK_GATHER&&T->kind!=KK_PROC) continue; for(int k=0;k<=2;k++){ unsigned q=T->period-T->period*(unsigned)k/8u; CHECK(q>=1&&q<=T->period&&(k==0||q<T->period),"type %d with %d kinds: period %u of %u",t,k,q,T->period); } } }
    // 4 save and load: the bonus is not saved, and it is back
    { CHECK(scene(1,13u)==KSE_OK,"init"); int c=48; int w=ksPlace(&S,KB_WOODCUT,c,c,1); S.B[w].work=1; ksPlace(&S,KB_WELL,c+5,c,1); ksPlace(&S,KB_TAVERN,c+5,c+4,1); ksPlace(&S,KB_TAVERN,c+20,c+10,0); ksCensus(&S); CHECK(S.cmf[w]==2,"setup");
      static u8 buf[9000]; ksPrepSave(&S); int len=kgEncode(buf,sizeof buf,&R,B,cells); CHECK(len>0,"encode %d",len);
      static KSim S2; static KRealm R2; static KBld B2[KS_MAXB]; static u8 c2[128*128]; memset(&R2,0,sizeof R2); memset(B2,0,sizeof B2);
      CHECK(kgDecode(buf,len,&R2,B2,0,0)==0&&kgDecode(buf,len,&R2,B2,c2,1)==0,"decode"); CHECK(ksInit(&S2,&R2,B2,c2)==KSE_OK&&ksCheck(&S2)==0,"ksInit of the loaded realm");
      CHECK(R2.nB==R.nB&&S2.B[1].type==KB_WELL&&S2.B[2].type==KB_TAVERN&&S2.B[3].lvl==0,"the types and the site came back"); CHECK(S2.cmf[0]==2&&S2.cmf[1]==0&&S2.cmf[2]==0,"the bonus is made again after the load (%d)",S2.cmf[0]);
      // a damaged save: a worker in a well
      B2[1].work=1; CHECK(ksInit(&S2,&R2,B2,c2)==KSE_OK&&B2[1].work==0&&S2.emp<=S2.pop,"a well never has a worker"); }
    // 5 a soak, three sizes: buildings of ALL types at random, built and sites, goods given, the clock running; the books balance and the bonus equals the independent count
    for(int tier=0;tier<3;tier++){ CHECK(scene(tier,200u+(u32)tier)==KSE_OK,"init"); int d=kgDim(tier), c=d/2; long placed[KS_NTYPES]={0}; int checks=0;
        for(int x=c-10;x<c+30&&x<d-3;x++) ksRoadSet(&S,x,c+1,1);
        for(int k=0;k<60000;k++){ int op=(int)(rnd()%10);
            if(op<3){ int t=(int)(rnd()%KS_NTYPES); int x=c-10+(int)(rnd()%36u), y=c-10+(int)(rnd()%24u); int r=ksPlace(&S,t,x,y,(rnd()&1)&&t!=KB_CASTLE); if(r>=0){ placed[t]++; if(S.B[r].lvl&&(rnd()&1)&&ksT(t)->staff) S.B[r].work=1; } }
            else if(op==3&&R.nB){ ksDemolish(&S,(int)(rnd()%(u32)R.nB)); }
            else if(op==4){ ksGift(&S,(int)(rnd()%8u),(int)(rnd()%6u)); }
            else { for(int j=0;j<7;j++) ksStep(&S); }
            if(k%97==0){ checks++; CHECK(ksCheck(&S)==0,"tier %d op %d: books %d",tier,k,ksCheck(&S)); for(int i=0;i<R.nB;i++){ CHECK(S.cmf[i]==indep(i),"tier %d op %d: building %d (type %d) has bonus %d, counted again %d",tier,k,i,S.B[i].type,S.cmf[i],indep(i)); if(fails>10) break; } if(fails>10) break; } }
        CHECK(placed[KB_WELL]>10&&placed[KB_TAVERN]>10,"the soak placed %ld wells and %ld taverns",placed[KB_WELL],placed[KB_TAVERN]);
        printf("  tier %d (%dx%d): 60,000 random operations, %d checks, %ld wells and %ld taverns placed, books balanced, every bonus equal to the independent count\n",tier,d,d,checks,placed[KB_WELL],placed[KB_TAVERN]); }
    // 6 the texts
    { CHECK(scene(1,14u)==KSE_OK,"init"); int c=48; int w=ksPlace(&S,KB_WOODCUT,c,c,1); S.B[w].work=1; char b[64]; ksCensus(&S);
      khInfo(&S,c,c,b); CHECK(!strcmp(b,"WOODCUTTER  WORKING"),"plain: %s",b);
      int wl=ksPlace(&S,KB_WELL,c+6,c,1); khInfo(&S,c,c,b); CHECK(!strcmp(b,"WOODCUTTER  WORKING  FASTER"),"with a well: %s",b);
      khInfo(&S,c+6,c,b); CHECK(!strcmp(b,"WELL  SPEEDS WORK NEARBY"),"a well: %s",b);
      ksPlace(&S,KB_TAVERN,c+6,c+4,1); khInfo(&S,c,c,b); CHECK(!strcmp(b,"WOODCUTTER  WORKING  MUCH FASTER"),"with both: %s",b);
      khInfo(&S,c+6,c+4,b); CHECK(!strcmp(b,"TAVERN  SPEEDS WORK NEARBY"),"a tavern: %s",b);
      int sp=ksPlace(&S,KB_TAVERN,c+10,c+4,0); (void)sp; khInfo(&S,c+10,c+4,b); CHECK(!strncmp(b,"TAVERN  SITE  GOODS 0 OF 6",26),"a tavern site: %s",b);
      char l[64]; khCost(l,&S,KHM_WELL); CHECK(!strncmp(l,"WELL  4 STONE  HAVE ",20),"menu: %s",l); khCost(l,&S,KHM_TAVERN); CHECK(!strncmp(l,"TAVERN  6 PLANKS  HAVE ",23),"menu: %s",l);
      int worst=0; for(int m=0;m<KHM_N;m++){ khCost(l,&S,m); int n=(int)strlen(l); if(n>worst) worst=n; CHECK(n>0&&n<=40,"menu line %d is %d long",m,n); }
      for(int i=0;i<R.nB;i++){ khInfo(&S,S.B[i].x,S.B[i].y,b); CHECK(strlen(b)<=38,"info line too long: %s",b); } (void)wl;
      KHand K; khInit(&K); K.mode=KHM_WELL; int r=khAct(&K,&S,c+14,c+8); CHECK(r==KHR_SITE,"a well is a mode: a site is placed (%d)",r); K.mode=KHM_TAVERN; r=khAct(&K,&S,c+17,c+8); CHECK(r==KHR_SITE,"and a tavern (%d)",r);
      K.mode=KHM_DEMO; r=khAct(&K,&S,c+6,c); CHECK(r==KHR_RAZED&&ksCheck(&S)==0,"a well can be demolished (%d)",r);
      printf("  texts and modes: longest menu line %d characters\n",worst); }
    // 7 the roofs and the palette banks: the tavern's roof bank is also the text layer's (indices 1 and 2 there), so no building tile may use those two indices
    { for(int a=0;a<KS_NTYPES;a++) for(int e=a+1;e<KS_NTYPES;e++){ CHECK(kvRoof[a]!=kvRoof[e],"map roofs %d and %d are the same",a,e); CHECK(koRoof[a]!=koRoof[e],"overview roofs %d and %d are the same",a,e); }
      CHECK(kvPalAt(KB_TAVERN+1,14)==kvRoof[KB_TAVERN]&&KB_TAVERN+1==14,"the tavern's bank is bank 14");
      for(int t=8;t<12;t++){ u8 g[32]; kvTileGfx(t,g); for(int i=0;i<32;i++) for(int h=0;h<2;h++){ int px=(g[i]>>(4*h))&15; CHECK(px!=1&&px!=2,"building tile %d uses colour %d, which bank 14's text overwrites",t,px); } }
      printf("  14 roofs all different on the map and the overview; the building tiles never use colours 1 and 2 (bank 14 = the tavern's roof and the text)\n"); }
    // 8 the mission numbers
    { CHECK(scene(1,15u)==KSE_OK,"init"); int c=48; ksPlace(&S,KB_WELL,c,c,1); ksPlace(&S,KB_TAVERN,c+4,c,1); ksPlace(&S,KB_TAVERN,c+8,c,1); ksPlace(&S,KB_TAVERN,c+12,c,0); int v[EXK_N]; ktFeed(&S,v);
      CHECK(v[EXK_BT+KB_WELL]==1&&v[EXK_BT+KB_TAVERN]==2&&v[EXK_BUILT]==3,"the feed counts finished wells and taverns (%d %d %d)",v[EXK_BT+KB_WELL],v[EXK_BT+KB_TAVERN],v[EXK_BUILT]);
      CHECK(exkVal(QK_KBT,KB_TAVERN,v)==2&&exkVal(QK_KBT,KB_WELL,v)==1,"a task can ask for a tavern or a well"); }
    printf("%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}
