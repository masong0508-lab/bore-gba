// Perspective-correct film-only ground plane from GodGuy's real world[][].
// Intersects the same eye-height / focal projection as cast boots. Fills
// continuous grass/water/road surfaces instead of vertical Atari-like
// stair bands on level terrain. No second frame buffer, textures, floats,
// heap allocations or any changes to the fast exploration renderer.
#ifndef GG_FILM_FLOOR3D_H
#define GG_FILM_FLOOR3D_H
#include "film_projection3d.h"
static inline int ggFilmFloorForward(int y,int horizon,
                                      int cameraHeight,int maxForward){
    if(y<=horizon)return -1;
    int d=cameraHeight*GG_FILM_HEIGHT_MULT*256/(y-horizon)-256;
    if(d<0)d=0;
    if(d>maxForward)d=maxForward;
    return d;
}
static inline int ggFilmFloorMist(int forward){
    int mist=(forward-6*256)/80;
    if(mist<0)mist=0;
    if(mist>144)mist=144;
    return mist;
}
// Stable ground micropattern follows world coordinates as the camera
// moves. A small phase offset adds wind without swimming screen noise.
static inline int ggFilmFloorGrain(int wx,int wy,int frame){
    int pattern=((wx>>5)*7+(wy>>6)*11)&7;
    int v=pattern<2?3:pattern>5?-2:0;
    int wind=(frame/8+(wx>>7)+(wy>>6))&31;
    if(wind<7)v+=3-wind/3;
    return v;
}
#endif
