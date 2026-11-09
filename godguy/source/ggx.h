// ggx.h - GODGUY world parameters (the WORLD menu), per-city policies, per-people customisation and a few extra counters.
// Included from main.c near the top, before the world generator, so the simulation can read it anywhere.
// Everything lives in one small block of bytes (gx) so the save file can carry it as one trailer ("GGX1"): old saves load with the defaults,
// and a save made here still loads in an older build (the trailer is simply never read). Integer maths only.
#define GX_V 40                       // = VIL_N (one slot per city)
typedef struct {
    u8 mon, birth, hunger, life, dis, war, wx, tm, gfx, pad[7];   // monsters (0 few, 1 normal, 2 many), birth rate, hunger, lifespan (0 low, 1 normal, 2 high), disasters (0 off, 1 rare, 2 normal),
                                                                   // wars (0 never, 1 allowed), weather lock (0 auto, 1 clear, 2 rain, 3 storm, 4 snow), time lock (0 auto, 1 day, 2 dawn, 3 dusk, 4 night), graphics (1 rich, 0 classic)
    u8 rBirth[8], rVigor[8], rTemper[8], rLife[8];                 // per people: birth rate, toughness, temper (0 peaceful, 1 as it was, 2 warlike), lifespan
    u8 vGrow[GX_V], vDef[GX_V];                                    // per city: growth (0 normal, 1 boom, 2 slow), defence (0 normal, 1 pacifist, 2 fortified)
} GgX;
static GgX gx;
static u32 stBorn, stDied;            // births and deaths since the world began
static void gxDefaults(void){ memset(&gx,0,sizeof gx); gx.mon=1; gx.birth=1; gx.hunger=1; gx.life=1; gx.dis=2; gx.war=1; gx.gfx=1;
    for(int i=0;i<8;i++){ gx.rBirth[i]=1; gx.rVigor[i]=1; gx.rTemper[i]=1; gx.rLife[i]=1; } stBorn=stDied=0; }
static int gxPct(int v,int a,int b,int c){ return v==0?a:v==1?b:c; }
static int gxBirthK(int arch,int diet){ if(diet==5) return 100; int k=gxPct(gx.birth,60,100,160); if(arch<8) k=k*gxPct(gx.rBirth[arch],60,100,160)/100; return k; }
static int gxLifeK(int arch){ int k=gxPct(gx.life,70,100,150); if(arch<8) k=k*gxPct(gx.rLife[arch],70,100,150)/100; return k; }
static int gxVigorK(int arch){ return arch<8?gxPct(gx.rVigor[arch],75,100,135):100; }
static int gxTemper(int race){ return race<8?gx.rTemper[race]:1; }
static int gxTimeP(int p){ switch(gx.tm){ case 1: return 1023; case 2: return 300; case 3: return 1747; case 4: return 0; default: return p; } }   // the position in the day (0 midnight .. 1023 noon .. 2047)
static void gxVilReset(int v){ if(v>=0&&v<GX_V){ gx.vGrow[v]=0; gx.vDef[v]=0; } }
