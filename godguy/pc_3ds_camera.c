// SDK-free regression for the actual 3DS live-world cinematic dolly math.
// gcc -std=c11 -O2 -Wall -Wextra -Werror pc_3ds_camera.c -o /tmp/gg-camera && /tmp/gg-camera
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_camera3d.h"
static unsigned char blocked[16][16],height[16][16];
static int checked;
static void check(int ok,const char *why){
    if(!ok){fprintf(stderr,"3DS CAMERA FAIL: %s\n",why);exit(1);}
}
static int filmCell(int x,int y,int elevation,void *context){
    (void)context;
    checked++;
    return x>=1&&x<15&&y>=1&&y<15&&!blocked[y][x]&&
           height[y][x]==elevation;
}
int main(void){
    const int sx=10*256+128,sy=10*256+128;
    GgFilmCameraTrack track;
    for(int y=0;y<16;y++)
        for(int x=0;x<16;x++)height[y][x]=8;
    ggFilmCameraPlan(&track,sx,sy,4096,0,8,filmCell,0);
    check(track.fromX==sx&&track.fromY==sy,"track lost its start");
    check(track.toX==sx+192,"open path did not move the camera through real terrain");
    check(track.toY==sy+48,"motivated dolly lost lateral motion");
    int a,b,c,d,px=sx,py=sy;
    for(int t=0;t<=192;t++){
        int beat=t/96,local=t%96;
        if(t==192){beat=1;local=96;}
        ggFilmCameraTrackAt(&track,beat,local,96,&a,&b);
        check(a>=px&&a>=sx&&a<=track.toX,"camera jumped backwards");
        check(b>=py&&b>=sy&&b<=track.toY,"camera drifted outside shot");
        px=a;py=b;
    }
    check(px==track.toX&&py==track.toY,"camera never reached final position");
    ggFilmCameraTrackAt(&track,0,96,96,&a,&b);
    ggFilmCameraTrackAt(&track,1,0,96,&c,&d);
    check(a==c&&b==d,"camera snapped between paired dialogue beats");
    ggFilmCameraTrackAt(&track,0,-999,96,&a,&b);
    check(a==sx&&b==sy,"negative elapsed time escaped start bound");
    ggFilmCameraTrackAt(&track,1,999,96,&a,&b);
    check(a==track.toX&&b==track.toY,"elapsed time escaped end bound");
    // A blocked route must shrink to a same-tile movement.
    blocked[10][11]=1;
    ggFilmCameraPlan(&track,sx,sy,4096,0,8,filmCell,0);
    check(track.toX>=sx&&track.toX<11*256,
          "camera moved through an occupied next tile");
    check(ggFilmCameraCorridorClear(sx,sy,track.toX,track.toY,
                                    8,filmCell,0),"selected corridor unsafe");
    blocked[10][11]=0;
    // Cliff height changes must not make the viewpoint pop up or down.
    height[10][11]=24;
    ggFilmCameraPlan(&track,sx,sy,4096,0,8,filmCell,0);
    check(track.toX<11*256,"camera drove onto a different-height cliff");
    height[10][11]=8;
    // A camera center can remain in a legal cell while its near edge
    // intersects an adjacent building or cliff. Never allow that.
    blocked[10][11]=1;
    check(!ggFilmCameraFootprintClear(11*256-8,10*256+128,8,filmCell,0),
          "camera clipped a nearby building through its right side");
    check(ggFilmCameraFootprintClear(11*256-32,10*256+128,8,filmCell,0),
          "camera was rejected despite safe neighboring clearance");
    blocked[10][11]=0;
    // At a diagonal corner BOTH neighboring edges and the corner
    // itself must be clear, even when the camera center is traversable.
    blocked[11][11]=1;
    check(!ggFilmCameraFootprintClear(11*256-8,11*256-8,8,filmCell,0),
          "diagonal house corner intruded into cinematic viewpoint");
    blocked[11][11]=0;
    height[11][11]=24;
    check(!ggFilmCameraFootprintClear(11*256-8,11*256-8,8,filmCell,0),
          "diagonal cliff corner intruded into cinematic viewpoint");
    height[11][11]=8;
    // No safe staging position: freeze rather than clip into buildings.
    blocked[10][10]=1;
    ggFilmCameraPlan(&track,sx,sy,4096,0,8,filmCell,0);
    check(track.toX==sx&&track.toY==sy,"blocked camera did not lock");
    blocked[10][10]=0;
    // Quarter-turn tracks stay inside the same checked 3D grid.
    ggFilmCameraPlan(&track,sx,sy,0,4096,8,filmCell,0);
    check(track.toY==sy+192&&track.toX==sx-48,
          "camera heading was not rotated in world coordinates");
    // Real-world entry blocking must also obey the 400x240 film
    // boundary. At two tiles the stage mark fits, but an entrance from
    // the adjacent near tile would put the actor under subtitles.
    GgFilmCameraSubject stage={sx+2*256,sy,8};
    GgFilmCameraSubject entrance={sx+256,sy,8};
    check(ggFilmCameraSubjectFits(sx,sy,4096,0,8,stage,3)&&
          !ggFilmCameraSubjectFits(sx,sy,4096,0,8,entrance,3),
          "close-up approach was allowed to enter from under captions");
    GgFilmCameraSubject mediumEntrance={sx+2*256,sy,8};
    check(ggFilmCameraSubjectFits(sx,sy,4096,0,8,mediumEntrance,2),
          "safe medium-shot entrance got disabled unnecessarily");
    // A close 3x camera that starts with a readable pair used to end
    // with the hero's boots under subtitles and the guest off-screen.
    // Keeping the entire pair in view takes priority over the dolly.
    const GgFilmCameraSubject pair[2]={
        {sx+512,sy,8},{sx+512,sy+256,8}
    };
    check(ggFilmCameraSubjectFits(sx,sy,4096,0,8,pair[0],3)&&
          ggFilmCameraSubjectFits(sx,sy,4096,0,8,pair[1],3),
          "safe paired cast was rejected before the camera moved");
    check(!ggFilmCameraSubjectFits(sx+192,sy,4096,0,8,pair[0],3)&&
          !ggFilmCameraSubjectFits(sx+192,sy,4096,0,8,pair[1],3),
          "cropped hero and guest at the dolly endpoint went unnoticed");
    GgFilmCameraTrack close={sx,sy,sx+192,sy};
    ggFilmCameraKeepCastInFrame(&close,pair,2,4096,0,8,3);
    check(close.toX==sx+96&&close.toY==sy,
          "camera failed to shorten a dolly for the visible pair");
    check(ggFilmCameraSubjectFits(close.toX,close.toY,
                                  4096,0,8,pair[0],3)&&
          ggFilmCameraSubjectFits(close.toX,close.toY,
                                  4096,0,8,pair[1],3),
          "clamped camera still cut a speaking partner off");
    close.toX=sx+192;
    ggFilmCameraKeepCastInFrame(&close,pair,1,4096,0,8,3);
    check(close.toX==sx+144,
          "solo film camera lost safe motion or clipped close-up boots");
        check(checked>50&&checked<800,"path validation cost is unbounded");
    puts("3DS CAMERA OK: continuous dolly, footprint-safe corners, cliff/obstacle fallback");
    return 0;
}
