// BORE: GODGUY (Nintendo DS) - a god game. v0.1: a 128 x 128 world you shape with the stylus, creatures that live, eat, hunt and breed.
// Bottom screen (touch): the world, 8 x 8 pixel tiles on a hardware background, creatures as sprites. Top screen: the HUD and the whole-planet map.
// Controls: D-pad pans (B = fast), L / R pick a power, X / Y change the brush size (or the species when SPAWN is picked), SELECT jumps ten species, A pauses, START makes a new world.
// The catalogue (species, powers) is data: tables below, generated names, so it grows by adding rows.
#ifndef PCTEST
#include <nds.h>
#include <fat.h>
#endif
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gg.h"
#include "gmdata.h"      // the music and sound-effect tables (ids like FX_SMITE and GM_TITLE are used by the game; the player is gmplay.h, DS and PC)

int gW=128, gH=128;
u8 world[MAXD][MAXD];
static u16 occ[MAXD][MAXD];            // creature index + 1 standing on a tile
static Cr cr[MAXC]; static int nCr;
static int camX, camY;             // camera, in world pixels
static int power=1, brush=2, spSel=40, paused=0, tick=0, year=0;
static char info[64];
static u32 stKill, stGift, stUse;                 // monsters slain, gifts given, powers used (the story counts them)
static u8 seenB[(SP_N+7)/8], slainB[(SP_N+7)/8]; static int seenN;   // the bestiary: which kinds of monster have appeared / been slain
static int chapter, storyOn=1, storyDone, monOn=1, natural, tierOpen=1;
static int fxQ=-1, musQ=-1;       // requests for the sound code (DS main loop plays them)
static void fxReq(int id){ fxQ=id; }
static void musReq(int id){ musQ=id; }
static int gearAtk(const Cr*c); static int armDef(const Cr*c); static void unlockTiers(void);
static u32 rs=12345;
u32 rnd(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return rs; }

// ---------------------------------------------------------------------------------------------- the catalogue
static const char* const advNm[16]={"CRIMSON","AMBER","GOLDEN","LIME","GREEN","JADE","AZURE","COBALT","VIOLET","MAGENTA","ROSE","BROWN","TAN","GREY","SNOW","BLACK"};
// body, dark, accent colours per family (RGB 0..31)
static const u8 famCol[16][3][3]={
 {{28,4,4},{16,1,1},{31,20,10}}, {{30,18,3},{18,8,1},{31,28,10}}, {{30,26,4},{18,14,1},{31,31,18}}, {{18,28,5},{8,16,1},{28,31,12}},
 {{6,22,6},{2,11,2},{18,28,10}}, {{4,24,16},{1,12,8},{16,31,24}}, {{6,22,30},{2,10,18},{20,30,31}}, {{6,10,28},{2,4,16},{16,22,31}},
 {{16,6,28},{8,2,16},{26,16,31}}, {{26,5,24},{14,1,12},{31,18,28}}, {{31,16,20},{20,6,10},{31,26,28}}, {{16,9,3},{8,4,1},{26,18,8}},
 {{26,20,12},{16,11,5},{31,28,20}}, {{16,16,18},{8,8,10},{26,26,28}}, {{29,29,31},{18,19,24},{31,24,24}}, {{6,6,8},{1,1,2},{22,6,6}} };
// shapes: 8 x 8, b body, d dark, a accent, e eye. Two frames each.
static const char* const shp[SHAPE_N][2][8]={
 /*0 quadruped*/ {{"........","..bb....",".bbbbb.b","bbbbbbbb","bbbbbbb.",".d.d.d.d",".d.d.d.d","........"},{"........","..bb....",".bbbbb.b","bbbbbbbb","bbbbbbb.","d.d.d.d.","d.d.d.d.","........"}},
 /*1 bird*/      {{"........","..ee....",".bbbb...","bbbbbbb.",".abbbb..","..d.d...","........","........"},{"........","..ee....","dbbbbbd.","bbbbbbb.",".abbbb..","..d.d...","........","........"}},
 /*2 fish*/      {{"........","........",".bbb..b.","bbbebbbb","bbbbbbbb",".aab..b.","........","........"},{"........","........",".bbb.bb.","bbbebbbb","bbbbbbbb",".aab.bb.","........","........"}},
 /*3 snake*/     {{"........","........","..bb....",".b..bb..","....b.b.","bbbb..bb","..aa....","........"},{"........","........","..bb....","..b.bb..",".b..b.b.","bb..bbbb","..aa....","........"}},
 /*4 insect*/    {{"........","d.d..d.d",".dbbbd..","..bebb..","..bbbb..",".dbbbd..","d.d..d.d","........"},{"........",".d....d.","d.bbbd.d","..bebb..","..bbbb..","d.bbbd.d",".d....d.","........"}},
 /*5 humanoid*/  {{"..bbb...","..beb...","...b....","..aaa...",".daaad..","..ddd...","..d.d...",".dd.dd.."},{"..bbb...","..beb...","...b....","..aaa...","daaaad..","..ddd...","..d.d...",".d...d.."}},
 /*6 blob*/      {{"........","........","..bbbb..",".bbbbbb.",".bebbeb.",".bbbbbb.","..dddd..","........"},{"........","........","........","..bbbb..",".bebbeb.","bbbbbbbb",".dddddd.","........"}},
 /*7 lizard*/    {{"........","........",".bbb....","bbbebbb.","bbbbbbbb",".d.d.d.d","d.d.d.d.","........"},{"........","........",".bbb....","bbbebbb.","bbbbbbbb","d.d.d.d.",".d.d.d.d","........"}},
 /*8 bear*/      {{"..b..b..",".bbbbbb.",".bebbeb.","bbbbbbbb","bbbbbbbb","bbbbbbbb",".dd..dd.",".dd..dd."},{"..b..b..",".bbbbbb.",".bebbeb.","bbbbbbbb","bbbbbbbb","bbbbbbbb","dd....dd","dd....dd"}},
 /*9 deer*/      {{"a.a.....",".a.bb...","..bbbb..","..bebbbb","...bbbbb","...bbbbb","...d.d.d","...d.d.d"},{"a.a.....",".a.bb...","..bbbb..","..bebbbb","...bbbbb","...bbbbb","..d.d.d.","..d.d.d."}},
 /*10 rabbit*/  {{".b.b....",".b.b....",".bbb....","bbebb...","bbbbbb..","bbbbbbb.",".d..d.d.",".d..d.d."},{".b.b....",".b.b....",".bbb....","bbebb...","bbbbbb..","bbbbbbb.","d..d..d.","d..d..d."}},
 /*11 dragon*/  {{"a....a..",".a..bbb.","aabbbbbb",".bbebbbb","bbbbbbbd","dbbbbbd.",".d.dd.d.","........"},{"a....a..","aa..bbb.",".abbbbbb",".bbebbbb","bbbbbbbd","dbbbbbd.","..dd.d..","........"}},
 /*12 spider*/  {{"d.d..d.d",".dbbbbd.","d.bebb.d","..bbbb..",".dbbbbd.","d.d..d.d","........","........"},{".d....d.","d.bbbb.d",".dbebbd.","..bbbb..","d.bbbb.d",".d....d.","........","........"}},
 /*13 whale*/   {{"........","........","....bb..","bbbbbbbb","bbebbbbb","dddddddd",".aa.....","........"},{"........","........",".....bb.","bbbbbbbb","bbebbbbb","dddddddd","..aa....","........"}},
 /*14 bat*/     {{"........","b......b","bb.bb.bb","bbbbbbbb",".bbebb..","..b.b...","........","........"},{"........","........",".bb..bb.","bbbbbbbb","bbbebbbb","..b.b...","........","........"}},
 /*15 turtle*/  {{"........","..dddd..",".dbabbd.","dbbbbbbd","dbbbbbbe",".d.dd.d.","........","........"},{"........","..dddd..",".dbabbd.","dbbbbbbd","dbbbbbbe","..d..d..","........","........"}},
 /*16 skeleton*/ {{"..bbb...","..beb...","...b....",".bbbbb..","b.bbb.b.","..bbb...","..b.b...",".bb.bb.."},{"..bbb...","..beb...","...b....",".bbbbb..",".bbbbbb.","..bbb...","..b.b...","bb...bb."}},
 /*17 ghost*/    {{"..bbbb..",".bbbbbb.",".bebbeb.",".bbbbbb.",".bbbbbb.","bbbbbbbb","b.bb.bb.","........"},{"..bbbb..",".bbbbbb.",".bebbeb.",".bbbbbb.",".bbbbbb.","bbbbbbbb",".bb.bb.b","........"}},
 /*18 golem*/    {{"..bbbb..","..beeb..","bbbbbbbb","bbbbbbbb","dbbbbbbd","..bddb..","..bddb..",".dd..dd."},{"..bbbb..","..beeb..","bbbbbbbb","bbbbbbbb","dbbbbbbd","..bddb..","..bddb..","dd....dd"}},
 /*19 kraken*/   {{"..bbbb..",".bbbbbb.",".bebbeb.",".bbbbbb.","b.b.b.b.","b.b.b.b.",".d.d.d.d","d.d.d.d."},{"..bbbb..",".bbbbbb.",".bebbeb.",".bbbbbb.",".b.b.b.b",".b.b.b.b","d.d.d.d.",".d.d.d.d"}}
};
// archetypes: noun, shape, diet (0 herb, 1 carn, 2 omni, 3 water-eater, 4 civ), walk mask, speed (frames per tile), hp, attack, lifespan (days), breed chance, tint of family used
typedef struct { const char*nm; u8 shape, diet; u16 mask; u8 spd, hp, atk, life, breed; } Arch;
#define M_LAND ((1<<T_GRASS)|(1<<T_FOREST)|(1<<T_HILL)|(1<<T_SAND)|(1<<T_DESERT)|(1<<T_SWAMP)|(1<<T_JUNGLE)|(1<<T_SNOW)|(1<<T_DIRT))
#define M_WATER ((1<<T_WATER)|(1<<T_DEEP))
#define M_ALL (M_LAND|M_WATER|(1<<T_MOUNT)|(1<<T_LAVA))
#define M_CLIMB (M_LAND|(1<<T_MOUNT))
static const Arch arch[ARCH_N]={
 {"HUMAN",5,4,M_LAND,14,30,5,90,20},{"ELF",5,4,M_LAND,12,26,6,140,12},{"DWARF",5,4,M_CLIMB,16,40,8,120,14},{"ORC",5,4,M_LAND,14,50,10,80,24},
 {"GOBLIN",5,4,M_LAND,10,16,4,50,34},{"HALFLING",5,4,M_LAND,14,18,3,100,16},{"GNOME",5,4,M_LAND,16,16,3,110,14},{"OGRE",5,4,M_LAND,22,90,16,70,8},
 {"SHEEP",0,0,M_LAND,26,14,0,60,28},{"COW",0,0,M_LAND,30,24,0,70,18},{"DEER",9,0,M_LAND,14,16,0,60,24},{"RABBIT",10,0,M_LAND,10,6,0,40,60},
 {"WOLF",0,1,M_LAND,12,22,6,70,18},{"FOX",0,1,M_LAND,10,12,4,50,22},{"BEAR",8,2,M_CLIMB,20,60,12,90,8},{"BOAR",0,2,M_LAND,16,26,6,60,22},
 {"EAGLE",1,1,M_ALL&~(1<<T_LAVA),6,10,5,60,12},{"SPARROW",1,0,M_ALL&~(1<<T_LAVA),6,4,0,30,50},{"BAT",14,2,M_ALL&~(1<<T_LAVA),5,5,2,40,30},{"SNAKE",3,1,M_LAND,18,10,5,50,26},
 {"LIZARD",7,2,M_LAND,14,8,2,50,32},{"SPIDER",12,1,M_CLIMB,12,8,4,40,40},{"BEETLE",4,0,M_LAND,20,5,1,30,60},{"TURTLE",15,2,M_LAND|M_WATER,34,40,1,200,8},
 {"FISH",2,3,M_WATER,8,5,0,40,50},{"SHARK",2,1,M_WATER,8,40,10,60,10},{"WHALE",13,3,M_WATER,18,120,6,150,5},{"BLOB",6,2,M_LAND|M_WATER,24,20,2,60,40},
 {"DRAGON",11,1,M_ALL,10,120,22,200,3},{"DRAKE",11,1,M_ALL&~(1<<T_LAVA),12,70,14,140,5},{"MOUSE",10,0,M_LAND,8,4,0,30,70},{"BADGER",8,2,M_LAND,18,26,7,60,14},
 // ---- the monsters (diet 5): they hunt every living thing, and burn the houses of the peoples. tier = the age of the world they first come in (monTier[])
 {"IMP",14,5,M_ALL&~(1<<T_LAVA),8,14,4,120,0},{"GHOUL",5,5,M_LAND,14,28,6,150,0},{"SKELETON",16,5,M_LAND,14,24,7,200,0},{"OOZE",6,5,M_LAND|M_WATER,24,30,4,200,0},
 {"WEREWOLF",0,5,M_LAND,10,40,11,160,0},{"ZOMBIE",5,5,M_LAND,22,60,7,220,0},{"WRAITH",17,5,M_ALL,10,30,10,180,0},{"GIANT SPIDER",12,5,M_CLIMB,10,36,9,170,0},{"SCORPION",4,5,M_LAND,12,30,10,170,0},
 {"TROLL",8,5,M_CLIMB,18,90,14,230,0},{"GOLEM",18,5,M_CLIMB,28,140,16,250,0},{"WYVERN",11,5,M_ALL,8,70,15,200,0},{"SEA SERPENT",2,5,M_WATER,8,80,14,200,0},
 {"KRAKEN",19,5,M_WATER,14,160,18,230,0},{"CYCLOPS",5,5,M_LAND,20,150,20,240,0},{"BASILISK",7,5,M_LAND,12,90,18,220,0},
 {"HYDRA",11,5,M_LAND|M_WATER,16,200,20,250,0},{"DEMON",5,5,M_ALL,10,140,24,250,0},{"LICH",17,5,M_LAND,12,120,26,250,0},{"ELDER DRAGON",11,5,M_ALL,8,250,30,250,0},
};
static const u8 monTier[ARCH_N-ARCH_A]={0,0,0,0, 1,1,1,1,1, 2,2,2,2, 3,3,3, 4,4,4,4};
static Sp sp[SP_N]; static int nSp;
static const char* const bioNm[]={"DEEP","WATER","SAND","GRASS","FOREST","HILL","MOUNTAIN","SNOW","LAVA","DESERT","SWAMP","JUNGLE","DIRT"};
static void makeSpecies(void){
    nSp=0;
    for(int a=0;a<8&&nSp<SP_N;a++){ sp[nSp].a=(u8)a; sp[nSp].fam=(u8)(a==0?12:a==1?5:a==2?11:a==3?4:a==4?3:a==5?1:a==6?9:2); nSp++; }   // the peoples: one each, their own colours
    for(int a=8;a<ARCH_N;a++) for(int f=0;f<16&&nSp<SP_N;f++){ sp[nSp].a=(u8)a; sp[nSp].fam=(u8)f; nSp++; }   // every animal in every colour: 8 + 24 x 16 = 392 kinds
}
static void spName(int s,char*out){ const Arch*A=&arch[sp[s].a]; if(sp[s].a<8) strcpy(out,A->nm); else { strcpy(out,advNm[sp[s].fam]); strcat(out," "); strcat(out,A->nm); } }

