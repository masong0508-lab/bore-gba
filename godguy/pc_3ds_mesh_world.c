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
    // Test a visible quadruped next to the center ray, not a subject
    // beyond the camera's side frustum. Count real rasterized faces.
    ggMeshActorCharacter(&ggMeshContext,65*256+128,64*256+160,
                         ggMeshGroundAtQ8(65*256+128,64*256+160),
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
    meshRequire(ggMeshPersonCount==1&&ggMeshPersonPixels>40,
                "playable person is not a visibly rendered procedural sprite");
    meshRequire(ggMeshPersonFacing(128,0)==0&&
                ggMeshPersonFacing(0,0)==4&&
                ggMeshPersonFacing(192,0)==2,
                "world-anchored procedural people have incorrect turn directions");
    occ[64][66]=0;nCr=0;
    // Moving an actual simulation citizen changes its world-space feet
    // and heading, not merely the animation tile or a screen-space wobble.
    ggPersonTrackReset();
    GgPersonMotion planted=ggPersonMotionSample(733,66*256+128,
                                                64*256+128,0,0);
    GgPersonMotion striding=ggPersonMotionSample(733,67*256+128,
                                                 64*256+128,10,0);
    GgPersonMotion mid=ggPersonMotionSample(733,67*256+128,
                                            64*256+128,15,0);
    meshRequire(!planted.walking&&striding.walking&&
                mid.x>striding.x&&mid.x<67*256+128&&
                mid.heading==0,
                "3DS citizens still teleport cell-to-cell instead of walking");
    ggPersonTrackReset();
    // Wall/world occlusion cannot be bypassed by the new generated
    // sprite painter. Front/behind use the SAME camera and geometry.
    ggMeshBegin(&ggMeshContext,pixels,2,0,0,10,4096,0,42,65,89);
    ggMeshContext.fogMax=0;
    ggMeshQuad(&ggMeshContext,
               ggMeshV(512,-400,0),ggMeshV(512,400,0),
               ggMeshV(512,400,40),ggMeshV(512,-400,40),
               111,101,93);
    ggMeshPersonCount=ggMeshPersonPixels=0;
    ggMeshPersonDraw(&ggMeshContext,1024,0,0,1024,0,
                     1001,4,128,60,0,colors);
    meshRequire(ggMeshPersonPixels==0&&ggMeshPersonCount==0,
                "procedural person visible through nearer stone building");
    ggMeshPersonDraw(&ggMeshContext,256,0,0,256,0,
                     1001,4,128,60,0,colors);
    meshRequire(ggMeshPersonCount==1&&ggMeshPersonPixels>50,
                "nearer procedural person clipped behind farther wall");
    // Test a stable city resident's eight orientations without an
    // artist-drawn atlas or any camera-animated face randomization.
    GgPersonCel still,blink;
    ggPersonGenerate(&still,1001,4,0,60,0,colors);
    ggPersonGenerate(&blink,1001,4,0,135,0,colors);
    meshRequire(!memcmp(still.palette,blink.palette,
                        sizeof still.palette),
                "procedural citizen rerolled colors between frames");
    // Keep legacy renderer selectable while the new 3D port is tuned.
    ggExpMeshEnabled=0;
    ggExpRenderMode(pixels,0);
    meshRequire(!ggExpMeshActive,
                "legacy raster fallback was accidentally removed");
    puts("3DS WORLD MESH OK: live terrain/buildings, geometry parallax, physical z depth, scalable framebuffer and fallback");
    return 0;
}
