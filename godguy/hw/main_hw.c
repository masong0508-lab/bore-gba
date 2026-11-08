// the DS build with no libnds: the game (source/main.c) over compat/ndshw.h
#define main game_main
#include "../source/main.c"
#undef main
void hwBoot(void){
    *(volatile unsigned short*)0x04000304=0x0203;                 // LCDs on, both 2D engines on, main engine on the bottom screen
    HwIpc*q=HWIPC; q->head=0; q->tail=0; q->busy=0; q->keys=0; q->pen=0; q->px=0; q->py=0; q->alive=0x41524D39;   // (the ARM7 only reads the ring once this is set)
}
int main(void){ game_main(); return 0; }
