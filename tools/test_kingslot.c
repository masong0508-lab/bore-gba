// test_kingslot.c - see test_kingslot.sh. A fake 128 KB flash chip (bits only go to 0 until a sector is erased) that can lose power at any write
// or erase (an erase cut half way leaves garbage), the real header/scan/verify code from slots.h, and source/kingslot.h on top of it.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <setjmp.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8;
#include "../source/kingdom.h"

#define SLOT_BASE 8192
#define SLOT_SZ 2048
#define SLOT_HDR 32
#define SLOT_NAME 10
#define SLOT_MAX 58
#define SLOT_DIR 4992
#define MW 20
#define MH 15
#define EWRAM_BSS
static int slN=SLOT_MAX;
#define SLOT_N slN
static u8 flash[131072]; static u8 svErr; static u8 slHPid;
#define SRAM_BASE flash
static long cutAt=-1, ops=0; static jmp_buf cutJ;
static void tick(void){ ops++; if(cutAt>=0&&ops>cutAt) longjmp(cutJ,1); }
static int svWr(u32 off,int v){ tick(); if(off>=sizeof flash||(flash[off]&v)!=v){ svErr=1; return 0; } flash[off]=(u8)v; return 1; }
static u8 svRd(u32 off){ return flash[off]; }
static volatile u8* svPtr(u32 off){ return flash+off; }
static void svErase(u32 off,u32 len){ for(u32 i=0;i<len;i+=SLOT_SZ){ if(cutAt>=0&&ops+1>cutAt){ for(u32 k=0;k<SLOT_SZ/2;k++) flash[off+i+k]=0xFF; for(u32 k=SLOT_SZ/2;k<SLOT_SZ;k++) flash[off+i+k]=(u8)(flash[off+i+k]^0x5A); } tick(); memset(flash+off+i,0xFF,SLOT_SZ); } }
static void svCommit(void){}
static void ldShow(const char*s,int a,int b){ (void)s;(void)a;(void)b; } static void ldEnd(void){}
#include SLOTS_INC
#include "../source/kingslot.h"

