// Exercise the real sky painter at native and fallback resolutions.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "3ds/mesh_raster3d.h"
static struct {unsigned char pre[32],rgb[400*240*3],post[32];} image;
static unsigned char reference[400*240*3];
static void require(int good,const char *why){
    if(!good){fprintf(stderr,"SKY FAIL: %s\n",why);exit(1);}
}
static void begin(int quality,int x,int y,int cosine,int sine,int cover){
    ggMeshBeginDaylight(&ggMeshContext,image.rgb,quality,x,y,13,
                        cosine,sine,49,78,108,cover);
}
static int different(void){
    int count=0;
    for(int i=0;i<400*240*3;i+=3)
        count+=memcmp(image.rgb+i,reference+i,3)!=0;
    return count;
}
int main(int argc,char **argv){
    memset(image.pre,0x53,sizeof image.pre);
    memset(image.post,0x71,sizeof image.post);
    for(int quality=1;quality<=2;quality++){
        GgMeshContext *m=&ggMeshContext;
        begin(quality,0,0,4096,0,1);
        memcpy(reference,image.rgb,sizeof reference);
        ggMeshSkyWeather(m,0);
        require(different()>900,"visible weather absent from sky");
        memcpy(reference,image.rgb,sizeof reference);
        begin(quality,600,700,4096,0,1);
        ggMeshSkyWeather(m,0);
        require(!different(),"infinite sky jumps at map-cell boundaries");
        begin(quality,0,0,4096,0,1);
        ggMeshSkyWeather(m,131072);
        require(!different(),"wrapped wind does not preserve its sky phase");
        begin(quality,0,0,4096,0,1);
        ggMeshSkyWeather(m,2048);
        require(different()>500,"weather does not move over time");
        for(int i=0;i<m->w*m->h;i++)
            require(m->depth[i]==0,"sky writes into opaque world depth");
        begin(quality,0,0,0,4096,1);
        ggMeshSkyWeather(m,0);
        require(different()>900,"clouds follow the HUD instead of camera yaw");
        // Very oblique, partially clipped sun and cloud shapes must stay
        // within both physical framebuffer edges at either viewport scale.
        for(int cover=0;cover<4;cover++)for(int dir=0;dir<8;dir++){
            static const int cs[8]={4096,2896,0,-2896,-4096,-2896,0,2896};
            static const int sn[8]={0,2896,4096,2896,0,-2896,-4096,-2896};
            begin(quality,0,0,cs[dir],sn[dir],cover);
            ggMeshSkyWeather(m,dir*701);
            if(m->scale==2)for(int y=0;y<240;y+=2)for(int x=0;x<400;x+=2){
                unsigned char *p=image.rgb+(y*400+x)*3;
                require(!memcmp(p,p+3,3)&&!memcmp(p,p+1200,6),
                        "fallback sky produces mismatched 2x2 pixels");
            }
            for(int i=0;i<32;i++)
                require(image.pre[i]==0x53&&image.post[i]==0x71,
                        "clipped sky escaped the RGB framebuffer");
        }
        begin(quality,0,0,4096,0,1);
        ggMeshSkyWeather(m,40);
        ggMeshQuad(m,ggMeshV(512,-160,0),ggMeshV(512,160,0),
                     ggMeshV(512,160,30),ggMeshV(512,-160,30),
                     189,98,67);
        require(ggMeshHidden(m,200,116,768),
                "sky painting disrupted subsequent physical wall depth");
        require(image.rgb[(116*400+200)*3]==189,
                "sky remains over foreground world geometry");
    }
    if(argc==2){
        begin(2,0,0,-3784,1567,1);ggMeshSkyWeather(&ggMeshContext,700);
        FILE *f=fopen(argv[1],"wb");require(f!=NULL,"capture open");
        fprintf(f,"P6\n400 240\n255\n");
        require(fwrite(image.rgb,1,sizeof image.rgb,f)==sizeof image.rgb,"capture write");
        require(!fclose(f),"capture close");
    }
    puts("3DS SKY OK: moving camera-anchored clouds, continuous wind, clipped sun, native/2x coverage, unchanged world depth");
    return 0;
}
