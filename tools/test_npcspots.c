#include <assert.h>
#include <stdio.h>
#include "../source/npcspots.h"
int main(void){
    assert(npcSpotSlot(1,999)==0);
    assert(npcSpotSlot(NPC_SPOT_CAP,999)==NPC_SPOT_CAP-1);
    assert(npcSpotSlot(NPC_SPOT_CAP+1,0)==0);  // replacement is possible
    assert(npcSpotSlot(NPC_SPOT_CAP+1,NPC_SPOT_CAP)==-1);
    unsigned short spots[NPC_SPOT_CAP]={0};
    for(unsigned n=1;n<=NPC_SPOT_CAP*4u;n++){
        int slot=npcSpotSlot(n,n>NPC_SPOT_CAP?0:n); // substituted by correct cap below
        if(slot>=0) spots[slot]=(unsigned short)n;
    }
    assert(spots[0]==NPC_SPOT_CAP*4u);
    for(unsigned i=1;i<NPC_SPOT_CAP;i++) assert(spots[i]==i+1);
    puts("BORE NPC targets: PASS");
    return 0;
}