static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<30){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u32 rs=777; static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static u8 cells[128*128], cells2[128*128], buf[KGS_MAXSPAN*SLOT_SZ], tmp[KGS_MAXSPAN*SLOT_SZ];
static KRealm R, R2; static KBld B[KG_MAXB], B2[KG_MAXB];
static void blank(void){ memset(flash,0xFF,sizeof flash); svErr=0; }
static u32 fixSeed;   // 0: a random seed; else the same land again (a later state of the same realm)
static void scenario(int tier,int bld,int roads,int cleared){
    int d=kgDim(tier); memset(&R,0,sizeof R); R.tier=(u8)tier; R.giant=(u8)(rnd()%KG_GIANTS); R.castle=(u8)(rnd()%5); R.genVer=KG_GENVER; R.seed=fixSeed?fixSeed:rnd(); R.day=rnd()%100000; R.rng=rnd();
    for(int i=0;i<KG_GOODS;i++) R.goods[i]=(u16)(rnd()%2000);
    kgGen(cells,R.seed,tier);
    for(int k=0;k<cleared;k+=12){ int cx=(int)(rnd()%d), cy=(int)(rnd()%d); for(int j=-2;j<=2;j++)for(int i=-2;i<=2;i++){ int x=cx+i,y=cy+j; if(x<0||y<0||x>=d||y>=d) continue; int t=cells[y*d+x]&7; if(t==KT_FOREST||t==KT_ROCK||t==KT_CRYSTAL||t==KT_MEADOW) cells[y*d+x]=KT_GRASS; } }
    for(int k=0;k<roads;){ int h=rnd()&1, a=(int)(rnd()%d), b=(int)(rnd()%d), len=5+(int)(rnd()%25); for(int i=0;i<len&&k<roads;i++,k++){ int x=h?(b+i)%d:a, y=h?a:(b+i)%d; if((cells[y*d+x]&7)!=KT_WATER) cells[y*d+x]=KT_ROAD; } }
    R.nB=(u16)bld; for(int i=0;i<bld;i++){ B[i].type=(u8)(rnd()%KG_BTYPES); B[i].x=(u8)(rnd()%(d-2)); B[i].y=(u8)(rnd()%(d-2)); B[i].lvl=(u8)(rnd()%4); B[i].work=(u8)(rnd()%10); B[i].stock=(u16)(rnd()%500); }
    kgStamp(cells,tier,B,bld);
}
// a saved realm to compare with: keep its pieces
typedef struct { KRealm R; KBld B[KG_MAXB]; u8 cells[128*128]; } Snap;
static Snap SA, SB, SC;
static void take(Snap*s){ s->R=R; memcpy(s->B,B,sizeof B); memcpy(s->cells,cells,sizeof cells); }
static void give(const Snap*s){ R=s->R; memcpy(B,s->B,sizeof B); memcpy(cells,s->cells,sizeof cells); }
static int sameAs(const Snap*s){   // the realm just loaded into R2,B2,cells2 is this snapshot
    const KRealm*a=&s->R; if(a->tier!=R2.tier||a->giant!=R2.giant||a->castle!=R2.castle||a->seed!=R2.seed||a->day!=R2.day||a->rng!=R2.rng||a->nB!=R2.nB||a->flags!=R2.flags) return 0;
    for(int i=0;i<KG_GOODS;i++) if(a->goods[i]!=R2.goods[i]) return 0;
    for(int i=0;i<a->nB;i++) if(s->B[i].type!=B2[i].type||s->B[i].x!=B2[i].x||s->B[i].y!=B2[i].y||s->B[i].lvl!=B2[i].lvl||s->B[i].work!=B2[i].work||s->B[i].stock!=B2[i].stock) return 0;
    return memcmp(s->cells,cells2,kgCells(a->tier))==0;
}
static int load(int slot){ memset(cells2,0xEE,sizeof cells2); return kgsLoad(slot,&R2,B2,cells2,tmp,(int)sizeof tmp); }
static int save(int id,const char*nm,int*idOut){ return kgsSaveI(id,nm,&R,B,cells,buf,(int)sizeof buf,idOut); }
// a filler slot of another kind, to take up room
static void filler(int slot,int span){ slOpen(slot,span); slHeader(slot,SLK_ROOM,span,1,10,0,1,"FILL"); }
static int count(void){ int l[KGS_LISTMAX]; slScan(); return kgsList(l,KGS_LISTMAX); }
static int copies(int id){ int n=0; slScan(); for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING&&slI[i].pid==id&&slGood[i]) n++; return n; }

