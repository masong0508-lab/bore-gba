// GodGuy step 10e, the home lookup of the war groups (source/ggwar.h: wvBuild / wvNear).
//   gcc -O2 -DPCTEST -w -o pc_home pc_home.c -lm && ./pc_home        ends with HOME OK
// 1. wvNear gives, for every tile of the planet and every people, exactly the village vilNear gives (the same tie-breaks), on a war world and again after 600 ticks of war.
// 2. a block with more villages than WV_K is marked full and falls back to vilNear, and still gives the same answer.
// 3. counting the groups the new way and the old way (-DGG_SLOWHOME) leaves the same groups: the state hash of the war is the same (run_tests.sh builds this file both ways and compares the "state hash" line).
#define PCTEST 1
#define GG_NOCIVIL 1
#define GG_NOCAPTURE 1
#define main pc_war_main
#include "pc_war.c"
#undef main
static int compareAll(const char*what){
    int bad=0; long n=0; wvBuild();
    for(int race=0;race<8;race++) for(int y=0;y<gH;y+=1) for(int x=0;x<gW;x+=1){ int a=wvNear(race,x,y), b=vilNear(race,x,y,14); n++; if(a!=b){ if(bad<5) printf("  differs at race %d tile %d,%d: block %d, vilNear %d\n",race,x,y,a,b); bad++; } }
    printf("%s: %ld lookups, %d differ\n",what,n,bad); return bad; }
static int fullBlocks(void){ int f=0, nb=wvW*((gH+(1<<WV_SH)-1)>>WV_SH); for(int b=0;b<nb;b++) if(wvN[b]==255) f++; return f; }
int main(void){
    stage(1000); camera(0); wgBuild();
    CHK(compareAll("war world at the start")==0,"wvNear equals vilNear everywhere (start)");
    for(int t=1;t<=600;t++){ tick=t; simulate(); }
    CHK(compareAll("war world after 600 ticks")==0,"wvNear equals vilNear everywhere (after 600 ticks)");
    // a crowd of villages in one place: more than WV_K inside one block's reach (the slots are all taken on the war world, so the villages are written straight into vil[])
    memset(vil,0,sizeof vil); int made=0; for(int k=0;k<VIL_N&&k<24;k++){ vil[k].on=1; vil[k].race=(u8)(k%3); vil[k].x=(coord_t)(100+(k%6)*2); vil[k].y=(coord_t)(100+(k/6)*2); vil[k].foe=0; made++; }
    int bad=compareAll("crowded block"); int full=fullBlocks(); printf("villages %d, blocks marked full: %d\n",made,full);
    CHK(bad==0,"wvNear equals vilNear in and round a crowded block"); CHK(full>0,"a crowded block is marked full (the fallback is really tested)");
    for(int v=0;v<VIL_N;v++) vil[v].on=0; compareAll("no villages at all"); CHK(compareAll("no villages")==0,"wvNear equals vilNear with no villages");
    stage(1001); camera(0); wgBuild();
    // the groups are the same counted either way
    wgBuild(); static WarGrp w1[VIL_N]; static u8 of1[4096]; memcpy(w1,wg,sizeof wg); int nn=nCr<4096?nCr:4096; memcpy(of1,wgOf,nn);
    wvOn=0; { int h; for(int i=0;i<nn;i++){ int hh=0; int s=wgSoldier(&cr[i],&hh); CHK((s?hh+1:0)==of1[i],"a creature's group is the same counted the old way"); if(fails>5) break; } }
    printf(fails?"HOME FAILED (%d)\n":"HOME OK\n",fails); return fails?1:0; }
