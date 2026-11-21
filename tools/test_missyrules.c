// Portable tests for TV SHOW & TELL's one-byte save metadata.
#include <assert.h>
#include <stdio.h>
#include "../source/missyrules.h"
int main(void){
    for(unsigned mem=0;mem<256;mem++){
        assert(msNightUsed(mem)==((mem&MS_ONE_MORE_NIGHT)!=0));
        for(unsigned pressure=0;pressure<=100;pressure++){
            unsigned packed=msPressurePack(mem,pressure);
            assert((packed&31u)==(mem&31u)); // slips and the last-night decision survive
            assert((packed>>5)==msPressureBand(pressure));
            unsigned revived=msPressureResume(packed,20u);
            assert(revived<=100u);
            if(msPressureBand(pressure)!=0)
                assert(revived+7u>=pressure&&revived<=pressure+13u);
        }
    }
    assert(msPressureBand(0)==0&&msPressureBand(100)==7);
    assert(msPressureResume(0,20)==20);
    assert(msPressureResume(msPressurePack(16,88),20)>=84);
    assert(msNightUsed(msPressurePack(16,88)));
    assert(msPressureStep(0,3)==0u);   // must never wrap a u8 to 255
    assert(msPressureStep(20,0)==22u);
    assert(msPressureStep(20,1)==21u);
    assert(msPressureStep(20,2)==20u);
    assert(msPressureStep(20,3)==19u);
    assert(msPressureStep(99,0)==100u);
    assert(msPressureStep(100,0)==100u);
    assert(msPressureStep(0,0)==2u);
    for(unsigned pressure=0;pressure<=100;pressure++)
        for(unsigned supports=0;supports<=5;supports++){
            unsigned next=msPressureStep(pressure,supports);
            assert(next<=100u);
            if(supports>=3) assert(next<=pressure);
        }
    assert(MS_PENDING_SCENE==64u&&MS_ONE_MORE_NIGHT==16u);
    puts("BORE Missy story memory: PASS");
    return 0;
}