int main(void){
    // 1 a new realm: saved, listed, loaded back the same, named
    blank(); scenario(1,60,500,800); take(&SA); int id=0, slot=save(0,"MY REALM",&id);
    CHECK(slot>=0&&id==1,"first save gave slot %d id %d",slot,id);
    { int l[KGS_LISTMAX]; slScan(); int n=kgsList(l,KGS_LISTMAX); CHECK(n==1&&l[0]==slot,"list: %d realms",n); CHECK(!strcmp(slI[slot].name,"MY REALM"),"name '%s'",slI[slot].name); CHECK(slI[slot].kind==SLK_KING&&slI[slot].pid==1,"kind/id"); }
    CHECK(load(slot)==SLE_OK&&sameAs(&SA),"loaded realm differs");
    { int t,g,c; u32 d; CHECK(kgsPeek(slot,&t,&g,&c,&d)&&t==SA.R.tier&&g==SA.R.giant&&c==SA.R.castle&&d==SA.R.day,"peek"); }
    // 2 saving again: same number, a fresh copy, the old one gone, name kept, seq counts up
    fixSeed=SA.R.seed; scenario(1,90,700,900); fixSeed=0; take(&SB);   // (a later state of the same realm: same seed, more built)
    { int id2=0; int s2=save(1,0,&id2); CHECK(s2>=0&&id2==1&&s2!=slot,"update gave slot %d (was %d) id %d",s2,slot,id2);
      CHECK(copies(1)==1,"%d copies after an update",copies(1)); CHECK(count()==1,"list after update"); CHECK(!strcmp(slI[s2].name,"MY REALM"),"update lost the name '%s'",slI[s2].name); CHECK(slI[s2].seq==2,"seq %d",slI[s2].seq);
      CHECK(load(s2)==SLE_OK&&sameAs(&SB),"updated realm differs"); }
    // 3 two realms, delete one
    scenario(0,10,60,100); take(&SC); { int idc=0; int s3=save(0,"HAMLET",&idc); CHECK(s3>=0&&idc==2,"second realm id %d",idc); CHECK(count()==2,"two realms"); CHECK(kgsDelete(1)==1,"delete freed"); CHECK(count()==1,"one left");
      slScan(); int l[KGS_LISTMAX]; kgsList(l,KGS_LISTMAX); CHECK(load(l[0])==SLE_OK&&sameAs(&SC),"the other realm was touched"); CHECK(kgsNewId()==1,"the freed number is free again"); }
    // 4 no room: refused, nothing touched
    blank(); scenario(2,200,1500,3000); take(&SA); { int idd=0; int s=save(0,"BIG",&idd); CHECK(s>=0,"big save %d",s); int span=slI[s].span; (void)span;
      slScan(); for(int i=0;i<SLOT_N;){ if(slOwner[i]>=0){ i++; continue; } filler(i,1); i++; }   // every free slot taken
      slScan(); fixSeed=SA.R.seed; scenario(2,210,1500,3000); fixSeed=0; int e=save(idd,0,0); CHECK(e==SLE_NOROOM,"full flash gave %d",e);
      CHECK(load(s)==SLE_OK&&sameAs(&SA),"the old copy did not survive a refused save"); }
    // 5 a damaged copy is refused and loads nothing
    blank(); scenario(1,60,500,800); take(&SA); { int idd=0; int s=save(0,"DAMAGE",&idd); slScan(); int len=slI[s].len; int bad=0;
      for(int k=0;k<40;k++){ int at=SLOT_HDR+(int)(rnd()%(u32)len); u8 keep=flash[SLO(s)+at]; flash[SLO(s)+at]^=(u8)(1+rnd()%255); R2.nB=999; memset(cells2,0xEE,sizeof cells2); int e=kgsLoad(s,&R2,B2,cells2,tmp,(int)sizeof tmp);
        if(e!=SLE_OK){ bad++; CHECK(R2.nB==999&&cells2[0]==0xEE&&cells2[100]==0xEE,"a refused load changed the realm in play"); } flash[SLO(s)+at]=keep; }
      CHECK(bad>=38,"only %d of 40 damaged copies refused",bad); CHECK(load(s)==SLE_OK&&sameAs(&SA),"restored copy"); printf("damaged copies refused: %d of 40\n",bad); }
    // 6 POWER CUT while saving a new realm, at every write: never a damaged or half realm
    { blank(); scenario(1,40,300,400); take(&SA); long total=0; { ops=0; cutAt=-1; int i0=0; save(0,"NEWONE",&i0); total=ops; }
      int none=0, whole=0;
      for(long k=0;k<=total;k++){ blank(); give(&SA); ops=0; cutAt=k; int idd=0;
        if(!setjmp(cutJ)){ save(0,"NEWONE",&idd); } cutAt=-1;
        slScan(); int l[KGS_LISTMAX]; int n=kgsList(l,KGS_LISTMAX);
        for(int i=0;i<SLOT_N;i++) CHECK(!(slOwner[i]==i&&slI[i].kind==SLK_KING)||slGood[i],"cut at %ld left a damaged realm in slot %d",k,i);
        if(n==0) none++; else { whole++; CHECK(n==1&&load(l[0])==SLE_OK&&sameAs(&SA),"cut at %ld left a wrong realm",k); } }
      printf("power cut during a first save: %ld cut points, %d left nothing, %d left the whole realm\n",total+1,none,whole); CHECK(none>0&&whole>0,"the sweep did not cover both outcomes"); }
    // 7 POWER CUT while saving over a realm that exists, at every write (small) and many (a 4 slot realm): the old or the new, whole; the other realm untouched
    { struct { int tier,bld,roads,cleared,stride; const char*nm; } T[]={ {1,40,300,400,1,"realm"},{2,250,3000,6500,97,"kingdom (many slots)"} };
      for(int t=0;t<2;t++){
        blank(); scenario(T[t].tier,T[t].bld,T[t].roads,T[t].cleared); take(&SA); scenario(0,8,40,80); take(&SC);
        int ia=0, ib=0; give(&SA); save(0,"FIRST",&ia); { slScan(); int fs=kgsFind(ia); CHECK(fs>=0,"the test realm did not save"); if(fs>=0) printf("(the realm under test spans %d slots)\n",slI[fs].span); } give(&SC); save(0,"SECOND",&ib);
        fixSeed=SA.R.seed; scenario(T[t].tier,T[t].bld+20>KG_MAXB?KG_MAXB:T[t].bld+20,T[t].roads+100,T[t].cleared+200); fixSeed=0; take(&SB);
        static u8 base[131072]; memcpy(base,flash,sizeof flash);
        long total=0; { give(&SB); ops=0; cutAt=-1; save(ia,0,0); total=ops; }
        int old=0, nw=0, both=0, k, pts=0;
        for(k=0;k<=total;k+=(k<60||k>total-60)?1:T[t].stride){ memcpy(flash,base,sizeof flash); svErr=0; give(&SB); ops=0; cutAt=k;
          if(!setjmp(cutJ)){ save(ia,0,0); } cutAt=-1; pts++;
          slScan(); int c=copies(ia); if(c==2) both++;
          int s=kgsFind(ia); CHECK(s>=0,"cut at %d lost the realm",k);
          if(s>=0){ int e=load(s); CHECK(e==SLE_OK,"cut at %d: load gave %d",k,e); if(e==SLE_OK){ if(sameAs(&SA)) old++; else if(sameAs(&SB)) nw++; else CHECK(0,"cut at %d: the realm is neither the old nor the new",k); } }
          for(int i=0;i<SLOT_N;i++) CHECK(!(slOwner[i]==i&&slI[i].kind==SLK_KING)||slGood[i],"cut at %d left a damaged copy",k);
          slScan(); int lb=kgsFind(ib); CHECK(lb>=0&&load(lb)==SLE_OK&&sameAs(&SC),"cut at %d touched the other realm",k);
          CHECK(count()==2,"cut at %d: %d realms listed",k,count());
          kgsHeal(ia); CHECK(copies(ia)==1,"cut at %d: %d copies after heal",k,copies(ia));
          { int e2=save(ia,0,0); CHECK(e2>=0,"cut at %d: the next save failed (%d)",k,e2); }   // the realm can be saved again after any cut
        }
        printf("power cut over an existing %s: %d cut points, old realm kept %d, new realm %d, both copies whole %d\n",T[t].nm,pts,old,nw,both); CHECK(old>0&&nw>0,"the sweep did not cover both outcomes"); }
    }
    // 8 a bad argument and a missing realm
    blank(); scenario(1,5,10,10); CHECK(save(9,0,0)==SLE_EMPTY,"saving a realm that is not there"); R.tier=3; CHECK(save(0,0,0)==SLE_BAD,"a bad tier"); R.tier=1; CHECK(kgsDelete(9)==0,"deleting nothing");
    // 9 a slot of another kind is never a realm
    blank(); filler(0,1); { SlInfo I; CHECK(slInfo(0,&I)&&I.kind==SLK_ROOM,"filler"); CHECK(kgsLoad(0,&R2,B2,cells2,tmp,(int)sizeof tmp)==SLE_EMPTY,"a room loaded as a realm"); CHECK(count()==0,"a room listed as a realm"); }
    // 10 FOUNDING: a new realm saved with no map and no buildings (cells 0, a 64 byte scratch) comes back as the made land, in every size
    for(int t=0;t<3;t++){ blank(); memset(&R,0,sizeof R); R.tier=(u8)t; R.giant=(u8)(t+1); R.genVer=KG_GENVER; R.seed=9000u+(u32)t; R.rng=5; R.goods[0]=10;
        u8 sb[64]; int fid=0; int fs=kgsSaveI(0,"FOUNDED",&R,0,0,sb,(int)sizeof sb,&fid); CHECK(fs>=0&&fid==1,"founding gave %d",fs);
        if(fs>=0){ CHECK(load(fs)==SLE_OK&&R2.tier==t&&R2.giant==t+1&&R2.nB==0&&R2.goods[0]==10&&R2.seed==9000u+(u32)t,"a founded realm did not load"); kgGen(cells,R.seed,t); CHECK(!memcmp(cells,cells2,kgCells(t)),"a founded realm has the wrong land (tier %d)",t); CHECK(slI[fs].span==1,"a new realm should take one slot"); } }
    printf(fails?"\n%d CHECK(S) FAILED\n":"\nall checks passed\n",fails);
    return fails?1:0;
}
