// Camera/biome tour of the actual native renderer, with mixed-terrain
// source captures. Host timings are diagnostics, never console FPS.
#define main ggArtTourExistingRegression
#include "pc_explore3d.c"
#undef main
static void artRequire(int good,const char *why){
    if(!good){fprintf(stderr,"ART TOUR FAIL: %s\n",why);exit(1);}
}
static void artCapture(const char *dir,const char *name){
    char path[512];
    artRequire(snprintf(path,sizeof path,"%s/%s.ppm",dir,name)<(int)sizeof path,"capture path");
    FILE *f=fopen(path,"wb");artRequire(f!=NULL,"capture open");
    fprintf(f,"P6\n400 240\n255\n");
    artRequire(fwrite(guarded.img,1,sizeof guarded.img,f)==sizeof guarded.img,"capture pixels");
    artRequire(!fclose(f),"capture close");
}
static int artFrames,artPeakTriangles;
static void artGuards(void){
    artFrames++;
    if(ggMeshContext.triangles>artPeakTriangles)artPeakTriangles=ggMeshContext.triangles;
    for(unsigned i=0;i<sizeof guarded.first;i++)
        artRequire(guarded.first[i]==0x5a&&guarded.last[i]==0xc3,"native framebuffer guard");
    artRequire(ggMeshContext.w==400&&ggMeshContext.h==240,"native viewport at either detail level");
    artRequire(ggMeshPropsDrawn<=ggFilmScenicBudget(ggExpScenicQuality,0),"fixed scenery queue budget");
    artRequire(sizeof ggMeshDepthStorage==192000,"depth memory grew");
}
static void artClear(int terrain){
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=(u8)terrain;bld[y][x]=0;occ[y][x]=0;
    }
    nCr=0;ggPersonTrackReset();
}
static void artMixedVillage(void){
    artClear(T_GRASS);
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        int bank=61+(x/9)%3;
        if(y<bank-2)world[y][x]=T_DEEP;
        else if(y<bank)world[y][x]=T_WATER;
        else if(y==bank)world[y][x]=T_SAND;
        else if(y>70&&x>65)world[y][x]=T_FOREST;
        else if(x>75&&y>65)world[y][x]=T_HILL;
        if(y==66&&x>=63&&x<75)world[y][x]=T_DIRT;
    }
    bld[64][68]=B_HOUSE;bld[68][68]=B_HOUSE;
    bld[69][71]=B_HALL;bld[65][73]=B_TEMPLE;
    bld[64][72]=B_DOCK;bld[68][73]=B_FARM;
    bld[69][73]=B_FARM;bld[69][74]=B_FARM;
    nCr=3;
    sp[0].a=0;arch[0].diet=4;arch[0].shape=1;
    sp[1].a=1;arch[1].diet=1;arch[1].shape=4;
    cr[0]=(Cr){0,67,66,1,0};crId[0]=1001;
    cr[1]=(Cr){0,69,67,1,0};crId[1]=2049;
    cr[2]=(Cr){1,67,67,1,0};crId[2]=1002;
    for(int i=0;i<nCr;i++)occ[cr[i].y][cr[i].x]=(unsigned short)(i+1);
}
int main(int argc,char **argv){
    if(argc!=2)return 2;
    memset(guarded.first,0x5a,sizeof guarded.first);
    memset(guarded.last,0xc3,sizeof guarded.last);
    ggExpBegin(64,65,0);ggExpMeshEnabled=1;
    char path[512];
    artRequire(snprintf(path,sizeof path,"%s/art_tour_metrics.csv",argv[1])<(int)sizeof path,"metrics path");
    FILE *stats=fopen(path,"wb");artRequire(stats!=NULL,"metrics open");
    fprintf(stats,"terrain,detail,frames,host_cpu_ms_per_frame,peak_triangles,depth_bytes\n");
    for(int terrain=0;terrain<T_N;terrain++){
        artClear(terrain);
        if(terrain>T_WATER&&terrain!=T_LAVA){
            bld[64][68]=B_HOUSE;bld[67][70]=B_TEMPLE;
            bld[62][67]=B_HALL;
        }
        for(int quality=1;quality<=2;quality++){
            int peak=0;
            clock_t start=clock();
            ggExpScenicQuality=quality;
            for(int view=0;view<32;view++){
                // Multiple sub-cell positions cross slope boundaries;
                // all headings exercise clipped roofs, crowns and actors.
                ggExplore.px=64*256+32+(view%4)*64;
                ggExplore.py=65*256+32+((view/4)%4)*64;
                ggExplore.angle=view*8;ggExplore.frame=40+view*7;
                ggExpRenderMode(guarded.img,1);artGuards();
                if(ggMeshContext.triangles>peak)peak=ggMeshContext.triangles;
            }
            double ms=(double)(clock()-start)*1000.0/CLOCKS_PER_SEC/32.0;
            fprintf(stats,"%d,%d,32,%.5f,%d,%zu\n",terrain,quality,ms,peak,sizeof ggMeshDepthStorage);
        }
        ggExplore.px=64*256+128;ggExplore.py=65*256+128;
        ggExplore.angle=0;ggExplore.frame=80;ggExpScenicQuality=2;
        ggExpRenderMode(guarded.img,1);artGuards();
        char name[40];snprintf(name,sizeof name,"tour_biome_%02d",terrain);
        artCapture(argv[1],name);
    }
    artMixedVillage();ggExpScenicQuality=2;
    ggExplore.px=64*256+128;ggExplore.py=66*256+128;
    ggExplore.angle=0;ggExplore.frame=40;ggExplore.active=1;
    ggExpRenderMode(guarded.img,1);artGuards();
    artCapture(argv[1],"tour_village");
    ggExpRender(guarded.img);artGuards();
    artCapture(argv[1],"tour_village_playable");
    ggExplore.px=65*256+128;ggExplore.py=64*256+128;
    ggExplore.angle=237;ggExplore.frame=240;
    ggExpRenderMode(guarded.img,1);artGuards();
    artCapture(argv[1],"tour_coast");
    nCr=0;memset(occ,0,sizeof occ);
    // Map corners must clip scenery scans without borrowing off-map data.
    for(int corner=0;corner<4;corner++)for(int angle=0;angle<256;angle+=16){
        ggExplore.px=((corner&1)?WW-1:0)*256+128;
        ggExplore.py=((corner&2)?WH-1:0)*256+128;
        ggExplore.angle=angle;ggExplore.frame=angle;
        ggExpRenderMode(guarded.img,1);artGuards();
    }
    artRequire(!fclose(stats),"metrics close");
    printf("3DS ART TOUR OK: %d guarded frames, 13 biomes, 32 headings, both native detail levels, four map corners; peak %d triangles; %zu depth bytes\n",artFrames,artPeakTriangles,sizeof ggMeshDepthStorage);
    return 0;
}