// ---------------------------------------------------------------------------------------------- the world
static u32 hsh(int x,int y,int s){ u32 h=(u32)x*374761393u+(u32)y*668265263u+(u32)s*2246822519u; h=(h^(h>>13))*1274126177u; return h^(h>>16); }
static int vnoise(int x,int y,int cell,int s){   // value noise 0..255, cell pixels per lattice step
    int gx=x/cell, gy=y/cell, fx=x%cell, fy=y%cell;
    int a=hsh(gx,gy,s)&255, b=hsh(gx+1,gy,s)&255, c=hsh(gx,gy+1,s)&255, d=hsh(gx+1,gy+1,s)&255;
    int t=a+(b-a)*fx/cell, u=c+(d-c)*fx/cell; return t+(u-t)*fy/cell;
}
static int fbm(int x,int y,int s){ return (vnoise(x,y,32,s)*8+vnoise(x,y,16,s+1)*4+vnoise(x,y,8,s+2)*2+vnoise(x,y,4,s+3))/15; }
static void genWorld(u32 seed){
    int s=(int)(seed&0xFFFF);
    for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){
        int h=fbm(x,y,s), m=fbm(x+300,y+300,s+9), lat=(y<WH/2?y:WH-1-y)*128/WH, temp=40+lat*3+(fbm(x,y,s+20)-128)/3;
        int dx=(x-WW/2)*128/WW, dy=(y-WH/2)*128/WH; h-= (dx*dx+dy*dy)/90; h+=18;   // a little pull to ocean at the edges
        u8 t;
        if(h<62) t=T_DEEP; else if(h<84) t=T_WATER; else if(h<94) t=(temp<70)?T_SNOW:T_SAND;
        else if(h>205) t=(temp<150)?T_SNOW:T_MOUNT; else if(h>170) t=T_HILL;
        else if(temp<70) t=T_SNOW; else if(temp>200&&m<110) t=T_DESERT; else if(temp>190&&m>150) t=T_JUNGLE; else if(m>175&&h<115) t=T_SWAMP; else if(m>135) t=T_FOREST; else t=T_GRASS;
        world[y][x]=t; }
    // a few lava pools in the mountains
    for(int i=0;i<40;i++){ int x=rnd()%WW, y=rnd()%WH; if(world[y][x]==T_MOUNT){ world[y][x]=T_LAVA; if(x+1<WW&&world[y][x+1]==T_MOUNT) world[y][x+1]=T_LAVA; } }
}

// ---------------------------------------------------------------------------------------------- tiles (generated), palette
static const u8 terrCol[T_N][3]={ {2,5,16},{4,12,26},{28,25,14},{8,22,6},{3,13,4},{16,14,8},{15,15,17},{28,30,31},{31,9,2},{29,22,11},{7,14,10},{3,17,9},{18,12,6} };
static int tilePix(int t,int v,int x,int y){   // palette entry (terrain * 8 + shade) of one pixel of ground tile variant v
    int n=(int)(hsh(x+v*8,y+t*16,t)&7); int c=3+(n>>1)-(n==0); if(c<0) c=0; if(c>7) c=7; int pix=t*8+c;
    if(t==T_WATER||t==T_DEEP){ int w=((x+v*2)&7)==(y&7)||((x+v*2+4)&7)==((y+3)&7); if(w&&t==T_WATER) pix=t*8+6; else if(w) pix=t*8+4; }
    if(t==T_FOREST||t==T_JUNGLE){ int dx=x-3-(v&1), dy=y-3-(v>>1); if(dx*dx+dy*dy<=5) pix=t*8+(dy<0?7:1); if(x==3+(v&1)&&y==6) pix=T_DIRT*8+2; }
    if(t==T_MOUNT){ if(y>=2&&x>=3-(y-2)&&x<=4+(y-2)&&y<7) pix=t*8+(x<4?7:2); if(y<=3&&x>=3&&x<=4) pix=T_SNOW*8+6; }
    if(t==T_HILL){ int dy=y-4; if(((x-3-v)*(x-3-v))/2+dy*dy<=4) pix=t*8+(dy<0?6:2); }
    if(t==T_SNOW&&n==7) pix=t*8+7;
    if(t==T_LAVA){ int g=(hsh(x+v*3,y,9)&7); pix=t*8+(g>5?7:g>2?4:2); }
    if(t==T_DESERT&&((y+v)&3)==0&&(x&2)) pix=t*8+2;
    if(t==T_SWAMP&&n>5) pix=T_WATER*8+3;
    return pix;
}
static void shadeRGB(int idx,int*r,int*g,int*b){ int t=idx/8,k=idx%8,f=60+k*20; *r=terrCol[t][0]*f/160; *g=terrCol[t][1]*f/160; *b=terrCol[t][2]*f/160; if(*r>31)*r=31; if(*g>31)*g=31; if(*b>31)*b=31; }
// buildings: B_*; a building tile is ground (grass) with the building on it, in the colours of the people who built it
enum { B_NONE, B_HUT, B_HOUSE, B_HALL, B_FARM, B_TOWER, B_RUIN, B_N };
#define BT0 (T_N*4)
static const u8 roofCol[8][3]={ {27,4,4},{6,22,8},{8,14,26},{14,3,3},{28,24,4},{29,15,3},{20,8,26},{14,9,4} };   // human elf dwarf orc goblin halfling gnome ogre
static const u8 bldCol[8][3]={ {27,22,14},{14,10,5},{16,16,19},{20,20,22},{4,24,6},{12,8,3},{10,10,12},{31,31,31} };   // wall, dark wood, door, stone, crop, soil, ruin, flag
static int bldPix(int type,int race,int x,int y){
    int bg=tilePix(T_GRASS,(x+y)&3,x,y), roof=128+race, wall=136, dark=137, door=138, stone=139, crop=140, soil=141, ruin=142, flag=143;
    switch(type){
    case B_HUT: if(y>=1&&y<=3){ int h=y; if(x>=4-h&&x<=3+h) return roof; } if(y>=4&&y<=6&&x>=2&&x<=5) return (y>=5&&x>=3&&x<=4)?door:wall; break;
    case B_HOUSE: if(y>=1&&y<=3){ if(x>=3-y&&x<=4+y) return roof; } if(y>=4&&y<=7&&x>=1&&x<=6) return (y>=5&&x>=3&&x<=4)?door:(y==5&&(x==1||x==6))?dark:wall; break;
    case B_HALL: if(y<=2&&x==2) return dark; if(y<=1&&x>=3&&x<=5) return flag; if(y==2&&(x&1)==0&&x>=1&&x<=7) return stone; if(y>=3) return (y>=5&&x>=3&&x<=4)?dark:((x==1||x==6)&&y==4)?dark:stone; break;
    case B_FARM: if(x==0||x==7||y==0||y==7) return (x+y)&1?dark:bg; return (y&1)?crop:soil;
    case B_TOWER: if(y==1&&(x==2||x==3||x==5)) return stone; if(y>=2&&x>=2&&x<=5) return (y==4&&x==3)?dark:stone; break;
    case B_RUIN: if((hsh(x,y,17)&3)==0) return ruin; if((hsh(x,y,18)&7)==1) return stone; break; }
    return bg;
}
#ifndef PCTEST
static void makeTiles(void){
    for(int t=0;t<T_N;t++) for(int k=0;k<8;k++){ int r,g,b; shadeRGB(t*8+k,&r,&g,&b); BG_PALETTE[t*8+k]=RGB15(r,g,b); }
    BG_PALETTE[0]=RGB15(0,0,0);
    for(int r=0;r<8;r++) BG_PALETTE[128+r]=RGB15(roofCol[r][0],roofCol[r][1],roofCol[r][2]);
    for(int k=0;k<8;k++) BG_PALETTE[136+k]=RGB15(bldCol[k][0],bldCol[k][1],bldCol[k][2]);
    { u8*bb=(u8*)BG_TILE_RAM(1); for(int ty=1;ty<B_N;ty++) for(int r=0;r<8;r++){ u8*tile=bb+(BT0+ty*8+r)*64; for(int y=0;y<8;y++) for(int x=0;x<8;x++) tile[y*8+x]=(u8)bldPix(ty,r,x,y); } }
    u8*base=(u8*)BG_TILE_RAM(1);
    for(int t=0;t<T_N;t++) for(int v=0;v<4;v++){ u8*tile=base+(t*4+v)*64; for(int y=0;y<8;y++) for(int x=0;x<8;x++) tile[y*8+x]=(u8)tilePix(t,v,x,y); }
}
// ---------------------------------------------------------------------------------------------- the view
#endif
#ifdef PCTEST
static u16 pcm[4096]; static u16*mapPtr=pcm;
#else
static u16*mapPtr;
#endif
static int winX=-99, winY=-99;
static u8 bld[MAXD][MAXD], bldv[MAXD][MAXD];   // what stands on a tile (B_*) and which village owns it (+1)
static u8 vilRace(int v);
static inline void putCell(int x,int y){   // world cell -> background entry (a 64 x 64 torus of 8 pixel tiles)
    int mx=x&63, my=y&63; int blk=(my>=32?2:0)+(mx>=32?1:0); u16 e=(u16)(world[y][x]*4+(int)(hsh(x,y,3)&3)); if(bld[y][x]) e=(u16)(BT0+bld[y][x]*8+(bldv[y][x]?vilRace(bldv[y][x]-1):0)); mapPtr[blk*1024+(my&31)*32+(mx&31)]=e; }
static void refillWindow(void){ int x0=camX>>3, y0=camY>>3; if(x0==winX&&y0==winY) return; winX=x0; winY=y0;
    for(int y=y0-1;y<y0+26&&y<WH;y++) for(int x=x0-1;x<x0+34&&x<WW;x++) if(x>=0&&y>=0) putCell(x,y); }
static void setCell(int x,int y,u8 t){ if(x<0||y<0||x>=WW||y>=WH) return; world[y][x]=t; if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); }

// ---------------------------------------------------------------------------------------------- creatures
static int walkable(const Sp*s,int x,int y){ if(x<0||y<0||x>=WW||y>=WH) return 0; return (arch[s->a].mask>>world[y][x])&1; }
static int addCr(int s,int x,int y){
    if(nCr>=MAXC||x<0||y<0||x>=WW||y>=WH||occ[y][x]) return -1; if(!walkable(&sp[s],x,y)) return -1;
    Cr*c=&cr[nCr]; memset(c,0,sizeof *c); c->sp=(u16)s; c->x=(u8)x; c->y=(u8)y; c->hp=arch[sp[s].a].hp; c->food=70; c->age=(u8)(rnd()&15); c->t=(u8)(rnd()&15); c->alive=1; occ[y][x]=(u16)(nCr+1);
    if(arch[sp[s].a].diet==5&&!((seenB[s>>3]>>(s&7))&1)){ seenB[s>>3]|=(u8)(1<<(s&7)); seenN++; char nm[32]; spName(s,nm); siprintf(info,"BESTIARY: %s",nm); fxReq(FX_MONSTER_SEEN); }
    return nCr++;
}
static void killCr(int i){ Cr*c=&cr[i]; if(!c->alive) return; c->alive=0; if(occ[c->y][c->x]==i+1) occ[c->y][c->x]=0;
    if(!natural&&arch[sp[c->sp].a].diet==5){ stKill++; if(!((slainB[c->sp>>3]>>(c->sp&7))&1)) slainB[c->sp>>3]|=(u8)(1<<(c->sp&7)); fxReq(FX_MONSTER_SLAIN); } }
