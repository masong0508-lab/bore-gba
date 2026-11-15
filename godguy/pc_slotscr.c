// gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_slotscr pc_slotscr.c -lm
// PC test of the save-slot screen's logic: nine file slots hold separate worlds, the list shows what each holds, erasing one leaves the others, and the chosen slot is remembered.
#define UI_NOTRANS 1
#define main ds_main
#include "source/main.c"
#undef main
void shimInput(int frame,int*held,int*tx,int*ty){ *held=0; *tx=*ty=0; }
void shimPresent(void){}
static int fails=0; static void chk(int c,const char*m){ printf("%s %s\n",c?"ok  ":"FAIL",m); if(!c) fails++; }
int main(void){
    mapPtr=(u16*)bgGetMapPtr(0); gxDefaults(); sdOk=1; planetSize=128; rs=7u; makeSpecies(); makeItems(); newWorld();
    chk(NSLOT==9,"nine slots on a file system");
    for(int i=0;i<SLOT_MAX;i++) slotErase(i); slotScan(); int allEmpty=1; for(int i=0;i<SLOT_MAX;i++) allEmpty&=slotYr[i]<0; chk(allEmpty,"every slot starts empty");
    int ys[SLOT_MAX]; for(int k=0;k<3;k++){ int s=k==0?0:k==1?4:8; slotSet(s); year=100+s; chk(saveGame(savePath),"save into a slot"); ys[s]=year; }
    slotScan(); chk(slotYr[0]==100&&slotYr[4]==104&&slotYr[8]==108,"the list shows each slot's year");
    chk(slotYr[1]<0&&slotYr[7]<0,"untouched slots stay empty");
    slotSet(4); chk(loadGame(savePath)&&year==104,"slot 5 loads its own world");
    slotErase(4); slotScan(); chk(slotYr[4]<0&&slotYr[0]==100&&slotYr[8]==108,"erasing one slot leaves the others");
    slotSet(8); slotRemember(); slotSet(0); slotRecall(); chk(slot==8,"the chosen slot is remembered");
    for(int i=0;i<SLOT_MAX;i++) slotErase(i); remove("godguy.slt");
    printf(fails?"SLOTSCR FAIL\n":"SLOTSCR OK\n"); return fails?1:0; }
