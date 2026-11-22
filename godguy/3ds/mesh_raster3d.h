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
enum {
    GG_MESH_SUN_X=-70,GG_MESH_SUN_Y=42,GG_MESH_SUN_Z=30,
    GG_MESH_NORMAL_HEIGHT=24,GG_MESH_LIGHT_HEIGHT=8
};
typedef struct { int x,y,z; } GgMeshVertex;
typedef struct { int64_t x,y,z; } GgMeshNormal;
typedef struct { int r,g,b; } GgMeshRgb;
typedef struct {
    int cover,sun;
    GgMeshRgb sky,ground;
    GgMeshRgb axis[3][2];
} GgMeshDaylight;
// The wide cross product supplies polygon lighting and material UVs.
// Height uses separate game-world units from the Q8 horizontal axes.
static inline GgMeshNormal ggMeshFaceNormal(GgMeshVertex a,GgMeshVertex b,
                                           GgMeshVertex c){
    int64_t ux=(int64_t)b.x-a.x,uy=(int64_t)b.y-a.y,
            uz=((int64_t)b.z-a.z)*GG_MESH_NORMAL_HEIGHT;
    int64_t vx=(int64_t)c.x-a.x,vy=(int64_t)c.y-a.y,
            vz=((int64_t)c.z-a.z)*GG_MESH_NORMAL_HEIGHT;
    return (GgMeshNormal){uy*vz-uz*vy,uz*vx-ux*vz,ux*vy-uy*vx};
}
static inline int64_t ggMeshNormalAbs(int64_t n){return n<0?-n:n;}
static inline int ggMeshNormalLight(GgMeshNormal n,int ambient){
    int64_t den=ggMeshNormalAbs(n.x)+ggMeshNormalAbs(n.y)+
                ggMeshNormalAbs(n.z);
    if(den==0)return ambient;
    // Neutral lighting used by mesh callers without outdoor daylight.
    int directional=(int)((n.x*GG_MESH_SUN_X+n.y*GG_MESH_SUN_Y+n.z*112)/den);
    int shade=ambient+directional*3/5;
    return shade<124?124:shade>285?285:shade;
}
static inline int ggMeshSunlight(GgMeshVertex a,GgMeshVertex b,
                                  GgMeshVertex c,int ambient){
    return ggMeshNormalLight(ggMeshFaceNormal(a,b,c),ambient);
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
    GgMeshDaylight daylight;
    int opacity,blendedFragments;
    int depthBias; // scoped decal tolerance; depth storage remains physical
    int fogR,fogG,fogB,fogMax;
    int texBaseU,texBaseV,groundUV;
    int detailQuality; // independent of native 400x240 pixel geometry
} GgMeshContext;
static uint16_t ggMeshDepthStorage[GG_MESH_MAX_W*GG_MESH_MAX_H];
static GgMeshContext ggMeshContext;
static inline int ggMeshClamp(int v,int a,int b){
    return v<a?a:v>b?b:v;
}
#include "mesh_lighting3d.h"
#include "mesh_sky3d.h"
// Distance haze is computed per actual polygon using camera-space
// forward depth, never as a flat tinted rectangle on top of the HUD.
static inline int ggMeshFogAmount(int forward,int maxHaze){
    int d=forward-3*256;
    if(d<=0)return 0;
    int v=d*maxHaze/(8*256);
    return v>maxHaze?maxHaze:v;
}
static inline void ggMeshSetCamera(GgMeshContext*m,int quality,int px,int py,
                                   int eye,int cosine,int sine){
#ifdef GG_NEW3DS_ONLY
    // Preserve a 400x240 optical camera on New-series hardware even
    // when frame-clock pressure reduces vegetation/material detail.
    // A quality transition must not change perspective or pixel size.
    m->scale=1;
#else
    m->scale=quality>=2?1:2;
#endif
    m->detailQuality=quality;
    m->w=400/m->scale;m->h=240/m->scale;
    m->cx=px;m->cy=py;m->eye=eye;
    m->cosine=cosine;m->sine=sine;m->near=GG_MESH_NEAR;
    m->focal=m->w*5/8;
#ifdef GG_NEW3DS_ONLY
    m->vertical=7;
#else
    m->vertical=quality>=2?7:4;
#endif
    m->horizon=m->h*29/60;
}
static inline void ggMeshBeginDaylight(GgMeshContext*m,unsigned char *rgb,
                                       int quality,int px,int py,int eye,
                                       int cosine,int sine,
                                       int skyR,int skyG,int skyB,int cover){
    m->rgb=rgb;m->depth=ggMeshDepthStorage;
    ggMeshSetCamera(m,quality,px,py,eye,cosine,sine);
    m->skyR=skyR;m->skyG=skyG;m->skyB=skyB;
    m->triangles=0;m->fragments=0;m->textureSamples=0;
    m->material=GG_MAT_NONE;m->materialSeed=0;
    m->ambientLight=212;
    m->opacity=256;m->blendedFragments=0;m->depthBias=0;
    m->fogR=skyR;m->fogG=skyG;m->fogB=skyB;
    m->fogMax=150;
    m->texBaseU=m->texBaseV=m->groundUV=0;
    m->daylight.cover=-1;
    if(cover>=0)ggMeshPrepareDaylight(m,ggMeshClamp(cover,0,3));
    memset(m->depth,0,(size_t)m->w*m->h*sizeof(uint16_t));
    ggMeshSkyFill(m);
}
static inline void ggMeshBegin(GgMeshContext*m,unsigned char *rgb,
                               int quality,int px,int py,int eye,int cosine,int sine,
                               int skyR,int skyG,int skyB){
    ggMeshBeginDaylight(m,rgb,quality,px,py,eye,cosine,sine,skyR,skyG,skyB,-1);
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
static inline GgMeshProjected ggMeshProjectNear(const GgMeshContext*m,
                                                GgMeshCameraVertex a,
                                                GgMeshCameraVertex b){
    // Combine the two endpoint weights before dividing. Both windings
    // land on the same screen pixel and keep fractional height until
    // projection, where a whole height unit can span several pixels.
    int weightA=b.f-m->near,weightB=m->near-a.f;
    int span=b.f-a.f,iz=65535/m->near;
    int64_t side=(int64_t)a.s*weightA+(int64_t)b.s*weightB;
    int64_t height=(int64_t)(m->eye-a.z)*weightA+
                   (int64_t)(m->eye-b.z)*weightB;
    int64_t screenDivisor=(int64_t)span*m->near;
    GgMeshProjected p={
        m->w/2+(int)(side*m->focal/screenDivisor),
        m->horizon+(int)(height*m->vertical*256/screenDivisor),
        iz,
        (int)(((int64_t)a.u*weightA+(int64_t)b.u*weightB)*iz/span),
        (int)(((int64_t)a.v*weightA+(int64_t)b.v*weightB)*iz/span)
    };
    return p;
}
static void ggMeshRasterProjected(GgMeshContext*m,GgMeshProjected a,
                                  GgMeshProjected b,GgMeshProjected c,
                                  int r,int g,int bl,int detailGain){
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
    // A shared edge belongs to its top/left triangle only. Inclusive
    // edges on both faces made translucent shadow fans blend twice at
    // their diagonals, leaving dark spokes across an otherwise soft decal.
    // Edge thresholds are computed once; the pixel loop stays additive.
    int edge0=(y2<y1||(y2==y1&&x2>x1))?0:1;
    int edge1=(y0<y2||(y0==y2&&x0>x2))?0:1;
    int edge2=(y1<y0||(y1==y0&&x1>x0))?0:1;
    // Depth and UV planes extrapolate across the whole bounding box.
    // A thin face can exceed the signed range far outside its edges;
    // wrap these fixed-point accumulators and sample only covered pixels.
    uint32_t dx=(uint32_t)(((int64_t)(b.iz-a.iz)*(y2-y0)-
                          (int64_t)(c.iz-a.iz)*(y1-y0))*4096/area);
    uint32_t dy=(uint32_t)(((int64_t)(c.iz-a.iz)*(x1-x0)-
                          (int64_t)(b.iz-a.iz)*(x2-x0))*4096/area);
    uint32_t rowIz=(uint32_t)a.iz*4096u+
                   (uint32_t)(left-x0)*dx+(uint32_t)(top-y0)*dy;
    // Beyond six world tiles, real atmosphere dominates the detail.
    // Retain sunlit geometry but stop per-fragment divisions and hashing
    // on distant surfaces, preserving New-3DS audio and frame budget.
    int inverseMid=(a.iz+b.iz+c.iz)/3;
    int doTexture=m->material!=GG_MAT_NONE&&m->scale==1&&
                  m->detailQuality>=2&&
                  inverseMid>=65535/(6*256);
    uint32_t dux=0,duy=0,dvx=0,dvy=0;
    uint32_t rowU=0,rowV=0;
    if(doTexture){
        // Perspective-correct u/z and v/z, interpolated incrementally.
        // The tile-local origin keeps samples inside the visible face
        // in range even when UVs extrapolate outside the face.
        dux=(uint32_t)(((int64_t)(b.uiz-a.uiz)*(y2-y0)-
                     (int64_t)(c.uiz-a.uiz)*(y1-y0))*256/area);
        duy=(uint32_t)(((int64_t)(c.uiz-a.uiz)*(x1-x0)-
                     (int64_t)(b.uiz-a.uiz)*(x2-x0))*256/area);
        dvx=(uint32_t)(((int64_t)(b.viz-a.viz)*(y2-y0)-
                     (int64_t)(c.viz-a.viz)*(y1-y0))*256/area);
        dvy=(uint32_t)(((int64_t)(c.viz-a.viz)*(x1-x0)-
                     (int64_t)(b.viz-a.viz)*(x2-x0))*256/area);
        rowU=(uint32_t)a.uiz*256u+(uint32_t)(left-x0)*dux+(uint32_t)(top-y0)*duy;
        rowV=(uint32_t)a.viz*256u+(uint32_t)(left-x0)*dvx+(uint32_t)(top-y0)*dvy;
    }
    int rowE0=e0,rowE1=e1,rowE2=e2;
    int stepX0=y1-y2,stepX1=y2-y0,stepX2=y0-y1;
    int stepY0=x2-x1,stepY1=x0-x2,stepY2=x1-x0;
    for(int y=top;y<=bottom;y++){
        int w0=rowE0,w1=rowE1,w2=rowE2;
        uint32_t iz=rowIz,uvX=rowU,uvY=rowV;
        for(int x=left;x<=right;x++){
            if(w0>=edge0&&w1>=edge1&&w2>=edge2){
                int inverse=(int32_t)(iz+2048u)>>12;
                int slot=y*m->w+x;
                // A scoped tolerance lets a surface decal survive
                // quantization at a shared edge. Stored depth remains
                // unbiased for later actors and scenery.
                if(inverse>0&&inverse+m->depthBias>m->depth[slot]){
                    // A translucent contact decal shades geometry but
                    // is not itself solid occluding geometry. Writing its
                    // depth made subsequently drawn walls and people clip
                    // against an invisible shadow polygon.
                    if(m->opacity>=256)
                        m->depth[slot]=(uint16_t)inverse;
                    int tr=r,tg=g,tb=bl;
                    if(doTexture){
                        // All world texture coordinates come from the
                        // actual 3D vertices: moving the camera does
                        // not move mortar joints or wood grain.
                        int u=(((int32_t)uvX>>8)/inverse)+m->texBaseU;
                        int v=(((int32_t)uvY>>8)/inverse)+m->texBaseV;
                        ggMatSampleLit(m->material,u,v,
                                       (uint32_t)m->materialSeed,detailGain,
                                       &tr,&tg,&tb);
                        m->textureSamples++;
                    }
                    int scale=m->scale,index=((y*scale)*400+x*scale)*3;
                    for(int py=0;py<scale;py++){
                        unsigned char*dst=m->rgb+index+py*400*3;
                        for(int px=0;px<scale;px++){
                            if(m->opacity<256){
                                int a=ggMeshClamp(m->opacity,0,256);
                                dst[px*3]=(unsigned char)((dst[px*3]*(256-a)+tr*a)>>8);
                                dst[px*3+1]=(unsigned char)((dst[px*3+1]*(256-a)+tg*a)>>8);
                                dst[px*3+2]=(unsigned char)((dst[px*3+2]*(256-a)+tb*a)>>8);
                            }else{
                                dst[px*3]=(unsigned char)tr;
                                dst[px*3+1]=(unsigned char)tg;
                                dst[px*3+2]=(unsigned char)tb;
                            }
                        }
                    }
                    m->fragments++;
                    if(m->opacity<256)m->blendedFragments++;
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
    GgMeshCameraVertex points[3]={
        ggMeshCameraPoint(m,a),ggMeshCameraPoint(m,b),
        ggMeshCameraPoint(m,c)
    };
    if(points[0].f<m->near&&points[1].f<m->near&&points[2].f<m->near)return;
    GgMeshNormal normal={0,0,0};
    int detailGain=256;
    int daylight=m->daylight.cover>=0;
    if(m->material!=GG_MAT_NONE){
        normal=ggMeshFaceNormal(a,b,c);
        if(daylight){
            GgMeshRgb gain=ggMeshDaylightGain(m,normal);
            r=ggMeshClamp(r*gain.r/256,0,255);
            g=ggMeshClamp(g*gain.g/256,0,255);
            bl=ggMeshClamp(bl*gain.b/256,0,255);
            detailGain=ggMeshClamp(gain.g,110,256);
        }else{
            int sun=ggMeshNormalLight(normal,m->ambientLight);
            r=ggMeshClamp(r*sun/256,0,255);
            g=ggMeshClamp(g*sun/256,0,255);
            bl=ggMeshClamp(bl*sun/256,0,255);
        }
    }
    if(m->material!=GG_MAT_NONE&&m->scale==1&&
       m->detailQuality>=2){
        // Project the two halves of a plane from the same axes, even
        // when their longest edges differ. Roof U follows the ridge and
        // V runs down the slope. Near-vertical surfaces use actual height.
        int64_t nx=ggMeshNormalAbs(normal.x),ny=ggMeshNormalAbs(normal.y);
        int64_t nz=ggMeshNormalAbs(normal.z);
        int uAxis=0,vAxis=1,vSign=1;
        if(!m->groundUV){
            if(m->material==GG_MAT_ROOF&&nz){
                uAxis=nx>ny;
                vAxis=1-uAxis;
                vSign=(uAxis?normal.x:normal.y)<0?-1:1;
            }else if(nz<=(nx+ny)/3){
                uAxis=nx>ny;
                vAxis=2;
            }
        }
        int us[3],vs[3];
        GgMeshVertex src[3]={a,b,c};
        for(int j=0;j<3;j++){
            us[j]=uAxis?src[j].y:src[j].x;
            vs[j]=vAxis==2?src[j].z*4:(vAxis?src[j].y:src[j].x)*vSign;
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
    if(m->fogMax>0){
        int avg=(points[0].f+points[1].f+points[2].f)/3;
        int haze=ggMeshFogAmount(avg,m->fogMax);
        if(haze){
            r=(r*(256-haze)+m->fogR*haze)/256;
            g=(g*(256-haze)+m->fogG*haze)/256;
            bl=(bl*(256-haze)+m->fogB*haze)/256;
            if(daylight)detailGain=detailGain*(256-haze)/256;
        }
    }
    r=ggMeshClamp(r,0,255);g=ggMeshClamp(g,0,255);bl=ggMeshClamp(bl,0,255);
    if(points[0].f>=m->near&&points[1].f>=m->near&&points[2].f>=m->near){
        ggMeshRasterProjected(m,ggMeshProject(m,points[0]),
            ggMeshProject(m,points[1]),ggMeshProject(m,points[2]),
            r,g,bl,detailGain);
        return;
    }
    GgMeshProjected clipped[4];
    int n=0;
    for(int i=0;i<3;i++){
        GgMeshCameraVertex prev=points[(i+2)%3],cur=points[i];
        if((prev.f<m->near&&cur.f>m->near)||
           (cur.f<m->near&&prev.f>m->near))
            clipped[n++]=ggMeshProjectNear(m,prev,cur);
        if(cur.f>=m->near)clipped[n++]=ggMeshProject(m,cur);
    }
    for(int i=1;i+1<n;i++)
        ggMeshRasterProjected(m,clipped[0],clipped[i],clipped[i+1],
                             r,g,bl,detailGain);
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