static void killNat(int i){ natural=1; killCr(i); natural=0; }     // old age, hunger, lava: not a slaying
static void compact(void){   // the dead are squeezed out of the list
    int j=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ if(i!=j) cr[j]=cr[i]; j++; } nCr=j;
    memset(occ,0,sizeof occ); for(int i=0;i<nCr;i++) occ[cr[i].y][cr[i].x]=(u16)(i+1);
}
static const s8 dxs[4]={1,0,-1,0}, dys[4]={0,1,0,-1};
static void moveTo(Cr*c,int i,int nx,int ny){ occ[c->y][c->x]=0; c->ox=(s8)((c->x-nx)*8); c->oy=(s8)((c->y-ny)*8); c->x=(u8)nx; c->y=(u8)ny; occ[ny][nx]=(u16)(i+1); c->dir=(u8)(nx>c->x?0:0); }
// ---------------------------------------------------------------------------------------------- villages
#define VIL_N 24
typedef struct { u8 on, race, x, y, lvl, houses, farms, towers, foe, dead, nm1, nm2; u16 pop, stock; } Vil;   // foe: the village (+1) it is at war with
static Vil vil[VIL_N]; static int nVilOn;
static u8 warM[8][8];   // [a][b]: the people of a are fighting the people of b
static u8 vilRace(int v){ return vil[v].race; }
static const char* const vn1[12]={"BRAN","DUN","MOR","TAL","KAR","EL","OR","ISH","VAL","GRO","HAR","NIM"}, * const vn2[8]={"TON","HOLM","BURG","DELL","FORD","MERE","STEAD","HOLD"};
static void vilName(int v,char*out){ strcpy(out,vn1[vil[v].nm1%12]); strcat(out,vn2[vil[v].nm2%8]); }
static int aggressive(int race){ return race==3||race==4||race==7; }   // the orcs, the goblins and the ogres go looking for trouble
static int vilNear(int race,int x,int y,int r){ int best=-1,bd=r*r+1; for(int v=0;v<VIL_N;v++){ if(!vil[v].on||vil[v].race!=race) continue; int dx=vil[v].x-x, dy=vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } } return best; }
static int vilAny(int x,int y,int r){ int best=-1,bd=r*r+1; for(int v=0;v<VIL_N;v++){ if(!vil[v].on) continue; int dx=vil[v].x-x, dy=vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } } return best; }
static int buildable(int x,int y){ if(x<2||y<2||x>=WW-2||y>=WH-2||bld[y][x]) return 0; u8 t=world[y][x]; return t==T_GRASS||t==T_DIRT||t==T_FOREST; }
static void putBld(int v,int x,int y,int type){ if(world[y][x]==T_FOREST) setCell(x,y,T_GRASS); bld[y][x]=(u8)type; bldv[y][x]=(u8)(v+1); if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); }
static int placeBld(int v,int type,int cost){
    Vil*V=&vil[v]; if(V->stock<cost) return 0; int R=3+V->lvl*3;
    for(int k=0;k<14;k++){ int x=V->x+(int)(rnd()%(2*R+1))-R, y=V->y+(int)(rnd()%(2*R+1))-R; if(!buildable(x,y)) continue; if(type==B_HOUSE||type==B_HUT){ int near=0; for(int d=0;d<4;d++) if(bld[y+dys[d]][x+dxs[d]]) near=1; if(!near&&V->houses>1) continue; }
        putBld(v,x,y,type); V->stock-=(u16)cost; return 1; }
    return 0;
}
static int vilFound(int race,int x,int y){
    if(!buildable(x,y)) return -1; if(vilAny(x,y,9)>=0) return -1; int v=-1; for(int i=0;i<VIL_N;i++) if(!vil[i].on){ v=i; break; } if(v<0) return -1;
    Vil*V=&vil[v]; memset(V,0,sizeof *V); V->on=1; V->race=(u8)race; V->x=(u8)x; V->y=(u8)y; V->lvl=1; V->stock=30; V->nm1=(u8)(rnd()%12); V->nm2=(u8)(rnd()%8);
    putBld(v,x,y,B_HALL); for(int k=0;k<2;k++) placeBld(v,B_HUT,0); nVilOn++; return v;
}
static void vilRaze(int v,int ruins){ Vil*V=&vil[v]; if(!V->on) return; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(bldv[y][x]==v+1){ bld[y][x]=ruins&&(rnd()&1)?B_RUIN:B_NONE; if(!bld[y][x]) bldv[y][x]=0; else bldv[y][x]=0; if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); }
    for(int u=0;u<VIL_N;u++) if(vil[u].on&&vil[u].foe==v+1) vil[u].foe=0; V->on=0; nVilOn--; }
static void burnAt(int x,int y,int chance){   // fire, lightning and meteors on a building
    if(x<0||y<0||x>=WW||y>=WH||!bld[y][x]||bld[y][x]==B_RUIN) return; if((int)(rnd()&255)>=chance) return; int v=bldv[y][x]-1;
    if(v>=0&&vil[v].on){ Vil*V=&vil[v]; if(bld[y][x]==B_HOUSE||bld[y][x]==B_HUT){ if(V->houses) V->houses--; } else if(bld[y][x]==B_FARM){ if(V->farms) V->farms--; } else if(bld[y][x]==B_TOWER){ if(V->towers) V->towers--; } }
    bld[y][x]=B_RUIN; bldv[y][x]=0; if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); }
static void vilTick(int v){
    Vil*V=&vil[v]; if(!V->on) return; int pop=0;
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(c->alive&&sp[c->sp].a==V->race&&sp[c->sp].a<8){ int dx=c->x-V->x, dy=c->y-V->y; if(dx*dx+dy*dy<=100) pop++; } }
    V->pop=(u16)pop; if(!pop){ if(++V->dead>=6){ vilRaze(v,1); return; } } else V->dead=0;
    int gain=pop+V->farms*3; V->stock=(u16)(V->stock+gain>400?400:V->stock+gain);
    // what to build next: homes for everyone, fields to feed them, then towers, then a bigger hall
    if(V->lvl>=2&&V->towers<V->houses/3&&V->stock>=25&&(rnd()&3)==0){ if(placeBld(v,B_TOWER,25)) V->towers++; }
    else if(V->houses<2||pop>V->houses*3){ if(placeBld(v,(rnd()&3)?B_HOUSE:B_HUT,12)) V->houses++; }
    else if(V->farms*2<V->houses&&V->stock>=10){ if(placeBld(v,B_FARM,10)) V->farms++; }
    else if(V->lvl>=2&&V->towers<V->houses/3&&V->stock>=25){ if(placeBld(v,B_TOWER,25)) V->towers++; }
    if(V->lvl==1&&V->houses>=6&&V->stock>=40){ if(placeBld(v,B_HALL,40)) V->lvl=2; }
    else if(V->lvl==2&&V->houses>=14&&V->stock>=80){ if(placeBld(v,B_HALL,80)) V->lvl=3; }
    // war: the restless races pick a neighbour of another people; it ends when one side is gone
    if(V->foe&&(!vil[V->foe-1].on)) V->foe=0;
    if(!V->foe&&aggressive(V->race)&&pop>=6&&(rnd()&7)==0){ int best=-1,bd=900; for(int u=0;u<VIL_N;u++){ if(!vil[u].on||u==v||vil[u].race==V->race) continue; int dx=vil[u].x-V->x, dy=vil[u].y-V->y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=u; } } if(best>=0) V->foe=(u8)(best+1); }
    if(V->foe&&(rnd()&15)==0&&pop<3) V->foe=0;   // a beaten army goes home
}
static void warMatrix(void){ memset(warM,0,sizeof warM); for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].foe&&vil[vil[v].foe-1].on){ int a=vil[v].race, b=vil[vil[v].foe-1].race; warM[a][b]=warM[b][a]=1; } }
static int vilTotals(int*houses){ int pop=0,h=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ pop+=vil[v].pop; h+=vil[v].houses; } if(houses) *houses=h; return pop; }

static int stepToward(Cr*c,int i,const Sp*s,int tx,int ty){   // one step towards a tile along the longer axis, then the other
    int dx=tx-c->x, dy=ty-c->y; int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0; int ax=dx<0?-dx:dx, ay=dy<0?-dy:dy; int first=ax>=ay;
    for(int k=0;k<2;k++){ int mx=((k==0)==first)?sx:0, my=((k==0)==first)?0:sy; if(!mx&&!my) continue; int nx=c->x+mx, ny=c->y+my; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return 1; } }
    return 0; }
static void think(int i){
    Cr*c=&cr[i]; if(!c->alive) return; const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a];
    c->age++; if(c->food>2) c->food-=2; else c->food=0;
    if(A->diet==5) c->food=100;
    if(c->food==0){ if(c->hp>2) c->hp-=2; else { killNat(i); return; } }
    if(c->age>A->life){ killNat(i); return; }
    int t=world[c->y][c->x];
    if(t==T_LAVA&&!((A->mask>>T_LAVA)&1)){ killNat(i); return; }
    if(!((A->mask>>t)&1)){ if(c->hp>8) c->hp-=8; else { killNat(i); return; } }
    int diet=A->diet;
    if(diet==5){   // a monster: it hunts every living thing in reach (people first), and burns the houses it passes
        int best=-1,bd=99,rd=99; for(int dy=-10;dy<=10;dy++) for(int dx=-10;dx<=10;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||arch[sp[cr[j].sp].a].diet==5) continue;
            int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); int civ=arch[sp[cr[j].sp].a].diet==4; if(!civ&&d>3) continue; if(d<bd){ bd=d; rd=d; best=j; } }   // people are hunted from ten tiles away, animals only when they are in the way
        for(int k=0;k<4;k++) burnAt(c->x+dxs[k],c->y+dys[k],40);
        if(best>=0){ Cr*p=&cr[best];
            if(rd<=1){ int dm=A->atk+gearAtk(c)+(int)(rnd()&3)-armDef(p); if(dm<1) dm=1; if(p->hp>dm) p->hp-=(u8)dm; else { killCr(best); c->hp=(u8)(c->hp+12>A->hp?A->hp:c->hp+12); } return; }
            if(stepToward(c,i,s,p->x,p->y)) return; }
        else { int v=vilAny(c->x,c->y,60); if(v>=0&&(rnd()&7)) { if(stepToward(c,i,s,vil[v].x,vil[v].y)) return; } }
        if((rnd()&3)!=0){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; if(walkable(s,nx,ny)&&!occ[ny][nx]) moveTo(c,i,nx,ny); }
        return; }
    // eat
    if(c->food<100){
        if(diet==0||diet==2){ if(t==T_GRASS||t==T_JUNGLE||t==T_FOREST){ c->food+=45; if(c->food>120) c->food=120; if(t==T_GRASS&&(rnd()&3)==0) setCell(c->x,c->y,T_DIRT); return; } }
        if(diet==3){ c->food=100; }
        if(diet==4){ int v=vilNear(s->a,c->x,c->y,6); if(v>=0&&vil[v].stock>0){ vil[v].stock--; c->food+=35; if(c->food>120) c->food=120; return; }
            if(t==T_GRASS||t==T_FOREST||t==T_JUNGLE){ c->food+=25; if(c->food>120) c->food=120; return; } }
    }
    if(diet==4){
        int home=vilNear(s->a,c->x,c->y,14);
        // fight: an enemy people within reach is attacked; otherwise an army walks to the enemy village
        { int best=-1,bd=99; for(int dy=-3;dy<=3;dy++) for(int dx=-3;dx<=3;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; int ra=sp[cr[j].sp].a; if(arch[ra].diet!=5&&(ra>=8||!warM[s->a][ra])) continue; int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){ bd=d; best=j; } }
          if(best>=0){ Cr*p=&cr[best]; if(bd<=1){ int dm=A->atk+gearAtk(c)+(int)(rnd()&3); if(p->hp>dm) p->hp-=(u8)dm; else { killCr(best); c->food+=20; } return; }
              int dx=p->x-c->x, dy=p->y-c->y; int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]) moveTo(c,i,nx,ny); return; } }
        if(home>=0&&vil[home].foe&&vil[vil[home].foe-1].on&&c->age>6&&c->hp>8){ const Vil*F=&vil[vil[home].foe-1]; int dx=F->x-c->x, dy=F->y-c->y; if(dx*dx+dy*dy>4){ int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return; } }
            else { int fv=vil[home].foe-1; vil[fv].stock=(u16)(vil[fv].stock>10?vil[fv].stock-10:0); if(rnd()%12==0&&vil[fv].pop<2){ vilRaze(fv,1); } } }
        // settle: a grown person far from their own village starts one
        if(home<0&&c->age>12&&c->food>60&&(rnd()%6)==0&&vilFound(s->a,c->x,c->y)>=0) return;
        // stay close to home
        if(home>=0){ int dx=vil[home].x-c->x, dy=vil[home].y-c->y; if(dx*dx+dy*dy>64){ int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return; } } }
    }
    // hunt
    if((diet==1||(diet==2&&c->food<50))&&c->food<90){
        int best=-1,bd=99; for(int dy=-4;dy<=4;dy++) for(int dx=-4;dx<=4;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i) continue;
                const Arch*B=&arch[sp[cr[j].sp].a]; if(B->diet==1||B->diet==5||B->diet==4&&diet!=1||sp[cr[j].sp].a==s->a) continue; if(diet==2&&B->diet!=0) continue;
                int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){ bd=d; best=j; } }
        if(best>=0){ Cr*p=&cr[best];
            if(bd<=1){ int dm=A->atk+gearAtk(c)+(int)(rnd()&3); if(p->hp>dm) p->hp-=(u8)dm; else { c->food+=80; if(c->food>120) c->food=120; killCr(best); } return; }
            int dx=p->x-c->x, dy=p->y-c->y; int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0; int try1=(((dx<0?-dx:dx)>(dy<0?-dy:dy))?0:1);
            for(int k=0;k<2;k++){ int ax=(k==try1)?sx:0, ay=(k==try1)?0:sy; if(!ax&&!ay) continue; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return; } } } }
    // breed
    int bch=A->breed; if(diet==4&&vilNear(s->a,c->x,c->y,8)>=0) bch*=2;
    if(c->food>=80&&c->age>8&&(int)(rnd()%100)<bch&&nCr<MAXC-4){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; int n=addCr(c->sp,nx,ny); if(n>=0){ cr[n].age=0; cr[n].food=50; c->food-=40; return; } }
    // wander
    if((rnd()&3)!=0){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; if(walkable(s,nx,ny)&&!occ[ny][nx]) moveTo(c,i,nx,ny); }
}
static int monTierMax(void){ int t=(tierOpen-1)/3+year/4; return t>4?4:t; }
static int monCount(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&arch[sp[cr[i].sp].a].diet==5; return n; }
static void monSpawn(void){   // rifts open: monsters of the strength the world has earned appear far from the villages
    if(!monOn||(year<2&&tierOpen<4)) return; int tm=monTierMax(); int cap=(4+tm*6)*WW*WH/16384; if(cap<3) cap=3; if(monCount()>=cap||(rnd()%3)) return;
    int a; for(int k=0;k<8;k++){ a=ARCH_A+(int)(rnd()%(ARCH_N-ARCH_A)); int tr=monTier[a-ARCH_A]; if(tr<=tm&&(tr>=tm-2||(rnd()&7)==0)) break; if(k==7) a=ARCH_A+(int)(rnd()%4); }
    int s=8+(a-8)*16+(int)(rnd()%16); int grp=monTier[a-ARCH_A]==0?1+(int)(rnd()%3):1;
    int home=-1; { int n=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&(rnd()%(++n))==0) home=v; }   // a rift opens 14 to 40 tiles from a village (any, at random)
    for(int k=0;k<40;k++){ int x=rnd()%WW, y=rnd()%WH; if(home>=0){ int r=14+(int)(rnd()%27); int ang=(int)(rnd()&3); x=vil[home].x+(ang&1?r:-r)+(int)(rnd()%9)-4; y=vil[home].y+(ang&2?r:-r)+(int)(rnd()%9)-4; if(x<0||y<0||x>=WW||y>=WH) continue; }
        if(!walkable(&sp[s],x,y)||occ[y][x]||vilAny(x,y,9)>=0) continue; int n=0; for(int q=0;q<grp;q++) n+=addCr(s,x+(q?(int)(rnd()%3)-1:0),y+(q?(int)(rnd()%3)-1:0))>=0; if(n) return; }
}
static void towerFire(void){   // the towers of a village shoot the nearest monsters in reach
    for(int v=0;v<VIL_N;v++){ const Vil*V=&vil[v]; if(!V->on||!V->towers) continue; int dm=4+3*(V->towers>4?4:V->towers), shots=0;
        for(int i=0;i<nCr&&shots<2;i++){ Cr*c=&cr[i]; if(!c->alive||arch[sp[c->sp].a].diet!=5) continue; int dx=c->x-V->x, dy=c->y-V->y; if(dx*dx+dy*dy>100) continue; shots++; if(c->hp>dm) c->hp-=(u8)dm; else killCr(i); } } }
