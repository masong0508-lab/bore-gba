// GodGuy GPU feeder vs. the actual LIVE world, not only test tiles.
// Compile with the same complete host game simulation and rendering path.
// The PICA producer consumes world[][] / bld[][] without copies or saves.
#define main ggGpuHostExploreTestMain
#include "pc_explore3d.c"
#undef main
#include "3ds/gpu_world_batch3d.h"
static GgGpuVertex liveGpuVertices[GG_GPU_MAX_VERTS];
static void gpuRequire(int ok,const char*reason){
    if(!ok){fprintf(stderr,"GODGUY GPU LIVE WORLD FAIL: %s\n",reason);exit(1);}
}
static uint32_t liveHash(const GgGpuBatch*b){
    uint32_t h=2166136261u;
    const unsigned char*data=(const unsigned char*)b->vertex;
    for(size_t i=0;i<(size_t)b->count*sizeof(GgGpuVertex);i++){
        h^=data[i];h*=16777619u;
    }
    return h;
}
int main(void){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;
        occ[y][x]=0;
    }
    ggExpBegin(64,65,0);
    ggExplore.angle=0;
    GgGpuBatch batch={0};
    batch.capacity=GG_GPU_MAX_VERTS;
    batch.vertex=liveGpuVertices;
    ggGpuSceneBuild(&batch,&world[0][0],&bld[0][0],WW,WW,WH,
                    ggExplore.px,ggExplore.py,6);
    gpuRequire(!batch.overflow&&batch.tiles==169&&
               batch.buildings==0,"unmodified live world does not match GPU range");
    uint32_t original=liveHash(&batch);
    ggExplore.material=5;  // Existing in-game MOUNTAIN edit palette.
    int changed=ggExpEdit(67,65,ggExplore.material,0);
    gpuRequire(changed&&world[65][67]==T_MOUNT,
               "actual GodGuy build/edit action did not update terrain");
    ggGpuSceneBuild(&batch,&world[0][0],&bld[0][0],WW,WW,WH,
                    ggExplore.px,ggExplore.py,6);
    gpuRequire(!batch.overflow&&liveHash(&batch)!=original,
               "PICA geometry ignored live game terrain edits");
    uint32_t terrainEdit=liveHash(&batch);
    bld[65][68]=B_TOWER;
    ggGpuSceneBuild(&batch,&world[0][0],&bld[0][0],WW,WW,WH,
                    ggExplore.px,ggExplore.py,6);
    gpuRequire(batch.buildings==1&&liveHash(&batch)!=terrainEdit,
               "PICA geometry ignored actual GodGuy village buildings");
    uint32_t settlement=liveHash(&batch);
    ggExplore.px+=75; ggExplore.py+=45;
    ggGpuSceneBuild(&batch,&world[0][0],&bld[0][0],WW,WW,WH,
                    ggExplore.px,ggExplore.py,6);
    gpuRequire(batch.buildings==1&&liveHash(&batch)!=settlement,
               "live 3D camera did not rebase GPU scene");
    int eye=ggGpuGroundAtQ8(&world[0][0],WW,WW,WH,
                             ggExplore.px,ggExplore.py)+13;
    gpuRequire(batch.eye==eye,"GPU live-world height ignores slope camera");
    gpuRequire(ggExpEdit(67,65,ggExplore.material,1)&&
               world[65][67]==T_GRASS,
               "source game terrain removal unexpectedly failed");
    ggGpuSceneBuild(&batch,&world[0][0],&bld[0][0],WW,WW,WH,
                    ggExplore.px,ggExplore.py,6);
    gpuRequire(!batch.overflow&&batch.count>0,
               "terrain undo invalidated GPU geometry");
    printf("GODGUY GPU LIVE WORLD OK: %d actual simulation tiles, %d real buildings, %d PICA vertices; terrain edit/undo, slopes and camera rebasing\n",
           batch.tiles,batch.buildings,batch.count);
    return 0;
}
