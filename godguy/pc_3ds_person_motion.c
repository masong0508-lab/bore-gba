// Runtime-independent tests for citizen movement, gait and world yaw.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/person_motion3d.h"
static void check(int ok,const char*why){
    if(!ok){fprintf(stderr,"3DS PERSON MOTION FAIL: %s\n",why);exit(1);}
}
int main(void){
    ggPersonTrackReset();
    check(sizeof ggPersonTracks<1024,
          "actor motion state exceeds bounded Old 3DS RAM budget");
    const int directions[8][3]={
       {1,0,0},{1,1,32},{0,1,64},{-1,1,96},
       {-1,0,128},{-1,-1,160},{0,-1,192},{1,-1,224}
    };
    for(int i=0;i<8;i++)
        check(ggPersonHeading8(directions[i][0],directions[i][1],17)==
              directions[i][2],"incorrect real-world heading octant");
    GgPersonMotion a=ggPersonMotionSample(7,1000,1000,0,201);
    check(a.x==1000&&a.y==1000&&a.heading==201&&!a.walking,
          "initial visible person spawns mid-air or with changed identity");
    a=ggPersonMotionSample(7,1256,1000,10,201);
    check(a.x==1000&&a.heading==0&&a.walking,
          "person snapped one tile or ignored actual movement direction");
    GgPersonMotion middle=ggPersonMotionSample(7,1256,1000,15,201);
    check(middle.x>1000&&middle.x<1256&&middle.walking,
          "midstride interpolation does not occupy physical world space");
    GgPersonMotion b=ggPersonMotionSample(9,2300,1500,16,90);
    check(b.x==2300&&b.heading==90,
          "separate citizen inherits another person's motion");
    // Both residents must retain independent velocity histories during
    // alternating draws. A free track is NOT a reason to evict another
    // citizen already crossing a real tile.
    GgPersonMotion shared=ggPersonMotionSample(7,1256,1000,17,201);
    check(shared.walking&&shared.x>1000&&shared.x<1256,
          "second visible resident evicted the first resident's walking stride");
    GgPersonMotion second=ggPersonMotionSample(9,2300,1756,18,90);
    check(second.walking&&second.y==1500&&second.heading==64,
          "independent second citizen's movement snapped or changed identity");
    shared=ggPersonMotionSample(7,1256,1000,19,201);
    check(shared.walking&&shared.x>1000&&shared.x<1256,
          "interleaving multiple residents resets motion on every frame");
    GgPersonMotion end=ggPersonMotionSample(7,1256,1000,20,201);
    check(end.x==1256&&!end.walking,
          "idle citizen keeps skidding beyond arrival at the tile");
    GgPersonMotion next=ggPersonMotionSample(7,1256,744,30,201);
    check(next.heading==192&&next.y==1000&&next.walking,
          "north step doesn't face north or snaps back to tile");
    next=ggPersonMotionSample(7,1256,744,35,201);
    check(next.y<1000&&next.y>744,
          "diagonal/vertical tween does not move continuously");
    // Teleport/level-load snaps; no slide across blocking architecture.
    next=ggPersonMotionSample(7,8000,9000,36,201);
    check(next.x==8000&&next.y==9000&&!next.walking,
          "teleport dragged person through buildings");
    next=ggPersonMotionSample(7,8000,9000,1,201);
    check(next.heading==201&&!next.walking,
          "clock rewind failed to reset a stale game session");
    ggPersonTrackReset();
    for(unsigned int i=1;i<=GG_PERSON_TRACK_SLOTS;i++)
        ggPersonMotionSample(i,1000+i*8,1000,20,0);
    for(int i=0;i<GG_PERSON_TRACK_SLOTS;i++){
        check(ggPersonTracks[i].id!=0,
              "free slots left unused while resident identities were evicted");
        for(int j=0;j<i;j++)
            check(ggPersonTracks[i].id!=ggPersonTracks[j].id,
                  "duplicate citizen identities occupy tracking slots");
    }
    GgPersonMotion latest=ggPersonMotionSample(999,3000,4000,25,73);
    check(latest.x==3000&&latest.y==4000&&
          sizeof ggPersonTracks<1024,
          "visible population tracker overflowed after eviction");
    puts("3DS PERSON MOTION OK: independent multiple residents, fixed-slot identities, real heading, eased gait, arrival, teleport and capacity");
    return 0;
}
