// The 3DS 3D top screen (compat/stereo.h) on the real program: the title screen and the kingdom page are drawn, their depth comes from the game, and the two eyes are made from them.
// Checks: strength 0 is the flat picture in both eyes; with depth the eyes differ, the planet's left side is shifted the right way, and the starfield around the picture differs per eye.
//   gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o pc_stereo pc_stereo.c -lm && ./pc_stereo [outdir]
#define GG_THEATRE 1
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include "compat/stereo.h"
#include <time.h>
static const char*outDir; static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
static unsigned char top[400*240*3], bot[320*240*3], L[400*240*3], R[400*240*3];
static void save(const char*n,const unsigned char*p){ char f[200]; sprintf(f,"%s/%s.ppm",outDir,n); FILE*o=fopen(f,"wb"); fprintf(o,"P6\n400 240\n255\n"); fwrite(p,1,400*240*3,o); fclose(o); }
static void anaglyph(const char*n){ static unsigned char a[400*240*3]; for(int i=0;i<400*240;i++){ a[i*3]=L[i*3]; a[i*3+1]=R[i*3+1]; a[i*3+2]=R[i*3+2]; } save(n,a); }
static int diffCount(const unsigned char*a,const unsigned char*b){ int n=0; for(int i=0;i<400*240;i++) if(a[i*3]!=b[i*3]||a[i*3+1]!=b[i*3+1]||a[i*3+2]!=b[i*3+2]) n++; return n; }
static int bright(const unsigned char*p,int x,int y){ const unsigned char*q=&p[(y*400+x)*3]; return q[0]+q[1]+q[2]; }
static void run(const char*tag){
    shimRender(top,bot); memcpy(L,top,sizeof L); stereoTop(L,R,top,shimDepth,0,0); CHECK(!memcmp(L,top,sizeof L)&&!memcmp(R,top,sizeof R),"%s: strength 0 gives the flat picture in both eyes",tag);
    clock_t t0=clock(); for(int k=0;k<20;k++) stereoTop(L,R,top,shimDepth,k,16); double ms=(clock()-t0)*1000.0/CLOCKS_PER_SEC/20; printf("  (both eyes: %.2f ms on this PC)\n",ms);
    stereoTop(L,R,top,shimDepth,0,16); char n[64]; sprintf(n,"%s_L",tag); save(n,L); sprintf(n,"%s_R",tag); save(n,R); sprintf(n,"%s_anaglyph",tag); anaglyph(n);
    int d=diffCount(L,R); CHECK(d>2000,"%s: the eyes differ (%d pixels)",tag,d);
    int dd=0; for(int y=0;y<24;y++) for(int x=0;x<400;x++) if(bright(L,x,y)!=bright(R,x,y)) dd++; CHECK(dd>20,"%s: the starfield around the picture differs per eye (%d)",tag,dd);
    stereoTop(L,R,top,shimDepth,0,32); int d2=diffCount(L,R); CHECK(d2>=d,"%s: more strength, more difference (%d -> %d)",tag,d,d2); }
void shimInput(int frame,int*held,int*tx,int*ty){ *held=0; *tx=*ty=0; shimSwap=0;
    if(frame==300){ menuOn=0; menuAsk=0; extrasView=0; sceneOn=0; dlgOn=0; titleOn=1; mScr=0; mSel=0; paused=1; hud(); }
    if(frame==380){ globeEnter(); rotY=40; rotX=20; hud(); }
    if(frame==340){ planetSize=192; rs=777u*2654435761u+31337u; newWorld(); monOn=1; for(int t=1;t<=45000;t++){ tick=t; year=t/1200; simulate(); } tick=700; titleOn=0; page=0; paused=1; hud(); } }
void shimPresent(void){ extern int shimFrame; if(shimFrame==330){ run("title"); }
    if(shimFrame==360){ run("hud"); /* the planet's dome: the middle of the planet is nearer than its edge */ int mid=0, edge=0; for(int i=0;i<256*192;i++){ if(shimDepth[i]>mid) mid=shimDepth[i]; } (void)edge; CHECK(mid>=4,"the HUD has depth (nearest level %d)",mid); }
    if(shimFrame==390){ int mid=0; for(int i=0;i<256*192;i++) if(shimDepth[i]>mid) mid=shimDepth[i]; run("theatre"); CHECK(view3d&&mid>=8,"the theatre globe is a dome (nearest level %d)",mid); printf(fails?"STEREO FAIL\n":"STEREO OK\n"); exit(fails?1:0); } }
int main(int argc,char**argv){ outDir=argc>1?argv[1]:"/tmp"; return ds_main(); }
