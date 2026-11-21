// Pure deterministic secondary film animation, no graphics SDK required.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_life3d.h"
static void check(int ok,const char*why){
    if(!ok){fprintf(stderr,"3DS LIFE FAIL: %s\n",why);exit(1);}
}
int main(void){
    check(ggFilmLifePulse(10,7)==0&&
          ggFilmLifePulse(20,7)==1&&
          ggFilmLifePulse(64,7)==0,
          "breathing cycle does not lift and settle the torso");
    check(ggFilmLifePulse(20+72,7)==ggFilmLifePulse(20,7),
          "actor breathing desynchronizes after a repeated cycle");
    for(int f=0;f<300;f++){
        int a=ggFilmLifeWeight(f,7),b=ggFilmLifeWeight(f+1,7);
        check(a>=-1&&a<=1&&b>=-1&&b<=1,
              "actor leaned or lurched outside bounded pose range");
        check(ggFilmLifePulse(f,31)>=0&&ggFilmLifePulse(f,31)<=1,
              "breathing unexpectedly moves the planted foot plane");
    }
    check(ggFilmLifeListenCue(25,2)==1&&
          ggFilmLifeListenCue(38,2)==-1&&
          ggFilmLifeListenCue(80,2)==0&&
          ggFilmLifeListenCue(56,3)==-1,
          "listeners have no staggered look-and-reaction choreography");
    check(ggFilmLifeNarratorCue(0,5,1)==0&&
          ggFilmLifeNarratorCue(0,35,1)==2&&
          ggFilmLifeNarratorCue(0,35,0)==1&&
          ggFilmLifeNarratorCue(1,35,1)==1,
          "narrated ending has no staggered, expressive cast animation");
    check(ggFilmLifeNarratorNod(0,30,1)==1&&
          ggFilmLifeNarratorNod(1,30,1)==-1&&
          ggFilmLifeNarratorNod(0,30,0)==0,
          "non-speaking reactions do not follow the authored dialogue time");
    check(ggFilmLifeBrow(1,3,48)==1&&
          ggFilmLifeBrow(1,4,48)==-1&&
          ggFilmLifeBrow(1,1,48)==1&&
          ggFilmLifeBrow(0,1,48)==0,
          "emotionally dissimilar dialogue has one static expression");
    puts("3DS LIFE OK: continuous deterministic motion, timed listener response and semantic emotion");
    return 0;
}
