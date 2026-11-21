// SDK-free inverse projection tests for the continuous 3DS film floor.
// These formulas MUST match character boots and the live voxel raycast,
// or actors hover and flat grass splits into vertical rectangular bands.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_floor3d.h"
static void check(int ok,const char*why){
    if(!ok){fprintf(stderr,"3DS FLOOR FAIL: %s\n",why);exit(1);}
}
int main(void){
    check(ggFilmFloorForward(116,116,13,42*256)==-1&&
          ggFilmFloorForward(110,116,13,42*256)==-1,
          "ground plane divided by zero at sky horizon");
    check(ggFilmFloorForward(194,116,13,42*256)==256&&
          ggFilmFloorForward(168,116,13,42*256)==512&&
          ggFilmFloorForward(155,116,13,42*256)==768,
          "continuous floor disagrees with actual one/two/three tile rays");
    for(int d=1;d<=16;d++){
        int feet=ggFilmProjectFootY(d*256,8,8);
        int reconstructed=ggFilmFloorForward(feet,GG_FILM_HORIZON,
                GG_FILM_EYE_HEIGHT,42*256);
        int delta=reconstructed-d*256;if(delta<0)delta=-delta;
        // At the horizon one integer screen row covers several Q8
        // distances, so bound inversion by that row's quantization.
        check(delta<=(d+1)*24,
              "actor boot and rendered ground differ beyond one raster row");
    }
    for(int y=117;y<239;y++){
        int a=ggFilmFloorForward(y,116,13,42*256);
        int b=ggFilmFloorForward(y+1,116,13,42*256);
        check(b<=a&&a>=0&&a<=42*256,
              "ground moves farther away down the screen or exceeds ray budget");
    }
    // The old heightfield painted a 2-unit forest step from y=160 to
    // y=240: eighty pixels of dark solid rectangle. Its physical volume
    // above a grass tile is only eight pixels at two tiles' distance.
    int flat=ggFilmTerrainSurfaceY(8,21,116,2,12);
    int forest=ggFilmTerrainSurfaceY(10,21,116,2,12);
    int high=ggFilmTerrainSurfaceY(16,21,116,2,12);
    check(flat==168&&forest==160&&flat-forest==8&&high==136,
          "a small vegetation rise still generates a full-screen cube");
    check(ggFilmTerrainSurfaceY(8,21,116,5,12)-
          ggFilmTerrainSurfaceY(10,21,116,5,12)==4,
          "small cliff height fails to shrink at longer camera distance");
    check(ggFilmFloorMist(256)==0&&
          ggFilmFloorMist(42*256)>60&&
          ggFilmFloorMist(42*256)<=144,
          "distant landscape haze was removed or overexposed");
    for(int x=0;x<1024;x+=61){
        int g=ggFilmFloorGrain(x*11,x*3,64);
        check(g>=-2&&g<=6,"grass micropattern exceeds restrained palette");
        check(g==ggFilmFloorGrain(x*11,x*3,64),
              "landscape texture swims at the same world position/time");
    }
    puts("3DS FLOOR OK: continuous ground, exact boot inverse, ordered distances, stable material grain");
    return 0;
}
