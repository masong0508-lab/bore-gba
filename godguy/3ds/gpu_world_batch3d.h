// GodGuy New-series GPU migration: bounded tile/building geometry producer.
// Independent of citro3d and the simulation. The caller passes the SAME
// authoritative map arrays (or a small synthetic map in the GPU smoke app).
// Shader receives genuine 3D vertices; hardware performs projection, depth
// and triangle rasterization. No ray columns or screen-space CPU triangles.
// The probe's VBO is supplied by the caller: 8192 vertices = 192 KiB max.
#ifndef GG_GPU_WORLD_BATCH3D_H
#define GG_GPU_WORLD_BATCH3D_H
#include <stdint.h>
enum { GG_GPU_MAX_VERTS=8192, GG_GPU_RADIUS=7 };
typedef struct { float x,y,z,r,g,b; } GgGpuVertex;
typedef struct {
    GgGpuVertex *vertex;
    int capacity,count,tiles,buildings,trees,overflow;
    int eye,px,py;
} GgGpuBatch;
typedef struct { int x,y,z; } GgGpuPoint;
static inline GgGpuPoint ggGpuP(int x,int y,int z){
    GgGpuPoint p={x,y,z};return p;
}
static inline int ggGpuTerrainHeight(int t){
    static const uint8_t heights[13]={0,1,5,10,16,25,48,31,6,8,7,18,8};
    return (unsigned)t<13u?heights[t]:8;
}
static inline int ggGpuBuildingHeight(int b){
    static const uint8_t heights[13]={
        0,18,25,37,6,55,12,7,34,28,31,0,0
    };
    return (unsigned)b<13u?heights[b]:0;
}
static inline int ggGpuTile(const uint8_t *map,int stride,
                            int width,int height,int x,int y,int fallback){
    if((unsigned)x>=(unsigned)width||(unsigned)y>=(unsigned)height)
        return fallback;
    return map[y*stride+x];
}
static inline int ggGpuCorner(const uint8_t *terrain,int stride,
                              int width,int height,int x,int y){
    int sum=0,n=0;
    for(int dy=-1;dy<=0;dy++)for(int dx=-1;dx<=0;dx++){
        int xx=x+dx,yy=y+dy;
        if((unsigned)xx>=(unsigned)width||
           (unsigned)yy>=(unsigned)height)continue;
        sum+=ggGpuTerrainHeight(terrain[yy*stride+xx]);n++;
    }
    return n?sum/n:8;
}
// Match the existing software renderer's four center-fan triangle
// sectors exactly. The GPU backend must not regress mountain camera
// grounding, planted tree trunks or foundations on adjacent biomes.
static inline int ggGpuGroundAtQ8(const uint8_t*terrain,int stride,
                                   int width,int height,int wx,int wy){
    int tx=wx>>8,ty=wy>>8;
    if((unsigned)tx>=(unsigned)width||
       (unsigned)ty>=(unsigned)height)return 8;
    int a=ggGpuCorner(terrain,stride,width,height,tx,ty);
    int b=ggGpuCorner(terrain,stride,width,height,tx+1,ty);
    int c=ggGpuCorner(terrain,stride,width,height,tx+1,ty+1);
    int d=ggGpuCorner(terrain,stride,width,height,tx,ty+1);
    int center=ggGpuTerrainHeight(terrain[ty*stride+tx]);
    int u=(wx&255)-128,v=(wy&255)-128;
    int au=u<0?-u:u,av=v<0?-v:v;
    if(au>=av){
        if(u>=0)return (center*(128-u)*2+
                        b*(u-v)+c*(u+v)+128)/256;
        int n=-u;
        return (center*(128-n)*2+a*(n-v)+d*(n+v)+128)/256;
    }
    if(v>=0)return (center*(128-v)*2+d*(v-u)+c*(v+u)+128)/256;
    int n=-v;
    return (center*(128-n)*2+a*(n-u)+b*(n+u)+128)/256;
}
static inline int ggGpuFoundation(const uint8_t*terrain,int stride,
                                  int width,int height,int x0,int y0,
                                  int x1,int y1,int nominal){
    int result=nominal;
    int z=ggGpuGroundAtQ8(terrain,stride,width,height,x0,y0);
    if(z<result)result=z;
    z=ggGpuGroundAtQ8(terrain,stride,width,height,x1,y0);
    if(z<result)result=z;
    z=ggGpuGroundAtQ8(terrain,stride,width,height,x1,y1);
    if(z<result)result=z;
    z=ggGpuGroundAtQ8(terrain,stride,width,height,x0,y1);
    if(z<result)result=z;
    return result-1;
}
static inline void ggGpuAppend(GgGpuBatch *b,GgGpuPoint p,
                               int red,int green,int blue,float light){
    GgGpuVertex *v=&b->vertex[b->count++];
    // Right-handed camera: XY tiles become GPU X/Z, height becomes Y.
    // Moving forward in GodGuy (+worldX) enters camera -Z at yaw=0.
    v->x=(float)(p.y-b->py)/256.0f;
    v->y=(float)(p.z-b->eye)/20.0f;
    v->z=-(float)(p.x-b->px)/256.0f;
    v->r=(float)red*light/255.0f;
    v->g=(float)green*light/255.0f;
    v->b=(float)blue*light/255.0f;
}
static inline void ggGpuTriangle(GgGpuBatch*b,GgGpuPoint a,
                                 GgGpuPoint c,GgGpuPoint d,
                                 int r,int g,int bl,float light){
    if(b->count+3>b->capacity){b->overflow=1;return;}
    ggGpuAppend(b,a,r,g,bl,light);
    ggGpuAppend(b,c,r,g,bl,light);
    ggGpuAppend(b,d,r,g,bl,light);
}
static inline void ggGpuQuad(GgGpuBatch*b,GgGpuPoint a,
                             GgGpuPoint c,GgGpuPoint d,GgGpuPoint e,
                             int r,int g,int bl,float light){
    ggGpuTriangle(b,a,c,d,r,g,bl,light);
    ggGpuTriangle(b,a,d,e,r,g,bl,light);
}
static inline void ggGpuBox(GgGpuBatch*b,int x0,int y0,int x1,int y1,
                            int floor,int roof,int r,int g,int bl){
    if(roof<=floor)return;
    GgGpuPoint a=ggGpuP(x0,y0,floor),c=ggGpuP(x1,y0,floor),
        d=ggGpuP(x1,y1,floor),e=ggGpuP(x0,y1,floor);
    GgGpuPoint A=ggGpuP(x0,y0,roof),C=ggGpuP(x1,y0,roof),
        D=ggGpuP(x1,y1,roof),E=ggGpuP(x0,y1,roof);
    ggGpuQuad(b,a,c,C,A,r,g,bl,0.76f);
    ggGpuQuad(b,c,d,D,C,r,g,bl,0.65f);
    ggGpuQuad(b,d,e,E,D,r,g,bl,0.87f);
    ggGpuQuad(b,e,a,A,E,r,g,bl,0.95f);
    ggGpuQuad(b,A,C,D,E,r,g,bl,1.07f);
}
static inline void ggGpuTree(GgGpuBatch*b,int x,int y,int z,uint32_t hash){
    int tip=z+28+(int)(hash&7u);
    ggGpuBox(b,x-9,y-9,x+9,y+9,z,z+17,91,68,45);
    GgGpuPoint t=ggGpuP(x,y,tip);
    int reach=29;
    ggGpuTriangle(b,ggGpuP(x-reach,y-reach,z+13),
                   ggGpuP(x+reach,y-reach,z+13),t,48,111,66,0.90f);
    ggGpuTriangle(b,ggGpuP(x+reach,y-reach,z+13),
                   ggGpuP(x+reach,y+reach,z+13),t,48,111,66,0.80f);
    ggGpuTriangle(b,ggGpuP(x+reach,y+reach,z+13),
                   ggGpuP(x-reach,y+reach,z+13),t,48,111,66,0.96f);
    ggGpuTriangle(b,ggGpuP(x-reach,y+reach,z+13),
                   ggGpuP(x-reach,y-reach,z+13),t,48,111,66,0.85f);
    b->trees++;
}
// Live-world data contract. No mutable world state, heap calls, math
// library, textures, or GPU-specific calls here; safe for SDK-free tests.
static void ggGpuSceneBuild(GgGpuBatch*b,const uint8_t*terrain,
                            const uint8_t*build,int stride,
                            int width,int height,int camX,int camY,
                            int radius){
    b->count=b->tiles=b->buildings=b->trees=b->overflow=0;
    b->px=camX;b->py=camY;
    int tileX=camX>>8,tileY=camY>>8;
    b->eye=ggGpuGroundAtQ8(terrain,stride,width,height,
                            camX,camY)+13;
    if(radius>GG_GPU_RADIUS)radius=GG_GPU_RADIUS;
    if(radius<0)radius=0;
    // Nearest-ring-first selection: when a very dense city exhausts
    // the 192 KiB VBO, near doors/buildings survive. The old row-major
    // ordering silently used up vertices on a distant scan edge first.
    // Maximum 680 candidate probes at a seven-tile radius.
    for(int ring=0;ring<=radius;ring++)
      for(int y=tileY-ring;y<=tileY+ring;y++)
        for(int x=tileX-ring;x<=tileX+ring;x++){
            if(x!=tileX-ring&&x!=tileX+ring&&
               y!=tileY-ring&&y!=tileY+ring)continue;
            if((unsigned)x>=(unsigned)width||
               (unsigned)y>=(unsigned)height)continue;
            int t=ggGpuTile(terrain,stride,width,height,x,y,3);
            int mid=ggGpuTerrainHeight(t);
            int x0=x*256,y0=y*256,x1=x0+256,y1=y0+256;
            int northWest=ggGpuCorner(terrain,stride,width,height,x,y);
            int northEast=ggGpuCorner(terrain,stride,width,height,x+1,y);
            int southEast=ggGpuCorner(terrain,stride,width,height,x+1,y+1);
            int southWest=ggGpuCorner(terrain,stride,width,height,x,y+1);
            int r=92,g=125,bl=80;
            switch(t){
            case 0:case 1:r=40;g=105;bl=145;break;
            case 2:case 9:r=178;g=152;bl=100;break;
            case 4:case 11:r=50;g=94;bl=65;break;
            case 5:r=118;g=120;bl=86;break;
            case 6:r=115;g=114;bl=122;break;
            case 7:r=206;g=214;bl=220;break;
            case 8:r=184;g=64;bl=37;break;
            case 10:r=72;g=109;bl=98;break;
            case 12:r=137;g=111;bl=82;break;
            }
            // Four triangles (12 vertices) form one complete floor.
            // Never strand half a slope at the fixed VBO boundary.
            if(b->count+12>b->capacity){b->overflow=1;continue;}
            GgGpuPoint c=ggGpuP(x0+128,y0+128,mid);
            GgGpuPoint A=ggGpuP(x0,y0,northWest);
            GgGpuPoint B=ggGpuP(x1,y0,northEast);
            GgGpuPoint C=ggGpuP(x1,y1,southEast);
            GgGpuPoint D=ggGpuP(x0,y1,southWest);
            ggGpuTriangle(b,c,A,B,r,g,bl,0.99f);
            ggGpuTriangle(b,c,B,C,r,g,bl,0.87f);
            ggGpuTriangle(b,c,C,D,r,g,bl,0.93f);
            ggGpuTriangle(b,c,D,A,r,g,bl,1.06f);
            b->tiles++;
            int building=ggGpuTile(build,stride,width,height,x,y,0);
            if(building){
                // Box = 5 quads, roof = 2 quads: 42 vertices.
                // Reject the entire building at once if a city scene
                // exceeds its budget; never leave floating half-gables.
                if(b->count+42>b->capacity){b->overflow=1;continue;}
                int roof=mid+ggGpuBuildingHeight(building);
                int wood=building==1||building==2||building==4||building==7;
                int base=ggGpuFoundation(terrain,stride,width,height,
                                         x0+38,y0+38,x1-38,y1-38,mid);
                ggGpuBox(b,x0+38,y0+38,x1-38,y1-38,
                         base,roof,wood?155:146,wood?110:139,
                         wood?78:138);
                // Distinct pitched roofs; geometry stays aligned with its
                // supporting walls and GPU depth instead of floating.
                int ridge=(y0+y1)/2,peak=roof+(building==5?4:9);
                ggGpuQuad(b,ggGpuP(x0+29,y0+29,roof),
                           ggGpuP(x1-29,y0+29,roof),
                           ggGpuP(x1-29,ridge,peak),
                           ggGpuP(x0+29,ridge,peak),
                           wood?132:110,wood?74:103,wood?62:107,0.92f);
                ggGpuQuad(b,ggGpuP(x0+29,ridge,peak),
                           ggGpuP(x1-29,ridge,peak),
                           ggGpuP(x1-29,y1-29,roof),
                           ggGpuP(x0+29,y1-29,roof),
                           wood?132:110,wood?74:103,wood?62:107,0.76f);
                b->buildings++;
            }else if(t==4||t==11||t==3){
                uint32_t k=(uint32_t)x*0x9e3779b9u+
                           (uint32_t)y*0x85ebca6bu;
                k^=k>>16;k*=0x7feb352du;k^=k>>15;
                if((k&7u)==0u){
                    // Trunk (5 quads) + crown (4 triangles) = 42 verts.
                    if(b->count+42>b->capacity){b->overflow=1;continue;}
                    ggGpuTree(b,x0+92,y0+144,
                        ggGpuGroundAtQ8(terrain,stride,width,height,
                                        x0+92,y0+144)-1,k);
                }
            }
        }
}
#endif
