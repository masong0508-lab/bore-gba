// test_kingdone.c - EP6's own done-bits block (source/kingdone.h) on a fake SRAM. Run: sh tools/test_kingdone.sh
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
static u8 fake[9000]; static u8*const SRAM_BASE_P=fake;
#define SRAM_BASE SRAM_BASE_P
#include "../source/kingdone.h"
static int fails;
#define CHK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } }while(0)
int main(void){
    memset(fake,0x00,sizeof fake); for(int b=0;b<16;b++) CHK(!kdGet(b),"blank block reads nothing done");
    memset(fake,0xFF,sizeof fake); for(int b=0;b<16;b++) CHK(!kdGet(b),"erased block reads nothing done");
    memset(fake,0,sizeof fake);
    for(int b=0;b<16;b++){ kdSet(b); for(int c=0;c<16;c++) CHK(kdGet(c)==(c<=b),"bits set one by one keep the others"); }
    CHK(kdGet(-1)==0&&kdGet(16)==0,"out of range reads 0"); kdSet(16); kdSet(-1); for(int c=0;c<16;c++) CHK(kdGet(c),"out of range sets change nothing");
    u8 good[3]; memcpy(good,fake+KD_OFF,3);
    for(int i=0;i<3;i++) for(int bit=0;bit<8;bit++){ memcpy(fake+KD_OFF,good,3); fake[KD_OFF+i]^=(u8)(1<<bit);
        int any=0; for(int c=0;c<16;c++) any|=kdGet(c); CHK(!any,"every single bit of damage is refused"); }
    for(int i=0;i<3;i++) for(int v=0;v<256;v++){ memcpy(fake+KD_OFF,good,3); if(v==good[i]) continue; fake[KD_OFF+i]=(u8)v;
        int any=0; for(int c=0;c<16;c++) any|=kdGet(c); CHK(!any,"every damaged byte value is refused"); }
    memcpy(fake+KD_OFF,good,3); fake[KD_OFF+1]=0x12; kdSet(3); CHK(kdGet(3)&&!kdGet(0),"a damaged block starts again from nothing");
    memset(fake,0,sizeof fake); kdSet(0); kdSet(9); CHK(kdGet(0)&&kdGet(9)&&!kdGet(1)&&!kdGet(8)&&!kdGet(10),"low and high byte bits");
    // nothing outside the block is touched
    memset(fake,0xA5,sizeof fake); memset(fake+KD_OFF,0,3); kdSet(5); kdSet(12);
    for(int i=0;i<(int)sizeof fake;i++) if(i<KD_OFF||i>=KD_OFF+KD_LEN) CHK(fake[i]==0xA5,"no byte outside the block changes");
    if(fails){ printf("%d FAILED\n",fails); return 1; } printf("test_kingdone: all checks passed\n"); return 0;
}
