// GodGuy 3DS: authored procedural world materials, no external atlas.
// These are 3D-surface coordinates, not screen pixels: a camera turn
// cannot slide mortar, roof courses, timber grain or alpine strata.
// The integer sampler is deterministic and bounded for Old/New 3DS.
#ifndef GG_MESH_MATERIAL3D_H
#define GG_MESH_MATERIAL3D_H
#include <stdint.h>
enum {
    GG_MAT_NONE,GG_MAT_MEADOW,GG_MAT_STONE,GG_MAT_WOOD,
    GG_MAT_ROOF,GG_MAT_SAND,GG_MAT_SNOW,GG_MAT_ROCK,
    GG_MAT_WATER,GG_MAT_LEAVES,GG_MAT_FABRIC
};
static inline uint32_t ggMatHash(int u,int v,uint32_t seed){
    uint32_t k=(uint32_t)u*0x9e3779b9u+
               (uint32_t)v*0x85ebca6bu+seed;
    k^=k>>16;k*=0x7feb352du;k^=k>>15;
    return k^(k>>16);
}
static inline int ggMatClamp(int v){
    return v<0?0:v>255?255:v;
}
static inline void ggMatSample(int mat,int u,int v,uint32_t seed,
                                int *r,int *g,int *b){
    if(mat<=GG_MAT_NONE||mat>GG_MAT_FABRIC)return;
    // Small low-frequency forms before high-frequency detail: avoid
    // turning the ground into television static at native 400x240.
    uint32_t block=ggMatHash(u>>4,v>>4,seed);
    int grain=(int)(block&15u)-7;
    int rr=*r,gg=*g,bb=*b;
    int shade=0;
    switch(mat){
    case GG_MAT_MEADOW:{
        int patch=(int)((block>>8)&7u);
        shade=patch<2?-14:patch>5?8:grain/2;
        // Individual blades and wildseed highlights, anchored to land.
        if(((u*5+v*13+(int)(seed&31u))&31)<2 &&
           ((v+(int)(block&7u))&15)<8){
            rr+=9;gg+=21;bb+=4;
        }
        break;
    }
    case GG_MAT_STONE:{
        int row=(v>=0?v:v-21)/22;
        int stagger=(row&1)?23:0;
        int uu=u+stagger;
        int mortar=(v%22+22)%22<2||
                   (uu%49+49)%49<2;
        shade=mortar?-38:grain/2;
        if(!mortar&&((block>>13)&15u)<2u){
            rr-=10;gg-=11;bb-=9; // age, veins and stone discoloration
        }
        break;
    }
    case GG_MAT_WOOD:{
        int plank=(u%29+29)%29;
        shade=plank<2?-42:grain/2;
        if(!((v*7+u*3+(int)(seed&15u))&31))
            shade-=16; // longitudinal fibres
        int knotU=plank-13;
        int knotV=(v%41+41)%41-21;
        if(knotU*knotU*2+knotV*knotV<28)
            shade-=15;
        rr+=4;bb-=3;
        break;
    }
    case GG_MAT_ROOF:{
        int row=(v%19+19)%19;
        int course=(v/19)&1;
        int edge=(u+(course?13:0))%31;
        shade=row<2?-32:edge<2?-17:grain;
        if(row<4){rr+=6;gg+=3;}
        break;
    }
    case GG_MAT_SAND:{
        shade=grain/2;
        if(((u+v/4)&31)<2)shade+=8;
        rr+=4;gg+=2;bb-=3;
        break;
    }
    case GG_MAT_SNOW:{
        shade=grain/3;
        if(((u*3-v*5+(int)(block>>19))&63)<2)
            {rr+=11;gg+=12;bb+=16;}
        break;
    }
    case GG_MAT_ROCK:{
        int strata=(v+u/7)&31;
        shade=strata<3?-25:grain;
        if(((block>>11)&7u)==0u)shade-=13;
        break;
    }
    case GG_MAT_WATER:{
        int ripple=(u+v/3+(int)(seed&15u))&63;
        shade=ripple<4?18:grain/4;
        if(ripple<3){gg+=8;bb+=19;}
        break;
    }
    case GG_MAT_LEAVES:{
        int pocket=(int)((block>>10)&7u);
        shade=pocket==0?-25:pocket>=6?13:grain/2;
        if(((u+v*3+(int)(seed&7u))&31)<4)
            gg+=10;
        break;
    }
    case GG_MAT_FABRIC:
        shade=((u+v)&7)<2?-7:grain/3;
        break;
    }
    *r=ggMatClamp(rr+shade);
    *g=ggMatClamp(gg+shade);
    *b=ggMatClamp(bb+shade);
}
#endif
