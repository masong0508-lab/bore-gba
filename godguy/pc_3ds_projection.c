// SDK-free ground contact and pixel-perspective regression.
// This reproduces the 400x240 raycaster's ground projection, without
// installing a console SDK, and catches the screenshot's floating cast.
#include <stdio.h>
#include <stdlib.h>
#include "3ds/film_projection3d.h"
static void check(int ok,const char*why){
    if(!ok){fprintf(stderr,"3DS PROJECTION FAIL: %s\n",why);exit(1);}
}
int main(void){
    check(ggFilmProjectFootY(512,8,8)==155,
          "actors at two tiles float above ground / camera eye omitted");
    check(ggFilmProjectFootY(768,8,8)==147,
          "three-tile feet do not follow the raycaster ground plane");
    check(ggFilmProjectFootY(640,8,8)==150&&
          ggFilmProjectFootY(640,8,8)<ggFilmProjectFootY(512,8,8)&&
          ggFilmProjectFootY(640,8,8)>ggFilmProjectFootY(768,8,8),
          "camera dolly snaps a grounded actor between tile sample bands");
    check(ggFilmProjectFootY(1024,8,8)==142,
          "four-tile feet are not grounded on the receding grass");
    check(ggFilmProjectFootY(512,8,12)==143&&
          ggFilmProjectFootY(512,12,8)==167,
          "cliff height moves boots the wrong direction");
    check(ggFilmProjectScreenX(0,512)==200&&
          ggFilmProjectScreenX(256,512)==325&&
          ggFilmProjectScreenX(-256,512)==75,
          "actor horizontal field of view differs from voxel raycaster");
    check(ggFilmProjectScreenX(128,0)==-10000&&
          ggFilmProjectFootY(0,8,8)==-10000,
          "zero forward depth divided by zero");
    check(ggFilmShotScale(2)==3&&ggFilmShotScale(3)==2&&
          ggFilmShotScale(4)==2&&ggFilmShotScale(5)==2&&
          ggFilmShotScale(6)==1,
          "actors change aspect scale during a fixed camera take");
    for(int d=2;d<=5;d++){
        int s=ggFilmShotScale(d);
        int depth=d*256;
        int feet=ggFilmProjectFootY(depth,8,8);
        check(ggFilmFootSafe(200,feet,s),
              "cinematic feet do not fit between title and captions");
        check(ggFilmFootSafe(125,feet,s)&&
              ggFilmFootSafe(275,feet,s),
              "two flanking guests are clipped in a legal ensemble shot");
        check(!ggFilmFootSafe(390,feet,s),
              "actor rendered cropped through right edge");
    }
    check(!ggFilmFootSafe(200,70,3)&&
          !ggFilmFootSafe(200,208,2),
          "offscreen heads / subtitle-overlapping boots were accepted");
    puts("3DS PROJECTION OK: planted boots, shared lens, identical cast scaling and safe framing");
    return 0;
}
