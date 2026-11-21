// World-anchored 3DS building facade regression, independent of SDK.
// Two cameras must see the same windows, arch and masonry HEIGHT.
#include <stdio.h>
#include <stdlib.h>
enum {B_NONE,B_HUT,B_HOUSE,B_HALL,B_FARM,B_TOWER,B_RUIN,
       B_DOCK,B_TEMPLE,B_STATUE,B_LIBRARY,B_BORDER,B_N};
#include "3ds/film_facade3d.h"
static void check(int ok,const char*why){
    if(!ok){fprintf(stderr,"3DS FACADE FAIL: %s\n",why);exit(1);}
}
int main(void){
    check(ggFilmFacadeHeightAt(72,116,21,2,8)==24&&
          ggFilmFacadeHeightAt(94,116,21,5,8)==24,
          "a physical wall detail drifts vertically as camera distance changes");
    check(ggFilmFacadeFeature(B_HOUSE,55,10)==GG_FACADE_GLASS&&
          ggFilmFacadeFeature(B_HOUSE,180,10)==GG_FACADE_GLASS&&
          ggFilmFacadeFeature(B_HOUSE,65,10)==GG_FACADE_FRAME,
          "timber windows do not have stable left/right glazing and frames");
    check(ggFilmFacadeFeature(B_HALL,55,22)==GG_FACADE_GLASS&&
          ggFilmFacadeFeature(B_HALL,55,10)!=GG_FACADE_GLASS,
          "stone halls lost their distinct taller window placements");
    int hallGlassWidth=0,houseGlassWidth=0,hallGlassHeight=0;
    for(int u=0;u<256;u++){
        if(ggFilmFacadeFeature(B_HALL,u,22)==GG_FACADE_GLASS)
            hallGlassWidth++;
        if(ggFilmFacadeFeature(B_HOUSE,u,10)==GG_FACADE_GLASS)
            houseGlassWidth++;
    }
    for(int v=0;v<=40;v++)
        if(ggFilmFacadeFeature(B_HALL,55,v)==GG_FACADE_GLASS)
            hallGlassHeight++;
    check(hallGlassWidth>15&&hallGlassWidth<=68&&
          houseGlassWidth>15&&houseGlassWidth<=68&&
          hallGlassHeight>=3&&hallGlassHeight<=7,
          "glowing window panel grew into an implausibly large billboard");
    check(ggFilmFacadeFeature(B_HOUSE,125,6)==GG_FACADE_DOOR&&
          ggFilmFacadeFeature(B_HOUSE,125,11)==GG_FACADE_FRAME,
          "central wood doorway and lintel not physically separated");
    check(ggFilmFacadeFeature(B_HALL,125,14)==GG_FACADE_PORTAL&&
          ggFilmFacadeFeature(B_HALL,125,17)==GG_FACADE_TRIM,
          "stone entry arch is not a recognizable curved portal");
    check(ggFilmFacadeFeature(B_HOUSE,125,20)==GG_FACADE_BEAM&&
          ggFilmFacadeFeature(B_HALL,100,24)==GG_FACADE_COURSE,
          "material-specific exposed beams or horizontal stone courses missing");
    check(ggFilmFacadeFeature(B_HOUSE,10,-3)==GG_FACADE_WALL&&
          ggFilmFacadeFeature(B_FARM,238,19)==GG_FACADE_WALL,
          "roof/ground or sparse farm material got phantom windows");
    check(ggFilmFacadeFeature(B_FARM,0,3)==GG_FACADE_BRACE&&
          ggFilmFacadeFeature(B_DOCK,2,3)==GG_FACADE_BRACE,
          "agricultural walls and timber docks are blank boxes");
    check(ggFilmFacadeFeature(B_RUIN,12,8)==GG_FACADE_COURSE&&
          ggFilmFacadeFeature(B_RUIN,125,6)==GG_FACADE_PORTAL,
          "collapsed settlements lack masonry or broken entrances");
    check(ggFilmFacadeFeature(B_STATUE,130,10)==GG_FACADE_SCULPT&&
          ggFilmFacadeFeature(B_STATUE,10,3)==GG_FACADE_CORNICE,
          "monuments still render as plain stone cubes");
    check(ggFilmFacadeFeature(B_HOUSE,56,6)==GG_FACADE_SILL&&
          ggFilmFacadeFeature(B_HALL,55,15)==GG_FACADE_SILL,
          "windows lack physically anchored stone/wood sills");
    puts("3DS FACADE OK: portals, framed windows, farmland, docks, ruins and sculpted monuments");
    return 0;
}
