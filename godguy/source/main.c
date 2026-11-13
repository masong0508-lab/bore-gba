// BORE: GODGUY (Nintendo DS) - a god game. v0.1: a 128 x 128 world you shape with the stylus, creatures that live, eat, hunt and breed.
// Bottom screen (touch): the world, 8 x 8 pixel tiles on a hardware background, creatures as sprites. Top screen: the HUD and the whole-planet map.
// Controls: D-pad pans (B = fast), L / R pick a power, X / Y change the brush size (or the species when SPAWN is picked), SELECT jumps ten species, A pauses, START makes a new world.
// The catalogue (species, powers) is data: tables below, generated names, so it grows by adding rows.
#if !defined(PCTEST)&&!defined(NDSSHIM)&&!defined(NDSHW)
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
static int zombA=-1; static u8 vilSick[64];     // vilSick: sick folk met near a village this season (for the journal line)
static u8 sick[MAXC];     // plague: 0 healthy, 1 sick (loses health, slow, spreads to neighbours), 2 recovered and immune. (Not saved: a loaded world starts healthy.)
static int camX, camY;             // camera, in world pixels
static int power=1, brush=2, spSel=40, paused=0, tick=0, year=0;
static char info[64];
static u32 worldSeed, seedFix;               // the seed of this planet; seedFix (if not 0) is used for the next one
static u32 stKill, stGift, stUse;                 // monsters slain, gifts given, powers used (the story counts them)
static u8 seenB[(SP_N+7)/8], slainB[(SP_N+7)/8]; static int seenN;   // the bestiary: which kinds of monster have appeared / been slain
#define ACH_N 26
static u8 favVil[48], favKd[8], favItem[600], favPow[40];   // stars on cities, kingdoms, items and powers (the browser, the toolbar)
static void achCheck(void);
static u32 achBits, stWars, stCapt, stAllies, stSnack;   // achievements earned and the counters they need
static int dlgPend=-1;      // a dialogue waiting to be shown (chapter number, 14 = the epilogue)
static int fullDone, auralOpen, debugOn, speedMul=1, chapter, storyOn=1, storyDone, monOn=1, natural, tierOpen=1;
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
static const char* const shp[SHAPE_OLD][2][8]={
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
// shapes 20..41: one frame each (b body, d dark, a accent, e eye); the walking frame flips the bottom three rows
static const char* const shpN[SHAPE_N-SHAPE_OLD][8]={
 /*20 wolf*/ {"b.b.....","bbbb..bb",".bbebbbb","..bbbbbb","..bbbbb.","..b.b.b.","..d.d.d.","........"},
 /*21 boar*/ {"........",".b......","bbbbbbbb","bebbbbbb","bbbbbbbb",".bbbbbb.","dd.d.d.d","........"},
 /*22 sheep*/ {".bbbb...","bbbbbb.b","bbbbbbbb","bbbbbbbb",".bbbbbb.",".d.d.d.d","........","........"},
 /*23 cow*/ {"..b..b..",".bbbbbb.","bbebbbbd","bbbbbbbb","bdbbbdbb","bbbbbbbb",".d.d.d.d","........"},
 /*24 elf*/ {"b.bbb.b.","..beb...","...b....","..aaa...",".daaad..","..aaa...","..d.d...",".dd.dd.."},
 /*25 dwarf*/ {"..ddd...",".ddbdd..","..beb...",".bbbbb..","dbaaabd.","..aaa...","..d.d...",".dd.dd.."},
 /*26 orc*/ {"..bbb...",".bbebb..","..bbb...","bbaaabb.","bbaaabb.","..aaa...","..d.d...",".dd.dd.."},
 /*27 goblin*/ {"b.....b.","bbbbbbb.",".bebeb..","..bbb...","..aaa...","..aaa...","..d.d...","........"},
 /*28 halfling*/ {"........","..bbb...","..beb...","...b....","..aaa...",".daaad..","..d.d...",".dd.dd.."},
 /*29 gnome*/ {"...a....","..aaa...","..beb...","...b....","..ddd...",".daaad..","..d.d...",".dd.dd.."},
 /*30 ogre*/ {".bbbbb..","bbebebb.","bbbbbbb.","bbaaabb.","bbaaabb.","baaaaab.",".dd.dd..",".dd.dd.."},
 /*31 imp*/ {"b.b.....","bbbb....",".beb..b.","..bbbbb.","..bb.bb.","..b..b..","........","........"},
 /*32 werewolf*/ {"b.b.....",".bbb....",".bbebb..","bbbbbbb.","bbbbbbb.",".bb.bb..",".dd.dd..",".dd.dd.."},
 /*33 cyclops*/ {".bbbb...","bbeebb..","bbeebb..",".bbbb...","bbbbbb..","bbbbbb..",".d..d...",".dd.dd.."},
 /*34 demon*/ {"a.a.a...",".bbbb...","bbebb...",".bbbb.a.","bbbbbba.","bbbbb...",".d.d....",".d.d...."},
 /*35 lich*/ {"..bbb...",".bbeb...",".bbbb...","..ddd...",".dddddd.","dddddddd",".dd.dd..","........"},
 /*36 hydra*/ {"b.b.b...","bbbbb...","e.e.e...",".bbb....","..bbbb..",".bbbbbb.",".b.bb.b.","dd.dd.dd"},
 /*37 scorpion*/ {"......bb",".....b..","b...bb..","bbbbbbb.","bebbbbb.","b.b.b.b.","........","........"},
 /*38 troll*/ {".bbbb...","bbebb...","bbbbb...","bbbbbbb.","bbbbbbbb","bb.bb.bb",".dd.dd..",".dd.dd.."},
 /*39 crown*/ {"........","........","a.a.a...","aaaaa...","aaaaa...","........","........","........"},
 /*40 raindrop*/ {".....b..","....b...","....b...","...b....","........","........","........","........"},
 /*41 snowflake*/ {"........","...b....","..bbb...","...b....","........","........","........","........"},
};
static const char* shpRow(int s,int f,int y){   // row y of frame f of shape s (the walking frame of the new shapes flips their lower three rows)
    if(s<SHAPE_OLD) return shp[s][f][y]; static char flip[9]; const char*r=shpN[s-SHAPE_OLD][y]; if(!f||y<5||s>=SHAPE_OLD+19) return r; for(int i=0;i<8;i++) flip[i]=r[7-i]; flip[8]=0; return flip; }
// archetypes: noun, shape, diet (0 herb, 1 carn, 2 omni, 3 water-eater, 4 civ), walk mask, speed (frames per tile), hp, attack, lifespan (days), breed chance, tint of family used
typedef struct { const char*nm; u8 shape, diet; u16 mask; u8 spd, hp, atk, life, breed; } Arch;
#define M_LAND ((1<<T_GRASS)|(1<<T_FOREST)|(1<<T_HILL)|(1<<T_SAND)|(1<<T_DESERT)|(1<<T_SWAMP)|(1<<T_JUNGLE)|(1<<T_SNOW)|(1<<T_DIRT))
#define M_WATER ((1<<T_WATER)|(1<<T_DEEP))
#define M_ALL (M_LAND|M_WATER|(1<<T_MOUNT)|(1<<T_LAVA))
#define M_CLIMB (M_LAND|(1<<T_MOUNT))
static const Arch arch[ARCH_N]={
 {"HUMAN",5,4,M_LAND,14,30,5,90,20},{"ELF",24,4,M_LAND,12,26,6,140,12},{"DWARF",25,4,M_CLIMB,16,40,8,120,14},{"ORC",26,4,M_LAND,14,50,10,80,24},
 {"GOBLIN",27,4,M_LAND,10,16,4,50,34},{"HALFLING",28,4,M_LAND,14,18,3,100,16},{"GNOME",29,4,M_LAND,16,16,3,110,14},{"OGRE",30,4,M_LAND,22,90,16,70,8},
 {"SHEEP",22,0,M_LAND,26,14,0,60,28},{"COW",23,0,M_LAND,30,24,0,70,18},{"DEER",9,0,M_LAND,14,16,0,60,24},{"RABBIT",10,0,M_LAND,10,6,0,40,60},
 {"WOLF",20,1,M_LAND,12,22,6,70,18},{"FOX",20,1,M_LAND,10,12,4,50,22},{"BEAR",8,2,M_CLIMB,20,60,12,90,8},{"BOAR",21,2,M_LAND,16,26,6,60,22},
 {"EAGLE",1,1,M_ALL&~(1<<T_LAVA),6,10,5,60,12},{"SPARROW",1,0,M_ALL&~(1<<T_LAVA),6,4,0,30,50},{"BAT",14,2,M_ALL&~(1<<T_LAVA),5,5,2,40,30},{"SNAKE",3,1,M_LAND,18,10,5,50,26},
 {"LIZARD",7,2,M_LAND,14,8,2,50,32},{"SPIDER",12,1,M_CLIMB,12,8,4,40,40},{"BEETLE",4,0,M_LAND,20,5,1,30,60},{"TURTLE",15,2,M_LAND|M_WATER,34,40,1,200,8},
 {"FISH",2,3,M_WATER,8,5,0,40,50},{"SHARK",2,1,M_WATER,8,40,10,60,10},{"WHALE",13,3,M_WATER,18,120,6,150,5},{"BLOB",6,2,M_LAND|M_WATER,24,20,2,60,40},
 {"DRAGON",11,1,M_ALL,10,120,22,200,3},{"DRAKE",11,1,M_ALL&~(1<<T_LAVA),12,70,14,140,5},{"MOUSE",10,0,M_LAND,8,4,0,30,70},{"BADGER",21,2,M_LAND,18,26,7,60,14},
 // ---- the monsters (diet 5): they hunt every living thing, and burn the houses of the peoples. tier = the age of the world they first come in (monTier[])
 {"IMP",31,5,M_ALL&~(1<<T_LAVA),8,14,4,120,0},{"GHOUL",5,5,M_LAND,14,28,6,150,0},{"SKELETON",16,5,M_LAND,14,24,7,200,0},{"OOZE",6,5,M_LAND|M_WATER,24,30,4,200,0},
 {"WEREWOLF",32,5,M_LAND,10,40,11,160,0},{"ZOMBIE",5,5,M_LAND,22,60,7,220,0},{"WRAITH",17,5,M_ALL,10,30,10,180,0},{"GIANT SPIDER",12,5,M_CLIMB,10,36,9,170,0},{"SCORPION",37,5,M_LAND,12,30,10,170,0},
 {"TROLL",38,5,M_CLIMB,18,90,14,230,0},{"GOLEM",18,5,M_CLIMB,28,140,16,250,0},{"WYVERN",11,5,M_ALL,8,70,15,200,0},{"SEA SERPENT",2,5,M_WATER,8,80,14,200,0},
 {"KRAKEN",19,5,M_WATER,14,160,18,230,0},{"CYCLOPS",33,5,M_LAND,20,150,20,240,0},{"BASILISK",7,5,M_LAND,12,90,18,220,0},
 {"HYDRA",36,5,M_LAND|M_WATER,16,200,20,250,0},{"DEMON",34,5,M_ALL,10,140,24,250,0},{"LICH",35,5,M_LAND,12,120,26,250,0},{"ELDER DRAGON",11,5,M_ALL,8,250,30,250,0},
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

static const s8 dxs[4]={1,0,-1,0}, dys[4]={0,1,0,-1};
// ---------------------------------------------------------------------------------------------- subspecies: heritable traits (my own design; the WorldBox wiki page could not be read from here)
enum { TR_NONE, TR_VENOM, TR_HIDE, TR_SWIFT, TR_LEAN, TR_FERTILE, TR_LONG, TR_GIANT, TR_TINY, TR_FIRE, TR_AMPH, TR_CLIMB, TR_WING, TR_PACK, TR_OMNI, TR_REGEN, TR_BRAVE, TR_CLEVER, TR_FIERCE, TR_CAMO, TR_NOCT, TR_SPIT, TR_SYMB, TR_FROST, TR_CHILL, TR_MUNCH, TR_GIGGLY, TR_ZEN, TR_FLOATY, TR_COUCH, TR_LUCKY, TR_SPACY, TR_N };
static const char* const heroN[12]={"GROK","MAWRA","SKRIT","VOLD","BRASK","YENNA","ZUUL","HORK","NYX","DRAVEN","MOG","TASH"};
static const char* const trNm[TR_N]={"","VENOMOUS","THICK-HIDED","SWIFT","LEAN","FERTILE","LONG-LIVED","GIANT","TINY","FIRE-BORN","AMPHIBIOUS","CLIMBING","WINGED","PACK","OMNIVORE","REGENERATING","BRAVE","CLEVER","FIERCE","CAMOUFLAGED","NOCTURNAL","SPITTING","SYMBIOTIC","FROST-BORN","CHILLED","MUNCHY","GIGGLY","ENLIGHTENED","FLOATY","COUCH-LOCKED","LUCKY","SPACED OUT"};
static const char* const trDesc[TR_N]={"","BITES HARDER: +4 DAMAGE","TAKES 3 LESS DAMAGE A HIT","MOVES 30% FASTER","HALF THE HUNGER","BREEDS HALF AGAIN AS OFTEN","LIVES HALF AGAIN AS LONG","60% MORE HP, +2 DAMAGE, SLOW","40% LESS HP, BREEDS MORE","WALKS ON LAVA","WALKS ON WATER AND LAND","WALKS UP MOUNTAINS","FLIES OVER ANYTHING BUT LAVA","+2 DAMAGE PER PACKMATE NEXT TO IT","EATS PLANTS AND PREY","HEALS A LITTLE EVERY TURN","+3 DAMAGE TO MONSTERS","BETTER GEAR, MORE BREEDING","HUNTS EVEN WHEN NOT HUNGRY","HALF OF THE HUNTERS MISS IT","FAST AT NIGHT, SLOW BY DAY","STRIKES FROM TWO TILES AWAY","HEALS THE ONES NEXT TO IT","WALKS SNOW AND MOUNTAINS","NOTHING MATTERS. HALF HUNGER, SLOW","HAS THE MUNCHIES: EATS ANYTHING, BREEDS MORE","LAUGHS IT OFF: HUNTERS MISS 1 IN 3","INNER PEACE: -2 DAMAGE, HEALS","FLOATS ACROSS WATER, LOST IN THOUGHT","50% MORE HP, VERY SLOW","1 HIT IN 8 LANDS FOR +6","STARES AT MOUNTAINS, THEN WALKS UP THEM"};
static u32 trSeen; static u32 stMut; static int trCount[TR_N], diff=1;      // traits met in this save, mutations so far, living bearers, difficulty 0 calm 1 normal 2 brutal
#define HAS(c,t) ((c)->tr==(t)||(c)->tr2==(t))
static int trMask(const Cr*c){ int m=0; if(HAS(c,TR_FIRE)) m|=1<<T_LAVA; if(HAS(c,TR_AMPH)||HAS(c,TR_FLOATY)) m|=M_WATER; if(HAS(c,TR_CLIMB)||HAS(c,TR_SPACY)) m|=1<<T_MOUNT; if(HAS(c,TR_WING)) m|=M_ALL&~(1<<T_LAVA); if(HAS(c,TR_FROST)) m|=(1<<T_SNOW)|(1<<T_MOUNT); return m; }
static int maxHp(const Cr*c){ int h=arch[sp[c->sp].a].hp; if(HAS(c,TR_GIANT)) h=h*16/10; if(HAS(c,TR_COUCH)) h=h*15/10; if(HAS(c,TR_TINY)) h=h*6/10; if(c->rk==1) h=h*13/10; else if(c->rk==2) h=h*2; if(h<3) h=3; return h>255?255:h; }
static int effSpd(const Cr*c){ int sd=arch[sp[c->sp].a].spd; if(sick[c-cr]==1) sd=sd*13/10; if(HAS(c,TR_SWIFT)) sd=sd*7/10; if(HAS(c,TR_GIANT)) sd=sd*13/10; if(HAS(c,TR_CHILL)) sd=sd*13/10; if(HAS(c,TR_COUCH)) sd=sd*17/10; if(HAS(c,TR_NOCT)){ int p=(tick>>1)&2047; sd=(p<256||p>1792)?sd*7/10:sd*12/10; } return sd<3?3:sd; }
static void crName(const Cr*c,char*out){ out[0]=0; if(c->rk==1) strcpy(out,"ELITE "); else if(c->rk==2) strcpy(out,"BOSS "); if(c->rk){ strcat(out,heroN[(c->sp*5+(int)(c-cr))%12]); strcat(out," "); } if(c->tr){ strcat(out,trNm[c->tr]); strcat(out," "); } if(c->tr2){ strcat(out,trNm[c->tr2]); strcat(out," "); } char b[32]; spName(c->sp,b); strcat(out,b); }
static int hitDmg(const Cr*a,const Cr*d){   // one blow: the attacker's strength, gear and traits against the defender's armour and traits
    const Arch*A=&arch[sp[a->sp].a]; int dm=A->atk+gearAtk(a)+(int)(rnd()&3); if(HAS(a,TR_CLEVER)) dm+=gearAtk(a)/2; if(HAS(a,TR_VENOM)) dm+=4; if(HAS(a,TR_GIANT)) dm+=2;
    if(HAS(a,TR_LUCKY)&&rnd()%8==0) dm+=6; if(HAS(a,TR_BRAVE)&&arch[sp[d->sp].a].diet==5) dm+=3;
    if(HAS(a,TR_PACK)){ int n=0; for(int k=0;k<4;k++){ int x=a->x+dxs[k], y=a->y+dys[k]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]&&cr[occ[y][x]-1].sp==a->sp) n++; } dm+=2*n; }
    if(a->rk==1) dm=dm*13/10; else if(a->rk==2) dm=dm*2; if(A->diet==5) dm=dm*(diff==0?70:diff==2?140:100)/100;
    dm-=armDef(d); if(HAS(d,TR_HIDE)) dm-=3; if(HAS(d,TR_ZEN)) dm-=2; if(HAS(d,TR_GIANT)) dm-=1; return dm<1?1:dm; }
static void countTraits(void){ memset(trCount,0,sizeof trCount); for(int i=0;i<nCr;i++) if(cr[i].alive){ if(cr[i].tr) trCount[cr[i].tr]++; if(cr[i].tr2) trCount[cr[i].tr2]++; } }
static void giveTrait(Cr*c,int t,int announce){ if(c->tr==0||c->tr==t) c->tr=(u8)t; else c->tr2=(u8)t; if(!((trSeen>>t)&1)){ trSeen|=1u<<t; if(announce){ char nm[48]; crName(c,nm); siprintf(info,"NEW SUBSPECIES: %.30s",nm); fxReq(FX_BIRTH); } } }
static void born(Cr*kid,const Cr*par){   // a child has its parent's traits, and now and then a mutation
    kid->tr=par->tr; kid->tr2=par->tr2; int rate=diff==0?1:diff==2?3:2; if(HAS(par,TR_FERTILE)) rate+=1;
    if((int)(rnd()%100)<rate){ int t=1+(int)(rnd()%(TR_N-1)); if(!HAS(kid,t)){ giveTrait(kid,t,1); stMut++; } }
    kid->hp=(u8)maxHp(kid); }
// ---------------------------------------------------------------------------------------------- the world
static u32 hsh(int x,int y,int s){ u32 h=(u32)x*374761393u+(u32)y*668265263u+(u32)s*2246822519u; h=(h^(h>>13))*1274126177u; return h^(h>>16); }
static int vnoise(int x,int y,int cell,int s){   // value noise 0..255, cell pixels per lattice step
    int gx=x/cell, gy=y/cell, fx=x%cell, fy=y%cell;
    int a=hsh(gx,gy,s)&255, b=hsh(gx+1,gy,s)&255, c=hsh(gx,gy+1,s)&255, d=hsh(gx+1,gy+1,s)&255;
    int t=a+(b-a)*fx/cell, u=c+(d-c)*fx/cell; return t+(u-t)*fy/cell;
}
static int fbm(int x,int y,int s){ return (vnoise(x,y,32,s)*8+vnoise(x,y,16,s+1)*4+vnoise(x,y,8,s+2)*2+vnoise(x,y,4,s+3))/15; }
static int cShape, cSea, cClim, cWet, cLife=2;   // create-a-planet choices: land shape (0 continents, 1 one supercontinent, 2 islands), sea level, climate, wetness (all 0 = the classic planet), amount of starting life 0..3
static void genWorld(u32 seed){
    int s=(int)(seed&0xFFFF);
    for(int y=0;y<WH;y++) for(int x=0;x<WW;x++){
        int h=fbm(x,y,s), m=fbm(x+300,y+300,s+9), lat=(y<WH/2?y:WH-1-y)*128/WH, temp=40+lat*3+(fbm(x,y,s+20)-128)/3+cClim*28; m+=cWet*30;
        int dx=(x-WW/2)*128/WW, dy=(y-WH/2)*128/WH; if(cShape==1){ h-=(dx*dx+dy*dy)/30; h+=24; } else if(cShape==2){ h-=(dx*dx+dy*dy)/260; h-=6; if(((x/3+y/3)&1)&&h>110) h-=8; } else { h-=(dx*dx+dy*dy)/90; h+=18; } h+=cSea*7;   // a little pull to ocean at the edges
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
static void vramCopy(void*dst,const void*src,int bytes){ u16*d=(u16*)dst; const u16*s=(const u16*)src; for(int i=0;i<bytes/2;i++) d[i]=s[i]; }   // 16 bit stores only
static void makeTiles(void){
    for(int t=0;t<T_N;t++) for(int k=0;k<8;k++){ int r,g,b; shadeRGB(t*8+k,&r,&g,&b); BG_PALETTE[t*8+k]=RGB15(r,g,b); }
    BG_PALETTE[0]=RGB15(0,0,0);
    for(int r=0;r<8;r++) BG_PALETTE[128+r]=RGB15(roofCol[r][0],roofCol[r][1],roofCol[r][2]);
    for(int k=0;k<8;k++) BG_PALETTE[136+k]=RGB15(bldCol[k][0],bldCol[k][1],bldCol[k][2]);
    // (tiles are built in RAM and copied 16 bits at a time: the DS ignores byte writes to VRAM)
    static u8 tl[64] __attribute__((aligned(4))); u8*bb=(u8*)BG_TILE_RAM(1);
    for(int ty=1;ty<B_N;ty++) for(int r=0;r<8;r++){ for(int y=0;y<8;y++) for(int x=0;x<8;x++) tl[y*8+x]=(u8)bldPix(ty,r,x,y); vramCopy(bb+(BT0+ty*8+r)*64,tl,64); }
    for(int t=0;t<T_N;t++) for(int v=0;v<4;v++){ for(int y=0;y<8;y++) for(int x=0;x<8;x++) tl[y*8+x]=(u8)tilePix(t,v,x,y); vramCopy(bb+(t*4+v)*64,tl,64); }
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
// Transition tiles: where two kinds of ground meet, the "stronger" one bleeds a dithered fringe into the other, and water gets a foam line on the shore.
// They are made on demand (64 bytes each) into spare tile slots and remembered, so a window of the map needs only a few dozen of them.
#define TR0 128
#define TR_SLOTS 500
static const u8 terrRank[T_N]={0,10,3,1,5,6,7,8,9,3,4,5,2};
static u16 trKey[1024], trVal[1024]; static int trUsed;   // a small hash of the transition tiles made so far
static int trTile(int t,int n,int mask,int v,int foam){
    int key=(((t*16+n)*16+mask)*4+v)*2+foam; unsigned hi=(unsigned)(key*2654435761u>>22)&1023; while(trKey[hi]){ if(trKey[hi]==key+1) return trVal[hi]; hi=(hi+1)&1023; }
    if(trUsed>=TR_SLOTS) return -1;
    static u8 tl[64] __attribute__((aligned(4)));
    for(int y=0;y<8;y++) for(int x=0;x<8;x++){ int px=tilePix(t,v,x,y); int d=9;
        if(mask&1) if(y<d) d=y; if(mask&2) if(7-x<d) d=7-x; if(mask&4) if(7-y<d) d=7-y; if(mask&8) if(x<d) d=x;
        int h=(int)(hsh(x+v*8,y+t*3+n*7,21)&7);
        if(foam){ if(d==0&&h<5) px=T_WATER*8+7; else if(d==1&&h<2) px=T_WATER*8+7; }
        else if(d<4&&h<(4-d)*2-(d==0?0:1)+(d==0?1:0)) px=tilePix(n,(x+y+v)&3,x,y);
        tl[y*8+x]=(u8)px; }
    int slot=TR0+trUsed++;
#ifndef PCTEST
    vramCopy((u8*)BG_TILE_RAM(1)+slot*64,tl,64);
#endif
    trKey[hi]=(u16)(key+1); trVal[hi]=(u16)slot; return slot; }
static inline void putCell(int x,int y){   // world cell -> background entry (a 64 x 64 torus of 8 pixel tiles)
    int mx=x&63, my=y&63; int blk=(my>=32?2:0)+(mx>=32?1:0); int t=world[y][x], v=(int)(hsh(x,y,3)&3); u16 e=(u16)(t*4+v);
    if(bld[y][x]) e=(u16)(BT0+bld[y][x]*8+(bldv[y][x]?vilRace(bldv[y][x]-1):0));
    else { int nb[4]; nb[0]=y>0?world[y-1][x]:t; nb[1]=x<WW-1?world[y][x+1]:t; nb[2]=y<WH-1?world[y+1][x]:t; nb[3]=x>0?world[y][x-1]:t;
        int best=-1, br=-1, foam=0, wat=t<=T_WATER;
        for(int i=0;i<4;i++){ int n=nb[i]; if(n==t) continue; if(wat&&n>T_WATER){ foam=1; best=n; continue; } if(!wat&&terrRank[n]>terrRank[t]&&terrRank[n]>br){ br=terrRank[n]; best=n; } }
        if(best>=0){ int mask=0; for(int i=0;i<4;i++) if(foam?(nb[i]>T_WATER):(nb[i]==best)) mask|=1<<i; int k=trTile(t,best,mask,v,foam); if(k>=0) e=(u16)k; } }
    mapPtr[blk*1024+(my&31)*32+(mx&31)]=e; }
static void refillWindow(void){ int x0=camX>>3, y0=camY>>3; if(x0==winX&&y0==winY) return; winX=x0; winY=y0;
    for(int y=y0-1;y<y0+26&&y<WH;y++) for(int x=x0-1;x<x0+34&&x<WW;x++) if(x>=0&&y>=0) putCell(x,y); }
static void setCell(int x,int y,u8 t){ if(x<0||y<0||x>=WW||y>=WH) return; world[y][x]=t; for(int k=0;k<5;k++){ int xx=x+(k==1)-(k==2), yy=y+(k==3)-(k==4); if(xx<0||yy<0||xx>=WW||yy>=WH) continue; if(xx>=winX-1&&xx<winX+34&&yy>=winY-1&&yy<winY+26) putCell(xx,yy); } }

// ---------------------------------------------------------------------------------------------- creatures
static int curExtra;      // the extra ground the creature being moved can cross (its traits)
static int walkable(const Sp*s,int x,int y){ if(x<0||y<0||x>=WW||y>=WH) return 0; return ((arch[s->a].mask|curExtra)>>world[y][x])&1; }
static int addCr(int s,int x,int y){
    if(nCr>=MAXC||x<0||y<0||x>=WW||y>=WH||occ[y][x]) return -1; if(!walkable(&sp[s],x,y)) return -1;
    Cr*c=&cr[nCr]; memset(c,0,sizeof *c); c->sp=(u16)s; c->x=(u8)x; c->y=(u8)y; c->hp=(u8)maxHp(c); c->food=70; c->age=(u8)(rnd()&15); c->t=(u8)(rnd()&15); c->alive=1; sick[nCr]=0; occ[y][x]=(u16)(nCr+1);
    if(arch[sp[s].a].diet==5&&!((seenB[s>>3]>>(s&7))&1)){ seenB[s>>3]|=(u8)(1<<(s&7)); seenN++; char nm[32]; spName(s,nm); siprintf(info,"BESTIARY: %s",nm); fxReq(FX_MONSTER_SEEN); }
    return nCr++;
}
static void logEv(const char*s);
static void killCr(int i){ Cr*c=&cr[i]; if(!c->alive) return; if(c->rk==2&&!natural){ char nm[48], b[40]; crName(c,nm); siprintf(b,"%.28s FALLS",nm); logEv(b); } c->alive=0; if(c->rk==1&&!natural&&(rnd()&3)==0){ char nm[48], b2[40]; crName(c,nm); siprintf(b2,"%.24s FALLS",nm); logEv(b2); } if(occ[c->y][c->x]==i+1) occ[c->y][c->x]=0;
    if(!natural&&arch[sp[c->sp].a].diet==5){ stKill++; if(!((slainB[c->sp>>3]>>(c->sp&7))&1)) slainB[c->sp>>3]|=(u8)(1<<(c->sp&7)); fxReq(FX_MONSTER_SLAIN); } }
static void killNat(int i){ natural=1; killCr(i); natural=0; }     // old age, hunger, lava: not a slaying
static void compact(void){   // the dead are squeezed out of the list
    int j=0; for(int i=0;i<nCr;i++) if(cr[i].alive){ if(i!=j){ cr[j]=cr[i]; sick[j]=sick[i]; } j++; } nCr=j;
    memset(occ,0,sizeof occ); for(int i=0;i<nCr;i++) occ[cr[i].y][cr[i].x]=(u16)(i+1);
}

static void moveTo(Cr*c,int i,int nx,int ny){ occ[c->y][c->x]=0; c->ox=(s8)((c->x-nx)*8); c->oy=(s8)((c->y-ny)*8); c->x=(u8)nx; c->y=(u8)ny; occ[ny][nx]=(u16)(i+1); c->dir=(u8)(nx>c->x?0:0); }
// ---------------------------------------------------------------------------------------------- villages
#define VIL_N 40
#define VIL_OLD 24     // older saves held 24 villages
typedef struct { u8 on, race, x, y, lvl, houses, farms, towers, foe, dead, nm1, nm2; u16 pop, stock; } Vil;   // foe: the village (+1) it is at war with
static Vil vil[VIL_N]; static int nVilOn;
static u8 warM[8][8];   // [a][b]: the people of a are fighting the people of b
static u8 vilRace(int v){ return vil[v].race; }
static const char* const vn1[12]={"BRAN","DUN","MOR","TAL","KAR","EL","OR","ISH","VAL","GRO","HAR","NIM"}, * const vn2[8]={"TON","HOLM","BURG","DELL","FORD","MERE","STEAD","HOLD"};
// ---------------------------------------------------------------------------------------------- languages and the name system
// Five tongues, each with its own words, kingdom endings, city endings and person-name syllables. Every name in the game is built from them: a kingdom speaks the language it was founded in
// (kingdom name), a city by the language of its people, a ruler by the kingdom's current language (title word + name), a faith by its language's words for god and for its dogma.
static const char* const cptN[16]={"STONE","RIVER","FIRE","STAR","OAK","WOLF","MOON","SUN","KING","WAR","PEACE","GOLD","HOME","GOD","DEATH","LIFE"};
static const char* const lwords[5][16]={
 {"ETH","ILUV","NAUR","ELEN","TAUR","DHAN","ITHIL","ANOR","ARAN","MACIL","IRMA","MALTA","ONDO","ILU","MORN","CUIV"},
 {"STAN","RIV","FYR","STERR","AK","WULF","MON","SUNN","KYNG","WYR","FRITH","GOLD","HAM","GOD","DETH","LIF"},
 {"KAZ","DUR","BARAZ","ZIL","BELEG","GRIMM","NAR","AZAN","THRAIN","KHAZ","SOL","ORIN","BARUK","MAHAL","MORG","DRAK"},
 {"OLI","UMA","FELA","SELI","ANI","LUPE","LUNA","SOLI","REI","MARTA","PACE","ORO","KASA","DEO","MORI","VITA"},
 {"GRUK","SLOSH","GRAKK","ZUG","MOKK","FANGG","NAKH","GAZ","KHAN","WAAG","PUNGH","SHINY","GROT","UGG","DRAKK","ZAK"} };
static const char* const lsufK[5][4]={{"IEL","ANOR","IA","ORE"},{"LAND","MARK","SHIRE","HOLM"},{"HEIM","GARD","DUM","KAZ"},{"LI","RA","NIA","VELLA"},{"ZAG","GOR","UKK","TAR"}};
static const char* const lsufC[5][4]={{"LOND","DELL","RIL","WEN"},{"TON","FORD","BURG","STEAD"},{"KAR","DUM","GROT","HOLD"},{"ELLA","ONA","INO","ASA"},{"GRUB","RUK","SKAR","MOZ"}};
static const char* const pn1[5][6]={{"EL","AR","LU","TH","CE","IL"},{"AL","BRY","CED","DOR","EM","FAR"},{"BAR","DUR","GIM","THOR","KHAZ","BOR"},{"LU","MA","OR","SE","IL","NA"},{"GRU","ZUG","MOK","SKA","KRA","UR"}};
static const char* const pn2[5][6]={{"AN","IR","AE","OL","IA","EN"},{"RI","N","RA","E","BE","IN"},{"A","I","UN","IN","O","AD"},{"CI","RI","NE","LI","VA","DE"},{"K","ZZ","G","RAK","TH","NG"}};
static const char* const pn3[5][6]={{"DIL","WEN","RIEL","DOR","NA","MIR"},{"C","N","D","RICK","TH","WIN"},{"IN","LI","GRIM","DAR","RUK","MUND"},{"A","ELLA","O","ANA","IO","ETTA"},{"GA","UK","OSH","AZ","ASH","TOK"}};
static int raceLang(int race){ static const u8 m[8]={1,0,2,4,4,3,2,4}; return m[race&7]; }
static void langKing(int l,int a,int b,char*o){ strcpy(o,lwords[l%5][a%8]); strcat(o,lsufK[l%5][b%4]); }
static void langCity(int l,int a,int b,char*o){ strcpy(o,lwords[l%5][a%8]); strcat(o,lsufC[l%5][b%4]); }
static void langPerson(int l,int a,int b,char*o){ strcpy(o,pn1[l%5][a%6]); strcat(o,pn2[l%5][b%6]); strcat(o,pn3[l%5][(a+b)%6]); }
static void vilName(int v,char*out){ langCity(raceLang(vil[v].race),vil[v].nm1,vil[v].nm2,out); }
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
    if(V->foe&&(rnd()&15)==0&&pop<3) V->foe=0;   // a beaten army goes home
}
static void warMatrix(void){ memset(warM,0,sizeof warM); for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].foe&&vil[vil[v].foe-1].on){ int a=vil[v].race, b=vil[vil[v].foe-1].race; warM[a][b]=warM[b][a]=1; } }
static int vilTotals(int*houses){ int pop=0,h=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ pop+=vil[v].pop; h+=vil[v].houses; } if(houses) *houses=h; return pop; }

