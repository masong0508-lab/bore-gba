// Travel across the player's map in the existing film viewport.
// DS and DSi use a flat overhead shot. The 3DS projects the same route.
#ifndef GG_STORY_FLIGHT_H
#define GG_STORY_FLIGHT_H
static u8 filmFlightInk(int t){
    return t==T_MOUNT?20:t==T_HILL?21:t==T_SNOW?22:
           t==T_FOREST?16:t==T_JUNGLE?17:t==T_SAND?26:
           t==T_DESERT?25:t==T_DIRT?24:
           filmVistaIsWater(t)?13:18;
}
#if !defined(__3DS__) || defined(GG_FILM_2D_FALLBACK)
static void filmFlightMap(const FilmRouteCamera *camera,int t){
    const int panX=(camera->x>>5)-128;
    const int panY=(camera->y>>5)-FILM_H/2;
    const int x0=-(int)((unsigned)panX&7);
    const int yOffset=-(int)((unsigned)panY&7);
    for(int ty=(panY+yOffset)/8,sy=FILM_TOP+yOffset;sy<FILM_BOTTOM;ty++,sy+=8){
        int y0=filmRouteMax(sy,FILM_TOP);
        int y1=sy+8<FILM_BOTTOM?sy+8:FILM_BOTTOM;
        for(int tx=(panX+x0)/8,sx=x0;sx<256;tx++,sx+=8){
            if((unsigned)tx>=WW||(unsigned)ty>=WH){
                // An uncharted margin lets the camera follow a route right
                // to the edge of the map without stalling or inventing land.
                uiRect(sx,y0,8,y1-y0,fmvSlots[2+((tx^ty)&1)]);
                continue;
            }
            int tile=world[ty][tx], building=bld[ty][tx];
            uiRect(sx,y0,8,y1-y0,fmvSlots[filmFlightInk(tile)]);
            // Details stay on their tiles, including along the frame edge.
            int detailY=sy+3;
            if(detailY<y0||detailY>=y1) continue;
            if(building){
                int left=building==B_HUT?2:1, width=8-left*2;
                int roof=building==B_TOWER||building==B_HALL?25:24;
                int roofY=filmRouteMax(y0,sy+1);
                int roofEnd=sy+7<y1?sy+7:y1;
                uiRect(sx+left,roofY,width,roofEnd-roofY,fmvSlots[roof]);
                uiRect(sx+left+1,detailY,width-2,1,
                       fmvSlots[building==B_TOWER?25:26]);
                if(sy+5<y1) uiRect(sx+3,sy+5,2,1,fmvSlots[20]);
            }else if(filmVistaIsWater(tile)&&((tx*3+ty+t/24)&7)==0){
                uiRect(sx+2,detailY,4,1,fmvSlots[15]);
            }else if((tile==T_FOREST||tile==T_JUNGLE)&&((tx^ty)&1)){
                uiRect(sx+2,detailY,3,1,fmvSlots[19]);
            }else if(tile==T_MOUNT){
                uiRect(sx+2,detailY,4,1,fmvSlots[21]);
            }else if(tile==T_GRASS&&((tx*11+ty*7)&7)==0){
                uiRect(sx+1,detailY,2,1,fmvSlots[17]);
            }
        }
    }
}
#else
static int filmFlightHeight(int t){
    return t==T_MOUNT?105:t==T_HILL?49:t==T_SNOW?67:
           t==T_FOREST?29:t==T_JUNGLE?33:t==T_GRASS?12:
           t==T_SWAMP?5:t==T_SAND?4:t==T_DESERT?8:
           t==T_DIRT?9:0;
}
static void filmFlight3D(const FilmRouteCamera *camera,int t){
    GgFilmCamera projection={0,-280};
    int models=0,trees=0;
    // Far to near to respect painter's order. At most 13x9 terrain cells,
    // six models and two trees: bounded time on old 3DS hardware.
    for(int depth=13;depth>=1;depth-=1){
        int z=250+depth*116;
        for(int side=-4;side<=4;side++){
            int building;
            int tile=filmRouteTerrain(camera,depth,side,WW,WH,
                                      filmWorldCell,&building);
            int h=filmFlightHeight(tile),x=side*120;
            ggFilmQuad(projection,ggV3(x-60,h,z-57),
                       ggV3(x+60,h,z-57),
                       ggV3(x+60,h,z+57),
                       ggV3(x-60,h,z+57),filmFlightInk(tile));
            int ignored;
            int ahead=filmRouteTerrain(camera,depth-1,side,WW,WH,
                                        filmWorldCell,&ignored);
            int front=filmFlightHeight(ahead);
            if(h>front+6){
                ggFilmQuad(projection,ggV3(x-60,front,z-57),
                           ggV3(x+60,front,z-57),
                           ggV3(x+60,h,z-57),
                           ggV3(x-60,h,z-57),
                           tile==T_MOUNT?20:tile==T_HILL?21:24);
            }
            if(filmVistaIsWater(tile)&&((t/18+depth+side)&3)==0){
                ggFilmQuad(projection,ggV3(x-43,1,z-53),
                           ggV3(x+42,1,z-53),
                           ggV3(x+42,1,z-57),
                           ggV3(x-43,1,z-57),15);
            }
            if(building&&models<6){
                if(building==B_TOWER){
                    ggFilmBox(projection,x,h,z,57,170,60,20,21,25);
                    ggFilmBox(projection,x,h+166,z,72,16,72,25,24,26);
                }else if(building==B_HALL||building==B_HOUSE||
                         building==B_HUT){
                    ggFilmHouse(projection,x,z,building==B_HALL?174:
                                building==B_HOUSE?115:86,
                                building==B_HALL?146:
                                building==B_HOUSE?107:73,1,1);
                }else if(building==B_DOCK){
                    ggFilmBox(projection,x,h,z,103,12,70,24,20,25);
                }else if(building==B_RUIN){
                    ggFilmBox(projection,x,h,z,64,74,58,21,20,24);
                }else{
                    ggFilmBox(projection,x,h,z,59,76,60,21,20,25);
                }
                models++;
            }else if((tile==T_FOREST||tile==T_JUNGLE)&&trees<2&&
                      depth>2){
                ggFilmTree(projection,x,z,90,t,depth+side);
                trees++;
            }
        }
    }
}
#endif
static int filmFlightDraw(const FmvClip *clip,int t){
    if(filmRoute.duration<=0||t<0||t>=filmRoute.duration)return 0;
    if(t%FILM_STAGE_TICKS)return 0;
    if(t==0){
        fmvSetPalette();
        filmBars(clip->title);
        if(filmTownName[0])uiText(6,15,filmTownName,U_CYAN,1);
    }
    FilmRouteCamera camera=filmRouteCameraAt(t);
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    int localBuilding;
    int below=filmRouteTerrain(&camera,0,0,WW,WH,filmWorldCell,&localBuilding);
    uiRect(0,FILM_TOP,256,FILM_H,fmvSlots[2]);
    uiRect(0,95,256,FILM_BOTTOM-95,fmvSlots[filmFlightInk(below)]);
    filmFlight3D(&camera,t);
#else
    filmFlightMap(&camera,t);
#endif
    int traveled=(int)((long long)t*filmRoute.total/
                       filmRoute.duration);
    int next=filmRoute.count>2&&
             traveled>=filmRoute.length[0]?2:1;
    const char *landmark=filmRoute.kind[next]==FILM_VISTA_COAST?
                         "ALONG THE COAST":
                         filmRoute.kind[next]==FILM_VISTA_RIDGE?
                         "UP TO THE RIDGE":
                         "APPROACHING THE SETTLEMENT";
    uiRect(0,169,256,17,U_SHADOW);
    uiTextC(128,174,landmark,U_WHITE,1);
    gmFilmMixSet(1,0,64);
    return 1;
}
#endif
