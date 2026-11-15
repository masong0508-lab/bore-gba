// Pack G: item rarity and modifiers are derived (stable, no save), a rarer item of one kind is never weaker than its base, a slain boss drops rare gear to the nearest folk, loot rain and corrosive rain change gear.
//   gcc -O2 -DPCTEST -w -o pc_loot pc_loot.c -lm && ./pc_loot
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int fails;
#define CHECK(c,...) do{ if(c) printf("  ok    "); else { printf("  FAIL  "); fails++; } printf(__VA_ARGS__); printf("\n"); }while(0)
int main(void){
    planetSize=128; rs=77; makeSpecies(); makeItems(); newWorld(); unlockAll=1; int cnt[4]={0}, gear=0, same=1, modsOk=1, bonusOk=1;
    for(int i=0;i<nIt;i++){ int r=itRar(i); if(it[i].kind==IK_WEAPON||it[i].kind==IK_ARMOR){ gear++; cnt[r]++; if(r!=itRar(i)||itMods(i)!=itMods(i)) same=0; if(__builtin_popcount(itMods(i))!=r) modsOk=0; if(itBonus(i)<r*2) bonusOk=0; } else if(r) modsOk=0; }
    printf("  (%d gear items: %d normal %d rare %d epic %d legendary)\n",gear,cnt[0],cnt[1],cnt[2],cnt[3]);
    CHECK(same&&modsOk&&bonusOk,"rarity is stable, modifiers match the rarity, the bonus is at least 2 per rarity step");
    CHECK(cnt[0]>cnt[1]&&cnt[1]>cnt[2]&&cnt[2]>=cnt[3]&&cnt[3]>0,"rarer is rarer: normal > rare > epic >= legendary");
    // a boss's drop
    int folk=-1, mon=-1; for(int s=0;s<nSp;s++){ if(arch[sp[s].a].diet==4&&folk<0) folk=s; if(arch[sp[s].a].diet==5&&mon<0) mon=s; }
    int x=40,y=40; for(int yy=y-6;yy<=y+6;yy++) for(int xx=x-6;xx<=x+6;xx++) setCell(xx,yy,T_GRASS);
    addCr(folk,x+2,y+1); int fi=occ[y+1][x+2]-1; addCr(mon,x,y); int mi=occ[y][x]-1; cr[mi].rk=2; cr[fi].wep=0; cr[fi].arm=0;
    killCr(mi); int got=cr[fi].wep?cr[fi].wep-1:cr[fi].arm?cr[fi].arm-1:-1; CHECK(got>=0&&itRar(got)>=3,"a slain boss's gear (rarity %d) goes to the nearest folk",got>=0?itRar(got):-1);
    // loot rain and corrosive rain
    cr[fi].wep=cr[fi].arm=0; int hit=0; for(int t=0;t<80&&!hit;t++){ power=-1; for(int i=0;i<PW_N;i++) if(pw[i].kind==PK_LOOT) power=i; brush=1; usePower(cr[fi].x,cr[fi].y,1); hit=cr[fi].wep||cr[fi].arm; } CHECK(hit,"LOOT RAIN gives gear to folk in the brush");
    for(int i=0;i<PW_N;i++) if(pw[i].kind==PK_CORRODE) power=i; usePower(cr[fi].x,cr[fi].y,1); CHECK(!cr[fi].wep&&!cr[fi].arm,"CORROSIVE strips it again");
    printf(fails?"FAILED\n":"PASSED\n"); return fails?1:0; }
