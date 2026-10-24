// test_kingmem.c - PC test of source/kingmem.h: the realm laid out inside a buffer the size of hhPool (30,720 bytes), however misaligned the buffer is.
// The buffer is a malloc of EXACTLY that size, so AddressSanitizer catches any byte written past it. Run from the repo root:   sh tools/test_kingmem.sh
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "../source/kingdom.h"
#include "../source/kingsim.h"
#include "../source/kinglog.h"
#include "../source/kingmem.h"
static int fails=0;
#define CHECK(c,...) do{ if(!(c)){ fails++; printf("FAIL line %d: ",__LINE__); printf(__VA_ARGS__); printf("\n"); } }while(0)
int main(void){
    printf("== kingmem: the realm inside a %d byte pool (hhPool)\n",KM_POOL);
    for(int tier=0;tier<3;tier++) for(int mis=0;mis<8;mis++){
        u8*raw=malloc((size_t)KM_POOL+(size_t)mis); u8*pool=raw+mis; int bytes=KM_POOL; if(mis) bytes=KM_POOL;   // (a pool that starts 0..7 bytes off a word boundary: it still has KM_POOL bytes)
        KmMem m; int r=kmLayout(pool,bytes,tier,&m); CHECK(r==0,"tier %d misalign %d: layout refused (%d)",tier,mis,r); if(r){ free(raw); continue; }
        CHECK(((uintptr_t)m.S%KM_AL)==0&&((uintptr_t)m.R%KM_AL)==0&&((uintptr_t)m.B%KM_AL)==0&&((uintptr_t)m.cells%KM_AL)==0,"a part is not word aligned");
        CHECK((u8*)m.S>=pool&&m.cells+kgCells(tier)<=pool+bytes,"a part lies outside the pool");
        CHECK((u8*)m.R>=(u8*)(m.S+1)&&(u8*)m.B>=(u8*)(m.R+1)&&m.cells>=(u8*)(m.B+KS_MAXB),"parts overlap");
        int d=kgDim(tier), c=d/2; m.R->tier=(u8)tier; m.R->genVer=KG_GENVER; m.R->seed=777u+(u32)tier; m.R->rng=99; kgGen(m.cells,m.R->seed,tier);
        CHECK(ksInit(m.S,m.R,m.B,m.cells)==KSE_OK,"ksInit"); KSim*S=m.S;
        int st=ksPlace(S,KB_STORE,c-1,c-1,1), hut=ksPlace(S,KB_HUT,c+2,c-1,0), hut2=ksPlace(S,KB_HUT,c-1,c+2,0);
        CHECK(st>=0&&hut>=0&&hut2>=0,"placing failed %d %d %d",st,hut,hut2);
        ksRoadSet(S,c+1,c-1,1); ksRoadSet(S,c,c+1,1); ksRoadSet(S,c-1,c+1,1); ksRoadSet(S,c+1,c,1); ksRoadSet(S,c+1,c+1,1); ksRoadSet(S,c+2,c+1,1);
        ksGift(S,G_WOOD,60);
        for(long k=0;k<KS_DAYT*20;k++) ksStep(S);
        CHECK(ksCheck(S)==0,"ksCheck %d",ksCheck(S)); CHECK(m.B[hut].lvl==1&&m.B[hut2].lvl==1,"the huts were not built (%d %d), delivered %u",m.B[hut].lvl,m.B[hut2].lvl,S->delivered);
        static u8 buf[20000]; ksPrepSave(S); int len=kgEncode(buf,sizeof buf,m.R,m.B,m.cells); CHECK(len>0,"kgEncode %d",len);
        if(mis==0) printf("  tier %d (%dx%d): %5d of %d bytes used, %d spare; huts built, delivered %u, encoded %d bytes\n",tier,d,d,m.used,KM_POOL,KM_POOL-m.used,S->delivered,len);
        free(raw); }
    { u8*raw=malloc(64); KmMem m; CHECK(kmLayout(raw,64,0,&m)==KM_NOFIT,"a too small pool was not refused"); CHECK(kmLayout(raw,KM_POOL,3,&m)==KM_NOFIT,"a bad tier was not refused"); CHECK(kmLayout(0,KM_POOL,0,&m)==KM_NOFIT,"a null pool was not refused"); free(raw); }
    for(int t=0;t<3;t++) CHECK(kmNeed(t)<=KM_POOL,"tier %d needs %d, more than the pool",t,kmNeed(t));
    printf("%s: %d failure(s)\n",fails?"FAILED":"ALL PASSED",fails); return fails?1:0;
}
