// DS save/compression streaming test. This must pass before linking the
// large banked 56-phrase vocal build: high memory is no longer scratch.
// gcc -O2 -std=c11 -Wall -Wextra -Werror pc_save_stream.c -o pc_save_stream
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint8_t u8; typedef uint32_t u32;
#include "compat/ggsave_stream.h"
enum { RAWMAX=700*1024, CMPMAX=0x80000-16 };
static u8 raw[RAWMAX],packed[CMPMAX+64],decoded[RAWMAX];
static void fail(const char*s){fprintf(stderr,"SAVE STREAM FAIL: %s\n",s);exit(1);}
static void run(int mode,unsigned seed){
    for(int i=0;i<RAWMAX;i++){
        seed=seed*1664525u+1013904223u;
        raw[i]=mode==0?(u8)(i/291):
                mode==1?(u8)((i/7)%17):
                mode==2?(u8)(seed>>24):
                (u8)((i/61+(i/960)%3)&255);
    }
    GgSaveWriter w={0};
    memset(packed,0xA5,sizeof packed);
    int len=mode==2?CMPMAX-5000:RAWMAX;
    for(int p=0;p<len;){
        int step=1+((unsigned)p*31u+seed)%1531u;
        if(step>len-p)step=len-p;
        if(!ggSvWrite(&w,packed+12,CMPMAX,raw+p,step,RAWMAX)){
            if(mode==2){
                if(packed[CMPMAX+12]!=0xA5)fail("buffer overrun when save full");
                return; // uncompressible 512-KiB save must fail safely
            }
            fail("compressed archive unexpectedly rejected");
        }
        p+=step;
    }
    if(!ggSvPack(&w,packed+12,CMPMAX))fail("final flush");
    if(w.raw!=(u32)len||!ggSvValidate(packed+12,w.packed,RAWMAX,len))
        fail("validation rejected a valid save");
    GgSaveReader d;
    ggSvReaderStart(&d,packed+12,w.packed,len);
    memset(decoded,0,sizeof decoded);
    for(int p=0;p<len;){
        int take=1+((unsigned)p*23u+seed)%547u;
        if(take>len-p)take=len-p;
        if(!ggSvRead(&d,decoded+p,take))fail("incremental save reader");
        p+=take;
    }
    if(memcmp(raw,decoded,len)||d.op!=(u32)len||d.left)
        fail("roundtrip mismatch");
    if(ggSvRead(&d,decoded,1))fail("reader accepted data past EOF");
    if(packed[CMPMAX+12]!=0xA5)fail("guard byte overwritten");
    // Legacy-format token stream is also RLE: the decoder must accept
    // existing saves, not just images produced by the new compressor.
    printf("SAVE STREAM %d: %d raw -> %u packed; identical round-trip\n",
           mode,len,w.packed);
}
int main(void){
    run(0,123);
    run(1,456);
    run(2,789);
    run(3,314159);
    {
        u8 old[]={131,7,3,1,2,3,4,255,9};
        GgSaveReader d;u8 tmp[133];
        // Run=5 sevens, literal=4, run=129 nines -> 138 bytes.
        ggSvReaderStart(&d,old,sizeof old,138);
        u8 buf[138];
        if(!ggSvValidate(old,sizeof old,138,138)||
           !ggSvRead(&d,buf,138)||buf[0]!=7||buf[5]!=1||buf[9]!=9)
           fail("historic RLE decode");
        (void)tmp;
    }
    puts("DS SAVE STREAM OK");
    return 0;
}
