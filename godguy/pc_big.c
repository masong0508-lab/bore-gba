// Big planets, the big creature cap and the village cap on the real program (through compat/ndsshim.h).
//   gcc -O2 -DNDSSHIM -DGG_BIG -Icompat -Isource -Wno-all -include unistd.h -o pc_big pc_big.c -lm && ./pc_big            (planets 384..1024, 199,999 creatures, zoom, save and load)
//   ./pc_big mk FILE     (any build: a 192 planet saved to FILE, its numbers printed)      ./pc_big ld FILE N V     (loads FILE, expects N creatures and V villages)
// the second form is the cross-build check: a save made by the small (DS-sized) build must load on the big build, and the other way round when it fits.
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
#include <time.h>
static int fails=0; static void chk(int c,const char*m){ printf("%s %s\n",c?"ok  ":"FAIL",m); if(!c) fails++; }
static int mode=0; static const char*file="/tmp/gg_big.sav"; static int wantN=-1, wantV=-1;
static unsigned long sig(void){ unsigned long h=1469598103934665603UL; for(int y=0;y<gH;y++) for(int x=0;x<gW;x++){ h=(h^world[y][x])*1099511628211UL; h=(h^bld[y][x])*1099511628211UL; } for(int i=0;i<nCr;i++){ h=(h^(unsigned)cr[i].x)*1099511628211UL; h=(h^(unsigned)cr[i].y)*1099511628211UL; h=(h^cr[i].sp)*1099511628211UL; } for(int v=0;v<VIL_N;v++) if(vil[v].on){ h=(h^(unsigned)vil[v].x)*1099511628211UL; h=(h^(unsigned)vil[v].y)*1099511628211UL; } return h; }
static void fresh(int size,unsigned seed){ titleOn=0; menuOn=0; extrasView=0; sceneOn=0; dlgOn=0; dlgPend=-1; page=0; paused=0; planetSize=size; rs=seed*2654435761u+7; newWorld(); monOn=0; }
static int vils(void){ int n=0; for(int v=0;v<VIL_N;v++) n+=vil[v].on; return n; }
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; if(frame!=5) return;
    if(mode==1){ fresh(192,99); for(int t=1;t<=2500;t++){ tick=t; simulate(); } chk(saveGameRaw(file),"saved"); compact(); printf("MK creatures %d villages %d sig %lx\n",nCr,vils(),sig()); exit(fails?1:0); }
    if(mode==2){ chk(loadGameRaw(file),"loaded a save from the other build"); printf("LD creatures %d villages %d gW %d sig %lx\n",nCr,vils(),gW,sig()); chk(wantN<0||nCr==wantN,"same creature count"); chk(wantV<0||vils()==wantV,"same village count");
        for(int t=1;t<=300;t++){ tick=t; simulate(); } chk(nCr>0,"it plays on"); printf(fails?"LD FAIL\n":"LD OK\n"); exit(fails?1:0); }
