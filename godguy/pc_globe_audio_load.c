// Profile the 3DS globe under rotating/zooming coastal-town weather with
// real music sequencing and the software mixer enabled. Also exercise two
// simulation ticks per picture. Reports host CPU work only, not hardware FPS.
// gcc -O2 -DNDSSHIM -D__3DS__ -DSHIM_HQ -DGG_HQVOX -Icompat -Isource \\
//    -include unistd.h pc_globe_audio_load.c -lm -o /tmp/gg_globe_load
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#define main ds_main
#include "source/main.c"
#undef main

enum { BENCH_FRAMES=360 };
static uint64_t frameNs[BENCH_FRAMES], globeNs[BENCH_FRAMES];
static short audio[(SHIM_SR/60+2)*2];
static int problems;
void shimInput(int f,int*h,int*tx,int*ty){(void)f;*h=*tx=*ty=0;}
void shimPresent(void){}
static uint64_t cpuNs(void){
    struct timespec t;clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t);
    return (uint64_t)t.tv_sec*1000000000u+(uint64_t)t.tv_nsec;
}
static int cmpU64(const void*a,const void*b){
    uint64_t x=*(const uint64_t*)a,y=*(const uint64_t*)b;
    return x<y?-1:x>y?1:0;
}
static void report(const char*which,uint64_t*a){
    uint64_t sum=0,tmp[BENCH_FRAMES];
    for(int i=0;i<BENCH_FRAMES;i++){sum+=a[i];tmp[i]=a[i];}
    qsort(tmp,BENCH_FRAMES,sizeof(tmp[0]),cmpU64);
    printf("coast %s: CPU mean %.3f ms p50 %.3f ms p99 %.3f ms max %.3f ms\n",
         which,sum/(BENCH_FRAMES*1.e6),tmp[BENCH_FRAMES/2]/1.e6,
         tmp[(BENCH_FRAMES*99+99)/100-1]/1.e6,tmp[BENCH_FRAMES-1]/1.e6);
}
static void townCoast(void){
    mapPtr=(u16*)bgGetMapPtr(0);rs=71971; cLife=0;
    planetSize=192;makeSpecies();makeItems();newWorld();
    // A wavy coastline with inland forests, hills and mountains plus
    // offshore shallows. Use the same terrain IDs read by the live renderer.
    for(int y=0;y<WH;y++){
        int shore=94+((int)(hsh(y/6,11,8)%19)-9);
        for(int x=0;x<WW;x++){
            int land=x<shore;
            int t;
            if(!land)t=x>shore+11?T_DEEP:T_WATER;
            else if(x>shore-4)t=T_SAND;
            else if(x>shore-15)t=T_GRASS;
            else {
                int noise=(int)(hsh(x/6,y/6,44)&15);
                t=noise==0?T_MOUNT:noise<4?T_HILL:noise<8?T_FOREST:T_GRASS;
            }
            world[y][x]=(u8)t;
            bld[y][x]=0;bldv[y][x]=0;
        }
    }
    int homes=0,folk=0;
    for(int v=0;v<18;v++){
        int y=10+v*9, x=58+(v%5)*6;
        for(int k=0;k<13;k++){
            int xx=x+(k%5)-2, yy=y+(k/5)-1;
            if(xx>=0&&yy>=0&&xx<WW&&yy<WH&&world[yy][xx]>T_WATER){
                bld[yy][xx]=B_HOUSE;homes++;
            }
        }
        for(int k=0;k<65;k++){
            int xx=x+(int)(rnd()%13)-6,yy=y+(int)(rnd()%13)-6;
            if(addCr(v&7,xx,yy)>=0)folk++;
        }
    }
    printf("coast setup: %d buildings, %d creatures, 18 coastal settlements, rain and clouds\n",homes,folk);
    if(homes<150||folk<300){fputs("populated coastline setup failed\n",stderr);problems++;}
    paused=0;view3d=1;gClouds=1;weather=WX_RAIN;tick=1000;zoom3=65;
    faceTile(91,94,&rotX,&rotY);
    soundEnable();gmTables();
    gpId=soundPlayPSG(DutyCycle_50,440,0,64);
    gnId=soundPlayNoise(8000,0,64);
    musReq(GM_SPHERE);musUpdate();
    globeEnter();
}
static void bench(int quality){
    gg3dsGlobeQuality=quality;shimHqOn=quality; // New 3DS HQ / Old 3DS basic
    uint64_t startTick=(uint64_t)tick;
    int uploads0=0; (void)uploads0;
    for(int f=0;f<BENCH_FRAMES;f++){
        uint64_t begin=cpuNs();
        tick++;
        shimMusicTicks=1;
        musUpdate();
        simulate();tick++;simulate();
        zoom3=(f/90==0)?30:(f/90==1)?50:(f/90==2)?75:100;
        rotY=(rotY+3)%360;
        rotX=(f%120<60)?18:30;
        if((f%45)==0)weather=(weather==WX_RAIN)?WX_STORM:WX_RAIN;
        uint64_t beforeGlobe=cpuNs();
        int drawn=globeDraw(1);
        if(drawn)globeFlush();
        globeNs[f]=cpuNs()-beforeGlobe;
        shimMix(audio,SHIM_SR/60);
        frameNs[f]=cpuNs()-begin;
        if(!drawn){fputs("active rotating globe skipped redraw\n",stderr);problems++;break;}
        if(f%60==0){int x,y;if(!globePick(128,96,&x,&y)){fputs("lost centre picking\n",stderr);problems++;}}
    }
    printf("coast %s: %d game ticks in %d rendering frames, music tick now %d\n",
          quality?"New 3DS fine/HQ":"Old 3DS regular/basic",tick-(int)startTick,BENCH_FRAMES,gmTick);
    report(quality?"New 3DS render":"Old 3DS render",globeNs);
    report(quality?"New 3DS frame+music":"Old 3DS frame+music",frameNs);
}
int main(void){
    townCoast();
    for(int mode=0;mode<2;mode++)bench(mode);
    printf("COAST GLOBE+MUSIC %s\n",problems?"FAIL":"OK");
    return problems?1:0;
}
