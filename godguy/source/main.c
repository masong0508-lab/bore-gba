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
#include "world_height.h"
#include "gmdata.h"      // the music and sound-effect tables (ids like FX_SMITE and GM_TITLE are used by the game; the player is gmplay.h, DS and PC)

#include "ggx.h"
// Optional instrumentation for the frame stress benchmark.
#ifndef GG_FRAME_BEGIN
#define GG_FRAME_BEGIN(phase) ((void)0)
#define GG_FRAME_END(phase) ((void)0)
#endif
int gW=128, gH=128;
u8 world[MAXD][MAXD];
static occ_t occ[MAXD][MAXD];            // creature index + 1 standing on a tile
static Cr cr[MAXC]; static int nCr;
#ifdef GG_WARSTAT
static unsigned long ggTurns;      // (benchmark builds only, pc_war.c: how many turns think() has taken; not compiled into any game build)
#define GG_TURN() (ggTurns++)
#else
#define GG_TURN() ((void)0)
#endif
#define FL_SAPIENT 0x0400     /* a sapient animal (Pack D) */
#define FL_EVOLVED 0x0800     /* an animal that has evolved (law EVOLUTION): its own name, half again the health, +3 damage, half again the years, a crown */
#define IS_EVO(c) ((c)>=cr&&(c)<cr+MAXC&&(crFl[(c)-cr]&FL_EVOLVED))
#define FL_BEEQUEEN 0x1000     /* queen of a local bee colony; stored in the existing creature-flags trailer */
static u32 crId[MAXC]; static u16 crFl[MAXC]; static u32 idNext=1;
static struct { int x, y, a, until; } rz[4]; static int rzN;      // the last four places a captain fell: the soldiers of his people near them are routed for a while
static void ntKill(Cr*c); static void ntHit(Cr*a,Cr*d,int dm); static void ntDeath(int i); static void ntGone(u32 id); static void ntBirth(Cr*kid,const Cr*par); static void ntFounder(Cr*c,int v); static void ntSweep(void);   // (ggid.h: names, ids and the notable table)
static int crNamed(const Cr*c); static void crFirst(const Cr*c,char*o); static void idReset(void);
static int camX, camY;             // camera, in world pixels
enum { MAP_PIN_N=3 };
static u16 mapPin[MAP_PIN_N][2];
static u8 mapPinMask;
static int warHeat;                // blows struck near the camera, decaying: how loud the battle music is (see musDirector)
static int power=1, brush=2, spSel=40, paused=0, tick=0, year=0;
static int sdOk=0, savedYear=-1, autoSaveOn=1;
static char info[64];
static u32 worldSeed, seedFix;               // the seed of this planet; seedFix (if not 0) is used for the next one
static u32 stKill, stGift, stUse;                 // monsters slain, gifts given, powers used (the story counts them)
static u8 seenB[(SP_N+7)/8], slainB[(SP_N+7)/8]; static int seenN;   // the bestiary: which kinds of monster have appeared / been slain
#define ACH_N 46
enum { GG_BUILD_FAVORITE=80 };
static u8 favVil[FAV_N], favKd[16], favItem[600], favPow[GG_BUILD_FAVORITE+1];   // saved stars on cities, kingdoms, items and powers
static void achCheck(void);
static u32 achX[3];   /* achievements 32 and up, the political powers used (a bit each), and event flags (1 schism, 2 crab bomb, 4 stomp, 8 goo, 16 fire elemental, 32 had a kingdom): they ride in the spare words of the stats block */
#define ACH_HAS(i) ((i)<32?(int)((achBits>>(i))&1):(int)((achX[0]>>((i)-32))&1))
#define ACH_SET(i) do{ if((i)<32) achBits|=1u<<(i); else achX[0]|=1u<<((i)-32); }while(0)
static u32 achBits, stWars, stCapt, stAllies, stSnack;   // achievements earned and the counters they need
static int dlgPend=-1;      // a dialogue waiting to be shown (chapter number, 14 = the epilogue)
#ifndef PCTEST
enum { STORY_HANDOFF_TICKS = 180 };
static int storyGoalT;      // real gameplay frames to read the objective after a dialogue
#endif
static int fullDone, auralOpen, debugOn, speedMul=1, chapter, storyOn=1, storyDone, monOn=1, natural, tierOpen=1;
static int autoSaveDue(void){ return autoSaveOn&&sdOk&&year>0&&year!=savedYear&&(tick&255)>=1&&(tick&255)<=(unsigned)speedMul; }
static int fxQ=-1, musQ=-1, storyScore=-1;       // requests for the sound code (DS main loop plays them)
static void fxReq(int id){ fxQ=id; }
static void musReq(int id){ musQ=id; }
// Defined by gmplay.h after the story renderer; declarations keep the
// film's visual and sound timelines in the same 60 Hz clock domain.
static void gmFilmMixSet(int active,int caption,int pan);
static void giveTool(Cr*c,int armor); static int gearAtk(const Cr*c); static int armDef(const Cr*c); static void unlockTiers(void);
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
 /*42 tornado top*/ {"bbbbbbbb","dbbbbbbd",".bbdbbb.",".dbbbbd.","..bbdb..","..dbbb..","...bbd..","...dbb.."},
 /*43 tornado tip*/ {"...bbd..","...dbb..","....bb..","....db..","...bb...","...bd...","....b...","........"},
 /*44 black hole*/ {"..eeee..",".edddde.","eddbbdde","edbbbbde","edbbbbde","eddbbdde",".edddde.","..eeee.."},
 /*45 boat*/ {"...b....","...bb...","..bbb...","..bbbb..","...d....","dddddddd",".dddddd.","........"},
 /*46 flame*/ {"...b....","..bb.b..","..bbbb..",".bbdbbb.",".bdddbb.",".bdddbb.","..bbbb..","...dd..."},
 /*47 goo*/ {"........","..bbb...",".bbbbb..","bbebbeb.","bbbbbbb.","dbbbbbd.",".dddddd.","........"},
 /*48 ghost*/ {"..bbbb..",".bbbbbb.",".bebbeb.",".bbbbbb.",".bbbbbb.",".bbbbbb.",".b.bb.b.","........"},
 /*49 snowman*/ {"..bbb...","..beb...","...b....","..bbbb..",".bbbbbb.",".bbbbbb.","..bbbb..","........"},
 /*50 crab*/ {"........","b.....b.","bb.bb.bb",".bbbbbb.","bbebbebb",".bbbbbb.","b.b..b.b","........"},
 /*51 penguin*/ {"..ddd...","..deb...","..dddd..",".dbbbd..",".dbbbd..",".dbbbd..","..aa.aa.","........"},
 /*52 frog*/ {"........","b.b..b.b","bbbbbbbb","bebbbbeb",".bbbbbb.",".bbbbbb.","bb.bb.bb","........"},
 /*53 bee*/ {"........","..b.b...",".bbbbb.b","bdbdbbbb","bbbbbbb.",".bdbd...","........","........"},
 /*54 headset*/ {"........","dddddddd","dbbbbbbd","dbeebeed","dbbbbbbd",".dbbbbd.","..d..d..","........"}, /*55 cat*/ {"b.b.....","bbbb....","bebeb...",".bbbb..b","..bbbbbb","..bbbbb.","..b.b.b.","..d.d.d."},
 /*56 dog*/ {"..b.....",".bbb.b..",".bebbb..","bbbbbbb.","bbbbbbb.",".bbbbb..",".b.b.b..",".d.d.d.."},
 /*57 chicken*/ {"..a.....","..bb....","..bbe...","...bb...","..bbbb..",".bbbbbb.","..a..a..","..a..a.."},
 /*58 goat*/ {"b..b....","bbbb....","bebbb...","bbbbb...",".bbbbbb.",".bbbbbb.",".b.b.b..",".d.d.d.."},
 /*59 armadillo*/ {"........","..dddd..",".dbdbdd.","dbdbdbd.","dbbbbbbe",".bbbbbb.",".d.dd.d.","........"},
 /*60 ostrich*/ {"..bb....","..be....","..b.....","..b.....",".bbbb...","bbbbbb..",".bbbb...",".a..a..."},
 /*61 seal*/ {"........","..bbb...",".bbebb..","bbbbbb..","bbbbbbb.",".bbbbbbb","..b..bb.","........"},
 /*62 piranha*/ {"........","...bb...",".bbbbb.b","bbbebbbb","baabbbb.",".bbbbbb.","..bb..bb","........"},
 /*63 butterfly*/ {"a.b..b.a","aabbbbaa","aabbbbaa",".ab..ba.",".a.bb.a.","..b..b..","........","........"},
 /*64 grasshopper*/ {"........","b.......","bb......",".bbebb..","..bbbbbb","..b.b.bb",".b..b..b",".b......"},
 /*65 fly*/ {"..b..b..",".bbb.bbb","b.bbbb.b","..bebb..","..bbbb..","...bb...","..b..b..","........"},
 /*66 fairy*/ {"a...a...","aa.aa...","a.bb.a..","..beb...","..bbb...","..a.a...","........","........"},
 /*67 unicorn*/ {"...a....","..ab....","bbeb....","bbbbbbb.","bbbbbbb.",".bbbbbb.",".b.b.b..",".d.d.d.."},
 /*68 white mage*/ {"..aaa...","..beb...","..bbb...",".bbbbb..","baabaab.","b.bbb.b.","..bbb...",".bb.bb.."},
 /*69 druid*/ {".aaaa...","aaaaaa..",".bebb...","..bbb.d.",".bddbbd.","bbddbbd.",".bbbb.d.",".d..d.d."},
 /*70 flower bud*/ {"..aaa...",".aabaa..","..aaa...","...d....","..dd.d..",".d.dd.d.","...d....","..ddd..."},
 /*71 crystal sword*/ {"....bb..","...bbb..","...bb...","..bb....",".bb.....","dbd.....","ddd.....","d......."},
 /*72 bitba*/ {"........",".bbbbb..","bbebebb.","bbbbbbb.","bbbbbbb.","bbaaabb.",".bbbbb..","........"},
 /*73 garl*/ {"...b....","..bb....",".bbbbb..","bbebebb.","bbbbbbb.","bbbbbbb.",".bbbbb..","..ddd..."},
 /*74 smore*/ {"bbbbbb..","dddddd..","aaaaaa..","dddddd..","bbbbbb..","........","........","........"},
 /*75 acid blob*/ {"........","..bbb...",".bbbbb..","bbebbeb.","bbbbbbb.","dbbbbbd.",".d.d.d..","........"},
 /*76 ant*/ {"........","...bb...","..bbbb..","b.bbbb.b",".b.bb.b.","b.bbbb.b","..b..b..","........"},
 /*77 worm*/ {"........","........","..bbb...",".bebbb..","..bbbbb.","....bbbb",".....bbb","......bb"},
 /*78 printer*/ {"dddddddd","dbbbbbbd","dbaabbbd","dbbbbbbd","dddddddd","dbbbbbbd","dddddddd","........"},
 /*79 finger*/ {"..b.....","..b.....","..bb....","bbbbb...","bbbbbb..","bbbbbb..",".bbbb...","..dd...."},
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
 {"RHINO",21,0,M_LAND,24,80,10,100,8},{"BUFFALO",23,0,M_LAND,22,60,7,90,14},{"HYENA",20,1,M_LAND,10,24,7,60,20},{"ALPACA",22,0,M_CLIMB,22,18,1,70,20},
 {"CAPYBARA",21,0,M_LAND|M_WATER,26,22,0,70,22},{"PENGUIN",51,2,M_LAND|M_WATER,18,10,2,60,24},{"MONKEY",8,2,M_CLIMB,10,14,3,60,26},{"CRAB",50,2,M_LAND|M_WATER,20,8,4,50,40},
 {"CROCODILE",7,1,M_LAND|M_WATER,20,60,12,120,8},{"FROG",52,2,M_LAND|M_WATER,12,5,1,40,50},{"BEE",53,0,M_ALL&~(1<<T_LAVA),4,3,2,25,60},{"RACCOON",20,2,M_LAND,14,14,3,60,24},
 // ---- the new animals, insects, magical folk and special animals (diet 0 to 3), then the terraformers (diet 6: they work the ground and harm nobody)
 {"CAT",55,1,M_LAND,10,12,4,60,24},
 {"DOG",56,2,M_LAND,12,18,5,70,22},
 {"CHICKEN",57,0,M_LAND,16,6,0,40,50},
 {"GOAT",58,0,M_CLIMB,16,16,3,60,28},
 {"ARMADILLO",59,2,M_LAND,22,26,2,80,18},
 {"OSTRICH",60,0,M_LAND,8,24,5,70,16},
 {"SEAL",61,3,M_LAND|M_WATER,14,36,4,100,12},
 {"PIRANHA",62,1,M_WATER,6,8,6,40,40},
 {"BUTTERFLY",63,0,M_ALL&~(1<<T_LAVA),5,2,0,20,60},
 {"GRASSHOPPER",64,0,M_LAND,6,4,1,25,60},
 {"FLY",65,0,M_ALL&~(1<<T_LAVA),4,2,0,15,70},
 {"FAIRY",66,0,M_ALL&~(1<<T_LAVA),6,8,0,150,8},
 {"UNICORN",67,0,M_LAND,10,50,8,160,6},
 {"WHITE MAGE",68,2,M_LAND,14,40,3,170,8},
 {"DRUID",69,2,M_LAND,14,40,4,170,8},
 {"FLOWER BUD",70,0,M_LAND,30,10,0,100,20},
 {"CRYSTAL SWORD",71,1,M_LAND,8,30,14,120,6},
 {"BITBA",72,2,M_LAND,12,20,4,80,18},
 {"GARL",73,2,M_LAND,14,24,6,90,14},
 {"SMORE",74,0,M_LAND,18,16,2,80,22},
 {"ACID BLOB",75,1,M_LAND|M_WATER,22,22,6,80,14},
 {"GOD FINGER",79,6,M_ALL,16,60,0,250,0},
 {"BLACK ANT",76,6,M_LAND,18,12,0,250,0},
 {"RED ANT",76,6,M_LAND,18,12,0,250,0},
 {"BLUE ANT",76,6,M_LAND,18,12,0,250,0},
 {"GREEN ANT",76,6,M_LAND,18,12,0,250,0},
 {"WORM",77,6,M_LAND,22,16,0,250,0},
 {"SAND SPIDER",12,6,M_LAND|(1<<T_MOUNT),16,20,0,250,0},
 {"PRINTER",78,6,M_LAND,26,40,0,250,0},
 // ---- the monsters (diet 5): they hunt every living thing, and burn the houses of the peoples. tier = the age of the world they first come in (monTier[])
 {"IMP",31,5,M_ALL&~(1<<T_LAVA),8,14,4,120,0},{"GHOUL",5,5,M_LAND,14,28,6,150,0},{"SKELETON",16,5,M_LAND,14,24,7,200,0},{"OOZE",6,5,M_LAND|M_WATER,24,30,4,200,0},
 {"WEREWOLF",32,5,M_LAND,10,40,11,160,0},{"ZOMBIE",5,5,M_LAND,22,60,7,220,0},{"WRAITH",17,5,M_ALL,10,30,10,180,0},{"GIANT SPIDER",12,5,M_CLIMB,10,36,9,170,0},{"SCORPION",37,5,M_LAND,12,30,10,170,0},
 {"TROLL",38,5,M_CLIMB,18,90,14,230,0},{"GOLEM",18,5,M_CLIMB,28,140,16,250,0},{"WYVERN",11,5,M_ALL,8,70,15,200,0},{"SEA SERPENT",2,5,M_WATER,8,80,14,200,0},
 {"KRAKEN",19,5,M_WATER,14,160,18,230,0},{"CYCLOPS",33,5,M_LAND,20,150,20,240,0},{"BASILISK",7,5,M_LAND,12,90,18,220,0},
 {"HYDRA",36,5,M_LAND|M_WATER,16,200,20,250,0},{"DEMON",34,5,M_ALL,10,140,24,250,0},{"LICH",35,5,M_LAND,12,120,26,250,0},{"ELDER DRAGON",11,5,M_ALL,8,250,30,250,0},
 {"EVIL MAGE",5,5,M_LAND,14,40,12,160,0},{"NECROMANCER",35,5,M_LAND,14,60,14,200,0},{"PLAGUE DOCTOR",5,5,M_LAND,16,50,9,170,0},{"COLD ONE",38,5,M_CLIMB,18,100,14,220,0},
 {"ALIEN",31,5,M_ALL,12,50,13,200,0},{"UFO",18,5,M_ALL&~(1<<T_LAVA),8,90,16,230,0},{"LIVING PLANT",6,5,M_LAND,26,60,8,210,2},{"LIVING HOUSE",18,5,M_LAND,30,110,12,240,0},
 {"TUMOR MONSTER",6,5,M_LAND,28,50,5,220,6},{"BIOBLOB",6,5,M_LAND|M_WATER,24,40,6,200,5},{"LIL PUMPKIN",31,5,M_LAND,14,20,5,150,4},{"ASSIMILATOR",18,5,M_LAND,20,120,14,240,3},
 {"RUDE SKULL",16,5,M_ALL,12,26,8,180,0},{"FIRE SKULL",16,5,M_ALL,10,34,11,190,0},{"VR HEADSET",54,5,M_ALL&~(1<<T_LAVA),9,70,10,200,0},{"ANGEL",1,5,M_ALL,6,110,20,250,0},
};
static const u8 monTier[ARCH_N-ARCH_A]={0,0,0,0, 1,1,1,1,1, 2,2,2,2, 3,3,3, 4,4,4,4, 2,3,2,3,3,4,1,3,2,2,1,4,1,2,2,4};
static Sp sp[SP_N]; static int nSp;
static const char* const bioNm[]={"DEEP","WATER","SAND","GRASS","FOREST","HILL","MOUNTAIN","SNOW","LAVA","DESERT","SWAMP","JUNGLE","DIRT"};
static void makeSpecies(void){
    nSp=0;
    for(int a=0;a<8&&nSp<SP_N;a++){ sp[nSp].a=(u8)a; sp[nSp].fam=(u8)(a==0?12:a==1?5:a==2?11:a==3?4:a==4?3:a==5?1:a==6?9:2); nSp++; }   // the peoples: one each, their own colours
    for(int a=8;a<ARCH_N;a++) for(int f=0;f<16&&nSp<SP_N;f++){ sp[nSp].a=(u8)a; sp[nSp].fam=(u8)f; nSp++; }   // every animal in every colour: 8 + 24 x 16 = 392 kinds
}
static int preferredFamily(int a){ return (a*7+3)&15; }  // each species has a favoured coat colour
static int randomFamily(int a){ u32 q=rnd(); return (q%100<65)?preferredFamily(a):(int)((q>>8)&15); }
enum { A_FAIRY=55, A_UNICORN, A_WMAGE, A_DRUID, A_BUD, A_SWORD, A_BITBA, A_GARL, A_SMORE, A_ACID, A_FINGER, A_BANT, A_RANT, A_BLANT, A_GANT, A_WORM, A_SSPIDER, A_PRINTER, A_BUTTERFLY=52 };
#define EVN_N 57
static const char* const evNm[EVN_N]={"WOOLLORD","MOOLORD","ANTLERFOLK","HOPLORD","FANGKIN","CUNNINGFOLK","BRUINLORD","TUSKFOLK","SKYLORD","CHIRPFOLK","NIGHTWING","COILFOLK","SCALEFOLK","WEBFOLK","DUNGLORD","SHELLFOLK","FINFOLK","MAWFOLK","DEEPSINGER","BLOBMORPH","WYRMLORD","DRAKELORD","SQUEAKFOLK","DIGGERFOLK","HORNLORD","HERDLORD","LAUGHFOLK","FLUFFLORD","CHILLFOLK","WADDLEFOLK","APEFOLK","CLAWFOLK","SNAPFOLK","BOGFOLK","HIVEKIN","BANDITFOLK","MEOWMORPH","BARKFOLK","PECKLORD","BLEATLORD","PLATEFOLK","STRIDEFOLK","FLIPPERFOLK","BITEFOLK","WINGMORPH","LEAPFOLK","BUZZFOLK","FAEFOLK","PUNKCORN","ARCHMAGE","ELDERDRUID","BLOOMFOLK","CRYSTALLORD","BITBYTE","GARLLORD","SMORELORD","ACIDMORPH"};      // the evolved forms, by archetype 8..64
static void spName(int s,char*out){ const Arch*A=&arch[sp[s].a]; if(sp[s].a<8) strcpy(out,A->nm); else { strcpy(out,advNm[sp[s].fam]); strcat(out," "); strcat(out,A->nm); } }

static const s8 dxs[4]={1,0,-1,0}, dys[4]={0,1,0,-1};
// ---------------------------------------------------------------------------------------------- subspecies: heritable traits (my own design; the WorldBox wiki page could not be read from here)
enum { TR_NONE, TR_VENOM, TR_HIDE, TR_SWIFT, TR_LEAN, TR_FERTILE, TR_LONG, TR_GIANT, TR_TINY, TR_FIRE, TR_AMPH, TR_CLIMB, TR_WING, TR_PACK, TR_OMNI, TR_REGEN, TR_BRAVE, TR_CLEVER, TR_FIERCE, TR_CAMO, TR_NOCT, TR_SPIT, TR_SYMB, TR_FROST, TR_CHILL, TR_MUNCH, TR_GIGGLY, TR_ZEN, TR_FLOATY, TR_COUCH, TR_LUCKY, TR_SPACY, TR_DESERT, TR_SWAMP, TR_PERMA, TR_THORNS, TR_WHIRL, TR_BUBBLE, TR_MIRACLE, TR_LUSTFUL, TR_MOONCHILD, TR_NIGHTCHILD, TR_HEART, TR_BURNFEET, TR_GRIN, TR_LAMP, TR_N };
static const char* const trNm[TR_N]={"","VENOMOUS","THICK-HIDED","SWIFT","LEAN","FERTILE","LONG-LIVED","GIANT","TINY","FIRE-BORN","AMPHIBIOUS","CLIMBING","WINGED","PACK","OMNIVORE","REGENERATING","BRAVE","CLEVER","FIERCE","CAMOUFLAGED","NOCTURNAL","SPITTING","SYMBIOTIC","FROST-BORN","CHILLED","MUNCHY","GIGGLY","ENLIGHTENED","FLOATY","COUCH-LOCKED","LUCKY","SPACED OUT","DESERT-ADAPTED","SWAMP-ADAPTED","PERMAFROST","THORNY","WHIRLWIND","BUBBLE-SKIN","MIRACLE-BORN","LUSTFUL","MOONCHILD","NIGHTCHILD","MEGA HEART","BURNING FEET","GRIN MARK","LIGHT LAMP"};
static const char* const trDesc[TR_N]={"","BITES HARDER: +4 DAMAGE","TAKES 3 LESS DAMAGE A HIT","MOVES 30% FASTER","HALF THE HUNGER","BREEDS HALF AGAIN AS OFTEN","LIVES HALF AGAIN AS LONG","60% MORE HP, +2 DAMAGE, SLOW","40% LESS HP, BREEDS MORE","WALKS ON LAVA","WALKS ON WATER AND LAND","WALKS UP MOUNTAINS","FLIES OVER ANYTHING BUT LAVA","+2 DAMAGE PER PACKMATE NEXT TO IT","EATS PLANTS AND PREY","HEALS A LITTLE EVERY TURN","+3 DAMAGE TO MONSTERS","BETTER GEAR, MORE BREEDING","HUNTS EVEN WHEN NOT HUNGRY","HALF OF THE HUNTERS MISS IT","FAST AT NIGHT, SLOW BY DAY","STRIKES FROM TWO TILES AWAY","HEALS THE ONES NEXT TO IT","WALKS SNOW AND MOUNTAINS","NOTHING MATTERS. HALF HUNGER, SLOW","HAS THE MUNCHIES: EATS ANYTHING, BREEDS MORE","LAUGHS IT OFF: HUNTERS MISS 1 IN 3","INNER PEACE: -2 DAMAGE, HEALS","FLOATS ACROSS WATER, LOST IN THOUGHT","50% MORE HP, VERY SLOW","1 HIT IN 8 LANDS FOR +6","STARES AT MOUNTAINS, THEN WALKS UP THEM","LESS SLOWED BY DESERT HEAT","LESS SLOWED BY SWAMP","RESISTS SNOW AND EXTREME COLD","SPIKES REFLECT SOME ATTACK DAMAGE","SOMETIMES DODGES A HIT","BUBBLE CUTS INCOMING DAMAGE","RARELY SURVIVES A FATAL HIT","BREEDS MORE OFTEN","HEALS UNDER THE NIGHT SKY","MOVES FASTER AT NIGHT","PERIODICALLY RESTORES HEALTH","SETS NEARBY GROUND ALIGHT","BRINGS JOY TO NEARBY FOLK","SENDS OUT A SMALL HEALING AURA"};
static u32 trSeen, trSeenHi; static u32 stMut; static int trCount[TR_N], diff=1;      // traits met in this save, mutations so far, living bearers, difficulty 0 calm 1 normal 2 brutal
#define HAS(c,t) ((c)->tr==(t)||(c)->tr2==(t))
// Village Grin Mark counts change only when a bearer moves, is born, dies,
// mutates, or a village changes. Keep this tiny cache outside the save format.
static u8 joyEpoch=1, joyStamp[VIL_N];
static u16 joyCached[VIL_N];
static void joyInvalidate(void){ if(++joyEpoch==0){ memset(joyStamp,0,sizeof joyStamp); joyEpoch=1; } }
/* Disease occupies sick bits 0..1; the six status flags use its spare bits. The DS record remains 20 bytes. */
enum { ST_INVINCIBLE=0x04, ST_ASH_FEVER=0x08, ST_COUGH=0x10, ST_MOTIVATED=0x20, ST_HANDSOME=0x40, ST_MINDLESS=0x80 };
#define DISEASE(c) ((c)->sick & 3u)
#define HAS_STATUS(c,b) (((c)->sick & (b)) != 0)
#define SET_DISEASE(c,v) ((c)->sick=(u8)(((c)->sick&0xFCu)|((v)&3u)))
static int traitMet(int t){ return t>0&&t<TR_N&&(t<32?((trSeen>>t)&1u):((trSeenHi>>(t-32))&1u)); }
static void traitMark(int t){ if(t>0&&t<TR_N){ if(t<32) trSeen|=1u<<t; else trSeenHi|=1u<<(t-32); } }
static int traitMetCount(void){ return __builtin_popcount(trSeen)+__builtin_popcount(trSeenHi); }
static int trMask(const Cr*c){ int m=0; if(HAS(c,TR_FIRE)) m|=1<<T_LAVA; if(HAS(c,TR_AMPH)||HAS(c,TR_FLOATY)) m|=M_WATER; if(HAS(c,TR_CLIMB)||HAS(c,TR_SPACY)) m|=1<<T_MOUNT; if(HAS(c,TR_WING)) m|=M_ALL&~(1<<T_LAVA); if(HAS(c,TR_FROST)) m|=(1<<T_SNOW)|(1<<T_MOUNT); if(HAS(c,TR_PERMA)) m|=1<<T_SNOW; return m; }
static int maxHp(const Cr*c){ int h=arch[sp[c->sp].a].hp*gxVigorK(sp[c->sp].a)/100; if(HAS(c,TR_GIANT)) h=h*16/10; if(HAS(c,TR_COUCH)) h=h*15/10; if(HAS(c,TR_TINY)) h=h*6/10; if(c->rk==1) h=h*13/10; else if(c->rk==2) h=h*2; if(IS_EVO(c)) h=h*15/10; h+=c->dir*5; if(h<3) h=3; return h>255?255:h; }      // (dir holds the level, 0..10)
static int effSpd(const Cr*c){ int sd=arch[sp[c->sp].a].spd, terrain=world[c->y][c->x], night=(((tick>>1)&2047)<256||((tick>>1)&2047)>1792);
    if(LAWON(LW_SNOW)&&terrain==T_SNOW&&!HAS(c,TR_FROST)&&!HAS(c,TR_CHILL)&&!HAS(c,TR_WING)&&!HAS(c,TR_PERMA)) sd=sd*13/10;
    if(terrain==T_DESERT&&!HAS(c,TR_DESERT)) sd=sd*12/10;
    if(terrain==T_SWAMP&&!HAS(c,TR_SWAMP)&&!HAS(c,TR_AMPH)) sd=sd*12/10;
    if(DISEASE(c)==1) sd=sd*13/10; else if(DISEASE(c)==3) sd=sd*12/10;
    if(HAS(c,TR_SWIFT)) sd=sd*7/10; if(HAS(c,TR_GIANT)) sd=sd*13/10; if(HAS(c,TR_CHILL)) sd=sd*13/10; if(HAS(c,TR_COUCH)) sd=sd*17/10;
    if(HAS(c,TR_NOCT)||HAS(c,TR_NIGHTCHILD)) sd=night?sd*7/10:sd*12/10;
    if(HAS_STATUS(c,ST_COUGH)) sd=sd*12/10; if(HAS_STATUS(c,ST_MOTIVATED)||HAS_STATUS(c,ST_HANDSOME)) sd=sd*9/10;
    return sd<3?3:sd; }
static void crName(const Cr*c,char*out){      // "ELITE ELANDIL SWIFT HUMAN": rank, personal name (people, ranked monsters, anyone with a record), traits, kind. Never longer than 46.
    char b[32]; out[0]=0;
#define CATN(t) do{ const char*q_=(t); if(strlen(out)+strlen(q_)<=46) strcat(out,q_); }while(0)
    if(c->rk==1) CATN("ELITE "); else if(c->rk==2) CATN("BOSS "); if(crNamed(c)){ crFirst(c,b); CATN(b); CATN(" "); } if(c->tr){ CATN(trNm[c->tr]); CATN(" "); } if(c->tr2){ CATN(trNm[c->tr2]); CATN(" "); }
    if(IS_EVO(c)){ int ea=sp[c->sp].a-8; if(ea>=0&&ea<EVN_N){ strcpy(b,advNm[sp[c->sp].fam]); strcat(b," "); strcat(b,evNm[ea]); } else spName(c->sp,b); } else spName(c->sp,b); CATN(b); if(DISEASE(c)==1&&strlen(out)+8<=46){ memmove(out+8,out,strlen(out)+1); memcpy(out,"PLAGUED ",8); }
#undef CATN
}
static int hitDmg(const Cr*a,const Cr*d){   // one blow: the attacker's strength, gear and traits against the defender's armour and traits
    const Arch*A=&arch[sp[a->sp].a]; int dm=A->atk+gearAtk(a)+(int)(rnd()&3); if(HAS(a,TR_CLEVER)) dm+=gearAtk(a)/2; if(HAS(a,TR_VENOM)) dm+=4; if(HAS(a,TR_GIANT)) dm+=2; if(IS_EVO(a)) dm+=3; dm+=a->dir/2;
    if(HAS(a,TR_LUCKY)&&rnd()%8==0) dm+=6; if(HAS_STATUS(a,ST_MOTIVATED)) dm+=2; if(HAS(a,TR_BRAVE)&&arch[sp[d->sp].a].diet==5) dm+=3;
    if(HAS(a,TR_PACK)){ int n=0; for(int k=0;k<4;k++){ int x=a->x+dxs[k], y=a->y+dys[k]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]&&cr[occ[y][x]-1].sp==a->sp) n++; } dm+=2*n; }
    if(a->rk==1) dm=dm*13/10; else if(a->rk==2) dm=dm*2; if(A->diet==5) dm=dm*(diff==0?70:diff==2?140:100)/100;
    dm-=armDef(d); if(HAS(d,TR_HIDE)) dm-=3; if(HAS(d,TR_ZEN)) dm-=2; if(HAS(d,TR_GIANT)) dm-=1; return dm<1?1:dm; }
static void countTraits(void){ memset(trCount,0,sizeof trCount); for(int i=0;i<nCr;i++) if(cr[i].alive){ if(cr[i].tr) trCount[cr[i].tr]++; if(cr[i].tr2) trCount[cr[i].tr2]++; } }
static void giveTrait(Cr*c,int t,int announce){
    int hadGrin=HAS(c,TR_GRIN);
    if(c->tr==0||c->tr==t) c->tr=(u8)t; else c->tr2=(u8)t;
    if(hadGrin!=HAS(c,TR_GRIN)) joyInvalidate();
    if(!traitMet(t)){ traitMark(t); if(announce){ char nm[48]; crName(c,nm); siprintf(info,"NEW SUBSPECIES: %.30s",nm); fxReq(FX_BIRTH); } }
}
static int sapNow;      // sapient animals alive (kept by denTick): a child inherits the mind only while there are fewer than a hundred
static void born(Cr*kid,const Cr*par){ stBorn++;   // a child has its parent's traits, and now and then a mutation
    kid->tr=par->tr; kid->tr2=par->tr2; if(HAS(kid,TR_GRIN)) joyInvalidate(); int rate=diff==0?1:diff==2?3:2; if(HAS(par,TR_FERTILE)) rate+=1; if(!LAWON(LW_MUTATE)) rate=0; if(LAWON(LW_MUTBOX)) rate=35; else if(LAWON(LW_SPAGHETTI)) rate*=3;
    if((int)(rnd()%100)<rate){ int t=1+(int)(rnd()%(TR_N-1)); if(!HAS(kid,t)){ giveTrait(kid,t,1); stMut++; } }
    if(arch[sp[kid->sp].a].diet==4&&rnd()%100<8) SET_DISEASE(kid,2);      // some children are born immune, as in WorldBox
    kid->hp=(u8)maxHp(kid); ntBirth(kid,par);
    { int ki=(int)(kid-cr), pi=(int)(par-cr); if(arch[sp[kid->sp].a].diet<4){ if(crFl[pi]&FL_SAPIENT){ if(sapNow<100) crFl[ki]|=FL_SAPIENT; } else if(LAWON(LW_EVOLUTION)&&(HAS(par,TR_CLEVER)||HAS(par,TR_FERTILE))&&nCr>200&&rnd()%300==0) crFl[ki]|=FL_SAPIENT; } } }
// ---------------------------------------------------------------------------------------------- the world
static u32 hsh(int x,int y,int s){ u32 h=(u32)x*374761393u+(u32)y*668265263u+(u32)s*2246822519u; h=(h^(h>>13))*1274126177u; return h^(h>>16); }
static int vnoise(int x,int y,int cell,int s){   // value noise 0..255, cell pixels per lattice step
    int gx=x/cell, gy=y/cell, fx=x%cell, fy=y%cell;
    int a=hsh(gx,gy,s)&255, b=hsh(gx+1,gy,s)&255, c=hsh(gx,gy+1,s)&255, d=hsh(gx+1,gy+1,s)&255;
    int t=a+(b-a)*fx/cell, u=c+(d-c)*fx/cell; return t+(u-t)*fy/cell;
}
static int fbm(int x,int y,int s){ return (vnoise(x,y,32,s)*8+vnoise(x,y,16,s+1)*4+vnoise(x,y,8,s+2)*2+vnoise(x,y,4,s+3))/15; }
static int cCoast, cMtn;      // coasts (-1 smooth, 0 as it always was, 1 rugged) and mountains (-1 few, 0 as before, 1 many): 0 makes exactly the planet a seed always made
static int cShape, cSea, cClim, cWet, cLife=2;   // land shape, sea level, climate and wetness (all 0 = the classic planet), and starting life 0..3
// ---- biomes: a second layer over the ground (0 none, 1 CORRUPTED, 2 INFERNAL, 3 ENCHANTED), two bits a tile. They spread, they act on whoever stands there, and they are painted with seed powers.
static u8 biome2[MAXD*MAXD/4]; static int nBio[4];
#define BIO_Q 8192
#define BIO_PAL 168      // the biomes' ground colours in the main palette (168..191)
static u32 bioQ[BIO_Q]; static int bioQn;      // the tiles that can still spread (the edge of each patch): a growing patch costs the same on a small or a huge planet
static inline int bioAt(int x,int y){ return (biome2[(y*MAXD+x)>>2]>>((x&3)*2))&3; }
static void bioSet(int x,int y,int b){ int o=bioAt(x,y); if(o==b) return; u8*p=&biome2[(y*MAXD+x)>>2]; int sh=(x&3)*2; *p=(u8)((*p&~(3<<sh))|(b<<sh)); nBio[o]--; nBio[b]++; if(b&&bioQn<BIO_Q) bioQ[bioQn++]=(u32)(x|(y<<16)); }
static void bioClear(void){ memset(biome2,0,sizeof biome2); memset(nBio,0,sizeof nBio); bioQn=0; }
static void bioCount(void){ memset(nBio,0,sizeof nBio); for(int y=0;y<gH;y++) for(int x=0;x<gW;x++) nBio[bioAt(x,y)]++; }
static int bioOpen(int x,int y){       // can the biome on this tile still take a neighbouring tile?
    int b=bioAt(x,y); static const int ddx[4]={0,1,0,-1}, ddy[4]={-1,0,1,0};
    for(int d=0;d<4;d++){ int nx=x+ddx[d], ny=y+ddy[d]; if(nx<0||ny<0||nx>=gW||ny>=gH) continue; int t=world[ny][nx]; if(t<=T_WATER||t==T_LAVA||t==T_MOUNT) continue; if(bioAt(nx,ny)!=b) return 1; }
    return 0; }
static void bioRescan(void){ bioQn=0; for(int y=0;y<gH;y++) for(int x=0;x<gW;x++) if(bioAt(x,y)&&bioQn<BIO_Q&&bioOpen(x,y)) bioQ[bioQn++]=(u32)(x|(y<<16)); }
static void atmoReset(void);
#include "worldgen.h"
static void genWorldOld(u32 seed){   // (the old generator, replaced by worldgen.h)
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
#include "terrain_palette.h"
static const u8 terrCol[T_N+3][3]=GG_TERRAIN_COLORS;
static int tilePixOld(int t,int v,int x,int y){   // (the old ground art, replaced by gfx2.h)
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
static void shadeRGB(int idx,int*r,int*g,int*b){
    int t=idx/8,k=idx%8;
    // Deliberate RGB ramps retain color in shadows. The sea uses narrow
    // crest steps so palette animation reads as ripples, not flashing noise.
    static const u8 sea[2][8][3]={
        {{2,6,11},{2,8,13},{3,10,16},{4,12,18},{5,13,19},{6,14,20},{7,15,21},{9,17,22}},
        {{4,12,16},{5,15,19},{6,17,21},{7,19,22},{9,21,24},{14,24,26},{18,27,28},{24,30,29}}
    };
    if(t<=T_WATER){*r=sea[t][k][0];*g=sea[t][k][1];*b=sea[t][k][2];return;}
    int f=86+k*18;
    *r=terrCol[t][0]*f/158;*g=terrCol[t][1]*f/158;*b=terrCol[t][2]*f/158;
    if(k<3)*b+=(3-k)/2;else if(k>4){*r+=(k-4)/2;*g+=(k-4)/3;}
    if(*r>31)*r=31;if(*g>31)*g=31;if(*b>31)*b=31;
}
// buildings: B_*; a building tile is ground (grass) with the building on it, in the colours of the people who built it
#include "building_types.h"
#define BT0 (T_N*4)
#define BT1 124      // the tiles of the later buildings (dock .. library, 8 a type: 124..155), after the meadow tiles at 108..123
#define BTILE(b,r) ((b)<B_DOCK?BT0+(b)*8+(r):BT1+((b)-B_DOCK)*8+(r))
static const u8 roofCol[8][3]={ {25,11,8},{11,21,15},{12,17,25},{16,11,10},{25,23,12},{27,17,10},{20,14,25},{18,14,10} };   // human elf dwarf orc goblin halfling gnome ogre
static const u8 bldCol[8][3]={ {29,26,19},{12,9,7},{14,12,12},{23,24,24},{14,25,11},{15,11,8},{13,15,15},{31,30,24} };   // wall, dark wood, door, stone, crop, soil, ruin, flag
#include "gfx2.h"
static int bldPixOld(int type,int race,int x,int y){   // (the old building art)
    int bg=tilePix(T_GRASS,(x+y)&3,x,y), roof=128+race, wall=136, dark=137, door=138, stone=139, crop=140, soil=141, ruin=142, flag=143;
    switch(type){
    case B_HUT: if(y>=1&&y<=3){ int h=y; if(x>=4-h&&x<=3+h) return roof; } if(y>=4&&y<=6&&x>=2&&x<=5) return (y>=5&&x>=3&&x<=4)?door:wall; break;
    case B_HOUSE: if(y>=1&&y<=3){ if(x>=3-y&&x<=4+y) return roof; } if(y>=4&&y<=7&&x>=1&&x<=6) return (y>=5&&x>=3&&x<=4)?door:(y==5&&(x==1||x==6))?dark:wall; break;
    case B_HALL: if(y<=2&&x==2) return dark; if(y<=1&&x>=3&&x<=5) return flag; if(y==2&&(x&1)==0&&x>=1&&x<=7) return stone; if(y>=3) return (y>=5&&x>=3&&x<=4)?dark:((x==1||x==6)&&y==4)?dark:stone; break;
    case B_FARM: if(x==0||x==7||y==0||y==7) return (x+y)&1?dark:bg; return (y&1)?crop:soil;
    case B_TOWER: if(y==1&&(x==2||x==3||x==5)) return stone; if(y>=2&&x>=2&&x<=5) return (y==4&&x==3)?dark:stone; break;
    case B_DOCK: if(y>=3&&y<=4) return (x&1)?door:wall; if(y>=5&&y<=6&&(x==1||x==6)) return door; break;
    case B_TEMPLE: if(y<=1&&x>=3&&x<=4) return flag; if(y==2&&x>=2&&x<=5) return roof; if(y==3&&x>=1&&x<=6) return roof; if(y>=4&&y<=7&&x>=1&&x<=6) return (y>=6&&x>=3&&x<=4)?door:((x&1)&&y<7)?stone:wall; break;
    case B_STATUE: if(y>=6&&x>=2&&x<=5) return stone; if(y>=2&&y<=5&&x>=3&&x<=4) return stone; if(y==1&&x>=3&&x<=4) return wall; if(y==3&&(x==2||x==5)) return stone; break;
    case B_LIBRARY: if(y>=1&&y<=2&&x>=1&&x<=6) return roof; if(y>=3&&y<=7&&x>=1&&x<=6) return (y>=6&&x>=3&&x<=4)?door:((y==4||y==5)&&(x==2||x==5))?flag:wall; break;
    case B_RUIN: if((hsh(x,y,17)&3)==0) return ruin; if((hsh(x,y,18)&7)==1) return stone; break;
    case B_BORDER: if(y>=3&&y<=6&&(x==3||x==4)) return dark; if(y==2&&(x==3||x==4)) return flag; break; }
    return bg;
}
#ifndef PCTEST
static void vramCopy(void*dst,const void*src,int bytes){ u16*d=(u16*)dst; const u16*s=(const u16*)src; for(int i=0;i<bytes/2;i++) d[i]=s[i]; }   // 16 bit stores only
static void makeTiles(void){
    for(int t=0;t<T_N;t++) for(int k=0;k<8;k++){ int r,g,b; shadeRGB(t*8+k,&r,&g,&b); BG_PALETTE[t*8+k]=RGB15(r,g,b); }
    for(int b=0;b<3;b++) for(int k=0;k<8;k++){ int r,g,bl; shadeRGB((T_N+b)*8+k,&r,&g,&bl); BG_PALETTE[BIO_PAL+b*8+k]=RGB15(r,g,bl); }      // the three biomes' ground (168..191)
    BG_PALETTE[0]=RGB15(0,0,0); gfxBgPal();
    for(int r=0;r<8;r++) BG_PALETTE[128+r]=RGB15(roofCol[r][0],roofCol[r][1],roofCol[r][2]);
    for(int k=0;k<8;k++) BG_PALETTE[136+k]=RGB15(bldCol[k][0],bldCol[k][1],bldCol[k][2]);
    // (tiles are built in RAM and copied 16 bits at a time: the DS ignores byte writes to VRAM)
    static u8 tl[64] __attribute__((aligned(4))); u8*bb=(u8*)BG_TILE_RAM(1);
    for(int ty=1;ty<B_N;ty++) for(int r=0;r<8;r++){ for(int y=0;y<8;y++) for(int x=0;x<8;x++) tl[y*8+x]=(u8)bldPix(ty,r,x,y); vramCopy(bb+BTILE(ty,r)*64,tl,64); }
    for(int t=0;t<T_N;t++) for(int v=0;v<4;v++){ for(int y=0;y<8;y++) for(int x=0;x<8;x++) tl[y*8+x]=(u8)tilePix(t,v,x,y); vramCopy(bb+(t*4+v)*64,tl,64); }
    for(int m=0;m<GFX_MEADOW_N;m++){ for(int y=0;y<8;y++) for(int x=0;x<8;x++) tl[y*8+x]=(u8)pxMeadow(m,x,y); vramCopy(bb+(GFX_MEADOW0+m)*64,tl,64); }
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
#include "residential_plots.h"
static int ggFortificationsPresent;
static u8 vilRace(int v);
// Transition tiles: where two kinds of ground meet, the "stronger" one bleeds a dithered fringe into the other, and water gets a foam line on the shore.
// They are made on demand (64 bytes each) into spare tile slots and remembered, so a window of the map needs only a few dozen of them.
#define TR0 (BT1+(B_N-B_DOCK)*8)
#define TR_SLOTS (640-TR0) // terrain cache ends at the start of the toolbar
_Static_assert(TR0>BTILE(B_N-1,7)&&TR_SLOTS>256,"world tile memory layout");
static const u8 terrRank[T_N]={0,10,3,1,5,6,7,8,9,3,4,5,2};
static u32 trKey[1024]; static u16 trVal[1024]; static int trUsed;
static inline int bioPix(int p,int b){ return (b&&p<T_N*8)?BIO_PAL+(b-1)*8+(p&7):p; }      // a pixel of ground, in the colours of biome b   // a small hash of the transition tiles made so far
static int edgePix(int t,int n,int mask,int v,int foam,int x,int y){   // one pixel of a transition tile: ground t with a dithered fringe of n along the sides in mask (or a foam line on a shore)
    int px=tilePix(t,v,x,y); int d=9;
    if(mask&1) if(y<d) d=y; if(mask&2) if(7-x<d) d=7-x; if(mask&4) if(7-y<d) d=7-y; if(mask&8) if(x<d) d=x;
    int along=(mask&5)?x:y;
    // Continuous contour with occasional two-pixel breaks. A darker wet
    // band separates surf from open sea; no glittering random white dots.
    if(foam){
        if(d==0)px=SH(T_WATER,7);
        else if(d==1)px=SH(T_WATER,((along+v)&7)<5?6:4);
        else if(d==2)px=SH(T_WATER,2);
    }else if(n<T_N){
        // Clustered feathering preserves each biome's authored motif,
        // rather than choosing a new tree/rock variant for every pixel.
        int fringe=((along+v*3)&7)<3?2:1;
        if(d<fringe)px=tilePix(n,v,x,y);
        else if(d==fringe&&((along+v)&3)==0)px=tilePix(n,v,x,y);
    }
    return px; }
static int cellEdge(int x,int y,int*mask,int*foam){   // the ground that bleeds into the cell (x,y) (-1: none), which sides it touches, and whether it is a shore
    int t=world[y][x]; int nb[4]; nb[0]=y>0?world[y-1][x]:t; nb[1]=x<WW-1?world[y][x+1]:t; nb[2]=y<WH-1?world[y+1][x]:t; nb[3]=x>0?world[y][x-1]:t;
    int best=-1, br=-1, fm=0, wat=t<=T_WATER;
    for(int i=0;i<4;i++){ int n=nb[i]; if(n==t) continue; if(wat&&n>T_WATER){ fm=1; best=n; continue; } if(!wat&&terrRank[n]>terrRank[t]&&terrRank[n]>br){ br=terrRank[n]; best=n; } }
    int m=0; if(best>=0) for(int i=0;i<4;i++) if(fm?(nb[i]>T_WATER):(nb[i]==best)) m|=1<<i;
    *mask=m; *foam=fm; return best; }
static int trTile(int t,int n,int mask,int v,int foam,int b){
    u32 key=(u32)(((((b*16+t)*16+n)*16+mask)*4+v)*2+foam); unsigned hi=(unsigned)(key*2654435761u>>22)&1023; while(trKey[hi]){ if(trKey[hi]==key+1) return trVal[hi]; hi=(hi+1)&1023; }
    if(trUsed>=TR_SLOTS) return -1;
    static u8 tl[64] __attribute__((aligned(4)));
    for(int y=0;y<8;y++) for(int x=0;x<8;x++) tl[y*8+x]=(u8)bioPix(edgePix(t,n,mask,v,foam,x,y),b);
    int slot=TR0+trUsed++;
#ifndef PCTEST
    vramCopy((u8*)BG_TILE_RAM(1)+slot*64,tl,64);
#endif
    trKey[hi]=key+1; trVal[hi]=(u16)slot; return slot; }
// These zoom numbers are part of the shared save format.
enum { Z_PLANET, Z_VFAR, Z_FAR, Z_NORMAL, Z_CLOSE, Z_MICRO };
// The spherical planet view is a shared console feature, not a
// New3DS-only decoration. DS/DSi use the already present 2x sampling
// path and a smaller maximum disc to preserve ARM9 drawing headroom.
// Host game tests keep their historical zoom contract unless they
// explicitly opt in to the restored portable-globe regression.
#if defined(__3DS__) || defined(GG_LEGACY_GLOBE_TEST) || \
    defined(GG_PORTABLE_GLOBE_TEST) || defined(NDSHW) || \
    (defined(ARM9) && !defined(NDSSHIM))
#define GG_HAS_GLOBE 1
enum { Z_MIN=Z_PLANET };
#else
enum { Z_MIN=Z_VFAR };
#endif
static int zl=Z_NORMAL;
static int zBmp=0;       // 1 while the background holds the zoom picture (a bitmap in tiles) instead of the map's own tiles
static inline void putCell(int x,int y){   // world cell -> background entry (a 64 x 64 torus of 8 pixel tiles)
    if(zBmp) return;
    int mx=x&63, my=y&63; int blk=(my>=32?2:0)+(mx>=32?1:0); int t=world[y][x], v=tileVar(t,x,y); u16 e=(u16)(t*4+v); if(t==T_GRASS&&v==2) e=(u16)(GFX_MEADOW0+(int)(hsh(x,y,5)&15));
    if(bld[y][x]) e=(u16)BTILE(bld[y][x],(bldv[y][x]?vilRace(bldv[y][x]-1):0));
    else { int mask,foam; int best=cellEdge(x,y,&mask,&foam); int b=(t>T_WATER&&t!=T_LAVA)?bioAt(x,y):0; if(best>=0||b){ int k=trTile(t,best>=0?best:15,best>=0?mask:0,v,best>=0?foam:0,b); if(k>=0) e=(u16)k; } }
    mapPtr[blk*1024+(my&31)*32+(mx&31)]=e; }
static void refillWindow(void){
    if(zBmp) return;
    int x0=camX>>3, y0=camY>>3;
    if(x0==winX&&y0==winY) return;
    int oldX=winX, oldY=winY;
    winX=x0;
    winY=y0;
    // Adjacent camera tiles share almost the entire map window. Only the
    // newly exposed rows and columns need their terrain/transition tiles.
    if(oldX>=0&&oldY>=0&&abs(x0-oldX)<34&&abs(y0-oldY)<26){
        for(int y=y0-1;y<y0+26&&y<WH;y++){
            if(y<0) continue;
            if(y<oldY-1||y>=oldY+26){
                for(int x=x0-1;x<x0+34&&x<WW;x++) if(x>=0) putCell(x,y);
            } else {
                for(int x=x0-1;x<oldX-1&&x<x0+34&&x<WW;x++) if(x>=0) putCell(x,y);
                for(int x=oldX+34>x0-1?oldX+34:x0-1;x<x0+34&&x<WW;x++) if(x>=0) putCell(x,y);
            }
        }
        return;
    }
    for(int y=y0-1;y<y0+26&&y<WH;y++)
        for(int x=x0-1;x<x0+34&&x<WW;x++)
            if(x>=0&&y>=0) putCell(x,y);
}
static void setCell(int x,int y,u8 t){
    if(x<0||y<0||x>=WW||y>=WH)return;
    world[y][x]=t;
    if(t<=T_WATER||t==T_LAVA||t==T_MOUNT){
        bioSet(x,y,0);
        if(ggBuildingIsPaving(bld[y][x]))bld[y][x]=bldv[y][x]=0;
    }
    for(int k=0;k<5;k++){
        int xx=x+(k==1)-(k==2),yy=y+(k==3)-(k==4);
        if(xx<0||yy<0||xx>=WW||yy>=WH)continue;
        if(xx>=winX-1&&xx<winX+34&&yy>=winY-1&&yy<winY+26)putCell(xx,yy);
    }
}

// ---------------------------------------------------------------------------------------------- creatures
static int curExtra;      // the extra ground the creature being moved can cross (its traits)
static int walkable(const Sp*s,int x,int y){ if(x<0||y<0||x>=WW||y>=WH) return 0; return !((ggFortificationsPresent)&&ggBuildingStopsPawn(bld[y][x]))&&(((arch[s->a].mask|curExtra)>>world[y][x])&1); }
static int edgeOut;     // a creature tried to walk off the map (EDGE = VANISH)
static int wgNc;      // (10d: how many creatures the war groups were last counted from; 0 when the lists are out of date)
static int wgKeep; static u8 wgOf[MAXC];      // (10f: wgOf, the group each creature belonged to (village +1), is carried through compact(); wgKeep: how many creatures at the front of the list still have a good wgOf after it)
#include "ggcell.h"      // (10c: the 8 x 8 cell grid for finding enemies; kept in step with occ[])
static int stepOk(const Sp*s,int*nx,int*ny){   // can the creature step to (*nx,*ny)? Off the map it wraps round (EDGE 2), leaves the world (1) or is stopped (0)
    int x=*nx, y=*ny;
    if(x<0||y<0||x>=WW||y>=WH){ if(cEdge==2){ x=(x+WW)%WW; y=(y+WH)%WH; *nx=x; *ny=y; } else { if(cEdge==1) edgeOut=1; return 0; } }
    return walkable(s,x,y)&&!occ[y][x]; }
static int addCr(int s,int x,int y){
    if(nCr>=MAXC||x<0||y<0||x>=WW||y>=WH||occ[y][x]) return -1; if(!walkable(&sp[s],x,y)) return -1;
    Cr*c=&cr[nCr]; memset(c,0,sizeof *c); c->sp=(u16)s; c->x=(coord_t)x; c->y=(coord_t)y; c->hp=(u8)maxHp(c); c->food=70; c->age=(u8)(rnd()&15); c->t=(u8)(rnd()&15); c->alive=1; occ[y][x]=(occ_t)(nCr+1); cellAdd(x,y,s); crId[nCr]=idNext++; crFl[nCr]=0;
    if(arch[sp[s].a].diet==5&&!((seenB[s>>3]>>(s&7))&1)){ seenB[s>>3]|=(u8)(1<<(s&7)); seenN++; char nm[32]; spName(s,nm); siprintf(info,"BESTIARY: %s",nm); fxReq(FX_MONSTER_SEEN); }
    return nCr++;
}
static void logEv(const char*s);
// Movement and removal both change the neighborhood used for village mood.
static inline void leaveTile(const Cr*c){
    if(HAS(c,TR_GRIN)) joyInvalidate();
    occ[c->y][c->x]=0;
    cellDel(c->x,c->y,c->sp);
}
static coord_t zq[16][2]; static int zqN;      // where plague victims bitten by a zombie fell: they rise again at the end of the tick (calamity.h)
#define FL_SLAYER 0x0020      /* killed a boss (the bit ggid.h reserved for a ruler mark, which nothing uses) */
static int lastKillRk;      // rank of what was last killed (killCr sets it): a killer of a boss earns the SLAYER mark
static void lootDrop(int mi); static int ghostFor(int i); static void killCr(int i){ Cr*c=&cr[i]; if(!c->alive||HAS_STATUS(c,ST_INVINCIBLE)) return; lastKillRk=c->rk; if(c->rk>=1&&arch[sp[c->sp].a].diet==5) lootDrop(i); ghostFor(i); ntDeath(i); if(c->rk==2&&!natural){ char nm[48], b[40]; crName(c,nm); siprintf(b,"%.28s FALLS",nm); logEv(b); } c->alive=0; stDied++; if(DISEASE(c)==3&&arch[sp[c->sp].a].diet!=5&&zqN<16){ zq[zqN][0]=c->x; zq[zqN][1]=c->y; zqN++; } if(c->rk==1&&!natural&&(rnd()&3)==0){ char nm[48], b2[40]; crName(c,nm); siprintf(b2,"%.24s FALLS",nm); logEv(b2); } if(occ[c->y][c->x]==i+1) leaveTile(c);
    if(!natural&&arch[sp[c->sp].a].diet==5){ stKill++; if(!((slainB[c->sp>>3]>>(c->sp&7))&1)) slainB[c->sp>>3]|=(u8)(1<<(c->sp&7)); fxReq(FX_MONSTER_SLAIN); } }
static int damageCreature(int i,int dmg,int attacker){
    if(i<0||i>=nCr||!cr[i].alive) return 0; Cr*v=&cr[i]; if(HAS_STATUS(v,ST_INVINCIBLE)) return 1;
    if(HAS(v,TR_WHIRL)&&rnd()%5==0) return 1; if(HAS(v,TR_BUBBLE)) dmg=(dmg+1)/2; if(dmg<1) dmg=1;
    if(dmg>=v->hp&&HAS(v,TR_MIRACLE)&&rnd()%8==0){ int h=maxHp(v)/3; v->hp=(u8)(h<1?1:h); return 1; }
    if(HAS(v,TR_THORNS)&&attacker>=0&&attacker<nCr&&attacker!=i&&cr[attacker].alive){ int reflected=dmg/4; if(reflected<1) reflected=1; damageCreature(attacker,reflected,-1); }
    if(dmg>=v->hp){ killCr(i); return cr[i].alive!=0; } v->hp=(u8)(v->hp-dmg); return 1;
}
static void killNat(int i){ natural=1; killCr(i); natural=0; }     // old age, hunger, lava: not a slaying
static void compactCreatures(int rebuildGrid){
    int j=0, keep=0, oldGroupCount=wgNc;
    for(int i=0;i<nCr;i++){
        if(!cr[i].alive){ if(crFl[i]&0x0080) ntGone(crId[i]); continue; }
        if(i!=j){
            cr[j]=cr[i]; crId[j]=crId[i]; crFl[j]=crFl[i];
            if(i<oldGroupCount) wgOf[j]=wgOf[i];
            // Movement and death maintain the live grid. Compaction changes
            // the occupant's index, but leaves its position and cell counts intact.
            if(!rebuildGrid) occ[cr[j].y][cr[j].x]=(occ_t)(j+1);
        }
        if(i<oldGroupCount) keep=j+1;
        j++;
    }
    nCr=j;
    if(rebuildGrid){
        memset(occ,0,sizeof occ);
        for(int i=0;i<nCr;i++) occ[cr[i].y][cr[i].x]=(occ_t)(i+1);
        cellRebuild();
    }
    wgKeep=keep;
    wgNc=0;
}
static void compact(void){ compactCreatures(1); }

static int glideOn=1;      // some creature may have a pixel offset to glide off (set by moveTo and the camera; simulate() only walks the screen when it is set)
static void moveTo(Cr*c,int i,int nx,int ny){ glideOn=1; leaveTile(c); { int ddx=c->x-nx, ddy=c->y-ny; if(ddx>1||ddx<-1) ddx=0; if(ddy>1||ddy<-1) ddy=0; c->ox=(s8)(ddx*8); c->oy=(s8)(ddy*8); } c->x=(coord_t)nx; c->y=(coord_t)ny; occ[ny][nx]=(occ_t)(i+1); cellAdd(nx,ny,c->sp); }
// ---- levels: a creature that kills grows stronger (+5 health and +1 damage every two levels, up to level 10), after the wiki's unit levels; Cr.dir holds the level
static void zombieBite(const Cr*a,int j){      // a zombie's hit infects half of the living it touches (not the immune, not monsters); the infected fall and rise as zombies
    const Arch*A=&arch[sp[a->sp].a]; if(A->diet!=5||strcmp(A->nm,"ZOMBIE")||gx.lPlague==1) return;
    if(!cr[j].alive||arch[sp[cr[j].sp].a].diet==5||DISEASE(&cr[j])==2||DISEASE(&cr[j])==3) return; if(rnd()&1) SET_DISEASE(&cr[j],3); }
static void gainXp(Cr*c){ if(lastKillRk==2&&arch[sp[c->sp].a].diet==4&&!(crFl[c-cr]&FL_SLAYER)){ crFl[c-cr]|=FL_SLAYER; char nm[48], b[48]; crName(c,nm); siprintf(b,"%.22s SLAYS A BOSS",nm); logEv(b); } ntKill(c); int L=c->dir; if(L>=10) return; if((int)(rnd()%(unsigned)(L*2+3))==0){ c->dir=(u8)(L+1); if(c->hp<250) c->hp=(u8)(c->hp+8); if(c->dir%5==0&&(c->rk||arch[sp[c->sp].a].diet==4)){ char nm[48], b[40]; crName(c,nm); siprintf(b,"%.20s REACHES LV %d",nm,c->dir); logEv(b); } } }
// ---------------------------------------------------------------------------------------------- villages
#define VIL_OLD 24     // older saves held 24 villages
typedef struct { u8 on, race; coord_t x, y; u8 lvl, houses, farms, towers, foe, dead, nm1, nm2; u16 pop, stock; } Vil;   // foe: the village (+1) it is at war with
static Vil vil[VIL_N]; static int nVilOn;
static u32 kdEp=1; /* kingdom answer cache epoch: bumped by everything a kingdom answer reads (villages, kingdoms) */
#define KD_DIRTY() (kdEp++)
static u8 vilK[VIL_N]; static s8 vilL[VIL_N];      // a village's kingdom (+1) and its loyalty to it
static u8 vilSick[VIL_N];      // the plagued folk around the village at its last look (the priests of a temple cure half of them)
static u8 vilBf[VIL_N];        // bit 0: the village has a temple, bit 1: a statue, bit 2: a library (worked out from the map, so a loaded game goes on as it was)
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
static int aggressive(int race){ int t=gxTemper(race); if(t!=1) return t==2; return race==3||race==4||race==7; }   // the orcs, the goblins and the ogres go looking for trouble
// Keep live villages grouped by race, in slot order. This avoids walking every
// empty village slot for each creature decision without changing tie breaks.
#if VIL_N>255
#error vilByRace needs a wider village index
#endif
static u8 vilByRace[VIL_N], vilRaceOff[9]; static int vilIndexValid;
static void vilIndexInvalidate(void){ vilIndexValid=0; }
static void vilIndexBuild(void){ u8 n[8]={0};
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].race<8) n[vil[v].race]++;
    int total=0; for(int a=0;a<8;a++){ vilRaceOff[a]=(u8)total; total+=n[a]; n[a]=(u8)vilRaceOff[a]; } vilRaceOff[8]=(u8)total;
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].race<8) vilByRace[n[vil[v].race]++]=(u8)v;
    vilIndexValid=1;
}
static int vilNear(int race,int x,int y,int r){ int best=-1,bd=r*r+1;
    if((unsigned)race>=8){ for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].race==race){ int dx=vil[v].x-x, dy=vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } } return best; }
    if(!vilIndexValid) vilIndexBuild();
    for(int q=vilRaceOff[race];q<vilRaceOff[race+1];q++){ int v=vilByRace[q]; int dx=vil[v].x-x, dy=vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } }
    return best; }
static int vilAny(int x,int y,int r){ int best=-1,bd=r*r+1; for(int v=0;v<VIL_N;v++){ if(!vil[v].on) continue; int dx=vil[v].x-x, dy=vil[v].y-y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=v; } } return best; }
static int buildable(int x,int y){ if(x<2||y<2||x>=WW-2||y>=WH-2||bld[y][x]) return 0; u8 t=world[y][x]; return t==T_GRASS||t==T_DIRT||t==T_FOREST; }
static void putBld(int v,int x,int y,int type){ if(world[y][x]==T_FOREST&&!ggBuildingIsPaving(type)) setCell(x,y,T_GRASS); bld[y][x]=(u8)type; bldv[y][x]=(u8)(v+1); if(ggBuildingStopsPawn(type)||ggBuildingIsGate(type))ggFortificationsPresent=1; if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); }
static int ggFortProtectedPlot(int x,int y){
    if(!ggFortificationsPresent||x<1||y<1||x>=WW-1||y>=WH-1)return 0;
    if(bld[y-1][x]==B_GATE_NS||bld[y+1][x]==B_GATE_NS||
       bld[y][x-1]==B_GATE_EW||bld[y][x+1]==B_GATE_EW)return 1;
    for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++)
        if(bld[y+dy][x+dx]==B_KEEP)return 1;
    return 0;
}
static int ggOpenBuildingPlot(int x,int y){
    return buildable(x,y)&&!occ[y][x]&&!ggFortProtectedPlot(x,y);
}
static int ggResidenceWingPlan(int x,int y,int type){
    int east=ggOpenBuildingPlot(x+1,y),south=ggOpenBuildingPlot(x,y+1);
    if(type==B_MANOR)return east&&south&&ggOpenBuildingPlot(x+1,y+1)?B_ESTATE:B_NONE;
    if(!east)return south?B_HOUSE_NS:B_NONE;
    if(!south)return B_HOUSE_EW;
    int streetNS=world[y][x-1]==T_DIRT||world[y][x+2]==T_DIRT;
    int streetEW=world[y-1][x]==T_DIRT||world[y+2][x]==T_DIRT;
    if(streetNS!=streetEW)return streetNS?B_HOUSE_NS:B_HOUSE_EW;
    return (hsh(x,y,81)&1)?B_HOUSE_EW:B_HOUSE_NS;
}
static int ggResidencePlan(int x,int y,int type){
    if((type!=B_HOUSE&&type!=B_MANOR)||!ggOpenBuildingPlot(x,y))return B_NONE;
    return ggResidenceWingPlan(x,y,type);
}
static void ggWriteResidence(int owner,int x,int y,int plan){
    KD_DIRTY();
    putBld(owner,x,y,plan);
    if(plan!=B_HOUSE_NS)putBld(owner,x+1,y,B_WING_E);
    if(plan!=B_HOUSE_EW)putBld(owner,x,y+1,B_WING_S);
    if(plan==B_ESTATE)putBld(owner,x+1,y+1,B_WING_SE);
}
static int ggPlaceResidence(int owner,int x,int y,int type){
    int plan=ggResidencePlan(x,y,type);
    if(!plan)return 0;
    ggWriteResidence(owner,x,y,plan);
    return 1;
}
static int placeBld(int v,int type,int cost){
    Vil*V=&vil[v]; if(V->stock<cost) return 0; int R=3+V->lvl*3;
    for(int k=0;k<14;k++){ int x=V->x+(int)(rnd()%(2*R+1))-R, y=V->y+(int)(rnd()%(2*R+1))-R; if(!ggOpenBuildingPlot(x,y)) continue; if(ggBuildingIsHome(type)){ int near=0; for(int d=0;d<4;d++) if(bld[y+dys[d]][x+dxs[d]]) near=1; if(!near&&V->houses>1) continue; }
        if(type==B_HOUSE||type==B_MANOR){ if(!ggPlaceResidence(v,x,y,type))continue; }
        else putBld(v,x,y,type);
        V->stock-=(u16)cost; return 1; }
    return 0;
}
#include "settlement_buildings.h"
// ---- boats and docks (after the wiki): a town of some size builds a dock on its shore; now and then a boat carries settlers across the water to a new shore and they found a village there
typedef struct { u8 on, race, n, i; s16 x, y, tx, ty; u16 t; u8 path[128]; } Boat; static Boat boat[8];      // path: the steps (0 up, 1 right, 2 down, 3 left) found when it set sail
static void bldFlags(int v){ const Vil*V=&vil[v]; int f=0; for(int dy=-12;dy<=12;dy++) for(int dx=-12;dx<=12;dx++){ int x=V->x+dx, y=V->y+dy; if(x>=0&&y>=0&&x<WW&&y<WH&&bldv[y][x]==v+1) f|=ggBuildingFlag(bld[y][x]); } vilBf[v]=(u8)f; }
static int hasDock(int v){ const Vil*V=&vil[v]; for(int dy=-9;dy<=9;dy++) for(int dx=-9;dx<=9;dx++){ int x=V->x+dx, y=V->y+dy; if(x>=0&&y>=0&&x<WW&&y<WH&&bld[y][x]==B_DOCK&&bldv[y][x]==v+1) return 1; } return 0; }
static int isWater(int x,int y){ return x>=0&&y>=0&&x<WW&&y<WH&&(world[y][x]==T_WATER||world[y][x]==T_DEEP); }
static int vilFound(int race,int x,int y);
static void dockTry(int v){      // a dock: a land tile of the village beside water
    Vil*V=&vil[v]; for(int k=0;k<40;k++){ int x=V->x+(int)(rnd()%17)-8, y=V->y+(int)(rnd()%17)-8; if(x<2||y<2||x>=WW-2||y>=WH-2||bld[y][x]||world[y][x]<=T_WATER||world[y][x]==T_LAVA||ggFortProtectedPlot(x,y)) continue;
        if(!(isWater(x+1,y)||isWater(x-1,y)||isWater(x,y+1)||isWater(x,y-1))) continue; bld[y][x]=B_DOCK; bldv[y][x]=(u8)(v+1); if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); return; } }
#ifdef GG_BIGMAP
#define BW 121      // the sea is searched in a window of BW x BW tiles round the dock (the DS gets a smaller one: its RAM is tight)
#else
#define BW 81
#endif
static u8 bfsDir[BW*BW]; static u16 bfsQ[BW*BW];
static void boatLaunch(int v){
    Vil*V=&vil[v]; int nb=0; for(int b=0;b<8;b++) nb+=boat[b].on; if(nb>=8) return; int bi=-1; for(int b=0;b<8;b++) if(!boat[b].on){ bi=b; break; }
    int sx=-1, sy=-1; for(int dy=-9;dy<=9&&sx<0;dy++) for(int dx=-9;dx<=9&&sx<0;dx++){ int x=V->x+dx, y=V->y+dy; if(x>=0&&y>=0&&x<WW&&y<WH&&bld[y][x]==B_DOCK&&bldv[y][x]==v+1){ for(int d=0;d<4;d++) if(isWater(x+dxs[d],y+dys[d])){ sx=x+dxs[d]; sy=y+dys[d]; break; } } } if(sx<0) return;
    // a search across the water for the nearest shore at least 12 tiles away where nobody lives yet
    int ox=sx-BW/2, oy=sy-BW/2; memset(bfsDir,255,sizeof bfsDir); int qh=0, qt=0; bfsQ[qt++]=(u16)((BW/2)*BW+BW/2); bfsDir[(BW/2)*BW+BW/2]=4; int goal=-1, gtx=0, gty=0, rot=(int)(rnd()&3);
    while(qh<qt&&goal<0){ int c=bfsQ[qh++]; int cx=c%BW, cy=c/BW; int wx=ox+cx, wy=oy+cy;
        if(abs(wx-V->x)+abs(wy-V->y)>=12) for(int k=0;k<4;k++){ int d=(k+rot)&3; int tx=wx+dxs[d], ty=wy+dys[d]; if(buildable(tx,ty)&&!ggFortProtectedPlot(tx,ty)&&vilAny(tx,ty,9)<0){ goal=c; gtx=tx; gty=ty; break; } }
        if(goal>=0) break;
        for(int k=0;k<4;k++){ int d=(k+rot)&3; int nx=cx+dxs[d], ny=cy+dys[d]; if(nx<0||ny<0||nx>=BW||ny>=BW||bfsDir[ny*BW+nx]!=255||!isWater(ox+nx,oy+ny)) continue; bfsDir[ny*BW+nx]=(u8)d; bfsQ[qt++]=(u16)(ny*BW+nx); } }
    if(goal<0) return;
    u8 rev[128]; int n=0; for(int c=goal;c!=(BW/2)*BW+BW/2;){ int d=bfsDir[c]; if(d>3||n>=128) return; rev[n++]=(u8)d; c=(c/BW-dys[d])*BW+(c%BW-dxs[d]); }
    int taken=0; for(int i=0;i<nCr&&taken<4;i++){ Cr*c=&cr[i]; if(c->alive&&sp[c->sp].a==V->race&&abs(c->x-V->x)<9&&abs(c->y-V->y)<9){ c->alive=0; if(occ[c->y][c->x]==i+1) leaveTile(c); taken++; } } if(taken<2) return;
    Boat*B=&boat[bi]; memset(B,0,sizeof *B); B->on=1; B->race=V->race; B->x=(s16)sx; B->y=(s16)sy; B->tx=(s16)gtx; B->ty=(s16)gty; B->n=(u8)n; for(int k=0;k<n;k++) B->path[k]=rev[n-1-k]; }
static void boatTick(void){
    for(int b=0;b<8;b++){ Boat*B=&boat[b]; if(!B->on||(tick&3)) continue;
        if(B->i>=B->n){ int v=vilFound(B->race,B->tx,B->ty); if(v>=0){ for(int q=0;q<4;q++) addCr(B->race,B->tx+(int)(rnd()%5)-2,B->ty+(int)(rnd()%5)-2); char vn[24], s2[48]; vilName(v,vn); siprintf(s2,"SETTLERS SAIL TO %.20s",vn); logEv(s2); } B->on=0; continue; }
        int d=B->path[B->i++]; B->x=(s16)(B->x+dxs[d]); B->y=(s16)(B->y+dys[d]); } }
static int vilFound(int race,int x,int y){
    KD_DIRTY(); if(!buildable(x,y)||ggFortProtectedPlot(x,y)) return -1; if(vilAny(x,y,9)>=0) return -1; int v=-1; for(int i=0;i<VIL_N;i++) if(!vil[i].on){ v=i; break; } if(v<0) return -1;
    Vil*V=&vil[v]; memset(V,0,sizeof *V); gxVilReset(v); vilBf[v]=0; vilSick[v]=0; V->on=1; V->race=(u8)race; V->x=(coord_t)x; V->y=(coord_t)y; V->lvl=1; V->stock=30; V->nm1=(u8)(rnd()%12); V->nm2=(u8)(rnd()%8);
    putBld(v,x,y,B_HALL); for(int k=0;k<2;k++) placeBld(v,B_HUT,0); nVilOn++; joyInvalidate(); vilIndexInvalidate(); return v;
}
static const char* vilWhyNot(int x,int y){      // the reason vilFound said no (for the message under a SETTLE touch)
    if(x<2||y<2||x>=WW-2||y>=WH-2) return "TOO NEAR THE MAP EDGE"; if(bld[y][x]) return "SOMETHING IS BUILT HERE"; if(ggFortProtectedPlot(x,y)) return "KEEP THE GATE APPROACH CLEAR"; { u8 t=world[y][x]; if(t!=T_GRASS&&t!=T_DIRT&&t!=T_FOREST) return "NEEDS GRASS, DIRT OR FOREST"; }
    if(vilAny(x,y,9)>=0) return "TOO NEAR ANOTHER VILLAGE"; return "TOO MANY VILLAGES"; }
static void vilRaze(int v,int ruins){ Vil*V=&vil[v]; if(!V->on) return; KD_DIRTY(); for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(bldv[y][x]==v+1){ bld[y][x]=ruins&&(rnd()&1)?B_RUIN:B_NONE; if(!bld[y][x]) bldv[y][x]=0; else bldv[y][x]=0; if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); }
    for(int u=0;u<VIL_N;u++) if(vil[u].on&&vil[u].foe==v+1) vil[u].foe=0; V->on=0; nVilOn--; vilBf[v]=0; vilSick[v]=0; joyInvalidate(); vilIndexInvalidate(); }
// ---- the atmosphere (after SimEarth and its Gaia model): oxygen, carbon dioxide, dust and the world's warmth, all in 1/16 units. Forests and grass breathe CO2 in and O2 out; crowds, cities, fires, lava
// and bombs do the opposite; CO2 warms (the greenhouse), dust and snow cool. Too hot melts the ice and dries the land, too cold freezes it, thin air suffocates, rich air feeds fires.
static int atO2=21*16, atCO2=30*16, atDust=0, atTemp=0, atFire=0, atFlag=0; static s8 atHist[48]; static int atHistN;
#define ATV(v) (((v)+8)>>4)
static void atmoReset(void){ atO2=21*16; atCO2=30*16; atDust=0; atTemp=0; atFire=0; atFlag=0; memset(atHist,0,sizeof atHist); atHistN=0; }
static void atmoDust(int d){ atDust+=d*16; if(atDust<0) atDust=0; if(atDust>100*16) atDust=100*16; }
static void razeAt(int x,int y,int chance){   // a building falls to ruin (fire, lightning, meteors, and the calamities: see burnAt and calamity.h)
    if(x<0||y<0||x>=WW||y>=WH||!bld[y][x]||bld[y][x]==B_RUIN) return; if((int)(rnd()&255)>=chance) return; int v=bldv[y][x]-1; if(v>=0) KD_DIRTY();
    int old=bld[y][x];
    bld[y][x]=B_RUIN; bldv[y][x]=0;
    if(v>=0&&v<VIL_N&&vil[v].on){
        Vil *V=&vil[v];
        if(ggBuildingIsHome(old)&&V->houses)V->houses--;
        if(old==B_FARM&&V->farms)V->farms--;
        if(old==B_TOWER&&V->towers)V->towers--;
        if(ggBuildingFlag(old))bldFlags(v);
    }
    if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y);
}
static void burnAt(int x,int y,int chance){ if(gx.lFire) return; if(!gx.lAtmo&&ATV(atO2)>28){ chance+=(ATV(atO2)-28)*8; if(chance>255) chance=255; }      // rich air feeds fires
    int had=x>=0&&y>=0&&x<WW&&y<WH&&bld[y][x]&&bld[y][x]!=B_RUIN; razeAt(x,y,chance); if(had&&bld[y][x]==B_RUIN) atFire+=3; }   // fire, lightning and meteors on a building (WORLD > LAWS > FIRE ON HOMES: SAFE turns them off)
// village mood, -100 sad .. 100 joyful (after the wiki's happiness), worked out on the spot so a loaded game goes on exactly as the saved one: food, crowding, war, sickness, loyalty, the air and the ground
static int vilMood(int v); static const char* moodName(int m); static int worldAge(void);
static int villageRelief(int v);
static int villageGreenbelt(int v){
    const Vil*V=&vil[v];
    if(!V->farms) return 0;
    // Check at most four sites on the regular village turn. Use a stable
    // hash so enabling a greenbelt doesn't consume simulation randomness.
    for(int i=0;i<4;i++){
        unsigned h=hsh(V->x,V->y,(unsigned)tick+i);
        int x=V->x+(int)(h%13)-6, y=V->y+(int)((h>>8)%13)-6;
        if((unsigned)x>=WW||(unsigned)y>=WH||bld[y][x]||occ[y][x]||world[y][x]!=T_DIRT||bioAt(x,y)) continue;
        setCell(x,y,T_GRASS);
        return 1;
    }
    return 0;
}
static void vilTick(int v){ KD_DIRTY();
    Vil*V=&vil[v]; if(!V->on) return; int pop=0, sk=0;
    for(int dy=-10;dy<=10;dy++){ int y=V->y+dy; if(y<0||y>=WH) continue; for(int dx=-10;dx<=10;dx++){ int x=V->x+dx; if(x<0||x>=WW||dx*dx+dy*dy>100) continue; int j=occ[y][x]-1; if(j>=0&&cr[j].alive&&sp[cr[j].sp].a==V->race&&sp[cr[j].sp].a<8){ pop++; if(DISEASE(&cr[j])==1) sk++; } } }   // (the tiles around the village, not every creature on the planet)
    V->pop=(u16)pop; if(!pop){ if(++V->dead>=6){ vilRaze(v,1); return; } } else V->dead=0;
    if(pop&&LAWON(LW_RELIEF)&&V->stock<64) villageRelief(v);
    if(pop&&LAWON(LW_GREENBELT)) villageGreenbelt(v);
    if((tick&7)==0||vilBf[v]==0) bldFlags(v);
    if(pop){      // recovery: the folk clear the ruins that fires and wars left round the village (a ruin blocks building for ever otherwise) and rebuild a hall that fell
        int R=3+V->lvl*3; for(int k=0;k<2;k++){ int x=V->x+(int)(rnd()%(2*R+1))-R, y=V->y+(int)(rnd()%(2*R+1))-R; if(x>=0&&y>=0&&x<WW&&y<WH&&bld[y][x]==B_RUIN){ bld[y][x]=B_NONE; bldv[y][x]=0; if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); } }
        { int cx=V->x, cy=V->y; if(V->stock>=20&&(bld[cy][cx]==B_RUIN||bld[cy][cx]==B_NONE)&&!ggFortProtectedPlot(cx,cy)){ u8 t=world[cy][cx]; if(t==T_GRASS||t==T_DIRT||t==T_FOREST){ bld[cy][cx]=B_NONE; putBld(v,cx,cy,B_HALL); V->stock=(u16)(V->stock-20); char vn[24], s2[40]; vilName(v,vn); siprintf(s2,"%.14s REBUILDS ITS HALL",vn); logEv(s2); } } } }
    if(V->lvl>=2&&pop>=16&&!(vilBf[v]&1)&&V->stock>=30&&(rnd()%6)==0&&placeBld(v,B_TEMPLE,30)){ vilBf[v]|=1; char vn[24], s2[40]; vilName(v,vn); siprintf(s2,"A TEMPLE RISES IN %.16s",vn); logEv(s2); }
    if(!(vilBf[v]&2)&&V->stock>=40&&(rnd()%10)==0){ int hero=0; for(int dy=-8;dy<=8&&!hero;dy++) for(int dx=-8;dx<=8;dx++){ int x=V->x+dx, y=V->y+dy; if(x<0||y<0||x>=WW||y>=WH) continue; int j=occ[y][x]-1; if(j>=0&&cr[j].alive&&(cr[j].dir>=1||cr[j].rk)&&sp[cr[j].sp].a==V->race){ hero=1; break; } } if(hero&&placeBld(v,B_STATUE,40)){ vilBf[v]|=2; char vn[24], s2[40]; vilName(v,vn); siprintf(s2,"A STATUE HONOURS A HERO OF %.12s",vn); logEv(s2); } }
    if(V->lvl>=2&&pop>=14&&!(vilBf[v]&4)&&V->stock>=35&&(rnd()%8)==0&&placeBld(v,B_LIBRARY,35)){ vilBf[v]|=4; char vn[24], s2[40]; vilName(v,vn); siprintf(s2,"A LIBRARY OPENS IN %.14s",vn); logEv(s2); }
    if((vilBf[v]&1)&&sk){ for(int dy=-10;dy<=10;dy++){ int y=V->y+dy; if(y<0||y>=WH) continue; for(int dx=-10;dx<=10;dx++){ int x=V->x+dx; if(x<0||x>=WW||dx*dx+dy*dy>100) continue; int j=occ[y][x]-1; if(j>=0&&cr[j].alive&&DISEASE(&cr[j])==1&&sp[cr[j].sp].a==V->race&&sp[cr[j].sp].a<8&&(rnd()&1)){ SET_DISEASE(&cr[j],2); sk--; } } } }      /* the temple's priests tend the sick: half of the plagued folk recover (and stay immune) */
    if(sk>=12&&vilSick[v]<12){ char vn[24], b[40]; vilName(v,vn); siprintf(b,"PLAGUE IN %.20s",vn); logEv(b); }      // the journal line
    vilSick[v]=(u8)(sk>255?255:sk);
    if(V->lvl>=2&&pop>=10){ if(!hasDock(v)){ if((rnd()%6)==0) dockTry(v); } else if(nVilOn<VIL_N-2&&(rnd()%10)==0) boatLaunch(v); }
    int mood=vilMood(v); int gain=pop+V->farms*3; if(gx.vGrow[v]==1) gain=gain*3/2+2; else if(gx.vGrow[v]==2) gain=gain/2; if(mood>=40) gain+=pop/4; else if(mood<=-40) gain=gain/2; if(mood<=-60&&vilK[v]&&(rnd()&7)==0&&vilL[v]>0) vilL[v]--; else if(mood>=40&&vilK[v]&&(rnd()&7)==0&&vilL[v]<100) vilL[v]++;
    V->stock=(u16)(V->stock+gain>400?400:V->stock+gain);
    // what to build next: homes for everyone, fields to feed them, then towers, then a bigger hall
    if(V->lvl>=2&&V->towers<V->houses/3&&V->stock>=25&&(rnd()&3)==0){ if(placeBld(v,B_TOWER,25)) V->towers++; }
    else if(V->houses<2||pop>V->houses*3){ int choice=(int)(rnd()&3);
        int home=choice?B_HOUSE:B_HUT;
        if(choice==3&&V->lvl>=2&&V->stock>=24)home=B_MANOR;
        int built=placeBld(v,home,home==B_MANOR?24:12);
        if(!built&&home==B_MANOR)built=placeBld(v,B_HOUSE,12);
        if(!built&&home!=B_HUT&&V->houses<5)built=placeBld(v,B_HUT,12);
        if(built) V->houses++; }
    else if(V->farms*2<V->houses&&V->stock>=10){ if(placeBld(v,B_FARM,10)) V->farms++; }
    else if(V->lvl>=2&&V->towers<V->houses/3&&V->stock>=25){ if(placeBld(v,B_TOWER,25)) V->towers++; }
    if(V->lvl==1&&V->houses>=6&&V->stock>=40){ if(placeBld(v,B_HALL,40)) V->lvl=2; }
    else if(V->lvl==2&&V->houses>=14&&V->stock>=80){ if(placeBld(v,B_HALL,80)) V->lvl=3; }
    ggSettlementGrow(v,pop);
    ggSettlementExpandHomes(v,pop);
    // war: the restless races pick a neighbour of another people; it ends when one side is gone
    if(V->foe&&(!vil[V->foe-1].on)) V->foe=0;
    if(V->foe&&(rnd()&15)==0&&pop<3) V->foe=0;   // a beaten army goes home
}
static void warMatrix(void){ memset(warM,0,sizeof warM); for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].foe&&vil[vil[v].foe-1].on){ int a=vil[v].race, b=vil[vil[v].foe-1].race;
#ifdef GG_NOCIVIL      // (benchmark builds only, pc_war.c: a war inside one people is ignored. Enemies are found by race, so a village taken or lost across kingdoms would otherwise set every soldier of that people on every other)
        if(a==b) continue;
#endif
        warM[a][b]=warM[b][a]=1; } }
static int vilTotals(int*houses){ int pop=0,h=0; for(int v=0;v<VIL_N;v++) if(vil[v].on){ pop+=vil[v].pop; h+=vil[v].houses; } if(houses) *houses=h; return pop; }

// ---------------------------------------------------------------------------------------------- kingdoms, wars, vassals and empires
// Villages group into kingdoms (at most 8). A kingdom has a ruler with a temper, gold, technology, relations with every other kingdom (-100 hatred .. +100 love), allies, wars,
// and may be another kingdom's vassal. Wars are declared from bad relations and fought by the villages' soldiers (the old village war marches); cities are captured, not just razed.
#define KN_N 16
typedef struct { u8 on, race, tech, rtr, rn1, rn2, n1, n2, lord, empire, weary, reign, trade, cul, cap, plot, plotT, truce, spite, rlg, lang, clan, ctr, pcool, nlang; s8 rel[KN_N]; u16 gold, born, cpts, allies, war, tradeW; u32 know; } Kd;   // rel[b]: this kingdom's opinion of b; cul: culture; cap: capital village (+1); plot: the kingdom it is plotting war against (+1)
static int kdKnow(int k,int b);
typedef struct { u8 tribute, tax; u16 ctr2, renown; } KdX;   // per-kingdom data that must not touch Kd (Kd is saved raw): policies (0 normal, 1 high, 2 low), the five newer culture traits, renown. Saved in the GGK1 trailer.
static KdX kx[KN_N];
static u8 whisT[KN_N], disT[KN_N], spiT[KN_N], vilIndep[VIL_N];   // Pack B, temporary on purpose (not saved): a whispered war, discord and spite counters (in kingdom turns), cities that left their kingdom by GOLD DUST
static int polSgn(int v){ return v==1?1:v==2?-1:0; }   // a policy value as +1 (high), -1 (low) or 0
static Kd kd[KN_N]; /* (vilK, vilL: a city's kingdom and its loyalty, declared with the villages) */ static char jr[24][34]; static u16 jrYear[24]; static int jrN;
static int villageRelief(int v){
    Vil*dest=&vil[v];
    if(!dest->on||dest->stock>=64) return 0;
    int donor=-1, best=24*24+1, targetKing=vilK[v]-1;
    for(int u=0;u<VIL_N;u++){
        const Vil*V=&vil[u];
        if(u==v||!V->on||V->stock<160||V->foe==v+1||dest->foe==u+1) continue;
        int dx=(int)V->x-dest->x, dy=(int)V->y-dest->y, d=dx*dx+dy*dy;
        if(d>=best) continue;
        int sourceKing=vilK[u]-1;
        if(targetKing<0||sourceKing<0){
            if(targetKing!=sourceKing||V->race!=dest->race) continue;
        } else {
            if(targetKing>=KN_N||sourceKing>=KN_N) continue;
            const Kd *a=&kd[targetKing], *b=&kd[sourceKing];
            if(!a->on||!b->on) continue;
            if(targetKing!=sourceKing){
                if(!(a->allies&(1u<<sourceKing))||!(b->allies&(1u<<targetKing))) continue;
                if((a->war&(1u<<sourceKing))||(b->war&(1u<<targetKing))) continue;
            }
        }
        donor=u; best=d;
    }
    if(donor<0) return 0;
    int amount=64-dest->stock;
    if(amount>32) amount=32;
    vil[donor].stock-=(u16)amount;
    dest->stock+=(u16)amount;
    return amount;
}
static u8 hall[KN_N][4][3];   // the last four rulers of each kingdom: name seeds rn1, rn2 and the years they reigned (GGR1 trailer)
static void hallPush(int k){ memmove(hall[k][1],hall[k][0],9); hall[k][0][0]=kd[k].rn1; hall[k][0][1]=kd[k].rn2; hall[k][0][2]=kd[k].reign>255?255:kd[k].reign; }
static const char* const kn1[12]={"AR","BEL","COR","DRA","EST","FEN","GAL","HOR","ISK","JOR","KEL","LUN"}, * const kn2[8]={"IA","ANIA","OTH","MARK","LAND","VIA","ELL","UM"};
static const char* const rnA[12]={"ALDRIC","BRYN","CEDRA","DORN","EMBER","FARIN","GRETA","HALDOR","ISOLDE","JORAN","KIRA","LOTHAR"}, * const rnB[8]={"THE BOLD","THE WISE","THE FAT","THE MAD","THE KIND","THE LATE","THE GREEDY","THE PALE"};
static const char* const rtrN[4]={"WARLIKE","PEACEFUL","GREEDY","WISE"}; static const char* const culN[5]={"SUN","MOON","IRON","SEA","STONE"}, * const rlgN[4]={"SUNFAITH","MOONFAITH","STONEWAY","OLD GODS"}, * const langN[5]={"HIGH TONGUE","LOWSPEAK","RUNIC","SONG","CLICKS"}, * const clanN[8]={"OAK","RAVEN","WOLF","ROSE","CROWN","STAR","IRON","TIDE"};
static const char* const ctr2N[5]={"SHATTERED CROWN","EQUAL HEIRS","EXPANSIONIST","ETHNOCENTRIC","XENOPHILE"};
static const char* const langTraitN[3]={"TRADE TONGUE","DIPLOMATIC TONGUE","SCHOLARLY TONGUE"}, * const clanTraitN[3]={"WARRIOR CLAN","STEWARD CLAN","EXPANSIONIST CLAN"}, * const faithTraitN[3]={"PEACEFUL FAITH","MILITANT FAITH","MISSIONARY FAITH"};
static const char* const ctrN[6]={"XENOPHOBIC","SCHOLARLY","MERCANTILE","MARTIAL","AGRARIAN","PIOUS"}, * const ageN[10]={"DARK","HOPE","SUN","TEARS","MOON","CHAOS","WONDERS","ICE","ASH","DESPAIR"};
static int worldAge(void){ return (year/12+(int)(worldSeed%10))%10; }   /* ten twelve-year ages: they tilt morale, war, growth and learning */
static int ghostMood(int v);
static int vilMood(int v){ const Vil*V=&vil[v]; int m=V->stock/8-20-ghostMood(v); if(V->pop>V->houses*3+3) m-=20; if(V->foe) m-=25; if(vilSick[v]) m-=15; if(vilK[v]) m+=(vilL[v]-60)/3-8*polSgn(kx[vilK[v]-1].tax); m+=V->towers>0?5:0;
    { int ag=worldAge(); m+=(ag==1||ag==2||ag==6)?10:(ag==0||ag==5||ag==8||ag==9)?-10:ag==3?-5:0; } if(!gx.lAtmo){ int T=ATV(atTemp); if(T>=22||T<=-18) m-=15; } { int b=bioAt(V->x,V->y); m+=b==1?-20:b==3?20:0; }
    /* Grin Mark is a social trait: nearby bearers lift the settlement's mood. */
    { int joy;
      if(joyStamp[v]==joyEpoch) joy=joyCached[v];
      else { static const u8 span[11]={10,9,9,9,9,8,8,7,6,4,0}; joy=0;
          for(int dy=-10;dy<=10;dy++){ int y=V->y+dy; if(y<0||y>=WH) continue; int r=span[dy<0?-dy:dy], lo=V->x-r, hi=V->x+r; if(lo<0) lo=0; if(hi>=WW) hi=WW-1;
              for(int x=lo;x<=hi;x++){ int j=occ[y][x]-1; if(j<0) continue; Cr*c=&cr[j]; if(c->alive&&HAS(c,TR_GRIN)&&sp[c->sp].a==V->race&&sp[c->sp].a<8) joy++; } }
          joyCached[v]=(u16)joy; joyStamp[v]=joyEpoch; }
      m+=joy*4; }
    m+=(vilBf[v]&1?12:0)+(vilBf[v]&2?6:0)+(vilBf[v]&4?4:0); return m<-100?-100:m>100?100:m; }
static const char* moodName(int m){ return m>=60?"JOYFUL":m>=20?"HAPPY":m>-20?"CONTENT":m>-60?"SAD":"MISERABLE"; }
static const char* const techN[10]={"NONE","AGRICULTURE","IRON TOOLS","MASONRY","TRADE ROADS","STEEL","ENGINEERING","ALCHEMY","ASTRONOMY","FEUDALISM"};
static void logEv(const char*s){ if(jrN<24){ strncpy(jr[jrN],s,33); jr[jrN][33]=0; jrYear[jrN]=(u16)year; jrN++; } else { memmove(jr[0],jr[1],23*34); memmove(jrYear,jrYear+1,23*2); strncpy(jr[23],s,33); jr[23][33]=0; jrYear[23]=(u16)year; } siprintf(info,"%.32s",s); }
static void kdName(int k,char*o){ langKing(kd[k].nlang,kd[k].n1,kd[k].n2,o); }
#include "ggid.h"
#include "ggwar.h"      // (10c: war groups: one record for an army, counted from its soldiers)
static void kdRuler(int k,char*o){ strcpy(o,lwords[kd[k].lang%5][8]); strcat(o," "); char p[24]; langPerson(kd[k].lang,kd[k].rn1,kd[k].rn2,p); strcat(o,p); strcat(o," "); strcat(o,rnB[kd[k].rn2%8]); }
static void kdFaith(int k,char*o){ static const u8 dn[4]={7,6,0,14}; strcpy(o,lwords[kd[k].lang%5][13]); strcat(o,"-"); strcat(o,lwords[kd[k].lang%5][dn[kd[k].rlg&3]]); }   /* the faith's local name: the tongue's word for god and for its dogma */
static int kdRStat(int k,int w){ if(!LAWON(LW_KINGSTATS)) return 5; const Kd*K=&kd[k]; int v=1+(int)(hsh(K->rn1*16+K->rn2+1,w+1,0x52554C45)%10)+(K->reign>=120?3:K->reign>=80?2:K->reign>=40?1:0); return v>10?10:v; }   // the ruler's four stats (1..10), worked out from the name seed and the years of rule, so nothing is saved and a new ruler changes them: 0 STEWARDSHIP, 1 WARFARE, 2 DIPLOMACY, 3 INTELLIGENCE
static void kdStatsRaw(int k,int*cities,int*pop,int*power){ int c=0,p=0,w=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1){ c++; p+=vil[v].pop; w+=vil[v].pop+vil[v].towers*3+vil[v].houses; } w+=kd[k].tech*4; if(kd[k].ctr>>3&1) w=w*5/4; w=w*(100+10*kdKnow(k,2))/100; w=w*(100+3*(kdRStat(k,1)-5))/100; if(kd[k].rlg==0) w=w*11/10; if(kx[k].ctr2&(1u<<8)) w=w*11/10; if(kx[k].ctr2&(1u<<12)) w=w*105/100; if(cities) *cities=c; if(pop) *pop=p; if(power) *power=w; }
static int kdDistRaw(int a,int b){ int best=9999; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==a+1) for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==b+1){ int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y; int d=dx*dx+dy*dy; if(d<best) best=d; } int r=0; while(r*r<best&&r<200) r++; return r; }
static u16 randomTraitBits(int bits,int count){ u16 m=0; if(bits>15) bits=15; if(count>bits) count=bits; while(count>0){ int b=(int)(rnd()%(unsigned)bits); if(!(m&(1u<<b))){ m|=(u16)(1u<<b); count--; } } return m; }
static int randomTraitCount(void){ return LAWON(LW_NOOSPHERE)?3+(int)(rnd()%4):1+(int)(rnd()%3); }
static int kdFound(int v){
    KD_DIRTY();
    int k=-1; for(int i=0;i<KN_N;i++) if(!kd[i].on){ k=i; break; } if(k<0) return -1;
    Kd*K=&kd[k]; memset(K,0,sizeof *K); K->on=1; K->race=vil[v].race; K->n1=(u8)(rnd()%12); K->n2=(u8)(rnd()%8); K->rn1=(u8)(rnd()%12); K->rn2=(u8)(rnd()%8); K->rtr=(u8)(aggressive(K->race)?0:rnd()%4); K->born=(u16)year; K->gold=30; K->cul=(u8)(rnd()%5); K->rlg=(u8)(rnd()%4); K->lang=(u8)(rnd()%5==0?rnd()%5:raceLang(K->race)); K->nlang=K->lang; K->clan=(u8)(rnd()%8); K->cap=(u8)(v+1); memset(&kx[k],0,sizeof kx[k]); memset(hall[k],0,sizeof hall[k]);
    /* Every new meta identity can pick up a few traits. Glitched Noosphere raises the count from 1-3 to 3-6. */
    if(LAWON(LW_NOOSPHERE)||rnd()%100<55){ int n=randomTraitCount(); u16 cb=randomTraitBits(11,n); K->ctr=(u8)(cb&63); kx[k].ctr2=(u16)((cb>>6)&31); }
    if(LAWON(LW_NOOSPHERE)||rnd()%100<55) kx[k].ctr2|=(u16)(randomTraitBits(3,randomTraitCount())<<5);  // language traits
    if(LAWON(LW_NOOSPHERE)||rnd()%100<55) kx[k].ctr2|=(u16)(randomTraitBits(3,randomTraitCount())<<8);  // clan traits
    if(LAWON(LW_NOOSPHERE)||rnd()%100<55) kx[k].ctr2|=(u16)(randomTraitBits(3,randomTraitCount())<<11); // religion traits
    { int t=K->rtr; kx[k].tribute=(u8)(t==0||t==2?1:t==1?2:0); kx[k].tax=(u8)(t==2?1:t==1?2:0); }
    vilK[v]=(u8)(k+1); vilL[v]=60; char n[24], b[72]; kdName(k,n); siprintf(b,"THE KINGDOM OF %s RISES",n); logEv(b); return k; }
static void kdAssign(void){
    KD_DIRTY();
    for(int v=0;v<VIL_N;v++){ if(!vil[v].on){ vilK[v]=0; vilIndep[v]=0; continue; } if(vilIndep[v]){ vilK[v]=0; continue; } if(vilK[v]&&kd[vilK[v]-1].on) continue; vilK[v]=0;
        int best=-1, bd=40*40; for(int u=0;u<VIL_N;u++) if(u!=v&&vil[u].on&&vilK[u]&&vil[u].race==vil[v].race){ int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y, d=dx*dx+dy*dy; if(d<bd){ bd=d; best=u; } }
        if(best>=0) vilK[v]=vilK[best]; else if(kdFound(v)<0){ int nb=vilAny(vil[v].x,vil[v].y,60); (void)nb; for(int u=0;u<VIL_N;u++) if(u!=v&&vilK[u]){ vilK[v]=vilK[u]; break; } } } }
static u16 warYr[KN_N][KN_N];   // the year each war began (not saved: the war records are only for the journal)
static void renown(int k,int n){ int r=kx[k].renown+n; kx[k].renown=(u16)(r>65000?65000:r); }
static void kdWar(int a,int b,int on){ if(on){ if(!(kd[a].war>>b&1)) warYr[a][b]=warYr[b][a]=(u16)year; kd[a].war|=(u16)(1<<b); kd[b].war|=(u16)(1<<a); kd[a].weary=kd[b].weary=0; } else { kd[a].war&=(u16)~(1<<b); kd[b].war&=(u16)~(1<<a); } }
static inline int peopleVillagesAtWar(int ra,int rb,int va,int vb){
    if(va<0||vb<0||!vilK[va]||!vilK[vb]) return ra!=rb&&warM[ra][rb];
    int ka=vilK[va]-1,kb=vilK[vb]-1;
    return ka!=kb&&kd[ka].on&&kd[kb].on&&((kd[ka].war>>kb)&1);
}
static int peopleKingdomsAtWar(int ia,int ib){
    if(ia<0||ib<0||ia>=nCr||ib>=nCr||!cr[ia].alive||!cr[ib].alive) return 0;
    int ra=sp[cr[ia].sp].a, rb=sp[cr[ib].sp].a; if(ra>=8||rb>=8) return 0;
    int va=vilNear(ra,cr[ia].x,cr[ia].y,20), vb=vilNear(rb,cr[ib].x,cr[ib].y,20);
    return peopleVillagesAtWar(ra,rb,va,vb);
}
static const char* const knBr[6]={"GOVERNANCE","ZONE CONTROL","WARFARE","ECONOMY","ARCHITECTURE","EDUCATION"};
static const char* const knN[6][3]={{"TRIBAL LAW","KINGS COURT","ROYAL CHARTER"},{"BOUNDARY STONES","BORDER FORTS","GRAND SURVEY"},{"DRILLED LEVIES","STEEL ARMS","SIEGECRAFT"},{"BARTER","COINAGE","BANKING"},{"MASONRY","ARCHES","CATHEDRALS"},{"WRITING","SCHOLARS","UNIVERSITIES"}};
static const char* const dogN[4]={"CRUSADERS: +10% POWER","PEACEKEEPERS: BETTER TALKS","BUILDERS: RICHER CITIES","MYSTICS: ODD GIFTS"};
static int kdKnow(int k,int b){ int n=0; for(int i=0;i<3;i++) n+=(kd[k].know>>(b*3+i))&1; return n; }
static int kdMaxCities(int k){ return 3+kdKnow(k,0)+(kd[k].tech>=6)+kdRStat(k,0)/6+((kx[k].ctr2>>2&1)?2:0)+((kx[k].ctr2&(1u<<10))?1:0); }   // (EXPANSIONIST: two more)   // governance: each level allows one more city
static int kdZonesRaw(int k){ int z=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1) z+=3+vil[v].lvl*3+vil[v].houses/3+kdKnow(k,1)*3; return z; }
// The kingdom answer cache: kdStats (cities, people, power), kdDist (the nearest two cities of two kingdoms) and kdZones each walk every village (kdDist, every pair of villages), and the
// kingdom turn, the pause menu's kingdom page and the browser ask them over and over for the same answers. An answer is kept until KD_DIRTY() says a village or a kingdom changed (or the tick moves on).
// Build with -DKD_CHECK and every answer served from the cache is compared with a fresh one (pc_speed.c does).
static struct { u32 ep, tk; u8 sOk[KN_N], zOk[KN_N], dOk[KN_N][KN_N]; int sc[KN_N], sp[KN_N], sw[KN_N], zv[KN_N]; u8 dv[KN_N][KN_N]; } kdC; static long kdHit, kdMiss, kdBad;
static void kdCacheSync(void){ if(kdC.ep!=kdEp||kdC.tk!=(u32)tick){ memset(&kdC,0,sizeof kdC); kdC.ep=kdEp; kdC.tk=(u32)tick; } }
static void kdStats(int k,int*cities,int*pop,int*power){ kdCacheSync(); if(!kdC.sOk[k]){ kdStatsRaw(k,&kdC.sc[k],&kdC.sp[k],&kdC.sw[k]); kdC.sOk[k]=1; kdMiss++; } else { kdHit++;
#ifdef KD_CHECK
        { int c,p,w; kdStatsRaw(k,&c,&p,&w); if(c!=kdC.sc[k]||p!=kdC.sp[k]||w!=kdC.sw[k]){ kdBad++; if(kdBad<6) fprintf(stderr,"BAD stats k%d tick %d: cache %d %d %d fresh %d %d %d ep %u\n",k,(int)tick,kdC.sc[k],kdC.sp[k],kdC.sw[k],c,p,w,kdEp); } }
#endif
    } if(cities) *cities=kdC.sc[k]; if(pop) *pop=kdC.sp[k]; if(power) *power=kdC.sw[k]; }
static int kdDist(int a,int b){ kdCacheSync(); if(!kdC.dOk[a][b]){ kdC.dv[a][b]=(u8)kdDistRaw(a,b); kdC.dOk[a][b]=1; kdMiss++; } else { kdHit++;
#ifdef KD_CHECK
        if(kdC.dv[a][b]!=(u8)kdDistRaw(a,b)){ kdBad++; if(kdBad<6) fprintf(stderr,"BAD dist %d %d tick %d\n",a,b,(int)tick); }
#endif
    } return kdC.dv[a][b]; }
static int kdZones(int k){ kdCacheSync(); if(!kdC.zOk[k]){ kdC.zv[k]=kdZonesRaw(k); kdC.zOk[k]=1; kdMiss++; } else { kdHit++;
#ifdef KD_CHECK
        if(kdC.zv[k]!=kdZonesRaw(k)){ kdBad++; if(kdBad<6) fprintf(stderr,"BAD zones %d tick %d\n",k,(int)tick); }
#endif
    } return kdC.zv[k]; }
static void kdDeclareX(int a,int b,int spite){
    if(a==b||!gx.war||!LAWON(LW_DIPLOMACY)||!kd[a].on||!kd[b].on||(kd[a].war>>b&1)) return; if(spite){ kd[a].spite=kd[b].spite=1; } char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); kdWar(a,b,1); stWars++; kd[a].allies&=(u16)~(1<<b); kd[b].allies&=(u16)~(1<<a); kd[a].rel[b]=kd[b].rel[a]=-60;
    if(kd[a].rlg!=kd[b].rlg&&((kd[a].ctr>>5&1)||kd[a].rlg==0||(kx[a].ctr2&(1u<<12)))) siprintf(s,"%s DECLARES HOLY WAR ON %s",x,y); else siprintf(s,"%s DECLARES WAR ON %s",x,y); logEv(s);
    for(int c=0;c<KN_N;c++) if(c!=a&&c!=b&&kd[c].on&&(kd[a].allies>>c&1)&&!(kd[b].allies>>c&1)&&kd[c].rel[b]<20&&!(kd[c].war>>b&1)){ kdWar(c,b,1); char z[24]; kdName(c,z); siprintf(s,"%s JOINS THE WAR",z); logEv(s); } }
static void kdDeclare(int a,int b){ kdDeclareX(a,b,0); }
static void kdDie(int k){ KD_DIRTY(); char n[24], s[72]; kdName(k,n); siprintf(s,"THE KINGDOM OF %s FALLS",n); logEv(s); for(int o=0;o<KN_N;o++){ kd[o].allies&=(u16)~(1<<k); kd[o].war&=(u16)~(1<<k); if(kd[o].lord==k+1) kd[o].lord=0; kd[o].rel[k]=0; } memset(&kd[k],0,sizeof kd[k]); memset(&kx[k],0,sizeof kx[k]); memset(hall[k],0,sizeof hall[k]); whisT[k]=disT[k]=spiT[k]=0; for(int o=0;o<KN_N;o++) if(whisT[o]==k+1) whisT[o]=0; }
static int vilConquer(int fv,int byV){   // a village taken in war changes kingdom instead of dying
    KD_DIRTY();
    int a=vilK[byV]-1, b=vilK[fv]-1; if(a<0||b<0||a==b||!(kd[a].war>>b&1)) return 0;
    /* Ethnocentric / xenophobic attackers raze a city of another people; other cultures capture it. */
    if(vil[fv].race!=kd[a].race&&((kd[a].ctr&1)||(kx[a].ctr2&(1u<<3)))){ char n[24], x[24], msg[72]; vilName(fv,n); kdName(a,x); siprintf(msg,"%.18s DESTROYS %.20s",x,n); logEv(msg); vilRaze(fv,1); renown(a,1); return 1; }
    int wasCap=kd[b].cap==fv+1; vilK[fv]=(u8)(a+1); vilL[fv]=-20; stCapt++; vil[fv].foe=0; vil[fv].stock/=2; renown(a,3);
    if(wasCap){ kd[b].cap=0; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==b+1){ vilL[u]=(s8)(vilL[u]-25); vil[u].foe=0; } logEv("A CAPITAL FALLS: ARMIES DESERT"); { char kn[24], t[72]; kdName(b,kn); if(hsh(fv,(u32)tick,0x464C4545)&1){ siprintf(t,"THE RULER OF %.20s FLEES THE FALLEN CAPITAL",kn); } else { hallPush(b); kd[b].rn1=(u8)(hsh(fv,(u32)tick,1)%12); kd[b].rn2=(u8)(hsh(fv,(u32)tick,2)%8); kd[b].reign=0; siprintf(t,"THE RULER OF %.20s FALLS WITH THE CAPITAL",kn); } logEv(t); } }   /* taking a capital shakes every other city of the kingdom */ char n[24], x[24], s[72]; vilName(fv,n); kdName(a,x); siprintf(s,"%s TAKES %s",x,n); logEv(s); kd[b].weary=(u8)(kd[b].weary>3?kd[b].weary-3:0); return 1; }
static void kdSplit(int k,int mode){      // a crown changes hands in a kingdom with the SHATTERED CROWN (mode 0: every far city becomes a kingdom of its own) or EQUAL HEIRS (mode 1: the farther half goes to one heir). Only as far as free kingdom slots allow: the cities that cannot be given away stay with the heir.
    int cap=kd[k].cap?kd[k].cap-1:-1; if(cap<0||!vil[cap].on) return; int vs[VIL_N], d[VIL_N], n=0;
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1&&v!=cap){ int dx=vil[v].x-vil[cap].x, dy=vil[v].y-vil[cap].y; vs[n]=v; d[n]=dx*dx+dy*dy; n++; }
    if(n<2) return; for(int i=1;i<n;i++){ int vv=vs[i], dd=d[i], j=i-1; while(j>=0&&d[j]<dd){ vs[j+1]=vs[j]; d[j+1]=d[j]; j--; } vs[j+1]=vv; d[j+1]=dd; }     // the farthest first
    int free_=0; for(int i=0;i<KN_N;i++) if(!kd[i].on) free_++; char x[24]; kdName(k,x); int made=0;
    if(mode==0){ int lim=n<free_?n:free_; if(lim>3) lim=3; for(int i=0;i<lim;i++){ int v=vs[i], cul=kd[k].cul, rlg=kd[k].rlg, lang=kd[k].lang; vilK[v]=0; int n2=kdFound(v); if(n2>=0){ kd[n2].cul=(u8)cul; kd[n2].rlg=(u8)rlg; kd[n2].lang=(u8)lang; kd[n2].race=vil[v].race; kd[n2].truce=20; made++; } } }
    else { int half=n/2; if(half<1||!free_) return; int v0=vs[0], cul=kd[k].cul, rlg=kd[k].rlg, lang=kd[k].lang; vilK[v0]=0; int n2=kdFound(v0); if(n2>=0){ kd[n2].cul=(u8)cul; kd[n2].rlg=(u8)rlg; kd[n2].lang=(u8)lang; kd[n2].race=vil[v0].race; kd[n2].truce=20; made=1; for(int i=1;i<half;i++){ vilK[vs[i]]=(u8)(n2+1); vilL[vs[i]]=60; } } }
    if(made&&mode==0) achX[2]|=1; if(made){ char s2[80]; siprintf(s2,mode==0?"%.22s SHATTERS INTO %d REALMS":"%.22s IS DIVIDED AMONG HEIRS",x,made+1); logEv(s2); KD_DIRTY(); } }
static void kingdomTick(void){
    { static int lastAge=-1; int ag=worldAge(); if(ag!=lastAge){ if(lastAge>=0){ char s2[72]; siprintf(s2,"THE AGE OF %s BEGINS",ageN[ag]); logEv(s2); } lastAge=ag; } }
    kdAssign();
    for(int k=0;k<KN_N;k++) if(kd[k].on){ KD_DIRTY(); if(disT[k]) disT[k]--; if(spiT[k]) spiT[k]--; int c,p,w; kdStats(k,&c,&p,&w); if(!c){ kdDie(k); continue; }
        Kd*K=&kd[k]; { int g=p/6+c*2+(K->trade?(K->ctr>>2&1)?10:4:0); if(kx[k].ctr2&(1u<<5)) g=g*110/100; if(kx[k].ctr2&(1u<<9)) g=g*110/100; if(worldAge()==1||worldAge()==2) g=g*5/4; g=g*(100+15*kdKnow(k,3))/100; g=g*(100+30*polSgn(kx[k].tribute)+15*polSgn(kx[k].tax))/100; K->gold=(u16)(K->gold+g>60000?60000:K->gold+g); } K->trade=0; K->tradeW=0;
        { int bon=(K->ctr>>4&1?3:0)+kdKnow(k,4)*2+(K->rlg==2?2:0); if(bon) for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1&&vil[v].stock<390) vil[v].stock=(u16)(vil[v].stock+bon); }
        { /* the culture tree: points from people and scholars buy the next node of the branch the ruler cares about */ static const u8 pref[4][6]={{2,1,0,3,4,5},{0,4,5,3,1,2},{3,1,4,0,2,5},{5,0,3,4,1,2}}; int lv=0; for(int b=0;b<6;b++) lv+=kdKnow(k,b); K->cpts=(u16)(K->cpts+p/8+c+((K->ctr>>1&1)?4:0)+kdKnow(k,5)*2+((kx[k].ctr2&(1u<<7))?3:0)); int cost=(30+lv*20)*(100-10*kdKnow(k,5))/100;
          if(lv<18&&K->cpts>=cost){ for(int q=0;q<6;q++){ int b=pref[K->rtr&3][q]; int l=kdKnow(k,b); if(l<3){ K->know|=1u<<(b*3+l); K->cpts-=(u16)cost; char y[24], s2[72]; kdName(k,y); siprintf(s2,"%s LEARNS %s",y,knN[b][l]); logEv(s2); break; } } } }
        if(K->rlg==3&&nCr&&(rnd()%100)<4){ for(int t=0;t<24;t++){ int i=(int)(rnd()%nCr); Cr*c2=&cr[i]; if(c2->alive&&sp[c2->sp].a==K->race&&c2->tr2==0){ giveTrait(c2,1+(int)(rnd()%(TR_N-1)),0); break; } } }
        if(((K->ctr>>5&1)||K->rlg==0)&&(rnd()%100)<5){ for(int o=0;o<KN_N;o++) if(o!=k&&kd[o].on&&kd[o].rlg!=K->rlg&&kd[o].rel[k]>-30&&kdDist(k,o)<40){ kd[o].rlg=K->rlg; char y[24], f[24], s2[72]; kdName(o,y); kdFaith(k,f); siprintf(s2,"%s ADOPTS FAITH %s",y,f); logEv(s2); break; } }
        if(__builtin_popcount(kx[k].ctr2)<2&&(rnd()%100)<2){ int t=(int)(rnd()%5); if(!(kx[k].ctr2>>t&1)){ kx[k].ctr2|=(u16)(1<<t); char y[24], s2[72]; kdName(k,y); siprintf(s2,"%.20s TURNS %s",y,ctr2N[t]); logEv(s2); } }
        if((LAWON(LW_SUCCESSION)||K->reign>=250)&&++K->reign>90+(rnd()%70)){ hallPush(k); K->reign=0; K->rn1=(u8)(rnd()%12); K->rn2=(u8)(rnd()%8); if(!aggressive(K->race)) K->rtr=(u8)(rnd()%4); else if(rnd()&1) K->rtr=(u8)(rnd()%4); }
        if(K->reign==0){ kdEp++; { char y1[24], r1[40], s1[80]; kdName(k,y1); kdRuler(k,r1); siprintf(s1,"%.22s CROWNS %.30s",y1,r1); logEv(s1); } if(year-(int)K->born>=10){ if(kx[k].ctr2&1) kdSplit(k,0); else if(kx[k].ctr2>>1&1) kdSplit(k,1); } int old=K->clan; if(rnd()%10>=7) K->clan=(u8)(rnd()%8); if(K->clan!=old){ char y[24], s2[72]; kdName(k,y); siprintf(s2,"HOUSE %s TAKES %s",clanN[K->clan],y); logEv(s2); } }
        if(__builtin_popcount(K->ctr)<4&&(rnd()%100)<3){ int t=(int)(rnd()%6); if(!(K->ctr>>t&1)){ K->ctr|=(u8)(1<<t); char y[24], s2[72]; kdName(k,y); siprintf(s2,"%s TURNS %s",y,ctrN[t]); logEv(s2); } }
        if(K->lord&&kd[K->lord-1].on&&(rnd()%100)<8){ Kd*L=&kd[K->lord-1]; int w2=(int)(rnd()%3); if(w2==0) K->cul=L->cul; else if(w2==1) K->rlg=L->rlg; else K->lang=L->lang; }   /* a vassal slowly takes on its lord's ways */
        if(K->gold>1500){ K->gold-=600; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1) vil[v].stock=(u16)(vil[v].stock+40>400?400:vil[v].stock+40); }
        if(K->gold>400){ K->gold-=150; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1&&vil[v].stock<350){ vil[v].stock+=50; break; } }
        int tcost=(70+K->tech*60)*((K->ctr>>1&1)?8:10)/10*((worldAge()==6||worldAge()==7)?7:10)/10*(100-10*kdKnow(k,5))/100; { int lb=0; for(int v2=0;v2<VIL_N;v2++) if(vil[v2].on&&vilK[v2]==k+1&&(vilBf[v2]&4)) lb++; if(lb>6) lb=6; tcost=tcost*(100-5*lb)/100; } tcost=tcost*(100-2*(kdRStat(k,3)-5))/100; if(K->tech<9&&K->gold>=(u16)tcost){ K->gold-=(u16)tcost; K->tech++; char y[24], s[72]; kdName(k,y); siprintf(s,"%s LEARNS %s",y,techN[K->tech]); logEv(s); }
        int emp=c>=5||(0); for(int o=0;o<KN_N;o++) if(kd[o].on&&kd[o].lord==k+1) emp+=0; { int vs=0; for(int o=0;o<KN_N;o++) vs+=kd[o].on&&kd[o].lord==k+1; if(vs>=2) emp=1; }
        if(emp&&!K->empire){ K->empire=1; char y[24], s[72]; kdName(k,y); siprintf(s,"%s BECOMES AN EMPIRE",y); logEv(s); } else if(!emp) K->empire=0;
        if(K->lord){ int l=K->lord-1; if(!kd[l].on) K->lord=0; else { int tr=K->gold/8; K->gold-=(u16)tr; kd[l].gold=(u16)(kd[l].gold+tr>60000?60000:kd[l].gold+tr); int c2,p2,w2; kdStats(l,&c2,&p2,&w2); if(w2*2<w||(rnd()%300)==0){ K->lord=0; char y[24], z[24], s[72]; kdName(k,y); kdName(l,z); kdWar(k,l,1); K->rel[l]=kd[l].rel[k]=-70; siprintf(s,"%s REVOLTS AGAINST %s",y,z); logEv(s); } } } }
    KD_DIRTY();   // (the loop above changed technology, culture, faith: no answer worked out inside it is still good)
    // opinions (each kingdom's view of every other: distance, species, culture, alliance, size, ruler tempers), then plots: war, peace and alliance
    for(int a=0;a<KN_N;a++) if(kd[a].on){ if(kd[a].truce) kd[a].truce--; for(int b=0;b<KN_N;b++) if(b!=a&&kd[b].on){
            int d=kdDist(a,b), op=0; if(d<16) op-=15; else if(d>45) op+=10;
            op+=kd[a].race==kd[b].race?15:-10; op+=kd[a].cul==kd[b].cul?15:-15; op+=kd[a].rlg==kd[b].rlg?15:-15; op+=kd[a].lang==kd[b].lang?8:-8; if(kx[a].ctr2&(1u<<6)) op+=4; if(kx[a].ctr2&(1u<<11)) op+=kd[a].rlg==kd[b].rlg?5:0; if(kx[a].ctr2&(1u<<12)) op+=kd[a].rlg==kd[b].rlg?0:-5; if(kx[a].ctr2&(1u<<13)) op+=kd[a].rlg==kd[b].rlg?0:4; op+=kd[a].clan==kd[b].clan?10:-12; if((kd[a].ctr&1)&&!(kx[a].ctr2>>4&1)&&(kd[a].cul!=kd[b].cul||kd[a].race!=kd[b].race)) op-=12; if((kx[a].ctr2>>3&1)&&kd[a].race==kd[b].race&&kd[a].cul!=kd[b].cul) op-=20; if((kx[a].ctr2>>4&1)&&kd[a].race!=kd[b].race) op+=10; if(worldAge()==1) op+=10; if(kd[a].rlg==1) op+=5; op+=kdRStat(a,2)-5; if(kd[a].allies>>b&1) op+=30; if(disT[a]||disT[b]) op-=20; if(spiT[b]) op-=25; if(kd[a].war>>b&1) op-=500; if(kd[a].truce) op+=20; if(kd[a].lord==b+1||kd[b].lord==a+1) op+=15;
            { int za=kdZones(a), zb=kdZones(b); if(zb>za){ int t=(zb-za)/5; op-=t>20?20:t; } int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb); if(wb>wa){ int t=(wb-wa)/30; op-=t>15?15:t; } }
            if(kd[a].rtr==0&&kd[b].rtr!=0) op-=10; else if(kd[a].rtr==1&&kd[b].rtr==0) op-=10; else if(kd[a].rtr==kd[b].rtr) op+=5;
            op+=worldAge()==5?(int)(rnd()%17)-8:(int)(rnd()%5)-2; if(op>100) op=100; if(op<-100) op=-100; kd[a].rel[b]=(s8)op; } }
    for(int a=0;a<KN_N;a++) if(kd[a].on){
        if(kd[a].plot){ int b=kd[a].plot-1; if(!kd[b].on||(kd[a].war>>b&1)||(kd[a].allies>>b&1)||(kd[a].rel[b]>-10&&whisT[a]!=b+1)){ kd[a].plot=0; kd[a].pcool=8; whisT[a]=0; } else if(++kd[a].plotT>=3){ int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb); kd[a].plot=0; kd[a].pcool=8; if(whisT[a]==b+1||wa*10>=wb*6) kdDeclare(a,b); whisT[a]=0; } }
        else if(kd[a].pcool) kd[a].pcool--;
        else for(int b=0;b<KN_N;b++) if(b!=a&&kd[b].on&&!(kd[a].war>>b&1)&&!(kd[a].allies>>b&1)&&!kd[a].truce&&kd[a].lord!=b+1&&kd[b].lord!=a+1){ int d=kdDist(a,b); int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb); if(kd[a].rel[b]<-25&&d<70&&wa*10>=wb*6&&(int)(rnd()%100)<(kd[a].rtr==0?35:10)){ kd[a].plot=(u8)(b+1); kd[a].plotT=0; char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); siprintf(s,"%s PLOTS WAR AGAINST %s",x,y); logEv(s); break; } }
        for(int b=a+1;b<KN_N;b++) if(kd[b].on){
            int war=kd[a].war>>b&1, ally=kd[a].allies>>b&1; int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb);
            if(war){ kd[a].weary++; kd[b].weary++; int ww=kd[a].weary; int spite=kd[a].spite||kd[b].spite;
                int stop=ww>40||wa*5<wb||wb*5<wa||(!spite&&ww>=8&&(int)(rnd()%100)<ww*3+((kd[a].rlg==1||kd[b].rlg==1)?10:0)&&(kd[a].rtr==1||kd[b].rtr==1||wa*2<wb||wb*2<wa||ww>20));   // peace talks
                if(stop){ kdWar(a,b,0); kd[a].spite=kd[b].spite=0; kd[a].truce=kd[b].truce=10; char x[24], y[24], s[72]; kdName(a,x); kdName(b,y);
                    int win=wa*2<=wb?b:wb*2<=wa?a:-1; if(win>=0){ int lo=win==a?b:a; renown(win,10); { char w1[24], l1[24], r1[72]; kdName(win,w1); kdName(lo,l1); siprintf(r1,"WAR RECORD: %.18s BEAT %.18s IN %d YEARS",w1,l1,year-(int)warYr[a][b]); logEv(r1); } if(!kd[lo].lord&&!kd[win].lord){ kd[lo].lord=(u8)(win+1); char t[24], u[24]; kdName(lo,t); kdName(win,u); siprintf(s,"%s BECOMES A VASSAL OF %s",t,u); logEv(s); } else { siprintf(s,"PEACE BETWEEN %s AND %s",x,y); logEv(s); } } else { siprintf(s,"PEACE BETWEEN %s AND %s",x,y); logEv(s); } } }
            else if(!ally&&!(kd[a].lord==b+1||kd[b].lord==a+1)){
                if(LAWON(LW_DIPLOMACY)&&kd[a].rel[b]>35&&kd[b].rel[a]>35&&(rnd()&3)==0){ int na=__builtin_popcount(kd[a].allies), nb=__builtin_popcount(kd[b].allies); if(na<3&&nb<3){ kd[a].allies|=(u16)(1<<b); kd[b].allies|=(u16)(1<<a); stAllies++; renown(a,5); renown(b,5); char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); siprintf(s,"%s AND %s ALLY",x,y); logEv(s); } }
                else if(kd[a].rel[b]>10&&kd[b].rel[a]>10&&kdDist(a,b)<90){ kd[a].trade=kd[b].trade=1; kd[a].tradeW|=(u16)(1<<b); kd[b].tradeW|=(u16)(1<<a); } }
            else if(ally&&(kd[a].rel[b]<10||kd[b].rel[a]<10)){ kd[a].allies&=(u16)~(1<<b); kd[b].allies&=(u16)~(1<<a); char x[24], y[24], s[72]; kdName(a,x); kdName(b,y); siprintf(s,"%s AND %s BREAK THEIR ALLIANCE",x,y); logEv(s); } } }
    // soldiers march on the nearest enemy city; rebellious cities of big kingdoms split off
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]){ int k=vilK[v]-1; Vil*V=&vil[v];
        if(V->foe&&(!vil[V->foe-1].on||!(kd[k].war>>(vilK[V->foe-1]-1)&1))) V->foe=0;
        if(kd[k].war&&!V->foe&&gx.vDef[v]!=1){ int best=-1, bd=80*80; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]&&u!=v&&(kd[k].war>>(vilK[u]-1)&1)){ int dx=vil[u].x-V->x, dy=vil[u].y-V->y, dd=dx*dx+dy*dy; if(dd<bd){ bd=dd; best=u; } } if(best>=0) V->foe=(u8)(best+1); }
        { int c,p,w; kdStats(k,&c,&p,&w); int cap=kd[k].cap?kd[k].cap-1:-1;
          if(cap<0||!vil[cap].on||vilK[cap]!=k+1){ int bp=-1; kd[k].cap=0; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==k+1&&vil[u].pop>=bp){ bp=vil[u].pop; kd[k].cap=(u8)(u+1); } cap=kd[k].cap-1; }
          if(v==cap) vilL[v]=100; else { int tg=50+(kdRStat(k,0)-5)*2-4*polSgn(kx[k].tribute)-3*polSgn(kx[k].tax); int over=c-kdMaxCities(k); if(over>0) tg-=over*25; if(cap>=0){ int dx=vil[cap].x-V->x, dy=vil[cap].y-V->y, dd=dx*dx+dy*dy; int r=0; while(r*r<dd&&r<100) r++; tg+=r<20?10:-(r/10); } if(V->stock>40) tg+=5; if(V->stock<8) tg-=10; if(kd[k].ctr>>5&1) tg+=10; if(worldAge()==1||worldAge()==2) tg+=5; if(worldAge()==0||worldAge()==9) tg-=5; if(worldAge()==5||worldAge()==8) tg-=8;
            if(vilL[v]<tg) vilL[v]++; else if(vilL[v]>tg) vilL[v]--; if(vilL[v]<-100) vilL[v]=-100;
            if(LAWON(LW_REBEL)&&vilL[v]<0&&(c>6||c>kdMaxCities(k))&&year-(int)kd[k].born>=15&&V->pop>=10&&(rnd()%10)==0){ int nk=-1; for(int i=0;i<KN_N;i++) if(!kd[i].on){ nk=i; break; } if(nk>=0){ int cul=kd[k].cul; vilK[v]=0; int n2=kdFound(v); if(n2>=0){ kd[n2].cul=(u8)cul; kd[n2].rlg=kd[k].rlg; kd[n2].lang=kd[k].lang; kd[n2].race=vil[v].race; kdWar(n2,k,1); char n[24], x[24], s[72]; vilName(v,n); kdName(k,x); siprintf(s,"%s REBELS AGAINST %s",n,x); logEv(s); } } } } } }
    // sieges: the stronger side of a war can take the nearest enemy city outright; big cities send out colonies
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]&&vil[v].foe&&vil[vil[v].foe-1].on){ int u=vil[v].foe-1, a=vilK[v]-1, b=vilK[u]-1; if(a<0||b<0||!(kd[a].war>>b&1)) continue; int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb);
        int dx=vil[u].x-vil[v].x, dy=vil[u].y-vil[v].y; if(dx*dx+dy*dy>45*45) continue; if(wa>wb&&vil[v].pop>=vil[u].pop&&(int)(rnd()%100)<6+(wa-wb)*8/(wb+1)) vilConquer(u,v); }
    for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]&&vil[v].pop>=12&&vil[v].lvl>=2&&(rnd()%5)==0){ for(int k=0;k<10;k++){ int x=vil[v].x+(int)(rnd()%41)-20, y=vil[v].y+(int)(rnd()%41)-20; if(vilAny(x,y,9)>=0||!buildable(x,y)) continue; int n=vilFound(vil[v].race,x,y); if(n>=0){ vilK[n]=vilK[v]; for(int q=0;q<4;q++) addCr(vil[v].race,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); char nm[24], s[72]; vilName(n,nm); siprintf(s,"A COLONY: %s IS FOUNDED",nm); logEv(s); } break; } }
    warMatrix(); }
typedef struct { const char*nm, *ds; } Ach;
static const Ach ach[ACH_N]={ {"FIRST SPARK","USE A POWER"},{"TOUCH OF GOD","USE POWERS 100 TIMES"},{"CREATOR","USE POWERS 1000 TIMES"},{"FIRST BLOOD","SLAY A MONSTER"},{"HUNTER","SLAY 50 MONSTERS"},{"EXTERMINATOR","SLAY 300 MONSTERS"},
 {"COLLECTOR","MEET 50 MONSTERS"},{"BESTIARY MASTER","MEET EVERY MONSTER"},{"GENEROUS","GIVE 25 GIFTS"},{"A KINGDOM RISES","SEE A KINGDOM FOUNDED"},{"EMPIRE BUILDER","SEE AN EMPIRE FORM"},{"LORD AND VASSAL","SEE A KINGDOM BECOME A VASSAL"},
 {"WARMONGER","SEE 5 WARS DECLARED"},{"PEACEMAKER","SEE AN ALLIANCE FORMED"},{"CITY PLANNER","GROW A LEVEL 3 HALL"},{"METROPOLIS","HAVE 12 CITIES AT ONCE"},{"SCHOLAR","A KINGDOM LEARNS EVERYTHING"},{"CONQUEROR","SEE 3 CITIES CAPTURED"},
 {"MUTANT","MUTATE 20 CREATURES"},{"ALL THE VIBES","MEET ALL EIGHT CHILL TRAITS"},{"SPECIES SPOTTER","MEET 20 SUBSPECIES"},{"ANCIENT","REACH YEAR 60"},{"STORY'S END","FINISH THE STORY"},{"COMPLETIONIST","FINISH THE GAME 100%"},{"SNACK TIME","GIVE 20 SNACKS"},{"THE COUCH AWAITS","GIVE 100 SNACKS"},
 {"PUPPETEER","USE 5 POLITICAL POWERS"},{"SCHISM","SEE A SHATTERED CROWN"},{"WISE KING","A RULER WITH ALL STATS 7+"},{"BANDIT KING","10 OUTLAWS AT ONCE"},{"HAUNTED","4 GHOSTS AT ONCE"},{"UPLIFTED","MAKE AN ANIMAL SAPIENT"},{"CRAB RAVE","DROP A CRAB BOMB"},{"STOMPED","LET CRABZILLA STOMP"},{"NO MORE KINGS","END THE LAST KINGDOM"},{"TREASURY","A KINGDOM WITH 5000 GOLD"},
 {"LEGENDARY","SEE A LEGENDARY ITEM CARRIED"},{"SLAYER","A BOSS FALLS TO FOLK"},{"GHOST STORY","SEE A GHOST"},{"DEN MOTHER","SAPIENT ANIMALS BUILD A DEN"},{"STANDING STONE","RAISE A MONOLITH"},{"GREY AREA","SEE GREY GOO"},{"FIRE SPIRIT","SEE A FIRE ELEMENTAL"},{"ALLIANCE WEB","SEE 10 ALLIANCES"},{"CENTURY","REACH YEAR 100"},{"DYNASTY","A REIGN OF 120 YEARS"} };
static int denCount, monoN; static int banditCount(void); static int sapientCount(void); static int hzCount(int kind); static int itRar(int i);
static int achOK(int i){ int c=0,p=0,w=0,king=0,emp=0,vas=0,tech=0,l3=0,nv=0; for(int k=0;k<KN_N;k++) if(kd[k].on){ king++; achX[2]|=32; emp+=kd[k].empire; vas+=kd[k].lord!=0; if(kd[k].tech>=9) tech=1; } for(int v=0;v<VIL_N;v++) if(vil[v].on){ nv++; if(vil[v].lvl>=3) l3=1; } (void)c;(void)p;(void)w;
    switch(i){ case 0: return stUse>=1; case 1: return stUse>=100; case 2: return stUse>=1000; case 3: return stKill>=1; case 4: return stKill>=50; case 5: return stKill>=300; case 6: return seenN>=50; case 7: return seenN>=MON_N; case 8: return stGift>=25;
        case 9: return king>0; case 10: return emp>0; case 11: return vas>0; case 12: return stWars>=5; case 13: return stAllies>=1; case 14: return l3; case 15: return nv>=12; case 16: return tech; case 17: return stCapt>=3; case 18: return stMut>=20;
        case 19: return ((trSeen>>TR_CHILL)&255)==255; case 20: return traitMetCount()>=20; case 21: return year>=60; case 22: return storyDone; case 23: return fullDone; case 24: return stSnack>=20; case 25: return stSnack>=100;
        case 26: return __builtin_popcount(achX[1])>=5; case 27: return (achX[2]&1)!=0; case 28: { for(int k=0;k<KN_N;k++) if(kd[k].on&&kdRStat(k,0)>=7&&kdRStat(k,1)>=7&&kdRStat(k,2)>=7&&kdRStat(k,3)>=7) return 1; return 0; }
        case 29: return banditCount()>=10; case 30: return hzCount(5 /*HZ_GHOST*/)>=4; case 31: return sapientCount()>=1; case 32: return (achX[2]&2)!=0; case 33: return (achX[2]&4)!=0; case 34: return king==0&&(achX[2]&32)!=0;
        case 35: { for(int k=0;k<KN_N;k++) if(kd[k].on&&kd[k].gold>=5000) return 1; return 0; }
        case 36: { for(int j=0;j<nCr;j++) if(cr[j].alive&&((cr[j].wep&&itRar(cr[j].wep-1)==3)||(cr[j].arm&&itRar(cr[j].arm-1)==3))) return 1; return 0; }
        case 37: { for(int j=0;j<nCr;j++) if(cr[j].alive&&(crFl[j]&FL_SLAYER)) return 1; return 0; } case 38: return hzCount(5)>=1; case 39: return denCount>0; case 40: return monoN>0; case 41: return (achX[2]&8)!=0; case 42: return (achX[2]&16)!=0;
        case 43: return stAllies>=10; case 44: return year>=100; default: { for(int k=0;k<KN_N;k++) if(kd[k].on&&kd[k].reign>=120) return 1; return 0; } } }
static void achCheck(void){ for(int i=0;i<ACH_N;i++) if(!ACH_HAS(i)&&((i!=29&&i!=31&&i!=36&&i!=37)||(tick&255)==33)&&achOK(i)){ ACH_SET(i); char b[40]; siprintf(b,"ACHIEVED: %s",ach[i].nm); logEv(b); fxReq(FX_UNLOCK); } }
static int stepToward(Cr*c,int i,const Sp*s,int tx,int ty){   // one step towards a tile along the longer axis, then the other
    int dx=tx-c->x, dy=ty-c->y; int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0; int ax=dx<0?-dx:dx, ay=dy<0?-dy:dy; int first=ax>=ay;
    for(int k=0;k<2;k++){ int mx=((k==0)==first)?sx:0, my=((k==0)==first)?0:sy; if(!mx&&!my) continue; int nx=c->x+mx, ny=c->y+my; if(stepOk(s,&nx,&ny)){ moveTo(c,i,nx,ny); return 1; } }
    return 0; }
// ---------------------------------------------------------------------------------------------- terraformers, special animals, evolution and the spawner monsters
static int monCount(void);
static int monAtLeast(int limit);
#define M_TUMOR (ARCH_A+28)
#define M_BIOBLOB (ARCH_A+29)
#define M_PUMPKIN (ARCH_A+30)
#define M_ASSIM (ARCH_A+31)
static int tfOk(int x,int y){ return x>=0&&y>=0&&x<WW&&y<WH&&!bld[y][x]&&!occ[y][x]; }
static void tfSet(int x,int y,int t){ if(world[y][x]!=t) setCell(x,y,(u8)t); }
static int tfWaterNear(int x,int y){ for(int dy=-2;dy<=2;dy++) for(int dx=-2;dx<=2;dx++){ int xx=x+dx, yy=y+dy; if(xx>=0&&yy>=0&&xx<WW&&yy<WH&&world[yy][xx]<=T_WATER&&(dx|dy)&&(dx*dx+dy*dy<=5)) return 1; } return 0; }
static int tfLand(int t){ return t==T_GRASS||t==T_FOREST||t==T_JUNGLE||t==T_SAND||t==T_DESERT||t==T_SWAMP||t==T_DIRT||t==T_SNOW; }
static void terraformTick(int i,Cr*c,const Sp*s){       // diet 6: they work the ground and harm nobody. A heading holds for 32 ticks; one time in three a tile beside it is worked.
    int a=s->a; c->food=100; int h=(int)(hsh(i,(u32)(tick>>5),7)&3); int d=(rnd()&3)?h:(int)(rnd()&3); int nx=c->x+dxs[d], ny=c->y+dys[d]; int ox=c->x, oy=c->y;
    if(stepOk(s,&nx,&ny)) moveTo(c,i,nx,ny); (void)ox; (void)oy;
    if(rnd()%3) return;
    if(a==A_PRINTER){ if((tick&15)==0){ int t=world[c->y][c->x]; if(tfLand(t)){ int cx=c->x+4*dxs[h], cy=c->y+4*dys[h]; for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int xx=cx+dx, yy=cy+dy; if(tfOk(xx,yy)&&world[yy][xx]!=T_LAVA&&world[yy][xx]>T_WATER) tfSet(xx,yy,t); } } } return; }
    int e=(int)(rnd()&3), wx=c->x+dxs[e], wy=c->y+dys[e]; if(!tfOk(wx,wy)) return; int t=world[wy][wx];
    switch(a){
    case A_FINGER: if(((i+(tick>>9))&1)==0){ if(t==T_WATER) tfSet(wx,wy,T_SAND); else if(t==T_DEEP) tfSet(wx,wy,T_WATER); else if(t==T_HILL) tfSet(wx,wy,T_MOUNT); else if(tfLand(t)) tfSet(wx,wy,T_HILL); }
        else { if(t==T_MOUNT) tfSet(wx,wy,T_HILL); else if(t==T_HILL) tfSet(wx,wy,T_DIRT); else if(t==T_SAND) tfSet(wx,wy,T_WATER); else if(t==T_WATER) tfSet(wx,wy,T_DEEP); else if(t==T_GRASS||t==T_DIRT||t==T_FOREST) tfSet(wx,wy,T_SAND); } break;
    case A_BANT: if(t==T_GRASS||t==T_FOREST||t==T_JUNGLE||t==T_SWAMP||t==T_HILL) tfSet(wx,wy,T_DIRT); else if(t==T_MOUNT) tfSet(wx,wy,T_HILL); break;
    case A_RANT: if(t==T_DIRT||t==T_SAND||t==T_DESERT||t==T_GRASS||t==T_SNOW) tfSet(wx,wy,T_HILL); else if(t==T_HILL&&(rnd()&3)==0) tfSet(wx,wy,T_MOUNT); break;
    case A_BLANT: if((t==T_SAND||t==T_DIRT||t==T_GRASS||t==T_DESERT||t==T_SWAMP||t==T_FOREST)&&tfWaterNear(wx,wy)) tfSet(wx,wy,T_WATER); break;
    case A_GANT: if(t==T_DIRT||t==T_SAND||t==T_DESERT) tfSet(wx,wy,T_GRASS); else if(t==T_GRASS&&(rnd()%3)==0) tfSet(wx,wy,T_FOREST); break;
    case A_WORM: if(t==T_FOREST||t==T_JUNGLE||t==T_GRASS||t==T_SWAMP) tfSet(wx,wy,T_DIRT); else if(t==T_DIRT&&(rnd()&3)==0) tfSet(wx,wy,T_SAND); break;
    case A_SSPIDER: if(t==T_GRASS||t==T_DIRT||t==T_HILL||t==T_FOREST||t==T_SWAMP) tfSet(wx,wy,T_SAND); else if(t==T_SAND&&(rnd()%5)==0) tfSet(wx,wy,T_DESERT); break; } }
static void healAround(Cr*c,int r,int amt,int cure){ for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ int x=c->x+dx, y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; Cr*q=&cr[occ[y][x]-1]; if(q==c||!q->alive) continue; int mh=maxHp(q); if(q->hp<mh) q->hp=(u8)(q->hp+amt>mh?mh:q->hp+amt); if(cure&&DISEASE(q)==1) SET_DISEASE(q,2); } }
static void beeMatriarchy(int i,Cr*c){
    int queen=-1, bees=0, bestAge=-1; const int a=sp[c->sp].a;
    for(int j=0;j<nCr;j++){ Cr*q=&cr[j]; if(!q->alive||sp[q->sp].a!=a) continue; int dx=(int)q->x-c->x,dy=(int)q->y-c->y; if(dx*dx+dy*dy>144) continue; bees++; if(crFl[j]&FL_BEEQUEEN){ int age=q->age; if(queen<0||age>bestAge||(age==bestAge&&crId[j]<crId[queen])){ queen=j; bestAge=age; } } }
    if(queen<0){ crFl[i]|=FL_BEEQUEEN; queen=i; }
    if(queen==i){ if((tick&255)==0&&bees<24){ for(int d=0;d<4;d++){ int x=c->x+dxs[d],y=c->y+dys[d]; if(x<0||y<0||x>=WW||y>=WH||occ[y][x]) continue; int n=addCr(c->sp,x,y); if(n>=0) cr[n].age=0; break; } } }
    else { Cr*q=&cr[queen]; int dx=(int)q->x-c->x,dy=(int)q->y-c->y; if(dx*dx+dy*dy>9&&(tick&15)==0) stepToward(c,i,&sp[c->sp],q->x,q->y); }
}
static void specialAnimal(int i,Cr*c,int a){        // the magical and special animals do their one thing every few ticks
    if(((tick+i)&7)!=0) return; int e=(int)(rnd()&3), wx=c->x+dxs[e], wy=c->y+dys[e]; int ok=wx>=0&&wy>=0&&wx<WW&&wy<WH&&!bld[wy][wx]; int t=ok?world[wy][wx]:-1;
    switch(a){
    case A_FAIRY: healAround(c,1,2,0); if(rnd()%200==0){ int x=c->x+dxs[e], y=c->y+dys[e]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]){ Cr*q=&cr[occ[y][x]-1]; if(!q->tr) giveTrait(q,TR_LUCKY,0); } } break;
    case A_UNICORN: healAround(c,2,4,1); break;
    case A_WMAGE: healAround(c,2,6,1); break;
    case A_DRUID: if(ok&&!occ[wy][wx]){ if(t==T_GRASS&&(rnd()%6)==0) tfSet(wx,wy,T_FOREST); else if(t==T_DIRT||t==T_SAND) tfSet(wx,wy,T_GRASS); } break;
    case A_BUD: if(ok&&!occ[wy][wx]&&(t==T_DIRT||t==T_SAND||t==T_DESERT)&&(rnd()%3)==0) tfSet(wx,wy,T_GRASS); break;
    case A_ACID: if(ok&&!occ[wy][wx]&&(t==T_GRASS||t==T_FOREST)&&(rnd()%5)==0) tfSet(wx,wy,T_DIRT); break;
    case A_BUTTERFLY: if(ok&&!occ[wy][wx]&&t==T_DIRT&&(rnd()%10)==0) tfSet(wx,wy,T_GRASS); break; } }
static void evolve(int i){      // an animal becomes its evolved form: sapient, half again the health, +3 damage, half again the years, a crown, and a name of its own
    Cr*c=&cr[i]; crFl[i]|=FL_EVOLVED|FL_SAPIENT; c->hp=(u8)maxHp(c); char nm[48], b[64]; crName(c,nm); siprintf(b,"A %.30s EVOLVES",nm); logEv(b); fxReq(FX_BIRTH); }
static int monMinion(const Cr*c,int arc,int*ox){ int d=(int)(rnd()&3), x=c->x+dxs[d], y=c->y+dys[d]; int sI=8+(arc-8)*16+sp[c->sp].fam; int n=addCr(sI,x,y); if(n>=0&&ox) *ox=n; return n; }
static void spawnerTick(int i,Cr*c,int a){      // the biomass monsters make more of themselves (the world's monster count is the limit)
    int br=arch[a].breed; if(!br||(int)(rnd()%100)>=br/2+1) return; if(monAtLeast(70)) return;
    if(a==M_TUMOR) monMinion(c,M_BIOBLOB,0); else if(a==M_BIOBLOB){ monMinion(c,M_BIOBLOB,0); int t=world[c->y][c->x]; if(t==T_GRASS||t==T_FOREST||t==T_DIRT) tfSet(c->x,c->y,T_SWAMP); } else if(a==M_PUMPKIN) monMinion(c,M_PUMPKIN,0); (void)i; }
static int routed(const Cr*c){ int a=sp[c->sp].a; for(int k=0;k<4;k++) if(rz[k].until>tick&&rz[k].a==a&&c->x-rz[k].x<=10&&rz[k].x-c->x<=10&&c->y-rz[k].y<=10&&rz[k].y-c->y<=10) return 1; return 0; }
static void thinkBody(int i){
    Cr*c=&cr[i]; if(!c->alive) return; const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a];
    if(c->age<255) c->age++; { int dec=(HAS(c,TR_LEAN)||HAS(c,TR_CHILL))?1:HAS(c,TR_MUNCH)?3:2; if(gx.hunger==0&&(c->age%3)==0) dec=0; else if(gx.hunger==2&&(c->age%3)==0) dec++; if(c->food>dec) c->food-=dec; else c->food=0; }
    if(!gx.lBio&&nBio[1]+nBio[2]+nBio[3]){ int b=bioAt(c->x,c->y);      // the ground works on whoever stands on it
        if(b==1){ if(A->diet!=5&&(tick&7)==0&&c->hp>maxHp(c)*4/10&&!HAS(c,TR_ZEN)&&rnd()%20==0) c->hp--; }      // corrupted: it slowly wastes the body down to four tenths, never to death
        else if(b==3){ if((tick&3)==0&&c->hp<maxHp(c)) c->hp++; if(DISEASE(c)==1&&rnd()%40==0) SET_DISEASE(c,2); } }      // enchanted: mends, and wears a plague off
    if(!gx.lAtmo&&A->diet!=5){      // the air and the weather of the whole world
        if(ATV(atO2)<12&&rnd()%30==0){ if(c->hp>3) c->hp-=3; else { killNat(i); return; } }                       // thin air: suffocation
        if(ATV(atTemp)<=-22&&!HAS(c,TR_FROST)&&!HAS(c,TR_CHILL)&&!HAS(c,TR_PERMA)&&rnd()%40==0){ if(c->hp>2) c->hp-=2; }            // an ice age: the cold bites
        if(ATV(atTemp)>=26&&!HAS(c,TR_FIRE)&&!HAS(c,TR_DESERT)&&rnd()%40==0){ if(c->hp>2) c->hp-=2; } }                                // a furnace world
    if(A->diet==5) c->food=100;
    if(c->food==0&&LAWON(LW_HUNGER)){ if(c->hp>2) c->hp-=2; else { killNat(i); return; } }
    { int lim=(A->life+(HAS(c,TR_LONG)?A->life/2:0))*gxLifeK(s->a)/100; if(IS_EVO(c)) lim=lim*15/10; if(lim>254) lim=254; if(!gx.lAge&&c->age>lim){ killNat(i); return; } }
    if((HAS(c,TR_REGEN)||HAS(c,TR_ZEN))&&c->hp<maxHp(c)) c->hp++;
    { int phase=(tick+(int)crId[i])&255, night=((tick>>1)&2047)<256||((tick>>1)&2047)>1792;
      if(HAS(c,TR_HEART)&&(phase==0||phase==128)&&c->hp<maxHp(c)) c->hp++;
      if(HAS(c,TR_MOONCHILD)&&night&&(tick&15)==0&&c->hp<maxHp(c)) c->hp++;
      if(HAS(c,TR_BURNFEET)&&(tick&31)==0) for(int q=0;q<4;q++){ int x=c->x+dxs[q], y=c->y+dys[q]; if(x<0||y<0||x>=WW||y>=WH) continue; burnAt(x,y,36); if((world[y][x]==T_GRASS||world[y][x]==T_FOREST||world[y][x]==T_JUNGLE)&&(rnd()&3)==0) setCell(x,y,T_DIRT); if(occ[y][x]&&occ[y][x]!=(occ[c->y][c->x])&&(rnd()&3)==0) damageCreature(occ[y][x]-1,2,i); }
      if(HAS(c,TR_LAMP)&&(tick&31)==0) healAround(c,1,1,0);
      if(HAS_STATUS(c,ST_ASH_FEVER)&&(tick&63)==7) damageCreature(i,1,-1);
      if(HAS_STATUS(c,ST_COUGH)&&(tick&127)==9) for(int q=0;q<4;q++){ int x=c->x+dxs[q], y=c->y+dys[q]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]){ Cr*z=&cr[occ[y][x]-1]; if(z->alive&&!HAS_STATUS(z,ST_COUGH)&&(rnd()&3)==0) z->sick|=ST_COUGH; } }
      if(HAS_STATUS(c,ST_HANDSOME)&&A->diet==4&&(tick&15)==3){ int v=vilNear(s->a,c->x,c->y,40); if(v>=0){ int vx=vil[v].x-c->x, vy=vil[v].y-c->y; if(vx*vx+vy*vy>16&&stepToward(c,i,s,vil[v].x,vil[v].y)) return; } }
      if(HAS_STATUS(c,ST_MINDLESS)){ int d=(int)(rnd()&3), nx=c->x+dxs[d], ny=c->y+dys[d]; if(stepOk(s,&nx,&ny)) moveTo(c,i,nx,ny); return; }
    }
    if(HAS(c,TR_SYMB)&&(tick&1)==0){ for(int k=0;k<4;k++){ int x=c->x+dxs[k], y=c->y+dys[k]; if(x>=0&&y>=0&&x<WW&&y<WH&&occ[y][x]){ Cr*q=&cr[occ[y][x]-1]; if(q->hp<maxHp(q)) q->hp+=2; } } }
    int t=world[c->y][c->x]; int amask=A->mask|curExtra;
    if(t==T_LAVA&&!((amask>>T_LAVA)&1)){ killNat(i); return; }
    if(!((amask>>t)&1)){ if(c->hp>8) c->hp-=8; else { killNat(i); return; } }
    int diet=A->diet; if((HAS(c,TR_OMNI)||HAS(c,TR_MUNCH))&&(diet==0||diet==1)) diet=2; if(LAWON(LW_PEACEMON)&&diet!=4) diet=0;      // peaceful monsters: animals and monsters only graze
   
    if(diet==5&&A->breed) spawnerTick(i,c,s->a);
    if(diet==6){ terraformTick(i,c,s); return; }
    if(diet<4&&(crFl[i]&FL_SAPIENT)){      // a sapient animal defends its home: a monster within two tiles is fought, with whatever tools it has made
        for(int dy=-2;dy<=2;dy++) for(int dx=-2;dx<=2;dx++){ int x=c->x+dx, y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||arch[sp[cr[j].sp].a].diet!=5) continue; Cr*p=&cr[j]; int dm=hitDmg(c,p); ntHit(c,p,dm); if(!damageCreature(j,dm,i)) gainXp(c); return; } }
    if(s->a==42&&(tick&63)==0) beeMatriarchy(i,c);
    if(diet<4&&s->a>=8){ specialAnimal(i,c,s->a); if(LAWON(LW_EVOLUTION)&&!(crFl[i]&FL_EVOLVED)&&c->age>40&&c->food>=90&&nCr>150&&rnd()%2500==0) evolve(i); }
    if(diet==5){   // a monster: it hunts every living thing in reach (people first), and burns the houses it passes
        int best=-1,bd=99,rd=99; for(int dy=-10;dy<=10;dy++) for(int dx=-10;dx<=10;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||arch[sp[cr[j].sp].a].diet==5||((HAS(&cr[j],TR_CAMO)&&(rnd()&1))||(HAS(&cr[j],TR_GIGGLY)&&rnd()%3==0))) continue;
            int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); int civ=arch[sp[cr[j].sp].a].diet==4; if(!civ&&d>3) continue; if(d<bd){ bd=d; rd=d; best=j; } }   // people are hunted from ten tiles away, animals only when they are in the way
        for(int k=0;k<4;k++) burnAt(c->x+dxs[k],c->y+dys[k],40);
        if(best>=0){ Cr*p=&cr[best];
            if(rd<=1+HAS(c,TR_SPIT)){ int dm=hitDmg(c,p); ntHit(c,p,dm); zombieBite(c,(int)(p-cr)); if(!damageCreature(best,dm,i)){ int px_=p->x, py_=p->y; gainXp(c); c->hp=(u8)(c->hp+12>A->hp?A->hp:c->hp+12); if(s->a==M_ASSIM&&!monAtLeast(70)){ int n_=addCr(8+(M_BIOBLOB-8)*16+s->fam,px_,py_); (void)n_; } } return; }
            if(stepToward(c,i,s,p->x,p->y)) return; }
        else { int v=vilAny(c->x,c->y,60); if(v>=0&&(rnd()&7)) { if(stepToward(c,i,s,vil[v].x,vil[v].y)) return; } }
        if((rnd()&3)!=0){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; if(stepOk(s,&nx,&ny)) moveTo(c,i,nx,ny); }
        return; }
    // eat
    if(c->food<100){
        if(diet==0||diet==2){ if(t==T_GRASS||t==T_JUNGLE||t==T_FOREST){ c->food+=45; if(c->food>120) c->food=120; if(t==T_GRASS&&(rnd()&3)==0) setCell(c->x,c->y,T_DIRT); return; } }
        if(diet==3){ c->food=100; }
        if(diet==4){ int v=vilNear(s->a,c->x,c->y,6); if(v>=0&&vil[v].stock>0){ vil[v].stock--; c->food+=35; if(c->food>120) c->food=120; return; }
            if(t==T_GRASS||t==T_FOREST||t==T_JUNGLE){ c->food+=25; if(c->food>120) c->food=120; return; } }
    }
    if(diet==4&&routed(c)){ int hv=vilNear(s->a,c->x,c->y,30); if(hv>=0){ if(stepToward(c,i,s,vil[hv].x,vil[hv].y)) return; } else { int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; if(stepOk(s,&nx,&ny)) moveTo(c,i,nx,ny); return; } }      // a routed soldier runs for home
    if(diet==4){
        int home=vilNear(s->a,c->x,c->y,14);
        // fight: an enemy people within reach is attacked; otherwise an army walks to the enemy village
        { int best=-1,bd=99;
#ifdef GG_NOCELL
          for(int dy=-3;dy<=3;dy++) for(int dx=-3;dx<=3;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i) continue; int ra=sp[cr[j].sp].a; if(arch[ra].diet!=5&&(ra>=8||(!warM[s->a][ra]&&ra!=s->a)||((ra==s->a)&&!peopleKingdomsAtWar(i,j)))) continue; int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){ bd=d; best=j; } }
#else
          { unsigned em=0x100; for(int r=0;r<8;r++) if(warM[s->a][r]&&r!=s->a) em|=1u<<r; int dd=99; best=cellNearest(i,3,em,&dd); if(best>=0) bd=dd;
            if(warM[s->a][s->a]) for(int dy=-3;dy<=3;dy++) for(int dx=-3;dx<=3;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||sp[cr[j].sp].a!=s->a||!peopleKingdomsAtWar(i,j)) continue; int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){bd=d;best=j;} } }      // (10c: nine cell masks instead of 49 tiles; the same answer)
#endif
          if(best>=0){ Cr*p=&cr[best]; if(bd<=1+HAS(c,TR_SPIT)){ int dm=hitDmg(c,p); ntHit(c,p,dm); zombieBite(c,(int)(p-cr)); if(!damageCreature(best,dm,i)){ gainXp(c); c->food+=20; } return; }
              int dx=p->x-c->x, dy=p->y-c->y; int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(stepOk(s,&nx,&ny)) moveTo(c,i,nx,ny); return; } }
        if(home>=0&&vil[home].foe&&vil[vil[home].foe-1].on&&c->age>6&&c->hp>8){ const Vil*F=&vil[vil[home].foe-1]; int dx=F->x-c->x, dy=F->y-c->y; if(dx*dx+dy*dy>4){ int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(stepOk(s,&nx,&ny)){ moveTo(c,i,nx,ny); return; } }
            else { int fv=vil[home].foe-1; vil[fv].stock=(u16)(vil[fv].stock>10?vil[fv].stock-10:0);
#ifndef GG_NOCAPTURE      // (benchmark builds only, pc_war.c: no village is taken or razed, so a war is only the two armies wearing each other down; a captured village sets its own people against each other)
            if(rnd()%12==0&&vil[fv].pop<2&&(gx.vDef[fv]!=2||(rnd()&3)==0)){ if(vil[fv].pop>0){ if(!vilConquer(fv,home)) vilRaze(fv,1); } else vilRaze(fv,1); }
#endif
            } }
        // settle: a grown person far from their own village starts one
        if(home<0&&c->age>(HAS_STATUS(c,ST_HANDSOME)?7:12)&&c->food>60&&(rnd()%(HAS_STATUS(c,ST_HANDSOME)?2:6))==0){ int fv=vilFound(s->a,c->x,c->y); if(fv>=0){ ntFounder(c,fv); return; } }
        // stay close to home
        if(home>=0){ int dx=vil[home].x-c->x, dy=vil[home].y-c->y; if(dx*dx+dy*dy>64){ int ax=dx>0?1:dx<0?-1:0, ay=dy>0?1:dy<0?-1:0; if(((dx<0?-dx:dx)<(dy<0?-dy:dy))) ax=0; else ay=0; int nx=c->x+ax, ny=c->y+ay; if(stepOk(s,&nx,&ny)){ moveTo(c,i,nx,ny); return; } } }
    }
    // hunt
    if((diet==1||(diet==2&&c->food<50))&&c->food<(HAS(c,TR_FIERCE)?115:90)){
        int best=-1,bd=99; for(int dy=-4;dy<=4;dy++) for(int dx=-4;dx<=4;dx++){ int x=c->x+dx,y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||(HAS(&cr[j],TR_CAMO)&&(rnd()&1))) continue;
                const Arch*B=&arch[sp[cr[j].sp].a]; if(B->diet==1||B->diet==5||B->diet==4&&diet!=1||sp[cr[j].sp].a==s->a) continue; if(diet==2&&B->diet!=0) continue;
                int d=(dx<0?-dx:dx)+(dy<0?-dy:dy); if(d<bd){ bd=d; best=j; } }
        if(best>=0){ Cr*p=&cr[best];
            if(bd<=1+HAS(c,TR_SPIT)){ int dm=hitDmg(c,p); ntHit(c,p,dm); zombieBite(c,(int)(p-cr)); if(!damageCreature(best,dm,i)){ c->food+=80; if(c->food>120) c->food=120; gainXp(c); } return; }
            int dx=p->x-c->x, dy=p->y-c->y; int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0; int try1=(((dx<0?-dx:dx)>(dy<0?-dy:dy))?0:1);
            for(int k=0;k<2;k++){ int ax=(k==try1)?sx:0, ay=(k==try1)?0:sy; if(!ax&&!ay) continue; int nx=c->x+ax, ny=c->y+ay; if(stepOk(s,&nx,&ny)){ moveTo(c,i,nx,ny); return; } } } }
    // A hungry or immature creature cannot breed. Skip the crowd and village
    // scans in that case; neither scan consumes randomness or changes state.
    if(c->food>=80&&c->age>8){
        int crowd=0; if(LAWON(LW_CROWD)){ int x0=c->x-2, x1=c->x+2, y0=c->y-2, y1=c->y+2;
            if(x0<0) x0=0; if(y0<0) y0=0; if(x1>=WW) x1=WW-1; if(y1>=WH) y1=WH-1;
            for(int yy=y0;yy<=y1;yy++) for(int xx=x0;xx<=x1;xx++) crowd+=occ[yy][xx]!=0;
        }      // crowding: no room, fewer births (a packed tile block stops breeding at 13 of 25)
        int bch=A->breed; if(LAWON(LW_CROWD)){ if(crowd>=13) bch=0; else if(crowd>6) bch=bch*(13-crowd)/7; } if(diet==4){ int vv=vilNear(s->a,c->x,c->y,8); if(vv>=0){ bch=bch*(gx.vGrow[vv]==1?28:gx.vGrow[vv]==2?12:20)/10; bch=bch*(100+vilMood(vv)/2)/100; } } if(HAS(c,TR_FERTILE)) bch=bch*3/2; if(HAS(c,TR_LUSTFUL)) bch=bch*5/4; if(HAS_STATUS(c,ST_MOTIVATED)) bch=bch*11/10; if(HAS(c,TR_TINY)) bch=bch*13/10; if(HAS(c,TR_CLEVER)) bch=bch*13/10; if(HAS(c,TR_MUNCH)) bch=bch*14/10; bch=bch*gxBirthK(s->a,diet)/100;
        if(!(((diet==4||sp[c->sp].a<8)?LAWON(LW_CIVBABY):LAWON(LW_ANIBABY)))) bch=0;      // CIV BABIES / ANIMAL BABIES
        if(LAWON(LW_LIMIT)&&diet!=4&&diet!=5&&nCr>WW*WH/3) bch=0;      // WORLD LIMIT: wildlife stops breeding once it fills a third of the world
        if(LAWON(LW_POP100)&&diet==4){ int vp=vilNear(s->a,c->x,c->y,8); if(vp>=0&&vil[vp].pop>=100) bch=0; }
        if((int)(rnd()%100)<bch&&nCr<MAXC-4){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; int n=addCr(c->sp,nx,ny); if(n>=0){ cr[n].age=0; cr[n].food=50; born(&cr[n],&cr[i]); c->food-=40; return; } }
    }
    // wander
    if((rnd()&3)!=0){ int d=rnd()&3; int nx=c->x+dxs[d], ny=c->y+dys[d]; if(stepOk(s,&nx,&ny)) moveTo(c,i,nx,ny); }
}
static void think(int i){ GG_TURN(); Cr*c=&cr[i]; curExtra=c->alive?trMask(c):0; int x0=c->x, y0=c->y; edgeOut=0; thinkBody(i); curExtra=0;
    if(edgeOut&&cEdge==1&&c->alive&&c->x==x0&&c->y==y0&&arch[sp[c->sp].a].diet!=4) killNat(i);   // VANISH: an animal or monster that walks off the map is gone (the peoples turn back)
    edgeOut=0; }
static int monTierMax(void){ int t=(tierOpen-1)/3+year/4; return t>4?4:t; }
static int monCount(void){ int n=0; for(int i=0;i<nCr;i++) n+=cr[i].alive&&arch[sp[cr[i].sp].a].diet==5; return n; }
static int monAtLeast(int limit){ int n=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==5&&++n>=limit) return 1; return 0; }
static int monCountHud(void){ static int c=0; static unsigned t=~0u; if(t!=(unsigned)(tick>>3)){ t=(unsigned)(tick>>3); c=monCount(); } return c; }      // (display only: counted once every 8 ticks; the spawn rules use monCount)
static void monSpawn(void){   // rifts open: monsters of the strength the world has earned appear far from the villages
    if(!monOn||(year<2&&tierOpen<4)) return; int tm=monTierMax(); int cap=(4+tm*6)*WW*WH/16384*(diff==0?6:diff==2?15:10)/10*(worldAge()==0?15:10)/10*(gx.mon==0?5:gx.mon==2?18:10)/10; if(cap<3) cap=3; if(monAtLeast(cap)||(rnd()%3)) return;
    int a; for(int k=0;k<8;k++){ a=ARCH_A+(int)(rnd()%(ARCH_N-ARCH_A)); int tr=monTier[a-ARCH_A]; if(tr<=tm&&(tr>=tm-2||(rnd()&7)==0)) break; if(k==7) a=ARCH_A+(int)(rnd()%4); }
    int s=8+(a-8)*16+randomFamily(a); int grp=monTier[a-ARCH_A]==0?1+(int)(rnd()%3):1;
    int home=-1; { int n=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&(rnd()%(++n))==0) home=v; }   // a rift opens 14 to 40 tiles from a village (any, at random)
    for(int k=0;k<40;k++){ int x=rnd()%WW, y=rnd()%WH; if(home>=0){ int r=14+(int)(rnd()%27); int ang=(int)(rnd()&3); x=vil[home].x+(ang&1?r:-r)+(int)(rnd()%9)-4; y=vil[home].y+(ang&2?r:-r)+(int)(rnd()%9)-4; if(x<0||y<0||x>=WW||y>=WH) continue; }
        if(!walkable(&sp[s],x,y)||occ[y][x]||vilAny(x,y,9)>=0) continue; int n=0; for(int q=0;q<grp;q++) { int k2=addCr(s,x+(q?(int)(rnd()%3)-1:0),y+(q?(int)(rnd()%3)-1:0)); if(k2>=0){ n++; int rr=(int)(rnd()%100); if(tm>=1&&rr<4+tm*3) cr[k2].rk=1; else if(tm>=2&&rr<6+tm*3) cr[k2].rk=2; if(tm>=2&&(rnd()&7)==0) giveTrait(&cr[k2],1+(int)(rnd()%(TR_N-1)),0); cr[k2].hp=(u8)maxHp(&cr[k2]); } } if(n) return; }
}
static void towerFire(void){   // the towers of a village shoot the nearest monsters in reach (the first two in list order within 10 tiles; found through the 8 x 8 cell grid, not by walking every creature)
    for(int v=0;v<VIL_N;v++){ const Vil*V=&vil[v]; if(!V->on||!V->towers) continue; int dm=4+3*(V->towers>4?4:V->towers);
        int cx=V->x, cy=V->y, cw=(gW+7)>>CELL_SH, ch=(gH+7)>>CELL_SH; int x0=cx-10<0?0:cx-10, x1=cx+10>=gW?gW-1:cx+10, y0=cy-10<0?0:cy-10, y1=cy+10>=gH?gH-1:cy+10; int f1=-1, f2=-1;
        for(int gy=y0>>CELL_SH;gy<=(y1>>CELL_SH)&&gy<ch;gy++) for(int gx2=x0>>CELL_SH;gx2<=(x1>>CELL_SH)&&gx2<cw;gx2++){ if(!(cellMask[gy*cw+gx2]&(1<<8))) continue;
            int ya=gy<<CELL_SH, yb=ya+7, xa=gx2<<CELL_SH, xb=xa+7; if(ya<y0) ya=y0; if(yb>y1) yb=y1; if(xa<x0) xa=x0; if(xb>x1) xb=x1;
            for(int y=ya;y<=yb;y++) for(int x=xa;x<=xb;x++){ int j=occ[y][x]-1; if(j<0) continue; const Cr*c=&cr[j]; if(!c->alive||cellKind(c->sp)!=8) continue; int dx=x-cx, dy=y-cy; if(dx*dx+dy*dy>100) continue;
                if(f1<0||j<f1){ f2=f1; f1=j; } else if(f2<0||j<f2) f2=j; } }
        for(int q=0;q<2;q++){ int i=q?f2:f1; if(i<0) break; Cr*c=&cr[i]; damageCreature(i,dm,-1); } } }
enum { WX_CLEAR, WX_RAIN, WX_STORM, WX_SNOW, WX_ASH, WX_MAGIC, WX_RAGE, WX_HEAT };
static int weather, weatherT=900, flashT;     // saved in SaveHdr.weather; 4..7 are the four climate events
static const char *weatherName(int w){
    static const char *const names[]={"CLEAR","RAIN","STORM","SNOW","ASH CLOUD","MAGIC CLOUD","RAGE CLOUD","HEATWAVE"};
    return names[(unsigned)w<8?(unsigned)w:0];
}
static void volcanoTick(void){   // lava creeps over nearby ground now and then, scorching it, and cools to rock in the rain
    if(tick&15) return; int x=rnd()%WW, y=rnd()%WH; if(world[y][x]!=T_LAVA) return; atmoDust(1);
    int dx=(int)(rnd()%3)-1, dy=(int)(rnd()%3)-1, nx=x+dx, ny=y+dy; if(nx<0||ny<0||nx>=WW||ny>=WH) return; u8 t=world[ny][nx];
    if(weather==WX_RAIN||weather==WX_STORM){ if(rnd()%6==0){ setCell(x,y,T_MOUNT); } return; }
    if(!gx.lLava&&LAWON(LW_NATDIS)&&!LAWON(LW_GAIA)&&rnd()%(gx.dis==0?100000:gx.dis==1?270:90)==0&&(t==T_MOUNT||t==T_HILL||t==T_DIRT||t==T_GRASS||t==T_FOREST||t==T_DESERT)){ setCell(nx,ny,T_LAVA); if(occ[ny][nx]) killCr(occ[ny][nx]-1); if(bld[ny][nx]){ bld[ny][nx]=0; putCell(nx,ny); } }
    else if(rnd()%400==0) setCell(x,y,T_MOUNT); }
#include "calamity.h"           // the Calamity pack: tornado, black hole, acid rain, atomic bomb, plague, curse
#include "wild.h"           // Pack C and D: fire elemental, goo, ghosts, crab powers, bandits, sapient animals and dens
#include "disasters.h"       // alien invasion, far-land dragon, moon horrors, biomass, dream ice and garden surprise
enum { EV_COMET, EV_HARVEST, EV_MUNCH, EV_FEST, EV_PROPHET, EV_FAMINE, EV_MIGRATE, EV_GOLDEN, EV_CARAVAN, EV_BLOOM, EV_CRISIS, EV_COLD, EV_N };
static const u8 evW[EV_N][10]={   // weights by the ten world ages: dark, hope, sun, tears, moon, chaos, wonders, ice, ash, despair
    {2,3,3,2,3,5,2,4,5,4},{4,6,6,3,4,2,5,3,1,1},{2,3,4,2,2,3,5,3,2,1},{4,5,4,3,4,1,5,2,2,4},
    {3,3,2,4,5,2,5,3,2,2},{3,1,1,4,2,4,1,3,5,5},{4,3,3,4,3,3,4,2,3,2},{5,4,3,3,2,1,5,2,1,1},
    {3,3,4,3,3,2,4,2,1,1},{2,2,3,5,3,2,5,3,1,1},{1,2,4,1,3,5,2,1,3,5},{2,3,1,2,4,2,1,6,2,3} };
static void eventDo(int e){      // one world event; each one is written to the journal
    char s[72]; int ag=worldAge();
    switch(e){
    case EV_COMET: for(int k=0;k<6;k++){ int x=rnd()%WW, y=rnd()%WH; burnAt(x,y,255); if(!LAWON(LW_GAIA)&&world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_MOUNT) setCell(x,y,(rnd()&1)?T_DIRT:T_LAVA); if(occ[y][x]) killCr(occ[y][x]-1); } siprintf(info,"A COMET SHOWER"); flashT=8; fxReq(FX_METEOR); break;
    case EV_HARVEST: for(int v=0;v<VIL_N;v++) if(vil[v].on) vil[v].stock=(u16)(vil[v].stock+100>400?400:vil[v].stock+100); siprintf(info,"A BOUNTIFUL HARVEST"); fxReq(FX_BLESS); break;
    case EV_MUNCH: { int n=0; for(int i=0;i<nCr&&n<12;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet!=5&&cr[i].tr2==0&&(rnd()&7)==0){ giveTrait(&cr[i],TR_MUNCH,0); cr[i].food=200; n++; } siprintf(info,"THE GREAT MUNCHIES"); fxReq(FX_BIRTH); } break;
    case EV_FEST: for(int v=0;v<VIL_N;v++) vil[v].foe=0; for(int k=0;k<KN_N;k++){ kd[k].war=0; kd[k].weary=0; } warMatrix(); siprintf(info,"A FESTIVAL: ALL FEUDS FORGOTTEN"); fxReq(FX_PEACE); break;
    case EV_PROPHET: { int n=0, ids[KN_N]; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[n++]=k; if(!n) return; int k=ids[rnd()%n], old=kd[k].rlg; kd[k].rlg=(u8)((old+1+rnd()%3)%4); kdName(k,s+0); char t[72]; siprintf(t,"A PROPHET CONVERTS %.18s TO %s",s,rlgN[kd[k].rlg]); logEv(t); siprintf(info,"A WANDERING PROPHET"); KD_DIRTY(); } return;
    case EV_FAMINE: { int n=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&(rnd()%3)==0){ vil[v].stock=(u16)(vil[v].stock>150?vil[v].stock-150:0); n++; } if(!n) return; siprintf(info,"FAMINE"); logEv("A FAMINE EMPTIES THE STORES"); } return;
    case EV_MIGRATE: { int n=0, ids[VIL_N]; for(int v=0;v<VIL_N;v++) if(vil[v].on) ids[n++]=v; if(n<2) return; int v=ids[rnd()%n]; for(int k=0;k<6;k++) addCr(vil[v].race,vil[v].x+(int)(rnd()%7)-3,vil[v].y+(int)(rnd()%7)-3); char t[72]; vilName(v,s); siprintf(t,"MIGRANTS ARRIVE AT %.30s",s); logEv(t); siprintf(info,"MIGRATION"); } return;
    case EV_GOLDEN: for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]){ int l=vilL[v]+15; vilL[v]=(s8)(l>100?100:l); } siprintf(info,"A GOLDEN AGE"); logEv("A GOLDEN AGE LIFTS EVERY CITY"); fxReq(FX_BLESS); break;
    case EV_CARAVAN: { int n=0, ids[KN_N]; for(int k=0;k<KN_N;k++) if(kd[k].on&&kd[k].tech>=2) ids[n++]=k; if(!n) return; int k=ids[rnd()%n]; int g=kd[k].gold+120; kd[k].gold=(u16)(g>60000?60000:g); kdName(k,s); char t[72]; siprintf(t,"A MERCHANT CARAVAN ENRICHES %.28s",s); logEv(t); siprintf(info,"A MERCHANT CARAVAN"); } return;
    case EV_BLOOM: { static const u8 tt[6]={TR_GIGGLY,TR_ZEN,TR_FLOATY,TR_LEAN,TR_REGEN,TR_FERTILE}; int n=0; for(int i=0;i<nCr&&n<8;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet!=5&&cr[i].tr2==0&&(rnd()&7)==0){ giveTrait(&cr[i],tt[rnd()%6],0); n++; } if(!n) return; siprintf(info,"A MUTANT BLOOM"); logEv("A MUTANT BLOOM CHANGES THE WILD"); fxReq(FX_BIRTH); } return;
    case EV_CRISIS: { int n=0, ids[KN_N]; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[n++]=k; if(!n) return; int k=ids[rnd()%n]; kd[k].reign=250; kdName(k,s); char t[72]; siprintf(t,"A SUCCESSION CRISIS SHAKES %.26s",s); logEv(t); siprintf(info,"SUCCESSION CRISIS"); } return;
    case EV_COLD: weather=3; weatherT=1800; siprintf(info,"A COLD SNAP"); logEv("A COLD SNAP BLANKETS THE LAND"); break; }
    (void)ag; }
static void eventTick(void){   // now and then the world has an event of its own: a weighted table by the age of the world (disasters off take the comet out instead of doubling the harvests)
    if(tick<1500||(tick%1500)!=0||rnd()%3) return; int ag=worldAge(), w[EV_N], tot=0;
    for(int e=0;e<EV_N;e++){ w[e]=evW[e][ag]; if(e==EV_COMET&&(gx.dis==0||(gx.dis==1&&rnd()%3)||!LAWON(LW_NATDIS))) w[e]=0; if(e==EV_CRISIS&&!LAWON(LW_SUCCESSION)) w[e]=0; if(e==EV_COLD&&!LAWON(LW_WEATHER)) w[e]=0; tot+=w[e]; }
    if(tot<=0) return; int r=(int)(rnd()%(u32)tot), e=0; while(e<EV_N-1&&r>=w[e]){ r-=w[e]; e++; } eventDo(e); }
static void weatherTick(void){
    if(!LAWON(LW_WEATHER)){ if(weather) weather=0; if(flashT>0) flashT--; return; }
    if(gx.wx){ weather=gx.wx-1; weatherT=3000; }
    if(--weatherT<=0){
        int r=(int)(rnd()%100), cold=(year&3)==3;
        weather=r<50?WX_CLEAR:r<68?(cold?WX_SNOW:WX_RAIN):r<76?(cold?WX_SNOW:WX_STORM):r<84?(cold?WX_SNOW:WX_RAIN):r<88?WX_ASH:r<92?WX_MAGIC:r<96?WX_RAGE:WX_HEAT;
        weatherT=weather?1200+(int)(rnd()%2400):900+(int)(rnd()%1800);
        siprintf(info,"%s",weather==WX_RAIN?"IT BEGINS TO RAIN":weather==WX_STORM?"A STORM ROLLS IN":weather==WX_SNOW?"SNOW FALLS":weather==WX_ASH?"AN ASH CLOUD GATHERS":weather==WX_MAGIC?"A MAGIC CLOUD DRIFTS IN":weather==WX_RAGE?"A RAGE CLOUD GATHERS":weather==WX_HEAT?"A HEATWAVE BEGINS":"THE SKY CLEARS");
        if(weather>=WX_ASH) logEv(info);
    }
    if((weather==WX_RAIN||weather==WX_STORM)&&(tick&7)==0){ int x=rnd()%WW, y=rnd()%WH; u8 t=world[y][x]; if(t==T_DIRT&&(rnd()&3)==0) setCell(x,y,T_GRASS); else if(t==T_LAVA&&(rnd()&15)==0) setCell(x,y,T_MOUNT); }
    if((tick&7)==0 && weather>=WX_ASH){
        int x=(int)(rnd()%WW), y=(int)(rnd()%WH), t=world[y][x], ci=occ[y][x]?(int)occ[y][x]-1:-1;
        if(weather==WX_ASH){ atmoDust(1); if(t==T_GRASS||t==T_FOREST||t==T_JUNGLE){ if((rnd()&3)==0) setCell(x,y,t==T_GRASS?T_DIRT:T_GRASS); } if(ci>=0&&cr[ci].alive&&arch[sp[cr[ci].sp].a].diet!=5) damageCreature(ci,1,-1); }
        else if(weather==WX_MAGIC){ if(ci>=0&&cr[ci].alive){ Cr*c=&cr[ci]; c->hp=(u8)(c->hp+2>maxHp(c)?maxHp(c):c->hp+2); if(c->food<120)c->food+=2; } else if(t==T_GRASS&&(rnd()&7)==0) setCell(x,y,T_FOREST); }
        else if(weather==WX_RAGE){ if(ci>=0&&cr[ci].alive&&arch[sp[cr[ci].sp].a].diet!=5) damageCreature(ci,2,-1); if(t==T_FOREST||t==T_JUNGLE){ burnAt(x,y,60); if((rnd()&3)==0)setCell(x,y,T_DIRT); } }
        else if(weather==WX_HEAT){ atmoDust(1); if(t==T_SWAMP&&(rnd()&3)==0)setCell(x,y,T_DIRT); else if((t==T_GRASS||t==T_FOREST)&&(rnd()&7)==0)setCell(x,y,T_DIRT); if(ci>=0&&cr[ci].alive&&arch[sp[cr[ci].sp].a].diet!=5) damageCreature(ci,1,-1); }
    }
    if(weather==WX_STORM&&gx.dis&&LAWON(LW_NATDIS)&&(rnd()%(gx.dis==1?1500:500))==0){ int x=rnd()%WW, y=rnd()%WH; burnAt(x,y,200); if(occ[y][x]) killCr(occ[y][x]-1); if(world[y][x]==T_FOREST&&(rnd()&3)==0) setCell(x,y,T_DIRT); flashT=8; fxReq(FX_LIGHTNING); }
    if(flashT>0) flashT--; }
static int cursor=0, vilCur=0;
static void atmoTick(void){        // every 64 ticks: sample the land, move the gases, warm or cool the world, and let the climate act on the ground
    if(!(!gx.lAtmo)) return;
    int fF=0,fG=0,fS=0,fW=0,fL=0; for(int k=0;k<256;k++){ int x=(int)(rnd()%WW), y=(int)(rnd()%WH); switch(world[y][x]){ case T_FOREST: case T_JUNGLE: fF++; break; case T_GRASS: case T_SWAMP: fG++; break; case T_SNOW: fS++; break; case T_WATER: case T_DEEP: fW++; break; case T_LAVA: fL++; break; } }
    int hh=0; vilTotals(&hh); int alive=nCr; int plants=fF*2+fG;
    atCO2+= fL*16+alive*16/3000+hh*16/200+atFire*16/10 - plants*(atCO2>60*16?60*16:atCO2)/1500 - fW*atCO2/16000;      // (plants can only eat so fast: a big puff of CO2 lingers)
    atO2 += plants*16/160 - alive*16/6000 - atFire*2 + (21*16-atO2)/8;
    atDust-= atDust/10+8; atFire=atFire*3/4;
    if(atCO2<0) atCO2=0; if(atCO2>150*16) atCO2=150*16; if(atO2<0) atO2=0; if(atO2>60*16) atO2=60*16; if(atDust<0) atDust=0;
    int tgt=(ATV(atCO2)-30)*45/100 - ATV(atDust)*30/100 + (fF+fG-130)/14 - (fS-20)/3; if(tgt<-35) tgt=-35; if(tgt>40) tgt=40;
    atTemp+=(tgt*16-atTemp)/10;
    int T=ATV(atTemp);
    if(T>=12){ int K=(T-10)/3+1; if(K>8) K=8; for(int k=0;k<K;k++){ int x=(int)(rnd()%WW), y=(int)(rnd()%WH); u8 t=world[y][x];
            if(t==T_SNOW){ int wet=0; for(int d=0;d<4;d++){ int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&(world[ny][nx]==T_WATER||world[ny][nx]==T_DEEP)) wet=1; } setCell(x,y,wet?T_WATER:T_GRASS); }
            else if(T>=22&&(t==T_GRASS||t==T_FOREST)&&(rnd()&3)==0) setCell(x,y,T_DESERT); } }
    else if(T<=-12){ int K=(-T-10)/3+1; if(K>8) K=8; for(int k=0;k<K;k++){ int x=(int)(rnd()%WW), y=(int)(rnd()%WH); u8 t=world[y][x];
            if(t==T_GRASS||t==T_FOREST||t==T_JUNGLE||t==T_SWAMP||t==T_DIRT||t==T_DESERT){ if(rnd()%3==0&&!bld[y][x]) setCell(x,y,T_SNOW); }
            else if(t==T_WATER){ int sn=0; for(int d=0;d<4;d++){ int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&world[ny][nx]==T_SNOW) sn=1; } if(sn&&(rnd()&1)) setCell(x,y,T_SNOW); } } }
    atHist[atHistN%48]=(s8)(T<-40?-40:T>40?40:T); atHistN++;
    { int f=0; if(ATV(atO2)<12) f|=1; if(T>=20) f|=2; if(T<=-18) f|=4; if(ATV(atDust)>=40) f|=8; if(ATV(atCO2)>=80) f|=16; int nw=f&~atFlag;
      if(nw&1) logEv("THE AIR IS THINNING"); if(nw&2) logEv("THE WORLD IS BURNING UP"); if(nw&4) logEv("AN ICE AGE BEGINS"); if(nw&8) logEv("A DUST VEIL DARKENS THE SKY"); if(nw&16) logEv("THE GREENHOUSE RUNS AWAY");
      atFlag=f; } }
static int bioIdle;
static void biomeTick(void){       // biomes creep outward over the ground, fight each other (enchantment beats corruption), and the infernal one lights fires by itself
    int tot=nBio[1]+nBio[2]+nBio[3]; if(!tot) return;
    if(bioQn==0){ if(++bioIdle>=2048){ bioIdle=0; bioRescan(); } }
    for(int k=0;k<2&&bioQn>0;k++){ int qi=(int)(rnd()%(unsigned)bioQn); int x=(int)(bioQ[qi]&0xFFFF), y=(int)(bioQ[qi]>>16); int b=bioAt(x,y);
        if(!b||!bioOpen(x,y)){ bioQ[qi]=bioQ[--bioQn]; continue; }      // gone, or surrounded: it leaves the edge list
        int d=(int)(rnd()&3); int nx=x+dxs[d], ny=y+dys[d]; if(nx<0||ny<0||nx>=WW||ny>=WH) continue;
        int t=world[ny][nx]; if(t<=T_WATER||t==T_LAVA||t==T_MOUNT) continue; int nb=bioAt(nx,ny); if(nb==b) continue;
        int win=nb==0?(b==1?3:b==3?4:6):(b==3&&nb==1?3:b==2?12:8);      // 1 in win to take the neighbouring tile
        if((int)(rnd()%(unsigned)win)) continue; bioSet(nx,ny,b); if(nx>=winX-1&&nx<winX+34&&ny>=winY-1&&ny<winY+26) putCell(nx,ny); }
    if(nBio[2]>=10&&(rnd()&15)==0){ for(int tr=0;tr<40;tr++){ int x=(int)(rnd()%WW), y=(int)(rnd()%WH); if(bioAt(x,y)==2){ burnAt(x,y,160); for(int q=0;q<4;q++) burnAt(x+dxs[q],y+dys[q],90); if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(arch[sp[c->sp].a].diet!=5&&!HAS(c,TR_FIRE)){ if(c->hp>12) c->hp-=12; else killCr(occ[y][x]-1); } } break; } } } }
#include "ggfar.h"      // (10d: tiers and distant battles)
static void thoughtCloudTick(void){
    if(!LAWON(LW_THOUGHTS)||(tick&255)!=77||rnd()%4) return;
    int x=(int)(rnd()%WW), y=(int)(rnd()%WH), race=0; u8 t=world[y][x];
    if(t==T_WATER||t==T_DEEP||t==T_LAVA) return;
    if(t==T_MOUNT||t==T_HILL) race=2; else if(t==T_DESERT||t==T_SAND) race=3; else if(t==T_SWAMP) race=4; else if(t==T_FOREST||t==T_JUNGLE) race=1; else if(t==T_SNOW) race=5;
    for(int r=0;r<=3;r++) for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ if((dx<0?-dx:dx)+(dy<0?-dy:dy)!=r) continue; int xx=x+dx,yy=y+dy; if(xx<0||yy<0||xx>=WW||yy>=WH) continue; if(addCr(race,xx,yy)>=0){ logEv("A CLOUD OF THOUGHTS DROPS A PERSON"); fxReq(FX_BIRTH); return; } }
}
static void simulate(void){
    if(paused) return;
    if(!gx.lBio) biomeTick();
    boatTick(); thoughtCloudTick();
    if((tick&63)==17) atmoTick();
    { static int gcx=-1, gcy=-1; int x0=(camX>>3)-1, y0=(camY>>3)-1; int camMoved=(x0!=gcx||y0!=gcy); int fullScan=camMoved||(tick&63)==0; if(fullScan){ gcx=x0; gcy=y0; glideOn=1; }      // glide into the tile: only the creatures near the screen (a crowd of thousands is not walked through every tick), and only while something is gliding
      if(glideOn){ int rem=0; for(int y=y0<0?0:y0;y<y0+27&&y<WH;y++) for(int x=x0<0?0:x0;x<x0+35&&x<WW;x++){ if(!cellTot[cellOf(x,y)]){ x|=7; continue; } int j=occ[y][x]-1; if(j<0) continue; Cr*c=&cr[j]; if(c->ox) c->ox+= c->ox<0?1:-1; if(c->oy) c->oy+= c->oy<0?1:-1; if(c->ox||c->oy) rem=1; } glideOn=rem; } }
    int n=nCr; if(n==0) return; int per=n>96?96+(n>2400?(n-2400)/300:0):n;     // everyone takes a turn in rotation; a bigger crowd gets a few more turns a tick
#ifdef GG_FULLTURNS
    per=n;      // (benchmark builds only, pc_war.c: every creature is visited every tick, so each thinks at its own pace: the full per-unit run that step 10d's tiers are compared with)
#endif
    GG_FRAME_BEGIN(creatures);
    if(farOn()&&wgRolling()) wgSlice();      // (10f) a few creatures a tick have their group looked at again, so the count every 256 ticks is a cheap pass
    farTick();      // (10d) the far battles, one roll each, and which armies are far and touching this tick
    if(farOn()){ int period=(n+per-1)/per; windowThink();      // tier A: the camera window thinks every tick; tier C: far soldiers also march
        for(int k=0;k<per;k++){ if(cursor>=nCr) cursor=0; Cr*c=&cr[cursor]; if(c->alive&&!inWin(c->x,c->y)){ if(++c->t>=effSpd(c)){ c->t=0; think(cursor); } if(cursor<nCr&&cr[cursor].alive) farStride(cursor,period); } cursor++; } }
    else {
    for(int k=0;k<per;k++){ if(cursor>=nCr) cursor=0; Cr*c=&cr[cursor]; if(c->alive&&++c->t>=effSpd(c)) { c->t=0; think(cursor); } cursor++; }
    if(n>600){ int x0=camX>>3, y0=camY>>3; for(int k=0;k<40;k++){ int x=x0+(int)(rnd()%32), y=y0+(int)(rnd()%20); if(x>=WW||y>=WH) continue; int j=occ[y][x]-1; if(j>=0&&cr[j].alive){ Cr*c=&cr[j]; if(++c->t>=effSpd(c)){ c->t=0; think(j); } } } }   // and whoever is on screen is always played at full pace
    }
    GG_FRAME_END(creatures);
    if(LAWON(LW_EROSION)&&(tick&3)==0){ for(int k=0;k<4;k++){ int x=rnd()%WW, y=rnd()%WH; u8 t=world[y][x]; if((t==T_GRASS||t==T_DIRT||t==T_HILL)&&!bld[y][x]&&(rnd()%60)==0){ for(int d=0;d<4;d++){ int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&(world[ny][nx]==T_WATER||world[ny][nx]==T_DEEP)){ setCell(x,y,T_SAND); break; } } } } }      // EROSION: the shore crumbles into sand
    // the ground heals: dirt turns back to grass beside grass, lava cools
    for(int k=0;k<12;k++){ int x=rnd()%WW, y=rnd()%WH; u8 t=world[y][x]; if(t==T_DIRT){ int d=rnd()&3; int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&(world[ny][nx]==T_GRASS||world[ny][nx]==T_FOREST)) setCell(x,y,T_GRASS); }
        else if(t==T_GRASS&&LAWON(LW_TREES)&&(rnd()&63)==0){ int d=rnd()&3; int nx=x+dxs[d], ny=y+dys[d]; if(nx>=0&&ny>=0&&nx<WW&&ny<WH&&world[ny][nx]==T_FOREST&&(rnd()&7)==0) setCell(x,y,T_FOREST); } }
    if((tick&15)==0){ int steps=(tick>>4)*VIL_N/40-((tick>>4)-1)*VIL_N/40; for(int s_=0;s_<steps;s_++){ vilCur=(vilCur+1)%VIL_N; if(LAWON(LW_GROWTH)) vilTick(vilCur); if(vilCur==0) warMatrix(); } } /* every village is looked at once in 640 ticks, however many slots the build has (40 slots: one a step, as before) */ if((tick&127)==64) kingdomTick(); if((tick&63)==33) achCheck();
    GG_FRAME_BEGIN(disasters);
    weatherTick(); volcanoTick(); eventTick(); calamityTick();
    GG_FRAME_END(disasters);
    if((tick&63)==0) monSpawn();
    if((tick&15)==0) towerFire();
    if(((tick)&255)==0){
        GG_FRAME_BEGIN(maintenance);
        GG_FRAME_BEGIN(compaction);
        compactCreatures(0);
        GG_FRAME_END(compaction);
        GG_FRAME_BEGIN(notables);
        ntSweep();
        GG_FRAME_END(notables);
        GG_FRAME_BEGIN(warIndex);
        if(wgRolling()) wgCommit(); else wgBuild();
        GG_FRAME_END(warIndex);
        if((tick%2048)==0){ year++; fxReq(FX_YEAR); } unlockTiers();
        GG_FRAME_END(maintenance);
    }
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
// ---- loot (Pack G): an item's rarity and modifiers are worked out from its index, so nothing is saved and every save agrees
static const char* const modNm[10]={"BAL","FIN","HP","KNOW","MAST","STR","PROT","SHARP","SPD","TRUTH"}, * const rarNm[4]={"","RARE","EPIC","LEGEND"};
static int itRar(int i){ if(it[i].kind!=IK_WEAPON&&it[i].kind!=IK_ARMOR) return 0; int h=(int)(hsh(i,0,0x4C4F4F54)%100); return h<60?0:h<85?1:h<96?2:3; }
static int itMods(int i){ int r=itRar(i), m=0, n=0; for(int k=0;n<r&&k<24;k++){ int b=(int)(hsh(i,k+1,0x4D4F4453)%10); if(!(m>>b&1)){ m|=1<<b; n++; } } return m; }     // up to three of the ten, as many as the rarity
static int itBonus(int i){ int m=itMods(i), r=itRar(i); if(it[i].kind==IK_WEAPON) return r*2+((m>>5&1)?2:0)+((m>>7&1)?2:0)+((m>>4&1)?1:0); return r*2+((m>>6&1)?2:0)+((m>>2&1)?2:0); }
static int itPow(int i){ return it[i].val+itBonus(i); }
static int gearAtk(const Cr*c){ return c->wep?itPow(c->wep-1):0; }
static void giveTool(Cr*c,int armor){ int want=armor?IK_ARMOR:IK_WEAPON; for(int k=0,j=(int)(rnd()%nIt);k<nIt;k++,j=(j+1)%nIt) if(it[j].kind==want&&it[j].tier<=1){ if(armor) c->arm=(u16)(j+1); else c->wep=(u16)(j+1); return; } }      // (a sapient animal near its den makes simple gear)
static int armDef(const Cr*c){ return c->arm?itPow(c->arm-1)/2:0; }
static void itTag(char*o,int i){ int m=itMods(i); o[0]=0; for(int b=0;b<10;b++) if(m>>b&1){ strcat(o,"+"); strcat(o,modNm[b]); strcat(o," "); } }
static int itDrop(int seed,int minRar){ for(int k=0;k<400;k++){ int i=(int)(hsh(seed,k,0x44524F50)%(u32)nIt); if((it[i].kind==IK_WEAPON||it[i].kind==IK_ARMOR)&&itOpen(i)&&itRar(i)>=minRar) return i; } return -1; }
static void lootDrop(int mi){      // a slain elite or boss drops its gear: the nearest folk within five tiles picks it up
    Cr*m=&cr[mi]; int best=-1, bd=26; for(int dy=-5;dy<=5;dy++) for(int dx=-5;dx<=5;dx++){ int x=m->x+dx, y=m->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(!cr[j].alive||arch[sp[cr[j].sp].a].diet!=4) continue; int d=dx*dx+dy*dy; if(d<bd){ bd=d; best=j; } }
    if(best<0) return; int i=itDrop((int)(mi*7+tick),m->rk==2?3:2); if(i<0) return; if(it[i].kind==IK_WEAPON) cr[best].wep=(u16)(i+1); else cr[best].arm=(u16)(i+1); char nm[48], b[48]; crName(&cr[best],nm); siprintf(b,"%.20s TAKES %.20s",nm,it[i].nm); logEv(b); }
static void itSay(char*out,int i){ const Item*I=&it[i]; static const char*kn[8]={"ATK","HP","POTION","SEED","FOOD","RELIC","GEM","TOOL"}; if(I->kind==IK_WEAPON||I->kind==IK_ARMOR) siprintf(out,"%s%s%s %s+%d",rarNm[itRar(i)],itRar(i)?" ":"",I->nm,kn[I->kind],itPow(i)); else if(I->kind==IK_TOOL) siprintf(out,"%s: %s",I->nm,toolFx[I->val]); else siprintf(out,"%s",I->nm); }
// ---------------------------------------------------------------------------------------------- the story: fourteen chapters; each one done opens a tier of items (and stronger monsters) for free play, for good
typedef struct { const char*nm, *goal; u8 kind; u16 target; } Chap;
enum { OB_USE, OB_FOLK, OB_HOMES, OB_PEOPLES, OB_SLAY, OB_GIFT, OB_L3, OB_SEEN, OB_YEAR, OB_FINAL };
#define CHAP_N 14
enum { STORY_FINAL_SLAIN=150, STORY_FINAL_YEAR=16 };
static const Chap chap[CHAP_N]={
 {"LET THERE BE LIGHT","USE A POWER 6 TIMES",OB_USE,6}, {"THE FIRST FOLK","20 FOLK IN YOUR WORLD",OB_FOLK,20}, {"A ROOF OVERHEAD","BUILD 8 HOMES",OB_HOMES,8}, {"TWO PEOPLES","TWO PEOPLES WITH VILLAGES",OB_PEOPLES,2},
 {"THE FIRST DARKNESS","SLAY 5 MONSTERS",OB_SLAY,5}, {"ARMS FOR THE FAITHFUL","GIVE 5 GIFTS",OB_GIFT,5}, {"A HALL OF STONE","GROW A LEVEL 3 HALL",OB_L3,1}, {"THE BESTIARY","MEET 15 KINDS OF MONSTER",OB_SEEN,15},
 {"AGE OF KINGS","REACH YEAR 8",OB_YEAR,8}, {"THE HORDE","SLAY 40 MONSTERS",OB_SLAY,40}, {"THE GREAT CITY","BUILD 40 HOMES",OB_HOMES,40}, {"A GOD'S LIBRARY","MEET 80 KINDS OF MONSTER",OB_SEEN,80},
 {"THE LONG PEACE","250 FOLK IN YOUR WORLD",OB_FOLK,250}, {"GODHOOD","SLAY 150 MONSTERS AND REACH YEAR 16",OB_FINAL,1} };
static int chapVal(int kind){
    int hh; switch(kind){ case OB_USE: return (int)stUse; case OB_FOLK: return vilTotals(&hh); case OB_HOMES: vilTotals(&hh); return hh; case OB_SLAY: return (int)stKill; case OB_GIFT: return (int)stGift;
        case OB_SEEN: return seenN; case OB_YEAR: return year; case OB_FINAL: return stKill>=STORY_FINAL_SLAIN&&year>=STORY_FINAL_YEAR;
        case OB_PEOPLES: { int m=0; for(int v=0;v<VIL_N;v++) if(vil[v].on) m|=1<<vil[v].race; int n=0; while(m){ n+=m&1; m>>=1; } return n; }
        default: { int l3=0; for(int v=0;v<VIL_N;v++) l3+=vil[v].on&&vil[v].lvl>=3; return l3; } } }
// ---------------------------------------------------------------------------------------------- the cast and their words
enum { W_WISP, W_MIRA, W_GRUUL, W_ELOWEN, W_THORGRIM, W_HOLLOW, W_DAVE, W_N };
static const char* const whoNm[W_N]={"WISP","MIRA","GRUUL","ELOWEN","THORGRIM","THE HOLLOW KING","DAVE THE COUCH WIZARD"};
static const char* const whoRole[W_N]={"THE FIRST SPARK","KEEPER OF THE HEARTH","WARDEN OF THE RIVER",
    "KEEPER OF NAMES","SMITH OF THE HALL","THE GOD WHO CAME BEFORE","WIZARD OF THE LONG COUCH"};
typedef struct { u8 who; const char*txt; } Dl;
static const Dl dl0[]={
    {W_WISP,"That seed took root. I'm Wisp. I can show you dry land. You get to decide what grows there."},
    {W_WISP,"Grass is selected. Tap the map six times to try your power. One patch at a time is fine."},{0,0}};
static const Dl dl1[]={
    {W_MIRA,"I left a cup on the porch to catch rain. Anyone who follows the light can have a drink. Stay long enough and we'll need a village."},
    {W_WISP,"Choose SETTLE and tap open grass to found a village. Leave it running so families can grow. Use SPAWN to bring more folk to the area."},{0,0}};
static const Dl dl2[]={
    {W_MIRA,"Thorgrim put the last nail in our roof. The goat claimed it before we could. Now the neighbors could use someplace dry."},
    {W_WISP,"Let the villagers build eight homes. Leave patches of open ground near the hall and give them time to gather supplies."},{0,0}};
static const Dl dl3[]={
    {W_GRUUL,"I guard the ford. Mira brought me a drink while I was busy trying to scare her off. We agreed both banks should have a place to live."},
    {W_WISP,"Select SPAWN and use X or Y to choose another founding species. Then switch to SETTLE and found their village on open ground."},
    {W_DAVE,"I was told this was a bridge meeting. I brought dip. Please tell me someone thought to bring the bridge."},{0,0}};
static const Dl dl4[]={
    {W_WISP,"The figure in the ruins is the Hollow King. He saw our lamps and sent his creatures down the road. Mira is getting people inside."},
    {W_GRUUL,"Use SMITE on monsters away from our folk. Towers can help hold the approaches. We need five monsters down to reopen the ford."},{0,0}};
static const Dl dl5[]={
    {W_THORGRIM,"Gruul took the first blade. The second piece of iron became a spoon. There are more people waiting for supper than for swords."},
    {W_MIRA,"Pick GIFT, choose an item with X or Y, and tap a creature to give it something. Try food for hungry folk or equipment for a guard."},{0,0}};
static const Dl dl6[]={
    {W_MIRA,"We brought everyone under one roof during the raid. I want a hall big enough to do that and still have room for supper."},
    {W_THORGRIM,"Let one village reach fourteen homes and stockpile supplies. When it has eighty in stock and room to build, it can raise a level three hall."},{0,0}};
static const Dl dl7[]={
    {W_ELOWEN,"The watch saw tracks they couldn't name. I started a bestiary so the next watch knows what went past. The first drawing was mostly teeth."},
    {W_ELOWEN,"The book records new monster kinds when they appear. Keep the world running or use SPAWN to introduce kinds you haven't met."},{0,0}};
static const Dl dl8[]={
    {W_GRUUL,"I thought Mira's cup meant a truce. Turns out she was saving me a seat. Now we take turns keeping the roads safe after dark."},
    {W_MIRA,"Keep the world running until year eight. If a village gets hungry or comes under attack, help it through. I'll keep a place by the fire."},{0,0}};
static const Dl dl9[]={
    {W_THORGRIM,"We braced the west gate and cleared the aisle in the hall. People sang inside so the watch would know they were still there."},
    {W_GRUUL,"The Hollow King's horde is still out there. Help the watch get to forty slain monsters. Use SMITE well away from people at the gate."},
    {W_DAVE,"My evacuation plan has one flaw. The couch doesn't fit through the door. We all have to sit very still until it's over."},{0,0}};
static const Dl dl10[]={
    {W_MIRA,"The gate held. Now the families coming back need homes. The cup on my sill leaks, which makes it perfect for watering beans."},
    {W_WISP,"Work toward forty homes across your villages. Found more villages where there's open land and give the builders room around each hall."},{0,0}};
static const Dl dl11[]={
    {W_ELOWEN,"I followed a monster back to the ford. It was feeding its young. The teeth were worth drawing, but so was what it did with them."},
    {W_ELOWEN,"Let's record eighty kinds in the bestiary. The total is on the Story page. The goat is welcome to look at the book from a distance."},{0,0}};
static const Dl dl12[]={
    {W_WISP,"The Hollow King told me about the houses that went dark in his old world. He keeps calling monsters because he knows they'll come."},
    {W_GRUUL,"We can leave him a seat and still ask him what he did. First help our villages grow to two hundred and fifty folk."},
    {W_DAVE,"I'll put out another plate. I'd offer him the couch, but the goat has tenure."},{0,0}};
static const Dl dl13[]={
    {W_HOLLOW,"Mira offered me a cup at the gate. The lamps are still burning in houses I gave orders to attack. I'll help keep the town standing."},
    {W_WISP,"Reach year sixteen and a hundred and fifty monsters slain. Both totals are on the Story page. Then we can put down the tools for supper."},{0,0}};
static const Dl dl14[]={
    {W_HOLLOW,"Thorgrim set a hammer aside for me. I'll help with the west hinge before supper. Tomorrow I'd like to take the watch with Gruul."},
    {W_MIRA,"He will. Leave your cup by the door. Someone on the next watch will want a drink."},
    {W_DAVE,"Great. One chair leans and the other is a goat. I am willing to negotiate with either."},{0,0}};
static const Dl*const dlgTab[15]={dl0,dl1,dl2,dl3,dl4,dl5,dl6,dl7,dl8,dl9,dl10,dl11,dl12,dl13,dl14};
static const char* const chapTxt[CHAP_N]={
    "THE GROUND IS READY","THE FIRST VILLAGE GROWS","EIGHT ROOFS ARE UP",
    "BOTH PEOPLES HAVE A HOME","THE FORD IS CLEAR","THE WATCH HAS SUPPLIES",
    "THE GREAT HALL OPENS","FIFTEEN MONSTERS RECORDED","EIGHT YEARS TOGETHER",
    "THE WATCH HOLDS THE GATE","FORTY HOMES SHELTER THE FOLK",
    "EIGHTY MONSTERS RECORDED","THE VILLAGES ARE THRIVING","THE WATCH CAN REST"};
static void chapterDone(void){   // a chapter is finished (by play, or by the debug menu): items open, the jingle plays, the aural sculptures open at chapter 3
    siprintf(info,"%.32s",chapTxt[chapter]); chapter++; dlgPend=chapter<CHAP_N?chapter:14;
    if(chapter>=3&&!auralOpen){ auralOpen=1; siprintf(info,"AURAL SCULPTURES UNLOCKED"); }
    if(chapter>=CHAP_N){ storyDone=1; tierOpen=14; musReq(GM_GODHOOD); fxReq(FX_CHAPTER_DONE); }
    else { if(chapter+1>tierOpen) tierOpen=chapter+1>14?14:chapter+1; musReq(GM_ASCEND); fxReq(FX_CHAPTER_DONE); } }
static int slainCount(void){ int n=0; for(int m=0;m<MON_N;m++){ int sI=8+(ARCH_A+m/16-8)*16+m%16; n+=(slainB[sI>>3]>>(sI&7))&1; } return n; }
static void checkFull(void){ if(!fullDone&&storyDone&&seenN>=MON_N&&slainCount()>=MON_N){ fullDone=1; siprintf(info,"100%%! THE REMIXES ARE YOURS"); musReq(GM_GODHOOD); fxReq(FX_UNLOCK); } }
static void unlockTiers(void){   // every 256 ticks: is the chapter you are on done?
#ifdef GG_NATIVE_STORY_BUSY
    if(GG_NATIVE_STORY_BUSY())return;
#endif
    checkFull();
#ifndef PCTEST
    // Keep a queued dialogue intact at high sim speeds. The story bot runs
    // without a dialogue UI, so this handoff only applies to the actual game.
    if(dlgPend>=0||storyGoalT>0) return;
#endif
    if(!storyOn||storyDone||chapter>=CHAP_N) return;
    const Chap*C=&chap[chapter]; if(chapVal(C->kind)<C->target) return;
    chapterDone(); }
// ---------------------------------------------------------------------------------------------- powers (the table is the catalogue)
typedef struct { const char*nm; u8 kind, arg; } Pw;
enum { PK_INSPECT, PK_PAINT, PK_SPAWN, PK_RAIN, PK_FIRE, PK_BOLT, PK_METEOR, PK_KILL, PK_HEAL, PK_FEED, PK_SETTLE, PK_WAR, PK_PEACE, PK_BLESS, PK_GIFT, PK_MUTATE, PK_QUAKE, PK_FLOOD, PK_BLOOM, PK_VIBE, PK_TORNADO, PK_ACID, PK_HOLE, PK_NUKE, PK_PLAGUE, PK_CURSE, PK_AIR, PK_INSPIRE, PK_WHISPER, PK_DISCORD, PK_UNITY, PK_SPITE, PK_FRIEND, PK_DUSTW, PK_DUSTR, PK_DUSTG, PK_LOOT, PK_CORRODE, PK_FIREEL, PK_GOO, PK_CRABBOMB, PK_CRABZILLA, PK_UPLIFT, PK_MONOLITH, PK_STATUS, PK_ALIEN, PK_DRAGON, PK_MOON, PK_BIOMASS, PK_ICE, PK_GARDEN, PK_BOLT_SMALL, PK_BOLT_MEDIUM, PK_PRINT_COPY, PK_PRINT_FOREST, PK_PRINT_WATER, PK_PRINT_MOUNTAIN, PK_PRINT_BIOME, PK_PRINT_CLEAR, PK_POSSESS, PK_HOME, PK_BORDER, PK_SPECIAL, PK_FIELD, PK_BUILD, PK_KIND_N };
static const Pw pw[]={ {"INSPECT",PK_INSPECT,0},{"WATER",PK_PAINT,T_WATER},{"DEEP SEA",PK_PAINT,T_DEEP},{"SAND",PK_PAINT,T_SAND},{"GRASS",PK_PAINT,T_GRASS},{"FOREST",PK_PAINT,T_FOREST},{"JUNGLE",PK_PAINT,T_JUNGLE},
    {"HILLS",PK_PAINT,T_HILL},{"MOUNTAIN",PK_PAINT,T_MOUNT},{"SNOW",PK_PAINT,T_SNOW},{"DESERT",PK_PAINT,T_DESERT},{"SWAMP",PK_PAINT,T_SWAMP},{"LAVA",PK_PAINT,T_LAVA},{"CORRUPT",PK_PAINT,101},{"INFERNAL",PK_PAINT,102},{"ENCHANT",PK_PAINT,103},{"GREENHOUSE",PK_AIR,0},{"OXYGEN",PK_AIR,1},{"DUST CLOUD",PK_AIR,2},
    {"SPAWN",PK_SPAWN,0},{"RAIN",PK_RAIN,0},{"FIRE",PK_FIRE,0},{"LIGHTNING",PK_BOLT,0},{"METEOR",PK_METEOR,0},{"SMITE",PK_KILL,0},{"HEAL",PK_HEAL,0},{"FEED",PK_FEED,0},{"SETTLE",PK_SETTLE,0},{"WAR",PK_WAR,0},{"PEACE",PK_PEACE,0},{"BLESS",PK_BLESS,0},{"GIFT",PK_GIFT,0},{"MUTATE",PK_MUTATE,0},{"EARTHQUAKE",PK_QUAKE,0},{"TSUNAMI",PK_FLOOD,0},{"BLOOM",PK_BLOOM,0},{"GOOD VIBES",PK_VIBE,0},
    {"TORNADO",PK_TORNADO,0},{"ACID RAIN",PK_ACID,0},{"BLACK HOLE",PK_HOLE,0},{"ATOMIC BOMB",PK_NUKE,0},{"PLAGUE",PK_PLAGUE,0},{"CURSE",PK_CURSE,0},
    {"INSPIRE",PK_INSPIRE,0},{"WHISPER",PK_WHISPER,0},{"DISCORD",PK_DISCORD,0},{"UNITY",PK_UNITY,0},{"SPITE",PK_SPITE,0},{"FRIENDSHIP",PK_FRIEND,0},{"WHITE DUST",PK_DUSTW,0},{"RED DUST",PK_DUSTR,0},{"GOLD DUST",PK_DUSTG,0},{"LOOT RAIN",PK_LOOT,0},{"CORROSIVE",PK_CORRODE,0},{"FIRE ELEM.",PK_FIREEL,0},{"GREY GOO",PK_GOO,0},{"CRAB BOMB",PK_CRABBOMB,0},{"CRABZILLA",PK_CRABZILLA,0},{"UPLIFT",PK_UPLIFT,0},{"MONOLITH",PK_MONOLITH,0},{"STATUS",PK_STATUS,0},{"ALIEN INVASION",PK_ALIEN,0},{"FAR DRAGON",PK_DRAGON,0},{"MOON HORRORS",PK_MOON,0},{"BIOMASS",PK_BIOMASS,0},{"DREAM ICE",PK_ICE,0},{"GARDEN SURPRISE",PK_GARDEN,0},{"SMALL LIGHTNING",PK_BOLT_SMALL,0},{"MEDIUM LIGHTNING",PK_BOLT_MEDIUM,0},{"PRINTER COPY",PK_PRINT_COPY,0},{"PRINTER FOREST",PK_PRINT_FOREST,0},{"PRINTER WATER",PK_PRINT_WATER,0},{"PRINTER MOUNTAIN",PK_PRINT_MOUNTAIN,0},{"PRINTER BIOME",PK_PRINT_BIOME,0},{"PRINTER CLEAR",PK_PRINT_CLEAR,0},{"POSSESS",PK_POSSESS,0},{"SEND HOME",PK_HOME,0},{"BORDER BRUSH",PK_BORDER,0},{"SPECIAL EVENT",PK_SPECIAL,0},{"FIELD TOOLS",PK_FIELD,0},{"BUILD",PK_BUILD,0} };   // Pack B: political powers   // the Calamity pack (calamity.h)
#define PW_N ((int)(sizeof(pw)/sizeof(pw[0])))
_Static_assert(PW_N==GG_BUILD_FAVORITE+1&&PW_N<=sizeof favPow,"Construction favorite save index");
enum { FIELD_SURVEY, FIELD_RATIONS, FIELD_CLINIC, FIELD_RESTORE, FIELD_N };
static int fieldMode, fieldRadius=2;
#ifndef PCTEST
static const char*const fieldName[FIELD_N]={"SURVEY","RATIONS","CLINIC","RESTORE"};
static void openFieldLedger(int v);
#endif
static int inspected=-1, trSel=1, statusSel=0, specialEventSel=0, fireElementalNext=0;
static u8 printerTile=T_GRASS, printerBiome; static int printerCopied, possessed=-1;
static const u8 statusMask[6]={ST_INVINCIBLE,ST_ASH_FEVER,ST_COUGH,ST_MOTIVATED,ST_HANDSOME,ST_MINDLESS};
static const char* const statusName[6]={"INVINCIBLE","ASH FEVER","COUGH","MOTIVATED","HANDSOME MIGRANT","MINDLESS"};
static int inspT, inspKind, inspIdx;   // the inspect card: what was looked at (1 creature, 2 village) and for how long it stays up
static int polPend=-1, polKind, polTick;   // the two-tap powers: the first kingdom tapped, which power, when (it is forgotten after about 6 seconds)
static int polKing(int x,int y){ int v=vilAny(x,y,4); return v>=0&&vilK[v]?vilK[v]-1:-1; }
static int polSecond(int kind,int k,int*first){      // 1 when this tap is the second of a pair (first = the kingdom tapped before); 0 after the first tap or a bad second tap
    if(polPend<0||polKind!=kind||tick-polTick>360||!kd[polPend].on){ polPend=k; polKind=kind; polTick=tick; siprintf(info,"NOW TAP THE TARGET"); return 0; }
    if(k==polPend){ siprintf(info,"TAP ANOTHER KINGDOM"); return 0; } *first=polPend; polPend=-1; return 1; }
static void polApply(int kind,int x,int y){      // the six political powers and the three dusts, on a tap
    achX[1]|=1u<<(kind-PK_INSPIRE);
    int k=polKing(x,y), v=vilAny(x,y,4); char a[24], b[24], s[72];
    if(kind==PK_INSPIRE){ if(v<0||!vilK[v]){ siprintf(info,"TAP A CITY OF A KINGDOM"); return; } k=vilK[v]-1; if(kd[k].cap==v+1){ siprintf(info,"THE CAPITAL STAYS"); return; }
        int cul=kd[k].cul, rlg=kd[k].rlg, lang=kd[k].lang; kdName(k,a); vilK[v]=0; vilIndep[v]=0; int n2=kdFound(v); if(n2<0){ vilK[v]=(u8)(k+1); siprintf(info,"NO ROOM FOR A KINGDOM"); return; }
        kd[n2].cul=(u8)cul; kd[n2].rlg=(u8)rlg; kd[n2].lang=(u8)lang; kd[n2].race=vil[v].race; kd[n2].truce=20; vilName(v,b); siprintf(s,"%.20s IS INSPIRED TO LEAVE %.20s",b,a); logEv(s); siprintf(info,"A NEW KINGDOM"); KD_DIRTY(); return; }
    if(k<0){ siprintf(info,"TAP A CITY OF A KINGDOM"); return; } kdName(k,a);
    if(kind==PK_WHISPER||kind==PK_UNITY){ int f; if(!polSecond(kind,k,&f)) return; kdName(f,a); kdName(k,b);
        if(kind==PK_WHISPER){ if(!gx.war||!LAWON(LW_DIPLOMACY)){ siprintf(info,"WAR IS FORBIDDEN"); return; } if(kd[f].war>>k&1){ siprintf(info,"ALREADY AT WAR"); return; } kd[f].plot=(u8)(k+1); kd[f].plotT=0; kd[f].pcool=0; whisT[f]=(u8)(k+1); siprintf(s,"%.20s HEARS WHISPERS OF WAR ON %.20s",a,b); logEv(s); siprintf(info,"WAR IS WHISPERED"); }
        else { kdWar(f,k,0); kd[f].allies|=(u16)(1<<k); kd[k].allies|=(u16)(1<<f); kd[f].rel[k]=kd[k].rel[f]=60; if(kd[f].plot==k+1) kd[f].plot=0; if(kd[k].plot==f+1) kd[k].plot=0; kd[f].truce=kd[k].truce=10; warMatrix(); siprintf(s,"%.20s AND %.20s BECOME ALLIES",a,b); logEv(s); siprintf(info,"UNITY"); } KD_DIRTY(); return; }
    if(kind==PK_DISCORD){ for(int o=0;o<KN_N;o++){ kd[k].allies&=(u16)~(1<<o); kd[o].allies&=(u16)~(1<<k); } disT[k]=8; siprintf(s,"DISCORD SPLITS %.22s FROM ITS ALLIES",a); logEv(s); siprintf(info,"DISCORD"); KD_DIRTY(); return; }
    if(kind==PK_SPITE){ spiT[k]=12; siprintf(s,"EVERYONE BEGINS TO DESPISE %.20s",a); logEv(s); siprintf(info,"SPITE"); KD_DIRTY(); return; }
    if(kind==PK_FRIEND){ for(int o=0;o<KN_N;o++) if(kd[k].war>>o&1){ kdWar(k,o,0); kd[k].truce=kd[o].truce=10; } for(int u=0;u<VIL_N;u++) if(vil[u].on&&vilK[u]==k+1) vil[u].foe=0; for(int u=0;u<VIL_N;u++) if(vil[u].on&&vil[u].foe&&vilK[vil[u].foe-1]==k+1) vil[u].foe=0; warMatrix(); siprintf(s,"%.22s MAKES PEACE",a); logEv(s); siprintf(info,"FRIENDSHIP"); KD_DIRTY(); return; }
    if(kind==PK_DUSTW){ kd[k].lang=(u8)((kd[k].lang+1+rnd()%4)%5); kd[k].nlang=(u8)((kd[k].nlang+1+rnd()%4)%5); kdName(k,b); siprintf(s,"%.20s FORGETS ITS LANGUAGE: NOW %.20s",a,b); logEv(s); siprintf(info,"LANGUAGE FORGOTTEN"); KD_DIRTY(); return; }
    if(kind==PK_DUSTG){ if(v<0||!vilK[v]){ return; } vilName(v,b); vilK[v]=0; vilIndep[v]=1; siprintf(s,"%.20s FORGETS %.20s AND STANDS ALONE",b,a); logEv(s); siprintf(info,"CITY IS INDEPENDENT"); KD_DIRTY(); return; } }
static int sendCreatureHome(int ci){
    if(ci<0||ci>=nCr||!cr[ci].alive) return 0;
    Cr*c=&cr[ci]; int race=sp[c->sp].a; if(race>=8||arch[race].diet!=4) return 0;
    int v=vilNear(race,c->x,c->y,WW>WH?WW:WH); if(v<0) return 0;
    for(int r=0;r<=4;r++) for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){
        if((dx<0?-dx:dx)+(dy<0?-dy:dy)!=r) continue;
        int x=vil[v].x+dx,y=vil[v].y+dy;
        if(x<0||y<0||x>=WW||y>=WH||!walkable(&sp[c->sp],x,y)||occ[y][x]) continue;
        moveTo(c,ci,x,y); return 1;
    }
    return 0;
}
static int paintCityBorder(int x,int y){
    int v=vilAny(x,y,8); if(v<0) return 0;
    int rad=6+vil[v].lvl*3, found=0;
    int x0=vil[v].x-rad-2, x1=vil[v].x+rad+2, y0=vil[v].y-rad-2, y1=vil[v].y+rad+2;
    if(x0<0)x0=0; if(y0<0)y0=0; if(x1>=WW)x1=WW-1; if(y1>=WH)y1=WH-1;
    for(int yy=y0;yy<=y1&&!found;yy++) for(int xx=x0;xx<=x1;xx++) if(bld[yy][xx]==B_BORDER&&bldv[yy][xx]==v+1){ found=1; break; }
    if(found){ for(int yy=y0;yy<=y1;yy++) for(int xx=x0;xx<=x1;xx++) if(bld[yy][xx]==B_BORDER&&bldv[yy][xx]==v+1){ bld[yy][xx]=B_NONE; bldv[yy][xx]=0; if(xx>=winX-1&&xx<winX+34&&yy>=winY-1&&yy<winY+26) putCell(xx,yy); } return -1; }
    int made=0;
    for(int yy=vil[v].y-rad;yy<=vil[v].y+rad;yy++) for(int xx=vil[v].x-rad;xx<=vil[v].x+rad;xx++){
        if(xx<1||yy<1||xx>=WW-1||yy>=WH-1||bld[yy][xx]||occ[yy][xx]) continue;
        int dx=xx-vil[v].x,dy=yy-vil[v].y; if(dx*dx+dy*dy>rad*rad) continue;
        int edge=0; for(int d=0;d<4;d++){ int ax=xx+dxs[d]-vil[v].x, ay=yy+dys[d]-vil[v].y; if(ax*ax+ay*ay>rad*rad) edge=1; }
        if(!edge) continue;
        bld[yy][xx]=B_BORDER; bldv[yy][xx]=(u8)(v+1); if(xx>=winX-1&&xx<winX+34&&yy>=winY-1&&yy<winY+26) putCell(xx,yy); made++;
    }
    return made;
}
static int creatureFamily(const Cr*c){
    const Sp*s=&sp[c->sp]; if(s->a>=8) return s->fam;
    int v=vilNear(s->a,c->x,c->y,WW>WH?WW:WH); if(v<0||!vilK[v]||!kd[vilK[v]-1].on) return s->fam;
    static const u8 base[8]={12,5,11,4,3,1,9,2};
    return (base[s->a]+3*(kd[vilK[v]-1].cul%5))&15;
}
static int fieldCityAt(int x,int y){
    int v=bldv[y][x]-1;
    if(bld[y][x]&&v>=0&&v<VIL_N&&vil[v].on) return v;
    return vilAny(x,y,5);
}
static int fieldRations(int v){
    Vil*V=&vil[v];
    int helped=0;
    for(int dy=-6;dy<=6&&helped<8&&V->stock;dy++) for(int dx=-6;dx<=6&&helped<8&&V->stock;dx++){
        int x=V->x+dx, y=V->y+dy;
        if((unsigned)x>=WW||(unsigned)y>=WH||dx*dx+dy*dy>36||!occ[y][x]) continue;
        Cr*c=&cr[occ[y][x]-1];
        if(!c->alive||sp[c->sp].a!=V->race||c->food>=65||vilNear(V->race,x,y,6)!=v) continue;
        c->food+=35; V->stock--; helped++;
    }
    return helped;
}
static int fieldRestore(int x,int y){
    int ground=world[y][x], bio=bioAt(x,y), ruin=bld[y][x]==B_RUIN;
    if(bld[y][x]&&!ruin) return 0;
    if(ground!=T_DIRT&&bio!=1&&bio!=2&&!ruin) return 0;
    if(ruin){ bld[y][x]=B_NONE; bldv[y][x]=0; }
    if(bio==1||bio==2) bioSet(x,y,0);
    if(ground==T_DIRT) setCell(x,y,T_GRASS);
    else if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y);
    return 1;
}
static int fieldClinic(int x,int y){
    int i=occ[y][x]-1;
    if(i<0||!cr[i].alive) return 0;
    Cr*c=&cr[i];
    u8 before=c->sick;
    if(DISEASE(c)==1) SET_DISEASE(c,2);
    else if(DISEASE(c)==3) SET_DISEASE(c,0);
    c->sick&=(u8)~(ST_ASH_FEVER|ST_COUGH);
    return before!=c->sick;
}
static void fieldApply(int x,int y){
    if((unsigned)x>=WW||(unsigned)y>=WH) return;
    if(fieldMode==FIELD_SURVEY||fieldMode==FIELD_RATIONS){
        int v=fieldCityAt(x,y);
        if(v<0){ siprintf(info,"TAP A CITY OR ITS HOMES"); return; }
        if(fieldMode==FIELD_SURVEY){
#ifndef PCTEST
            openFieldLedger(v);
#endif
            return;
        }
        int n=fieldRations(v);
        if(!n){ siprintf(info,vil[v].stock?"NO HUNGRY FOLK NEARBY":"CITY STORE IS EMPTY"); return; }
        siprintf(info,"RATIONS FOR %d FOLK",n);
    } else {
        int r=fieldRadius, n=0;
        for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){
            int xx=x+dx, yy=y+dy;
            if((unsigned)xx>=WW||(unsigned)yy>=WH||dx*dx+dy*dy>r*r) continue;
            n+=fieldMode==FIELD_CLINIC?fieldClinic(xx,yy):fieldRestore(xx,yy);
        }
        if(!n){ siprintf(info,fieldMode==FIELD_CLINIC?"NO AILMENTS IN REACH":"NO DAMAGED LAND IN REACH"); return; }
        siprintf(info,fieldMode==FIELD_CLINIC?"TREATED %d CREATURES":"RESTORED %d TILES",n);
    }
    stUse++;
    fxReq(FX_BLESS);
}
static const u8 constructionTypes[]={B_HOUSE,B_MANOR,B_WORKSHOP,B_INN,B_MARKET,
    B_BARRACKS,B_WALL,B_GATE_NS,B_GATE_EW,B_KEEP,
    GG_MARKET_SQUARE,GG_GUILD_YARD,GG_GARDEN_COURT,B_PAVING,B_WELL};
enum { GG_CONSTRUCTION_N=sizeof constructionTypes/sizeof *constructionTypes };
static int constructionMode,constructionTurn=-1;
static const char *constructionName(int mode){
    int kind=constructionTypes[mode];
    if(kind==B_KEEP)return "CASTLE COMPOUND";
    if(kind==B_HOUSE)return "FAMILY HOUSE";
    if(kind==B_MANOR)return "MANOR ESTATE";
    if(kind==GG_MARKET_SQUARE)return "MARKET SQUARE";
    if(kind==GG_GUILD_YARD)return "GUILD YARD";
    if(kind==GG_GARDEN_COURT)return "GARDEN COURT";
    return ggBuildingName(kind);
}
static int constructionTurns(int kind){
    return kind>=B_N?4:(kind==B_HOUSE||kind==B_KEEP?2:0);
}
static const char *constructionFacing(int kind){
    static const char *const entrance[]={"SOUTH","WEST","NORTH","EAST"};
    if(!constructionTurns(kind))return "";
    if(constructionTurn<0)return "AUTO";
    if(kind>=B_N)return entrance[constructionTurn];
    return constructionTurn==0?"E-W":"N-S";
}
static int constructionPlan(GgBuildPlan *plan,int x,int y){
    int kind=constructionTypes[constructionMode],turn=constructionTurn;
    if(kind>=B_N){
        int count=turn<0?4:1;
        for(int i=0;i<count;i++){
            ggCompoundPlan(plan,kind,turn<0?i:turn);
            if(ggPlanCheck(plan,x,y)==GG_SITE_CLEAR)return GG_SITE_CLEAR;
        }
        if(turn<0)ggCompoundPlan(plan,kind,0);
    }else if(kind==B_KEEP){
        ggFortPlan(plan,turn==1?B_GATE_NS:B_GATE_EW);
        int status=ggPlanCheck(plan,x,y);
        if(status!=GG_SITE_CLEAR&&turn<0){
            ggFortPlan(plan,B_GATE_NS);
            if(ggPlanCheck(plan,x,y)==GG_SITE_CLEAR)return GG_SITE_CLEAR;
            ggFortPlan(plan,B_GATE_EW);
        }
    }else{
        plan->count=0;
        if(kind==B_MANOR)kind=B_ESTATE;
        else if(kind==B_HOUSE){
            kind=turn<0?ggResidencePlan(x,y,B_HOUSE):turn==0?B_HOUSE_EW:B_HOUSE_NS;
            if(!kind)kind=B_HOUSE_EW;
        }
        ggPlanAdd(plan,0,0,kind,0);
        if(kind==B_HOUSE_EW||kind==B_ESTATE)ggPlanAdd(plan,1,0,B_WING_E,0);
        if(kind==B_HOUSE_NS||kind==B_ESTATE)ggPlanAdd(plan,0,1,B_WING_S,0);
        if(kind==B_ESTATE)ggPlanAdd(plan,1,1,B_WING_SE,0);
    }
    return ggPlanCheck(plan,x,y);
}

static const char *constructionIssue(int status){
    static const char *const issue[]={"LIFT TO BUILD","MOVE AWAY FROM MAP EDGE",
        "NEEDS CLEAR GRASS, DIRT OR FOREST","A PLOT IS OCCUPIED",
        "ENTRANCE NEEDS A WALKABLE LANDING","KEEP THE CASTLE APPROACH CLEAR"};
    return issue[status];
}

static void constructionFinish(const GgBuildPlan *plan,int x,int y){
    int status=ggPlanCheck(plan,x,y);
    if(status!=GG_SITE_CLEAR){ siprintf(info,"%s",constructionIssue(status));return; }
    int radius=constructionTypes[constructionMode]>=B_N?10:12;
    ggPlanCommit(plan,vilAny(x,y,radius),x,y);
    siprintf(info,"BUILT %s",constructionName(constructionMode));
    stUse++;
    fxReq(FX_BLESS);
}
static void constructionApply(int x,int y){
    GgBuildPlan plan;
    constructionPlan(&plan,x,y);
    constructionFinish(&plan,x,y);
}
static void usePower(int tx,int ty,int fresh){
    if(pw[power].kind==PK_BUILD){ if(fresh)constructionApply(tx,ty); return; }
    if(pw[power].kind==PK_FIELD){ if(fresh) fieldApply(tx,ty); return; }
    KD_DIRTY();
    const Pw*P=&pw[power]; int r=brush;
    if(fresh){ stUse++; if(P->kind==PK_GIFT) stGift++; static const u8 fxOf[65]={FX_INSPECT,FX_PAINT,FX_SPAWN,FX_RAIN,FX_FIRE,FX_LIGHTNING,FX_METEOR,FX_SMITE,FX_HEAL,FX_FEED,FX_SETTLE,FX_WAR,FX_PEACE,FX_BLESS,FX_GIFT,FX_BIRTH,FX_METEOR,FX_RAIN,FX_BIRTH,FX_BLESS,FX_LIGHTNING,FX_RAIN,FX_SMITE,FX_METEOR,FX_MONSTER_SEEN,FX_SMITE,FX_RAIN,FX_BLESS,FX_WAR,FX_WAR,FX_PEACE,FX_SMITE,FX_PEACE,FX_RAIN,FX_RAIN,FX_RAIN,FX_GIFT,FX_RAIN,FX_SMITE,FX_SMITE,FX_BIRTH,FX_SMITE,FX_BLESS,FX_BLESS,FX_BLESS,FX_MONSTER_SEEN,FX_SMITE,FX_MONSTER_SEEN,FX_SMITE,FX_RAIN,FX_BLESS,0,0,0,0,0,0,0,0,0,0,0,FX_MONSTER_SEEN}; fxReq(fxOf[P->kind]); } if(P->kind==PK_GIFT&&(it[itSel].kind==IK_WEAPON||it[itSel].kind==IK_ARMOR||it[itSel].kind==IK_RELIC||it[itSel].kind==IK_GEM)) r=1;
    for(int dy=-r;dy<=r;dy++) for(int dx=-r;dx<=r;dx++){ if(dx*dx+dy*dy>r*r+r/2) continue; int x=tx+dx, y=ty+dy; if(x<0||y<0||x>=WW||y>=WH) continue;
        switch(P->kind){
        case PK_PAINT: if(P->arg>=100){ int t=world[y][x]; if(t>T_WATER&&t!=T_LAVA&&t!=T_MOUNT&&(fresh||(rnd()&1))){ bioSet(x,y,P->arg-100); if(x>=winX-1&&x<winX+34&&y>=winY-1&&y<winY+26) putCell(x,y); } break; }
            if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } bioSet(x,y,0); setCell(x,y,P->arg); if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(!walkable(&sp[c->sp],x,y)&&(rnd()&3)==0) killCr(occ[y][x]-1); } break;
        case PK_SPAWN: if(fresh&&dx==0&&dy==0){ int count=0;
            for(int k=0;k<(brush?brush:1);k++) count+=addCr(spSel,x+(k?(int)(rnd()%3)-1:0),y+(k?(int)(rnd()%3)-1:0))>=0;
            if(!count) siprintf(info,"TRY OPEN GROUND FOR THIS SPECIES"); } break;
        case PK_RAIN: if(world[y][x]==T_DIRT||world[y][x]==T_SAND||world[y][x]==T_DESERT) { if((rnd()&3)==0) setCell(x,y,T_GRASS); } else if(world[y][x]==T_LAVA) setCell(x,y,T_MOUNT); else if(world[y][x]==T_GRASS&&(rnd()&15)==0) setCell(x,y,T_FOREST); break;
        case PK_FIRE: burnAt(x,y,90); if(world[y][x]==T_FOREST||world[y][x]==T_GRASS||world[y][x]==T_JUNGLE){ if((rnd()&1)==0) setCell(x,y,T_DIRT); } if(occ[y][x]&&(rnd()&1)) { Cr*c=&cr[occ[y][x]-1]; if(c->hp>10) c->hp-=10; else killCr(occ[y][x]-1); } break;
        case PK_BOLT: case PK_BOLT_SMALL: case PK_BOLT_MEDIUM: { int rad=P->kind==PK_BOLT_SMALL?0:P->kind==PK_BOLT_MEDIUM?1:0; if(dx*dx+dy*dy<=rad*rad&&fresh){ burnAt(x,y,255); if(occ[y][x]){ int ci=occ[y][x]-1; if(P->kind==PK_BOLT_SMALL) damageCreature(ci,18,-1); else killCr(ci); } if(world[y][x]==T_FOREST||world[y][x]==T_JUNGLE) setCell(x,y,T_DIRT); } } break;
        case PK_PRINT_COPY: if(fresh&&dx==0&&dy==0){ if(!printerCopied){ printerTile=world[y][x]; printerBiome=(u8)bioAt(x,y); printerCopied=1; siprintf(info,"PRINTER COPIED TILE"); } else { for(int py=-r;py<=r;py++) for(int px=-r;px<=r;px++){ int xx=x+px, yy=y+py; if(xx<0||yy<0||xx>=WW||yy>=WH||px*px+py*py>r*r+r/2) continue; if(printerTile!=T_WATER&&printerTile!=T_DEEP&&printerTile!=T_LAVA&&bld[yy][xx]){ bld[yy][xx]=0; bldv[yy][xx]=0; } setCell(xx,yy,printerTile); bioSet(xx,yy,printerBiome); } siprintf(info,"PRINTER COPIED AREA"); } } break;
        case PK_PRINT_FOREST: if(world[y][x]==T_GRASS||world[y][x]==T_DIRT||world[y][x]==T_SAND) setCell(x,y,T_FOREST); break;
        case PK_PRINT_WATER: if(world[y][x]!=T_MOUNT&&world[y][x]!=T_LAVA) setCell(x,y,T_WATER); break;
        case PK_PRINT_MOUNTAIN: if(world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_LAVA) setCell(x,y,T_MOUNT); break;
        case PK_PRINT_BIOME: if(fresh&&dx==0&&dy==0){ u8 b=(u8)bioAt(x,y); for(int py=-r;py<=r;py++) for(int px=-r;px<=r;px++){ int xx=x+px, yy=y+py; if(xx>=0&&yy>=0&&xx<WW&&yy<WH&&px*px+py*py<=r*r+r/2&&world[yy][xx]>T_WATER&&world[yy][xx]!=T_LAVA) bioSet(xx,yy,b); } } break;
        case PK_PRINT_CLEAR: if(world[y][x]!=T_WATER&&world[y][x]!=T_DEEP&&world[y][x]!=T_LAVA){ bioSet(x,y,0); if(bld[y][x]){ bld[y][x]=0; bldv[y][x]=0; } } break;
        case PK_HOME: if(fresh&&dx==0&&dy==0&&occ[y][x]){ int ci=occ[y][x]-1; siprintf(info,sendCreatureHome(ci)?"SENT HOME":"NO HOME CITY OR ROOM"); } break;
        case PK_BORDER: if(fresh&&dx==0&&dy==0){ int n=paintCityBorder(x,y); siprintf(info,n>0?"CITY BORDER PAINTED":n<0?"CITY BORDER CLEARED":"NO CITY HERE"); } break;
        case PK_POSSESS: if(fresh&&dx==0&&dy==0){ int ci=occ[y][x]?occ[y][x]-1:-1; if(ci>=0&&ci<nCr&&cr[ci].alive){ possessed=possessed==ci?-1:ci; siprintf(info,possessed<0?"RELEASED":"POSSESSING CREATURE"); } else { possessed=-1; siprintf(info,"TAP A CREATURE TO POSSESS"); } } break;
        case PK_AIR: if(fresh&&dx==0&&dy==0){ if(P->arg==0){ atCO2+=8*16; atFire+=4; siprintf(info,"CO2 UP: %d",ATV(atCO2)); } else if(P->arg==1){ atO2+=4*16; atCO2-=5*16; siprintf(info,"OXYGEN UP: %d%%",ATV(atO2)); } else { atmoDust(10); siprintf(info,"DUST: %d",ATV(atDust)); } if(atCO2<0) atCO2=0; if(atCO2>150*16) atCO2=150*16; if(atO2>60*16) atO2=60*16; } break;
        case PK_METEOR: if(fresh){ if(dx==0&&dy==0){ atmoDust(6); atFire+=6; } int rr=dx*dx+dy*dy; burnAt(x,y,255); if(rr<=2) setCell(x,y,T_LAVA); else if(rr<=r*r) setCell(x,y,(rnd()&1)?T_DIRT:T_HILL); if(occ[y][x]) killCr(occ[y][x]-1); } break;
        case PK_KILL: if(occ[y][x]) killCr(occ[y][x]-1); break;
        case PK_HEAL: if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; c->hp=(u8)maxHp(c); if(DISEASE(c)==1) SET_DISEASE(c,2); else if(DISEASE(c)==3) SET_DISEASE(c,0); } break;      // heal ends the plague (immune after) and the zombie infection (not immune)
        case PK_FEED: if(occ[y][x]) cr[occ[y][x]-1].food=120; else if(world[y][x]==T_DIRT) setCell(x,y,T_GRASS); break;
        case PK_SETTLE: if(fresh&&dx==0&&dy==0){ int race=spSel<8?sp[spSel].a:0; if(vilFound(race,x,y)<0) siprintf(info,"%s",vilWhyNot(x,y)); else { for(int k=0;k<4;k++) addCr(spSel<8?spSel:0,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2); siprintf(info,"VILLAGE FOUNDED. LET IT GROW"); } } break;
        case PK_INSPIRE: case PK_WHISPER: case PK_DISCORD: case PK_UNITY: case PK_SPITE: case PK_FRIEND: case PK_DUSTW: case PK_DUSTG: if(fresh&&dx==0&&dy==0) polApply(P->kind,x,y); break;
        case PK_MONOLITH: if(fresh&&dx==0&&dy==0){ if(world[y][x]==T_WATER||world[y][x]==T_DEEP||world[y][x]==T_LAVA||bld[y][x]) siprintf(info,"NO ROOM FOR A MONOLITH"); else { bld[y][x]=B_STATUE; bldv[y][x]=255; monoScan(); siprintf(info,"A MONOLITH RISES"); logEv("A MONOLITH RISES"); if(winX>=0) putCell(x,y); } } break;
        case PK_FIREEL: if(fresh&&dx==0&&dy==0){ int i=hzAdd(HZ_FIRE,x,y); if(i<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { hz[i].pad=(u8)(fireElementalNext++%5); static const char*const fn[5]={"EMBER","MAGMA","INFERNO","PLASMA","SOLAR"}; siprintf(info,"A %s ELEMENTAL WAKES",fn[hz[i].pad]); } } break;
        case PK_SPECIAL: if(fresh&&dx==0&&dy==0){ static const u8 sk[6]={HZ_NAVY,HZ_PLANES,HZ_HYPERCUBE,HZ_GHOSTCIV,HZ_CYBERCORE,HZ_PUMPKIN}; static const char*const sn[6]={"NAVY SEALS","PLANES","HYPERCUBE","GHOST CIVILIZATION","CYBERCORE","SUPER PUMPKIN"}; if(hzAdd(sk[specialEventSel],x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"%s DEPLOYED",sn[specialEventSel]); specialEventSel=(specialEventSel+1)%6; } } break;
        case PK_GOO: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_GOO,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else siprintf(info,"GREY GOO SPREADS"); } break;
        case PK_CRABBOMB: if(fresh&&dx==0&&dy==0) crabBomb(x,y); break;
        case PK_CRABZILLA: if(fresh&&dx==0&&dy==0) crabStomp(x,y); break;
        case PK_UPLIFT: if(occ[y][x]&&(fresh||(rnd()&7)==0)){ if(uplift(occ[y][x]-1)&&fresh&&dx==0&&dy==0) siprintf(info,"IT THINKS"); } break;
        case PK_LOOT: if(occ[y][x]&&(fresh||(rnd()&7)==0)){ Cr*c=&cr[occ[y][x]-1]; if(arch[sp[c->sp].a].diet==4&&(rnd()%3)==0){ int i=itDrop((int)rnd(),0); if(i>=0){ if(it[i].kind==IK_WEAPON) c->wep=(u16)(i+1); else c->arm=(u16)(i+1); } } } break;
        case PK_CORRODE: if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(c->wep||c->arm){ if(c->arm){ int hp=c->hp-itPow(c->arm-1); c->hp=(u8)(hp<1?1:hp); } c->wep=0; c->arm=0; } } break;
        case PK_DUSTR: if(occ[y][x]){ u32 id=crId[occ[y][x]-1]; int q=nbFind(id); if(q>=0){ nb[q].par=0; nb[q].mate=0; } } if(fresh&&dx==0&&dy==0){ int vv=vilAny(x,y,4); if(vv>=0&&vilK[vv]){ int k=vilK[vv]-1; kd[k].clan=(u8)(rnd()%8); KD_DIRTY(); } siprintf(info,"FAMILY FORGOTTEN"); } break;
        case PK_WAR: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,4); if(v>=0){ int best=-1,bd=2000; for(int u=0;u<VIL_N;u++){ if(!vil[u].on||u==v||vil[u].race==vil[v].race) continue; int ddx=vil[u].x-vil[v].x, ddy=vil[u].y-vil[v].y, d=ddx*ddx+ddy*ddy; if(d<bd){ bd=d; best=u; } } if(best>=0){ vil[v].foe=(u8)(best+1); siprintf(info,"WAR! %s ATTACKS %s",arch[vil[v].race].nm,arch[vil[best].race].nm); if(vilK[v]&&vilK[best]&&vilK[v]!=vilK[best]) kdDeclareX(vilK[v]-1,vilK[best]-1,1); } } } break;
        case PK_PEACE: if(fresh&&dx==0&&dy==0){ for(int v=0;v<VIL_N;v++) vil[v].foe=0; for(int k=0;k<KN_N;k++){ kd[k].war=0; kd[k].weary=0; } warMatrix(); siprintf(info,"PEACE"); } break;
        case PK_BLESS: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,4); if(v>=0&&LAWON(LW_MAGIC)&&vilK[v]){ Kd*K=&kd[vilK[v]-1]; if(K->gold<20){ siprintf(info,"THE TREASURY CANNOT PAY"); break; } K->gold=(u16)(K->gold-20); } if(v>=0){ vil[v].stock=(u16)(vil[v].stock+150>400?400:vil[v].stock+150); } } break;
        case PK_GIFT: { const Item*I=&it[itSel]; Cr*c=occ[y][x]?&cr[occ[y][x]-1]:0; int cen=(dx==0&&dy==0);
            switch(I->kind){
            case IK_WEAPON: if(c&&cen&&fresh){ c->wep=(u16)(itSel+1); } break;
            case IK_ARMOR: if(c&&cen&&fresh){ int old=c->arm?itPow(c->arm-1):0; c->arm=(u16)(itSel+1); int hp=c->hp+itPow(itSel)*2-old*2; c->hp=(u8)(hp>250?250:hp<1?1:hp); } break;
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
        case PK_MUTATE: if(fresh&&dx==0&&dy==0&&occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; if(trSel==0){ if(HAS(c,TR_GRIN)) joyInvalidate(); c->tr=c->tr2=0; siprintf(info,"TRAITS CLEARED"); } else { giveTrait(c,trSel,1); char nm[48]; crName(c,nm); siprintf(info,"%.46s",nm); } c->hp=(u8)maxHp(c); } break;
        case PK_STATUS: if(fresh&&dx==0&&dy==0&&occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; u8 mask=statusMask[statusSel]; c->sick^=mask; siprintf(info,"%s: %s",statusName[statusSel],HAS_STATUS(c,mask)?"ON":"OFF"); } break;
        case PK_ALIEN: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_ALIEN,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"ALIEN INVASION BEGINS"); flashT=5; } } break;
        case PK_DRAGON: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_DRAGON,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"A DRAGON COMES FROM FAR LANDS"); flashT=5; } } break;
        case PK_MOON: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_MOON,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"MOON HORRORS DESCEND"); flashT=5; } } break;
        case PK_BIOMASS: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_BIOMASS,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"A BIOMASS MONSTROSITY AWAKENS"); flashT=4; } } break;
        case PK_ICE: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_ICE,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"ICE FALLS FROM A DREAM"); flashT=4; } } break;
        case PK_GARDEN: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_GARDEN,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"A GARDEN SURPRISE BLOOMS"); fxReq(FX_BLESS); } } break;
        case PK_TORNADO: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_TORNADO,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"A TORNADO TOUCHES DOWN"); flashT=3; } } break;
        case PK_ACID: acidAt(x,y); break;
        case PK_HOLE: if(fresh&&dx==0&&dy==0){ if(hzAdd(HZ_HOLE,x,y)<0) siprintf(info,"TOO MANY CALAMITIES AT ONCE"); else { siprintf(info,"A BLACK HOLE OPENS"); flashT=4; } } break;
        case PK_NUKE: if(fresh&&dx==0&&dy==0) nukeAt(x,y,5+2*brush>18?18:5+2*brush); break;
        case PK_PLAGUE: if(occ[y][x]){ if(gx.lPlague==1){ if(fresh&&dx==0&&dy==0) siprintf(info,"THE LAW FORBIDS PLAGUE"); } else if(infect(occ[y][x]-1)&&fresh&&dx==0&&dy==0) siprintf(info,"THE PLAGUE SPREADS"); } break;
        case PK_CURSE: if(fresh&&dx==0&&dy==0){ int v=vilAny(x,y,5); if(v>=0){ vil[v].stock=(u16)(vil[v].stock>150?vil[v].stock-150:0); siprintf(info,"THE VILLAGE STORE ROTS"); } } if(occ[y][x]){ Cr*c=&cr[occ[y][x]-1]; c->hp=(u8)(c->hp/2+1); c->food=(u8)(c->food/2); } break;
        case PK_INSPECT:
            if(dx!=0||dy!=0)break;
            if(occ[y][x]){
                inspected=occ[y][x]-1;inspKind=1;inspIdx=inspected;inspT=420;
            }else if(bldv[y][x]&&bldv[y][x]<=VIL_N){
                int v=bldv[y][x]-1;
                char nm[24],kn[24]="";
                vilName(v,nm);
                if(vilK[v])kdName(vilK[v]-1,kn);
                siprintf(info,"%s %s POP%d [%s]",arch[vil[v].race].nm,nm,vil[v].pop,kn);
                inspected=-2;inspKind=2;inspIdx=v;inspT=420;
            }else if(bld[y][x]){
                int rootX,rootY;
                int parent=ggResidenceResolveWing(x,y,bld[y][x],&rootX,&rootY);
                siprintf(info,"%s",ggBuildingName(parent?parent:bld[y][x]));
                inspected=-2;inspKind=0;inspT=0;
            }else if(inspected==-2)inspected=-1;
            break;
        } }
    if(P->kind==PK_INSPECT&&inspected!=-2){ if(inspected>=0&&inspected<nCr&&cr[inspected].alive){ Cr*c=&cr[inspected]; char nm[48]; crName(c,nm); siprintf(info,"%.30s HP%d/%d",nm,c->hp,maxHp(c)); } else siprintf(info,"%s",bioNm[world[ty<WH?ty:0][tx<WW?tx:0]]); }
}
static int planetSize=128;
static void (*loadHook)(const char*,int);
#define LOADING(m,p) do{ if(loadHook) loadHook(m,p); }while(0)
#include "march_landmarks.h"
static void newWorld(void){
    mapPinMask=0;
    ggFortificationsPresent=0;
    dlgPend=-1; joyInvalidate(); vilIndexInvalidate();
#ifndef PCTEST
    storyGoalT=0;
#endif
    KD_DIRTY(); LOADING("THE VOID",4); trSeen=trSeenHi=stMut=0; gxDefaults(); gW=gH=planetSize; nCr=0; idReset(); memset(hz,0,sizeof hz); memset(occ,0,sizeof occ); cellClear(); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(vil,0,sizeof vil); memset(kd,0,sizeof kd); memset(kx,0,sizeof kx); memset(hall,0,sizeof hall); memset(whisT,0,sizeof whisT); memset(disT,0,sizeof disT); memset(spiT,0,sizeof spiT); memset(vilIndep,0,sizeof vilIndep); memset(vilK,0,sizeof vilK); memset(vilL,0,sizeof vilL); memset(boat,0,sizeof boat); memset(rz,0,sizeof rz); rzN=0; cursor=0; vilCur=0; memset(vilBf,0,sizeof vilBf); memset(vilSick,0,sizeof vilSick); memset(favVil,0,sizeof favVil); memset(favKd,0,sizeof favKd); jrN=0; stWars=stCapt=stAllies=stSnack=0; nVilOn=0; worldSeed=seedFix?seedFix:rnd(); genWorld(worldSeed); LOADING("SHAPING THE LAND",35); winX=-99; refillWindow();
    LOADING("SEEDING LIFE",50);
    // the first life: a few of each of the first animals on their own ground
    int nAn=260*WW*WH/16384*cLife/2; { int cap=900*(WW*WH>65536?WW*WH/65536:1); if(nAn>cap) nAn=cap; }
    for(int k=0;k<nAn;k++){ int a=8+(int)(rnd()%(ARCH_A-8)); int s=8+(a-8)*16+randomFamily(a); int x=rnd()%WW, y=rnd()%WH; if(arch[a].diet==3&&k%2) continue; if(arch[a].diet==6&&(rnd()&7)) continue; addCr(s,x,y); }
    LOADING("RAISING THE FIRST VILLAGES",80);
    for(int r=0;r<8&&cLife>0;r++){
        if(r>=5&&(rnd()&1)) continue; // the restless peoples sometimes stay away
        for(int k=0;k<80;k++){
            int x,y;
            if(cShape==GW_MARCH&&k==0) marchSite(r,&x,&y);
            else { x=rnd()%WW; y=rnd()%WH; }
            if(vilFound(r,x,y)<0) continue;
            for(int q=0;q<5;q++) addCr(r,x+(int)(rnd()%5)-2,y+(int)(rnd()%5)-2);
            break;
        }
    }
    if(cShape==GW_MARCH){ marchLandmarks();winX=-99;refillWindow(); }
#ifdef GG_3DS_QUEST_WORLD_RESET
    GG_3DS_QUEST_WORLD_RESET(NULL);
#endif
    info[0]=0; LOADING("READY",100); }
// ---------------------------------------------------------------------------------------------- saving: the whole world, the villages and the story, to one file (the SD card on the DS)
static int shape3;
#define SAVE_MAGIC 0x31534747u
typedef struct { u32 magic; u16 ver, gw, szCr, szVil; u32 tick, year, rs; u32 stKill, stGift, stUse, trSeen, stMut, seed; u16 nCr, seenN, camX, camY; u8 chapter, tierOpen, storyOn, storyDone, monOn, shape3, nVilOn, planet, vcur, aural, debug, full, diff, weather; u16 weatherT; u8 pad_[3]; u8 seen[(SP_N+7)/8], slain[(SP_N+7)/8]; } SaveHdr;   // (version 10: planet = how many villages the file holds, pad_[1] = the creature count above 65535; pad_[0] is the map edge)
#ifdef GG_BIGMAP
typedef struct { u16 sp, wep, arm; u8 x, y; s8 ox, oy; u8 hp, food, age, t, dir, alive, tr, tr2, rk, sick; } CrOld;      // the 8-bit-coordinate layouts of the DS and of older saves: read and widened
typedef struct { u8 on, race, x, y, lvl, houses, farms, towers, foe, dead, nm1, nm2; u16 pop, stock; } VilOld;
#endif
typedef struct { u8 kind, x, y; s8 dx, dy; u8 r, t, pad; u16 life, age; } HzOld;       // the GGH1 trailer before the coordinates went 16-bit
static const u16 szList[]={128,192,256
#ifdef GG_BIGMAP
 ,384,512,768
#ifndef GG_DSI16
 ,1024
#endif
#endif
};
#define SZ_N ((int)(sizeof szList/sizeof szList[0]))
static int sizeStep(int cur,int d){ int i=0; for(int k=0;k<SZ_N;k++) if(szList[k]==cur) i=k; i=(i+d+SZ_N)%SZ_N; return szList[i]; }
static int validSize(int g){ for(int k=0;k<SZ_N;k++) if(szList[k]==g) return 1; return 0; }
#ifndef NDSHW
typedef FILE SV;
static SV*svOpen(const char*p,const char*m){ return fopen(p,m); } static size_t svWrite(const void*b,size_t s,size_t n,SV*f){ return fwrite(b,s,n,f); } static size_t svRead(void*b,size_t s,size_t n,SV*f){ return fread(b,s,n,f); } static int svClose(SV*f){ return fclose(f); }
static int svRewind(SV*f){ return fseek(f,0,SEEK_SET)==0; }
#endif
static int rdVec(void*dst,int dstN,int srcN,int esz,SV*f){ u8 tmp[32]; int ok=1; for(int i=0;i<srcN&&ok;i++){ ok=svRead(tmp,esz,1,f)==1; if(ok&&i<dstN) memcpy((u8*)dst+i*esz,tmp,esz); } return ok; }      // read srcN records of esz bytes, keep the first dstN (a save may hold more or fewer villages than this build)
#include "ggidsv.h"
#include "save_preflight.h"
#ifndef PCTEST
static void zSet(int nz);
static void zRestoreSaved(int zoom,int x,int y);
static void tbInvalidate(void);
#endif
static int saveGameRaw(const char*path){
    compact(); wgBuild(); cursor=0;     // (10d: and the war groups are counted again, as a load will count them) the dead are dropped first, so a loaded game goes on exactly as this one would
    SV*f=svOpen(path,"wb"); if(!f) return 0; SaveHdr h; memset(&h,0,sizeof h); h.vcur=(u8)vilCur;
    h.magic=SAVE_MAGIC; h.pad_[0]=(u8)cEdge; h.pad_[1]=(u8)(nCr>>16); h.pad_[2]=(u8)zl; h.ver=14; h.weather=(u8)weather; h.weatherT=(u16)weatherT; h.trSeen=trSeen; h.stMut=stMut; h.seed=worldSeed; h.diff=(u8)diff; h.full=(u8)fullDone; h.aural=(u8)auralOpen; h.debug=(u8)debugOn; h.gw=(u16)gW; h.szCr=sizeof(Cr); h.szVil=sizeof(Vil); h.tick=(u32)tick; h.year=(u32)year; h.rs=rs; h.stKill=stKill; h.stGift=stGift; h.stUse=stUse; h.nCr=(u16)(nCr&0xFFFF); h.seenN=(u16)seenN; h.camX=(u16)camX; h.camY=(u16)camY;
    h.chapter=(u8)chapter; h.tierOpen=(u8)tierOpen; h.storyOn=(u8)storyOn; h.storyDone=(u8)storyDone; h.monOn=(u8)monOn; h.shape3=(u8)shape3; h.nVilOn=(u8)nVilOn; h.planet=(u8)VIL_N; memcpy(h.seen,seenB,sizeof seenB); memcpy(h.slain,slainB,sizeof slainB);
    int ok=svWrite(&h,sizeof h,1,f)==1;
    for(int y=0;y<gH&&ok;y++){ ok&=svWrite(world[y],1,gW,f)==(size_t)gW; ok&=svWrite(bld[y],1,gW,f)==(size_t)gW; ok&=svWrite(bldv[y],1,gW,f)==(size_t)gW; }
    if(ok&&nCr) ok&=svWrite(cr,sizeof(Cr),nCr,f)==(size_t)nCr; if(ok) ok&=svWrite(vil,sizeof(Vil),VIL_N,f)==(size_t)VIL_N;
    if(ok) ok&=svWrite(kd,sizeof(Kd),KN_N,f)==(size_t)KN_N; if(ok) ok&=svWrite(vilL,1,VIL_N,f)==(size_t)VIL_N; if(ok) ok&=svWrite(vilK,1,VIL_N,f)==(size_t)VIL_N; if(ok){ int n=jrN; ok&=svWrite(&n,sizeof n,1,f)==1; ok&=svWrite(jr,34,24,f)==24; ok&=svWrite(jrYear,2,24,f)==24; u32 sx[8]={achBits,stWars,stCapt,stAllies,stSnack,achX[0],achX[1],achX[2]}; ok&=svWrite(sx,4,8,f)==8; ok&=svWrite(favVil,1,FAV_N,f)==(size_t)FAV_N; ok&=svWrite(favKd,1,8,f)==8; ok&=svWrite(favItem,1,600,f)==600; ok&=svWrite(favPow,1,40,f)==40; }
    if(ok){ u32 mg=0x31584747u, sz=sizeof gx, cs[2]={stBorn,stDied}; ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&sz,4,1,f)==1; ok&=svWrite(&gx,sizeof gx,1,f)==1; ok&=svWrite(cs,4,2,f)==2; }   // GGX1 trailer: old builds never read it
    if(ok){ u32 mg=0x31484747u, sz=sizeof(Hz); ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&sz,4,1,f)==1; ok&=svWrite(hz,sizeof(Hz),HZ_N,f)==(size_t)HZ_N; }   // GGH1 trailer (calamity.h): the tornadoes and black holes in flight
    if(ok){ u32 mg=0x31424747u; ok&=svWrite(&mg,4,1,f)==1; for(int y=0;y<gH&&ok;y++) ok&=svWrite(biome2+y*(MAXD/4),1,gW/4,f)==(size_t)(gW/4); if(ok){ int at[8]={atO2,atCO2,atDust,atTemp,atFire,atFlag,favPow[GG_BUILD_FAVORITE],0}; ok&=svWrite(at,sizeof at,1,f)==1; if(ok) ok&=svWrite(favPow+40,1,24,f)==24; } }      // (the stars on powers 40 to 63 ride behind the atmosphere: the reserved atmosphere word at[6] stores the construction favorite)
    if(ok){ u32 mg=0x31544747u, sz=sizeof(Boat), nb=0; for(int b=0;b<8;b++) nb+=boat[b].on?1u:0u; ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&sz,4,1,f)==1; ok&=svWrite(&nb,4,1,f)==1; for(int b=0;b<8&&ok;b++) if(boat[b].on) ok&=svWrite(&boat[b],sizeof(Boat),1,f)==1; }      // GGT1 trailer: the boats at sea (only those; none, and it is twelve bytes). Older builds never read it
    if(ok){ u32 mg=0x314C4747u, n=LW_N; ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&n,4,1,f)==1; ok&=svWrite(lw,1,LW_N,f)==LW_N; }      // GGL1 trailer: the World Laws that differ from their defaults (an old build never reads it; an old save has none)
    if(ok) ok&=idWrite(f);      // GGN1 trailer (ggid.h): permanent creature ids, their flag words, the stars on creatures and the notable table. Old builds never read it; an old save has none (ids are then handed out in list order)
    if(ok){ u32 mg=0x314B4747u, n=KN_N, sz=sizeof(KdX); ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&n,4,1,f)==1; ok&=svWrite(&sz,4,1,f)==1; ok&=svWrite(kx,sizeof(KdX),KN_N,f)==KN_N; }      // GGK1 trailer: the policies, newer culture traits and renown of each kingdom
    if(ok){ u32 mg=0x31524747u, n=sizeof hall; ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&n,4,1,f)==1; ok&=svWrite(hall,1,sizeof hall,f)==sizeof hall; }      // GGR1 trailer: the hall of rulers (the last four of each kingdom)
    if(ok){ u32 mg=0x31554747u; ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&trSeenHi,4,1,f)==1; }      // GGU1: trait discoveries above bit 31; old builds ignore this optional tail
    if(ok){ u32 mg=0x31464747u; ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(favPow+64,1,16,f)==16; }      // GGF1: extra starred powers 64..79; version 13 adds this optional trailer
    if(ok){ u32 mg=0x32524747u, n=(u32)(rzN<0?0:rzN); ok&=svWrite(&mg,4,1,f)==1; ok&=svWrite(&n,4,1,f)==1; ok&=svWrite(rz,sizeof rz,1,f)==1; }      // GGR2: four temporary captain-rout zones; required by v14 for save/load determinism
    // (an old build never reads it; an old save has none, so every policy is normal)
    { int cl_=svClose(f); return ok&&cl_==0; } }   // (a pack that does not fit the cartridge chip fails the save instead of reporting SAVED)
static int loadGameRaw(const char*path){ KD_DIRTY(); joyInvalidate(); vilIndexInvalidate();
    SV*f=svOpen(path,"rb"); if(!f) return 0; SaveHdr h;
    if(svRead(&h,sizeof h,1,f)!=1||!saveHeaderValid(&h)||!savePreflight(f,&h)||!svRewind(f)){
        svClose(f); return 0;
    }
    // Re-read from the same stream only after validation, before touching
    // any world state. The hardware backend replays its existing buffer.
    SaveHdr checked=h;
    if(svRead(&h,sizeof h,1,f)!=1||memcmp(&h,&checked,sizeof h)){ svClose(f); return 0; }
    int ok=1;
    int nC=(int)h.nCr|(h.ver>=10?(int)h.pad_[1]<<16:0); int nvs=h.ver>=10&&h.planet?h.planet:VIL_OLDN; int legacy=0;
#ifdef GG_BIGMAP
    legacy=h.szCr==sizeof(CrOld)&&h.szVil==sizeof(VilOld);      // a DS save (or one from before the big planets) on a big build: widened as it is read
#endif
    trSeenHi=0; memset(favPow,0,sizeof favPow);
    memset(rz,0,sizeof rz); rzN=0; gxDefaults(); gW=gH=h.gw; planetSize=gW; memset(world,0,sizeof world); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(occ,0,sizeof occ);
    for(int y=0;y<gH&&ok;y++){ ok&=svRead(world[y],1,gW,f)==(size_t)gW; ok&=svRead(bld[y],1,gW,f)==(size_t)gW; ok&=svRead(bldv[y],1,gW,f)==(size_t)gW; }
    nCr=nC;
#ifdef GG_BIGMAP
    if(legacy){ static CrOld tmp[512]; for(int i=0;i<nCr&&ok;){ int n=nCr-i<512?nCr-i:512; ok&=svRead(tmp,sizeof(CrOld),n,f)==(size_t)n; for(int k=0;k<n&&ok;k++){ const CrOld*o=&tmp[k]; Cr*c=&cr[i+k]; memset(c,0,sizeof *c); c->sp=o->sp; c->wep=o->wep; c->arm=o->arm; c->x=o->x; c->y=o->y; c->ox=o->ox; c->oy=o->oy; c->hp=o->hp; c->food=o->food; c->age=o->age; c->t=o->t; c->dir=o->dir; c->alive=o->alive; c->tr=o->tr; c->tr2=o->tr2; c->rk=o->rk; c->sick=o->sick; } i+=n; } }
    else
#endif
    if(ok&&nCr) ok&=svRead(cr,sizeof(Cr),nCr,f)==(size_t)nCr;
    memset(vil,0,sizeof vil); { int nv=h.ver>=8?nvs:VIL_OLD;
#ifdef GG_BIGMAP
        if(legacy){ for(int k=0;k<nv&&ok;k++){ VilOld o; ok&=svRead(&o,sizeof o,1,f)==1; if(!ok||k>=VIL_N) continue; Vil*V=&vil[k]; V->on=o.on; V->race=o.race; V->x=o.x; V->y=o.y; V->lvl=o.lvl; V->houses=o.houses; V->farms=o.farms; V->towers=o.towers; V->foe=o.foe; V->dead=o.dead; V->nm1=o.nm1; V->nm2=o.nm2; V->pop=o.pop; V->stock=o.stock; } }
        else
#endif
        if(ok) ok&=rdVec(vil,VIL_N,nv,sizeof(Vil),f); }
    memset(kd,0,sizeof kd); memset(kx,0,sizeof kx); memset(hall,0,sizeof hall); memset(whisT,0,sizeof whisT); memset(disT,0,sizeof disT); memset(spiT,0,sizeof spiT); memset(vilIndep,0,sizeof vilIndep); memset(vilK,0,sizeof vilK); memset(vilL,0,sizeof vilL); jrN=0; achBits=stWars=stCapt=stAllies=stSnack=0; memset(achX,0,sizeof achX);
    if(ok&&h.ver>=9){ ok&=svRead(kd,sizeof(Kd),KN_N,f)==(size_t)KN_N; if(ok) ok&=rdVec(vilL,VIL_N,nvs,1,f); if(ok) ok&=rdVec(vilK,VIL_N,nvs,1,f); if(ok){ int n=0; ok&=svRead(&n,sizeof n,1,f)==1; ok&=svRead(jr,34,24,f)==24; ok&=svRead(jrYear,2,24,f)==24; jrN=n<0?0:n>24?24:n; u32 sx[8]={0}; if(svRead(sx,4,8,f)==8){ achBits=sx[0]; stWars=sx[1]; stCapt=sx[2]; stAllies=sx[3]; stSnack=sx[4]; achX[0]=sx[5]&0xFFFFu; achX[1]=sx[6]&0xFFFFu; achX[2]=sx[7]&0xFFu; u8 t1[FAV_N], t2[8], t3[600], t4[40]; memset(t1,0,sizeof t1); if(rdVec(t1,FAV_N,nvs>48?nvs:48,1,f)&&svRead(t2,1,8,f)==8&&svRead(t3,1,600,f)==600&&svRead(t4,1,40,f)==40){ memcpy(favVil,t1,sizeof favVil); memcpy(favKd,t2,8); memcpy(favItem,t3,600); memcpy(favPow,t4,40); } } } }
    if(ok&&h.ver>=9){ u32 mg=0, sz=0; if(svRead(&mg,4,1,f)==1&&mg==0x31584747u&&svRead(&sz,4,1,f)==1&&sz>=(u32)GX_HEAD&&sz<=(u32)GX_HEAD+2*512){ u8 tb[GX_HEAD+2*512]; if(svRead(tb,1,sz,f)==sz){ memcpy(&gx,tb,GX_HEAD); int n=(int)(sz-GX_HEAD)/2; for(int k=0;k<n&&k<GX_V;k++){ gx.vGrow[k]=tb[GX_HEAD+k]; gx.vDef[k]=tb[GX_HEAD+n+k]; } u32 cs[2]; if(svRead(cs,4,2,f)==2){ stBorn=cs[0]; stDied=cs[1]; } } else gxDefaults(); } }   // (the per-city tables are copied by their own count: a save from a build with another village count still loads)
    memset(hz,0,sizeof hz); if(ok&&h.ver>=9){ u32 mg=0, sz=0; if(svRead(&mg,4,1,f)==1&&mg==0x31484747u&&svRead(&sz,4,1,f)==1&&(sz==sizeof(Hz)||sz==sizeof(HzOld))){ if(sz==sizeof(Hz)){ Hz t2[HZ_N]; if(svRead(t2,sizeof(Hz),HZ_N,f)==(size_t)HZ_N) memcpy(hz,t2,sizeof hz); }
            else { HzOld t3[HZ_N]; if(svRead(t3,sizeof(HzOld),HZ_N,f)==(size_t)HZ_N) for(int i=0;i<HZ_N;i++){ Hz*q=&hz[i]; const HzOld*o=&t3[i]; q->kind=o->kind; q->x=o->x; q->y=o->y; q->dx=o->dx; q->dy=o->dy; q->r=o->r; q->t=o->t; q->life=o->life; q->age=o->age; } }
            for(int i=0;i<HZ_N;i++) if(hz[i].kind>HZ_PUMPKIN||hz[i].x>=gW||hz[i].y>=gH) memset(&hz[i],0,sizeof hz[i]); } }   // GGH1 trailer: an old save has none
    int bioRead=0; bioClear(); atmoReset(); if(ok&&h.ver>=9){ u32 mg=0; if(svRead(&mg,4,1,f)==1&&mg==0x31424747u){ int got=1; for(int y=0;y<gH&&got;y++) got=svRead(biome2+y*(MAXD/4),1,gW/4,f)==(size_t)(gW/4); if(!got) memset(biome2,0,sizeof biome2); else { int at[8]; if(svRead(at,sizeof at,1,f)==1){ atO2=at[0]; atCO2=at[1]; atDust=at[2]; atTemp=at[3]; atFire=at[4]; atFlag=at[5]; favPow[GG_BUILD_FAVORITE]=(u8)(at[6]==1); u8 fp[24]; if(svRead(fp,1,24,f)==24){ memcpy(favPow+40,fp,24); bioRead=1; } } } } }   // GGB1 trailer: an old save has none
    memset(boat,0,sizeof boat); if(ok&&bioRead){ u32 mg=0, sz=0, nb=0; if(svRead(&mg,4,1,f)==1&&mg==0x31544747u&&svRead(&sz,4,1,f)==1&&sz==sizeof(Boat)&&svRead(&nb,4,1,f)==1&&nb<=8){ for(u32 b=0;b<nb;b++){ Boat t; if(svRead(&t,sizeof t,1,f)!=1) break; if(t.on&&t.race<8&&t.x>=0&&t.y>=0&&t.x<gW&&t.y<gH&&t.tx>=0&&t.ty>=0&&t.tx<gW&&t.ty<gH&&t.n<=128&&t.i<=t.n) boat[b]=t; } } }   // GGT1 trailer: the boats at sea (an old save has none)
    memset(lw,0,sizeof lw); int lwRead=0; if(ok&&bioRead){ u32 mg=0, n=0; if(svRead(&mg,4,1,f)==1&&mg==0x314C4747u&&svRead(&n,4,1,f)==1&&n<=sizeof lw){ u8 t[sizeof lw]; memset(t,0,sizeof t); if(svRead(t,1,n,f)==n){ for(u32 k=0;k<n&&k<LW_N;k++) lw[k]=t[k]&1; lwRead=1; } } }      // GGL1 trailer: an old save has none, so every law is at its default
    { int idOk=ok&&lwRead&&idRead(f); if(!idOk) idAssign(); memset(kx,0,sizeof kx); if(idOk){ u32 mg=0, n=0, sz=0; if(svRead(&mg,4,1,f)==1&&mg==0x314B4747u&&svRead(&n,4,1,f)==1&&n==KN_N&&svRead(&sz,4,1,f)==1&&sz==sizeof(KdX)){ KdX t[KN_N]; if(svRead(t,sizeof(KdX),KN_N,f)==KN_N){ for(int i=0;i<KN_N;i++){ if(t[i].tribute>2||t[i].tax>2) continue; kx[i]=t[i]; kx[i].ctr2&=0x3FFF; } u32 g2=0, n2=0; if(svRead(&g2,4,1,f)==1&&g2==0x31524747u&&svRead(&n2,4,1,f)==1&&n2==sizeof hall){ svRead(hall,1,sizeof hall,f); } } } } }      // GGK1 trailer (after GGN1): none in an old save; GGR1 (the hall of rulers) behind it      // GGN1 trailer: ids, flags, stars, notables (none in an old save: ids in list order, no records)
    if(ok&&h.ver>=12){ u32 mg=0; if(svRead(&mg,4,1,f)==1&&mg==0x31554747u&&svRead(&trSeenHi,4,1,f)==1){} else ok=0; }      // GGU1 is required by version 12
    if(ok&&h.ver>=13){ u32 mg=0; if(svRead(&mg,4,1,f)==1&&mg==0x31464747u&&svRead(favPow+64,1,16,f)==16){} else ok=0; }      // GGF1 stores starred powers 64..79; earlier saves default them to unstarred
    if(ok&&h.ver>=14){ u32 mg=0,n=0; if(svRead(&mg,4,1,f)==1&&mg==0x32524747u&&svRead(&n,4,1,f)==1&&n<=0x7FFFFFFFu&&svRead(rz,sizeof rz,1,f)==1) rzN=(int)n; else ok=0; }      // GGR2 preserves captain routs across a save; v13 and earlier start without temporary rout zones
    bioCount(); bioRescan();
    svClose(f);
    if(!ok){ return 0; }
    ggFortificationsPresent=0;
    for(int y=0;y<WH&&!ggFortificationsPresent;y++)for(int x=0;x<WW;x++)
        if(ggBuildingStopsPawn(bld[y][x])||ggBuildingIsGate(bld[y][x])){
            ggFortificationsPresent=1;break;
        }
    mapPinMask=0;
    tick=(int)h.tick; year=(int)h.year; rs=h.rs; stKill=h.stKill; stGift=h.stGift; stUse=h.stUse; seenN=h.seenN; camX=h.camX; camY=h.camY; chapter=h.chapter; tierOpen=h.tierOpen; storyOn=h.storyOn; storyDone=h.storyDone; monOn=h.monOn; shape3=h.shape3; nVilOn=h.nVilOn;
    for(int q=0;q<4;q++) if(rz[q].until>(int)h.tick&&(rz[q].a<0||rz[q].a>=8||rz[q].x<0||rz[q].y<0||rz[q].x>=gW||rz[q].y>=gH)){ memset(rz,0,sizeof rz); rzN=0; break; }
    memcpy(seenB,h.seen,sizeof seenB); memcpy(slainB,h.slain,sizeof slainB); nVilOn=0; memset(vilSick,0,sizeof vilSick); for(int v=0;v<VIL_N;v++){ nVilOn+=vil[v].on; if(vil[v].on) bldFlags(v); else vilBf[v]=0; }
    for(int i=0;i<nCr;i++) if(cr[i].alive&&cr[i].x<gW&&cr[i].y<gH) occ[cr[i].y][cr[i].x]=(occ_t)(i+1); cellRebuild(); vilCur=h.vcur; weather=h.weather; weatherT=h.weatherT; trSeen=h.trSeen; stMut=h.stMut; worldSeed=h.seed; cEdge=h.pad_[0]<3?h.pad_[0]:0; diff=h.diff; fullDone=h.full; auralOpen=h.aural; debugOn=h.debug; cursor=0; compact(); warMatrix(); wgBuild(); winX=-99;
    dlgPend=-1;
#ifndef PCTEST
    storyGoalT=0;
    tbInvalidate();
    // A legacy globe save still carries its last flat camera position.
    zRestoreSaved(h.pad_[2],h.camX,h.camY);
#endif
    return 1; }
#ifndef NDSHW
static int saveGame(const char*path){ char t[64], b[64]; siprintf(t,"%s.tmp",path); siprintf(b,"%s.bak",path);
    if(!saveGameRaw(t)){ remove(t); return 0; }
    remove(b); rename(path,b); if(rename(t,path)){ rename(b,path); remove(t); return 0; } return 1; }
static int loadGame(const char*path){ char b[64]; int loaded=loadGameRaw(path);
    if(!loaded){ siprintf(b,"%s.bak",path); loaded=loadGameRaw(b); }
#ifdef GG_3DS_QUEST_WORLD_RESET
    if(loaded) GG_3DS_QUEST_WORLD_RESET(path);
#endif
    return loaded; }
#else
static int saveGame(const char*p){ return saveGameRaw(p); } static int loadGame(const char*p){ return loadGameRaw(p); }
#endif
#ifdef GG_HAS_GLOBE
static int walkOn, walkX=64, walkY=64;     // walking the globe: the avatar's tile
static int view3d=0, rotX=20, rotY=0, zoom3=40, gClouds=1;       // the globe: rotY = longitude of the centre, rotX = latitude of the centre (degrees), zoom3 = how big
static void faceTile(int x,int y,int*lat,int*lon){ *lon=x*360/WW; int la=90-y*180/WH; *lat=la>85?85:la<-85?-85:la; }
#else
#define view3d 0
#define walkOn 0
#endif
#ifndef PCTEST
// ---------------------------------------------------------------------------------------------- screens
// ---- light: the day goes round and the water shimmers (a tint on the background palette) ----
static u16 basePal[256];
static void saveBasePal(void){ for(int i=0;i<GFX_PAL_N;i++) basePal[i]=BG_PALETTE[i]; }
static void applyLight(void){
    int p=gxTimeP((tick>>1)&2047), sun=p<1024?p:2047-p; int L=70+sun*186/1024, Lr=L*236/256, Lg=L*246/256, Lb=L+(256-L)/5+14; if(Lb>256) Lb=256; if(L>256) L=256;
    { int tw=300-(sun>300?sun-300:300-sun); if(tw<0) tw=0; Lr+=tw*Lr/900; Lg+=tw*Lg/3200; Lb-=tw*Lb/1100; if(sun<150){ Lr=Lr*(100-(150-sun)/12)/100; Lb+=(150-sun)/10; } if(Lr>256) Lr=256; if(Lg>256) Lg=256; if(Lb>256) Lb=256; if(Lb<0) Lb=0; }   // warm dawn and dusk, bluer deep night
    if(!gx.lAtmo&&atDust>16*8){ int d=ATV(atDust); if(d>60) d=60; Lr=Lr*(100-d/3)/100; Lg=Lg*(100-d/2)/100; Lb=Lb*(100-d*2/3)/100; }      // a dust veil: dimmer, browner
    if(weather==WX_RAIN||weather==WX_STORM){ Lr=Lr*78/100; Lg=Lg*80/100; Lb=Lb*90/100; } else if(weather==WX_SNOW){ Lr=Lr*95/100; Lg=Lg*97/100; }
    else if(weather==WX_ASH){ Lr=Lr*76/100; Lg=Lg*72/100; Lb=Lb*68/100; } else if(weather==WX_MAGIC){ Lr=Lr*92/100; Lg=Lg*96/100; Lb=Lb*82/100; } else if(weather==WX_RAGE){ Lr=Lr*86/100; Lg=Lg*72/100; Lb=Lb*70/100; } else if(weather==WX_HEAT){ Lr=Lr*105/100; Lg=Lg*96/100; Lb=Lb*82/100; if(Lr>256)Lr=256; }
    if(flashT>0){ Lr=Lg=Lb=256; }
    int sh=(tick>>4)&3;
    for(int i=0;i<GFX_PAL_N;i++){ if(UI_RESV(i)) continue; int src=i; if(i>=T_WATER*8+5&&i<T_WATER*8+8) src=T_WATER*8+5+((i-T_WATER*8-5+sh)%3); else if(i>=T_DEEP*8+4&&i<T_DEEP*8+7) src=T_DEEP*8+4+((i-T_DEEP*8-4+(sh>>1))%3);
        u16 c=basePal[src]; int r=(c&31)*Lr>>8, g=((c>>5)&31)*Lg>>8, b=((c>>10)&31)*Lb>>8; if(i>=T_LAVA*8&&i<T_LAVA*8+8){ int pu=((tick>>3)&15); pu=pu<8?pu:15-pu; r=r*(24+pu)/28; g=g*(20+pu*2)/28; } if(r>31) r=31; if(g>31) g=31; if(b>31) b=31; BG_PALETTE[i]=(u16)(r|(g<<5)|(b<<10)); }
    for(int i=PX_LAVAHOT;i<=PX_LAVAWHITE;i++){ int pu=((tick>>3)&15); pu=pu<8?pu:15-pu; u16 c=basePal[i]; int r=(c&31)*Lr>>8, g=((c>>5)&31)*Lg>>8, b=((c>>10)&31)*Lb>>8; g=g*(22+pu)/28; b=b*(22+pu)/28; BG_PALETTE[i]=(u16)(r|(g<<5)|(b<<10)); }
    gfxLight(Lr,Lg,Lb);
    BG_PALETTE[0]=0;
}
#include "ui.h"
#include "story_fmv.h"
#include "title3d.h"
static u8 gbuf[256*192] __attribute__((aligned(4)));
#ifdef GG_HAS_GLOBE
// CPU-rendered globe used by the 3DS in addition to its native 3D world.
#ifdef __3DS__
#define GR_MAX 120
#else
// Portable gameplay caps the disc at 88px. Do not reserve the unused
// 89..120 radius normal table in the 4 MiB DS memory layout.
#define GR_MAX 88
#endif
static u16 gnz[GR_MAX*GR_MAX+1]; static s16 gxn[2*GR_MAX+1]; static int gR=0;
static u16 gAtan[257]; static u16 gAsin[4097]; static u8 gCloud[64][64]; static int gInit;
static u16 gSavedPal[256];
// palette: 1 space, 2 3 stars, 8.. ground (13 kinds x 8 light levels), 112.. clouds, 120.. atmosphere, 126 127 128 lights, 129 sparkle
#define GP_GROUND 8
#define GP_CLOUD 112
#define GP_ATMO 120
#define GP_BIO 130      // the three biomes' ground, 8 light levels each (130..153)
#ifdef __3DS__
// Dedicated 3DS-only globe palette banks. They are restored when the map
// returns, so normal DS palettes, zoom art and save files are untouched.
#define GP3_AIR   160
#define GP3_SEA   168
#define GP3_SHORE 176
#define GP3_LIGHT 184
#define GP3_STAR  192
#define GP3_HIGH  200
// Set by the 3DS platform entry point. The larger/older 3DS globe keeps
// two-pixel sampling for audio headroom; New 3DS can resolve smaller discs
// at individual pixels without quadrupling the largest planet's workload.
static int gg3dsGlobeQuality;
static inline int gg3Height(int t){
    return t==T_MOUNT?7:t==T_HILL?4:t==T_FOREST||t==T_JUNGLE?2:t==T_SNOW?1:0;
}
#endif
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
static int globeR(void){
    int R=3500/zoom3;
#ifdef __3DS__
    return R>GR_MAX?GR_MAX:R<26?26:R;
#else
    // A 176px planet is fully readable on DS/DSi; the original 240px
    // disc required too many globeDir/biome/cloud samples on 67 MHz ARM9.
    return R>88?88:R<26?26:R;
#endif
}
static int globePick(int px,int py,int*tx,int*ty){
    int R=globeR(); globeTables(R); int x=px-128, y=96-py; int d2=x*x+y*y; if(d2>R*R) return 0; int cL,sL,cP,sP; globeAim(rotY,rotX,&cL,&sL,&cP,&sP);
    globeDir(gxn[x+R],gxn[y+R],gnz[d2],cL,sL,cP,sP,tx,ty); return 1; }
static void globePalette(void){
    BG_PALETTE[0]=0; BG_PALETTE[1]=RGB15(0,0,2); BG_PALETTE[2]=RGB15(12,12,16); BG_PALETTE[3]=RGB15(31,31,31);
    for(int t=0;t<T_N;t++) for(int l=0;l<8;l++){ int r,g,b; shadeRGB(t*8+(l<1?0:l),&r,&g,&b); int f=l==0?9:l==1?14:18+l*2; r=r*f/26; g=g*f/26; b=b*f/26; if(l==0){ b+=3; } if(r>31) r=31; if(g>31) g=31; if(b>31) b=31; BG_PALETTE[GP_GROUND+t*8+l]=RGB15(r,g,b); }
    for(int l=0;l<8;l++){ int v=6+l*3; BG_PALETTE[GP_CLOUD+l]=RGB15(v,v,v+(l>5?0:2)); }
    for(int b=0;b<3;b++) for(int l=0;l<8;l++){ int r,g,bl; shadeRGB((T_N+b)*8+(l<1?0:l),&r,&g,&bl); int f=l==0?9:l==1?14:18+l*2; r=r*f/26; g=g*f/26; bl=bl*f/26; if(r>31) r=31; if(g>31) g=31; if(bl>31) bl=31; BG_PALETTE[GP_BIO+b*8+l]=RGB15(r,g,bl); }
    for(int l=0;l<6;l++) BG_PALETTE[GP_ATMO+l]=RGB15(2+l,6+l*3,14+l*3);
    BG_PALETTE[126]=RGB15(31,28,10); BG_PALETTE[127]=RGB15(31,18,4); BG_PALETTE[128]=RGB15(31,6,2); BG_PALETTE[129]=RGB15(31,31,28);
#ifdef __3DS__
    // Expanded 3DS lighting palette: luminous cyan limb, depth-graded sea,
    // foam coast, warm cities, multicolour stars and high-altitude ice.
    // These use no extra image assets or framebuffers.
    for(int i=0;i<8;i++){
        BG_PALETTE[GP3_AIR+i]=RGB15(1+i/2,4+i*2,10+i*2);
        BG_PALETTE[GP3_SEA+i]=RGB15(0+i/3,2+i*2,8+i*3);
        BG_PALETTE[GP3_SHORE+i]=RGB15(8+i*2,11+i*2,7+i*2);
        BG_PALETTE[GP3_LIGHT+i]=RGB15(14+i*2,7+i*2,1+i*2);
        BG_PALETTE[GP3_STAR+i]=RGB15(11+i*2,13+i*2,20+i);
        BG_PALETTE[GP3_HIGH+i]=RGB15(9+i*3,12+i*2,16+i*2);
    }
#endif
}
static void globeDrawNow(void){
    int R=globeR(); globeTables(R); int cL,sL,cP,sP; globeAim(rotY,rotX,&cL,&sL,&cP,&sP);
    memset(gbuf,1,sizeof gbuf);
#ifdef __3DS__
    for(int k=0;k<108;k++){
        u32 h=hsh(k,1,9); int x=(int)(h%256), y=(int)((h>>8)%192);
        int shimmer=(int)((tick>>5)+(h>>24))&7;
        gbuf[y*256+x]=(u8)(GP3_STAR+(k%3==0?7:shimmer));
        if((k%11)==0 && x+1<256 && y+1<192){
            gbuf[y*256+x+1]=(u8)(GP3_STAR+2);
            gbuf[(y+1)*256+x]=(u8)(GP3_STAR+2);
        }
    }
#else
    for(int k=0;k<44;k++){ int x=(int)(hsh(k,1,9)%256), y=(int)(hsh(k,2,9)%192); gbuf[y*256+x]=(u8)(2+((k+(tick>>5))&1)); }
#endif
    int drift=tick>>6, R2=R*R, H=R+3, step=2;
    const int cloudThr=(weather==WX_RAIN||weather==WX_STORM||weather==WX_ASH||weather==WX_RAGE)?105:weather==WX_SNOW?125:weather==WX_MAGIC?95:weather==WX_HEAT?180:165;
#ifdef __3DS__
    H=R+7;
    if(gg3dsGlobeQuality&&R<=55) step=1; // full detail on smaller discs
#endif
    for(int sy=96-H;sy<96+H;sy+=step){ if(sy<0||sy>=192) continue; int y=96-sy;
        for(int sx=128-H;sx<128+H;sx+=step){ if(sx<0||sx>=256) continue; int x=sx-128, d2=x*x+y*y; u8 col;
            if(d2>R2){
#ifdef __3DS__
                if(d2>(R+7)*(R+7)) continue;
                int rim=((d2-R2)/(2*R)); if(rim>7) rim=7;
                int lit=(-x*5-y*5+R*3)>0;
                col=(u8)(GP3_AIR+(lit?(7-rim):(4-rim/2)));
#else
                if(d2<=(R+3)*(R+3)){ int lam=(-x*5-y*5+R*3)>0?5:2; col=(u8)(GP_ATMO+(lam>3?(d2<=(R+1)*(R+1)?5:3):1)); } else continue;
#endif
            }else{
                int nx=gxn[x+R], ny=gxn[y+R], nz=gnz[d2], tx,ty;
                globeDir(nx,ny,nz,cL,sL,cP,sP,&tx,&ty);
                int terr=world[ty][tx];
                int lam=(-nx*7+ny*7+nz*12)/26;
#ifdef __3DS__
                // Relief shading at mountain ridges and forest edges without
                // changing the true globe geometry or picking transform.
                int height=gg3Height(terr);
                if(height){
                    int east=world[ty][tx+1<WW?tx+1:0];
                    int south=world[ty+1<WH?ty+1:ty][tx];
                    lam+=(height-gg3Height(east))*480+(height-gg3Height(south))*320;
                }
#endif
                int lvl=lam<-1500?0:(lam+1500)*7/17000; if(lvl>7) lvl=7;
                if(lvl<0) lvl=0;
                int cv=0;
                if(gClouds) cv=gCloud[(ty*64/WH)&63][((tx*64/WW)+drift)&63];
#ifdef __3DS__
                if(gClouds && cv>cloudThr-17 && cv<=cloudThr && lvl>1) lvl--; // projected cloud shadow
#endif
                col=(u8)(GP_GROUND+terr*8+lvl);
                { int bb=bioAt(tx,ty); if(bb&&terr>T_WATER&&terr!=T_LAVA) col=(u8)(GP_BIO+(bb-1)*8+lvl); }
                if(bld[ty][tx]&&bld[ty][tx]!=B_RUIN){
#ifdef __3DS__
                    col=(u8)(lam<2400?GP3_LIGHT+3+((tick>>4)&1):GP3_LIGHT+7);
#else
                    col=lam<2000?(u8)(126+((tick>>4)&1)):(u8)(GP_GROUND+T_DIRT*8+7);
#endif
                }
                else if(terr==T_LAVA&&lam<3000) col=128;
                else if((terr==T_WATER||terr==T_DEEP)&&lam>11000&&nz>9000&&((nx+ny)&255)<120) col=129;
#ifdef __3DS__
                else if(terr==T_WATER||terr==T_DEEP) col=(u8)(GP3_SEA+lvl);
                else if(terr==T_SNOW && lam>2800) col=(u8)(GP3_HIGH+lvl);
                else if((terr==T_SAND||terr==T_DIRT||terr==T_GRASS) &&
                        (world[ty][tx+1<WW?tx+1:0]<=T_WATER ||
                         world[ty+1<WH?ty+1:ty][tx]<=T_WATER))
                    col=(u8)(GP3_SHORE+lvl);
#endif
                if(gClouds&&cv>cloudThr){
                    int cl=lvl+(cv-cloudThr)/12; if(cl>7) cl=7;
                    col=(u8)(GP_CLOUD+cl);
                }
#ifdef __3DS__
                // An independent high cirrus layer adds depth without a new
                // cloud texture: the same 64x64 noise is sampled offset in
                // world-space and animated at half the lower layer's speed.
                if(gClouds&&cv<=cloudThr && lam>1000){
                    int high=gCloud[(((ty*64/WH)+17)&63)][((((tx*64/WW)+37)+(drift>>1))&63)];
                    if(high>236 && ((tx+ty)&3)==0) col=(u8)(GP_CLOUD+7);
                }
                if(nz<4000){
                    int a=(4000-nz)/650; if(a>7) a=7;
                    if(lam>-2000||a>3) col=(u8)(GP3_AIR+a);
                }
#else
                if(nz<3800){ int a=(3800-nz)/700; if(a>5) a=5; if(lam>-2000||a>3) col=(u8)(GP_ATMO+a); }
#endif
            }
            gbuf[sy*256+sx]=col;
            if(step==2){
                if(sx+1<256) gbuf[sy*256+sx+1]=col;
                if(sy+1<192){
                    gbuf[(sy+1)*256+sx]=col;
                    if(sx+1<256) gbuf[(sy+1)*256+sx+1]=col;
                }
#ifdef __3DS__
                // Tiny one-pixel facets preserve relief in the larger 2x2
                // sampling mode; colour variations stay within one bank.
                if(d2<=R2 && col>=GP_GROUND && col<GP_GROUND+T_N*8
                   && ((x*3+y*7)&7)==0 && (col&7)>0 && sx+1<256)
                    gbuf[sy*256+sx+1]=col-1;
#endif
            }
        }
    }
    if(walkOn){ int c=((tick>>3)&1)?3:129; for(int d=4;d<=8;d++){ gbuf[96*256+128+d]=(u8)c; gbuf[96*256+128-d]=(u8)c; gbuf[(96+d)*256+128]=(u8)c; gbuf[(96-d)*256+128]=(u8)c; } }
}
// The globe is drawn again only when something it shows has changed: the view (turn, zoom, walking, clouds, weather), the blink of the stars, the city lights and the walker's cross, a power
// used (force), or every 16 ticks, so a planet that is not turning (paused, or a still view) costs nothing, and the picture goes to the screen only when it was redrawn (and now and then anyway).
static int gKeyOk=0, gDrawN=0;
#ifdef __3DS__
static int gKey[11];
#else
static int gKey[10];
#endif
static int globeDraw(int force){
#ifdef __3DS__
    // A quality-governor transition must immediately repaint the still globe
    // even when the rotation/zoom/time components are unchanged.
    int key[11]={rotX,rotY,zoom3,walkOn,gClouds,weather,(int)(tick>>6),(int)(tick>>5),(int)(tick>>4),walkOn?(int)((tick>>3)&1):0,gg3dsGlobeQuality};
#else
    int key[10]={rotX,rotY,zoom3,walkOn,gClouds,weather,(int)(tick>>6),(int)(tick>>5),(int)(tick>>4),walkOn?(int)((tick>>3)&1):0};
#endif
    if(!force&&gKeyOk&&!memcmp(key,gKey,sizeof key)) return 0;
    memcpy(gKey,key,sizeof key); gKeyOk=1; gDrawN++; globeDrawNow(); return 1; }
#ifdef __3DS__
static u32 gg3GlobeHash[32*24]; static int gg3GlobeHashValid;
#ifdef GG_GLOBE_TEST
static int gg3GlobeUploaded,gg3GlobeSkipped;
#endif
#endif
static void globeFlush(void){
    static u8 tl[64] __attribute__((aligned(4)));
    for(int ty=0;ty<24;ty++) for(int tx=0;tx<32;tx++){
#ifdef __3DS__
        u32 hash=2166136261u;
#endif
        for(int y=0;y<8;y++){
            const u8*r=gbuf+(ty*8+y)*256+tx*8;
            u32 a=((const u32*)r)[0], b=((const u32*)r)[1];
            ((u32*)tl)[y*2]=a; ((u32*)tl)[y*2+1]=b;
#ifdef __3DS__
            hash=(hash^a)*16777619u; hash=(hash^b)*16777619u;
#endif
        }
#ifdef __3DS__
        int k=ty*32+tx;
        if(gg3GlobeHashValid&&gg3GlobeHash[k]==hash){
#ifdef GG_GLOBE_TEST
            gg3GlobeSkipped++;
#endif
            continue;
        }
        gg3GlobeHash[k]=hash;
#ifdef GG_GLOBE_TEST
        gg3GlobeUploaded++;
#endif
#endif
        vramCopy((u8*)BG_TILE_RAM(1)+(ty*32+tx)*64,tl,64);
    }
#ifdef __3DS__
    gg3GlobeHashValid=1;
#endif
}
static void globeEnter(void){
    for(int i=0;i<256;i++) gSavedPal[i]=BG_PALETTE[i]; globePalette();
#ifdef __3DS__
    gg3GlobeHashValid=0;
#endif
    videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32);
    for(int i=0;i<128;i++) oamSetHidden(&oamMain,i,true);
    bgSetScroll(0,0,0); for(int my=0;my<32;my++) for(int mx=0;mx<32;mx++) mapPtr[my*32+mx]=(u16)(my<24?my*32+mx:0);
    globeDraw(1); globeFlush(); view3d=1; }
static void leaveGlobe(void){
    walkOn=0; trUsed=0; memset(trKey,0,sizeof trKey); zl=3; makeTiles(); { int b0=0; (void)b0; } videoSetMode(MODE_0_2D|DISPLAY_BG0_ACTIVE|DISPLAY_BG1_ACTIVE|DISPLAY_SPR_ACTIVE|DISPLAY_SPR_1D_LAYOUT|DISPLAY_SPR_1D_SIZE_32);
    uiPalette(); tbInit(); saveBasePal(); winX=-99; view3d=0; }
#else
#define leaveGlobe() ((void)0)
#endif
// The bitmap zooms use the regular map's terrain art and sprites.
static int zcx=0, zcy=0, zForce=1;        // zcx, zcy: the centre of the view, in 1/8 tile units (the normal view's pixels)
static int zS_=8, zK_=1, zSh_=3;     // pixels to a tile (1, 2, 4, 8, 16, 32) and tiles to a pixel (1: each pixel is one tile; 2 or 4: the planet squeezed, for a big planet's VERY FAR view)
static void zScale(void){ int W=gW; switch(zl){ case Z_VFAR: if(W<=256){ zS_=2; zK_=1; } else { zS_=1; zK_=W>=768?4:W>=384?2:1; } break; case Z_FAR: zS_=W<=256?4:2; zK_=1; break; case Z_CLOSE: zS_=16; zK_=1; break; case Z_MICRO: zS_=32; zK_=1; break; default: zS_=8; zK_=1; }
    zSh_=zS_>=32?5:zS_>=16?4:zS_>=8?3:zS_>=4?2:zS_>=2?1:0; }
static const char* const zNm[6]={"PLANET","VERY FAR","FAR","NORMAL","CLOSE","MICRO"};
#define ZH 168                                    // rows above the toolbar
typedef struct { u32 xy; u32 sig; u8 px[64]; } ZArt; static ZArt zart[512];     // the 8 x 8 art of the tiles in view, so a scroll builds only the new column
// All 16 families have private colors. 200..247 is unused by terrain,
// roof shades, biomes and the toolbar, so zoom never recolors a building.
#define Z_FAM_PAL0 200
#ifdef GG_ZOOM_TEST
static int zArtLookups;
#endif
static inline int zFamPal(int fam){ return Z_FAM_PAL0+(fam&15)*3; }
static int zCol(int fam,int k){ return zFamPal(fam)+k; }
static const u8* zArtOf(int x,int y){
#ifdef GG_ZOOM_TEST
    zArtLookups++;
#endif
    ZArt*z=&zart[((y&15)<<5)|(x&31)]; int t=world[y][x]; int mask=0,foam=0; int bld_=bld[y][x]; int race=bld_&&bldv[y][x]?vilRace(bldv[y][x]-1):0; int best=bld_?-1:cellEdge(x,y,&mask,&foam); int bb=(bld_||t<=T_WATER||t==T_LAVA)?0:bioAt(x,y);
    // Leave all eight bits of the building id independent of its race.
    u32 sig=(u32)(t|((best+1)<<4)|(mask<<8)|(foam<<12)|(bld_<<13)|(race<<21)|(bb<<24))+1u; u32 xy=(u32)(x|(y<<16));
    if(z->sig==sig&&z->xy==xy) return z->px;
    int v=tileVar(t,x,y);
    for(int j=0;j<8;j++) for(int i=0;i<8;i++) z->px[j*8+i]=(u8)(bld_?bldPix(bld_,race,i,j):bioPix(best>=0?edgePix(t,best,mask,v,foam,i,j):(!bb&&t==T_GRASS&&v==2?pxMeadow((int)(hsh(x,y,5)&15),i,j):tilePix(t,v,i,j)),bb));
    z->sig=sig; z->xy=xy; return z->px; }
static inline void zPut(int x,int y,int c){ if((unsigned)x<256&&(unsigned)y<ZH) gbuf[y*256+x]=(u8)c; }
static void zRect(int x,int y,int w,int h,int c){
    // Clip ONCE per rectangle instead of testing every enlarged pixel.
    // The 16x/32x creatures and the 2x/4x terrain call this thousands
    // of times on busy views. A row is contiguous in the shared buffer.
    int x0=x<0?0:x, y0=y<0?0:y, x1=x+w>256?256:x+w, y1=y+h>ZH?ZH:y+h;
    if(x1<=x0||y1<=y0) return;
    for(int yy=y0;yy<y1;yy++) memset(gbuf+yy*256+x0,(u8)c,(size_t)(x1-x0));
}
static int zOx(void){ zScale(); return zcx*zS_/(8*zK_)-128; } static int zOy(void){ zScale(); return zcy*zS_/(8*zK_)-ZH/2; }
static void zClamp(void){ zScale(); int hx=128*8*zK_/zS_, hy=(ZH/2)*8*zK_/zS_; int mx=WW*8, my=WH*8;
    if(2*hx>=mx) zcx=mx/2; else { if(zcx<hx) zcx=hx; if(zcx>mx-hx) zcx=mx-hx; } if(2*hy>=my) zcy=my/2; else { if(zcy<hy) zcy=hy; if(zcy>my-hy) zcy=my-hy; } }
static void zSyncCam(void){ camX=zcx-128; camY=zcy-ZH/2; if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192; }
static int zHit(int px,int py,int*tx,int*ty){ int ox=zOx(), oy=zOy(); int sh=zSh_; int x=zK_>1?(ox+px)*zK_+zK_/2:(ox+px)>>sh, y=zK_>1?(oy+py)*zK_+zK_/2:(oy+py)>>sh; if(x<0||y<0||x>=WW||y>=WH) return 0; *tx=x; *ty=y; return 1; }
static void zCrit(int i,int px,int py,int S){   // one creature, its tile's top left corner at (px, py), enlarged to S pixels
    const Cr*c=&cr[i]; const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a]; int fam=creatureFamily(c);
    if(S<=2){ zRect(px,py,2,2,zCol(fam,0)); return; }
    int mv=(c->ox||c->oy); int fr=mv?(((tick>>3)+i)&1):0; int flip=0; if(!mv) fr=0;
    int gx=c->ox*S/8, gy=c->oy*S/8; px+=gx; py+=gy;
    u8 g[64]; for(int y=0;y<8;y++)for(int x=0;x<8;x++){
        int p=gfxSprIdx(A->shape,fr,flip?7-x:x,y);
        g[y*8+x]=(u8)(p==4?UIP(U_WHITE):p==7?UIP(U_SHADOW):
            p==2||p==6?zCol(fam,1):p==3?zCol(fam,2):p?zCol(fam,0):0);
    }
    if(S>=16){ int sh=S/8; for(int r=0;r<sh;r++){ int yy=py+S-sh+r; for(int xx=px+S/8+r;xx<px+S-S/8-r;xx++){ if((unsigned)xx<256&&(unsigned)yy<ZH){ u8 v=gbuf[yy*256+xx]; if(v<128) gbuf[yy*256+xx]=(u8)((v&~7)|((v&7)>2?(v&7)-3:0)); } } } }   // a shadow: the ground under its feet darkened
    if(S>=16&&mv&&((tick>>1)&1)&&gfxCitizenRace(A->shape)<0) py-=S/16; // animals hop; citizen boots stay grounded
    if(S==4){ for(int dy=0;dy<4;dy++) for(int dx=0;dx<4;dx++){ int c2=g[(dy*2)*8+dx*2]; if(!c2) c2=g[(dy*2)*8+dx*2+1]; if(!c2) c2=g[(dy*2+1)*8+dx*2]; if(!c2) c2=g[(dy*2+1)*8+dx*2+1]; if(c2) zPut(px+dx,py+dy,c2); } }
    else if(S>=16&&gfxCitizenRace(A->shape)>=0){
        int race=gfxCitizenRace(A->shape),k=S/16;
        for(int y=0;y<16;y++)for(int x=0;x<16;x++){
            int p=gfxCitizenPix(race,fr,(int)(crId[i]&3u),x,y);if(!p)continue;
            int c2=p==4?UIP(U_WHITE):p==7?UIP(U_SHADOW):p==2?zCol(fam,1):
                p==3?128+race:p==8?PX_ROOFL+race:p==9?PX_ROOFD+race:
                p==6?zCol(fam,1):zCol(fam,0);
            zRect(px+x*k,py+y*k,k,k,c2);
        }
    }
    else { int k=S/8; for(int y=0;y<8;y++) for(int x=0;x<8;x++){ int c2=g[y*8+x]; if(c2) zRect(px+x*k,py+y*k,k,k,c2); } }
    if((c->rk||(crFl[i]&FL_EVOLVED))&&S>=16){ int cc=c->rk==2?UIP(U_RED):c->rk?UIP(U_GOLD):UIP(U_CYAN); zRect(px+S/2-S/8,py-S/8,S/4,S/8,cc); zRect(px+S/2-S/4,py-S/4+1,S/8,S/8,cc); zRect(px+S/2+S/8,py-S/4+1,S/8,S/8,cc); }      // a crown
    if((DISEASE(&cr[i])==1||DISEASE(&cr[i])==3)&&S>=16&&((tick>>2)&1)) zRect(px+S*5/8,py-S/8,S/4,S/4,UIP(DISEASE(&cr[i])==3?U_PURPLE:U_GREEN));     // the plague's flicker
    if(S>=16&&c->wep){ zRect(px+(flip?0:S-S/8),py+S/4,S/8,S/2,UIP(U_LGREY)); } }
// Building colors for views too small to show the tile art.
static int bZoomCol(int b,int race){
    if(b==B_PAVING)return SH(T_MOUNT,4);
    if(b==B_WELL)return SH(T_MOUNT,7);
    return (b==B_HALL||b==B_TOWER||b==B_TEMPLE||b==B_STATUE)?139:b==B_FARM?140:b==B_DOCK?137:128+race;
}
static GgBuildPlan constructionGhost;
static int constructionGhostX,constructionGhostY,constructionGhostValid,constructionGhostShown;
static int constructionAimPending;
static void constructionCancelAim(void){
    constructionAimPending=constructionGhostShown=0;
}

static void zConstructionPreview(int S,int K,int ox,int oy){
    int ink=UIP(constructionGhostValid?U_GREEN:U_RED);
    for(int i=0;i<constructionGhost.count;i++){
        const GgPlanCell *cell=&constructionGhost.cell[i];
        int wx=constructionGhostX+cell->dx,wy=constructionGhostY+cell->dy;
        if((unsigned)wx>=WW||(unsigned)wy>=WH)continue;
        int x=(K>1?wx/K:wx*S)-ox,y=(K>1?wy/K:wy*S)-oy;
        if(S<=2||K>1){ zPut(x,y,ink);continue; }
        int lane=!cell->kind||ggBuildingIsPaving(cell->kind);
        if(lane){
            int corner=S/4;
            zRect(x,y,corner,1,ink);zRect(x,y,1,corner,ink);
            zRect(x+S-corner,y+S-1,corner,1,ink);
            zRect(x+S-1,y+S-corner,1,corner,ink);
        }else{
            zRect(x,y,S,1,ink);zRect(x,y+S-1,S,1,ink);
            zRect(x,y,1,S,ink);zRect(x+S-1,y,1,S,ink);
        }
    }
}
static int zKey=-1;
static int zDraw(void){
    zScale(); int S=zS_, sh=zSh_, K=zK_; int ox=zOx(), oy=zOy();
    // CLOSE/MICRO artwork and animated entities refresh at up to 30 Hz.
    // The game loop (including sound) still runs at its regular cadence.
    // Camera movement, creature count, weather, and a divine-power edit
    // force an immediate new image; only redundant interleaved frames skip.
    int key=(ox*131+oy*17+zl*7)^((tick>>(zl>=Z_CLOSE?1:2))*257)^(weather*977)^(nCr*29);
    if(constructionGhostShown)key^=0x20000000;
    if(!zForce&&key==zKey)return 0;
    zKey=key;zForce=0;
    memset(gbuf,0,256*ZH);
    if(K>1){       // a planet squeezed K tiles to a pixel: one sampled tile a pixel, creatures as dots
        for(int sy=0;sy<ZH;sy++){ int ty=(oy+sy)*K+K/2; if(ty<0||ty>=WH) continue; u8*d=gbuf+sy*256; for(int sx=0;sx<256;sx++){ int tx=(ox+sx)*K+K/2; if(tx<0||tx>=WW) continue; int t=world[ty][tx]; int b=bld[ty][tx]; int c;
                if(b&&b!=B_RUIN){ int race=bldv[ty][tx]?vilRace(bldv[ty][tx]-1):0; c=bZoomCol(b,race); } else { int h=(int)(hsh(tx,ty,3)&3); c=t*8+((t==T_WATER||t==T_DEEP)?3+((h+(tick>>4))&1):2+h); if(t>T_WATER&&t!=T_LAVA) c=bioPix(c,bioAt(tx,ty)); }
                d[sx]=(u8)c; } }
        for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(!c->alive) continue; int px=c->x/K-ox, py=c->y/K-oy; if((unsigned)px<256&&(unsigned)py<ZH&&(arch[sp[c->sp].a].diet==4||arch[sp[c->sp].a].diet==5||c->rk)) gbuf[py*256+px]=(u8)(arch[sp[c->sp].a].diet==5?UIP(U_RED):UIP(U_YELLOW)); }      // (only people, monsters and notables)
        if(constructionGhostShown)zConstructionPreview(S,K,ox,oy);
        return 1; }
    int tx0=ox>>sh, tx1=(ox+255)>>sh, ty0=oy>>sh, ty1=(oy+ZH-1)>>sh;
    if(S>=16){ int r8=sh-3;      // enlarge each source tile's 8x8 art
        // Resolve art once per visible WORLD tile, not again for every
        // output scanline. CLOSE used to look up each tile 16 times and
        // MICRO 32 times even though zArtOf's signature rarely changes.
        const u8* rowArt[18];   // CLOSE covers at most 17 tiles horizontally
        for(int ty=ty0;ty<=ty1;ty++){
            if(ty<0||ty>=WH) continue;
            for(int tx=tx0;tx<=tx1;tx++) rowArt[tx-tx0]=(tx>=0&&tx<WW)?zArtOf(tx,ty):0;
            int sy0=(ty<<sh)-oy;
            int syBeg=sy0<0?0:sy0, syEnd=sy0+S>ZH?ZH:sy0+S;
            for(int sy=syBeg;sy<syEnd;sy++){
                const int ry=(sy-sy0)>>r8; u8*d=gbuf+sy*256;
                for(int tx=tx0;tx<=tx1;tx++){
                    const u8*art=rowArt[tx-tx0]; if(!art) continue;
                    const u8*a=art+ry*8; const int sx=(tx<<sh)-ox;
                    for(int i=0;i<8;i++){
                        int x0=sx+(i<<r8); int left=x0<0?0:x0, right=x0+(1<<r8);
                        if(right>256) right=256;
                        for(int xx=left;xx<right;xx++) d[xx]=a[i];
                    }
                }
            }
        } }
    else { for(int ty=ty0;ty<=ty1;ty++){ if(ty<0||ty>=WH) continue; int sy0=(ty<<sh)-oy;
            for(int tx=tx0;tx<=tx1;tx++){ if(tx<0||tx>=WW) continue; int sx0=(tx<<sh)-ox; int t=world[ty][tx]; int c; int b=bld[ty][tx];
                if(b&&b!=B_RUIN){ int race=bldv[ty][tx]?vilRace(bldv[ty][tx]-1):0; c=bZoomCol(b,race); }
                else { int h=(int)(hsh(tx,ty,3)&3); c=t*8+((t==T_WATER||t==T_DEEP)?3+((h+(tick>>4))&1):2+h); if(t>T_WATER&&t!=T_LAVA) c=bioPix(c,bioAt(tx,ty)); if(b==B_RUIN) c=142; }
                zRect(sx0,sy0,S,S,c);
                if(S==4){ int h2=(int)(hsh(tx,ty,5)&3); if(b==B_NONE||b==B_RUIN){ if(h2==0) zPut(sx0+1,sy0+2,c^1); else if(h2==1) zPut(sx0+2,sy0+1,c^1); } else if(b==B_HUT||b==B_HOUSE) zRect(sx0,sy0+2,4,2,136); } } } }
    for(int b=0;b<8;b++) if(boat[b].on&&K==1){ int px=boat[b].x*S-ox, py=boat[b].y*S-oy; if(S>=4) zRect(px,py,S,S,137); if(S>=16) zRect(px+S/4,py+S/8,S/2,S/2,UIP(U_WHITE)); else zRect(px,py,S>2?S:2,S>2?S:2,UIP(U_WHITE)); }      // boats at sea
    // creatures: every one on screen
    for(int ty=ty0-1;ty<=ty1+1;ty++){ if(ty<0||ty>=WH) continue; for(int tx=tx0-1;tx<=tx1+1;tx++){ if(tx<0||tx>=WW) continue; int i=(int)occ[ty][tx]-1; if(i<0||!cr[i].alive) continue; zCrit(i,(tx<<sh)-ox,(ty<<sh)-oy,S); } }
    if(constructionGhostShown)zConstructionPreview(S,K,ox,oy);
    return 1; }
// Only 2.6 KiB of signatures: no second framebuffer (43 KiB) and no
// per-frame allocations. These signatures belong exclusively to the
// zoomed BG, and are invalidated whenever that BG is entered afresh.
static u32 zTileHash[32*(ZH/8)]; static int zTileHashValid;
#ifdef GG_ZOOM_TEST
static int zUploadN, zSkipN;
#endif
static void zFlush(void){
    static u8 tl[64] __attribute__((aligned(4)));
    for(int ty=0;ty<ZH/8;ty++) for(int tx=0;tx<32;tx++){
        u32 sig=2166136261u; u32*dst=(u32*)tl;
        for(int y=0;y<8;y++){
            const u32*r=(const u32*)(gbuf+(ty*8+y)*256+tx*8);
            u32 a=r[0], b=r[1]; dst[y*2]=a; dst[y*2+1]=b;
            sig=(sig^a)*16777619u; sig=(sig^b)*16777619u;
        }
        int idx=ty*32+tx;
        if(zTileHashValid&&zTileHash[idx]==sig){
#ifdef GG_ZOOM_TEST
            zSkipN++;
#endif
            continue;
        }
        zTileHash[idx]=sig;
        vramCopy((u8*)BG_TILE_RAM(1)+idx*64,tl,64);
#ifdef GG_ZOOM_TEST
        zUploadN++;
#endif
    }
    zTileHashValid=1;
}
static void zPalette(void){ for(int f=0;f<16;f++) for(int k=0;k<3;k++) BG_PALETTE[zFamPal(f)+k]=RGB15(famCol[f][k][0],famCol[f][k][1],famCol[f][k][2]); }
static void zEnterBmp(void){
    zPalette(); for(int i=0;i<128;i++) oamSetHidden(&oamMain,i,true);
    zBmp=1; bgSetScroll(0,0,0); for(int my=0;my<32;my++) for(int mx=0;mx<32;mx++) mapPtr[my*32+mx]=(u16)(my<ZH/8?my*32+mx:0);
    memset(zart,0,sizeof zart); zTileHashValid=0; zKey=-1; zForce=1; }
static void zLeaveBmp(void){
    zBmp=0; memset(trKey,0,sizeof trKey); trUsed=0; makeTiles(); winX=-99; }
static void zSet(int nz){
    if(nz<Z_MIN) nz=Z_MIN;
    if(nz>Z_MICRO) nz=Z_MICRO;
    if(nz==zl) return;
#ifdef GG_HAS_GLOBE
    if(zl==Z_PLANET){ int tx=rotY*WW/360, ty=(90-rotX)*WH/180; if(tx<0) tx=0; if(tx>=WW) tx=WW-1; if(ty<0) ty=0; if(ty>=WH) ty=WH-1; zcx=tx*8+4; zcy=ty*8+4; leaveGlobe(); }     // (back on the map, with its own tiles)
    else
#endif
    if(zl==Z_NORMAL){ zcx=camX+128; zcy=camY+ZH/2; }
    zl=nz;
#ifdef GG_HAS_GLOBE
    if(nz==Z_PLANET){ if(zBmp) zLeaveBmp(); zl=Z_PLANET; faceTile(zcx/8,zcy/8,&rotX,&rotY); globeEnter(); }
    else
#endif
    if(nz==Z_NORMAL){ if(zBmp){ zClamp(); zLeaveBmp(); } camX=zcx-128; camY=zcy-ZH/2; if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192; winX=-99; }
    else { if(!zBmp) zEnterBmp(); zClamp(); zSyncCam(); zForce=1; }
    siprintf(info,"ZOOM: %s",zNm[zl]); }
static void zRestoreSaved(int zoom,int x,int y){
    zSet(Z_NORMAL);
    camX=x;
    camY=y;
    winX=-99;
    if(zoom>=Z_MIN&&zoom<=Z_MICRO) zSet(zoom);
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
static int ghEnd[4];      // the music tick a hit on each slot stops sounding: a finished sample's channel number is free to be reused (by another hit, or by a note), so killing the old id once it has ended would cut a different sound short
static void gb_hit(int slot,int part,int idx,int vol,int pan){ if(ghId[slot]>=0&&gmTick-ghEnd[slot]<0) soundKill(ghId[slot]); const GmDrum*D=part==2?gmVoxLoad(idx,slot):part?&gmKitB[idx]:&gmKitA[idx]; if(vol>127) vol=127;
#ifdef NDSSHIM
    shimNextBus=part==2&&((idx>=GM_VOX_LYR0&&idx<GM_VOX_LYR0+GM_VOX_LYRN)||(idx>=GM_VOX_CLYR0&&idx<GM_VOX_CLYR0+GM_VOX_CLYRN)); shimNextKick=part==0&&idx==0;
#endif
    ghId[slot]=soundPlaySample(D->d,SoundFormat_8Bit,D->n,D->rate,(u8)vol,(u8)pan,false,0); ghEnd[slot]=gmTick+(int)((unsigned)D->n*60u/(D->rate?D->rate:1))+2; }
static int midi8Hz(int m8){ int m=m8>>3; if(m<0) m=0; if(m>126) m=126; int a=gmHz64[m], b=gmHz64[m+1]; return (a+(b-a)*(m8&7)/8)/64; }
static void gb_ptone(int midi8,int vol,int duty){ (void)duty; if(gpId<0) return; int hz=midi8Hz(midi8); if(hz<20) hz=20; if(vol>127) vol=127; if(vol<0) vol=0; soundSetFreq(gpId,(u16)hz); soundSetVolume(gpId,(u8)vol);
#ifdef __3DS__
    // Keep the actor's on-screen sound with that actor, not pinned to
    // the centre of the listener's stereo field.
    soundSetPan(gpId,(u8)gmFilmPan);
#endif
}   // (one reserved PSG voice, 50% pulse: the 16 channels are all spoken for)
static void gb_poff(void){ if(gpId>=0) soundSetVolume(gpId,0); }
static void gb_pnoise(int freq,int vol){ if(gnId<0) return; if(vol>127) vol=127; if(vol<0) vol=0; if(freq>60000) freq=60000; soundSetFreq(gnId,(u16)freq); soundSetVolume(gnId,(u8)vol);
#ifdef __3DS__
    soundSetPan(gnId,(u8)gmFilmPan);
#endif
}
static void gb_pnoff(void){ if(gnId>=0) soundSetVolume(gnId,0); }
// ---- the music director: what is happening in the world picks the track (with a 30 second hold so it never flips about); jingles play once and hand back
static int musHold, jukeOn, fxCool;
#ifdef __3DS__
// A 3DS-only optional world director owns the music while first-person
// roaming or playing The Unwritten Reign. It never alters normal DS/DSi
// playback, the original 2D story or the user's jukebox data.
static int (*gg3dMusicTick)(void);
static int (*gg3dMusicNext)(int);
#endif
static int isJingle(int s){ return s>=0&&gmSongs[s].loopRow==65535; }
static int famOpen(int k);
// A distant-crowd soundtrack used to scan every creature twice just to
// decide whether a monster was nearby or anywhere in the world. The maintained
// 8x8 spatial-cell mask answers both presence queries without a 12k..200k
// creature walk. If a nearby monster exists we deliberately retain the old
// index-ordered creature walk, so the exact monster-theme priority is unchanged.
static int musCellMonsterNear(int cx,int cy){
#ifdef GG_NOCELL
    (void)cx;(void)cy;return 1;
#else
    int x0=cx-30, x1=cx+30, y0=cy-30, y1=cy+30;
    if(x0<0)x0=0;if(y0<0)y0=0;if(x1>=WW)x1=WW-1;if(y1>=WH)y1=WH-1;
    if(x0>x1||y0>y1)return 0;
    int cw=(WW+7)>>CELL_SH;
    for(int gy=y0>>CELL_SH;gy<=y1>>CELL_SH;gy++)
        for(int gx=x0>>CELL_SH;gx<=x1>>CELL_SH;gx++)
            if(cellMask[gy*cw+gx]&(1u<<8))return 1;
    return 0;
#endif
}
static int musCellMonsterAnywhere(void){
#ifdef GG_NOCELL
    return monCount()>0;
#else
    int n=((WW+7)>>CELL_SH)*((WH+7)>>CELL_SH);
    for(int k=0;k<n;k++)if(cellMask[k]&(1u<<8))return 1;
    return 0;
#endif
}
static int musDirector(void){
    if(storyScore>=0) return storyScore;
    if(paused) return GM_PAUSE;
    if((weather==WX_STORM||weather==WX_RAGE)&&!view3d) return GM_STORM;
    if(view3d) return GM_SPHERE;
    int cx=(camX>>3)+16, cy=(camY>>3)+12;
    if(musCellMonsterNear(cx,cy))
        for(int i=0;i<nCr;i++){ const Cr*c=&cr[i]; if(!c->alive) continue; int a=sp[c->sp].a; if(arch[a].diet!=5||c->hp<100) continue; int dx=c->x-cx, dy=c->y-cy; if(dx*dx+dy*dy>900) continue;
            if(a==ARCH_A+19) return GM_DRAGON; if(a==ARCH_A+17) return GM_DEMON; if(a==ARCH_A+18) return GM_LICH; if(a==ARCH_A+13||a==ARCH_A+12) return GM_KRAKEN; if(a==ARCH_A+16) return GM_DRAGON; }
    if(warHeat>=8||(gmCur==GM_WAR&&warHeat>=2)) return GM_WAR;      // the war drums play while blows are really being struck nearby (not merely while two peoples are at war)
    if(musCellMonsterAnywhere()) return GM_MONSTER;
    { int t=world[cy<WH?cy:WH-1][cx<WW?cx:WW-1]; if(t==T_DESERT) return GM_DESERT; if(t==T_SNOW) return GM_SNOW; if(t==T_DEEP||t==T_WATER) return GM_OCEAN; if(t==T_JUNGLE) return GM_JUNGLE; if(t==T_SWAMP) return GM_SWAMP; if(t==T_LAVA||t==T_MOUNT) return GM_VOLCANO; }
    { int lv=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vil[v].lvl>lv) lv=vil[v].lvl; if(lv>=3) return GM_CITY; if(lv==2) return GM_MARKET; if(lv==1&&nVilOn>0&&(year&1)) return GM_VILLAGE; }
    int p=(tick>>1)&2047; { int nf=0, op[GM_FAMN]; for(int k=0;k<GM_FAMN;k++) if(famOpen(k)) op[nf++]=k; if(nf&&((year+(p>>10))%3==0)) return GM_FAM0+op[(year*5+(p>>9))%nf]; }      // an unlocked REPRISE now and then
    if(fullDone&&(year&1)) return GM_REMIX0+(year*7+(p>>9))%GM_REMIXES; if(!storyOn&&(year%5==4)) return GM_FREEPLAY;
    return (p<256||p>1792)?GM_NIGHT:p<640?GM_MORNING:p<1408?GM_NOON:GM_SUNSET;
}
static int musLast=-1;   // what the director wanted when it last chose: the music only changes when that changes (a song picked by musNext keeps playing until the world gives a new reason)
static unsigned musRot=12345;
static int trackOpen(int i);
// A song has run to its end (it has faded out). In the Aural Sculptures (a song played on its own) the music stops: it faded to silence. In the game the next song takes over:
// what the world calls for if that is a different song, else another open song from the jukebox, the remixes and the rebirths (never the same one twice in a row).
static int musNext(int cur){
    if(storyScore>=0) return storyScore;
    if(jukeOn) return -1;
    int want=musDirector(); musLast=want; musHold=1800;
    if(want!=cur&&!isJingle(want)) return want;
    int pool[GM_SONGS], n=0;
    for(int i=0;i<GM_EPIC0;i++){ if(i==cur||!trackOpen(i)||isJingle(i)) continue; if(i<GM_REMIX0&&gmSongs[i].use[0]!='j') continue; pool[n++]=i; }   // (the place and moment tracks are for the director; "jukebox: ..." songs are free to play anywhere)
    if(!n) return want!=cur?want:-1;
    musRot=musRot*1103515245u+12345u+(unsigned)tick; return pool[(musRot>>16)%(unsigned)n]; }
static void musUpdate(void){
#if defined(GG_SAFE) || defined(GG_NO_AUDIO)
    return;     // diagnostic no-audio build and safe build bypass the mixer
#endif
    gmNextHook=musNext;
#ifdef __3DS__
    if(gg3dMusicNext)gmNextHook=gg3dMusicNext;
#endif
#ifdef NDSSHIM
    shimTempoBpm=gmCur>=0?gmSongs[gmCur].bpm:0;
#endif
#ifdef NDSSHIM
    for(int k=0;k<shimMusicTicks;k++) gmFrame();
#else
    gmFrame();
#endif
    if(fxCool>0) fxCool--; if(fxQ>=0){ if(fxQ!=FX_PAINT||fxCool==0){ fxPlay(fxQ); if(fxQ==FX_PAINT) fxCool=10; } fxQ=-1; }
#ifdef __3DS__
    if(gg3dMusicTick&&gg3dMusicTick()){
        // The dynamic 3D score advances on real music frames, even when
        // the underlying 2D simulation is paused for first-person play.
        musQ=-1;
        return;
    }
#endif
    if(musHold>0) musHold--;
    if(musQ>=0){ gmPlay(musQ);
        // Fade the score in across the film's establishing shot rather
        // than slamming a new mastered track over the last scene.
        if(gmFilmOn){gmInN=gmInT=45;}
        musHold=1800; musQ=-1; musLast=-1; return; }
    if(jukeOn||storyScore>=0) return;
    if((tick&31)==0){ static const short ht[5]={8,40,120,300,700}; int h=warHeat, c=0;      // the crowd: battle heat -> 0..400 (1/100 levels): a skirmish is level 1, a few dozen fighters level 4
        if(h>=ht[4]) c=400; else if(h>=ht[0]){ int k=0; while(h>=ht[k+1]) k++; c=k*100+(h-ht[k])*100/(ht[k+1]-ht[k]); }
        gmCrowd=c; warHeat-=warHeat/4+(warHeat>0); }
    if((tick&31)==0){ int want=musDirector(); if(gmCur<0){ gmPlay(want); musLast=want; } else if(want==gmCur) musLast=want; else if(want!=musLast&&(musHold<=0||((want==GM_WAR||gmCur==GM_WAR)&&musHold<100000))&&!isJingle(gmCur)){ gmPlay(want); musLast=want; musHold=1800; } }
}
#include "construction_preview.h"
static u16*gfxF[SHAPE_N][2];
static void makeSprites(void){
    for(int b=0;b<16;b++){ gfxSprPal(b); }
    for(int s=0;s<SHAPE_N;s++) for(int f=0;f<2;f++){ gfxF[s][f]=oamAllocateGfx(&oamMain,SpriteSize_8x8,SpriteColorFormat_16Color); u8 g[32] __attribute__((aligned(4)));
        for(int y=0;y<8;y++) for(int x=0;x<8;x+=2){ int v[2]; for(int q=0;q<2;q++){ v[q]=gfxSprIdx(s,f,x+q,y); } g[y*4+x/2]=(u8)(v[0]|(v[1]<<4)); } vramCopy(gfxF[s][f],g,32); }
    ggBuildPreviewInit();
}
static struct { short x, y; } wpt[20]; static int wptInit;
static void drawSprites(void){
    int preview=constructionGhostShown;
    constructionGhostShown=0;
    if(zBmp) return;  // all 128 were hidden once by zEnterBmp; the zoom renderer draws its own creatures
    int n=0, nc=100;       // creatures use sprites 0..99 (the last few go to a tornado or a black hole while one is on screen), rank crowns 100..107, weather 108..127
    int hs[8][4], nh=0;    // hazard sprites: x, y, shape, flip
    for(int i=0;i<HZ_N&&nh<7;i++){ const Hz*h=&hz[i]; if(!h->kind) continue; int px=h->x*8-camX, py=h->y*8-camY; if(px<-8||px>=256||py<-16||py>=TB_Y) continue;
        if(h->kind==HZ_TORNADO){ int fl=(tick/6)&1; hs[nh][0]=px; hs[nh][1]=py-8; hs[nh][2]=42; hs[nh][3]=fl; nh++; hs[nh][0]=px; hs[nh][1]=py; hs[nh][2]=43; hs[nh][3]=!fl; nh++; } else { int sh=h->kind==HZ_FIRE||h->kind==HZ_DRAGON?46:h->kind==HZ_GOO||h->kind==HZ_BIOMASS?47:h->kind==HZ_GHOST||h->kind==HZ_MOON||h->kind==HZ_GHOSTCIV?48:h->kind==HZ_SNOWMAN||h->kind==HZ_ICE?49:h->kind==HZ_GARDEN?41:h->kind==HZ_NAVY?61:h->kind==HZ_PLANES?1:h->kind==HZ_HYPERCUBE?44:h->kind==HZ_CYBERCORE?54:h->kind==HZ_PUMPKIN?31:44; hs[nh][0]=px; hs[nh][1]=py; hs[nh][2]=sh; hs[nh][3]=(tick/8)&1; nh++; } }
    int hazardStart=100-nh;
    int crMax=hazardStart-(preview?constructionGhost.count:0);
    for(int ty=(camY>>3)-1;ty<(camY>>3)+26&&n<crMax;ty++) for(int tx=(camX>>3)-1;tx<(camX>>3)+34&&n<crMax;tx++){ if(tx<0||ty<0||tx>=WW||ty>=WH) continue; int i=occ[ty][tx]-1; if(i<0) continue; Cr*c=&cr[i]; if(!c->alive) continue; int px=c->x*8+c->ox-camX, py=c->y*8+c->oy-camY; if(px<-8||px>=256||py<-8||py>=TB_Y-6) continue;   // (the bottom rows are the toolbar)
        const Sp*s=&sp[c->sp]; const Arch*A=&arch[s->a]; int fam=creatureFamily(c); int fr=((tick>>3)+i)&1; if(!c->ox&&!c->oy) fr=0;
        oamSet(&oamMain,n,px,py,0,(DISEASE(c)==1||DISEASE(c)==3)?((fam==3||fam==4)?11:3):fam,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[A->shape][fr],-1,false,false,false,false,false); n++;
        if((c->rk||(crFl[i]&FL_EVOLVED))&&nc<108){ oamSet(&oamMain,nc,px,py-6,0,c->rk==2?10:c->rk?2:6,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[39][0],-1,false,false,false,false,false); nc++; } }
    for(int b=0;b<8;b++) if(boat[b].on&&nc<108){ int px=boat[b].x*8-camX, py=boat[b].y*8-camY; if(px>=-8&&px<256&&py>=-8&&py<TB_Y-6){ oamSet(&oamMain,nc,px,py,0,14,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[45][0],-1,false,false,false,false,false); nc++; } }      // boats at sea
    for(int k=n;k<crMax;k++) oamSetHidden(&oamMain,k,true);
    if(preview)ggBuildPreviewDraw(crMax,hazardStart,&constructionGhost,constructionGhostX,
                                  constructionGhostY,constructionGhostValid);
    for(int k=0;k<nh;k++) oamSet(&oamMain,hazardStart+k,hs[k][0],hs[k][1],0,hs[k][2]==44?15:hs[k][2]==46?1:hs[k][2]==47?13:(hs[k][2]==41||hs[k][2]>=48)?14:13,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[hs[k][2]][0],-1,false,false,hs[k][3]!=0,false,false);
    for(int k=nc;k<108;k++) oamSetHidden(&oamMain,k,true);
    // weather: rain and snow as sprites falling across the touch screen
    int cnt=weather==WX_RAIN?14:weather==WX_STORM?20:weather==WX_SNOW?16:weather==WX_ASH?12:weather==WX_MAGIC?10:weather==WX_RAGE?16:weather==WX_HEAT?8:0;
    if(!wptInit){ wptInit=1; for(int i=0;i<20;i++){ wpt[i].x=(short)(hsh(i,1,3)%256); wpt[i].y=(short)(hsh(i,2,3)%160); } }
    for(int i=0;i<20;i++){ if(i>=cnt||view3d){ oamSetHidden(&oamMain,108+i,true); continue; }
        if(weather==WX_SNOW||weather==WX_MAGIC){ wpt[i].y+=1+(i&1); wpt[i].x+=(short)(((tick>>3)+i)%3)-1; } else if(weather==WX_HEAT){ wpt[i].y-=1+(i&1); wpt[i].x+=(short)(((tick>>2)+i)%3)-1; } else { wpt[i].y+=weather==WX_ASH?3:7; wpt[i].x-=weather==WX_RAGE?1:2; } if(wpt[i].y>=160){ wpt[i].y=-8; wpt[i].x=(short)(rnd()%256); } else if(wpt[i].y<-8){ wpt[i].y=160; wpt[i].x=(short)(rnd()%256); } if(wpt[i].x<0) wpt[i].x+=256; if(wpt[i].x>255) wpt[i].x-=256;
        oamSet(&oamMain,108+i,wpt[i].x,wpt[i].y,0,weather==WX_SNOW?14:weather==WX_MAGIC?13:weather==WX_ASH?3:weather==WX_RAGE?2:weather==WX_HEAT?4:6,SpriteSize_8x8,SpriteColorFormat_16Color,gfxF[(weather==WX_SNOW||weather==WX_MAGIC)?41:40][0],-1,false,false,false,false,false); }
}
static u8 mmCol[T_N];
static int page=0, titleOn=1, sceneOn=0;
static void drawMinimap(void){ if(zBmp&&!view3d){ zScale(); uiVw=256*zK_/zS_; uiVh=ZH*zK_/zS_; if(uiVw>WW) uiVw=WW; if(uiVh>WH) uiVh=WH; uiVx=zcx/8-uiVw/2; uiVy=zcy/8-uiVh/2; } else { uiVw=32; uiVh=24; uiVx=camX>>3; uiVy=camY>>3; } }   // (the minimap is part of the status screen now: see uiStatus)
static const char* const introTxt[]={
    "","  WISP FOUND ONE SEED ON THE SHORE.","","  HELP IT TAKE ROOT.",
    "  THEN GIVE SOMEONE A REASON TO STAY.","","  TAP THE MAP TO USE A POWER.",0};
static const char* const endTxt[]={
    "","  THE WEST HINGE IS FIXED.","  THE FORD IS OPEN.","",
    "  MIRA SET OUT A CUP FOR THE WATCH.","  THE HOLLOW KING TOOK THE NEXT TURN.","",
    "  THE WORLD CARRIES ON.","  THERE IS A SEAT FOR YOU AT SUPPER.",0};
static const char* const credTxt[]={"","","       BORE: GODGUY","","  A SPINOFF OF BORE","","  DESIGN, CODE, MUSIC","  AND MONSTERS","  BY CLAUDE, FOR YOU","","  MUSIC AMALGAMATED FROM","  THE SONGS OF BORE","  (THE DIPPER MAN, AND ALL","  THE REST)","","  MANY MONSTERS","  392 CREATURES","  364 ITEMS","  23 TRAITS","  134 TRACKS","","  THANK YOU FOR PLAYING","","","","",0};
static const char* const helpTxt[]={
    "TOUCH THE WORLD: USE POWER","L R      POWER IN CURRENT TAB","B + L R  SWITCH KIT TABS",
    "X Y      BRUSH SIZE","         (SPAWN: SPECIES)","         (GIFT: ITEM)",
    "         (MUTATE: TRAIT)","         (FIELD TOOLS: MODE)","B + X Y  FIELD BRUSH RADIUS",
    "         OR JUMP TEN SPECIES/ITEMS","B + DRAG PANS THE MAP","L + R    ZOOM IN","X + Y    ZOOM OUT",
#ifdef GG_HAS_GLOBE
    "  PLANET, VERY FAR, FAR,",
    "  NORMAL, CLOSE, MICRO",
    "  PLANET: TOUCH = POWER",
    "  B + DRAG = SPIN, A = CLOUDS",
    "  Y + A  WALK THE GLOBE",
#else
    "  VERY FAR, FAR, NORMAL, CLOSE, MICRO",
#endif
    "D-PAD    PAN (B = FAST)","A        PAUSE  (B+A: SPEED)",
    "SELECT   NEXT INFO PAGE","START    PAUSE MENU","L+R+SELECT    SAVE NOW","",
#ifdef __3DS__
    "3DS OUTDOOR WALKING","SELECT+L   WALK FROM MAP","L+A        USE TARGET","START      RETURN TO MAP","",
#endif
    "STORY: FINISH CHAPTERS TO","OPEN ITEMS FOR FREE PLAY.",
    "100%: MEET AND SLAY EVERY","MONSTER AFTER THE STORY.","",
    "TRAITS: CHILDREN INHERIT AND","SOMETIMES MUTATE. MUTATE","MAKES A SUBSPECIES ON PURPOSE.","",
    "TOWERS SHOOT MONSTERS.","GIFTS DECIDE WHO WINS.","",
    "EVENTS TAB: TORNADO, ACID RAIN,","BLACK HOLE, ATOMIC BOMB, PLAGUE",
    "AND CURSE. HEAL CURES A PLAGUE.","WORLD > LAWS SETS SIM RULES.","",
    "KIT SHOWS STARTERS AND STARRED POWERS.","STAR POWERS IN THE WORLD BROWSER.",
    "WORLD > PINS BOOKMARKS MAP VIEWS.",0};
static const char* const*sceneTxt, *const*sceneNext; static int menuOn, menuSel, menuAsk, toastT, dlgOn, dlgIdx, dlgChars, dlgPaused, sceneOff, sceneT, sceneScroll, endShown, scenePaused, helpSel, trPage;
static const Dl*dlgSet; static char lastInfo[64];
static const FmvClip *sceneClip;
static int sceneFilmLength;
_Static_assert(sizeof fmvClips/sizeof *fmvClips==CHAP_N+1,"One FMV per chapter plus the finale");
_Static_assert(sizeof fmvPortraitOffsets/sizeof *fmvPortraitOffsets==(W_N+1)*FMV_PORTRAIT_CELS,
    "Portrait cels for the cast and older Mira");
static const char* const pwDesc[]={"LOOK AT A CREATURE OR A VILLAGE","PAINT SHALLOW WATER","PAINT DEEP SEA","PAINT SAND","PAINT GRASS","PLANT FOREST","PLANT JUNGLE","RAISE HILLS","RAISE MOUNTAINS","PAINT SNOW","PAINT DESERT","PAINT SWAMP","PAINT LAVA","PAINT CORRUPTED GROUND","PAINT INFERNAL GROUND","PAINT ENCHANTED GROUND","WARM THE AIR (GREENHOUSE)","ADD OXYGEN TO THE AIR","RAISE A DUST CLOUD","CREATE LIFE  (X Y: SPECIES)","RAIN GREENS THE LAND","BURN WHAT YOU TOUCH","STRIKE ONE TILE DEAD","DROP A METEOR","KILL WHAT YOU TOUCH","HEAL CREATURES","FEED CREATURES","FOUND A VILLAGE","START A WAR","END ALL WARS","FILL A VILLAGE STORE","GIVE AN ITEM  (X Y: ITEM)","GIVE A TRAIT  (X Y: TRAIT)","SHAKE THE GROUND","FLOOD THE LAND","MAKE THE LAND BLOOM","CHILL A CREATURE OR VILLAGE","A ROAMING TORNADO TEARS THINGS UP","ACID BURNS PLANTS, HOMES AND FLESH","A HOLE THAT EATS LAND AND LIFE","THE BIG ONE: A CRATER AND FALLOUT","SICKEN FOLK: IT SPREADS BY TOUCH","WITHER FOLK AND ROT A STORE","A CITY BECOMES ITS OWN KINGDOM","WHISPER WAR: TAP A KINGDOM, THEN A TARGET","TAP A KINGDOM: ITS ALLIANCES BREAK","TAP TWO KINGDOMS: THEY ALLY","EVERYONE DESPISES THIS KINGDOM","TAP A KINGDOM: ITS WARS END","A KINGDOM FORGETS ITS LANGUAGE","CREATURES FORGET FAMILY AND LOVED ONES","A CITY FORGETS ITS KINGDOM","RAIN GEAR ON FOLK AND CREATURES","STRIP GEAR FROM ALL IN THE BRUSH","A ROAMING FIRE SPIRIT (FIVE ELEMENTAL VARIANTS)","A CREEPING GREY BLOB EATS THE LAND","A FALLING CRAB: EVERYONE RAVES","THE GIANT CRAB STOMPS (COOLS DOWN)","GIVE ANIMALS A MIND OF THEIR OWN","A STONE THAT SLOWLY UPLIFTS ANIMALS NEAR IT","TOGGLE A CREATURE STATUS (X Y: STATUS)","ALIENS LAND AND HUNT THE LIVING","AN ELDER DRAGON BURNS A PATH","WRAITHS HAUNT SETTLEMENTS","A GROWING TUMOR SPREADS OVER THE LAND","A MOVING BLIZZARD FREEZES THE GROUND","A HEALING GARDEN SPREADS LIFE","A SMALL PRECISE LIGHTNING STRIKE","A WIDER LIGHTNING STRIKE","COPY A TILE AND STAMP IT WITH THE BRUSH","PRINT FOREST","PRINT WATER","PRINT MOUNTAIN","COPY THE BIOME UNDER THE BRUSH","CLEAR BIOME AND BUILDINGS", "MOVE ONE CREATURE WITH THE D-PAD","SEND A CREATURE TO ITS HOME","TOGGLE A CITY'S BORDER MARKERS","CYCLE THROUGH SPECIAL EVENTS","X/Y SWITCHES FIELD TOOLS","X/Y CHOOSE. TAP OPEN LAND."};
_Static_assert(sizeof pwDesc/sizeof *pwDesc==PW_N,"Each power needs a description");
static const char*const fieldDesc[FIELD_N]={
    "TAP A HOME TO OPEN ITS CITY LEDGER",
    "FEED FOLK FROM A CITY'S OWN STORE",
    "TAP TO TREAT PLAGUE AND FEVERS",
    "TAP TO RECLAIM RUINS AND BAD SOIL"
};
static int sceneLines(const char*const*t){ int n=0; while(t[n]) n++; return n; }
static void sceneStart(const char*const*t,int scroll){ sceneTxt=t; sceneScroll=scroll; sceneOff=scroll?-20:0; sceneT=0; if(!sceneOn) scenePaused=paused; sceneOn=1; paused=1; }
static void sceneStartFmv(const char*const*t,const FmvClip*clip){
    filmCaptureTownName();
    sceneStart(t,0);
    sceneClip=clip;
    gmFilmMixSet(1,0,64);
    sceneFilmLength=fmvLength(clip);
    storyScore=filmOpeningScore[clip-fmvClips];
    musReq(storyScore);
    uiNBtn=0;
    uiBtn(0,0,256,192,KEY_A,0,0,0);
    uiTrans=0;
}
static int dbgSel=0, dbgMon=0, dbgFx=0, fxSel=0, konami=0;
// the REPRISE set (26 of the original BORE songs, kept close to what they were, with voices): each one opens with a milestone of the story. 0..13: finishing chapter 1..14; 14..23: every 4 achievements;
// 24: the story's end; 25: 100%.
static int famOpen(int k){ if(unlockAll||debugOn) return 1; if(k<14) return chapter>=k+1; if(k<24) return __builtin_popcount(achBits)+__builtin_popcount(achX[0])>=4*(k-13); return k==24?storyDone:fullDone; }
static void famHint(int k,char*o){ (void)k; strcpy(o,"?? HIDDEN TRACK ??"); }      // the hidden tracks do not say how to open them: they come with the story
static int trackOpen(int i){ if(i>=GM_FAM0) return famOpen(i-GM_FAM0); return i<GM_REMIX0||i>=GM_EPIC0||fullDone||debugOn||unlockAll; }
static int auralOK(void){ return auralOpen||debugOn||unlockAll; }
static int bestSel=0, juke=0, hasSave=0;
#ifndef SAVE_PATH
#define SAVE_PATH "godguy.sav"
#endif
static const char*savePath=SAVE_PATH;
#define SLOT_MAX 9                   /* files: nine worlds side by side. The cartridge chip is cut into 1 to 4 (see compat/ndshw.h) */
#ifdef NDSHW
#define NSLOT hwSlots
#else
#define NSLOT SLOT_MAX
#endif
static int slot=0; static char slotPath[48];
static void slotSet(int n){ slot=((n%NSLOT)+NSLOT)%NSLOT;
#ifdef NDSHW
    hwSlotCur=slot;
#else
    if(slot==0) savePath=SAVE_PATH; else { siprintf(slotPath,"%s.%d",SAVE_PATH,slot+1); savePath=slotPath; }
#endif
}
static int slotPeek(int n,int*yr){     // is there a readable save in slot n, and what year was it saved in (the header only: nothing in the game changes)
    SaveHdr h;
#ifdef NDSHW
    if(!svPeek(n,&h,sizeof h)||!saveHeaderValid(&h)) return 0; *yr=(int)h.year; return 1;
#else
    char p[48], b[56]; if(n==0) strcpy(p,SAVE_PATH); else siprintf(p,"%s.%d",SAVE_PATH,n+1);
    siprintf(b,"%s.bak",p);
    for(int attempt=0;attempt<2;attempt++){
        SV*f=svOpen(attempt?b:p,"rb"); if(!f) continue;
        int ok=svRead(&h,sizeof h,1,f)==1; svClose(f);
        if(ok&&saveHeaderValid(&h)){ *yr=(int)h.year; return 1; }
    }
    return 0;
#endif
}
static void slotRemember(void){
#ifndef NDSHW
    FILE*f=fopen("godguy.slt","wb"); if(f){ fputc('0'+slot,f); fclose(f); }
#endif
}
static void slotRecall(void){
#ifndef NDSHW
    FILE*f=fopen("godguy.slt","rb"); if(f){ int c=fgetc(f); fclose(f); if(c>='0'&&c<'0'+NSLOT) slotSet(c-'0'); }
#endif
}
static int slotYr[SLOT_MAX]={-1,-1,-1,-1,-1,-1,-1,-1,-1};           // what each slot holds (-1 = empty), looked up when it can have changed, not every frame
#define slotOn (NSLOT>1&&sdOk)
static void slotScan(void){ for(int i=0;i<SLOT_MAX;i++){ int y=0; slotYr[i]=(i<NSLOT&&slotPeek(i,&y))?y:-1; } }
static void slotStep(int d){ slotSet(slot+d); slotScan(); hasSave=slotYr[slot]>=0; savedYear=-1; slotRemember(); }
static int slotErase(int k){     // empty slot k (its file and backup, or its first page in the cartridge chip)
#ifdef NDSHW
    return hwSlotErase(k);
#else
    char p[48], b[56]; if(k==0) strcpy(p,SAVE_PATH); else siprintf(p,"%s.%d",SAVE_PATH,k+1); siprintf(b,"%s.bak",p); remove(p); remove(b); return 1;
#endif
}
static int doSave(void){ if(!sdOk) { siprintf(info,"NO SD CARD: NOT SAVED"); return 0; } if(saveGame(savePath)){
#ifdef GG_3DS_QUEST_WORLD_SAVED
    GG_3DS_QUEST_WORLD_SAVED(savePath);
#endif
    savedYear=year; hasSave=1; siprintf(info,"SAVED"); fxReq(FX_SAVE); slotScan(); return 1; } else {
#ifdef NDSHW
 if(svTooBig) siprintf(info,"TOO BIG: %dK NEEDED, SLOT HOLDS %dK",svTooBig,(int)(hwSlotSz/1024)); else
#endif
 siprintf(info,"SAVE FAILED"); return 0; } }
static void setSize(int sz){ planetSize=sz; }
static void startWorld(int story){
    possessed=-1; printerCopied=0; storyOn=story;
    newWorld();
    int x=WW/2,y=WH/2,best=33*33;
    for(int dy=-32;dy<=32;dy++) for(int dx=-32;dx<=32;dx++){
        int tx=WW/2+dx,ty=WH/2+dy,d=dx*dx+dy*dy;
        if(d<best&&buildable(tx,ty)&&!occ[ty][tx]){ x=tx; y=ty; best=d; }
    }
    zSet(Z_NORMAL);
    camX=x*8-128; camY=y*8-72; winX=-99;
    power=0;
    for(int i=0;i<PW_N;i++) if(pw[i].kind==PK_PAINT&&pw[i].arg==T_GRASS){ power=i; break; }
    if(stUse<6) spSel=0;
    fieldMode=FIELD_SURVEY;
    refillWindow(); openStarterKit();
    titleOn=0; paused=0; endShown=storyDone; musReq(GM_NEWWORLD);
    if(story&&chapter==0&&stUse==0){ sceneStartFmv(introTxt,&fmvClips[0]); dlgPend=0; }
}
static void monName(int m,char*out){ int a=ARCH_A+m/16, fam=m%16; strcpy(out,advNm[fam]); strcat(out," "); strcat(out,arch[a].nm); }
// Shared 3D menu globe (DS, DSi, New3DS). This never allocates a
// framebuffer or a texture; terrain comes from the actual world[][]. The
// DS uses a 46px precomputed hemisphere, keeping the source's 54px 3DS
// loading globe and the original world-space rotation/lighting.
#ifdef __3DS__
#define GG_TITLE_GLOBE_RADIUS 54
#else
#define GG_TITLE_GLOBE_RADIUS 46
#endif
#define GG_TITLE_GLOBE_DIAMETER (2*GG_TITLE_GLOBE_RADIUS+1)
#define GG_TITLE_GLOBE_PIXELS (GG_TITLE_GLOBE_DIAMETER*GG_TITLE_GLOBE_DIAMETER)
static short plLon[GG_TITLE_GLOBE_PIXELS],plWy[GG_TITLE_GLOBE_PIXELS];
static u8 plJ[GG_TITLE_GLOBE_PIXELS];
#ifdef NDSSHIM
static signed char plZ[GG_TITLE_GLOBE_PIXELS];
#define PLZSET(k) plZ[k]=(signed char)(z*14.f);
#define PLZ(k) plZ[k]
#else
#define PLZSET(k)
#define PLZ(k) 0
#endif
static int plInit;static short plHalo[180][2];
// The surface cache is a real shaded hemisphere; rotate the WORLD
// coordinates rather than spinning a flat map under a circular mask.
static void uiWorldPreview(int cx,int cy,int R){
    const int B=GG_TITLE_GLOBE_RADIUS,D=GG_TITLE_GLOBE_DIAMETER;
    if(!plInit){
        plInit=1;
        for(int y=-B;y<=B;y++)for(int x=-B;x<=B;x++){
            int k=(y+B)*D+(x+B);
            plJ[k]=255;
            if(x*x+y*y>B*B)continue;
            float nx=(float)x/B,ny=(float)y/B;
            // Integer radicand stays nonnegative even on the exact rim.
            float z=sqrtf((float)(B*B-x*x-y*y))/B;
            float lon=atan2f(nx,z),lat=asinf(ny);
            plLon[k]=(short)((0.5f+lon/6.2831853f)*4096);
            plWy[k]=(short)((0.5f+lat/3.1415927f)*4096);
            float light=0.3f+0.7f*(-nx*0.45f-ny*0.45f+z*0.8f);
            plJ[k]=(u8)(light<0.5f?0:light<0.78f?1:2);
            PLZSET(k)
        }
        for(int a=0;a<180;a++){
            float rr=(float)B+1.5f;
            plHalo[a][0]=(short)(rr*cosf(a*2*0.0174533f));
            plHalo[a][1]=(short)(rr*sinf(a*2*0.0174533f));
        }
    }
    if(R<4)R=4;if(R>B)R=B;
    int shift=(int)(((unsigned)tick>>5)*(unsigned)WW/600u);
    for(int y=-R;y<=R;y++)for(int x=-R;x<=R;x++){
        if(x*x+y*y>R*R)continue;
        int k=((y*B/R)+B)*D+((x*B/R)+B);
        if(plJ[k]==255)continue;
        int wx=((plLon[k]*WW>>12)+shift)%WW;
        int wy=plWy[k]*WH>>12;
        if(wy<0)wy=0;if(wy>=WH)wy=WH-1;
        uiPx(cx+x,cy+y,100+world[wy][wx]*3+plJ[k]);
        uiZSet(cx+x,cy+y,3+PLZ(k));
    }
    for(int a=0;a<180;a++)
        uiPx(cx+plHalo[a][0]*R/B,
             cy+plHalo[a][1]*R/B,U_CYAN);
}
static void uiLoadHook(const char*msg,int pct){
    uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=3) uiRect(0,y,256,1,U_PANEL); for(int k=0;k<60;k++){ int x=(int)(hsh(k,1,7)%256), y=(int)(hsh(k,2,7)%192); uiPx(x,y,(k&3)==0?U_WHITE:U_DIM); }
    uiWorldPreview(128,76,8+pct*46/100); uiTextC(128,138,"BORE: GODGUY",U_GOLD,1); uiTextC(128,150,msg,U_WHITE,1);
    uiBar(48,162,160,9,pct,100,U_GREEN); { char b[8]; siprintf(b,"%d%%",pct); uiTextC(128,163,b,U_WHITE,1); }
    uiText(6,182,"TAP THE KIT TO CHANGE POWERS",U_DIM,1); uiFlush(); }
static int mScr, mSel; static u32 cSeed; static int cSel, cT=-1, cLand; static const char*const cEdgeN[]={"WALLS","VANISH","WRAP"};
static const char*const cCoastN[]={"SMOOTH","NORMAL","RUGGED"}, *const cMtnN[]={"FEW","NORMAL","MANY"};
static const char*const cShapeN[]={"CONTINENTS","SUPERCONTINENT","ISLANDS","ARCHIPELAGO","TWO CONTINENTS","INLAND SEAS",[GW_MARCH]="RIVEN MARCH"}, *const cSeaN[]={"DRY","LOW","NORMAL","HIGH","FLOODED"}, *const cClimN[]={"ICE AGE","COOL","TEMPERATE","WARM","SCORCHED"}, *const cWetN[]={"ARID","NORMAL","LUSH"}, *const cLifeN[]={"EMPTY","SPARSE","NORMAL","TEEMING"};
static void previewWorld(void){ gW=gH=planetSize>256?256:planetSize;      // (the preview of a huge planet is drawn at 256: only its shape and climate matter)
    joyInvalidate(); vilIndexInvalidate(); nCr=0; idReset(); memset(occ,0,sizeof occ); cellClear(); memset(bld,0,sizeof bld); memset(bldv,0,sizeof bldv); memset(vil,0,sizeof vil); nVilOn=0; genWorld(cSeed); camX=(WW*8-256)/2; camY=(WH*8-192)/2; winX=-99; refillWindow();
    int l=0; for(int y=0;y<WH;y++) for(int x=0;x<WW;x++) if(world[y][x]>T_WATER) l++; cLand=l*100/(WW*WH); cT=-1; }
static void createStart(void){ if(!cSeed) cSeed=(rnd()&0xFFFF)|1; cSel=0; previewWorld(); }
static void createInput(int kd){
    int d=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0); int ch=0;
    if(kd&KEY_UP) cSel=(cSel+12)%13; if(kd&KEY_DOWN) cSel=(cSel+1)%13;
    if(kd&KEY_B){ fxReq(FX_MENU_BACK); mScr=0; mSel=1; return; }
    if(cSel==12&&(kd&KEY_A)){ fxReq(FX_MENU_OK); seedFix=cSeed; startWorld(storyOn); seedFix=0; return; }
    if(d||(kd&KEY_A)){ if(!d) d=1; switch(cSel){
        case 0: setSize(sizeStep(planetSize,d>0?1:-1)); ch=1; break;
        case 1: storyOn^=1; break;
        case 2: { int n=sizeof cShapeN/sizeof cShapeN[0]; cShape=(cShape+n+d)%n; ch=1; break; }
        case 3: cCoast=(cCoast+4+d)%3-1; ch=1; break; case 4: cMtn=(cMtn+4+d)%3-1; ch=1; break;
        case 5: cSea=(cSea+7+d)%5-2; ch=1; break; case 6: cClim=(cClim+7+d)%5-2; ch=1; break; case 7: cWet=(cWet+4+d)%3-1; ch=1; break;
        case 8: cLife=(cLife+4+d)%4; break; case 9: diff=(diff+3+d)%3; break;
        case 10: if(kd&KEY_A) cSeed=(rnd()&0xFFFF)|1; else cSeed=(cSeed+d)&0xFFFF; ch=1; break;
        case 11: cEdge=(cEdge+3+d)%3; ch=1; break; } }
    if(kd&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A)) fxReq(FX_MENU_MOVE);
    if(ch) cT=8; }
// GFX1 menus: shared pieces so every menu looks and behaves the same: a header strip, a hint strip, a framed description box
static int menuNoteT, mTop;   // how long a note ("SAVED") stays in the hint strip; the first visible row of a scrolling list
static void gfxHead(const char*t,int back){
    uiGrad(0,0,256,15,G_HEAD); uiRect(0,0,256,1,U_BHI); uiRect(0,15,256,1,U_BHI); uiRect(0,15,44,1,U_GOLD); uiRect(0,16,256,1,U_SHADOW); uiTextS(6,4,t,U_GOLD,1);
    if(back){ uiGradRound(204,2,48,11,G_SEL,U_GOLD); uiTextC(228,4,"BACK",U_WHITE,1); uiBtn(202,0,52,15,KEY_B,0,0,0); } }
static void gfxHint(const char*t,int col){ uiGrad(0,180,256,12,G_FOOT); uiRect(0,180,256,1,U_BHI); uiTextC(128,183,t,col,1); }
static void gfxDescBox(int x,int y,int w,int h,const char*t){ uiFrame(x,y,w,h,U_SHADOW,U_BLO,U_BHI); uiGrad(x+1,y+1,w-2,h-2,G_BG); uiRect(x+1,y+2,2,h-4,U_GOLD); uiWrap(x+5,y+(h>=24?5:h>=22?4:3),w/6-2,h>=30?3:2,t,U_LGREY); }
#ifdef GG_BIGMAP
#define SZHELP "BIGGER PLANETS HOLD MORE LIVES"
#else
#define SZHELP "DS RAM STOPS AT 256: 384-768 NEED THE DSI MODE BUILD"
#endif
static const char* const cHelp[13]={SZHELP,"STORY HAS CHAPTERS, FREE PLAY HAS NONE","THE SHAPE OF THE LAND","SMOOTH BEACHES OR RAGGED, TORN COASTS","HOW MANY PEAKS AND HILLS RISE","HOW MUCH OF THE PLANET IS WATER","FROM ICE AGE TO SCORCHED","HOW LUSH OR DRY THE LAND IS","HOW MANY CREATURES START OUT","HOW HARD THE MONSTERS BITE","THE SAME SEED MAKES THE SAME PLANET","WALLS STOP, VANISH ERASES, WRAP LOOPS","MAKE THE PLANET AND BEGIN"};
static void uiCreate(void){
    uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=4) uiRect(0,y,256,1,U_PANEL);
    gfxHead("CREATE A PLANET",1);
    const char*lb[13]={"SIZE","TYPE","LAND","COASTS","MOUNTAINS","SEA","CLIMATE","WETNESS","LIFE","DIFFICULTY","SEED","EDGE","CREATE THIS PLANET"}; char v[13][20];
    siprintf(v[0],"%dx%d",planetSize,planetSize); strcpy(v[1],storyOn?"STORY":"FREE PLAY"); strcpy(v[2],cShapeN[cShape]); strcpy(v[3],cCoastN[cCoast+1]); strcpy(v[4],cMtnN[cMtn+1]); strcpy(v[5],cSeaN[cSea+2]); strcpy(v[6],cClimN[cClim+2]); strcpy(v[7],cWetN[cWet+1]); strcpy(v[8],cLifeN[cLife]);
    strcpy(v[9],diff==0?"CALM":diff==1?"NORMAL":"BRUTAL"); siprintf(v[10],"%04X",(unsigned)cSeed); strcpy(v[11],cEdgeN[cEdge]); v[12][0]=0;
    for(int i=0;i<13;i++){ int y=19+i*12, sel=i==cSel;
        uiRound(4,y,156,11,i==12?(sel?U_TEAL:U_PANELHI):(sel?U_SEL:U_PANELHI),sel?U_GOLD:U_BHI);
        uiBtn(4,y,156,12,i==12?KEY_A:0,0,&cSel,i); if(i<10||i==11){ uiBtn(90,y,34,12,KEY_LEFT,0,&cSel,i); uiBtn(124,y,36,12,KEY_RIGHT,0,&cSel,i); }
        if(i==12){ uiTextC(78,y+2,lb[i],sel?U_WHITE:U_GOLD,1); uiIcon8(146,y+2,IC8_RIGHT); continue; }
        uiText(10,y+2,lb[i],sel?U_WHITE:U_LGREY,1);
        if(i==10){ uiRound(120,y+1,37,9,sel?U_SEL:U_PANEL,sel?U_GOLD:U_BHI); uiTextC(138,y+2,"ROLL",sel?U_WHITE:U_LGREY,1); uiBtn(120,y,40,12,KEY_A,0,&cSel,i); uiTextR(114,y+2,v[i],sel?U_GOLD:U_LGREY); continue; }
        int vc=i==9?(diff==0?U_GREEN:diff==2?U_RED:(sel?U_GOLD:U_LGREY)):(sel?U_GOLD:U_LGREY);
        int vw=uiTextW(v[i],1); if(sel){ uiIcon8(154-vw-14,y+2,IC8_LEFT); uiIcon8(150,y+2,IC8_RIGHT); } uiTextR(148,y+2,v[i],vc); }
    // the preview of the planet as it will be, and what it is
    uiFrame(163,18,90,90,U_SHADOW,U_BLO,U_BHI);
    // The seed and climate are previewed on the same lit, rotating
    // sphere used by the title and loading screens. A rectangular
    // equirectangular atlas is no longer passed off as a 3D globe.
    uiWorldPreview(208,63,41);
    char b[24]; uiBar(165,113,86,7,cLand,100,U_GREEN); siprintf(b,"LAND %d%%",cLand); uiTextC(208,123,b,U_WHITE,1);
    siprintf(b,"%dX%d",planetSize,planetSize); uiTextC(208,138,b,U_DIM,1);
    uiTextC(208,149,storyOn?"STORY":"FREE PLAY",U_GOLD,1); uiTextC(208,159,diff==0?"CALM":diff==1?"NORMAL":"BRUTAL",diff==0?U_GREEN:diff==2?U_RED:U_LGREY,1);
    uiRound(170,168,76,11,U_PANELHI,cT>=0?U_ORANGE:U_GREEN); uiTextC(208,170,cT>=0?"SHAPING...":"READY",cT>=0?U_ORANGE:U_GREEN,1);
    gfxHint(cSel==2&&cShape==GW_MARCH?"ISLANDS, RIVER FORDS AND OLD RUINS":cHelp[cSel],U_LGREY);
    uiFlush(); }
enum { MI_CONT, MI_STORY, MI_FREE, MI_OPT, MI_EXTRA, MI_BACK, MI_SIZE, MI_DIFF, MI_VOL, MI_MON, MI_SCULPT, MI_BEST, MI_CODEX, MI_HELP, MI_CRED, MI_DEBUG, MI_RESUME, MI_SAVE, MI_NEW, MI_MODE, MI_TITLE, MI_CREATE, MI_GO, MI_STYPE, MI_SPEED, MI_KING, MI_JOURNAL, MI_ACH, MI_CULT, MI_LANG, MI_BROWSE, MI_STATS, MI_FAVS, MI_MISSION, MI_WORLD, MI_LOAD, MI_SLOT, MI_CAL, MI_SCALE, MI_MMODE, MI_PEOPLES, MI_MMICON, MI_AUTOSAVE, MI_FILMS };
static int fieldLedgerOpen;
static int extrasView;      // which menu (0 main, 1 options, 2 extras), the highlighted row, a pending "are you sure", and whether an extras page is open
static int menuList(int*ids){ int n=0, ctx=titleOn?0:1;
    if(mScr==0){ if(ctx==0){ ids[n++]=MI_CONT; ids[n++]=MI_CREATE; ids[n++]=MI_OPT; ids[n++]=MI_EXTRA; if(slotOn) ids[n++]=MI_SLOT; } else { ids[n++]=MI_RESUME; if(storyOn&&chapter<CHAP_N) ids[n++]=MI_MISSION; ids[n++]=MI_JOURNAL; ids[n++]=MI_STATS; ids[n++]=MI_FAVS; ids[n++]=MI_WORLD; ids[n++]=MI_SAVE; ids[n++]=MI_AUTOSAVE; if(sdOk) ids[n++]=MI_LOAD; if(slotOn) ids[n++]=MI_SLOT; ids[n++]=MI_OPT; ids[n++]=MI_EXTRA; } }
    else if(mScr==3){ ids[n++]=MI_SIZE; ids[n++]=MI_STYPE; ids[n++]=MI_DIFF; ids[n++]=MI_GO; ids[n++]=MI_BACK; }
    else if(mScr==1){ ids[n++]=MI_DIFF; ids[n++]=MI_SPEED; ids[n++]=MI_VOL; ids[n++]=MI_MON; ids[n++]=MI_SCALE; ids[n++]=MI_MMODE; ids[n++]=MI_MMICON;
#ifdef HW_CAL
 ids[n++]=MI_CAL;
#endif
 if(ctx){ ids[n++]=MI_NEW; ids[n++]=MI_MODE; ids[n++]=MI_TITLE; } ids[n++]=MI_BACK; }
    else { ids[n++]=MI_SCULPT; ids[n++]=MI_BROWSE; ids[n++]=MI_KING; ids[n++]=MI_CULT; ids[n++]=MI_LANG; ids[n++]=MI_PEOPLES; if(!ctx) ids[n++]=MI_JOURNAL; ids[n++]=MI_ACH; ids[n++]=MI_BEST; ids[n++]=MI_CODEX; ids[n++]=MI_HELP;
#ifdef __3DS__
 ids[n++]=MI_FILMS;
#endif
 ids[n++]=MI_CRED; if(debugOn) ids[n++]=MI_DEBUG; ids[n++]=MI_BACK; }
    return n; }
static void menuText(int id,char*l,char*v){ v[0]=0;
    switch(id){ case MI_CONT: strcpy(l,hasSave?"CONTINUE YOUR WORLD":"START THE STORY"); break; case MI_STORY: strcpy(l,"NEW STORY WORLD"); break; case MI_FREE: strcpy(l,"NEW FREE PLAY WORLD"); break;
        case MI_BROWSE: strcpy(l,"WORLD BROWSER"); break; case MI_KING: strcpy(l,"KINGDOMS"); break; case MI_CULT: strcpy(l,"CULTURE TREE"); break; case MI_PEOPLES: strcpy(l,"POPULATION"); break; case MI_MMICON: strcpy(l,"MAP DOTS"); strcpy(v,mmIcons?"EVERY KIND":"FOLK ONLY"); break; case MI_AUTOSAVE: strcpy(l,"AUTOSAVE"); strcpy(v,autoSaveOn?"ON":"OFF"); break; case MI_LANG: strcpy(l,"LANGUAGES"); break; case MI_JOURNAL: strcpy(l,"JOURNAL"); siprintf(v,"%d",jrN); break; case MI_ACH: { strcpy(l,"ACHIEVEMENTS"); int c=0; for(int i=0;i<ACH_N;i++) c+=ACH_HAS(i); siprintf(v,"%d/%d",c,ACH_N); } break; case MI_SPEED: strcpy(l,"GAME SPEED"); siprintf(v,"%dx",speedMul); break; case MI_CREATE: strcpy(l,"CREATE A PLANET"); strcpy(v,">"); break; case MI_GO: strcpy(l,"CREATE THE PLANET!"); break; case MI_STYPE: strcpy(l,"WORLD TYPE"); strcpy(v,storyOn?"STORY":"FREE PLAY"); break;
        case MI_MMODE: strcpy(l,"MAP TINT"); strcpy(v,mmMode==0?"KINGDOM":mmMode==1?"CULTURE":mmMode==2?"FAITH":mmMode==3?"LANGUAGE":mmMode==4?"RELATIONS":"CITY"); break;
        case MI_SCALE: strcpy(l,"UI SCALE"); strcpy(v,hwUiScale==0?"NORMAL":hwUiScale==1?"LARGE":"HUGE"); break; case MI_CAL: strcpy(l,"CALIBRATE TOUCH"); strcpy(v,">"); break; case MI_OPT: strcpy(l,"OPTIONS"); strcpy(v,">"); break; case MI_EXTRA: strcpy(l,"EXTRAS"); strcpy(v,">"); break; case MI_BACK: strcpy(l,"BACK"); break;
        case MI_SIZE: strcpy(l,"PLANET SIZE"); siprintf(v,"%dx%d",planetSize,planetSize); break; case MI_DIFF: strcpy(l,"DIFFICULTY"); strcpy(v,diff==0?"CALM":diff==1?"NORMAL":"BRUTAL"); break;
        case MI_VOL: strcpy(l,"MUSIC VOLUME"); siprintf(v,"%d%%",gmMaster); break; case MI_MON: strcpy(l,"MONSTERS"); strcpy(v,monOn?"ON":"OFF"); break;
        case MI_SCULPT: strcpy(l,"AURAL SCULPTURES"); strcpy(v,auralOK()?"":"LOCKED"); break; case MI_BEST: strcpy(l,"BESTIARY"); siprintf(v,"%d/%d",seenN,MON_N); break; case MI_CODEX: strcpy(l,"SUBSPECIES"); siprintf(v,"%d/%d",traitMetCount(),TR_N-1); break;
        case MI_STATS: strcpy(l,"STATS"); break; case MI_FAVS: strcpy(l,"FAVOURITES"); break; case MI_WORLD: strcpy(l,"WORLD"); strcpy(v,">"); break; case MI_MISSION: strcpy(l,"MISSION GOAL"); break; case MI_HELP: strcpy(l,"HELP"); break; case MI_FILMS: strcpy(l,"FILM ARCHIVE"); strcpy(v,">"); break; case MI_CRED: strcpy(l,"CREDITS"); break; case MI_DEBUG: strcpy(l,"DEBUG MENU"); break;
        case MI_LOAD: strcpy(l,"LOAD GAME"); break; case MI_SLOT: strcpy(l,"SAVE SLOT"); if(slotYr[slot]>=0) siprintf(v,"%d Y%d",slot+1,slotYr[slot]&0xFFFF); else siprintf(v,"%d EMPTY",slot+1); break; case MI_RESUME: strcpy(l,"RESUME"); break; case MI_SAVE: strcpy(l,"SAVE GAME"); if(savedYear>=0) siprintf(v,"SAVED Y%d",savedYear&0xFFFF); break; case MI_NEW: strcpy(l,"NEW WORLD"); break; case MI_MODE: strcpy(l,"MODE"); strcpy(v,storyOn?"STORY":"FREE PLAY"); break; case MI_TITLE: strcpy(l,"TITLE SCREEN"); break; } }
static int menuAdj(int id){ return id==MI_SIZE||id==MI_STYPE||id==MI_SPEED||id==MI_DIFF||id==MI_VOL||id==MI_MON||id==MI_SLOT||id==MI_SCALE||id==MI_MMODE||id==MI_MMICON; }
static const u8 extOrder[15]={13,1,8,12,14,7,15,10,11,9,2,5,6,3,4};
static const char* extName(int pg){ switch(pg){ case 13: return "STATS"; case 14: return "WORLD"; case 1: return "MISSION"; case 12: return "BROWSER"; case 2: return "BESTIARY"; case 3: return "AURAL"; case 4: return "DEBUG"; case 5: return "TRAITS"; case 6: return "HELP"; case 7: return "KINGDOMS"; case 8: return "JOURNAL"; case 9: return "GOALS"; case 10: return "CULTURE"; case 15: return "PEOPLES"; default: return "LANGUAGES"; } }
static int extOpen(int pg){ return !(pg==3&&!auralOK())&&!(pg==4&&!debugOn); }
static int extStep(int d){ int i=0; for(int k=0;k<15;k++) if(extOrder[k]==page) i=k; for(int n=0;n<15;n++){ i=(i+d+15)%15; if(extOpen(extOrder[i])) return extOrder[i]; } return page; }
static const char* menuDesc(int id){ switch(id){
    case MI_CONT: return hasSave?"PICK UP YOUR SAVED WORLD WHERE YOU LEFT IT":"BEGIN THE STORY ON A NEW PLANET"; case MI_CREATE: return "CHOOSE SIZE, LAND, CLIMATE AND SEED OF A NEW PLANET"; case MI_OPT: return "DIFFICULTY, SPEED, VOLUME, MONSTERS"; case MI_EXTRA: return "KINGDOMS, MUSIC, BESTIARY, HELP AND MORE";
    case MI_MMICON: return "THE DOTS ON THE MINIMAP: ONLY FOLK AND MONSTERS, OR EVERY KIND OF CREATURE"; case MI_MMODE: return "WHAT THE RINGS ON THE MAP SHOW. RELATIONS: GREEN ALLY, RED WAR"; case MI_AUTOSAVE: return "AUTOMATICALLY SAVE AT EACH NEW YEAR; MANUAL SAVE STILL WORKS WHEN OFF"; case MI_SCALE: return "BIGGER TOOLBAR AND TOUCH TARGETS"; case MI_CAL: return "TAP TWO CROSSES SO THE STYLUS LANDS WHERE YOU TOUCH"; case MI_DIFF: return "HOW HARD THE MONSTERS BITE"; case MI_SPEED: return "HOW FAST TIME RUNS"; case MI_VOL: return "MUSIC AND EFFECT LOUDNESS"; case MI_MON: return "TURN THE NIGHTMARES ON OR OFF";
    case MI_SCULPT: return "THE JUKEBOX: ALL THE TRACKS AND SUITES"; case MI_BROWSE: return "FIND AND STAR CITIES, FOLK, ITEMS, POWERS"; case MI_KING: return "WHO RULES WHAT: WARS, ALLIES, VASSALS"; case MI_CULT: return "WHAT EACH KINGDOM HAS LEARNED"; case MI_LANG: return "THE FIVE TONGUES AND THEIR NAMES"; case MI_PEOPLES: return "WHO LIVES UNDER WHICH CULTURE, FAITH, TONGUE"; case MI_JOURNAL: return "THE LAST EVENTS OF THE WORLD"; case MI_ACH: return "THINGS YOU HAVE DONE AS A GOD"; case MI_BEST: return "EVERY NIGHTMARE YOU HAVE MET"; case MI_CODEX: return "SUBSPECIES AND WHAT THEY DO"; case MI_HELP: return "THE CONTROLS"; case MI_FILMS: return "UNLOCK NEW 3DS STORY FILMS AS YOU PLAY"; case MI_CRED: return "WHO MADE THIS"; case MI_DEBUG: return "CHEATS AND TOOLS";
    case MI_STATS: return "COUNTS OF EVERYTHING"; case MI_FAVS: return "YOUR STARRED CITIES, KINGS, ITEMS"; case MI_WORLD: return "CHANGE THE RULES OF THE WORLD"; case MI_MISSION: return "THE CURRENT CHAPTER GOAL"; case MI_RESUME: return "BACK TO THE WORLD"; case MI_SAVE: return "WRITE THE WORLD TO THE CARD"; case MI_LOAD: return "GO BACK TO THE LAST SAVE"; case MI_SLOT: return "PICK THE SAVE FILE FOR SAVE AND CONTINUE"; case MI_NEW: return "REPLACE THIS WORLD WITH A NEW ONE"; case MI_MODE: return "SWITCH BETWEEN STORY AND FREE PLAY"; case MI_TITLE: return "SAVE AND GO TO THE TITLE SCREEN"; case MI_SIZE: return "BIGGER PLANETS TAKE LONGER TO MAKE"; case MI_STYPE: return "STORY HAS CHAPTERS, FREE PLAY HAS NONE"; case MI_GO: return "MAKE THE PLANET AND BEGIN"; default: return ""; } }
static const char* gfxDesc(int id){ if(menuNoteT>0&&info[0]) return info; const char*d=menuDesc(id); if(d[0]) return d; return id==MI_BACK?"GO BACK TO THE LAST MENU":""; }
static void uiMenuList(int x,int y,int w,int rh,int vis){   // rows with a steady gold highlight; more rows than fit scroll (scroll bar, touch arrows)
    if(hwUiScale){ int tot=vis*rh; rh+=3*hwUiScale; vis=tot/rh; if(vis<3) vis=3; }   // UI SCALE: taller rows, fewer of them
    int ids[16]; int n=menuList(ids); if(mSel>=n) mSel=n-1; if(mSel<0) mSel=0; char l[32], v[16];
    int sc=n>vis; if(!sc) vis=n; if(mSel<mTop) mTop=mSel; if(mSel>=mTop+vis) mTop=mSel-vis+1; if(mTop>n-vis) mTop=n-vis; if(mTop<0) mTop=0;
    int rw=sc?w-10:w;
    for(int r=0;r<vis;r++){ int i=mTop+r, id=ids[i]; menuText(id,l,v); int yy=y+r*rh, sel=i==mSel, ty=yy+(rh-9)/2, vx=x+rw;
        uiRect(x+2,yy+rh-2,rw-3,1,U_SHADOW); uiGradRound(x,yy,rw,rh-2,sel?G_SEL:G_BTN,sel?U_GOLD:U_BHI); if(sel){ uiRect(x+1,yy+2,2,rh-6,U_GOLD); uiRect(x+3,yy+1,rw-6,1,U_WHITE); }
        if(sel) uiText(x+5,ty,">",U_GOLD,1); uiText(x+(sel?13:8),ty,l,sel?U_WHITE:U_LGREY,1);
        uiBtn(x,yy,rw,rh-1,menuAdj(id)?0:KEY_A,0,&mSel,i); if(menuAdj(id)){ uiBtn(vx-78,yy,30,rh-1,KEY_LEFT,0,&mSel,i); uiBtn(vx-30,yy,30,rh-1,KEY_RIGHT,0,&mSel,i); }
        if(v[0]){ if(v[0]=='>'&&!v[1]) uiIcon8(vx-13,yy+(rh-10)/2,IC8_RIGHT);
            else if(menuAdj(id)){ if(sel){ uiIcon8(vx-72,yy+(rh-10)/2,IC8_LEFT); uiIcon8(vx-12,yy+(rh-10)/2,IC8_RIGHT); } uiTextC(vx-42,ty,v,sel?U_GOLD:U_LGREY,1); }
            else uiTextR(vx-8,ty,v,sel?U_GOLD:U_LGREY); } }
    if(sc){ int sx=x+w-8, sh=vis*rh-2, th=(sh-20)*vis/n; if(th<5) th=5; int t0=y+10+(sh-20-th)*mTop/(n-vis);
        uiRect(sx,y,8,sh,U_SHADOW); uiRect(sx+2,t0,4,th,U_BHI); if(mTop>0) uiIcon8(sx,y+1,IC8_UP); if(mTop<n-vis) uiIcon8(sx,y+sh-9,IC8_DOWN);
        uiBtn(sx-3,y,12,sh/2,KEY_UP,0,0,0); uiBtn(sx-3,y+sh/2,12,sh-sh/2,KEY_DOWN,0,0,0); } }
// the 3D title logo (title3d.h): BORE in chrome over GODGUY in stone slabs. The 50 colours sit at 204.. of the sub palette (free on the title screen), set on every draw.
static void uiTitleArt(int x0,int y0){
    for(int i=0;i<TT_N;i++) BG_PALETTE_SUB[TT_PAL0+i]=ttPal[i];
    int p=0; for(int k=0;k<(int)(sizeof ttRle/sizeof ttRle[0]);k++){ int v=ttRle[k]>>8, n=ttRle[k]&255; if(v) for(int j=0;j<n;j++){ int q=p+j; uiPx(x0+q%TT_W,y0+q/TT_W,TT_PAL0+v-1); } p+=n; } }
static int slSel, slAsk=-1;
static void uiSlots(void){     // the SAVE SLOTS screen: every slot with the year it holds, A picks it, X (twice) empties it; on the cartridge L / R change how many slots the chip is cut into
    uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=4) uiRect(0,y,256,1,U_PANEL);
    gfxHead("SAVE SLOTS",1);
    for(int i=0;i<NSLOT;i++){ int y=19+i*15, sel=i==slSel; char b[28];
        uiRound(4,y,248,14,sel?U_SEL:U_PANELHI,sel?U_GOLD:U_BHI); uiBtn(4,y,200,14,KEY_A,0,&slSel,i); uiBtn(206,y,44,14,KEY_X,0,&slSel,i);
        siprintf(b,"SLOT %d",i+1); uiText(10,y+3,b,sel?U_WHITE:U_LGREY,1);
        if(slotYr[i]>=0){ siprintf(b,"YEAR %d",slotYr[i]&0xFFFF); uiText(70,y+3,b,sel?U_GOLD:U_LGREY,1); } else uiText(70,y+3,"EMPTY",U_DIM,1);
        if(i==slot) uiText(140,y+3,"IN USE",U_GREEN,1);
        if(slotYr[i]>=0){ uiRound(208,y+2,40,10,slAsk==i?U_RED:U_PANEL,sel?U_GOLD:U_BHI); uiTextC(228,y+3,slAsk==i?"SURE?":"ERASE",U_WHITE,1); } }
#ifdef NDSHW
    { char b[44]; siprintf(b,"CHIP %dK    %d SLOT%s OF %dK",(int)(hwCap/1024),hwSlots,hwSlots>1?"S":"",(int)(hwSlotSz/1024)); uiText(8,19+NSLOT*15+3,b,U_DIM,1); uiText(8,19+NSLOT*15+14,"L / R: HOW MANY SLOTS",U_LGREY,1); }
#endif
    // The lower half of the slot page is unused on 1-4 slot cartridges.
    // Keep a small planet there without covering the selection buttons
    // or the footer. The underlying world is not changed by this preview.
    if(NSLOT<=4){
        uiWorldPreview(208,144,25);
        uiTextC(208,170,"YOUR WORLD",U_GOLD,1);
    }
    gfxHint("A: USE THIS SLOT   X: ERASE   B: BACK",U_LGREY); }
static void uiTitle(void){
    if(mScr==3){ uiCreate(); return; }
    if(mScr==4){ uiSlots(); return; }
    uiGrad(0,0,256,192,G_BG); for(int y=0;y<192;y++) if((y&3)==0) uiRect(0,y,256,1,U_PANEL);
    for(int k=0;k<70;k++){ int x=(int)(hsh(k,1,7)%256), y=(int)(hsh(k,2,7)%192); uiPx(x,y,(k&3)==0?U_WHITE:U_DIM); }
    int ids[16];
    if(mScr==0){
        uiWorldPreview(206,128,planetSize==128?34:planetSize==192?40:46);
        uiTitleArt(6,2);
        uiRect(8,73,148,1,U_BHI); uiRect(8,73,36,1,U_GOLD); uiText(10,77,"MAIN MENU",U_GOLD,1);
        uiMenuList(8,88,148,slotOn?13:16,slotOn?5:4); menuList(ids); gfxDescBox(8,154,148,34,gfxDesc(ids[mSel])); }
    else { uiWorldPreview(215,108,planetSize==128?30:36);
        gfxHead(mScr==1?"MAIN MENU  >  OPTIONS":"MAIN MENU  >  EXTRAS",1);
        uiMenuList(4,21,170,15,9); menuList(ids); gfxDescBox(4,158,170,33,gfxDesc(ids[mSel])); }
    uiFlush(); }
static void uiScene(void){
    uiBtn(0,0,256,192,KEY_A,0,0,0);
    int n=sceneLines(sceneTxt); uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=2) uiRect(0,y,256,1,U_PANEL);
    if(sceneScroll){ for(int r=0;r<24;r++){ int l=sceneOff+r; if(l>=0&&l<n){ int yy=r*8; int c=(yy<24||yy>160)?U_DIM:U_WHITE; if(sceneTxt[l][0]) uiTextC(128,yy,sceneTxt[l],c,1); } } uiText(6,184,"A SKIP",U_DIM,1); }
    else { uiFrame(14,10,228,160,U_PANEL,U_BHI,U_BLO); uiFrame(17,13,222,154,U_PANELHI,U_BLO,U_BHI); for(int l=0;l<n;l++) uiTextC(128,20+l*9,sceneTxt[l],U_WHITE,1); uiTextC(128,160,"PRESS A",U_GOLD,1); }
    uiFlush(); }

static void uiBig(int x,int y,int idx,int sc){ for(int j=0;j<8;j++) for(int i=0;i<8;i++){ int c=icon8[idx][j*8+i]; if(c) uiRect(x+i*sc,y+j*sc,sc,sc,c); } }
static int crCaptain(int i){      // the captain of its war band: the notable fighter with the most kills among its own kind within ten tiles (worked out when asked, nothing saved)
    const Cr*c=&cr[i]; int k0=nbFind(crId[i]); if(k0<0) return 0; for(int dy=-10;dy<=10;dy++) for(int dx=-10;dx<=10;dx++){ int x=c->x+dx, y=c->y+dy; if(x<0||y<0||x>=WW||y>=WH||!occ[y][x]) continue; int j=occ[y][x]-1; if(j==i||!cr[j].alive||cr[j].sp!=c->sp||!(crFl[j]&FL_NOTE)) continue; int k=nbFind(crId[j]); if(k>=0&&(nb[k].kills>nb[k0].kills||(nb[k].kills==nb[k0].kills&&crId[j]<crId[i]))) return 0; } return 1; }
static const char*crDoing(const Cr*c){     // "what is it doing?": worked out from what it stands on and how it is, nothing is stored
    int t=world[c->y][c->x], d=arch[sp[c->sp].a].diet;
    if(DISEASE(c)==3) return "HUNTING THE LIVING"; if(HAS_STATUS(c,ST_MINDLESS)) return "WANDERING AIMLESSLY";
    if(t==T_DEEP||t==T_WATER) return d==5?"HUNTING IN THE WATER":"SWIMMING";
    if(t==T_LAVA) return "BURNING";
    if(c->hp*3<maxHp(c)) return "NURSING WOUNDS";
    if(c->food<40) return d==4?"LOOKING FOR FOOD":"HUNTING FOR FOOD";
    if(t==T_SNOW) return "WADING THROUGH SNOW";
    if(d==5) return "PROWLING";
    if(d==4) return bld[c->y][c->x]?"AT HOME":"GOING ABOUT ITS DAY";
    return t==T_GRASS?"GRAZING":"WANDERING"; }
static void uiInspectCard(void){
    uiFrame(138,15,116,132,U_PANEL,U_GOLD,U_BLO); char b[64], nm[48]; int y=19;
    if(inspKind==1&&inspIdx>=0&&inspIdx<nCr&&cr[inspIdx].alive){ Cr*c=&cr[inspIdx]; int fam=creatureFamily(c), shape=arch[sp[c->sp].a].shape;
        for(int k=0;k<3;k++) BG_PALETTE_SUB[200+k]=RGB15(famCol[fam][k][0],famCol[fam][k][1],famCol[fam][k][2]); BG_PALETTE_SUB[203]=RGB15(31,31,31);
        uiFrame(142,19,38,38,U_SHADOW,U_BHI,U_BLO); for(int j=0;j<8;j++) for(int i=0;i<8;i++){ char ch=shpRow(shape,(tick>>4)&1,j)[i]; int col=ch=='b'?200:ch=='d'?201:ch=='a'?202:ch=='e'?203:0; if(col) uiRect(144+i*4,21+j*4,4,4,col); }
        crName(c,nm); uiWrap(184,20,11,4,nm,U_WHITE); spName(c->sp,nm); (void)nm; y=62;
        { char hb[24]; siprintf(hb,"HEALTH   LEVEL %d",c->dir); uiText(142,y,hb,U_DIM,1); } uiBar(142,y+9,108,7,c->hp,maxHp(c),c->hp*3<maxHp(c)?U_RED:U_GREEN); y+=19;
        uiText(142,y,"HUNGER",U_DIM,1); uiBar(142,y+9,108,7,c->food>200?200:c->food,200,U_ORANGE); y+=19;
        siprintf(b,"AGE %d   %s",c->age,arch[sp[c->sp].a].diet==5?"MONSTER":arch[sp[c->sp].a].diet==4?"FOLK":"BEAST"); uiText(142,y,b,U_LGREY,1); y+=10;
        uiText(142,y,crDoing(c),U_GREEN,1); y+=10;
        if(crNamed(c)&&y<=128){ int po[2]; idPers(crId[inspIdx],crFl[inspIdx],po); siprintf(b,"%s %s",persN[po[0]],persN[po[1]]); uiText(142,y,b,U_CYAN,1); y+=10; }      // personality: worked out from its id
        if((crFl[inspIdx]&FL_NOTE)&&y<=128){ int k=nbFind(crId[inspIdx]); if(k>=0){ siprintf(b,"KILLS %d BATTLES %d",nb[k].kills>99?99:nb[k].kills,nb[k].battles>9?9:nb[k].battles); uiText(142,y,b,U_GOLD,1); y+=10;
            u16 fl=crFl[inspIdx]; b[0]=0; if(fl&FL_HERO) strcat(b,"HERO "); if(fl&FL_FOUND) strcat(b,"FOUNDER "); if(fl&FL_VET) strcat(b,"VET "); if(fl&FL_SLAYER) strcat(b,"SLAYER "); if(fl&FL_SAPIENT) strcat(b,"SAPIENT "); if((fl&FL_HERO)&&isFighter(c)&&crCaptain(inspIdx)) strcat(b,"CAPTAIN "); if(fl&FL_DESERT) strcat(b,"OUTLAW "); if(fl&FL_SCAR) strcat(b,"SCARRED "); if(fl&FL_GRIEF) strcat(b,"GRIEVING"); if(b[0]&&y<=128){ b[18]=0; uiText(142,y,b,U_ORANGE,1); y+=10; } } }
        if(DISEASE(c)==3){ uiText(142,y,"INFECTED",U_RED,1); y+=10; } else if(DISEASE(c)==1){ uiText(142,y,"SICK (PLAGUE)",U_GREEN,1); y+=10; } else if(DISEASE(c)==2){ uiText(142,y,"IMMUNE NOW",U_CYAN,1); y+=10; }
        for(int q=0;q<6&&y<=128;q++) if(HAS_STATUS(c,statusMask[q])){ uiText(142,y,statusName[q],q==0?U_GOLD:U_ORANGE,1); y+=10; }
        if(c->wep){ uiText(142,y,it[c->wep-1].nm,itRar(c->wep-1)?U_ORANGE:U_YELLOW,1); y+=10; if(itRar(c->wep-1)&&y<=128){ char tg[40]; itTag(tg,c->wep-1); uiText(142,y,tg,U_DIM,1); y+=10; } } if(c->arm){ uiText(142,y,it[c->arm-1].nm,itRar(c->arm-1)?U_ORANGE:U_CYAN,1); y+=10; }
        if(c->tr){ uiText(142,y,trNm[c->tr],U_PURPLE,1); y+=10; uiWrap(142,y,18,2,trDesc[c->tr],U_DIM); y+=18; } if(c->tr2){ uiText(142,y,trNm[c->tr2],U_PURPLE,1); y+=10; } }
    else if(inspKind==2&&inspIdx>=0&&inspIdx<VIL_N&&vil[inspIdx].on){ Vil*V=&vil[inspIdx]; int v=inspIdx; char vn[24]; vilName(v,vn);
        uiFrame(142,19,38,38,U_SHADOW,U_BHI,U_BLO); uiBig(145,22,IC8_HOME,4); uiText(184,20,vn,U_WHITE,1); uiText(184,30,arch[V->race].nm,U_DIM,1); { int mo=vilMood(v); uiText(184,50,moodName(mo),mo>=20?U_GREEN:mo<=-20?U_RED:U_LGREY,1); } { static const char* const tier[4]={"","HAMLET","TOWN","CITY"}; int k=vilK[v]-1; uiText(184,40,(k>=0&&kd[k].cap==v+1)?"CAPITAL":tier[V->lvl>3?3:V->lvl],(k>=0&&kd[k].cap==v+1)?U_GOLD:U_LGREY,1); }
        y=62; if(vilK[v]){ int k=vilK[v]-1; char kn[24]; kdName(k,kn); uiRect(142,y,8,8,40+k); uiText(153,y,kn,U_WHITE,1); y+=11; uiText(142,y,"LOYALTY",U_DIM,1); int lo=vilL[v]<0?0:vilL[v]; uiBar(142,y+9,108,7,lo,100,vilL[v]<20?U_RED:U_GREEN); y+=19; }
        uiText(142,y,"STORE",U_DIM,1); uiBar(142,y+9,108,7,V->stock,400,U_YELLOW); y+=19;
        siprintf(b,"POP %d  HOMES %d",V->pop,V->houses); uiText(142,y,b,U_WHITE,1); y+=10; siprintf(b,"FARMS %d  TOWERS %d",V->farms,V->towers); uiText(142,y,b,U_WHITE,1); y+=10;
        if(V->foe&&vil[V->foe-1].on){ char fn[24]; vilName(V->foe-1,fn); siprintf(b,"AT WAR: %s",fn); uiText(142,y,b,U_RED,1); } }
    else { uiText(142,24,"NOTHING HERE",U_DIM,1); }
#ifdef GG_THEATRE
    // The 3DS can enter a first-person world view through the exact
    // creature inspected on this card. Never advertise the mode for
    // animals without sentience, mindless folk or infected zombies.
    int canPOV=inspKind==1&&inspIdx>=0&&inspIdx<nCr&&
        cr[inspIdx].alive&&
        (arch[sp[cr[inspIdx].sp].a].diet==4||
         (crFl[inspIdx]&FL_SAPIENT))&&
        !HAS_STATUS(&cr[inspIdx],ST_MINDLESS)&&
        DISEASE(&cr[inspIdx])!=3;
    uiText(142,136,canPOV?"SEL+R: 3D POV":"A: INSPECT AGAIN",
           canPOV?U_GOLD:U_DIM,1);
#else
    uiText(142,136,"A: INSPECT AGAIN",U_DIM,1);
#endif
}
#ifdef GG_THEATRE
// ---- the 3DS "theatre": while the globe is up on the touch screen, the top screen shows the same planet big (a ball 176 pixels across) over a deep starfield, and gives every pixel of it a depth
// (the ball is a dome, its atmosphere a thin shell, the stars far behind), so with the 3D slider up the planet really is a ball in front of the screen (compat/stereo.h). It turns with the globe.
static void uiTheatre(void){
    const int R=88; globeTables(R); int cL,sL,cP,sP; globeAim(rotY,rotX,&cL,&sL,&cP,&sP);
    uiRect(0,0,256,192,U_BG); for(int k=0;k<90;k++){ int x=(int)(hsh(k,3,9)%256), y=(int)(hsh(k,4,9)%192); uiPx(x,y,((k+(tick>>5))&3)?U_DIM:U_WHITE); uiZSet(x,y,-6+(k%3)); }
    int drift=(tick>>6);
    for(int y=-R-3;y<=R+3;y++) for(int x=-R-3;x<=R+3;x++){ int d2=x*x+y*y, sx=128+x, sy=96-y; if(sx<0||sx>=256||sy<0||sy>=192) continue;
        if(d2>R*R){ if(d2<=(R+3)*(R+3)){ int lam=(-x*5-y*5+R*3)>0; uiPx(sx,sy,lam?U_CYAN:U_BLUE); uiZSet(sx,sy,2); } continue; }
        int nx=gxn[x+R], ny=gxn[y+R], nz=gnz[d2]; int tx,ty; globeDir(nx,ny,nz,cL,sL,cP,sP,&tx,&ty);
        int lam=(-nx*7+ny*7+nz*12)/26; int j=lam<2500?0:lam<9000?1:2; int terr=world[ty][tx]; int col=100+terr*3+j; { int bb=bioAt(tx,ty); if(bb&&terr>T_WATER&&terr!=T_LAVA) col=85+bb; }
        if(bld[ty][tx]&&bld[ty][tx]!=B_RUIN&&lam>-2000) col=lam<9000?U_ORANGE:U_YELLOW;
        if(gClouds){ int cv=gCloud[(ty*64/WH)&63][((tx*64/WW)+drift)&63]; int thr=(weather==WX_RAIN||weather==WX_STORM||weather==WX_ASH||weather==WX_RAGE)?105:weather==WX_SNOW?125:weather==WX_MAGIC?95:weather==WX_HEAT?180:165; if(cv>thr) col=U_WHITE; }
        if(nz<3000&&lam>-3000) col=U_CYAN;
        uiPx(sx,sy,col); uiZSet(sx,sy,col==U_WHITE?nz*10/16384+4:nz*10/16384+1); }
    { char b[40]; siprintf(b,"YEAR %d  %d KINGDOMS",year,(int)(kd[0].on+kd[1].on+kd[2].on+kd[3].on+kd[4].on+kd[5].on+kd[6].on+kd[7].on)); uiTextC(128,182,b,U_GOLD,1); uiZSet(128,182,0); } }
#endif
static const char* starterHint(void){
    int kind=pw[power].kind;
    if(kind==PK_BUILD)return "HOLD TO AIM, LIFT TO BUILD";
    if(storyOn&&chapter==0&&stUse<2&&kind==PK_PAINT)
        return "TOUCH THE MAP TO SHAPE THE GROUND";
    if(storyOn&&(chapter==1||chapter==2)){
        if(kind==PK_SETTLE) return "TAP OPEN GRASS TO FOUND A VILLAGE";
        if(kind==PK_INSPECT) return "TAP A HOME TO SEE HOW IT'S DOING";
    }
    if(!storyOn&&stUse<3&&kind==PK_PAINT) return "PAINT LAND, THEN PICK SPAWN OR SETTLE";
    return 0;
}
static void uiStatus(void){
#ifdef GG_THEATRE
    if(view3d){ uiTheatre(); uiFlush(); return; }
#endif
    static int aliveC=0; static unsigned aliveT=~0u; if(aliveT!=(unsigned)(tick>>3)){ aliveT=(unsigned)(tick>>3); aliveC=0; for(int i=0;i<nCr;i++) aliveC+=cr[i].alive; } int alive=aliveC; int hh; int folk=vilTotals(&hh); char b[64];
    siprintf(b,"%s  YEAR %d",storyOn?"STORY":"FREE PLAY",year); uiWin(view3d?(walkOn?"BORE: GODGUY  WALKING":"BORE: GODGUY  GLOBE"):"BORE: GODGUY",b,0);
    { int p=(tick>>1)&2047; uiIcon8(238-uiTextW(b,1)-10,3,(p<256||p>1792)?IC8_MOON:IC8_SUN); if(paused) uiText(82,3,"PAUSED",U_RED,1); }
    uiFrame(2,15,132,132,U_SHADOW,U_BHI,U_BLO); uiMinimap(4,17);
    { int ids[KN_N], m=0; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[m++]=k; if(m>12) m=12; if(m&&!view3d){ int rows=(m+1)/2, bh=rows*8+3, y0=17+128-bh-1; uiRect(5,y0,126,bh,U_SHADOW); uiRect(5,y0,126,1,U_BHI);   // the legend: kingdom colours, names, and a red mark for the ones at war
        for(int i=0;i<m;i++){ int k=ids[i], x=7+(i&1)*62, y=y0+2+(i>>1)*8; char nm[24]; kdName(k,nm); nm[6]=0; uiRect(x,y+1,5,5,mmRingCol(k)); if(mmMode&&mmMode<4){ siprintf(nm,"%.6s",mmMode==1?culN[kd[k].cul]:mmMode==2?rlgN[kd[k].rlg]:langN[kd[k].lang]); } uiText(x+7,y,nm,kd[k].empire?U_GOLD:U_LGREY,1); if(kd[k].war) uiText(x+48,y,"!",U_RED,1); } } }
    if(inspT>0){ uiInspectCard(); } else {
    uiFrame(138,15,116,86,U_PANEL,U_BHI,U_BLO);
    { struct { int ic; const char*l; int v; } r[6]={{IC8_LIFE,"LIFE",alive},{IC8_FOLK,"FOLK",folk},{IC8_HOME,"HOMES",hh},{IC8_FLAG,"VILLAGES",nVilOn},{IC8_SKULL,"MONSTERS",monCountHud()},{IC8_STAR,"SLAIN",(int)stKill}};
      for(int i=0;i<6;i++){ int y=19+i*13; uiIcon8(143,y,r[i].ic); uiText(155,y+1,r[i].l,U_DIM,1); siprintf(b,"%d",r[i].v); uiTextR(250,y+1,b,U_WHITE); } }
    uiFrame(138,104,116,44,U_PANEL,U_BHI,U_BLO);
    if(pw[power].kind==PK_SPAWN||pw[power].kind==PK_GIFT||pw[power].kind==PK_MUTATE||pw[power].kind==PK_STATUS||pw[power].kind==PK_FIELD||pw[power].kind==PK_BUILD){   // a picker: the choice before, the choice now, the choice after
        uiText(143,107,pw[power].kind==PK_BUILD?"X Y  |  B+X Y TURN":"X Y: CHOOSE",U_DIM,1); for(int k=-1;k<=1;k++){ char nm[40]; int ok=1;
            if(pw[power].kind==PK_SPAWN){ spName((spSel+nSp+k)%nSp,nm); } else if(pw[power].kind==PK_GIFT){ int j=k==0?itSel:itNext(itSel,k); itSay(nm,j); } else if(pw[power].kind==PK_STATUS){ int q=(statusSel+6+k)%6; strcpy(nm,statusName[q]); } else if(pw[power].kind==PK_FIELD){ strcpy(nm,fieldName[(fieldMode+FIELD_N+k)%FIELD_N]); } else if(pw[power].kind==PK_BUILD){ strcpy(nm,constructionName((constructionMode+GG_CONSTRUCTION_N+k)%GG_CONSTRUCTION_N)); } else { int t=(trSel+TR_N+k)%TR_N; strcpy(nm,t?trNm[t]:"(CLEAR)"); (void)ok; }
            nm[17]=0; if(k==0) uiRect(140,116+(k+1)*10,112,10,U_SEL); uiText(143,118+(k+1)*10,nm,k==0?U_WHITE:U_DIM,1); } }
    else if(storyOn&&chapter<CHAP_N){
        const Chap*C=&chap[chapter];
        siprintf(b,"CHAPTER %d",chapter+1);
        uiText(143,108,b,U_GOLD,1);
        if(C->kind==OB_FINAL){
            siprintf(b,"SLAIN %u/%d",stKill>STORY_FINAL_SLAIN?(unsigned)STORY_FINAL_SLAIN:(unsigned)stKill,STORY_FINAL_SLAIN);
            uiText(143,120,b,stKill>=STORY_FINAL_SLAIN?U_GREEN:U_WHITE,1);
            siprintf(b,"YEAR %d/%d",year>STORY_FINAL_YEAR?STORY_FINAL_YEAR:year,STORY_FINAL_YEAR);
            uiText(143,134,b,year>=STORY_FINAL_YEAR?U_GREEN:U_WHITE,1);
        }else{
            int v=chapVal(C->kind);
            if(v>C->target) v=C->target;
            uiWrap(143,118,18,2,C->goal,U_WHITE);
            uiBar(143,137,106,9,v,C->target,U_GREEN);
            siprintf(b,"%d/%d",v,C->target);
            uiTextC(196,138,b,U_WHITE,1);
        }
    }
    else if(storyOn){ uiText(143,108,"STORY TOLD",U_GOLD,1); uiText(143,120,"YOU ARE A GOD",U_WHITE,1); }
    else { int o; int tot=itCount(&o); uiText(143,108,"FREE PLAY",U_GOLD,1); siprintf(b,"ITEMS %d/%d",o,tot); uiText(143,120,b,U_WHITE,1); siprintf(b,"MET %d/%d",seenN,MON_N); uiText(143,132,b,U_DIM,1); }
    }
    uiFrame(2,150,252,28,U_PANEL,U_BHI,U_BLO); uiRect(5,153,14,14,U_SHADOW); uiPowerIcon(power,7,155);
    { const char*hint=starterHint(); char nm[40]; if(pw[power].kind==PK_SPAWN){ spName(spSel,nm); siprintf(b,"%s",nm); } else if(pw[power].kind==PK_GIFT){ itSay(nm,itSel); siprintf(b,"%.28s",nm); } else if(pw[power].kind==PK_MUTATE) siprintf(b,"TRAIT: %s",trSel?trNm[trSel]:"CLEAR"); else if(pw[power].kind==PK_STATUS) siprintf(b,"STATUS: %.20s",statusName[statusSel]); else if(pw[power].kind==PK_FIELD){ if(fieldMode>=FIELD_CLINIC) siprintf(b,"%s  R%d",fieldName[fieldMode],fieldRadius); else siprintf(b,"%s",fieldName[fieldMode]); } else if(pw[power].kind==PK_BUILD) siprintf(b,"%s %s",constructionName(constructionMode),constructionFacing(constructionTypes[constructionMode])); else siprintf(b,"BRUSH %d",brush);
      uiText(24,154,pw[power].nm,U_GOLD,1); uiText(24+uiTextW(pw[power].nm,1)+6,154,b,U_WHITE,1);
      if(storyGoalT>0&&storyOn&&chapter<CHAP_N) uiText(24,166,chap[chapter].goal,U_GOLD,1);
      else if(toastT>0&&info[0]) uiText(24,166,info,U_YELLOW,1); else uiText(24,166,hint?hint:pw[power].kind==PK_FIELD?fieldDesc[fieldMode]:pwDesc[power],U_DIM,1); }
    uiIcon8(4,182,IC8_NOTE); { char mn[24]=""; if(gmCur>=0){ strncpy(mn,gmSongs[gmCur].name,19); mn[19]=0; } uiText(14,183,mn,U_DIM,1); } { siprintf(b,"%s %s",ageN[worldAge()],weatherName(weather)); uiTextR(250,183,b,U_DIM); }
    uiFlush(); }
static int achSel, kingSel, jrSel;
static void uiAch(void){
    int n=0; for(int i=0;i<ACH_N;i++) n+=ACH_HAS(i); char b[40]; siprintf(b,"%d/%d",n,ACH_N); uiWin("ACHIEVEMENTS",b,"UP DOWN SCROLL    START: BACK");
    int first=achSel<4?0:achSel-4; if(first>ACH_N-9) first=ACH_N-9; if(first<0) first=0;
    for(int r=0;r<9;r++){ int i=first+r; if(i>=ACH_N) break; int y=16+r*18, got=ACH_HAS(i); uiBtn(4,y,232,17,0,0,&achSel,i); if(i==achSel) uiRect(4,y,242,17,U_SEL); else if(r&1) uiRect(4,y,242,17,U_PANEL);
        uiIcon8(8,y+4,got?IC8_STAR:IC8_SKULL); uiText(22,y+1,got?ach[i].nm:"???",got?U_GOLD:U_DIM,1); uiText(22,y+10,ach[i].ds,got?U_WHITE:U_LGREY,1); }
    uiScroll(ACH_N,first,9,16,162); uiFlush(); }
static void uiJournal(void){
    char b[16]; siprintf(b,"%d EVENTS",jrN); uiWin("JOURNAL",b,"UP DOWN SCROLL    START: BACK");
    if(!jrN) uiText(10,30,"NOTHING HAS HAPPENED YET.",U_DIM,1);
    for(int r=0;r<14;r++){ int i=jrN-1-jrSel-r; if(i<0) break; int y=17+r*11; if(r&1) uiRect(4,y-2,240,11,U_PANEL); siprintf(b,"Y%d",jrYear[i]); uiText(8,y,b,U_GOLD,1); { const char*j=jr[i]; int jc=strstr(j,"WAR")||strstr(j,"SLAYS")||strstr(j,"FLEES")||strstr(j,"REBELS")?U_RED:strstr(j,"PEACE")||strstr(j,"ALLY")||strstr(j,"ALLIES")||strstr(j,"RISES")?U_GREEN:strstr(j,"CROWNS")||strstr(j,"TAKES")?U_YELLOW:strstr(j,"FALLS")||strstr(j,"GHOST")?U_ORANGE:strstr(j,"ACHIEVED")||strstr(j,"MONOLITH")||strstr(j,"UPLIFT")?U_CYAN:strstr(j,"LEARNS")||strstr(j,"REBUILDS")||strstr(j,"TURNS")?U_LGREY:U_WHITE; uiText(40,y,j,jc,1); } }
    uiFlush(); }
static int cmpOn, cmpB=1;
static void uiCompare(int a,int b,int y0){      // two kingdoms side by side (WORLD > KINGS, X); LEFT RIGHT picks the second one
    uiFrame(4,y0,248,177-y0,U_PANELHI,U_GOLD,U_BLO); char na[24], nb2[24], s[40]; kdName(a,na); kdName(b,nb2); int ca,pa,wa,cb,pb,wb; kdStats(a,&ca,&pa,&wa); kdStats(b,&cb,&pb,&wb);
    uiRect(8,y0+3,6,6,40+a); uiText(17,y0+2,na,U_WHITE,1); uiRect(132,y0+3,6,6,40+b); uiText(141,y0+2,nb2,U_WHITE,1);
    struct { const char*l; int x, y; } r[8]={{"CITIES",ca,cb},{"PEOPLE",pa,pb},{"POWER",wa,wb},{"GOLD",kd[a].gold,kd[b].gold},{"TECH",kd[a].tech,kd[b].tech},{"RENOWN",kx[a].renown,kx[b].renown},{"STEW",kdRStat(a,0),kdRStat(b,0)},{"WARFARE",kdRStat(a,1),kdRStat(b,1)}};
    int rows=(177-y0-14)/9; if(rows>8) rows=8; for(int i=0;i<rows;i++){ int y=y0+12+i*9; uiText(8,y,r[i].l,U_DIM,1); siprintf(s,"%d",r[i].x); uiTextR(122,y,s,r[i].x>r[i].y?U_GREEN:r[i].x<r[i].y?U_LGREY:U_WHITE); siprintf(s,"%d",r[i].y); uiTextR(246,y,s,r[i].y>r[i].x?U_GREEN:r[i].y<r[i].x?U_LGREY:U_WHITE); }
    siprintf(s,"OPINION %d / %d",kd[a].rel[b],kd[b].rel[a]); if(rows<8||y0<170){} }
static void uiKing(void){
    int n=0; for(int k=0;k<KN_N;k++) n+=kd[k].on; char b[64]; siprintf(b,"%d KINGDOMS",n); uiWin("KINGDOMS","",cmpOn?"LEFT RIGHT: SECOND   X: BACK":"UP DOWN CHOOSE   X: COMPARE");
    uiTextR(246,4,"POWER",U_GOLD); uiTextR(204,4,"PEOPLE",U_GOLD); uiTextR(160,4,"CITIES",U_GOLD);      // column labels in the title bar, so the numbers below are plain
    if(!n){ uiText(10,30,"NO KINGDOMS YET. FOUND VILLAGES",U_DIM,1); uiText(10,40,"AND LET THEM GROW.",U_DIM,1); uiFlush(); return; }
    int ids[KN_N], m=0; for(int k=0;k<KN_N;k++) if(kd[k].on) ids[m++]=k; if(kingSel>=m) kingSel=m-1; if(kingSel<0) kingSel=0;
    int vis=m<8?m:8, top=kingSel-3; if(top>m-vis) top=m-vis; if(top<0) top=0;      // up to sixteen kingdoms: the list shows eight and scrolls
    for(int r=top;r<top+vis;r++){ int k=ids[r], c,p,w; kdStats(k,&c,&p,&w); char nm[24]; kdName(k,nm); int y=16+(r-top)*11; uiBtn(4,y-1,232,11,0,0,&kingSel,r); if(r==kingSel) uiRect(4,y-1,246,11,U_SEL); uiRect(7,y+1,6,6,40+k); uiText(17,y,nm,r==kingSel?U_WHITE:U_LGREY,1);
        uiText(70,y,kd[k].empire?"EMPIRE":kd[k].lord?"VASSAL":"KINGDOM",kd[k].empire?U_GOLD:U_DIM,1); { int rc=kd[k].war?U_RED:U_WHITE; siprintf(b,"%d",c); uiTextR(160,y,b,rc); siprintf(b,"%d",p); uiTextR(204,y,b,rc); siprintf(b,"%d",w); uiTextR(246,y,b,rc); } }
    int k=ids[kingSel], c,p,w; relK=k; kdStats(k,&c,&p,&w); char nm[24], rn[40]; kdName(k,nm); kdRuler(k,rn); int y0=16+vis*11+4;
    if(cmpOn){ int o=ids[cmpB%m]; if(o==k) o=ids[(cmpB+1)%m]; if(m>1){ uiCompare(k,o,y0); uiFlush(); return; } cmpOn=0; }
    if(cmpOn){ int o=ids[cmpB%m]; if(o==k) o=ids[(cmpB+1)%m]; if(m>1){ uiCompare(k,o,y0); uiFlush(); return; } cmpOn=0; }
    uiFrame(4,y0,248,177-y0,U_PANELHI,40+k,U_BLO);
    int barLine=-1; char L[18][64]; int col[18], pri[18], nl=0; char cp[24]="NONE"; if(kd[k].cap) vilName(kd[k].cap-1,cp);
    siprintf(L[nl],"%s %s",kd[k].empire?"EMPEROR":"RULER",rn); col[nl]=U_WHITE; pri[nl++]=0;
    siprintf(L[nl],"CITIES %d/%d  CAPITAL %s",c,kdMaxCities(k),cp); col[nl]=c>kdMaxCities(k)?U_ORANGE:U_WHITE; pri[nl++]=1;
    barLine=nl; L[nl][0]=0; col[nl]=U_LGREY; pri[nl++]=1;      // the four ruler stats are drawn as bars on this line
    { char s2[64]="WAR: "; int any=0; for(int o=0;o<KN_N;o++) if(kd[k].war>>o&1){ char z[24]; kdName(o,z); if(strlen(s2)<30){ strcat(s2,z); strcat(s2," "); any=1; } } if(!any) strcpy(s2,kd[k].plot?"PLOTTING WAR":"AT PEACE"); strcpy(L[nl],s2); col[nl]=any?U_RED:kd[k].plot?U_ORANGE:U_GREEN; pri[nl++]=2; }
    { char s2[64]="ALLIES: "; int any=0; for(int o=0;o<KN_N;o++) if(kd[k].allies>>o&1){ char z[24]; kdName(o,z); if(strlen(s2)<26){ strcat(s2,z); strcat(s2," "); any=1; } } if(kd[k].lord){ char z[24]; kdName(kd[k].lord-1,z); siprintf(s2,"VASSAL OF %s",z); any=1; } if(!any) strcpy(s2,"NO ALLIES"); strcpy(L[nl],s2); col[nl]=U_CYAN; pri[nl++]=3; }
    siprintf(L[nl],"%s  %s  %s",arch[kd[k].race].nm,culN[kd[k].cul],rlgN[kd[k].rlg]); col[nl]=U_DIM; pri[nl++]=4;
    { int pp[8]={0}, tot=0; for(int v=0;v<VIL_N;v++) if(vil[v].on&&vilK[v]==k+1){ pp[vil[v].race&7]+=vil[v].pop; tot+=vil[v].pop; }      // who lives in it: the people of its cities, counted now
      char t2[48]="FOLK "; for(int q=0;q<2;q++){ int bi=-1; for(int r=0;r<8;r++) if(pp[r]>0&&(bi<0||pp[r]>pp[bi])) bi=r; if(bi<0) break; char z[20]; siprintf(z,"%s %d%% ",arch[bi].nm,pp[bi]*100/(tot?tot:1)); if(strlen(t2)+strlen(z)<34) strcat(t2,z); pp[bi]=0; }
      if(tot){ strcpy(L[nl],t2); col[nl]=U_LGREY; pri[nl++]=3; } }
    siprintf(L[nl],"HOUSE %s",clanN[kd[k].clan]); col[nl]=U_DIM; pri[nl++]=6;
    siprintf(L[nl],"TECH %d %s  %s",kd[k].tech,techN[kd[k].tech],langN[kd[k].lang]); col[nl]=U_DIM; pri[nl++]=5;
    { char t2[48]=""; for(int q=0;q<6;q++) if(kd[k].ctr>>q&1){ if(strlen(t2)+strlen(ctrN[q])<34){ strcat(t2,ctrN[q]); strcat(t2," "); } } if(t2[0]){ strcpy(L[nl],t2); col[nl]=U_PURPLE; pri[nl++]=7; } }
    { static const char*const pn[3]={"NORMAL","HIGH","LOW"}; char t2[56]; siprintf(t2,"TRIBUTE %s TAX %s",pn[kx[k].tribute%3],pn[kx[k].tax%3]); strcpy(L[nl],t2); col[nl]=U_LGREY; pri[nl++]=6; }
    { char t2[48]=""; for(int q=0;q<5;q++) if(kx[k].ctr2>>q&1){ if(strlen(t2)+strlen(ctr2N[q])<34){ strcat(t2,ctr2N[q]); strcat(t2," "); } } if(t2[0]){ strcpy(L[nl],t2); col[nl]=U_PURPLE; pri[nl++]=7; } }
    { char t2[56]="LANG "; int any=0; for(int q=0;q<3;q++) if(kx[k].ctr2>>(5+q)&1){ if(strlen(t2)+strlen(langTraitN[q])<52){ strcat(t2,langTraitN[q]); strcat(t2," "); any=1; } } if(any){ strcpy(L[nl],t2); col[nl]=U_CYAN; pri[nl++]=7; } }
    { char t2[56]="CLAN "; int any=0; for(int q=0;q<3;q++) if(kx[k].ctr2>>(8+q)&1){ if(strlen(t2)+strlen(clanTraitN[q])<52){ strcat(t2,clanTraitN[q]); strcat(t2," "); any=1; } } if(any){ strcpy(L[nl],t2); col[nl]=U_GOLD; pri[nl++]=7; } }
    { char t2[56]="FAITH "; int any=0; for(int q=0;q<3;q++) if(kx[k].ctr2>>(11+q)&1){ if(strlen(t2)+strlen(faithTraitN[q])<52){ strcat(t2,faithTraitN[q]); strcat(t2," "); any=1; } } if(any){ strcpy(L[nl],t2); col[nl]=U_PURPLE; pri[nl++]=7; } }
    { char t2[64]="PAST: "; int any=0; for(int q=0;q<2;q++) if(hall[k][q][2]||hall[k][q][0]||hall[k][q][1]){ char p[24], z[40]; langPerson(kd[k].lang,hall[k][q][0],hall[k][q][1],p); siprintf(z,"%.12s %dY ",p,hall[k][q][2]); if(strlen(t2)+strlen(z)<40){ strcat(t2,z); any=1; } } if(any){ strcpy(L[nl],t2); col[nl]=U_DIM; pri[nl++]=9; } }
    siprintf(L[nl],"GOLD %d  POWER %d  RENOWN %d",kd[k].gold,w,kx[k].renown); col[nl]=U_GOLD; pri[nl++]=8;
    { int fit=(177-y0-5)/10; int show[18]; for(int i=0;i<nl;i++) show[i]=1; int cnt=nl; while(cnt>fit&&cnt>1){ int bi=-1; for(int i=0;i<nl;i++) if(show[i]&&(bi<0||pri[i]>pri[bi])) bi=i; show[bi]=0; cnt--; }
      int y=y0+4; for(int i=0;i<nl;i++) if(show[i]){ if(i==barLine){ static const char*const sn="SWDI"; static const u8 sc[4]={U_GOLD,U_RED,U_CYAN,U_GREEN}; for(int q=0;q<4;q++){ int x=10+q*58; char one[2]={sn[q],0}; uiText(x,y,one,U_DIM,1); uiBar(x+8,y+1,42,6,kdRStat(k,q),10,sc[q]); } } else uiText(10,y,L[i],col[i],1); y+=10; } }
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
static void uiPeople(void){     // the population of the world, by people, culture, faith and tongue (the sum over the kingdoms)
    uiWin("POPULATION","","L R: PAGES    START: BACK"); int byRace[8]={0}, byCul[5]={0}, byRlg[4]={0}, byLang[5]={0}, tot=0;
    for(int k=0;k<KN_N;k++) if(kd[k].on){ int c,p,pw; kdStats(k,&c,&p,&pw); tot+=p; if(kd[k].race<8) byRace[kd[k].race]+=p; byCul[kd[k].cul%5]+=p; byRlg[kd[k].rlg%4]+=p; byLang[kd[k].lang%5]+=p; }
    char b[48]; siprintf(b,"%d FOLK UNDER A CROWN",tot); uiText(8,18,b,U_GOLD,1);
    struct { const char*h; int n; const char*const*nm; const int*v; int col; } S[4]={{"BY PEOPLE",8,0,byRace,0},{"BY CULTURE",5,culN,byCul,0},{"BY FAITH",4,rlgN,byRlg,124},{"BY TONGUE",5,langN,byLang,124}}; int yy[2]={30,30};
    for(int s=0;s<4;s++){ int x=4+S[s].col, y=yy[S[s].col?1:0]; uiText(x+2,y,S[s].h,U_CYAN,1); y+=10; int mx=1; for(int i=0;i<S[s].n;i++) if(S[s].v[i]>mx) mx=S[s].v[i];
        for(int i=0;i<S[s].n;i++){ if(!S[s].v[i]&&!S[s].nm) continue; char nm[12]; siprintf(nm,"%.7s",S[s].nm?S[s].nm[i]:arch[i].nm); uiText(x+2,y,nm,U_LGREY,1); int w=S[s].v[i]*50/mx; uiRect(x+46,y+1,w>0?w:(S[s].v[i]?1:0),5,U_GOLD); siprintf(b,"%d",S[s].v[i]); uiTextR(x+118,y,b,U_WHITE); y+=9; }
        yy[S[s].col?1:0]=y+6; }
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
static int crNotable(const Cr*c){ return c->rk*4+(c->tr?2:0)+(c->tr2?1:0)+(c->wep?1:0)+((crFl[c-cr]&FL_NOTE)?4:0)+(crStarred((int)(c-cr))?8:0); }      // (a creature with a record ranks with an elite; a starred one tops the list)
static void brBuild(void){
    brN=0;
    #define BRADD(t,i) do{ if(brN<160){ brL[brN].type=(u8)(t); brL[brN].id=(u16)(i); brN++; } }while(0)
    switch(brCat){
        case 0: for(int v=0;v<VIL_N;v++) if(favVil[v]&&vil[v].on) BRADD(1,v); for(int k=0;k<KN_N;k++) if(favKd[k]&&kd[k].on) BRADD(2,k); for(int i=0;i<nIt;i++) if(favItem[i]) BRADD(3,i); for(int i=0;i<PW_N;i++) if(favPow[i]) BRADD(4,i); for(int k=0;k<CSTAR_N;k++) if(favCrId[k]){ int j=idFind(favCrId[k]); if(j>=0&&cr[j].alive) BRADD(arch[sp[cr[j].sp].a].diet==5?6:5,j); } break;
        case 1: for(int k=0;k<KN_N;k++) if(kd[k].on) BRADD(2,k); break;
        case 2: { int used[VIL_N]; memset(used,0,sizeof used); for(int n=0;n<VIL_N;n++){ int bp=-1,bv=-1; for(int v=0;v<VIL_N;v++) if(vil[v].on&&!used[v]&&vil[v].pop>bp){ bp=vil[v].pop; bv=v; } if(bv<0) break; used[bv]=1; BRADD(1,bv); } } break;
        case 3: case 4: { int want=brCat==3?4:5; for(int pass=3;pass>=0&&brN<150;pass--) for(int i=0;i<nCr&&brN<150;i++){ const Cr*c=&cr[i]; if(!c->alive||arch[sp[c->sp].a].diet!=want) continue; int nt=crNotable(c); int bucket=nt>=8?3:nt>=4?2:nt>=1?1:0; if(bucket!=pass) continue; if(pass==0&&brN>40) continue; BRADD(brCat==3?5:6,i); } } break;
        case 5: for(int i=0;i<nIt&&brN<150;i++) if(itOpen(i)&&it[i].kind==brKind) BRADD(3,i); break;
        default: for(int i=0;i<PW_N;i++) BRADD(4,i); break; }
    if(brSel>=brN) brSel=brN?brN-1:0; }
static u8*brFav(const BrE*e){ switch(e->type){ case 1: return &favVil[e->id]; case 2: return &favKd[e->id]; case 3: return &favItem[e->id]; case 4: return &favPow[e->id]; default: return 0; } }
static void brText(const BrE*e,char*l,char*r){ l[0]=r[0]=0; switch(e->type){
    case 1: { vilName(e->id,l); int k=vilK[e->id]-1; char kn[24]=""; if(k>=0) kdName(k,kn); siprintf(r,"%s %dP",kn,vil[e->id].pop); } break;
    case 2: { kdName(e->id,l); int c,p,w; kdStats(e->id,&c,&p,&w); siprintf(r,"%d CITIES  %d PEOPLE",c,p); } break;
    case 3: itSay(l,e->id); l[26]=0; break; case 4: strcpy(l,pw[e->id].nm); break;
    default: { const Cr*c=&cr[e->id]; crName(c,l); l[24]=0; int k=(crFl[e->id]&FL_NOTE)?nbFind(crId[e->id]):-1; if(k>=0) siprintf(r,"%dK %dB",nb[k].kills>999?999:nb[k].kills,nb[k].battles>99?99:nb[k].battles); else siprintf(r,"HP %d",c->hp); } } }
static struct { int cx, cy, cat, sel, z; } brHist[8]; static int brHn;      // where the camera was before each jump from the browser (the SELECT key goes back)
static void brGo(const BrE*e){   // jump to it
    for(int k=7;k>0;k--) brHist[k]=brHist[k-1]; brHist[0].cx=camX; brHist[0].cy=camY; brHist[0].cat=brCat; brHist[0].sel=brSel; brHist[0].z=zl; if(brHn<8) brHn++;
    int x=-1,y=-1; switch(e->type){ case 1: x=vil[e->id].x; y=vil[e->id].y; inspKind=2; inspIdx=e->id; inspT=480; break;
        case 2: { int cp=kd[e->id].cap?kd[e->id].cap-1:-1; if(cp>=0){ x=vil[cp].x; y=vil[cp].y; inspKind=2; inspIdx=cp; inspT=480; } } break;
        case 3: itSel=e->id; for(int i=0;i<PW_N;i++) if(pw[i].kind==PK_GIFT) power=i; break; case 4: power=e->id; break;
        default: if(e->id<nCr&&cr[e->id].alive){ x=cr[e->id].x; y=cr[e->id].y; inspKind=1; inspIdx=e->id; inspT=480; } }
    if(x>=0){ camX=x*8-128; camY=y*8-96; if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192; winX=-99; }
    fieldLedgerOpen=0; extrasView=0; page=0; if(!titleOn){ menuOn=0; paused=dlgPaused; } }
static void uiBrowser(void){
    brBuild(); uiWin("WORLD BROWSER","",brHn?"SELECT: BACK TO WHERE YOU WERE":"");
    for(int c=0;c<7;c++){ int x=4+c*36, sel=c==brCat; uiRound(x,15,35,13,sel?U_SEL:U_PANELHI,sel?U_GOLD:U_BHI); uiTextC(x+17,18,brCatN[c],sel?U_WHITE:U_DIM,1); uiBtn(x,15,35,13,0,0,&brCat,c); }
    if(brCat==5){ static const char* const kn[8]={"WEAPONS","ARMOUR","POTIONS","SEEDS","FOOD","RELICS","GEMS","TOOLS"}; char b[32]; siprintf(b,"< %s >  Y: NEXT KIND",kn[brKind]); uiText(8,32,b,U_CYAN,1); uiBtn(4,30,160,11,KEY_Y,0,0,0); }
    else { char b[40]; siprintf(b,"%d %s",brN,brCat==0?"STARRED":"LISTED"); uiText(8,32,b,U_DIM,1); }
    int first=brSel<5?0:brSel-4; if(first>brN-9) first=brN-9; if(first<0) first=0;
    if(!brN) uiText(10,60,brCat==0?"STAR THINGS WITH X TO KEEP THEM HERE":"NOTHING TO SHOW YET",U_DIM,1);
    for(int r=0;r<9;r++){ int i=first+r; if(i>=brN) break; int y=43+r*12; const BrE*e=&brL[i]; char l[40], rr[40]; brText(e,l,rr); uiBtn(4,y-1,232,12,0,KEY_A,&brSel,i); if(i==brSel) uiRect(4,y-1,232,12,U_SEL); else if(r&1) uiRect(4,y-1,232,12,U_PANEL);
        static const u8 ty[7]={0,IC8_HOME,IC8_FLAG,IC8_STAR,IC8_SUN,IC8_FOLK,IC8_SKULL}; uiIcon8(7,y,ty[e->type]); uiText(19,y,l,i==brSel?U_WHITE:U_LGREY,1); uiTextR(228,y,rr,U_DIM); const u8*f=brFav(e); if((f&&*f)||(e->type>=5&&crStarred(e->id))) uiIcon8(229,y,IC8_STAR); }
    uiScroll(brN,first,9,43,108);
    uiRound(110,153,40,12,U_SEL,U_GOLD); uiTextC(130,156,"A: GO",U_WHITE,1); uiBtn(108,151,44,16,KEY_A,0,0,0); uiRound(158,153,52,12,U_SEL,U_GOLD); uiTextC(184,156,"X: STAR",U_YELLOW,1); uiBtn(156,151,56,16,KEY_X,0,0,0);
    uiText(6,156,"< > KIND",U_DIM,1);
    uiFlush(); }
static void uiHelp(void){ uiWin("HELP","UP DOWN SCROLL","SELECT NEXT PAGE"); int n=sceneLines(helpTxt); for(int r=0;r<15;r++){ int l=helpSel+r; if(l>=n) break; uiText(10,18+r*10,helpTxt[l],helpTxt[l][0]&&helpTxt[l][0]!=' '&&l<n&&helpTxt[l][1]!=' '&&!(helpTxt[l][0]=='B'&&helpTxt[l][1]==' ')?U_WHITE:U_DIM,1); } uiScroll(n,helpSel,15,16,160); uiFlush(); }
#ifdef __3DS__
// Dedicated archive, not the regular chapter sequence. Unlocks derive from
// existing saved chapter progression; no new save-format field or data block.
static int filmBonusSel;
static int filmBonusOpen(int which){
    return which>=0&&which<FILM_BONUS_N&&
           (unlockAll||debugOn||storyDone||
            (which<FILM_BONUS_N-1&&
             chapter>=filmBonusChapterUnlock[which]));
}
static void uiBonusFilms(void){
    uiWin("FILM ARCHIVE","SIX STORIES TO DISCOVER","A WATCH  B BACK");
    uiText(9,17,"UNLOCK MORE AS CHAPTERS ADVANCE",U_GOLD,1);
    for(int i=0;i<FILM_BONUS_N;i++){
        int y=33+i*23,open=filmBonusOpen(i),selected=i==filmBonusSel;
        uiRect(6,y-3,244,21,selected?U_SEL:(i&1?U_PANEL:U_BG));
        if(selected)uiRect(7,y-2,2,18,U_GOLD);
        // A tap on a row uses the same focus/confirm path as other Extras.
        uiBtn(6,y-3,244,21,KEY_A,0,&filmBonusSel,i);
        uiText(13,y,open?filmBonusTitles[i]:"?? HIDDEN FILM ??",
               selected?U_WHITE:open?U_LGREY:U_DIM,1);
        char state[27];
        if(open)siprintf(state,"READY    %dm %ds",
                        filmReelTicks(&filmBonusReels[i])/3600,
                        (filmReelTicks(&filmBonusReels[i])/60)%60);
        else if(i==FILM_BONUS_N-1)strcpy(state,"AFTER THE FINALE");
        else siprintf(state,"AFTER CHAPTER %d",(int)filmBonusChapterUnlock[i]);
        uiText(18,y+10,state,open?U_GREEN:U_DIM,1);
    }
    uiFlush();
}
#endif
static void uiStory(void){
    char b[64]; uiWin("STORY",storyOn?(storyDone?"FINISHED":"STORY MODE"):"FREE PLAY: PAUSED","SELECT NEXT PAGE");
    if(chapter<CHAP_N){
        const Chap*C=&chap[chapter];
        uiFrame(4,16,248,34,U_PANELHI,U_GOLD,U_BLO);
        siprintf(b,"CHAPTER %d: %s",chapter+1,C->nm);
        uiText(10,20,b,U_GOLD,1);
        uiText(10,30,C->goal,U_WHITE,1);
        if(C->kind==OB_FINAL){
            siprintf(b,"SLAIN %u/%d",stKill>STORY_FINAL_SLAIN?(unsigned)STORY_FINAL_SLAIN:(unsigned)stKill,STORY_FINAL_SLAIN);
            uiText(10,40,b,stKill>=STORY_FINAL_SLAIN?U_GREEN:U_WHITE,1);
            siprintf(b,"YEAR %d/%d",year>STORY_FINAL_YEAR?STORY_FINAL_YEAR:year,STORY_FINAL_YEAR);
            uiText(141,40,b,year>=STORY_FINAL_YEAR?U_GREEN:U_WHITE,1);
        }else{
            int v=chapVal(C->kind);
            if(v>C->target) v=C->target;
            uiBar(10,40,188,7,v,C->target,U_GREEN);
            siprintf(b,"%d/%d",v,C->target);
            uiTextR(246,40,b,U_WHITE);
        }
    }
    else { uiFrame(4,16,248,34,U_PANELHI,U_GOLD,U_BLO); uiText(10,28,"THE STORY IS TOLD. YOU ARE A GOD.",U_GOLD,1); }
    for(int c=0;c<CHAP_N;c++){
        int y=52+c*8, done=c<chapter, active=storyOn&&c==chapter;
        uiRect(4,y-1,248,8,active?U_SEL:(c&1?U_PANEL:U_BG));
        uiRect(8,y,7,7,done?U_GREEN:(active?U_GOLD:U_SHADOW));
        if(done){ uiPx(10,y+3,U_WHITE); uiPx(11,y+4,U_WHITE); uiPx(12,y+2,U_WHITE); }
        siprintf(b,"%2d",c+1);
        uiText(20,y,b,active?U_GOLD:U_DIM,1);
        uiText(36,y,done||active?chap[c].nm:"??????????",active?U_GOLD:(done?U_WHITE:U_DIM),1);
    }
    { int o; int tot=itCount(&o); siprintf(b,"ITEMS %d/%d  MET %d/%d  SLAIN %u",o,tot,seenN,MON_N,(unsigned)stKill); uiText(6,172,b,U_DIM,1); }
    uiFlush(); }
static void uiBest(void){
    char b[64]; siprintf(b,"%d/%d MET",seenN,MON_N); uiWin("BESTIARY",b,"UP DOWN  LEFT RIGHT +10   * = SLAIN"); int first=bestSel-7; if(first<0) first=0; if(first>MON_N-15) first=MON_N-15;
    for(int r=0;r<15;r++){ int m=first+r; int s_=8+(ARCH_A+m/16-8)*16+m%16; int seen=(seenB[s_>>3]>>(s_&7))&1, sl=(slainB[s_>>3]>>(s_&7))&1; char n2[40]; if(seen) monName(m,n2); else strcpy(n2,"??????????"); char nn[8]; siprintf(nn,"%3d",m+1);
        int y=17+r*10; if(m==bestSel) uiRect(4,y-1,240,10,U_SEL); else if(r&1) uiRect(4,y-1,240,10,U_PANEL); uiText(8,y,nn,U_DIM,1); uiText(32,y,n2,seen?U_WHITE:U_DIM,1); if(sl) uiIcon8(228,y,IC8_STAR); else if(seen) uiIcon8(228,y,IC8_SKULL); }
    uiScroll(MON_N,first,15,16,150); uiFlush(); }
static void uiCodex(void){
    countTraits(); char b[64]; siprintf(b,"%d/%d MET  MUT %u",traitMetCount(),TR_N-1,(unsigned)stMut); uiWin("SUBSPECIES",b,"UP DOWN   MUTATE: X Y PICKS A TRAIT");
    int first=trPage-6; if(first<1) first=1; if(first>TR_N-1-11) first=TR_N-1-11; if(first<1) first=1;
    for(int r=0;r<12;r++){ int t=first+r; if(t>=TR_N) break; int met=traitMet(t)||debugOn||unlockAll; int y=17+r*10; if(t==trPage) uiRect(4,y-1,240,10,U_SEL); else if(r&1) uiRect(4,y-1,240,10,U_PANEL); siprintf(b,"%d",trCount[t]); uiText(8,y,met?trNm[t]:"??????????",met?U_WHITE:U_DIM,1); uiTextR(242,y,b,met?U_GOLD:U_DIM); }
    uiFrame(4,142,248,34,U_PANELHI,U_BHI,U_BLO); { int met=traitMet(trPage)||debugOn||unlockAll; uiText(10,146,met?trNm[trPage]:"UNKNOWN",U_GOLD,1); uiWrap(10,157,40,2,met?trDesc[trPage]:"MUTATE A CREATURE TO DISCOVER IT",U_WHITE); }
    uiScroll(TR_N-1,first-1,12,16,120); uiFlush(); }
static void uiAural(void){
    uiWin("AURAL SCULPTURES",0,"PAD TRACK A PLAY B AUTO X Y SFX L R VOL"); char b[64]; siprintf(b,"%d TRACKS",GM_SONGS); uiTextR(250,3,b,U_DIM);
    int first=juke-5; if(first<0) first=0; if(first>GM_SONGS-11) first=GM_SONGS-11;
    for(int r=0;r<11;r++){ int m=first+r; if(m>=GM_SONGS) break; int y=17+r*10; int op=trackOpen(m); char nn[8]; siprintf(nn,"%3d",m+1); if(m==juke) uiRect(4,y-1,240,10,U_SEL); else if(r&1) uiRect(4,y-1,240,10,U_PANEL); uiText(8,y,nn,U_DIM,1);
        { char lk[28]; if(m>=GM_FAM0) famHint(m-GM_FAM0,lk); else strcpy(lk,"?? LOCKED: 100% ??"); uiText(32,y,op?gmSongs[m].name:lk,op?(m>=GM_FAM0?U_CYAN:m>=GM_EPIC0?U_GOLD:m>=GM_REMIX0?U_PINK:U_WHITE):U_DIM,1); } if(gmCur==m) uiIcon8(228,y,IC8_NOTE); }
    uiScroll(GM_SONGS,first,11,16,110);
    uiFrame(4,130,248,48,U_PANELHI,U_BHI,U_BLO);
    uiText(10,134,trackOpen(juke)?gmSongs[juke].use:"FINISH THE GAME 100%",U_DIM,1);
    if(gmCur>=0){ int sec=gmRow*15/gmSongs[gmCur].bpm, tot=gmSongs[gmCur].rows*15/gmSongs[gmCur].bpm; siprintf(b,"NOW  %.26s",gmSongs[gmCur].name); uiText(10,146,b,U_GOLD,1); siprintf(b,"%d:%02d / %d:%02d  %d BPM",sec/60,sec%60,tot/60,tot%60,gmSongs[gmCur].bpm); uiText(10,156,b,U_WHITE,1); uiBar(10,167,150,6,sec,tot>0?tot:1,U_CYAN); }
    siprintf(b,"SFX %d %.9s",fxSel+1,gmFx[fxSel].name); uiText(166,146,b,U_DIM,1); siprintf(b,"VOL %d%%",gmMaster); uiText(166,156,b,U_DIM,1); uiText(166,167,jukeOn?"SCULPTURE":"AUTO",U_DIM,1);
    uiFlush(); }
static void uiDebug(void){
    static const char*const dn[24]={"SIM SPEED","FREEZE SIM","YEAR","CHAPTER","ITEM TIER","UNLOCK ALL ITEMS","MONSTERS ON","MONSTER KIND","SPAWN THE KIND HERE","KILL ALL MONSTERS","MEET ALL MONSTERS","HEAL + FEED ALL","FILL VILLAGE STORES","FOUND VILLAGE HERE","MARK ALL SLAIN","FINISH CHAPTER NOW","SAVE / LOAD / ERASE","PLAY SFX","OPEN AURAL SCULPT.","SET 100% DONE","WORLD SEED","NEW WORLD, THIS SEED","MUTATE 50 CREATURES","DIFFICULTY"};
    uiWin("DEBUG","L R ADJUST  A: DO",0); { int al=0; for(int i=0;i<nCr;i++) al+=cr[i].alive; char b[64]; siprintf(b,"CR %d  MON %d  TICK %d  SEED %04X",al,monCount(),tick,(unsigned)(worldSeed&0xFFFF)); uiText(6,169,b,U_DIM,1); }
    int st0=dbgSel-7; if(st0<0) st0=0; if(st0>24-15) st0=24-15;
    for(int r=st0;r<st0+15;r++){ char v[28]; v[0]=0; switch(r){ case 0: siprintf(v,"x%d",speedMul); break; case 1: siprintf(v,"%s",paused?"FROZEN":"RUNNING"); break; case 2: siprintf(v,"%d",year); break; case 3: siprintf(v,"%d/%d",chapter,CHAP_N); break; case 4: siprintf(v,"%d/14",tierOpen); break; case 5: siprintf(v,"%s",unlockAll?"ON":"OFF"); break;
            case 6: siprintf(v,"%s",monOn?"ON":"OFF"); break; case 7: { char n2[32]; monName(dbgMon,n2); siprintf(v,"%.22s",n2); } break; case 16: siprintf(v,"%s",sdOk?"SD OK":"NO SD"); break; case 17: siprintf(v,"%d %.14s",dbgFx+1,gmFx[dbgFx].name); break; case 18: siprintf(v,"%s",auralOpen?"OPEN":"LOCKED"); break; case 19: siprintf(v,"%s",fullDone?"YES":"NO"); break;
            case 20: siprintf(v,"%04X",(unsigned)((seedFix?seedFix:worldSeed)&0xFFFF)); break; case 23: siprintf(v,"%s",diff==0?"CALM":diff==1?"NORMAL":"BRUTAL"); break; }
        uiListRow(17+(r-st0)*10,r==dbgSel,r-st0,dn[r],v,U_WHITE); }
    uiScroll(24,st0,15,16,150); uiFlush(); }
static void uiDialog(void){
    uiBtn(0,0,256,192,KEY_A,0,0,0);
    const Dl*d=&dlgSet[dlgIdx]; uiRect(0,0,256,192,U_BG); for(int y=0;y<192;y+=2) uiRect(0,y,256,1,U_PANEL);
    uiFrame(8,8,70,70,U_SHADOW,U_GOLD,U_BLO);
    uiGrad(11,11,64,64,G_BG);
    int who=d->who;
    int portrait=who==W_MIRA&&dlgSet==dlgTab[10]?W_N:who;
    int tl=(int)strlen(d->txt), cc=dlgChars<tl?dlgChars:tl;
    int blink=(tick+who*11)%107<5;
    int cel=blink?2:cc<tl&&((tick/5)&1)?1:0;
    fmvPortrait(11,11,portrait,cel);
    int len=(int)strlen(whoNm[who]);
    uiTextS(86,17,whoNm[who],U_GOLD,len>12?1:2);
    uiText(86,37,whoRole[who],U_DIM,1);
    uiRect(86,49,160,1,U_BHI);
    int lines=0;
    while(dlgSet[lines].txt) lines++;
    char count[16];
    siprintf(count,"%d/%d",dlgIdx+1,lines);
    if(chapter<CHAP_N){
        char heading[24];
        siprintf(heading,"CHAPTER %d",chapter+1);
        uiText(86,57,heading,U_CYAN,1);
    }else uiText(86,57,"EPILOGUE",U_CYAN,1);
    uiTextR(244,57,count,U_DIM);
    uiFrame(8,84,240,94,U_PANEL,U_BHI,U_BLO);
    uiWrapReveal(15,93,37,5,d->txt,cc,U_WHITE);
    if(chapter<CHAP_N){
        uiRect(15,139,226,1,U_BHI);
        uiText(15,145,chap[chapter].goal,U_GOLD,1);
    }
    uiRound(114,157,64,16,U_PANELHI,U_BHI);
    uiTextC(146,162,"B SKIP",U_LGREY,1);
    uiBtn(114,157,64,16,KEY_B,0,0,0);
    uiRound(182,157,60,16,U_SEL,U_GOLD);
    uiTextC(212,162,cc<tl?"A REVEAL":dlgSet[dlgIdx+1].txt?"A NEXT":"A CLOSE",U_WHITE,1);
    uiFlush(); }
static void uiMenu(void){
    if(mScr==4){ uiSlots(); return; }
    uiHold=1; uiStatus(); uiHold=0;   // (the status screen is only the backdrop: it is drawn but not sent, or the screen flickered between it and the menu every frame)
    for(int y=0;y<192;y++) for(int x=(y&1);x<256;x+=2) uiPx(x,y,U_SHADOW);
    uiRect(30,12,196,168,U_SHADOW); uiFrame(32,14,192,164,U_PANEL,U_GOLD,U_BLO); uiRect(33,15,190,16,U_PANELHI); uiRect(33,31,190,1,U_BHI);
    uiText(42,19,mScr==0?"PAUSED":mScr==1?"PAUSED  >  OPTIONS":"PAUSED  >  EXTRAS",U_GOLD,1);
    if(mScr==0) uiTextR(214,19,"START: RESUME",U_DIM); else { uiRound(172,17,44,12,U_SEL,U_GOLD); uiTextC(194,20,"BACK",U_WHITE,1); uiBtn(170,16,48,14,KEY_B,0,0,0); }
    uiMenuList(38,35,180,13,9);
    int ids[16]; menuList(ids);
    if(menuAsk){ const char*q=menuAsk==5?"SWITCH MODE?":menuAsk==6?"START A NEW WORLD?":menuAsk==9?"LOAD YOUR SAVED WORLD?":"BACK TO THE TITLE?", *m=menuAsk==5?"THIS STARTS A NEW WORLD.":menuAsk==6?"THIS WORLD WILL BE REPLACED.":menuAsk==9?"UNSAVED PROGRESS IS LOST.":"THE GAME IS SAVED FIRST.";
        uiFrame(36,122,184,54,U_PANEL,U_RED,U_BLO); uiRect(37,123,182,13,U_RED); uiTextC(128,126,q,U_WHITE,1); uiTextC(128,141,m,U_LGREY,1);
        uiRound(44,156,80,15,U_SEL,U_GOLD); uiTextC(84,160,"A  YES",U_WHITE,1); uiBtn(44,156,80,15,KEY_A,0,0,0);
        uiRound(132,156,80,15,U_PANELHI,U_BHI); uiTextC(172,160,"B  NO",U_LGREY,1); uiBtn(132,156,80,15,KEY_B,0,0,0); gfxHint("A: YES    B: NO",U_LGREY); }
    else { gfxDescBox(36,154,184,22,gfxDesc(ids[mSel]));
        if(menuNoteT>0&&info[0]){ char hb[44]; siprintf(hb,"%.40s",info); gfxHint(hb,strncmp(info,"SAVED",5)?U_RED:U_GREEN); } else gfxHint(mScr==0?"A: SELECT    START: RESUME":"A: SELECT    B: BACK",U_DIM); }
    uiFlush(); }
#include "ggmenu.h"
static void slotsInput(int kd){
    int n=NSLOT; if(menuNoteT>0) menuNoteT--;
    if(kd&KEY_UP){ slSel=(slSel+n-1)%n; slAsk=-1; fxReq(FX_MENU_MOVE); } if(kd&KEY_DOWN){ slSel=(slSel+1)%n; slAsk=-1; fxReq(FX_MENU_MOVE); }
    if(kd&KEY_B){ fxReq(FX_MENU_BACK); mScr=0; slAsk=-1; menuSelId(MI_SLOT); return; }
    if(kd&KEY_A){ slotSet(slSel); slotScan(); hasSave=slotYr[slot]>=0; savedYear=-1; slotRemember(); fxReq(FX_MENU_OK); siprintf(info,"SLOT %d",slot+1); mScr=0; slAsk=-1; menuSelId(MI_SLOT); return; }
    if((kd&KEY_X)&&slotYr[slSel]>=0){ if(slAsk==slSel){ slotErase(slSel); slotScan(); if(slSel==slot){ hasSave=0; savedYear=-1; } slAsk=-1; fxReq(FX_MENU_BACK); } else { slAsk=slSel; fxReq(FX_MENU_MOVE); } }
#ifdef NDSHW
    { int d=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0); if(d){ int want=d>0?(hwSlots==1?2:4):(hwSlots==4?2:1); if(want>hwSlots&&!hwLayoutOk(want)) want=hwSlots; if(want!=hwSlots){ if(hwLayoutSet(want)){ slotSet(0); slSel=0; slotScan(); hasSave=slotYr[0]>=0; savedYear=-1; fxReq(FX_MENU_OK); } else { siprintf(info,"SLOT 1 IS TOO BIG FOR THAT"); fxReq(FX_MENU_BACK); } } } }
#endif
}
static int gOnTop=-1;
static void queueUiScreens(void){
    int menuOnBottom=titleOn||menuOn||extrasView||sceneOn||dlgOn;
    if(gOnTop<0){
        if(menuOnBottom) lcdMainOnTop(); else lcdMainOnBottom();
        gOnTop=menuOnBottom;
    } else if(gOnTop!=menuOnBottom){
        uiNextLcd=gOnTop=menuOnBottom;
        uiCut=1;
    }
}
static int uiScrId=-1, uiScrDepth, uiTabDir;
static void hud(void){
    if(sceneClip) return;
    queueUiScreens();
    uiNBtn=0;
    { int id=(dlgOn?1:0)|(sceneOn?2:0)|(titleOn?4:0)|(menuOn?8:0)|(extrasView?16:0)|(mScr<<5)|(page<<8); int dep=(dlgOn||sceneOn)?3:extrasView?2:(titleOn||menuOn)?(mScr>0?1:0):(page?1:0);
      if(uiScrId>=0&&id!=uiScrId){
          uiTrans=uiTabDir?uiTabDir:dep>uiScrDepth?1:dep<uiScrDepth?-1:0;
          if(!uiTrans&&!UI_NOTRANS) uiCut=1;
      } uiScrId=id; uiScrDepth=dep; uiTabDir=0; }
    uiBackHint=fieldLedgerOpen?2:extrasView; if(extrasView&&page!=3&&page!=4&&page!=16){ uiTabPrev=extName(extStep(-1)); uiTabNext=extName(extStep(1)); } else uiTabPrev=uiTabNext=0; uiPageOpen=1|2|4|(auralOK()?8:0)|(debugOn?16:0)|32|64; uiPages=titleOn||sceneOn||dlgOn||extrasView?0:7; uiPageNow=page; uiDotX=124;
    if(dlgOn){ uiDialog(); return; } if(sceneOn){ uiScene(); return; } if(!extrasView){ if(titleOn){ uiTitle(); return; } if(menuOn){ uiMenu(); return; } }
    switch(page){ case 1: uiStory(); break; case 2: uiBest(); break; case 3: uiAural(); break; case 4: uiDebug(); break; case 5: uiCodex(); break; case 6: uiHelp(); break; case 7: uiKing(); break; case 8: uiJournal(); break; case 9: uiAch(); break; case 10: uiCult(); break; case 11: uiLang(); break; case 12: uiBrowser(); break; case 13: uiStats(); break; case 14: uiWorld(); break; case 15: uiPeople(); break;
#ifdef __3DS__
    case 16: uiBonusFilms(); break;
#endif
    default: uiStatus(); }
}

static void menuToWorld(void){ setSize(planetSize); newWorld(); paused=1; camX=(WW*8-256)/2; camY=(WH*8-192)/2; winX=-99; refillWindow(); }
static void menuOpenPage(int pg){ fieldLedgerOpen=0; menuRetScr=mScr; menuRetSel=mSel; extrasView=1; page=pg; if(pg==3) juke=gmCur>=0?gmCur:0; menuOn=0; siprintf(info,"START: BACK TO MENU"); }
static void openFieldLedger(int v){
    int k=vilK[v]-1;
    wTab=wTabPrev=2; wSel=0;
    wCity=v; wPeo=vil[v].race;
    if(k>=0&&k<KN_N&&kd[k].on) wKing=k;
    dlgPaused=paused; paused=1;
    menuOpenPage(14); fieldLedgerOpen=1;
    fxReq(FX_MENU_MOVE);
}
static int closeFieldLedger(int keys){
    if(!fieldLedgerOpen||!(keys&(KEY_START|KEY_B))) return 0;
    fieldLedgerOpen=extrasView=page=0; paused=dlgPaused;
    return 1;
}

static void presentStoryCut(void){
    uiScrId=-1;
    uiCut=1;
    hud();
}
static void advanceStoryFilm(void){
    if(fmvRenderAt(sceneClip,sceneT)){
        queueUiScreens();
        uiCut=1;
        uiFlush();
    } else swiWaitForVBlank();
    sceneT++;
}
static void startFinale(void){
    endShown=1;
    if(view3d) leaveGlobe();
    if(zBmp) zSet(Z_NORMAL);
    sceneNext=credTxt;
    sceneStartFmv(endTxt,&fmvClips[CHAP_N]);
    advanceStoryFilm();
}

static void uiScaleSave(void){
#ifdef HW_CAL
    hwCalStore();
#endif
}
#ifdef HW_CAL
// ---- touch calibration: the stylus comes up as a 12-bit number per axis and an emulator's (or a console's) numbers do not always line up with the picture, worst along the edges.
// The player holds two crosses near opposite corners and every touch after that is converted from those two readings (one straight line per axis).
static int calOn, calPh, calN, calSx, calSy, calCnt, calDirty, calMsg, calPrev, calRx[2], calRy[2], calPx=-1, calPy=-1;
static const int calTx[2]={16,240}, calTy[2]={16,176};
static int calBakOn, calBak[8];   // the calibration in force before a redo, put back if the redo is skipped
static void calStart(void){ calBakOn=hwCalOn; for(int i=0;i<8;i++) calBak[i]=hwCal[i]; calOn=1; calPh=0; calN=0; calSx=calSy=0; calCnt=0; calDirty=1; calMsg=0; calPrev=0; hwCalOn=0; calPx=calPy=-1; pen.held=0; pen.age=-1; uiTrans=0; }
static void calDraw(int down){
    uiGrad(0,0,256,192,G_BG); uiGrad(0,0,256,13,G_HEAD); uiRect(0,12,256,1,U_BHI); uiRect(0,12,44,1,U_GOLD); uiTextS(6,3,"TOUCH CALIBRATION",U_GOLD,1);
    uiGrad(0,179,256,13,G_FOOT); uiRect(0,179,256,1,U_BHI);
    if(calPh<2){ int cx=calTx[calPh], cy=calTy[calPh];
        for(int r=5;r<=9;r+=4) for(int a=0;a<24;a++){ int dx=(int)(r*cosf(a*0.2618f)+0.5f), dy=(int)(r*sinf(a*0.2618f)+0.5f); uiPx(cx+dx,cy+dy,down?U_GREEN:U_GOLD); }
        uiRect(cx-12,cy,25,1,down?U_GREEN:U_GOLD); uiRect(cx,cy-12,1,25,down?U_GREEN:U_GOLD); uiRect(cx-1,cy-1,3,3,U_WHITE);
        uiTextC(128,70,"TAP AND HOLD THE CROSS",U_WHITE,1); uiTextC(128,84,"WITH THE TIP OF THE STYLUS",U_LGREY,1);
        char b[24]; siprintf(b,"POINT %d OF 2",calPh+1); uiTextC(128,104,b,U_GOLD,1);
        if(calMsg==1) uiTextC(128,124,"THAT DID NOT LOOK RIGHT. AGAIN.",U_RED,1); else if(calMsg==2) uiTextC(128,124,"HOLD A LITTLE LONGER",U_ORANGE,1);
        uiText(6,183,"START: SKIP",U_DIM,1); }
    else { uiTextC(128,60,"TOUCH THE SCREEN TO TEST",U_WHITE,1); uiTextC(128,74,"THE GREEN CROSS FOLLOWS THE STYLUS",U_LGREY,1);
        for(int k=0;k<4;k++){ int cx=k&1?240:16, cy=k&2?176:16; uiRect(cx-4,cy-4,9,9,U_DGREY); uiRect(cx-3,cy-3,7,7,U_PANEL); }
        uiTextC(128,100,"A: KEEP IT      B: DO IT AGAIN",U_GOLD,1);
        if(calPx>=0){ uiRect(calPx-9,calPy,19,1,U_GREEN); uiRect(calPx,calPy-9,1,19,U_GREEN); uiPx(calPx,calPy,U_WHITE); }
        uiText(6,183,"A KEEP   B REDO",U_DIM,1); }
    uiFlush(); }
static void calStep(int kd,int kh){
    int down=(kh&KEY_TOUCH)!=0;
    if(calPh<2){
        if(kd&KEY_START){ calOn=0; gOnTop=-1; pen.held=0; pen.age=-1; hwCalOn=calBakOn; for(int i=0;i<8;i++) hwCal[i]=calBak[i]; return; }   // skipped: what was in force before stays
        if(down){ calCnt++; if(calCnt>=2){ calSx+=hwRx; calSy+=hwRy; calN++; } if(calCnt==1) calDirty=1; }
        else { if(calPrev){ if(calN>=2){ calRx[calPh]=calSx/calN; calRy[calPh]=calSy/calN; calPh++; calMsg=0;
                    if(calPh==2){ int dxr=calRx[1]-calRx[0], dyr=calRy[1]-calRy[0];
                        if(dxr<2200||dxr>4800||dyr<2000||dyr>4800){ calPh=0; calMsg=1; }
                        else { hwCal[0]=calRx[0]; hwCal[1]=calRy[0]; hwCal[2]=calTx[0]; hwCal[3]=calTy[0]; hwCal[4]=calRx[1]; hwCal[5]=calRy[1]; hwCal[6]=calTx[1]; hwCal[7]=calTy[1]; hwCalOn=1; } } }
                else calMsg=2;
                calDirty=1; } calSx=calSy=calN=calCnt=0; }
        calPrev=down; }
    else { touchPosition tp; touchRead(&tp); int px=down?tp.px:-1, py=down?tp.py:-1; if(px!=calPx||py!=calPy){ calPx=px; calPy=py; calDirty=1; }
        if(kd&(KEY_A|KEY_START)){ calOn=0; gOnTop=-1; pen.held=0; pen.age=-1; hwCalStore(); }
        else if(kd&KEY_B) calStart(); } }
#endif
static void menuInput(int kd){
    int ids[16]; int n=menuList(ids); int d=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0); if(menuNoteT>0) menuNoteT--;
    if(menuAsk){ if(kd&KEY_B) fxReq(FX_MENU_BACK); else if(kd&KEY_A) fxReq(FX_MENU_OK); if(kd&KEY_A){ int a=menuAsk; menuAsk=0; menuOn=0; if(a==5) startWorld(!storyOn); else if(a==6) startWorld(storyOn); else if(a==9){ if(loadGame(savePath)){ refillWindow(); paused=dlgPaused; siprintf(info,"LOADED"); } else { siprintf(info,"NO SAVE TO LOAD"); menuOn=1; menuNoteT=100; } } else if(a==8){ if(doSave()){ titleOn=1; paused=1; mScr=0; mSel=0; newWorld(); camX=(WW*8-256)/2; camY=(WH*8-192)/2; hasSave=1; musReq(GM_TITLE); } else { menuOn=1; paused=1; menuNoteT=180; fxReq(FX_ERROR); } } } if(kd&(KEY_B|KEY_START)){ menuAsk=0; menuOn=1; } return; }
    if(titleOn&&konami>=4) return;          // the secret code is in progress: the menu holds still
    if(mScr==3){ createInput(kd); return; }
    if(mScr==4){ slotsInput(kd); return; }
    if(!titleOn&&(kd&KEY_START)){ menuOn=0; paused=dlgPaused; return; }
    if(kd&KEY_UP) mSel=(mSel+n-1)%n; if(kd&KEY_DOWN) mSel=(mSel+1)%n; if(kd&(KEY_UP|KEY_DOWN)) fxReq(FX_MENU_MOVE);
    if(kd&KEY_B){ if(mScr||!titleOn) fxReq(FX_MENU_BACK); if(mScr){ int bk=mScr==3?MI_CREATE:mScr==4?MI_SLOT:mScr==2?MI_EXTRA:MI_OPT; mScr=0; menuSelId(bk); } else if(!titleOn){ menuOn=0; paused=dlgPaused; } return; }
    int id=ids[mSel]; if(d&&menuAdj(id)&&id!=MI_SIZE) fxReq(FX_MENU_MOVE); if((kd&KEY_A)&&!menuAdj(id)) fxReq(FX_MENU_OK);
    if(d){ switch(id){ case MI_SIZE: setSize(sizeStep(planetSize,d>0?1:-1)); fxReq(FX_MENU_MOVE); break; case MI_STYPE: storyOn^=1; break;
            case MI_MMODE: mmMode=(mmMode+6+d)%6; break; case MI_MMICON: mmIcons^=1; break; case MI_AUTOSAVE: autoSaveOn^=1; break; case MI_SLOT: slotStep(d); break; case MI_DIFF: diff=(diff+3+d)%3; break; case MI_SPEED: speedMul=d>0?(speedMul==1?2:speedMul==2?4:1):(speedMul==1?4:speedMul==2?1:2); break; case MI_VOL: gmMaster+=d*10; if(gmMaster<0) gmMaster=0; if(gmMaster>100) gmMaster=100; break; case MI_MON: monOn^=1; break; case MI_SCALE: hwUiScale=(hwUiScale+3+d)%3; tbInit(); uiScaleSave(); fxReq(FX_MENU_MOVE); break; } }
    if(kd&KEY_A){ switch(id){
        case MI_CONT: if(hasSave){ LOADING("LOADING YOUR WORLD",30); if(!loadGame(savePath)){ planetSize=128; newWorld(); hasSave=0; siprintf(info,"SAVE UNREADABLE"); menuNoteT=100; break; } endShown=storyDone; titleOn=0; paused=0; refillWindow(); LOADING("READY",100); musReq(GM_NEWWORLD); } else startWorld(1); break;
        case MI_CREATE: mScr=3; mSel=0; createStart(); break; case MI_GO: startWorld(storyOn); break;
#ifdef HW_CAL
        case MI_CAL: calStart(); break;
#endif
        case MI_OPT: mScr=1; mSel=0; break; case MI_EXTRA: mScr=2; mSel=0; break; case MI_BACK: { int bk=mScr==3?MI_CREATE:mScr==2?MI_EXTRA:MI_OPT; mScr=0; menuSelId(bk); } break;
        case MI_AUTOSAVE: autoSaveOn^=1; siprintf(info,autoSaveOn?"AUTOSAVE ON":"AUTOSAVE OFF"); menuNoteT=80; break; case MI_SLOT: mScr=4; slSel=slot; slAsk=-1; fxReq(FX_MENU_OK); break; case MI_SIZE: case MI_DIFF: case MI_VOL: case MI_MON: case MI_SPEED: break;
        case MI_SCULPT: if(auralOK()) menuOpenPage(3); else { siprintf(info,"UNLOCK IN STORY CHAPTER 3"); menuNoteT=100; fxReq(FX_ERROR); } break;
        case MI_STATS: menuOpenPage(13); break; case MI_FAVS: brCat=0; brSel=0; menuOpenPage(12); break; case MI_MISSION: menuOpenPage(1); break; case MI_WORLD: menuOpenPage(14); break; case MI_BROWSE: menuOpenPage(12); break; case MI_KING: menuOpenPage(7); break; case MI_CULT: menuOpenPage(10); break; case MI_LANG: menuOpenPage(11); break; case MI_PEOPLES: menuOpenPage(15); break; case MI_JOURNAL: menuOpenPage(8); jrSel=0; break; case MI_ACH: menuOpenPage(9); break; case MI_BEST: menuOpenPage(2); break; case MI_CODEX: menuOpenPage(5); break; case MI_HELP: menuOpenPage(6); break;
#ifdef __3DS__
        case MI_FILMS: filmBonusSel=0; menuOpenPage(16); break;
#endif
        case MI_DEBUG: menuOpenPage(4); break;
        case MI_CRED: menuOn=0; sceneStart(credTxt,1); musReq(GM_CREDITS); break;
        case MI_RESUME: menuOn=0; paused=dlgPaused; break; case MI_SAVE: doSave(); menuNoteT=100; break; case MI_LOAD: menuAsk=9; break;
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
    gxDefaults();
    rs^=(u32)(REG_VCOUNT*2654435761u)^0xA5A5; for(int i=0;i<8;i++) rnd();
#ifndef GG_SAFE
#ifndef GG_NO_CART_SAVE
    // Optional DraStic diagnostic: allow music to run while leaving cartridge
    // save-device probing entirely off (emulator save states still work).
    sdOk=fatInitDefault();
#ifdef NDSHW
    if(sdOk) hwSlotsInit();
#endif
#endif
#ifndef GG_NO_AUDIO
    soundEnable(); DC_FlushRange((void*)gmWave0,64); gmTables();
    // the two effect voices (a PSG pulse and the noise channel, silent until an effect plays) are reserved first, so the wave voices and drum slots can only fill the other 14 channels
    gpId=soundPlayPSG(DutyCycle_50,440,0,64); gnId=soundPlayNoise(8000,0,64);
#endif
#endif
    makeSpecies(); makeItems(); makeTiles(); makeSprites(); tbInit(); saveBasePal();
    if(sdOk){ slotRecall(); slotScan(); hasSave=loadGame(savePath); }
    if(!hasSave){ planetSize=128; newWorld(); } camX=(WW*8-256)/2; camY=(WH*8-192)/2; paused=1; if(hasSave){ camX=camX>WW*8-256?WW*8-256:camX; }
    musReq(GM_TITLE); musUpdate();
    int held=0, lastTx=-9, lastTy=-9; hud();
#ifdef HW_CAL
    { int calGood=sdOk&&hwCalLoad(); if(hwUiScale) tbInit(); if(!calGood) calStart(); }   // a saved calibration is used as it is; otherwise the first thing on a console or emulator is two taps, so the stylus lands where it is touched (START skips)
#endif
    while(1){
        scanKeys(); int kd=keysDown(), kh=keysHeld();
        if(!sceneClip){ tick++; if(inspT>0) inspT--; }
        GG_FRAME_BEGIN(frame);
#ifdef HW_CAL
        if(calOn){ if(gOnTop!=1){ lcdMainOnTop(); gOnTop=1; } calStep(kd,kh); musUpdate(); if(calDirty){ calDirty=0; calDraw((kh&KEY_TOUCH)!=0); } swiWaitForVBlank(); continue; }
#endif
        { int ui=titleOn||menuOn||extrasView||sceneOn||dlgOn; penPoll((kh&KEY_TOUCH)!=0,ui);   // hud() pairs any LCD move with the new picture
          if(ui||view3d)constructionCancelAim();
          if(ui){ if(pen.press||pen.rep){ int h=uiHitSlop(pen.x,pen.y); if(pen.rep&&h>=0&&!(uiBtns[h].key&(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT))) h=-1; if(h>=0){ const UiBtn*bt=&uiBtns[h]; int was=bt->sp&&*bt->sp==bt->sv; if(bt->sp) *bt->sp=bt->sv; kd|=was&&bt->key2?bt->key2:bt->key; if(bt->key&(KEY_L|KEY_R)) uiTabDir=(bt->key&KEY_L)?-1:1; } } kd&=~KEY_TOUCH; kh&=~KEY_TOUCH; } }
        { static int rpT; int ui2=(titleOn||menuOn||extrasView)&&!sceneOn&&!dlgOn&&!(titleOn&&konami>0), dir=kh&(KEY_UP|KEY_DOWN);   // GFX1: a held UP or DOWN repeats
          if(ui2&&dir&&!(kd&dir)){ rpT++; if(rpT>=18&&((rpT-18)&3)==0) kd|=dir; } else rpT=0; }
        GG_FRAME_BEGIN(sequencer);
        musUpdate();
        GG_FRAME_END(sequencer);
        if(view3d&&fieldLedgerOpen) zSet(Z_NORMAL);
        if(view3d&&(sceneOn||dlgOn||menuOn||titleOn)) leaveGlobe();
        if(zBmp&&(sceneOn||titleOn)) zSet(Z_NORMAL);
        if(sceneClip){
            int skipped=kd&(KEY_A|KEY_START);
            if(skipped||sceneT>=sceneFilmLength){
                sceneClip=0;
#ifdef __3DS__
                filmBonusIndex=-1;
#endif
                gmFilmMixSet(0,0,64);
                sceneT=0;
                storyScore=musLast=-1; musHold=0;
                // The first complete dialogue or narration frame replaces
                // the final cel in the same display refresh.
                if(!sceneTxt){ sceneOn=0; paused=scenePaused; }
                presentStoryCut();
            } else advanceStoryFilm();
            continue;
        }
        if(sceneOn){
            sceneT++;
            int n=sceneLines(sceneTxt), done=(kd&(KEY_A|KEY_START))!=0;
            if(sceneScroll&&(sceneT&7)==0) sceneOff++;
            if(sceneScroll&&sceneOff>n+2) done=1;
            if(done){
                if(sceneNext){
                    const char*const*nx=sceneNext;
                    int priorPause=scenePaused;
                    sceneNext=0;
                    sceneStart(nx,1);
                    scenePaused=priorPause;
                    musReq(GM_CREDITS);
                    presentStoryCut();
                    continue;
                }
                sceneOn=0;
                paused=scenePaused;
                kd&=~(KEY_A|KEY_START);
                if(dlgPend<0){
                    presentStoryCut();
                    continue;
                }
            } else {
                // A stationary story card is already on screen. Credits
                // replace it with a new frame only as the scroll advances.
                if(sceneScroll&&(sceneT&7)==0) presentStoryCut();
                else swiWaitForVBlank();
                continue;
            }
        }
        if(!titleOn&&!menuOn&&!extrasView&&!sceneOn&&!dlgOn&&dlgPend>=0&&storyOn){
            int next=dlgPend;
            dlgSet=dlgTab[next];
            dlgPend=-1;
            dlgIdx=0;
            dlgChars=0;
            dlgOn=1;
            dlgPaused=paused;
            paused=1;
            if(next>0&&next<CHAP_N){
                if(view3d) leaveGlobe();
                if(zBmp) zSet(Z_NORMAL);
                sceneStartFmv(0,&fmvClips[next]);
                advanceStoryFilm();
            } else presentStoryCut();
            continue;
        }
        if(dlgOn){
            int previousLine=dlgIdx;
            int len=(int)strlen(dlgSet[dlgIdx].txt);
            if(kd&KEY_B){
                dlgOn=0;
            } else if(kd&KEY_A){
                if(dlgChars<len) dlgChars=len;
                else {
                    dlgIdx++;
                    dlgChars=0;
                    dlgOn=dlgSet[dlgIdx].txt!=0;
                }
            } else if(dlgChars<len){
                dlgChars+=len-dlgChars>2?2:1;
            }
            if(!dlgOn){
                paused=dlgPaused;
                if(storyOn&&chapter<CHAP_N) storyGoalT=STORY_HANDOFF_TICKS;
                if(storyOn&&storyDone&&!endShown&&dlgPend<0) startFinale();
                else presentStoryCut();
                continue;
            }
            if(dlgIdx!=previousLine){
                presentStoryCut();
                continue;
            }
            refillWindow();
            swiWaitForVBlank();
            bgSetScroll(0,zBmp?0:camX&511,zBmp?0:camY&511);
            bgUpdate();
            drawSprites();
            oamUpdate(&oamMain);
            if((tick&7)==0) applyLight();
            if((tick&1)==0||kd) hud();
            continue;
        }
        if(!titleOn&&!sceneOn&&strcmp(info,lastInfo)){ strcpy(lastInfo,info); toastT=420; } if(toastT>0) toastT--;
        if(menuOn){ menuInput(kd); refillWindow(); swiWaitForVBlank(); bgSetScroll(0,zBmp?0:camX&511,zBmp?0:camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); if(kd||(tick&15)==0) hud(); continue; }   // (redrawn when a button was pressed, and now and then for the clock: a full redraw and copy every frame took longer than a frame)
        if(!titleOn&&!extrasView&&storyGoalT>0
#ifdef GG_NATIVE_STORY_BUSY
           &&!GG_NATIVE_STORY_BUSY()
#endif
          ) storyGoalT--;
        if(!titleOn&&!extrasView&&(tick&15)==0) unlockTiers();   // the chapter goal is checked here as well as in simulate(): a paused world, or a speed that steps over the 256th tick, never reached the check there
        if(storyOn&&storyDone&&!endShown&&!titleOn&&!extrasView&&!dlgOn&&dlgPend<0){ startFinale(); continue; }
        if(titleOn&&!extrasView){
            int ch=kd!=0; { static const u16 code[10]={KEY_UP,KEY_UP,KEY_DOWN,KEY_DOWN,KEY_LEFT,KEY_LEFT,KEY_RIGHT,KEY_B,KEY_A,KEY_START};
                menuInput(kd);
                if(kd){ if(kd&code[konami]) konami++; else konami=(kd&code[0])?1:0; if(konami==10){ konami=0; debugOn=1; auralOpen=1; fxReq(FX_UNLOCK); siprintf(info,"DEBUG MENU UNLOCKED"); ch=1; } } }
            if(!titleOn) continue;
            if(mScr==3&&cT>=0&&--cT<0){ previewWorld(); ch=1; }
            refillWindow(); swiWaitForVBlank(); bgSetScroll(0,zBmp?0:camX&511,zBmp?0:camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain); if((tick&7)==0) applyLight(); if(ch||(tick&31)==0){ drawMinimap(); hud(); }
            continue; }
        if(titleOn) kh&=~KEY_TOUCH;
        int sp_=(kh&KEY_B)?6:3, dirty=0;
        if(closeFieldLedger(kd)){ kd&=~(KEY_START|KEY_B); dirty=1; }
        else if(extrasView&&(kd&KEY_START)){ extrasView=0; page=0; kd&=~KEY_START; if(titleOn) mScr=menuRetScr; else { menuOn=1; mScr=menuRetScr; } mSel=menuRetSel; }
        if((kd&KEY_SELECT)&&!extrasView){ if(0){ } else if(kh&KEY_L&&kh&KEY_R){ doSave(); } else { do page=(page+1)%7; while((page==3&&!auralOK())||(page==4&&!debugOn)); if(page==3) juke=gmCur>=0?gmCur:0; } dirty=1; }
        if((kd&(KEY_X|KEY_Y))&&(kh&KEY_X)&&(kh&KEY_Y)){ zSet(zl-1); dirty=1; kd&=~(KEY_X|KEY_Y); }      // X + Y: zoom out one level (planet, very far, far, NORMAL, close, micro)
        if((kd&(KEY_L|KEY_R))&&(kh&KEY_L)&&(kh&KEY_R)&&!(kh&(KEY_START|KEY_SELECT))){ zSet(zl+1); dirty=1; kd&=~(KEY_L|KEY_R); }      // L + R: zoom in
        if(kd&KEY_START){ if((kh&KEY_L)&&(kh&KEY_R)){ unlockAll=1; siprintf(info,"EVERYTHING UNLOCKED"); } else if(!(kh&KEY_SELECT)){ menuOn=1; menuSel=0; menuAsk=0; mScr=0; mSel=0; dlgPaused=paused; paused=1; } dirty=1; }
        if(extrasView&&page!=3&&page!=4&&page!=16&&(kd&(KEY_L|KEY_R))){ uiTabDir=(kd&KEY_L)?-1:1; page=(kd&KEY_L)?extStep(-1):extStep(1); kd&=~(KEY_L|KEY_R); dirty=1; fxReq(FX_MENU_MOVE); }
#ifdef __3DS__
        if(extrasView&&page==16){
            if(kd&KEY_UP)filmBonusSel=(filmBonusSel+FILM_BONUS_N-1)%FILM_BONUS_N;
            if(kd&KEY_DOWN)filmBonusSel=(filmBonusSel+1)%FILM_BONUS_N;
            if(kd&KEY_A){
                if(filmBonusOpen(filmBonusSel)){
                    filmBonusIndex=filmBonusSel;
                    sceneStartFmv(NULL,&fmvClips[CHAP_N]);
                    fxReq(FX_MENU_OK);
                }else{
                    if(filmBonusSel==FILM_BONUS_N-1)
                        siprintf(info,"COMPLETE THE FINALE TO UNLOCK");
                    else
                        siprintf(info,"FINISH CHAPTER %d TO UNLOCK",
                                 (int)filmBonusChapterUnlock[filmBonusSel]);
                    fxReq(FX_ERROR);
                }
            }
            if(kd&KEY_B){
                extrasView=0;page=0;
                if(titleOn)mScr=menuRetScr;
                else {menuOn=1;mScr=menuRetScr;}
                mSel=menuRetSel;
            }
            if(kd)dirty=1;
            kd&=~(KEY_UP|KEY_DOWN|KEY_A|KEY_B);
        }else
#endif
        if(page==2){ if(kd&KEY_UP) bestSel=bestSel>0?bestSel-1:0; if(kd&KEY_DOWN) bestSel=bestSel<319?bestSel+1:319; if(kd&KEY_LEFT) bestSel=bestSel>=10?bestSel-10:0; if(kd&KEY_RIGHT) bestSel=bestSel<310?bestSel+10:319; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(extrasView&&(page==1||page==13||page==14)){ if(page==14){ if(wTab!=wTabPrev) dirty=1; worldInput(kd); } if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A|KEY_X|KEY_Y|KEY_SELECT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page==12){ brBuild(); if(kd&KEY_UP) brSel=brSel>0?brSel-1:0; if(kd&KEY_DOWN) brSel=brSel<brN-1?brSel+1:(brN?brN-1:0); if(kd&KEY_LEFT){ brCat=(brCat+6)%7; brSel=0; } if(kd&KEY_RIGHT){ brCat=(brCat+1)%7; brSel=0; }
            if((kd&KEY_Y)&&brCat==5){ brKind=(brKind+1)%8; brSel=0; } if((kd&KEY_X)&&brN){ u8*f=brFav(&brL[brSel]); if(f){ *f^=1; if(brL[brSel].type==4) tbInvalidate(); fxReq(*f?FX_UNLOCK:FX_MENU_MOVE); siprintf(info,*f?"STARRED":"STAR REMOVED"); } else if(brL[brSel].type>=5&&brL[brSel].id<nCr){ int on=crStarToggle(brL[brSel].id); fxReq(on?FX_UNLOCK:FX_MENU_MOVE); siprintf(info,on?"STARRED":"STAR REMOVED"); } }
            if((kd&KEY_SELECT)&&brHn){ camX=brHist[0].cx; camY=brHist[0].cy; brCat=brHist[0].cat; brBuild(); brSel=brHist[0].sel<brN?brHist[0].sel:0; int zz=brHist[0].z; for(int k=0;k<7;k++) brHist[k]=brHist[k+1]; brHn--; winX=-99; if(zz!=zl) zSet(zz); fxReq(FX_MENU_BACK); siprintf(info,"BACK WHERE YOU WERE"); fieldLedgerOpen=0; extrasView=0; page=0; if(!titleOn){ menuOn=0; paused=dlgPaused; } }
            if((kd&KEY_A)&&brN){ brGo(&brL[brSel]); } if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A|KEY_X|KEY_Y); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        else if(page>=7&&page<=11){ int*sel=(page==7||page==10)?&kingSel:page==8?&jrSel:page==11?&langSel:&achSel; int mx=(page==7||page==10)?KN_N-1:page==8?(jrN>14?jrN-14:0):page==11?4:ACH_N-1; if(page==7){ if(kd&KEY_X) cmpOn^=1; if(cmpOn){ if(kd&KEY_RIGHT) cmpB++; if(kd&KEY_LEFT) cmpB+=KN_N-1; cmpB%=KN_N; } } if(kd&KEY_UP) *sel=*sel>0?*sel-1:0; if(kd&KEY_DOWN) *sel=*sel<mx?*sel+1:mx; if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
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
                    case 5: unlockAll^=1; break; case 6: monOn^=1; break; case 7: dbgMon=(dbgMon+MON_N+d)%MON_N; break;
                    case 8: if(go){ int a=ARCH_A+dbgMon/16, sIdx=8+(a-8)*16+dbgMon%16; int ok=0; for(int k=0;k<40&&!ok;k++) ok=addCr(sIdx,cx+(int)(rnd()%7)-3,cy+(int)(rnd()%7)-3)>=0; siprintf(info,ok?"SPAWNED":"NO ROOM HERE"); } break;
                    case 9: if(go){ int n=0; for(int i=0;i<nCr;i++) if(cr[i].alive&&arch[sp[cr[i].sp].a].diet==5){ killNat(i); n++; } siprintf(info,"%d MONSTERS GONE",n); } break;
                    case 10: if(go){ for(int m=0;m<MON_N;m++){ int sIdx=8+(ARCH_A+m/16-8)*16+m%16; seenB[sIdx>>3]|=(u8)(1<<(sIdx&7)); } seenN=MON_N; siprintf(info,"ALL MONSTERS MET"); } break;
                    case 11: if(go){ for(int i=0;i<nCr;i++) if(cr[i].alive){ cr[i].hp=(u8)maxHp(&cr[i]); cr[i].food=120; } siprintf(info,"EVERYONE HEALED"); } break;
                    case 12: if(go){ for(int v=0;v<VIL_N;v++) if(vil[v].on) vil[v].stock=400; siprintf(info,"STORES FULL"); } break;
                    case 13: if(go){ int r=spSel<8?sp[spSel].a:0; siprintf(info,vilFound(r,cx,cy)>=0?"VILLAGE FOUNDED":"NO ROOM"); } break;
                    case 14: if(go){ for(int m=0;m<MON_N;m++){ int sI=8+(ARCH_A+m/16-8)*16+m%16; slainB[sI>>3]|=(u8)(1<<(sI&7)); } stKill+=50; siprintf(info,"ALL MONSTERS SLAIN"); } break;
                    case 15: if(go&&chapter<CHAP_N) chapterDone(); break;
                    case 16: if(d<0) doSave(); else if(d>0||go){ if(sdOk&&loadGame(savePath)){ refillWindow(); siprintf(info,"LOADED"); } else siprintf(info,"NO SAVE TO LOAD"); } if(go&&(kh&KEY_B)){ if(sdOk){ remove(savePath); { char b[64]; siprintf(b,"%s.bak",savePath); remove(b); } hasSave=0; siprintf(info,"SAVE ERASED"); slotScan(); } } break;
                    case 17: dbgFx=(dbgFx+GM_FX+d)%GM_FX; if(go||d) fxPlay(dbgFx); break; case 18: if(go||d) auralOpen^=1; break; case 20: seedFix=(seedFix?seedFix:worldSeed)+(d?d:0); if(!seedFix) seedFix=1; break;
                    case 21: if(go){ if(!seedFix) seedFix=worldSeed; startWorld(storyOn); seedFix=0; siprintf(info,"SEED %04X",(unsigned)(worldSeed&0xFFFF)); page=0; } break;
                    case 22: if(go){ int n=0; for(int k=0;k<400&&n<50;k++){ int i=rnd()%(nCr?nCr:1); if(nCr&&cr[i].alive){ giveTrait(&cr[i],1+(int)(rnd()%(TR_N-1)),0); cr[i].hp=(u8)maxHp(&cr[i]); n++; } } siprintf(info,"%d MUTATED",n); } break;
                    case 23: diff=(diff+3+d)%3; if(!d&&go) diff=(diff+1)%3; break;
                    case 19: if(go||d){ fullDone^=1; siprintf(info,fullDone?"100%% SET: REMIXES OPEN":"100%% CLEARED"); } break; }
                dirty=1; }
            if(kd) dirty=1; kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_L|KEY_R|KEY_A); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT); }
        /* Possession temporarily gives the D-pad to one creature; A releases it. */
        if(possessed>=0){ if(possessed>=nCr||!cr[possessed].alive) possessed=-1; else {
            if(kd&KEY_A){ possessed=-1; siprintf(info,"POSSESSION RELEASED"); dirty=1; }
            else { int dxp=((kd&KEY_RIGHT)?1:0)-((kd&KEY_LEFT)?1:0), dyp=((kd&KEY_DOWN)?1:0)-((kd&KEY_UP)?1:0);
                if(dxp||dyp){ Cr*pc=&cr[possessed]; int nx=pc->x+dxp, ny=pc->y+dyp; if(stepOk(&sp[pc->sp],&nx,&ny)) moveTo(pc,possessed,nx,ny); dirty=1; } }
            kd&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT|KEY_A); kh&=~(KEY_UP|KEY_DOWN|KEY_LEFT|KEY_RIGHT);
        } }
        if(zBmp&&!view3d){ zScale(); int st=sp_*8*zK_/zS_; if(st<1) st=1; if(kh&KEY_LEFT) zcx-=st; if(kh&KEY_RIGHT) zcx+=st; if(kh&KEY_UP) zcy-=st; if(kh&KEY_DOWN) zcy+=st; zClamp(); zSyncCam(); }      // zoomed: the pad moves the view's centre
        else {
        if(kh&KEY_LEFT) camX-=sp_; if(kh&KEY_RIGHT) camX+=sp_; if(kh&KEY_UP) camY-=sp_; if(kh&KEY_DOWN) camY+=sp_;
        if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192; }
        if(kd&(KEY_L|KEY_R)){
            int d=(kd&KEY_R)?1:-1;
            if(kh&KEY_B) tbSelect((tbCat+TB_NCAT+d)%TB_NCAT);
            else tbStep(d);
            dirty=1;
        }
        if(autoSaveDue()){ doSave(); dirty=1; }
        if(pw[power].kind==PK_BUILD&&!view3d&&(kd&(KEY_X|KEY_Y))){
            int direction=(kd&KEY_X)?1:-1;
            if(kh&KEY_B){
                int count=constructionTurns(constructionTypes[constructionMode])+1;
                constructionTurn=(constructionTurn+1+count+direction)%count-1;
            }else{
                constructionMode=(constructionMode+GG_CONSTRUCTION_N+direction)%GG_CONSTRUCTION_N;
                constructionTurn=-1;
                constructionCancelAim();
            }
            kd&=~(KEY_X|KEY_Y); dirty=1;
        }
        if(pw[power].kind==PK_FIELD&&(kd&(KEY_X|KEY_Y))&&!(view3d&&(kh&(KEY_A|KEY_B)))){
            int d=(kd&KEY_X)?1:-1;
            if(kh&KEY_B){
                fieldRadius+=d;
                if(fieldRadius<0) fieldRadius=0;
                if(fieldRadius>4) fieldRadius=4;
            } else fieldMode=(fieldMode+FIELD_N+d)%FIELD_N;
            kd&=~(KEY_X|KEY_Y); dirty=1;
        }
#ifdef GG_HAS_GLOBE
        if(view3d&&pw[power].kind==PK_BUILD&&
           ((pen.press&&!pen.fromUi&&!(kh&KEY_B))||(walkOn&&(kd&KEY_A)&&!(kh&KEY_Y)))){
            zSet(Z_NORMAL);
            constructionCancelAim();
            pen.press=0;
            kd&=~KEY_A;
            siprintf(info,"AIM ON THE MAP TO BUILD");
            dirty=1;
        }
        if(view3d){
            int ts=(kh&KEY_B)?4:2; if(!walkOn){ if(kh&KEY_LEFT) rotY-=ts; if(kh&KEY_RIGHT) rotY+=ts; if(kh&KEY_UP) rotX+=ts; if(kh&KEY_DOWN) rotX-=ts; if(!(kh&(KEY_LEFT|KEY_RIGHT|KEY_UP|KEY_DOWN))&&(tick&3)==0) rotY+=1; }
            if(rotX>85) rotX=85; if(rotX<-85) rotX=-85; rotY%=360; if(rotY<0) rotY+=360;
            if((kh&KEY_X)&&(pw[power].kind!=PK_FIELD||(kh&KEY_B))){ zoom3-=1; if(zoom3<30) zoom3=30; } if((kh&KEY_Y)&&(pw[power].kind!=PK_FIELD||(kh&KEY_B))){ zoom3+=1; if(zoom3>110) zoom3=110; }
            if((kd&KEY_A)&&(kh&KEY_Y)){ walkOn^=1; if(walkOn){ walkX=(camX>>3)+16; walkY=(camY>>3)+12; if(walkX>=WW) walkX=WW-1; if(walkY>=WH) walkY=WH-1; zoom3=30; siprintf(info,"WALKING: PAD MOVES, A USES POWER"); } else siprintf(info,"WALK OFF"); dirty=1; kd&=~KEY_A; }
            if(walkOn){ static int wt=0; int mv=(kh&KEY_B)?2:4; if(++wt>=mv){ wt=0; int dx2=((kh&KEY_RIGHT)?1:0)-((kh&KEY_LEFT)?1:0), dy2=((kh&KEY_DOWN)?1:0)-((kh&KEY_UP)?1:0); if(dx2||dy2){ walkX=(walkX+dx2+WW)%WW; walkY+=dy2; if(walkY<0) walkY=0; if(walkY>=WH) walkY=WH-1; } }
                faceTile(walkX,walkY,&rotX,&rotY); if(kh&KEY_A){ static int at=0; if((kd&KEY_A)||++at>=4){ at=0; usePower(walkX,walkY,(kd&KEY_A)?1:(pw[power].kind==PK_PAINT)); } } kd&=~KEY_A; }
            if(kd&KEY_A){ gClouds^=1; dirty=1; }
            { static int h3=0, l3x=-9, l3y=-9, lpx=-1, lpy=-1;
              static int sax=0, say=0;
              if(pen.held&&!pen.fromUi){ touchPosition tp; tp.px=pen.x; tp.py=pen.y;   // the filtered pen, the same one the 2D world uses
                  if(kh&KEY_B){ if(lpx>=0){ sax+=tp.px-lpx; say+=tp.py-lpy; rotY-=sax/2; rotX+=say/2; sax%=2; say%=2; } else { sax=say=0; } lpx=tp.px; lpy=tp.py; h3=0; }   // B held: drag to turn the planet (the odd pixel is carried over, nothing is lost)
                  else { lpx=-1; int tx,ty; if(globePick(tp.px,tp.py,&tx,&ty)){ int fresh=!h3||tx!=l3x||ty!=l3y; if(pw[power].kind==PK_PAINT||fresh) usePower(tx,ty,!h3); h3=1; l3x=tx; l3y=ty; dirty=1; } } }
              else { h3=0; lpx=-1; } }
            if(!paused) simulate(); for(int k=1;k<speedMul&&!paused;k++){ tick++; simulate(); }
            { int gd=globeDraw(dirty);
#ifdef __3DS__
              // On 3DS the tile VRAM is software memory: upload BEFORE the
              // frame is composited/presented. This removes a full frame of
              // rotation latency and avoids a redundant scan every 32 frames.
              if(gd) globeFlush();
              swiWaitForVBlank();
#else
              // Real DS hardware keeps the VBlank-only upload and its
              // periodic refresh behaviour for compatibility.
              static int gfc=0;
              swiWaitForVBlank();
              if(gd||((++gfc)&31)==0) globeFlush();
#endif
            }
            if((tick&15)==0||dirty){ drawMinimap(); hud(); } continue; }
#endif
        if(kd&(KEY_X|KEY_Y)){ int d=(kd&KEY_X)?1:-1; if(kh&KEY_B){ if(pw[power].kind==PK_GIFT) itSel=itNext(itSel,10*d); else spSel=(spSel+nSp+10*d)%nSp; } else if(pw[power].kind==PK_SPAWN){ spSel=(spSel+nSp+d)%nSp; } else if(pw[power].kind==PK_GIFT){ itSel=itNext(itSel,d); } else if(pw[power].kind==PK_MUTATE){ for(int k=1;k<=TR_N;k++){ int t=(trSel+TR_N*2+d*k)%TR_N; if(t==0||traitMet(t)||debugOn||unlockAll){ trSel=t; break; } } } else if(pw[power].kind==PK_STATUS){ statusSel=(statusSel+6+d)%6; } else { brush+=d; if(brush<0) brush=0; if(brush>8) brush=8; } dirty=1; }
        if(kd&KEY_A&&page!=3){ if(kh&KEY_B){ speedMul=speedMul==1?2:speedMul==2?4:1; siprintf(info,"SPEED X%d",speedMul); } else paused^=1; dirty=1; }
        int panning=0; { static int pnx=-1, pny=-1;   // B + drag pans without painting.
          if(pen.held&&!pen.fromUi&&!pen.fromBar&&pen.y<TB_Y&&(kh&KEY_B)){ panning=1; if(pnx>=0&&(pnx!=pen.x||pny!=pen.y)){ int ddx=pnx-pen.x, ddy=pny-pen.y;
                  if(zBmp){ zScale(); zcx+=ddx*8*zK_/zS_; zcy+=ddy*8*zK_/zS_; zClamp(); zSyncCam(); } else { camX+=ddx; camY+=ddy; if(camX<0) camX=0; if(camY<0) camY=0; if(camX>WW*8-256) camX=WW*8-256; if(camY>WH*8-192) camY=WH*8-192; } dirty=1; }
              pnx=pen.x; pny=pen.y; held=1; } else pnx=-1; }
        int tool=pw[power].kind;
        if(tool!=PK_BUILD||panning||menuOn||(kh&KEY_B)||pen.fromUi||pen.fromBar||
           (pen.held&&pen.y>=TB_Y))constructionCancelAim();
        if(panning){ }
        else if(pen.rel&&constructionAimPending){
            constructionFinish(&constructionGhost,constructionGhostX,constructionGhostY);
            constructionCancelAim();held=0;
            dirty=1;
        }else if(pen.held&&!pen.fromUi){
            // A touch started over the toolbar stays on it for the stroke.
            if(pen.fromBar){
                if((pen.press||pen.rep)&&pen.y>=TB_Y){ tbTouch(pen.x,pen.y);fxReq(FX_MENU_MOVE);dirty=1; }
            }else if(pen.y<TB_Y){
                int tx=(pen.x+camX)>>3,ty=(pen.y+camY)>>3;
                if(zBmp&&!zHit(pen.x,pen.y,&tx,&ty))constructionCancelAim();
                else if(tool==PK_BUILD){
                    if(pen.press||constructionAimPending){
                        GgBuildPlan plan={0};
                        int status=constructionPlan(&plan,tx,ty);
                        int valid=status==GG_SITE_CLEAR;
                        if(!constructionAimPending||constructionGhostX!=tx||constructionGhostY!=ty||
                           constructionGhostValid!=valid||constructionGhost.count!=plan.count||
                           memcmp(constructionGhost.cell,plan.cell,plan.count*sizeof plan.cell[0]))
                            dirty=1;
                        constructionGhost=plan;
                        constructionGhostX=tx;constructionGhostY=ty;
                        constructionGhostValid=valid;
                        constructionAimPending=constructionGhostShown=1;
                        siprintf(info,"%s",constructionIssue(status));
                    }
                }else{
                    int fresh=!held||tx!=lastTx||ty!=lastTy;
                    if(tool==PK_PAINT&&held&&(abs(tx-lastTx)>1||abs(ty-lastTy)>1)){
                        int n=abs(tx-lastTx)>abs(ty-lastTy)?abs(tx-lastTx):abs(ty-lastTy);
                        if(n>24)n=24;
                        for(int q=1;q<n;q++)usePower(lastTx+(tx-lastTx)*q/n,lastTy+(ty-lastTy)*q/n,0);
                    }
                    if(tool==PK_PAINT||fresh)usePower(tx,ty,!held);
                    lastTx=tx;lastTy=ty;dirty=1;
                }
            }
            held=1;
        }else{ held=0;constructionCancelAim(); }
        GG_FRAME_BEGIN(draw);
        refillWindow();
        GG_FRAME_END(draw);
        GG_FRAME_BEGIN(simulation);
        simulate();   // (this tick first, then the extra ones of 2x / 4x: each tick is simulated once. Before, the tick the frame started on was never simulated and the last one twice, so at 2x a year, a chapter or a monster wave that fell on the skipped ticks never came)
        for(int k=1;k<speedMul&&!paused;k++){ tick++; simulate(); }
        GG_FRAME_END(simulation);
        GG_FRAME_BEGIN(draw);
        int zd=0; if(zBmp){ if(dirty) zForce=1; if(winX==-99){ zForce=1; winX=-98; zcx=camX+128; zcy=camY+ZH/2; zClamp(); } zd=zDraw(); }      // (a zoomed picture is drawn before the blank, sent after it)
        GG_FRAME_END(draw);
        swiWaitForVBlank();
        GG_FRAME_BEGIN(draw);
        if(zd) zFlush();
        bgSetScroll(0,zBmp?0:camX&511,zBmp?0:camY&511); bgUpdate(); drawSprites(); oamUpdate(&oamMain);
        if((tick&7)==0||flashT>0) applyLight();
        if(dirty||!tbStamp) tbDraw();
        if((tick&15)==0||dirty){
            GG_FRAME_BEGIN(hudWork);
            drawMinimap(); hud();
            GG_FRAME_END(hudWork);
        }
        GG_FRAME_END(draw);
        GG_FRAME_END(frame);
    }
    return 0;
}

#endif
