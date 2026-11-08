// round trip: project mesh vertices to the screen, then pick the pixel back to a map tile
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
int main(void){
    rs=5; makeSpecies(); makeItems(); newWorld();
    for(int sh=0;sh<3;sh++) for(int rot=0;rot<3;rot++){ shape3=sh; buildMesh(); rotX=rot==0?0:(rot==1?30:-50); rotY=rot*70+10; zoom3=40;
        float dist=zoom3/16.f, ra=rotY*0.0174533f, rb=rotX*0.0174533f, cy=cosf(ra), sy=sinf(ra), cx=cosf(rb), sx=sinf(rb);
        int n=0, ok=0; int worst=0;
        for(int j=1;j<MN;j+=3) for(int i=1;i<MN;i+=3){ int k=j*(MN+1)+i; float x=mvx[k]/4096.f,y=mvy[k]/4096.f,z=mvz[k]/4096.f, nx=mnx[k]/500.f,ny=mny[k]/500.f,nz=mnz[k]/500.f;
            float x1=cy*x+sy*z, z1=-sy*x+cy*z, y1=cx*y-sx*z1, z2=sx*y+cx*z1; float n1=cy*nx+sy*nz, nz1=-sy*nx+cy*nz, nz2=sx*ny+cx*nz1; (void)n1;
            if(nz2<=0.6f) continue;   // facing the camera
            float zz=z2-dist; int px=(int)(128+x1/(-zz)*184.4f), py=(int)(96-(y1/(-zz))*184.4f); int tx,ty; n++;
            if(pick3D(px,py,&tx,&ty)){ int ex=i*128/MN, ey=j*128/MN; int d=abs(tx-ex)>abs(ty-ey)?abs(tx-ex):abs(ty-ey); if(d>worst) worst=d; if(d<=3) ok++; } }
        printf("shape %d rot %d/%d: %d/%d within 3 tiles, worst %d\n",sh,rotX,rotY,ok,n,worst); }
    return 0; }
