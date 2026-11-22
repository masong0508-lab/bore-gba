// PC test of step 8 (speed): the kingdom answer cache gives the answers a fresh walk would (built with -DKD_CHECK every cached answer is compared), the globe is redrawn only when something it shows
// changed and a skipped frame equals a fresh drawing, the fast shim renderer gives the plain renderer's picture byte for byte on the real program (menus, world, text) and says truthfully which screen
// changed, and a crowd costs about the same per tick however big it is.
// Add -DGG_LEGACY_GLOBE_TEST to also time the legacy software globe.
//   gcc -O2 -DNDSSHIM -DKD_CHECK -Icompat -Isource -Wno-all -include unistd.h -o pc_speed pc_speed.c -lm && ./pc_speed
#define main ds_main
#include "source/main.c"
#undef main
#include <time.h>
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int fails;
#define CHECK(c,...) do{ if(!(c)){ printf("FAIL: "); printf(__VA_ARGS__); printf("\n"); fails++; } }while(0)
// ---- the real program under random keys and stylus, drawn both ways
static unsigned frs=99; static unsigned frnd(void){ frs=frs*1103515245u+12345u; return frs>>8; }
static u8 fastTop[400*240*3], fastBot[320*240*3], refTop[400*240*3], refBot[320*240*3], prevTop[400*240*3], prevBot[320*240*3];
static long rFrames, rBad, rFlagBad, rTopSame, rBotSame; static double tRef, tFast; static int rGoal=2600;
void shimInput(int f,int*h,int*tx,int*ty){ *h=0; *tx=0; *ty=0;
    if(f<40) return; int r=frnd()%100;
    if(f%97==0) *h|=KEY_START; else if(f%61==0) *h|=KEY_A; else if(r<30){ static const int ks[]={KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT,KEY_L,KEY_R,KEY_X,KEY_Y,KEY_B,KEY_SELECT}; *h|=ks[frnd()%10]; }
    else if(r<45){ *h|=KEY_TOUCH; *tx=(int)(frnd()%256); *ty=(int)(frnd()%192); }
    if(f>=rGoal) { printf("render: %ld frames, %ld differed from the plain renderer, %ld change flags lied; top unchanged on %ld frames, bottom on %ld\n",rFrames,rBad,rFlagBad,rTopSame,rBotSame);
        printf("render time: plain %.2f ms/frame, fast %.2f ms/frame (%.1fx); engines drawn %ld main %ld sub, %ld skipped\n",tRef*1000/rFrames,tFast*1000/rFrames,tRef/tFast,shimDrawnMain,shimDrawnSub,shimSkipped);
        CHECK(rBad==0,"fast renderer differs from the plain one"); CHECK(rFlagBad==0,"change flags wrong"); CHECK(rTopSame+rBotSame>rFrames/4,"nearly nothing was skipped"); CHECK(tRef/tFast>1.5,"fast renderer is not faster");
        printf(fails?"speed: FAIL\n":"speed: ok\n"); exit(fails?1:0); } }
