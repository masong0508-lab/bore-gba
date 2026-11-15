// The creature caps, filled to the brim (the cap raise: DS 19,999, DSi mode 42,000, 3DS / PC 199,999).
//   DS sizes   (a 256 planet, 19,999 creatures, the DS's raw / packed save blocks; the PC test build is DS-sized in everything but MAXC, so GG_MAXC_TEST=19999 stands in for it):
//     gcc -O2 -DNDSSHIM -DGG_MAXC_TEST=19999 -Icompat -Isource -Wno-all -include unistd.h -o pc_cap_ds pc_cap.c -lm && ./pc_cap_ds
//   DSi mode   (a 768 planet, 42,000 creatures, the 3 MB raw buffer and the 2 MB packed block):
//     gcc -O2 -DNDSSHIM -DGG_DSI16 -Icompat -Isource -Wno-all -include unistd.h -o pc_cap_dsi pc_cap.c -lm && ./pc_cap_dsi
//   3DS / PC   (a 1024 planet, 199,999 creatures):
//     gcc -O2 -DNDSSHIM -DGG_BIG -Icompat -Isource -Wno-all -include unistd.h -o pc_cap_big pc_cap.c -lm && ./pc_cap_big
// For each: fill a world to the cap, check that the next creature is refused, that a save and a load give the same world (every creature record, id and flag),
// that the loader refuses a save that holds more creatures than this build's cap, that the world keeps playing, and print the raw and packed save sizes
// against the build's blocks and the time a tick takes. Prints "CAP <name> OK" or "CAP <name> FAIL". Optional argument: ticks to play (default 300).
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
#include <time.h>

#define DS_RAWMAX (700*1024)
#if defined(GG_DSI16)
#define CAP_NAME "DSI"
#define CAP_WANT 42000
#define CAP_PLANET 768
#define CAP_RAWMAX (3*1024*1024)
#define CAP_PKMAX (0x200000-16)
#elif defined(GG_BIG)
#define CAP_NAME "BIG"
#define CAP_WANT 199999
#define CAP_PLANET 1024
#define CAP_RAWMAX 0                      // saves go to a file: no block
#define CAP_PKMAX 0
#else
#define CAP_NAME "DS"                      // the PC test build is DS-sized in everything but MAXC (30,000): GG_MAXC_TEST=19999 gives it the DS figure, and the DS block sizes are used here
#define CAP_WANT 19999
#define CAP_PLANET 256
#define CAP_RAWMAX DS_RAWMAX
#define CAP_PKMAX (0x80000-16)
#endif

static int fails=0; static void chk(int c,const char*m){ printf("%s %s\n",c?"ok  ":"FAIL",m); if(!c) fails++; }
static unsigned long hashAll(void){ unsigned long h=1469598103934665603UL;
    for(int y=0;y<gH;y++) for(int x=0;x<gW;x++){ h=(h^world[y][x])*1099511628211UL; h=(h^bld[y][x])*1099511628211UL; }
    for(int i=0;i<nCr;i++){ const u8*p=(const u8*)&cr[i]; for(unsigned k=0;k<sizeof(Cr);k++) h=(h^p[k])*1099511628211UL; h=(h^crId[i])*1099511628211UL; h=(h^crFl[i])*1099511628211UL; }
    return h; }
static unsigned rleEnc(const u8*in,unsigned n,u8*out,unsigned max){ unsigned o=0,i=0; while(i<n){ if(o+140>max) return 0; unsigned r=1; while(i+r<n&&in[i+r]==in[i]&&r<129) r++; if(r>=3){ out[o++]=(u8)(126+r); out[o++]=in[i]; i+=r; } else { unsigned st=i,l=0; while(i<n&&l<128){ unsigned rr=1; while(i+rr<n&&in[i+rr]==in[i]&&rr<3) rr++; if(rr>=3) break; i++; l++; } out[o++]=(u8)(l-1); for(unsigned k=0;k<l;k++) out[o++]=in[st+k]; } } return o; }
static int ticksToPlay=300; static const char*file="/tmp/gg_cap.sav";

