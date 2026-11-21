// World-anchored 3DS building facade regression, independent of SDK.
// Two cameras must see the same windows, arch and masonry HEIGHT.
#include <stdio.h>
#include <stdlib.h>
enum {B_HUT=1,B_HOUSE,B_HALL,B_TOWER,B_TEMPLE,B_LIBRARY,B_FARM,B_DOCK};
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
    puts("3DS FACADE OK: world-height windows, portico, masonry and timber volume");
    return 0;
}
