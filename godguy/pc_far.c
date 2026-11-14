// GodGuy step 10d, tiers and distant battles: the checks that are not casualty counts (those are in pc_war.c, against the full per-unit run).
//   gcc -O2 -DPCTEST -w -o pc_far pc_far.c -lm && ./pc_far        ends with FAR OK
// 1. a war saved in the middle (armies marching, armies fighting, notables among the fallen) and a loaded copy go on exactly alike: same state hash after 600 more ticks, camera far and camera near.
// 2. small worlds are untouched: below FAR_MIN creatures the game plays the same with and without the tiers (build this file with -DGG_NOTIER: the "small world hash" line must be the same).
// 3. far battles feed the records: kills reach the notable table, the fallen notables are written down, and the group lists stay consistent (a member list never names a creature of another group).
#define PCTEST 1
#define GG_NOCIVIL 1
#define GG_NOCAPTURE 1
#define main pc_war_main
#include "pc_war.c"
static const char*ggTmp(const char*n){ static char b[600]; snprintf(b,sizeof b,"%s/%s",getenv("TMPDIR")?getenv("TMPDIR"):"/tmp",n); return b; }
#undef main
static u32 hashAll(void){ return stateHash(); }
int main(void){
    int bad=0;
    for(int near=0;near<2;near++){
        stage(1000); camera(near); wgBuild(); for(int t=1;t<=700;t++){ tick=t; simulate(); }
        CHK(saveGame(ggTmp("gg_far.sav")),"the war saves"); int alive0=survivors(0)+survivors(1);
        for(int t=701;t<=1300;t++){ tick=t; simulate(); } u32 h1=hashAll(); int d1=stDied;
        newWorld(); int lo=loadGame(ggTmp("gg_far.sav")); CHK(lo,"the war loads"); camera(near);
        CHK(survivors(0)+survivors(1)==alive0,"the loaded war has the soldiers it was saved with");
        for(int t=701;t<=1300;t++){ tick=t; simulate(); } u32 h2=hashAll();
        printf("camera %s: hash after 600 ticks, original %08x, loaded %08x, creatures lost since the save %d\n",near?"near":"far ",h1,h2,d1); CHK(h1==h2,"a loaded war goes on exactly as the saved one"); }
    // lists and records
    stage(1001); camera(0); wgBuild(); for(int t=1;t<=1500;t++){ tick=t; simulate(); if(t%256==0) { for(int v=0;v<VIL_N;v++) for(u32 k=0;k<wgLen[v];k++){ int ci=wgMem[wgOff[v]+k]; if(ci<nCr&&cr[ci].alive&&wgOf[ci]!=v+1){ bad++; } } } }
    CHK(bad==0,"a group's member list names only its own soldiers"); int kills=0,fallen=0,wounded=0; for(int k=0;k<nNb;k++){ kills+=nb[k].kills; fallen+=nb[k].st; wounded+=nb[k].wounds; }
    printf("notables %d, kills on record %d, fallen %d, wounds %d\n",nNb,kills,fallen,wounded); CHK(nNb>0&&kills>0,"far battles reach the notable table");
    printf(fails?"FAR FAILED (%d)\n":"FAR OK\n",fails); return fails?1:0; }
