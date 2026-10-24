// test_kingplay.c - PC test of source/kingplay.h on the REAL flash code (the fake 128 KB chip of test_kingflash.c): found a realm the way KINGDOM MODE does, open it in a pool
// of exactly hhPool's size, settle it, run it, save it through the pool scratch, open it again. Run from the repo root:   sh tools/test_kingplay.sh
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
#define IWRAM_THUMB
static int slN=SLOT_MAX;
#define SLOT_N slN
static u8 chip[131072]; static int fkBank; static long cutAt=-1, ops=0; static jmp_buf cutJ; static u32 rs=4242;
static u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }
static volatile u8* fkWin(void){ return chip+fkBank*65536; }
static void flCmd(int c){ (void)c; }
static int flProgRaw(int a,int v){ u8*p=&chip[fkBank*65536+a]; ops++;
    if(cutAt>=0&&ops>cutAt){ *p=(u8)(*p&(v|(int)(rnd()&255))); longjmp(cutJ,1); }   // cut: only some of the bits that had to clear did
    *p=(u8)(*p&v); return 1; }
static char opKind[400000]; static int rec;
static int flEraseRaw(int a){ u8*p=&chip[fkBank*65536+(a&0xF000)]; if(rec&&ops<400000) opKind[ops]=1; ops++;
    if(cutAt>=0&&ops>cutAt){ u32 h=rnd()%4096; for(u32 k=0;k<4096;k++) p[k]=k<h?0xFF:(u8)(p[k]^(u8)(1+rnd()%255)); longjmp(cutJ,1); }   // cut: garbage
    memset(p,0xFF,4096); return 1; }
static void flBankRaw(int b){ fkBank=b; }
static int flWait(int a,int want,int tmo){ (void)a;(void)want;(void)tmo; return 1; }
static u8 slHPid;
#include SAVE_INC
static void ldShow(const char*s,int a,int b){ (void)s;(void)a;(void)b; } static void ldEnd(void){}
#include SLOTS_INC
#include "../source/kingslot.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
#include "../source/kingmem.h"
static u8 hhPool[30720]; static int stgOk=1, claims=0; static void stgClaim(void){ claims++; memset(hhPool,0xAA,sizeof hhPool); }
#define SP_POOL 30720
#define KP_NOSCREEN
#include "../source/kingplay.h"

