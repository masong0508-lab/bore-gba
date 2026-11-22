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
    GG_MAT_TRACK,GG_MAT_LOAM,GG_MAT_MARSH,GG_MAT_LAVA,GG_MAT_BARK,
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
static inline void ggMatSampleLit(int mat,int u,int v,uint32_t seed,int detail,
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
        int bend=ggMatWave(v-u/4,127);
        int growth=ggMatWave(u+v/2+bend,255)+bend-96;
        rr+=growth/6;gg+=growth/8;bb-=growth/12;
        shade=grain/3;
        int dv=(v&15)-(int)((block>>17)&7u)-3;
        int du=(u&15)-(int)((block>>12)&7u)-3+dv/3;
        if(dv>=0&&dv<6&&du>=0&&du<3){
            rr-=8;gg+=9;bb-=7;
            if(du==0){rr+=14;gg+=10;bb+=5;}
        }
        break;
    }
    case GG_MAT_STONE:{
        // Wide, staggered masonry with recessed joints and a worn lit
        // bevel. A stone's tint is shared through its whole face.
        int course=v>>4;
        int uu=u+((course&1)?17:0);
        int bed=v&15;
        int joint=uu&31;
        int stone=(int)(((uint32_t)(uu>>5)*13u^
                         (uint32_t)course*23u^seed)&15u)-7;
        if(bed==0||joint==0||(bed<=1&&joint==1)){
            shade=-28;
            bb+=4;
        }else{
            shade=stone+grain/4;
            if(bed>=14||joint==31){rr+=16;gg+=12;bb+=6;}
            else if(bed==1||joint<=2)shade-=10;
            if(((block>>11)&7u)==0u){rr-=6;gg-=2;bb-=7;}
        }
        break;
    }
    case GG_MAT_WOOD:{
        int uu=u+(int)(seed&7u);
        int board=uu&15;
        int split=(int)(((uint32_t)(uu>>4)*17u+seed)&15u)-7;
        int grainLine=(board+ggMatWave(v+(int)(seed&31u),63)/7)&7;
        shade=board==0?-24:board==1?10:split+grain/3;
        if(grainLine==0)shade-=8;
        int knotU=board-8,knotV=(v+(int)(seed&31u))&63;
        if(knotV>=22&&knotV<=32&&knotU*knotU*2+(knotV-27)*(knotV-27)<24)
            shade-=18+(knotU==0?5:0);
        rr+=6;bb-=4;
        break;
    }
    case GG_MAT_BARK:{
        // Vertical fissures split and taper instead of repeating
        // the straight joints of sawn boards.
        int bend=ggMatWave(v+(int)(seed&31u),63)/6;
        int furrow=(u+bend+(int)(seed&7u))&15;
        int scar=(u-v/4+(int)((block>>13)&15u))&31;
        shade=furrow<2?-23:furrow<=4?12:grain/2;
        if(scar<2){rr-=8;gg-=6;bb-=4;}
        if(((block>>16)&7u)==0u){rr-=3;gg+=10;bb-=3;}
        break;
    }
    case GG_MAT_ROOF:{
        int row=v&15;
        int uu=u+((v&16)?9:0);
        int tile=uu&15;
        int crown=ggMatWave(uu,15);
        int kiln=(int)(((uint32_t)(uu>>4)*21u^
                        (uint32_t)(v>>4)*11u^seed)&15u)-7;
        shade=row<=1?-24:tile==0?-13:kiln+grain/4+crown;
        if(row>=13){rr+=16;gg+=11;bb+=6;}
        if(tile==1&&row>1)shade-=7;
        if(((block>>12)&15u)==0u){rr-=1;gg+=4;bb-=5;}
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
        // Broken mineral beds follow the same stone through each face.
        int bed=(v+u/4+ggMatWave(v/2+u/2,127)/3)&63;
        int facet=(u+v/2+(int)((block>>13)&7u))&31;
        shade=(bed-32)/6+grain/3;
        if(bed<4&&facet<21){shade-=16;bb+=4;}
        if(bed>=59&&facet<23){rr+=14;gg+=12;bb+=8;}
        if((block&48u)==0u&&facet>10){rr+=6;gg+=9;bb-=7;}
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
        // Lit clusters sit above cooler gaps in the canopy. Keep the
        // pattern broader than individual leaves at 400x240.
        int du=(u&15)-(int)((block>>12)&7u)-4;
        int dv=(v&15)-(int)((block>>16)&7u)-4;
        int cluster=du*du+(dv+du/2)*(dv+du/2);
        shade=grain/3;
        if(cluster<26){
            rr+=11;gg+=15;bb-=2;
            if(dv<0&&cluster<17){rr+=10;gg+=8;bb+=5;}
        }else if(cluster>60){
            rr-=10;gg-=9;bb+=2;
        }
        break;
    }
    case GG_MAT_FABRIC:
        shade=((u+v)&7)<2?-7:grain/3;
        break;
    case GG_MAT_TRACK:{
        int worn=ggMatWave(u+v/3+ggMatWave(v-u/2,127)/4,127);
        int du=(u&15)-(int)((block>>15)&7u)-3;
        int dv=(v&15)-(int)((block>>19)&7u)-3;
        shade=(worn-32)/5+grain/3;
        if(worn<13){rr-=10;gg-=8;bb-=3;}
        if((block&48u)==16u&&du*du+dv*dv*2<15){
            shade+=dv<1?20:-12;
            bb+=3;
        }
        break;
    }
    case GG_MAT_LOAM:{
        int patch=ggMatWave(u-v/2,127)+ggMatWave(v+u/3,63);
        rr+=(46-patch)/4;gg+=(patch-46)/4;
        shade=grain/3;
        int du=(u&15)-(int)((block>>12)&7u)-3;
        int dv=(v&15)-(int)((block>>16)&7u)-3;
        if((block&16u)!=0u&&dv*dv+(du+dv)*(du+dv)<14){
            rr+=dv<0?26:15;gg+=dv<0?16:8;bb+=5;
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
    // The input already includes polygon illumination and haze. Let
    // only the lit, visible share of pigment and relief modify it.
    *r=ggMatClamp(*r+(rr+shade-*r)*detail/256);
    *g=ggMatClamp(*g+(gg+shade-*g)*detail/256);
    *b=ggMatClamp(*b+(bb+shade-*b)*detail/256);
}
static inline void ggMatSample(int mat,int u,int v,uint32_t seed,
                                int *r,int *g,int *b){
    ggMatSampleLit(mat,u,v,seed,256,r,g,b);
}
#endif
