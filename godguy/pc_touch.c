// PC test of the touch menus: the real program, stylus taps on the menu screen (which is the touch screen while a menu is open)
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
#include <sys/stat.h>
static const char*outDir="/tmp/gg_touch";
typedef struct { int f0, f1, keys, tx, ty; } Ev;
static const Ev script[]={
 {40,44,KEY_TOUCH,60,119},      // OPTIONS
 {70,74,KEY_TOUCH,226,172},     // BACK
 {100,104,KEY_TOUCH,60,134},    // EXTRAS
 {130,134,KEY_TOUCH,60,50},     // KINGDOMS (second row of the list)
 {160,164,KEY_TOUCH,128,185},   // BACK to the menu
 {190,194,KEY_TOUCH,226,172},   // BACK to the main menu
 {220,224,KEY_TOUCH,60,89},     // CREATE A PLANET
 {250,254,KEY_TOUCH,140,80},    // SIZE, right half: bigger
 {280,284,KEY_TOUCH,228,6},     // BACK
 {310,314,KEY_TOUCH,60,90} };   // START THE STORY (row 0)
static int shots[]={50,80,110,140,170,200,230,260,290,330,-1};
void shimInput(int frame,int*held,int*tx,int*ty){
    *held=0; for(unsigned i=0;i<sizeof script/sizeof script[0];i++) if(frame>=script[i].f0&&frame<script[i].f1){ *held|=script[i].keys; *tx=script[i].tx; *ty=script[i].ty; }
    if(frame>=400){ printf("end: titleOn %d menuOn %d page %d planetSize %d chapter %d storyOn %d\n",titleOn,menuOn,page,planetSize,chapter,storyOn); exit(0); } }
void shimPresent(void){ for(int i=0;shots[i]>=0;i++) if(shots[i]==shimFrame){ static u8 top[400*240*3], bot[320*240*3]; shimRender(top,bot); char fn[200]; sprintf(fn,"%s/bot_%d.ppm",outDir,shimFrame); FILE*f=fopen(fn,"wb"); fprintf(f,"P6\n320 240\n255\n"); fwrite(bot,1,sizeof bot,f); fclose(f); } }
int main(int argc,char**argv){ mkdir(outDir,0777); return ds_main(); }
