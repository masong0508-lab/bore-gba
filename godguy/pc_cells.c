// PC test of GodGuy step 10c, cells (source/ggcell.h): the 8 x 8 cell grid that lets a soldier find the nearest enemy by reading nine cell masks instead of 49 tiles.
// The grid is kept in step with the creatures through years of births, deaths, moves, a mass die-off, compaction and a load; cellNearest() must give exactly the tile scan's answer
// (nearest, then first in row order) for every creature, every window up to 8, every mask; and the game must run exactly as it did with the old scan.
// Build it twice (once with -DGG_NOCELL, which puts the old tile scan back into the soldiers' thinking) and compare the "state hash" line of the two runs: run_tests.sh does.
#define PCTEST 1
#include <stdlib.h>
#include <time.h>
#include "source/main.c"
static int fails;
#define CHK(c,msg) do{ if(!(c)){ fails++; printf("  FAIL: %s\n",msg); } }while(0)
static void setup(unsigned seed,int size){ planetSize=size; rs=seed; makeSpecies(); makeItems(); newWorld(); chapter=3; tierOpen=4; monOn=1; }
static int brute(int ci,int R,unsigned em,int*dist){      // the tile scan, row by row, first nearest wins
    int best=-1,bd=99; for(int dy=-R;dy<=R;dy++) for(int dx=-R;dx<=R;dx++){ int x=cr[ci].x+dx,y=cr[ci].y+dy; if(x<0||y<0||x>=gW||y>=gH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==ci) continue; int k=cellKind(cr[j].sp); if(k<0||!((em>>k)&1)) continue;
        int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){ bd=d; best=j; } } if(dist) *dist=bd; return best; }
static unsigned long long qs, qhit; static long tilesBrute, tilesCell;
static int compareAll(int step,unsigned seed){      // every (or every step-th) creature, three windows, three masks
    int bad=0; static const int Rs[3]={1,3,8};
    for(int i=0;i<nCr;i+=step){ if(!cr[i].alive) continue; for(int r=0;r<3;r++) for(int m=0;m<3;m++){
            unsigned em= m==0?0x100u : m==1?(hmix(seed+(unsigned)i)&0x1FFu) : 0x1FFu; int d1=-1,d2=-1; int a=brute(i,Rs[r],em,&d1), b=cellNearest(i,Rs[r],em,&d2); qs++; if(a>=0) qhit++;
            if(a!=b||(a>=0&&d1!=d2)) bad++; } }
    return bad; }
static u32 stateHash(void){ u32 h=2166136261u; const u8*p;
#define H(ptr,n) p=(const u8*)(ptr); for(size_t q=0;q<(size_t)(n);q++) h=(h^p[q])*16777619u;
    for(int y=0;y<gH;y++){ H(world[y],gW); H(bld[y],gW); } H(cr,nCr*sizeof(Cr)); H(crId,nCr*sizeof(u32)); H(crFl,nCr*sizeof(u16)); H(nb,nNb*sizeof(Nb)); H(&nNb,sizeof nNb); H(vil,sizeof vil); H(&nCr,sizeof nCr); H(&year,sizeof year); H(jr,sizeof jr); H(&jrN,sizeof jrN); return h; }
int main(void){
    // ---- 1. the grid and the scan agree through years of play, and wars are fought by the same soldiers either way
    u32 hs=0; int warTicks=0;
    for(int size=128;size<=192;size+=64){
        setup(4242u+size,size); int checks=0, gridBad=0, qBad=0; unsigned long long q0=qs;
        for(int t=1;t<=4*2048;t++){ tick=t; simulate(); { int w=0; for(int a=0;a<8&&!w;a++) for(int b=0;b<8;b++) if(warM[a][b]){ w=1; break; } warTicks+=w; }
            if((t&511)==0){ checks++; if(cellCheck()) gridBad++; qBad+=compareAll(nCr>1500?7:1,(unsigned)t); } }
        printf("size %d: %d checks, grid differs from a rebuild %d times, window answers differing from the tile scan %d (of %llu)\n",size,checks,gridBad,qBad,qs-q0);
        CHK(gridBad==0,"the kept grid always equals a rebuilt one"); CHK(qBad==0,"cellNearest equals the tile scan");
        hs=hs*31u+stateHash(); }
    printf("ticks with a war on: %d; queries %llu, with an answer %llu\n",warTicks,qs,qhit); CHK(qhit>1000,"the queries found creatures to compare");
    // ---- 2. a mass die-off, compaction, a save and a load
    setup(777u,128); for(int t=1;t<=2*2048;t++){ tick=t; simulate(); }
    for(int i=0;i<nCr;i++) if(i%3!=0&&cr[i].alive) killNat(i); CHK(cellCheck()==0,"grid after a mass die-off (before compaction)"); compact(); CHK(cellCheck()==0,"grid after compaction"); CHK(compareAll(1,5u)==0,"answers after compaction");
    int n0=0; for(int c=0;c<((gW+7)>>3)*((gH+7)>>3);c++) for(int k=0;k<CELL_K;k++) n0+=cellCnt[c][k];
    saveGame("/tmp/godguy_cells.sav"); planetSize=192; rs=3; newWorld(); int lo=loadGame("/tmp/godguy_cells.sav"); int n1=0; for(int c=0;c<((gW+7)>>3)*((gH+7)>>3);c++) for(int k=0;k<CELL_K;k++) n1+=cellCnt[c][k];
    printf("load: %d, counted creatures %d before, %d after\n",lo,n0,n1); CHK(lo&&n0==n1&&cellCheck()==0,"grid after a load"); CHK(compareAll(1,9u)==0,"answers after a load");
    // ---- 3. the saving: tiles read for a soldier's scan, real war on (every people at war with every other) and a quiet world
    { setup(31u,192); for(int t=1;t<=3*2048;t++){ tick=t; simulate(); } int nq=0; volatile int sink=0; clock_t c0,c1;
      memset(warM,0,sizeof warM); unsigned emQuiet=0x100u; unsigned emWar=0x1FFu; double ns[2][2];
      for(int mode=0;mode<2;mode++){ unsigned em=mode?emWar:emQuiet; int dd; c0=clock(); for(int rep=0;rep<20;rep++) for(int i=0;i<nCr;i++){ if(!cr[i].alive) continue; sink+=brute(i,3,em,&dd); nq++; } c1=clock(); ns[0][mode]=1e9*(double)(c1-c0)/CLOCKS_PER_SEC/nq; nq=0;
        c0=clock(); for(int rep=0;rep<20;rep++) for(int i=0;i<nCr;i++){ if(!cr[i].alive) continue; sink+=cellNearest(i,3,em,&dd); nq++; } c1=clock(); ns[1][mode]=1e9*(double)(c1-c0)/CLOCKS_PER_SEC/nq; nq=0; }
      printf("scan cost per soldier (PC, ns): quiet world tiles %.0f cells %.0f | everyone at war tiles %.0f cells %.0f\n",ns[0][0],ns[1][0],ns[0][1],ns[1][1]); (void)sink; }
    printf("state hash %08x\n",hs);
    printf(fails?"CELLS FAILED (%d)\n":"CELLS OK\n",fails); return fails?1:0; }