// ---------------------------------------------------------------------------------------------- kingdoms, wars, vassals and empires
// Villages group into kingdoms (at most 8). A kingdom has a ruler with a temper, gold, technology, relations with every other kingdom (-100 hatred .. +100 love), allies, wars,
// and may be another kingdom's vassal. Wars are declared from bad relations and fought by the villages' soldiers (the old village war marches); cities are captured, not just razed.
#define KN_N 8
typedef struct { u8 on, race, tech, rtr, rn1, rn2, n1, n2, lord, allies, war, empire, weary, reign, trade, cul, cap, plot, plotT, truce, spite, rlg, lang, clan, ctr, tradeW, pcool, nlang; s8 rel[KN_N]; u16 gold, born, cpts; u32 know; } Kd;   // rel[b]: this kingdom's opinion of b; cul: culture; cap: capital village (+1); plot: the kingdom it is plotting war against (+1)
static int kdKnow(int k,int b);
static Kd kd[KN_N]; static u8 vilK[VIL_N]; static s8 vilL[VIL_N]; /* a city's loyalty to its kingdom */ static char jr[24][34]; static u16 jrYear[24]; static int jrN;
static const char* const kn1[12]={"AR","BEL","COR","DRA","EST","FEN","GAL","HOR","ISK","JOR","KEL","LUN"}, * const kn2[8]={"IA","ANIA","OTH","MARK","LAND","VIA","ELL","UM"};
static const char* const rnA[12]={"ALDRIC","BRYN","CEDRA","DORN","EMBER","FARIN","GRETA","HALDOR","ISOLDE","JORAN","KIRA","LOTHAR"}, * const rnB[8]={"THE BOLD","THE WISE","THE FAT","THE MAD","THE KIND","THE LATE","THE GREEDY","THE PALE"};
static const char* const rtrN[4]={"WARLIKE","PEACEFUL","GREEDY","WISE"}; static const char* const culN[5]={"SUN","MOON","IRON","SEA","STONE"}, * const rlgN[4]={"SUNFAITH","MOONFAITH","STONEWAY","OLD GODS"}, * const langN[5]={"HIGH TONGUE","LOWSPEAK","RUNIC","SONG","CLICKS"}, * const clanN[8]={"OAK","RAVEN","WOLF","ROSE","CROWN","STAR","IRON","TIDE"};
static const char* const ctrN[6]={"XENOPHOBIC","SCHOLARLY","MERCANTILE","MARTIAL","AGRARIAN","PIOUS"}, * const ageN[5]={"HOPE","SHADOWS","CHAOS","WONDERS","PEACE"};
static int worldAge(void){ return (year/12+(int)(worldSeed&3))%5; }   /* ages of the world, twelve years each: they tilt growth, monsters, rebellion, learning and peace */
static const char* const techN[10]={"NONE","AGRICULTURE","IRON TOOLS","MASONRY","TRADE ROADS","STEEL","ENGINEERING","ALCHEMY","ASTRONOMY","FEUDALISM"};
static void logEv(const char*s){ if(jrN<24){ strncpy(jr[jrN],s,33); jr[jrN][33]=0; jrYear[jrN]=(u16)year; jrN++; } else { memmove(jr[0],jr[1],23*34); memmove(jrYear,jrYear+1,23*2); strncpy(jr[23],s,33); jr[23][33]=0; jrYear[23]=(u16)year; } siprintf(info,"%.32s",s); }
static void kdName(int k,char*o){ langKing(kd[k].nlang,kd[k].n1,kd[k].n2,o); }
static void kdRuler(int k,char*o){ strcpy(o,lwords[kd[k].lang%5][8]); strcat(o," "); char p[24]; langPerson(kd[k].lang,kd[k].rn1,kd[k].rn2,p); strcat(o,p); strcat(o," "); strcat(o,rnB[kd[k].rn2%8]); }
static void kdFaith(int k,char*o){ static const u8 dn[4]={7,6,0,14}; strcpy(o,lwords[kd[k].lang%5][13]); strcat(o,"-"); strcat(o,lwords[kd[k].lang%5][dn[kd[k].rlg&3]]); }   /* the faith's local name: the tongue's word for god and for its dogma */
static void kdStats0(int k,int*cities,int*pop,int*power){ int c=0,p=0,w=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1){ c++; p+=vil[v].pop; w+=vil[v].pop+vil[v].towers*3+vil[v].houses; } w+=kd[k].tech*4; if(kd[k].ctr>>3&1) w=w*5/4; w=w*(100+10*kdKnow(k,2))/100; if(kd[k].rlg==0) w=w*11/10; if(cities) *cities=c; if(pop) *pop=p; if(power) *power=w; }
static int kdDist0(int a,int b){ int best=9999; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==a+1) for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==b+1){ int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y; int d=dx*dx+dy*dy; if(d<best) best=d; } int r=0; while(r*r<best&&r<200) r++; return r; }
// kingdomTick asks the same questions over and over inside nested kingdom loops (each answer walks every village): while it runs, the answers are remembered
static u8 kcOn, kcSv[KN_N], kcDv[KN_N][KN_N], kcZv[KN_N]; static int kcS[KN_N][3], kcD[KN_N][KN_N], kcZ[KN_N];
static void kdStats(int k,int*cities,int*pop,int*power){ if(!kcOn){ kdStats0(k,cities,pop,power); return; } if(!kcSv[k]){ kdStats0(k,&kcS[k][0],&kcS[k][1],&kcS[k][2]); kcSv[k]=1; } if(cities) *cities=kcS[k][0]; if(pop) *pop=kcS[k][1]; if(power) *power=kcS[k][2]; }
static int kdDist(int a,int b){ if(!kcOn) return kdDist0(a,b); if(!kcDv[a][b]){ kcD[a][b]=kcD[b][a]=kdDist0(a,b); kcDv[a][b]=kcDv[b][a]=1; } return kcD[a][b]; }
static int kdFound(int v){
    int k=-1; for(int i=0;i<KN_N;i++) if(!kd[i].on){ k=i; break; } if(k<0) return -1;
    Kd*K=&kd[k]; memset(K,0,sizeof *K); K->on=1; K->race=vil[v].race; K->n1=(u8)(rnd()%12); K->n2=(u8)(rnd()%8); K->rn1=(u8)(rnd()%12); K->rn2=(u8)(rnd()%8); K->rtr=(u8)(aggressive(K->race)?0:rnd()%4); K->born=(u16)year; K->gold=30; K->cul=(u8)(rnd()%5); K->rlg=(u8)(rnd()%4); K->lang=(u8)(rnd()%5==0?rnd()%5:raceLang(K->race)); K->nlang=K->lang; K->clan=(u8)(rnd()%8); K->cap=(u8)(v+1);
    vilK[v]=(u8)(k+1); vilL[v]=60; char n[24], b[72]; kdName(k,n); siprintf(b,"THE KINGDOM OF %s RISES",n); logEv(b); return k; }
static void kdAssign(void){
    for(int v=0;v<VIL_N;v++){ if(!vil[v].on){ vilK[v]=0; continue; } if(vilK[v]&&kd[vilK[v]-1].on) continue; vilK[v]=0;
        int best=-1, bd=40*40; for(int u=0;u<VIL_N;u++) if(u!=v&&vil[u].on&&vilK[u]&&vil[u].race==vil[v].race){ int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=u; } }
        if(best>=0) vilK[v]=vilK[best]; else if(kdFound(v)<0){ int nb=vilAny(vil[v].x,vil[v].y,60); (void)nb; for(int u=0;u<VIL_N;u++) if(u!=v&&vilK[u]){ vilK[v]=vilK[u]; break; } } } }
static void kdWar(int a,int b,int on){ if(on){ kd[a].war|=(u8)(1<<b); kd[b].war|=(u8)(1<<a); kd[a].weary=kd[b].weary=0; } else { kd[a].war&=(u8)~(1<<b); kd[b].war&=(u8)~(1<<a); } }
static const char* const knBr[6]={"GOVERNANCE","ZONE CONTROL","WARFARE","ECONOMY","ARCHITECTURE","EDUCATION"};
static const char* const knN[6][3]={{"TRIBAL LAW","KINGS COURT","ROYAL CHARTER"},{"BOUNDARY STONES","BORDER FORTS","GRAND SURVEY"},{"DRILLED LEVIES","STEEL ARMS","SIEGECRAFT"},{"BARTER","COINAGE","BANKING"},{"MASONRY","ARCHES","CATHEDRALS"},{"WRITING","SCHOLARS","UNIVERSITIES"}};
static const char* const dogN[4]={"CRUSADERS: +10% POWER","PEACEKEEPERS: BETTER TALKS","BUILDERS: RICHER CITIES","MYSTICS: ODD GIFTS"};
static int kdKnow(int k,int b){ int n=0; for(int i=0;i<3;i++) n+=(kd[k].know>>(b*3+i))&1; return n; }
static int kdMaxCities(int k){ return 3+kdKnow(k,0)+(kd[k].tech>=6); }   // governance: each level allows one more city
static int kdZones0(int k){ int z=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1) z+=3+vil[v].lvl*3+vil[v].houses/3+kdKnow(k,1)*3; return z; }
static int kdZones(int k){ if(!kcOn) return kdZones0(k); if(!kcZv[k]){ kcZ[k]=kdZones0(k); kcZv[k]=1; } return kcZ[k]; }
static void kdDeclareX(int a,int b,int spite){
    if(a==b||!kd[a].on||!kd[b].on||(kd[a].war>>b&1)) return; if(spite){ kd[a].spite=kd[b].spite=1; } char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); kdWar(a,b,1); stWars++; kd[a].allies&=(u8)~(1<<b); kd[b].allies&=(u8)~(1<<a); kd[a].rel[b]=kd[b].rel[a]=-60;
    if(kd[a].rlg!=kd[b].rlg&&((kd[a].ctr>>5&1)||kd[a].rlg==0)) siprintf(s,"%s DECLARES HOLY WAR ON %s",x,y); else siprintf(s,"%s DECLARES WAR ON %s",x,y); logEv(s);
    for(int c=0;c<KN_N;c++) if(c!=a&&c!=b&&kd[c].on&&(kd[a].allies>>c&1)&&!(kd[b].allies>>c&1)&&kd[c].rel[b]<20&&!(kd[c].war>>b&1)){ kdWar(c,b,1); char z[24]; kdName(c,z); siprintf(s,"%s JOINS THE WAR",z); logEv(s); } }
static void kdDeclare(int a,int b){ kdDeclareX(a,b,0); }
static void kdDie(int k){ char n[24], s[72]; kdName(k,n); siprintf(s,"THE KINGDOM OF %s FALLS",n); logEv(s); for(int o=0;o<KN_N;o++){ kd[o].allies&=(u8)~(1<<k); kd[o].war&=(u8)~(1<<k); if(kd[o].lord==k+1) kd[o].lord=0; kd[o].rel[k]=0; } memset(&kd[k],0,sizeof kd[k]); }
static int vilConquer(int fv,int byV){   // a village taken in war changes kingdom instead of dying
    int a=vilK[byV]-1, b=vilK[fv]-1; if(a<0||b<0||a==b||!(kd[a].war>>b&1)) return 0;
    int wasCap=kd[b].cap==fv+1; vilK[fv]=(u8)(a+1); vilL[fv]=-20; stCapt++; vil[fv].foe=0; vil[fv].stock/=2;
    if(wasCap){ kd[b].cap=0; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==b+1){ vilL[u]=(s8)(vilL[u]-25); vil[u].foe=0; } logEv("A CAPITAL FALLS: ARMIES DESERT"); }   /* taking a capital shakes every other city of the kingdom */ char n[24], x[24], s[72]; vilName(fv,n); kdName(a,x); siprintf(s,"%s TAKES %s",x,n); logEv(s); kd[b].weary=(u8)(kd[b].weary>3?kd[b].weary-3:0); return 1; }
static void kingdomTick0(void){
    { static int lastAge=-1; int ag=worldAge(); if(ag!=lastAge){ if(lastAge>=0){ char s2[72]; siprintf(s2,"THE AGE OF %s BEGINS",ageN[ag]); logEv(s2); } lastAge=ag; } }
    kdAssign();
    for(int k=0;k<KN_N;k++) if(kd[k].on){ int c,p,w; kdStats(k,&c,&p,&w); if(!c){ kdDie(k); continue; }
        Kd*K=&kd[k]; { int g=p/6+c*2+(K->trade?(K->ctr>>2&1)?10:4:0); if(worldAge()==0) g=g*5/4; g=g*(100+15*kdKnow(k,3))/100; K->gold=(u16)(K->gold+g>60000?60000:K->gold+g); } K->trade=0; K->tradeW=0;
        { int bon=(K->ctr>>4&1?3:0)+kdKnow(k,4)*2+(K->rlg==2?2:0); if(bon) for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1&&vil[v].stock<390) vil[v].stock=(u16)(vil[v].stock+bon); }
        { /* the culture tree: points from people and scholars buy the next node of the branch the ruler cares about */ static const u8 pref[4][6]={{2,1,0,3,4,5},{0,4,5,3,1,2},{3,1,4,0,2,5},{5,0,3,4,1,2}}; int lv=0; for(int b=0;b<6;b++) lv+=kdKnow(k,b); K->cpts=(u16)(K->cpts+p/8+c+((K->ctr>>1&1)?4:0)+kdKnow(k,5)*2); int cost=(30+lv*20)*(100-10*kdKnow(k,5))/100;
          if(lv<18&&K->cpts>=cost){ for(int q=0;q<6;q++){ int b=pref[K->rtr&3][q]; int l=kdKnow(k,b); if(l<3){ K->know|=1u<<(b*3+l); K->cpts-=(u16)cost; char y[24], s2[72]; kdName(k,y); siprintf(s2,"%s LEARNS %s",y,knN[b][l]); logEv(s2); break; } } } }
        if(K->rlg==3&&nCr&&(rnd()%100)<4){ for(int t=0;t<24;t++){ int i=(int)(rnd()%nCr); Cr*c2=&cr[i]; if(c2->alive&&sp[c2->sp].a==K->race&&c2->tr2==0){ giveTrait(c2,1+(int)(rnd()%(TR_N-1)),0); break; } } }
        if(((K->ctr>>5&1)||K->rlg==0)&&(rnd()%100)<5){ for(int o=0;o<KN_N;o++) if(o!=k&&kd[o].on&&kd[o].rlg!=K->rlg&&kd[o].rel[k]>-30&&kdDist(k,o)<40){ kd[o].rlg=K->rlg; char y[24], f[24], s2[72]; kdName(o,y); kdFaith(k,f); siprintf(s2,"%s ADOPTS FAITH %s",y,f); logEv(s2); break; } }
        if(++K->reign>90+(rnd()%70)){ K->reign=0; K->rn1=(u8)(rnd()%12); K->rn2=(u8)(rnd()%8); if(!aggressive(K->race)) K->rtr=(u8)(rnd()%4); else if(rnd()&1) K->rtr=(u8)(rnd()%4); }
        if(K->reign==0){ int old=K->clan; if(rnd()%10>=7) K->clan=(u8)(rnd()%8); if(K->clan!=old){ char y[24], s2[72]; kdName(k,y); siprintf(s2,"HOUSE %s TAKES %s",clanN[K->clan],y); logEv(s2); } }
        if(__builtin_popcount(K->ctr)<4&&(rnd()%100)<3){ int t=(int)(rnd()%6); if(!(K->ctr>>t&1)){ K->ctr|=(u8)(1<<t); char y[24], s2[72]; kdName(k,y); siprintf(s2,"%s TURNS %s",y,ctrN[t]); logEv(s2); } }
        if(K->lord&&kd[K->lord-1].on&&(rnd()%100)<8){ Kd*L=&kd[K->lord-1]; int w2=(int)(rnd()%3); if(w2==0) K->cul=L->cul; else if(w2==1) K->rlg=L->rlg; else K->lang=L->lang; }   /* a vassal slowly takes on its lord's ways */
        if(K->gold>1500){ K->gold-=600; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1) vil[v].stock=(u16)(vil[v].stock+40>400?400:vil[v].stock+40); }
        if(K->gold>400){ K->gold-=150; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1&&vil[v].stock<350){ vil[v].stock+=50; break; } }
        int tcost=(70+K->tech*60)*((K->ctr>>1&1)?8:10)/10*(worldAge()==3?7:10)/10*(100-10*kdKnow(k,5))/100; if(K->tech<9&&K->gold>=(u16)tcost){ K->gold-=(u16)tcost; K->tech++; char y[24], s[72]; kdName(k,y); siprintf(s,"%s LEARNS %s",y,techN[K->tech]); logEv(s); }
        int emp=c>=5||(0); for(int o=0;o<KN_N;o++) if(kd[o].on&&kd[o].lord==k+1) emp+=0; { int vs=0; for(int o=0;o<KN_N;o++) vs+=kd[o].on&&kd[o].lord==k+1; if(vs>=2) emp=1; }
        if(emp&&!K->empire){ K->empire=1; char y[24], s[72]; kdName(k,y); siprintf(s,"%s BECOMES AN EMPIRE",y); logEv(s); } else if(!emp) K->empire=0;
        if(K->lord){ int l=K->lord-1; if(!kd[l].on) K->lord=0; else { int tr=K->gold/8; K->gold-=(u16)tr; kd[l].gold=(u16)(kd[l].gold+tr>60000?60000:kd[l].gold+tr); int c2,p2,w2; kdStats(l,&c2,&p2,&w2); if(w2*2<w||(rnd()%300)==0){ K->lord=0; char y[24], z[24], s[72]; kdName(k,y); kdName(l,z); kdWar(k,l,1); K->rel[l]=kd[l].rel[k]=-70; siprintf(s,"%s REVOLTS AGAINST %s",y,z); logEv(s); } } } }
    // opinions (each kingdom's view of every other: distance, species, culture, alliance, size, ruler tempers), then plots: war, peace and alliance
    for(int a=0;a<KN_N;a++) if(kd[a].on){ if(kd[a].truce) kd[a].truce--; for(int b=0;b<KN_N;b++) if(b!=a&&kd[b].on){
            int d=kdDist(a,b), op=0; if(d<16) op-=15; else if(d>45) op+=10;
            op+=kd[a].race==kd[b].race?15:-10; op+=kd[a].cul==kd[b].cul?15:-15; op+=kd[a].rlg==kd[b].rlg?15:-15; op+=kd[a].lang==kd[b].lang?8:-8; op+=kd[a].clan==kd[b].clan?10:-12; if((kd[a].ctr&1)&&(kd[a].cul!=kd[b].cul||kd[a].race!=kd[b].race)) op-=12; if(worldAge()==4) op+=10; if(kd[a].rlg==1) op+=5; if(kd[a].allies>>b&1) op+=30; if(kd[a].war>>b&1) op-=500; if(kd[a].truce) op+=20; if(kd[a].lord==b+1||kd[b].lord==a+1) op+=15;
            { int za=kdZones(a), zb=kdZones(b); if(zb>za){ int t=(zb-za)/5; op-=t>20?20:t; } int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb); if(wb>wa){ int t=(wb-wa)/30; op-=t>15?15:t; } }
            if(kd[a].rtr==0&&kd[b].rtr!=0) op-=10; else if(kd[a].rtr==1&&kd[b].rtr==0) op-=10; else if(kd[a].rtr==kd[b].rtr) op+=5;
            op+=worldAge()==2?(int)(rnd()%17)-8:(int)(rnd()%5)-2; if(op>100) op=100; if(op<-100) op=-100; kd[a].rel[b]=(s8)op; } }
    for(int a=0;a<KN_N;a++) if(kd[a].on){
        if(kd[a].plot){ int b=kd[a].plot-1; if(!kd[b].on||(kd[a].war>>b&1)||(kd[a].allies>>b&1)||kd[a].rel[b]>-10){ kd[a].plot=0; kd[a].pcool=8; } else if(++kd[a].plotT>=3){ int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb); kd[a].plot=0; kd[a].pcool=8; if(wa*10>=wb*6) kdDeclare(a,b); } }
        else if(kd[a].pcool) kd[a].pcool--;
        else for(int b=0;b<KN_N;b++) if(b!=a&&kd[b].on&&!(kd[a].war>>b&1)&&!(kd[a].allies>>b&1)&&!kd[a].truce&&kd[a].lord!=b+1&&kd[b].lord!=a+1){ int d=kdDist(a,b); int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb); if(kd[a].rel[b]<-25&&d<70&&wa*10>=wb*6&&(int)(rnd()%100)<(kd[a].rtr==0?35:10)){ kd[a].plot=(u8)(b+1); kd[a].plotT=0; char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); siprintf(s,"%s PLOTS WAR AGAINST %s",x,y); logEv(s); break; } }
        for(int b=a+1;b<KN_N;b++) if(kd[b].on){
            int war=kd[a].war>>b&1, ally=kd[a].allies>>b&1; int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb);
            if(war){ kd[a].weary++; kd[b].weary++; int ww=kd[a].weary; int spite=kd[a].spite||kd[b].spite;
                int stop=ww>40||wa*5<wb||wb*5<wa||(!spite&&ww>=8&&(int)(rnd()%100)<ww*3+((kd[a].rlg==1||kd[b].rlg==1)?10:0)&&(kd[a].rtr==1||kd[b].rtr==1||wa*2<wb||wb*2<wa||ww>20));   // peace talks
                if(stop){ kdWar(a,b,0); kd[a].spite=kd[b].spite=0; kd[a].truce=kd[b].truce=10; char x[24], y[24], s[72]; kdName(a,x); kdName(b,y);
                    int win=wa*2<=wb?b:wb*2<=wa?a:-1; if(win>=0){ int lo=win==a?b:a; if(!kd[lo].lord&&!kd[win].lord){ kd[lo].lord=(u8)(win+1); char t[24], u[24]; kdName(lo,t); kdName(win,u); siprintf(s,"%s BECOMES A VASSAL OF %s",t,u); logEv(s); } else { siprintf(s,"PEACE BETWEEN %s AND %s",x,y); logEv(s); } } else { siprintf(s,"PEACE BETWEEN %s AND %s",x,y); logEv(s); } } }
            else if(!ally&&!(kd[a].lord==b+1||kd[b].lord==a+1)){
                if(kd[a].rel[b]>35&&kd[b].rel[a]>35&&(rnd()&3)==0){ int na=__builtin_popcount(kd[a].allies), nb=__builtin_popcount(kd[b].allies); if(na<3&&nb<3){ kd[a].allies|=(u8)(1<<b); kd[b].allies|=(u8)(1<<a); stAllies++; char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); siprintf(s,"%s AND %s ALLY",x,y); logEv(s); } }
                else if(kd[a].rel[b]>10&&kd[b].rel[a]>10&&kdDist(a,b)<90){ kd[a].trade=kd[b].trade=1; kd[a].tradeW|=(u8)(1<<b); kd[b].tradeW|=(u8)(1<<a); } }
            else if(ally&&(kd[a].rel[b]<10||kd[b].rel[a]<10)){ kd[a].allies&=(u8)~(1<<b); kd[b].allies&=(u8)~(1<<a); char x[24], y[24], s[96]; kdName(a,x); kdName(b,y); siprintf(s,"%s AND %s BREAK THEIR ALLIANCE",x,y); logEv(s); } } }
    // soldiers march on the nearest enemy city; rebellious cities of big kingdoms split off
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]){ int k=vilK[v]-1; Vil*V=&vil[v];
        if(V->foe&&(!vil[V->foe-1].on||!(kd[k].war>>(vilK[V->foe-1]-1)&1))) V->foe=0;
        if(kd[k].war&&!V->foe){ int best=-1, bd=80*80; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]&&u!=v&&(kd[k].war>>(vilK[u]-1)&1)){ int dx=vil[u].x-V->x, dy=vil[u].y-V->y, dd=dx*dx+dy*dy; if(dd<bd){ bd=dd; best=u; } } if(best>=0) V->foe=(u8)(best+1); }
        { int c,p,w; kdStats(k,&c,&p,&w); int cap=kd[k].cap?kd[k].cap-1:-1;
          if(cap<0||!vil[cap].on||vilK[cap]!=k+1){ int bp=-1; kd[k].cap=0; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==k+1&&vil[u].pop>=bp){ bp=vil[u].pop; kd[k].cap=(u8)(u+1); } cap=kd[k].cap-1; }
          if(v==cap) vilL[v]=100; else { int tg=50; int over=c-kdMaxCities(k); if(over>0) tg-=over*25; if(cap>=0){ int dx=vil[cap].x-V->x, dy=vil[cap].y-V->y, dd=dx*dx+dy*dy; int r=0; while(r*r<dd&&r<100) r++; tg+=r<20?10:-(r/10); } if(V->stock>40) tg+=5; if(V->stock<8) tg-=10; if(kd[k].ctr>>5&1) tg+=10; if(worldAge()==0) tg+=5; if(worldAge()==1) tg-=5; if(worldAge()==2) tg-=8;
            if(vilL[v]<tg) vilL[v]++; else if(vilL[v]>tg) vilL[v]--; if(vilL[v]<-100) vilL[v]=-100;
            if(vilL[v]<0&&(c>6||c>kdMaxCities(k))&&year-(int)kd[k].born>=15&&V->pop>=10&&(rnd()%10)==0){ int nk=-1; for(int i=0;i<KN_N;i++) if(!kd[i].on){ nk=i; break; } if(nk>=0){ int cul=kd[k].cul; vilK[v]=0; int n2=kdFound(v); if(n2>=0){ kd[n2].cul=(u8)cul; kd[n2].rlg=kd[k].rlg; kd[n2].lang=kd[k].lang; kd[n2].race=vil[v].race; kdWar(n2,k,1); char n[24], x[24], s[72]; vilName(v,n); kdName(k,x); siprintf(s,"%s REBELS AGAINST %s",n,x); logEv(s); } } } } } }
    // sieges: the stronger side of a war can take the nearest enemy city outright; big cities send out colonies
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]&&vil[v].foe&&vil[vil[v].foe-1].on){ int u=vil[v].foe-1, a=vilK[v]-1, b=vilK[u]-1; if(a<0||b<0||!(kd[a].war>>b&1)) continue; int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb);
        int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y; if(dx*dx+dy*dy>45*45) continue; if(wa>wb&&vil[v].pop>=vil[u].pop&&(int)(rnd()%100)<6+(wa-wb)*8/(wb+1)) vilConquer(u,v); }
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]&&vil[v].pop>=12&&vil[v].lvl>=2&&(rnd()%5)==0){ for(int k=0;k<10;k++){ int x=vil[v].x+(int)(rnd()%41)-20, y=vil[v].y+(int)(rnd()%41)-20; if(vilAny(x,y,9)>=0||!buildable(x,y)) continue; int n=vilFound(vil[v].race,x,y); if(n>=0){ vilK[n]=vilK[v]; for(int q=0;q<4;q++) addCr(vil[v].race,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); char nm[24], s[72]; vilName(n,nm); siprintf(s,"A COLONY: %s IS FOUNDED",nm); logEv(s); } break; } }
    warMatrix(); }
