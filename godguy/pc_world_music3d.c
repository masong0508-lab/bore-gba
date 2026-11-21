// SDK-free dynamic score director checks. Reuses the same fake villages,
// creature grids, real terrain and 3DS story structs as the gameplay
// regression. Backend calls are stubs: assert decisions, NOT audio quality.
#define main ggWorldMusicGameplayTests
#include "pc_explore3d.c"
#undef main
// Minimal IDs mirroring the existing 168-song gmdata.h catalogue.
enum { GM_MORNING=6,GM_NIGHT=9,GM_JUNGLE=14,GM_OCEAN=13,
       GM_VILLAGE=17,GM_CITY=19,GM_WAR=22,GM_SIEGE=23,
       GM_MONSTER=24,GM_ASCEND=30,GM_GODHOOD=32,GM_GAMEOVER=35,
       GM_GENESIS=38,GM_FIRSTSUN=39,GM_GLASSRIVER=41,
       GM_MTNKING=43,GM_CARAVAN=44,GM_HEARTH=45,
       GM_SAILS=46,GM_MIDNIGHTP=48,GM_LONGWINTER=49,
       GM_FIELDSGOLD=51,GM_GLASSCITY=55,GM_OCEANKEEPS=56,
       GM_BLOODMOON=57,GM_LASTBEACON=58,GM_ASHES=59,
       GM_DREAMINGDEEP=61,GM_CROWNTHORNS=62,
       GM_LANTERNRIVER=63,GM_GREATCLOCK=66,GM_ALLKINGS=67,
       GM_HOMEWORLD=68,GM_STONECIRCLES=72,GM_COUNTINGSTARS=77,
       GM_CORPORATIONS=82,GM_DUCKCOVER=83 };
static int gmCur=-1,gmWorldMode,gmWorldGain=100,gmInN,gmInT;
static int musLast,musHold,gmRow,playCalls,stopped,musQ=-1;
static int musDirector(void){return GM_MORNING;}
static void musReq(int id){musQ=id;}
static int (*gg3dMusicNext)(int);
static void gmPlay(int x){gmCur=x;playCalls++;}
static void gmStop(void){gmCur=-1;stopped++;}
static void gmFilmMixSet(int a,int b,int c){(void)a;(void)b;(void)c;}
static void gmWorldOff(void){gmWorldMode=0;gmWorldGain=100;}
static void gmWorldSet(int gain,int drums,int motif,int patch,
                       const unsigned char *stem){
    (void)drums;(void)motif;(void)patch;(void)stem;
    gmWorldMode=1;gmWorldGain=gain;
}
#include "3ds/world_music3d.h"
int main(void){
    memset(world,T_GRASS,sizeof world);
    ggExplore.active=1;
    ggExplore.px=60*256+128;ggExplore.py=60*256+128;
    ggExplore.angle=0;
    ggChron.active=1;ggChron.act=0;ggChron.foeN=0;
    ggChron.briefTime=0;ggChron.hp=100;ggChron.day=0;
    ggOrigin.active=0;ggSaga.mode=0;ggReign.menu=0;
    ggQuest.talk=0;
    ggWorldMusicTick();
    if(!gmWorldMode||gmCur<0||playCalls!=1)return 2;
    int first=gmCur;
    // Crossing 50 tiles cannot thrash / restart a song 50 times.
    for(int f=0;f<240;f++){
        ggExplore.px=(59+(f&1))*256+128;
        ggWorldMusicTick();
    }
    if(playCalls>2)return 3;
    // Quest completion gains a short fanfare without allocating a new
    // audio channel; it cannot override dialogue or a final boss.
    ggQuest.completed++;
    ggWorldMusicTick();
    if(ggWorldMusic.celebrate<=0)return 11;
    // Nearby foes immediately change atmosphere, then do not restart
    // every frame of combat.
    ggChron.foeN=1;
    ggChron.foe[0]=(GgChronFoe){60,60,40,0,0};
    for(int f=0;f<200;f++)ggWorldMusicTick();
    if(ggWorldMusic.state!=GG_MUSIC_BATTLE||gmCur<0)return 4;
    int fight=playCalls;
    for(int f=0;f<100;f++)ggWorldMusicTick();
    if(playCalls!=fight)return 5;
    // Both landmark songs are reachable from actual world context.
    ggChron.foeN=0;ggChron.act=2;
    ggReign.role=GG_ROLE_TRADER;
    vil[0]=(Vil){1,60,60,100};vil[0].lvl=3;
    ggWorldMusic.state=GG_MUSIC_CITY;
    if(ggWorldMusicTrack(GG_MUSIC_CITY)!=GM_CORPORATIONS)return 6;
    ggReign.role=GG_ROLE_SOLDIER;
    if(ggWorldMusicTrack(GG_MUSIC_BATTLE)!=GM_DUCKCOVER)return 7;
    ggChron.act=3;ggChron.post=2;
    ggChron.foeN=1;ggChron.foe[0]=(GgChronFoe){60,61,105,2,0};
    if(ggWorldMusicScene()!=GG_MUSIC_BOSS)return 8;
    ggSaga.mode=GG_SAGA_CHOICE;
    if(ggWorldMusicScene()!=GG_MUSIC_CHOICE)return 9;
    ggSaga.mode=0;ggExplore.active=0;
    ggWorldMusicTick();
    if(gmWorldMode||gg3dMusicNext||gmCur!=-1)return 10;
    printf("WORLD SCORE OK: %d music starts, exploration %d, battle, "
           "cities, two vocal songs, films, cleanup\n",playCalls,first);
    return 0;
}