static int cursor=0, vilCur=0;
static void simulate(void){
    if(paused) return;
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(c->ox){ int sp_=arch[sp[c->sp].a].spd; (void)sp_; c->ox+= c->ox<0?1:-1; } if(c->oy){ c->oy+= c->oy<0?1:-1; } }   // glide into the tile
    int n=nCr; if(n==0) return; int per=n>96?96:n;
    for(int k=0;k<per;k++){ if(cursor>=nCr) cursor=0; Cr*c=&cr[cursor]; if(c->alive&&++c->t>=arch[sp[c->sp].a].spd) { c->t=0; think(cursor); } cursor++; }
    // the ground heals: dirt turns back to grass beside grass, lava cools
    for(int k=0;k<12;k++){ int x=rnd()%WW, y=rnd()%WH; u8 t=world[y][x]; if(t==T_DIRT){ int d=rnd()&3; int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&(world[ny][nx]==T_GRASS||world[ny][nx]==T_FOREST)) setCell(x,y,T_GRASS); }
        else if(t==T_GRASS&&(rnd()&63)==0){ int d=rnd()&3; int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&world[ny][nx]==T_FOREST&&(rnd()&7)==0) setCell(x,y,T_FOREST); } }
    if((tick&31)==0){ vilCur=(vilCur+1)%VIL_N; vilTick(vilCur); if(vilCur==0) warMatrix(); }
    if((tick&63)==0) monSpawn();
    if((tick&15)==0) towerFire();
    if(((tick)&255)==0){ compact(); if((tick%2048)==0){ year++; fxReq(FX_YEAR); } unlockTiers(); }
}

// ---------------------------------------------------------------------------------------------- the item catalogue (generated: materials x kinds, plus potions, seeds, foods, relics, gems, god tools)
enum { IK_WEAPON, IK_ARMOR, IK_POTION, IK_SEED, IK_FOOD, IK_RELIC, IK_GEM, IK_TOOL };
typedef struct { char nm[26]; u8 kind, tier, val, sub; } Item;
#define IT_N 400
static Item it[IT_N]; static int nIt, itSel=0, unlockAll=0;
static const char* const matNm[14]={"WOODEN","STONE","BONE","COPPER","BRONZE","IRON","STEEL","SILVER","GOLDEN","CRYSTAL","OBSIDIAN","MITHRIL","DRAGON","STAR"};
static const char* const wepNm[8]={"SWORD","AXE","SPEAR","BOW","MACE","DAGGER","STAFF","HAMMER"}; static const u8 wepAtk[8]={5,6,5,4,7,3,4,8};
static const char* const armNm[6]={"HELM","SHIELD","BREASTPLATE","BOOTS","GAUNTLETS","CLOAK"}; static const u8 armHp[6]={6,9,14,4,5,6};
static const char* const potNm[20]={"HEALING","FEEDING","YOUTH","FERTILITY","RAGE","GIANT","SWIFT","CALM","LUCK","VIGOR","PURITY","WONDER","BRAVERY","SLEEP","GLOW","STONESKIN","HARVEST","FRIENDSHIP","FIRE BREATH","REBIRTH"};
static const char* const seedNm[20]={"OAK","PINE","BIRCH","PALM","JUNGLE","CACTUS","FERN","MOSS","REED","BAMBOO","WILLOW","MAPLE","CYPRESS","BERRY","WHEAT","CLOVER","LOTUS","IVY","MUSHROOM","SUNFLOWER"}; static const u8 seedTerr[20]={T_FOREST,T_FOREST,T_FOREST,T_JUNGLE,T_JUNGLE,T_DESERT,T_GRASS,T_GRASS,T_SWAMP,T_JUNGLE,T_SWAMP,T_FOREST,T_SWAMP,T_GRASS,T_GRASS,T_GRASS,T_WATER,T_JUNGLE,T_FOREST,T_GRASS};
static const char* const foodNm[20]={"BREAD","APPLE","FISH","MEAT","CHEESE","HONEY","STEW","PIE","CAKE","SOUP","BERRIES","MUSHROOMS","NUTS","CARROT","PUMPKIN","SAUSAGE","EGGS","RICE","MELON","FEAST"};
static const char* const relNm[40]={"GOLDEN IDOL","WAR DRUM","SACRED FLAME","LUCKY CHARM","ANCIENT TOME","RAIN STONE","SUN DISC","MOON MIRROR","STORM HORN","HEARTH STONE","KING'S CROWN","PEACE BANNER","TRADE SEAL","HARVEST BELL","FOUNDERS STATUE","DRAGON EGG","STAR MAP","WORLD TREE SEED","TITAN BONE","SPIRIT LANTERN","BLOOD OATH","WISHING WELL","BUILDER'S GAVEL","SCOUT'S COMPASS","HEALER'S STAFF","ORACLE EYE","THUNDER MASK","IRON THRONE","SILVER HARP","JESTER'S CAP","SCHOLAR'S QUILL","MINER'S LAMP","FISHER'S NET","SMITH'S ANVIL","WEAVER'S LOOM","TIDE PEARL","FROST CROWN","EMBER HEART","SAND CLOCK","DREAM CATCHER"};
static const char* const gemNm[20]={"QUARTZ","AMBER","JADE","GARNET","OPAL","TOPAZ","PEARL","ONYX","RUBY","SAPPHIRE","EMERALD","DIAMOND","AMETHYST","TURQUOISE","MOONSTONE","SUNSTONE","BLOODSTONE","AGATE","LAPIS","STAR GEM"};
static const char* const toolNm[12]={"HOE","SHOVEL","PICKAXE","WATERING CAN","TORCH","BELLOWS","CHISEL","SCYTHE","BROOM","SPONGE","SEED BAG","LEVELLER"}; static const char* const toolFx[12]={"DIRT TO GRASS","DIG A LAKE","RAISE HILLS","GROW GRASS","BURN FIELDS","LAVA TO ROCK","FLATTEN HILLS","CLEAR FOREST","DRAIN WATER","DRY SWAMPS","PLANT FOREST","FLATTEN ALL"};
static void makeItems(void){
    nIt=0;
    for(int m=0;m<14;m++) for(int w=0;w<8;w++){ Item*I=&it[nIt++]; strcpy(I->nm,matNm[m]); strcat(I->nm," "); strcat(I->nm,wepNm[w]); I->kind=IK_WEAPON; I->tier=(u8)m; I->val=(u8)(wepAtk[w]+m*2); I->sub=(u8)w; }
    for(int m=0;m<14;m++) for(int a=0;a<6;a++){ Item*I=&it[nIt++]; strcpy(I->nm,matNm[m]); strcat(I->nm," "); strcat(I->nm,armNm[a]); I->kind=IK_ARMOR; I->tier=(u8)m; I->val=(u8)(armHp[a]+m*3); I->sub=(u8)a; }
    for(int k=0;k<20;k++){ Item*I=&it[nIt++]; strcpy(I->nm,"POTION OF "); strcat(I->nm,potNm[k]); I->kind=IK_POTION; I->tier=(u8)(1+k*13/20); I->val=(u8)k; I->sub=(u8)k; }
    for(int k=0;k<20;k++){ Item*I=&it[nIt++]; strcpy(I->nm,seedNm[k]); strcat(I->nm," SEEDS"); I->kind=IK_SEED; I->tier=(u8)(k/3); I->val=seedTerr[k]; I->sub=(u8)k; }
    for(int k=0;k<20;k++){ Item*I=&it[nIt++]; strcpy(I->nm,foodNm[k]); I->kind=IK_FOOD; I->tier=(u8)(k*6/20); I->val=(u8)(30+k*4); I->sub=(u8)k; }
    for(int k=0;k<40;k++){ Item*I=&it[nIt++]; strcpy(I->nm,relNm[k]); I->kind=IK_RELIC; I->tier=(u8)(2+k*11/40); I->val=(u8)(k%5); I->sub=(u8)k; }
    for(int k=0;k<20;k++){ Item*I=&it[nIt++]; strcpy(I->nm,gemNm[k]); I->kind=IK_GEM; I->tier=(u8)(1+k*12/20); I->val=(u8)(10+k*8); I->sub=(u8)k; }
    for(int m=0;m<4;m++) for(int t=0;t<12;t++){ Item*I=&it[nIt++]; strcpy(I->nm,m==0?"":m==1?"IRON ":m==2?"GOLD ":"STAR "); strcat(I->nm,toolNm[t]); I->kind=IK_TOOL; I->tier=(u8)(m*4+t/3); I->val=(u8)t; I->sub=(u8)m; }
}
static int itOpen(int i){ return unlockAll||it[i].tier<=tierOpen; }
static int itNext(int from,int d){ for(int k=1;k<=nIt;k++){ int j=(from+d*k%nIt+nIt*2)%nIt; if(itOpen(j)) return j; } return from; }
static int itCount(int*open){ int o=0; for(int i=0;i<nIt;i++) o+=itOpen(i); if(open) *open=o; return nIt; }
static int gearAtk(const Cr*c){ return c->wep?it[c->wep-1].val:0; }
static int armDef(const Cr*c){ return c->arm?it[c->arm-1].val/2:0; }
static void itSay(char*out,int i){ const Item*I=&it[i]; static const char*kn[8]={"ATK","HP","POTION","SEED","FOOD","RELIC","GEM","TOOL"}; if(I->kind==IK_WEAPON||I->kind==IK_ARMOR) siprintf(out,"%s %s+%d",I->nm,kn[I->kind],I->val); else if(I->kind==IK_TOOL) siprintf(out,"%s: %s",I->nm,toolFx[I->val]); else siprintf(out,"%s",I->nm); }
// ---------------------------------------------------------------------------------------------- the story: fourteen chapters; each one done opens a tier of items (and stronger monsters) for free play, for good
typedef struct { const char*nm, *goal; u8 kind; u16 target; } Chap;
enum { OB_USE, OB_FOLK, OB_HOMES, OB_PEOPLES, OB_SLAY, OB_GIFT, OB_L3, OB_SEEN, OB_YEAR, OB_FINAL };
#define CHAP_N 14
static const Chap chap[CHAP_N]={
 {"LET THERE BE LIGHT","USE YOUR POWERS 20 TIMES",OB_USE,20}, {"THE FIRST FOLK","20 FOLK IN YOUR WORLD",OB_FOLK,20}, {"A ROOF OVERHEAD","BUILD 8 HOMES",OB_HOMES,8}, {"TWO PEOPLES","TWO PEOPLES WITH VILLAGES",OB_PEOPLES,2},
 {"THE FIRST DARKNESS","SLAY 5 MONSTERS",OB_SLAY,5}, {"ARMS FOR THE FAITHFUL","GIVE 5 GIFTS",OB_GIFT,5}, {"A HALL OF STONE","GROW A LEVEL 3 HALL",OB_L3,1}, {"THE BESTIARY","MEET 15 KINDS OF MONSTER",OB_SEEN,15},
 {"AGE OF KINGS","REACH YEAR 8",OB_YEAR,8}, {"THE HORDE","SLAY 40 MONSTERS",OB_SLAY,40}, {"THE GREAT CITY","BUILD 40 HOMES",OB_HOMES,40}, {"A GOD'S LIBRARY","MEET 80 KINDS OF MONSTER",OB_SEEN,80},
 {"THE LONG PEACE","250 FOLK IN YOUR WORLD",OB_FOLK,250}, {"GODHOOD","SLAY 150, REACH YEAR 16",OB_FINAL,1} };
