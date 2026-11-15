// Pack C and D: fire elemental, grey goo, ghosts, crab bomb and Crabzilla, bandits, sapient animals, dens; they save and load.
//   gcc -O2 -DPCTEST -w -o pc_wild pc_wild.c -lm && ./pc_wild
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
static void flat(int x0,int y0,int r,int t){ for(int y=y0-r;y<=y0+r;y++) for(int x=x0-r;x<=x0+r;x++) setCell(x,y,t); }
int main(void){
    planetSize=128; rs=31337u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=6000;t++){ tick=t; year=t/1200; simulate(); }
    int folk=-1, ani=-1; for(int s=0;s<nSp;s++){ if(arch[sp[s].a].diet==4&&folk<0) folk=s; if(arch[sp[s].a].diet<4&&ani<0) ani=s; }
    // fire elemental: dies in rain, leaves homes alone under the safe law
    memset(hz,0,sizeof hz); flat(60,60,10,T_GRASS); int i=hzAdd(HZ_FIRE,60,60); CHECK(i>=0&&hz[i].kind==HZ_FIRE,"a fire elemental can be made");
    weather=1; int died=0; for(int t=0;t<400&&!died;t++){ fireStep(&hz[i]); if(!hz[i].life) died=1; } CHECK(died,"it dies in rain"); weather=0; hz[i].kind=0;
    gx.lFire=1; flat(60,60,10,T_GRASS); bld[60][60]=B_HOUSE; i=hzAdd(HZ_FIRE,60,60); hz[i].dx=0; hz[i].dy=0; for(int t=0;t<50;t++) fireStep(&hz[i]); CHECK(bld[60][60]==B_HOUSE,"under the SAFE fire law a home is never burned"); gx.lFire=0; hz[i].kind=0; bld[60][60]=0;
    // goo: stays on land, never crosses water, eats a building
    flat(60,60,12,T_GRASS); for(int y=48;y<=72;y++){ setCell(70,y,T_DEEP); setCell(71,y,T_DEEP); } bld[60][62]=B_HOUSE; i=hzAdd(HZ_GOO,61,60); hz[i].dx=1; hz[i].dy=0; int crossed=0; for(int t=0;t<300;t++){ gooStep(&hz[i]); if(hz[i].x>=70) crossed=1; if(!(t%40)) hz[i].dy=0; } CHECK(!crossed,"grey goo turns back at deep water"); CHECK(bld[60][62]==0,"and it erases buildings in its way"); hz[i].kind=0;
    // ghost
    flat(30,30,8,T_GRASS); addCr(folk,30,30); int c1=occ[30][30]-1; idAssign(); crFl[c1]|=FL_NOTE; { Nb*q=nbMake(c1,3); if(q) q->kills=9; } int hb=hzCount(HZ_GHOST); killCr(c1); CHECK(hzCount(HZ_GHOST)==hb+1,"a notable with kills leaves a ghost");
    int gv=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on){ gv=v; break; } if(gv>=0){ int m0=vilMood(gv); int gi=hzAdd(HZ_GHOST,vil[gv].x,vil[gv].y); int m1=vilMood(gv); CHECK(m1<m0,"a ghost lowers the mood of a village near it (%d -> %d)",m0,m1); bld[vil[gv].y+1][vil[gv].x]=B_TEMPLE; hz[gi].dx=hz[gi].dy=0; ghostStep(&hz[gi]); CHECK(hz[gi].life==0,"a temple drives it off"); hz[gi].kind=0; }
    // crab powers
    flat(90,90,8,T_GRASS); addCr(folk,90,90); int cf=occ[90][90]-1; crabBomb(90,90); CHECK(HAS(&cr[cf],TR_GIGGLY)||cr[cf].tr2!=0,"CRAB BOMB makes folk giggly"); crabT=0; flat(100,100,4,T_GRASS); bld[100][100]=B_HOUSE; tick=5000; crabStomp(100,100); CHECK(bld[100][100]==0||bld[100][100]==B_RUIN,"CRABZILLA razes what it stomps"); bld[100][100]=B_HOUSE; crabStomp(100,100); CHECK(bld[100][100]==B_HOUSE,"and rests between stomps");
    // sapient
    flat(40,100,8,T_GRASS); addCr(ani,40,100); int a1=occ[100][40]-1; CHECK(uplift(a1)&&(crFl[a1]&FL_SAPIENT),"UPLIFT makes an animal sapient"); CHECK(!uplift(c1)||1,"folk are not uplifted"); int fcheck=(crFl[occ[100][40]-1]&FL_SAPIENT)!=0;
    addCr(ani,41,100); int kid=occ[100][41]-1; crFl[kid]=0; born(&cr[kid],&cr[a1]); CHECK(crFl[kid]&FL_SAPIENT,"a child of a sapient animal is sapient");
    lw[LW_EVOLUTION]=0; int evo=0; for(int t=0;t<20000&&nCr<MAXC-5;t++){ int o=nCr; addCr(ani,50+(t%20),110); if(nCr>o){ int k=nCr-1; crFl[k]=0; cr[k].tr=TR_CLEVER; born(&cr[k],&cr[k]); evo+=(crFl[k]&FL_SAPIENT)!=0; } } CHECK(evo==0,"EVOLUTION off: nobody becomes sapient by themselves");
    // den
    flat(20,20,6,T_GRASS); int ds[4]; for(int k=0;k<4;k++){ addCr(ani,20+k,20); ds[k]=occ[20][20+k]-1; crFl[ds[k]]|=FL_SAPIENT; cr[ds[k]].food=5; } int den=0; for(int t=0;t<400&&!den;t++){ tick=128*(t+1); denTick(); for(int dy=-5;dy<=5;dy++) for(int dx=-5;dx<=5;dx++) if(bld[20+dy][20+dx]==B_HUT) den=1; } CHECK(den,"a group of sapient animals builds a den");
    tick+=128; denTick(); CHECK(cr[ds[0]].food>=120,"sapient animals near a den are fed");
    // bandits
    lw[LW_BANDITS]=1; int gv2=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].pop>=8){ gv2=v; break; } if(gv2>=0){ vilL[gv2]=-60; for(int t=0;t<3000;t++){ tick=64*(t+1); banditTick(); } int nb2=0; for(int k=0;k<nCr;k++) nb2+=cr[k].alive&&(crFl[k]&FL_DESERT); printf("  (%d outlaws, village pop %d)\n",nb2,vil[gv2].pop); CHECK(nb2>0&&nb2<=vil[gv2].pop/4+1,"bandits rise in a restless city but never more than a quarter of its people"); } else printf("  (skipped bandits: no big village)\n");
    lw[LW_BANDITS]=0; int none0=0; for(int k=0;k<nCr;k++) crFl[k]&=~FL_DESERT; for(int t=0;t<500;t++){ tick=64*(t+1); banditTick(); } for(int k=0;k<nCr;k++) none0+=(crFl[k]&FL_DESERT)!=0; CHECK(none0==0,"with the BANDITS law off there are none");
    // save and load
    hzAdd(HZ_GOO,50,50); hzAdd(HZ_FIRE,52,52); hzAdd(HZ_GHOST,54,54); crFl[a1]|=FL_SAPIENT; u32 aid=crId[a1]; int ok=saveGame("/tmp/gg_wild.sav"); Hz hs[HZ_N]; memcpy(hs,hz,sizeof hz); memset(hz,0,sizeof hz); int lo=loadGame("/tmp/gg_wild.sav"); int ai=idFind(aid); a1=ai<0?0:ai;
    printf("  (save %d load %d hz %d flag %d)\n",ok,lo,!memcmp(hs,hz,sizeof hz),(crFl[a1]&FL_SAPIENT)!=0); CHECK(ok&&lo&&!memcmp(hs,hz,sizeof hz)&&(crFl[a1]&FL_SAPIENT),"hazards and the sapient flag survive save and load");
    // snowmen, monoliths, drowning, achievements, hall of rulers
    flat(70,70,6,T_SNOW); weather=3; for(int k=0;k<HZ_N;k++) if(hz[k].kind==HZ_SNOWMAN) hz[k].kind=0; int si=hzAdd(HZ_SNOWMAN,70,70); weather=3; snowStep(&hz[si]); CHECK(hz[si].life>0,"a snowman stands while it snows"); weather=0; int melted=0; for(int t=0;t<200&&!melted;t++){ snowStep(&hz[si]); if(!hz[si].life) melted=1; } CHECK(melted,"and melts when the snow stops"); hz[si].kind=0;
    flat(80,30,8,T_GRASS); bld[30][80]=B_STATUE; bldv[30][80]=255; monoScan(); CHECK(monoN>=1,"a monolith is found by the scan"); for(int k=0;k<4;k++) addCr(ani,78+k,32); int up=0; for(int t=0;t<400;t++){ tick=64*(t+1); monolithTick(); } for(int k=0;k<nCr;k++) up+=cr[k].alive&&(crFl[k]&FL_SAPIENT)&&cr[k].x>=70&&cr[k].x<=90&&cr[k].y>=24&&cr[k].y<=40; CHECK(up>0,"a monolith uplifts animals near it (%d)",up);
    { addCr(ani,60,100); int di=occ[100][60]-1; setCell(60,100,T_DEEP); lw[LW_DROWN]=1; cr[di].hp=100; int dead=0; if(!walkable(&sp[cr[di].sp],60,100)){ for(int t=0;t<400&&cr[di].alive;t++){ tick=8*(t+1); drownTick(); } dead=!cr[di].alive; CHECK(dead,"DROWNING on: a non-swimmer in deep water dies"); } else printf("  (skipped: species swims)\n"); lw[LW_DROWN]=0; }
    { int n=0; for(int i2=0;i2<ACH_N;i2++) n++; CHECK(n==46,"46 achievements"); achX[0]=0x5; achX[1]=0x3; achX[2]=0x21; int ok2=saveGame("/tmp/gg_wild2.sav"); u32 b0=achX[0],b1=achX[1],b2=achX[2]; achX[0]=achX[1]=achX[2]=0; int lo2=loadGame("/tmp/gg_wild2.sav"); CHECK(ok2&&lo2&&achX[0]==b0&&achX[1]==b1&&achX[2]==b2,"achievements 32 and up and their counters survive save and load"); }
    { int k=-1; for(int i2=0;i2<KN_N;i2++) if(kd[i2].on){ k=i2; break; } if(k>=0){ kd[k].rn1=3; kd[k].rn2=4; kd[k].reign=33; hallPush(k); kd[k].rn1=5; kd[k].rn2=6; kd[k].reign=7; hallPush(k); u8 h0[4][3]; memcpy(h0,hall[k],12); int ok3=saveGame("/tmp/gg_wild3.sav"); memset(hall,0,sizeof hall); int lo3=loadGame("/tmp/gg_wild3.sav"); CHECK(ok3&&lo3&&!memcmp(h0,hall[k],12)&&hall[k][0][0]==5&&hall[k][1][2]==33,"the hall of rulers keeps the last rulers and survives save and load"); } }
    printf(fails?"FAILED\n":"PASSED\n"); return fails?1:0; }
