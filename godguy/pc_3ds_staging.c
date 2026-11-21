// SDK-free regression for 3DS film blocking and painter-order continuity.
// gcc -O2 -std=c11 -Wall -Wextra -Werror pc_3ds_staging.c -o /tmp/gg-staging && /tmp/gg-staging
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_stage3d.h"
static void check(int ok,const char *why){
    if(!ok){fprintf(stderr,"STAGING FAIL: %s\n",why);exit(1);}
}
int main(void){
    for(int p=0;p<=1;p++)for(int m=0;m<=1;m++){
        int first=ggFilmStageGuestSide(p,m,0);
        int second=ggFilmStageGuestSide(p,m,1);
        int prop=ggFilmStagePropSide(p,m,1);
        check((first!=1||p)&&(first!=-1||m),
              "primary guest was placed on a blocked tile");
        check((second!=1||p)&&(second!=-1||m),
              "second guest was placed on a blocked tile");
        check((prop!=1||p)&&(prop!=-1||m),
              "prop was placed on a blocked tile");
        check(first==0||prop!=first,"grounded prop shares the guest tile");
        check(first==0||second!=first,"two guests overlap on one tile");
        check(ggFilmStagePropSide(p,m,2)==0,
              "three-actor conversation spawned an obstructing prop");
    }
    check(ggFilmStageGuestSide(1,1,0)==1&&
          ggFilmStageGuestSide(1,1,1)==-1&&
          ggFilmStagePropSide(1,1,1)==-1,
          "fully open set lost distinct left/right stage positions");
    check(ggFilmStageGuestSide(0,1,0)==-1&&
          ggFilmStageGuestSide(0,1,1)==0&&
          ggFilmStagePropSide(0,1,1)==0,
          "single-side fallback would overlap actor and prop");
    check(ggFilmStageGuestSide(0,0,0)==0,
          "impassable scene wrongly spawned guests");
    check(ggFilmStageDepth(5*256,4*256,3*256,4*256,4096,0)==512,
          "camera-relative forward depth wrong");
    check(ggFilmStageDepth(5*256,4*256,3*256,4*256,0,4096)==0,
          "rotated camera gives incorrect depth");
    GgFilmStageItem items[4]={
        {GG_STAGE_HERO,200,110,2,400},
        {GG_STAGE_GUEST_ONE,170,119,3,600},
        {GG_STAGE_GUEST_TWO,210,120,3,400},
        {GG_STAGE_PROP,220,115,1,800}
    };
    ggFilmStageSort(items,4);
    check(items[0].kind==GG_STAGE_PROP&&
          items[1].kind==GG_STAGE_GUEST_ONE&&
          items[2].kind==GG_STAGE_HERO&&
          items[3].kind==GG_STAGE_GUEST_TWO,
          "far actors and props did not paint behind near actors");
    // Tie ordering must not flicker across adjacent animation frames.
    ggFilmStageSort(items,4);
    check(items[2].kind==GG_STAGE_HERO&&
          items[3].kind==GG_STAGE_GUEST_TWO,
          "same-depth guest flickered across frames");
    GgFilmStageItem alone={GG_STAGE_HERO,0,0,1,100};
    ggFilmStageSort(&alone,1);
    check(alone.kind==GG_STAGE_HERO,"single-character shot was corrupted");
    puts("3DS STAGING OK: distinct guest/prop tiles, depth-ordered cel drawing, stable ties");
    return 0;
}