static int chapVal(int kind){
    int hh; switch(kind){ case OB_USE: return (int)stUse; case OB_FOLK: return vilTotals(&hh); case OB_HOMES: vilTotals(&hh); return hh; case OB_SLAY: return (int)stKill; case OB_GIFT: return (int)stGift;
        case OB_SEEN: return seenN; case OB_YEAR: return year; case OB_FINAL: return stKill>=150&&year>=16;
        case OB_PEOPLES: { int m=0; for(int v=0;v<VIL_N;v++) if(vil[v].on) m|=1<<vil[v].race; int n=0; while(m){ n+=m&1; m>>=1; } return n; }
        default: { int l3=0; for(int v=0;v<VIL_N;v++) l3+=vil[v].on&&vil[v].lvl>=3; return l3; } } }
static void unlockTiers(void){   // every 256 ticks: is the chapter you are on done?
    if(!storyOn||storyDone||chapter>=CHAP_N) return;
    const Chap*C=&chap[chapter]; if(chapVal(C->kind)<C->target) return;
    siprintf(info,"CH%d DONE: %s",chapter+1,C->nm); chapter++; if(chapter>=CHAP_N){ storyDone=1; tierOpen=14; musReq(GM_GODHOOD); fxReq(FX_CHAPTER_DONE); }
    else { if(chapter+1>tierOpen) tierOpen=chapter+1>14?14:chapter+1; musReq(GM_ASCEND); fxReq(FX_CHAPTER_DONE); } }
// ---------------------------------------------------------------------------------------------- powers (the table is the catalogue)
typedef struct { const char*nm; u8 kind, arg; } Pw;
enum { PK_INSPECT, PK_PAINT, PK_SPAWN, PK_RAIN, PK_FIRE, PK_BOLT, PK_METEOR, PK_KILL, PK_HEAL, PK_FEED, PK_SETTLE, PK_WAR, PK_PEACE, PK_BLESS, PK_GIFT };
static const Pw pw[]={ {"INSPECT",PK_INSPECT,0},{"WATER",PK_PAINT,T_WATER},{"DEEP SEA",PK_PAINT,T_DEEP},{"SAND",PK_PAINT,T_SAND},{"GRASS",PK_PAINT,T_GRASS},{"FOREST",PK_PAINT,T_FOREST},{"JUNGLE",PK_PAINT,T_JUNGLE},
    {"HILLS",PK_PAINT,T_HILL},{"MOUNTAIN",PK_PAINT,T_MOUNT},{"SNOW",PK_PAINT,T_SNOW},{"DESERT",PK_PAINT,T_DESERT},{"SWAMP",PK_PAINT,T_SWAMP},{"LAVA",PK_PAINT,T_LAVA},
    {"SPAWN",PK_SPAWN,0},{"RAIN",PK_RAIN,0},{"FIRE",PK_FIRE,0},{"LIGHTNING",PK_BOLT,0},{"METEOR",PK_METEOR,0},{"SMITE",PK_KILL,0},{"HEAL",PK_HEAL,0},{"FEED",PK_FEED,0},{"SETTLE",PK_SETTLE,0},{"WAR",PK_WAR,0},{"PEACE",PK_PEACE,0},{"BLESS",PK_BLESS,0},{"GIFT",PK_GIFT,0} };
#define PW_N ((int)(sizeof(pw)/sizeof(pw[0])))
static int inspected=-1;
static void usePower(int tx,int ty,int fresh){
    const Pw*P=&pw[power]; int r=brush;
    if(fresh){ stUse++; if(P->kind==PK_GIFT) stGift++; static const u8 fxOf[15]={FX_INSPECT,FX_PAINT,FX_SPAWN,FX_RAIN,FX_FIRE,FX_LIGHTNING,FX_METEOR,FX_SMITE,FX_HEAL,FX_FEED,FX_SETTLE,FX_WAR,FX_PEACE,FX_BLESS,FX_GIFT}; fxReq(fxOf[P->kind]); } if(P->kind==PK_GIFT&&(it[itSel].kind==IK_WEAPON||it[itSel].kind==IK_ARMOR||it[itSel].kind==IK_RELIC||it[itSel].kind==IK_GEM)) r=1;
    for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ if(dx*dx+dy*dy>r*r+r/2) continue; int x=tx+dx, y=ty+dy; if(x<0||y<0||x>=WW||y>=WH) continue;
        switch(P->kind){
        case PK_PAINT: if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } setCell(x,y,P->arg); if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(!walkable(&sp[c->sp],x,y)&&(rnd()&3)==0) killCr(occ[y][x]-1); } break;
        case PK_SPAWN: if(fresh&&dx==0&&dy==0){ for(int k=0;k<brush;k++) addCr(spSel,x+(k?(int)(rnd()%3)-1:0),y+(k?(int)(rnd()%3)-1:0)); } break;
        case PK_RAIN: if(world[y][x]==T_DIRT||world[y][x]==T_SAND||world[y][x]==T_DESERT) { if((rnd()&3)==0) setCell(x,y,T_GRASS); } else if(world[y][x]==T_LAVA) setCell(x,y,T_MOUNT); else if(world[y][x]==T_GRASS&&(rnd()&15)==0) setCell(x,y,T_FOREST); break;
        case PK_FIRE: burnAt(x,y,90); if(world[y][x]==T_FOREST||world[y][x]==T_GRASS||world[y][x]==T_JUNGLE){ if((rnd()&1)==0) setCell(x,y,T_DIRT); } if(occ[y][x]&&(rnd()&1)) { Cr*c=&cr[occ[y][x]-1]; if(c->hp>10) c->hp-=10; else killCr(occ[y][x]-1); } break;
        case PK_BOLT: if(dx==0&&dy==0){ burnAt(x,y,255); if(occ[y][x]) killCr(occ[y][x]-1); if(world[y][x]==T_FOREST) setCell(x,y,T_DIRT); } break;
        case PK_METEOR: if(fresh){ int rr=dx*dx+dy*dy; burnAt(x,y,255); if(rr<=2) setCell(x,y,T_LAVA); else if(rr<=r*r) setCell(x,y,(rnd()&1)?T_DIRT:T_HILL); if(occ[y][x]) killCr(occ[y][x]-1); } break;
        case PK_KILL: if(occ[y][x]) killCr(occ[y][x]-1); break;
        case PK_HEAL: if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; c->hp=arch[sp[c->sp].a].hp; } break;
        case PK_FEED: if(occ[y][x]) cr[occ[y][x]-1].food=120; else if(world[y][x]==T_DIRT) setCell(x,y,T_GRASS); break;
        case PK_SETTLE: if(fresh&&dx==0&&dy==0){ int race=spSel<8?sp[spSel].a:0; if(vilFound(race,x,y)<0) siprintf(info,"NO ROOM FOR A VILLAGE"); else { for(int k=0;k<4;k++) addCr(spSel<8?spSel:0,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); } } break;
        case PK_WAR: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,4); if(v>=0){ int best=-1,bd=2000; for(int u=0;u<VIL_N;u++){ if(!vil[u].on||u==v||vil[u].race==vil[v].race) continue; int ddx=vil[u].x-vil[v].x, ddy=vil[u].y-vil[v].y, d=ddx*ddx+ddy*ddy; if(d<bd){ bd=d; best=u; } } if(best>=0){ vil[v].foe=(u8)(best+1); siprintf(info,"WAR! %s ATTACKS %s",arch[vil[v].race].nm,arch[vil[best].race].nm); } } } break;
        case PK_PEACE: if(fresh&&dx==0&&dy==0){ for(int v=0;v<VIL_N;v++) vil[v].foe=0; warMatrix(); siprintf(info,"PEACE"); } break;
        case PK_BLESS: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,4); if(v>=0){ vil[v].stock=(u16)(vil[v].stock+150>400?400:vil[v].stock+150); } } break;
        case PK_GIFT: { const Item*I=&it[itSel]; Cr*c=occ[y][x]?&cr[occ[y][x]-1]:0; int cen=(dx==0&&dy==0);
            switch(I->kind){
            case IK_WEAPON: if(c&&cen&&fresh){ c->wep=(u16)(itSel+1); } break;
            case IK_ARMOR: if(c&&cen&&fresh){ int old=c->arm?it[c->arm-1].val:0; c->arm=(u16)(itSel+1); int hp=c->hp+I->val*2-old*2; c->hp=(u8)(hp>250?250:hp<1?1:hp); } break;
            case IK_FOOD: if(c){ int f=c->food+I->val*2; c->food=(u8)(f>250?250:f); } else if(world[y][x]==T_DIRT) setCell(x,y,T_GRASS); break;
            case IK_POTION: if(c){ switch(I->sub){ case 0: case 19: c->hp=arch[sp[c->sp].a].hp; if(I->sub==19){ c->age=0; c->food=120; } break; case 1: case 16: c->food=250; break; case 2: c->age=0; break; case 3: if(cen&&fresh){ int n=addCr(c->sp,c->x+1,c->y); if(n<0) addCr(c->sp,c->x,c->y+1); } break; case 4: c->wep=(u16)(c->wep?c->wep:1); break; case 5: { int hp=c->hp+40; c->hp=(u8)(hp>250?250:hp); } break; case 15: { int hp=c->hp+60; c->hp=(u8)(hp>250?250:hp); } break; default: c->food=100; } } break;
            case IK_SEED: if(world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_LAVA&&!bld[y][x]&&(rnd()&1)) setCell(x,y,(u8)I->val); break;
            case IK_RELIC: case IK_GEM: if(cen&&fresh){ int v=vilAny(x,y,5); if(v>=0){ Vil*V=&vil[v]; if(I->kind==IK_GEM) V->stock=(u16)(V->stock+I->val*3>400?400:V->stock+I->val*3); else switch(I->val){ case 0: V->stock=400; break; case 1: for(int k=0;k<3;k++) if(placeBld(v,B_HOUSE,0)) V->houses++; break; case 2: if(V->lvl<3) V->lvl++; break; case 3: V->foe=0; break; default: for(int k=0;k<8;k++) addCr(0,V->x+(int)(rnd()%5)-2,V->y+(int)(rnd()%5)-2); } } } break;
            case IK_TOOL: { int t=I->val; u8 w=world[y][x]; if(bld[y][x]) break;
                switch(t){ case 0: if(w==T_DIRT) setCell(x,y,T_GRASS); break; case 1: if(w==T_GRASS||w==T_SAND||w==T_DIRT) setCell(x,y,T_WATER); break; case 2: if(w==T_GRASS||w==T_FOREST) setCell(x,y,T_HILL); else if(w==T_HILL) setCell(x,y,T_MOUNT); break; case 3: if(w==T_DIRT||w==T_SAND||w==T_DESERT) setCell(x,y,T_GRASS); break;
                    case 4: if(w==T_GRASS||w==T_FOREST) setCell(x,y,T_DIRT); break; case 5: if(w==T_LAVA) setCell(x,y,T_MOUNT); break; case 6: if(w==T_MOUNT) setCell(x,y,T_HILL); else if(w==T_HILL) setCell(x,y,T_GRASS); break; case 7: if(w==T_FOREST||w==T_JUNGLE) setCell(x,y,T_GRASS); break;
                    case 8: if(w==T_WATER) setCell(x,y,T_SAND); break; case 9: if(w==T_SWAMP) setCell(x,y,T_GRASS); break; case 10: if(w==T_GRASS&&(rnd()&1)) setCell(x,y,T_FOREST); break; default: if(w==T_HILL||w==T_MOUNT||w==T_FOREST||w==T_JUNGLE) setCell(x,y,T_GRASS); } } break; } } break;
        case PK_INSPECT: if(dx==0&&dy==0){ if(occ[y][x]) inspected=occ[y][x]-1; else if(bldv[y][x]){ int v=bldv[y][x]-1; char nm[24]; vilName(v,nm); siprintf(info,"%s %s POP%d HOMES%d",arch[vil[v].race].nm,nm,vil[v].pop,vil[v].houses); inspected=-2; } } break; } }
    if(P->kind==PK_INSPECT&&inspected!=-2){ if(inspected>=0&&inspected<nCr&&cr[inspected].alive){ Cr*c=&cr[inspected]; char nm[32]; spName(c->sp,nm); siprintf(info,"%s HP%d FOOD%d AGE%d",nm,c->hp,c->food,c->age); } else siprintf(info,"%s",bioNm[world[ty<WH?ty:0][tx<WW?tx:0]]); }
}
static int planetSize=128;
static void newWorld(void){ gW=gH=planetSize; nCr=0; memset(occ,0,sizeof occ); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(vil,0,sizeof vil); nVilOn=0; genWorld(rnd()); winX=-99; refillWindow();
    // the first life: a few of each of the first animals on their own ground
    int nAn=260*WW*WH/16384; if(nAn>900) nAn=900;
    for(int k=0;k<nAn;k++){ int a=8+(int)(rnd()%(ARCH_A-8)); int s=8+(a-8)*16+(int)(rnd()%16); int x=rnd()%WW, y=rnd()%WH; if(arch[a].diet==3&&k%2) continue; addCr(s,x,y); }
    for(int r=0;r<8;r++){ if(r>=5&&(rnd()&1)) continue; for(int k=0;k<80;k++){ int x=rnd()%WW, y=rnd()%WH; int v=vilFound(r,x,y); if(v>=0){ for(int q=0;q<5;q++) addCr(r,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); break; } } }   // the peoples start with a village each (the restless ones sometimes stay away)
    info[0]=0; }
