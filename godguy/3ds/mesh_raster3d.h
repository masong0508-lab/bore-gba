// GodGuy 3DS: actual perspective triangle rasterizer with inverse-depth
// testing. 3DS-only; no textures, runtime allocations or second RGB buffer.
// Vertices are Q8 world XY and integer world-height Z. Backing stores are
// the existing 400x240 RGB screen and a fixed 187.5 KiB depth-only buffer.
// The same code runs with a 200x120 (Old 3DS) or native 400x240 viewport.
#ifndef GG_MESH_RASTER3D_H
#define GG_MESH_RASTER3D_H
#include <stdint.h>
#include <string.h>
#include "mesh_material3d.h"
enum { GG_MESH_MAX_W=400,GG_MESH_MAX_H=240,GG_MESH_NEAR=96 };
typedef struct { int x,y,z; } GgMeshVertex;
// GodGuy material-lighting contract. Geometry is real 3D and the
// directional sunlight is computed ONCE PER TRIANGLE, never per pixel.
// Height is expressed in smaller game-world units than XY; scale it for
// meaningful architectural, hillside and roof normal response.
static inline int ggMeshSunlight(GgMeshVertex a,GgMeshVertex b,
                                  GgMeshVertex c,int ambient){
    int64_t ux=(int64_t)b.x-a.x,uy=(int64_t)b.y-a.y,
            uz=((int64_t)b.z-a.z)*24;
    int64_t vx=(int64_t)c.x-a.x,vy=(int64_t)c.y-a.y,
            vz=((int64_t)c.z-a.z)*24;
    int64_t nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
    int64_t den=(nx<0?-nx:nx)+(ny<0?-ny:ny)+(nz<0?-nz:nz);
    if(den==0)return ambient;
    // Stable south-west sun. High, low and unlit faces keep distinct
    // values even with all textures disabled on an Old 3DS.
    int directional=(int)((nx*(-70)+ny*42+nz*112)/den);
    int shade=ambient+directional*3/5;
    return ggMeshClamp(shade,124,285);
}

