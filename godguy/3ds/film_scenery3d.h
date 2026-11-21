// GodGuy film scenery: compact, live-world biome and settlement grammar.
// No texture atlas, cached prop list, world mutation, heap or extra screen.
// Props are sampled only from the bounded visible neighborhood and use
// the SAME Q8 world camera and existing RGB/foreground depth renderer.
#ifndef GG_FILM_SCENERY3D_H
#define GG_FILM_SCENERY3D_H
#include <stdint.h>
enum {
    GG_SCENE_NONE,GG_SCENE_GRASS,GG_SCENE_SHRUB,GG_SCENE_OAK,
    GG_SCENE_PINE,GG_SCENE_STONE,GG_SCENE_LAMP,GG_SCENE_BANNER,
    GG_SCENE_CRATES,GG_SCENE_FENCE,GG_SCENE_CROPS,GG_SCENE_BRAZIER,
    GG_SCENE_COLUMN,GG_SCENE_REEDS,GG_SCENE_CART,GG_SCENE_STALL
};
enum {GG_SCENE_RADIUS=10,GG_SCENE_LIMIT=84};
// 24 bytes x 84 = 2,016 bytes, reused within this rendered film frame.
// No dynamic allocation, additional framebuffer or repeated map scan.
typedef struct {
    int x,foot,height,depth,kind;
    uint32_t key;
} GgFilmScenicItem;
static inline void ggFilmScenicSort(GgFilmScenicItem *items,int count){
    // Far-to-near insertion sort for small bounded painter queue.
    // Stable on equal distances, so camera motion cannot reshuffle
    // equal-plane scenery or create frame-to-frame flicker.
    for(int i=1;i<count;i++){
        GgFilmScenicItem item=items[i];
        int j=i;
        while(j>0&&items[j-1].depth<item.depth){
            items[j]=items[j-1];
            j--;
        }
        items[j]=item;
    }
}
// Faceted instead of rectangular. Nine fixed crown bands are a compact
// silhouette profile shared by every species; a 2-bit world hash breaks
// symmetry without introducing a texture, float or dynamic allocation.
static inline int ggFilmScenicCrownHalf(int radius,int band,uint32_t seed){
    static const unsigned char profile[9]={3,6,8,9,10,9,8,6,3};
    if(radius<1)radius=1;
    if(band<0)band=0;
    if(band>8)band=8;
    int half=radius*profile[band]/10;
    if(radius>=5){
        int jag=(int)((seed>>(band*2))&3u);
        if(jag==0)half--;
        if(jag==3)half++;
    }
    if(half<1)half=1;
    if(half>radius)half=radius;
    return half;
}
static inline uint32_t ggFilmScenicHash(int x,int y,int salt){
    uint32_t h=(uint32_t)x*0x9e3779b9u+
               (uint32_t)y*0x85ebca6bu+(uint32_t)salt*0xc2b2ae35u;
    h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;
    return h^(h>>16);
}
// Wind is derived from the world seed and film time. Tree trunks and
// boot/road anchors remain immobile; only upper foliage sways by one
// native pixel. Replay/seek produces identical camera and foliage.
static inline int ggFilmScenicWind(uint32_t key,int frame){
    int phase=((frame<0?0:frame)/14+(int)(key>>23))&31;
    return phase<8?0:phase<16?1:phase<24?0:-1;
}
static inline int ggFilmScenicKind(int terrain,int nearby,uint32_t key){
    int pick=(int)(key&15u);
    // Specific settlements outrank biome scatter: farms have crop rows
    // and fences, temples have ceremony, towns contain human activity.
    if(nearby==B_FARM)
        return pick<7?GG_SCENE_CROPS:pick<12?GG_SCENE_FENCE:
               pick<14?GG_SCENE_CART:GG_SCENE_NONE;
    if(nearby==B_TEMPLE||nearby==B_STATUE)
        return pick<5?GG_SCENE_BRAZIER:pick<9?GG_SCENE_BANNER:
               pick<13?GG_SCENE_COLUMN:GG_SCENE_NONE;
    if(nearby==B_RUIN)
        return pick<8?GG_SCENE_COLUMN:pick<13?GG_SCENE_STONE:
               GG_SCENE_SHRUB;
    if(nearby==B_DOCK)
        return pick<6?GG_SCENE_REEDS:pick<10?GG_SCENE_CRATES:
               pick<13?GG_SCENE_LAMP:GG_SCENE_NONE;
    if(nearby==B_HUT||nearby==B_HOUSE||nearby==B_HALL||
       nearby==B_LIBRARY||nearby==B_TOWER)
        return pick<5?GG_SCENE_LAMP:pick<9?GG_SCENE_CRATES:
               pick<12?GG_SCENE_STALL:pick<14?GG_SCENE_BANNER:
               GG_SCENE_NONE;
    switch(terrain){
    case T_FOREST:case T_JUNGLE:
        return pick<9?GG_SCENE_OAK:pick<14?GG_SCENE_SHRUB:
               GG_SCENE_GRASS;
    case T_SNOW:case T_MOUNT:
        return pick<6?GG_SCENE_PINE:pick<12?GG_SCENE_STONE:
               pick<14?GG_SCENE_SHRUB:GG_SCENE_NONE;
    case T_WATER:case T_DEEP:case T_SWAMP:
        return pick<9?GG_SCENE_REEDS:pick<12?GG_SCENE_STONE:
               GG_SCENE_NONE;
    case T_SAND:case T_DESERT:
        return pick<7?GG_SCENE_STONE:pick<12?GG_SCENE_SHRUB:
               GG_SCENE_NONE;
    case T_DIRT:
        return pick<5?GG_SCENE_GRASS:pick<9?GG_SCENE_STONE:
               pick<12?GG_SCENE_FENCE:GG_SCENE_NONE;
    case T_HILL:
        return pick<7?GG_SCENE_SHRUB:pick<11?GG_SCENE_STONE:
               pick<14?GG_SCENE_OAK:GG_SCENE_NONE;
    case T_GRASS:
        return pick<6?GG_SCENE_GRASS:pick<11?GG_SCENE_SHRUB:
               pick<13?GG_SCENE_OAK:GG_SCENE_NONE;
    default:return GG_SCENE_NONE;
    }
}
// Thumbnail silhouettes remain recognizable even when the film lens
// compresses a distant object to 3–8 pixels. Previously every distant
// non-plant (lamps, banners, stalls, crates) became the same brown stick.
static inline int ggFilmScenicFarClass(int kind){
    switch(kind){
    case GG_SCENE_OAK:case GG_SCENE_PINE:return 1;
    case GG_SCENE_GRASS:case GG_SCENE_SHRUB:
    case GG_SCENE_REEDS:case GG_SCENE_CROPS:return 2;
    case GG_SCENE_STONE:case GG_SCENE_COLUMN:return 3;
    case GG_SCENE_LAMP:case GG_SCENE_BRAZIER:return 4;
    case GG_SCENE_BANNER:return 5;
    case GG_SCENE_CRATES:case GG_SCENE_CART:return 6;
    case GG_SCENE_FENCE:return 7;
    case GG_SCENE_STALL:return 8;
    default:return 0;
    }
}
static inline int ggFilmScenicHeight(int kind){
    switch(kind){
    case GG_SCENE_OAK:return 19;
    case GG_SCENE_PINE:return 23;
    case GG_SCENE_LAMP:return 15;
    case GG_SCENE_BANNER:return 18;
    case GG_SCENE_STALL:return 13;
    case GG_SCENE_COLUMN:return 15;
    case GG_SCENE_BRAZIER:return 10;
    case GG_SCENE_CART:return 9;
    case GG_SCENE_FENCE:return 7;
    case GG_SCENE_CROPS:return 7;
    case GG_SCENE_REEDS:return 8;
    case GG_SCENE_SHRUB:return 8;
    case GG_SCENE_STONE:return 6;
    case GG_SCENE_CRATES:return 6;
    case GG_SCENE_GRASS:return 4;
    default:return 0;
    }
}
// Optional detail within the existing continuous floor. Patterns follow
// actual Q8 world coordinates, never screen pixels or changing RNG seeds.
// Small rectilinear wagon-ruts and ground strata can be seen while moving.
// Climate-aware lighting is sampled ONCE per camera frame and shared
// by sky, distant surfaces and fog. Fixed subtle offsets retain the
// common naturalistic palette (no fluorescent terrain, no flat blue
// atmosphere regardless of the physical world biome).
typedef struct {
    signed char skyR,skyG,skyB;
    unsigned char hazeR,hazeG,hazeB;
} GgFilmScenicAtmosphere;
// Small shared world-light scalar for film character materials. Clothing,
// eyes, skin and foliage still retain their authored hue/value hierarchy.
static inline int ggFilmScenicActorLight(int terrain){
    switch(terrain){
    case T_FOREST:case T_JUNGLE:return 226;
    case T_SWAMP:return 220;
    case T_MOUNT:return 233;
    case T_SNOW:return 254;
    case T_SAND:case T_DESERT:return 246;
    case T_WATER:case T_DEEP:return 237;
    default:return 244;
    }
}
static inline GgFilmScenicAtmosphere ggFilmScenicAtmosphere(int terrain){
    switch(terrain){
    case T_SAND:case T_DESERT:
        return (GgFilmScenicAtmosphere){12,7,-9,104,104,98};
    case T_SNOW:
        return (GgFilmScenicAtmosphere){7,11,18,104,119,136};
    case T_MOUNT:
        return (GgFilmScenicAtmosphere){2,3,11,93,104,124};
    case T_SWAMP:
        return (GgFilmScenicAtmosphere){-5,4,-10,70,96,91};
    case T_JUNGLE:
        return (GgFilmScenicAtmosphere){-3,3,-6,69,91,93};
    case T_DEEP:case T_WATER:
        return (GgFilmScenicAtmosphere){0,6,10,72,102,124};
    case T_HILL:
        return (GgFilmScenicAtmosphere){2,3,2,82,102,111};
    default:
        return (GgFilmScenicAtmosphere){0,0,0,76,95,113};
    }
}
static inline int ggFilmScenicFloorDetail(int type,int wx,int wy){
    int u=wx&255,v=wy&255;
    int grain=((wx>>4)*5+(wy>>5)*3)&15;
    int patch=((wx>>6)*11+(wy>>6)*17)&15;
    switch(type){
    case T_DIRT:
        if((u>70&&u<86)||(u>171&&u<187))return -10;
        return grain<2?7:0;
    case T_GRASS:case T_FOREST:case T_JUNGLE:
        if((u&31)<3&&(v&63)<14)return 9;
        return patch<4?-8:patch>=13?7:grain<2?4:0;
    case T_SAND:case T_DESERT:
        return (((u+(v>>2))&63)<5)?7:
               patch<4?-6:grain<2?-3:0;
    case T_SNOW:
        return grain<2?12:patch<4?-6:patch>=13?6:0;
    case T_MOUNT:case T_HILL:
        return ((u+(v>>1))&31)<4?10:
               patch<4?-9:patch>=13?6:0;
    case T_WATER:case T_DEEP:case T_SWAMP:
        return ((u+(v>>2))&63)<4?8:0;
    default:return 0;
    }
}
#endif
