// Live terrain appearance shared by the triangle and column renderers.
#ifndef GG_OUTDOOR_ART3D_H
#define GG_OUTDOOR_ART3D_H

static inline int ggMeshTerrainAt(int x,int y){
    if((unsigned)x>=WW||(unsigned)y>=WH)return T_DIRT;
    int type=world[y][x];
    return (unsigned)type<T_N?type:T_DIRT;
}

static inline int ggMeshGroundMaterial(int type){
    switch(type){
    case T_DEEP:case T_WATER:return GG_MAT_WATER;
    case T_SAND:case T_DESERT:return GG_MAT_SAND;
    case T_GRASS:case T_HILL:return GG_MAT_MEADOW;
    case T_FOREST:case T_JUNGLE:return GG_MAT_LOAM;
    case T_SNOW:return GG_MAT_SNOW;
    case T_LAVA:return GG_MAT_LAVA;
    case T_SWAMP:return GG_MAT_MARSH;
    case T_DIRT:return GG_MAT_TRACK;
    default:return GG_MAT_ROCK;
    }
}

static inline void ggOutdoorGroundColor(int type,int *r,int *g,int *b){
    static const unsigned char color[T_N][3]={
        [T_DEEP]={35,66,93}, [T_WATER]={42,108,120},
        [T_SAND]={177,151,103}, [T_GRASS]={96,128,66},
        [T_FOREST]={64,91,62}, [T_HILL]={117,131,75},
        [T_MOUNT]={105,111,117}, [T_SNOW]={192,210,219},
        [T_LAVA]={122,56,41}, [T_DESERT]={171,128,76},
        [T_SWAMP]={77,98,76}, [T_JUNGLE]={62,105,76},
        [T_DIRT]={144,105,76}
    };
    const unsigned char *c=color[(unsigned)type<T_N?type:T_DIRT];
    *r=c[0];*g=c[1];*b=c[2];
}

static inline int ggOutdoorPaved(int type,int build){
    // The well rests on the same masonry apron as its courtyard.
    return type>T_WATER&&type!=T_LAVA&&(ggBuildingIsPaving(build)||build==B_WELL);
}

static inline void ggOutdoorPavingColor(int *r,int *g,int *b){
    *r=139;*g=141;*b=140;
}

static inline uint32_t ggOutdoorGroundSeed(int type,int frame){
    if(type<=T_WATER)return (uint32_t)(frame/6);
    return (uint32_t)type*733u+17u;
}

// Sides follow the same order for reeds, shore strips and bank colors.
static const signed char ggOutdoorSideX[4]={-1,1,0,0};
static const signed char ggOutdoorSideY[4]={0,0,-1,1};
static inline unsigned ggOutdoorShoreSides(int tx,int ty,int water){
    unsigned sides=0;
    for(int side=0;side<4;side++){
        int x=tx+ggOutdoorSideX[side],y=ty+ggOutdoorSideY[side];
        if((unsigned)x>=WW||(unsigned)y>=WH)continue;
        int t=ggMeshTerrainAt(x,y);
        if((t<=T_WATER)!=water&&t!=T_LAVA)sides|=1u<<side;
    }
    return sides;
}

static inline int ggOutdoorPropKind(int tx,int ty,int type,uint32_t key){
    int nearby=0;
    for(int side=0;side<4&&!nearby;side++){
        int x=tx+ggOutdoorSideX[side],y=ty+ggOutdoorSideY[side];
        if((unsigned)x<WW&&(unsigned)y<WH&&!ggBuildingIsPaving(bld[y][x]))
            nearby=bld[y][x];
    }
    int pick=(int)(key&15u);
    if(type==T_DEEP||type==T_LAVA)return GG_SCENE_NONE;
    if(type==T_WATER)
        return pick<5&&ggOutdoorShoreSides(tx,ty,1)?
               GG_SCENE_REEDS:GG_SCENE_NONE;
    if(nearby)return ggFilmScenicKind(type,nearby,key);
    if(type==T_DIRT)return pick==0?GG_SCENE_GRASS:GG_SCENE_NONE;
    if(type==T_SWAMP)
        return pick<6?GG_SCENE_REEDS:pick<10?GG_SCENE_SHRUB:
               pick<13?GG_SCENE_OAK:GG_SCENE_NONE;
    if(type==T_MOUNT)
        return pick<8?GG_SCENE_STONE:pick<11?GG_SCENE_PINE:GG_SCENE_NONE;
    if(type==T_SNOW)
        return pick<8?GG_SCENE_PINE:pick<12?GG_SCENE_STONE:GG_SCENE_NONE;
    if(type==T_DESERT)
        return pick<6?GG_SCENE_STONE:pick<8?GG_SCENE_SHRUB:GG_SCENE_NONE;
    if(type==T_SAND)
        return pick<4?GG_SCENE_STONE:pick<7?GG_SCENE_GRASS:GG_SCENE_NONE;
    if(type==T_GRASS&&pick>=11){
        // Trees and tall shrubs gather into copses separated by open
        // meadows, using stable four-cell stands across a camera turn.
        int grove=(int)(ggFilmScenicHash(tx>>2,ty>>2,41)&15u);
        if(grove<8)return pick==15?GG_SCENE_STONE:GG_SCENE_GRASS;
        return pick<13?GG_SCENE_SHRUB:GG_SCENE_OAK;
    }
    return ggFilmScenicKind(type,0,key);
}
#endif
