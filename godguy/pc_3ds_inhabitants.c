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
static void inhabitantFooting(void){
    world[64][65]=T_HILL;
    for(int heading=0;heading<256;heading+=32){
        int wx=65*256+16,wy=64*256+128;
        GgMeshCreaturePose pose={wx,wy,ggMeshGroundAtQ8(wx,wy),
                                 ggExpCos[heading],ggExpSin[heading]};
        for(int along=-36;along<=36;along+=24)
            for(int across=-30;across<=30;across+=20){
                GgMeshVertex toe=ggMeshCreatureV(&pose,along,across,0);
                int floor=ggMeshGroundAtQ8(toe.x,toe.y);
                int boot=pose.z+ggMeshCreatureFoot(&pose,along,across);
                inhabitantRequire(boot<=floor&&boot>=floor-1,
                                  "a planted toe sits above or below its local ground");
            }
    }
    world[64][65]=T_GRASS;
}
typedef struct {int top,bottom;} InhabitantProfile;
static InhabitantProfile inhabitantProfile(int shape,int heading,int phase,
                                           int ground,int left,int right){
    int x=66*256+128,y=64*256+128;
    ggMeshBegin(&ggMeshContext,guarded.img,2,x-384,y,ground+13,
                4096,0,55,77,99);
    ggMeshActorSpecies(&ggMeshContext,x,y,ground,heading,phase,0,
                       shape,famCol[0],2);
    InhabitantProfile profile={240,-1};
    for(int row=0;row<240;row++)
        for(int col=left;col<right;col++)
            if(ggMeshContext.depth[row*400+col]){
                if(row<profile.top)profile.top=row;
                if(row>profile.bottom)profile.bottom=row;
            }
    inhabitantGuard();
    return profile;
}
static void inhabitantAnatomy(void){
    InhabitantProfile wolf=inhabitantProfile(20,64,0,5,150,250);
    InhabitantProfile rabbit=inhabitantProfile(10,64,0,5,150,250);
    InhabitantProfile deer=inhabitantProfile(9,64,0,5,150,250);
    inhabitantRequire(wolf.top+4<rabbit.top&&deer.top+4<wolf.top,
                      "rabbit, wolf and deer lack a distinct adult height hierarchy");

    // Nose-on, the bat's outer membrane should have vertical thickness
    // even with the wings near horizontal, and lift visibly with the stroke.
    InhabitantProfile bat=inhabitantProfile(14,128,0,5,216,240);
    InhabitantProfile up=inhabitantProfile(14,128,64,5,216,240);
    inhabitantRequire(bat.bottom-bat.top>=11&&up.top+13<bat.top,
                      "bat wing collapsed to a flat sheet or lost its upstroke");

    // Same nominal base and camera. Only a real water surface should
    // displace the swimmer below its dry-land profile.
    InhabitantProfile dry=inhabitantProfile(2,64,0,1,185,215);
    world[64][66]=T_WATER;
    InhabitantProfile wet=inhabitantProfile(2,64,0,1,185,215);
    world[64][66]=T_GRASS;
    inhabitantRequire(dry.top+4<wet.top&&wet.bottom>dry.bottom,
                      "fish hull does not sit into the water when swimming");
}
int main(int argc,char**argv){
    // Numeric shapes come from the authored source/main.c SHAPE table.
    // Include the largest (deer, scorpion, whale) as well as the
    // smallest models, and the skeleton/printer/finger sprite fallbacks.
    // Same palette/identity/camera for all, so geometry must distinguish
    // them. These records run through ggExpRenderCreatures, not a mock.
    static const int shapes[]={20,1,2,12,3,6,63,10,15,14,9,37,13,50,
                               51,53,60,61,16,78,79};
    enum {CASES=sizeof shapes/sizeof shapes[0]};
    unsigned long picture[CASES];
    static u8 empty[400*240*3];
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        world[y][x]=T_GRASS;bld[y][x]=0;occ[y][x]=0;
    }
    memset(guarded.first,0x5a,sizeof guarded.first);
    memset(guarded.last,0xc3,sizeof guarded.last);
    ggExpBegin(64,64,0);
    inhabitantFooting();
    inhabitantAnatomy();
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
    printf("3DS INHABITANTS OK: %d source species x 8 headings, %d minimum visible pixels, %d maximum added triangles, anatomy, footing, wall/near clipping and shared haze\n",
           CASES,least,mostTriangles);
    return 0;
}
