// Compile the actual New 3DS origin renderer without requiring libctru.
#define GG_TEST_NATIVE_ORIGIN
#include "3ds/main3ds.c"

static struct { u8 before[16],rgb[400*240*3],after[16]; } frame;
static u32 sceneHash(void){
    u32 h=2166136261u;
    for(int i=40*400*3;i<175*400*3;i++)h=(h^frame.rgb[i])*16777619u;
    return h;
}
static void check(int ok,const char *message){
    if(!ok){fprintf(stderr,"ORIGIN RENDER FAIL: %s\n",message);exit(1);}
}
static void checkSagaSceneryDepth(void){
    memset(world,T_GRASS,sizeof world);
    memset(bld,0,sizeof bld);
    ggExpSceneryEnabled=0;
    ggExpMeshEnabled=1;
    ggExpScenicQuality=2;
    ggExpBegin(65,65,0);
    ggSaga.mode=GG_SAGA_INTERLUDE;
    ggSaga.event=GG_AMBIENT_COAST;
    ggSaga.eventX=68;ggSaga.eventY=65;
    ggSaga.frame=0;
    int x,y;
    ggSagaCamera(0,0,&x,&y);
    const int cx=65*256+128,cy=65*256+128;
    ggSagaShot.angle=0;
    ggSagaShot.camera=(GgFilmCameraTrack){cx,cy,cx,cy};
    ggSagaShot.sidesResolved=ggSagaShot.plusValid=ggSagaShot.minusValid=1;
    ggSagaShot.plusX=x*256+128;ggSagaShot.plusY=y*256+384;
    ggSagaShot.minusX=x*256+128;ggSagaShot.minusY=y*256-128;
    world[y-2][x]=T_HILL;
    ggSagaCamera(0,0,&x,&y);
    ggExpRenderMode(frame.rgb,1);
    static u16 terrain[GG_MESH_MAX_W*GG_MESH_MAX_H];
    const size_t count=(size_t)ggMeshContext.w*ggMeshContext.h;
    memcpy(terrain,ggMeshContext.depth,count*sizeof *terrain);
    ggSagaDraw(frame.rgb);
    int nearer=0;
    for(size_t i=0;i<count;i++)nearer+=ggMeshContext.depth[i]>terrain[i];
    check(nearer>12,"the staged coastal wagon does not enter the terrain depth buffer");
}
int main(void){
    static const int phases[]={GG_ORIGIN_BIRTH,GG_ORIGIN_CHILD,GG_ORIGIN_YOUTH,GG_ORIGIN_AWAKEN};
    ggOrigin.active=1;
    ggOrigin.phase=GG_ORIGIN_AWAKEN;
    GgOriginMark falling=ggOriginSet(frame.rgb,GG_ORIGIN_AWAKEN,0);
    int feetPixel=((falling.feet-2)*400+falling.x)*3;
    u8 inAir[3];
    memcpy(inAir,frame.rgb+feetPixel,sizeof inAir);
    ggOriginDraw(frame.rgb);
    check(memcmp(inAir,frame.rgb+feetPixel,sizeof inAir)==0,
          "the falling stranger carries a floating contact strip between their boots");
    memset(uiBuf,0x7a,sizeof uiBuf);
    memset(frame.before,0x5a,sizeof frame.before);
    memset(frame.after,0x6b,sizeof frame.after);
    for(unsigned i=0;i<sizeof phases/sizeof *phases;i++){
        ggOrigin.phase=phases[i];
        ggOrigin.frame=0;
        ggOriginDraw(frame.rgb);
        u32 start=sceneHash();
        ggOrigin.frame=53;
        ggOriginDraw(frame.rgb);
        check(start!=sceneHash(),"a projected scene freezes as the camera moves");
        for(unsigned j=0;j<sizeof uiBuf;j++)
            check(uiBuf[j]==0x7a,"drawing the origin alters the shared game UI");
        for(int j=0;j<16;j++)
            check(frame.before[j]==0x5a&&frame.after[j]==0x6b,
                  "the origin writes past the RGB framebuffer");
    }
    checkSagaSceneryDepth();
    puts("ORIGIN RENDER OK: four moving scenes, grounded fall shadow, saga prop depth and bounded RGB output");
    return 0;
}
