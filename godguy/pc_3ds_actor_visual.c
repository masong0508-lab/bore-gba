// Source-rendered film actor regression: actual RGB pixels, not just
// a mocked count of drawing calls. Exercises left/right profiles, feet
// and silhouettes in a 400x240 framebuffer without a console SDK.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8;
enum {GG_SAGA_BEAT_TICKS=96};
static char ggOriginHumanCel[24][17];
static int ggExpClamp(int n,int low,int high){
    return n<low?low:n>high?high:n;
}
static void ggExpRect(u8*out,int x,int y,int w,int h,int r,int g,int b){
    if(w<=0||h<=0){fputs("VISUAL FAIL: empty primitive\n",stderr);exit(1);}
    int x0=x<0?0:x,y0=y<0?0:y,x1=x+w,y1=y+h;
    if(x1>400)x1=400;if(y1>240)y1=240;
    for(int py=y0;py<y1;py++)for(int px=x0;px<x1;px++){
        u8*p=out+(py*400+px)*3;
        p[0]=(u8)ggExpClamp(r,0,255);
        p[1]=(u8)ggExpClamp(g,0,255);
        p[2]=(u8)ggExpClamp(b,0,255);
    }
}
#include "3ds/film_acting3d.h"
static u8 front[400*240*3],side[400*240*3];
static void check(int okay,const char*why){
    if(!okay){fprintf(stderr,"3DS VISUAL FAIL: %s\n",why);exit(1);}
}
static int whites(const u8*buf){
    int count=0;
    for(unsigned i=0;i<400*240;i++)
        if(buf[i*3]==230&&buf[i*3+1]==226&&buf[i*3+2]==211)count++;
    return count;
}
int main(void){
    memset(front,48,sizeof(front));
    memset(side,48,sizeof(side));
    const u8 *hair=ggFilmGuestPalette[0][0],
             *skin=ggFilmGuestPalette[0][1],
             *coat=ggFilmGuestPalette[0][2];
    GgFilmPose frontPose=ggFilmPoseFor("The village lives.",1,48,0,0,0,0);
    GgFilmPose profilePose=frontPose;profilePose.turn=4;
    ggFilmDrawGuestActor(front,200,150,2,hair,skin,coat,&frontPose,0);
    ggFilmDrawGuestActor(side,200,150,2,hair,skin,coat,&profilePose,0);
    check(memcmp(front,side,sizeof(front))!=0,
          "three-quarter view still stamps an unchanged front cutout");
    check(whites(front)>whites(side)&&whites(side)>0,
          "turning does not correctly occlude the far eye behind the nose");
    // Shadow and boots contact the SAME world-projected row. Posing
    // upper limbs or turning the head may not shift the feet up/down.
    for(int x=195;x<=205;x++){
        int k=(149*400+x)*3;
        check(front[k]!=48&&side[k]!=48,
              "one or both actor contact shadows disappear when turning");
    }
    for(int y=151;y<240;y++)for(int x=155;x<245;x++){
        int i=(y*400+x)*3;
        check(front[i]==48&&side[i]==48,
              "shoe, silhouette or shadow spills below projected ground");
    }
    GgFilmPose other=ggFilmPoseFor("The village lives.",1,14,0,0,0,0);
    other.turn=-4;
    memset(side,48,sizeof(side));
    ggFilmDrawGuestActor(side,200,150,2,hair,skin,coat,&other,0);
    check(whites(side)>0,
          "opposite three-quarter profile lost its visible eye");
    puts("3DS VISUAL OK: actually distinct profiles, correct far-eye occlusion and physically planted shadows");
    return 0;
}
