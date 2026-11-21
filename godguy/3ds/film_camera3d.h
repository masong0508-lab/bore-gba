// 3DS dialogue film: one grounded camera dolly per two-line take.
// All route validation happens at shot setup, never inside the renderer.
// Integer Q8 interpolation; no heap, second scene buffer or GPU assets.
#ifndef GG_FILM_CAMERA3D_H
#define GG_FILM_CAMERA3D_H
#include "film_projection3d.h"
typedef struct {int fromX,fromY,toX,toY;} GgFilmCameraTrack;
typedef int (*GgFilmCameraCell)(int x,int y,int elevation,void *context);
// Three is the upper bound: hero, one guest and optional second guest.
// Only short-lived stack state during one-shot planning.
typedef struct {int x,y,ground;} GgFilmCameraSubject;
static int ggFilmCameraSubjectFits(int cameraX,int cameraY,
                                    int cosA,int sinA,int cameraGround,
                                    GgFilmCameraSubject subject,int scale){
    int dx=subject.x-cameraX,dy=subject.y-cameraY;
    int forward=(dx*cosA+dy*sinA)/4096;
    int side=(-dx*sinA+dy*cosA)/4096;
    if(forward<=80||forward>16*256||
       (side<0?-side:side)*4>forward*5)return 0;
    int screenX=ggFilmProjectScreenX(side,forward);
    int feet=ggFilmProjectFootY(forward,cameraGround,subject.ground);
    return ggFilmFootSafe(screenX,feet,scale);
}
// The original director scores only the opening frame. A 3/4-tile dolly
// can bring boots into the subtitles or move a guest beyond the screen
// edge halfway through a two-line take. Shorten the already collision-
// checked track to the farthest still-readable frame. No render-time
// visibility searches and no additional framebuffer are required.
static void ggFilmCameraKeepCastInFrame(GgFilmCameraTrack *track,
                                        const GgFilmCameraSubject *cast,
                                        int count,int cosA,int sinA,
                                        int cameraGround,int scale){
    if(count<1)return;
    if(count>3)count=3;
    int fullX=track->toX,fullY=track->toY;
    for(int fraction=4;fraction>=0;fraction--){
        int x=track->fromX+
              (fullX-track->fromX)*fraction/4;
        int y=track->fromY+
              (fullY-track->fromY)*fraction/4;
        int allFit=1;
        for(int i=0;i<count;i++)
            if(!ggFilmCameraSubjectFits(x,y,cosA,sinA,
                                       cameraGround,cast[i],scale)){
                allFit=0;break;
            }
        if(allFit){
            track->toX=x;track->toY=y;
            return;
        }
    }
    // No safe optical endpoint: don't compound an already crowded shot.
    track->toX=track->fromX;
    track->toY=track->fromY;
}
static int ggFilmCameraEase(int t){
    if(t<0)t=0;
    if(t>256)t=256;
    return (t*t*(768-2*t))/(256*256);
}
static void ggFilmCameraTrackAt(const GgFilmCameraTrack *track,
                                int beat,int local,int beatTicks,
                                int *x,int *y){
    if(beatTicks<=0){*x=track->toX;*y=track->toY;return;}
    // Ease across the WHOLE two-line shot: no jolt at the beat switch.
    int elapsed=(beat&1)?beatTicks:0;
    if(local<0)local=0;
    if(local>beatTicks)local=beatTicks;
    elapsed+=local;
    int t=ggFilmCameraEase(elapsed*256/(beatTicks*2));
    *x=track->fromX+(track->toX-track->fromX)*t/256;
    *y=track->fromY+(track->toY-track->fromY)*t/256;
}
// A ray through the center of the camera is not a safe corridor: the
// previous shot planner could graze the corner of a house, wall or cliff.
// Test a 16/256-tile collision footprint, including diagonal corners.
// This is evaluated ONLY during shot selection, never during rendering.
// Local x/y are nonnegative Q8 map coordinates (world map indices).
static int ggFilmCameraFootprintClear(int x,int y,int elevation,
                                      GgFilmCameraCell pass,void *context){
    const int radius=16;
    int tx=x>>8,ty=y>>8;
    int fx=x&255,fy=y&255;
    int nx=fx<radius?tx-1:fx>=256-radius?tx+1:tx;
    int ny=fy<radius?ty-1:fy>=256-radius?ty+1:ty;
    if(!pass(tx,ty,elevation,context))return 0;
    if(nx!=tx&&!pass(nx,ty,elevation,context))return 0;
    if(ny!=ty&&!pass(tx,ny,elevation,context))return 0;
    if(nx!=tx&&ny!=ty&&!pass(nx,ny,elevation,context))return 0;
    return 1;
}
static int ggFilmCameraCorridorClear(int sx,int sy,int ex,int ey,
                                     int elevation,GgFilmCameraCell pass,
                                     void *context){
    // Quarter-of-a-tile probes and 16-Q8 footprint prevent thin corner
    // intrusions while keeping shot search bounded and allocation-free.
    // The longest proposed dolly is < one tile.
    for(int i=0;i<=16;i++){
        int x=sx+(ex-sx)*i/16,y=sy+(ey-sy)*i/16;
        if(!ggFilmCameraFootprintClear(x,y,elevation,pass,context))
            return 0;
    }
    return 1;
}
static void ggFilmCameraPlan(GgFilmCameraTrack *track,
                              int startX,int startY,int cosA,int sinA,
                              int elevation,GgFilmCameraCell pass,
                              void *context){
    track->fromX=track->toX=startX;
    track->fromY=track->toY=startY;
    // Up to 3/4 of a map tile of motivated dolly movement.
    // Smaller takes avoid obstacle clipping in cluttered settlements.
    for(int forward=192;forward>=64;forward-=64){
        const int sides[3]={48,0,-48};
        for(int i=0;i<3;i++){
            int side=sides[i];
            int ex=startX+(cosA*forward-sinA*side)/4096;
            int ey=startY+(sinA*forward+cosA*side)/4096;
            if(ggFilmCameraCorridorClear(startX,startY,ex,ey,
                                         elevation,pass,context)){
                track->toX=ex;track->toY=ey;
                return;
            }
        }
    }
    // No safe lane: lock camera where originally staged.
}
#endif