// ---------------------------------------------------------------------------------------------- saving: the whole world, the villages and the story, to one file (the SD card on the DS)
static int shape3;
#define SAVE_MAGIC 0x31534747u
typedef struct { u32 magic; u16 ver, gw, szCr, szVil; u32 tick, year, rs; u32 stKill, stGift, stUse; u16 nCr, seenN, camX, camY; u8 chapter, tierOpen, storyOn, storyDone, monOn, shape3, nVilOn, planet, vcur; u8 seen[(SP_N+7)/8], slain[(SP_N+7)/8]; } SaveHdr;
static int saveGame(const char*path){
    compact(); cursor=0;     // the dead are dropped first, so a loaded game goes on exactly as this one would
    FILE*f=fopen(path,"wb"); if(!f) return 0; SaveHdr h; memset(&h,0,sizeof h); h.vcur=(u8)vilCur;
    h.magic=SAVE_MAGIC; h.ver=1; h.gw=(u16)gW; h.szCr=sizeof(Cr); h.szVil=sizeof(Vil); h.tick=(u32)tick; h.year=(u32)year; h.rs=rs; h.stKill=stKill; h.stGift=stGift; h.stUse=stUse; h.nCr=(u16)nCr; h.seenN=(u16)seenN; h.camX=(u16)camX; h.camY=(u16)camY;
    h.chapter=(u8)chapter; h.tierOpen=(u8)tierOpen; h.storyOn=(u8)storyOn; h.storyDone=(u8)storyDone; h.monOn=(u8)monOn; h.shape3=(u8)shape3; h.nVilOn=(u8)nVilOn; h.planet=(u8)planetSize/2; memcpy(h.seen,seenB,sizeof seenB); memcpy(h.slain,slainB,sizeof slainB);
    int ok=fwrite(&h,sizeof h,1,f)==1;
    for(int y=0;y<gH&&ok;y++){ ok&=fwrite(world[y],1,gW,f)==(size_t)gW; ok&=fwrite(bld[y],1,gW,f)==(size_t)gW; ok&=fwrite(bldv[y],1,gW,f)==(size_t)gW; }
    if(ok&&nCr) ok&=fwrite(cr,sizeof(Cr),nCr,f)==(size_t)nCr; if(ok) ok&=fwrite(vil,sizeof(Vil),VIL_N,f)==(size_t)VIL_N;
    fclose(f); return ok; }
static int loadGame(const char*path){
    FILE*f=fopen(path,"rb"); if(!f) return 0; SaveHdr h; int ok=fread(&h,sizeof h,1,f)==1;
    if(!ok||h.magic!=SAVE_MAGIC||h.ver!=1||h.szCr!=sizeof(Cr)||h.szVil!=sizeof(Vil)||(h.gw!=128&&h.gw!=192&&h.gw!=256)||h.nCr>MAXC){ fclose(f); return 0; }
    gW=gH=h.gw; planetSize=gW; memset(world,0,sizeof world); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(occ,0,sizeof occ);
    for(int y=0;y<gH&&ok;y++){ ok&=fread(world[y],1,gW,f)==(size_t)gW; ok&=fread(bld[y],1,gW,f)==(size_t)gW; ok&=fread(bldv[y],1,gW,f)==(size_t)gW; }
    nCr=h.nCr; if(ok&&nCr) ok&=fread(cr,sizeof(Cr),nCr,f)==(size_t)nCr; if(ok) ok&=fread(vil,sizeof(Vil),VIL_N,f)==(size_t)VIL_N; fclose(f);
    if(!ok){ return 0; }
    tick=(int)h.tick; year=(int)h.year; rs=h.rs; stKill=h.stKill; stGift=h.stGift; stUse=h.stUse; seenN=h.seenN; camX=h.camX; camY=h.camY; chapter=h.chapter; tierOpen=h.tierOpen; storyOn=h.storyOn; storyDone=h.storyDone; monOn=h.monOn; shape3=h.shape3; nVilOn=h.nVilOn;
    memcpy(seenB,h.seen,sizeof seenB); memcpy(slainB,h.slain,sizeof slainB); nVilOn=0; for(int v=0;v<VIL_N;v++) nVilOn+=vil[v].on;
    for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].x<gW&&cr[i].y<gH) occ[cr[i].y][cr[i].x]=(u16)(i+1); vilCur=h.vcur; cursor=0; compact(); warMatrix(); winX=-99; return 1; }
