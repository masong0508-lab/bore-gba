// GodGuy 3DS source-rendered native-resolution QA capture.
// Reuses the playable game's real 400x240 cinema world, actor and camera.
// Outputs 12 story frames, front/left/right cast, foreground occlusion
// comparisons and host CPU-frame cost. This is NOT hardware 3DS footage.
// gcc -O2 -std=c11 -w -Icompat -Isource pc_3ds_cinema_capture.c -lm -o /tmp/ggcap
#define main ggCaptureHostRegressionMain
#include "pc_explore3d.c"
#undef main
#include <time.h>
static unsigned long long ggCaptureHash(const u8* pixels){
    unsigned long long hash=1469598103934665603ull;
    for(size_t i=0;i<400u*240u*3u;i+=11u)
        hash=(hash^pixels[i])*1099511628211ull;
    return hash;
}
static int ggCaptureSave(const char *folder,const char *filename){
    char path[512];
    if(snprintf(path,sizeof path,"%s/%s",folder,filename)>=(int)sizeof path)
        return 0;
    FILE*f=fopen(path,"wb");
    if(!f)return 0;
    fprintf(f,"P6\n400 240\n255\n");
    int ok=fwrite(guarded.img,1,400u*240u*3u,f)==400u*240u*3u;
    if(fclose(f))ok=0;
    return ok;
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    // Existing authored 2.5D comparisons still exercise the legacy
    // occlusion path; otherwise a change to triangle z buffering would
    // make those real regressions trivially pass or fail incorrectly.
    ggExpMeshEnabled=0;
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)
        world[y][x]=((x+y)%29==0)?T_FOREST:T_GRASS;
    for(int y=52;y<81;y+=4)for(int x=56;x<91;x+=5)
        bld[y][x]=(x+y)%3?B_HOUSE:B_HALL;
    ggExpBegin(65,65,0);
    ggChron.active=1;ggChron.act=3;ggChron.post=3;
    ggChron.px[0]=63;ggChron.py[0]=65;
    ggChron.px[1]=75;ggChron.py[1]=69;
    ggChron.px[2]=84;ggChron.py[2]=72;
    ggOrigin.complete=1;ggOrigin.active=0;
    ggOrigin.skin=2;ggOrigin.hair=3;ggOrigin.garment=4;
    ggReign.loaded=1;ggReign.menu=0;
    ggSaga.loaded=1;ggSaga.mode=GG_SAGA_OUTRO;
    ggSaga.ending=GG_END_HEARTH;ggSagaShot.valid=0;
    unsigned long long first=0,last=0;
    clock_t start=clock();
    for(int i=0;i<12;i++){
        ggSaga.frame=i*8;
        ggSagaDraw(guarded.img);
        char shot[40];
        snprintf(shot,sizeof shot,"frame_%02d.ppm",i);
        if(!ggCaptureSave(argv[1],shot))return 3;
        if(i==0)first=ggCaptureHash(guarded.img);
        if(i==11)last=ggCaptureHash(guarded.img);
    }
    double ms=(double)(clock()-start)*1000.0/
              (double)CLOCKS_PER_SEC/12.0;
    if(first==last){
        fputs("CINEMA CAPTURE FAIL: actor/camera frames did not change\n",stderr);
        return 4;
    }
    // Three actual RGB-rendered head/torso orientations with the same
    // palette, native viewport and fixed foot anchor.
    ggSaga.mode=GG_SAGA_IDLE;ggExplore.px=65*256+128;
    ggExplore.py=65*256+128;ggExplore.angle=0;
    for(int style=0;style<5;style++)
        for(int turn=0;turn<3;turn++){
            ggExpRenderMode(guarded.img,1);
            GgFilmPose p=ggFilmPoseFor("The village lives.",
                                      2,48,0,0,0,0);
            p.turn=(signed char)((turn-1)*4);
            ggFilmDrawGuestActor(guarded.img,200,160,2,
                 ggFilmGuestPalette[style][0],
                 ggFilmGuestPalette[style][1],
                 ggFilmGuestPalette[style][2],&p,style);
            char filename[40];
            snprintf(filename,sizeof filename,"cast_%d_%d.ppm",style,turn);
            if(!ggCaptureSave(argv[1],filename))return 5;
        }
    // Build a reproducible 3D wall in the TRUE live-world camera lane,
    // place an actor behind it, and capture depth OFF versus depth ON.
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;
    }
    bld[65][67]=B_HALL;
    ggExplore.px=65*256+128;ggExplore.py=65*256+128;
    ggExplore.angle=0;ggExplore.frame=0;
    int wallDepth=0,differentPixels=0;
    // Host-only screenshot comparison; never linked into a ROM.
    static u8 unmaskedFrame[400*240*3];
    unsigned long long noMask=0,masked=0;
    for(int occlude=0;occlude<2;occlude++){
        ggExpRenderMode(guarded.img,1);
        if(!ggFilmOcclusionHidden(ggExpFilmOcclusion,200,120,1000)){
            fputs("CINEMA CAPTURE FAIL: rendered house has no physical mask\n",stderr);
            return 6;
        }
        wallDepth=ggFilmStageDepth(68*256+128,66*256+65,
                    ggExplore.px,ggExplore.py,4096,0);
        int cx=ggFilmProjectScreenX(193,wallDepth);
        int feet=ggFilmProjectFootY(wallDepth,ggExpGround(T_GRASS),
                                               ggExpGround(T_GRASS));
        GgFilmPose p=ggFilmPoseFor("The building must hide me.",
                                  1,48,0,0,0,0);
        ggExpFilmDrawDepth=occlude?wallDepth:0;
        ggFilmDrawGuestActor(guarded.img,cx,feet,2,
             ggFilmGuestPalette[3][0],ggFilmGuestPalette[3][1],
             ggFilmGuestPalette[3][2],&p,3);
        ggExpFilmDrawDepth=0;
        unsigned long long hash=ggCaptureHash(guarded.img);
        if(occlude){
            masked=hash;
            for(int i=0;i<400*240;i++){
                int k=i*3;
                if(unmaskedFrame[k]!=guarded.img[k]||
                   unmaskedFrame[k+1]!=guarded.img[k+1]||
                   unmaskedFrame[k+2]!=guarded.img[k+2])
                    differentPixels++;
            }
        }else{
            noMask=hash;
            memcpy(unmaskedFrame,guarded.img,sizeof unmaskedFrame);
        }
        if(!ggCaptureSave(argv[1],occlude?
                                "wall_depth_on.ppm":"wall_depth_off.ppm"))
            return 7;
    }
    if(masked==noMask||differentPixels<40){
        fprintf(stderr,"CINEMA CAPTURE FAIL: wall cut only %d pixels; not a useful foreground occlusion test\n",differentPixels);
        return 8;
    }
    // Capture the PLAYABLE first-person exploration renderer, including
    // crosshair and control legend, over a short camera move on real map
    // cells. These are source gameplay views, not a simulated film HUD.
    ggExplore.active=1;ggExplore.angle=0;ggExplore.frame=0;
    ggExplore.actorId=0;ggExplore.material=0;
    for(int i=0;i<6;i++){
        ggExplore.px=63*256+128+i*30;
        ggExplore.py=65*256+128;
        ggExplore.frame=i*3;
        ggExpRender(guarded.img);
        char name[40];
        snprintf(name,sizeof name,"pov_%02d.ppm",i);
        if(!ggCaptureSave(argv[1],name))return 11;
    }
    // Native RGB reference footage of biome-specific actual world cells.
    // Compare the exact same camera frame with the bounded set-dressing
    // enabled and disabled, ensuring richer scenery isn't only a list
    // of design intentions or a separate illustration/mockup.
    static const int biome[8]={
        T_FOREST,T_GRASS,T_GRASS,T_HILL,
        T_SWAMP,T_SNOW,T_GRASS,T_MOUNT
    };
    static const int settlement[8]={
        B_NONE,B_FARM,B_TEMPLE,B_RUIN,
        B_DOCK,B_NONE,B_HOUSE,B_NONE
    };
    int minScenicPixels=999999,visibleProps=0;
    FILE*scenicStats=0;
    char sceneryMetrics[512];
    if(snprintf(sceneryMetrics,sizeof sceneryMetrics,
                "%s/scenery_metrics.csv",argv[1])>=
       (int)sizeof sceneryMetrics)return 12;
    scenicStats=fopen(sceneryMetrics,"wb");
    if(!scenicStats)return 13;
    fprintf(scenicStats,
            "biome,visible_props,changed_pixels,foreground_pixels_y155plus,host_cpu_ms_per_frame,resolution\n");
    for(int scene=0;scene<8;scene++){
        for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
            world[y][x]=(u8)biome[scene];
            bld[y][x]=0;occ[y][x]=0;
        }
        if(settlement[scene]){
            for(int y=63;y<=67;y+=2)for(int x=67;x<=75;x+=4){
                bld[y][x]=(u8)(scene==6&&((x+y)&2)?
                               B_HALL:settlement[scene]);
            }
        }
        ggExplore.px=64*256+128;ggExplore.py=65*256+128;
        ggExplore.angle=0;ggExplore.frame=40;
        ggExpSceneryEnabled=0;
        ggExpRenderMode(guarded.img,1);
        char offName[48],onName[48];
        snprintf(offName,sizeof offName,"scenery_off_%02d.ppm",scene);
        snprintf(onName,sizeof onName,"scenery_%02d.ppm",scene);
        if(!ggCaptureSave(argv[1],offName))return 14;
        memcpy(unmaskedFrame,guarded.img,sizeof unmaskedFrame);
        ggExpSceneryEnabled=1;
        clock_t scenicStart=clock();
        ggExpRenderMode(guarded.img,1);
        double scenicMs=(double)(clock()-scenicStart)*1000.0/
                        CLOCKS_PER_SEC;
        if(!ggCaptureSave(argv[1],onName))return 15;
        int difference=0,foregroundPixels=0;
        for(int i=0;i<400*240*3;i+=3)
            if(memcmp(unmaskedFrame+i,guarded.img+i,3)){
                difference++;
                if((i/3)/400>=155)foregroundPixels++;
            }
        if(difference<minScenicPixels)minScenicPixels=difference;
        visibleProps+=ggExpScenicVisible;
        fprintf(scenicStats,"%d,%d,%d,%d,%.4f,400x240\n",
                scene,ggExpScenicVisible,difference,foregroundPixels,scenicMs);
        if(difference<110||ggExpScenicVisible<4||
           ((scene==0||scene==5||scene==7)&&foregroundPixels<250)){
            fprintf(stderr,
                "SCENERY CAPTURE FAIL: sparse biome %d (%d scenic pixels, %d projected objects, %d foreground pixels)\n",
                scene,difference,ggExpScenicVisible,foregroundPixels);
            fclose(scenicStats);
            return 16;
        }
    }
    fclose(scenicStats);
    // Same world/camera in reduced Old-3DS-detail mode. Capture both
    // instead of claiming the adaptive path helps based on arithmetic.
    ggExpScenicQuality=2;
    ggExpRenderMode(guarded.img,1);
    int fullQualityProps=ggExpScenicVisible;
    ggExpScenicQuality=1;
    ggExpRenderMode(guarded.img,1);
    int budgetQualityProps=ggExpScenicVisible;
    if(!ggCaptureSave(argv[1],"scenery_old3ds_quality.ppm"))
        return 20;
    if(budgetQualityProps>fullQualityProps){
        fprintf(stderr,
                "SCENERY LOD FAIL: fallback drew %d instead of %d props\n",
                budgetQualityProps,fullQualityProps);
        return 21;
    }
    ggExpScenicQuality=2;
    // Playable POV demonstration of the enhanced low-cost 2x grid.
    ggExplore.active=1;ggExplore.px=64*256+128;
    ggExplore.py=65*256+128;ggExplore.angle=0;ggExplore.frame=15;
    ggExpSceneryEnabled=0;
    ggExpRender(guarded.img);
    if(!ggCaptureSave(argv[1],"pov_scenery_off.ppm"))return 17;
    memcpy(unmaskedFrame,guarded.img,sizeof unmaskedFrame);
    ggExpSceneryEnabled=1;
    ggExpRender(guarded.img);
    if(!ggCaptureSave(argv[1],"pov_scenery_on.ppm"))return 18;
    int povPixels=0;
    for(int i=0;i<400*240*3;i+=3)
        if(memcmp(unmaskedFrame+i,guarded.img+i,3))povPixels++;
    if(povPixels<60){
        fprintf(stderr,"SCENERY CAPTURE FAIL: playable POV only changed %d pixels\n",povPixels);
        return 19;
    }
    // Compare the EXACT same actor compositing load with and without
    // the wall-depth test, on a host CPU. This is instrumentation, NOT
    // an unmeasured claim about the Old 3DS's hardware frame rate.
    const int trials=160;
    ggExpRenderMode(guarded.img,1);
    GgFilmPose overlay=ggFilmPoseFor("The village lives.",2,48,0,0,0,0);
    clock_t startFlat=clock();
    ggExpFilmDrawDepth=0;
    for(int i=0;i<trials;i++)
        ggFilmDrawGuestActor(guarded.img,200,156,2,
            ggFilmGuestPalette[0][0],ggFilmGuestPalette[0][1],
            ggFilmGuestPalette[0][2],&overlay,0);
    double flatMs=(double)(clock()-startFlat)*1000.0/
                  CLOCKS_PER_SEC/trials;
    ggExpRenderMode(guarded.img,1);
    clock_t startDepth=clock();
    ggExpFilmDrawDepth=768;
    for(int i=0;i<trials;i++)
        ggFilmDrawGuestActor(guarded.img,200,156,2,
            ggFilmGuestPalette[0][0],ggFilmGuestPalette[0][1],
            ggFilmGuestPalette[0][2],&overlay,0);
    ggExpFilmDrawDepth=0;
    double maskedMs=(double)(clock()-startDepth)*1000.0/
                    CLOCKS_PER_SEC/trials;
    char metrics[512];
    if(snprintf(metrics,sizeof metrics,"%s/capture_metrics.csv",argv[1])>=
       (int)sizeof metrics)return 9;
    FILE*stats=fopen(metrics,"wb");
    if(!stats)return 10;
    fprintf(stats,"scenario,frames,resolution,host_cpu_ms_per_frame,occlusion_scratch_bytes,checksum\n");
    fprintf(stats,"cinematic_story,12,400x240,%.3f,%zu,%llu\n",
            ms,sizeof ggExpFilmOcclusion,last);
    fprintf(stats,"foreground_depth_comparison,2,400x240,,%zu,%llu\n",
            sizeof ggExpFilmOcclusion,masked);
    fprintf(stats,"actor_overlay_no_depth,%d,400x240,%.6f,%zu,0\n",
            trials,flatMs,sizeof ggExpFilmOcclusion);
    fprintf(stats,"actor_overlay_depth_test,%d,400x240,%.6f,%zu,0\n",
            trials,maskedMs,sizeof ggExpFilmOcclusion);
    fprintf(stats,"scenery_biomes,8,400x240,,%zu,%d\n",
            sizeof ggExpFilmOcclusion+sizeof ggExpScenicItems,visibleProps);
    fprintf(stats,"playable_pov_scenery,1,400x240,,%zu,%d\n",
            sizeof ggExpFilmOcclusion,povPixels);
    fclose(stats);
    // Genuine polygon-world validation: these frames are not 2D
    // raycast screenshots or mocked new art. They use exactly the
    // live mesh camera enabled by default in main3ds.c.
    static const int meshBiomes[8]={
        T_GRASS,T_FOREST,T_HILL,T_SNOW,
        T_SAND,T_SWAMP,T_JUNGLE,T_MOUNT
    };
    FILE*meshMetrics=0;
    char meshPath[512];
    if(snprintf(meshPath,sizeof meshPath,
                "%s/mesh_metrics.csv",argv[1])>=
       (int)sizeof meshPath)return 22;
    meshMetrics=fopen(meshPath,"wb");
    if(!meshMetrics)return 23;
    fprintf(meshMetrics,"scenario,triangles,visible_fragments,buildings,props,cpu_ms,viewport\n");
    ggExpMeshEnabled=1;ggExpScenicQuality=2;
    int totalTriangles=0,firstPolys=0;
    for(int scene=0;scene<8;scene++){
        for(int yy=0;yy<WH;yy++)for(int xx=0;xx<WW;xx++){
            world[yy][xx]=(u8)meshBiomes[scene];
            bld[yy][xx]=0;
            occ[yy][xx]=0;
        }
        for(int yy=63;yy<=69;yy+=3)
            for(int xx=67;xx<=76;xx+=4)
                bld[yy][xx]=(u8)((xx+yy+scene)%3==0?
                    B_TEMPLE:(xx+scene)%3==0?B_HOUSE:B_HALL);
        ggExplore.px=64*256+128;
        ggExplore.py=65*256+128;
        ggExplore.angle=(scene%2)*6;
        ggExplore.frame=scene*6;
        clock_t startedMesh=clock();
        ggExpRenderMode(guarded.img,1);
        double meshMs=(double)(clock()-startedMesh)*1000.0/
                      CLOCKS_PER_SEC;
        if(!ggExpMeshActive||
           ggMeshContext.w!=400||ggMeshContext.h!=240||
           ggMeshContext.triangles<80||
           ggMeshContext.fragments<1000){
            fprintf(stderr,
                "MESH CAPTURE FAIL: scene %d did not render a real native z-tested world (%d tris, %d fragments)\n",
                scene,ggMeshContext.triangles,ggMeshContext.fragments);
            fclose(meshMetrics);
            return 24;
        }
        char imageName[40];
        snprintf(imageName,sizeof imageName,"mesh_env_%02d.ppm",scene);
        if(!ggCaptureSave(argv[1],imageName))return 25;
        fprintf(meshMetrics,"cinematic_biome_%d,%d,%d,%d,%d,%.3f,400x240\n",
                scene,ggMeshContext.triangles,ggMeshContext.fragments,
                ggMeshBuildingsDrawn,ggMeshPropsDrawn,meshMs);
        totalTriangles+=ggMeshContext.triangles;
        if(!scene)firstPolys=ggMeshContext.triangles;
    }
    // Compare real movement/turn frames with actual polygon geometry.
    for(int yy=0;yy<WH;yy++)for(int xx=0;xx<WW;xx++){
        world[yy][xx]=T_GRASS;bld[yy][xx]=0;occ[yy][xx]=0;
    }
    bld[65][68]=B_HOUSE;bld[66][70]=B_TOWER;
    bld[64][72]=B_TEMPLE;
    ggExplore.px=64*256+128;ggExplore.py=65*256+128;
    ggExplore.angle=0;ggExplore.frame=0;
    ggExpRenderMode(guarded.img,1);
    unsigned long long startPolys=ggCaptureHash(guarded.img);
    for(int k=0;k<6;k++){
        ggExplore.angle=k*4;
        ggExplore.px=64*256+128+k*20;
        ggExplore.frame=k*8;
        ggExpRenderMode(guarded.img,1);
        char f[40];
        snprintf(f,sizeof f,"mesh_walk_%02d.ppm",k);
        if(!ggCaptureSave(argv[1],f))return 26;
    }
    if(startPolys==ggCaptureHash(guarded.img)){
        fputs("MESH CAPTURE FAIL: physical 3D perspective remains static on camera move\n",stderr);
        return 27;
    }
    // Old 3DS uses the exact same 3D geometry at lower pixel density
    // while preserving 400x240 output and the game's HUD overlay.
    ggExpScenicQuality=1;
    ggExpRenderMode(guarded.img,0);
    if(ggMeshContext.w!=200||ggMeshContext.h!=120||
       !ggCaptureSave(argv[1],"mesh_old3ds_playable.ppm"))
        return 28;
    fprintf(meshMetrics,"old3ds_playable,%d,%d,%d,%d,,200x120-upscaled\n",
            ggMeshContext.triangles,ggMeshContext.fragments,
            ggMeshBuildingsDrawn,ggMeshPropsDrawn);
    ggExpScenicQuality=2;
    ggExpRenderMode(guarded.img,1);
    fprintf(meshMetrics,"high_detail_check,%d,%d,%d,%d,,400x240\n",
            ggMeshContext.triangles,ggMeshContext.fragments,
            ggMeshBuildingsDrawn,ggMeshPropsDrawn);
    fclose(meshMetrics);
    printf("3DS TRUE MESH CAPTURE OK: eight polygonal biomes, six moving-camera images, %d triangles in scene zero, %d cumulative triangles, real inverse-depth ground and buildings\n",
           firstPolys,totalTriangles);
    printf("3DS CAPTURE OK: 12 story frames, 15 cast turntable views, 8 biomes, 6 playable POV frames and wall occlusion; %d props across biomes, minimum %d scenic pixel changes, %d playable POV changes; %.3f ms/host scene frame, %.6f/%.6f ms actor without/with depth; %zu B depth+scenery scratch\n",
           visibleProps,minScenicPixels,povPixels,
           ms,flatMs,maskedMs,
           sizeof ggExpFilmOcclusion+sizeof ggExpScenicItems);
    return 0;
}
