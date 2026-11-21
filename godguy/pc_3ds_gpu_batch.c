// Host-only GPU geometry feeder regression: no libctru or PICA SDK.
// Check the SAME packed terrain/building format supplied to GPU probe.
// gcc -O2 -std=c11 -Wall -Wextra -Werror pc_3ds_gpu_batch.c -o /tmp/ggpu
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "3ds/gpu_world_batch3d.h"
#define W 48
static uint8_t terrain[W*W],building[W*W];
static GgGpuVertex vertices[GG_GPU_MAX_VERTS+3];
static void require(int ok,const char*message){
    if(!ok){fprintf(stderr,"NEW3DS GPU BATCH FAIL: %s\n",message);exit(1);}
}
static uint32_t checksum(const GgGpuBatch*b){
    uint32_t x=2166136261u;
    for(int i=0;i<b->count;i++){
        const unsigned char*bytes=(const unsigned char*)&b->vertex[i];
        for(unsigned j=0;j<sizeof(GgGpuVertex);j++){
            x^=bytes[j];x*=16777619u;
        }
    }
    return x;
}
int main(void){
    for(int i=0;i<W*W;i++)terrain[i]=3;
    GgGpuBatch batch={0};
    batch.vertex=vertices;batch.capacity=GG_GPU_MAX_VERTS;
    require(sizeof(GgGpuVertex)==6*sizeof(float),
            "vertex layout does not match Citro3D position+RGB stride");
    require(ggGpuTerrainHeight(6)>ggGpuTerrainHeight(3)&&
            ggGpuTerrainHeight(0)==0&&
            ggGpuBuildingHeight(5)>ggGpuBuildingHeight(1),
            "GodGuy heights diverged from live-world terrain/building heights");
    ggGpuSceneBuild(&batch,terrain,building,W,W,W,
                    24*256+128,24*256+128,3);
    require(batch.tiles==49&&batch.buildings==0&&
            batch.count>=49*12&&!batch.overflow,
            "plain world cannot assemble bounded four-sector ground mesh");
    uint32_t first=checksum(&batch);
    int firstZ=batch.eye;
    ggGpuSceneBuild(&batch,terrain,building,W,W,W,
                    24*256+128,24*256+128,3);
    require(first==checksum(&batch)&&firstZ==batch.eye,
            "GPU geometry changes between identical world frames");
    building[24*W+27]=5;
    terrain[24*W+26]=6;
    ggGpuSceneBuild(&batch,terrain,building,W,W,W,
                    24*256+128,24*256+128,3);
    require(batch.buildings==1&&batch.count>49*12&&
            checksum(&batch)!=first,
            "camera-adjacent world edits do not rebuild 3D buildings/slopes");
    int foundForward=0,foundHeight=0;
    for(int i=0;i<batch.count;i++){
        const GgGpuVertex*v=&batch.vertex[i];
        if(v->z<-0.1f)foundForward=1;
        if(v->y>1.0f)foundHeight=1;
        require(v->r>=0.0f&&v->g>=0.0f&&v->b>=0.0f&&
                v->r<=1.0f&&v->g<=1.0f&&v->b<=1.0f,
                "GPU color is invalid or exceeds normalized fragment range");
    }
    require(foundForward&&foundHeight,
            "GPU camera/world transforms lost height or forward depth");
    ggGpuSceneBuild(&batch,terrain,building,W,W,W,
                    25*256+128,24*256+128,3);
    require(batch.count>0&&checksum(&batch)!=first,
            "camera translation does not rebase world-space geometry");
    batch.capacity=96;
    vertices[96]=(GgGpuVertex){99,99,99,99,99,99};
    ggGpuSceneBuild(&batch,terrain,building,W,W,W,
                    24*256+128,24*256+128,7);
    require(batch.count<=96&&batch.overflow&&
            vertices[96].x==99&&vertices[96].z==99,
            "dense settlement can overwrite the fixed PICA vertex budget");
    batch.capacity=GG_GPU_MAX_VERTS;
    ggGpuSceneBuild(&batch,terrain,building,W,W,W,
                    24*256+128,24*256+128,7);
    require(batch.tiles==225&&!batch.overflow&&
            batch.count<=GG_GPU_MAX_VERTS,
            "real New3DS city-scene budget exceeds 192 KiB GPU geometry cap");
    printf("NEW3DS GPU BATCH OK: %d grounded tiles, %d buildings, %d trees, %d vertices, deterministic PICA vertex stream and bounded memory\n",
           batch.tiles,batch.buildings,batch.trees,batch.count);
    return 0;
}
