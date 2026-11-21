// GodGuy 3DS: real world-space pitched roof sections for live buildings.
// A gable has ONE ridge axis and sloped front/back faces, rather than the
// old axis-independent pyramid stuck on top of every voxel column.
// All Q8 integer math, evaluated per visible ray sample; no new models,
// textures, heaps, render targets, or extra terrain iteration.
#ifndef GG_FILM_ARCHITECTURE3D_H
#define GG_FILM_ARCHITECTURE3D_H
typedef struct {
    int roofTop;       // world-height at this specific ray intersection
    int wallTop;       // invariant wall/eave height of the building tile
    int roofPitch;     // height of ridge above eaves
    int faceLight;     // brighter roof slope facing the world light
} GgFilmRoofVolume;
static inline GgFilmRoofVolume ggFilmRoofVolume(
        int build,int worldX,int worldY,int ground,int rise){
    GgFilmRoofVolume v={ground+rise,ground+rise,0,256};
    int pitch=0;
    switch(build){
    case B_HUT:pitch=5;break;
    case B_HOUSE:pitch=7;break;
    case B_HALL:pitch=10;break;
    case B_TEMPLE:pitch=7;break;
    case B_LIBRARY:pitch=7;break;
    default:return v;
    }
    if(rise<=pitch)return v;
    // Deterministic orientation per building tile. Both actors and scene
    // camera observe the SAME roof from any angle and any film beat.
    int ridgeAlongX=(((worldX>>8)+(worldY>>8)+build)&1)!=0;
    int across=(ridgeAlongX?worldY:worldX)&255;
    int deviation=across-128;if(deviation<0)deviation=-deviation;
    v.wallTop=ground+rise-pitch;
    v.roofPitch=pitch;
    v.roofTop=v.wallTop+pitch*(128-deviation)/128;
    // One diagonal slope reflects sunlight, the other is cooler.
    v.faceLight=(across<128?290:208);
    return v;
}
static inline int ggFilmRoofBottomScreen(int wallTop,int eye,
                                          int horizon,int sample,int mult){
    if(sample<1)sample=1;
    return horizon-(wallTop-eye)*mult/(sample+1);
}
static inline void ggFilmRoofMaterial(int build,int worldX,int worldY,
                                      int light,int shade,
                                      int *red,int *green,int *blue){
    int r=116,g=78,b=69;
    if(build==B_HALL){r=83;g=91;b=116;}
    else if(build==B_TEMPLE){r=172;g=150;b=114;}
    else if(build==B_LIBRARY){r=67;g=100;b=112;}
    else if(build==B_HUT){r=135;g=82;b=61;}
    // Tile courses run across the physical roof, never along the screen.
    int course=(((worldX&255)/24)+((worldY&255)/40))&3;
    if(course==0){r+=11;g+=8;b+=5;}
    else if(course==3){r-=12;g-=9;b-=7;}
    int lit=light*shade/256;
    *red=r*lit/256;*green=g*lit/256;*blue=b*lit/256;
}
#endif