typedef struct { const char*nm, *ds; } Ach;
static const Ach ach[ACH_N]={ {"FIRST SPARK","USE A POWER"},{"TOUCH OF GOD","USE POWERS 100 TIMES"},{"CREATOR","USE POWERS 1000 TIMES"},{"FIRST BLOOD","SLAY A MONSTER"},{"HUNTER","SLAY 50 MONSTERS"},{"EXTERMINATOR","SLAY 300 MONSTERS"},
 {"COLLECTOR","MEET 50 MONSTERS"},{"BESTIARY MASTER","MEET ALL 320 MONSTERS"},{"GENEROUS","GIVE 25 GIFTS"},{"A KINGDOM RISES","SEE A KINGDOM FOUNDED"},{"EMPIRE BUILDER","SEE AN EMPIRE FORM"},{"LORD AND VASSAL","SEE A KINGDOM BECOME A VASSAL"},
 {"WARMONGER","SEE 5 WARS DECLARED"},{"PEACEMAKER","SEE AN ALLIANCE FORMED"},{"CITY PLANNER","GROW A LEVEL 3 HALL"},{"METROPOLIS","HAVE 12 CITIES AT ONCE"},{"SCHOLAR","A KINGDOM LEARNS EVERYTHING"},{"CONQUEROR","SEE 3 CITIES CAPTURED"},
 {"MUTANT","MUTATE 20 CREATURES"},{"ALL THE VIBES","MEET ALL EIGHT CHILL TRAITS"},{"SPECIES SPOTTER","MEET 20 SUBSPECIES"},{"ANCIENT","REACH YEAR 60"},{"STORY'S END","FINISH THE STORY"},{"COMPLETIONIST","FINISH THE GAME 100%"},{"SNACK TIME","GIVE 20 SNACKS"},{"THE COUCH AWAITS","GIVE 100 SNACKS"} };
static int achOK(int i){ int c=0,p=0,w=0,king=0,emp=0,vas=0,tech=0,l3=0,nv=0; for(int k=0;k<KN_N;k++) if(kd[k].on){ king++; emp+=kd[k].empire; vas+=kd[k].lord!=0; if(kd[k].tech>=9) tech=1; } for(int v=0;v<VIL_N;v++) if(vil[v].on){ nv++; if(vil[v].lvl>=3) l3=1; } (void)c;(void)p;(void)w;
    switch(i){ case 0: return stUse>=1; case 1: return stUse>=100; case 2: return stUse>=1000; case 3: return stKill>=1; case 4: return stKill>=50; case 5: return stKill>=300; case 6: return seenN>=50; case 7: return seenN>=320; case 8: return stGift>=25;
        case 9: return king>0; case 10: return emp>0; case 11: return vas>0; case 12: return stWars>=5; case 13: return stAllies>=1; case 14: return l3; case 15: return nv>=12; case 16: return tech; case 17: return stCapt>=3; case 18: return stMut>=20;
        case 19: return ((trSeen>>TR_CHILL)&255)==255; case 20: return __builtin_popcount(trSeen)>=20; case 21: return year>=60; case 22: return storyDone; case 23: return fullDone; case 24: return stSnack>=20; default: return stSnack>=100; } }
static void achCheck(void){ for(int i=0;i<ACH_N;i++) if(!((achBits>>i)&1)&&achOK(i)){ achBits|=1u<<i; char b[40]; siprintf(b,"ACHIEVED: %s",ach[i].nm); logEv(b); fxReq(FX_UNLOCK); } }
static int stepToward(Cr*c,int i,const Sp*s,int tx,int ty){   // one step towards a tile along the longer axis, then the other
    int dx=tx-c->x, dy=ty-c->y; int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0; int ax=dx<0?-dx:dx, ay=dy<0?-dy:dy; int first=ax>=ay;
    for(int k=0;k<2;k++){ int mx=((k==0)==first)?sx:0, my=((k==0)==first)?0:sy; if(!mx&&!my) continue; int nx=c->x+mx, ny=c->y+my; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return 1; } }
    return 0; }
static void thinkBody(int i){
    Cr*c=&cr[i]; if(!c->alive) return; const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a];
    if(c->age<255) c->age++; { int dec=(HAS(c,TR_LEAN)||HAS(c,TR_CHILL))?1:HAS(c,TR_MUNCH)?3:2; if(c->food>dec) c->food-=dec; else c->food=0; }
    if(sick[i]==1){          // the plague: it wears the body down, passes to whoever stands next to it, and mostly burns itself out
        int mon=(A->diet==5);      // monsters (zombies) carry it without suffering from it
        if(!mon&&(tick&3)==0&&c->hp>1&&!HAS(c,TR_ZEN)) c->hp--;
        else if(!mon&&c->hp<=1&&rnd()%3==0){    // death by plague: sometimes the body gets up again as a zombie that keeps spreading it
            int x0=c->x, y0=c->y, rise=(A->diet==4?20:8)>(int)(rnd()%100); killNat(i);
            if(rise){ if(zombA<0) for(int a2=8;a2<ARCH_N;a2++) if(!strcmp(arch[a2].nm,"ZOMBIE")) zombA=a2;
                if(zombA>=8){ int k2=addCr(8+(zombA-8)*16+(int)(rnd()&15),x0,y0); if(k2>=0){ sick[k2]=1; cr[k2].hp=(u8)maxHp(&cr[k2]); siprintf(info,"A ZOMBIE RISES"); } } }
            return; }
        for(int dy=-2;dy<=2;dy++) for(int dx=-2;dx<=2;dx++){ int x=c->x+dx, y=c->y+dy; if((dx||dy)&&x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]){ int j=occ[y][x]-1; if(sick[j]==0&&cr[j].alive&&(int)(rnd()%100)<(HAS(&cr[j],TR_REGEN)||HAS(&cr[j],TR_HIDE)?4:(abs(dx)+abs(dy)<=1?22:8))) sick[j]=1; } }
        if(!mon&&rnd()%100<(HAS(c,TR_REGEN)||HAS(c,TR_ZEN)?12:3)) sick[i]=2;
        if(A->diet==4){ int v=vilNear(s->a,c->x,c->y,8); if(v>=0){ if(vil[v].stock>0&&(tick&7)==0) vil[v].stock--;      // sick folk eat and do not work: the store drains
                if(vilSick[v]<255) vilSick[v]++; if(vilSick[v]==12){ char vn[24], b[40]; vilName(v,vn); siprintf(b,"PLAGUE IN %.20s",vn); logEv(b); } } } }
    if(A->diet==5) c->food=100;
    if(c->food==0){ if(c->hp>2) c->hp-=2; else { killNat(i); return; } }
    { int lim=A->life+(HAS(c,TR_LONG)?A->life/2:0); if(lim>254) lim=254; if(c->age>lim){ killNat(i); return; } }
    if((HAS(c,TR_REGEN)||HAS(c,TR_ZEN))&&c->hp<maxHp(c)) c->hp++;
    if(HAS(c,TR_SYMB)&&(tick&1)==0){ for(int k=0;k<4;k++){ int x=c->x+dxs[k], y=c->y+dys[k]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]){ Cr*q=&cr[occ[y][x]-1]; if(q->hp<maxHp(q)) q->hp+=2; } } }
    int t=world[c->y][c->x]; int amask=A->mask|curExtra;
    if(t==T_LAVA&&!((amask>>T_LAVA)&1)){ killNat(i); return; }
    if(!((amask>>t)&1)){ if(c->hp>8) c->hp-=8; else { killNat(i); return; } }
    int diet=A->diet; if((HAS(c,TR_OMNI)||HAS(c,TR_MUNCH))&&(diet==0||diet==1)) diet=2;
    if(diet==5){   // a monster: it hunts every living thing in reach (people first), and burns the houses it passes
        int best=-1,bd=99,rd=99; for(int dy=-10;dy<=10;dy++) for(int dx=-10;dx<=10;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||arch[sp[cr[j].sp].a].diet==5||((HAS(&cr[j],TR_CAMO)&&(rnd()&1))||(HAS(&cr[j],TR_GIGGLY)&&rnd()%3==0))) continue;
            int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); int civ=arch[sp[cr[j].sp].a].diet==4; if(!civ&&d>3) continue; if(d<bd){ bd=d; rd=d; best=j; } }   // people are hunted from ten tiles away, animals only when they are in the way
        for(int k=0;k<4;k++) burnAt(c->x+dxs[k],c->y+dys[k],40);
        if(best>=0){ Cr*p=&cr[best];
            if(rd<=1+HAS(c,TR_SPIT)){ int dm=hitDmg(c,p); if(p->hp>dm) p->hp-=(u8)dm; else { killCr(best); c->hp=(u8)(c->hp+12>A->hp?A->hp:c->hp+12); } return; }
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
          if(best>=0){ Cr*p=&cr[best]; if(bd<=1+HAS(c,TR_SPIT)){ int dm=hitDmg(c,p); if(p->hp>dm) p->hp-=(u8)dm; else { killCr(best); c->food+=20; } return; }
              int dx=p->x-c->x, dy=p->y-c->y; int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]) moveTo(c,i,nx,ny); return; } }
        if(home>=0&&vil[home].foe&&vil[vil[home].foe-1].on&&c->age>6&&c->hp>8){ const Vil*F=&vil[vil[home].foe-1]; int dx=F->x-c->x, dy=F->y-c->y; if(dx*dx+dy*dy>4){ int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return; } }
            else { int fv=vil[home].foe-1; vil[fv].stock=(u16)(vil[fv].stock>10?vil[fv].stock-10:0); if(rnd()%12==0&&vil[fv].pop<2){ if(!vilConquer(fv,home)) vilRaze(fv,1); } } }
        // settle: a grown person far from their own village starts one
        if(home<0&&c->age>12&&c->food>60&&(rnd()%6)==0&&vilFound(s->a,c->x,c->y)>=0) return;
        // stay close to home
        if(home>=0){ int dx=vil[home].x-c->x, dy=vil[home].y-c->y; if(dx*dx+dy*dy>64){ int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return; } } }
    }
    // hunt
    if((diet==1||(diet==2&&c->food<50))&&c->food<(HAS(c,TR_FIERCE)?115:90)){
        int best=-1,bd=99; for(int dy=-4;dy<=4;dy++) for(int dx=-4;dx<=4;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||(HAS(&cr[j],TR_CAMO)&&(rnd()&1))) continue;
                const Arch*B=&arch[sp[cr[j].sp].a]; if(B->diet==1||B->diet==5||B->diet==4&&diet!=1||sp[cr[j].sp].a==s->a) continue; if(diet==2&&B->diet!=0) continue;
                int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){ bd=d; best=j; } }
        if(best>=0){ Cr*p=&cr[best];
            if(bd<=1+HAS(c,TR_SPIT)){ int dm=hitDmg(c,p); if(p->hp>dm) p->hp-=(u8)dm; else { c->food+=80; if(c->food>120) c->food=120; killCr(best); } return; }
            int dx=p->x-c->x, dy=p->y-c->y; int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0; int try1=(((dx<0?-dx:dx)>(dy<0?-dy:dy))?0:1);
            for(int k=0;k<2;k++){ int ax=(k==try1)?sx:0, ay=(k==try1)?0:sy; if(!ax&&!ay) continue; int nx=c->x+ax, ny=c->y+ay; if(walkable(s,nx,ny)&&!occ[ny][nx]){ moveTo(c,i,nx,ny); return; } } } }
    // breed
    int bch=A->breed; if(diet==4&&vilNear(s->a,c->x,c->y,8)>=0) bch*=2; if(HAS(c,TR_FERTILE)) bch=bch*3/2; if(HAS(c,TR_TINY)) bch=bch*13/10; if(HAS(c,TR_CLEVER)) bch=bch*13/10; if(HAS(c,TR_MUNCH)) bch=bch*14/10;
    if(c->food>=80&&c->age>8&&(int)(rnd()%100)<bch&&nCr<MAXC-4){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; int n=addCr(c->sp,nx,ny); if(n>=0){ cr[n].age=0; cr[n].food=50; born(&cr[n],&cr[i]); c->food-=40; return; } }
    // wander
    if((rnd()&3)!=0){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; if(walkable(s,nx,ny)&&!occ[ny][nx]) moveTo(c,i,nx,ny); }
}
static void think(int i){ Cr*c=&cr[i]; curExtra=c->alive?trMask(c):0; thinkBody(i); curExtra=0; }
static int monTierMax(void){ int t=(tierOpen-1)/3+year/4; return t>4?4:t; }
static int monCount(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&arch[sp[cr[i].sp].a].diet==5; return n; }
static void monSpawn(void){   // rifts open: monsters of the strength the world has earned appear far from the villages
    if(!monOn||(year<2&&tierOpen<4)) return; int tm=monTierMax(); int cap=(4+tm*6)*WW*WH/16384*(diff==0?6:diff==2?15:10)/10*(worldAge()==1?15:10)/10; if(cap<3) cap=3; if(monCount()>=cap||(rnd()%3)) return;
    int a; for(int k=0;k<8;k++){ a=ARCH_A+(int)(rnd()%(ARCH_N-ARCH_A)); int tr=monTier[a-ARCH_A]; if(tr<=tm&&(tr>=tm-2||(rnd()&7)==0)) break; if(k==7) a=ARCH_A+(int)(rnd()%4); }
    int s=8+(a-8)*16+(int)(rnd()%16); int grp=monTier[a-ARCH_A]==0?1+(int)(rnd()%3):1;
    int home=-1; { int n=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&(rnd()%(++n))==0) home=v; }   // a rift opens 14 to 40 tiles from a village (any, at random)
    for(int k=0;k<40;k++){ int x=rnd()%WW, y=rnd()%WH; if(home>=0){ int r=14+(int)(rnd()%27); int ang=(int)(rnd()&3); x=vil[home].x+(ang&1?r:-r)+(int)(rnd()%9)-4; y=vil[home].y+(ang&2?r:-r)+(int)(rnd()%9)-4; if(x<0||y<0||x>=WW||y>=WH) continue; }
        if(!walkable(&sp[s],x,y)||occ[y][x]||vilAny(x,y,9)>=0) continue; int n=0; for(int q=0;q<grp;q++) { int k2=addCr(s,x+(q?(int)(rnd()%3)-1:0),y+(q?(int)(rnd()%3)-1:0)); if(k2>=0){ n++; int rr=(int)(rnd()%100); if(tm>=1&&rr<4+tm*3) cr[k2].rk=1; else if(tm>=2&&rr<6+tm*3) cr[k2].rk=2; if(tm>=2&&(rnd()&7)==0) giveTrait(&cr[k2],1+(int)(rnd()%(TR_N-1)),0); cr[k2].hp=(u8)maxHp(&cr[k2]); } } if(n) return; }
}
static void towerFire(void){   // the towers of a village shoot the nearest monsters in reach
    for(int v=0;v<VIL_N;v++){ const Vil*V=&vil[v]; if(!V->on||!V->towers) continue; int dm=4+3*(V->towers>4?4:V->towers), shots=0;
        for(int i=0;i<nCr&&shots<2;i++){ Cr*c=&cr[i]; if(!c->alive||arch[sp[c->sp].a].diet!=5) continue; int dx=c->x-V->x, dy=c->y-V->y; if(dx*dx+dy*dy>100) continue; shots++; if(c->hp>dm) c->hp-=(u8)dm; else killCr(i); } } }
static int weather, weatherT=900, flashT;     // 0 clear, 1 rain, 2 storm, 3 snow
static void volcanoTick(void){   // lava creeps over nearby ground now and then, scorching it, and cools to rock in the rain
    if(tick&15) return; int x=rnd()%WW, y=rnd()%WH; if(world[y][x]!=T_LAVA) return;
    int dx=(int)(rnd()%3)-1, dy=(int)(rnd()%3)-1, nx=x+dx, ny=y+dy; if(nx<0||ny<0||nx>=WW||ny>=WH) return; u8 t=world[ny][nx];
    if(weather==1||weather==2){ if(rnd()%6==0){ setCell(x,y,T_MOUNT); } return; }
    if(rnd()%90==0&&(t==T_MOUNT||t==T_HILL||t==T_DIRT||t==T_GRASS||t==T_FOREST||t==T_DESERT)){ setCell(nx,ny,T_LAVA); if(occ[ny][nx]) killCr(occ[ny][nx]-1); if(bld[ny][nx]){ bld[ny][nx]=0; putCell(nx,ny); } }
    else if(rnd()%400==0) setCell(x,y,T_MOUNT); }
static void kingdomTick(void){ memset(kcSv,0,sizeof kcSv); memset(kcDv,0,sizeof kcDv); memset(kcZv,0,sizeof kcZv); kcOn=1; kingdomTick0(); kcOn=0; }
static void eventTick(void){   // now and then the world has an event of its own
    if(tick%1200==0) memset(vilSick,0,sizeof vilSick);
    if(tick<1500||(tick%1500)!=0||rnd()%3) return; int e=(int)(rnd()%5);
    if(e==0){ for(int k=0;k<6;k++){ int x=rnd()%WW, y=rnd()%WH; burnAt(x,y,255); if(world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_MOUNT) setCell(x,y,(rnd()&1)?T_DIRT:T_LAVA); if(occ[y][x]) killCr(occ[y][x]-1); } siprintf(info,"A COMET SHOWER"); flashT=8; fxReq(FX_METEOR); }
    else if(e==1){ for(int v=0;v<VIL_N;v++) if(vil[v].on) vil[v].stock=(u16)(vil[v].stock+100>400?400:vil[v].stock+100); siprintf(info,"A BOUNTIFUL HARVEST"); fxReq(FX_BLESS); }
    else if(e==4){ int tries=0; while(tries++<40){ int i=(int)(rnd()%(nCr?nCr:1)); if(nCr&&cr[i].alive&&arch[sp[cr[i].sp].a].diet!=5&&sick[i]==0){ sick[i]=1; siprintf(info,"A PLAGUE BREAKS OUT"); fxReq(FX_WAR); break; } } }
    else if(e==2){ int n=0; for(int i=0;i<nCr&&n<12;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet!=5&&cr[i].tr2==0&&(rnd()&7)==0){ giveTrait(&cr[i],TR_MUNCH,0); cr[i].food=200; n++; } siprintf(info,"THE GREAT MUNCHIES"); fxReq(FX_BIRTH); }
    else { for(int v=0;v<VIL_N;v++) vil[v].foe=0; for(int k=0;k<KN_N;k++){ kd[k].war=0; kd[k].weary=0; } warMatrix(); siprintf(info,"A FESTIVAL: ALL FEUDS FORGOTTEN"); fxReq(FX_PEACE); } }
static void weatherTick(void){
    if(--weatherT<=0){ int r=(int)(rnd()%100), cold=(year&3)==3; weather=r<55?0:r<80?(cold?3:1):r<90?(cold?3:2):(cold?3:1); weatherT=weather?1200+(int)(rnd()%2400):900+(int)(rnd()%1800);
        siprintf(info,"%s",weather==1?"IT BEGINS TO RAIN":weather==2?"A STORM ROLLS IN":weather==3?"SNOW FALLS":"THE SKY CLEARS"); }
    if((weather==1||weather==2)&&(tick&7)==0){ int x=rnd()%WW, y=rnd()%WH; u8 t=world[y][x]; if(t==T_DIRT&&(rnd()&3)==0) setCell(x,y,T_GRASS); else if(t==T_LAVA&&(rnd()&15)==0) setCell(x,y,T_MOUNT); }
    if(weather==2&&(rnd()%500)==0){ int x=rnd()%WW, y=rnd()%WH; burnAt(x,y,200); if(occ[y][x]) killCr(occ[y][x]-1); if(world[y][x]==T_FOREST&&(rnd()&3)==0) setCell(x,y,T_DIRT); flashT=8; fxReq(FX_LIGHTNING); }
    if(flashT>0) flashT--; }
static int cursor=0, vilCur=0;
static void simulate(void){
    if(paused) return;
    for(int i=0;i<nCr;i++){ Cr*c=&cr[i]; if(c->ox){ int sp_=arch[sp[c->sp].a].spd; (void)sp_; c->ox+= c->ox<0?1:-1; } if(c->oy){ c->oy+= c->oy<0?1:-1; } }   // glide into the tile
    int n=nCr; if(n==0) return; int per=n>96?96:n;
    for(int k=0;k<per;k++){ if(cursor>=nCr) cursor=0; Cr*c=&cr[cursor]; if(c->alive&&++c->t>=effSpd(c)) { c->t=0; think(cursor); } cursor++; }
    // the ground heals: dirt turns back to grass beside grass, lava cools
    for(int k=0;k<12;k++){ int x=rnd()%WW, y=rnd()%WH; u8 t=world[y][x]; if(t==T_DIRT){ int d=rnd()&3; int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&(world[ny][nx]==T_GRASS||world[ny][nx]==T_FOREST)) setCell(x,y,T_GRASS); }
        else if(t==T_GRASS&&(rnd()&63)==0){ int d=rnd()&3; int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&world[ny][nx]==T_FOREST&&(rnd()&7)==0) setCell(x,y,T_FOREST); } }
    if((tick&15)==0){ vilCur=(vilCur+1)%VIL_N; vilTick(vilCur); if(vilCur==0) warMatrix(); } if((tick&127)==64) kingdomTick(); if((tick&63)==33) achCheck();
    weatherTick(); volcanoTick(); eventTick();
    if((tick&63)==0) monSpawn();
    if((tick&15)==0) towerFire();
    if(((tick)&255)==0){ compact(); if((tick%2048)==0){ year++; fxReq(FX_YEAR); } unlockTiers(); }
}

