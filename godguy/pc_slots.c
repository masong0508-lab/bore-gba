// Save layer on a fake cartridge chip (step 9b). The packer lines are taken straight out of compat/ndshw.h at compile time, so the test runs the real code.
//   sh -c 'grep -E "^static u32 rle(Enc|Dec)" compat/ndshw.h > /tmp/gg_rle.inc' && gcc -O2 -w -DRLE_INC='"/tmp/gg_rle.inc"' -o /tmp/gg_slots pc_slots.c && /tmp/gg_slots
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef unsigned char u8; typedef unsigned int u32;
#include RLE_INC
static int fails=0; static void chk(int c,const char*m){ printf("%s %s\n",c?"ok  ":"FAIL",m); if(!c) fails++; }
static u8 chip[1<<20]; static u32 chipSize;
// the same rule as svClose: a pack (plus its 12 byte header) that is larger than the chip is refused, otherwise it is written at offset 0
static int store(const u8*raw,u32 n,u8*scratch,u32 smax){ u32 clen=rleEnc(raw,n,scratch+12,smax); if(!clen) return -1; u32 tot=clen+12; if(tot>chipSize) return -1; memcpy(chip,scratch,12); memcpy(chip+12,scratch+12,clen); *(u32*)(chip+4)=clen; *(u32*)(chip+8)=n; return 0; }
static int load(u8*out,u32 max,u32*n){ u32 clen=*(u32*)(chip+4), raw=*(u32*)(chip+8); u32 g=rleDec(chip+12,clen,out,max); *n=g; return g==raw; }
int main(void){
    static u8 raw[3<<20], back[3<<20], sc[2<<20]; u32 n=1800000; srand(5);
    for(u32 i=0;i<n;i++) raw[i]=(i/997)%7==0?(u8)rand():(u8)((i>>9)&3);          // runs and noise, like a world and its creatures
    chipSize=1<<19; chk(store(raw,n,sc,sizeof sc-16)==0,"a world that packs under the chip size is stored"); u32 g; chk(load(back,sizeof back,&g)&&g==n&&!memcmp(raw,back,n),"and reads back identical");
    chipSize=1<<10; memset(chip,0xAA,16); chk(store(raw,n,sc,sizeof sc-16)==-1,"a pack bigger than a 1 KB chip is refused"); chk(chip[0]==0xAA,"and the chip is left untouched");
    static u8 noise[600000]; for(u32 i=0;i<sizeof noise;i++) noise[i]=(u8)rand(); chipSize=1<<20; chk(store(noise,sizeof noise,sc,sizeof sc-16)==0&&load(back,sizeof back,&g)&&!memcmp(noise,back,sizeof noise),"incompressible data still round-trips");
    printf(fails?"SLOTS FAIL\n":"SLOTS OK\n"); return fails?1:0; }
