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
            unsigned revived=msPressureResume(packed);
            assert(revived<=100u);
            if(msPressureBand(pressure)!=0)
                assert(revived+7u>=pressure&&revived<=pressure+13u);
        }
    }
    // Reloading hospital pressure before a good-news event must keep the
    // benefit. Regression: the old event handler eased PLUG_START=20 first,
    // then the first hospital tick overwrote it from the save's pressure band.
    for(unsigned old=0;old<=100;old++){
        unsigned saved=msPressurePack(0,old);
        unsigned restored=msPressureResume(saved);
        for(unsigned relief=10;relief<=15;relief+=5){
            unsigned eased=msPressureEase(restored,relief);
            assert(eased==(restored>relief?restored-relief:0u));
            assert(eased<=restored && eased<=100u);
            unsigned stored=msPressurePack(saved,eased);
            unsigned resumed=msPressureResume(stored);
            assert(resumed<=100u);
            assert(msNightUsed(stored)==msNightUsed(saved));
        }
    }
    // Boundary matrix: before 100 is safe; at 100, the first crisis grants
    // exactly one final night and any subsequent crisis is terminal.
    for(unsigned night=0;night<=1;night++) for(unsigned pressure=0;pressure<=101;pressure++){
        unsigned outcome=msPressureOutcome(pressure,night);
        assert(outcome==(pressure<100u?0u:(night?2u:1u)));
    }
    assert(msPressureOutcome(99,0)==0u);
    assert(msPressureOutcome(100,0)==1u);
    assert(msPressureOutcome(100,1)==2u);
    // The final night is persisted independently of the pressure band.
    for(unsigned mem=0;mem<256;mem++){
        unsigned used=msNightUsed(mem);
        unsigned crisis=msPressureOutcome(100u,used);
        assert(crisis==(used?2u:1u));
        if(!used){
            unsigned extended=msPressurePack(mem|MS_ONE_MORE_NIGHT,50u);
            assert(msNightUsed(extended));
            assert(msPressureOutcome(100u,msNightUsed(extended))==2u);
        }
    }
    assert(msPressureEase(0,15)==0u);
    assert(msPressureEase(20,15)==5u);
    assert(msPressureEase(90,10)==80u);
    // A user who lowers pressure to 0 must not be reset to 20 by reload.
    // New chapter-four saves explicitly start at PLUG_START (20) instead.
    for(unsigned p=0;p<=6;p++)
        assert(msPressureResume(msPressurePack(0,p))==0u);
    assert(msPressureResume(msPressurePack(0,20u))==14u);
    assert(msPressureResume(msPressurePack(0,0u))==0u);
    assert(msPressureBand(0)==0&&msPressureBand(100)==7);
    assert(msPressureResume(0)==0u);
    assert(msPressureResume(msPressurePack(16,88))>=84);
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
    assert(msDaysProgress(0,0)==0u&&msDaysProgress(0,1)==0u);
    assert(msDaysProgress(5,0)==5u);
    assert(msDaysProgress(5,1)==4u); // the earned fifth day is not lost
    assert(msDaysProgress(6,1)==5u); // only ONE additional day is required
    for(unsigned d=0;d<=20;d++){
        assert(msDaysProgress(d,0)==d);
        assert(msDaysProgress(d,1)==(d?d-1:0));
    }
    // Every possible save status byte round-trips exactly, including when
    // pending scenes and guest-state flags are both set during reload.
    for(unsigned status=0;status<256;status++){
        unsigned ch=msStatusChapter(status), tasks=msStatusTasks(status);
        unsigned pending=msStatusPending(status), guest=msStatusGuest(status);
        assert(msStatusPack(ch,tasks,pending,guest)==status);
    }
    for(unsigned ch=0;ch<8;ch++) for(unsigned tasks=0;tasks<8;tasks++)
        for(unsigned pending=0;pending<2;pending++) for(unsigned guest=0;guest<2;guest++){
            unsigned status=msStatusPack(ch,tasks,pending,guest);
            assert(msStatusChapter(status)==ch);
            assert(msStatusTasks(status)==tasks);
            assert(msStatusPending(status)==pending);
            assert(msStatusGuest(status)==guest);
        }
    // Scenario matrix: selection of another housemate must never replace
    // the patient saved at story start. Legacy 255 saves use the selection.
    for(unsigned saved=0;saved<256u;saved++)
        for(unsigned selected=0;selected<256u;selected++){
            unsigned expected=saved==255u?selected:saved;
            assert(msPatientUid(saved,selected)==expected);
            assert(msPatientUid(msPatientUid(saved,selected),selected)==expected);
        }
    assert(msPatientUid(0u,7u)==0u);
    assert(msPatientUid(12u,9u)==12u);
    assert(msPatientUid(255u,9u)==9u);
    assert(MS_PENDING_SCENE==64u&&MS_ONE_MORE_NIGHT==16u);
    puts("BORE Missy story memory: PASS");
    return 0;
}