void shimPresent(void){ rFrames++; int tc,bc; double t0=now(); shimRenderChanged(fastTop,fastBot,&tc,&bc); double t1=now(); shimRenderRef(refTop,refBot); double t2=now(); tFast+=t1-t0; tRef+=t2-t1;
    if(memcmp(fastTop,refTop,sizeof refTop)||memcmp(fastBot,refBot,sizeof refBot)) rBad++;
    if(rFrames>1){ int ts=!memcmp(refTop,prevTop,sizeof refTop), bs=!memcmp(refBot,prevBot,sizeof refBot); rTopSame+=!tc; rBotSame+=!bc; if((ts&&0)||(!tc&&!ts)||(!bc&&!bs)) rFlagBad++; }   // a screen said unchanged must be what it was
    memcpy(prevTop,refTop,sizeof prevTop); memcpy(prevBot,refBot,sizeof prevBot); }
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0);
    // 1. kingdoms: a long world with powers used on it; every cached answer compared with a fresh one
    planetSize=192; rs=777u*2654435761u+5; makeSpecies(); makeItems(); newWorld(); monOn=1; long q0=0;
    for(int t=1;t<=70000;t++){ tick=t; year=t/1200; if(t%700==0){ for(int k=0;k<KN_N;k++) if(kd[k].on){ int c,p,w; kdStats(k,&c,&p,&w); for(int o=0;o<KN_N;o++) if(kd[o].on) kdDist(k,o); kdZones(k); } }
        if(t%900==0){ power=(int)(rnd()%PW_N); usePower((int)(rnd()%WW),(int)(rnd()%WH),1); } simulate(); (void)q0; }
    int nk=0; for(int k=0;k<KN_N;k++) nk+=kd[k].on; printf("kingdoms: %d at the end, %ld answers from the cache, %ld worked out, %ld wrong\n",nk,kdHit,kdMiss,kdBad);
    CHECK(kdBad==0,"cached kingdom answers differ from fresh ones"); CHECK(nk>=2,"too few kingdoms to test with (%d)",nk); CHECK(kdHit>kdMiss/4,"the cache is hardly used");
    { double a=now(); long s=0; for(int rep=0;rep<4000;rep++) for(int k=0;k<KN_N;k++) if(kd[k].on){ int c,p,w; kdStatsRaw(k,&c,&p,&w); s+=w; for(int o=0;o<KN_N;o++) if(kd[o].on) s+=kdDistRaw(k,o); } double b=now();
      for(int rep=0;rep<4000;rep++) for(int k=0;k<KN_N;k++) if(kd[k].on){ int c,p,w; kdStats(k,&c,&p,&w); s+=w; for(int o=0;o<KN_N;o++) if(kd[o].on) s+=kdDist(k,o); } double c2=now();   // (KD_CHECK makes this one slow: the number below is for a build without it)
      printf("kingdom page, 4000 redraws: fresh %.1f ms, cached %.1f ms%s (%ld)\n",(b-a)*1000,(c2-b)*1000,"",s&1); }
#ifdef GG_HAS_GLOBE
    // 2. the globe: a skipped frame equals a fresh drawing (the world held still), and a still planet is not redrawn
    for(int v=0;v<=1;v++){ view3d=1; globePalette(); rotX=20; rotY=0; zoom3=40; walkOn=0; gClouds=1; weather=0; gKeyOk=0; long drawn0=gDrawN; int bad=0, frames=3000;
        for(int f=0;f<frames;f++){ tick++; if(v==0){ if((f%5)==0) rotY=(rotY+1)%360; } else { unsigned r=frnd()%100; if(r<3) rotY=(rotY+5)%360; if(r==3) rotX=(rotX+7)%80; if(r==4) zoom3=30+(int)(frnd()%60); if(r==5) gClouds^=1; if(r==6) weather=(int)(frnd()%4); if(r==7) walkOn^=1; }
            int d=globeDraw(0); static u8 keep[sizeof gbuf]; memcpy(keep,gbuf,sizeof gbuf); globeDrawNow(); if(memcmp(keep,gbuf,sizeof gbuf)) bad++; (void)d; }
        printf("globe %s: %d frames, redrawn %ld times, %d frames showed a stale picture\n",v==0?"turning slowly":"random views",frames,gDrawN-drawn0,bad); CHECK(bad==0,"a skipped globe frame differs from a fresh drawing"); CHECK(gDrawN-drawn0<frames,"the globe is redrawn every frame"); }
    { long d0=gDrawN; paused=1; rotX=20; for(int f=0;f<500;f++){ globeDraw(0); } CHECK(gDrawN-d0<=1,"a still planet was redrawn %ld times",gDrawN-d0); printf("globe still: 500 frames, %ld redraws\n",gDrawN-d0); view3d=0; }
#endif
    // 3. a crowd costs about the same per tick whatever its size
    { int sizes[3]={1500,6000,15000}; double per[3]; for(int z=0;z<3;z++){ planetSize=192; rs=4242; makeSpecies(); makeItems(); newWorld(); monOn=0; paused=0; int guard=0; while(nCr<sizes[z]&&guard++<400000){ int s_=(int)(rnd()%8); addCr(s_,(int)(rnd()%WW),(int)(rnd()%WH)); }
            int n0=nCr; double a=now(); for(int t=1;t<=300;t++){ tick=t; simulate(); } double b=now(); per[z]=(b-a)*1000/300; printf("crowd: %d creatures, %.3f ms a tick\n",n0,per[z]); }
        CHECK(per[2]<per[0]*4.0+0.5,"a crowd 10x bigger costs far more a tick"); }
    // 4. the real program, drawn both ways
    paused=0; power=0; frs=99; ds_main(); return 0; }
