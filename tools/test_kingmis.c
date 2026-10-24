// test_kingmis.c - PC test of Phase 5c: the ten missions (kingmis.h) and the numbers the realm hands them (kingq.h, kingtask.h). It checks: the data (ten missions, the words fit, every number is reachable
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
static KSim S; static KRealm R; static KBld B[KS_MAXB]; static u8 cells[128*128];
static int scene(u32 seed){
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=1; R.genVer=KG_GENVER; R.seed=seed; R.rng=7; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,seed,1);
    int d=96, c=48;
    for(int y=c-12;y<=c+14;y++) for(int x=c-12;x<=c+34;x++) cells[y*d+x]=KT_GRASS;
    for(int y=c+5;y<=c+9;y++) for(int x=c-12;x<=c-5;x++) cells[y*d+x]=KT_FOREST;
    for(int y=c+6;y<=c+10;y++) for(int x=c-4;x<=c+6;x++) cells[y*d+x]=KT_ROCK;
    for(int y=c-6;y<=c-3;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_MEADOW;
    for(int y=c+5;y<=c+7;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_CRYSTAL;
    return ksInit(&S,&R,B,cells);
}
int main(void){
    printf("== kingmis: the ten missions and the realm's numbers\n");
    // 1 the data
    CHECK(NM==10,"ten missions, not %d",NM);
    for(int m=0;m<NM;m++){ const XMs*x=&xmKing[m]; int n=0; while(n<3&&x->t[n].k) n++;
        CHECK(strlen(x->nm)>0&&strlen(x->nm)<=27,"mission %d title %d",m,(int)strlen(x->nm));
        for(int l=0;l<2;l++) CHECK(strlen(x->br[l])>0&&strlen(x->br[l])<=119,"mission %d line %d is %d long",m,l,(int)strlen(x->br[l]));
        CHECK(n>=2&&n<=3,"mission %d has %d tasks",m,n); CHECK(x->pay>0&&x->pay<=100,"mission %d pays %d",m,x->pay);
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
    // 3 a player plays the missions in order: each mission's buildings go up when it opens (nothing is built before it asks)
    for(int run=0;run<3;run++){
        CHECK(scene(40u+(u32)run*17u)==KSE_OK,"init"); int c=48; for(int x=c-8;x<=c+34;x++) ksRoadSet(&S,x,c+1,1);
        ksPlace(&S,KB_STORE,c-8,c-1,1); for(int k=0;k<5;k++) ksPlace(&S,KB_HUT,c-5+3*k,c-1,1); ksGift(&S,G_WOOD,30);
        int cur=0, ph=0; u8 done=0; int doneAt[10]; for(int i=0;i<10;i++) doneAt[i]=-1; long limit=KS_DAYT*300L; long k=0; int v[EXK_N]; memset(v,0,sizeof v);
        for(;k<limit&&cur<NM;k++){ ksStep(&S); if(k%60) continue; ktFeed(&S,v);
            if(ph!=cur+1){ ph=cur+1; switch(cur){   // the player's moves when mission cur opens
                case 1: ksPlace(&S,KB_WOODCUT,c-8,c+2,0); ksPlace(&S,KB_SAWMILL,c-5,c+2,0); break;
                case 2: ksPlace(&S,KB_QUARRY,c-2,c+2,0); break;
                case 3: ksPlace(&S,KB_SHEEP,c+20,c-1,0); break;
                case 4: ksPlace(&S,KB_CRYMINE,c+20,c+2,0); ksPlace(&S,KB_CUTTER,c+14,c+2,0); break;
                case 6: ksPlace(&S,KB_HOUSE,c+17,c-1,0); ksPlace(&S,KB_HOUSE,c+23,c-1,0); ksPlace(&S,KB_HOUSE,c+26,c-1,0); break;
                case 7: ksPlace(&S,KB_STORE,c+10,c-1,0); ksPlace(&S,KB_HOUSE,c+29,c-1,0); ksPlace(&S,KB_HOUSE,c+32,c-1,0); break;
                case 8: ksPlace(&S,KB_CASTLE,c+14,c-1,0); break; } }
            if(cur==3&&v[EXK_GOOD+G_WOOL]>=10&&!v[EXK_BT+KB_WEAVER]){ ksPlace(&S,KB_WEAVER,c+10,c+2,0); }   // the weaver once there is wool to spare
            if(cur==2&&v[EXK_GOOD+G_STONE]>=20&&!v[EXK_BT+KB_MASON]&&R.nB<KS_MAXB) ksPlace(&S,KB_MASON,c+1,c+2,0);   // the mason once there is stone to spare
            const XMs*x=&xmKing[cur]; int n=0; while(n<3&&x->t[n].k) n++;
            for(int i=0;i<n;i++) if(!((done>>i)&1)&&exkVal(x->t[i].k,x->t[i].a,v)>=(int)x->t[i].n) done|=(u8)(1<<i);
            if(done==(1<<n)-1){ doneAt[cur]=(int)R.day; cur++; done=0; } if(fails>20) break; }
        CHECK(cur==NM,"run %d: only %d of %d missions could be finished (day %u, mission %d, tasks %x, castle %d, pop %d, built %d, wood %d stone %d wool %d polish %d)",run,cur,NM,(unsigned)R.day,cur,done,R.castle,S.pop,v[EXK_BUILT],v[EXK_GOOD+G_WOOD],v[EXK_GOOD+G_STONE],v[EXK_GOOD+G_WOOL],v[EXK_GOOD+G_POLISH]);
        CHECK(ksCheck(&S)==0,"run %d: books",run);
        printf("  run %d: missions done on days",run); for(int i=0;i<NM;i++) printf(" %d",doneAt[i]); printf("  (pop %d, working %d, %d buildings, %d road cells, %u loads)\n",S.pop,S.emp,v[EXK_BUILT],v[EXK_ROAD],S.delivered); }
    printf("%s: %d failure(s)\n",fails?"kingmis: FAILED":"kingmis: ALL PASSED",fails); return fails?1:0;
}
