// 3DS dialogue film: one grounded camera dolly per two-line take.
// All route validation happens at shot setup, never inside the renderer.
// Integer Q8 interpolation; no heap, second scene buffer or GPU assets.
#ifndef GG_FILM_CAMERA3D_H
#define GG_FILM_CAMERA3D_H
typedef struct {int fromX,fromY,toX,toY;} GgFilmCameraTrack;
typedef int (*GgFilmCameraCell)(int x,int y,int elevation,void *context);
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
static int ggFilmCameraCorridorClear(int sx,int sy,int ex,int ey,
                                     int elevation,GgFilmCameraCell pass,
                                     void *context){
    // Probe 16 Q8 points along the entire route only ONCE per shot.
    // This keeps the camera out of water, walls, roofs and cliff steps.
    for(int i=0;i<=16;i++){
        int x=sx+(ex-sx)*i/16,y=sy+(ey-sy)*i/16;
        if(!pass(x>>8,y>>8,elevation,context))return 0;
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
