// Runs the REAL Bore: GodGuy program (source/main.c, DS user interface and all) on the PC through compat/ndsshim.h with scripted keys and stylus, saves screenshots of both screens
// and the sound it produced. gcc -O2 -DNDSSHIM -Icompat -Isource -o pc_ds pc_ds.c -lm && ./pc_ds outdir
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static const char*outDir="/tmp/gg_ds"; static short*audio; static long audioN, audioCap; static double carry;
typedef struct { int f0, f1, keys, tx, ty; } Ev;
#define K_ (KEY_TOUCH)
static const Ev script[]={
 {20,22,KEY_UP,0,0},{26,28,KEY_UP,0,0},{32,34,KEY_DOWN,0,0},{38,40,KEY_DOWN,0,0},{44,46,KEY_LEFT,0,0},{50,52,KEY_LEFT,0,0},{56,58,KEY_RIGHT,0,0},{62,64,KEY_B,0,0},{68,70,KEY_A,0,0},{74,76,KEY_START,0,0},   // the debug code
 {3200,3202,KEY_SELECT,0,0},{3210,3212,KEY_SELECT,0,0},{3220,3222,KEY_SELECT,0,0},{3230,3232,KEY_SELECT,0,0},{3240,3242,KEY_SELECT,0,0},   // to the aural sculptures page (3) after 0..6 wraps

 {200,203,KEY_X,0,0},                       // title: new story world
 {260,262,KEY_A,0,0},                       // the intro scene: dismiss
 {300,1900,0,0,0},
 {400,420,KEY_TOUCH,100,90},                // use the power under the stylus (INSPECT)
 {2000,2002,KEY_R,0,0},{2010,2012,KEY_R,0,0},{2020,2022,KEY_R,0,0},{2030,2032,KEY_R,0,0},{2040,2042,KEY_R,0,0},{2050,2052,KEY_R,0,0},
 {2060,2062,KEY_R,0,0},{2070,2072,KEY_R,0,0},{2080,2082,KEY_R,0,0},{2090,2092,KEY_R,0,0},{2100,2102,KEY_R,0,0},{2110,2112,KEY_R,0,0},{2120,2122,KEY_R,0,0},   // to SPAWN, then touch
 {2200,2230,KEY_TOUCH,128,96},
 {2400,2402,KEY_SELECT,0,0},                // page 1: story
 {2500,2502,KEY_SELECT,0,0},                // page 2: bestiary
 {2600,2602,KEY_SELECT,0,0},                // page 5 (3 is locked): subspecies  [page 3 locked, 4 debug locked]
 {2700,2702,KEY_SELECT,0,0},                // page 6: help
 {2800,2802,KEY_SELECT,0,0},                // back to 0
 {3000,3002,KEY_SELECT|KEY_L|KEY_R,0,0},    // L+R+SELECT: save now
};
static int shots[]={100,3250,3300,150,230,280,2300,2450,2550,2650,2750,2850,3100,-1};
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; for(unsigned i=0;i<sizeof script/sizeof script[0];i++) if(frame>=script[i].f0&&frame<script[i].f1){ *held|=script[i].keys; if(script[i].keys&KEY_TOUCH){ *tx=script[i].tx; *ty=script[i].ty; } }
    if(frame>=3400){ char fn[200]; sprintf(fn,"%s/audio.wav",outDir); FILE*w=fopen(fn,"wb"); int dl=(int)(audioN*4); unsigned hdr[11]={0x46464952,36+dl,0x45564157,0x20746d66,16,0x00020001,SHIM_SR,SHIM_SR*4,0x00100004,0x61746164,dl}; fwrite(hdr,4,11,w); fwrite(audio,4,audioN,w); fclose(w);
        printf("frames %d, audio %.1fs, peak %.2f, clipped samples %ld, failed channel allocations %d, creatures %d, year %d, chapter %d, stUse %u\n",frame,audioN/(double)SHIM_SR,shimPeak,shimClip,shimChFail,nCr,year,chapter,(unsigned)stUse);
        printf("sound channels in use now: "); int u=0; for(int i=0;i<16;i++) u+=shimCh[i].on; printf("%d of 16\n",u); exit(0); } }
void shimPresent(void){
    double want=SHIM_SR/60.0+carry; int n=(int)want; carry=want-n; if(audioN+n>audioCap){ audioCap=audioCap?audioCap*2:SHIM_SR*8; audio=realloc(audio,audioCap*4); } shimMix(audio+audioN*2,n); audioN+=n;
    for(int i=0;shots[i]>=0;i++) if(shots[i]==shimFrame){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[200]; sprintf(fn,"%s/top_%d.ppm",outDir,shimFrame); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n400 240\n255\n"); fwrite(top,1,sizeof top,f); fclose(f);
        sprintf(fn,"%s/bot_%d.ppm",outDir,shimFrame); f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); } }
int main(int argc,char**argv){ if(argc>1) outDir=argv[1]; mkdir(outDir,0777); if(chdir(outDir)) return 1; return ds_main(); }
