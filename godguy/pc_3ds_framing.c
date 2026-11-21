// SDK-free tests for the REAL 3DS cutscene camera composition scorer.
// gcc -O2 -std=c11 -Wall -Wextra -Werror pc_3ds_framing.c -o /tmp/gg-framing && /tmp/gg-framing
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_framing3d.h"
static void check(int ok,const char *reason){
    if(!ok){fprintf(stderr,"3DS FRAMING FAIL: %s\n",reason);exit(1);}
}
int main(void){
    const int hx=10*256+128,hy=10*256+128;
    const int plusX=hx,plusY=hy+256,minusX=hx,minusY=hy-256;
    const int nearX=hx-512,farX=hx-1024;
    int x,depth;
    check(ggFilmFrameProject(nearX,hy,4096,0,hx,hy,&x,&depth)&&
          x==200&&depth==512,"protagonist not projected to shot center");
    check(ggFilmFrameProject(nearX,hy,4096,0,plusX,plusY,&x,&depth)&&
          x==270,"right guest not visible at legible separation");
    check(ggFilmFrameProject(nearX,hy,4096,0,minusX,minusY,&x,&depth)&&
          x==130,"left guest not visible at legible separation");
    check(!ggFilmFrameProject(hx+512,hy,4096,0,hx,hy,&x,&depth),
          "character behind the camera incorrectly visible");
    check(!ggFilmFrameProject(nearX,hy,4096,0,hx,hy+2048,&x,&depth),
          "clipped off-screen speaker passed frame safety check");
    const int both=ggFilmFrameScore(nearX,hy,4096,0,hx,hy,
                          plusX,plusY,minusX,minusY,1,1);
    const int one=ggFilmFrameScore(nearX,hy,4096,0,hx,hy,
                          plusX,plusY,minusX,minusY,1,0);
    const int none=ggFilmFrameScore(nearX,hy,4096,0,hx,hy,
                          plusX,plusY,minusX,minusY,0,0);
    check(both>one&&one>none,
          "director preferred an empty or one-speaker shot");
    check(ggFilmFrameHeightPenalty(8,8)==0&&
          ggFilmFrameHeightPenalty(8,12)==72&&
          ggFilmFrameHeightPenalty(12,8)==72,
          "camera height bias is not symmetric or scale-bounded");
    check(both-ggFilmFrameHeightPenalty(8,14)<one,
          "director preferred perched guest silhouettes over level terrain");
    const int distant=ggFilmFrameScore(farX,hy,4096,0,hx,hy,
                          plusX,plusY,minusX,minusY,1,1);
    check(both>distant,"distant tiny figures were favored over legible cels");
    const int overlap=ggFilmFrameScore(nearX,hy,4096,0,hx,hy,
                          hx,hy,hx,hy,1,1);
    check(both>overlap+150,
          "guest silhouettes merged into protagonist instead of separating");
    const int onlyMinus=ggFilmFrameScore(nearX,hy,4096,0,hx,hy,
                          plusX,plusY,minusX,minusY,0,1);
    check(onlyMinus>none,"blocked plus side erased surviving speaking guest");
    const int occluded=ggFilmFrameScore(nearX,hy,4096,0,hx,hy,
                          plusX,hy+4096,minusX,minusY,1,0);
    check(one>occluded+100,
          "director accepted a physically off-screen guest");
    // Orthogonal framing uses the same live-world fixed-point geometry.
    const int north=ggFilmFrameScore(hx,hy-512,0,4096,hx,hy,
                          hx+256,hy,hx-256,hy,1,1);
    check(north>one,"rotating the shot lost world-relative stage flanks");
    check(ggFilmFrameScore(hx+512,hy,4096,0,hx,hy,
          plusX,plusY,minusX,minusY,1,1)==-10000,
          "unsafe camera heading was rewarded");
    puts("3DS FRAMING OK: centered hero, visible distinct guests, close readable camera, clipped-actor rejection");
    return 0;
}
