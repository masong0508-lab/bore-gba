// DSi mode sizes (step 9): -DGG_DSI16 planets to 768, 42,000 creatures, 128 villages; a 768 world saves, packs (the cartridge-chip run-length packer of compat/ndshw.h) and loads back identical.
//   gcc -O2 -DNDSSHIM -DGG_DSI16 -Icompat -Isource -Wno-all -include unistd.h -o pc_vilsave pc_vilsave.c -lm && ./pc_vilsave
// (old text) Big planets, the big creature cap and the village cap on the real program (through compat/ndsshim.h).
//   gcc -O2 -DNDSSHIM -DGG_BIG -Icompat -Isource -Wno-all -include unistd.h -o pc_big pc_big.c -lm && ./pc_big            (planets 384..1024, 100,000 creatures, zoom, save and load)
//   ./pc_big mk FILE     (any build: a 192 planet saved to FILE, its numbers printed)      ./pc_big ld FILE N V     (loads FILE, expects N creatures and V villages)
// the second form is the cross-build check: a save made by the small (DS-sized) build must load on the big build, and the other way round when it fits.
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
#include <time.h>

static int fails=0; static void chk(int c,const char*m){ printf("%s %s\n",c?"ok  ":"FAIL",m); if(!c) fails++; }
static unsigned long sig(void){ unsigned long h=1469598103934665603UL; for(int y=0;y<gH;y++) for(int x=0;x<gW;x++){ h=(h^world[y][x])*1099511628211UL; h=(h^bld[y][x])*1099511628211UL; } for(int i=0;i<nCr;i++){ h=(h^cr[i].sp)*1099511628211UL; h=(h^cr[i].x)*1099511628211UL; h=(h^cr[i].y)*1099511628211UL; } return h; }
static unsigned rleEnc(const u8*in,unsigned n,u8*out,unsigned max){ unsigned o=0,i=0; while(i<n){ if(o+140>max) return 0; unsigned r=1; while(i+r<n&&in[i+r]==in[i]&&r<129) r++; if(r>=3){ out[o++]=(u8)(126+r); out[o++]=in[i]; i+=r; } else { unsigned st=i,l=0; while(i<n&&l<128){ unsigned rr=1; while(i+rr<n&&in[i+rr]==in[i]&&rr<3) rr++; if(rr>=3) break; i++; l++; } out[o++]=(u8)(l-1); for(unsigned k=0;k<l;k++) out[o++]=in[st+k]; } } return o; }
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; if(frame!=5) return;
    chk(MAXD==768&&MAXC==42000&&VIL_N==128,"DSi build limits");
    chk(sizeStep(512,1)==768&&sizeStep(768,1)==128&&sizeStep(128,-1)==768,"the size list stops at 768");
    titleOn=0; menuOn=0; extrasView=0; sceneOn=0; dlgOn=0; dlgPend=-1; page=0; paused=0; planetSize=768; rs=4242u*2654435761u+7; newWorld(); monOn=0;
    chk(gW==768&&gH==768,"a 768 planet is made"); for(int t=1;t<=600;t++){ tick=t; simulate(); }
    printf("creatures %d\n",nCr);
    chk(saveGameRaw("/tmp/gg_dsi16.sav"),"saved"); compact(); unsigned long a=sig(); int n=nCr;
    FILE*f=fopen("/tmp/gg_dsi16.sav","rb"); fseek(f,0,SEEK_END); long raw=ftell(f); fseek(f,0,SEEK_SET); u8*buf=malloc(raw); if(fread(buf,1,raw,f)!=(size_t)raw) raw=0; fclose(f);
    u8*out=malloc(raw+1000); unsigned pk=rleEnc(buf,(unsigned)raw,out,(unsigned)raw+900);
    printf("raw save %ld bytes (buffer 3145728), packed %u bytes (block 2097136)\n",raw,pk);
    chk(raw>0&&raw<=3*1024*1024,"the raw save fits the 3 MB buffer"); chk(pk>0&&pk<=0x200000-16,"the packed save fits the 2 MB block");
    memset(world,0,sizeof world); nCr=0; chk(loadGameRaw("/tmp/gg_dsi16.sav"),"loaded"); chk(nCr==n&&sig()==a,"the loaded world is the saved one");
    for(int t=601;t<=800;t++){ tick=t; simulate(); } chk(nCr>0,"it plays on");
    printf(fails?"DSI16 FAIL\n":"DSI16 OK\n"); exit(fails?1:0); }
void shimPresent(void){}
int main(void){ mkdir("/tmp/gg_dsi16_out",0777); if(chdir("/tmp/gg_dsi16_out")) return 1; return ds_main(); }
