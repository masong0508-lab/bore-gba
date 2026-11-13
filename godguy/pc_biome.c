// PC test of the biomes: seeds are painted, they creep over the ground, corruption curses and raises the dead, enchantment cures and beats corruption, the infernal one lights fires, and a save keeps them
#define PCTEST 1
#include <stdlib.h>
#include "source/main.c"
static int cnt(int b){ return nBio[b]; }
int main(int argc,char**argv){
    planetSize=128; rs=909u; makeSpecies(); makeItems(); newWorld(); monOn=0; for(int t=1;t<=3000;t++){ tick=t; simulate(); }
    int pc=-1,pi=-1,pe=-1; for(int q=0;q<PW_N;q++){ if(pw[q].kind==PK_PAINT&&pw[q].arg==101) pc=q; if(pw[q].kind==PK_PAINT&&pw[q].arg==102) pi=q; if(pw[q].kind==PK_PAINT&&pw[q].arg==103) pe=q; }
    printf("biome powers found: %d %d %d\n",pc>=0,pi>=0,pe>=0);
    // a land spot with creatures around
    int cx=-1,cy=-1; for(int i=0;i<nCr&&cx<0;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==4&&world[cr[i].y][cr[i].x]>T_WATER){ cx=cr[i].x; cy=cr[i].y; }
    power=pc; brush=4; usePower(cx,cy,1); int c0=cnt(1); printf("corrupted tiles painted: %d\n",c0);
    int cursed=0, rose0=0; for(int t=3001;t<=9000;t++){ tick=t; simulate(); if(t%500==0){ int cu=0; for(int i=0;i<nCr;i++) cu+=cr[i].alive&&sick[i]==4; if(cu>cursed) cursed=cu; } }
    int c1=cnt(1); printf("corrupted tiles after 6000 ticks: %d (spread %s), most cursed at once: %d\n",c1,c1>c0*2?"YES":"NO",cursed);
    // enchantment beats corruption and cures
    power=pe; brush=6; usePower(cx,cy,1); int e0=cnt(3); int cur0=0; for(int i=0;i<nCr;i++) cur0+=cr[i].alive&&sick[i]==4;
    for(int t=9001;t<=13000;t++){ tick=t; simulate(); } int cur1=0; for(int i=0;i<nCr;i++) cur1+=cr[i].alive&&sick[i]==4;
    int curE=0, onE=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&bioAt(cr[i].x,cr[i].y)==3){ onE++; curE+=sick[i]==4; }
    printf("enchanted tiles %d -> %d, corrupted %d -> %d, cursed %d -> %d; creatures standing on enchanted ground %d, cursed among them %d\n",e0,cnt(3),c1,cnt(1),cur0,cur1,onE,curE);
    // infernal: fires
    power=pi; brush=4; int fx=-1,fy=-1; for(int k=0;k<4000&&fx<0;k++){ int x=rnd()%WW,y=rnd()%WH; if(world[y][x]>T_WATER&&world[y][x]!=T_LAVA&&world[y][x]!=T_MOUNT&&!bld[y][x]){ fx=x; fy=y; } } usePower(fx,fy,1); int i0=cnt(2);
    int ruins0=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) ruins0+=bld[y][x]==B_RUIN; for(int t=13001;t<=16000;t++){ tick=t; simulate(); }
    printf("infernal tiles %d -> %d\n",i0,cnt(2));
    // painting terrain over a biome removes it; laws off freezes it
    power=-1; for(int q=0;q<PW_N;q++) if(pw[q].kind==PK_PAINT&&pw[q].arg==T_SAND) power=q; int b0=cnt(1)+cnt(2)+cnt(3); brush=8; usePower(cx,cy,1); int b1=cnt(1)+cnt(2)+cnt(3);
    printf("sand painted over them: %d -> %d biome tiles\n",b0,b1);
    // save keeps them
    int tot=cnt(1)+cnt(2)+cnt(3); u32 h=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) h=h*31+bioAt(x,y); int ok=saveGame("/tmp/gg_biome.sav"); bioClear(); int ok2=loadGame("/tmp/gg_biome.sav"); u32 h2=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) h2=h2*31+bioAt(x,y);
    printf("save %d load %d: biome tiles %d -> %d, layer identical: %s\n",ok,ok2,tot,cnt(1)+cnt(2)+cnt(3),h==h2?"YES":"NO");
    printf("%s\n",(pc>=0&&c0>0&&c1>c0*2&&cursed>0&&cnt(3)>e0&&curE*10<=onE&&b1<b0&&h==h2&&ok&&ok2)?"BIOMES OK":"BIOMES WEAK"); return 0; }