#define MN 28
static int shape3=0, texId=-1;   // zoom3: camera distance in 1/16 planet radii
static s16 mvx[(MN+1)*(MN+1)], mvy[(MN+1)*(MN+1)], mvz[(MN+1)*(MN+1)], mnx[(MN+1)*(MN+1)], mny[(MN+1)*(MN+1)], mnz[(MN+1)*(MN+1)];
static u16 tex[128*128];
static float csp(float a,float e){ float c=cosf(a); float r=powf(fabsf(c)+1e-6f,e); return c<0?-r:r; }
static float snp(float a,float e){ float c=sinf(a); float r=powf(fabsf(c)+1e-6f,e); return c<0?-r:r; }
static void buildMesh(void){
    for(int j=0;j<=MN;j++) for(int i=0;i<=MN;i++){ int k=j*(MN+1)+i; float x,y,z,nx,ny,nz;
        if(shape3==2){ float u=6.2831853f*i/MN, v=6.2831853f*j/MN, R=0.72f, r=0.36f; x=(R+r*cosf(v))*cosf(u); z=(R+r*cosf(v))*sinf(u); y=r*sinf(v); nx=cosf(v)*cosf(u); ny=sinf(v); nz=cosf(v)*sinf(u); }
        else { float u=6.2831853f*i/MN, v=-1.5707963f+3.1415927f*j/MN, e=shape3==0?1.0f:0.28f;
            x=csp(v,e)*csp(u,e); y=snp(v,e); z=csp(v,e)*snp(u,e); nx=csp(v,2-e)*csp(u,2-e); ny=snp(v,2-e); nz=csp(v,2-e)*snp(u,2-e); }
        float l=sqrtf(nx*nx+ny*ny+nz*nz)+1e-6f; nx/=l; ny/=l; nz/=l;
        mvx[k]=(s16)(x*4096*0.9f); mvy[k]=(s16)(y*4096*0.9f); mvz[k]=(s16)(z*4096*0.9f); mnx[k]=(s16)(nx*500); mny[k]=(s16)(ny*500); mnz[k]=(s16)(nz*500); }
}
static void fillTexture(void){
    int bs=(WW+127)/128;   // a bigger planet is squeezed into the 128 x 128 texture: a texel covers bs x bs tiles, and a house or a person in any of them shows
    for(int y=0;y<128;y++) for(int x=0;x<128;x++){ int wx=x*WW/128, wy=y*WH/128; int t=world[wy][wx]; int r,g,b; shadeRGB(t*8+3+(int)(hsh(x,y,5)&3),&r,&g,&b);
        for(int dy=0;dy<bs&&wy+dy<WH;dy++) for(int dx=0;dx<bs&&wx+dx<WW;dx++){ int yy=wy+dy, xx=wx+dx;
            if(bld[yy][xx]&&bld[yy][xx]!=B_RUIN){ int rc=bldv[yy][xx]?vil[bldv[yy][xx]-1].race:0; r=roofCol[rc][0]; g=roofCol[rc][1]; b=roofCol[rc][2]; }
            if(occ[yy][xx]){ int da=arch[sp[cr[occ[yy][xx]-1].sp].a].diet; if(da==4){ r=31; g=31; b=24; } else if(da==5){ r=31; g=2; b=2; } } }
        tex[y*128+x]=(u16)(r|(g<<5)|(b<<10)|0x8000); }
}
#ifndef PCTEST
static void uploadTexture(void){ fillTexture();
    glResetTextures(); glGenTextures(1,&texId); glBindTexture(0,texId); glTexImage2D(0,0,GL_RGB,TEXTURE_SIZE_128,TEXTURE_SIZE_128,0,TEXGEN_TEXCOORD|GL_TEXTURE_WRAP_S|GL_TEXTURE_WRAP_T,(u8*)tex);
}
#endif
static int view3d=0, rotX=0, rotY=0, zoom3=40, glOn=0, texDirty=0;
// where the stylus touches the planet: every grid point is moved and projected exactly as the 3D hardware does (turn about Y, then X, then away from the camera, perspective 55 degrees),
// then the nearest quad under the pixel gives the point on the map. Returns 1 and the map tile if the planet was hit.
static int pick3D(int px,int py,int*tx,int*ty){
    static float P[(MN+1)*(MN+1)][3]; static float uu[(MN+1)*(MN+1)], vv[(MN+1)*(MN+1)];
    float dist=zoom3/16.0f, ra=rotY*0.0174533f, rb=rotX*0.0174533f, cy=cosf(ra), sy=sinf(ra), cx=cosf(rb), sx=sinf(rb), f=184.4f;
    for(int j=0;j<=MN;j++) for(int i=0;i<=MN;i++){ int k=j*(MN+1)+i; float x=mvx[k]/4096.f, y=mvy[k]/4096.f, z=mvz[k]/4096.f;
        float x1=cy*x+sy*z, z1=-sy*x+cy*z, y1=cx*y-sx*z1, z2=sx*y+cx*z1+0.f; float zz=z2-dist; P[k][0]=128+x1/(-zz)*f; P[k][1]=96-y1/(-zz)*f; P[k][2]=-zz; uu[k]=(float)i; vv[k]=(float)j; }
    float best=1e9f, bu=0, bv=0;
    for(int j=0;j<MN;j++) for(int i=0;i<MN;i++){ int k0=j*(MN+1)+i, k1=k0+1, k2=k0+MN+2, k3=k0+MN+1; int tri[2][3]={{k0,k1,k2},{k0,k2,k3}};
        for(int q=0;q<2;q++){ const float*A=P[tri[q][0]],*B=P[tri[q][1]],*C=P[tri[q][2]]; float d=(B[1]-C[1])*(A[0]-C[0])+(C[0]-B[0])*(A[1]-C[1]); if(d>-1e-6f&&d<1e-6f) continue;
            float a=((B[1]-C[1])*(px-C[0])+(C[0]-B[0])*(py-C[1]))/d, b=((C[1]-A[1])*(px-C[0])+(A[0]-C[0])*(py-C[1]))/d, c=1-a-b; if(a<0||b<0||c<0) continue;
            float z=a*A[2]+b*B[2]+c*C[2]; if(z<best){ best=z; bu=a*uu[tri[q][0]]+b*uu[tri[q][1]]+c*uu[tri[q][2]]; bv=a*vv[tri[q][0]]+b*vv[tri[q][1]]+c*vv[tri[q][2]]; } } }
    if(best>=1e8f) return 0; int x=(int)(bu*WW/MN), y=(int)(bv*WH/MN); if(x<0) x=0; if(x>WW-1) x=WW-1; if(y<0) y=0; if(y>WH-1) y=WH-1; *tx=x; *ty=y; return 1;
}
#ifndef PCTEST
// ---------------------------------------------------------------------------------------------- screens
// ---- light: the day goes round and the water shimmers (a tint on the background palette) ----
static u16 basePal[256];
static void saveBasePal(void){ for(int i=0;i<144;i++) basePal[i]=BG_PALETTE[i]; }
static void applyLight(void){
    int p=(tick>>1)&2047, sun=p<1024?p:2047-p; int L=70+sun*186/1024, Lr=L*236/256, Lg=L*246/256, Lb=L+(256-L)/5+14; if(Lb>256) Lb=256; if(L>256) L=256;
    int sh=(tick>>4)&3;
    for(int i=0;i<144;i++){ int src=i; if(i>=T_WATER*8+5&&i<T_WATER*8+8) src=T_WATER*8+5+((i-T_WATER*8-5+sh)%3);
        u16 c=basePal[src]; int r=(c&31)*Lr>>8, g=((c>>5)&31)*Lg>>8, b=((c>>10)&31)*Lb>>8; if(r>31) r=31; if(g>31) g=31; if(b>31) b=31; BG_PALETTE[i]=(u16)(r|(g<<5)|(b<<10)); }
    BG_PALETTE[0]=0;
}
// ---- the planet in 3D: sphere, cube-planet and ring-world, all wearing the 128 x 128 map as a texture ----
static void enter3D(void){
    videoSetMode(MODE_0_3D);
    if(!glOn){ glInit(); glEnable(GL_TEXTURE_2D); glEnable(GL_ANTIALIAS); vramSetBankD(VRAM_D_TEXTURE); glClearColor(0,0,3,31); glClearPolyID(63); glClearDepth(0x7FFF); glViewport(0,0,255,191); glOn=1; }
    lcdMainOnBottom(); buildMesh(); uploadTexture(); view3d=1;   // (the planet is on the touch screen: the stylus uses the powers on it)
}
static void leave3D(void){
    videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32); lcdMainOnBottom();
    bgInit(0,BgType_Text8bpp,BgSize_T_512x512,0,1); mapPtr=(u16*)bgGetMapPtr(0); winX=-99; view3d=0;
}
static void draw3D(void){
    glMatrixMode(GL_PROJECTION); glLoadIdentity(); gluPerspective(55,256.0/192.0,0.05,40);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glLight(0,RGB15(31,31,29),floattov10(-0.55),floattov10(-0.35),floattov10(-0.75));
    glTranslatef32(0,0,-(zoom3*4096/16)); glRotateX((float)rotX); glRotateY((float)rotY);
    glMaterialf(GL_AMBIENT,RGB15(13,13,14)); glMaterialf(GL_DIFFUSE,RGB15(20,20,19)); glMaterialf(GL_SPECULAR,RGB15(0,0,0)); glMaterialf(GL_EMISSION,RGB15(2,2,3));
    glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE|POLY_FORMAT_LIGHT0);
    glBindTexture(0,texId);
    glBegin(GL_QUAD);
    for(int j=0;j<MN;j++) for(int i=0;i<MN;i++){
        int k0=j*(MN+1)+i, k1=k0+1, k2=k0+MN+2, k3=k0+MN+1; int ks[4]={k0,k1,k2,k3}; int us[4]={i,i+1,i+1,i}, vs[4]={j,j,j+1,j+1};
        for(int q=0;q<4;q++){ int k=ks[q]; glNormal(NORMAL_PACK(mnx[k],mny[k],mnz[k])); GFX_TEX_COORD=TEXTURE_PACK(inttot16(us[q]*128/MN),inttot16(vs[q]*128/MN)); glVertex3v16(mvx[k],mvy[k],mvz[k]); } }
    glEnd(); glFlush(0);
}
// ---- the sound backend: the libnds channels behind gmplay.h. Wave voices are looped 64-sample PCM channels that are only retuned (never restarted) while a song plays; drums are one-shots.
static void gb_vstart(int v,int wave); static void gb_vstop(int v); static void gb_vset(int v,int rate,int vol,int pan); static void gb_hit(int slot,int part,int idx,int vol,int pan);
static void gb_ptone(int midi8,int vol,int duty); static void gb_poff(void); static void gb_pnoise(int freq,int vol); static void gb_pnoff(void);
#include "gmplay.h"
static int gvId[GMV]={-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, gvRate[GMV], gvVol[GMV], gvPan[GMV], ghId[4]={-1,-1,-1,-1}, gpId=-1, gnId=-1;
static void gb_vstop(int v){ if(gvId[v]>=0){ soundKill(gvId[v]); gvId[v]=-1; } }
static void gb_vstart(int v,int wave){ gb_vstop(v); gvId[v]=soundPlaySample(gmWaveT[wave],SoundFormat_8Bit,64,gmHz64[60],0,64,true,0); gvRate[v]=gmHz64[60]; gvVol[v]=0; gvPan[v]=64; }
static void gb_vset(int v,int rate,int vol,int pan){ int id=gvId[v]; if(id<0) return; if(rate>65535) rate=65535; if(vol<0) vol=0; if(vol>127) vol=127;
    if(rate!=gvRate[v]){ soundSetFreq(id,(u16)rate); gvRate[v]=rate; } if(vol!=gvVol[v]){ soundSetVolume(id,(u8)vol); gvVol[v]=vol; } if(pan!=gvPan[v]){ soundSetPan(id,(u8)pan); gvPan[v]=pan; } }
static void gb_hit(int slot,int part,int idx,int vol,int pan){ if(ghId[slot]>=0) soundKill(ghId[slot]); const GmDrum*D=part?&gmKitB[idx]:&gmKitA[idx]; if(vol>127) vol=127;
    ghId[slot]=soundPlaySample(D->d,SoundFormat_8Bit,D->n,D->rate,(u8)vol,(u8)pan,false,0); }
static int midi8Hz(int m8){ int m=m8>>3; if(m<0) m=0; if(m>126) m=126; int a=gmHz64[m], b=gmHz64[m+1]; return (a+(b-a)*(m8&7)/8)/64; }
static void gb_ptone(int midi8,int vol,int duty){ (void)duty; if(gpId<0) return; int hz=midi8Hz(midi8); if(hz<20) hz=20; if(vol>127) vol=127; if(vol<0) vol=0; soundSetFreq(gpId,(u16)hz); soundSetVolume(gpId,(u8)vol); }   // (one reserved PSG voice, 50% pulse: the 16 channels are all spoken for)
static void gb_poff(void){ if(gpId>=0) soundSetVolume(gpId,0); }
static void gb_pnoise(int freq,int vol){ if(gnId<0) return; if(vol>127) vol=127; if(vol<0) vol=0; if(freq>60000) freq=60000; soundSetFreq(gnId,(u16)freq); soundSetVolume(gnId,(u8)vol); }
static void gb_pnoff(void){ if(gnId>=0) soundSetVolume(gnId,0); }
// ---- the music director: what is happening in the world picks the track (with a 30 second hold so it never flips about); jingles play once and hand back
static int musHold, jukeOn, fxCool;
static int isJingle(int s){ return s>=0&&gmSongs[s].loopRow==65535; }
static int musDirector(void){
    if(paused) return GM_PAUSE;
    if(view3d) return shape3==0?GM_SPHERE:shape3==1?GM_CUBE:GM_RING;
    int cx=(camX>>3)+16, cy=(camY>>3)+12;
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(!c->alive) continue; int a=sp[c->sp].a; if(arch[a].diet!=5||c->hp<100) continue; int dx=c->x-cx, dy=c->y-cy; if(dx*dx+dy*dy>900) continue;
        if(a==ARCH_A+19) return GM_DRAGON; if(a==ARCH_A+17) return GM_DEMON; if(a==ARCH_A+18) return GM_LICH; if(a==ARCH_A+13||a==ARCH_A+12) return GM_KRAKEN; if(a==ARCH_A+16) return GM_DRAGON; }
    for(int a=0;a<8;a++) for(int b=a+1;b<8;b++) if(warM[a][b]) return GM_WAR;
    if(monCount()>0) return GM_MONSTER;
    { int t=world[cy<WH?cy:WH-1][cx<WW?cx:WW-1]; if(t==T_DESERT) return GM_DESERT; if(t==T_SNOW) return GM_SNOW; if(t==T_DEEP||t==T_WATER) return GM_OCEAN; if(t==T_JUNGLE) return GM_JUNGLE; if(t==T_SWAMP) return GM_SWAMP; if(t==T_LAVA||t==T_MOUNT) return GM_VOLCANO; }
    { int lv=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].lvl>lv) lv=vil[v].lvl; if(lv>=3) return GM_CITY; if(lv==2) return GM_MARKET; if(lv==1&&nVilOn>0&&(year&1)) return GM_VILLAGE; }
    int p=(tick>>1)&2047; if(!storyOn&&(year%5==4)) return GM_FREEPLAY;
    return (p<256||p>1792)?GM_NIGHT:p<640?GM_MORNING:p<1408?GM_NOON:GM_SUNSET;
}
static void musUpdate(void){
    gmFrame();
    if(fxCool>0) fxCool--; if(fxQ>=0){ if(fxQ!=FX_PAINT||fxCool==0){ fxPlay(fxQ); if(fxQ==FX_PAINT) fxCool=10; } fxQ=-1; }
    if(musHold>0) musHold--;
    if(musQ>=0){ gmPlay(musQ); musHold=1800; musQ=-1; return; }
    if(jukeOn) return;
    if((tick&31)==0){ int want=musDirector(); if(gmCur<0) gmPlay(want); else if(want!=gmCur&&musHold<=0&&!isJingle(gmCur)){ gmPlay(want); musHold=1800; } }
}
static u16*gfxF[SHAPE_N][2];
static void makeSprites(void){
    for(int b=0;b<16;b++){ SPRITE_PALETTE[b*16+0]=0; for(int k=0;k<3;k++) SPRITE_PALETTE[b*16+1+k]=RGB15(famCol[b][k][0],famCol[b][k][1],famCol[b][k][2]); SPRITE_PALETTE[b*16+4]=RGB15(31,31,31); }
    for(int s=0;s<SHAPE_N;s++) for(int f=0;f<2;f++){ gfxF[s][f]=oamAllocateGfx(&oamMain,SpriteSize_8x8,SpriteColorFormat_16Color); u8*g=(u8*)gfxF[s][f];
        for(int y=0;y<8;y++) for(int x=0;x<8;x+=2){ int v[2]; for(int q=0;q<2;q++){ char ch=shp[s][f][y][x+q]; v[q]=ch=='b'?1:ch=='d'?2:ch=='a'?3:ch=='e'?4:0; } g[y*4+x/2]=(u8)(v[0]|(v[1]<<4)); } }
}
static void drawSprites(void){
    int n=0; int vx0=camX-8, vy0=camY-8;
    for(int i=0;i<nCr&&n<120;i++){ Cr*c=&cr[i]; if(!c->alive) continue; int px=c->x*8+c->ox-camX, py=c->y*8+c->oy-camY; if(px<-8||px>=256||py<-8||py>=192) continue; (void)vx0; (void)vy0;
        const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a]; int fr=((tick>>3)+i)&1; if(!c->ox&&!c->oy) fr=0;
        oamSet(&oamMain,n,px,py,0,s->fam,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[A->shape][fr],-1,false,false,(i&1)&&c->dir,false,false); n++; }
    for(int k=n;k<128;k++) oamSetHidden(&oamMain,k,true);
}
static u8 mmCol[T_N];
static void drawMinimap(void){
    u8*bm=(u8*)bgGetGfxPtr(3);
    for(int y=0;y<128;y++) for(int x=0;x<128;x++){ bm[(y+8)*256+x+64]=(u8)(64+world[y*WH/128][x*WW/128]); }   // (the whole planet, squeezed to 128 x 128 on the big sizes)
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(c->alive){ int da=arch[sp[c->sp].a].diet; bm[(c->y*128/WH+8)*256+c->x*128/WW+64]=(u8)(da==4?80:da==5?85:da==1?81:82); } }
    for(int v=0;v<VIL_N;v++) if(vil[v].on){ for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=vil[v].x*128/WW+dx,y=vil[v].y*128/WH+dy; if(x>=0&&y>=0&&x<128&&y<128) bm[(y+8)*256+x+64]=(u8)(vil[v].foe?81:84); } }
    int cx0=(camX>>3)*128/WW, cy0=(camY>>3)*128/WH, cw=32*128/WW, ch=24*128/WH; for(int x=cx0;x<cx0+cw&&x<128;x++){ bm[(cy0+8)*256+x+64]=83; if(cy0+ch<128) bm[(cy0+ch+8)*256+x+64]=83; } for(int y=cy0;y<cy0+ch&&y<128;y++){ bm[(y+8)*256+cx0+64]=83; if(cx0+cw<128) bm[(y+8)*256+cx0+cw+64]=83; }
}
static int page=0, bestSel=0, juke=0, titleOn=1, sdOk=0, savedYear=-1, hasSave=0;
static const char*savePath="godguy.sav";
static void doSave(void){ if(!sdOk) { siprintf(info,"NO SD CARD: NOT SAVED"); return; } if(saveGame(savePath)){ savedYear=year; siprintf(info,"SAVED"); fxReq(FX_SAVE); } else siprintf(info,"SAVE FAILED"); }
static void setSize(int sz){ planetSize=sz; }
static void startWorld(int story){ storyOn=story; newWorld(); camX=(WW*8-256)/2; camY=(WH*8-192)/2; winX=-99; refillWindow(); titleOn=0; paused=0; musReq(GM_NEWWORLD); }
static void monName(int m,char*out){ int a=ARCH_A+m/16, fam=m%16; strcpy(out,advNm[fam]); strcat(out," "); strcat(out,arch[a].nm); }
static void hud(void){
    char nm[32]; spName(spSel,nm); int alive=0; for(int i=0;i<nCr;i++) alive+=cr[i].alive;
    consoleClear();
    if(titleOn){
        iprintf("\x1b[2;3HBORE: GODGUY\n\x1b[4;3HA wizard? No. A god. With a stylus.\n");
        iprintf("\x1b[8;4HA  %s\n",hasSave?"CONTINUE YOUR WORLD":"START THE STORY");
        iprintf("\x1b[10;4HX  NEW STORY WORLD\n\x1b[12;4HB  NEW FREE PLAY WORLD\n\x1b[14;4HY  PLANET SIZE  %d x %d\n",planetSize,planetSize);
        iprintf("\x1b[17;3H%d species  %d items  %d tracks\n\x1b[19;3HStory chapter %d of %d  (%d items open)\n",nSp,nIt,GM_SONGS,chapter<CHAP_N?chapter+1:CHAP_N,CHAP_N,({int o; itCount(&o); o;}));
        iprintf("\x1b[22;3H%s\n",sdOk?"SD card found: autosave on":"No SD card: no saving");
        return; }
    if(page==1){   // the story
        iprintf("\x1b[0;0HSTORY  %s\n",storyOn?(storyDone?"(FINISHED)":"MODE"):"(FREE PLAY: PROGRESS PAUSED)");
        if(chapter<CHAP_N){ const Chap*C=&chap[chapter]; int v=chapVal(C->kind); iprintf("\x1b[2;0HCHAPTER %d: %s\n\x1b[3;0H%s\n\x1b[4;0H%d / %d\n",chapter+1,C->nm,C->goal,v>(int)C->target?(int)C->target:v,C->target); }
        else iprintf("\x1b[2;0HTHE STORY IS TOLD. YOU ARE A GOD.\n");
        for(int c=0;c<CHAP_N;c++) iprintf("\x1b[%d;0H%s %2d %s\n",6+c,c<chapter?"[X]":"[ ]",c+1,c<chapter?chap[c].nm:"??????????");
        { int o; int tot=itCount(&o); iprintf("\x1b[21;0HITEMS OPEN %d / %d   MONSTERS MET %d/320\n\x1b[22;0HSLAIN %u  GIFTS %u\n",o,tot,seenN,(unsigned)stKill,(unsigned)stGift); }
        iprintf("\x1b[23;0HSELECT PAGE\n"); return; }
    if(page==2){   // the bestiary
        iprintf("\x1b[0;0HBESTIARY  %d / 320 MET\n",seenN);
        for(int r=0;r<18;r++){ int m=bestSel-5+r; if(m<0||m>=320) continue; int s_=8+(ARCH_A+m/16-8)*16+m%16; int seen=(seenB[s_>>3]>>(s_&7))&1, sl=(slainB[s_>>3]>>(s_&7))&1; char n2[32]; if(seen) monName(m,n2); else strcpy(n2,"??????????");
            iprintf("\x1b[%d;0H%s%3d %s%s\n",2+r,m==bestSel?">":" ",m+1,n2,sl?" *":""); }
        iprintf("\x1b[21;0HUP DOWN BROWSE  LEFT RIGHT +10\n\x1b[22;0H* = SLAIN   SELECT PAGE\n"); return; }
    if(page==3){   // the jukebox
        iprintf("\x1b[0;0HJUKEBOX  %d TRACKS\n",GM_SONGS);
        for(int r=0;r<10;r++){ int m=juke-3+r; if(m<0||m>=GM_SONGS) continue; iprintf("\x1b[%d;0H%s%2d %.26s\n",2+r,m==juke?">":" ",m+1,gmSongs[m].name); }
        iprintf("\x1b[13;0H%.30s\n",gmSongs[juke].use);
        if(gmCur>=0){ int sec=gmRow*15/gmSongs[gmCur].bpm, tot=gmSongs[gmCur].rows*15/gmSongs[gmCur].bpm; iprintf("\x1b[15;0HNOW %.22s\n\x1b[16;0H%d:%02d / %d:%02d  %d BPM\n",gmSongs[gmCur].name,sec/60,sec%60,tot/60,tot%60,gmSongs[gmCur].bpm); }
        iprintf("\x1b[19;0HLEFT RIGHT TRACK  UP DOWN +10\n\x1b[20;0HA PLAY / STOP   B BACK TO AUTO MUSIC\n\x1b[21;0HL R  VOLUME %d%%   %s\n\x1b[23;0HSELECT PAGE\n",gmMaster,jukeOn?"JUKEBOX ON":"AUTO"); return; }
    iprintf("\x1b[0;0HBORE: GODGUY v0.5 %s%s %s\n",view3d?(shape3==0?"SPHERE":shape3==1?"CUBE":"RING"):"",paused?" PAUSED":"",storyOn?"STORY":"FREE");
    if(storyOn&&chapter<CHAP_N){ const Chap*C=&chap[chapter]; int v=chapVal(C->kind); iprintf("\x1b[17;0HCH%d %s %d/%d\n",chapter+1,C->goal,v>(int)C->target?(int)C->target:v,C->target); } else iprintf("\x1b[17;0HMONSTERS %d  SLAIN %u  MET %d\n",monCount(),(unsigned)stKill,seenN);
    iprintf("\x1b[19;0HPOWER < %s > BRUSH %d\n",pw[power].nm,brush); if(pw[power].kind==PK_GIFT){ char ib[40]; itSay(ib,itSel); int op; int tot=itCount(&op); iprintf("\x1b[20;0HITEM %s (%d/%d OPEN)\n",ib,op,tot); } else iprintf("\x1b[20;0HSPECIES %s (%d/%d)\n",nm,spSel+1,nSp);
    { int hh; int pp=vilTotals(&hh); iprintf("\x1b[21;0HLIFE %d YEAR %d VIL %d FOLK %d HOMES %d\n",alive,year,nVilOn,pp,hh); } iprintf("\x1b[22;0H%.32s\n",info);
    iprintf("\x1b[23;0H%.32s\n",gmCur>=0?gmSongs[gmCur].name:"SELECT PAGE  X+Y 3D");
}

