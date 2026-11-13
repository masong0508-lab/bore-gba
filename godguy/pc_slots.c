// PC test of the save slots (the file layout of the 3DS and PC): slot 1 is the old godguy.sav, nine slots in all, each with its own world; the list reads the headers; the game starts on the furthest world; clearing and loading work
#define PCTEST 1
#define SAVE_PATH "/tmp/gg_slotdata/godguy.sav"
#include <stdlib.h>
#include <sys/stat.h>
#include "source/main.c"
static int ok=1;
static void chk(int c,const char*what){ printf("  %s: %s\n",what,c?"yes":"NO"); if(!c) ok=0; }
int main(void){
    system("rm -rf /tmp/gg_slotdata; mkdir -p /tmp/gg_slotdata"); makeSpecies(); makeItems();
    int sizes[3]={128,192,128}; int ticks[3]={2000,5000,3500}; int cre[3];
    slotsInit(); printf("fresh: %d slots, current %d, none used: %s\n",nSlots,curSlot,slotMeta[0].used?"NO":"yes"); chk(nSlots==9&&!slotMeta[0].used,"nine empty slots");
    int slotOf[3]={0,4,8};
    for(int i=0;i<3;i++){ planetSize=sizes[i]; rs=1000u+i; newWorld(); for(int t=1;t<=ticks[i];t++){ tick=t; year=t/1200; simulate(); } tick=ticks[i]; cre[i]=0; for(int c=0;c<nCr;c++) cre[i]+=cr[c].alive; chk(slotSave(slotOf[i]),"save"); }
    struct stat st; chk(stat("/tmp/gg_slotdata/godguy.sav",&st)==0,"slot 1 is godguy.sav (an old save stays where it was)"); chk(stat("/tmp/gg_slotdata/godguy_5.sav",&st)==0&&stat("/tmp/gg_slotdata/godguy_9.sav",&st)==0,"slots 5 and 9 are godguy_5.sav and godguy_9.sav");
    slotsInit(); for(int k=0;k<nSlots;k++) if(slotMeta[k].used) printf("  slot %d: year %u, %dx%d, %d creatures, tick %u\n",k+1,(unsigned)slotMeta[k].year,slotMeta[k].gw,slotMeta[k].gw,slotMeta[k].nCr,(unsigned)slotMeta[k].tick);
    chk(slotMeta[0].used&&slotMeta[4].used&&slotMeta[8].used&&!slotMeta[1].used,"the list sees exactly the three saves");
    chk(slotMeta[0].gw==128&&slotMeta[4].gw==192&&slotMeta[8].gw==128,"planet sizes read from the headers");
    chk(abs(slotMeta[4].nCr-cre[1])<=cre[1]/50+5,"creature counts read from the headers");
    chk(curSlot==4,"the game starts on the furthest-along world (slot 5)");
    // loading each slot gives back its own planet
    int good=1; for(int i=0;i<3;i++){ nCr=0; chk(slotLoad(slotOf[i]),"load"); good&=gW==sizes[i]&&tick==ticks[i]; } chk(good,"each slot loads its own planet");
    // clearing one
    slotDelete(4); chk(!slotMeta[4].used&&slotMeta[0].used&&slotMeta[8].used,"clearing slot 5 leaves the others"); slotsSettle(); chk(curSlot==8&&hasSave,"the game moves to the next furthest slot (9)");
    printf("%s\n",ok?"SLOTS OK":"SLOTS WEAK"); return !ok; }
