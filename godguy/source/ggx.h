// ggx.h - GODGUY world parameters (the WORLD menu), per-city policies, per-people customisation and a few extra counters.
// Included from main.c near the top, before the world generator, so the simulation can read it anywhere.
// Everything lives in one small block of bytes (gx) so the save file can carry it as one trailer ("GGX1"): old saves load with the defaults,
// and a save made here still loads in an older build (the trailer is simply never read). Integer maths only.
#define GX_V VIL_N                    // one slot per city (the save trailer keeps its own count, so builds with other village counts still read each other's saves)
typedef struct {
    u8 mon, birth, hunger, life, dis, war, wx, tm, gfx, lAge, lPlague, lFire, lLava, lStorm, lBio, lAtmo;   // monsters (0 few, 1 normal, 2 many), birth rate, hunger, lifespan (0 low, 1 normal, 2 high), disasters (0 off, 1 rare, 2 normal),
                                                                   // wars (0 never, 1 allowed), weather lock (0 auto, 1 clear, 2 rain, 3 storm, 4 snow), time lock (0 auto, 1 day, 2 dawn, 3 dusk, 4 night), graphics (1 rich, 0 classic),
                                                                   // the WORLD LAWS (0 is always how the game was before): lAge old age (0 on, 1 off), lPlague (0 normal, 1 off, 2 deadly), lFire fire on homes (0 burns, 1 safe), lLava lava creep (0 on, 1 off), lStorm tornado / acid / black hole on homes (0 hit, 1 spare)
    u8 rBirth[8], rVigor[8], rTemper[8], rLife[8];                 // per people: birth rate, toughness, temper (0 peaceful, 1 as it was, 2 warlike), lifespan
    u8 vGrow[GX_V], vDef[GX_V];                                    // per city: growth (0 normal, 1 boom, 2 slow), defence (0 normal, 1 pacifist, 2 fortified)
} GgX;
static GgX gx;
// WORLD LAWS (port step 5, from the old line's 20 laws). The older laws live in gx (lAge, lPlague, lFire, lLava, lStorm, lBio, lAtmo); the rest are bytes in lw[]:
// lw[i]!=0 means law i differs from its default (0 is always "as before"), saved in the GGL1 trailer, so an old save loads with every default.
enum { LW_HUNGER, LW_CIVBABY, LW_ANIBABY, LW_PEACEMON, LW_MUTBOX, LW_MUTATE, LW_EROSION, LW_TREES, LW_WEATHER, LW_GAIA, LW_DIPLOMACY, LW_REBEL, LW_GROWTH, LW_POP100, LW_CROWD, LW_LIMIT, LW_NATDIS, LW_OTHERDIS, LW_SUCCESSION, LW_KINGSTATS, LW_SPAGHETTI, LW_SNOW, LW_N };
static u8 lw[LW_N+2];
#define LAWDEFON (((1u<<LW_N)-1)&~((1u<<LW_PEACEMON)|(1u<<LW_MUTBOX)|(1u<<LW_GAIA)|(1u<<LW_POP100)|(1u<<LW_SPAGHETTI)|(1u<<LW_SNOW)))     // off by default, like WorldBox: peaceful monsters, mutant box, Gaia's covenant, 100 people
#define LAWON(i) (((LAWDEFON>>(i))&1u)^(u32)(lw[i]&1))
#define GX_HEAD ((int)__builtin_offsetof(GgX,vGrow))      // the bytes before the per-city tables: the same in every build
static u32 stBorn, stDied;            // births and deaths since the world began
static void gxDefaults(void){ memset(&gx,0,sizeof gx); memset(lw,0,sizeof lw); gx.mon=1; gx.birth=1; gx.hunger=1; gx.life=1; gx.dis=2; gx.war=1; gx.gfx=1;
    for(int i=0;i<8;i++){ gx.rBirth[i]=1; gx.rVigor[i]=1; gx.rTemper[i]=1; gx.rLife[i]=1; } stBorn=stDied=0; }
static int gxPct(int v,int a,int b,int c){ return v==0?a:v==1?b:c; }
static int gxBirthK(int arch,int diet){ if(diet==5) return 100; int k=gxPct(gx.birth,60,100,160); if(arch<8) k=k*gxPct(gx.rBirth[arch],60,100,160)/100; return k; }
static int gxLifeK(int arch){ int k=gxPct(gx.life,70,100,150); if(arch<8) k=k*gxPct(gx.rLife[arch],70,100,150)/100; return k; }
static int gxVigorK(int arch){ return arch<8?gxPct(gx.rVigor[arch],75,100,135):100; }
static int gxTemper(int race){ return race<8?gx.rTemper[race]:1; }
static int gxTimeP(int p){ switch(gx.tm){ case 1: return 1023; case 2: return 300; case 3: return 1747; case 4: return 0; default: return p; } }   // the position in the day (0 midnight .. 1023 noon .. 2047)
static void gxVilReset(int v){ if(v>=0&&v<GX_V){ gx.vGrow[v]=0; gx.vDef[v]=0; } }