// ---------------------------------------------------------------------------------------------- the item catalogue (generated: materials x kinds, plus potions, seeds, foods, relics, gems, god tools)
enum { IK_WEAPON, IK_ARMOR, IK_POTION, IK_SEED, IK_FOOD, IK_RELIC, IK_GEM, IK_TOOL };
typedef struct { char nm[26]; u8 kind, tier, val, sub; } Item;
#define IT_N 600
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
static const char* const jokeWep[8]={"ROLLED NEWSPAPER","LONG SOCK OF COINS","LAWN CHAIR","GIANT FOAM FINGER","BASS GUITAR","FROZEN BAGUETTE","INFLATABLE HAMMER","THE SPATULA"}; static const u8 jokeWepAtk[8]={4,6,7,3,9,8,2,10};
static const char* const jokeArm[8]={"TIE-DYE CLOAK","BATHROBE","BEANIE","SLIPPERS","COUCH CUSHION","PIZZA BOX SHIELD","FANNY PACK","SWEATPANTS OF VALOR"}; static const u8 jokeArmHp[8]={6,8,5,4,12,10,5,14};
static const char* const snackNm[20]={"MUNCHIES BAG","NACHO MOUNTAIN","3AM PIZZA SLICE","INSTANT NOODLE CUP","GAS STATION BURRITO","SLEEPY SANDWICH","CHEESY PUFFS","FROZEN WAFFLES","SPACE BROWNIES","BOTTOMLESS CEREAL","SUSPICIOUS GUMMIES","THE LAST DORITO","TACO TUESDAY","HOT POCKET LAVA","MIDNIGHT CEREAL","TEN DOLLAR NACHOS","SLEEPING BAG BURRITO","SOUR WORMS","ICE CREAM FOR DINNER","THE BIG BAG OF CHIPS"};
static const char* const chillSeed[12]={"GIGGLE WEED","CHILL LEAF","LAZY LOTUS","SPACE SHROOM","DANK FERN","MUNCHBERRY","COUCH POTATO","TRIPPY VINE","SNACK BUSH","SLEEPY MOSS","MELLOW MELON","PURPLE HAZE FERN"}; static const u8 chillTerr[12]={T_FOREST,T_GRASS,T_SWAMP,T_FOREST,T_JUNGLE,T_GRASS,T_DIRT,T_JUNGLE,T_FOREST,T_SWAMP,T_GRASS,T_JUNGLE};
static const char* const chillRel[20]={"LAVA LAMP","BEANBAG OF PEACE","GLOWING POSTER","BONG OF DESTINY","OLD VINYL","LOST LIGHTER","HACKY SACK","WIND CHIMES","CONE OF WISDOM","ROLLING TRAY","SNOOP'S BLESSING","THE VERY LONG COUCH","DISCO BALL","BLACKLIGHT","GLITTER JAR","PAPER CRANE","GOOD VIBES JAR","WOBBLY TABLE","TOASTER OF ENLIGHTENMENT","WI-FI PASSWORD"};
static void makeChillItems(void);
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
    makeChillItems();
}
static void makeChillItems(void){
    for(int k=0;k<20;k++){ Item*I=&it[nIt++]; strcpy(I->nm,snackNm[k]); I->kind=IK_FOOD; I->tier=(u8)(1+k*12/20); I->val=(u8)(45+k*4); I->sub=(u8)(20+k); }
    for(int k=0;k<12;k++){ Item*I=&it[nIt++]; strcpy(I->nm,chillSeed[k]); strcat(I->nm," SEEDS"); I->kind=IK_SEED; I->tier=(u8)(1+k); I->val=chillTerr[k]; I->sub=(u8)(20+k); }
    for(int k=0;k<20;k++){ Item*I=&it[nIt++]; strcpy(I->nm,chillRel[k]); I->kind=IK_RELIC; I->tier=(u8)(2+k*11/20); I->val=(u8)(k%5); I->sub=(u8)(40+k); }
    for(int k=0;k<8;k++){ Item*I=&it[nIt++]; strcpy(I->nm,jokeWep[k]); I->kind=IK_WEAPON; I->tier=(u8)(1+k*10/8); I->val=(u8)(jokeWepAtk[k]+k); I->sub=(u8)(k%8); }
    for(int k=0;k<8;k++){ Item*I=&it[nIt++]; strcpy(I->nm,jokeArm[k]); I->kind=IK_ARMOR; I->tier=(u8)(1+k*10/8); I->val=(u8)(jokeArmHp[k]+k*2); I->sub=(u8)(k%6); } }
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
// ---------------------------------------------------------------------------------------------- the cast and their words
enum { W_WISP, W_MIRA, W_GRUUL, W_ELOWEN, W_THORGRIM, W_HOLLOW, W_DAVE, W_N };
static const char* const whoNm[W_N]={"WISP","MIRA","GRUUL","ELOWEN","THORGRIM","THE HOLLOW KING","DAVE THE COUCH WIZARD"};
static const u8 whoShape[W_N]={17,5,26,24,25,17,22}, whoFam[W_N]={14,12,4,5,11,15,3};
typedef struct { u8 who; const char*txt; } Dl;
static const Dl dl0[]={{W_WISP,"You woke. Good. I am Wisp. I was a spark in the dark before you, and I have waited a very long time for a god."},{W_WISP,"The world is empty and quiet. Touch it. Rain, fire, new land. Teach me what you are."},{0,0}};
static const Dl dl1[]={{W_WISP,"Land is not enough. Land is a stage. We need someone to stand on it. Use SPAWN, or SETTLE, and make us some folk."},{W_MIRA,"Who spoke the rain? ...Was it you, up there? I am Mira. We are cold, and we are many, and we are afraid."},{W_WISP,"Be gentle with her. She is the first to ever look up."},{0,0}};
static const Dl dl2[]={{W_MIRA,"The nights are long and the wolves are longer. We piled stones against the wind, but stones do not hold a roof."},{W_WISP,"Give them time and food and room. Homes will follow. People are better builders than gods."},{0,0}};
static const Dl dl3[]={{W_GRUUL,"I am Gruul of the orcs. The humans took the river. It was OUR river. Tell your sky-god to keep to the clouds."},{W_MIRA,"We did not know it was yours. We only knew we were thirsty."},{W_WISP,"Two peoples, one river. You can bless them, or break them. A god chooses."},{W_DAVE,"Whoa. Hey. Is this the part where the world starts? I am Dave. I have been a wizard on this couch for, like, a thousand years."},{W_DAVE,"Anyway. Snacks. If you give a creature a snack, it might just... get it. Like, really get it. Try the nachos."},{0,0}};
static const Dl dl4[]={{W_HOLLOW,"I smell a new god. How sweet. How young."},{W_WISP,"...He is here. The Hollow King. He was the god before you, and the one before that."},{W_WISP,"His creatures are coming. Smite them, arm your people, build towers. He will not stop."},{0,0}};
static const Dl dl5[]={{W_THORGRIM,"Thorgrim, smith. Give us iron, godguy, and we will make the dark bleed. Swords, shields, anything with an edge."},{W_GRUUL,"Orcs fight better than they build. Give us an axe and stand back."},{W_MIRA,"Use the GIFT power. A gift to a hand is worth ten prayers."},{0,0}};
static const Dl dl6[]={{W_MIRA,"A hall of stone. My grandmother would not believe it. Our children will never know this cold."},{W_THORGRIM,"Stone remembers what wood forgets. Build it tall."},{0,0}};
static const Dl dl7[]={{W_ELOWEN,"I am Elowen. I keep the names of things. If a nightmare has a name, it shrinks a little."},{W_ELOWEN,"Walk the world and show me every creature the Hollow King sends. I will write each one in the bestiary."},{0,0}};
static const Dl dl8[]={{W_GRUUL,"The humans and the orcs sit at one fire now. I never thought I would say it. Peace is a strange meat."},{W_MIRA,"Kings and queens rise and fall, godguy. We do not need them to be good. We need them to be many."},{0,0}};
static const Dl dl9[]={{W_HOLLOW,"I was here before you. I made seas, and cities, and loved them. They all burned, or forgot me. Or I forgot them."},{W_WISP,"Do not listen. He wants you to be as tired as he is."},{W_THORGRIM,"Hold the walls! The horde is at the gate!"},{W_DAVE,"Dude. Kings. Crowns. So much stress. Have you tried just... sitting? The couch is very long, godguy. It has room."},{0,0}};
static const Dl dl10[]={{W_MIRA,"I am old now, godguy. The city never sleeps, and neither do I. Thank you for the rain. And the fire."},{W_WISP,"She will not be here forever. That is what makes it matter."},{0,0}};
static const Dl dl11[]={{W_ELOWEN,"Every one of them has a page now. Venomous, giant, tiny, winged. I never thought the dark could be so varied."},{W_ELOWEN,"And do you know, they are not all evil. Some only follow the Hollow King because no one asked them to stay."},{0,0}};
static const Dl dl12[]={{W_WISP,"I have learned something about the Hollow King. He is not a monster. He is the last god who gave up."},{W_WISP,"Every world he made, he watched die. So he made monsters instead. They never leave."},{W_GRUUL,"Then end him. Or end the loneliness. I do not know which is harder."},{W_DAVE,"The long peace. Yeah. That is the vibe. Honestly the best thing you ever made was a quiet afternoon and a bag of cheesy puffs."},{W_WISP,"Dave, how are you in my story."},{W_DAVE,"Wisp. Man. The couch is in everyone's story."},{0,0}};
static const Dl dl13[]={{W_HOLLOW,"You have counted my children and you have killed many. Why do you still come?"},{W_WISP,"Because they are not only monsters. Because you are not only a king."},{W_HOLLOW,"...Show me, then. Show me a world that does not burn."},{0,0}};
static const Dl dl14[]={{W_HOLLOW,"The sky is open. The rain falls. Mira's children laugh in a city that never sleeps. It does not burn."},{W_HOLLOW,"Thank you, godguy. I will stay, if the world will have me."},{W_WISP,"There is room for two gods. There always was. Welcome home."},{0,0}};
static const Dl*const dlgTab[15]={dl0,dl1,dl2,dl3,dl4,dl5,dl6,dl7,dl8,dl9,dl10,dl11,dl12,dl13,dl14};
static const char* const chapTxt[CHAP_N]={"THE FIRST LIGHT TOUCHES THE LAND","SOMEONE LOOKS UP AT YOU","ROOFS GO UP AGAINST THE NIGHT","TWO PEOPLES MEET AT THE WELL","THE DARKNESS HAS TEETH","THEY LEARN TO FIGHT BACK","STONE REMEMBERS, WOOD FORGETS","YOU BEGIN TO NAME THE FEARS","KINGS RISE. KINGS FALL.","THE HORDE BREAKS ON THE WALLS","A CITY THAT NEVER SLEEPS","EVERY NIGHTMARE HAS A PAGE","THE LONG PEACE BEGINS","THE WORLD KNOWS YOUR NAME"};
static void chapterDone(void){   // a chapter is finished (by play, or by the debug menu): items open, the jingle plays, the aural sculptures open at chapter 3
    siprintf(info,"%.32s",chapTxt[chapter]); chapter++; dlgPend=chapter<CHAP_N?chapter:14;
    if(chapter>=3&&!auralOpen){ auralOpen=1; siprintf(info,"AURAL SCULPTURES UNLOCKED"); }
    if(chapter>=CHAP_N){ storyDone=1; tierOpen=14; musReq(GM_GODHOOD); fxReq(FX_CHAPTER_DONE); }
    else { if(chapter+1>tierOpen) tierOpen=chapter+1>14?14:chapter+1; musReq(GM_ASCEND); fxReq(FX_CHAPTER_DONE); } }
static int slainCount(void){ int n=0; for(int m=0;m<320;m++){ int sI=8+(ARCH_A+m/16-8)*16+m%16; n+=(slainB[sI>>3]>>(sI&7))&1; } return n; }
static void checkFull(void){ if(!fullDone&&storyDone&&seenN>=320&&slainCount()>=320){ fullDone=1; siprintf(info,"100%%! THE REMIXES ARE YOURS"); musReq(GM_GODHOOD); fxReq(FX_UNLOCK); } }
static void unlockTiers(void){   // every 256 ticks: is the chapter you are on done?
    checkFull();
    if(!storyOn||storyDone||chapter>=CHAP_N) return;
    const Chap*C=&chap[chapter]; if(chapVal(C->kind)<C->target) return;
    chapterDone(); }
// ---------------------------------------------------------------------------------------------- powers (the table is the catalogue)
typedef struct { const char*nm; u8 kind, arg; } Pw;
enum { PK_INSPECT, PK_PAINT, PK_SPAWN, PK_RAIN, PK_FIRE, PK_BOLT, PK_METEOR, PK_KILL, PK_HEAL, PK_FEED, PK_SETTLE, PK_WAR, PK_PEACE, PK_BLESS, PK_GIFT, PK_MUTATE, PK_QUAKE, PK_FLOOD, PK_BLOOM, PK_VIBE, PK_PLAGUE };
static const Pw pw[]={ {"INSPECT",PK_INSPECT,0},{"WATER",PK_PAINT,T_WATER},{"DEEP SEA",PK_PAINT,T_DEEP},{"SAND",PK_PAINT,T_SAND},{"GRASS",PK_PAINT,T_GRASS},{"FOREST",PK_PAINT,T_FOREST},{"JUNGLE",PK_PAINT,T_JUNGLE},
    {"HILLS",PK_PAINT,T_HILL},{"MOUNTAIN",PK_PAINT,T_MOUNT},{"SNOW",PK_PAINT,T_SNOW},{"DESERT",PK_PAINT,T_DESERT},{"SWAMP",PK_PAINT,T_SWAMP},{"LAVA",PK_PAINT,T_LAVA},
    {"SPAWN",PK_SPAWN,0},{"RAIN",PK_RAIN,0},{"FIRE",PK_FIRE,0},{"LIGHTNING",PK_BOLT,0},{"METEOR",PK_METEOR,0},{"SMITE",PK_KILL,0},{"HEAL",PK_HEAL,0},{"FEED",PK_FEED,0},{"SETTLE",PK_SETTLE,0},{"WAR",PK_WAR,0},{"PEACE",PK_PEACE,0},{"BLESS",PK_BLESS,0},{"GIFT",PK_GIFT,0},{"MUTATE",PK_MUTATE,0},{"EARTHQUAKE",PK_QUAKE,0},{"TSUNAMI",PK_FLOOD,0},{"BLOOM",PK_BLOOM,0},{"GOOD VIBES",PK_VIBE,0},{"PLAGUE",PK_PLAGUE,0} };
#define PW_N ((int)(sizeof(pw)/sizeof(pw[0])))
static int inspected=-1, trSel=1;
static int inspT, inspKind, inspIdx;   // the inspect card: what was looked at (1 creature, 2 village) and for how long it stays up
static void usePower(int tx,int ty,int fresh){
    const Pw*P=&pw[power]; int r=brush;
    if(fresh){ stUse++; if(P->kind==PK_GIFT) stGift++; static const u8 fxOf[21]={FX_INSPECT,FX_PAINT,FX_SPAWN,FX_RAIN,FX_FIRE,FX_LIGHTNING,FX_METEOR,FX_SMITE,FX_HEAL,FX_FEED,FX_SETTLE,FX_WAR,FX_PEACE,FX_BLESS,FX_GIFT,FX_BIRTH,FX_METEOR,FX_RAIN,FX_BIRTH,FX_BLESS,FX_WAR}; fxReq(fxOf[P->kind]); } if(P->kind==PK_GIFT&&(it[itSel].kind==IK_WEAPON||it[itSel].kind==IK_ARMOR||it[itSel].kind==IK_RELIC||it[itSel].kind==IK_GEM)) r=1;
    for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ if(dx*dx+dy*dy>r*r+r/2) continue; int x=tx+dx, y=ty+dy; if(x<0||y<0||x>=WW||y>=WH) continue;
        switch(P->kind){
        case PK_PAINT: if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } setCell(x,y,P->arg); if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(!walkable(&sp[c->sp],x,y)&&(rnd()&3)==0) killCr(occ[y][x]-1); } break;
        case PK_SPAWN: if(fresh&&dx==0&&dy==0){ for(int k=0;k<brush;k++) addCr(spSel,x+(k?(int)(rnd()%3)-1:0),y+(k?(int)(rnd()%3)-1:0)); } break;
        case PK_RAIN: if(world[y][x]==T_DIRT||world[y][x]==T_SAND||world[y][x]==T_DESERT) { if((rnd()&3)==0) setCell(x,y,T_GRASS); } else if(world[y][x]==T_LAVA) setCell(x,y,T_MOUNT); else if(world[y][x]==T_GRASS&&(rnd()&15)==0) setCell(x,y,T_FOREST); break;
        case PK_FIRE: burnAt(x,y,90); if(world[y][x]==T_FOREST||world[y][x]==T_GRASS||world[y][x]==T_JUNGLE){ if((rnd()&1)==0) setCell(x,y,T_DIRT); } if(occ[y][x]&&(rnd()&1)) { Cr*c=&cr[occ[y][x]-1]; if(c->hp>10) c->hp-=10; else killCr(occ[y][x]-1); } break;
        case PK_BOLT: if(dx==0&&dy==0){ burnAt(x,y,255); if(occ[y][x]) killCr(occ[y][x]-1); if(world[y][x]==T_FOREST) setCell(x,y,T_DIRT); } break;
        case PK_METEOR: if(fresh){ int rr=dx*dx+dy*dy; burnAt(x,y,255); if(rr<=2) setCell(x,y,T_LAVA); else if(rr<=r*r) setCell(x,y,(rnd()&1)?T_DIRT:T_HILL); if(occ[y][x]) killCr(occ[y][x]-1); } break;
        case PK_KILL: if(occ[y][x]) killCr(occ[y][x]-1); break;
        case PK_HEAL: if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; c->hp=(u8)maxHp(c); if(sick[occ[y][x]-1]==1) sick[occ[y][x]-1]=2; } break;      // healing cures the plague and leaves the creature immune
        case PK_PLAGUE: if(occ[y][x]&&sick[occ[y][x]-1]==0&&(fresh||rnd()%4==0)) sick[occ[y][x]-1]=1; break;
        case PK_FEED: if(occ[y][x]) cr[occ[y][x]-1].food=120; else if(world[y][x]==T_DIRT) setCell(x,y,T_GRASS); break;
        case PK_SETTLE: if(fresh&&dx==0&&dy==0){ int race=spSel<8?sp[spSel].a:0; if(vilFound(race,x,y)<0) siprintf(info,"NO ROOM FOR A VILLAGE"); else { for(int k=0;k<4;k++) addCr(spSel<8?spSel:0,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); } } break;
        case PK_WAR: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,4); if(v>=0){ int best=-1,bd=2000; for(int u=0;u<VIL_N;u++){ if(!vil[u].on||u==v||vil[u].race==vil[v].race) continue; int ddx=vil[u].x-vil[v].x, ddy=vil[u].y-vil[v].y, d=ddx*ddx+ddy*ddy; if(d<bd){ bd=d; best=u; } } if(best>=0){ vil[v].foe=(u8)(best+1); siprintf(info,"WAR! %s ATTACKS %s",arch[vil[v].race].nm,arch[vil[best].race].nm); if(vilK[v]&&vilK[best]&&vilK[v]!=vilK[best]) kdDeclareX(vilK[v]-1,vilK[best]-1,1); } } } break;
        case PK_PEACE: if(fresh&&dx==0&&dy==0){ for(int v=0;v<VIL_N;v++) vil[v].foe=0; for(int k=0;k<KN_N;k++){ kd[k].war=0; kd[k].weary=0; } warMatrix(); siprintf(info,"PEACE"); } break;
        case PK_BLESS: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,4); if(v>=0){ vil[v].stock=(u16)(vil[v].stock+150>400?400:vil[v].stock+150); } } break;
        case PK_GIFT: { const Item*I=&it[itSel]; Cr*c=occ[y][x]?&cr[occ[y][x]-1]:0; int cen=(dx==0&&dy==0);
            switch(I->kind){
            case IK_WEAPON: if(c&&cen&&fresh){ c->wep=(u16)(itSel+1); } break;
            case IK_ARMOR: if(c&&cen&&fresh){ int old=c->arm?it[c->arm-1].val:0; c->arm=(u16)(itSel+1); int hp=c->hp+I->val*2-old*2; c->hp=(u8)(hp>250?250:hp<1?1:hp); } break;
            case IK_FOOD: if(c){ int f=c->food+I->val*2; c->food=(u8)(f>250?250:f); if(I->sub>=20&&(rnd()&3)==0&&c->tr2==0){ giveTrait(c,TR_CHILL+(int)(rnd()%8),1); siprintf(info,"IT HITS DIFFERENT"); } if(I->sub>=20) stSnack++; } else if(world[y][x]==T_DIRT) setCell(x,y,T_GRASS); break;
            case IK_POTION: if(c){ switch(I->sub){ case 0: case 19: c->hp=arch[sp[c->sp].a].hp; if(I->sub==19){ c->age=0; c->food=120; } break; case 1: case 16: c->food=250; break; case 2: c->age=0; break; case 3: if(cen&&fresh){ int n=addCr(c->sp,c->x+1,c->y); if(n<0) addCr(c->sp,c->x,c->y+1); } break; case 4: c->wep=(u16)(c->wep?c->wep:1); break; case 5: { int hp=c->hp+40; c->hp=(u8)(hp>250?250:hp); } break; case 15: { int hp=c->hp+60; c->hp=(u8)(hp>250?250:hp); } break; default: c->food=100; } } break;
            case IK_SEED: if(world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_LAVA&&!bld[y][x]&&(rnd()&1)) setCell(x,y,(u8)I->val); break;
            case IK_RELIC: case IK_GEM: if(cen&&fresh){ int v=vilAny(x,y,5); if(v>=0){ Vil*V=&vil[v]; if(I->kind==IK_GEM) V->stock=(u16)(V->stock+I->val*3>400?400:V->stock+I->val*3); else switch(I->val){ case 0: V->stock=400; break; case 1: for(int k=0;k<3;k++) if(placeBld(v,B_HOUSE,0)) V->houses++; break; case 2: if(V->lvl<3) V->lvl++; break; case 3: V->foe=0; break; default: for(int k=0;k<8;k++) addCr(0,V->x+(int)(rnd()%5)-2,V->y+(int)(rnd()%5)-2); } } } break;
            case IK_TOOL: { int t=I->val; u8 w=world[y][x]; if(bld[y][x]) break;
                switch(t){ case 0: if(w==T_DIRT) setCell(x,y,T_GRASS); break; case 1: if(w==T_GRASS||w==T_SAND||w==T_DIRT) setCell(x,y,T_WATER); break; case 2: if(w==T_GRASS||w==T_FOREST) setCell(x,y,T_HILL); else if(w==T_HILL) setCell(x,y,T_MOUNT); break; case 3: if(w==T_DIRT||w==T_SAND||w==T_DESERT) setCell(x,y,T_GRASS); break;
                    case 4: if(w==T_GRASS||w==T_FOREST) setCell(x,y,T_DIRT); break; case 5: if(w==T_LAVA) setCell(x,y,T_MOUNT); break; case 6: if(w==T_MOUNT) setCell(x,y,T_HILL); else if(w==T_HILL) setCell(x,y,T_GRASS); break; case 7: if(w==T_FOREST||w==T_JUNGLE) setCell(x,y,T_GRASS); break;
                    case 8: if(w==T_WATER) setCell(x,y,T_SAND); break; case 9: if(w==T_SWAMP) setCell(x,y,T_GRASS); break; case 10: if(w==T_GRASS&&(rnd()&1)) setCell(x,y,T_FOREST); break; default: if(w==T_HILL||w==T_MOUNT||w==T_FOREST||w==T_JUNGLE) setCell(x,y,T_GRASS); } } break; } } break;
        case PK_QUAKE: { int rr=dx*dx+dy*dy; if(fresh&&dx==0&&dy==0) flashT=6; u8 t=world[y][x]; if(t==T_MOUNT||t==T_HILL){ if(rnd()%3==0) setCell(x,y,(rnd()&1)?T_HILL:T_DIRT); } else if(t!=T_WATER&&t!=T_DEEP&&t!=T_LAVA&&rnd()%7==0) setCell(x,y,T_DIRT);
            if(bld[y][x]&&rnd()%3==0){ bld[y][x]=0; bldv[y][x]=0; putCell(x,y); } if(occ[y][x]&&rnd()%3==0){ Cr*c=&cr[occ[y][x]-1]; if(c->hp>8) c->hp-=8; else killCr(occ[y][x]-1); } (void)rr; } break;
        case PK_FLOOD: { u8 t=world[y][x]; if(t!=T_MOUNT&&t!=T_DEEP&&(rnd()&3)!=0){ if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } setCell(x,y,t==T_LAVA?T_MOUNT:T_WATER); if(occ[y][x]&&!walkable(&sp[cr[occ[y][x]-1].sp],x,y)&&(rnd()&1)) killCr(occ[y][x]-1); } } break;
        case PK_BLOOM: { u8 t=world[y][x]; if(t==T_GRASS){ if(rnd()&1) setCell(x,y,T_FOREST); } else if(t==T_DIRT||t==T_SAND){ setCell(x,y,T_GRASS); } else if(t==T_DESERT&&(rnd()&3)==0) setCell(x,y,T_GRASS); if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; c->hp=(u8)maxHp(c); c->food=120; } } break;
        case PK_VIBE: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,5); if(v>=0){ vil[v].foe=0; warMatrix(); } if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(c->tr2==0&&!(HAS(c,TR_CHILL)||HAS(c,TR_MUNCH)||HAS(c,TR_GIGGLY)||HAS(c,TR_ZEN)||HAS(c,TR_FLOATY))){ giveTrait(c,TR_CHILL+(int)(rnd()%8),1); siprintf(info,"GOOD VIBES"); } else siprintf(info,"ALREADY VIBING"); } else if(v>=0) siprintf(info,"THE VILLAGE CHILLS OUT"); } break;
        case PK_MUTATE: if(fresh&&dx==0&&dy==0&&occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(trSel==0){ c->tr=c->tr2=0; siprintf(info,"TRAITS CLEARED"); } else { giveTrait(c,trSel,1); char nm[48]; crName(c,nm); siprintf(info,"%.46s",nm); } c->hp=(u8)maxHp(c); } break;
        case PK_INSPECT: if(dx==0&&dy==0){ if(occ[y][x]){ inspected=occ[y][x]-1; inspKind=1; inspIdx=inspected; inspT=420; } else if(bldv[y][x]){ int v=bldv[y][x]-1; inspKind=2; inspIdx=v; inspT=420; char nm[24]; vilName(v,nm); { char kn[24]=""; if(vilK[v]) kdName(vilK[v]-1,kn); siprintf(info,"%s %s POP%d [%s]",arch[vil[v].race].nm,nm,vil[v].pop,kn); } inspected=-2; } } break; } }
    if(P->kind==PK_INSPECT&&inspected!=-2){ if(inspected>=0&&inspected<nCr&&cr[inspected].alive){ Cr*c=&cr[inspected]; char nm[48]; crName(c,nm); siprintf(info,"%.30s HP%d/%d",nm,c->hp,maxHp(c)); } else siprintf(info,"%s",bioNm[world[ty<WH?ty:0][tx<WW?tx:0]]); }
}
static int planetSize=128;
static void (*loadHook)(const char*,int);
#define LOADING(m,p) do{ if(loadHook) loadHook(m,p); }while(0)
static void newWorld(void){ LOADING("THE VOID",4); gW=gH=planetSize; nCr=0; memset(occ,0,sizeof occ); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(vil,0,sizeof vil); memset(kd,0,sizeof kd); memset(vilK,0,sizeof vilK); memset(vilL,0,sizeof vilL); memset(favVil,0,sizeof favVil); memset(favKd,0,sizeof favKd); jrN=0; stWars=stCapt=stAllies=stSnack=0; nVilOn=0; worldSeed=seedFix?seedFix:rnd(); genWorld(worldSeed); LOADING("SHAPING THE LAND",35); winX=-99; refillWindow();
    LOADING("SEEDING LIFE",50);
    // the first life: a few of each of the first animals on their own ground
    int nAn=260*WW*WH/16384*cLife/2; if(nAn>900) nAn=900;
    for(int k=0;k<nAn;k++){ int a=8+(int)(rnd()%(ARCH_A-8)); int s=8+(a-8)*16+(int)(rnd()%16); int x=rnd()%WW, y=rnd()%WH; if(arch[a].diet==3&&k%2) continue; addCr(s,x,y); }
    LOADING("RAISING THE FIRST VILLAGES",80);
    for(int r=0;r<8&&cLife>0;r++){ if(r>=5&&(rnd()&1)) continue; for(int k=0;k<80;k++){ int x=rnd()%WW, y=rnd()%WH; int v=vilFound(r,x,y); if(v>=0){ for(int q=0;q<5;q++) addCr(r,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); break; } } }   // the peoples start with a village each (the restless ones sometimes stay away)
    info[0]=0; LOADING("READY",100); }
// ---------------------------------------------------------------------------------------------- saving: the whole world, the villages and the story, to one file (the SD card on the DS)
static int shape3;
#define SAVE_MAGIC 0x31534747u
typedef struct { u32 magic; u16 ver, gw, szCr, szVil; u32 tick, year, rs; u32 stKill, stGift, stUse, trSeen, stMut, seed; u16 nCr, seenN, camX, camY; u8 chapter, tierOpen, storyOn, storyDone, monOn, shape3, nVilOn, planet, vcur, aural, debug, full, diff, weather; u16 weatherT; u8 pad_[2]; u8 seen[(SP_N+7)/8], slain[(SP_N+7)/8]; } SaveHdr;
#ifndef NDSHW
typedef FILE SV;
static SV*svOpen(const char*p,const char*m){ return fopen(p,m); } static size_t svWrite(const void*b,size_t s,size_t n,SV*f){ return fwrite(b,s,n,f); } static size_t svRead(void*b,size_t s,size_t n,SV*f){ return fread(b,s,n,f); } static int svClose(SV*f){ return fclose(f); }
#endif
static int saveGame(const char*path){
    compact(); cursor=0;     // the dead are dropped first, so a loaded game goes on exactly as this one would
    SV*f=svOpen(path,"wb"); if(!f) return 0; SaveHdr h; memset(&h,0,sizeof h); h.vcur=(u8)vilCur;
    h.magic=SAVE_MAGIC; h.ver=9; h.weather=(u8)weather; h.weatherT=(u16)weatherT; h.trSeen=trSeen; h.stMut=stMut; h.seed=worldSeed; h.diff=(u8)diff; h.full=(u8)fullDone; h.aural=(u8)auralOpen; h.debug=(u8)debugOn; h.gw=(u16)gW; h.szCr=sizeof(Cr); h.szVil=sizeof(Vil); h.tick=(u32)tick; h.year=(u32)year; h.rs=rs; h.stKill=stKill; h.stGift=stGift; h.stUse=stUse; h.nCr=(u16)nCr; h.seenN=(u16)seenN; h.camX=(u16)camX; h.camY=(u16)camY;
    h.chapter=(u8)chapter; h.tierOpen=(u8)tierOpen; h.storyOn=(u8)storyOn; h.storyDone=(u8)storyDone; h.monOn=(u8)monOn; h.shape3=(u8)shape3; h.nVilOn=(u8)nVilOn; h.planet=(u8)planetSize/2; memcpy(h.seen,seenB,sizeof seenB); memcpy(h.slain,slainB,sizeof slainB);
    int ok=svWrite(&h,sizeof h,1,f)==1;
    for(int y=0;y<gH&&ok;y++){ ok&=svWrite(world[y],1,gW,f)==(size_t)gW; ok&=svWrite(bld[y],1,gW,f)==(size_t)gW; ok&=svWrite(bldv[y],1,gW,f)==(size_t)gW; }
    if(ok&&nCr) ok&=svWrite(cr,sizeof(Cr),nCr,f)==(size_t)nCr; if(ok) ok&=svWrite(vil,sizeof(Vil),VIL_N,f)==(size_t)VIL_N;
    if(ok) ok&=svWrite(kd,sizeof(Kd),KN_N,f)==(size_t)KN_N; if(ok) ok&=svWrite(vilL,1,VIL_N,f)==(size_t)VIL_N; if(ok) ok&=svWrite(vilK,1,VIL_N,f)==(size_t)VIL_N; if(ok){ int n=jrN; ok&=svWrite(&n,sizeof n,1,f)==1; ok&=svWrite(jr,34,24,f)==24; ok&=svWrite(jrYear,2,24,f)==24; u32 sx[8]={achBits,stWars,stCapt,stAllies,stSnack,0,0,0}; ok&=svWrite(sx,4,8,f)==8; ok&=svWrite(favVil,1,48,f)==48; ok&=svWrite(favKd,1,8,f)==8; ok&=svWrite(favItem,1,600,f)==600; ok&=svWrite(favPow,1,40,f)==40; }
    svClose(f); return ok; }
