// SDK-free identity/turntable contract for authored geometry.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_anatomy3d.h"
static void test(int okay,const char *why){
    if(!okay){fprintf(stderr,"3DS ANATOMY FAIL: %s\n",why);exit(1);}
}
int main(void){
    GgFilmAnatomy mira=ggFilmAnatomyFor(0),guard=ggFilmAnatomyFor(1),
        varro=ggFilmAnatomyFor(2),serin=ggFilmAnatomyFor(3),
        elowen=ggFilmAnatomyFor(4);
    test(guard.shoulder>varro.shoulder&&varro.shoulder>mira.shoulder&&
         mira.shoulder>serin.shoulder,"identical generic shoulders");
    test(guard.jaw>mira.jaw&&mira.jaw>serin.jaw&&serin.nose>mira.nose,
         "beard, cheek and nose geometry are indistinguishable");
    test(elowen.hip>serin.hip&&elowen.eyeGap>serin.eyeGap,
         "facial identity lacks variant proportions");
    for(int id=0;id<7;id++){
        GgFilmAnatomy shape=ggFilmAnatomyFor(id);
        test(shape.skull>shape.jaw&&shape.cheek>=shape.jaw,
             "jaw larger than skull");
        for(int turn=-4;turn<=4;turn++)
            test(ggFilmAnatomyWidth(shape.shoulder,turn,8)>=8&&
                 ggFilmAnatomyWidth(shape.skull,turn/2,7)>=7,
                 "turning collapsed torso or skull");
    }
    test(ggFilmAnatomyFor(-1).shoulder==ggFilmAnatomyFor(6).shoulder,
         "invalid actor index escaped player fallback");
    puts("3DS ANATOMY OK: bounded distinct silhouettes, jaws and profiles");
    return 0;
}
