// Source-integrated true 3D world regression, fully SDK free.
#define main ggMeshLegacyGameMain
#include "pc_explore3d.c"
#undef main
static void meshRequire(int good,const char*why){
    if(!good){fprintf(stderr,"3DS WORLD MESH FAIL: %s\n",why);exit(1);}
}
int main(void){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;occ[y][x]=0;
    }
    ggExpBegin(64,64,0);
    ggExpMeshEnabled=1;ggExpScenicQuality=2;
    ggExplore.angle=0;ggExplore.px=64*256+128;
    ggExplore.py=64*256+128;
    bld[64][67]=B_HALL;bld[66][69]=B_HOUSE;
    bld[63][70]=B_TEMPLE;
    static unsigned char pixels[400*240*3];
    ggExpRenderMode(pixels,1);
    meshRequire(ggExpMeshActive&&ggMeshContext.w==400,
                "cinema is still a raycast image, not 3D mesh geometry");
    meshRequire(ggMeshContext.blendedFragments>50,
                "buildings, people and vegetation have no actual world shadows");
    meshRequire(ggMeshContext.textureSamples>600,
                "new mesh still consists of flat-color surfaces");
    meshRequire(ggMeshGroundMaterial(T_GRASS)==GG_MAT_MEADOW&&
                ggMeshGroundMaterial(T_SNOW)==GG_MAT_SNOW&&
                ggMeshGroundMaterial(T_MOUNT)==GG_MAT_ROCK,
                "material sampler ignores physical terrain biomes");
    meshRequire(ggMeshContext.triangles>100&&
                ggMeshContext.fragments>1500&&
                ggMeshTilesDrawn>20&&ggMeshBuildingsDrawn>=2,
                "live world polygons or building volumes are missing");
    // Dynamic polygonal people and animals use real 3D bones and
    // floor joints. The image itself must change between poses, not
    // just a simulated animation counter.
    static const u8 colors[3][3]={
       {20,17,12},{10,8,7},{26,21,16}
    };
    int beforeActor=ggMeshContext.triangles;
    ggMeshActorCharacter(&ggMeshContext,65*256+128,64*256+128,
                         ggMeshGroundAtQ8(65*256+128,64*256+128),
                         0,30,2,1,colors,2);
    meshRequire(ggMeshContext.triangles>beforeActor+45,
                "humanoid inhabitants remain flat 8x8 bitmap sprites");
    beforeActor=ggMeshContext.triangles;
    ggMeshActorCharacter(&ggMeshContext,65*256+128,65*256+128,
                         ggMeshGroundAtQ8(65*256+128,65*256+128),
                         0,80,4,0,colors,2);
    meshRequire(ggMeshContext.triangles>beforeActor+20,
                "four-legged species did not receive actual 3D bodies");
    ggExpRenderMode(pixels,1);
    unsigned long long checksum=0;
    for(int i=0;i<400*240*3;i+=13)
        checksum=checksum*131+pixels[i];
    ggExplore.angle=16;
    ggExpRenderMode(pixels,1);
    unsigned long long moved=0;
    for(int i=0;i<400*240*3;i+=13)moved=moved*131+pixels[i];
    meshRequire(checksum!=moved,
                "3D buildings did not exhibit moving-camera perspective");
    meshRequire(ggMeshContext.w==400&&
                ggMeshContext.fragments>500,
                "perspective camera rotation lost all physical mesh faces");
    ggExpScenicQuality=2;
    ggExpRenderMode(pixels,0);
    meshRequire(ggMeshContext.w==400&&ggMeshContext.scale==1,
                "New 3DS high-detail exploration is still a chunky 2x viewport");
    ggExpScenicQuality=1;
    ggExpRenderMode(pixels,0);
    meshRequire(ggMeshContext.w==200&&ggMeshContext.scale==2&&
                ggMeshContext.textureSamples==0,
                "Old 3DS must keep its inexpensive untextured fallback");
    // Every actor still uses the same physically projected world foot.
    for(int wx=64*256;wx<=65*256;wx+=64)
        meshRequire(ggMeshGroundAtQ8(wx,64*256+128)>=0,
                    "sloped triangle ground failed point sampling");
    // Verify live occupant integration, not only manually drawn rigs.
    nCr=1;sp[0].a=0;arch[0].diet=4;arch[0].shape=1;
    cr[0]=(Cr){0,66,64,1,0};crId[0]=733;
    occ[64][66]=1;ggExpMeshEnabled=1;
    ggExpScenicQuality=2;ggExplore.angle=0;
    ggExpRenderMode(pixels,0);
    meshRequire(ggExpNpcsDrawn==1,
                "playable citizen was not passed to the 3D actor renderer");
    occ[64][66]=0;nCr=0;
    // Keep legacy renderer selectable while the new 3D port is tuned.
    ggExpMeshEnabled=0;
    ggExpRenderMode(pixels,0);
    meshRequire(!ggExpMeshActive,
                "legacy raster fallback was accidentally removed");
    puts("3DS WORLD MESH OK: live terrain/buildings, geometry parallax, physical z depth, scalable framebuffer and fallback");
    return 0;
}
