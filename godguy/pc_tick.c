// Deterministic simulation fingerprint for performance regressions.
#define main ds_main
#include "source/main.c"
#undef main
#include <stdlib.h>
void shimInput(int f,int*h,int*tx,int*ty){ *h=*tx=*ty=0; }
void shimPresent(void){}
static unsigned long long mix(unsigned long long h,const void*data,size_t n){ const u8*p=data; while(n--){ h^=*p++; h*=1099511628211ULL; } return h; }
int main(int argc,char**argv){
    int seed=argc>1?atoi(argv[1]):777, ticks=argc>2?atoi(argv[2]):12000;
    mapPtr=(u16*)bgGetMapPtr(0); planetSize=128; rs=(u32)seed*2654435761u+5;
    makeSpecies(); makeItems(); newWorld(); monOn=1;
    // Keep the benchmark action sequence stable as the toolbox grows.
    enum { WORKLOAD_POWERS=79 };
    for(int t=1;t<=ticks;t++){ tick=t; year=t/1200; if(t%900==0){ power=(int)(rnd()%WORKLOAD_POWERS); usePower((int)(rnd()%WW),(int)(rnd()%WH),1); } simulate();
        if((t&31)==0){
            for(int v=0;v<VIL_N;v++) if(vil[v].on){ int cached=vilMood(v); joyStamp[v]=0; int fresh=vilMood(v); if(cached!=fresh){ fprintf(stderr,"stale village mood at tick %d, village %d\n",t,v); return 1; } }
            for(int a=0;a<8;a++){ int x=(t*37+a*13)%WW, y=(t*17+a*29)%WH, r=4+(t+a)%39, best=-1, bd=r*r+1;
                for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].race==a){ int dx=vil[v].x-x, dy=vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } }
                if(vilNear(a,x,y,r)!=best){ fprintf(stderr,"stale village index at tick %d, race %d\n",t,a); return 1; }
            }
        }
    }
    unsigned long long h=1469598103934665603ULL;
    h=mix(h,&rs,sizeof rs); h=mix(h,&nCr,sizeof nCr); h=mix(h,&nVilOn,sizeof nVilOn);
    h=mix(h,cr,nCr*sizeof cr[0]); h=mix(h,vil,sizeof vil);
    for(int y=0;y<WH;y++){ h=mix(h,world[y],WW); h=mix(h,occ[y],WW*sizeof occ[0][0]); }
    printf("%d %d: %016llx, %d creatures, %d villages\n",seed,ticks,(unsigned long long)h,nCr,nVilOn);
    if(ticks==12000&&VIL_N==64&&MAXC==30000){ unsigned long long want=seed==777?0x0e66d8cf8371a92bULL:seed==7?0xdbcd50bc616d360bULL:0;
        if(want&&h!=want){ fputs("simulation fingerprint changed\n",stderr); return 1; } }
    return 0;
}
