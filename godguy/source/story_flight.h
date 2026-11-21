// GodGuy step four: uninterrupted camera flight over the player's real map.
// DS/DSi draw a cheap, animated perspective; the 3DS renders projected
// terrain and buildings into the existing film page. Read-only camera:
// no duplicate world, save changes, heap allocations or video buffers.
#ifndef GG_STORY_FLIGHT_H
#define GG_STORY_FLIGHT_H
static int filmFlightHeight(int t){
    return t==T_MOUNT?105:t==T_HILL?49:t==T_SNOW?67:
           t==T_FOREST?29:t==T_JUNGLE?33:t==T_GRASS?12:
           t==T_SWAMP?5:t==T_SAND?4:t==T_DESERT?8:
           t==T_DIRT?9:0;
}
static u8 filmFlightInk(int t){
    return t==T_MOUNT?20:t==T_HILL?21:t==T_SNOW?22:
           t==T_FOREST?16:t==T_JUNGLE?17:t==T_SAND?26:
           t==T_DESERT?25:t==T_DIRT?24:
           filmVistaIsWater(t)?13:18;
}
static void filmFlight2D(const FilmRouteCamera *camera,int t){
    // Only 7x13 samples. Every sample is rotated into authentic world
    // coordinates. The cheap DS/DSi version has stable outlines and
    // relative building locations across the entire continuous shot.
    for(int depth=13;depth>=1;depth-=2){
        int rowY=83+(14-depth)*5;
        int size=filmRouteMax(4,19-depth);
        int step=filmRouteMax(4,34-depth*2);
        for(int side=-6;side<=6;side++){
            int building;
            int tile=filmRouteTerrain(camera,depth,side,WW,WH,
                                      filmWorldCell,&building);
            int x=128+side*step-size/2;
            int height=filmFlightHeight(tile);
            int rise=height*(15-depth)/40;
            int y=rowY-rise;
            uiRect(x,y,size,filmRouteMax(3,6+rise/3),
                   fmvSlots[filmFlightInk(tile)]);
            if(filmVistaIsWater(tile)&&((t/16+side+depth)&3)==0)
                uiRect(x+1,rowY,size>4?size-2:2,1,fmvSlots[15]);
            if(building){
                int h=building==B_TOWER?14:building==B_HALL?12:
                      building==B_HOUSE?9:building==B_HUT?7:
                      building==B_DOCK?4:6;
                uiRect(x,y-h,size,h,fmvSlots[20]);
                if(building==B_TOWER||building==B_HALL)
                    uiRect(x-1,y-h-2,size+2,3,fmvSlots[25]);
                else if(building==B_HOUSE||building==B_HUT)
                    uiRect(x,y-h-2,size,2,fmvSlots[24]);
            }
        }
    }
}
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
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
    // A single uninterrupted trip; keep the bars and replace only the
    // world viewport. The 3DS and DS use identical geographical samples.
    uiRect(0,FILM_TOP,256,FILM_H,fmvSlots[2]);
    uiRect(0,95,256,FILM_BOTTOM-95,fmvSlots[13]);
    FilmRouteCamera camera=filmRouteCameraAt(t);
#if defined(__3DS__) && !defined(GG_FILM_2D_FALLBACK)
    filmFlight3D(&camera,t);
#else
    filmFlight2D(&camera,t);
#endif
    int traveled=(int)((long long)t*filmRoute.total/
                       filmRoute.duration);
    int next=filmRoute.count>2&&
             traveled>=filmRoute.length[0]?2:1;
    const char *landmark=filmRoute.kind[next]==FILM_VISTA_COAST?
                           "THE REAL COAST":
                         filmRoute.kind[next]==FILM_VISTA_RIDGE?
                           "THE REAL RIDGE":"YOUR SETTLEMENT";
    uiRect(0,169,256,17,U_SHADOW);
    uiTextC(128,170,"A JOURNEY THROUGH YOUR WORLD",U_GOLD,1);
    char label[48];
    siprintf(label,"%s  %d,%d",landmark,
             camera.x>>8,camera.y>>8);
    uiTextC(128,180,label,U_WHITE,1);
    gmFilmMixSet(1,0,64);
    return 1;
}
#endif
