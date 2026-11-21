// GodGuy 3DS: tiny bounded world-space movement memory for visible
// procedural citizens. Turns/walk timing come from REAL creature tile
// movement, never a random pose seed or simulated-looking camera wobble.
// Resident identity survives hiding/re-entering camera; teleport/save
// transitions snap cleanly rather than dragging a model across the map.
#ifndef GG_PERSON_MOTION3D_H
#define GG_PERSON_MOTION3D_H
#include <stdint.h>
#include <string.h>
enum {GG_PERSON_TRACK_SLOTS=12,GG_PERSON_STEP_FRAMES=10};
typedef struct {
    uint32_t id;
    int fromX,fromY,toX,toY;
    int start,lastSeen;
    unsigned char heading;
} GgPersonTrack;
typedef struct {
    int x,y,heading,walking,step;
} GgPersonMotion;
static GgPersonTrack ggPersonTracks[GG_PERSON_TRACK_SLOTS];
static inline int ggPersonAbs(int a){return a<0?-a:a;}
static inline int ggPersonHeading8(int dx,int dy,int fallback){
    if(dx==0&&dy==0)return fallback&255;
    if(dx>0)return dy>0?32:dy<0?224:0;
    if(dx<0)return dy>0?96:dy<0?160:128;
    return dy>0?64:192;
}
static inline void ggPersonTrackReset(void){
    memset(ggPersonTracks,0,sizeof ggPersonTracks);
}
static inline int ggPersonTrackInterp(int a,int b,int passed){
    if(passed<=0)return a;
    if(passed>=GG_PERSON_STEP_FRAMES)return b;
    // Smoothstep, fixed-point and exact endpoints: a moving character
    // doesn't snap ahead one cell or spend half the tween at a wall.
    int t=passed*256/GG_PERSON_STEP_FRAMES;
    int eased=t*t*(768-2*t)/(256*256);
    return a+(b-a)*eased/256;
}
static GgPersonMotion ggPersonMotionSample(uint32_t identity,
                                           int targetX,int targetY,
                                           int frame,int seedHeading){
    GgPersonMotion sample={targetX,targetY,seedHeading&255,0,0};
    if(!identity)identity=1;
    int slot=-1,firstEmpty=-1,oldestOccupied=-1;
    for(int i=0;i<GG_PERSON_TRACK_SLOTS;i++){
        if(ggPersonTracks[i].id==identity){slot=i;break;}
        if(!ggPersonTracks[i].id){
            if(firstEmpty<0)firstEmpty=i;
            continue;
        }
        if(oldestOccupied<0||ggPersonTracks[i].lastSeen<
                             ggPersonTracks[oldestOccupied].lastSeen)
            oldestOccupied=i;
    }
    // Never evict a moving citizen while any unused tracking slot
    // remains. Previously oldest could point at an occupied slot even
    // though a free slot existed, so two visible creatures continually
    // evicted one another and the screen showed no walking animation.
    if(slot<0)slot=firstEmpty>=0?firstEmpty:
                    oldestOccupied>=0?oldestOccupied:0;
    GgPersonTrack*p=&ggPersonTracks[slot];
    if(p->id!=identity||frame<p->lastSeen||
       frame-p->lastSeen>120||
       ggPersonAbs(targetX-p->toX)>256||
       ggPersonAbs(targetY-p->toY)>256){
        p->id=identity;
        p->fromX=p->toX=targetX;
        p->fromY=p->toY=targetY;
        p->start=p->lastSeen=frame;
        p->heading=(unsigned char)(seedHeading&255);
        return sample;
    }
    if(targetX!=p->toX||targetY!=p->toY){
        int dx=targetX-p->toX,dy=targetY-p->toY;
        // Continuing a journey picks up at the last interpolated
        // location; fast simulation tick changes cannot jerk backwards.
        int elapsed=frame-p->start;
        p->fromX=ggPersonTrackInterp(p->fromX,p->toX,elapsed);
        p->fromY=ggPersonTrackInterp(p->fromY,p->toY,elapsed);
        p->toX=targetX;p->toY=targetY;
        p->heading=(unsigned char)ggPersonHeading8(dx,dy,p->heading);
        p->start=frame;
    }
    int elapsed=frame-p->start;
    if(elapsed<0)elapsed=GG_PERSON_STEP_FRAMES;
    sample.x=ggPersonTrackInterp(p->fromX,p->toX,elapsed);
    sample.y=ggPersonTrackInterp(p->fromY,p->toY,elapsed);
    sample.heading=p->heading;
    sample.walking=(p->fromX!=p->toX||p->fromY!=p->toY)&&
                    elapsed<GG_PERSON_STEP_FRAMES;
    sample.step=sample.walking?elapsed:0;
    p->lastSeen=frame;
    return sample;
}
#endif
