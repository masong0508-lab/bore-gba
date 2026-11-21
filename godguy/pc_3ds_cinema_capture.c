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
    int wallDepth=0;
    unsigned long long noMask=0,masked=0;
    for(int occlude=0;occlude<2;occlude++){
        ggExpRenderMode(guarded.img,1);
        if(!ggFilmOcclusionHidden(ggExpFilmOcclusion,200,120,1000)){
            fputs("CINEMA CAPTURE FAIL: rendered house has no physical mask\n",stderr);
            return 6;
        }
        wallDepth=ggFilmStageDepth(68*256+128,66*256+128,
                    ggExplore.px,ggExplore.py,4096,0);
        int cx=ggFilmProjectScreenX(256,wallDepth);
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
        if(occlude)masked=hash;else noMask=hash;
        if(!ggCaptureSave(argv[1],occlude?
                                "wall_depth_on.ppm":"wall_depth_off.ppm"))
            return 7;
    }
    if(masked==noMask){
        fputs("CINEMA CAPTURE FAIL: wall does not actually occlude cel pixels\n",stderr);
        return 8;
    }
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
    fclose(stats);
    printf("3DS CAPTURE OK: 12 native 400x240 source frames, 15 cast angles, wall depth on/off, %.3f ms/host frame, %zu B scratch\n",
           ms,sizeof ggExpFilmOcclusion);
    return 0;
}
