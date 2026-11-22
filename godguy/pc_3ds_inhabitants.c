// Actual-population renderer regression for the native inhabitants pass.
// gcc -O2 -std=c11 -DGG_NEW3DS_ONLY -w -Icompat -Isource \
//     pc_3ds_inhabitants.c -lm -o /tmp/gg-inhabitants
#define main ggInhabitantLegacyMain
#include "pc_explore3d.c"
#undef main
static void inhabitantRequire(int good,const char*why){
    if(!good){fprintf(stderr,"3DS INHABITANTS FAIL: %s\n",why);exit(1);}
}
static void inhabitantGuard(void){
    for(size_t i=0;i<sizeof guarded.first;i++)
        inhabitantRequire(guarded.first[i]==0x5a&&guarded.last[i]==0xc3,
                          "close creature wrote outside native RGB frame");
}
static void inhabitantCapture(const char*folder,int shape,int turn){
    if(!folder)return;
    char path[512];
    inhabitantRequire(snprintf(path,sizeof path,"%s/species_%02d_%d.ppm",
                               folder,shape,turn)<(int)sizeof path,
                      "capture path is too long");
    FILE*f=fopen(path,"wb");
    inhabitantRequire(f!=NULL,"cannot open inhabitant capture");
    fprintf(f,"P6\n400 240\n255\n");
    inhabitantRequire(fwrite(guarded.img,1,sizeof guarded.img,f)==
                      sizeof guarded.img,"cannot finish inhabitant capture");
    inhabitantRequire(fclose(f)==0,"cannot close inhabitant capture");
}
int main(int argc,char**argv){
    // Numeric shapes come from the authored source/main.c SHAPE table:
    // wolf, bird, fish, spider, snake, blob, butterfly, rabbit, turtle,
    // bat and the authored skeleton/printer/finger fallback fixtures.
    // Same palette/identity/camera for all, so geometry must distinguish
    // them. These records run through ggExpRenderCreatures, not a mock.
    static const int shapes[]={20,1,2,12,3,6,63,10,15,14,16,78,79};
    enum {CASES=sizeof shapes/sizeof shapes[0]};
    unsigned long picture[CASES];
    static u8 empty[400*240*3];
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;occ[y][x]=0;
    }
    memset(guarded.first,0x5a,sizeof guarded.first);
    memset(guarded.last,0xc3,sizeof guarded.last);
    ggExpBegin(64,64,0);
    ggExpMeshEnabled=1;ggExpScenicQuality=2;ggExpSceneryEnabled=0;ggExpBuildPreview=0;
    ggExplore.angle=0;ggExplore.px=64*256+128;
    ggExplore.py=64*256+128;ggExplore.frame=52;ggExplore.actorId=0;
    nCr=0;ggExpRenderMode(guarded.img,0);
    int terrainTriangles=ggMeshContext.triangles;
    memcpy(empty,guarded.img,sizeof empty);
    nCr=1;sp[0].a=0;arch[0].diet=1;
    cr[0]=(Cr){0,66,64,1,0};crId[0]=1001;occ[64][66]=1;
    int least=100000,mostTriangles=0;
    for(int i=0;i<CASES;i++){
        arch[0].shape=shapes[i];
        for(int turn=0;turn<8;turn++){
            ggPersonTrackReset();
            ggPersonMotionSample(1001,66*256+128,64*256+128,52,turn*32);
            ggExpRenderMode(guarded.img,0);
            inhabitantGuard();
            inhabitantRequire(ggExpNpcsDrawn==1&&ggMeshPersonCount==0,
                              "non-human species bypassed live mesh population path");
            int extra=ggMeshContext.triangles-terrainTriangles;
            int original=ggMeshCreatureKind(shapes[i])==GG_CREATURE_ORIGINAL;
            inhabitantRequire(extra>=(original?0:12)&&extra<=320,
                              "species mesh vanished or exceeded 320-triangle actor budget");
            if(extra>mostTriangles)mostTriangles=extra;
            int changed=0;
            for(size_t p=28u*400u*3u;p<195u*400u*3u;p+=3)
                if(memcmp(empty+p,guarded.img+p,3))changed++;
            inhabitantRequire(changed>=80,
                              "live species collapsed to a hidden/one-pixel silhouette");
            if(changed<least)least=changed;
            if(turn==2){
                picture[i]=hash();
                for(int j=0;j<i;j++)
                    inhabitantRequire(picture[i]!=picture[j],
                                      "different authored species still draw the same quadruped");
                inhabitantCapture(argc>1?argv[1]:NULL,shapes[i],turn);
            }
        }
    }
    // Occlusion is tested through the actual lookup and world ray, too.
    // A living creature behind a house must not be repainted over it.
    bld[64][65]=B_HALL;nCr=0;
    ggExpRenderMode(guarded.img,0);memcpy(empty,guarded.img,sizeof empty);
    nCr=1;arch[0].shape=20;
    ggExpRenderMode(guarded.img,0);
    inhabitantRequire(ggExpNpcsDrawn==0&&
                      !memcmp(empty,guarded.img,sizeof empty),
                      "species shows through a building in the active population path");
    bld[64][65]=0;
    // Wide fins, ant legs and tails crossing near/screen planes are
    // clipped by the same triangle rasterizer, with guarded RGB writes.
    for(int quality=1;quality<=2;quality++)
      for(int i=0;i<CASES;i++)
        for(int side=-1;side<=1;side++){
            ggMeshBegin(&ggMeshContext,guarded.img,quality,
                        64*256+128,64*256+128,13,4096,0,55,77,99);
            if(ggMeshCreatureKind(shapes[i])==GG_CREATURE_ORIGINAL)
                ggMeshOriginalCreature(&ggMeshContext,64*256+240,
                    64*256+128+side*100,0,112,side*100,
                    shapes[i],0,famCol[0]);
            else
                ggMeshActorSpecies(&ggMeshContext,64*256+240,
                                   64*256+128+side*100,0,64,91,0,
                                   shapes[i],famCol[0],quality);
            inhabitantGuard();
        }
    ggMeshBegin(&ggMeshContext,guarded.img,2,0,0,13,4096,0,55,77,99);
    ggMeshQuad(&ggMeshContext,ggMeshV(256,-400,0),ggMeshV(256,400,0),
                ggMeshV(256,400,40),ggMeshV(256,-400,40),111,101,93);
    memcpy(empty,guarded.img,sizeof empty);
    ggMeshOriginalCreature(&ggMeshContext,512,0,0,512,0,79,0,famCol[0]);
    inhabitantRequire(!memcmp(empty,guarded.img,sizeof empty),
                      "authored-sprite fallback bypasses physical z buffer");
    inhabitantGuard();
    // Human ink grading shares the actual mesh's distance haze. With
    // fog disabled/enabled the same distant person must change color.
    unsigned char dryPalette[GG_PERSON_INKS][3];
    for(int haze=0;haze<2;haze++){
        ggMeshBegin(&ggMeshContext,guarded.img,2,0,0,13,4096,0,55,77,99);
        ggMeshContext.fogMax=haze?160:0;
        ggMeshContext.fogR=164;ggMeshContext.fogG=182;ggMeshContext.fogB=193;
        ggMeshPersonDraw(&ggMeshContext,2048,0,0,2048,0,
                         1001,0,128,52,0,famCol[0]);
        if(!haze)memcpy(dryPalette,ggMeshPersonCel.palette,sizeof dryPalette);
        else inhabitantRequire(memcmp(dryPalette,ggMeshPersonCel.palette,
                                      sizeof dryPalette)!=0,
                               "distant citizen ignores physical scene haze");
        inhabitantGuard();
    }
    printf("3DS INHABITANTS OK: 13 source species x 8 headings, %d minimum visible pixels, %d maximum added triangles, wall/near clipping and shared haze\n",
           least,mostTriangles);
    return 0;
}
