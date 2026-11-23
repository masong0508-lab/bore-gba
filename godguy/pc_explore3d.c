// SDK-free regression for GodGuy's real 3DS first-person voxel code.
// gcc -O2 -std=c11 -Wall -Wextra -Icompat -Isource pc_explore3d.c -lm -o /tmp/gg-exp && /tmp/gg-exp
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <time.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t s8;
enum { T_DEEP,T_WATER,T_SAND,T_GRASS,T_FOREST,T_HILL,T_MOUNT,
       T_SNOW,T_LAVA,T_DESERT,T_SWAMP,T_JUNGLE,T_DIRT,T_N };
#include "source/building_types.h"
enum { KEY_A=1,KEY_B=2,KEY_SELECT=4,KEY_START=8,
       KEY_RIGHT=16,KEY_LEFT=32,KEY_UP=64,KEY_DOWN=128,
       KEY_R=256,KEY_L=512,KEY_X=1024,KEY_Y=2048,
       KEY_TOUCH=4096 };
#define WW 128
#define WH 128
static u8 world[WH][WW],bld[WH][WW],bldv[WH][WW];
#include "source/residential_plots.h"
static unsigned short occ[WH][WW];
static int camX,camY,editCalls;
#include "source/terrain_palette.h"
static const u8 terrCol[T_N+3][3]=GG_TERRAIN_COLORS;
static void setCell(int x,int y,u8 tile){
    if(x>=0&&y>=0&&x<WW&&y<WH){world[y][x]=tile;editCalls++;}
}
#include "source/font5x7.h"
static void ggExpRenderCreatures(u8 *top);
#include "3ds/explore3d.h"
// Minimal host simulation of the *same* creature records, identity
// compaction, occupancy and possession rules used by the full 3DS port.
#define ARCH_N 8
#define FL_SAPIENT 0x0400
#define ST_MINDLESS 0x80
#define HAS_STATUS(c,f) (((c)->sick&(f))!=0)
#define DISEASE(c) ((c)->sick&3)
typedef struct {int a;} Sp;
typedef struct {int diet,shape;} Arch;
typedef struct {int sp,x,y,alive,sick;} Cr;
static Cr cr[8];
static unsigned crId[8];
static unsigned short crFl[8];
static int nCr=5,nSp=8;
static Sp sp[8];
static Arch arch[8];
static int possessed=-1,inspected=-1,inspKind,inspIdx,inspT;
static void crName(const Cr*c,char*out){
    snprintf(out,40,"CITIZEN %d",c->sp);
}
static int ggHybridWalkable(const Cr *actor,int x,int y){
    (void)actor;
    return x>0&&y>0&&x<WW-1&&y<WH-1&&
           world[y][x]>T_WATER&&world[y][x]!=T_LAVA;
}
static void moveTo(Cr*c,int i,int nx,int ny){
    occ[c->y][c->x]=0;
    c->x=nx;c->y=ny;
    occ[ny][nx]=(unsigned short)(i+1);
}
#include "3ds/hybrid3d.h"
#define SHAPE_N 80
static const u8 famCol[16][3][3]={
    {{24,18,15},{12,8,7},{29,22,16}}
};
static int creatureFamily(const Cr *c){(void)c;return 0;}
static const char *shpRow(int shape,int frame,int row){
    // Exact authored source/main.c rows for iconic fallback regression.
    // Keep this small SDK-free fixture distinct from the generic citizen
    // below so species tests catch accidentally replacing every special
    // creature with one common silhouette.
    static const char *const iconic[3][8]={
        {"..bbb...","..beb...","...b....",".bbbbb..",
         "b.bbb.b.","..bbb...","..b.b...",".bb.bb.."}, // 16 skeleton
        {"dddddddd","dbbbbbbd","dbaabbbd","dbbbbbbd",
         "dddddddd","dbbbbbbd","dddddddd","........"}, // 78 printer
        {"..b.....","..b.....","..bb....","bbbbb...",
         "bbbbbb..","bbbbbb..",".bbbb...","..dd...."}  // 79 finger
    };
    if(shape==16)return iconic[0][row];
    if(shape==78)return iconic[1][row];
    if(shape==79)return iconic[2][row];
    static const char *const body[8]={
        "..bbbb..",".bbbbbb.",".beebbb.","..bbbb..",
        ".abbba..",".abbba..","..d..d..",".dd..dd."
    };
    (void)shape;(void)frame;
    return body[row];
}
 #include "3ds/population3d.h"
