// Verify the incremental map window against a full rebuild at each camera position.
#define main ds_main
#include "source/main.c"
#undef main
#include <time.h>
void shimInput(int frame,int*held,int*tx,int*ty){ *held=0; *tx=*ty=0; }
void shimPresent(void){}
static double seconds(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec+t.tv_nsec*1e-9; }
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0); planetSize=128; makeSpecies(); makeItems(); newWorld();
    u16 expected[4096]; int positions[][2]={{512,512},{520,512},{520,520},{512,520},{505,505},{0,0},{8,0},{16,8},{0,16},{768,768},{760,760}};
    for(unsigned i=0;i<sizeof positions/sizeof positions[0];i++){
        if(i==3){ int x=69,y=69; setCell(x,y,T_SAND); }
        camX=positions[i][0]; camY=positions[i][1]; refillWindow();
        memcpy(expected,mapPtr,sizeof expected);
        winX=-99; refillWindow();
        if(memcmp(expected,mapPtr,sizeof expected)){ fprintf(stderr,"window differs at step %u\n",i); return 1; }
    }
    // Exercise every pan direction and world-edge clipping; compare the full
    // circular tilemap, including slots reused after a long camera jump.
    for(int i=0;i<500;i++){
        camX=(int)(hsh(i,11,3)%(WW*8-256));
        camY=(int)(hsh(i,17,5)%(WH*8-192));
        if(i&1){ camX=(camX&~7)+((i%3)-1)*8; camY=(camY&~7)+((i%5)-2)*8; }
        if(camX<0) camX=0; if(camY<0) camY=0;
        if(camX>WW*8-256) camX=WW*8-256;
        if(camY>WH*8-192) camY=WH*8-192;
        refillWindow(); memcpy(expected,mapPtr,sizeof expected);
        winX=-99; refillWindow();
        if(memcmp(expected,mapPtr,sizeof expected)){ fprintf(stderr,"window differs at random step %d\n",i); return 1; }
    }
    camX=256; camY=256; winX=-99; refillWindow();
    double start=seconds();
    for(int i=0;i<3000;i++){ camX=256+(i%40)*8; camY=256+((i/40)%20)*8; refillWindow(); }
    printf("window pan: %.3f ms per tile\n",(seconds()-start)*1000/3000);
    puts("window tiles match full rebuild");
    return 0;
}
