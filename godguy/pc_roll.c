// GodGuy step 10f, the rolled count of the war groups (source/ggwar.h: wgSlice / wgCommit).
//   gcc -O2 -DPCTEST -w -o pc_roll pc_roll.c -lm && ./pc_roll        ends with ROLL OK
// 1. at every 256th tick the groups the rolled count writes are close to the exact count (wgBuild) of the same moment: the soldiers in all groups within 3%, no group more than 8% (or 4 soldiers) off,
//    the member lists name only soldiers of their own group, and the army averages are within 15% of the exact ones.
// 2. the exact path is untouched: built with -DGG_NOROLL the war has the state hash of step 10e (run_tests.sh compares the two builds' "state hash" lines to the saved value in war_roll_hash.txt).
// 3. a rolled war saved in the middle and loaded goes on exactly as the saved one (pc_far.c does this too, with the rolled count in use).
#define PCTEST 1
#define GG_NOCIVIL 1
#define GG_NOCAPTURE 1
#define main pc_war_main
#include "pc_war.c"
#undef main
static WarGrp rg[VIL_N]; static WarAvg ra[VIL_N]; static u32 rLen[VIL_N]; static u8 rOf[MAXC];
int main(void){
    int worst=0, worstAvg=0, bad=0, commits=0; long totR=0, totE=0;
    for(int seed=0;seed<2;seed++){
        stage(1000+seed); camera(seed); wgBuild();
        for(int t=1;t<=2048;t++){ tick=t; simulate();
            if((t&255)==0){ commits++;      // the rolled count has just been written (compact, then wgCommit)
                if(!(nCr>FAR_MIN)) continue;
                memcpy(rg,wg,sizeof wg); memcpy(ra,wga,sizeof wga); memcpy(rLen,wgLen,sizeof wgLen); memcpy(rOf,wgOf,nCr); int nc=wgNc;
                for(int v=0;v<VIL_N;v++) for(u32 k=0;k<rLen[v];k++){ int ci=wgMem[wgOff[v]+k]; if(ci>=nCr||!cr[ci].alive||rOf[ci]!=v+1) bad++; }
                wgBuild();      // the exact count of this moment (this also restarts the slices, as a save would)
                for(int v=0;v<VIL_N;v++){ totR+=rg[v].cnt; totE+=wg[v].cnt; int e=wg[v].cnt, r=rg[v].cnt, d=e>r?e-r:r-e; if(d>4&&d*100>8*(e>r?e:r)) { worst++; if(worst<6) printf("  tick %d group %d: rolled %d exact %d\n",t,v,r,e); }
                    if(e>20&&r>20){ int a1=ra[v].atk, a2=wga[v].atk, h1=ra[v].hp, h2=wga[v].hp; if((a1>a2?a1-a2:a2-a1)*100>15*(a2?a2:1)||(h1>h2?h1-h2:h2-h1)*100>15*(h2?h2:1)) worstAvg++; } } } } }
    printf("commits checked %d, soldiers in groups: rolled %ld, exact %ld (%.2f%% off), groups more than 8%% off %d, averages more than 15%% off %d, member list errors %d\n",commits,totR,totE,totE?100.0*(totR-totE)/totE:0.0,worst,worstAvg,bad);
    CHK(totE>0,"there were groups to compare"); CHK(totR*100>=totE*97&&totR*100<=totE*103,"soldiers in groups within 3% of the exact count"); CHK(worst==0,"no group more than 8% off"); CHK(worstAvg==0,"army averages within 15%"); CHK(bad==0,"member lists name only their own soldiers");
    printf(fails?"ROLL FAILED (%d)\n":"ROLL OK\n",fails); return fails?1:0; }