typedef struct { int f,s,z,u,v; } GgMeshCameraVertex;
typedef struct { int x,y,iz,uiz,viz; } GgMeshProjected;
typedef struct {
    unsigned char *rgb;
    uint16_t *depth;
    int w,h,scale,eye,cx,cy,cosine,sine,near;
    int focal,vertical,horizon,triangles,fragments;
    int skyR,skyG,skyB;
    int material,materialSeed,textureSamples;
    int ambientLight;
    int texBaseU,texBaseV;
} GgMeshContext;
static uint16_t ggMeshDepthStorage[GG_MESH_MAX_W*GG_MESH_MAX_H];
static GgMeshContext ggMeshContext;
static inline int ggMeshClamp(int v,int a,int b){
    return v<a?a:v>b?b:v;
}
static void ggMeshBegin(GgMeshContext*m,unsigned char *rgb,
                        int quality,int px,int py,int eye,int cosine,int sine,
                        int skyR,int skyG,int skyB){
    m->rgb=rgb;m->depth=ggMeshDepthStorage;
    m->scale=quality>=2?1:2;m->w=400/m->scale;m->h=240/m->scale;
    m->cx=px;m->cy=py;m->eye=eye;
    m->cosine=cosine;m->sine=sine;m->near=GG_MESH_NEAR;
    m->focal=m->w*5/8;m->vertical=quality>=2?7:4;
    m->horizon=m->h*29/60;
    m->skyR=skyR;m->skyG=skyG;m->skyB=skyB;
    m->triangles=0;m->fragments=0;m->textureSamples=0;
    m->material=GG_MAT_NONE;m->materialSeed=0;
    m->ambientLight=212;
    m->texBaseU=m->texBaseV=0;
    memset(m->depth,0,(size_t)m->w*m->h*sizeof(uint16_t));
    // At low quality, directly write 2x2 pixels to the sole RGB screen.
    // Sky grading reuses the climate palette; no per-pixel atmosphere LUT.
    for(int y=0;y<m->h;y++){
        int r=ggMeshClamp(skyR+y/8,0,255);
        int g=ggMeshClamp(skyG+y/6,0,255);
        int b=ggMeshClamp(skyB+y/5,0,255);
        for(int x=0;x<m->w;x++){
            int stride=m->w==400?1:2;
            int index=((y*stride)*400+x*stride)*3;
            for(int dy=0;dy<stride;dy++){
                unsigned char *line=m->rgb+index+dy*400*3;
                for(int dx=0;dx<stride;dx++){
                    line[dx*3]=(unsigned char)r;
                    line[dx*3+1]=(unsigned char)g;
                    line[dx*3+2]=(unsigned char)b;
                }
            }
        }
    }
}
static inline GgMeshCameraVertex ggMeshCameraPoint(const GgMeshContext*m,
                                                    GgMeshVertex v){
    int dx=v.x-m->cx,dy=v.y-m->cy;
    GgMeshCameraVertex out={
        (dx*m->cosine+dy*m->sine)/4096,
        (-dx*m->sine+dy*m->cosine)/4096,
        v.z,0,0
    };
    return out;
}
static inline GgMeshProjected ggMeshProject(const GgMeshContext*m,
                                             GgMeshCameraVertex v){
    int f=v.f>m->near?v.f:m->near;
    GgMeshProjected p={
        m->w/2+v.s*m->focal/f,
        m->horizon+(m->eye-v.z)*m->vertical*256/f,
        65535/f,
        v.u*(65535/f),
        v.v*(65535/f)
    };
    return p;
}
static inline GgMeshCameraVertex ggMeshIntersectNear(GgMeshCameraVertex a,
                                                      GgMeshCameraVertex b,
                                                      int near){
    int d=b.f-a.f;
    int t=near-a.f;
    GgMeshCameraVertex result={near,a.s,a.z,a.u,a.v};
    if(d){
        result.s+=((int64_t)(b.s-a.s)*t)/d;
        result.z+=((int64_t)(b.z-a.z)*t)/d;
        result.u+=((int64_t)(b.u-a.u)*t)/d;
        result.v+=((int64_t)(b.v-a.v)*t)/d;
    }
    return result;
}
static void ggMeshRasterProjected(GgMeshContext*m,GgMeshProjected a,
                                  GgMeshProjected b,GgMeshProjected c,
                                  int r,int g,int bl){
    int x0=a.x,y0=a.y,x1=b.x,y1=b.y,x2=c.x,y2=c.y;
    int area=(x1-x0)*(y2-y0)-(y1-y0)*(x2-x0);
    if(!area)return;
    if(area<0){
        GgMeshProjected temp=b;b=c;c=temp;
        x1=b.x;y1=b.y;x2=c.x;y2=c.y;area=-area;
    }
    int left=ggMeshClamp(x0<x1?(x0<x2?x0:x2):(x1<x2?x1:x2),0,m->w-1);
    int right=ggMeshClamp(x0>x1?(x0>x2?x0:x2):(x1>x2?x1:x2),0,m->w-1);
    int top=ggMeshClamp(y0<y1?(y0<y2?y0:y2):(y1<y2?y1:y2),0,m->h-1);
    int bottom=ggMeshClamp(y0>y1?(y0>y2?y0:y2):(y1>y2?y1:y2),0,m->h-1);
    if(right<left||bottom<top||
       (x0<0&&x1<0&&x2<0)||
       (x0>=m->w&&x1>=m->w&&x2>=m->w)||
       (y0<0&&y1<0&&y2<0)||
       (y0>=m->h&&y1>=m->h&&y2>=m->h))return;
    int e0=(x2-x1)*(top-y1)-(y2-y1)*(left-x1);
    int e1=(x0-x2)*(top-y2)-(y0-y2)*(left-x2);
    int e2=(x1-x0)*(top-y0)-(y1-y0)*(left-x0);
    // A single reciprocal-depth plane, computed once per triangle.
    // Pixel loop only adds; it has no divisions, floats or heap use.
    int dx=((int64_t)(b.iz-a.iz)*(y2-y0)-
            (int64_t)(c.iz-a.iz)*(y1-y0))*4096/area;
    int dy=((int64_t)(c.iz-a.iz)*(x1-x0)-
            (int64_t)(b.iz-a.iz)*(x2-x0))*4096/area;
    int start=(int64_t)a.iz*4096+
              (int64_t)(left-x0)*dx+(int64_t)(top-y0)*dy;
    int doTexture=m->material!=GG_MAT_NONE&&m->scale==1;
    int dux=0,duy=0,dvx=0,dvy=0;
    int rowU=0,rowV=0;
    if(doTexture){
        // Perspective-correct u/z and v/z, interpolated incrementally.
        // Each material coordinate uses a small tile-local origin, so
        // 32-bit gradients stay bounded even at map edge coordinates.
        dux=(int)(((int64_t)(b.uiz-a.uiz)*(y2-y0)-
                     (int64_t)(c.uiz-a.uiz)*(y1-y0))*256/area);
        duy=(int)(((int64_t)(c.uiz-a.uiz)*(x1-x0)-
                     (int64_t)(b.uiz-a.uiz)*(x2-x0))*256/area);
        dvx=(int)(((int64_t)(b.viz-a.viz)*(y2-y0)-
                     (int64_t)(c.viz-a.viz)*(y1-y0))*256/area);
        dvy=(int)(((int64_t)(c.viz-a.viz)*(x1-x0)-
                     (int64_t)(b.viz-a.viz)*(x2-x0))*256/area);
        rowU=(int)((int64_t)a.uiz*256+
             (int64_t)(left-x0)*dux+(int64_t)(top-y0)*duy);
        rowV=(int)((int64_t)a.viz*256+
             (int64_t)(left-x0)*dvx+(int64_t)(top-y0)*dvy);
    }
    int rowE0=e0,rowE1=e1,rowE2=e2,rowIz=start;
    int stepX0=y1-y2,stepX1=y2-y0,stepX2=y0-y1;
    int stepY0=x2-x1,stepY1=x0-x2,stepY2=x1-x0;
    for(int y=top;y<=bottom;y++){
        int w0=rowE0,w1=rowE1,w2=rowE2,iz=rowIz;
        int uvX=rowU,uvY=rowV;
        for(int x=left;x<=right;x++){
            if(w0>=0&&w1>=0&&w2>=0){
                int inverse=(iz+2048)>>12;
                int slot=y*m->w+x;
                if(inverse>0&&inverse>m->depth[slot]){
                    m->depth[slot]=(uint16_t)inverse;
                    int tr=r,tg=g,tb=bl;
                    if(doTexture){
                        // All world texture coordinates come from the
                        // actual 3D vertices: moving the camera does
                        // not move mortar joints or wood grain.
                        int u=((uvX>>8)/inverse)+m->texBaseU;
                        int v=((uvY>>8)/inverse)+m->texBaseV;
                        ggMatSample(m->material,u,v,
                                    (uint32_t)m->materialSeed,
                                    &tr,&tg,&tb);
                        m->textureSamples++;
                    }
                    int scale=m->scale,index=((y*scale)*400+x*scale)*3;
                    for(int py=0;py<scale;py++){
                        unsigned char*dst=m->rgb+index+py*400*3;
                        for(int px=0;px<scale;px++){
                            dst[px*3]=(unsigned char)tr;
                            dst[px*3+1]=(unsigned char)tg;
                            dst[px*3+2]=(unsigned char)tb;
                        }
                    }
                    m->fragments++;
                }
            }
            w0+=stepX0;w1+=stepX1;w2+=stepX2;iz+=dx;
            uvX+=dux;uvY+=dvx;
        }
        rowE0+=stepY0;rowE1+=stepY1;rowE2+=stepY2;rowIz+=dy;
        rowU+=duy;rowV+=dvy;
    }
    m->triangles++;
}
static void ggMeshTriangle(GgMeshContext*m,GgMeshVertex a,
                           GgMeshVertex b,GgMeshVertex c,
                           int r,int g,int bl){
    if(m->material!=GG_MAT_NONE){
        int sun=ggMeshSunlight(a,b,c,m->ambientLight);
        r=ggMeshClamp(r*sun/256,0,255);
        g=ggMeshClamp(g*sun/256,0,255);
        bl=ggMeshClamp(bl*sun/256,0,255);
    }
    GgMeshCameraVertex points[3]={
        ggMeshCameraPoint(m,a),ggMeshCameraPoint(m,b),
        ggMeshCameraPoint(m,c)
    };
    if(m->material!=GG_MAT_NONE&&m->scale==1){
        // Use XY on floors and roofs, and horizontal-plus-height on
        // walls. Local 512-unit origins retain absolute UV continuity
        // across adjacent triangles while avoiding 32-bit overflow.
        int xx=a.x-b.x;if(xx<0)xx=-xx;
        int xx2=a.x-c.x;if(xx2<0)xx2=-xx2;
        int yy=a.y-b.y;if(yy<0)yy=-yy;
        int yy2=a.y-c.y;if(yy2<0)yy2=-yy2;
        int zz=a.z-b.z;if(zz<0)zz=-zz;
        int zz2=a.z-c.z;if(zz2<0)zz2=-zz2;
        int vertical=zz+zz2>8;
        int axis=xx+xx2>=yy+yy2?0:1;
        int us[3],vs[3];
        GgMeshVertex src[3]={a,b,c};
        for(int j=0;j<3;j++){
            us[j]=(axis==0||!vertical)?src[j].x:src[j].y;
            vs[j]=vertical?src[j].z*4:src[j].y;
        }
        int baseU=us[0],baseV=vs[0];
        for(int j=1;j<3;j++){
            if(us[j]<baseU)baseU=us[j];
            if(vs[j]<baseV)baseV=vs[j];
        }
        baseU=baseU&~511;baseV=baseV&~511;
        m->texBaseU=baseU/4;
        m->texBaseV=baseV/4;
        for(int j=0;j<3;j++){
            points[j].u=(us[j]-baseU)/4;
            points[j].v=(vs[j]-baseV)/4;
        }
    }
    GgMeshCameraVertex clipped[5];
    int n=0;
    for(int i=0;i<3;i++){
        GgMeshCameraVertex prev=points[(i+2)%3],cur=points[i];
        int prior=prev.f>=m->near,present=cur.f>=m->near;
        if(prior!=present&&n<5)
            clipped[n++]=ggMeshIntersectNear(prev,cur,m->near);
        if(present&&n<5)clipped[n++]=cur;
    }
    if(n<3)return;
    for(int i=1;i+1<n;i++)
        ggMeshRasterProjected(m,
            ggMeshProject(m,clipped[0]),
            ggMeshProject(m,clipped[i]),
            ggMeshProject(m,clipped[i+1]),
            ggMeshClamp(r,0,255),ggMeshClamp(g,0,255),
            ggMeshClamp(bl,0,255));
}
static inline GgMeshVertex ggMeshV(int x,int y,int z){
    GgMeshVertex v={x,y,z};return v;
}
static void ggMeshQuad(GgMeshContext*m,
                       GgMeshVertex a,GgMeshVertex b,
                       GgMeshVertex c,GgMeshVertex d,int r,int g,int bl){
    ggMeshTriangle(m,a,b,c,r,g,bl);
    ggMeshTriangle(m,a,c,d,r,g,bl);
}
static int ggMeshHidden(const GgMeshContext*m,int px,int py,int depth){
    if((unsigned)px>=400u||(unsigned)py>=240u||depth<=0)return 0;
    int x=px/m->scale,y=py/m->scale;
    if((unsigned)x>=(unsigned)m->w||(unsigned)y>=(unsigned)m->h)
        return 0;
    int threshold=65535/depth+2;
    return m->depth[y*m->w+x]>threshold;
}
#endif