static int loadGame(const char*path){
    SV*f=svOpen(path,"rb"); if(!f) return 0; SaveHdr h; int ok=svRead(&h,sizeof h,1,f)==1;
    if(!ok||h.magic!=SAVE_MAGIC||(h.ver<5||h.ver>9)||h.szCr!=sizeof(Cr)||h.szVil!=sizeof(Vil)||(h.gw!=128&&h.gw!=192&&h.gw!=256)||h.nCr>MAXC){ svClose(f); return 0; }
    gW=gH=h.gw; planetSize=gW; memset(world,0,sizeof world); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(occ,0,sizeof occ);
    for(int y=0;y<gH&&ok;y++){ ok&=svRead(world[y],1,gW,f)==(size_t)gW; ok&=svRead(bld[y],1,gW,f)==(size_t)gW; ok&=svRead(bldv[y],1,gW,f)==(size_t)gW; }
    nCr=h.nCr; if(ok&&nCr) ok&=svRead(cr,sizeof(Cr),nCr,f)==(size_t)nCr; memset(vil,0,sizeof vil); { int nv=h.ver>=8?VIL_N:VIL_OLD; if(ok) ok&=svRead(vil,sizeof(Vil),nv,f)==(size_t)nv; }
    memset(kd,0,sizeof kd); memset(vilK,0,sizeof vilK); memset(vilL,0,sizeof vilL); jrN=0; achBits=stWars=stCapt=stAllies=stSnack=0;
    if(ok&&h.ver>=9){ ok&=svRead(kd,sizeof(Kd),KN_N,f)==(size_t)KN_N; if(ok) ok&=svRead(vilL,1,VIL_N,f)==(size_t)VIL_N; if(ok) ok&=svRead(vilK,1,VIL_N,f)==(size_t)VIL_N; if(ok){ int n=0; ok&=svRead(&n,sizeof n,1,f)==1; ok&=svRead(jr,34,24,f)==24; ok&=svRead(jrYear,2,24,f)==24; jrN=n<0?0:n>24?24:n; u32 sx[8]={0}; if(svRead(sx,4,8,f)==8){ achBits=sx[0]; stWars=sx[1]; stCapt=sx[2]; stAllies=sx[3]; stSnack=sx[4]; u8 t1[48], t2[8], t3[600], t4[40]; if(svRead(t1,1,48,f)==48&&svRead(t2,1,8,f)==8&&svRead(t3,1,600,f)==600&&svRead(t4,1,40,f)==40){ memcpy(favVil,t1,48); memcpy(favKd,t2,8); memcpy(favItem,t3,600); memcpy(favPow,t4,40); } } } }
    svClose(f);
    if(!ok){ return 0; }
    tick=(int)h.tick; year=(int)h.year; rs=h.rs; stKill=h.stKill; stGift=h.stGift; stUse=h.stUse; seenN=h.seenN; camX=h.camX; camY=h.camY; chapter=h.chapter; tierOpen=h.tierOpen; storyOn=h.storyOn; storyDone=h.storyDone; monOn=h.monOn; shape3=h.shape3; nVilOn=h.nVilOn;
    memcpy(seenB,h.seen,sizeof seenB); memcpy(slainB,h.slain,sizeof slainB); nVilOn=0; for(int v=0;v<VIL_N;v++) nVilOn+=vil[v].on;
    for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].x<gW&&cr[i].y<gH) occ[cr[i].y][cr[i].x]=(u16)(i+1); vilCur=h.vcur; weather=h.weather; weatherT=h.weatherT; trSeen=h.trSeen; stMut=h.stMut; worldSeed=h.seed; diff=h.diff; fullDone=h.full; auralOpen=h.aural; debugOn=h.debug; cursor=0; compact(); warMatrix(); winX=-99; return 1; }
static int walkOn, walkX=64, walkY=64;     // walking the globe: the avatar's tile
static int view3d=0, rotX=20, rotY=0, zoom3=40, gClouds=1;       // the globe: rotY = longitude of the centre, rotX = latitude of the centre (degrees), zoom3 = how big
static void faceTile(int x,int y,int*lat,int*lon){ *lon=x*360/WW; int la=90-y*180/WH; *lat=la>85?85:la<-85?-85:la; }
static int shape3;
#ifndef PCTEST
// ---------------------------------------------------------------------------------------------- screens
// ---- light: the day goes round and the water shimmers (a tint on the background palette) ----
static u16 basePal[256];
static void saveBasePal(void){ for(int i=0;i<144;i++) basePal[i]=BG_PALETTE[i]; }
static void applyLight(void){
    int p=(tick>>1)&2047, sun=p<1024?p:2047-p; int L=70+sun*186/1024, Lr=L*236/256, Lg=L*246/256, Lb=L+(256-L)/5+14; if(Lb>256) Lb=256; if(L>256) L=256;
    { int tw=300-(sun>300?sun-300:300-sun); if(tw<0) tw=0; Lr+=tw*Lr/900; Lg+=tw*Lg/3200; Lb-=tw*Lb/1100; if(sun<150){ Lr=Lr*(100-(150-sun)/12)/100; Lb+=(150-sun)/10; } if(Lr>256) Lr=256; if(Lg>256) Lg=256; if(Lb>256) Lb=256; if(Lb<0) Lb=0; }   // warm dawn and dusk, bluer deep night
    if(weather==1||weather==2){ Lr=Lr*78/100; Lg=Lg*80/100; Lb=Lb*90/100; } else if(weather==3){ Lr=Lr*95/100; Lg=Lg*97/100; }
    if(flashT>0){ Lr=Lg=Lb=256; }
    int sh=(tick>>4)&3;
    for(int i=0;i<144;i++){ int src=i; if(i>=T_WATER*8+5&&i<T_WATER*8+8) src=T_WATER*8+5+((i-T_WATER*8-5+sh)%3); else if(i>=T_DEEP*8+4&&i<T_DEEP*8+7) src=T_DEEP*8+4+((i-T_DEEP*8-4+(sh>>1))%3);
        u16 c=basePal[src]; int r=(c&31)*Lr>>8, g=((c>>5)&31)*Lg>>8, b=((c>>10)&31)*Lb>>8; if(i>=T_LAVA*8&&i<T_LAVA*8+8){ int pu=((tick>>3)&15); pu=pu<8?pu:15-pu; r=r*(24+pu)/28; g=g*(20+pu*2)/28; } if(r>31) r=31; if(g>31) g=31; if(b>31) b=31; BG_PALETTE[i]=(u16)(r|(g<<5)|(b<<10)); }
    BG_PALETTE[0]=0;
}
#include "ui.h"
// ---------------------------------------------------------------------------------------------- the globe: a planet drawn by the CPU (no 3D hardware, so it works in every build)
// Every pixel of the disc has a normal (from a table made once per size); a small matrix turns it into a direction on the planet, two table lookups give the longitude and latitude, and the
// map gives the ground. Lit from the upper left, a dark side with city lights, a glowing atmosphere, drifting clouds, a sparkle on the water. Picking a point with the stylus is the same
// maths backwards, so it is exact. The picture is drawn in RAM and sent to the screen as 768 tiles.
#define GR_MAX 120
static u8 gbuf[256*192] __attribute__((aligned(4)));
static u16 gnz[GR_MAX*GR_MAX+1]; static s16 gxn[2*GR_MAX+1]; static int gR=0;
static u16 gAtan[257]; static u16 gAsin[4097]; static u8 gCloud[64][64]; static int gInit;
static u16 gSavedPal[256];
// palette: 1 space, 2 3 stars, 8.. ground (13 kinds x 8 light levels), 112.. clouds, 120.. atmosphere, 126 127 128 lights, 129 sparkle
#define GP_GROUND 8
#define GP_CLOUD 112
#define GP_ATMO 120
static int isqrt32(u32 v){ u32 r=0, b=1u<<30; while(b>v) b>>=2; while(b){ if(v>=r+b){ v-=r+b; r=(r>>1)+b; } else r>>=1; b>>=2; } return (int)r; }
static void globeTables(int R){
    if(!gInit){ gInit=1;
        for(int i=0;i<=256;i++) gAtan[i]=(u16)(atanf(i/256.0f)/6.2831853f*65536.0f);
        for(int i=0;i<=4096;i++){ float v=(i-2048)/2048.0f; gAsin[i]=(u16)((0.5f-asinf(v>1?1:v<-1?-1:v)/3.1415927f)*4095.0f); }
        for(int y=0;y<64;y++) for(int x=0;x<64;x++) gCloud[y][x]=(u8)((vnoise(x*3,y*3,8,77)*3+vnoise(x*3,y*3,4,78))>>2); }
    if(R==gR) return; gR=R; for(int i=0;i<=2*R;i++) gxn[i]=(s16)((i-R)*16384/R);
    for(int d2=0;d2<=R*R;d2++){ u32 num=(u32)(((unsigned long long)(R*R-d2)<<28)/(unsigned)(R*R)); gnz[d2]=(u16)isqrt32(num); } }
static int atan2Turn(int y,int x){   // angle of (x forward, y sideways) as a fraction of a turn, 0..65535 (0 = straight ahead)
    int ax=x<0?-x:x, ay=y<0?-y:y; if(!ax&&!ay) return 32768; int r=ax>=ay?gAtan[(ay<<8)/ax]:16384-gAtan[(ax<<8)/ay];   // 0..16384 (quarter turn)
    int psi=x>=0?r:32768-r; if(y<0) psi=-psi; return psi&65535; }
static int gSin[360], gCos[360];
static void globeAim(int lon,int lat,int*cL,int*sL,int*cP,int*sP){ lon%=360; if(lon<0) lon+=360; int la=lat<0?lat+360:lat; if(!gSin[90]){ for(int i=0;i<360;i++){ gSin[i]=(int)(sinf(i*0.0174533f)*16384.0f); gCos[i]=(int)(cosf(i*0.0174533f)*16384.0f); } } *cL=gCos[lon]; *sL=gSin[lon]; *cP=gCos[la]; *sP=gSin[la]; }
static inline void globeDir(int nx,int ny,int nz,int cL,int sL,int cP,int sP,int*tx,int*ty){
    int t=(-sP*ny+cP*nz)>>14; int mx=(cL*nx+sL*t)>>14, my=(cP*ny+sP*nz)>>14, mz=(-sL*nx+cL*t)>>14;
    *tx=(atan2Turn(mx,mz)*WW)>>16; int ix=(my+16384)>>3; if(ix<0) ix=0; if(ix>4096) ix=4096; int yy=gAsin[ix]; *ty=(yy*WH)>>12; if(*tx>=WW) *tx=WW-1; if(*ty>=WH) *ty=WH-1; if(*ty<0) *ty=0; }
static int globeR(void){ int R=3500/zoom3; return R>GR_MAX?GR_MAX:R<26?26:R; }
static int globePick(int px,int py,int*tx,int*ty){
    int R=globeR(); globeTables(R); int x=px-128, y=96-py; int d2=x*x+y*y; if(d2>R*R) return 0; int cL,sL,cP,sP; globeAim(rotY,rotX,&cL,&sL,&cP,&sP);
    globeDir(gxn[x+R],gxn[y+R],gnz[d2],cL,sL,cP,sP,tx,ty); return 1; }
static void globePalette(void){
    BG_PALETTE[0]=0; BG_PALETTE[1]=RGB15(0,0,2); BG_PALETTE[2]=RGB15(12,12,16); BG_PALETTE[3]=RGB15(31,31,31);
    for(int t=0;t<T_N;t++) for(int l=0;l<8;l++){ int r,g,b; shadeRGB(t*8+(l<1?0:l),&r,&g,&b); int f=l==0?9:l==1?14:18+l*2; r=r*f/26; g=g*f/26; b=b*f/26; if(l==0){ b+=3; } if(r>31) r=31; if(g>31) g=31; if(b>31) b=31; BG_PALETTE[GP_GROUND+t*8+l]=RGB15(r,g,b); }
    for(int l=0;l<8;l++){ int v=6+l*3; BG_PALETTE[GP_CLOUD+l]=RGB15(v,v,v+(l>5?0:2)); }
    for(int l=0;l<6;l++) BG_PALETTE[GP_ATMO+l]=RGB15(2+l,6+l*3,14+l*3);
    BG_PALETTE[126]=RGB15(31,28,10); BG_PALETTE[127]=RGB15(31,18,4); BG_PALETTE[128]=RGB15(31,6,2); BG_PALETTE[129]=RGB15(31,31,28); }
static int globeKey=-1, globeForce=1;     // the globe is redrawn only when something visible changed (turn, zoom, weather, the 8-tick animation step); the rest of the frames cost nothing
static int globeDraw(void){
    int key=(rotY*31+rotX*17+zoom3*7)^((tick>>3)*131)^(weather*977)^(walkOn*5521); if(!globeForce&&key==globeKey) return 0; globeKey=key; globeForce=0;
    int R=globeR(); globeTables(R); int cL,sL,cP,sP; globeAim(rotY,rotX,&cL,&sL,&cP,&sP);
    memset(gbuf,1,sizeof gbuf); for(int k=0;k<44;k++){ int x=(int)(hsh(k,1,9)%256), y=(int)(hsh(k,2,9)%192); gbuf[y*256+x]=(u8)(2+((k+(tick>>5))&1)); }
    int drift=(tick>>6); int R2=R*R, H=R+3;
    for(int sy=96-H;sy<96+H;sy+=2){ if(sy<0||sy>=192) continue; int y=96-sy; for(int sx=128-H;sx<128+H;sx+=2){ if(sx<0||sx>=256) continue; int x=sx-128; int d2=x*x+y*y; u8 col;
            if(d2>R2){ if(d2<=(R+3)*(R+3)){ int lam=(-x*5-y*5+R*3)>0?5:2; col=(u8)(GP_ATMO+(lam>3?(d2<=(R+1)*(R+1)?5:3):1)); } else continue; }
            else { int nx=gxn[x+R], ny=gxn[y+R], nz=gnz[d2]; int tx,ty; globeDir(nx,ny,nz,cL,sL,cP,sP,&tx,&ty);
                int lam=(-nx*7+ny*7+nz*12)/26;   // light from the upper left, toward the viewer (about -0.43, 0.43, 0.75 in Q14)
                int terr=world[ty][tx]; int lvl; if(lam<-1500) lvl=0; else lvl=(lam+1500)*7/17000; if(lvl>7) lvl=7; if(lvl<0) lvl=0;
                col=(u8)(GP_GROUND+terr*8+lvl);
                if(bld[ty][tx]&&bld[ty][tx]!=B_RUIN){ col=lam<2000?(u8)(126+((tick>>4)&1)):(u8)(GP_GROUND+T_DIRT*8+7); }
                else if(terr==T_LAVA&&lam<3000) col=128;
                else if((terr==T_WATER||terr==T_DEEP)&&lam>11000&&nz>9000&&((nx+ny)&255)<120) col=129;
                if(gClouds){ int cv=gCloud[(ty*64/WH)&63][((tx*64/WW)+drift)&63]; int thr=weather==1||weather==2?105:weather==3?125:165; if(cv>thr){ int cl=lvl+(cv-thr)/12; if(cl>7) cl=7; col=(u8)(GP_CLOUD+cl); } }
                if(nz<3800){ int a=(3800-nz)/700; if(a>5) a=5; if(lam>-2000||a>3) col=(u8)(GP_ATMO+a); } }
            gbuf[sy*256+sx]=col; if(sx+1<256) gbuf[sy*256+sx+1]=col; if(sy+1<192){ gbuf[(sy+1)*256+sx]=col; if(sx+1<256) gbuf[(sy+1)*256+sx+1]=col; } } }
    if(walkOn){ int c=((tick>>3)&1)?3:129; for(int d=4;d<=8;d++){ gbuf[96*256+128+d]=(u8)c; gbuf[96*256+128-d]=(u8)c; gbuf[(96+d)*256+128]=(u8)c; gbuf[(96-d)*256+128]=(u8)c; } }
    return 1;
}
static void globeFlush(void){
    static u8 tl[64] __attribute__((aligned(4)));
    for(int ty=0;ty<24;ty++) for(int tx=0;tx<32;tx++){ for(int y=0;y<8;y++){ const u8*r=gbuf+(ty*8+y)*256+tx*8; for(int x=0;x<8;x++) tl[y*8+x]=r[x]; } vramCopy((u8*)BG_TILE_RAM(1)+(ty*32+tx)*64,tl,64); } }
static void globeEnter(void){
    for(int i=0;i<256;i++) gSavedPal[i]=BG_PALETTE[i]; globePalette();
    videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32); lcdMainOnBottom();
    for(int i=0;i<128;i++) oamSetHidden(&oamMain,i,true);
    bgSetScroll(0,0,0); for(int my=0;my<32;my++) for(int mx=0;mx<32;mx++) mapPtr[my*32+mx]=(u16)(my<24?my*32+mx:0);
    globeForce=1; globeDraw(); globeFlush(); view3d=1; }
static void leaveGlobe(void){
    walkOn=0; makeTiles(); { int b0=0; (void)b0; } videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_BG1_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32); lcdMainOnBottom();
    uiPalette(); tbInit(); saveBasePal(); winX=-99; view3d=0; }
#define enter3D globeEnter
#define leave3D leaveGlobe
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
    if(weather==2&&!view3d) return GM_STORM;
    if(view3d) return GM_SPHERE;
    int cx=(camX>>3)+16, cy=(camY>>3)+12;
    for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(!c->alive) continue; int a=sp[c->sp].a; if(arch[a].diet!=5||c->hp<100) continue; int dx=c->x-cx, dy=c->y-cy; if(dx*dx+dy*dy>900) continue;
        if(a==ARCH_A+19) return GM_DRAGON; if(a==ARCH_A+17) return GM_DEMON; if(a==ARCH_A+18) return GM_LICH; if(a==ARCH_A+13||a==ARCH_A+12) return GM_KRAKEN; if(a==ARCH_A+16) return GM_DRAGON; }
    for(int a=0;a<8;a++) for(int b=a+1;b<8;b++) if(warM[a][b]) return GM_WAR;
    if(monCount()>0) return GM_MONSTER;
    { int t=world[cy<WH?cy:WH-1][cx<WW?cx:WW-1]; if(t==T_DESERT) return GM_DESERT; if(t==T_SNOW) return GM_SNOW; if(t==T_DEEP||t==T_WATER) return GM_OCEAN; if(t==T_JUNGLE) return GM_JUNGLE; if(t==T_SWAMP) return GM_SWAMP; if(t==T_LAVA||t==T_MOUNT) return GM_VOLCANO; }
    { int lv=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].lvl>lv) lv=vil[v].lvl; if(lv>=3) return GM_CITY; if(lv==2) return GM_MARKET; if(lv==1&&nVilOn>0&&(year&1)) return GM_VILLAGE; }
    int p=(tick>>1)&2047; if(fullDone&&(year&1)) return GM_REMIX0+(year*7+(p>>9))%GM_REMIXES; if(!storyOn&&(year%5==4)) return GM_FREEPLAY;
    return (p<256||p>1792)?GM_NIGHT:p<640?GM_MORNING:p<1408?GM_NOON:GM_SUNSET;
}
static void musUpdate(void){
#ifdef GG_SAFE
    return;     // the safe build has no sound at all
#endif
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
    for(int s=0;s<SHAPE_N;s++) for(int f=0;f<2;f++){ gfxF[s][f]=oamAllocateGfx(&oamMain,SpriteSize_8x8,SpriteColorFormat_16Color); u8 g[32] __attribute__((aligned(4)));
        for(int y=0;y<8;y++) for(int x=0;x<8;x+=2){ int v[2]; for(int q=0;q<2;q++){ char ch=shpRow(s,f,y)[x+q]; v[q]=ch=='b'?1:ch=='d'?2:ch=='a'?3:ch=='e'?4:0; } g[y*4+x/2]=(u8)(v[0]|(v[1]<<4)); } vramCopy(gfxF[s][f],g,32); }
}
static struct { short x, y; } wpt[20]; static int wptInit;
static void drawSprites(void){
    int n=0, nc=100;       // creatures use sprites 0..99, rank crowns 100..107, weather 108..127
    for(int i=0;i<nCr&&n<100;i++){ Cr*c=&cr[i]; if(!c->alive) continue; int px=c->x*8+c->ox-camX, py=c->y*8+c->oy-camY; if(px<-8||px>=256||py<-8||py>=162) continue;   // (the bottom 24 pixels are the toolbar)
        const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a]; int fr=((tick>>3)+i)&1; if(!c->ox&&!c->oy) fr=0;
        oamSet(&oamMain,n,px,py,0,s->fam,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[A->shape][fr],-1,false,false,(i&1)&&c->dir,false,false); n++;
        if(sick[i]==1&&nc<108&&((tick>>2)&1)){ oamSet(&oamMain,nc,px+3,py-5,0,12,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[39][0],-1,false,false,false,false,false); nc++; }      // a flickering green mark over the sick
        if(c->rk&&nc<108){ oamSet(&oamMain,nc,px,py-6,0,c->rk==2?10:2,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[39][0],-1,false,false,false,false,false); nc++; } }
    for(int k=n;k<100;k++) oamSetHidden(&oamMain,k,true);
    for(int k=nc;k<108;k++) oamSetHidden(&oamMain,k,true);
    // weather: rain and snow as sprites falling across the touch screen
    int cnt=weather==1?14:weather==2?20:weather==3?16:0;
    if(!wptInit){ wptInit=1; for(int i=0;i<20;i++){ wpt[i].x=(short)(hsh(i,1,3)%256); wpt[i].y=(short)(hsh(i,2,3)%160); } }
    for(int i=0;i<20;i++){ if(i>=cnt||view3d){ oamSetHidden(&oamMain,108+i,true); continue; }
        if(weather==3){ wpt[i].y+=1+(i&1); wpt[i].x+=(short)(((tick>>3)+i)%3)-1; } else { wpt[i].y+=7; wpt[i].x-=2; } if(wpt[i].y>=160){ wpt[i].y=-8; wpt[i].x=(short)(rnd()%256); } if(wpt[i].x<0) wpt[i].x+=256; if(wpt[i].x>255) wpt[i].x-=256;
        oamSet(&oamMain,108+i,wpt[i].x,wpt[i].y,0,weather==3?14:6,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[weather==3?41:40][0],-1,false,false,false,false,false); }
}
static u8 mmCol[T_N];
static int page=0, titleOn=1, sceneOn=0;
static void drawMinimap(void){}   // (the minimap is part of the status screen now: see uiStatus)
static const char* const introTxt[]={"","  IN THE BEGINNING THERE WAS","  ONLY A QUIET, EMPTY PLANET.","","  THEN YOU WOKE.","","  YOUR HANDS MAKE LAND, RAIN","  AND FIRE. YOUR WORD MAKES LIFE.","","  TEACH THE FIRST FOLK TO LIVE.","","  BUT THE DARK IS PATIENT,","  AND IT HAS TEETH.","","  TOUCH THE WORLD TO BEGIN.",0};
static const char* const endTxt[]={"","  THE FOLK HAVE CITIES NOW,","  AND SONGS, AND WAR, AND PEACE.","","  YOU MET EVERY NIGHTMARE","  AND GAVE IT A NAME.","","  THEY DO NOT KNOW YOUR FACE.","  THEY KNOW THE RAIN,","  THE HARVEST, THE SPARK THAT","  LEAPS FROM THE SKY.","","  THE STORY IS TOLD.","","  THE WORLD IS YOURS, GODGUY.",0};
static const char* const credTxt[]={"","","       BORE: GODGUY","","  A SPINOFF OF BORE","","  DESIGN, CODE, MUSIC","  AND MONSTERS","  BY CLAUDE, FOR YOU","","  MUSIC AMALGAMATED FROM","  THE SONGS OF BORE","  (THE DIPPER MAN, AND ALL","  THE REST)","","  320 MONSTERS","  392 CREATURES","  364 ITEMS","  23 TRAITS","  134 TRACKS","","  THANK YOU FOR PLAYING","","","","",0};
static const char* const helpTxt[]={"TOUCH THE WORLD: USE POWER","L R      CHOOSE POWER","X Y      BRUSH SIZE","         (SPAWN: SPECIES)","         (GIFT: ITEM)","         (MUTATE: TRAIT)","B + X Y  JUMP TEN","D-PAD    PAN (B = FAST)","A        PAUSE","X + Y    3D GLOBE ON / OFF","  3D: TOUCH = USE POWER","  B + DRAG = SPIN, A = SHAPE","SELECT   NEXT INFO PAGE","START    PAUSE MENU","Y + A   WALK THE GLOBE (3D)","L+R+SELECT    SAVE NOW","","STORY: FINISH CHAPTERS TO","OPEN ITEMS FOR FREE PLAY.","100%: MEET AND SLAY ALL 320","MONSTERS AFTER THE STORY.","","TRAITS: CHILDREN INHERIT AND","SOMETIMES MUTATE. MUTATE","MAKES A SUBSPECIES ON PURPOSE.","","TOWERS SHOOT MONSTERS.","GIFTS DECIDE WHO WINS.",0};
static const char* const tipTxt[]={"TOUCH THE WORLD TO USE A POWER","L R CHANGES THE POWER","TRY SPAWN, THEN RAIN AND FIRE","START A VILLAGE: SETTLE","SELECT: STORY, BESTIARY, HELP","X+Y SHOWS THE WORLD AS A GLOBE","SNACKS MAY GIVE FOLK CHILL TRAITS","DAVE SAYS: HAVE YOU TRIED SITTING","FEED A MONSTER A NACHO. SEE WHAT HAPPENS","THE COUCH HAS ROOM FOR EVERYONE","MUNCHIES ARE NOT A BUG"};
static const char* const*sceneTxt, *const*sceneNext; static int menuOn, menuSel, menuAsk, toastT, dlgOn, dlgIdx, dlgChars, dlgPaused, sceneOff, sceneT, sceneScroll, endShown, scenePaused, helpSel, trPage, tipN;
static const Dl*dlgSet; static char lastInfo[64];
static const char* const pwDesc[32]={"LOOK AT A CREATURE OR A VILLAGE","PAINT SHALLOW WATER","PAINT DEEP SEA","PAINT SAND","PAINT GRASS","PLANT FOREST","PLANT JUNGLE","RAISE HILLS","RAISE MOUNTAINS","PAINT SNOW","PAINT DESERT","PAINT SWAMP","PAINT LAVA","CREATE LIFE  (X Y: SPECIES)","RAIN GREENS THE LAND","BURN WHAT YOU TOUCH","STRIKE ONE TILE DEAD","DROP A METEOR","KILL WHAT YOU TOUCH","HEAL CREATURES","FEED CREATURES","FOUND A VILLAGE","START A WAR","END ALL WARS","FILL A VILLAGE STORE","GIVE AN ITEM  (X Y: ITEM)","GIVE A TRAIT  (X Y: TRAIT)","SHAKE THE GROUND","FLOOD THE LAND","MAKE THE LAND BLOOM","CHILL A CREATURE OR VILLAGE","SPREAD A SICKNESS. HEAL CURES IT"};
static int sceneLines(const char*const*t){ int n=0; while(t[n]) n++; return n; }
static void sceneStart(const char*const*t,int scroll){ sceneTxt=t; sceneScroll=scroll; sceneOff=scroll?-20:0; sceneT=0; if(!sceneOn) scenePaused=paused; sceneOn=1; paused=1; }
static int dbgSel=0, dbgMon=0, dbgFx=0, fxSel=0, konami=0;
static int trackOpen(int i){ return i<GM_REMIX0||i>=GM_EPIC0||fullDone||debugOn||unlockAll; }
static int auralOK(void){ return auralOpen||debugOn||unlockAll; }
static int bestSel=0, juke=0, sdOk=0, savedYear=-1, hasSave=0;
#ifndef SAVE_PATH
#define SAVE_PATH "godguy.sav"
#endif
static const char*savePath=SAVE_PATH;
static void doSave(void){ if(!sdOk) { siprintf(info,"NO SD CARD: NOT SAVED"); return; } if(saveGame(savePath)){ savedYear=year; siprintf(info,"SAVED"); fxReq(FX_SAVE); } else siprintf(info,"SAVE FAILED"); }
static void setSize(int sz){ planetSize=sz; }
static void startWorld(int story){ storyOn=story; newWorld(); camX=(WW*8-256)/2; camY=(WH*8-192)/2; winX=-99; refillWindow(); titleOn=0; paused=0; endShown=storyDone; musReq(GM_NEWWORLD); if(story&&chapter==0&&stUse==0){ sceneStart(introTxt,0); dlgPend=0; } }
static void monName(int m,char*out){ int a=ARCH_A+m/16, fam=m%16; strcpy(out,advNm[fam]); strcat(out," "); strcat(out,arch[a].nm); }
static short plLon[109*109], plWy[109*109]; static u8 plJ[109*109]; static int plInit; static short plHalo[180][2];
static void uiPlanetSprite(int cx,int cy,int R){   // the planet as a lit ball (the sphere maths is done once for radius 54; any smaller radius samples it)
    const int B=54; if(!plInit){ plInit=1; for(int y=-B;y<=B;y++) for(int x=-B;x<=B;x++){ int k=(y+B)*109+(x+B); plJ[k]=255; if(x*x+y*y>B*B) continue; float nx=(float)x/B, ny=(float)y/B, z=sqrtf(1.f-nx*nx-ny*ny);
            float lon=atan2f(nx,z), lat=asinf(ny); plLon[k]=(short)((0.5f+lon/6.2831853f)*4096); plWy[k]=(short)((0.5f+lat/3.1415927f)*4096); float l=0.3f+0.7f*(-nx*0.45f-ny*0.45f+z*0.8f); plJ[k]=(u8)(l<0.5f?0:l<0.78f?1:2); }
        for(int a=0;a<180;a++){ float r=(float)B+1.5f; plHalo[a][0]=(short)(r*cosf(a*2*0.0174533f)); plHalo[a][1]=(short)(r*sinf(a*2*0.0174533f)); } }
    if(R<4) R=4; if(R>B) R=B; int shift=(tick>>5)*WW/600;
    for(int y=-R;y<=R;y++) for(int x=-R;x<=R;x++){ if(x*x+y*y>R*R) continue; int k=((y*B/R)+B)*109+((x*B/R)+B); if(plJ[k]==255) continue; int wx=((plLon[k]*WW>>12)+shift)%WW, wy=plWy[k]*WH>>12; if(wy<0) wy=0; if(wy>=WH) wy=WH-1; uiPx(cx+x,cy+y,100+world[wy][wx]*3+plJ[k]); }
    for(int a=0;a<180;a++) uiPx(cx+plHalo[a][0]*R/B,cy+plHalo[a][1]*R/B,U_CYAN); }
