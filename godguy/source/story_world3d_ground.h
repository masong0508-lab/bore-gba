#ifndef GG_STORY_WORLD3D_GROUND_H
#define GG_STORY_WORLD3D_GROUND_H

enum {
    GG_FILM_BRIDGE_CHAPTER=8, GG_FILM_BRIDGE_TOP=34,
    GG_FILM_BRIDGE_FIRST=466, GG_FILM_BRIDGE_BAY=88, GG_FILM_BRIDGE_BAYS=8,
    GG_FILM_BRIDGE_END=GG_FILM_BRIDGE_FIRST+GG_FILM_BRIDGE_BAY*GG_FILM_BRIDGE_BAYS,
    GG_FILM_BRIDGE_HALF=96, GG_FILM_FORD_HALF=113,
    GG_FILM_RAMP_SHOULDER=23, GG_FILM_RUG_TOP=5, GG_FILM_ROAD_TOP=3,
    GG_FILM_ROAD_LEFT_SHOULDER=12, GG_FILM_ROAD_RIGHT_SHOULDER=17
};
typedef struct { short z; u8 y; } GgFilmGrade;
typedef struct { short x,near,far,width; } GgFilmRugPlan;
typedef struct { short x,z,half; } GgFilmRoadPoint;
typedef struct { short x,z; } GgFilmCoastPoint;
static const GgFilmRoadPoint ggFilmRoad[]={
    {-8,350,174},{-24,625,135},{9,845,107},{50,1050,99},
    {91,1240,76},{128,1520,57},{133,2030,33}
};
static const GgFilmCoastPoint ggFilmCoast[]={
    {-1900,1110},{-1120,1050},{-665,925},{-360,810},{-125,855},
    {40,966},{240,1000},{440,981},{680,1044},{1110,1250},{1900,1220}
};
static const GgFilmRugPlan ggFilmRugs[F_SET_N]={
    [F_PORCH]={-151,465,856,137}, [F_ARCHIVE]={36,556,1201,314},
    [F_HALL]={0,397,1550,397}, [F_THRONE]={0,397,1550,275},
    [F_HEARTH]={-115,469,1170,445}, [F_COUCH]={120,469,1170,445}
};
static const GgFilmGrade ggFilmFordGrade[]={
    {436,1},{491,6},{556,6},{588,10},{663,10},{687,13},{780,13},
    {820,16},{908,16},{951,12},{1044,12},{1091,8},{1168,8},{1247,1}
};
static const GgFilmGrade ggFilmBridgeGrade[]={
    {351,1},{GG_FILM_BRIDGE_FIRST,GG_FILM_BRIDGE_TOP},
    {GG_FILM_BRIDGE_END,GG_FILM_BRIDGE_TOP},{1350,1}
};
static int ggFilmBankHeight(int x,int z){
    // Same sloping-in-plan bank edges as the two slabs in ggFilmFloor.
    int near=616-x*12/1900,far=1129-x*27/1900;
    return z<=near||z>=far?1:0;
}
static int ggFilmBeachEdge(int x){
    if(x<=ggFilmCoast[0].x)return ggFilmCoast[0].z;
    for(unsigned i=1;i<sizeof ggFilmCoast/sizeof *ggFilmCoast;i++){
        GgFilmCoastPoint a=ggFilmCoast[i-1],b=ggFilmCoast[i];
        if(x<=b.x)return a.z+(x-a.x)*(b.z-a.z)/(b.x-a.x);
    }
    return ggFilmCoast[sizeof ggFilmCoast/sizeof *ggFilmCoast-1].z;
}
static int ggFilmRoadHeight(int x,int z){
    for(unsigned i=1;i<sizeof ggFilmRoad/sizeof *ggFilmRoad;i++){
        GgFilmRoadPoint a=ggFilmRoad[i-1],b=ggFilmRoad[i];
        if(z<a.z||z>b.z)continue;
        int span=b.z-a.z,offset=z-a.z;
        int l=a.x-a.half+offset*((b.x-b.half)-(a.x-a.half))/span;
        int r=a.x+a.half+offset*((b.x+b.half)-(a.x+a.half))/span;
        if(x>=l&&x<=r)return GG_FILM_ROAD_TOP;
        int margin=x<l?GG_FILM_ROAD_LEFT_SHOULDER:GG_FILM_ROAD_RIGHT_SHOULDER;
        int distance=x<l?l-x:x-r;
        if(distance<margin)return GG_FILM_ROAD_TOP*(margin-distance)/margin;
        break;
    }
    return 0;
}
static int ggFilmSurfaceHeight(int place,int chapter,int x,int z){
    if(place==F_FORD){
        int bridge=chapter>=GG_FILM_BRIDGE_CHAPTER;
        int half=bridge?GG_FILM_BRIDGE_HALF:GG_FILM_FORD_HALF;
        int across=x<0?-x:x;
        if(across<=half+GG_FILM_RAMP_SHOULDER){
            const GgFilmGrade *course=bridge?ggFilmBridgeGrade:ggFilmFordGrade;
            int spans=bridge?sizeof ggFilmBridgeGrade/sizeof *course:
                             sizeof ggFilmFordGrade/sizeof *course;
            for(int i=1;i<spans;i++){
                GgFilmGrade a=course[i-1],b=course[i];
                if(z<a.z||z>b.z)continue;
                int y=a.y+(z-a.z)*(b.y-a.y)/(b.z-a.z);
                if(across<=half)return y;
                if(!bridge||i!=2)
                    return 1+(y-1)*(half+GG_FILM_RAMP_SHOULDER-across)/GG_FILM_RAMP_SHOULDER;
                break;
            }
        }
        return ggFilmBankHeight(x,z);
    }
    if(place==F_SHORE||place==F_STORM)return z<=ggFilmBeachEdge(x)?3:0;
    if(place==F_VILLAGE||place==F_ORBIT)return ggFilmRoadHeight(x,z);
    if((place==F_GATE||place==F_RUINS)&&x>=-139&&x<=139&&z>=428&&z<=1570)return 2;
    if(ggFilmRoom(place)){
        const GgFilmRugPlan *rug=&ggFilmRugs[place];
        if(rug->width&&z>=rug->near&&z<=rug->far&&
           x>=rug->x-rug->width/2&&x<=rug->x+rug->width/2)
            return GG_FILM_RUG_TOP;
        return 1;
    }
    return 0;
}
static void ggFilmCourseTop(GgFilmCamera cam,int left,int right,GgFilmGrade a,
                            GgFilmGrade b,u8 ink){
    ggFilmQuad(cam,ggV3(left,a.y,a.z),ggV3(right,a.y,a.z),
                  ggV3(right,b.y,b.z),ggV3(left,b.y,b.z),ink);
}
static void ggFilmStoneCourse(GgFilmCamera cam,GgFilmGrade a,GgFilmGrade b,
                              int half,int offset,u8 surface){
    // A shoulder runs down into the river bed. The upper facets tile one
    // shared walking surface, including the gentle grades between treads.
    ggFilmQuad(cam,ggV3(-half,1,a.z),ggV3(half,1,a.z),
                  ggV3(half,a.y,a.z),ggV3(-half,a.y,a.z),21);
    int shoulder=half+GG_FILM_RAMP_SHOULDER;
    ggFilmQuad(cam,ggV3(-shoulder,1,a.z),ggV3(-half,a.y,a.z),
                  ggV3(-half,b.y,b.z),ggV3(-shoulder,1,b.z),GG_STONE);
    ggFilmQuad(cam,ggV3(half,a.y,a.z),ggV3(shoulder,1,a.z),
                  ggV3(shoulder,1,b.z),ggV3(half,b.y,b.z),GG_SLATE);
    ggFilmCourseTop(cam,-half,half,a,b,21);
    int cut1=-half/2+offset,cut2=half/3+offset;
    ggFilmCourseTop(cam,-half+3,cut1-3,a,b,surface);
    ggFilmCourseTop(cam,cut1+3,cut2-3,a,b,GG_STONE);
    ggFilmCourseTop(cam,cut2+3,half-3,a,b,surface);
}
static void ggFilmSteppingStones(GgFilmCamera cam){
    int last=sizeof ggFilmFordGrade/sizeof *ggFilmFordGrade-1;
    for(int i=last;i>0;i--)
        ggFilmStoneCourse(cam,ggFilmFordGrade[i-1],ggFilmFordGrade[i],
                          GG_FILM_FORD_HALF,(i%3-1)*15,i&1?GG_STONE:GG_LIMESTONE);
}
#endif