int main(void){
    videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32);
    vramSetBankA(VRAM_A_MAIN_BG); vramSetBankB(VRAM_B_MAIN_SPRITE); vramSetBankC(VRAM_C_SUB_BG);
    lcdMainOnBottom();
    videoSetModeSub(MODE_5_2D|DISPLAY_BG0_ACTIVE|DISPLAY_BG3_ACTIVE);
    int bg=bgInit(0,BgType_Text8bpp,BgSize_T_512x512,0,1); (void)bg; mapPtr=(u16*)bgGetMapPtr(0);
    oamInit(&oamMain,SpriteMapping_1D_32,false);
    int sb=bgInitSub(3,BgType_Bmp8,BgSize_B8_256x256,0,0); (void)sb;
    consoleInit(NULL,0,BgType_Text4bpp,BgSize_T_256x256,40,4,false,true);
    for(int t=0;t<T_N;t++){ int f=t; BG_PALETTE_SUB[64+t]=RGB15(terrCol[t][0],terrCol[t][1],terrCol[t][2]); (void)f; }
    BG_PALETTE_SUB[80]=RGB15(31,31,6); BG_PALETTE_SUB[81]=RGB15(31,3,3); BG_PALETTE_SUB[82]=RGB15(31,31,31); BG_PALETTE_SUB[83]=RGB15(31,31,0); BG_PALETTE_SUB[84]=RGB15(31,24,6); BG_PALETTE_SUB[85]=RGB15(31,0,31);
    memset(bgGetGfxPtr(3),0,256*192);
    rs^=(u32)(REG_VCOUNT*2654435761u)^0xA5A5; for(int i=0;i<8;i++) rnd();
    sdOk=fatInitDefault();
    soundEnable(); DC_FlushRange((void*)gmWave0,64); gmTables();
    // the two effect voices (a PSG pulse and the noise channel, silent until an effect plays) are reserved first, so the wave voices and drum slots can only fill the other 14 channels
    gpId=soundPlayPSG(DutyCycle_50,440,0,64); gnId=soundPlayNoise(8000,0,64);
    makeSpecies(); makeItems(); makeTiles(); saveBasePal(); makeSprites();
    if(sdOk){ hasSave=loadGame(savePath); }
    if(!hasSave){ planetSize=128; newWorld(); } camX=(WW*8-256)/2; camY=(WH*8-192)/2; paused=1; if(hasSave){ camX=camX>WW*8-256?WW*8-256:camX; }
    musReq(GM_TITLE); musUpdate();
    int held=0, lastTx=-9, lastTy=-9; hud();
    while(1){
        scanKeys(); int kd=keysDown(), kh=keysHeld(); tick++;
        musUpdate();
        if(titleOn){
            int ch=0;
            if(kd&KEY_Y){ setSize(planetSize==128?192:planetSize==192?256:128); newWorld(); paused=1; camX=(WW*8-256)/2; camY=(WH*8-192)/2; ch=1; }
            if(kd&KEY_A){ if(hasSave){ loadGame(savePath); titleOn=0; paused=0; refillWindow(); musReq(GM_NEWWORLD); } else startWorld(1); ch=1; }
            if(kd&KEY_X){ startWorld(1); ch=1; }
            if(kd&KEY_B){ startWorld(0); ch=1; }
            refillWindow(); swiWaitForVBlank(); bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); if((tick&7)==0) applyLight(); if(ch||(tick&31)==0){ drawMinimap(); hud(); }
            continue; }
        int sp_=(kh&KEY_B)?6:3; int dirty=0;
        if(kd&KEY_SELECT){ if(kh&KEY_START){ storyOn=!storyOn; startWorld(storyOn); } else if(kh&KEY_L&&kh&KEY_R){ doSave(); } else { page=(page+1)&3; if(page==3) juke=gmCur>=0?gmCur:0; } dirty=1; }
        if((kd&(KEY_X|KEY_Y))&&(kh&KEY_X)&&(kh&KEY_Y)){ if(view3d) leave3D(); else enter3D(); dirty=1; kd&=~(KEY_X|KEY_Y); }
        if(kd&KEY_START){ if((kh&KEY_L)&&(kh&KEY_R)){ unlockAll=1; siprintf(info,"EVERYTHING UNLOCKED"); } else if(!(kh&KEY_SELECT)){ startWorld(storyOn); } dirty=1; }
        if(page==2){ if(kd&KEY_UP) bestSel=bestSel>0?bestSel-1:0; if(kd&KEY_DOWN) bestSel=bestSel<319?bestSel+1:319; if(kd&KEY_LEFT) bestSel=bestSel>=10?bestSel-10:0; if(kd&KEY_RIGHT) bestSel=bestSel<310?bestSel+10:319; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page==3){ if(kd&KEY_LEFT) juke=(juke+GM_SONGS-1)%GM_SONGS; if(kd&KEY_RIGHT) juke=(juke+1)%GM_SONGS; if(kd&KEY_UP) juke=(juke+GM_SONGS-10)%GM_SONGS; if(kd&KEY_DOWN) juke=(juke+10)%GM_SONGS;
            if(kd&KEY_A){ if(jukeOn&&gmCur==juke){ gmStop(); jukeOn=0; } else { jukeOn=1; gmStop(); gmPlay(juke); } kd&=~KEY_A; }
            if(kd&KEY_B){ jukeOn=0; musHold=0; } if(kd&KEY_R){ gmMaster=gmMaster<100?gmMaster+10:100; } if(kd&KEY_L){ gmMaster=gmMaster>10?gmMaster-10:0; }
            if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_L|KEY_R|KEY_B); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_B); }
        if(kh&KEY_LEFT) camX-=sp_; if(kh&KEY_RIGHT) camX+=sp_; if(kh&KEY_UP) camY-=sp_; if(kh&KEY_DOWN) camY+=sp_;
        if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192;
        if(kd&KEY_R){ power=(power+1)%PW_N; dirty=1; } if(kd&KEY_L){ power=(power+PW_N-1)%PW_N; dirty=1; }
        if(year!=savedYear&&year>0&&(tick&255)==1){ doSave(); dirty=1; }
        if(view3d){
            int ts=(kh&KEY_B)?4:2; if(kh&KEY_LEFT) rotY-=ts; if(kh&KEY_RIGHT) rotY+=ts; if(kh&KEY_UP) rotX-=ts; if(kh&KEY_DOWN) rotX+=ts; if(!(kh&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))) rotY+=1;
            if(rotX>85) rotX=85; if(rotX<-85) rotX=-85;
            if(kh&KEY_X){ zoom3-=1; if(zoom3<19) zoom3=19; } if(kh&KEY_Y){ zoom3+=1; if(zoom3>110) zoom3=110; }
            if(kd&KEY_A){ shape3=(shape3+1)%3; buildMesh(); dirty=1; }
            { static int h3=0, l3x=-9, l3y=-9, lpx=-1, lpy=-1;
              if(kh&KEY_TOUCH){ touchPosition tp; touchRead(&tp);
                  if(kh&KEY_B){ if(lpx>=0){ rotY+=(tp.px-lpx)/2; rotX-=(tp.py-lpy)/2; } lpx=tp.px; lpy=tp.py; h3=0; }   // B held: drag to turn the planet
                  else { lpx=-1; int tx,ty; if(pick3D(tp.px,tp.py,&tx,&ty)){ int fresh=!h3||tx!=l3x||ty!=l3y; if(pw[power].kind==PK_PAINT||fresh) usePower(tx,ty,!h3); h3=1; l3x=tx; l3y=ty; texDirty=1; dirty=1; } } }
              else { h3=0; lpx=-1; } }
            if(rotX>85) rotX=85; if(rotX<-85) rotX=-85;
            if(!paused) simulate(); if((texDirty&&(tick%6)==0)||(tick&63)==0){ uploadTexture(); texDirty=0; } swiWaitForVBlank(); draw3D(); if((tick&15)==0||dirty){ drawMinimap(); hud(); } if((tick&7)==0) applyLight(); continue; }
        if(kd&(KEY_X|KEY_Y)){ int d=(kd&KEY_X)?1:-1; if(kh&KEY_B){ if(pw[power].kind==PK_GIFT) itSel=itNext(itSel,10*d); else spSel=(spSel+nSp+10*d)%nSp; } else if(pw[power].kind==PK_SPAWN){ spSel=(spSel+nSp+d)%nSp; } else if(pw[power].kind==PK_GIFT){ itSel=itNext(itSel,d); } else { brush+=d; if(brush<0) brush=0; if(brush>8) brush=8; } dirty=1; }
        if(kd&KEY_A&&page!=3){ paused^=1; dirty=1; }
        if(kh&KEY_TOUCH){ touchPosition tp; touchRead(&tp); int tx=(tp.px+camX)>>3, ty=(tp.py+camY)>>3; int fresh=!held||tx!=lastTx||ty!=lastTy; if(pw[power].kind==PK_PAINT||fresh) usePower(tx,ty,!held); held=1; lastTx=tx; lastTy=ty; dirty=1; } else held=0;
        refillWindow(); simulate();
        swiWaitForVBlank();
        bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain);
        if((tick&7)==0) applyLight();
        if((tick&15)==0||dirty){ drawMinimap(); hud(); }
    }
    return 0;
}

#endif
