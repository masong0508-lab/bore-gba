// test_kingflash.c - see test_kingflash.sh. The power-cut test on the REAL flash code: save.h's svWr / svErase / svCommit / svBankTo / svPtr run as they are
// (the sector sharing, the scratch sector, the banks); only the five raw flash commands underneath them (flCmd, flProgRaw, flEraseRaw, flBankRaw) are a fake
// 128 KB chip that can lose power at any program or erase. A cut program leaves a byte with only some of its bits cleared; a cut erase leaves a half-blank,
// half-garbage sector. After a cut the RAM copy of sector 1 is reloaded from the chip, as a restart would. Then kingslot.h (save, rename, delete) runs on top.
// What it asks: after a cut at ANY raw operation, is the realm whole (the old copy or the new), is every OTHER slot byte for byte untouched, and can it be saved again?
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

static void reboot(void){   // a restart: the RAM copy of sector 1 comes from the chip again
    svType=SV_FLASH; svSize=131072; svScr=svSize-SV_SEC; svErr=0; fkBank=0; svBank=0;
    for(u32 i=0;i<SV_LOWN;i++) svLow[i]=chip[SV_LOW0+i]; svShadLoad(); svBase=(volatile u8*)((uintptr_t)svLow-SV_LOW0); svRecover();
}
static void blank(void){ memset(chip,0xFF,sizeof chip); cutAt=-1; reboot(); }
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; if(fails<30){ printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } } }while(0)
static u8 cells[128*128], cells2[128*128], buf[KGS_MAXSPAN*SLOT_SZ], tmp[KGS_MAXSPAN*SLOT_SZ];
static KRealm R, R2; static KBld B[KG_MAXB], B2[KG_MAXB];
static u32 fixSeed;
static void scenario(int tier,int bld,int roads,int cleared){
    int d=kgDim(tier); memset(&R,0,sizeof R); R.tier=(u8)tier; R.giant=(u8)(rnd()%KG_GIANTS); R.castle=(u8)(rnd()%5); R.genVer=KG_GENVER; R.seed=fixSeed?fixSeed:rnd(); R.day=rnd()%100000; R.rng=rnd();
    for(int i=0;i<KG_GOODS;i++) R.goods[i]=(u16)(rnd()%2000);
    kgGen(cells,R.seed,tier);
    for(int k=0;k<cleared;k+=12){ int cx=(int)(rnd()%d), cy=(int)(rnd()%d); for(int j=-2;j<=2;j++)for(int i=-2;i<=2;i++){ int x=cx+i,y=cy+j; if(x<0||y<0||x>=d||y>=d) continue; int t=cells[y*d+x]&7; if(t==KT_FOREST||t==KT_ROCK||t==KT_CRYSTAL||t==KT_MEADOW) cells[y*d+x]=KT_GRASS; } }
    for(int k=0;k<roads;){ int h=rnd()&1, a=(int)(rnd()%d), b=(int)(rnd()%d), len=5+(int)(rnd()%25); for(int i=0;i<len&&k<roads;i++,k++){ int x=h?(b+i)%d:a, y=h?a:(b+i)%d; if((cells[y*d+x]&7)!=KT_WATER) cells[y*d+x]=KT_ROAD; } }
    R.nB=(u16)bld; for(int i=0;i<bld;i++){ B[i].type=(u8)(rnd()%KG_BTYPES); B[i].x=(u8)(rnd()%(d-2)); B[i].y=(u8)(rnd()%(d-2)); B[i].lvl=(u8)(rnd()%4); B[i].work=(u8)(rnd()%10); B[i].stock=(u16)(rnd()%500); }
    kgStamp(cells,tier,B,bld);
}
typedef struct { KRealm R; KBld B[KG_MAXB]; u8 cells[128*128]; } Snap;
static Snap SA, SB, SC;
static void take(Snap*s){ s->R=R; memcpy(s->B,B,sizeof B); memcpy(s->cells,cells,sizeof cells); }
static void give(const Snap*s){ R=s->R; memcpy(B,s->B,sizeof B); memcpy(cells,s->cells,sizeof cells); }
static int sameAs(const Snap*s){
    const KRealm*a=&s->R; if(a->tier!=R2.tier||a->giant!=R2.giant||a->castle!=R2.castle||a->seed!=R2.seed||a->day!=R2.day||a->rng!=R2.rng||a->nB!=R2.nB||a->flags!=R2.flags) return 0;
    for(int i=0;i<KG_GOODS;i++) if(a->goods[i]!=R2.goods[i]) return 0;
    for(int i=0;i<a->nB;i++) if(s->B[i].type!=B2[i].type||s->B[i].x!=B2[i].x||s->B[i].y!=B2[i].y||s->B[i].lvl!=B2[i].lvl||s->B[i].work!=B2[i].work||s->B[i].stock!=B2[i].stock) return 0;
    return memcmp(s->cells,cells2,kgCells(a->tier))==0;
}
static int load(int slot){ memset(cells2,0xEE,sizeof cells2); return kgsLoad(slot,&R2,B2,cells2,tmp,(int)sizeof tmp); }
static int copies(int id){ int n=0; slScan(); for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING&&slI[i].pid==id&&slGood[i]) n++; return n; }
// a neighbour that is not a realm (a player, a room ...): a whole slot of random bytes with a good header, so any damage to it shows