static void uiLoadHook(const char*msg,int pct){   // a loading screen: the planet grows as the world is made
    static int n; n++; uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=3) uiRect(0,y,256,1,U_PANEL); for(int k=0;k<60;k++){ int x=(int)(hsh(k,1,7)%256), y=(int)(hsh(k,2,7)%192); uiPx(x,y,(k&3)==0?U_WHITE:U_DIM); }
    uiPlanetSprite(128,76,8+pct*46/100); uiTextC(128,138,"BORE: GODGUY",U_GOLD,1); uiTextC(128,150,msg,U_WHITE,1);
    uiBar(48,162,160,9,pct,100,U_GREEN); { char b[8]; siprintf(b,"%d%%",pct); uiTextC(128,163,b,U_WHITE,1); }
    uiText(6,182,tipTxt[(year+pct/20+n)%11],U_DIM,1); uiFlush(); }
static int mScr, mSel; static u32 cSeed; static int cSel, cT=-1, cLand;
static const char*const cShapeN[]={"CONTINENTS","SUPERCONTINENT","ISLANDS"}, *const cSeaN[]={"DRY","LOW","NORMAL","HIGH","FLOODED"}, *const cClimN[]={"ICE AGE","COOL","TEMPERATE","WARM","SCORCHED"}, *const cWetN[]={"ARID","NORMAL","LUSH"}, *const cLifeN[]={"EMPTY","SPARSE","NORMAL","TEEMING"};
static void previewWorld(void){ gW=gH=planetSize; nCr=0; memset(occ,0,sizeof occ); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(vil,0,sizeof vil); nVilOn=0; genWorld(cSeed); camX=(WW*8-256)/2; camY=(WH*8-192)/2; winX=-99; refillWindow();
    int l=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(world[y][x]>T_WATER) l++; cLand=l*100/(WW*WH); cT=-1; }
static void createStart(void){ if(!cSeed) cSeed=(rnd()&0xFFFF)|1; cSel=0; previewWorld(); }
static void createInput(int kd){
    int d=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0); int ch=0;
    if(kd&KEY_UP) cSel=(cSel+9)%10; if(kd&KEY_DOWN) cSel=(cSel+1)%10;
    if(kd&KEY_B){ mScr=0; mSel=1; return; }
    if(cSel==9&&(kd&KEY_A)){ seedFix=cSeed; startWorld(storyOn); seedFix=0; return; }
    if(d||(kd&KEY_A)){ if(!d) d=1; switch(cSel){
        case 0: setSize(d>0?(planetSize==128?192:planetSize==192?256:128):(planetSize==256?192:planetSize==192?128:256)); ch=1; break;
        case 1: storyOn^=1; break;
        case 2: cShape=(cShape+3+d)%3; ch=1; break; case 3: cSea=(cSea+7+d)%5-2; ch=1; break; case 4: cClim=(cClim+7+d)%5-2; ch=1; break; case 5: cWet=(cWet+4+d)%3-1; ch=1; break;
        case 6: cLife=(cLife+4+d)%4; break; case 7: diff=(diff+3+d)%3; break;
        case 8: if(kd&KEY_A) cSeed=(rnd()&0xFFFF)|1; else cSeed=(cSeed+d)&0xFFFF; ch=1; break; } }
    if(kd&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A)) fxReq(FX_MENU_MOVE);
    if(ch) cT=8; }
static void uiCreate(void){
    uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=4) uiRect(0,y,256,1,U_PANEL);
    uiRect(0,0,256,15,U_PANELHI); uiText(6,4,"CREATE A PLANET",U_GOLD,1); uiRound(204,2,48,11,U_SEL,U_GOLD); uiTextC(228,4,"BACK",U_WHITE,1); uiBtn(202,0,52,15,KEY_B,0,0,0);
    char l[10][16], v[10][20]; const char*lb[10]={"SIZE","TYPE","LAND","SEA","CLIMATE","WETNESS","LIFE","DIFFICULTY","SEED","CREATE!"};
    siprintf(v[0],"%dx%d",planetSize,planetSize); strcpy(v[1],storyOn?"STORY":"FREE PLAY"); strcpy(v[2],cShapeN[cShape]); strcpy(v[3],cSeaN[cSea+2]); strcpy(v[4],cClimN[cClim+2]); strcpy(v[5],cWetN[cWet+1]); strcpy(v[6],cLifeN[cLife]); strcpy(v[7],diff==0?"CALM":diff==1?"NORMAL":"BRUTAL"); siprintf(v[8],"%04X  A:ROLL",(unsigned)cSeed); v[9][0]=0; (void)l;
    for(int i=0;i<10;i++){ int y=19+i*15, sel=i==cSel; uiRound(4,y,156,13,sel?U_SEL:U_PANELHI,sel?U_GOLD:U_BHI); uiText(10,y+2,lb[i],sel?U_WHITE:U_LGREY,1);
        uiBtn(4,y,156,13,i==9?KEY_A:0,0,&cSel,i); if(i<8){ uiBtn(90,y,34,13,KEY_LEFT,0,&cSel,i); uiBtn(124,y,36,13,KEY_RIGHT,0,&cSel,i); } if(i==8) uiBtn(124,y,36,13,KEY_A,0,&cSel,i);
        if(i==9){ uiTextC(82+30,y+2,"",U_GOLD,1); continue; }
        int vw=uiTextW(v[i],1); if(sel&&i!=8){ uiIcon8(154-vw-14,y+1,IC8_LEFT); uiIcon8(150,y+1,IC8_RIGHT); } uiTextR(148,y+2,v[i],sel?U_GOLD:U_DIM); }
    // the preview of the planet as it will be
    uiFrame(163,19,90,90,U_SHADOW,U_BLO,U_BHI);
    for(int y=0;y<88;y++) for(int x=0;x<88;x++){ int wx=x*WW/88, wy=y*WH/88; int t=world[wy][wx]; uiPx(164+x,20+y,100+t*3+((wx+wy)&1)); }
    char b[24]; siprintf(b,"LAND %d%%",cLand); uiTextC(208,113,b,U_WHITE,1); siprintf(b,"SEED %04X",(unsigned)cSeed); uiTextC(208,124,b,U_DIM,1);
    uiTextC(208,140,cT>=0?"SHAPING...":"READY",cT>=0?U_ORANGE:U_GREEN,1);
    uiText(6,172,cSel==9?"A: BEGIN YOUR WORLD":"PAD: UP DOWN  LEFT RIGHT CHANGE",U_DIM,1);
    uiFlush(); }
enum { MI_CONT, MI_STORY, MI_FREE, MI_OPT, MI_EXTRA, MI_BACK, MI_SIZE, MI_DIFF, MI_VOL, MI_MON, MI_SCULPT, MI_BEST, MI_CODEX, MI_HELP, MI_CRED, MI_DEBUG, MI_RESUME, MI_SAVE, MI_NEW, MI_MODE, MI_TITLE, MI_CREATE, MI_GO, MI_STYPE, MI_SPEED, MI_KING, MI_JOURNAL, MI_ACH, MI_CULT, MI_LANG, MI_BROWSE };
static int extrasView;      // which menu (0 main, 1 options, 2 extras), the highlighted row, a pending "are you sure", and whether an extras page is open
static int menuList(int*ids){ int n=0, ctx=titleOn?0:1;
    if(mScr==0){ if(ctx==0){ ids[n++]=MI_CONT; ids[n++]=MI_CREATE; } else { ids[n++]=MI_RESUME; ids[n++]=MI_SAVE; } ids[n++]=MI_OPT; ids[n++]=MI_EXTRA; if(ctx){ ids[n++]=MI_NEW; ids[n++]=MI_MODE; ids[n++]=MI_TITLE; } }
    else if(mScr==3){ ids[n++]=MI_SIZE; ids[n++]=MI_STYPE; ids[n++]=MI_DIFF; ids[n++]=MI_GO; ids[n++]=MI_BACK; }
    else if(mScr==1){ ids[n++]=MI_DIFF; ids[n++]=MI_SPEED; ids[n++]=MI_VOL; ids[n++]=MI_MON; ids[n++]=MI_BACK; }
    else { ids[n++]=MI_SCULPT; ids[n++]=MI_BROWSE; ids[n++]=MI_KING; ids[n++]=MI_CULT; ids[n++]=MI_LANG; ids[n++]=MI_JOURNAL; ids[n++]=MI_ACH; ids[n++]=MI_BEST; ids[n++]=MI_CODEX; ids[n++]=MI_HELP; ids[n++]=MI_CRED; if(debugOn) ids[n++]=MI_DEBUG; ids[n++]=MI_BACK; }
    return n; }
static void menuText(int id,char*l,char*v){ v[0]=0;
    switch(id){ case MI_CONT: strcpy(l,hasSave?"CONTINUE YOUR WORLD":"START THE STORY"); break; case MI_STORY: strcpy(l,"NEW STORY WORLD"); break; case MI_FREE: strcpy(l,"NEW FREE PLAY WORLD"); break;
        case MI_BROWSE: strcpy(l,"WORLD BROWSER"); break; case MI_KING: strcpy(l,"KINGDOMS"); break; case MI_CULT: strcpy(l,"CULTURE TREE"); break; case MI_LANG: strcpy(l,"LANGUAGES"); break; case MI_JOURNAL: strcpy(l,"JOURNAL"); siprintf(v,"%d",jrN); break; case MI_ACH: { strcpy(l,"ACHIEVEMENTS"); int c=0; for(int i=0;i<ACH_N;i++) c+=(achBits>>i)&1; siprintf(v,"%d/%d",c,ACH_N); } break; case MI_SPEED: strcpy(l,"GAME SPEED"); siprintf(v,"%dx",speedMul); break; case MI_CREATE: strcpy(l,"CREATE A PLANET"); strcpy(v,">"); break; case MI_GO: strcpy(l,"CREATE THE PLANET!"); break; case MI_STYPE: strcpy(l,"WORLD TYPE"); strcpy(v,storyOn?"STORY":"FREE PLAY"); break;
        case MI_OPT: strcpy(l,"OPTIONS"); strcpy(v,">"); break; case MI_EXTRA: strcpy(l,"EXTRAS"); strcpy(v,">"); break; case MI_BACK: strcpy(l,"BACK"); break;
        case MI_SIZE: strcpy(l,"PLANET SIZE"); siprintf(v,"%dx%d",planetSize,planetSize); break; case MI_DIFF: strcpy(l,"DIFFICULTY"); strcpy(v,diff==0?"CALM":diff==1?"NORMAL":"BRUTAL"); break;
        case MI_VOL: strcpy(l,"MUSIC VOLUME"); siprintf(v,"%d%%",gmMaster); break; case MI_MON: strcpy(l,"MONSTERS"); strcpy(v,monOn?"ON":"OFF"); break;
        case MI_SCULPT: strcpy(l,"AURAL SCULPTURES"); strcpy(v,auralOK()?"":"LOCKED"); break; case MI_BEST: strcpy(l,"BESTIARY"); siprintf(v,"%d/320",seenN); break; case MI_CODEX: strcpy(l,"SUBSPECIES"); siprintf(v,"%d/%d",__builtin_popcount(trSeen),TR_N-1); break;
        case MI_HELP: strcpy(l,"HELP"); break; case MI_CRED: strcpy(l,"CREDITS"); break; case MI_DEBUG: strcpy(l,"DEBUG MENU"); break;
        case MI_RESUME: strcpy(l,"RESUME"); break; case MI_SAVE: strcpy(l,"SAVE GAME"); break; case MI_NEW: strcpy(l,"NEW WORLD"); break; case MI_MODE: strcpy(l,"MODE"); strcpy(v,storyOn?"STORY":"FREE PLAY"); break; case MI_TITLE: strcpy(l,"TITLE SCREEN"); break; } }
static int menuAdj(int id){ return id==MI_SIZE||id==MI_STYPE||id==MI_SPEED||id==MI_DIFF||id==MI_VOL||id==MI_MON; }
static const u8 extOrder[11]={12,7,10,11,8,9,2,5,6,3,4};
static const char* extName(int pg){ switch(pg){ case 12: return "BROWSER"; case 2: return "BESTIARY"; case 3: return "AURAL"; case 4: return "DEBUG"; case 5: return "TRAITS"; case 6: return "HELP"; case 7: return "KINGDOMS"; case 8: return "JOURNAL"; case 9: return "GOALS"; case 10: return "CULTURE"; default: return "LANGUAGES"; } }
static int extOpen(int pg){ return !(pg==3&&!auralOK())&&!(pg==4&&!debugOn); }
static int extStep(int d){ int i=0; for(int k=0;k<11;k++) if(extOrder[k]==page) i=k; for(int n=0;n<11;n++){ i=(i+d+11)%11; if(extOpen(extOrder[i])) return extOrder[i]; } return page; }
static const char* menuDesc(int id){ switch(id){
    case MI_CONT: return hasSave?"PICK UP YOUR SAVED WORLD WHERE YOU LEFT IT":"BEGIN THE STORY ON A NEW PLANET"; case MI_CREATE: return "CHOOSE SIZE, LAND, CLIMATE AND SEED OF A NEW PLANET"; case MI_OPT: return "DIFFICULTY, SPEED, VOLUME, MONSTERS"; case MI_EXTRA: return "KINGDOMS, MUSIC, BESTIARY, HELP AND MORE";
    case MI_DIFF: return "HOW HARD THE MONSTERS BITE"; case MI_SPEED: return "HOW FAST TIME RUNS"; case MI_VOL: return "MUSIC AND EFFECT LOUDNESS"; case MI_MON: return "TURN THE NIGHTMARES ON OR OFF";
    case MI_SCULPT: return "THE JUKEBOX: ALL THE TRACKS AND SUITES"; case MI_BROWSE: return "FIND AND STAR CITIES, FOLK, ITEMS, POWERS"; case MI_KING: return "WHO RULES WHAT: WARS, ALLIES, VASSALS"; case MI_CULT: return "WHAT EACH KINGDOM HAS LEARNED"; case MI_LANG: return "THE FIVE TONGUES AND THEIR NAMES"; case MI_JOURNAL: return "THE LAST EVENTS OF THE WORLD"; case MI_ACH: return "THINGS YOU HAVE DONE AS A GOD"; case MI_BEST: return "EVERY NIGHTMARE YOU HAVE MET"; case MI_CODEX: return "SUBSPECIES AND WHAT THEY DO"; case MI_HELP: return "THE CONTROLS"; case MI_CRED: return "WHO MADE THIS"; case MI_DEBUG: return "CHEATS AND TOOLS";
    case MI_RESUME: return "BACK TO THE WORLD"; case MI_SAVE: return "WRITE THE WORLD TO THE CARD"; case MI_NEW: return "REPLACE THIS WORLD WITH A NEW ONE"; case MI_MODE: return "SWITCH BETWEEN STORY AND FREE PLAY"; case MI_TITLE: return "SAVE AND GO TO THE TITLE SCREEN"; case MI_SIZE: return "BIGGER PLANETS TAKE LONGER TO MAKE"; case MI_STYPE: return "STORY HAS CHAPTERS, FREE PLAY HAS NONE"; case MI_GO: return "MAKE THE PLANET AND BEGIN"; default: return ""; } }
static void uiMenuList(int x,int y,int w,int rh){
    int ids[14]; int n=menuList(ids); if(mSel>=n) mSel=n-1; char l[32], v[16]; if(y+n*rh>176) rh=(176-y)/n;
    for(int i=0;i<n;i++){ menuText(ids[i],l,v); int yy=y+i*rh, sel=i==mSel; uiRound(x,yy,w,rh-2,sel?U_SEL:U_PANELHI,sel?((tick>>3)&1?U_GOLD:U_YELLOW):U_BHI);
        if(sel) uiText(x+3,yy+(rh-9)/2,">",U_GOLD,1); uiText(x+(sel?11:8),yy+(rh-9)/2,l,sel?U_WHITE:U_LGREY,1);
        uiBtn(x,yy,w,rh-1,menuAdj(ids[i])?0:KEY_A,0,&mSel,i); if(menuAdj(ids[i])){ uiBtn(x+w-78,yy,30,rh-1,KEY_LEFT,0,&mSel,i); uiBtn(x+w-30,yy,30,rh-1,KEY_RIGHT,0,&mSel,i); }
        if(v[0]){ if(menuAdj(ids[i])&&sel){ uiIcon8(x+w-72,yy+(rh-10)/2,IC8_LEFT); uiIcon8(x+w-12,yy+(rh-10)/2,IC8_RIGHT); uiTextC(x+w-42,yy+(rh-9)/2,v,U_GOLD,1); } else uiTextR(x+w-8,yy+(rh-9)/2,v,sel?U_GOLD:U_DIM); } } }
static void uiTitle(void){
    if(mScr==3){ uiCreate(); return; }
    uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y++) if((y&3)==0) uiRect(0,y,256,1,U_PANEL);
    for(int k=0;k<70;k++){ int x=(int)(hsh(k,1,7)%256), y=(int)(hsh(k,2,7)%192); uiPx(x,y,(k&3)==0?U_WHITE:U_DIM); }
    uiPlanetSprite(204,128,planetSize==128?36:planetSize==192?42:48);
    if(mScr>0){ uiRound(200,166,52,12,U_SEL,U_GOLD); uiTextC(226,169,"BACK",U_WHITE,1); uiBtn(198,164,56,16,KEY_B,0,0,0); }
    if(mScr==2){ uiText(10,6,"MAIN MENU  >  EXTRAS",U_GOLD,1); uiMenuList(8,18,156,15); { int ids[14]; menuList(ids); uiFrame(168,18,84,56,U_PANEL,U_BHI,U_BLO); uiWrap(172,22,13,6,menuDesc(ids[mSel]),U_LGREY); } }
    else { uiTextS(12,8,"BORE",U_LGREY,3); for(int i=0;i<6;i++){ char g[2]={"GODGUY"[i],0}; int c=U_G0+i; uiTextS(12+i*28,30,g,c,4); }
    uiText(14,60,"A GOD GAME FOR THE TOUCH SCREEN",U_DIM,1);
    uiText(10,72,mScr==0?"MAIN MENU":"MAIN MENU  >  OPTIONS",U_GOLD,1);
    uiMenuList(8,83,148,15); { int ids[14]; menuList(ids); uiWrap(8,168,34,2,menuDesc(ids[mSel]),U_LGREY); } }
    uiFlush(); }
static void uiScene(void){
    uiBtn(0,0,256,192,KEY_A,0,0,0);
    int n=sceneLines(sceneTxt); uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=2) uiRect(0,y,256,1,U_PANEL);
    if(sceneScroll){ for(int r=0;r<24;r++){ int l=sceneOff+r; if(l>=0&&l<n){ int yy=r*8; int c=(yy<24||yy>160)?U_DIM:U_WHITE; if(sceneTxt[l][0]) uiTextC(128,yy,sceneTxt[l],c,1); } } uiText(6,184,"A SKIP",U_DIM,1); }
    else { uiFrame(14,10,228,160,U_PANEL,U_BHI,U_BLO); uiFrame(17,13,222,154,U_PANELHI,U_BLO,U_BHI); for(int l=0;l<n;l++) uiTextC(128,20+l*9,sceneTxt[l],U_WHITE,1); if((tick>>4)&1) uiTextC(128,160,"PRESS A",U_GOLD,1); }
    uiFlush(); }

static void uiBig(int x,int y,int idx,int sc){ for(int j=0;j<8;j++) for(int i=0;i<8;i++){ int c=icon8[idx][j*8+i]; if(c) uiRect(x+i*sc,y+j*sc,sc,sc,c); } }
static void uiInspectCard(void){
    uiFrame(138,15,116,132,U_PANEL,U_GOLD,U_BLO); char b[64], nm[48]; int y=19;
    if(inspKind==1&&inspIdx>=0&&inspIdx<nCr&&cr[inspIdx].alive){ Cr*c=&cr[inspIdx]; int fam=sp[c->sp].fam, shape=arch[sp[c->sp].a].shape;
        for(int k=0;k<3;k++) BG_PALETTE_SUB[200+k]=RGB15(famCol[fam][k][0],famCol[fam][k][1],famCol[fam][k][2]); BG_PALETTE_SUB[203]=RGB15(31,31,31);
        uiFrame(142,19,38,38,U_SHADOW,U_BHI,U_BLO); for(int j=0;j<8;j++) for(int i=0;i<8;i++){ char ch=shpRow(shape,(tick>>4)&1,j)[i]; int col=ch=='b'?200:ch=='d'?201:ch=='a'?202:ch=='e'?203:0; if(col) uiRect(144+i*4,21+j*4,4,4,col); }
        crName(c,nm); uiWrap(184,20,11,4,nm,U_WHITE); spName(c->sp,nm); (void)nm; y=62;
        uiText(142,y,"HEALTH",U_DIM,1); uiBar(142,y+9,108,7,c->hp,maxHp(c),c->hp*3<maxHp(c)?U_RED:U_GREEN); y+=19;
        uiText(142,y,"HUNGER",U_DIM,1); uiBar(142,y+9,108,7,c->food>200?200:c->food,200,U_ORANGE); y+=19;
        siprintf(b,"AGE %d   %s",c->age,arch[sp[c->sp].a].diet==5?"MONSTER":arch[sp[c->sp].a].diet==4?"FOLK":"BEAST"); uiText(142,y,b,U_LGREY,1); y+=10;
        if(sick[inspIdx]==1){ uiText(142,y,"SICK (PLAGUE)",U_GREEN,1); y+=10; } else if(sick[inspIdx]==2){ uiText(142,y,"IMMUNE NOW",U_CYAN,1); y+=10; }
        if(c->wep) uiText(142,y,it[c->wep-1].nm,U_YELLOW,1),y+=10; if(c->arm) uiText(142,y,it[c->arm-1].nm,U_CYAN,1),y+=10;
        if(c->tr){ uiText(142,y,trNm[c->tr],U_PURPLE,1); y+=10; uiWrap(142,y,18,2,trDesc[c->tr],U_DIM); y+=18; } if(c->tr2){ uiText(142,y,trNm[c->tr2],U_PURPLE,1); y+=10; } }
    else if(inspKind==2&&inspIdx>=0&&inspIdx<VIL_N&&vil[inspIdx].on){ Vil*V=&vil[inspIdx]; int v=inspIdx; char vn[24]; vilName(v,vn);
        uiFrame(142,19,38,38,U_SHADOW,U_BHI,U_BLO); uiBig(145,22,IC8_HOME,4); uiText(184,20,vn,U_WHITE,1); uiText(184,30,arch[V->race].nm,U_DIM,1); { static const char* const tier[4]={"","HAMLET","TOWN","CITY"}; int k=vilK[v]-1; uiText(184,40,(k>=0&&kd[k].cap==v+1)?"CAPITAL":tier[V->lvl>3?3:V->lvl],(k>=0&&kd[k].cap==v+1)?U_GOLD:U_LGREY,1); }
        y=62; if(vilK[v]){ int k=vilK[v]-1; char kn[24]; kdName(k,kn); uiRect(142,y,8,8,40+k); uiText(153,y,kn,U_WHITE,1); y+=11; uiText(142,y,"LOYALTY",U_DIM,1); int lo=vilL[v]<0?0:vilL[v]; uiBar(142,y+9,108,7,lo,100,vilL[v]<20?U_RED:U_GREEN); y+=19; }
        uiText(142,y,"STORE",U_DIM,1); uiBar(142,y+9,108,7,V->stock,400,U_YELLOW); y+=19;
        siprintf(b,"POP %d  HOMES %d",V->pop,V->houses); uiText(142,y,b,U_WHITE,1); y+=10; siprintf(b,"FARMS %d  TOWERS %d",V->farms,V->towers); uiText(142,y,b,U_WHITE,1); y+=10;
        if(V->foe&&vil[V->foe-1].on){ char fn[24]; vilName(V->foe-1,fn); siprintf(b,"AT WAR: %s",fn); uiText(142,y,b,U_RED,1); } }
    else { uiText(142,24,"NOTHING HERE",U_DIM,1); }
    uiText(142,136,"A: INSPECT AGAIN",U_DIM,1); }
static void uiStatus(void){
    int alive=0; for(int i=0;i<nCr;i++) alive+=cr[i].alive; int hh; int folk=vilTotals(&hh); char b[64];
    siprintf(b,"%s  YEAR %d",storyOn?"STORY":"FREE PLAY",year); uiWin(view3d?(walkOn?"BORE: GODGUY  WALKING":"BORE: GODGUY  GLOBE"):"BORE: GODGUY",b,0);
    { int p=(tick>>1)&2047; uiIcon8(238-uiTextW(b,1)-10,3,(p<256||p>1792)?IC8_MOON:IC8_SUN); if(paused) uiText(82,3,"PAUSED",U_RED,1); }
    uiFrame(2,15,132,132,U_SHADOW,U_BHI,U_BLO); uiMinimap(4,17);
    { int ids[8], m=0; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[m++]=k; if(m&&!view3d){ int rows=(m+1)/2, bh=rows*8+3, y0=17+128-bh-1; uiRect(5,y0,126,bh,U_SHADOW); uiRect(5,y0,126,1,U_BHI);   // the legend: kingdom colours, names, and a red mark for the ones at war
        for(int i=0;i<m;i++){ int k=ids[i], x=7+(i&1)*62, y=y0+2+(i>>1)*8; char nm[24]; kdName(k,nm); nm[6]=0; uiRect(x,y+1,5,5,40+k); uiText(x+7,y,nm,kd[k].empire?U_GOLD:U_LGREY,1); if(kd[k].war) uiText(x+48,y,"!",U_RED,1); } } }
    if(inspT>0){ uiInspectCard(); } else {
    uiFrame(138,15,116,86,U_PANEL,U_BHI,U_BLO);
    { struct { int ic; const char*l; int v; } r[6]={{IC8_LIFE,"LIFE",alive},{IC8_FOLK,"FOLK",folk},{IC8_HOME,"HOMES",hh},{IC8_FLAG,"VILLAGES",nVilOn},{IC8_SKULL,"MONSTERS",monCount()},{IC8_STAR,"SLAIN",(int)stKill}};
      for(int i=0;i<6;i++){ int y=19+i*13; uiIcon8(143,y,r[i].ic); uiText(155,y+1,r[i].l,U_DIM,1); siprintf(b,"%d",r[i].v); uiTextR(250,y+1,b,U_WHITE); } }
    uiFrame(138,104,116,44,U_PANEL,U_BHI,U_BLO);
    if(pw[power].kind==PK_SPAWN||pw[power].kind==PK_GIFT||pw[power].kind==PK_MUTATE){   // a picker: the choice before, the choice now, the choice after
        uiText(143,107,"X Y: CHOOSE",U_DIM,1); for(int k=-1;k<=1;k++){ char nm[40]; int ok=1;
            if(pw[power].kind==PK_SPAWN){ spName((spSel+nSp+k)%nSp,nm); } else if(pw[power].kind==PK_GIFT){ int j=k==0?itSel:itNext(itSel,k); itSay(nm,j); } else { int t=(trSel+TR_N+k)%TR_N; strcpy(nm,t?trNm[t]:"(CLEAR)"); (void)ok; }
            nm[17]=0; if(k==0) uiRect(140,116+(k+1)*10,112,10,U_SEL); uiText(143,118+(k+1)*10,nm,k==0?U_WHITE:U_DIM,1); } }
    else if(storyOn&&chapter<CHAP_N){ const Chap*C=&chap[chapter]; int v=chapVal(C->kind); if(v>(int)C->target) v=C->target; siprintf(b,"CHAPTER %d",chapter+1); uiText(143,108,b,U_GOLD,1); uiWrap(143,118,18,2,C->goal,U_WHITE); uiBar(143,137,106,9,v,C->target,U_GREEN); siprintf(b,"%d/%d",v,C->target); uiTextC(196,138,b,U_WHITE,1); }
    else if(storyOn){ uiText(143,108,"STORY TOLD",U_GOLD,1); uiText(143,120,"YOU ARE A GOD",U_WHITE,1); }
    else { int o; int tot=itCount(&o); uiText(143,108,"FREE PLAY",U_GOLD,1); siprintf(b,"ITEMS %d/%d",o,tot); uiText(143,120,b,U_WHITE,1); siprintf(b,"MET %d/320",seenN); uiText(143,132,b,U_DIM,1); }
    }
    uiFrame(2,150,252,28,U_PANEL,U_BHI,U_BLO); uiRect(5,153,14,14,U_SHADOW); uiPowerIcon(power,7,155);
    { char nm[40]; if(pw[power].kind==PK_SPAWN){ spName(spSel,nm); siprintf(b,"%s",nm); } else if(pw[power].kind==PK_GIFT){ itSay(nm,itSel); siprintf(b,"%.28s",nm); } else if(pw[power].kind==PK_MUTATE) siprintf(b,"TRAIT: %s",trSel?trNm[trSel]:"CLEAR"); else if(pw[power].kind==PK_PAINT) siprintf(b,"BRUSH %d",brush); else siprintf(b,"BRUSH %d",brush);
      uiText(24,154,pw[power].nm,U_GOLD,1); uiText(24+uiTextW(pw[power].nm,1)+6,154,b,U_WHITE,1); if(toastT>0&&info[0]) uiText(24,166,info,U_YELLOW,1); else uiText(24,166,pw[power].kind<PW_N?pwDesc[power]:"",U_DIM,1); }
    uiIcon8(4,182,IC8_NOTE); { char mn[24]=""; if(gmCur>=0){ strncpy(mn,gmSongs[gmCur].name,19); mn[19]=0; } uiText(14,183,mn,U_DIM,1); } { static const char* const wn[4]={"CLEAR","RAIN","STORM","SNOW"}; siprintf(b,"%s %s",ageN[worldAge()],wn[weather&3]); uiTextR(250,183,b,U_DIM); }
    uiFlush(); }
