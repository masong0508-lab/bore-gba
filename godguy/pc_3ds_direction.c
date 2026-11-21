// No-SDK camera and character motion director tests. Cinematic look
// targets are pure functions of script time; pause, replay and jumping
// between lines must never pop the camera or rotate a full 2D billboard.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_direction3d.h"
static void check(int b,const char*why){
    if(!b){fprintf(stderr,"3DS DIRECTION FAIL: %s\n",why);exit(1);}
}
int main(void){
    check(ggFilmDirectionWeight(0,96)==0&&
          ggFilmDirectionWeight(24,96)==256&&
          ggFilmDirectionWeight(48,96)==256&&
          ggFilmDirectionWeight(95,96)<20&&
          ggFilmDirectionWeight(96,96)==0,
          "camera direction does not establish, hold and settle");
    check(ggFilmFocusYaw(24,96,1)==3&&
          ggFilmFocusYaw(24,96,-1)==-3&&
          ggFilmFocusYaw(24,96,0)==0,
          "speaker yaw is neither bounded nor left/right symmetric");
    check(ggFilmFigureTurn(24,96,1)==4&&
          ggFilmFigureTurn(24,96,-1)==-4&&
          ggFilmFigureTurn(24,96,0)==0,
          "cast profiles do not look toward the line's actor");
    int prevYaw=0,prevTurn=0;
    for(int t=0;t<=96;t++){
        int yaw=ggFilmFocusYaw(t,96,1);
        int turn=ggFilmFigureTurn(t,96,-1);
        check(yaw>=0&&yaw<=3&&turn<=0&&turn>=-4,
              "camera or silhouette is spinning beyond film budget");
        int dy=yaw-prevYaw;if(dy<0)dy=-dy;
        int dt=turn-prevTurn;if(dt<0)dt=-dt;
        check(dy<=1&&dt<=1,"single-frame camera/sprite hard pop");
        prevYaw=yaw;prevTurn=turn;
    }
    check(prevYaw==0&&prevTurn==0,
          "the end of a dialogue beat does not match the following cut");
    check(ggFilmFocusYaw(3,0,1)==0&&
          ggFilmFigureTurn(3,0,-1)==0,
          "zero-duration cinematic generated invalid motion");
    puts("3DS DIRECTION OK: coherent camera ease, far/near profiles and frame-to-frame continuity");
    return 0;
}
