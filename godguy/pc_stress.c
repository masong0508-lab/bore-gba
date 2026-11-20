// Drive the real gameplay loop through the desktop shim and time its phases.
#include <stdint.h>
#include <time.h>

enum Phase { frame, simulation, creatures, disasters, maintenance, compaction, notables, warIndex, draw, hudWork, sequencer, composite, mix, phaseCount };
static void measureBegin(enum Phase phase);
static void measureEnd(enum Phase phase);
#define GG_FRAME_BEGIN(phase) measureBegin(phase)
#define GG_FRAME_END(phase) measureEnd(phase)
#define main ds_main
#include "source/main.c"
#undef main

enum { warmup = 120, speed = 4, perVillage = 216 };
static int worldSize=256, frames=1200, sample=-1, seeded, hazards, panFrames;
static int startCount, startGroups, startTick, startDeaths, startX, startY;
static int mixedFrames, composedFrames, audioHq;
static uint64_t elapsed[phaseCount], started[phaseCount];
static uint64_t offloadedAudio;
static struct Sample { uint64_t ns[phaseCount]; } *samples;

static uint64_t cpuTime(void){
    struct timespec t;
    if(clock_gettime(CLOCK_THREAD_CPUTIME_ID,&t)!=0){ perror("CPU clock"); exit(1); }
    return (uint64_t)t.tv_sec*1000000000+t.tv_nsec;
}
static void measureBegin(enum Phase phase){
    if(sample>=0) started[phase]=cpuTime();
}
static void measureEnd(enum Phase phase){
    if(sample<0) return;
    elapsed[phase]+=cpuTime()-started[phase];
    if(phase==frame){
        elapsed[frame]-=offloadedAudio;
        memcpy(samples[sample].ns,elapsed,sizeof elapsed);
    }
}

static void stressWorld(void){
    // A shared 256-square scene, with larger versions for the wide-map ports.
    int columns=worldSize==256?8:worldSize==768?12:16;
    int rows=worldSize==256?6:10;
    int kingdoms[2]={-1,-1};
    rs=777;
    cLife=0;
    loadHook=NULL;
    planetSize=worldSize;
    newWorld();
    memset(world,T_GRASS,sizeof world);
    bioClear();
    atmoReset();
    for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){
        unsigned tile=hsh(x,y,777)&15;
        if(tile<2) world[y][x]=T_FOREST;
        else if(tile==2) world[y][x]=T_DIRT;
    }
    for(int row=0;row<rows;row++) for(int col=0;col<columns;col++){
        int side=(col+row)&1, race=side+1;
        int x=WW/2-9*(columns-1)+18*col, y=WH/2-9*(rows-1)+18*row;
        int v=vilFound(race,x,y);
        if(v<0){ fputs("could not stage a village\n",stderr); exit(1); }
        if(kingdoms[side]<0) kingdoms[side]=kdFound(v);
        else vilK[v]=(u8)(kingdoms[side]+1);
        int placed=0;
        for(int attempt=0;attempt<8192&&placed<perVillage;attempt++){
            int dx=(int)(rnd()%21)-10;
            int dy=(int)(rnd()%21)-10;
            if(dx*dx+dy*dy>100) continue;
            int i=addCr(race,x+dx,y+dy);
            if(i>=0){ cr[i].age=20; cr[i].food=115; placed++; }
        }
        if(placed!=perVillage){ fputs("could not stage a full army\n",stderr); exit(1); }
        vil[v].stock=350;
        vil[v].pop=(u16)placed;
    }
    for(int v=0;v<VIL_N;v++) if(vil[v].on){
        int nearest=-1, closest=WW*WW+WH*WH;
        for(int u=0;u<VIL_N;u++) if(vil[u].on&&vil[u].race!=vil[v].race){
            int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y, distance=dx*dx+dy*dy;
            if(distance<closest){ nearest=u; closest=distance; }
        }
        vil[v].foe=(u8)(nearest+1);
    }
    if(kingdoms[0]<0||kingdoms[1]<0){ fputs("could not stage both kingdoms\n",stderr); exit(1); }
    kdDeclareX(kingdoms[0],kingdoms[1],1);
    warMatrix();
    wgBuild();
    startCount=nCr;
    startGroups=wgN;
    paused=titleOn=menuOn=dlgOn=sceneOn=extrasView=storyOn=autoSaveOn=0;
    // This fixture enters a running world directly. Start its LCD routing
    // in gameplay too, before sampling stable frames.
    gOnTop=0;
    lcdMainOnBottom();
    speedMul=speed;
    year=tick=0;
    camX=WW*4-128;
    camY=WH*4-96;
    winX=-99;
    gmCrowd=400;
    musReq(GM_WAR);
}

