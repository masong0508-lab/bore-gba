// SDK-free foreground occlusion regression: the raycaster's real
// painted spans, two near/far layers, correct silhouettes and bounded RAM.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_occlusion3d.h"
static void check(int ok,const char *reason){
    if(!ok){fprintf(stderr,"3DS OCCLUSION FAIL: %s\n",reason);exit(1);}
}
static GgFilmOcclusionColumn columns[GG_FILM_OCCLUSION_COLS];
int main(void){
    check(sizeof columns==3200,"occlusion scratch exceeded 3.2 KiB");
    ggFilmOcclusionClear(columns);
    check(!ggFilmOcclusionHidden(columns,200,110,1200),
          "stale first-frame depth corrupts the picture");
    ggFilmOcclusionAdd(columns,200,80,138,512);
    ggFilmOcclusionAdd(columns,200,42,80,1024);
    check(ggFilmOcclusionHidden(columns,200,100,1200),
          "actor behind near wall is drawn on top");
    check(!ggFilmOcclusionHidden(columns,200,100,512),
          "actor on wall's plane was hidden");
    check(!ggFilmOcclusionHidden(columns,200,100,600),
          "near foreground tolerance hides a same-plane figure");
    check(!ggFilmOcclusionHidden(columns,200,139,1200)&&
          !ggFilmOcclusionHidden(columns,199,100,1200),
          "occlusion leaks below the foundation or between columns");
    check(ggFilmOcclusionHidden(columns,200,60,1200),
          "higher distant roof should mask a farther figure");
    check(!ggFilmOcclusionHidden(columns,200,60,600),
          "distant roof erased an actor standing in front");
    ggFilmOcclusionAdd(columns,200,70,110,512);
    check(ggFilmOcclusionHidden(columns,200,75,1200)&&
          ggFilmOcclusionHidden(columns,200,137,1200),
          "same-depth wall bands failed to coalesce");
    ggFilmOcclusionAdd(columns,200,12,28,2048);
    check(!ggFilmOcclusionHidden(columns,200,18,3600),
          "third layer exceeded bounded two-layer cache");
    ggFilmOcclusionAdd(columns,399,-120,260,768);
    check(ggFilmOcclusionHidden(columns,399,0,1200)&&
          ggFilmOcclusionHidden(columns,399,239,1200),
          "screen clipping mishandled first/last film row");
    check(!ggFilmOcclusionHidden(columns,400,80,1200)&&
          !ggFilmOcclusionHidden(columns,-1,80,1200),
          "out-of-view column access escaped depth guard");
    ggFilmOcclusionClear(columns);
    check(!ggFilmOcclusionHidden(columns,200,100,1600)&&
          !ggFilmOcclusionHidden(columns,399,239,1600),
          "old cinematic frame occluded a new camera take");
    puts("3DS OCCLUSION OK: 3.2 KiB two-layer world depth, wall/roof contact, no bleed");
    return 0;
}
