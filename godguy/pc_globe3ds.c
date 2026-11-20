// GodGuy 3DS globe regression: native 3DS-only palette banks, lighting,
// adaptive one-pixel detail, unchanged picking, and incremental VRAM upload.
// Host-side validation; no 3DS hardware is assumed.
// gcc -O2 -DNDSSHIM -D__3DS__ -DGG_GLOBE_TEST -Icompat -Isource \\
//   -include unistd.h pc_globe3ds.c -lm -o /tmp/gg_globe3ds && /tmp/gg_globe3ds
#define main ds_main
#include "source/main.c"
#undef main
#include <time.h>
#include <stdio.h>
static unsigned char oldImage[256*192];
static int failures;
#define VERIFY(c,why) do{if(!(c)){fprintf(stderr,"3DS GLOBE FAIL: %s\n",why);failures++;}}while(0)
void shimInput(int f,int*h,int*tx,int*ty){(void)f;*h=*tx=*ty=0;}
void shimPresent(void){}
static unsigned long long micros(void){
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (unsigned long long)t.tv_sec*1000000+(unsigned long long)t.tv_nsec/1000;
}
static int compareTiles(void){
    const u8*t=(const u8*)BG_TILE_RAM(1);
    for(int by=0;by<24;by++) for(int bx=0;bx<32;bx++) for(int py=0;py<8;py++){
        const u8*d=gbuf+(by*8+py)*256+bx*8;
        if(memcmp(t+(by*32+bx)*64+py*8,d,8))return 0;
    }
    return 1;
}
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0);
    rs=773; planetSize=192; makeSpecies();makeItems();newWorld();
    rotY=41;rotX=15; zoom3=80; gClouds=1;weather=WX_RAIN;tick=1032;
    gg3dsGlobeQuality=1;gg3GlobeUploaded=gg3GlobeSkipped=0;
    globeEnter();
    VERIFY(gg3GlobeUploaded==768,"first entry uploads entire 32x24 globe");
    VERIFY(compareTiles(),"new globe pixels exactly match VRAM");
    int start=gg3GlobeUploaded, skips=gg3GlobeSkipped;
    globeFlush();
    VERIFY(gg3GlobeUploaded==start&&gg3GlobeSkipped-skips==768,
        "unchanged 3DS globe does not rewrite 768 tiles");
    int special=0,lit=0,disc=0;for(int i=0;i<256*192;i++){
        u8 col=gbuf[i];
        if(col>=GP3_AIR&&col<GP3_HIGH+8)special++;
        if(col>=GP3_LIGHT&&col<GP3_LIGHT+8)lit++;
        if(col==GP3_SEA+2)disc++;
    }
    VERIFY(special>40,"expanded 3DS atmosphere/ocean/star palette is in use");
    int pickX=-1,pickY=-1;VERIFY(globePick(128,96,&pickX,&pickY),
        "touch picking still hits the globe centre");
    memcpy(oldImage,gbuf,sizeof oldImage);
    gg3dsGlobeQuality=0;globeDraw(1);
    int diff=0;for(int i=0;i<256*192;i++)diff+=(oldImage[i]!=gbuf[i]);
    VERIFY(diff>30,"New 3DS small globe uses visibly finer pixels");
    // The old-detail image must actually reach VRAM before testing the
    // reverse transition. Otherwise the cached signatures correctly match
    // the already-uploaded fine image and zero uploads are expected.
    globeFlush();
    VERIFY(compareTiles(),"old-detail image is uploaded before switching back");
    printf("3DS globe: %d palette detail pixels, %d fine-sampling differences, first 768 VRAM uploads then zero; picking (%d,%d)\n",
        special,diff,pickX,pickY);
    gg3dsGlobeQuality=1;globeDraw(1);
    gg3GlobeUploaded=0;gg3GlobeSkipped=0;globeFlush();
    VERIFY(compareTiles(),"fine globe after a mode change matches VRAM");
    VERIFY(gg3GlobeUploaded>0,"new pixels are uploaded after a mode change");
    int before=gg3GlobeUploaded;tick+=128;rotY+=10;globeDraw(1);globeFlush();
    VERIFY(compareTiles(),"rotating/animated globe has no stale VRAM");
    VERIFY(gg3GlobeUploaded>before,"rotating globe causes changed tiles");
    int diffLarge=0;zoom3=30;gg3dsGlobeQuality=0;globeDraw(1);
    memcpy(oldImage,gbuf,sizeof oldImage);
    gg3dsGlobeQuality=1;globeDraw(1);
    for(int i=0;i<256*192;i++)diffLarge+=(oldImage[i]!=gbuf[i]);
    VERIFY(diffLarge==0,"New and Old 3DS use the same bounded detail on large discs");
    // Quality changes must also invalidate the globe key even with an
    // otherwise motionless planet. They must be visible in the next frame.
    zoom3=80;gg3dsGlobeQuality=0;globeDraw(1);
    gg3dsGlobeQuality=1;
    VERIFY(globeDraw(0)==1,"adaptive quality change repaints a still planet");
    globeFlush();VERIFY(compareTiles(),"quality change flush matches rendered pixels");
    VERIFY(globeDraw(0)==0,"identical quality/angle/time skips a duplicate redraw");
    for(int mode=0;mode<2;mode++){
        gg3dsGlobeQuality=mode;zoom3=80;rotY=40;
        unsigned long long begin=micros();
        for(int n=0;n<48;n++){rotY=(rotY+3)%360;tick+=4;globeDraw(1);}
        unsigned long long end=micros();
        printf("3DS globe CPU render (%s): %.2f ms/frame on this host\n",
            mode?"New 3DS 1px":"Old 3DS 2px",(double)(end-begin)/48000.0);
    }
    printf(failures?"3DS GLOBE FAIL (%d)\n":"3DS GLOBE OK\n",failures);
    return failures?1:0;
}
