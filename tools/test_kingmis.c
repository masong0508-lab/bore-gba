// test_kingmis.c - PC test of Phase 5c: the sixteen missions (kingmis.h: the ten and the six of THE ROYAL YEARS) and the numbers the realm hands them (kingq.h, kingtask.h). It checks: the data (ten missions, the words fit, every number is reachable
// by the checker, every building and good a task names exists); the feed (the numbers match the realm); and a whole village that plays the missions IN ORDER the way expand.h's exKingTick does (a task that
// comes true while its mission is the open one stays done; the mission ends when all its tasks are done), so every mission can be finished and none is finished by what came before. From the repo root:   sh tools/test_kingmis.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
#include "../source/kingtask.h"
// expand.h's types and macros, as far as the missions' data needs them
enum { XS_NONE };
typedef struct { u8 k, a; u32 n; char nm[31]; } XT;
typedef struct { char nm[28]; char br[2][120]; u8 sc, pay; XT t[3]; u8 ar; const char*in,*out; } XMs;
#define XM(nm,b0,b1,sc,pay,...) {nm,{b0,b1},sc,pay,{__VA_ARGS__}}
#include "../source/kingmis.h"
#define NM ((int)(sizeof(xmKing)/sizeof(xmKing[0])))
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<20){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static int c0=48;
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int scene(u32 seed){
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=1; R.genVer=KG_GENVER; R.seed=seed; R.rng=7; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,seed,1);
    int d=96, c=48;
    for(int y=c-14;y<=c+16;y++) for(int x=c-14;x<=c+34;x++) cells[y*d+x]=KT_GRASS;
    for(int y=c+5;y<=c+9;y++) for(int x=c-12;x<=c-5;x++) cells[y*d+x]=KT_FOREST;
    for(int y=c+6;y<=c+10;y++) for(int x=c-4;x<=c+6;x++) cells[y*d+x]=KT_ROCK;
    for(int y=c-6;y<=c-3;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_MEADOW;
    for(int y=c+5;y<=c+7;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_CRYSTAL;
    for(int y=c-14;y<=c+16;y++) for(int x=c+35;x<=c+38;x++) cells[y*d+x]=KT_WATER;   // a river four cells wide, east of the village (THE FAR BANK)
    return ksInit(&S,&R,B,cells);
}
static int roadsLaid; static void roads(int c){   // two more road rows and a connector (free to lay), so the houses on them can be reached
    for(int x=c-12;x<=c+34;x++){ ksRoadSet(&S,x,c-5,1); ksRoadSet(&S,x,c+12,1); } for(int y=c-5;y<=c+12;y++) ksRoadSet(&S,c+12,y,1); roadsLaid=1; }
static void houses(int y,int n){ int put=0; for(int x=c0-12;x<=c0+33&&put<n;x+=3) if(ksPlace(&S,KB_HOUSE,x,y,0)>=0) put++; }   // a row of house sites (the Nubbins carry the planks)
int main(void){
    printf("== kingmis: the sixteen missions and the realm's numbers\n");
    // 1 the data
    CHECK(NM==16,"sixteen missions, not %d",NM);
    for(int m=0;m<NM;m++){ const XMs*x=&xmKing[m]; int n=0; while(n<3&&x->t[n].k) n++;
        CHECK(strlen(x->nm)>0&&strlen(x->nm)<=27,"mission %d title %d",m,(int)strlen(x->nm));
        for(int l=0;l<2;l++) CHECK(strlen(x->br[l])>0&&strlen(x->br[l])<=119,"mission %d line %d is %d long",m,l,(int)strlen(x->br[l]));
        CHECK(n==3||(m<9&&n>=2),"mission %d has %d tasks",m,n); CHECK(x->pay>0&&x->pay<=100,"mission %d pays %d",m,x->pay);
        for(int i=0;i<n;i++){ const XT*t=&x->t[i]; CHECK(exkIs(t->k),"m%d t%d kind %d",m,i,t->k); CHECK(strlen(t->nm)>0&&strlen(t->nm)<=30,"m%d t%d name",m,i);
            CHECK(t->n>=1&&t->n<=EXK_CAP,"m%d t%d n %u",m,i,t->n);
            if(t->k==QK_KBT) CHECK(t->a<KS_NTYPES&&t->a!=KB_CASTLE,"m%d t%d building %d",m,i,t->a);
            if(t->k==QK_KGOOD) CHECK(t->a<KG_GOODS,"m%d t%d good %d",m,i,t->a);
            if(t->k==QK_KCASTLE) CHECK(t->n>=1&&t->n<=KG_CASTLE,"m%d t%d castle stage %u",m,i,t->n); } }
    // 2 the feed matches the realm
    { CHECK(scene(1u)==KSE_OK,"init"); int c=48; for(int x=c-8;x<=c+10;x++) ksRoadSet(&S,x,c+1,1); ksPlace(&S,KB_STORE,c-8,c-1,1); ksPlace(&S,KB_HUT,c-5,c-1,1); ksPlace(&S,KB_WOODCUT,c-8,c+2,1); ksPlace(&S,KB_QUARRY,c-2,c+2,1);
      ksPlace(&S,KB_HOUSE,c+1,c-1,1); ksGift(&S,G_WOOD,7); int v[EXK_N]; ktFeed(&S,v);
      CHECK(v[EXK_BUILT]==5&&v[EXK_BT+KB_STORE]==1&&v[EXK_BT+KB_HUT]==1&&v[EXK_BT+KB_WOODCUT]==1&&v[EXK_BT+KB_QUARRY]==1&&v[EXK_BT+KB_HOUSE]==1,"built %d (a site is not built)",v[EXK_BUILT]);
      CHECK(v[EXK_ROAD]==19&&v[EXK_GOOD+G_WOOD]==7&&v[EXK_CASTLE]==0&&v[EXK_POP]==S.pop,"road %d wood %d",v[EXK_ROAD],v[EXK_GOOD+G_WOOD]);
      CHECK(exkVal(QK_KBT,KB_HUT,v)==1&&exkVal(QK_KGOOD,G_WOOD,v)==7&&exkVal(QK_KBT,99,v)==0&&exkVal(QK_KGOOD,99,v)==0&&exkVal(5,0,v)==-1,"exkVal"); printf("  the feed matches the realm\n"); }
    // 2b bridges: a road over water costs cut stone, balances the books, carries a save, and lifts back to water
    { CHECK(scene(5u)==KSE_OK,"init"); int c=48, d=96, w0=c+35; for(int x=c+30;x<=c+34;x++) ksRoadSet(&S,x,c+1,1);
      CHECK(ksRoadSet(&S,w0,c+1,1)==KSE_POOR&&S.cells[(c+1)*d+w0]==KT_WATER&&S.nN==5,"no cut stone: no bridge (%d)",S.nN);
      ksGift(&S,G_CUT,3); for(int i=0;i<3;i++) CHECK(ksRoadSet(&S,w0+i,c+1,1)==KSE_OK&&S.cells[(c+1)*d+w0+i]==KT_BRIDGE,"bridge cell %d",i);
      CHECK(R.goods[G_CUT]==0&&S.nN==8&&ksCheck(&S)==0&&ksRoadSet(&S,w0+3,c+1,1)==KSE_POOR,"3 cut stone, 3 cells, the books balanced (%d goods, %d nodes, check %d)",R.goods[G_CUT],S.nN,ksCheck(&S));
      int v[EXK_N]; ktFeed(&S,v); CHECK(v[EXK_BRIDGE]==3&&v[EXK_ROAD]==8&&exkVal(QK_KBRIDGE,0,v)==3,"the feed counts 3 bridge cells (%d) and 8 road cells (%d)",v[EXK_BRIDGE],v[EXK_ROAD]);
      ksPrepSave(&S); static u8 sv[40000]; int len=kgEncode(sv,sizeof sv,&R,B,cells); CHECK(len>0,"encode %d",len);
      static KSim S2; static KRealm R2; static KBld B2[KS_MAXB]; static u8 c2[128*128]; memset(&R2,0,sizeof R2); memset(c2,0xEE,sizeof c2);
      CHECK(kgDecode(sv,len,&R2,B2,0,0)==0&&kgDecode(sv,len,&R2,B2,c2,1)==0&&ksInit(&S2,&R2,B2,c2)==KSE_OK,"the bridge saves and loads");
      CHECK(c2[(c+1)*d+w0]==KT_BRIDGE&&S2.nN==8&&ksCheck(&S2)==0,"it came back a bridge, in the network (%d nodes)",S2.nN);
      { int wx=-1, wy=-1; for(int y=4;y<d-4&&wx<0;y++) for(int x=4;x<d-4;x++) if(kgCell(R.seed,R.tier,x,y)==KT_WATER&&S.cells[y*d+x]==KT_WATER){ wx=x; wy=y; break; }   // a natural water cell
        ksGift(&S,G_CUT,1); CHECK(wx>=0&&ksRoadSet(&S,wx,wy,1)==KSE_OK&&S.cells[wy*d+wx]==KT_BRIDGE,"a bridge on natural water (%d,%d)",wx,wy);
        CHECK(ksRoadSet(&S,wx,wy,0)==KSE_OK&&S.cells[wy*d+wx]==KT_WATER&&ksCheck(&S)==0&&R.goods[G_CUT]==0,"lifted: water again, the stone stays sunk"); }
      CHECK(ksPlace(&S,KB_HUT,w0,c+1,1)<0,"no building goes on a bridge or the water");
      printf("  bridges: no stone no bridge, 1 cut stone a cell, books balanced, saved and loaded, lifted back to water\n"); }
    // 3 a player plays the missions in order: each mission's buildings go up when it opens (nothing is built before it asks)
    for(int run=0;run<3;run++){
        CHECK(scene(40u+(u32)run*17u)==KSE_OK,"init"); int c=48; for(int x=c-8;x<=c+34;x++) ksRoadSet(&S,x,c+1,1);
        ksPlace(&S,KB_STORE,c-8,c-1,1); for(int k=0;k<5;k++) ksPlace(&S,KB_HUT,c-5+3*k,c-1,1); ksGift(&S,G_WOOD,30);
        int cur=0, ph=0; u8 done=0; int doneAt[16]; for(int i=0;i<16;i++) doneAt[i]=-1; long limit=KS_DAYT*900L; long k=0; int v[EXK_N]; memset(v,0,sizeof v);
        for(;k<limit&&cur<NM;k++){ ksStep(&S); if(k%60) continue; ktFeed(&S,v);
            if(ph!=cur+1){ ph=cur+1; switch(cur){   // the player's moves when mission cur opens
                case 1: ksPlace(&S,KB_WOODCUT,c-8,c+2,0); ksPlace(&S,KB_SAWMILL,c-5,c+2,0); break;
                case 2: ksPlace(&S,KB_QUARRY,c-2,c+2,0); break;
                case 3: ksPlace(&S,KB_SHEEP,c+20,c-1,0); break;
                case 4: ksPlace(&S,KB_CRYMINE,c+20,c+2,0); ksPlace(&S,KB_CUTTER,c+14,c+2,0); break;
                case 6: ksPlace(&S,KB_HOUSE,c+17,c-1,0); ksPlace(&S,KB_HOUSE,c+23,c-1,0); ksPlace(&S,KB_HOUSE,c+26,c-1,0); break;
                case 7: ksPlace(&S,KB_STORE,c+10,c-1,0); ksPlace(&S,KB_HOUSE,c+29,c-1,0); ksPlace(&S,KB_HOUSE,c+32,c-1,0); break;
                case 8: ksPlace(&S,KB_CASTLE,c+14,c-1,0); break;
                // THE ROYAL YEARS: roads first (two more rows and a connector, all free), then what each mission asks for
                case 10: ksGift(&S,G_STONE,60); ksGift(&S,G_WOOD,60); roads(c); ksPlace(&S,KB_WELL,c+30,c+2,0); ksPlace(&S,KB_TAVERN,c+27,c+2,0); houses(c-7,8); break;
                case 11: break;                                                       // (the bridge is laid below as the cut stone comes)
                case 12: ksGift(&S,G_STONE,60); ksGift(&S,G_WOOD,120); houses(c-4,8); ksPlace(&S,KB_WOODCUT,c-11,c+13,0); ksPlace(&S,KB_QUARRY,c+8,c+13,0); break;   // (two more workplaces: ten Nubbins at work)
                case 13: ksGift(&S,G_STONE,140); ksGift(&S,G_WOOD,120); ksGift(&S,G_WOOL,40); break;   // (stop building: the stockpile fills; a player would open more quarries and pens: the first quarry has dug its rock away by now)
                case 14: ksGift(&S,G_WOOD,200); ksGift(&S,G_STONE,40); ksPlace(&S,KB_TAVERN,c+24,c+2,0); houses(c+10,6); houses(c+13,6); break;
                case 15: ksGift(&S,G_WOOD,300); ksPlace(&S,KB_WOODCUT,c+30,c+13,0); ksPlace(&S,KB_SAWMILL,c+30,c+10,0); houses(c-7,16); houses(c-4,16); houses(c+10,16); houses(c+13,16); break; } }   // (the last: the two workplaces first, then houses up to the cap)
            if(cur==3&&v[EXK_GOOD+G_WOOL]>=10&&!v[EXK_BT+KB_WEAVER]){ ksPlace(&S,KB_WEAVER,c+10,c+2,0); }   // the weaver once there is wool to spare
            if((cur==7||cur==11)&&v[EXK_BRIDGE]<(cur==7?4:12)&&v[EXK_GOOD+G_CUT]>0){ for(int row=0;row<(cur==7?1:3);row++){ int yy=row==0?c+1:row==1?c-5:c+12; for(int x=c+35;x<=c+38;x++) if(ksRoadSet(&S,x,yy,1)==KSE_POOR) goto nostone; } nostone:; }   // a cell of the bridge whenever there is cut stone (the long bridge: three rows)
            if(cur==2&&v[EXK_GOOD+G_STONE]>=20&&!v[EXK_BT+KB_MASON]&&R.nB<KS_MAXB) ksPlace(&S,KB_MASON,c+1,c+2,0);   // the mason once there is stone to spare
            const XMs*x=&xmKing[cur]; int n=0; while(n<3&&x->t[n].k) n++;
            for(int i=0;i<n;i++) if(!((done>>i)&1)&&exkVal(x->t[i].k,x->t[i].a,v)>=(int)x->t[i].n) done|=(u8)(1<<i);
            if(done==(1<<n)-1){ doneAt[cur]=(int)R.day; cur++; done=0; } if(fails>20) break; }
        CHECK(cur==NM,"run %d: only %d of %d missions could be finished (day %u, mission %d, tasks %x, castle %d, pop %d, built %d, wood %d stone %d wool %d polish %d)",run,cur,NM,(unsigned)R.day,cur,done,R.castle,S.pop,v[EXK_BUILT],v[EXK_GOOD+G_WOOD],v[EXK_GOOD+G_STONE],v[EXK_GOOD+G_WOOL],v[EXK_GOOD+G_POLISH]);
        if(getenv("KMDBG")&&cur<NM) for(int i=0;i<R.nB;i++){ const KBld*b=&S.B[i]; if(b->type!=KB_HOUSE||!b->lvl) printf("   b%d type %d at %d,%d lvl %d work %d in %d out %d\n",i,b->type,b->x-48,b->y-48,b->lvl,b->work,ksIn(b),ksOut(b)); }
        CHECK(ksCheck(&S)==0,"run %d: books",run);
        printf("  run %d: missions done on days",run); for(int i=0;i<NM;i++) printf(" %d",doneAt[i]); printf("  (pop %d, working %d, %d buildings, %d road cells, %u loads)\n",S.pop,S.emp,v[EXK_BUILT],v[EXK_ROAD],S.delivered); }
    printf("%s: %d failure(s)\n",fails?"kingmis: FAILED":"kingmis: ALL PASSED",fails); return fails?1:0;
}