static void scheduleHazards(int f){
    static const int kinds[]={HZ_TORNADO,HZ_HOLE,HZ_FIRE,HZ_DRAGON};
    if(f<4||f%100==0){
        int kind=kinds[f<4?f:(f/100)%4];
        int slot=hzAdd(kind,(camX>>3)+13+2*(f%4),(camY>>3)+10);
        if(slot>=0){
            hazards++;
            fxReq(FX_LIGHTNING);
        }
    }
}

static int orderTime(const void*a,const void*b){
    uint64_t x=*(const uint64_t*)a, y=*(const uint64_t*)b;
    return x<y?-1:x>y;
}
static int orderFrame(const void*a,const void*b){
    const struct Sample *x=a, *y=b;
    return orderTime(&x->ns[frame],&y->ns[frame]);
}
static unsigned hashBytes(unsigned hash,const void*data,size_t bytes){
    const unsigned char *p=data;
    while(bytes--) hash=(hash^*p++)*16777619u;
    return hash;
}
static unsigned hashWorldState(void){
    unsigned hash=2166136261u;
    hash=hashBytes(hash,&rs,sizeof rs);
    for(int y=0;y<WH;y++){
        hash=hashBytes(hash,world[y],WW);
        hash=hashBytes(hash,bld[y],WW);
        hash=hashBytes(hash,occ[y],(size_t)WW*sizeof occ[0][0]);
    }
    hash=hashBytes(hash,cr,(size_t)nCr*sizeof cr[0]);
    hash=hashBytes(hash,crId,(size_t)nCr*sizeof crId[0]);
    hash=hashBytes(hash,crFl,(size_t)nCr*sizeof crFl[0]);
    hash=hashBytes(hash,nb,(size_t)nNb*sizeof nb[0]);
    hash=hashBytes(hash,vil,sizeof vil);
    hash=hashBytes(hash,kd,sizeof kd);
    hash=hashBytes(hash,wg,sizeof wg);
    hash=hashBytes(hash,wga,sizeof wga);
    return hashBytes(hash,wgOf,(size_t)wgNc);
}
static void report(void){
    int ticks=tick-startTick;
    if(ticks!=frames*speed||startGroups==0||hazards==0||panFrames<frames/2||stDied==(u32)startDeaths){
        fprintf(stderr,"incomplete scene: ticks %d, groups %d, hazards %d, pan frames %d, deaths %u\n",
                ticks,startGroups,hazards,panFrames,stDied-(u32)startDeaths);
        exit(1);
    }
    int living=0;
    unsigned fingerprint=2166136261u;
    for(int i=0;i<nCr;i++) if(cr[i].alive){
        living++;
        fingerprint=(fingerprint^crId[i])*16777619u;
        fingerprint=(fingerprint^(cr[i].x+1024u*cr[i].y))*16777619u;
        fingerprint=(fingerprint^cr[i].hp)*16777619u;
    }
    printf("scene: %d x %d, %d frames, %d ticks at %dx, %d starting creatures, %d starting war groups\n",
           WW,WH,frames,ticks,speed,startCount,startGroups);
    printf("activity: %d scripted disasters started (including warmup), %d pan frames, %u deaths during sampling, %d alive at end, fingerprint %08x\n",
           hazards,panFrames,stDied-(u32)startDeaths,living,fingerprint);
    printf("world state: %08x\n",hashWorldState());
    printf("presentation: %d composed frames, %d frames mixed, mixer %s\n",
           composedFrames,mixedFrames,mixedFrames?(audioHq?"high quality":"basic"):"hardware proxy");
    static const char *const names[]={"total","simulation (4 ticks)","  creature turns & war","  weather & disasters","  256-tick maintenance",
                                     "    creature compaction","    notable bookkeeping","    war group rebuild","game draw/update","  HUD & minimap","audio sequencer",
                                     "software composition","software mixer"};
    uint64_t *sorted=malloc((size_t)frames*sizeof *sorted);
    if(!sorted){ fputs("cannot allocate report buffer\n",stderr); exit(1); }
    qsort(samples,(size_t)frames,sizeof *samples,orderFrame);
    int p99=(frames*99+99)/100-1;
    int tail=(frames+99)/100;
    int overBudget=0, tailMaintenance=0;
    for(int i=0;i<frames;i++){
        if(!samples[i].ns[frame]||!samples[i].ns[simulation]){
            fputs("a frame skipped the gameplay timing hooks\n",stderr);
            exit(1);
        }
        overBudget+=samples[i].ns[frame]>1000000000/60;
        if(i>=frames-tail&&samples[i].ns[maintenance]) tailMaintenance++;
    }
    printf("%-24s %8s %8s %8s %8s %9s\n","host CPU ms/frame","mean","p50","p99","max","top 1%*");
    for(int p=0;p<phaseCount;p++){
        uint64_t sum=0, tailSum=0;
        for(int i=0;i<frames;i++){
            sorted[i]=samples[i].ns[p];
            sum+=sorted[i];
            if(i>=frames-tail) tailSum+=sorted[i];
        }
        if(sum==0) continue;
        qsort(sorted,(size_t)frames,sizeof *sorted,orderTime);
        printf("%-24s %8.3f %8.3f %8.3f %8.3f %9.3f\n",names[p],
               sum/1e6/frames,sorted[(frames-1)/2]/1e6,sorted[p99]/1e6,sorted[frames-1]/1e6,tailSum/1e6/tail);
    }
    printf("* Mean phase cost in the %d slowest total frames, %d with maintenance. Host frames over 16.67 ms: %d/%d.\n",
           tail,tailMaintenance,overBudget,frames);
    free(sorted);
}

