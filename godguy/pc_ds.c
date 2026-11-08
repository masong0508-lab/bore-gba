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
static const Ev script[]={
 {20,22,KEY_UP,0,0},{26,28,KEY_UP,0,0},{32,34,KEY_DOWN,0,0},{38,40,KEY_DOWN,0,0},{44,46,KEY_LEFT,0,0},{50,52,KEY_LEFT,0,0},{56,58,KEY_RIGHT,0,0},{62,64,KEY_B,0,0},{68,70,KEY_A,0,0},{74,76,KEY_START,0,0},   // the debug code
 {90,92,KEY_DOWN,0,0},{104,106,KEY_A,0,0},{108,110,KEY_DOWN,0,0},{112,114,KEY_DOWN,0,0},{116,118,KEY_RIGHT,0,0},{120,122,KEY_DOWN,0,0},{124,126,KEY_RIGHT,0,0},{128,130,KEY_DOWN,0,0},{132,134,KEY_LEFT,0,0},{140,142,KEY_B,0,0},{150,152,KEY_DOWN,0,0},{154,156,KEY_DOWN,0,0},{158,160,KEY_A,0,0},{170,172,KEY_DOWN,0,0},{174,176,KEY_A,0,0},{185,187,KEY_START,0,0},{188,190,KEY_B,0,0},{192,194,KEY_UP,0,0},{196,198,KEY_UP,0,0},{200,202,KEY_UP,0,0},{208,210,KEY_A,0,0},   // title: options, extras, bestiary, back, then new story world
 {270,272,KEY_A,0,0},                       // dismiss the intro scene
 {400,402,KEY_A,0,0},{480,482,KEY_A,0,0},{560,562,KEY_A,0,0},{640,642,KEY_A,0,0},{720,722,KEY_A,0,0},{800,802,KEY_A,0,0},   // the first dialogue
 {900,920,KEY_TOUCH,160,180},{950,970,KEY_TOUCH,185,180},   // tap toolbar slots
 {1000,1002,KEY_R,0,0},{1010,1012,KEY_R,0,0},{1020,1022,KEY_R,0,0},{1030,1032,KEY_R,0,0},{1040,1042,KEY_R,0,0},{1050,1052,KEY_R,0,0},{1060,1062,KEY_R,0,0},{1070,1072,KEY_R,0,0},{1080,1082,KEY_R,0,0},{1090,1092,KEY_R,0,0},{1100,1102,KEY_R,0,0},{1110,1112,KEY_R,0,0},{1120,1122,KEY_R,0,0},   // to SPAWN
 {1400,1402,KEY_START,0,0},{1470,1472,KEY_DOWN,0,0},{1480,1482,KEY_DOWN,0,0},{1500,1502,KEY_RIGHT,0,0},{1540,1542,KEY_START,0,0},   // the pause menu
 {1700,1702,KEY_SELECT,0,0},{1760,1762,KEY_SELECT,0,0},{1820,1822,KEY_SELECT,0,0},{1880,1882,KEY_SELECT,0,0},{1940,1942,KEY_SELECT,0,0},{2000,2002,KEY_SELECT,0,0},   // through the pages
 {2100,2102,KEY_SELECT|KEY_L|KEY_R,0,0},
 {2150,2154,KEY_X|KEY_Y,0,0},{2300,2330,KEY_RIGHT,0,0},{2400,2403,KEY_A|KEY_Y,0,0},{2450,2490,KEY_RIGHT,0,0},{2520,2540,KEY_TOUCH,140,80},{2560,2564,KEY_X|KEY_Y,0,0},{2610,2612,KEY_START,0,0},{2620,2622,KEY_DOWN,0,0},{2630,2632,KEY_DOWN,0,0},{2640,2642,KEY_DOWN,0,0},{2650,2652,KEY_A,0,0},{2660,2662,KEY_DOWN,0,0},{2670,2672,KEY_A,0,0},{2760,2762,KEY_START,0,0},{2770,2772,KEY_DOWN,0,0},{2780,2782,KEY_A,0,0},{2880,2882,KEY_START,0,0},{2890,2892,KEY_DOWN,0,0},{2900,2902,KEY_DOWN,0,0},{2910,2912,KEY_A,0,0},
};
static int shots[]={2656,2640,2720,2840,2960,110,135,162,180,2250,2340,2430,2500,2600,100,150,300,500,1150,1250,1300,1450,1520,1740,1800,1860,1920,1980,2050,-1};
void shimInput(int frame,int*held,int*tx,int*ty){
    if(frame==1140){ weather=2; weatherT=3000; } if(frame==1248||frame==1249) flashT=6; if(frame==1290){ weather=3; weatherT=3000; }
    *held=0; for(unsigned i=0;i<sizeof script/sizeof script[0];i++) if(frame>=script[i].f0&&frame<script[i].f1){ *held|=script[i].keys; if(script[i].keys&KEY_TOUCH){ *tx=script[i].tx; *ty=script[i].ty; } }
    if(frame>=3300){ char fn[200]; sprintf(fn,"%s/audio.wav",outDir); FILE*w=fopen(fn,"wb"); int dl=(int)(audioN*4); unsigned hdr[11]={0x46464952,36+dl,0x45564157,0x20746d66,16,0x00020001,SHIM_SR,SHIM_SR*4,0x00100004,0x61746164,dl}; fwrite(hdr,4,11,w); fwrite(audio,4,audioN,w); fclose(w);
        printf("frames %d, audio %.1fs, peak %.2f, clipped samples %ld, failed channel allocations %d, creatures %d, year %d, chapter %d, stUse %u\n",frame,audioN/(double)SHIM_SR,shimPeak,shimClip,shimChFail,nCr,year,chapter,(unsigned)stUse);
        printf("sound channels in use now: "); int u=0; for(int i=0;i<16;i++) u+=shimCh[i].on; printf("%d of 16\n",u); exit(0); } }
void shimPresent(void){
    double want=SHIM_SR/60.0+carry; int n=(int)want; carry=want-n; if(audioN+n>audioCap){ audioCap=audioCap?audioCap*2:SHIM_SR*8; audio=realloc(audio,audioCap*4); } shimMix(audio+audioN*2,n); audioN+=n;
    for(int i=0;shots[i]>=0;i++) if(shots[i]==shimFrame){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[200]; sprintf(fn,"%s/top_%d.ppm",outDir,shimFrame); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n400 240\n255\n"); fwrite(top,1,sizeof top,f); fclose(f);
        sprintf(fn,"%s/bot_%d.ppm",outDir,shimFrame); f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); } }
int main(int argc,char**argv){ if(argc>1) outDir=argv[1]; mkdir(outDir,0777); if(chdir(outDir)) return 1; return ds_main(); }
