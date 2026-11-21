// New Nintendo 3DS-only native-geometry target regression.
// This exercises the compile-time release profile WITHOUT Nintendo SDK.
// The adaptive clock may reduce scenery/material cost, not resolution,
// world projection, depth, or the camera's field of view.
#define GG_NEW3DS_ONLY 1
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds/mesh_raster3d.h"
static unsigned char pixels[400*240*3];
static void require(int ok,const char *why){
    if(!ok){fprintf(stderr,"NEW3DS WORLD FAIL: %s\n",why);exit(1);}
}
int main(void){
    GgMeshContext *m=&ggMeshContext;
    GgMeshVertex a=ggMeshV(512,-230,0),b=ggMeshV(512,230,0),
                 c=ggMeshV(512,230,23),d=ggMeshV(512,-230,23);
    ggMeshBegin(m,pixels,2,0,0,12,4096,0,44,64,83);
    m->fogMax=0;
    require(m->w==400&&m->h==240&&m->scale==1&&
            m->detailQuality==2&&m->vertical==7,
            "high detail does not use New-series native optics");
    GgMeshProjected high=ggMeshProject(m,ggMeshCameraPoint(m,ggMeshV(768,0,12)));
    m->material=GG_MAT_STONE;
    ggMeshQuad(m,a,b,c,d,160,142,122);
    require(m->triangles>=2&&m->fragments>50&&m->textureSamples>50,
            "native detail did not rasterize world-space material");
    int center=(116*400+200)*3;
    unsigned char first[3]={pixels[center],pixels[center+1],pixels[center+2]};
    ggMeshBegin(m,pixels,1,0,0,12,4096,0,44,64,83);
    m->fogMax=0;
    require(m->w==400&&m->h==240&&m->scale==1&&
            m->detailQuality==1&&m->vertical==7,
            "adaptive detail incorrectly downscales New-series framebuffer");
    GgMeshProjected low=ggMeshProject(m,ggMeshCameraPoint(m,ggMeshV(768,0,12)));
    require(high.x==low.x&&high.y==low.y&&high.iz==low.iz,
            "adaptive detail makes the camera snap, zoom or change perspective");
    m->material=GG_MAT_STONE;
    ggMeshQuad(m,a,b,c,d,160,142,122);
    require(m->textureSamples==0&&m->fragments>50,
            "degraded detail did not trade expensive textures for native pixels");
    int light=ggMeshSunlight(a,b,c,m->ambientLight);
    require(pixels[center]==160*light/256&&
            pixels[center+1]==142*light/256&&
            pixels[center+2]==122*light/256,
            "reduced detail lost the same directional wall lighting");
    require(memcmp(first,pixels+center,3)!=0,
            "native high detail was indistinguishable from untextured fallback");
    require(sizeof ggMeshDepthStorage==192000,
            "depth buffer grew despite New-3DS-only profile");
    puts("NEW3DS WORLD OK: native 400x240 both quality modes, stable lens, texture-detail fallback and 187.5 KiB depth");
    return 0;
}
