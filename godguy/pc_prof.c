// pc_prof.c: per-tick and per-frame timing of a developed world.  gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_prof pc_prof.c -lm && ./pc_prof [planet] [ticks] [seed]
#define main ds_main
#include "source/main.c"
#undef main
#include <time.h>
#include <stdlib.h>
void shimInput(int f,int*h,int*tx,int*ty){ *h=0; *tx=0; *ty=0; }
void shimPresent(void){}
static double now(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
static int cmpd(const void*a,const void*b){ double x=*(const double*)a, y=*(const double*)b; return x<y?-1:x>y; }
int main(int argc,char**argv){
    int size=argc>1?atoi(argv[1]):192, ticks=argc>2?atoi(argv[2]):60000, seed=argc>3?atoi(argv[3]):777;
    mapPtr=(u16*)bgGetMapPtr(0); planetSize=size; rs=(u32)seed*2654435761u+5; makeSpecies(); makeItems(); newWorld(); monOn=1;
    double*dt=(double*)malloc(sizeof(double)*ticks); double tot=0;
    for(int t=1;t<=ticks;t++){ tick=t; year=t/1200; if(t%900==0){ power=(int)(rnd()%PW_N); usePower((int)(rnd()%WW),(int)(rnd()%WH),1); }
        double a=now(); simulate(); double b=now(); dt[t-1]=(b-a)*1000; tot+=dt[t-1]; }
    qsort(dt,ticks,sizeof(double),cmpd);
    int nk=0; for(int k=0;k<KN_N;k++) nk+=kd[k].on; int alive=0; for(int i=0;i<nCr;i++) alive+=cr[i].alive;
    printf("planet %d, %d ticks: %d creatures, %d villages, %d kingdoms\n",size,ticks,alive,nVilOn,nk);
    printf("tick ms: mean %.4f p50 %.4f p99 %.4f p99.9 %.4f max %.4f\n",tot/ticks,dt[ticks/2],dt[(int)(ticks*0.99)],dt[(int)(ticks*0.999)],dt[ticks-1]);
    { int bv=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop>vil[bv].pop) bv=v; camX=vil[bv].x*8-128; camY=vil[bv].y*8-96; if(camX<0) camX=0; if(camY<0) camY=0; paused=0; zBmp=0; view3d=0; menuOn=0; titleOn=0; page=0; winX=-99; refillWindow();
      int N=3000; double a,b;
      a=now(); for(int i=0;i<N;i++){ towerFire(); } b=now(); printf("FRAME towerFire: %.4f ms\n",(b-a)*1000/N);
      a=now(); for(int i=0;i<N;i++){ hud(); } b=now(); printf("FRAME hud: %.4f ms\n",(b-a)*1000/N);
      a=now(); for(int i=0;i<N;i++){ uiMinimap(4,17); } b=now(); printf("FRAME uiMinimap: %.4f ms\n",(b-a)*1000/N);
      a=now(); for(int i=0;i<N;i++){ tick++; simulate(); } b=now(); printf("FRAME simulate: %.4f ms\n",(b-a)*1000/N); }
    return 0; }
