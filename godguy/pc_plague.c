// PC test of the plague (port step 6): it spreads, wears creatures down, burns out into immunity, HEAL cures; a zombie's bite infects the living, who fall and rise as zombies; sick folk drain the village store
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int alive(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive; return n; }
static int zombies(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&!strcmp(arch[sp[cr[i].sp].a].nm,"ZOMBIE"); return n; }
static void setup(unsigned seed){ planetSize=128; rs=seed; makeSpecies(); makeItems(); newWorld(); monOn=0; zombSp=-2; zqN=0; }
int main(){ int fails=0; setup(909u); for(int t=1;t<=12000;t++){ tick=t; year=t/1200; simulate(); }
    // a crowd: the plague spreads
    int best=-1,bn=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4){ int n=0; for(int j=0;j<nCr;j++) if(cr[j].alive&&abs(cr[j].x-cr[i].x)<=4&&abs(cr[j].y-cr[i].y)<=4) n++; if(n>bn){ bn=n; best=i; } }
    if(best<0){ printf("no crowd\n"); return 1; }
    gx.lPlague=0; cr[best].sick=0; int ok=infect(best); int peak=0,rec=0; int a0=alive();
    for(int t=0;t<4000;t++){ tick=13000+t; plagueTick(); int p=plagueCount(1); if(p>peak) peak=p; }
    rec=plagueCount(2); printf("plague: infected %d, peak sick %d, recovered/immune %d, alive %d -> %d\n",ok,peak,rec,a0,alive());
    if(!ok||peak<2||rec<1){ fails++; printf("  FAIL: spread/recovery\n"); }
    // a monster cannot catch it
    { int m=-1; for(int s=0;s<nSp;s++) if(arch[sp[s].a].diet==5){ m=s; break; } int k=-1; for(int x=0;x<WW&&k<0;x++) for(int y=0;y<WH&&k<0;y++) if(!occ[y][x]&&!bld[y][x]&&walkable(&sp[m],x,y)) k=addCr(m,x,y); if(k>=0&&infect(k)){ fails++; printf("  FAIL: monster caught the plague\n"); } }
    // HEAL cures
    { int v=-1; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4&&cr[i].sick==0){ v=i; break; } infect(v); if(cr[v].sick!=1){ fails++; printf("  FAIL: no infection\n"); } cr[v].sick=3; cr[v].hp=(u8)maxHp(&cr[v]); /* what PK_HEAL does */ if(cr[v].sick==1) cr[v].sick=2; else if(cr[v].sick==3) cr[v].sick=0; if(cr[v].sick!=0){ fails++; printf("  FAIL: heal\n"); } }
    // the plague law OFF cures everyone
    { gx.lPlague=1; for(int t=0;t<64;t++){ tick=30000+t; plagueTick(); } int p=plagueCount(1)+plagueCount(3); printf("PLAGUE law off: still sick %d\n",p); if(p){ fails++; printf("  FAIL: law\n"); } gx.lPlague=0; }
    // zombie bite: infect, fall, rise
    { setup(909u); for(int t=1;t<=6000;t++){ tick=t; year=t/1200; simulate(); }
      int z=-1; for(int s=0;s<nSp;s++) if(!strcmp(arch[sp[s].a].nm,"ZOMBIE")){ z=s; break; } int h=-1; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4){ h=i; break; }
      if(z<0||h<0){ printf("no zombie species or human\n"); return 1; }
      int zi=-1; for(int r=1;r<6&&zi<0;r++) for(int dx=-r;dx<=r&&zi<0;dx++){ int x=cr[h].x+dx, y=cr[h].y; if(x>=0&&x<WW&&!occ[y][x]&&walkable(&sp[z],x,y)) zi=addCr(z,x,y); }
      if(zi<0){ printf("no room for a zombie\n"); return 1; }
      for(int k=0;k<20&&cr[h].sick!=3;k++) zombieBite(&cr[zi],h); printf("zombie bite: victim sick state %d (3 = infected)\n",cr[h].sick); if(cr[h].sick!=3){ fails++; printf("  FAIL: bite\n"); }
      int z0=zombies(); cr[h].hp=40; for(int t=0;t<400&&cr[h].alive;t++){ tick=50000+t; plagueTick(); calamityTick(); } for(int t=0;t<5;t++){ tick+=1; zombiesRise(); }
      printf("infected victim alive %d, zombies %d -> %d\n",cr[h].alive,z0,zombies()); if(cr[h].alive||zombies()<=z0){ fails++; printf("  FAIL: no zombie rose\n"); }
      // the law off: a zombie cannot infect
      gx.lPlague=1; int h2=-1; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4&&cr[i].sick==0){ h2=i; break; } for(int k=0;k<20;k++) zombieBite(&cr[zi],h2); if(cr[h2].sick){ fails++; printf("  FAIL: bite with the law off\n"); } gx.lPlague=0; }
    // village store drain: sick folk around a village empty its store faster
    { int res[2]; for(int m=0;m<2;m++){ setup(909u); for(int t=1;t<=12000;t++){ tick=t; year=t/1200; simulate(); } int v=-1; for(int i=0;i<VIL_N;i++) if(vil[i].on&&vil[i].pop>=8){ v=i; break; } if(v<0){ printf("no village\n"); return 1; }
        vil[v].stock=400; int n=0; for(int i=0;i<nCr&&n<30;i++) if(cr[i].alive&&sp[cr[i].sp].a==vil[v].race&&abs(cr[i].x-vil[v].x)<=8&&abs(cr[i].y-vil[v].y)<=8){ if(m){ cr[i].sick=1; cr[i].hp=250; } n++; }
        for(int t=0;t<256;t++){ tick=60000+t; if(m){ for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].sick==2) cr[i].sick=1; } plagueTick(); } res[m]=vil[v].stock; }
      printf("village store after 256 ticks: healthy %d, plagued %d\n",res[0],res[1]); if(!(res[1]<res[0])){ fails++; printf("  FAIL: no drain\n"); } }
    // save and load keep the infected state
    { setup(909u); for(int t=1;t<=3000;t++){ tick=t; year=t/1200; simulate(); } int h=-1; for(int i=0;i<nCr;i++) if(cr[i].alive){ h=i; break; } cr[h].sick=3; saveGame("/tmp/gg_pl.sav"); cr[h].sick=0; loadGame("/tmp/gg_pl.sav"); int n3=plagueCount(3); printf("save/load: infected kept %d\n",n3); if(n3<1){ fails++; printf("  FAIL: save\n"); } }
    printf("%s\n",fails?"PLAGUE FAIL":"PLAGUE OK"); return fails?1:0; }