void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; if(frame!=5) return;
    printf("=== %s build: cap %d (this build's MAXC is %d), planet %d\n",CAP_NAME,CAP_WANT,MAXC,CAP_PLANET);
    chk(MAXC==CAP_WANT,"MAXC is the requested cap");
    titleOn=0; menuOn=0; extrasView=0; sceneOn=0; dlgOn=0; dlgPend=-1; page=0; paused=0; planetSize=CAP_PLANET; rs=9091u*2654435761u+7; newWorld(); monOn=0;
    chk(gW==CAP_PLANET&&gH==CAP_PLANET,"the biggest planet of this build is made");
    // ---- fill to the cap: every kind in turn, on whatever tile it can stand on
    clock_t t0=clock(); long tries=0, maxTries=400L*CAP_WANT+20000000L; int stuck=0;
    while(nCr<CAP_WANT&&tries<maxTries){ tries++; int s=(int)(rnd()%SP_N); int x=(int)(rnd()%WW), y=(int)(rnd()%WH); addCr(s,x,y); }
    printf("filled to %d creatures in %ld tries (%.1fs)\n",nCr,tries,(double)(clock()-t0)/CLOCKS_PER_SEC);
    chk(nCr==CAP_WANT,"the world holds exactly the cap"); if(nCr!=CAP_WANT) stuck=1;
    // ---- the next one is refused (and nothing is damaged by trying)
    { int before=nCr, refused=1, trials=0; unsigned long h0=hashAll();
      for(int y=0;y<WH&&trials<2000;y++) for(int x=0;x<WW&&trials<2000;x+=3) if(!occ[y][x]&&walkable(&sp[0],x,y)){ trials++; if(addCr(0,x,y)>=0) refused=0; }
      if(nCr<CAP_WANT) refused=refused; /* below the cap the test cannot say anything */
      printf("tried %d free tiles at the cap\n",trials);
      chk(trials>0,"there are free tiles to try (the cap, not the planet, is what stops a creature)"); chk(refused&&nCr==before,"addCr refuses cleanly at the cap"); chk(hashAll()==h0,"a refused add changes nothing"); }
    // ---- save, compare, pack
    chk(saveGameRaw(file),"saved the full world"); unsigned long a=hashAll(); int n=nCr, v=0; for(int i=0;i<VIL_N;i++) v+=vil[i].on;
    FILE*f=fopen(file,"rb"); fseek(f,0,SEEK_END); long raw=ftell(f); fseek(f,0,SEEK_SET); u8*buf=malloc(raw); if(fread(buf,1,raw,f)!=(size_t)raw) raw=0; fclose(f);
    u8*out=malloc(raw+2000); unsigned pk=rleEnc(buf,(unsigned)raw,out,(unsigned)raw+1900);
    printf("raw save %ld bytes (%.2f MB)%s; packed %u bytes (%.2f MB)\n",raw,raw/1048576.0,CAP_RAWMAX?"":", no block on this build",pk,pk/1048576.0);
    if(CAP_RAWMAX){ printf("raw buffer %d bytes: %ld spare (%.1f%%); packed block %d bytes: %ld spare\n",CAP_RAWMAX,(long)CAP_RAWMAX-raw,100.0*((long)CAP_RAWMAX-raw)/CAP_RAWMAX,CAP_PKMAX,(long)CAP_PKMAX-(long)pk);
        chk(raw>0&&raw<=CAP_RAWMAX,"the raw save of a full world fits the raw buffer"); chk(pk>0&&pk<=(unsigned)CAP_PKMAX,"the packed save of a full world fits the packed block"); }
    else chk(raw>0,"the raw save of a full world was written");
    // a pack that does not fit must fail, not corrupt: the packer returns 0 when the output block is too small
    if(CAP_PKMAX){ unsigned small=rleEnc(buf,(unsigned)raw,out,(unsigned)(raw/8)); chk(small==0,"packing into a block that is too small is refused (the save would say SAVE FAILED)"); }
    // ---- load it back into a different world
    planetSize=128; rs=5; newWorld(); monOn=0; chk(gW==128,"another world in memory"); chk(loadGameRaw(file),"loaded the full world");
    chk(gW==CAP_PLANET&&nCr==n,"the planet and the creature count come back"); chk(hashAll()==a,"every creature record, id and flag come back exactly");
    { int vv=0; for(int i=0;i<VIL_N;i++) vv+=vil[i].on; chk(vv==v,"the villages come back"); }
    // ---- play on at the cap
    { clock_t t1=clock(); int nb=nCr; for(int t=1;t<=ticksToPlay;t++){ tick=t; year=t/1200; simulate(); } double sec=(double)(clock()-t1)/CLOCKS_PER_SEC;
      printf("%d ticks from %d creatures: %.2fs, %.1f ms a tick; now %d creatures (born %u died %u)\n",ticksToPlay,nb,sec,1000.0*sec/ticksToPlay,nCr,stBorn,stDied);
      chk(nCr>0&&nCr<=MAXC,"the world plays on and stays within the cap");
      for(int i=0;i<nCr;i++) if(cr[i].alive&&(cr[i].x>=WW||cr[i].y>=WH||occ[cr[i].y][cr[i].x]!=(occ_t)(i+1))){ chk(0,"every living creature stands on its own tile"); break; } }
    // ---- a long-lived world is the worst case for the save: the dead leave gaps in the id list (more bytes a creature) and the flags fill in. Play on, keep the crowd at the cap, save again.
    { int churn=ticksToPlay*20; if(churn>20000) churn=20000; for(int t=1;t<=churn;t++){ tick=ticksToPlay+t; year=tick/1200; simulate(); if(nCr<MAXC-200&&(t&7)==0) for(int k=0;k<40&&nCr<MAXC-4;k++) addCr((int)(rnd()%SP_N),(int)(rnd()%WW),(int)(rnd()%WH)); }
      chk(saveGameRaw(file),"saved again after a long run at the cap"); FILE*g=fopen(file,"rb"); fseek(g,0,SEEK_END); long raw2=ftell(g); fclose(g);
      printf("after %d more ticks at the cap (%d creatures, ids up to %u): raw save %ld bytes%s\n",churn,nCr,(unsigned)idNext,raw2,CAP_RAWMAX?"":" (no block)");
      if(CAP_RAWMAX){ printf("  spare in the raw buffer: %ld bytes (%.1f%%)\n",(long)CAP_RAWMAX-raw2,100.0*((long)CAP_RAWMAX-raw2)/CAP_RAWMAX); chk(raw2<=CAP_RAWMAX,"the raw save still fits after a long run at the cap"); } }
    chk(loadGameRaw(file),"and loaded again");
    // ---- a save that holds more creatures than this build's cap is refused (the other direction: a world saved at a higher cap on a lower one)
    { FILE*g=fopen(file,"r+b"); if(g){ SaveHdr h; if(fread(&h,sizeof h,1,g)==1){ int big=MAXC+1; h.nCr=(u16)(big&0xFFFF); h.pad_[1]=(u8)(big>>16); fseek(g,0,SEEK_SET); fwrite(&h,sizeof h,1,g); } fclose(g); }
      planetSize=128; rs=6; newWorld(); monOn=0; chk(!loadGameRaw(file),"a save with MAXC+1 creatures is refused by the loader"); chk(gW==128||gW==CAP_PLANET,"the refusal leaves a world in memory"); }
    // ---- the old caps: a save at the old (lower) count loads, count and all
    { int oldCap=CAP_WANT==19999?16000:CAP_WANT==42000?40000:100000; planetSize=CAP_PLANET>256?CAP_PLANET:256; rs=777; newWorld(); monOn=0; long tr=0; while(nCr<oldCap&&tr<200L*oldCap){ tr++; addCr((int)(rnd()%SP_N),(int)(rnd()%WW),(int)(rnd()%WH)); }
      chk(saveGameRaw(file),"saved a world at the old cap"); int m=nCr; planetSize=128; rs=8; newWorld(); chk(loadGameRaw(file)&&nCr==m,"a world at the old cap loads on the new build"); }
    printf("CAP %s %s\n",CAP_NAME,fails?"FAIL":"OK"); (void)stuck; exit(fails?1:0); }
void shimPresent(void){}
int main(int argc,char**argv){ if(argc>1) ticksToPlay=atoi(argv[1]); mkdir("/tmp/gg_cap_run",0777); if(chdir("/tmp/gg_cap_run")) return 1; return ds_main(); }
