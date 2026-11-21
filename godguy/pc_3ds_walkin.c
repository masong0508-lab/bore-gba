// SDK-free 3DS film approach: no collision bypass, snap or second-beat reset.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_walk3d.h"
static void check(int ok,const char *why){
    if(!ok){fprintf(stderr,"3DS WALK-IN FAIL: %s\n",why);exit(1);}
}
int main(void){
    int x,y,prior=0;
    for(int t=0;t<=48;t++){
        ggFilmWalkWorldPosition(20,20,19,20,4096,0,0,t,96,&x,&y);
        check(y==20*256+128,"walk drifted off the terrain lane");
        if(t)check(x>=prior,"world position reversed during a walk-in");
        check(x>=19*256+128&&x<=20*256+128,"walk left the two clear cells");
        prior=x;
    }
    check(x==20*256+128,"walk never reached its mark");
    for(int t=0;t<96;t++){
        ggFilmWalkWorldPosition(20,20,19,20,4096,0,1,t,96,&x,&y);
        check(x==20*256+128&&y==20*256+128,
              "the paired dialogue beat snapped the actor backward");
    }
    ggFilmWalkWorldPosition(20,20,20,20,4096,0,0,0,96,&x,&y);
    check(x>=20*256&&x<=20*256+255&&y==20*256+128,
          "blocked approach left the safe staging tile");
    ggFilmWalkWorldPosition(20,20,20,20,4096,0,0,48,96,&x,&y);
    check(x==20*256+128&&y==20*256+128,
          "blocked approach did not settle into the grounded mark");
    ggFilmWalkWorldPosition(20,20,20,20,-2896,2896,0,-50,96,&x,&y);
    check(x>=20*256&&x<21*256&&y>=20*256&&y<21*256,
          "diagonal fallback crossed a blocked cell");
    check(ggFilmWalkProgress(0,-1,96)==0&&
          ggFilmWalkProgress(0,500,96)==256&&
          ggFilmWalkProgress(1,0,96)==256,
          "shot interpolation or edge clamps broken");
    puts("3DS WALK-IN OK: one-tile terrain travel, continuous dialogue, safe blocked fallback");
    return 0;
}