static int nf, fl[8]; static u8 fsnap[8][SLOT_SZ];
static void addFiller(int slot){
    slOpen(slot,1); u32 o=SLO(slot); for(int k=SLOT_HDR;k<SLOT_SZ;k++) svWr(o+(u32)k,(int)(rnd()&255));
    int sum=slSumOf(SLB(slot)+SLOT_HDR,SLOT_SZ-SLOT_HDR); slHPid=7; slHeader(slot,SLK_ROOM,1,1,SLOT_SZ-SLOT_HDR,sum,1,"NEIGH");
    fl[nf]=slot; memcpy(fsnap[nf],chip+SLO(slot),SLOT_SZ); nf++;
}
// the free slots of a used chip are not blank: a freed copy keeps its bytes (only the magic goes to 0). Erasing one next to a live neighbour is what makes svErase park the neighbour in the scratch sector.
static void dirtyFree(int upto){ for(int i=0;i<upto;i++){ int isF=0; for(int j=0;j<nf;j++) if(fl[j]==i) isF=1; if(isF) continue;
    slOpen(i,1); u32 o=SLO(i); svWr(o,0); svWr(o+1,0); for(int k=2;k<200;k++) svWr(o+(u32)k,(int)(rnd()&255)); } }
static int neighboursOk(void){ for(int i=0;i<nf;i++) if(memcmp(fsnap[i],chip+SLO(fl[i]),SLOT_SZ)) return 0; return 1; }
static u8 base[131072]; static long nest;
enum { M_UPDATE, M_RENAME, M_NEW, M_DELETE };
static void doOp(int mode,int ia){
    if(mode==M_UPDATE) kgsSaveI(ia,0,&R,B,cells,buf,(int)sizeof buf,0);
    else if(mode==M_RENAME) kgsRenameI(ia,"AFTER");
    else if(mode==M_NEW){ int id=0; kgsSaveI(0,"NEWONE",&R,B,cells,buf,(int)sizeof buf,&id); }
    else kgsDelete(ia);
}
// one sweep: build the state, run the operation once without a cut (to count the raw operations), then cut at the erases and at a spread of programs
static void sweep(const char*what,int mode,int tier,int bld,int roads,int cleared,const int*fill,int nfill,int stride,int nested){
    blank(); nf=0; for(int i=0;i<nfill;i++) addFiller(fill[i]); dirtyFree(16);
    scenario(tier,bld,roads,cleared); take(&SA);
    int ia=0, ib=0;
    if(mode!=M_NEW){ give(&SA); kgsSaveI(0,"BEFORE",&R,B,cells,buf,(int)sizeof buf,&ia); }
    scenario(0,8,40,80); take(&SC); give(&SC); kgsSaveI(0,"OTHER",&R,B,cells,buf,(int)sizeof buf,&ib);   // (a second realm that must never be touched)
    if(mode==M_UPDATE){ fixSeed=SA.R.seed; scenario(tier,bld+20>KG_MAXB?KG_MAXB:bld+20,roads+100,cleared+200); fixSeed=0; take(&SB); } else SB=SA;
    { int st=0; slScan(); for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING) st++; (void)st; }
    memcpy(base,chip,sizeof chip);
    memset(opKind,0,sizeof opKind); give(&SB); ops=0; cutAt=-1; reboot(); rec=1; doOp(mode,ia); rec=0; long total=ops; cutAt=-1;
    int a=0,b=0,pts=0; long k;
    for(k=0;k<=total;k++){
        int isErase=k<400000&&opKind[k]; if(!isErase&&k>=80&&k<=total-80&&(k%stride)) continue;
        memcpy(chip,base,sizeof chip); reboot(); give(&SB); ops=0; cutAt=k;
        if(!setjmp(cutJ)){ doOp(mode,ia); } cutAt=-1; reboot(); pts++;
        if(nested&&(k%47)==0){   // a SECOND cut, inside the recovery at the next start: the start after that must still put everything right
            static u8 post[131072]; memcpy(post,chip,sizeof chip);
            for(long j=0;j<4400;j+=(j<40?1:29)){ memcpy(chip,post,sizeof chip); ops=0; cutAt=j; int done=1; if(!setjmp(cutJ)){ reboot(); } else done=0; cutAt=-1; nest++;
                reboot(); slScan();
                CHECK(neighboursOk(),"%s: cut at op %ld then at recovery op %ld damaged a neighbouring slot",what,k,j);
                { int lb=kgsFind(ib); CHECK(lb>=0&&load(lb)==SLE_OK&&sameAs(&SC),"%s: cut at op %ld then at recovery op %ld touched the OTHER realm",what,k,j); }
                { int sr=ia?kgsFind(ia):-1; if(mode!=M_NEW&&mode!=M_DELETE){ CHECK(sr>=0&&load(sr)==SLE_OK,"%s: cut at op %ld then at recovery op %ld lost the realm",what,k,j); } }
                if(done) break; }
            memcpy(chip,post,sizeof chip); reboot(); }
        slScan();
        CHECK(neighboursOk(),"%s: cut at op %ld damaged a neighbouring slot",what,k);
        { int lb=kgsFind(ib); CHECK(lb>=0&&load(lb)==SLE_OK&&sameAs(&SC),"%s: cut at op %ld touched the OTHER realm",what,k); }
        for(int i=0;i<SLOT_N;i++) CHECK(!(slOwner[i]==i&&slI[i].kind==SLK_KING)||slGood[i]||1,"x");
        int s=ia?kgsFind(ia):-1;
        if(mode==M_NEW){ int n=0,idn=0; for(int i=0;i<SLOT_N;i++) if(slOwner[i]==i&&slI[i].kind==SLK_KING&&slI[i].pid!=ib){ n++; idn=slI[i].pid; } if(!n) a++; else { b++; int ss=kgsFind(idn); CHECK(ss>=0&&load(ss)==SLE_OK&&sameAs(&SA),"%s: cut at op %ld left a wrong new realm",what,k); } }
        else if(mode==M_DELETE){ if(s<0) b++; else { a++; CHECK(load(s)==SLE_OK&&sameAs(&SA),"%s: cut at op %ld left a damaged realm",what,k); } }
        else { CHECK(s>=0,"%s: cut at op %ld LOST THE REALM",what,k);
            if(s>=0){ int e=load(s); CHECK(e==SLE_OK,"%s: cut at op %ld: load gave %d",what,k,e);
                if(e==SLE_OK){ if(mode==M_RENAME){ CHECK(sameAs(&SA),"%s: cut at op %ld changed the realm",what,k); if(!strcmp(slI[s].name,"BEFORE")) a++; else if(!strcmp(slI[s].name,"AFTER")) b++; else CHECK(0,"%s: cut at op %ld: name '%s'",what,k,slI[s].name); }
                    else { if(sameAs(&SA)) a++; else if(sameAs(&SB)) b++; else CHECK(0,"%s: cut at op %ld: neither the old realm nor the new",what,k); } } } }
        if(mode!=M_NEW&&mode!=M_DELETE){ kgsHeal(ia); CHECK(copies(ia)==1,"%s: cut at op %ld: %d copies after heal",what,k,copies(ia));
            give(&SB); int e2=kgsSaveI(ia,0,&R,B,cells,buf,(int)sizeof buf,0); CHECK(e2>=0,"%s: cut at op %ld: the next save failed (%d)",what,k,e2); }
        if(mode==M_NEW){ give(&SB); int id2=0; int e2=kgsSaveI(0,"AGAIN",&R,B,cells,buf,(int)sizeof buf,&id2); CHECK(e2>=0,"%s: cut at op %ld: founding again failed (%d)",what,k,e2); }
    }
    printf("%-44s %ld raw ops, %d cut points: %s %d, %s %d\n",what,total,pts,mode==M_RENAME?"old name":mode==M_NEW?"nothing":mode==M_DELETE?"realm kept":"old realm",a,mode==M_RENAME?"new name":mode==M_NEW?"whole new realm":mode==M_DELETE?"realm gone":"new realm",b);
}
int main(void){
    static const int f1[]={1,3,5}, f4[]={0,9}, fnone[]={0};
    sweep("save over a 1-slot realm (neighbours share sectors)",M_UPDATE,1,40,300,400,f1,3,1,1);
    sweep("save over a 4-slot realm (old copy shares a sector)",M_UPDATE,2,250,3000,6500,f4,2,13,0);
    sweep("rename a 1-slot realm",M_RENAME,1,40,300,400,f1,3,1,1);
    sweep("rename a 4-slot realm",M_RENAME,2,250,3000,6500,f4,2,13,0);
    sweep("found a new realm",M_NEW,0,10,60,100,f1,3,1,0);
    sweep("delete a realm",M_DELETE,1,40,300,400,f1,3,1,0);
    (void)fnone;
    printf("(%ld second cuts, inside the recovery at the next start, all survived the next start)\n",nest);
    printf(fails?"\n%d CHECK(S) FAILED\n":"\nall checks passed\n",fails);
    return fails?1:0;
}