static int chapter,storyDone;
static u32 worldSeed,rs;
static int tick;
#define VIL_N 16
typedef struct {int on,x,y;u16 stock;int lvl,race,nm1,nm2;} Vil;
static Vil vil[VIL_N];
static s8 vilL[VIL_N];
static void vilName(int v,char *name){
    snprintf(name,24,"%s",v==0?"WESTFORD":"EASTFORD");
}
static int politicalDirty;
#define KD_DIRTY() (politicalDirty++)
#define GG_REIGN_SAVE_PATH "/tmp/godguy_unwritten_reign_pc_test.dat"
#define GG_CHRON_SAVE_PATH "/tmp/godguy_chronicles_pc_test.dat"
#define GG_CHRON_CHECKPOINT_PATH "/tmp/godguy_expedition_pc_test.dat"
#include "3ds/chronicles3d.h"
#include "3ds/reign3d.h"
#define GG_ORIGIN_SAVE_PATH "/tmp/godguy_origin_pc_test.dat"
#include "3ds/origin3d.h"
#include "3ds/reign_scenery.h"
#define GG_QUEST_SAVE_PATH "/tmp/godguy_quests_pc_test.dat"
#include "3ds/reign_quests.h"
#define GG_SAGA_SAVE_PATH "/tmp/godguy_saga_pc_test.dat"
#include "3ds/reign_saga3d.h"
#define GG_AMBIENT_SAVE_PATH "/tmp/godguy_ambient_pc_test.dat"
#include "3ds/reign_ambient3d.h"
static int testToolbarY=160;
#define TB_Y testToolbarY
static int ggOutdoorWaypointX=-1,ggOutdoorWaypointY=-1;
#include "3ds/reign_map3d.h"
#include "3ds/mode_input3d.h"
static void check(int value,const char *message){
    if(!value){fprintf(stderr,"EXPLORE3D FAIL: %s\n",message);exit(1);}
}
static struct {u8 first[48],img[400*240*3],last[48];} guarded;
static unsigned long hash(void){
    unsigned long h=2166136261u;
    for(int i=0;i<400*240*3;i++)h=(h^guarded.img[i])*16777619u;
    return h;
}
static void checkSagaMeshProjection(void){
    GgExplore3D saved=ggExplore;
    int quality=ggExpScenicQuality,meshEnabled=ggExpMeshEnabled;
    u8 terrain[3][7],buildings[7];
    for(int y=82;y<=84;y++)for(int x=79;x<=85;x++){
        terrain[y-82][x-79]=world[y][x];
        world[y][x]=T_GRASS;
        if(y==83){buildings[x-79]=bld[y][x];bld[y][x]=0;}
    }
    world[83][81]=T_HILL;
    world[84][83]=T_HILL;
    ggExplore.px=80*256+240;ggExplore.py=83*256+128;
    ggExplore.angle=0;ggExplore.stepPhase=5;
    ggExpMeshEnabled=1;
    check(ggMeshGroundAtQ8(ggExplore.px,ggExplore.py)!=ggExpGround(T_GRASS),
          "saga projection fixture did not place the camera on a slope");
    for(int detail=1;detail<=2;detail++){
        ggExpScenicQuality=detail;
        ggExpRenderMode(guarded.img,1);
        int heights[2];
        for(int i=0;i<2;i++){
            int wx=ggExplore.px+576+i*320,wy=ggExplore.py+64;
            int z=ggMeshGroundAtQ8(wx,wy),x,feet;
            GgMeshCameraVertex base=ggMeshCameraPoint(&ggMeshContext,
                                                      ggMeshV(wx,wy,z));
            GgMeshProjected boot=ggMeshProject(&ggMeshContext,base);
            base.z+=GG_WORLD_PERSON_HEIGHT;
            GgMeshProjected crown=ggMeshProject(&ggMeshContext,base);
            check(ggSagaProjectActor(wx,wy,&x,&feet,&heights[i])&&
                  x==boot.x*ggMeshContext.scale&&feet==boot.y*ggMeshContext.scale,
                  "saga actors do not share the rendered mesh camera on slopes");
            check(heights[i]==(boot.y-crown.y)*ggMeshContext.scale,
                  "saga actor height does not match the world's standing adult");
            if(i==0){
                GgFilmPose neutral={0};
                memset(guarded.img,0,sizeof guarded.img);
                int scale=ggSagaArtBegin(x,feet,heights[i]);
                ggFilmDrawGuestActor(guarded.img,x,feet,scale,
                     ggFilmGuestPalette[0][0],ggFilmGuestPalette[0][1],
                     ggFilmGuestPalette[0][2],&neutral,0);
                ggSagaArt.height=0;
                int top=240,bottom=-1;
                for(int y=0;y<240;y++)for(int px=0;px<400;px++){
                    const u8 *rgb=guarded.img+(y*400+px)*3;
                    if(!(rgb[0]||rgb[1]||rgb[2]))continue;
                    if(y<top)top=y;
                    if(y>bottom)bottom=y;
                }
                check(abs((bottom-top+1)-heights[i])<=2&&
                      bottom>=feet-2&&bottom<=feet,
                      "saga figure painting lost its projected height or feet");
            }
        }
        check(heights[0]>heights[1],
              "saga actors retain the same apparent height across depths");
    }
    ggExplore=saved;ggExpScenicQuality=quality;ggExpMeshEnabled=meshEnabled;
    for(int y=82;y<=84;y++)for(int x=79;x<=85;x++){
        world[y][x]=terrain[y-82][x-79];
        if(y==83)bld[y][x]=buildings[x-79];
    }
}
static void combatFixture(int style,int distance){
    ggChron=(GgChronicles){.active=1,.hp=100,.stamina=100,.focus=100,
        .food=100,.morale=70,.style=style,.foeN=1};
    ggChron.foe[0]=(GgChronFoe){20+distance,20,80,0,120};
    ggExplore.px=20*256+128;ggExplore.py=20*256+128;
    ggExplore.angle=0;ggExplore.active=1;ggExplore.actorId=0;
}
static int combatShots(void){
    int count=0;
    for(int i=0;i<GG_CHRON_SHOTS;i++)count+=ggChron.shots[i].life!=0;
    return count;
}
static void combatStyles(void){
    GgChronicles savedChron=ggChron;
    GgExplore3D savedExplore=ggExplore;
    u8 terrain[5][13],buildings[5][13];
    unsigned short inhabitants[5][13];
    for(int y=0;y<5;y++)for(int x=0;x<13;x++){
        terrain[y][x]=world[y+18][x+18];world[y+18][x+18]=T_GRASS;
        buildings[y][x]=bld[y+18][x+18];bld[y+18][x+18]=0;
        inhabitants[y][x]=occ[y+18][x+18];occ[y+18][x+18]=0;
    }
    combatFixture(GG_CHRON_BLADE,1);
    for(int i=0;i<22;i++)ggChronStep(KEY_A);
    check(ggChron.foe[0].hp==80,"holding the blade did damage before release");
    ggChronStep(0);
    check(ggChron.stamina==77,"heavy blade did not spend stamina");
    for(int i=0;i<7;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==49,"heavy blade did not resolve once in reach");

    combatFixture(GG_CHRON_BLADE,2);
    ggExplore.px=20*256+248;
    bld[20][21]=B_HALL;
    for(int i=0;i<22;i++)ggChronStep(KEY_A);
    ggChronStep(0);
    for(int i=0;i<6;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==80&&ggChron.combatNotice==GG_COMBAT_COVER,
          "blade did not distinguish cover from missing its target");
    bld[20][21]=0;

    combatFixture(GG_CHRON_BLADE,5);
    ggChronStep(GG_3DS_ZR);
    check(ggChron.style==GG_CHRON_BOW,"weapon cycle did not equip the bow");
    combatFixture(GG_CHRON_BOW,3);
    ggChronStep(KEY_A);ggChronStep(0);
    check(combatShots()==1&&ggChron.stamina==94&&ggChron.foe[0].hp==80,
          "tapping A did not launch one low-cost arrow");
    for(int i=0;i<18&&combatShots();i++)ggChronStep(0);
    check(ggChron.foe[0].hp==71&&!combatShots(),
          "tap arrow did not resolve once at short range");
    check(!ggChron.attackFlash&&ggChron.combatNotice==GG_COMBAT_HIT,
          "a distant arrow impact replayed the player's release pose");

    combatFixture(GG_CHRON_BOW,5);
    for(int i=0;i<22;i++)ggChronStep(KEY_A|GG_3DS_ZL);
    ggChronStep(GG_3DS_ZL);
    check(combatShots()==1&&ggChron.foe[0].hp==80&&ggChron.stamina<100,
          "released arrow skipped flight time or bypassed stamina");
    for(int i=0;i<31;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==56&&!combatShots(),
          "aimed arrow missed a visible foe or struck it twice");

    combatFixture(GG_CHRON_BOW,5);
    ggChron.foeN=0;
    for(int i=0;i<8;i++)ggChronStep(KEY_A|GG_3DS_ZL);
    int drawY=ggExplore.py;
    ggChronStep(KEY_R|KEY_RIGHT);
    check(combatShots()==1&&ggChron.shots[0].y==ggExplore.py&&
          ggChron.shots[0].y!=drawY&&ggChron.shots[0].vy==ggExpSin[2]*76/4096,
          "releasing A and ZL together changed a moving shot's aim");

    combatFixture(GG_CHRON_BOW,5);
    bld[20][22]=B_HALL;
    for(int i=0;i<24;i++)ggChronStep(KEY_A|GG_3DS_ZL);
    ggChronStep(GG_3DS_ZL);
    for(int i=0;i<30;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==80&&!combatShots()&&ggChron.combatNotice==GG_COMBAT_COVER,
          "arrow passed through structural cover");
    int x=22*256+128,y=20*256+128,ground=ggExpGround(T_GRASS);
    int roof=ground+ggExpBuilding(B_HALL);
    check(ggChronCombatCover(x,y,roof+6)&&!ggChronCombatCover(x,y,roof+17),
          "roof collision ignores the rendered roof height");
    bld[20][22]=B_RUIN;
    check(ggChronCombatLine(x-320,y,ground+11,x+320,y,ground+11)&&
          !ggChronCombatLine(x-320,y-54,ground+11,x+320,y-54,ground+11),
          "ruin collision blocks a visible breach or leaks through a wall");
    bld[20][22]=0;

    combatFixture(GG_CHRON_EMBER,2);
    ggChronStep(KEY_A);ggChronStep(0);
    check(combatShots()==1&&ggChron.focus==88&&ggChron.foe[0].hp==80,
          "tapping A did not launch one low-focus ember bolt");
    for(int i=0;i<14;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==67&&!combatShots(),
          "tap ember did not resolve once at short range");

    combatFixture(GG_CHRON_EMBER,4);
    ggChronStep(KEY_A);ggChronStep(0);
    ggChronStep(KEY_A);
    check(!ggChron.charge&&ggChron.combatNotice==GG_COMBAT_RECOVERY,
          "pressing A during recovery was silently ignored");
    for(int i=0;i<14;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==80&&!combatShots()&&
          ggChron.combatNotice==GG_COMBAT_SHOT_MISS,
          "tap ember reached charged range or missed without feedback");

    combatFixture(GG_CHRON_EMBER,4);
    ggChron.focus=17;
    for(int i=0;i<22;i++)ggChronStep(KEY_A);
    ggChronStep(0);
    check(!combatShots()&&ggChron.focus==17&&ggChron.combatNotice==GG_COMBAT_FOCUS,
          "an unaffordable charged cast changed focus or fired");
    ggChron.focus=5;
    ggChronStep(KEY_A);
    check(!ggChron.charge&&ggChron.combatNotice==GG_COMBAT_FOCUS,
          "an unaffordable tap began drawing a cast");

    combatFixture(GG_CHRON_EMBER,4);
    for(int i=0;i<GG_CHRON_SHOTS;i++){
        ggChronStep(KEY_A);ggChronStep(0);
        ggChron.attackWait=0;
    }
    int reserve=ggChron.focus;
    ggChronStep(KEY_A);ggChronStep(0);
    check(combatShots()==GG_CHRON_SHOTS&&ggChron.focus==reserve&&
          ggChron.combatNotice==GG_COMBAT_BUSY,
          "full projectile pool lost a cast or spent its focus");

    combatFixture(GG_CHRON_EMBER,4);
    for(int i=0;i<22;i++)ggChronStep(KEY_A);
    ggChronStep(0);
    check(combatShots()==1&&ggChron.focus==78&&ggChron.foe[0].hp==80,
          "ember cast did not consume focus and launch a bolt");
    for(int i=0;i<26;i++)ggChronStep(0);
    check(ggChron.foe[0].hp==52,"ember bolt failed to hit at its world position");
    ggChron.guardTicks=10;
    int focus=ggChron.focus;
    ggChronIncomingHit(0,12);
    check(ggChron.hp==97&&ggChron.focus==focus-13&&ggChron.combatNotice==GG_COMBAT_WARD,
          "ward did not trade focus for protection");

    combatFixture(GG_CHRON_EMBER,1);
    ggChronStep(GG_3DS_ZL);
    ggChronStep(KEY_A|GG_3DS_ZL);
    ggChronIncomingHit(0,12);
    check(ggChron.hp==88&&ggChron.focus==100,"charging ember still allowed a ward");
    ggChronStep(GG_3DS_ZL);
    ggChronIncomingHit(0,12);
    check(ggChron.hp==85&&ggChron.focus==75&&ggChron.combatNotice==GG_COMBAT_WARD,
          "holding ZL through a cast did not resume warding");
    ggChron.focus=12;
    ggChronIncomingHit(0,12);
    check(ggChron.hp==73&&ggChron.focus==12&&ggChron.combatNotice==GG_COMBAT_FOCUS,
          "an exhausted ward did not report the failed defense");

    combatFixture(GG_CHRON_BLADE,1);
    ggChron.guardTicks=1;
    ggChronIncomingHit(0,12);
    check(ggChron.hp==100&&ggChron.stamina==91&&ggChron.combatNotice==GG_COMBAT_PARRY,
          "timed frontal sword parry failed");
    ggExplore.angle=128;
    ggChronIncomingHit(0,12);
    check(ggChron.hp==88,"sword guard blocked a rear attack");
    ggChronCombatStow();
    ggChronCombatStep(GG_3DS_ZL,0);
    check(!ggChron.guardTicks,"held overlay key rearmed a fresh parry");

    combatFixture(GG_CHRON_BLADE,1);
    ggChronStep(GG_3DS_ZL);
    ggChronStep(KEY_A|GG_3DS_ZL);
    ggChronIncomingHit(0,12);
    check(ggChron.hp==88,"blade charge still allowed a parry");
    ggChronStep(GG_3DS_ZL);
    for(int i=0;i<3;i++)ggChronStep(GG_3DS_ZL);
    ggChronIncomingHit(0,12);
    check(ggChron.hp==84&&ggChron.combatNotice==GG_COMBAT_BLOCK&&!ggChron.attackFlash,
          "holding ZL through a swing did not resume a steady block");
    ggChron.stamina=8;
    ggChronIncomingHit(0,12);
    check(ggChron.hp==72&&ggChron.stamina==8&&ggChron.combatNotice==GG_COMBAT_STAMINA,
          "an exhausted shield did not report the failed defense");
    ggExplore.angle=0;
    ggExplore.px=21*256-6;
    ggChronStep(KEY_UP);
    check(ggExplore.px==21*256-6,"player movement entered a living foe's tile");
    combatFixture(GG_CHRON_BLADE,5);
    bld[20][21]=B_TEMPLE;
    ggExplore.px=21*256-40;ggExplore.py=20*256+66;
    ggChronStep(KEY_UP);
    check(ggExplore.px==21*256-40,"player walked inside a visible temple column");
    ggExplore.py=20*256+128;
    ggChronStep(KEY_UP);
    check(ggExplore.px>21*256-40,"column collision also closed the open temple aisle");
    bld[20][21]=0;

    for(int y=0;y<5;y++)for(int x=0;x<13;x++){
        world[y+18][x+18]=terrain[y][x];
        bld[y+18][x+18]=buildings[y][x];
        occ[y+18][x+18]=inhabitants[y][x];
    }
    ggChron=savedChron;ggExplore=savedExplore;
}
int main(void){
    remove(GG_CHRON_SAVE_PATH);remove(GG_CHRON_CHECKPOINT_PATH);
    // Real-world citizen sprites must use the same 250px lens,
    // physically projected foot row and 2px walking-camera bob.
    int npcX,npcFoot;
    ggExpNpcProject(512,256,10,10,0,3,&npcX,&npcFoot);
    check(npcX==325-12&&npcFoot==ggFilmProjectFootY(512,10,10),
          "playable NPCs still drift against the world's camera lens");
    ggExpNpcProject(512,-256,10,10,1,3,&npcX,&npcFoot);
    check(npcX==75-12&&npcFoot==ggFilmProjectFootY(512,10,10)+2,
          "citizen profile/ground height changes across POV movement");
    // An entry combo must not become a save or council command on the
    // following frame, and START must not pause the 2D game on exit.
    GgModeInputGuard guard={0};
    int enter=KEY_SELECT|KEY_R;
    check(ggModeInputFilter(&guard,enter)==enter,
          "unblocked mode-entry keys should be readable");
    ggModeInputTransition(&guard,enter);
    check(ggModeInputFilter(&guard,enter)==0,
          "held 3D entry combo leaked into the gameplay input");
    check(ggModeInputFilter(&guard,KEY_R)==0,
          "partial combo release prematurely retriggered a shoulder");
    check(ggModeInputFilter(&guard,KEY_R|KEY_A)==KEY_A,
          "new attack key was blocked by held transition button");
    check(ggModeInputFilter(&guard,0)==0,
          "release did not clear transition suppression");
    check(ggModeInputFilter(&guard,KEY_R)==KEY_R,
          "a genuinely fresh press remained blocked");
    ggModeInputTransition(&guard,KEY_START);
    check(ggModeInputFilter(&guard,KEY_START)==0,
          "3D exit START leaked to the original 2D pause menu");
    check(ggModeInputFilter(&guard,0)==0&&
          ggModeInputFilter(&guard,KEY_START)==KEY_START,
          "released START did not become usable again");
    for(int y=0;y<WH;y++)for(int x=0;x<WW;x++)world[y][x]=T_GRASS;
    memset(guarded.first,0x5a,sizeof guarded.first);
    memset(guarded.last,0xc3,sizeof guarded.last);
    ggExpBegin(64,64,0);
    // Objective pins are painted only on the existing lower-screen
    // pixels, and never write outside its 320x240 RGB frame.
    camX=64*8-128;camY=64*8-96;
    ggChron.active=1;ggChron.post=0;
    ggChron.px[0]=64;ggChron.py[0]=64;
    memset(guarded.img,0,320*240*3);
    check(ggMapObjectiveSignature()!=0,"live mission pin not requested");
    check(ggMapPinDraw(guarded.img,64,64,244,204,82),
          "mission objective is not visible on bottom 2D map");
    int pinX=32+128+4,pinY=24+96+4;
    check(guarded.img[(pinY*320+pinX)*3]==255,
          "objective marker did not render a centered crosshair");
    check(!ggMapPinDraw(guarded.img,0,0,244,204,82),
          "off-screen map pin tried to draw outside map");
    ggChron.px[0]=100;ggChron.py[0]=64;
    memset(guarded.img,0,320*240*3);
    ggMapObjectivesDraw(guarded.img);
    int edgeInk=0;
    for(int y=112;y<137;y++)for(int x=277;x<288;x++)
        edgeInk+=guarded.img[(y*320+x)*3]==244;
    check(edgeInk>=3,"off-screen objective has no bottom-map direction cue");
    ggChron.px[0]=64;ggChron.py[0]=100;
    testToolbarY=136;
    memset(guarded.img,0,320*240*3);
    ggMapObjectivesDraw(guarded.img);
    int southInk=0,toolbarInk=0;
    for(int y=140;y<216;y++)for(int x=145;x<175;x++){
        int bright=guarded.img[(y*320+x)*3]==244;
        if(y<24+testToolbarY)southInk+=bright;
        else toolbarInk+=bright;
    }
    check(southInk>=3&&!toolbarInk,"south objective cue draws on the toolbar");
    testToolbarY=160;
    ggChron.px[0]=ggChron.py[0]=64;
    check(guarded.first[0]==0x5a&&guarded.last[0]==0xc3,
          "objective pin overflowed 3DS map buffer");
    ggChron.active=0;
    check(ggExplore.active&&(ggExplore.px>>8)==64&&(ggExplore.py>>8)==64,
          "did not enter the player's map at its saved camera position");
    check(ggExpTrigReady,"first-person camera trig not cached");
    // A planet of water must refuse an invalid free-walk spawn rather
    // than enter a trapped/black 3D camera. A subsequent dry planet
    // must still enter successfully with the same renderer.
    for(int yy=0;yy<WH;yy++)for(int xx=0;xx<WW;xx++)
        world[yy][xx]=T_WATER;
    check(!ggExpBegin(64,64,0)&&!ggExplore.active,
          "all-water world entered an impassable 3D free-walk state");
    for(int yy=0;yy<WH;yy++)for(int xx=0;xx<WW;xx++)
        world[yy][xx]=T_GRASS;
    check(ggExpBegin(64,64,0)&&ggExplore.active,
          "valid land failed to recover 3D after a refused spawn");
    // The faster incremental ray must sample exactly the old positions,
    // especially left/backwards rays where C division truncates to zero.
    for(int a=0;a<256;a+=7)for(int col=0;col<200;col+=5){
        int x=ggExpCos[a]-ggExpSin[a]*(col-100)/125;
        int y=ggExpSin[a]+ggExpCos[a]*(col-100)/125;
        int sx=0,sy=0;
        for(int d=1;d<=GG_EXP_MAX_DEPTH;d++){
            sx+=x*256;sy+=y*256;
            check(sx/4096==x*d*256/4096 &&
                  sy/4096==y*d*256/4096,
                  "incremental ray drifted from original world sampling");
        }
    }
    // The unclipped 2x2 raster is byte-identical for valid rays.
    ggExpDot(guarded.img,25,35,97,149,201);
    u8 pixelRef[12];
    for(int yy=0;yy<2;yy++)
        memcpy(pixelRef+yy*6,guarded.img+((35*2+yy)*400+25*2)*3,6);
    ggExpDotInView(guarded.img,25,35,97,149,201);
    for(int yy=0;yy<2;yy++)
        check(!memcmp(pixelRef+yy*6,
               guarded.img+((35*2+yy)*400+25*2)*3,6),
              "fast terrain pixel differs from clipped reference");
    int ox=ggExplore.px,oy=ggExplore.py;
    ggExplore.angle=0;
    ggExpStep(KEY_UP,0,0);
    check(ggExplore.px>ox&&ggExplore.py==oy,
          "D-pad forward does not travel along camera heading");
    ggExpStep(0,0,0);
    int yaw=ggExplore.angle;
    ggExpStep(KEY_R,0,0);
    check(ggExplore.angle!=yaw,"R shoulder does not turn the viewpoint");
    ggExplore.angle=0;ggExplore.px=64*256+128;ggExplore.py=64*256+128;
    int tx,ty;ggExpTarget(&tx,&ty);
    check(tx==67&&ty==64,"build crosshair does not target the adjacent world");
    ggExplore.material=4; // HILL
    check(ggExpEdit(tx,ty,ggExplore.material,0),"cannot place a terrain block");
    check(world[ty][tx]==T_HILL&&editCalls==1,
          "voxel edit did not update the existing 2D god-game map");
    bld[ty][tx]=B_HALL;
    check(!ggExpEdit(tx,ty,0,1)&&world[ty][tx]==T_HILL,
          "terrain edit razed a real city hall");
    bld[ty][tx]=0;occ[ty][tx]=1;
    check(!ggExpEdit(tx,ty,0,1),"terrain edit damaged an occupied creature");
    occ[ty][tx]=0;
    // Corrupted terrain must never index outside terrCol or allow a
    // player to begin walking through an unknown physical material.
    world[ty][tx]=255;
    check(!ggExpWalkable(tx,ty,60,1),
          "invalid terrain tile accepted as walkable");
    ggExpRender(guarded.img);
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "corrupted terrain crashed/overran the software voxel raster");
    world[ty][tx]=T_HILL;
    check(ggExpEdit(tx,ty,0,1)&&world[ty][tx]==T_GRASS,
          "removing the terrain does not restore the live world");
    ggExplore.px=63*256+248;ggExplore.py=64*256+128;
    ggExplore.angle=0;world[64][64]=T_DEEP;
    int before=ggExplore.px;
    ggExpStep(KEY_UP,0,0);
    check(ggExplore.px==before,"walking crossed unwalkable deep water");
    ggExpStep(0,0,0);
    world[64][64]=T_GRASS;bld[64][64]=B_TOWER;
    ggExpStep(KEY_UP,0,0);
    check(ggExplore.px==before,"walking crossed a built city tower");
    bld[64][64]=0;
    ggExplore.px=64*256+128;ggExplore.py=64*256+128;
    camX=64*8-128;camY=64*8-96;
    ggExpStep(0,0,0);
    ggExpStep(KEY_TOUCH,128+4*8,96+3*8);
    check((ggExplore.px>>8)==68&&(ggExplore.py>>8)==67,
          "tapping the actual bottom map cannot visit its world location");
    // Lower 32 pixels of the visible touch map used to be unreachable.
    ggExpStep(0,0,0);
    ggExpStep(KEY_TOUCH,128,176);
    check((ggExplore.px>>8)==64&&(ggExplore.py>>8)==74,
          "southern 32px of the bottom-screen map ignored touch travel");
    ggExpStep(0,0,0);
    check(ggExpStep(KEY_SELECT,0,0)==GG_EXP_SAVE,
          "SELECT cannot invoke existing save system");
    ggExpStep(0,0,0);
    check(ggExpStep(KEY_START,0,0)==GG_EXP_EXIT,
          "START cannot safely leave first-person mode");
    ggExplore.px=64*256+128;ggExplore.py=64*256+128;
    ggExplore.angle=0;
    clock_t started=clock();
    ggExpRender(guarded.img);
    unsigned long first=hash();
    ggExplore.angle=64;ggExpRender(guarded.img);
    check(first!=hash(),"camera does not render a different scene when turned");
    bld[70][70]=B_HALL;bld[69][70]=B_TOWER;
    ggExplore.angle=32;
    ggExpRender(guarded.img);unsigned long built=hash();
    bld[70][70]=0;bld[69][70]=0;
    ggExpRender(guarded.img);
    check(hash()!=built,"user-built city buildings are absent from the view");
    check(guarded.first[0]==0x5a&&guarded.first[47]==0x5a&&
          guarded.last[0]==0xc3&&guarded.last[47]==0xc3,
          "renderer overran 400x240 target framebuffer");
    // Every playable mind comes from the actual game's diet or
    // FL_SAPIENT classification; a common animal is not a human brain.
    sp[0].a=0;arch[0].diet=4;  // person
    sp[1].a=1;arch[1].diet=1;  // unevolved animal
    cr[0]=(Cr){0,40,41,1,0};crId[0]=1001;
    cr[1]=(Cr){1,43,41,1,0};crId[1]=1002;
    cr[2]=(Cr){1,47,41,1,0};crId[2]=1003;
    occ[41][40]=1;occ[41][43]=2;occ[41][47]=3;
    check(ggHybridEligible(0)&&!ggHybridEligible(1),
          "hybrid mind filter confused people with ordinary animals");
    int savedSpecies=cr[0].sp,savedX=cr[0].x;
    cr[0].sp=-1;
    check(!ggHybridEligible(0),
          "damaged creature species index accessed outside species array");
    cr[0].sp=savedSpecies;cr[0].x=WW;
    check(!ggHybridEligible(0),
          "out-of-world creature selected into a crashing POV");
    cr[0].x=savedX;
    check(ggHybridPick(43,41)==-1,
          "hybrid selection possessed a nonsentient animal");
    crFl[2]=FL_SAPIENT;
    check(ggHybridEligible(2)&&ggHybridPick(47,41)==2,
          "uplifted/sapient animal cannot become a selected hybrid mind");
    cr[2].sick=ST_MINDLESS;
    check(!ggHybridEligible(2),"mindless creature can enter first-person POV");
    cr[2].sick=3;
    check(!ggHybridEligible(2),"infected mind can enter hybrid POV");
    cr[2].sick=0;
    check(ggHybridPick(40,41)==0,
          "2D tile select did not use the authoritative occupant grid");
    check(ggHybridBegin(0,0)&&ggExplore.actorId==1001&&
          ggExplore.px>>8==40&&ggExplore.py>>8==41,
          "hybrid camera failed to attach to the chosen 2D person");
    check(possessed==0&&inspected==0,
          "2D Inspect/Possess selection was lost on entering 3D");
    ggExplore.angle=0;
    ggExplore.px=40*256+244;ggExplore.py=41*256+128;
    int ox2=ggExplore.px,oy2=ggExplore.py;
    ggExpStep(KEY_UP,0,0);
    check(ggHybridMove(ox2,oy2)&&cr[0].x==41&&cr[0].y==41&&
          occ[41][41]==1&&occ[41][40]==0,
          "3D walking did not move that same creature on the 2D map");
    ggExpStep(0,0,0);
    bld[41][42]=B_HOUSE;
    ggExplore.px=41*256+249;ggExplore.py=41*256+128;
    ox2=ggExplore.px;oy2=ggExplore.py;
    ggExpStep(KEY_UP,0,0);
    check(ggHybridMove(ox2,oy2)&&cr[0].x==41,
          "hybrid character phased through a city building");
    bld[41][42]=0;
    // Saving compacts the creature array. The hybrid camera follows
    // crId, not a stale index that may now point to somebody else.
    cr[4]=cr[0];crId[4]=crId[0];crFl[4]=crFl[0];
    cr[0].alive=0;crId[0]=9999;
    occ[41][41]=5;
    check(ggHybridResolve()==4,
          "save compaction rebound the camera to a different person");
    ggHybridAfterSave();
    check(possessed==4&&inspected==4&&inspIdx==4,
          "2D selection no longer tracks the same mind after save");
    cr[4].alive=0;
    check(ggHybridResolve()<0,
          "dead or invalid creature remains possessed in 3D");
    // Actor can begin in a house and leave it, instead of having its POV
    // teleported to a fake grassy spawn to satisfy generic walk checks.
    cr[2].x=47;cr[2].y=41;cr[2].alive=1;crId[2]=1003;
    bld[41][47]=B_HUT;
    check(ggHybridBegin(2,0)&&ggExplore.px>>8==47,
          "a sentient household occupant was displaced at scene entry");
    check(ggExpWalkable(47,41,10,0),
          "first-person control locked a resident inside their own home");
    bld[41][47]=0;
    // The new 3D renderer must retain the existing 2D people and pets,
    // not render an eerily empty block world. An intervening city wall
    // must hide the billboard instead of drawing people through stone.
    cr[1]=(Cr){1,47,45,1,0};crId[1]=1002;
    occ[45][47]=2;
    ggExplore.angle=64;
    ggExpRender(guarded.img);
    unsigned long crowd=hash();
    check(ggExpNpcsDrawn==1,
          "3DS gameplay dropped native-detail 2D NPC sprites");
    occ[45][47]=0;
    ggExpRender(guarded.img);
    check(crowd!=hash()&&ggExpNpcsDrawn==0,
          "visible NPC sprite did not respond to live occupancy");
    occ[45][47]=2;
    bld[43][47]=B_HALL;
    ggExpRender(guarded.img);
    check(ggExpNpcsDrawn==0,
          "an NPC sprite appeared through a city hall wall");
    bld[43][47]=0;
    occ[45][47]=0;
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "live NPC sprite animation exceeded the 3DS top framebuffer");
    // ASHEN CHRONICLES: the existing 2D story gates the separate 3DS
    // campaign. All objectives must occupy reachable real world tiles.
    chapter=0;
    check(!ggChronReady(),"3DS side story opened before its 2D unlock");
    chapter=3;
    check(ggChronReady()&&!ggChronUnlockedAct(1),
          "campaign acts ignored their original-story chapter gates");
    ggExplore.actorId=0;
    ggChron.loaded=1;ggChron.cleared=0;
    check(ggChronBegin(60,60,0)==1&&ggChron.active,
          "chapter 3 cannot enter the unlocked companion campaign");
    check(ggChron.foeN>=3&&ggChron.foeN<=GG_CHRON_FOES,
          "campaign has no bounded enemy encounters");
    // Route directions must follow passable tiles rather than lying
    // about where the player can go through walls, cliffs or water.
    int navX,navY,navSteps=0;
    int navOk=ggChronNavNext(ggExplore.px>>8,ggExplore.py>>8,
                            &navX,&navY,&navSteps);
    check(navOk&&navSteps>=0,
          "mission route navigation failed to reach starting region");
    if(navSteps>0){
        check(ggChronLand(navX,navY)&&
              ggChronNear(navX,navY,ggExplore.px>>8,
                          ggExplore.py>>8)==1,
              "route-aware navigation proposed an impassable or remote step");
        int nextSteps=0,nextX,nextY;
        check(ggChronNavNext(navX,navY,&nextX,&nextY,&nextSteps)&&
              nextSteps<navSteps,"mission route did not progress toward post");
    }
    for(int i=0;i<GG_CHRON_POSTS;i++){
        check(ggChronLand(ggChron.px[i],ggChron.py[i]),
              "the campaign objective is on blocked or water terrain");
        for(int j=0;j<i;j++)
            check(ggChronNear(ggChron.px[i],ggChron.py[i],
                              ggChron.px[j],ggChron.py[j])>=5,
                  "two mission objectives share an indistinguishable site");
    }
    ggChron.briefTime=0;
    ggExplore.angle=0;
    int cx=ggExplore.px>>8,cy=ggExplore.py>>8;
    ggChron.foeN=1;
    ggChron.foe[0]=(GgChronFoe){cx+1,cy,25,0,0};
    ggChronStep(KEY_A);
    check(ggChron.foe[0].hp==25,
          "drawing a weapon hit the guard before the action committed");
    for(int i=0;i<8&&ggChron.foe[0].hp==25;i++)ggChronStep(0);
    check(ggChron.foe[0].hp<25&&ggChron.attackWait>0,
          "real-time aimed melee did not hit a visible guard");
    combatStyles();
    ggChronStep(0);
    ggChron.hp=55;
    int rations=ggChron.rations;
    ggChronStep(KEY_Y);
    check(ggChron.hp>55&&ggChron.rations==rations-1,
          "survival rations did not consume supplies and restore health");
    ggChron.foeN=0;
    for(int i=0;i<GG_CHRON_POSTS;i++){
        ggChron.briefTime=0;
        ggExplore.px=ggChron.px[i]*256+128;
        ggExplore.py=ggChron.py[i]*256+128;
        ggChronStep(0); // release X between capture attempts
        ggChronStep(KEY_X);
        check(ggChron.post==i+1,"guard-cleared outpost failed to capture");
    }
    check(ggSaga.mode==GG_SAGA_CHOICE&&!ggChron.won,
          "chapter climax skipped the player's meaningful story decision");
    ggSagaCommitChoice(0);
    check(ggSaga.mode==GG_SAGA_OUTRO&&ggSaga.decided&1,
          "chapter decision was not recorded for future endings");
    check(ggChron.won&&ggChron.cleared==1,
          "campaign victory did not unlock the following act");
    check(ggChronBegin(60,60,0)==-1,
          "later chapters bypassed their 2D storyline prerequisite");
    storyDone=1;
    check(ggChronBegin(60,60,0)==1&&ggChron.act==1,
          "completed original story cannot access the next campaign act");
    ggExpRender(guarded.img);
    memset(guarded.img,0,sizeof guarded.img);
    ggChronDraw(guarded.img,NULL,0);
    check(guarded.img[(26*400+10)*3]==0,
          "Reign HUD still covers space below its objective strip");
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "campaign banners or HUD overran the shared 3DS framebuffer");
    // First entry should be an ordinary childhood and the age-22
    // awakening, not a selection of preordained career classes.
    remove(GG_REIGN_SAVE_PATH);
    memset(&ggReign,0,sizeof ggReign);
    ggReignLoad();
    check(ggReign.role==GG_ROLE_UNBOUND&&!ggReign.initialChoice,
          "demigod began with an imposed job before creation");
    remove(GG_ORIGIN_SAVE_PATH);
    memset(&ggOrigin,0,sizeof ggOrigin);
    ggOriginBegin();
    check(ggOrigin.active&&ggOrigin.phase==GG_ORIGIN_BIRTH,
          "first entry omitted the ordinary birth opening");
    ggOriginStep(KEY_A);
    check(ggOrigin.phase==GG_ORIGIN_CHILD,
          "the birth story did not reach childhood");
    ggOriginStep(0);
    ggOriginStep(KEY_A);
    check(ggOrigin.phase==GG_ORIGIN_YOUTH,
          "the youth story jumped straight to divine powers");
    ggOriginStep(0);
    ggOriginStep(KEY_START);
    check(ggOrigin.phase==GG_ORIGIN_CREATE,
          "the opening cannot reach basic adult creation");
    ggOriginStep(0);
    ggOriginStep(KEY_RIGHT);
    ggOriginStep(0);
    ggOriginStep(KEY_DOWN);
    ggOriginStep(0);
    ggOriginStep(KEY_RIGHT);
    check(ggOrigin.name==1&&ggOrigin.body==1,
          "name and appearance choices are not independent");
    ggOriginDraw(guarded.img);
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "character art exceeded the 3DS framebuffer");
    ggOriginStep(0);
    ggOriginStep(KEY_A);
    check(ggOrigin.phase==GG_ORIGIN_AWAKEN,
          "character creation cannot proceed to age-22 revelation");
    for(int t=0;t<45;t++)ggOriginStep(0);
    ggOriginDraw(guarded.img);
    ggOriginStep(KEY_A);
    check(ggOrigin.phase==GG_ORIGIN_EPILOGUE,
          "the first miracle did not lead to the aftermath");
    ggOriginStep(0);
    ggOriginStep(KEY_A);
    check(!ggOrigin.active&&ggOrigin.complete&&ggReign.divine==28,
          "demigod powers were not discovered at early adulthood");
    memset(&ggOrigin,0,sizeof ggOrigin);
    ggOriginBegin();
    check(!ggOrigin.active&&ggOrigin.complete&&
          ggOrigin.name==1&&ggOrigin.body==1,
          "completed opening and adult creator were not persisted");
    remove(GG_ORIGIN_SAVE_PATH);
    remove(GG_REIGN_SAVE_PATH);
    // UNWRITTEN REIGN - class, crime, civic, office and divine systems
    // are independent from the native 2D story; all civic actions use
    // actual village stocks/loyalty rather than invented map markers.
    vil[0]=(Vil){1,60,60,125};
    vil[1]=(Vil){1,70,60,125};
    vil[2]=(Vil){1,80,60,125};
    vil[3]=(Vil){1,90,60,125};
    ggChron.active=1;ggChron.won=0;ggChron.lost=0;ggChron.foeN=0;
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggExplore.angle=0;
    memset(&ggReign,0,sizeof ggReign);
    ggReign.loaded=1;ggReign.credits=110;ggReign.divine=100;
    ggReign.legitimacy=80;ggReign.influence=25;
    ggReignBegin();
    check(ggReignEnroll(GG_ROLE_SHADE)&&ggReign.role==GG_ROLE_SHADE,
          "a godborn character could not freely choose a thieving class");
    int stock=vil[0].stock;
    check(ggReignDeed()&&vil[0].stock==stock-8&&ggReign.infamy==9 &&
          politicalDirty>0,
          "a heist did not change the generated town or public trust");
    check(!ggReignDeed(),"repeated heists bypassed the world-time cooldown");
    ggReign.frame=ggReign.nextCivicAt;
    check(ggReignEnroll(GG_ROLE_TRADER),
          "trader was not a reachable non-linear profession");
    int cash=ggReign.credits,goods=vil[0].stock;
    check(ggReignDeed()&&ggReign.credits==cash+4&&
          vil[0].stock==goods+12,
          "trader failed to pay eight and earn twelve from real goods");
    ggReign.frame=ggReign.nextCivicAt;
    ggReign.frame=ggReign.nextCivicAt;
    check(ggReignEnroll(GG_ROLE_ENVOY)&&ggReign.role==GG_ROLE_ENVOY,
          "a thief cannot retrain into civic leadership at a real town");
    check(ggReignDeed()&&vil[0].stock>stock-8,
          "civic relief never reached the simulation's village stores");
    check(ggReignPetition()&&ggReign.endorsements==1,
          "village endorsement was not earned on generated world geography");
    check(!ggReignPetition()&&ggReign.endorsements==1,
          "the same settlement can vote repeatedly for one election");
    ggReign.influence=40;ggReign.legitimacy=40;
    check(ggReignElection()&&ggReign.office==1,
          "legitimate elected councillor office did not unlock");
    check(!ggReignElection(),"higher offices bypassed distinct town support");
    ggExplore.px=70*256+128;ggExplore.py=60*256+128;
    check(ggReignPetition()&&ggReign.endorsements==2,
          "second real town did not record its own endorsement");
    check(ggReignElection()&&ggReign.office==2,
          "mayoral office was not reachable with local civic consent");
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggReign.frame=ggReign.nextCivicAt;
    check(ggReignEnroll(GG_ROLE_FARMER),
          "local farmer opportunity was not available after creation");
    stock=vil[0].stock;
    check(ggReignDeed()&&vil[0].stock==stock+18,
          "farming did not increase the real village food stock");
    ggReign.frame=ggReign.nextCivicAt;
    check(ggReignEnroll(GG_ROLE_SAILOR)&&!ggReignDeed(),
          "inland towns incorrectly offered sea voyages");
    ggChron.foeN=1;
    ggChron.foe[0]=(GgChronFoe){61,60,40,0,0};
    check(ggReignRecruit()&&ggReign.squadN==1 &&
          vil[0].stock>0,"real settlement could not provision a companion");
    check(ggReign.squad[0].x!=61||ggReign.squad[0].y!=60,
          "recruit spawned on a living foe");
    ggChron.foeN=0;
    // A real village structure blocks the straight-line path. The
    // follower must route around it, without walking through houses.
    int gx=ggReign.squad[0].x,gy=ggReign.squad[0].y;
    ggExplore.px=(gx+3)*256+128;
    ggExplore.py=gy*256+128;
    bld[gy][gx+1]=B_HALL;bld[gy][gx+2]=B_HALL;
    int px=0,py=0;
    check(ggReignGuardStep(gx,gy,gx+3,gy,&px,&py)&&
          ggChronLand(px,py)&&
          !(px==gx+1&&py==gy),
          "companion did not find an obstacle-free first step");
    bld[gy][gx+1]=0;bld[gy][gx+2]=0;
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggChron.foeN=1;
    ggChron.foe[0]=(GgChronFoe){61,60,40,0,0};
    check(ggReignDivine(0)&&ggChron.foe[0].hp<=0&&ggReign.divine==80,
          "the native godborn ranged power failed or hit civilians");
    ggChron.foe[0].hp=40;ggReign.order=GG_REIGN_ASSAULT;
    for(int t=0;t<100;t++)ggReignSquadTick();
    check(ggChron.foe[0].hp<40,
          "tactical field squad did not damage an opposing soldier");
    ggReign.squad[0]=(GgReignGuard){61,60,45};
    ggReign.order=GG_REIGN_HOLD;
    ggChron.foe[0]=(GgChronFoe){63,60,4,0,0};
    ggChron.foeWindup[0]=4;ggChron.foeGuard[0]=0;
    u8 guardWall=bld[60][62];
    bld[60][62]=B_TOWER;
    ggReign.frame=11;ggReignSquadTick();
    check(ggChron.foe[0].hp==4,"guard attacked through a tower");
    bld[60][62]=0;
    ggReign.frame=11;ggReignSquadTick();
    check(ggChron.foe[0].hp<=0&&!ggChron.foeWindup[0],
          "guard kill left the foe's attack pending");
    bld[60][62]=guardWall;
    // Foes must recognize followers as real battlefield participants:
    // a nearer guard can absorb attacks, fall and be replaced, while
    // the 2D creature population and player health remain untouched.
    ggChron.foe[0].hp=40;
    ggChron.foe[0].x=62;ggChron.foe[0].y=60;ggChron.foe[0].wait=0;
    ggReign.squad[0].x=61;ggReign.squad[0].y=60;
    ggReign.squad[0].hp=6;ggReign.squadN=1;
    ggExplore.px=64*256+128;ggExplore.py=60*256+128;
    int healthBefore=ggChron.hp;
    ggChronEnemiesStep();
    check(ggReign.squad[0].hp==6&&ggChron.hp==healthBefore,
          "enemy damaged a guard before the windup");
    for(int t=0;t<3;t++){
        ggChronEnemiesStep();
        check(ggReign.squad[0].hp==6,"enemy windup ended too early");
    }
    ggChronEnemiesStep();
    check(ggReign.squad[0].hp<6&&ggChron.hp==healthBefore,
          "enemy completely ignored closer living squad member");
    int guardHealth=ggReign.squad[0].hp;
    ggChronEnemiesStep();
    check(ggReign.squad[0].hp==guardHealth,
          "enemy attacked again while recovering");
    ggChron.foe[0].wait=0;
    ggChronEnemiesStep();
    for(int t=0;t<4;t++)ggChronEnemiesStep();
    check(ggReign.squad[0].hp==0,
          "allies could not be defeated by real enemy AI");
    // Step past sword reach within the same tile, then try a new wall.
    int moraleBefore=ggChron.morale;
    ggChron.foe[0].wait=0;
    ggChronEnemiesStep();
    check(ggChron.hp==healthBefore,"enemy damaged the hero before the windup");
    ggExplore.px+=64;
    for(int t=0;t<4;t++)ggChronEnemiesStep();
    check(ggChron.hp==healthBefore&&ggChron.foe[0].wait>0,
          "enemy hit a hero who moved out of reach or skipped miss recovery");
    ggExplore.px=64*256+128;
    ggChron.foe[0].wait=0;
    ggChronEnemiesStep();
    u8 combatWall=bld[60][63];
    bld[60][63]=B_TOWER;
    for(int t=0;t<4;t++)ggChronEnemiesStep();
    check(ggChron.hp==healthBefore,"enemy hit through a blocking tower");
    bld[60][63]=combatWall;
    ggChron.foe[0].wait=0;
    ggChronEnemiesStep();
    for(int t=0;t<4;t++)ggChronEnemiesStep();
    check(ggChron.hp<healthBefore,"wound-up enemy failed to hit an exposed hero");
    ggChron.hp=healthBefore;
    ggChron.morale=moraleBefore;
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggChron.foeN=0;
    check(ggReignRecruit()&&ggReign.squadN==1&&
          ggReign.squad[0].hp>0,
          "fallen squad member permanently occupied a recruit slot");
    // A nearby follower holds formation rather than orbiting the hero.
    ggChron.foeN=0;ggReign.order=GG_REIGN_FOLLOW;
    int hx=ggReign.squad[0].x,hy=ggReign.squad[0].y;
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggReign.frame=11;ggReignSquadTick();
    check(ggReign.squad[0].x==hx&&ggReign.squad[0].y==hy,
          "FOLLOW formation drifted at close range");
    // A solid row of structures must not make the foe walk into
    // the same wall repeatedly: the route points around an open end.
    for(int yy=59;yy<=62;yy++)bld[yy][61]=B_HALL;
    ggChron.foeN=1;
    ggChron.foe[0]=(GgChronFoe){60,60,40,0,0};
    int rx=0,ry=0;
    check(ggChronEnemyRoute(60,60,63,60,&rx,&ry)&&
          ggChronLand(rx,ry)&&ggChronNear(60,60,rx,ry)==1&&
          !(rx==61&&ry==60),
          "bounded enemy pursuit cannot route around a built wall");
    for(int yy=59;yy<=62;yy++)bld[yy][61]=0;
    GgReignGuard routeGuard=ggReign.squad[0];
    ggReign.squad[0]=(GgReignGuard){61,60,45};
    check(ggChronEnemyRoute(60,60,63,60,&rx,&ry)&&(rx!=61||ry!=60),
          "pursuing foe planned a step into a guard");
    ggReign.squad[0].x=64;
    ggExplore.px=61*256+128;
    check(ggChronEnemyRoute(60,60,63,60,&rx,&ry)&&(rx!=61||ry!=60),
          "foe chasing a guard planned a step into the player");
    ggExplore.px=64*256+128;
    ggChron.foe[0].x=61;
    check(ggReignGuardStep(60,60,63,60,&rx,&ry)&&(rx!=61||ry!=60),
          "advancing guard planned a step into a foe");
    ggReign.squad[0]=routeGuard;
    ggExplore.px=60*256+128;
    ggChron.foeN=0;
    int storyChapter=chapter;
    ggReignStep(KEY_SELECT);
    check(ggReign.menu,"SELECT did not open the independent council");
    ggReignStep(0);ggReignStep(KEY_RIGHT);
    check(ggReign.page==1,"council could not navigate to the war room");
    ggReignStep(0);ggReignStep(KEY_B);
    check(!ggReign.menu&&chapter==storyChapter,
          "side campaign political controls changed the original story");
    remove(GG_REIGN_SAVE_PATH);
    ggReignSave();
    int savedCredits=ggReign.credits,savedOffice=ggReign.office;
    memset(&ggReign,0,sizeof ggReign);
    ggReignLoad();
    check(ggReign.credits==savedCredits&&ggReign.office==savedOffice&&
          ggReign.endorsements==2,
          "godborn class and office progression did not survive saving");
    remove(GG_REIGN_SAVE_PATH);
    ggReign.menu=0;ggReign.noticeT=0;
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggExplore.angle=0;ggChron.active=1;
    bld[60][62]=B_FARM;
    int meshBeforeTown=ggExpMeshEnabled;
    ggExpMeshEnabled=1;
    ggExpRender(guarded.img);
    unsigned long meshFarm=hash();
    ggReignSceneDraw(guarded.img);
    check(ggMeshBuildingsDrawn>0&&ggReignSceneDrawn==0&&hash()==meshFarm,
          "legacy building decals paint over the native settlement");
    ggExpMeshEnabled=0;
    ggExpRender(guarded.img);
    unsigned long bareFarm=hash();
    ggReignSceneDraw(guarded.img);
    check(ggReignSceneDrawn>0&&hash()!=bareFarm,
          "3DS farm decorations are absent from the real settlement");
    ggExpMeshEnabled=meshBeforeTown;
    bld[60][62]=0;
    // Dense districts must not allocate more than the fixed scene cap.
    for(int yy=57;yy<65;yy++)for(int xx=60;xx<68;xx++)
        bld[yy][xx]=(xx+yy)&1?B_HOUSE:B_HALL;
    ggExpRender(guarded.img);
    ggReignSceneDraw(guarded.img);
    check(ggReignSceneDrawn<=GG_REIGN_SCENE_CAP,
          "busy city bypassed the bounded 3DS scene-decoration budget");
    for(int yy=57;yy<65;yy++)for(int xx=60;xx<68;xx++)bld[yy][xx]=0;
    ggChronDraw(guarded.img,NULL,0);
    ggReignDraw(guarded.img);
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "new 3DS civic HUD overflowed its sole allocated frame buffer");
    // The Skyrim-style foundation must be actual conversations and real
    // world-linked quests, not cosmetic text or a fabricated second map.
    const char *questWorldSlot="host-native-slot";
    u8 courierTerrain[35][35],courierBuildings[35][35],courierOwners[35][35];
    remove(GG_QUEST_SAVE_PATH);
    memset(&ggQuest,0,sizeof ggQuest);
    memset(vil,0,sizeof vil);
    vil[0]=(Vil){1,60,60,20};
    vil[1]=(Vil){1,70,60,45};
    worldSeed=1986;
    for(int y=43;y<=77;y++)for(int x=44;x<=78;x++){
        courierTerrain[y-43][x-44]=world[y][x];
        courierBuildings[y-43][x-44]=bld[y][x];
        courierOwners[y-43][x-44]=bldv[y][x];
        world[y][x]=T_GRASS;
        bld[y][x]=bldv[y][x]=0;
    }
    bld[60][60]=bld[60][70]=B_HALL;
    bldv[60][60]=1;bldv[60][70]=2;
    ggChron.active=1;ggChron.won=0;ggChron.lost=0;
    ggChron.foeN=0;ggChron.briefTime=0;
    ggReign.menu=0;ggReign.noticeT=0;
    ggExplore.actorId=0;ggExplore.active=1;
    ggExplore.px=61*256+128;ggExplore.py=60*256+128;
    // The hall blocks one side and the three remaining exits are cliffs.
    world[60][62]=world[59][61]=world[61][61]=T_MOUNT;
    check(ggQuestSource(0)<0,
          "courier assigned a town across an impassable cliff ring");
    world[60][62]=world[59][61]=world[61][61]=T_GRASS;
    for(int y=43;y<=77;y++)for(int x=44;x<66;x++)world[y][x]=T_SNOW;
    check(ggQuestSource(0)<0,"courier assigned a one-way downhill route");
    for(int y=43;y<=77;y++)for(int x=44;x<66;x++)world[y][x]=T_GRASS;
    for(int y=43;y<=77;y++)bld[y][66]=B_WALL;
    bld[60][66]=B_GATE_NS;
    check(ggQuestSource(0)<0,"courier routed sideways through a gate pier");
    bld[60][66]=B_GATE_EW;
    check(ggQuestSource(0)==1,"courier rejected the open gate passage");
    for(int y=43;y<=77;y++)bld[y][66]=B_NONE;
    check(ggQuestSource(0)==1,
          "courier never recovered after removing blocked terrain");
    ggQuestBegin();
    check(ggQuest.stage==GG_QUEST_IDLE&&!ggQuest.talk,
          "travel journal started with a fake completed quest");
    ggChron.active=0;
    ggExplore.angle=128;
    check(ggQuestWalkActionAt(60,60)&&!ggQuestWalkOpen(70,60)&&
          ggQuestWalkOpen(60,60)&&ggQuest.talkVillage==0,
          "walking interaction did not use the actual aimed town hall");
    ggQuest.lastKeys=KEY_A;
    check(ggQuestWalkStep(KEY_A)&&ggQuest.stage==GG_QUEST_IDLE,
          "held interaction skipped the courier menu");
    ggQuestWalkStep(0);ggQuestWalkStep(KEY_A);
    check(ggQuest.stage==GG_QUEST_FETCH&&ggQuest.home==0&&
          ggQuest.target==1,
          "town dialogue failed to assign a reachable real trade route");
    char courierHint[64];
    check(ggQuestWalkHint(courierHint,sizeof courierHint)&&strstr(courierHint,"EASTFORD")&&
          ggMapObjectiveSignature()!=0,
          "walking courier had no named map objective");
    int courierCamX=camX,courierCamY=camY;
    camX=70*8+4-128;camY=60*8+4-80;
    memset(guarded.img,0,sizeof guarded.img);
    ggMapObjectivesDraw(guarded.img);
    check(!memcmp(&guarded.img[(99*320+160)*3],(u8[]){88,221,234},3),
          "walking courier did not draw the shared cyan settlement pin");
    camX=courierCamX;camY=courierCamY;
    ggQuestWalkStep(0);ggQuestWalkStep(KEY_B);
    ggChron.active=1;
    ggQuestBegin();
    ggExplore.px=69*256+128;ggExplore.py=60*256+128;
    int sourceStock=vil[1].stock;
    ggQuestStep(0);ggQuestStep(KEY_A);ggQuestStep(0);ggQuestStep(KEY_A);
    check(ggQuest.stage==GG_QUEST_RETURN&&vil[1].stock==sourceStock-12,
          "collecting supplies did not change the source village stock");
    ggQuestStep(0);ggQuestStep(KEY_B);
    ggQuestWorldSaved(questWorldSlot);
    vil[1].stock=(u16)sourceStock;
    ggQuestWorldReset(questWorldSlot);
    check(ggQuest.stage==GG_QUEST_IDLE,"older native save recreated courier cargo");
    vil[1].stock=(u16)(sourceStock-12);
    ggQuestWorldReset("another-native-slot");
    check(ggQuest.stage==GG_QUEST_IDLE,"another save slot inherited courier cargo");
    worldSeed++;
    ggQuestWorldReset(questWorldSlot);
    check(ggQuest.stage==GG_QUEST_IDLE,"another planet inherited courier cargo");
    worldSeed--;
    ggQuestWorldReset(NULL);
    check(ggQuest.stage==GG_QUEST_IDLE,"a newly created same-seed planet inherited cargo");
    ggQuestWorldReset(questWorldSlot);
    check(ggQuest.stage==GG_QUEST_RETURN&&ggQuest.target==1,
          "quest cargo and destination were not persisted across sessions");
    ggExplore.px=61*256+128;ggExplore.py=60*256+128;
    ggChron.active=0;
    int homeStock=vil[0].stock,oldRenown=ggReign.renown,oldCash=ggReign.credits;
    int homeLoyalty=vilL[0];
    int oldChapter=chapter,oldPost=ggChron.post;
    vil[0].nm1++;
    check(!ggQuestWalkHint(courierHint,sizeof courierHint)&&ggQuestWalkOpen(60,60),
          "a reused settlement slot still displays the old contract");
    ggQuestWalkStep(KEY_A);
    check(vil[0].stock==homeStock&&ggReign.credits==oldCash,
          "a reused settlement slot could receive cargo and pay");
    vil[0].nm1--;
    ggQuestWorldReset(questWorldSlot);
    check(ggQuestWalkOpen(60,60),"could not return through ordinary walking");
    ggQuestWalkStep(KEY_A);
    check(ggQuest.stage==GG_QUEST_IDLE&&vil[0].stock==homeStock+12&&
          ggReign.credits==oldCash&&ggQuest.completed==0,
          "unsaved stock transfer already awarded durable courier pay");
    vil[0].stock=(u16)homeStock;vilL[0]=(s8)homeLoyalty;
    ggQuestWorldReset(questWorldSlot);
    check(ggQuest.stage==GG_QUEST_RETURN&&ggReign.credits==oldCash&&
          ggQuestWalkOpen(60,60),"reload did not restore only the unpaid shipment");
    ggQuestWalkStep(KEY_A);
    ggQuestWorldSaved(questWorldSlot);
    ggQuestWorldSaved(questWorldSlot);
    check(ggQuest.stage==GG_QUEST_IDLE&&ggQuest.completed==1&&
          vil[0].stock==homeStock+12&&ggReign.renown>oldRenown&&
          ggReign.credits==oldCash+16&&chapter==oldChapter&&ggChron.post==oldPost,
          "saving the delivered stock failed to pay exactly once without advancing the main act");
    ggQuestWalkStep(0);ggQuestWalkStep(KEY_A);
    check(ggQuest.stage==GG_QUEST_IDLE&&ggReign.credits==oldCash+16,
          "balanced town stores allow another paid delivery");
    ggQuestWalkStep(0);ggQuestWalkStep(KEY_B);
    ggChron.active=1;
    ggQuestBegin();
    ggChron.rations=2;
    int localFood=vil[0].stock,localTrust=vilL[0];
    ggQuestStep(0);ggQuestStep(KEY_A);
    ggQuestStep(0);ggQuestStep(KEY_DOWN);
    ggQuestStep(0);ggQuestStep(KEY_DOWN);
    check(ggQuest.choice==2,"town relief option cannot be selected");
    ggQuestStep(0);ggQuestStep(KEY_A);
    check(ggChron.rations==1&&vil[0].stock==localFood+8&&
          vilL[0]==localTrust+3,
          "town relief dialogue did not alter real resources and trust");
    ggQuestStep(0);ggQuestStep(KEY_B);
    ggChron.foeN=1;
    ggChron.foe[0]=(GgChronFoe){61,60,30,0,0};
    ggQuestStep(0);
    check(!ggQuestStep(KEY_A)&&!ggQuest.talk,
          "civilian dialogue stole the attack button in combat");
    ggChron.foeN=0;
    ggReign.menu=1;ggReign.page=4;
    ggReignDraw(guarded.img);
    ggReign.menu=0;
    ggQuestDraw(guarded.img);
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "quest journal or town conversation overran 3DS RGB memory");
    GgQuestDiskV1 legacy={0x47515154u,1,GG_QUEST_RETURN,0,1,7,0};
    legacy.checksum=ggQuestChecksum(&legacy,sizeof legacy-sizeof legacy.checksum);
    FILE *questFile=fopen(GG_QUEST_SAVE_PATH,"wb");
    check(questFile&&fwrite(&legacy,sizeof legacy,1,questFile)==1&&fclose(questFile)==0,
          "could not create legacy courier save fixture");
    ggQuestWorldReset(questWorldSlot);
    check(ggQuest.completed==7&&ggQuest.stage==GG_QUEST_IDLE,
          "legacy save failed to keep history while dropping unbound cargo");
    remove(GG_QUEST_SAVE_PATH);
    for(int y=43;y<=77;y++)for(int x=44;x<=78;x++){
        world[y][x]=courierTerrain[y-43][x-44];
        bld[y][x]=courierBuildings[y-43][x-44];
        bldv[y][x]=courierOwners[y-43][x-44];
    }
    // Branching-film regression: all four acts and nine alternate endings
    // must remain within one 400x240 buffer and persist a witnessed ending.
    remove(GG_SAGA_SAVE_PATH);
    ggSaga.loaded=1;ggSaga.decisions=0;ggSaga.decided=0;
    ggSaga.seen=0;ggSaga.mode=0;ggReign.role=GG_ROLE_WAYFINDER;
    ggReign.office=0;ggReign.infamy=0;ggReign.credits=100;
    ggExplore.active=1;ggOrigin.active=0;
    ggChron.briefTime=0;ggChron.foeN=0;
    // Parley consumes real provisions or earned influence; the final
    // captain cannot be removed by a dialogue exploit.
    ggChron.act=0;ggChron.active=1;ggChron.foeN=2;ggChron.rations=3;
    ggChron.foe[0]=(GgChronFoe){61,60,25,0,0};
    ggChron.foe[1]=(GgChronFoe){61,61,25,0,0};
    check(ggSagaParley(0)&&ggChron.foe[0].hp==0&&
          ggChron.foe[1].hp==0&&ggChron.rations==2,
          "caravan rescue cannot resolve using shared emergency rations");
    ggChron.act=3;ggChron.foe[0].hp=105;
    ggChron.foe[0].post=2;
    check(!ggSagaParley(2)&&ggChron.foe[0].hp==105,
          "final campaign captain was deleted by cheap diplomacy");
    ggChron.foeN=0;
    for(int act=0;act<GG_CHRON_ACTS;act++){
        ggChron.act=act;ggChron.active=1;ggChron.won=0;
        ggChron.post=GG_CHRON_POSTS;
        ggSagaStartChoice();
        check(ggSaga.mode==GG_SAGA_CHOICE,
              "chapter ended without a decision reel");
        ggSagaCommitChoice(act==3);
        check(ggChron.won&&ggSaga.decided&(1u<<act),
              "chosen act decision did not complete the mission");
    }
    check(ggSaga.mode==GG_SAGA_OUTRO&&ggSaga.ending>=0&&
          ggSaga.ending<GG_SAGA_ENDINGS&&ggSaga.seen,
          "final campaign choice failed to produce a recorded ending");
    int achieved=ggSaga.ending;
    checkSagaMeshProjection();
    // Film character and architecture must use the same camera, with a
    // stable location, a smooth dolly and exact gameplay camera restore.
    ggChron.act=0;ggSaga.mode=GG_SAGA_INTERLUDE;
    ggSaga.eventX=60;ggSaga.eventY=60;ggSaga.frame=0;
    ggExplore.px=50*256+128;ggExplore.py=50*256+128;
    ggExplore.angle=190;
    int beforePX=ggExplore.px,beforePY=ggExplore.py,beforeAngle=ggExplore.angle;
    int shotX,shotY,ax,ay,projX,projY,projSize;
    ggSagaCamera(0,0,&shotX,&shotY);
    check(shotX==60&&shotY==60&&
          ggChronLand(ggExplore.px>>8,ggExplore.py>>8),
          "story camera began inside a live world obstacle");
    ggSagaActorPosition(shotX,shotY,0,0,&ax,&ay);
    check(ggSagaProjectActor(ax,ay,&projX,&projY,&projSize)&&
          projX>60&&projX<340&&projSize>0,
          "protagonist is not grounded and visible through film camera");
    int filmCameraPX=ggExplore.px,filmAngle=ggExplore.angle;
    ggSagaCamera(0,GG_SAGA_BEAT_TICKS/2,&shotX,&shotY);
    check(ggExplore.angle==filmAngle&&ggExplore.px!=filmCameraPX,
          "shot camera jumps bearings or never eases within a beat");
    // Consecutive dialogue beats share a stable physical set, with a
    // dolly that continues across their boundary instead of rewinding.
    ggSagaCamera(0,GG_SAGA_BEAT_TICKS-1,&shotX,&shotY);
    int endFirstPX=ggExplore.px,endFirstPY=ggExplore.py;
    ggSagaCamera(1,0,&shotX,&shotY);
    check(ggExplore.angle==filmAngle&&
          abs(ggExplore.px-endFirstPX)<24&&
          abs(ggExplore.py-endFirstPY)<24,
          "continuous dialogue scene teleported the camera at its cut");
    int sharedStageX=shotX,sharedStageY=shotY;
    ggSagaCamera(1,40,&shotX,&shotY);
    check(shotX==sharedStageX&&shotY==sharedStageY,
          "two-line conversation jumped between unrelated settlements");
    // A known building/occupied landmark is never used as the ground
    // under a character; the shoot remains in this actual district.
    bld[60][60]=B_HALL;
    ggSagaCamera(2,0,&shotX,&shotY);
    check(ggChronLand(shotX,shotY)&&
          ggChronNear(shotX,shotY,60,60)<=3,
          "cinematic actor stood inside a real building");
    bld[60][60]=0;
    ggExplore.px=beforePX;ggExplore.py=beforePY;
    ggExplore.angle=beforeAngle;
    ggSagaDraw(guarded.img);
    check(ggExplore.px==beforePX&&ggExplore.py==beforePY&&
          ggExplore.angle==beforeAngle,
          "cinematic mutated the live first-person camera on return");
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "film camera/protagonist overran the display buffer");
    check(GG_AMBIENT_MIRACLE+1==GG_AMBIENT_N,
          "ambient scene IDs no longer match scripted event count");
    // Recurring speakers must have their own identity and must stand
    // on visible real-world terrain, not at arbitrary screen offsets.
    ggSagaCamera(0,0,&shotX,&shotY);
    check(ggSagaGuestStyle("MIRA")==0&&
          ggSagaGuestStyle("VARRO")==2&&
          ggSagaGuestStyle("NARRATOR")==-1,
          "film director lost stable recurring speaker identities");
    int beforeFilmMode=ggSaga.mode,beforeFilmAct=ggChron.act;
    ggSaga.mode=GG_SAGA_OPEN;ggChron.act=0;
    check(ggSagaConversationGuest(0)==0&&
          ggSagaConversationGuest(1)==0,
          "Mira vanished while the narrator continued her scene");
    int guestA,guestB;
    ggSagaPairGuests(2,&guestA,&guestB);
    check(guestA==0&&guestB==2,
          "Mira and Varro were not staged as two distinct actors");
    int otherA,otherB;
    ggSagaPairGuests(3,&otherA,&otherB);
    check(otherA==guestA&&otherB==guestB&&
          ggSagaConversationGuest(3)==0,
          "paired dialogue swapped the cast identity halfway through");
    ggSaga.mode=beforeFilmMode;ggChron.act=beforeFilmAct;
    int speakerX,speakerY;
    check(ggSagaGuestTile(shotX,shotY,&speakerX,&speakerY)&&
          ggChronLand(speakerX>>8,speakerY>>8),
          "cinematic supporting actor cannot occupy a grounded tile");
    // Staging is frozen across a take even if an unrelated simulation
    // update briefly marks the chosen cell occupied.
    int cachedGuestX=speakerX,cachedGuestY=speakerY;
    int occupiedX=speakerX>>8,occupiedY=speakerY>>8;
    unsigned short oldOcc=occ[occupiedY][occupiedX];
    occ[occupiedY][occupiedX]=1;
    check(ggSagaGuestTile(shotX,shotY,&speakerX,&speakerY)&&
          speakerX==cachedGuestX&&speakerY==cachedGuestY,
          "a moving crowd cell teleported the film actor mid-shot");
    occ[occupiedY][occupiedX]=oldOcc;
    memset(guarded.img,0,sizeof guarded.img);
    ggSagaGuestPortrait(guarded.img,200,150,2,0,1);
    unsigned long miraPicture=hash();
    memset(guarded.img,0,sizeof guarded.img);
    ggSagaGuestPortrait(guarded.img,200,150,2,2,1);
    check(hash()!=miraPicture,
          "distinct named film cast render with indistinguishable colours");
    check(ggSagaPropKind()==GG_FILM_PROP_CRATES,
          "local town film failed to select a physical community prop");
    memset(guarded.img,0,sizeof guarded.img);
    ggSagaPropDraw(guarded.img,195,180,2,GG_FILM_PROP_CART);
    unsigned long wagonPicture=hash();
    memset(guarded.img,0,sizeof guarded.img);
    ggSagaPropDraw(guarded.img,195,180,2,GG_FILM_PROP_RECORDS);
    check(hash()!=wagonPicture,
          "wagon and records used identical cinematic props");
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "named cast drew outside the one existing RGB framebuffer");
    // Film performances must be visibly different while retaining
    // stable identities, deterministic poses and framebuffer guards.
    GgFilmPose idle=ggFilmPoseAt("NARRATOR","",1,40,0,0,
                                GG_FILM_PROP_RECORDS);
    GgFilmPose speaking=ggFilmPoseAt("MIRA",
                "The names and records belong to these families.",
                1,38,0,0,GG_FILM_PROP_RECORDS);
    GgFilmPose anticip=ggFilmPoseAt("MIRA",
                "The names and records belong to these families.",
                1,1,0,0,GG_FILM_PROP_RECORDS);
    check(idle.pose==GG_FILM_LISTEN&&
          speaking.pose==GG_FILM_PRESENT&&
          speaking.armR>anticip.armR&&speaking.holdProp,
          "cinematic key poses did not anticipate, present and hold");
    GgFilmPose walkingA=ggFilmPoseAt("YOU","",0,0,1,1,
                                    GG_FILM_PROP_CART);
    GgFilmPose walkingB=ggFilmPoseAt("YOU","",0,12,1,1,
                                    GG_FILM_PROP_CART);
    check(walkingA.pose==GG_FILM_WALK&&
          walkingA.legL!=walkingB.legL&&
          walkingA.legR!=walkingB.legR,
          "4-phase walk did not alternate the leading leg");
    int endingX,endingY,nextX,nextY;
    ggSagaActorPosition(60,60,2,GG_SAGA_BEAT_TICKS-1,
                        &endingX,&endingY);
    ggSagaActorPosition(60,60,3,0,&nextX,&nextY);
    check(abs(endingX-nextX)<=2&&abs(endingY-nextY)<=2,
          "hero snapped backward when the dialogue beat changed");
    GgFilmPose listener=ggFilmPoseFor("Follow the soldiers",
                       2,28,0,0,0,GG_FILM_PROP_BANNER);
    GgFilmPose speaker=ggFilmPoseFor("Follow the soldiers",
                       2,28,0,1,0,GG_FILM_PROP_BANNER);
    check(listener.pose==GG_FILM_LISTEN&&listener.mouth==0&&
          speaker.pose!=GG_FILM_LISTEN&&speaker.mouth>0,
          "listening character mouths the other actor's speech");
    GgFilmPose talk=ggFilmPoseAt("MIRA","Save the families",
                                2,28,0,0,GG_FILM_PROP_CART);
    check(talk.mouth!=0&&
          ggFilmPoseAt("MIRA","Save the families",
                       2,28,0,0,GG_FILM_PROP_CART).mouth==talk.mouth,
          "mouth timing is not stable for cinematic dialogue");
    memset(guarded.img,0,sizeof guarded.img);
    ggFilmDrawActor(guarded.img,190,160,3,
                    ggFilmGuestPalette[0][0],
                    ggFilmGuestPalette[0][1],
                    ggFilmGuestPalette[0][2],&speaking);
    unsigned long speakingPicture=hash();
    memset(guarded.img,0,sizeof guarded.img);
    ggFilmDrawActor(guarded.img,190,160,3,
                    ggFilmGuestPalette[0][0],
                    ggFilmGuestPalette[0][1],
                    ggFilmGuestPalette[0][2],&idle);
    check(hash()!=speakingPicture,
          "the keyed cinematic gestures rendered like static mannequins");
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "layered film actor touched memory outside 3DS framebuffer");
    // User can pause lengthy subtitles, advance one beat while paused,
    // then resume without skipping or softlocking the 3D story.
    ggSaga.mode=GG_SAGA_OPEN;ggSaga.paused=0;ggSaga.frame=12;
    ggSaga.lastKeys=0;
    check(ggSagaStep(KEY_X)==1&&ggSaga.paused&&ggSaga.frame==12,
          "X did not pause the 3DS cinematic playback");
    ggSagaStep(0);ggSagaStep(0);
    check(ggSaga.frame==12,
          "paused dialogue advanced before the player could read it");
    ggSagaStep(KEY_A);
    check(ggSaga.paused&&ggSaga.frame==GG_SAGA_BEAT_TICKS,
          "A did not advance precisely one movie beat during pause");
    ggSagaStep(0);ggSagaStep(KEY_X);
    check(!ggSaga.paused,"cinematic X resume did not restore playback");
    ggSagaStep(0);ggSagaStep(KEY_B);
    check(ggSaga.mode==GG_SAGA_IDLE,
          "skipping a paused scene failed to return to world");
    ggSaga.mode=GG_SAGA_OUTRO;ggSaga.ending=achieved;
    ggSaga.frame=0;ggSaga.paused=0;
    ggSagaDraw(guarded.img);
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "branching 3D film overran the 3DS video buffer");
    ggSagaSave();
    memset(&ggSaga,0,sizeof ggSaga);
    ggSagaLoad();
    check(ggSaga.seen&(1u<<achieved),
          "ending collection did not survive an independent save reload");
    for(int e=0;e<GG_SAGA_ENDINGS;e++){
        ggSaga.mode=GG_SAGA_OUTRO;ggSaga.ending=e;
        ggSaga.frame=0;ggSagaDraw(guarded.img);
        ggSaga.frame=GG_SAGA_BEAT_TICKS*3;
        ggSagaDraw(guarded.img);
        check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
              "alternative 3D ending overran the framebuffer");
    }
    ggSaga.mode=0;
    ggReign.page=5;ggReign.menu=1;
    ggReignDraw(guarded.img);
    ggReign.menu=0;
    remove(GG_SAGA_SAVE_PATH);
    // Nonlinear 3D world stories happen when a relevant REAL map feature
    // is first discovered. Skipping is safe, flags persist, and no
    // second framebuffer is allocated for the additional cinematics.
    remove(GG_AMBIENT_SAVE_PATH);
    memset(&ggAmbient,0,sizeof ggAmbient);
    ggAmbientBegin();
    ggAmbient.cooldown=0; // host jumps straight to a discovered town
    ggChron.active=1;ggChron.won=0;ggChron.lost=0;
    ggChron.briefTime=0;ggChron.foeN=0;ggChron.frame=64;
    ggReign.role=GG_ROLE_UNBOUND;ggReign.office=0;ggReign.squadN=0;
    ggSaga.mode=GG_SAGA_IDLE;ggOrigin.active=0;ggQuest.talk=0;
    ggReign.menu=0;ggQuest.completed=0;
    vil[0]=(Vil){1,60,60,125};
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    check(ggAmbientTick()&&ggSaga.mode==GG_SAGA_INTERLUDE&&
          ggSaga.event==GG_AMBIENT_TOWN,
          "ordinary village discovery did not trigger a location film");
    ggSagaDraw(guarded.img);
    check(guarded.first[0]==0x5a&&guarded.last[47]==0xc3,
          "nonlinear 3D cinematic overran native framebuffer");
    check(ggSagaStep(KEY_B)==1&&ggSaga.mode==GG_SAGA_IDLE,
          "optional town film was not skippable to the open world");
    u32 encountered=ggAmbient.seen;
    memset(&ggAmbient,0,sizeof ggAmbient);
    ggAmbientBegin();
    check((ggAmbient.seen&encountered)==encountered,
          "visited world scenes did not persist between sessions");
    remove(GG_AMBIENT_SAVE_PATH);
    // Repeat on moving viewpoints, not just one flat frame. Host timings
    // are diagnostics; they are not a claim of Old/New 3DS hardware FPS.
    clock_t benchStart=clock();
    for(int f=0;f<36;f++){
        ggExplore.angle=(f*7)&255;ggExplore.frame=f*2;
        ggExpRender(guarded.img);
    }
    double benchMs=1000.0*(clock()-benchStart)/CLOCKS_PER_SEC/36.0;
    printf("EXPLORE3D render diagnostic: 36 camera frames, %.3f ms/frame (host CPU)\n",benchMs);
    double elapsed=1000.0*(clock()-started)/CLOCKS_PER_SEC;
    printf("EXPLORE3D OK: hybrid minds, stable IDs, selection, 2D/3D "
           "actor movement, protected homes, safe frame buffers; "
           "4 host renders %.1f ms total\n",elapsed);
    return 0;
}