static int achSel, kingSel, jrSel;
static void uiAch(void){
    int n=0; for(int i=0;i<ACH_N;i++) n+=(achBits>>i)&1; char b[40]; siprintf(b,"%d/%d",n,ACH_N); uiWin("ACHIEVEMENTS",b,"UP DOWN SCROLL    START: BACK");
    int first=achSel<4?0:achSel-4; if(first>ACH_N-9) first=ACH_N-9; if(first<0) first=0;
    for(int r=0;r<9;r++){ int i=first+r; if(i>=ACH_N) break; int y=16+r*18, got=(achBits>>i)&1; uiBtn(4,y,232,17,0,0,&achSel,i); if(i==achSel) uiRect(4,y,242,17,U_SEL); else if(r&1) uiRect(4,y,242,17,U_PANEL);
        uiIcon8(8,y+4,got?IC8_STAR:IC8_SKULL); uiText(22,y+1,got?ach[i].nm:"???",got?U_GOLD:U_DIM,1); uiText(22,y+10,ach[i].ds,got?U_WHITE:U_LGREY,1); }
    uiScroll(ACH_N,first,9,16,162); uiFlush(); }
static void uiJournal(void){
    char b[16]; siprintf(b,"%d EVENTS",jrN); uiWin("JOURNAL",b,"UP DOWN SCROLL    START: BACK");
    if(!jrN) uiText(10,30,"NOTHING HAS HAPPENED YET.",U_DIM,1);
    for(int r=0;r<14;r++){ int i=jrN-1-jrSel-r; if(i<0) break; int y=17+r*11; if(r&1) uiRect(4,y-2,240,11,U_PANEL); siprintf(b,"Y%d",jrYear[i]); uiText(8,y,b,U_GOLD,1); uiText(40,y,jr[i],U_WHITE,1); }
    uiFlush(); }
static void uiKing(void){
    int n=0; for(int k=0;k<KN_N;k++) n+=kd[k].on; char b[64]; siprintf(b,"%d KINGDOMS",n); uiWin("KINGDOMS",b,"UP DOWN CHOOSE    START: BACK");
    if(!n){ uiText(10,30,"NO KINGDOMS YET. FOUND VILLAGES",U_DIM,1); uiText(10,40,"AND LET THEM GROW.",U_DIM,1); uiFlush(); return; }
    int ids[KN_N], m=0; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[m++]=k; if(kingSel>=m) kingSel=m-1; if(kingSel<0) kingSel=0;
    for(int r=0;r<m;r++){ int k=ids[r], c,p,w; kdStats(k,&c,&p,&w); char nm[24]; kdName(k,nm); int y=16+r*11; uiBtn(4,y-1,232,11,0,0,&kingSel,r); if(r==kingSel) uiRect(4,y-1,246,11,U_SEL); uiRect(7,y+1,6,6,40+k); uiText(17,y,nm,r==kingSel?U_WHITE:U_LGREY,1);
        uiText(70,y,kd[k].empire?"EMPIRE":kd[k].lord?"VASSAL":"KINGDOM",kd[k].empire?U_GOLD:U_DIM,1); siprintf(b,"%dC %dP W%d",c,p,w); uiTextR(246,y,b,kd[k].war?U_RED:U_WHITE); }
    int k=ids[kingSel], c,p,w; kdStats(k,&c,&p,&w); char nm[24], rn[40]; kdName(k,nm); kdRuler(k,rn); int y0=16+m*11+4;
    uiFrame(4,y0,248,177-y0,U_PANELHI,40+k,U_BLO);
    char L[9][64]; int col[9], pri[9], nl=0; char cp[24]="NONE"; if(kd[k].cap) vilName(kd[k].cap-1,cp);
    siprintf(L[nl],"%s %s",kd[k].empire?"EMPEROR":"RULER",rn); col[nl]=U_WHITE; pri[nl++]=0;
    siprintf(L[nl],"CITIES %d/%d  CAPITAL %s",c,kdMaxCities(k),cp); col[nl]=c>kdMaxCities(k)?U_ORANGE:U_WHITE; pri[nl++]=1;
    { char s2[64]="WAR: "; int any=0; for(int o=0;o<KN_N;o++) if(kd[k].war>>o&1){ char z[24]; kdName(o,z); if(strlen(s2)<30){ strcat(s2,z); strcat(s2," "); any=1; } } if(!any) strcpy(s2,kd[k].plot?"PLOTTING WAR":"AT PEACE"); strcpy(L[nl],s2); col[nl]=any?U_RED:kd[k].plot?U_ORANGE:U_GREEN; pri[nl++]=2; }
    { char s2[64]="ALLIES: "; int any=0; for(int o=0;o<KN_N;o++) if(kd[k].allies>>o&1){ char z[24]; kdName(o,z); if(strlen(s2)<26){ strcat(s2,z); strcat(s2," "); any=1; } } if(kd[k].lord){ char z[24]; kdName(kd[k].lord-1,z); siprintf(s2,"VASSAL OF %s",z); any=1; } if(!any) strcpy(s2,"NO ALLIES"); strcpy(L[nl],s2); col[nl]=U_CYAN; pri[nl++]=3; }
    siprintf(L[nl],"%s  %s  %s",arch[kd[k].race].nm,culN[kd[k].cul],rlgN[kd[k].rlg]); col[nl]=U_DIM; pri[nl++]=4;
    siprintf(L[nl],"HOUSE %s  %s",clanN[kd[k].clan],kd[k].lord?"":"INDEPENDENT"); col[nl]=U_DIM; pri[nl++]=6;
    siprintf(L[nl],"TECH %d %s  %s",kd[k].tech,techN[kd[k].tech],langN[kd[k].lang]); col[nl]=U_DIM; pri[nl++]=5;
    { char t2[48]=""; for(int q=0;q<6;q++) if(kd[k].ctr>>q&1){ if(strlen(t2)+strlen(ctrN[q])<34){ strcat(t2,ctrN[q]); strcat(t2," "); } } if(t2[0]){ strcpy(L[nl],t2); col[nl]=U_PURPLE; pri[nl++]=7; } }
    siprintf(L[nl],"GOLD %d  POWER %d",kd[k].gold,w); col[nl]=U_GOLD; pri[nl++]=8;
    { int fit=(177-y0-5)/10; int show[9]; for(int i=0;i<nl;i++) show[i]=1; int cnt=nl; while(cnt>fit&&cnt>1){ int bi=-1; for(int i=0;i<nl;i++) if(show[i]&&(bi<0||pri[i]>pri[bi])) bi=i; show[bi]=0; cnt--; }
      int y=y0+4; for(int i=0;i<nl;i++) if(show[i]){ uiText(10,y,L[i],col[i],1); y+=10; } }
    uiFlush(); }
static int langSel;
static void uiCult(void){
    int n=0; for(int k=0;k<KN_N;k++) n+=kd[k].on; uiWin("CULTURE TREE","","UP DOWN CHOOSE    START: BACK");
    if(!n){ uiText(10,30,"NO KINGDOMS YET.",U_DIM,1); uiFlush(); return; }
    int ids[KN_N], m=0; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[m++]=k; if(kingSel>=m) kingSel=m-1; if(kingSel<0) kingSel=0; int k=ids[kingSel]; char nm[24], f[24], b[64]; kdName(k,nm); kdFaith(k,f);
    uiRect(7,17,8,8,40+k); siprintf(b,"%s  (%d/%d)",nm,kingSel+1,m); uiText(20,17,b,U_WHITE,1); siprintf(b,"POINTS %d",kd[k].cpts); uiTextR(246,17,b,U_GOLD);
    for(int br=0;br<6;br++){ int y=28+br*17, lv=kdKnow(k,br); uiRect(4,y,248,17,br&1?U_PANEL:U_PANELHI); uiText(8,y+1,knBr[br],lv?U_WHITE:U_DIM,1);
        for(int i=0;i<3;i++){ int on=(kd[k].know>>(br*3+i))&1; uiRect(96+i*12,y+2,9,9,on?U_GOLD:U_SHADOW); uiRect(97+i*12,y+3,7,7,on?U_YELLOW:U_BG); }
        { static const char* const fx[6]={"MORE CITIES","MORE ZONES","STRONGER ARMY","MORE GOLD","RICHER CITIES","FASTER LEARNING"}; uiText(8,y+9,fx[br],lv?U_GREEN:U_DGREY,1); }
        if(lv) uiTextR(246,y+1,knN[br][lv-1],U_CYAN); }
    uiFrame(4,133,248,44,U_PANELHI,U_BHI,U_BLO); siprintf(b,"FAITH %s",rlgN[kd[k].rlg]); uiText(10,137,b,U_GOLD,1); siprintf(b,"CALLED %s",f); uiText(10,147,b,U_WHITE,1); uiText(10,157,dogN[kd[k].rlg],U_CYAN,1);
    { char t2[48]=""; for(int q=0;q<6;q++) if(kd[k].ctr>>q&1){ if(strlen(t2)+strlen(ctrN[q])<34){ strcat(t2,ctrN[q]); strcat(t2," "); } } uiText(10,167,t2[0]?t2:"NO CULTURE TRAITS YET",t2[0]?U_PURPLE:U_DIM,1); }
    uiFlush(); }
static void uiLang(void){
    uiWin("LANGUAGES","","UP DOWN CHOOSE    START: BACK"); int l=langSel; char b[64], t[32];
    uiRect(4,16,248,14,U_PANELHI); uiText(8,19,langN[l],U_GOLD,1); { char s2[48]=""; int any=0; for(int k=0;k<KN_N;k++) if(kd[k].on&&kd[k].lang==l){ char z[24]; kdName(k,z); if(strlen(s2)+strlen(z)<22){ strcat(s2,z); strcat(s2," "); any=1; } } siprintf(b,"SPOKEN: %s",any?s2:"NOBODY NOW"); uiTextR(246,19,b,U_DIM); }
    for(int i=0;i<16;i++){ int col=i/8, row=i%8; int x=8+col*124, y=36+row*11; uiText(x,y,cptN[i],U_DIM,1); uiText(x+54,y,lwords[l][i],U_WHITE,1); }
    uiFrame(4,128,248,49,U_PANELHI,U_BHI,U_BLO); uiText(10,132,"HOW ITS NAMES ARE MADE",U_GOLD,1);
    langKing(l,2,1,t); siprintf(b,"KINGDOM  %s",t); uiText(10,142,b,U_WHITE,1); langCity(l,5,3,t); siprintf(b,"CITY     %s",t); uiText(10,152,b,U_WHITE,1);
    langPerson(l,3,2,t); siprintf(b,"PERSON   %s %s",lwords[l][8],t); uiText(10,162,b,U_WHITE,1); siprintf(b,"FAITH    %s-%s",lwords[l][13],lwords[l][7]); uiText(130,142,b,U_CYAN,1);
    uiFlush(); }

// ---------------------------------------------------------------------------------------------- the world browser: look at (almost) anything, sort it by kind, star favourites and jump to them
static int brCat, brSel, brKind;
typedef struct { u8 type; u16 id; } BrE;      // type: 1 city, 2 kingdom, 3 item, 4 power, 5 person, 6 monster
static BrE brL[160]; static int brN;
static const char* const brCatN[7]={"STARS","KINGS","CITY","FOLK","MOBS","ITEMS","POWER"};
static int crNotable(const Cr*c){ return c->rk*4+(c->tr?2:0)+(c->tr2?1:0)+(c->wep?1:0); }
static void brBuild(void){
    brN=0;
    #define BRADD(t,i) do{ if(brN<160){ brL[brN].type=(u8)(t); brL[brN].id=(u16)(i); brN++; } }while(0)
    switch(brCat){
        case 0: for(int v=0;v<VIL_N;v++) if(favVil[v]&&vil[v].on) BRADD(1,v); for(int k=0;k<KN_N;k++) if(favKd[k]&&kd[k].on) BRADD(2,k); for(int i=0;i<nIt;i++) if(favItem[i]) BRADD(3,i); for(int i=0;i<PW_N;i++) if(favPow[i]) BRADD(4,i); break;
        case 1: for(int k=0;k<KN_N;k++) if(kd[k].on) BRADD(2,k); break;
        case 2: { int used[VIL_N]; memset(used,0,sizeof used); for(int n=0;n<VIL_N;n++){ int bp=-1,bv=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on&&!used[v]&&vil[v].pop>bp){ bp=vil[v].pop; bv=v; } if(bv<0) break; used[bv]=1; BRADD(1,bv); } } break;
        case 3: case 4: { int want=brCat==3?4:5; for(int pass=3;pass>=0&&brN<150;pass--) for(int i=0;i<nCr&&brN<150;i++){ const Cr*c=&cr[i]; if(!c->alive||arch[sp[c->sp].a].diet!=want) continue; int nt=crNotable(c); int bucket=nt>=8?3:nt>=4?2:nt>=1?1:0; if(bucket!=pass) continue; if(pass==0&&brN>40) continue; BRADD(brCat==3?5:6,i); } } break;
        case 5: for(int i=0;i<nIt&&brN<150;i++) if(itOpen(i)&&it[i].kind==brKind) BRADD(3,i); break;
        default: for(int i=0;i<PW_N;i++) BRADD(4,i); break; }
    if(brSel>=brN) brSel=brN?brN-1:0; }
static u8*brFav(const BrE*e){ switch(e->type){ case 1: return &favVil[e->id]; case 2: return &favKd[e->id]; case 3: return &favItem[e->id]; case 4: return &favPow[e->id]; default: return 0; } }
static void brText(const BrE*e,char*l,char*r){ l[0]=r[0]=0; switch(e->type){
    case 1: { vilName(e->id,l); int k=vilK[e->id]-1; char kn[24]=""; if(k>=0) kdName(k,kn); siprintf(r,"%s %dP",kn,vil[e->id].pop); } break;
    case 2: { kdName(e->id,l); int c,p,w; kdStats(e->id,&c,&p,&w); siprintf(r,"%dC %dP W%d",c,p,w); } break;
    case 3: itSay(l,e->id); l[26]=0; break; case 4: strcpy(l,pw[e->id].nm); break;
    default: { const Cr*c=&cr[e->id]; crName(c,l); l[24]=0; siprintf(r,"HP %d",c->hp); } } }
static void brGo(const BrE*e){   // jump to it
    int x=-1,y=-1; switch(e->type){ case 1: x=vil[e->id].x; y=vil[e->id].y; inspKind=2; inspIdx=e->id; inspT=480; break;
        case 2: { int cp=kd[e->id].cap?kd[e->id].cap-1:-1; if(cp>=0){ x=vil[cp].x; y=vil[cp].y; inspKind=2; inspIdx=cp; inspT=480; } } break;
        case 3: itSel=e->id; for(int i=0;i<PW_N;i++) if(pw[i].kind==PK_GIFT) power=i; break; case 4: power=e->id; break;
        default: if(e->id<nCr&&cr[e->id].alive){ x=cr[e->id].x; y=cr[e->id].y; inspKind=1; inspIdx=e->id; inspT=480; } }
    if(x>=0){ camX=x*8-128; camY=y*8-96; if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192; winX=-99; }
    extrasView=0; page=0; if(!titleOn){ menuOn=0; paused=dlgPaused; } }
static void uiBrowser(void){
    brBuild(); uiWin("WORLD BROWSER","","");
    for(int c=0;c<7;c++){ int x=4+c*36, sel=c==brCat; uiRound(x,15,35,13,sel?U_SEL:U_PANELHI,sel?U_GOLD:U_BHI); uiTextC(x+17,18,brCatN[c],sel?U_WHITE:U_DIM,1); uiBtn(x,15,35,13,0,0,&brCat,c); }
    if(brCat==5){ static const char* const kn[8]={"WEAPONS","ARMOUR","POTIONS","SEEDS","FOOD","RELICS","GEMS","TOOLS"}; char b[32]; siprintf(b,"< %s >  Y: NEXT KIND",kn[brKind]); uiText(8,32,b,U_CYAN,1); uiBtn(4,30,160,11,KEY_Y,0,0,0); }
    else { char b[40]; siprintf(b,"%d %s",brN,brCat==0?"STARRED":"LISTED"); uiText(8,32,b,U_DIM,1); }
    int first=brSel<5?0:brSel-4; if(first>brN-9) first=brN-9; if(first<0) first=0;
    if(!brN) uiText(10,60,brCat==0?"STAR THINGS WITH X TO KEEP THEM HERE":"NOTHING TO SHOW YET",U_DIM,1);
    for(int r=0;r<9;r++){ int i=first+r; if(i>=brN) break; int y=43+r*12; const BrE*e=&brL[i]; char l[40], rr[40]; brText(e,l,rr); uiBtn(4,y-1,232,12,0,KEY_A,&brSel,i); if(i==brSel) uiRect(4,y-1,232,12,U_SEL); else if(r&1) uiRect(4,y-1,232,12,U_PANEL);
        static const u8 ty[7]={0,IC8_HOME,IC8_FLAG,IC8_STAR,IC8_SUN,IC8_FOLK,IC8_SKULL}; uiIcon8(7,y,ty[e->type]); uiText(19,y,l,i==brSel?U_WHITE:U_LGREY,1); uiTextR(228,y,rr,U_DIM); const u8*f=brFav(e); if(f&&*f) uiIcon8(229,y,IC8_STAR); }
    uiScroll(brN,first,9,43,108);
    uiRound(110,153,40,12,U_SEL,U_GOLD); uiTextC(130,156,"A: GO",U_WHITE,1); uiBtn(108,151,44,16,KEY_A,0,0,0); uiRound(158,153,52,12,U_SEL,U_GOLD); uiTextC(184,156,"X: STAR",U_YELLOW,1); uiBtn(156,151,56,16,KEY_X,0,0,0);
    uiText(6,156,"< > KIND",U_DIM,1);
    uiFlush(); }
static void uiHelp(void){ uiWin("HELP","UP DOWN SCROLL","SELECT NEXT PAGE"); int n=sceneLines(helpTxt); for(int r=0;r<15;r++){ int l=helpSel+r; if(l>=n) break; uiText(10,18+r*10,helpTxt[l],helpTxt[l][0]&&helpTxt[l][0]!=' '&&l<n&&helpTxt[l][1]!=' '&&!(helpTxt[l][0]=='B'&&helpTxt[l][1]==' ')?U_WHITE:U_DIM,1); } uiScroll(n,helpSel,15,16,160); uiFlush(); }
static void uiStory(void){
    char b[64]; uiWin("STORY",storyOn?(storyDone?"FINISHED":"STORY MODE"):"FREE PLAY: PAUSED","SELECT NEXT PAGE");
    if(chapter<CHAP_N){ const Chap*C=&chap[chapter]; int v=chapVal(C->kind); if(v>(int)C->target) v=C->target; uiFrame(4,16,248,34,U_PANELHI,U_GOLD,U_BLO); siprintf(b,"CHAPTER %d: %s",chapter+1,C->nm); uiText(10,20,b,U_GOLD,1); uiText(10,30,C->goal,U_WHITE,1); uiBar(10,40,200,7,v,C->target,U_GREEN); siprintf(b,"%d/%d",v,C->target); uiText(216,40,b,U_WHITE,1); }
    else { uiFrame(4,16,248,34,U_PANELHI,U_GOLD,U_BLO); uiText(10,28,"THE STORY IS TOLD. YOU ARE A GOD.",U_GOLD,1); }
    for(int c=0;c<CHAP_N;c++){ int y=52+c*8; int done=c<chapter; uiRect(4,y-1,248,8,c&1?U_PANEL:U_BG); uiRect(8,y,7,7,done?U_GREEN:U_SHADOW); if(done){ uiPx(10,y+3,U_WHITE); uiPx(11,y+4,U_WHITE); uiPx(12,y+2,U_WHITE); }
        siprintf(b,"%2d",c+1); uiText(20,y,b,U_DIM,1); uiText(36,y,done?chap[c].nm:"??????????",done?U_WHITE:U_DIM,1); }
    { int o; int tot=itCount(&o); siprintf(b,"ITEMS %d/%d  MET %d/320  SLAIN %u",o,tot,seenN,(unsigned)stKill); uiText(6,172,b,U_DIM,1); }
    uiFlush(); }
static void uiBest(void){
    char b[64]; siprintf(b,"%d/320 MET",seenN); uiWin("BESTIARY",b,"UP DOWN  LEFT RIGHT +10   * = SLAIN"); int first=bestSel-7; if(first<0) first=0; if(first>320-15) first=320-15;
    for(int r=0;r<15;r++){ int m=first+r; int s_=8+(ARCH_A+m/16-8)*16+m%16; int seen=(seenB[s_>>3]>>(s_&7))&1, sl=(slainB[s_>>3]>>(s_&7))&1; char n2[40]; if(seen) monName(m,n2); else strcpy(n2,"??????????"); char nn[8]; siprintf(nn,"%3d",m+1);
        int y=17+r*10; if(m==bestSel) uiRect(4,y-1,240,10,U_SEL); else if(r&1) uiRect(4,y-1,240,10,U_PANEL); uiText(8,y,nn,U_DIM,1); uiText(32,y,n2,seen?U_WHITE:U_DIM,1); if(sl) uiIcon8(228,y,IC8_STAR); else if(seen) uiIcon8(228,y,IC8_SKULL); }
    uiScroll(320,first,15,16,150); uiFlush(); }
static void uiCodex(void){
    countTraits(); char b[64]; siprintf(b,"%d/%d MET  MUT %u",__builtin_popcount(trSeen),TR_N-1,(unsigned)stMut); uiWin("SUBSPECIES",b,"UP DOWN   MUTATE: X Y PICKS A TRAIT");
    int first=trPage-6; if(first<1) first=1; if(first>TR_N-1-11) first=TR_N-1-11; if(first<1) first=1;
    for(int r=0;r<12;r++){ int t=first+r; if(t>=TR_N) break; int met=(trSeen>>t)&1||debugOn||unlockAll; int y=17+r*10; if(t==trPage) uiRect(4,y-1,240,10,U_SEL); else if(r&1) uiRect(4,y-1,240,10,U_PANEL); siprintf(b,"%d",trCount[t]); uiText(8,y,met?trNm[t]:"??????????",met?U_WHITE:U_DIM,1); uiTextR(242,y,b,met?U_GOLD:U_DIM); }
    uiFrame(4,142,248,34,U_PANELHI,U_BHI,U_BLO); { int met=(trSeen>>trPage)&1||debugOn||unlockAll; uiText(10,146,met?trNm[trPage]:"UNKNOWN",U_GOLD,1); uiWrap(10,157,40,2,met?trDesc[trPage]:"MUTATE A CREATURE TO DISCOVER IT",U_WHITE); }
    uiScroll(TR_N-1,first-1,12,16,120); uiFlush(); }
static void uiAural(void){
    uiWin("AURAL SCULPTURES",0,"PAD TRACK A PLAY B AUTO X Y SFX L R VOL"); char b[64]; siprintf(b,"%d TRACKS",GM_SONGS); uiTextR(250,3,b,U_DIM);
    int first=juke-5; if(first<0) first=0; if(first>GM_SONGS-11) first=GM_SONGS-11;
    for(int r=0;r<11;r++){ int m=first+r; if(m>=GM_SONGS) break; int y=17+r*10; int op=trackOpen(m); char nn[8]; siprintf(nn,"%3d",m+1); if(m==juke) uiRect(4,y-1,240,10,U_SEL); else if(r&1) uiRect(4,y-1,240,10,U_PANEL); uiText(8,y,nn,U_DIM,1);
        uiText(32,y,op?gmSongs[m].name:"?? LOCKED: 100% ??",op?(m>=GM_EPIC0?U_GOLD:m>=GM_REMIX0?U_PINK:U_WHITE):U_DIM,1); if(gmCur==m) uiIcon8(228,y,IC8_NOTE); }
    uiScroll(GM_SONGS,first,11,16,110);
    uiFrame(4,130,248,48,U_PANELHI,U_BHI,U_BLO);
    uiText(10,134,trackOpen(juke)?gmSongs[juke].use:"FINISH THE GAME 100%",U_DIM,1);
    if(gmCur>=0){ int sec=gmRow*15/gmSongs[gmCur].bpm, tot=gmSongs[gmCur].rows*15/gmSongs[gmCur].bpm; siprintf(b,"NOW  %.26s",gmSongs[gmCur].name); uiText(10,146,b,U_GOLD,1); siprintf(b,"%d:%02d / %d:%02d  %d BPM",sec/60,sec%60,tot/60,tot%60,gmSongs[gmCur].bpm); uiText(10,156,b,U_WHITE,1); uiBar(10,167,150,6,sec,tot>0?tot:1,U_CYAN); }
    siprintf(b,"SFX %d %.9s",fxSel+1,gmFx[fxSel].name); uiText(166,146,b,U_DIM,1); siprintf(b,"VOL %d%%",gmMaster); uiText(166,156,b,U_DIM,1); uiText(166,167,jukeOn?"SCULPTURE":"AUTO",U_DIM,1);
    uiFlush(); }
static void uiDebug(void){
    static const char*const dn[24]={"SIM SPEED","FREEZE SIM","YEAR","CHAPTER","ITEM TIER","UNLOCK ALL ITEMS","MONSTERS ON","MONSTER KIND","SPAWN THE KIND HERE","KILL ALL MONSTERS","MEET ALL MONSTERS","HEAL + FEED ALL","FILL VILLAGE STORES","FOUND VILLAGE HERE","MARK ALL SLAIN","FINISH CHAPTER NOW","SAVE / LOAD / ERASE","PLAY SFX","OPEN AURAL SCULPT.","SET 100% DONE","WORLD SEED","NEW WORLD, THIS SEED","MUTATE 50 CREATURES","DIFFICULTY"};
    uiWin("DEBUG","L R ADJUST  A: DO",0); { int al=0; for(int i=0;i<nCr;i++) al+=cr[i].alive; char b[64]; siprintf(b,"CR %d  MON %d  TICK %d  SEED %04X",al,monCount(),tick,(unsigned)(worldSeed&0xFFFF)); uiText(6,183,b,U_DIM,1); }
    int st0=dbgSel-7; if(st0<0) st0=0; if(st0>24-15) st0=24-15;
    for(int r=st0;r<st0+15;r++){ char v[28]; v[0]=0; switch(r){ case 0: siprintf(v,"x%d",speedMul); break; case 1: siprintf(v,"%s",paused?"FROZEN":"RUNNING"); break; case 2: siprintf(v,"%d",year); break; case 3: siprintf(v,"%d/%d",chapter,CHAP_N); break; case 4: siprintf(v,"%d/14",tierOpen); break; case 5: siprintf(v,"%s",unlockAll?"ON":"OFF"); break;
            case 6: siprintf(v,"%s",monOn?"ON":"OFF"); break; case 7: { char n2[32]; monName(dbgMon,n2); siprintf(v,"%.22s",n2); } break; case 16: siprintf(v,"%s",sdOk?"SD OK":"NO SD"); break; case 17: siprintf(v,"%d %.14s",dbgFx+1,gmFx[dbgFx].name); break; case 18: siprintf(v,"%s",auralOpen?"OPEN":"LOCKED"); break; case 19: siprintf(v,"%s",fullDone?"YES":"NO"); break;
            case 20: siprintf(v,"%04X",(unsigned)((seedFix?seedFix:worldSeed)&0xFFFF)); break; case 23: siprintf(v,"%s",diff==0?"CALM":diff==1?"NORMAL":"BRUTAL"); break; }
        uiListRow(17+(r-st0)*10,r==dbgSel,r-st0,dn[r],v,U_WHITE); }
    uiScroll(24,st0,15,16,150); uiFlush(); }
static void uiDialog(void){
    uiBtn(0,0,256,192,KEY_A,0,0,0);
    const Dl*d=&dlgSet[dlgIdx]; uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=2) uiRect(0,y,256,1,U_PANEL);
    uiFrame(8,10,66,66,U_SHADOW,U_GOLD,U_BLO); { int fam=whoFam[d->who]; for(int k=0;k<3;k++) BG_PALETTE_SUB[200+k]=RGB15(famCol[fam][k][0],famCol[fam][k][1],famCol[fam][k][2]); BG_PALETTE_SUB[203]=RGB15(31,31,31);
        for(int y=0;y<8;y++) for(int x=0;x<8;x++){ char ch=shpRow(whoShape[d->who],((tick>>4)&1),y)[x]; int c=ch=='b'?200:ch=='d'?201:ch=='a'?202:ch=='e'?203:0; if(c) uiRect(13+x*7,15+y*7,7,7,c); } }
    int len=(int)strlen(whoNm[d->who]); uiTextS(84,18,whoNm[d->who],U_GOLD,len>9?1:2); uiRect(84,38,160,1,U_BHI); { char b[32]; int n=0; while(dlgSet[n].txt) n++; siprintf(b,"%d/%d",dlgIdx+1,n); uiTextR(246,28,b,U_DIM); }
    uiFrame(8,84,240,92,U_PANEL,U_BHI,U_BLO); char buf[160]; int tl=(int)strlen(d->txt); int cc=dlgChars<tl?dlgChars:tl; memcpy(buf,d->txt,cc); buf[cc]=0; uiWrap(14,90,37,9,buf,U_WHITE);
    if(dlgChars>=tl&&((tick>>4)&1)) uiText(222,164,"A >",U_GOLD,1); else if(dlgChars<tl) uiText(196,164,"A: HURRY  B: SKIP",U_DIM,1);
    uiFlush(); }
