// PC test of the touch hit test: exact hits win, near misses take the nearest button, far misses hit nothing
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
void shimInput(int f,int*h,int*tx,int*ty){ (void)f;(void)h;(void)tx;(void)ty; }
void shimPresent(void){}
int main(){ int fails=0; uiNBtn=0; uiBtn(10,10,20,12,KEY_A,0,0,0); uiBtn(100,10,20,12,KEY_B,0,0,0); uiBtn(0,0,0,0,0,0,0,0); uiNBtn=2;
    #define T(x,y,want,msg) do{ int h=uiHit(x,y); if(h!=want){ fails++; printf("  FAIL: %s: got %d want %d\n",msg,h,want); } }while(0)
    T(15,15,0,"inside button 0"); T(105,15,1,"inside button 1"); T(8,15,0,"2 px left of button 0"); T(34,15,0,"4 px right of button 0"); T(36,15,-1,"6 px right is a miss");
    T(15,3,-1,"7 px above is a miss"); T(60,15,-1,"between the buttons"); T(103,26,1,"2 px below button 1");
    uiNBtn=0; uiBtn(0,0,256,192,KEY_A,0,0,0); uiBtn(10,10,20,12,KEY_B,0,0,0); T(15,15,1,"a button over a full-screen one wins"); T(200,100,0,"the full-screen button elsewhere");
    printf("%s\n",fails?"HIT FAIL":"HIT OK"); return fails?1:0; }
