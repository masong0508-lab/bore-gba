// PC test of the globe: facing a tile must put that tile in the middle of the screen (walk mode), picking is the inverse of drawing, and a few pictures are saved.
//   gcc -O2 -DNDSSHIM -Icompat -Isource -include unistd.h -o pc_globe pc_globe.c -lm && ./pc_globe
#define main ds_main
#include "source/main.c"
#undef main
void shimInput(int f,int*h,int*tx,int*ty){ (void)f;(void)tx;(void)ty; *h=0; }
void shimPresent(void){}
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0); rs=3; makeSpecies(); makeItems(); newWorld(); for(int t=1;t<=4096;t++){ tick=t; simulate(); }
    int bad=0,n=0,worst=0; for(int sz=0;sz<3;sz++){ zoom3=sz==0?30:sz==1?60:100;
        for(int y=10;y<WH-10;y+=9) for(int x=0;x<WW;x+=11){ int lat,lon; faceTile(x,y,&lat,&lon); rotX=lat; rotY=lon; int tx,ty; n++; if(!globePick(128,96,&tx,&ty)){ bad++; continue; } int dx=abs(tx-x); if(dx>WW/2) dx=WW-dx; int d=dx>abs(ty-y)?dx:abs(ty-y); if(d>worst) worst=d; if(d>3) bad++; } }
    printf("walk: %d tiles faced at 3 zooms, %d more than 3 tiles from the middle, worst %d\n",n,bad,worst);
    int hit=0, miss=0; zoom3=40; rotX=20; rotY=100; for(int py=0;py<192;py+=7) for(int px=0;px<256;px+=7){ int tx,ty; if(globePick(px,py,&tx,&ty)) hit++; else miss++; } printf("pick on a disc of radius %d: %d hits %d misses (space)\n",globeR(),hit,miss);
    static u8 top[400*240*3], bot[320*240*3]; static short snd[10]; (void)snd;
    int views[4][3]={{0,20,40},{150,-30,40},{250,60,60},{40,0,30}};
    for(int v=0;v<4;v++){ rotY=views[v][0]; rotX=views[v][1]; zoom3=views[v][2]; tick=900+v*500; weather=(v==2)?1:0; globePalette(); globeDraw();
        FILE*f=fopen(v==0?"/tmp/globe0.ppm":v==1?"/tmp/globe1.ppm":v==2?"/tmp/globe2.ppm":"/tmp/globe3.ppm","wb"); fprintf(f,"P6\n256 192\n255\n"); for(int i=0;i<256*192;i++){ u16 c=BG_PALETTE[gbuf[i]]; unsigned char px[3]={(unsigned char)((c&31)*8),(unsigned char)(((c>>5)&31)*8),(unsigned char)(((c>>10)&31)*8)}; fwrite(px,1,3,f); } fclose(f); }
    (void)top;(void)bot; return 0; }
