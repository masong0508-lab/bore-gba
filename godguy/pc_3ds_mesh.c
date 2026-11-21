// GodGuy real 3D mesh regression, C11 with no platform SDK.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds/mesh_raster3d.h"
static struct {unsigned char before[32],rgb[400*240*3],after[32];}image;
static void verify(int ok,const char *name){
    if(!ok){fprintf(stderr,"MESH3D FAIL: %s\n",name);exit(1);}
}
static unsigned char *pixel(int x,int y){
    return image.rgb+(y*400+x)*3;
}
int main(void){
    memset(image.before,0x5a,32);
    memset(image.after,0x6c,32);
    GgMeshContext *m=&ggMeshContext;
    ggMeshBegin(m,image.rgb,2,0,0,10,4096,0,36,59,89);
    GgMeshVertex groundA=ggMeshV(0,0,0),
                 groundB=ggMeshV(256,0,0),
                 groundC=ggMeshV(0,256,0);
    GgMeshVertex wallB=ggMeshV(256,0,0),
                 wallC=ggMeshV(256,0,20);
    int sunlit=ggMeshSunlight(groundA,groundB,groundC,212);
    int sided=ggMeshSunlight(groundA,wallB,wallC,212);
    verify(sunlit>sided&&sunlit<=285&&sided>=124,
           "flat polygon shader has no physically directional sunlight");
    verify(sizeof ggMeshDepthStorage==192000,
           "fixed depth-only image exceeded 187.5 KiB");
    verify(m->w==400&&m->h==240&&m->scale==1,
           "native 3DS film viewport incorrectly scaled");
    GgMeshProjected p=ggMeshProject(m,ggMeshCameraPoint(m,ggMeshV(512,0,10)));
    verify(p.x==200&&p.y==116,"3D camera optical center drift");
    GgMeshVertex far[4]={ggMeshV(1024,-300,0),ggMeshV(1024,300,0),
                          ggMeshV(1024,300,21),ggMeshV(1024,-300,21)};
    GgMeshVertex near[4]={ggMeshV(512,-240,0),ggMeshV(512,240,0),
                           ggMeshV(512,240,21),ggMeshV(512,-240,21)};
    ggMeshQuad(m,far[0],far[1],far[2],far[3],52,168,83);
    ggMeshQuad(m,near[0],near[1],near[2],near[3],188,69,51);
    verify(pixel(200,116)[0]==188&&pixel(200,116)[1]==69,
           "near physical wall did not occlude farther wall");
    // Actual world material is sampled in the raster; this test
    // exercises the same near-plane clipping, depth and pixel writes.
    ggMeshBegin(m,image.rgb,2,0,0,10,4096,0,36,59,89);
    m->material=GG_MAT_STONE;m->materialSeed=24;
    ggMeshQuad(m,near[0],near[1],near[2],near[3],167,159,147);
    verify(m->textureSamples>140,
           "world material sampler never reached z-tested mesh pixels");
    int stoneA=pixel(197,105)[0],stoneB=pixel(204,116)[0];
    verify(stoneA!=stoneB,
           "stone facade remains a solid flat RGB billboard");
    m->material=GG_MAT_NONE;
    ggMeshBegin(m,image.rgb,2,0,0,10,4096,0,36,59,89);
    ggMeshQuad(m,far[0],far[1],far[2],far[3],52,168,83);
    ggMeshQuad(m,near[0],near[1],near[2],near[3],188,69,51);
    verify(ggMeshHidden(m,200,116,1024)&&
           !ggMeshHidden(m,200,116,256),
           "world depth does not mask correctly projected performers");
    ggMeshBegin(m,image.rgb,2,0,0,10,4096,0,36,59,89);
    ggMeshQuad(m,near[0],near[1],near[2],near[3],188,69,51);
    ggMeshQuad(m,far[0],far[1],far[2],far[3],52,168,83);
    verify(pixel(200,116)[0]==188&&pixel(200,116)[1]==69,
           "depth test depends on polygon submission order");
    // Near plane clips geometry crossing the camera: it must never
    // generate an out-of-screen write or reject the whole triangle.
    ggMeshTriangle(m,ggMeshV(-150,-120,12),
                   ggMeshV(480,-120,12),ggMeshV(480,120,12),
                   209,205,92);
    verify(m->triangles>=5&&m->fragments>500,
           "real polygon clipping/rasterizer emitted no live geometry");
    for(int i=0;i<32;i++)
        verify(image.before[i]==0x5a&&image.after[i]==0x6c,
               "raster output escaped 400x240 RGB framebuffer");
    ggMeshBegin(m,image.rgb,2,0,0,10,4096,0,36,59,89);
    ggMeshQuad(m,far[0],far[1],far[2],far[3],120,120,120);
    m->opacity=128;
    ggMeshQuad(m,near[0],near[1],near[2],near[3],28,28,28);
    verify(pixel(200,116)[0]>28&&pixel(200,116)[0]<120&&
           m->blendedFragments>40,
           "transparent terrain contact shadow is not compositing");
    ggMeshBegin(m,image.rgb,1,0,0,10,4096,0,36,59,89);
    verify(m->w==200&&m->h==120&&m->scale==2,
           "Old 3DS low-resolution geometry mode changed aspect");
    ggMeshQuad(m,near[0],near[1],near[2],near[3],188,69,51);
    verify(pixel(200,116)[0]==188&&
           !memcmp(pixel(200,116),pixel(201,116),3)&&
           !memcmp(pixel(200,116),pixel(200,117),3),
           "2x geometry output is not pixel-coherent");
    verify(!ggMeshHidden(m,401,100,300)&&
           !ggMeshHidden(m,200,240,300),
           "world-depth mask reads outside viewport");
    puts("3DS MESH OK: perspective, near-plane clipping, stable z depth, stereo-ready RGB and 2x low-cost output");
    return 0;
}
