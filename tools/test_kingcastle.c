// test_kingcastle.c - PC test of step 5b: the castle (one building, four stages of two lots of goods each; kingsim.h, kinghand.h, kinghold.h, kinglog.h, the roofs).
// It checks: the stage table (every good it asks for has a maker, no lot above KS_BUF, all four better goods are used); the rules (one castle, none after the crown, it cannot be lifted or demolished);
// the stages by hand (a lot at a time, the books balanced at every step, a short lot does not advance); a whole village that builds every chain and then the castle by itself, the stages in order and the crown once;
// a realm saved and loaded at every stage, and the loader's repairs; the text lines (at most 38 characters). From the repo root:   sh tools/test_kingcastle.sh
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
static KSim S, S2; static KRealm R, R2; static KBld B[KS_MAXB], B2[KS_MAXB]; static u8 cells[128*128], c2[128*128]; static u8 buf[40000];
// tier 1 (96 x 96), the middle cleared to grass; optionally forest, rock, meadow and crystal in reach of the village below
static int scene(u32 seed,int land){
    memset(&R,0,sizeof R); memset(B,0,sizeof B); R.tier=1; R.genVer=KG_GENVER; R.seed=seed; R.rng=7; R.gx=KG_NOPOS; R.gy=KG_NOPOS; kgGen(cells,seed,1);
    int d=96, c=48;
    for(int y=c-12;y<=c+14;y++) for(int x=c-12;x<=c+34;x++) cells[y*d+x]=KT_GRASS;
    if(land){ for(int y=c+5;y<=c+9;y++) for(int x=c-12;x<=c-5;x++) cells[y*d+x]=KT_FOREST;
              for(int y=c+6;y<=c+10;y++) for(int x=c-4;x<=c+6;x++) cells[y*d+x]=KT_ROCK;   // 55 rock cells: the castle alone needs about 46 stone (23 cut stone with the cutter's own 3)
              for(int y=c-6;y<=c-3;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_MEADOW;
              for(int y=c+5;y<=c+7;y++) for(int x=c+16;x<=c+26;x++) cells[y*d+x]=KT_CRYSTAL; }
    return ksInit(&S,&R,B,cells);
}
static int typeAt(int t){ for(int i=0;i<R.nB;i++) if(B[i].type==t) return i; return -1; }
static int stageOf(void){ return R.castle; }
int main(void){
    printf("== kingcastle: the castle and its four stages\n");
    // 1 the table and the names
    { int made[8]={0}, used[8]={0}; for(int t=0;t<KS_NTYPES;t++){ const KsType*T=&KS_T[t]; if(T->kind==KK_GATHER||T->kind==KK_PROC) made[T->outG]=1; }
      for(int s=0;s<KG_CASTLE;s++) for(int l=0;l<2;l++){ int g=KS_CL[s][l][0], n=KS_CL[s][l][1]; CHECK(g<8&&made[g],"stage %d lot %d: good %d has no maker",s+1,l,g); CHECK(n>0&&n<=KS_BUF,"stage %d lot %d: %d goods (the most a site can hold is %d)",s+1,l,n,KS_BUF); CHECK(l==0||g!=KS_CL[s][0][0],"stage %d: two lots of the same good",s+1); if(g<8) used[g]=1; }
      CHECK(used[G_PLANKS]&&used[G_CUT]&&used[G_CLOTH]&&used[G_POLISH],"the four better goods are all used by the castle");
      CHECK(KG_CASTLE==4&&KS_NTYPES==14&&KB_CASTLE==11&&KB_WELL==12&&KB_TAVERN==13&&KS_NTYPES<=14,"14 types, the castle 11 (it never moved), the well 12 and the tavern 13, the palette banks fit");
      CHECK(KHM_CASTLE-KHM_HUT==KB_CASTLE&&KHM_N==KS_NTYPES+5&&strcmp(khName[KHM_CASTLE],"CASTLE")==0,"the castle's build mode lines up");
      for(int e=0;e<KB_CASTLE;e++){ CHECK(kvRoof[e]!=kvRoof[KB_CASTLE],"map roof %d is the castle's",e); CHECK(koRoof[e]!=koRoof[KB_CASTLE],"overview roof %d is the castle's",e); }
      CHECK(kvPalAt(KB_CASTLE+1,14)==kvRoof[KB_CASTLE]&&koRoof[KB_CASTLE]!=0,"the castle's palette bank");
      CHECK(scene(1u,0)==KSE_OK,"init"); char b[64]; khCost(b,&S,KHM_CASTLE); CHECK(strcmp(b,"CASTLE  6 PLANKS  HAVE 0")==0,"%s",b);
      printf("  4 stages of 2 lots: foundation 6 planks + 6 cut stone, walls 8 cut stone + 4 planks, towers 6 cut stone + 4 polished, throne room 6 polished + 6 cloth\n"); }
    // 2 the rules: one castle, it stays, none after the crown
    { CHECK(scene(2u,0)==KSE_OK,"init"); KHand K; khInit(&K); K.mode=KHM_CASTLE; int r=khAct(&K,&S,60,60);
      CHECK(r==KHR_SITE&&R.nB==1&&B[0].type==KB_CASTLE&&B[0].lvl==0&&B[0].work==0&&B[0].stock==0,"a castle site: %d",r);
      CHECK(khAct(&K,&S,70,60)==KHR_SPOT&&R.nB==1,"a second castle is refused");
      CHECK(ksPlace(&S,KB_CASTLE,70,60,1)==KSE_SPOT,"ksPlace refuses a second one too");
      K.mode=KHM_MOVE; CHECK(khAct(&K,&S,60,60)==KHR_KEEP&&R.nB==1&&S.hd.k==KHD_NONE,"the castle cannot be lifted");
      K.mode=KHM_DEMO; CHECK(khAct(&K,&S,61,61)==KHR_KEEP&&R.nB==1,"the castle cannot be demolished by hand");
      K.mode=KHM_NUBBIN; CHECK(khAct(&K,&S,60,60)!=KHR_HEAD,"no Nubbin comes out of a castle site");
      char l[64]; khInfo(&S,60,60,l); CHECK(strcmp(l,"CASTLE  STAGE 1 OF 4  PLANKS 0 OF 6")==0,"%s",l);
      R.castle=KG_CASTLE; B[0].lvl=1; B[0].work=0; B[0].stock=0; khInfo(&S,60,60,l); CHECK(strcmp(l,"CASTLE  CROWNED")==0,"%s",l);
      CHECK(ksDemolish(&S,0)==KSE_OK&&ksPlace(&S,KB_CASTLE,60,60,0)==KSE_SPOT,"after the crown no castle can be founded");
      khCost(l,&S,KHM_CASTLE); CHECK(strcmp(l,"CASTLE  ALREADY CROWNED")==0,"%s",l); }
    // 3 the stages by hand: a lot at a time, the books balanced, a short lot does not advance
    { CHECK(scene(3u,0)==KSE_OK,"init"); CHECK(ksPlace(&S,KB_CASTLE,60,60,0)==0,"placed"); S.t=1; char l[64]; int worst=0;
      for(int s=0;s<KG_CASTLE;s++) for(int lot=0;lot<2;lot++){
          int g=-1,n=0; ksCost(&S,&B[0],&g,&n); CHECK(g==KS_CL[s][lot][0]&&n==KS_CL[s][lot][1]&&R.castle==s&&B[0].work==lot,"stage %d lot %d asks for good %d x %d",s+1,lot,g,n);
          int gw=-1, want=ksWant(&S,0,&gw); CHECK(gw==g&&want==n,"the matcher asks for %d x good %d (wanted %d x %d)",want,gw,n,g);
          S.made[g]+=n-1; ksSetIn(&B[0],n-1); ksEcon(&S); CHECK(R.castle==s&&B[0].work==lot&&ksIn(&B[0])==n-1&&S.ev==0&&ksCheck(&S)==0,"a short lot (stage %d lot %d) moved on",s+1,lot);
          S.made[g]+=1; ksSetIn(&B[0],n); khInfo(&S,60,60,l); int len=(int)strlen(l); if(len>worst) worst=len; CHECK(len<=38,"info line %d long: %s",len,l);
          ksEcon(&S); int e=ksCheck(&S); CHECK(e==0,"books after stage %d lot %d: %d",s+1,lot,e);
          if(lot==0) CHECK(B[0].work==1&&R.castle==s&&ksIn(&B[0])==0&&S.ev==0&&B[0].lvl==0,"stage %d lot 0 paid",s+1);
          else { CHECK(B[0].work==0&&R.castle==s+1&&S.ev==s+1&&ksIn(&B[0])==0,"stage %d done (castle %d, ev %d)",s+1,R.castle,S.ev); CHECK(B[0].lvl==(s==KG_CASTLE-1),"built only after stage 4"); S.ev=0; } }
      for(int g=0;g<KG_GOODS;g++) CHECK(S.made[g]==0&&ksHeld(&S,g)==0,"good %d: the stages used up exactly what was brought (%ld, held %ld)",g,S.made[g],ksHeld(&S,g));
      for(long k=0;k<2000;k++){ S.t++; ksEcon(&S); } CHECK(stageOf()==KG_CASTLE&&B[0].lvl==1&&S.ev==0&&S.emp==0&&ksCheck(&S)==0,"after the crown it just stands (emp %d)",S.emp);
      printf("  stages by hand: 8 lots paid in order, the books empty at the end, the longest info line %d characters\n",worst); }
    // 4 a whole village: every chain, then the castle, by itself
    for(int run=0;run<3;run++){
        CHECK(scene(40u+(u32)run*17u,1)==KSE_OK,"init"); int c=48; for(int x=c-8;x<=c+27;x++) ksRoadSet(&S,x,c+1,1);
        ksPlace(&S,KB_STORE,c-8,c-1,1); for(int k=0;k<5;k++) ksPlace(&S,KB_HUT,c-5+3*k,c-1,1);
        ksPlace(&S,KB_WOODCUT,c-8,c+2,0); ksPlace(&S,KB_SAWMILL,c-5,c+2,0); ksPlace(&S,KB_QUARRY,c-2,c+2,0); ksPlace(&S,KB_MASON,c+1,c+2,0);
        ksPlace(&S,KB_SHEEP,c+20,c-1,0); ksPlace(&S,KB_WEAVER,c+10,c+2,0); ksPlace(&S,KB_CRYMINE,c+20,c+2,0); ksPlace(&S,KB_CUTTER,c+14,c+2,0);
        ksPlace(&S,KB_CASTLE,c+14,c-1,0); CHECK(typeAt(KB_CASTLE)>=0&&R.nB==15,"placed (%d buildings)",R.nB);
        ksGift(&S,G_WOOD,30); ksGift(&S,G_PLANKS,10); ksGift(&S,G_CUT,4);
        int seq[8], ns=0, crownDay=-1, lastStage=0; long k=0; long limit=KS_DAYT*220L;
        for(;k<limit;k++){ ksStep(&S);
            if(S.ev){ if(ns<8) seq[ns]=S.ev; ns++; CHECK(S.ev==lastStage+1,"run %d: stage %d came after stage %d",run,S.ev,lastStage); lastStage=S.ev; if(S.ev==KG_CASTLE) crownDay=(int)R.day; S.ev=0; }
            CHECK(R.castle>=lastStage,"the stage went backwards"); if(k%5000==0){ int e=ksCheck(&S); CHECK(e==0,"run %d frame %ld: ksCheck %d",run,k,e); }
            if(crownDay>=0&&(int)R.day>=crownDay+5) break; if(fails>20) break; }
        CHECK(ns==4&&seq[0]==1&&seq[1]==2&&seq[2]==3&&seq[3]==4,"run %d: %d stage events (castle %d at day %u)",run,ns,R.castle,(unsigned)R.day);
        int ci=typeAt(KB_CASTLE); CHECK(ci>=0&&B[ci].lvl==1&&R.castle==KG_CASTLE,"run %d: the castle is crowned",run);
        CHECK(ksCheck(&S)==0,"run %d: the books %d",run,ksCheck(&S)); int built=0; for(int i=0;i<R.nB;i++) built+=B[i].lvl; CHECK(built==R.nB,"run %d: %d of %d built",run,built,R.nB);
        printf("  run %d: crowned on day %d (%d buildings), then 5 more days, books balanced; delivered %u, cloth left %ld, polished left %ld\n",run,crownDay,R.nB,S.delivered,ksHeld(&S,G_CLOTH),ksHeld(&S,G_POLISH)); }
    // 5 save and load at every stage, and the loader's repairs
    { for(int s=0;s<=KG_CASTLE;s++) for(int lot=0;lot<(s<KG_CASTLE?2:1);lot++){
          CHECK(scene(60u+(u32)s,0)==KSE_OK,"init"); CHECK(ksPlace(&S,KB_CASTLE,60,60,0)==0,"placed"); R.castle=(u8)s; B[0].work=(u8)lot; B[0].lvl=(u8)(s==KG_CASTLE); B[0].stock=0;
          int g=0,n=0; if(s<KG_CASTLE){ ksCost(&S,&B[0],&g,&n); ksSetIn(&B[0],n-2>0?n-2:0); }
          ksGift(&S,G_WOOD,5); CHECK(ksInit(&S,&R,B,cells)==KSE_OK&&ksCheck(&S)==0,"re-init stage %d lot %d",s,lot);
          ksPrepSave(&S); int len=kgEncode(buf,sizeof buf,&R,B,cells); CHECK(len>0,"encode %d",len); if(len<=0) continue;
          memset(&R2,0,sizeof R2); memset(B2,0,sizeof B2); memset(c2,0xEE,sizeof c2);
          CHECK(kgDecode(buf,len,&R2,B2,0,0)==0&&kgDecode(buf,len,&R2,B2,c2,1)==0,"decode stage %d lot %d",s,lot);
          CHECK(ksInit(&S2,&R2,B2,c2)==KSE_OK&&ksCheck(&S2)==0,"ksInit of the loaded realm");
          CHECK(R2.castle==s&&B2[0].type==KB_CASTLE&&B2[0].work==B[0].work&&B2[0].stock==B[0].stock&&B2[0].lvl==B[0].lvl,"stage %d lot %d came back as stage %d lot %d stock %d lvl %d",s,lot,R2.castle,B2[0].work,B2[0].stock,B2[0].lvl);
          if(s<KG_CASTLE){ int g2=0,n2=0; ksCost(&S2,&B2[0],&g2,&n2); CHECK(g2==g&&n2==n,"the lot asks for the same after the load"); }
          for(long k=0;k<3000;k++) ksStep(&S2); CHECK(ksCheck(&S2)==0,"books after running the loaded realm"); }
      // the repairs: a crowned realm gets a finished castle, any other realm a site at its stage; two castles are refused
      CHECK(scene(70u,0)==KSE_OK,"init"); ksPlace(&S,KB_CASTLE,60,60,0); R.castle=KG_CASTLE; B[0].lvl=0; B[0].work=1; B[0].stock=3; CHECK(ksInit(&S,&R,B,cells)==KSE_OK&&B[0].lvl==1&&B[0].work==0&&B[0].stock==0&&ksCheck(&S)==0,"a crowned realm with a castle site is mended");
      CHECK(scene(71u,0)==KSE_OK,"init"); ksPlace(&S,KB_CASTLE,60,60,0); R.castle=1; B[0].lvl=1; B[0].work=0; B[0].stock=0; CHECK(ksInit(&S,&R,B,cells)==KSE_OK&&B[0].lvl==0&&ksCheck(&S)==0,"a half built realm with a finished castle is mended into a site");
      CHECK(scene(72u,0)==KSE_OK,"init"); ksPlace(&S,KB_CASTLE,60,60,0); B[R.nB]=B[0]; B[R.nB].x=70; for(int j=0;j<2;j++) for(int q=0;q<2;q++) cells[(60+j)*96+70+q]|=KC_BLD; R.nB++; CHECK(ksInit(&S,&R,B,cells)==KSE_ARG,"two castles are refused");
      CHECK(scene(73u,0)==KSE_OK,"init"); ksPlace(&S,KB_CASTLE,60,60,0); B[0].work=9; B[0].stock=0xFFFF; CHECK(ksInit(&S,&R,B,cells)==KSE_OK&&B[0].work<=1&&ksIn(&B[0])<=KS_BUF&&ksCheck(&S)==0,"a damaged castle site is clamped");
      printf("  saved and loaded at all 9 stage and lot states, and the 4 repairs\n"); }
    printf("%s: %d failure(s)\n",fails?"kingcastle: FAILED":"kingcastle: ALL PASSED",fails); return fails?1:0;
}
