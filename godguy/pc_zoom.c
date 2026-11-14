// Runs the REAL Bore: GodGuy program (source/main.c, DS user interface and all) on the PC through compat/ndsshim.h with scripted keys and stylus, saves screenshots of both screens
// and the sound it produced. gcc -O2 -DNDSSHIM -Icompat -Isource -o pc_ds pc_ds.c -lm && ./pc_ds outdir
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static const char*outDir="/tmp/gg_ds"; static short*audio; static long audioN, audioCap; static double carry;
typedef struct { int f0, f1, keys, tx, ty; } Ev;
#define K_ (KEY_TOUCH)
typedef struct { int f; int keys; } Zk;
static const Zk zk[]={ {60,KEY_L|KEY_R},{80,KEY_L|KEY_R},{100,KEY_X|KEY_Y},{110,KEY_X|KEY_Y},{120,KEY_X|KEY_Y},{130,KEY_X|KEY_Y},{140,KEY_X|KEY_Y},{150,KEY_L|KEY_R},{160,KEY_L|KEY_R},{170,KEY_L|KEY_R},{180,KEY_L|KEY_R},{190,KEY_L|KEY_R},
    {205,KEY_RIGHT},{206,KEY_RIGHT},{207,KEY_RIGHT},{208,KEY_RIGHT},{209,KEY_RIGHT},{210,KEY_RIGHT},{211,KEY_RIGHT},{212,KEY_RIGHT},{213,KEY_RIGHT},{214,KEY_RIGHT},{215,KEY_RIGHT},{216,KEY_RIGHT},{217,KEY_RIGHT},{218,KEY_RIGHT},{219,KEY_RIGHT},
    {240,KEY_X|KEY_Y},{250,KEY_X|KEY_Y},{260,KEY_X|KEY_Y} };
static int shots[]={59,70,90,105,115,125,135,145,155,165,175,185,195,200,230,245,255,270,-1};
static int hv=0;
void shimInput(int frame,int*held,int*tx,int*ty){
    if(frame==5){ titleOn=0; menuOn=0; extrasView=0; sceneOn=0; dlgOn=0; dlgPend=-1; page=0; paused=0; planetSize=192; rs=777u*2654435761u+31337u; newWorld(); monOn=1; for(int t=1;t<=30000;t++){ tick=t; year=t/1200; simulate(); } tick=700;
        int bv=-1; for(int i=0;i<VIL_N;i++) if(vil[i].on&&(bv<0||vil[i].pop>vil[bv].pop)) bv=i; hv=bv; camX=vil[bv].x*8-128; camY=vil[bv].y*8-96; if(camX<0) camX=0; if(camY<0) camY=0; refillWindow(); hud(); printf("village %d at %d,%d pop %d, creatures %d\n",bv,vil[bv].x,vil[bv].y,vil[bv].pop,nCr); }
    *held=0; for(unsigned i=0;i<sizeof zk/sizeof zk[0];i++) if(frame==zk[i].f||(frame==zk[i].f+1&&(zk[i].keys&(KEY_L|KEY_R|KEY_X|KEY_Y)))) *held|=zk[i].keys;
    if(frame>=290){ int tx=-1,ty=-1; int hit=zHit(128,84,&tx,&ty); int okc=hit&&abs(tx-zcx/8)<=1&&abs(ty-zcy/8)<=1; printf("frames %d zl %d zBmp %d view3d %d creatures %d, centre touch -> tile %d,%d (view centre %d,%d)\n",frame,zl,zBmp,view3d,nCr,tx,ty,zcx/8,zcy/8); printf("%s\n",(zl==2&&zBmp&&!view3d&&okc)?"ZOOM OK":"ZOOM WEAK"); exit(0); } }
void shimPresent(void){
    double want=SHIM_SR/60.0+carry; int n=(int)want; carry=want-n; if(audioN+n>audioCap){ audioCap=audioCap?audioCap*2:SHIM_SR*8; audio=realloc(audio,audioCap*4); } shimMix(audio+audioN*2,n); audioN+=n;
    for(int i=0;shots[i]>=0;i++) if(shots[i]==shimFrame){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[200]; sprintf(fn,"%s/top_%d.ppm",outDir,shimFrame); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n400 240\n255\n"); fwrite(top,1,sizeof top,f); fclose(f);
        sprintf(fn,"%s/bot_%d.ppm",outDir,shimFrame); f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); } }
int main(int argc,char**argv){ if(argc>1) outDir=argv[1]; mkdir(outDir,0777); if(chdir(outDir)) return 1; return ds_main(); }
