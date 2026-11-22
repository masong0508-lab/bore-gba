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
        // Broad low-contrast growth bands cross tile boundaries. Tiny
        // clustered blades sit over that coherent colour, so distant
        // meadows read as land instead of random square paving slabs.
        int wave=(u+v/3+(int)(seed&63u))&127;
        wave=wave<64?wave:127-wave;
        shade=(wave-32)/7+grain/3;
        int tuftU=(u+(int)((block>>12)&7u))&15;
        int tuftV=(v+(int)((block>>17)&7u))&15;
        if(tuftU<2&&tuftV<5){rr-=4;gg+=8;bb-=3;}
        if(tuftU==2&&tuftV==1){rr+=7;gg+=12;bb+=2;}
        break;
    }
    case GG_MAT_STONE:{
        int row=(v>=0?v:v-21)/22;
        int stagger=(row&1)?23:0;
        int uu=u+stagger;
        int mortar=(v%22+22)%22<2||
                   (uu%49+49)%49<2;
        shade=mortar?-24:grain/3;
        if(!mortar&&((block>>13)&15u)<2u){
            rr-=6;gg-=7;bb-=5; // softly weathered masonry, not black pockmarks
        }
        break;
    }
    case GG_MAT_WOOD:{
        int plank=(u%29+29)%29;
        shade=plank<2?-24:grain/3;
        if(!((v*7+u*3+(int)(seed&15u))&31))
            shade-=8; // longitudinal fibres
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
        shade=row<2?-22:edge<2?-12:grain/2;
        if(row<4){rr+=6;gg+=3;}
        break;
    }
    case GG_MAT_SAND:{
        int dune=(u+v/3)&63;
        dune=dune<32?dune:63-dune;
        shade=(dune-16)/4+grain/3;
        if(((u+v/4)&31)<2)shade+=5;
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
        shade=strata<3?-12:grain/2;
        if(((block>>11)&7u)==0u)shade-=6;
        break;
    }
    case GG_MAT_WATER:{
        int bend=(v>>3)&15;
        bend=bend<8?bend:15-bend;
        int ripple=(u+v/4+bend+(int)(seed&15u))&63;
        shade=ripple<3?12:grain/4;
        if(ripple<2&&((v>>4)&3)!=0){gg+=8;bb+=12;}
        break;
    }
    case GG_MAT_LEAVES:{
        int pocket=(int)((block>>10)&7u);
        shade=pocket==0?-13:pocket>=6?9:grain/3;
        if(((u+v*3+(int)(seed&7u))&31)<3)
            {rr+=5;gg+=9;bb+=2;}
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
