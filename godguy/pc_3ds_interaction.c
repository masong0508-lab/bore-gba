// Actual exploration targeting and world-space tool feedback. SDK free.
#define main ggInteractionExistingMain
#include "pc_explore3d.c"
#undef main
static void interactionRequire(int good,const char *message){
    if(!good){fprintf(stderr,"3DS INTERACTION FAIL: %s\n",message);exit(1);}
}
int main(int argc,char **argv){
    memset(bld,0,sizeof bld);memset(occ,0,sizeof occ);nCr=0;
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)world[y][x]=T_GRASS;
    ggExpBegin(64,64,0);ggExplore.angle=0;ggExplore.material=4;
    int tx,ty;ggExpTarget(&tx,&ty);
    interactionRequire(tx==67&&ty==64,"open construction ray changed reach");
    bld[64][66]=B_HOUSE;
    ggExpTarget(&tx,&ty);
    interactionRequire(tx==66&&ty==64&&
        ggExpEditStatus(tx,ty,4,0)==GG_EDIT_BUILDING,
        "aim passes through protected architecture");
    ggExpStep(KEY_A,0,0);
    interactionRequire(world[64][67]==T_GRASS&&bld[64][66]==B_HOUSE&&
        ggExplore.editStatus==GG_EDIT_BUILDING,
        "blocked action changed hidden land or lost its specific reason");
    bld[64][66]=0;occ[64][65]=1;
    ggExpTarget(&tx,&ty);
    interactionRequire(tx==65&&ty==64&&
        ggExpEditStatus(tx,ty,4,0)==GG_EDIT_CREATURE,
        "aim passes through a protected inhabitant");
    occ[64][65]=0;occ[64][64]=1;ggExplore.actorId=77;
    ggExpTarget(&tx,&ty);
    interactionRequire(tx==67&&ty==64,"possessed citizen blocks their own aim");
    occ[64][64]=0;ggExplore.actorId=0;
    world[64][66]=T_MOUNT;ggExpTarget(&tx,&ty);
    interactionRequire(tx==66&&ty==64,"aim paints behind a tall terrain obstruction");
    world[64][66]=T_GRASS;
    world[64][64]=T_MOUNT;
    for(int y=63;y<=65;y++)for(int x=65;x<=66;x++)world[y][x]=T_MOUNT;
    ggExplore.py=64*256+250;ggExpTarget(&tx,&ty);
    interactionRequire(tx<67&&ty==64,
        "slope-edge targeting sees through a ridge below the nominal tile eye");
    world[64][64]=T_GRASS;
    for(int y=63;y<=65;y++)for(int x=65;x<=66;x++)world[y][x]=T_GRASS;
    // The target cell can cross a diagonal tile corner while its center
    // is still within reach of the player's fractional world position.
    for(int offset=16;offset<=240;offset+=224)for(int angle=0;angle<256;angle++){
        ggExplore.px=64*256+offset;ggExplore.py=64*256+offset;
        ggExplore.angle=angle;ggExpTarget(&tx,&ty);
        interactionRequire(ggExpEditStatus(tx,ty,4,0)==GG_EDIT_READY,
            "valid diagonal aim is rejected at a tile boundary");
    }
    ggExplore.px=64*256+128;ggExplore.py=64*256+128;ggExplore.angle=0;
    interactionRequire(ggExpEditStatus(67,64,0,0)==GG_EDIT_UNCHANGED,
        "unchanged terrain is reported as a protected object");
    interactionRequire(ggExpEditStatus(67,64,-1,0)==GG_EDIT_MATERIAL&&
        ggExpEditStatus(67,64,GG_EXP_MATERIALS,0)==GG_EDIT_MATERIAL,
        "invalid material indices can escape the palette");
    interactionRequire(!ggExpEdit(75,64,4,0)&&
        ggExplore.editStatus==GG_EDIT_RANGE&&world[64][75]==T_GRASS,
        "remote edit reaches beyond the local construction tool");
    interactionRequire(!ggExpEdit(64,64,4,0)&&
        ggExplore.editStatus==GG_EDIT_SELF,
        "construction can remove the player's footing");
    ggExplore.px=64*256+250;ggExplore.py=64*256+128;
    bld[64][65]=B_TOWER;ggExplore.stepPhase=12;ggExplore.lastHeld=0;
    ggExpStep(KEY_UP,0,0);
    interactionRequire(ggExplore.px==64*256+250&&!ggExplore.stepPhase,
        "walking into a wall still shakes the camera");
    bld[64][65]=0;ggExplore.px=64*256+128;
    ggExpMeshEnabled=1;ggExpScenicQuality=2;ggExpBuildPreview=0;
    ggExplore.feedbackT=0;ggExplore.frame=0;
    ggExpRender(guarded.img);
    static u8 before[400*240*3];static uint16_t depth[400*240];
    memcpy(before,guarded.img,sizeof before);
    memcpy(depth,ggMeshDepthStorage,sizeof depth);
    ggExpBuildPreview=1;ggExpRender(guarded.img);
    int changed=0;
    for(int y=28;y<179;y++)for(int x=0;x<400;x++){
        int k=(y*400+x)*3;
        if(memcmp(before+k,guarded.img+k,3))changed++;
    }
    interactionRequire(changed>40&&changed<3000,
        "target outline is missing or floods the world with overlay pixels");
    interactionRequire(!memcmp(depth,ggMeshDepthStorage,sizeof depth),
        "translucent construction preview changes opaque world depth");
    interactionRequire(ggMeshContext.w==400&&ggMeshContext.scale==1,
        "interaction preview changed native New 3DS resolution");
    // Cover the complete world with an existing nearer depth surface:
    // the same ribbon must disappear instead of drawing through walls.
    memcpy(before,guarded.img,sizeof before);
    for(int i=0;i<400*240;i++)ggMeshDepthStorage[i]=65535;
    ggExpTargetOutline(67,64,GG_EDIT_READY);
    interactionRequire(!memcmp(before,guarded.img,sizeof before),
        "target outline ignores actual foreground occlusion");
    if(argc==2){
        ggExpRender(guarded.img);
        FILE *f=fopen(argv[1],"wb");if(!f)return 2;
        fprintf(f,"P6\n400 240\n255\n");
        int good=fwrite(guarded.img,1,sizeof guarded.img,f)==sizeof guarded.img;
        if(fclose(f)||!good)return 3;
    }
    printf("3DS INTERACTION OK: protected ray, precise reasons, native depth-safe preview (%d pixels), stable blocked camera\n",changed);
    return 0;
}
