// GodGuy GPU migration probe: a separate New 3DS homebrew executable.
// This is a PICA200 geometry and depth test, not a replacement for
// GodGuy's playable software world or a standalone release of the game.
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gpu_world_batch3d.h"
#include "gpu_world_shader_data.h"
#define GG_GPU_MAP_W 48
#define GG_GPU_MAP_H 48
#define DISPLAY_TRANSFER_FLAGS \
    (GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | \
     GX_TRANSFER_RAW_COPY(0) | \
     GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
     GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
     GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))
static uint8_t probeTerrain[GG_GPU_MAP_W*GG_GPU_MAP_H];
static uint8_t probeBuild[GG_GPU_MAP_W*GG_GPU_MAP_H];
static GgGpuVertex *gpuVertices;
static GgGpuBatch batch;
static shaderProgram_s program;
static DVLB_s *shader;
static int uProjection,uModelView;
static int camX=24*256+128,camY=24*256+128;
static int angleDegrees=0;
static int radius=GG_GPU_RADIUS;
static void generateProbe(void){
    for(int y=0;y<GG_GPU_MAP_H;y++)
        for(int x=0;x<GG_GPU_MAP_W;x++){
            int idx=y*GG_GPU_MAP_W+x;
            int dx=x-24,dy=y-24;
            int outer=dx*dx+dy*dy;
            int t=3;
            if(outer>225)t=4;
            if(y<16)t=7;
            if(y>36)t=2;
            if(x>31&&y<29)t=5;
            if(x>39&&y<28)t=6;
            if((x+y)%17==0)t=12;
            probeTerrain[idx]=(uint8_t)t;
            if(x>=25&&x<=36&&y>=20&&y<=30&&
               ((x*13+y*7)%6==0))
                probeBuild[idx]=(uint8_t)((x+y)%4==0?5:
                                        (x+y)%3==0?3:2);
            else probeBuild[idx]=0;
        }
}
static void rebuild(void){
    ggGpuSceneBuild(&batch,probeTerrain,probeBuild,
                    GG_GPU_MAP_W,GG_GPU_MAP_W,GG_GPU_MAP_H,
                    camX,camY,radius);
    GSPGPU_FlushDataCache(gpuVertices,
                          (u32)(batch.count*sizeof(GgGpuVertex)));
}
static int initGpu(void){
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))return 0;
    gpuVertices=(GgGpuVertex*)linearAlloc(
        GG_GPU_MAX_VERTS*sizeof(GgGpuVertex));
    if(!gpuVertices){C3D_Fini();return 0;}
    batch.vertex=gpuVertices;batch.capacity=GG_GPU_MAX_VERTS;
    shader=DVLB_ParseFile((u32*)ggGpuShader,ggGpuShaderSize);
    if(!shader){linearFree(gpuVertices);gpuVertices=NULL;C3D_Fini();return 0;}
    shaderProgramInit(&program);
    shaderProgramSetVsh(&program,&shader->DVLE[0]);
    C3D_BindProgram(&program);
    uProjection=shaderInstanceGetUniformLocation(
        program.vertexShader,"projection");
    uModelView=shaderInstanceGetUniformLocation(
        program.vertexShader,"modelView");
    if(uProjection<0||uModelView<0){
        shaderProgramFree(&program);DVLB_Free(shader);
        linearFree(gpuVertices);gpuVertices=NULL;C3D_Fini();return 0;
    }
    C3D_AttrInfo *attrib=C3D_GetAttrInfo();
    AttrInfo_Init(attrib);
    AttrInfo_AddLoader(attrib,0,GPU_FLOAT,3);
    AttrInfo_AddLoader(attrib,1,GPU_FLOAT,3);
    C3D_BufInfo *buf=C3D_GetBufInfo();
    BufInfo_Init(buf);
    BufInfo_Add(buf,gpuVertices,sizeof(GgGpuVertex),2,0x10);
    C3D_TexEnv *env=C3D_GetTexEnv(0);
    C3D_TexEnvInit(env);
    C3D_TexEnvSrc(env,C3D_Both,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvFunc(env,C3D_Both,GPU_REPLACE);
    C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_ALL);
    C3D_CullFace(GPU_CULL_NONE);
    return 1;
}
static void cleanupGpu(void){
    shaderProgramFree(&program);
    DVLB_Free(shader);
    linearFree(gpuVertices);
    C3D_Fini();
}
int main(void){
    gfxInitDefault();
    consoleInit(GFX_BOTTOM,NULL);
    bool newSeries=false;
    if(R_FAILED(APT_CheckNew3DS(&newSeries))||!newSeries){
        puts("GodGuy PICA200 GPU preview\nNEW 3DS / NEW 2DS XL ONLY");
        puts("Original 3DS/2DS is unsupported.\nPress START to exit.");
        while(aptMainLoop()){
            hidScanInput();
            if(hidKeysDown()&(KEY_START|KEY_B))break;
            gspWaitForVBlank();
        }
        gfxExit();return 0;
    }
    osSetSpeedupEnable(true);
    if(!initGpu()){
        puts("GPU initialization failed.\nPress START to exit.");
        while(aptMainLoop()){
            hidScanInput();
            if(hidKeysDown()&(KEY_START|KEY_B))break;
            gspWaitForVBlank();
        }
        gfxExit();return 1;
    }
    C3D_RenderTarget *target=C3D_RenderTargetCreate(
        240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if(!target){
        cleanupGpu();gfxExit();return 1;
    }
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,
                             DISPLAY_TRANSFER_FLAGS);
    C3D_Mtx perspective;
    Mtx_PerspTilt(&perspective,C3D_AngleFromDegrees(73.0f),
                  C3D_AspectRatioTop,0.15f,24.0f,false);
    generateProbe();
    rebuild();
    int frames=0;
    puts("GODGUY / PICA200 WORLD LAB");
    puts("GPU perspective + depth test");
    puts("D-PAD L/R: rotate camera");
    puts("D-PAD U/D: move one tile");
    puts("A / X: detail radius");
    puts("START: exit to Homebrew Menu");
    puts("This is a separate GPU smoke app.");
    while(aptMainLoop()){
        hidScanInput();
        u32 pressed=hidKeysDown();
        u32 held=hidKeysHeld();
        if(pressed&(KEY_START|KEY_B))break;
        if((held&KEY_LEFT)&&!(frames&1))angleDegrees=(angleDegrees+357)%360;
        if((held&KEY_RIGHT)&&!(frames&1))angleDegrees=(angleDegrees+3)%360;
        int move=(pressed&KEY_UP)?1:(pressed&KEY_DOWN)?-1:0;
        if(move){
            // Bounded grid walk: map data is the SAME array schema as
            // GodGuy's authoritative world, not an imported art scene.
            int heading=(angleDegrees+45)/90%4;
            int stepx=heading==0?1:heading==2?-1:0;
            int stepy=heading==1?1:heading==3?-1:0;
            int tx=(camX>>8)+move*stepx;
            int ty=(camY>>8)+move*stepy;
            if(tx>=1&&tx<GG_GPU_MAP_W-1&&
               ty>=1&&ty<GG_GPU_MAP_H-1){
                camX=tx*256+128;camY=ty*256+128;rebuild();
            }
        }
        if(pressed&KEY_A&&radius<GG_GPU_RADIUS){radius++;rebuild();}
        if(pressed&KEY_X&&radius>3){radius--;rebuild();}
        C3D_Mtx modelView;
        Mtx_Identity(&modelView);
        Mtx_RotateY(&modelView,
                    C3D_AngleFromDegrees((float)angleDegrees),true);
        if(C3D_FrameBegin(C3D_FRAME_SYNCDRAW)){
            C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x6288A7FF,0);
            C3D_FrameDrawOn(target);
            C3D_BindProgram(&program);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,uProjection,&perspective);
            C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER,uModelView,&modelView);
            C3D_DrawArrays(GPU_TRIANGLES,0,batch.count);
            C3D_FrameEnd(0);
        }
        if((frames++%40)==0){
            printf("\x1b[10;1HCells: %d  Buildings: %d  Trees: %d\x1b[K",
                   batch.tiles,batch.buildings,batch.trees);
            printf("\x1b[11;1HVertices: %d / %d  Overflow: %d\x1b[K",
                   batch.count,batch.capacity,batch.overflow);
            printf("\x1b[12;1HCPU: %.2f ms GPU: %.2f ms\x1b[K",
                   C3D_GetProcessingTime(),C3D_GetDrawingTime());
            printf("\x1b[13;1HR: %d  Camera: %d,%d angle %d\x1b[K",
                   radius,camX>>8,camY>>8,angleDegrees);
        }
    }
    cleanupGpu();
    gfxExit();
    return 0;
}
