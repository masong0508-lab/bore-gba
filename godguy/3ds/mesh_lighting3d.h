// Outdoor lighting for the fixed-point world rasterizer. Palette and
// weather are prepared once per frame, RGB gains once per polygon.
#ifndef GG_MESH_LIGHTING3D_H
#define GG_MESH_LIGHTING3D_H

static inline int ggMeshDaylightChannel(int ground,int sky,int up,
                                       int sun,int intensity){
    int gain=ground+(sky-ground)*up/256+sun*intensity/256;
    // Keep highlight headroom on pale plaster, snow, and stone.
    return gain<=278?gain:278+(gain-278)/2;
}

static inline GgMeshRgb ggMeshDaylightAngles(const GgMeshDaylight *light,
                                             int up,int cosine){
    int direct=ggMeshClamp((cosine+4+light->cover*5)*3,0,256)*light->sun/256;
    return (GgMeshRgb){
        ggMeshDaylightChannel(light->ground.r,light->sky.r,up,207,direct),
        ggMeshDaylightChannel(light->ground.g,light->sky.g,up,164,direct),
        ggMeshDaylightChannel(light->ground.b,light->sky.b,up,117,direct)
    };
}

static inline void ggMeshPrepareDaylight(GgMeshContext *m,int cover){
    static const int sunlight[4]={256,218,150,65};
    static const int sunAxis[3]={GG_MESH_SUN_X,GG_MESH_SUN_Y,GG_MESH_SUN_Z};
    GgMeshDaylight *light=&m->daylight;
    int red=m->skyR-49,green=m->skyG-78,blue=m->skyB-108;
    light->cover=cover;light->sun=sunlight[cover];
    light->sky=(GgMeshRgb){170+cover*12+red/2,196+cover*7+green/2,
                          238-cover+blue/2};
    light->ground=(GgMeshRgb){141+cover*5+red,127+cover*9+green/2,
                             115+cover*14+blue/2};
    // Most ground and building faces use an axis-aligned normal.
    for(int axis=0;axis<3;axis++){
        light->axis[axis][0]=ggMeshDaylightAngles(light,axis==2?0:128,-sunAxis[axis]);
        light->axis[axis][1]=ggMeshDaylightAngles(light,axis==2?256:128,sunAxis[axis]);
    }
    m->fogR=ggMeshClamp(184-cover*7+red,0,255);
    m->fogG=ggMeshClamp(192-cover*3+green,0,255);
    m->fogB=ggMeshClamp(192+cover*2+blue,0,255);
}

static inline GgMeshRgb ggMeshDaylightSlope(const GgMeshDaylight *light,
                                            GgMeshNormal n,int *cosine){
    // The established UV normals emphasize height. Use approximately
    // the camera's XY/Z ratio to light the visible plane itself.
    n.x/=GG_MESH_NORMAL_HEIGHT/GG_MESH_LIGHT_HEIGHT;
    n.y/=GG_MESH_NORMAL_HEIGHT/GG_MESH_LIGHT_HEIGHT;
    int64_t x=ggMeshNormalAbs(n.x),y=ggMeshNormalAbs(n.y),z=ggMeshNormalAbs(n.z);
    int64_t sum=x+y+z;
    if(!sum){*cosine=0;return (GgMeshRgb){256,256,256};}
    // Work in 32-bit fixed point after the wide cross product. This
    // also bounds both dot products on large near-clipped polygons.
    while(sum>INT32_MAX/128){
        n.x/=256;n.y/=256;n.z/=256;
        x=ggMeshNormalAbs(n.x);y=ggMeshNormalAbs(n.y);z=ggMeshNormalAbs(n.z);
        sum=x+y+z;
    }
    int64_t major=x>y?x:y;
    if(z>major)major=z;
    // Approximate the Euclidean length from its major and minor axes.
    int length=(int)(major+(sum-major)*3/8);
    int up=ggMeshClamp(128+(int)(n.z*128)/length,0,256);
    *cosine=(int)(n.x*GG_MESH_SUN_X+n.y*GG_MESH_SUN_Y+
                  n.z*GG_MESH_SUN_Z)/length;
    return ggMeshDaylightAngles(light,up,*cosine);
}

static inline GgMeshRgb ggMeshDaylightGain(const GgMeshContext *m,
                                           GgMeshNormal n){
    const GgMeshDaylight *light=&m->daylight;
    int cosine;
    GgMeshRgb gain;
    if(!(n.x|n.y)){
        if(!n.z)return (GgMeshRgb){256,256,256};
        gain=light->axis[2][n.z>0];
        cosine=n.z>0?GG_MESH_SUN_Z:-GG_MESH_SUN_Z;
    }else if(!(n.y|n.z)){
        gain=light->axis[0][n.x>0];
        cosine=n.x>0?GG_MESH_SUN_X:-GG_MESH_SUN_X;
    }else if(!(n.x|n.z)){
        gain=light->axis[1][n.y>0];
        cosine=n.y>0?GG_MESH_SUN_Y:-GG_MESH_SUN_Y;
    }else{
        gain=ggMeshDaylightSlope(light,n,&cosine);
    }
    switch(m->material){
    case GG_MAT_LEAVES:{
        // A small share of the sun comes through a leaf even from behind.
        int transmitted=ggMeshClamp(-cosine,0,70)*light->sun/1024;
        gain.r+=transmitted/2;gain.g+=transmitted;
        break;
    }
    case GG_MAT_LAVA:
        // Emissive cracks keep their heat in the shadow of a slope.
        if(gain.r<256)gain.r=256;
        if(gain.g<225)gain.g=225;
        break;
    default:break;
    }
    return gain;
}
#endif
