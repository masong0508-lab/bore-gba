// Exercise the real cartridge save adapter without touching DS registers.
// A rejected write must make close return before it can call hwFlash.
#include "compat/ndshw.h"
static int fails;
#define CHECK(c,m) do{if(!(c)){fprintf(stderr,"FAIL: %s\n",m);fails++;}}while(0)
int main(void){
    const u8 value=1;
    u8 block[512];memset(block,7,sizeof block);
    svMode=1;svLen=0;svFailed=0;memset(&ggSaveWriter,0,sizeof ggSaveWriter);
    while(svLen<SVMAX-1){u32 n=SVMAX-1-svLen;if(n>sizeof block)n=sizeof block;
        CHECK(svWrite(block,1,n,&svDummy)==n,"prepare a valid nearly full archive");
        if(svFailed)return 1;
    }
    CHECK(svWrite(&value,1,2,&svDummy)==0,"raw-size overflow is refused");
    CHECK(svFailed&&svLen==SVMAX-1,"write failure is latched without changing length");
    CHECK(svWrite(&value,1,1,&svDummy)==0,"later writes cannot mask a failed archive");
    CHECK(svClose(&svDummy)==-1,"failed save never attempts a cartridge commit");
    svMode=1;svLen=0;svFailed=0;memset(&ggSaveWriter,0,sizeof ggSaveWriter);
    CHECK(svWrite(block,1,sizeof block,&svDummy)==sizeof block,"prepare a nonempty second archive");
    CHECK(svWrite(&value,(size_t)SVMAX+1,1,&svDummy)==0,"oversized record is refused");
    CHECK(svClose(&svDummy)==-1,"oversized record cannot commit a partial save");
    // Read-only validation can replay the existing packed buffer. No
    // additional cartridge read or raw-world allocation is needed.
    CHECK(ggSvPack(&ggSaveWriter,SVCMP+12,SVCMAX),"flush rewind fixture");
    svMode=0;svPos=0;ggSvReaderStart(&ggSaveReader,SVCMP+12,ggSaveWriter.packed,svLen);
    u8 first[31],second[31];
    CHECK(svRead(first,1,sizeof first,&svDummy)==sizeof first,"read archive prefix");
    CHECK(svRewind(&svDummy)&&svPos==0,"rewind archive in memory");
    CHECK(svRead(second,1,sizeof second,&svDummy)==sizeof second&&!memcmp(first,second,sizeof first),"replayed stream is identical");
    printf(fails?"HW SAVE GUARD FAILED (%d)\n":"HW SAVE GUARD OK (%d failures)\n",fails);
    return fails?1:0;
}
