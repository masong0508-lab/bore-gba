// Source-integrated regression for the restored animated title/menu globes.
// Run once as DS, DSi16, BIG and as native 3DS using the same game source.
// gcc -O1 -std=c11 -DNDSSHIM -Icompat -Isource -include unistd.h
//     pc_title_globe.c -lm -o /tmp/gg-title && /tmp/gg-title
#define main ggTitleLegacyMain
#include "source/main.c"
#undef main
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static int titleFailures;
#define TITLE_CHECK(ok,why) do{if(!(ok)){fprintf(stderr,"TITLE GLOBE FAIL: %s\n",why);titleFailures++;}}while(0)
void shimInput(int f,int*h,int*tx,int*ty){(void)f;*h=*tx=*ty=0;}
void shimPresent(void){}

static int titleGlobeColorCount(int cx,int cy,int radius){
    int sum=0;
    for(int y=cy-radius;y<=cy+radius;y++){
        if((unsigned)y>=192)continue;
        for(int x=cx-radius;x<=cx+radius;x++){
            if((unsigned)x>=256)continue;
            int idx=uiBuf[y*256+x];
            sum+=idx>=100&&idx<100+T_N*3;
        }
    }
    return sum;
}
static unsigned long long titleGlobeHash(int cx,int cy,int radius){
    unsigned long long sum=0;
    for(int y=cy-radius;y<=cy+radius;y++)
        for(int x=cx-radius;x<=cx+radius;x++)
            if((unsigned)x<256&&(unsigned)y<192)
                sum=sum*131+uiBuf[y*256+x];
    return sum;
}
static void titlePPM(const char*path){
    FILE*fp=fopen(path,"wb");
    if(!fp){perror(path);exit(2);}
    fprintf(fp,"P6\n256 192\n255\n");
    for(int i=0;i<256*192;i++){
        unsigned p=BG_PALETTE_SUB[uiBuf[i]];
        unsigned char rgb[3]={(unsigned char)(((p>>0)&31)*255/31),
                              (unsigned char)(((p>>5)&31)*255/31),
                              (unsigned char)(((p>>10)&31)*255/31)};
        fwrite(rgb,1,3,fp);
    }
    fclose(fp);
}
int main(int argc,char**argv){
    mapPtr=(u16*)bgGetMapPtr(0);
    rs=74717;
    makeSpecies();makeItems();planetSize=128;newWorld();
    // The checkerboard has recognizable continents at every longitude.
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++){
        int k=((x/10)+(y/12))%7;
        world[y][x]=(unsigned char)(k==0?T_WATER:k==1?T_DEEP:
                                    k==2?T_SNOW:k==3?T_HILL:
                                    k==4?T_FOREST:k==5?T_GRASS:T_MOUNT);
        bld[y][x]=0;
    }
    for(int y=35;y<55;y++)for(int x=30;x<48;x++)
        if((x+y)%7==0){world[y][x]=T_GRASS;bld[y][x]=B_HOUSE;}
    uiPalette();uiShadePal();
    memset(uiBuf,U_BG,sizeof uiBuf);
    tick=0;uiWorldPreview(128,96,43);
    int n=titleGlobeColorCount(128,96,43);
    TITLE_CHECK(n>2000,"spherical preview did not paint enough actual world tiles");
    TITLE_CHECK(uiBuf[0]==U_BG&&uiBuf[191*256+255]==U_BG,
                "sphere drew outside its safe menu region");
    TITLE_CHECK(uiBuf[96*256+128]>=100&&
                uiBuf[96*256+128]<100+T_N*3,
                "the centre of the planet is not shaded terrain");
    TITLE_CHECK(uiBuf[(96+43)*256+(128+43)]==U_BG,
                "the planet still has rectangular corners");
    unsigned long long spin0=titleGlobeHash(128,96,43);
    tick=476;memset(uiBuf,U_BG,sizeof uiBuf);uiWorldPreview(128,96,43);
    TITLE_CHECK(spin0!=titleGlobeHash(128,96,43),
                "globe no longer rotates with title/menu time");
    titleOn=1;mScr=0;mSel=0;uiTitle();
    TITLE_CHECK(titleGlobeColorCount(206,128,34)>900,
                "title main menu lost its 3D globe");
    if(argc>1)titlePPM(argv[1]);
    mScr=1;mSel=0;uiTitle();
    TITLE_CHECK(titleGlobeColorCount(215,108,30)>600,
                "title options submenu lost its globe");
    mScr=2;mSel=0;uiTitle();
    TITLE_CHECK(titleGlobeColorCount(215,108,30)>600,
                "title extras submenu lost its globe");
    mScr=3;cSel=0;cLand=42;cSeed=111;uiTitle();
    TITLE_CHECK(titleGlobeColorCount(208,63,38)>1200,
                "Create Planet is still a flat square map");
    uiLoadHook("BUILDING WORLD",63);
    TITLE_CHECK(titleGlobeColorCount(128,76,35)>800,
                "loading screen lost its lit globe");
    wTab=0;wTabPrev=0;wSel=0;uiWorld();
    TITLE_CHECK(titleGlobeColorCount(243,46,9)>40,
                "WORLD settings page no longer has its unobtrusive planet");
#ifdef GG_HAS_GLOBE
    puts("3DS title/menu hemisphere and world sphere available");
#else
    puts("DS/DSi title/menu hemisphere restored without gameplay sphere RAM");
#endif
    puts(titleFailures?"TITLE GLOBE FAILED":"TITLE GLOBE OK");
    return titleFailures?1:0;
}