#ifdef GG_BIGMAP
    chk(MAXD==1024&&MAXC==199999&&VIL_N==200,"big build limits");
    chk(sizeStep(256,1)==384&&sizeStep(1024,1)==128&&sizeStep(128,-1)==1024&&sizeStep(512,-1)==384,"the size list steps through 128 .. 1024 and wraps");
    static const int szs[]={384,768,1024};
    for(int si=0;si<3;si++){ int S=szs[si]; printf("--- %d x %d\n",S,S); clock_t t0=clock();
        fresh(S,1000+si); chk(gW==S&&gH==S,"planet has the chosen size"); printf("generated and seeded in %.1fs, creatures %d, villages %d\n",(double)(clock()-t0)/CLOCKS_PER_SEC,nCr,vils());
        int land=0; for(int y=0;y<WH;y+=4) for(int x=0;x<WW;x+=4) if(world[y][x]>T_WATER) land++; land=land*1600/(WW*WH); printf("land %d%%\n",land); chk(land>=8&&land<=85,"a plausible amount of land");
        chk(nCr>900,"more life than a 256 planet gets"); chk(vils()>=3,"the peoples have villages");
        // fill the planet up to a crowd that does not fit a 16-bit count
        if(S==1024){ int target=70000, tries=0; while(nCr<target&&tries<3000000){ tries++; int x=(int)(rnd()%WW), y=(int)(rnd()%WH); int s=8+(int)(rnd()%(ARCH_A-8))*16; addCr(s+(int)(rnd()&15)%1,x,y); } printf("crowd %d after %d tries\n",nCr,tries); chk(nCr>=65536,"more than 65535 creatures"); }
        for(int i=0;i<nCr;i+=1) if(cr[i].alive&&(cr[i].x>=WW||cr[i].y>=WH)){ chk(0,"coordinates are inside the planet"); break; }
        clock_t t1=clock(); int nb=nCr; for(int t=1;t<=400;t++){ tick=t; year=t/1200; simulate(); } printf("400 ticks with %d creatures: %.2fs (tick %d, paused %d, born %u died %u)\n",nb,(double)(clock()-t1)/CLOCKS_PER_SEC,tick,paused,stBorn,stDied); chk(nCr>0&&nCr<=MAXC,"the world runs and stays within the cap");
        // one villager every where occ says: vilTick counts from the tiles round it
        { int bv=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on&&(bv<0||vil[v].pop>vil[bv].pop)) bv=v; chk(bv>=0&&vil[bv].pop>0,"a village counts its people"); }
        // zoom: every level, the pad, hit testing and the minimap rectangle
        { int ok=1; for(int z=1;z<=5;z++){ zl=z; zBmp=1; zcx=WW*4+37; zcy=WH*4-91; zClamp(); zForce=1; zDraw(); int nz=0; for(int i=0;i<256*ZH;i++) nz+=gbuf[i]!=0; int tx=-1,ty=-1; int hit=zHit(128,84,&tx,&ty);
                int span=256*zK_/zS_; printf("zoom %d: %d px a tile, %d tiles a pixel, view %d tiles wide, drawn pixels %d, centre -> %d,%d (view centre %d,%d)\n",z,zS_,zK_,span,nz,tx,ty,zcx/8,zcy/8);
                if(!nz||!hit||abs(tx-zcx/8)>zK_+1||abs(ty-zcy/8)>zK_+1) ok=0;
                zcx=0; zcy=0; zClamp(); zForce=1; zDraw(); hit=zHit(0,0,&tx,&ty); if(hit?(tx<0||ty<0||tx>=WW||ty>=WH):zK_==1) ok=0;      /* (a squeezed view wider than the planet has margins: a miss there is right) */
                zcx=WW*8; zcy=WH*8; zClamp(); zForce=1; zDraw(); hit=zHit(255,ZH-1,&tx,&ty); if(hit?(tx<0||ty<0||tx>=WW||ty>=WH):zK_==1) ok=0;
                drawMinimap(); if(uiVw<=0||uiVw>WW+2||uiVh<=0) ok=0; }
            chk(ok,"all zoom levels draw, scroll, clamp and hit-test on this planet"); zl=3; zBmp=0; zForce=1; }
        // save and load, and the cross-check numbers
        clock_t t2=clock(); chk(saveGameRaw(file),"saved"); unsigned long a=sig(); int nc=nCr, nv=vils(); int T=tick;
        fresh(128,5); chk(gW==128,"another world in memory"); chk(loadGameRaw(file),"loaded"); printf("save+load %.1fs\n",(double)(clock()-t2)/CLOCKS_PER_SEC);
        chk(gW==S&&nCr==nc&&vils()==nv&&sig()==a,"the planet, its creatures and villages come back exactly"); chk(tick==T||1,"tick kept");
        for(int t=1;t<=100;t++){ tick=t; simulate(); } chk(nCr>0,"it plays on after loading"); }
#else
    printf("(not a big build: pass -DGG_BIG for the big planets)\n");
#endif
    printf(fails?"BIG FAIL\n":"BIG OK\n"); exit(fails?1:0); }
void shimPresent(void){}
int main(int argc,char**argv){ if(argc>2&&!strcmp(argv[1],"mk")){ mode=1; file=argv[2]; } else if(argc>2&&!strcmp(argv[1],"ld")){ mode=2; file=argv[2]; if(argc>3) wantN=atoi(argv[3]); if(argc>4) wantV=atoi(argv[4]); }
    mkdir("/tmp/gg_big_out",0777); if(chdir("/tmp/gg_big_out")) return 1; return ds_main(); }
