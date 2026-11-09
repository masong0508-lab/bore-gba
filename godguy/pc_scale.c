#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
static int sc;
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; if(frame>=20&&frame<24){ *held=KEY_TOUCH; *tx=60; *ty=90; }
    if(frame>=40&&frame<500&&(frame%12)<2) *held=KEY_A;
    if(frame==520){ *held=KEY_TOUCH; *tx=16+2*0; *ty=190-0; }
    if(frame==560){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[80]; sprintf(fn,"/tmp/gg_sc%d.ppm",sc); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); printf("sc %d titleOn=%d sceneOn=%d dlgOn=%d menuOn=%d TB_Y=%d slots=%d\n",sc,titleOn,sceneOn,dlgOn,menuOn,TB_Y,TB_SLOTS); exit(0); } }
void shimPresent(void){}
int main(int argc,char**argv){ sc=atoi(argv[1]); hwUiScale=sc; return ds_main(); }
