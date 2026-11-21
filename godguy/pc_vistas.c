// Source-faithful cutscene vista test: no ROM/SDK or procedurally invented
// replacement world. Tests the same compact capture code used on DS/3DS.
// gcc -O2 -std=c11 -DPCTEST pc_vistas.c -o /tmp/gg-vistas && /tmp/gg-vistas
#include <stdio.h>
#include <stdlib.h>
#define PCTEST 1
#include "source/gg.h"
enum { F_ORBIT,F_SHORE,F_STORM,F_VILLAGE,F_FORD,F_GATE,F_FORGE,
       F_HALL,F_ARCHIVE,F_HEARTH,F_THRONE,F_COUCH,F_PORCH,F_RUINS,F_SET_N };
enum { B_NONE,B_HUT,B_HOUSE,B_HALL,B_FARM,B_TOWER,B_RUIN,
       B_DOCK,B_TEMPLE,B_STATUE,B_LIBRARY,B_BORDER,B_N };
#include "source/story_vistas.h"
#include "source/story_route.h"

static void require(int ok,const char *message){
    if(!ok){fprintf(stderr,"VISTA FAIL: %s\n",message);exit(1);}
}
static int inventedSceneCells;
// Two different synthetic *game maps*, designed to test selection, rather
// than "fake" movie scenery. The actual films call a live world[][] reader.
static int terrainTown(int x,int y,int layer){
    if(layer){
        if(x==64&&y==64)return B_HALL;
        if(x==61&&y==63)return B_TOWER;
        if(x==67&&y==64)return B_HOUSE;
        if(x==46&&y==64)return B_DOCK;
        return B_NONE;
    }
    if(x<45)return T_WATER;
    if(x<=49)return T_SAND;
    if(x>=92)return T_MOUNT;
    if(x>=87)return T_HILL;
    return y%5==0?T_FOREST:T_GRASS;
}
static int plainWorld(int x,int y,int layer){
    (void)x;(void)y;
    return layer?B_NONE:T_GRASS;
}
int main(void){
    filmVistaCapture(160,160,64,64,64,64,terrainTown);
    const FilmVista *town=&filmVista[FILM_VISTA_TOWN];
    require(town->valid&&town->x==64&&town->y==64,
            "village camera did not lock onto the actual site");
    require(filmVistaRead(town,12,4,1)==B_HALL,
            "center hall is missing from the captured town");
    require(filmVistaRead(town,9,3,1)==B_TOWER,
            "real tower lost its position relative to the hall");
    require(filmVistaRead(town,15,4,1)==B_HOUSE,
            "house moved away from its real map tile");
    require(filmVistaRead(town,0,0,1)==B_NONE,
            "nonexistent buildings appeared on open terrain");
    const FilmVista *shore=&filmVista[FILM_VISTA_COAST];
    const FilmVista *ridge=&filmVista[FILM_VISTA_RIDGE];
    require(shore->valid,"coastline search failed on a true water/sand edge");
    require(ridge->valid,"mountain search failed on a true ridge");
    require(shore->x>=45&&shore->x<=49,
            "shoreline was not anchored to the actual sand-water boundary");
    require(ridge->x>=87,
            "the ridge shot does not reflect the real mountain position");
    int water=0,sand=0,peak=0;
    for(int j=0;j<FILM_VISTA_H;j++)for(int i=0;i<FILM_VISTA_W;i++){
        int t=filmVistaRead(shore,i,j,0);
        water+=filmVistaIsWater(t);
        sand+=t==T_SAND;
        peak+=filmVistaRead(ridge,i,j,0)==T_MOUNT;
    }
    require(water>0&&sand>0,"beach shot lost its beach or sea");
    require(peak>0,"ridge lost real mountain tiles");
    require(filmVistaPlace(F_VILLAGE)==FILM_VISTA_TOWN&&
            filmVistaPlace(F_GATE)==FILM_VISTA_TOWN,
            "castle and village scenes use different settlement maps");
    require(filmVistaPlace(F_SHORE)==FILM_VISTA_COAST&&
            filmVistaPlace(F_FORD)==FILM_VISTA_COAST&&
            filmVistaPlace(F_RUINS)==FILM_VISTA_RIDGE,
            "cliffs and coastline shots use the wrong world landmark");
    require(filmVistaPlace(F_PORCH)==-1&&filmVistaPlace(F_ARCHIVE)==-1&&
            filmVistaPlace(F_HALL)==-1,
            "distant landscape paints over a furnished interior wall");
    // Genuine camera flight: sample one continuous sequence from the
    // hall across the real coast to the sampled mountains. Positions
    // are Q8 game tiles, not unrelated static shot/pan coordinates.
    filmRouteCapture(1,1080);
    require(filmRoute.count==3&&filmRoute.total>10&&
            filmRoute.kind[1]==FILM_VISTA_COAST&&
            filmRoute.kind[2]==FILM_VISTA_RIDGE,
            "the geographical route lost its real coast or cliff");
    FilmRouteCamera start=filmRouteCameraAt(0);
    FilmRouteCamera mid=filmRouteCameraAt(540);
    FilmRouteCamera finish=filmRouteCameraAt(1080);
    require(start.x==town->x*256&&start.y==town->y*256,
            "journey did not start at the actual player city");
    require(finish.x==ridge->x*256&&finish.y==ridge->y*256,
            "journey failed to reach the actual mountain landmark");
    require(start.x!=mid.x&&mid.x!=finish.x,
            "continuous flight never changes map coordinates");
    int smallDiff=0,moving=0;
    FilmRouteCamera previous=start;
    for(int t=1;t<=1080;t++){
        FilmRouteCamera camera=filmRouteCameraAt(t);
        int delta=filmRouteAbs(camera.x-previous.x)+
                  filmRouteAbs(camera.y-previous.y);
        require(delta<300,
                "flight teleported through the planet between movie frames");
        if(delta>0&&delta<256)smallDiff++;
        if(delta>0)moving++;
        int building=0;
        int terrain=filmRouteTerrain(&camera,2,0,160,160,
                                      terrainTown,&building);
        require(terrain>=T_DEEP&&terrain<T_N&&
                building>=B_NONE&&building<B_N,
                "flight sampled outside the actual map bounds");
        previous=camera;
    }
    require(moving>700&&smallDiff>500,
            "camera lacks genuine sub-tile progress through the world");
    // No coastline exists on the second world; do not invent one.
    filmVistaCapture(128,128,64,64,64,64,plainWorld);
    filmRouteCapture(1,1080);
    require(filmRoute.count==0&&filmRoute.duration==0,
            "an invented beach triggered a cinematic flight");
    require(filmVista[FILM_VISTA_TOWN].valid&&
            !filmVista[FILM_VISTA_COAST].valid&&
            !filmVista[FILM_VISTA_RIDGE].valid,
            "cutscene created a fake beach/cliff on a grass-only map");
    // Boundary sampling must not fabricate deep ocean outside a small
    // world. The actual playable map is bounded; the outer pixel extends
    // the same edge cell rather than an imaginary beach past its border.
    filmVistaSample(&filmVista[FILM_VISTA_TOWN],0,0,1,
                    128,128,plainWorld);
    for(int j=0;j<FILM_VISTA_H;j++)for(int i=0;i<FILM_VISTA_W;i++)
        require(filmVistaRead(&filmVista[FILM_VISTA_TOWN],i,j,0)==T_GRASS,
                "world-edge cinema incorrectly introduces open water");
    require(sizeof filmVista+sizeof filmRoute<1700,
            "geographic flight exceeded the 1.7 KiB added RAM budget");
    require(sizeof filmVista<1600,
            "world-based cinematics exceeded the 1.6 KiB scenery budget");
    (void)inventedSceneCells;
    puts("WORLD VISTAS OK: saved settlement layout, true coast/cliff, "
         "no imaginary castles, stable 2D skyline and 1.4 KiB memory footprint");
    return 0;
}