static void uiMenu(void){
    uiStatus(); for(int y=0;y<192;y++) for(int x=(y&1);x<256;x+=2) uiPx(x,y,U_SHADOW);
    uiRect(30,12,196,168,U_SHADOW); uiFrame(32,14,192,164,U_PANEL,U_GOLD,U_BLO); uiRect(33,15,190,16,U_PANELHI); uiText(42,19,mScr==0?"PAUSED":mScr==1?"PAUSED  >  OPTIONS":"PAUSED  >  EXTRAS",U_GOLD,1); if(mScr==0) uiTextR(214,19,"START: RESUME",U_DIM);
    uiMenuList(38,36,180,16);
    if(mScr>0){ uiRound(176,162,44,12,U_SEL,U_GOLD); uiTextC(198,165,"BACK",U_WHITE,1); uiBtn(174,160,48,16,KEY_B,0,0,0); }
    if(menuAsk){ uiRect(40,154,176,20,U_RED); uiTextC(128,160,"ARE YOU SURE?   A YES   B NO",U_WHITE,1); uiBtn(40,154,88,20,KEY_A,0,0,0); uiBtn(128,154,88,20,KEY_B,0,0,0); } else if(mScr<2){ int ids[14]; menuList(ids); uiWrap(40,152,24,3,menuDesc(ids[mSel]),U_LGREY); }
    uiFlush(); }
static int uiScrId=-1, uiScrDepth, uiTabDir;
static void hud(void){
    uiNBtn=0;
    { int id=(dlgOn?1:0)|(sceneOn?2:0)|(titleOn?4:0)|(menuOn?8:0)|(extrasView?16:0)|(mScr<<5)|(page<<8); int dep=(dlgOn||sceneOn)?3:extrasView?2:(titleOn||menuOn)?(mScr>0?1:0):(page?1:0);
      if(uiScrId>=0&&id!=uiScrId){ uiTrans=uiTabDir?uiTabDir:dep>uiScrDepth?1:dep<uiScrDepth?-1:2; } uiScrId=id; uiScrDepth=dep; uiTabDir=0; }
    uiBackHint=extrasView; if(extrasView&&page!=3&&page!=4){ uiTabPrev=extName(extStep(-1)); uiTabNext=extName(extStep(1)); } else uiTabPrev=uiTabNext=0; uiPageOpen=1|2|4|(auralOK()?8:0)|(debugOn?16:0)|32|64; uiPages=titleOn||sceneOn||dlgOn||extrasView?0:7; uiPageNow=page; uiDotX=124;
    if(dlgOn){ uiDialog(); return; } if(sceneOn){ uiScene(); return; } if(!extrasView){ if(titleOn){ uiTitle(); return; } if(menuOn){ uiMenu(); return; } }
    switch(page){ case 1: uiStory(); break; case 2: uiBest(); break; case 3: uiAural(); break; case 4: uiDebug(); break; case 5: uiCodex(); break; case 6: uiHelp(); break; case 7: uiKing(); break; case 8: uiJournal(); break; case 9: uiAch(); break; case 10: uiCult(); break; case 11: uiLang(); break; case 12: uiBrowser(); break; default: uiStatus(); }
}

static void menuToWorld(void){ setSize(planetSize); newWorld(); paused=1; camX=(WW*8-256)/2; camY=(WH*8-192)/2; winX=-99; refillWindow(); }
static void menuOpenPage(int pg){ extrasView=1; page=pg; if(pg==3) juke=gmCur>=0?gmCur:0; menuOn=0; siprintf(info,"START: BACK TO MENU"); }
static void menuInput(int kd){
    int ids[14]; int n=menuList(ids); int d=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0);
    if(menuAsk){ if(kd&KEY_A){ int a=menuAsk; menuAsk=0; menuOn=0; if(a==5) startWorld(!storyOn); else if(a==6) startWorld(storyOn); else if(a==8){ doSave(); titleOn=1; paused=1; mScr=0; mSel=0; newWorld(); camX=(WW*8-256)/2; camY=(WH*8-192)/2; hasSave=sdOk; musReq(GM_TITLE); } } if(kd&(KEY_B|KEY_START)){ menuAsk=0; menuOn=1; } return; }
    if(titleOn&&konami>=4) return;          // the secret code is in progress: the menu holds still
    if(mScr==3){ createInput(kd); return; }
    if(!titleOn&&(kd&KEY_START)){ menuOn=0; paused=dlgPaused; return; }
    if(kd&KEY_UP) mSel=(mSel+n-1)%n; if(kd&KEY_DOWN) mSel=(mSel+1)%n;
    if(kd&KEY_B){ if(mScr){ mSel=mScr==3?1:(titleOn?2:2)+(mScr==2); mScr=0; } else if(!titleOn){ menuOn=0; paused=dlgPaused; } return; }
    int id=ids[mSel];
    if(d){ switch(id){ case MI_SIZE: setSize(d>0?(planetSize==128?192:planetSize==192?256:128):(planetSize==256?192:planetSize==192?128:256)); fxReq(FX_MENU_MOVE); break; case MI_STYPE: storyOn^=1; break;
            case MI_DIFF: diff=(diff+3+d)%3; break; case MI_SPEED: speedMul=d>0?(speedMul==1?2:speedMul==2?4:1):(speedMul==1?4:speedMul==2?1:2); break; case MI_VOL: gmMaster+=d*10; if(gmMaster<0) gmMaster=0; if(gmMaster>100) gmMaster=100; break; case MI_MON: monOn^=1; break; } }
    if(kd&KEY_A){ switch(id){
        case MI_CONT: if(hasSave){ LOADING("LOADING YOUR WORLD",30); loadGame(savePath); endShown=storyDone; titleOn=0; paused=0; refillWindow(); LOADING("READY",100); musReq(GM_NEWWORLD); } else startWorld(1); break;
        case MI_CREATE: mScr=3; mSel=0; createStart(); break; case MI_GO: startWorld(storyOn); break;
        case MI_OPT: mScr=1; mSel=0; break; case MI_EXTRA: mScr=2; mSel=0; break; case MI_BACK: mSel=mScr==3?1:2+(mScr==2); mScr=0; break;
        case MI_SIZE: case MI_DIFF: case MI_VOL: case MI_MON: case MI_SPEED: break;
        case MI_SCULPT: if(auralOK()) menuOpenPage(3); else { siprintf(info,"UNLOCK IN STORY CHAPTER 3"); fxReq(FX_ERROR); } break;
        case MI_BROWSE: menuOpenPage(12); break; case MI_KING: menuOpenPage(7); break; case MI_CULT: menuOpenPage(10); break; case MI_LANG: menuOpenPage(11); break; case MI_JOURNAL: menuOpenPage(8); jrSel=0; break; case MI_ACH: menuOpenPage(9); break; case MI_BEST: menuOpenPage(2); break; case MI_CODEX: menuOpenPage(5); break; case MI_HELP: menuOpenPage(6); break; case MI_DEBUG: menuOpenPage(4); break;
        case MI_CRED: menuOn=0; sceneStart(credTxt,1); musReq(GM_CREDITS); break;
        case MI_RESUME: menuOn=0; paused=dlgPaused; break; case MI_SAVE: doSave(); menuOn=0; paused=dlgPaused; break;
        case MI_NEW: menuAsk=6; break; case MI_MODE: menuAsk=5; break; case MI_TITLE: menuAsk=8; break; } } }

int main(void){
#if !defined(NDSSHIM)&&!defined(NDSHW)
    defaultExceptionHandler();      // a crash shows its registers on screen instead of a frozen display
#endif
    videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_BG1_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32);
    vramSetBankA(VRAM_A_MAIN_BG); vramSetBankB(VRAM_B_MAIN_SPRITE); vramSetBankC(VRAM_C_SUB_BG);
    lcdMainOnBottom();
    videoSetModeSub(MODE_5_2D|DISPLAY_BG3_ACTIVE);
    int bg=bgInit(0,BgType_Text8bpp,BgSize_T_512x512,0,1); (void)bg; mapPtr=(u16*)bgGetMapPtr(0);
    oamInit(&oamMain,SpriteMapping_1D_32,false);
    int sb=bgInitSub(3,BgType_Bmp8,BgSize_B8_256x256,0,0); (void)sb;
    { int b1=bgInit(1,BgType_Text8bpp,BgSize_T_256x256,4,1); bgSetPriority(b1,0); bgSetPriority(bg,1); mapPtr1=(u16*)bgGetMapPtr(b1); }
    for(int t=0;t<T_N;t++){ int f=t; BG_PALETTE_SUB[64+t]=RGB15(terrCol[t][0],terrCol[t][1],terrCol[t][2]); (void)f; }
    BG_PALETTE_SUB[80]=RGB15(31,31,6); BG_PALETTE_SUB[81]=RGB15(31,3,3); BG_PALETTE_SUB[82]=RGB15(31,31,31); BG_PALETTE_SUB[83]=RGB15(31,31,0); BG_PALETTE_SUB[84]=RGB15(31,24,6); BG_PALETTE_SUB[85]=RGB15(31,0,31);
    loadHook=uiLoadHook; memset(uiBuf,0,sizeof uiBuf); uiPalette(); uiRect(0,0,256,192,U_BG); uiTextC(128,84,"BORE: GODGUY",U_GOLD,2); uiTextC(128,110,"LOADING...",U_DIM,1);
#ifdef GG_SAFE
    uiTextC(128,124,"SAFE BUILD: NO SOUND, NO SD",U_DIM,1);
#endif
    uiFlush();
    rs^=(u32)(REG_VCOUNT*2654435761u)^0xA5A5; for(int i=0;i<8;i++) rnd();
#ifndef GG_SAFE
    sdOk=fatInitDefault();
    soundEnable(); DC_FlushRange((void*)gmWave0,64); gmTables();
    // the two effect voices (a PSG pulse and the noise channel, silent until an effect plays) are reserved first, so the wave voices and drum slots can only fill the other 14 channels
    gpId=soundPlayPSG(DutyCycle_50,440,0,64); gnId=soundPlayNoise(8000,0,64);
#endif
    makeSpecies(); makeItems(); makeTiles(); makeSprites(); tbInit(); saveBasePal();
    if(sdOk){ hasSave=loadGame(savePath); }
    if(!hasSave){ planetSize=128; newWorld(); } camX=(WW*8-256)/2; camY=(WH*8-192)/2; paused=1; if(hasSave){ camX=camX>WW*8-256?WW*8-256:camX; }
    musReq(GM_TITLE); musUpdate();
    int held=0, lastTx=-9, lastTy=-9; hud();
    while(1){
        scanKeys(); int kd=keysDown(), kh=keysHeld(); tick++; if(inspT>0) inspT--;
        { static int onTop=-1; int ui=titleOn||menuOn||extrasView||sceneOn||dlgOn; if(ui!=onTop){ onTop=ui; if(ui) lcdMainOnTop(); else lcdMainOnBottom(); }   // menus move to the touch screen, the world goes up
          if(ui){ if(kd&KEY_TOUCH){ touchPosition tp; touchRead(&tp); int h=uiHit(tp.px,tp.py); if(h>=0){ const UiBtn*bt=&uiBtns[h]; int was=bt->sp&&*bt->sp==bt->sv; if(bt->sp) *bt->sp=bt->sv; kd|=was&&bt->key2?bt->key2:bt->key; if(bt->key&(KEY_L|KEY_R)) uiTabDir=(bt->key&KEY_L)?-1:1; } } kd&=~KEY_TOUCH; kh&=~KEY_TOUCH; } }
        musUpdate();
        if(view3d&&(sceneOn||dlgOn||menuOn||titleOn)) leaveGlobe();
        if(sceneOn){ sceneT++; int n=sceneLines(sceneTxt); if(sceneScroll&&(sceneT&7)==0) sceneOff++; int done=(kd&(KEY_A|KEY_START))!=0; if(sceneScroll&&sceneOff>n+2) done=1;
            if(done){ if(sceneNext){ const char*const*nx=sceneNext; int sp0=scenePaused; sceneNext=0; sceneOn=0; sceneStart(nx,1); scenePaused=sp0; musReq(GM_CREDITS); } else { sceneOn=0; paused=scenePaused; } }
            refillWindow(); swiWaitForVBlank(); bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); if((tick&7)==0){ applyLight(); hud(); } continue; }
        if(!titleOn&&!sceneOn&&!dlgOn&&dlgPend>=0&&storyOn){ dlgSet=dlgTab[dlgPend]; dlgPend=-1; dlgIdx=0; dlgChars=0; dlgOn=1; dlgPaused=paused; paused=1; }
        if(dlgOn){ const Dl*d=&dlgSet[dlgIdx]; int tl=(int)strlen(d->txt); if(dlgChars<tl) dlgChars+=2; if(kd&KEY_A){ if(dlgChars<tl) dlgChars=tl; else { dlgIdx++; dlgChars=0; if(!dlgSet[dlgIdx].txt){ dlgOn=0; paused=dlgPaused; } } } if(kd&KEY_B){ dlgOn=0; paused=dlgPaused; }
            refillWindow(); swiWaitForVBlank(); bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); if((tick&7)==0) applyLight(); if(dlgOn&&((tick&1)==0||(kd))) hud(); else if(!dlgOn) hud(); continue; }
        if(!titleOn&&!sceneOn&&strcmp(info,lastInfo)){ strcpy(lastInfo,info); toastT=420; } if(toastT>0) toastT--;
        if(menuOn){ menuInput(kd); refillWindow(); swiWaitForVBlank(); bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); hud(); continue; }
        if(storyDone&&!endShown&&!titleOn&&!dlgOn&&dlgPend<0){ endShown=1; if(view3d) leave3D(); sceneNext=credTxt; sceneStart(endTxt,0); }
        if(!titleOn&&!sceneOn&&stUse<12&&tick>300&&(tick%600)==0){ siprintf(info,"%.32s",tipTxt[tipN++%11]); }
        if(titleOn&&!extrasView){
            int ch=kd!=0; { static const u16 code[10]={KEY_UP,KEY_UP,KEY_DOWN,KEY_DOWN,KEY_LEFT,KEY_LEFT,KEY_RIGHT,KEY_B,KEY_A,KEY_START};
                menuInput(kd);
                if(kd){ if(kd&code[konami]) konami++; else konami=(kd&code[0])?1:0; if(konami==10){ konami=0; debugOn=1; auralOpen=1; fxReq(FX_UNLOCK); siprintf(info,"DEBUG MENU UNLOCKED"); ch=1; } } }
            if(!titleOn) continue;
            if(mScr==3&&cT>=0&&--cT<0){ previewWorld(); ch=1; }
            refillWindow(); swiWaitForVBlank(); bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); if((tick&7)==0) applyLight(); if(ch||(tick&31)==0){ drawMinimap(); hud(); }
            continue; }
        if(titleOn) kh&=~KEY_TOUCH;
        if(extrasView&&(kd&KEY_START)){ extrasView=0; page=0; kd&=~KEY_START; if(titleOn) mScr=2; else { menuOn=1; mScr=2; } mSel=3; }
        int sp_=(kh&KEY_B)?6:3; int dirty=0;
        if((kd&KEY_SELECT)&&!extrasView){ if(0){ } else if(kh&KEY_L&&kh&KEY_R){ doSave(); } else { do page=(page+1)%7; while((page==3&&!auralOK())||(page==4&&!debugOn)); if(page==3) juke=gmCur>=0?gmCur:0; } dirty=1; }
        if((kd&(KEY_X|KEY_Y))&&(kh&KEY_X)&&(kh&KEY_Y)){ if(view3d) leaveGlobe(); else globeEnter(); dirty=1; kd&=~(KEY_X|KEY_Y); }
        if(kd&KEY_START){ if((kh&KEY_L)&&(kh&KEY_R)){ unlockAll=1; siprintf(info,"EVERYTHING UNLOCKED"); } else if(!(kh&KEY_SELECT)){ menuOn=1; menuSel=0; menuAsk=0; mScr=0; mSel=0; dlgPaused=paused; paused=1; } dirty=1; }
        if(extrasView&&page!=3&&page!=4&&(kd&(KEY_L|KEY_R))){ uiTabDir=(kd&KEY_L)?-1:1; page=(kd&KEY_L)?extStep(-1):extStep(1); kd&=~(KEY_L|KEY_R); dirty=1; fxReq(FX_MENU_MOVE); }
        if(page==2){ if(kd&KEY_UP) bestSel=bestSel>0?bestSel-1:0; if(kd&KEY_DOWN) bestSel=bestSel<319?bestSel+1:319; if(kd&KEY_LEFT) bestSel=bestSel>=10?bestSel-10:0; if(kd&KEY_RIGHT) bestSel=bestSel<310?bestSel+10:319; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page==12){ brBuild(); if(kd&KEY_UP) brSel=brSel>0?brSel-1:0; if(kd&KEY_DOWN) brSel=brSel<brN-1?brSel+1:(brN?brN-1:0); if(kd&KEY_LEFT){ brCat=(brCat+6)%7; brSel=0; } if(kd&KEY_RIGHT){ brCat=(brCat+1)%7; brSel=0; }
            if((kd&KEY_Y)&&brCat==5){ brKind=(brKind+1)%8; brSel=0; } if((kd&KEY_X)&&brN){ u8*f=brFav(&brL[brSel]); if(f){ *f^=1; fxReq(*f?FX_UNLOCK:FX_MENU_MOVE); siprintf(info,*f?"STARRED":"STAR REMOVED"); } }
            if((kd&KEY_A)&&brN){ brGo(&brL[brSel]); } if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A|KEY_X|KEY_Y); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page>=7&&page<=11){ int*sel=(page==7||page==10)?&kingSel:page==8?&jrSel:page==11?&langSel:&achSel; int mx=(page==7||page==10)?KN_N-1:page==8?(jrN>14?jrN-14:0):page==11?4:ACH_N-1; if(kd&KEY_UP) *sel=*sel>0?*sel-1:0; if(kd&KEY_DOWN) *sel=*sel<mx?*sel+1:mx; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page==5){ if(kd&KEY_UP) trPage=trPage>1?trPage-1:1; if(kd&KEY_DOWN) trPage=trPage<TR_N-1?trPage+1:TR_N-1; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page==6){ int hn=sceneLines(helpTxt); if(kd&KEY_UP) helpSel=helpSel>0?helpSel-1:0; if(kd&KEY_DOWN) helpSel=helpSel<hn-10?helpSel+1:helpSel; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page==3){ if(kd&KEY_LEFT) juke=(juke+GM_SONGS-1)%GM_SONGS; if(kd&KEY_RIGHT) juke=(juke+1)%GM_SONGS; if(kd&KEY_UP) juke=(juke+GM_SONGS-10)%GM_SONGS; if(kd&KEY_DOWN) juke=(juke+10)%GM_SONGS;
            if((kd&KEY_A)&&!trackOpen(juke)){ siprintf(info,"LOCKED UNTIL 100%%"); fxReq(FX_ERROR); kd&=~KEY_A; }
            if(kd&KEY_A){ if(jukeOn&&gmCur==juke){ gmStop(); jukeOn=0; } else { jukeOn=1; gmStop(); gmPlay(juke); } kd&=~KEY_A; }
            if(kd&KEY_X){ fxSel=(fxSel+1)%GM_FX; fxPlay(fxSel); } if(kd&KEY_Y){ fxSel=(fxSel+GM_FX-1)%GM_FX; fxPlay(fxSel); } kd&=~(KEY_X|KEY_Y);
            if(kd&KEY_B){ jukeOn=0; musHold=0; } if(kd&KEY_R){ gmMaster=gmMaster<100?gmMaster+10:100; } if(kd&KEY_L){ gmMaster=gmMaster>10?gmMaster-10:0; }
            if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_L|KEY_R|KEY_B); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_B); }
        else if(page==4){ int d=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0), go=kd&KEY_A;
            if(kd&KEY_UP) dbgSel=(dbgSel+23)%24; if(kd&KEY_DOWN) dbgSel=(dbgSel+1)%24;
            if(d||go){ int cx=(camX>>3)+16, cy=(camY>>3)+12;
                switch(dbgSel){ case 0: if(d||go){ speedMul=speedMul==1?2:speedMul==2?4:speedMul==4?8:1; } break; case 1: if(d||go) paused^=1; break; case 2: year+=d?d:1; if(year<0) year=0; break;
                    case 3: chapter+=d; if(chapter<0) chapter=0; if(chapter>CHAP_N) chapter=CHAP_N; storyDone=chapter>=CHAP_N; break; case 4: tierOpen+=d; if(tierOpen<1) tierOpen=1; if(tierOpen>14) tierOpen=14; break;
                    case 5: unlockAll^=1; break; case 6: monOn^=1; break; case 7: dbgMon=(dbgMon+320+d)%320; break;
                    case 8: if(go){ int a=ARCH_A+dbgMon/16, sIdx=8+(a-8)*16+dbgMon%16; int ok=0; for(int k=0;k<40&&!ok;k++) ok=addCr(sIdx,cx+(int)(rnd()%7)-3,cy+(int)(rnd()%7)-3)>=0; siprintf(info,ok?"SPAWNED":"NO ROOM HERE"); } break;
                    case 9: if(go){ int n=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==5){ killNat(i); n++; } siprintf(info,"%d MONSTERS GONE",n); } break;
                    case 10: if(go){ for(int m=0;m<320;m++){ int sIdx=8+(ARCH_A+m/16-8)*16+m%16; seenB[sIdx>>3]|=(u8)(1<<(sIdx&7)); } seenN=320; siprintf(info,"ALL 320 MET"); } break;
                    case 11: if(go){ for(int i=0;i<nCr;i++) if(cr[i].alive){ cr[i].hp=(u8)maxHp(&cr[i]); cr[i].food=120; } siprintf(info,"EVERYONE HEALED"); } break;
                    case 12: if(go){ for(int v=0;v<VIL_N;v++) if(vil[v].on) vil[v].stock=400; siprintf(info,"STORES FULL"); } break;
                    case 13: if(go){ int r=spSel<8?sp[spSel].a:0; siprintf(info,vilFound(r,cx,cy)>=0?"VILLAGE FOUNDED":"NO ROOM"); } break;
                    case 14: if(go){ for(int m=0;m<320;m++){ int sI=8+(ARCH_A+m/16-8)*16+m%16; slainB[sI>>3]|=(u8)(1<<(sI&7)); } stKill+=50; siprintf(info,"ALL 320 SLAIN"); } break;
                    case 15: if(go&&chapter<CHAP_N) chapterDone(); break;
                    case 16: if(d<0) doSave(); else if(d>0||go){ if(sdOk&&loadGame(savePath)){ refillWindow(); siprintf(info,"LOADED"); } else siprintf(info,"NO SAVE TO LOAD"); } if(go&&(kh&KEY_B)){ if(sdOk){ remove(savePath); hasSave=0; siprintf(info,"SAVE ERASED"); } } break;
                    case 17: dbgFx=(dbgFx+GM_FX+d)%GM_FX; if(go||d) fxPlay(dbgFx); break; case 18: if(go||d) auralOpen^=1; break; case 20: seedFix=(seedFix?seedFix:worldSeed)+(d?d:0); if(!seedFix) seedFix=1; break;
                    case 21: if(go){ if(!seedFix) seedFix=worldSeed; startWorld(storyOn); seedFix=0; siprintf(info,"SEED %04X",(unsigned)(worldSeed&0xFFFF)); page=0; } break;
                    case 22: if(go){ int n=0; for(int k=0;k<400&&n<50;k++){ int i=rnd()%(nCr?nCr:1); if(nCr&&cr[i].alive){ giveTrait(&cr[i],1+(int)(rnd()%(TR_N-1)),0); cr[i].hp=(u8)maxHp(&cr[i]); n++; } } siprintf(info,"%d MUTATED",n); } break;
                    case 23: diff=(diff+3+d)%3; if(!d&&go) diff=(diff+1)%3; break;
                    case 19: if(go||d){ fullDone^=1; siprintf(info,fullDone?"100%% SET: REMIXES OPEN":"100%% CLEARED"); } break; }
                dirty=1; }
            if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_L|KEY_R|KEY_A); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        if(kh&KEY_LEFT) camX-=sp_; if(kh&KEY_RIGHT) camX+=sp_; if(kh&KEY_UP) camY-=sp_; if(kh&KEY_DOWN) camY+=sp_;
        if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192;
        if(kd&KEY_R){ power=(power+1)%PW_N; dirty=1; } if(kd&KEY_L){ power=(power+PW_N-1)%PW_N; dirty=1; }
        if(year!=savedYear&&year>0&&(tick&255)==1){ doSave(); dirty=1; }
        if(view3d){
            int ts=(kh&KEY_B)?4:2; if(!walkOn){ if(kh&KEY_LEFT) rotY-=ts; if(kh&KEY_RIGHT) rotY+=ts; if(kh&KEY_UP) rotX+=ts; if(kh&KEY_DOWN) rotX-=ts; if(!(kh&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))&&(tick&3)==0) rotY+=1; }
            if(rotX>85) rotX=85; if(rotX<-85) rotX=-85; rotY%=360; if(rotY<0) rotY+=360;
            if(kh&KEY_X){ zoom3-=1; if(zoom3<30) zoom3=30; } if(kh&KEY_Y){ zoom3+=1; if(zoom3>110) zoom3=110; }
            if((kd&KEY_A)&&(kh&KEY_Y)){ walkOn^=1; if(walkOn){ walkX=(camX>>3)+16; walkY=(camY>>3)+12; if(walkX>=WW) walkX=WW-1; if(walkY>=WH) walkY=WH-1; zoom3=30; siprintf(info,"WALKING: PAD MOVES, A USES POWER"); } else siprintf(info,"WALK OFF"); dirty=1; kd&=~KEY_A; }
            if(walkOn){ static int wt=0; int mv=(kh&KEY_B)?2:4; if(++wt>=mv){ wt=0; int dx2=((kh&KEY_RIGHT)?1:0)-((kh&KEY_LEFT)?1:0), dy2=((kh&KEY_DOWN)?1:0)-((kh&KEY_UP)?1:0); if(dx2||dy2){ walkX=(walkX+dx2+WW)%WW; walkY+=dy2; if(walkY<0) walkY=0; if(walkY>=WH) walkY=WH-1; } }
                faceTile(walkX,walkY,&rotX,&rotY); if(kh&KEY_A){ static int at=0; if((kd&KEY_A)||++at>=4){ at=0; usePower(walkX,walkY,(kd&KEY_A)?1:(pw[power].kind==PK_PAINT)); } } kd&=~KEY_A; }
            if(kd&KEY_A){ gClouds^=1; dirty=1; }
            { static int h3=0, l3x=-9, l3y=-9, lpx=-1, lpy=-1;
              if(kh&KEY_TOUCH){ touchPosition tp; touchRead(&tp);
                  if(kh&KEY_B){ if(lpx>=0){ rotY-=(tp.px-lpx)/2; rotX+=(tp.py-lpy)/2; } lpx=tp.px; lpy=tp.py; h3=0; }   // B held: drag to turn the planet
                  else { lpx=-1; int tx,ty; if(globePick(tp.px,tp.py,&tx,&ty)){ int fresh=!h3||tx!=l3x||ty!=l3y; if(pw[power].kind==PK_PAINT||fresh) usePower(tx,ty,!h3); h3=1; l3x=tx; l3y=ty; dirty=1; } } }
              else { h3=0; lpx=-1; } }
            for(int k=1;k<speedMul&&!paused;k++){ tick++; simulate(); }
            if(!paused) simulate(); { int gd=globeDraw(); swiWaitForVBlank(); if(gd) globeFlush(); } if((tick&15)==0||dirty){ drawMinimap(); hud(); } continue; }
        if(kd&(KEY_X|KEY_Y)){ int d=(kd&KEY_X)?1:-1; if(kh&KEY_B){ if(pw[power].kind==PK_GIFT) itSel=itNext(itSel,10*d); else spSel=(spSel+nSp+10*d)%nSp; } else if(pw[power].kind==PK_SPAWN){ spSel=(spSel+nSp+d)%nSp; } else if(pw[power].kind==PK_GIFT){ itSel=itNext(itSel,d); } else if(pw[power].kind==PK_MUTATE){ for(int k=1;k<=TR_N;k++){ int t=(trSel+TR_N*2+d*k)%TR_N; if(t==0||((trSeen>>t)&1)||debugOn||unlockAll){ trSel=t; break; } } } else { brush+=d; if(brush<0) brush=0; if(brush>8) brush=8; } dirty=1; }
        if(kd&KEY_A&&page!=3){ paused^=1; dirty=1; }
        if(kh&KEY_TOUCH){ touchPosition tp; touchRead(&tp); if(tp.py>=168){ if(!held){ tbTouch(tp.px,tp.py); fxReq(FX_MENU_MOVE); dirty=1; } held=1; } else { int tx=(tp.px+camX)>>3, ty=(tp.py+camY)>>3; int fresh=!held||tx!=lastTx||ty!=lastTy; if(pw[power].kind==PK_PAINT||fresh) usePower(tx,ty,!held); held=1; lastTx=tx; lastTy=ty; dirty=1; } } else held=0;
        for(int k=1;k<speedMul&&!paused;k++){ tick++; simulate(); }
        refillWindow(); simulate();
        swiWaitForVBlank();
        bgSetScroll(0,camX&511,camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain);
        if((tick&7)==0||flashT>0) applyLight();
        if(dirty) tbDraw();
        if((tick&15)==0||dirty){ drawMinimap(); hud(); }
    }
    return 0;
}

#endif
