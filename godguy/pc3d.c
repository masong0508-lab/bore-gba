// PC preview of the 3D planets: the same mesh and texture as the DS build, drawn by a tiny painter's-algorithm rasteriser.
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static unsigned char img[256*192*3]; static float zb[256*192];
static void tri(float*p0,float*p1,float*p2,float*uv0,float*uv1,float*uv2,float*l0,float*l1,float*l2){
    float minx=fminf(p0[0],fminf(p1[0],p2[0])), maxx=fmaxf(p0[0],fmaxf(p1[0],p2[0])), miny=fminf(p0[1],fminf(p1[1],p2[1])), maxy=fmaxf(p0[1],fmaxf(p1[1],p2[1]));
    float d=(p1[1]-p2[1])*(p0[0]-p2[0])+(p2[0]-p1[0])*(p0[1]-p2[1]); if(fabsf(d)<1e-6f) return;
    for(int y=(int)floorf(miny);y<=(int)ceilf(maxy);y++) for(int x=(int)floorf(minx);x<=(int)ceilf(maxx);x++){ if(x<0||y<0||x>=256||y>=192) continue;
        float a=((p1[1]-p2[1])*(x-p2[0])+(p2[0]-p1[0])*(y-p2[1]))/d, b=((p2[1]-p0[1])*(x-p2[0])+(p0[0]-p2[0])*(y-p2[1]))/d, c=1-a-b; if(a<0||b<0||c<0) continue;
        float z=a*p0[2]+b*p1[2]+c*p2[2]; if(z>=zb[y*256+x]) continue; zb[y*256+x]=z;
        float u=a*uv0[0]+b*uv1[0]+c*uv2[0], v=a*uv0[1]+b*uv1[1]+c*uv2[1]; float L=a*l0[0]+b*l1[0]+c*l2[0]; int tx=((int)u)&127, ty=((int)v)&127; u16 t=tex[ty*128+tx];
        float lit=0.35f+0.65f*(L<0?0:L); int r=(int)((t&31)*8*lit), g=(int)(((t>>5)&31)*8*lit), bb=(int)(((t>>10)&31)*8*lit); img[(y*256+x)*3]=r>255?255:r; img[(y*256+x)*3+1]=g>255?255:g; img[(y*256+x)*3+2]=bb>255?255:bb; } }
int main(int argc,char**argv){
    rs=77*2654435761u+1; makeSpecies(); makeItems(); newWorld(); for(int t=0;t<3*2048;t++){ tick=t; simulate(); }
    int open; int tot=itCount(&open); printf("items %d, open at year %d: %d (tier %d)\n",tot,year,open,tierOpen); for(int i=0;i<nIt;i+=37){ char b[40]; itSay(b,i); printf("  %d %s\n",i,b); }
    int rx=20,ry=40; float dist=3.0f;
    for(int sh=0;sh<3;sh++){ shape3=sh; buildMesh(); fillTexture(); memset(img,0,sizeof img); for(int i=0;i<256*192;i++) zb[i]=1e9f;
        float cy=cosf(ry*0.0174533f), sy=sinf(ry*0.0174533f), cx=cosf(rx*0.0174533f), sx=sinf(rx*0.0174533f);
        static float P[(MN+1)*(MN+1)][3], Lg[(MN+1)*(MN+1)];
        for(int k=0;k<(MN+1)*(MN+1);k++){ float x=mvx[k]/4096.f, y=mvy[k]/4096.f, z=mvz[k]/4096.f, nx=mnx[k]/500.f, ny=mny[k]/500.f, nz=mnz[k]/500.f;
            float x1=cy*x+sy*z, z1=-sy*x+cy*z, y1=cx*y-sx*z1, z2=sx*y+cx*z1; float n1=cy*nx+sy*nz, nz1=-sy*nx+cy*nz, ny1=cx*ny-sx*nz1, nz2=sx*ny+cx*nz1;
            float zz=z2-dist; float f=200.f/(-zz); P[k][0]=128+x1*f*1.0f; P[k][1]=96-y1*f; P[k][2]=-zz; Lg[k]=-(n1*-0.55f+ny1*-0.35f+nz2*-0.75f); }
        for(int j=0;j<MN;j++) for(int i=0;i<MN;i++){ int k0=j*(MN+1)+i, k1=k0+1, k2=k0+MN+2, k3=k0+MN+1; float uv[4][2]={{(float)i*128/MN,(float)j*128/MN},{(float)(i+1)*128/MN,(float)j*128/MN},{(float)(i+1)*128/MN,(float)(j+1)*128/MN},{(float)i*128/MN,(float)(j+1)*128/MN}};
            tri(P[k0],P[k1],P[k2],uv[0],uv[1],uv[2],&Lg[k0],&Lg[k1],&Lg[k2]); tri(P[k0],P[k2],P[k3],uv[0],uv[2],uv[3],&Lg[k0],&Lg[k2],&Lg[k3]); }
        char fn[40]; sprintf(fn,"/tmp/gg_p%d.ppm",sh); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n256 192\n255\n"); fwrite(img,1,sizeof img,f); fclose(f); }
    return 0; }
