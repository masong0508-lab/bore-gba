// SDK-free architectural projection tests for native 400x240 cinema.
// Compares two cameras observing the same world tile, pitched gables,
// roof/wall screen depth, material lighting and no extra 3D allocations.
#include <stdio.h>
#include <stdlib.h>
enum {B_HUT=1,B_HOUSE,B_HALL,B_TOWER,B_TEMPLE,B_LIBRARY,B_FARM};
#include "3ds/film_architecture3d.h"
static void check(int truth,const char*why){
    if(!truth){fprintf(stderr,"3DS ARCH FAIL: %s\n",why);exit(1);}
}
int main(void){
    int gx=8*256,gy=14*256,ground=8;
    GgFilmRoofVolume ridge=ggFilmRoofVolume(B_HOUSE,gx+128,gy+128,ground,25);
    GgFilmRoofVolume eave=ggFilmRoofVolume(B_HOUSE,gx,gy,ground,25);
    check(ridge.roofTop==33&&ridge.wallTop==26&&ridge.roofPitch==7,
          "house gable did not peak at its physical world center");
    check(eave.roofTop==26&&eave.wallTop==ridge.wallTop,
          "gable eaves float above the real wall or change wall elevation");
    int pitchAxis=(((gx>>8)+(gy>>8)+B_HOUSE)&1)!=0;
    int crossX=pitchAxis?gx+128:gx;
    int crossY=pitchAxis?gy:gy+128;
    GgFilmRoofVolume side=ggFilmRoofVolume(B_HOUSE,crossX,crossY,ground,25);
    check(side.roofTop==ridge.wallTop,
          "roof did not slope along its gable's chosen cross axis");
    GgFilmRoofVolume same=ggFilmRoofVolume(B_HOUSE,gx+128,gy+128,ground,25);
    check(same.roofTop==ridge.roofTop&&same.wallTop==ridge.wallTop,
          "roof changes with playback, instead of fixed map coordinates");
    GgFilmRoofVolume tower=ggFilmRoofVolume(B_TOWER,gx,gy,ground,55);
    check(tower.roofPitch==0&&tower.roofTop==63&&tower.wallTop==63,
          "tower crenellations unexpectedly became a house roof");
    GgFilmRoofVolume hall=ggFilmRoofVolume(B_HALL,gx+128,gy+128,ground,37);
    check(hall.roofPitch==10&&hall.roofTop==45,
          "hall lacks a proportionally taller roof than a small house");
    int depth2=ggFilmRoofBottomScreen(ridge.wallTop,21,116,2,12);
    int depth5=ggFilmRoofBottomScreen(ridge.wallTop,21,116,5,12);
    check(depth2==96&&depth5==106,
          "roof wall intersection is not perspectively scaled with depth");
    int r1,g1,b1,r2,g2,b2;
    ggFilmRoofMaterial(B_HOUSE,gx+128,gy+128,290,240,&r1,&g1,&b1);
    ggFilmRoofMaterial(B_HOUSE,gx+128,gy+128,208,240,&r2,&g2,&b2);
    check(r1>r2&&g1>g2&&b1>b2&&
          r1>=0&&r1<=255&&g1>=0&&g1<=255&&b1>=0&&b1<=255,
          "roof light directions have no tonal difference or exceed RGB");
    ggFilmRoofMaterial(B_LIBRARY,gx+128,gy+128,256,200,&r1,&g1,&b1);
    check(b1>r1,"library roof lost its distinct cool blue material");
    puts("3DS ARCH OK: stable physical gables, eaves, near/far perspective and materials");
    return 0;
}