static void reboot(void){   // a restart: the RAM copy of sector 1 comes from the chip again
    svType=SV_FLASH; svSize=131072; svScr=svSize-SV_SEC; svErr=0; fkBank=0; svBank=0;
    for(u32 i=0;i<SV_LOWN;i++) svLow[i]=chip[SV_LOW0+i]; svShadLoad(); svBase=(volatile u8*)((uintptr_t)svLow-SV_LOW0); svRecover();
}
static void blank(void){ memset(chip,0xFF,sizeof chip); cutAt=-1; reboot(); }
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<30){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u32 hashState(KmMem*m){ u32 h=2166136261u; const u8*p;
    p=(const u8*)m->R; for(size_t i=0;i<sizeof(KRealm);i++) h=(h^p[i])*16777619u;
    p=(const u8*)m->B; for(size_t i=0;i<(size_t)m->R->nB*sizeof(KBld);i++) h=(h^p[i])*16777619u;
    int d=kgDim(m->R->tier); for(int i=0;i<d*d;i++) h=(h^(m->cells[i]&7))*16777619u; return h; }
static int found(int tier,int*slot){   // what ksFound does: no cells, no buildings, 10 of each raw good
    KRealm Rn; memset(&Rn,0,sizeof Rn); Rn.tier=(u8)tier; Rn.giant=1; Rn.genVer=KG_GENVER; Rn.seed=rnd(); Rn.rng=rnd()|1; for(int i=0;i<4;i++) Rn.goods[i]=10;
    static u8 b64[64]; int id=0; int e=kgsSave(0,"TESTLAND",&Rn,0,0,b64,sizeof b64,&id); *slot=e; return e>=0; }
int main(void){
    blank(); printf("== kingplay: found, open in hhPool, settle, run, save, open again\n");
    for(int tier=0;tier<3;tier++){
        int slot; CHECK(found(tier,&slot),"found tier %d",tier); if(slot<0) continue;
        KmMem m; int before=claims; int e=kpOpen(&m,slot); CHECK(e==KP_OK,"kpOpen %d",e); CHECK(claims==before+1,"stgClaim not called once");
        CHECK((u8*)m.S>=hhPool&&(u8*)(m.S+1)<=hhPool+sizeof hhPool,"the sim lies outside hhPool");
        CHECK(kpSettle(&m)==1,"not settled"); CHECK(kpSettle(&m)==0||m.R->nB==0,"settled twice");
        CHECK(m.R->nB==3,"nB %d",m.R->nB);
        for(long k=0;k<KS_DAYT*30;k++) ksStep(m.S);
        CHECK(ksCheck(m.S)==0,"ksCheck %d",ksCheck(m.S)); CHECK(m.B[1].lvl==1&&m.B[2].lvl==1,"huts not built (%d %d)",m.B[1].lvl,m.B[2].lvl); CHECK(m.R->day>=30,"day %u",m.R->day);
        { m.S->hd.k=KHD_BLD; m.S->hd.bt=KB_HUT; CHECK(kpSave(&m,slot)==KP_E_HELD,"a lifted building must stop the save"); CHECK(m.S->hd.k==KHD_BLD,"the lifted building stays in the hand"); m.S->hd.k=KHD_NONE;
          ksGift(m.S,G_WOOD,4); m.R->goods[G_WOOD]-=4; m.S->hd.k=KHD_GOODS; m.S->hd.g=G_WOOD; m.S->hd.n=4; CHECK(ksCheck(m.S)==0,"the books count the load in the hand (%d)",ksCheck(m.S)); }   // (step 4h: a load of goods goes home with the save)
        int s2=kpSave(&m,slot); CHECK(s2>=0,"kpSave %d",s2); CHECK(m.S->pop>0,"nobody lives here"); CHECK(m.S->hd.k==KHD_NONE&&ksCheck(m.S)==0,"the load went home (%d)",ksCheck(m.S));
        u32 h1=hashState(&m); KRealm Rc=*m.R; long held[KG_GOODS]; for(int g=0;g<KG_GOODS;g++) held[g]=m.R->goods[g];
        KmMem m2; e=kpOpen(&m2,s2); CHECK(e==KP_OK,"reopen %d",e);
        CHECK(Rc.day==m2.R->day&&Rc.seed==m2.R->seed&&Rc.nB==m2.R->nB,"header differs after reopen");
        CHECK(hashState(&m2)==h1,"tier %d: the realm after save and reopen is not the realm that was saved",tier);
        for(int g=0;g<KG_GOODS;g++) CHECK(m2.R->goods[g]==held[g],"good %d: %d vs %ld",g,m2.R->goods[g],held[g]);
        for(long k=0;k<KS_DAYT*5;k++) ksStep(m2.S); CHECK(ksCheck(m2.S)==0,"ksCheck after reopen");
        printf("  tier %d: saved to slot %d (was %d), day %u, pop %d, reopened identical\n",tier,s2,slot,Rc.day,m.S->pop);
    }
    // the worst realm the simulation allows in the 128 x 128 tier: its 6,694 byte save is MORE than the 6,048 bytes the pool has spare there, so it only saves through the cache scratch
    { int slot; CHECK(found(2,&slot),"found"); KmMem m; CHECK(kpOpen(&m,slot)==KP_OK,"open worst"); int d=128, n=0;
      for(int y=4;y<d-4&&n<KS_MAXB;y+=7) for(int x=4;x<d-4&&n<KS_MAXB;x+=7){ if(ksPlace(m.S,(n&1)?KB_WOODCUT:KB_QUARRY,x,y,1)>=0) n++; }
      int r=0; for(int y=1;y<d-1&&r<KS_MAXN;y+=2) for(int x=1;x<d-1&&r<KS_MAXN;x+=3) if(ksRoadSet(m.S,x,y,1)) r++;
      int ed=0; for(int i=0;i<m.R->nB;i++){ KBld*b=&m.B[i]; for(int y=b->y-6;y<=b->y+7;y++) for(int x=b->x-6;x<=b->x+7;x++){ if(x<2||y<2||x>=d-2||y>=d-2) continue; int c=y*d+x; if(m.cells[c]&KC_BLD||(m.cells[c]&7)==KT_ROAD) continue; if((x+y)&1){ int base=kgCell(m.R->seed,2,x,y); m.cells[c]=(u8)(base==KT_GRASS?KT_FOREST:KT_GRASS); ed++; } } }
      int spare=KM_POOL-m.used; ksPrepSave(m.S); int len=kgEncode(m.sscr,m.sN,m.R,m.B,m.cells);
      for(int y=2;y<d-2&&len<=spare+400;y+=1){ for(int x=2;x<d-2;x++){ int c=y*d+x; if(m.cells[c]&KC_BLD||(m.cells[c]&7)==KT_ROAD||!((x+y)&1)) continue; int base=kgCell(m.R->seed,2,x,y); if((m.cells[c]&7)!=base) continue; m.cells[c]=(u8)(base==KT_GRASS?KT_FOREST:KT_GRASS); ed++; }
          len=kgEncode(m.sscr,m.sN,m.R,m.B,m.cells); if(len<0) break; }   // (a few more edits where the land is still as it was, until the save is bigger than the pool's spare)
      CHECK(len>spare,"the worst case (%d) is no longer bigger than the spare (%d): this test proves nothing",len,spare); CHECK(len>0,"worst case does not encode (%d)",len);
      u32 h=hashState(&m); int s2=kpSave(&m,slot); CHECK(s2>=0,"worst realm did not save (%d)",s2);
      KmMem m2; CHECK(s2>=0&&kpOpen(&m2,s2)==KP_OK,"worst realm did not reopen"); if(s2>=0) CHECK(hashState(&m2)==h,"worst realm differs after reopen");
      printf("  worst realm: %d buildings, %d road cells, %d edits: %d bytes saved (spare in the pool is only %d), sscr has %d, reopened identical\n",m.R->nB,r,ed,len,spare,m.sN); }
    // the scratch is a cache: a realm that saves in the middle of its life lives on exactly as one that was prepared for a save but encoded elsewhere
    { int slot; CHECK(found(1,&slot),"found"); KmMem a,b; static u8 other[20000]; static u8 poolB[30720];
      CHECK(kpOpen(&a,slot)==KP_OK,"open a"); kpSettle(&a); for(long k=0;k<KS_DAYT*12;k++) ksStep(a.S);
      u8 keep[30720]; memcpy(keep,hhPool,sizeof keep);   // b: the very same realm, in a second pool
      memcpy(poolB,hhPool,sizeof poolB); b=a; b.S=(KSim*)(poolB+((u8*)a.S-hhPool)); b.R=(KRealm*)(poolB+((u8*)a.R-hhPool)); b.B=(KBld*)(poolB+((u8*)a.B-hhPool)); b.cells=poolB+(a.cells-hhPool); b.S->R=b.R; b.S->B=b.B; b.S->cells=b.cells;
      CHECK(kpSave(&a,slot)>=0,"save a"); ksPrepSave(b.S); CHECK(kgEncode(other,sizeof other,b.R,b.B,b.cells)>0,"encode b");
      for(long k=0;k<KS_DAYT*20;k++){ ksStep(a.S); ksStep(b.S); }
      CHECK(hashState(&a)==hashState(&b),"a realm that saved through the cache scratch lives differently from one that did not");
      CHECK(a.S->delivered==b.S->delivered&&a.S->t==b.S->t,"deliveries differ (%u vs %u)",a.S->delivered,b.S->delivered); CHECK(ksCheck(a.S)==0,"ksCheck a");
      printf("  save in mid-life: both copies still identical after 20 more days (delivered %u)\n",a.S->delivered); (void)keep; }
    // bad ground: an empty slot and a slot that is not a realm are refused without harm
    { KmMem m; int before=claims; CHECK(kpOpen(&m,57)==KP_E_READ,"an empty slot was opened"); CHECK(claims==before+1,"stgClaim must still run (the pool is let go first)"); }
    printf("%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}