void shimInput(int f,int*held,int*tx,int*ty){
    static const int pan[]={KEY_RIGHT,KEY_DOWN,KEY_LEFT,KEY_UP};
    if(!seeded){ seeded=1; stressWorld(); }
    if(f==warmup){ startTick=tick; startDeaths=(int)stDied; startX=camX; startY=camY; }
    if(f>=warmup){
        if(camX!=startX||camY!=startY) panFrames++;
        startX=camX;
        startY=camY;
    }
    if(f==warmup+frames){ report(); exit(0); }
    sample=f>=warmup?f-warmup:-1;
    memset(elapsed,0,sizeof elapsed);
    offloadedAudio=0;
    scheduleHazards(f);
    *held=pan[(f/60)%4];
    *tx=*ty=0;
}

void shimPresent(void){
    static short audio[(SHIM_SR/60+1)*2];
    static int remainder;
    int count=(SHIM_SR+remainder)/60;
    remainder=(SHIM_SR+remainder)%60;
#ifdef GG_STRESS_SOFTWARE
    static u8 top[400*240*3], bottom[320*240*3];
    measureBegin(mix);
    shimMix(audio,count);
    measureEnd(mix);
    measureBegin(composite);
    shimRenderChanged(top,bottom,NULL,NULL);
    measureEnd(composite);
    if(sample>=0){ mixedFrames++; composedFrames++; }
#else
    // Advance the modeled DS sound channels, but exclude that desktop mixer
    // from the simulated ARM9 workload. The actual DS mixes in hardware.
    uint64_t start=sample>=0?cpuTime():0;
    shimMix(audio,count);
    if(sample>=0) offloadedAudio=cpuTime()-start;
#endif
}

int main(int argc,char**argv){
    if(argc>1) worldSize=atoi(argv[1]);
    if(argc>2) frames=atoi(argv[2]);
    if(argc>3) audioHq=atoi(argv[3]);
    if((worldSize!=256&&worldSize!=768&&worldSize!=1024)||worldSize>MAXD||frames<300||frames>20000||audioHq<0||audioHq>1){
        fputs("usage: pc_stress [256, 768, or 1024 within this port's map limit] [300..20000 frames] [HQ audio 0 or 1]\n",stderr);
        return 2;
    }
#ifdef SHIM_HQ
    shimHqOn=audioHq;
#endif
#ifdef GG_HQVOX
    gmVoxWarm();
#endif
    samples=calloc((size_t)frames,sizeof *samples);
    if(!samples){ fputs("cannot allocate samples\n",stderr); return 1; }
    return ds_main();
}
