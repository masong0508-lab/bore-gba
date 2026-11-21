// Standalone 3DS world-stage geometry regression and CPU baseline.
// No generated movie data or Nintendo SDK is needed to validate projection,
// world occupancy, clipping, moving water, and depth-linked actor travel.
// Build: gcc -O2 -std=c11 -Wall -Wextra -D__3DS__ pc_world3d.c -o /tmp/ggworld && /tmp/ggworld
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
typedef uint8_t u8;
enum { FILM_TOP=24, FILM_BOTTOM=168, FILM_H=144 };
enum { F_ORBIT,F_SHORE,F_STORM,F_VILLAGE,F_FORD,F_GATE,F_FORGE,
       F_HALL,F_ARCHIVE,F_HEARTH,F_THRONE,F_COUCH,F_PORCH,F_RUINS,F_SET_N };
enum { F_WIDE,F_NEAR,F_TRACK,F_HAND,F_INSERT };
enum { F_WISP,F_MIRA,F_GRUUL,F_ELOWEN,F_THORGRIM,F_HOLLOW,F_DAVE,
       F_ELDER,F_NESSA,F_SERIN,F_ACTOR_N,F_OFFSCREEN=255 };
enum { F_CUP,F_GOAT,F_HAMMER,F_SPOON,F_QUILL,F_LAMP,F_CROWN,F_LEAF,
       F_BREAD,F_MASK,F_CRUMB,F_SEED,F_PROP_N,F_NO_PROP=255 };
typedef struct { int place,camera,lead,other,prop; } FilmBeat;
typedef struct { int leadX,otherX,leadFeet,otherFeet,leadHeight,otherHeight; } FilmBlocking;
static int filmEase(int elapsed,int total) {
    int u=elapsed>=total?256:(elapsed*256)/total;
    return (u*u*(768-2*u))>>16;
}
static int filmActorLeftOf(int a,int b){return a<b;}
static struct { u8 before[32],page[256*192],after[32]; } picture;
#define uiBuf picture.page
static u8 fmvSlots[32];
#include "source/story_world3d.h"

static void require(int condition,const char *why){
    if(!condition){fprintf(stderr,"WORLD3D FAIL: %s\n",why);exit(1);}
}
static unsigned long hashFrame(void){
    unsigned long h=2166136261u;
    for(int i=FILM_TOP*256;i<FILM_BOTTOM*256;i++)
        h=(h^uiBuf[i])*16777619u;
    return h;
}
static unsigned long frame(int place,int camera,int t){
    FilmBeat b={place,camera,F_MIRA,F_THORGRIM,F_NO_PROP};
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&b,t,filmEase(t,420),t/300,1,0);
    return hashFrame();
}
int main(void){
    memset(picture.before,0xa6,sizeof picture.before);
    memset(picture.after,0x6d,sizeof picture.after);
    for(int i=0;i<32;i++)fmvSlots[i]=(u8)(i+1);
    GgFilmCamera center={0,0},moved={64,20};
    GgS2 near=ggFilmProject(center,ggV3(120,0,560));
    GgS2 far=ggFilmProject(center,ggV3(120,0,1450));
    require(near.x>far.x&&far.x>128,"perspective does not reduce far displacement");
    require(near.y>far.y,"world-space floor has no depth gradient");
    require(ggFilmProject(moved,ggV3(120,0,560)).x!=near.x,
            "a camera track does not transform a point");
    memset(uiBuf,0,sizeof picture.page);
    unsigned long empty=hashFrame();
    int populated=0;
    for(int p=F_SHORE;p<F_SET_N;p++){
        unsigned long h=frame(p,F_WIDE,0);
        populated+=(h!=empty);
        require(picture.before[0]==0xa6&&picture.before[31]==0xa6&&
                picture.after[0]==0x6d&&picture.after[31]==0x6d,
                "world renderer wrote outside the 256x192 film surface");
    }
    require(populated>=10,"set geometry missing in most locations");
    require(frame(F_PORCH,F_NEAR,0)!=frame(F_COUCH,F_NEAR,0),
            "the first-rain porch is still a generic couch stage");
    FilmBeat porch={F_PORCH,F_NEAR,F_MIRA,F_WISP,F_CUP};
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&porch,40,128,1,1,0);
    unsigned long rainPorch=hashFrame();
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&porch,40,128,14,1,0);
    require(hashFrame()!=rainPorch,
            "porch window never clears after the first-rain chapter");
    // The worksite has visibly exposed rafters before its completed roof.
    FilmBeat village={F_VILLAGE,F_WIDE,F_MIRA,F_THORGRIM,F_NO_PROP};
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&village,0,128,2,0,0);
    unsigned long roofOpen=hashFrame();
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&village,0,128,2,1,0);
    require(hashFrame()!=roofOpen,
            "the house roof did not change when construction finished");
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&village,44,128,14,1,1);
    unsigned long snowVillage=hashFrame();
    memset(uiBuf,0,sizeof picture.page);
    ggFilmWorldDraw(&village,44,128,14,1,0);
    require(snowVillage!=hashFrame(),
            "bonus winter stories have the same environment as summer");
    require(frame(F_SHORE,F_NEAR,0)!=frame(F_SHORE,F_NEAR,180),
            "water is not animated within its world plane");
    require(frame(F_STORM,F_NEAR,0)!=frame(F_STORM,F_NEAR,180),
            "world-space rain did not animate");
    FilmBeat walk={F_VILLAGE,F_WIDE,F_MIRA,F_THORGRIM,F_NO_PROP};
    FilmBlocking a={85,171,156,156,45,45},b=a;
    ggFilmWorldBlocking(&walk,0,0,&a);
    ggFilmWorldBlocking(&walk,180,256,&b);
    require(a.leadHeight<b.leadHeight&&a.leadFeet<b.leadFeet,
            "walking actor never approaches in Z");
    unsigned long checksum=0;
    clock_t started=clock();
    for(int t=0;t<480;t+=3)
        checksum^=frame(t/30%F_SET_N,F_TRACK,t);
    double ms=1000.0*(clock()-started)/CLOCKS_PER_SEC/160.0;
    require(picture.before[17]==0xa6&&picture.after[17]==0x6d,"canaries damaged");
    printf("WORLD3D OK: %d populated sets, animated geometry, ground travel, "
           "160 frames / %.3f ms per frame on host CPU, checksum %lu\n",
           populated,ms,checksum);
    return 0;
}
