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
    GG_MAT_WATER,GG_MAT_LEAVES,GG_MAT_FABRIC,
    GG_MAT_TRACK,GG_MAT_LOAM,GG_MAT_MARSH,GG_MAT_LAVA,
    GG_MAT_COUNT
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
static inline int ggMatWave(int v,int mask){
    int phase=v&mask;
    return phase<=mask/2?phase:mask-phase;
}
static inline void ggMatSample(int mat,int u,int v,uint32_t seed,
                                int *r,int *g,int *b){
    if(mat<=GG_MAT_NONE||mat>=GG_MAT_COUNT)return;
    // Small low-frequency forms before high-frequency detail: avoid
    // turning the ground into television static at native 400x240.
    uint32_t block=ggMatHash(u>>4,v>>4,mat==GG_MAT_WATER?113u:seed);
    int grain=(int)(block&15u)-7;
    int rr=*r,gg=*g,bb=*b;
    int shade=0;
    switch(mat){
    case GG_MAT_MEADOW:{
        // Intersecting broad growth patterns keep the meadow coherent
        // across cell edges without repeating narrow diagonal stripes.
        int growth=ggMatWave(u+v/2,255)+ggMatWave(v-u/2,127)-96;
        rr+=growth/6;gg+=growth/7;bb-=growth/12;
        shade=grain/2;
        int tuftU=(u+(int)((block>>12)&7u)-(v&3)/2)&15;
        int tuftV=(v+(int)((block>>17)&7u))&15;
        if(tuftU<2&&tuftV<7){rr-=7;gg+=9;bb-=7;}
        if(tuftU==2&&tuftV<2){rr+=12;gg+=14;bb+=2;}
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
        int crest=ggMatWave(u+v*2+ggMatWave(u/3-v/5,63),63);
        shade=(crest-16)/4+grain/3;
        if(crest>27&&((block>>13)&3u)!=0)shade+=8;
        if(crest<4)shade-=4;
        rr+=6;gg+=3;bb-=4;
        break;
    }
    case GG_MAT_SNOW:{
        int drift=ggMatWave(v+u/3+ggMatWave(u/2-v/3,127),127);
        shade=(drift-32)/4+grain/3;
        if(drift<18){rr-=12;gg-=5;bb+=5;}
        if(((u*3-v*5+(int)(block>>19))&63)<2){rr+=13;gg+=14;bb+=17;}
        break;
    }
    case GG_MAT_ROCK:{
        int strata=(v+u/5+ggMatWave(u/2+v/3,63)/3)&31;
        int cracks=(u-v/3+(int)((block>>13)&7u))&31;
        shade=strata<2?-18:strata<6?8:grain/2;
        if(cracks==0&&((block>>11)&3u)!=0u)shade-=10;
        if((block&48u)==0u){rr+=6;gg+=5;bb-=5;}
        break;
    }
    case GG_MAT_WATER:{
        int phase=(int)(seed&127u);
        int bend=ggMatWave(u/3-v/3,63);
        int swell=ggMatWave(u+v/3+bend/2+phase,63);
        int chop=ggMatWave(v-u/3+bend-phase,63);
        shade=(swell-16)/3+grain/4;
        if(swell<8)bb+=5;
        if(swell>27&&chop>18){rr+=9;gg+=18;bb+=18;}
        if(swell>29&&chop>25){rr+=16;gg+=22;bb+=15;}
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
    case GG_MAT_TRACK:{
        // Packed earth, with irregular flat gravel and worn shallow ruts.
        int worn=ggMatWave(u+v/3+ggMatWave(v-u/2,127)/4,127);
        int pebbleU=(u+(int)((block>>15)&7u))&15;
        int pebbleV=(v+(int)((block>>19)&7u))&15;
        shade=(worn-32)/4+grain/2;
        if(worn<10){rr-=9;gg-=7;bb-=4;}
        if((block&48u)==16u&&pebbleU<5&&pebbleV<4)
            shade+=pebbleV<2?22:-11;
        break;
    }
    case GG_MAT_LOAM:{
        int patch=ggMatWave(u-v/2,127)+ggMatWave(v+u/3,63);
        rr+=(46-patch)/4;gg+=(patch-46)/4;
        shade=grain/2;
        if(((u+v*2+(int)(block>>20))&31)<4&&
           ((v+(int)((block>>14)&15u))&15)<7){
            rr+=24;gg+=15;bb+=5;
        }
        break;
    }
    case GG_MAT_MARSH:{
        int basin=ggMatWave(u+v/3,127)+ggMatWave(v-u/2,63);
        shade=grain/2;
        if(basin<24){rr-=13;gg-=7;bb+=18;}
        else if(basin<32){rr+=9;gg+=10;bb+=13;}
        else if(basin>58){rr+=10;gg+=15;bb-=8;}
        break;
    }
    case GG_MAT_LAVA:{
        int joint=ggMatWave(u+v/2+ggMatWave(v/2,63),63);
        int breakage=ggMatWave(v-u/3+ggMatWave(u/2,63),127);
        if(joint<3||breakage<4){rr+=60;gg+=55;bb+=20;}
        else if(joint>23&&breakage>12){rr-=29;gg-=17;bb-=6;}
        shade=grain;
        break;
    }
    }
    *r=ggMatClamp(rr+shade);
    *g=ggMatClamp(gg+shade);
    *b=ggMatClamp(bb+shade);
}
#endif
