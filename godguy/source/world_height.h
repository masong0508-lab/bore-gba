#ifndef GG_WORLD_HEIGHT_H
#define GG_WORLD_HEIGHT_H

enum { GG_WORLD_WALK_RISE=19 };

static inline int ggExpGround(int terrain){
    static const unsigned char heights[T_N]={
        0,1,5,10,16,25,48,31,6,8,7,18,8
    };
    return (unsigned)terrain<T_N?heights[terrain]:8;
}

#endif
