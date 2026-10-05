// house.h - BORE households: up to 14 Sims living together. You control one (the player, main.c's life globals); the others look after
// themselves with FREE WILL. SELECT (tap) in the life game switches who you control.
// Include AFTER faceView / rotPos / tileH / lifeMap / bakeSprites / blit / menu / toast and BEFORE drawRoomRect.
//
//  MEMBERS   hhM[0..hhN-1] are the Sims you are NOT controlling. Each has a look and persona (like the creator's), a stage, a position, its
//            own needs (FOOD WC REST CLEAN COMFY FUN) and an action. Their sprites are baked into hhSpr[] (11 KB each, EWRAM).
//  FREE WILL every so often a member scores what it could do: each kind of furniture ADVERTISES a need (fridge FOOD, toilet WC, bed REST,
//            shower CLEAN, sofa COMFY) and skating about advertises FUN. score = how low the need is (squared, so urgent wins) x the trait
//            tilt, minus the walk. It picks one of the two best at random, finds a path (BFS on the 40x40 tiles, one member plans per
//            step), walks next to the furniture and uses it. With nothing pressing it wanders. OPTIONS > PLAY > FREE WILL: OFF / LOW / HIGH.
//  SWITCH    SELECT swaps the player with the next member: position, needs, look, persona and sprites change places. (The aspiration
//            meter, wants, job and cash belong to the household for now: they stay with whoever you control.)
//  FAMILIES  premade households (original characters) move in from the pause menu (HOUSEHOLD). Saved in SRAM at HH_OFF.
//
// TUNING
#define HH_MAX     7       // members besides the player: 8 Sims in all, as in The Sims 2. Only the Sims on screen hold a hardware sprite (OBJ slots,
                           // below); each place costs about 5.6 KB of EWRAM. (Households saved as 'H6'..'H9' held up to 13 members: hhLoad keeps 7.)
#define HH_MAX9    13      // members an 'H6'..'H9' household could hold (14 uids)
#define HH_THINK   90      // steps between a member's decisions (FREE WILL HIGH; LOW thinks half as often and lets needs sink lower)
#define HH_PATH    96      // longest path a member remembers (steps between tiles)
#define HH_USE     240     // steps a member spends using a piece of furniture
#define HH_OFF     SL_HH_OFF   // SRAM: the household (slots.h keeps the map of SRAM)
#define HH_MAXOLD  9           // households saved before 'H6' kept their relationships for 10 uids
enum { HA_IDLE, HA_WALK, HA_USE, HA_WANDER, HA_SEEK, HA_SOC, HA_LEAVE, HA_AWAY };   // SEEK: walking to someone to talk to; SOC: in a conversation; LEAVE: off to work or school; AWAY: off the lot
enum { HN_FOOD, HN_WC, HN_REST, HN_CLEAN, HN_COMFY, HN_FUN, HN_SOC, HN_N };
static const char hnFurn[HN_N]={'F','T','S','H','C',0,0};   // what each need's furniture is (FUN: skate about; SOCIAL: find someone)
#define HH_NM 12   // a first or a last name: up to 11 characters (any the font has: capitals, lowercase, digits, symbols)
typedef struct {
    u8 look[LK_N], stage, asp, ltw, tr[TR_N];
    char name[HH_NM], last[HH_NM];   // first and last name
    s32 fx, fy;              // position in 1/256 tiles (like lfx, lfy)
    u8 hd, need[HN_N];       // heading (16 steps), needs 0..100 (WC here is 100 = empty bladder, like every other need: high is good)
    u8 act, use, pn, pi;     // action, need being refilled, path length, step along it
    u8 gok; s32 gx, gy;      // the tile centre this step walks to (fixed when the step starts)
    u8 hp;                   // health 0..100 (not saved: everyone comes back at 100). Punches take it, it creeps back (hhTick)
    u8 uid, tgt, bub, bubT;  // who this is (relationships are kept by uid), who it is going to talk to (uid), balloon icon and time
    short think, t;          // steps to the next decision, steps left in the action
    u8 path[HH_PATH];        // directions: 0 +x, 1 +y, 2 -x, 3 -y
} HhSim;
static HhSim hhM[HH_MAX] EWRAM_BSS; static int hhN;
// ---- relationships (Sims 2 style): for every pair a DAILY and a LIFETIME score, -100..100, kept by uid and one-way (how a feels about b) ----
#define HU_N (HH_MAX+1)
static signed char relD[HU_N][HU_N], relL[HU_N][HU_N]; static u8 relF[HU_N][HU_N];
enum { RF_CRUSH=1, RF_LOVE=2, RF_STEADY=4, RF_KISSED=8, RF_FRIEND=16, RF_BFF=32, RF_ENEMY=64 };   // FRIEND/BFF/ENEMY: remembered so they fire once
static int hhPUid;                         // the uid of the Sim you control
static char hhPName[HH_NM]="YOU", hhPLast[HH_NM]="";   // the first and last name of the Sim you control (premade Sims bring theirs)
static u8 hhBubT; static const char* hhBubTxt;   // the word over your head during a social (shown by hud.h's bubble)
// HARDWARE SPRITES: the members are GBA OBJ sprites (32x64, 16 colours each), so moving them costs no drawing: the CPU only draws their
// shadow and talk balloons into the room. Their 4 views are baked like the player's, cut down to 15 colours + clear (hhQuant), and kept
// as 4bpp tiles; each frame the view being shown goes into OBJ VRAM (1 KB a member) and OAM says where (hhObjUpdate, in vblank).
// A window keeps them inside the room view (never over the HUD); menus hide them (box() -> objHideAll). Sprites always sit on top of the
// picture, so a member standing behind a full-height wall is drawn see-through instead (the "x-ray" blend).
#define OBJ_VRAM ((volatile u16*)0x06014000)   // OBJ tiles 512.. in the bitmap modes
#define OBJ_PAL  ((volatile u16*)0x05000200)
#define OAM      ((volatile u16*)0x07000000)
#define OBJ_B 1024                               // one view: 32 wide x 64 high (tile rows 0..7; the bake is 60 high) x 4bpp = 32 tiles, in 1D order
static u8 hhObj[HH_MAX][4][OBJ_B] EWRAM_BSS;    // 4 views
#define STR_B0 128                               // the stride frame: tile rows 1..7 of each view (bytes 128..1023), the rest is as standing
#define STR_BN 896
static u8 hhObjS[HH_MAX][4][STR_BN] EWRAM_BSS;
#define TW_N 2                                   // passers-by (townies): OAM, OBJ VRAM and palettes after the members'
#define TW_V(k) (HH_MAX-1-(k))                    // VISITORS (the old passers-by): visitor k lives in the empty member place TW_V(k) - its HhSim, sprites
                                                 // and palette are that place's - so they take no RAM of their own and only come while the house has room
static u16 hhPal[HH_MAX][16];                  // a palette per member (index 0 = clear)
// (the 16-bit bake of a member on its way to 4bpp uses the player's own spr4 / spr4s: they are baked again right after)
static u16 hhDist[MH*MW] EWRAM_BSS;
#define hhQ bfsQ   // (main.c's shared search queue)   // BFS scratch, shared (one member plans per step)
static int hhPlanNext;   // round robin: whose turn it is to plan
static const signed char hhDx[4]={1,0,-1,0}, hhDy[4]={0,1,0,-1};

// ---- OBJ slots: a Sim only holds sprite memory, an OAM entry pair and a palette while it is on screen ----
// OBJ VRAM is 16 KB in the bitmap modes and there are 16 OBJ palettes, so there are 16 slots: slot s = tiles 512+24*s.. (24 tiles, the 32x48 a Sim
// really uses) and palette s. hhObjUpdate hands slots to the Sims in view (nearest the middle of the screen first) and takes them back when a Sim
// walks off, goes to work or school, or a passer-by leaves the map. Ids: members 0..HH_MAX-1, then the passers-by.
#define OBJ_SLOTS 8   // (a household is 8 Sims: never more than 7 sprites on screen)
#define HH_IDS    HH_MAX
#define UP_BUDGET 5                             // fresh sprite uploads per vblank (a full one is ~700 halfword writes); the rest wait a frame
static signed char hhSlotOf[HH_IDS], hhSlotId[OBJ_SLOTS], hhSlotKey[OBJ_SLOTS];   // id -> slot, slot -> id, view*2+frame in the slot (-1: tiles not loaded)
static u8 hhSlotOk;
static void hhSlotsFree(void){ for(int i=0;i<HH_IDS;i++) hhSlotOf[i]=-1; for(int s=0;s<OBJ_SLOTS;s++){ hhSlotId[s]=-1; hhSlotKey[s]=-1; } hhSlotOk=1; }   // after anything that changes the baked sprites or who is who
static inline const u8* hhTiles(int id,int v){ return hhObj[id][v]; }
static inline const u8* hhStrideB(int id,int v){ return hhObjS[id][v]; }
static inline const u16* hhPalOf(int id){ return hhPal[id]; }

// ---- premade families (original characters) ----
typedef struct { const char* name; u8 look[LK_N]; u8 stage, asp, sign; } HhPre;   // the whole look (the first families only set the base picks); traits come from a sign
typedef struct { const char* fam; u8 n; HhPre m[4]; } HhFam;
//                     look: SHAPE SKIN EYES MOUTH EARS HSTYLE HCOL TOP BOT  TONE EARSZ EARLF
static const HhFam hhFams[]={
    {"THE GRINDERS",3,{ {"REX", {5,2,2,1,1,0,0,1,1, 0,0,0},AG_ADULT,AS_POP,  0},
                        {"DEE", {4,1,1,1,1,2,3,3,0, 0,0,0},AG_ADULT,AS_FORTUNE,9},
                        {"PIP", {0,1,2,2,2,1,3,2,5, 0,0,0},AG_CHILD,AS_GROW,  2} }},
    {"THE MIDNIGHTS",3,{ {"MORTIMER",{6,0,0,0,0,2,0,7,1, 4,0,0},AG_ADULT,AS_KNOW, 5},
                        {"VESPER", {4,0,0,1,1,2,7,5,1, 4,0,0},AG_ADULT,AS_HOME, 7},
                        {"WREN",   {0,0,0,0,1,1,0,7,7, 4,0,0},AG_TEEN, AS_PLEAS,10} }},
    {"THE FRESHLYS",2,{ {"BEN",   {0,3,1,1,1,0,1,4,3, 0,0,0},AG_ADULT,AS_FORTUNE,1},
                        {"BEA",   {4,3,2,1,1,2,2,6,0, 0,0,0},AG_ADULT,AS_PLEAS, 6} }},
    {"THE NOVAS",2,{   {"JUNO",  {5,4,5,3,1,5,6,2,3, 0,0,0},AG_ADULT,AS_POP,    3},
                        {"KIT",   {0,2,3,1,2,4,1,5,2, 0,0,0},AG_TEEN, AS_GROW,   8} }},
    // the 32 Sims of the bake test (tools: every slider and part random), now families of their own
    {"THE STACKS",4,{ {"ROXY",{6,4,4,0,1,1,6,4,0,0,2,2,2,3,3,0,0,0,0,0,0,1,0,0,0,2,4,0,2,7,0,1,1,1,1,0,0,0,1,3,4,2,7,1,1,2,3,0,2,2,3,3,1,5,1,1,4,8,6,6,5,3,7,0,5,3,2,3,5,0,5,7,8,0,7,7,1,2,1,0,4,8,1,5,5,0},AG_ELDER,AS_FORTUNE, 0},
                {"DUKE",{5,7,6,6,2,4,1,3,2,2,5,7,0,1,2,0,0,0,0,0,0,0,0,0,3,4,1,7,6,1,0,0,1,3,3,0,0,0,3,5,8,8,8,0,6,3,3,1,3,7,2,3,3,0,6,5,0,0,4,8,0,5,5,4,6,2,5,1,8,0,7,7,8,5,7,5,8,5,0,7,8,2,7,0,6,1},AG_ADULT,AS_KNOW, 1},
                {"MILO",{3,2,4,7,2,4,0,1,7,0,1,8,0,2,2,0,0,0,3,0,0,0,0,0,0,5,2,3,3,7,7,2,5,2,2,0,0,3,6,2,5,1,1,6,8,5,1,1,1,8,0,6,7,1,5,3,1,5,7,8,6,4,7,7,7,2,7,2,7,0,2,7,8,1,4,2,1,0,5,3,2,4,0,6,7,6},AG_ADULT,AS_POP, 2},
                {"IVY",{0,5,1,5,1,0,6,0,1,0,5,6,2,0,2,0,0,0,2,0,0,0,0,0,0,7,3,1,7,5,8,5,1,3,1,0,0,7,4,2,8,2,7,8,1,6,3,3,2,3,7,5,6,5,7,2,3,8,3,7,6,6,0,3,5,0,1,2,4,0,5,0,0,7,6,0,2,0,3,5,0,0,2,3,7,5},AG_ADULT,AS_PLEAS, 3} }},
    {"THE PIXELS",4,{ {"ZED",{1,0,6,6,1,7,6,4,4,1,4,1,2,3,1,0,0,0,2,0,0,5,0,0,0,7,5,0,3,4,2,0,8,1,3,0,0,0,7,3,2,3,4,6,4,5,0,3,0,2,6,5,3,0,3,6,1,5,4,1,8,5,5,5,5,6,2,8,5,0,2,0,7,0,6,3,2,2,2,4,3,6,4,1,8,8},AG_ADULT,AS_HOME, 4},
                {"LULU",{5,0,1,8,1,8,2,1,1,8,7,3,2,3,1,0,0,0,1,0,0,0,0,0,0,3,8,4,4,1,8,6,1,1,1,0,0,6,0,7,2,4,6,5,1,7,1,1,2,8,2,6,8,5,5,2,3,8,7,6,1,7,1,8,5,1,4,0,5,0,8,1,5,8,5,4,1,2,6,2,8,8,1,7,5,8},AG_TEEN,AS_GROW, 5},
                {"BYTE",{4,6,7,8,2,5,2,7,3,8,0,7,3,2,3,4,0,0,2,0,0,5,0,0,0,1,4,2,1,1,1,3,4,3,1,0,0,6,0,8,5,2,3,6,0,6,0,1,3,7,7,2,6,8,8,6,2,4,1,5,3,4,8,7,1,6,2,8,0,0,8,8,0,2,4,3,3,8,5,3,0,8,4,0,7,8},AG_ELDER,AS_KNOW, 6},
                {"PIXIE",{4,0,6,3,2,4,1,6,5,8,5,7,2,3,2,0,0,0,3,0,0,5,0,0,0,6,6,1,5,5,1,7,5,3,1,0,0,2,8,8,1,8,4,0,3,2,0,1,0,5,5,5,2,6,3,1,1,4,4,5,8,5,5,8,5,1,7,5,5,0,0,8,0,4,5,1,0,4,6,3,8,0,5,8,0,3},AG_TEEN,AS_GROW, 7} }},
    {"THE VOXELLS",4,{ {"OTIS",{4,6,7,0,2,7,4,2,0,7,7,2,0,2,3,0,0,0,3,0,0,5,0,0,2,4,7,5,8,5,1,1,4,3,0,0,0,7,5,5,4,7,5,3,0,5,3,0,3,2,4,4,7,0,3,1,5,7,7,1,5,7,6,2,0,0,8,6,3,0,0,8,1,5,4,8,5,2,0,3,4,5,3,2,7,8},AG_ADULT,AS_PLEAS, 8},
                {"JUNE",{5,2,3,5,1,8,6,4,5,3,7,4,3,0,2,0,0,0,0,0,0,1,0,0,0,0,6,3,7,6,6,1,2,2,1,0,0,2,3,2,3,2,5,7,4,5,1,1,1,6,4,3,3,3,5,8,5,2,6,7,3,6,7,7,2,6,5,3,7,0,5,4,8,1,4,5,1,5,3,4,2,7,1,2,7,0},AG_ADULT,AS_HOME, 9},
                {"CASH",{1,7,2,0,1,3,1,5,1,1,0,0,2,1,2,4,0,0,1,0,0,4,0,0,0,5,7,8,0,6,2,3,7,1,0,0,0,0,8,5,6,4,4,5,2,2,1,3,3,5,3,8,2,1,4,5,5,3,7,5,8,4,6,7,3,8,5,7,8,0,7,2,1,6,7,7,8,4,6,6,3,3,3,2,4,4},AG_ADULT,AS_FORTUNE,10},
                {"MAYA",{5,0,1,8,1,0,0,1,7,1,8,3,1,1,0,0,0,0,0,0,0,2,0,0,1,2,2,3,0,6,5,0,5,3,0,0,0,4,0,6,6,2,6,5,0,8,2,2,0,2,5,3,1,2,7,6,0,1,4,2,4,0,0,1,1,3,8,0,0,0,6,8,2,8,1,8,6,4,4,1,7,7,1,2,5,1},AG_ELDER,AS_KNOW,11} }},
    {"THE LOWPOLYS",4,{ {"TOBY",{6,5,2,7,1,3,1,1,6,2,5,2,0,0,3,0,0,0,0,0,0,0,0,0,0,6,7,6,0,8,4,0,1,1,1,0,0,2,6,6,7,0,5,6,8,1,3,2,3,6,2,6,3,1,3,7,2,5,6,1,4,6,3,6,1,1,6,0,2,0,2,4,8,1,2,2,6,8,4,1,3,4,3,4,6,7},AG_TEEN,AS_GROW, 0},
                {"ELLA",{3,7,6,1,1,5,0,5,2,6,0,2,3,3,0,0,0,0,2,0,0,3,0,0,3,4,3,7,1,3,7,8,8,3,2,0,0,4,5,5,3,4,6,0,7,0,3,1,0,7,1,1,8,4,2,2,5,7,1,0,7,4,7,7,7,3,1,3,2,0,0,8,2,7,0,6,8,1,1,5,0,7,6,7,8,5},AG_TEEN,AS_GROW, 1},
                {"FINN",{5,6,2,8,1,3,4,3,3,0,3,4,1,3,0,3,0,0,3,0,0,2,0,0,3,3,6,6,1,6,4,7,3,2,2,0,0,7,6,3,7,5,2,5,3,7,0,0,1,4,2,6,1,7,6,0,0,3,3,2,0,2,5,0,3,3,1,8,6,0,6,1,1,0,3,8,5,8,4,3,3,5,6,0,4,7},AG_ELDER,AS_HOME, 2},
                {"SAGE",{4,6,1,0,1,7,2,1,6,4,1,2,2,3,2,0,0,0,2,0,0,5,0,0,0,7,4,2,1,2,4,5,2,1,0,0,0,1,5,0,2,2,5,7,3,2,3,3,3,7,2,6,8,7,1,3,6,8,3,0,2,3,3,4,0,0,6,2,4,0,8,8,5,5,1,8,4,6,8,6,1,7,0,1,6,0},AG_ADULT,AS_FORTUNE, 3} }},
    {"THE KICKFLIPS",4,{ {"GUS",{6,1,6,2,1,2,6,1,3,0,1,2,0,3,1,0,0,0,3,0,0,1,0,0,0,0,8,8,2,8,3,7,2,0,2,0,0,2,4,5,7,7,7,6,7,4,0,1,3,6,8,1,0,7,2,6,3,4,6,8,4,3,0,2,6,2,5,8,4,0,6,1,6,8,3,5,6,6,5,4,5,6,1,6,8,0},AG_ADULT,AS_KNOW, 4},
                {"NELL",{1,3,3,4,1,2,1,4,1,4,7,7,2,0,0,0,0,0,3,0,0,2,0,0,0,3,4,6,5,7,8,3,2,0,3,0,0,1,1,0,5,7,5,5,2,3,1,0,1,2,1,3,1,2,1,8,0,7,6,5,8,2,1,4,4,4,1,8,7,0,8,5,2,1,6,0,4,4,1,3,1,3,3,6,5,4},AG_ADULT,AS_POP, 5},
                {"ACE",{3,5,0,1,1,6,3,3,6,8,3,6,1,0,2,4,0,0,3,0,0,1,0,0,0,3,2,5,3,6,1,6,8,3,3,0,0,6,0,2,6,3,2,4,0,6,0,0,3,6,5,8,1,4,0,1,5,7,1,2,1,4,0,3,8,3,2,0,0,0,6,3,4,5,0,4,8,2,3,3,5,1,2,5,7,7},AG_TEEN,AS_GROW, 6},
                {"POPPY",{4,5,1,4,2,7,3,5,4,0,4,6,1,2,2,0,0,0,2,0,0,0,0,0,0,0,6,7,4,2,5,1,5,0,3,0,0,8,6,6,0,3,3,8,0,2,0,1,1,1,2,0,4,0,2,4,5,3,0,2,2,1,6,8,1,4,1,6,8,0,8,1,2,3,6,1,4,3,2,0,1,0,6,6,6,0},AG_ADULT,AS_HOME, 7} }},
    {"THE BUFFERS",4,{ {"HANK",{4,7,6,2,1,1,2,3,5,4,8,5,3,0,2,0,0,0,0,0,0,5,0,0,1,4,0,3,8,8,7,4,1,3,1,0,0,8,7,0,4,6,1,7,0,4,3,1,3,0,4,2,3,0,2,3,8,3,4,3,3,4,7,3,7,8,7,2,6,0,0,6,7,5,4,5,4,7,2,0,4,1,2,8,1,5},AG_ADULT,AS_FORTUNE, 8},
                {"DOT",{6,5,0,7,1,8,4,1,7,2,0,6,1,0,0,2,0,0,0,0,0,2,0,0,0,1,1,8,1,6,7,3,2,1,0,0,0,7,6,1,3,5,7,0,1,0,1,2,0,1,1,2,1,8,7,5,1,5,8,1,7,2,2,7,2,2,5,0,2,0,0,8,2,3,5,0,5,0,6,7,0,0,8,1,4,0},AG_ELDER,AS_KNOW, 9},
                {"RIO",{6,1,7,2,2,5,2,6,4,3,8,0,1,2,0,0,0,0,2,0,0,1,0,0,2,6,0,5,8,1,4,2,2,2,1,0,0,4,2,2,7,0,0,2,2,4,3,3,3,2,2,6,1,3,3,5,2,7,4,2,5,7,1,6,7,0,1,1,1,0,6,1,1,6,1,4,4,1,6,8,1,6,4,8,3,1},AG_ADULT,AS_POP,10},
                {"SUKI",{4,2,8,2,2,2,6,6,4,7,6,2,2,2,0,0,0,0,3,0,0,5,0,0,0,1,6,2,0,8,4,3,6,2,2,0,0,1,2,7,5,1,5,6,5,1,3,0,3,4,2,7,4,4,0,1,8,6,7,1,4,7,4,2,5,5,7,1,8,0,6,6,5,3,7,6,0,3,6,7,0,2,1,1,3,7},AG_ELDER,AS_PLEAS,11} }},
    {"THE SPRITES",4,{ {"WADE",{3,2,4,7,1,5,2,3,5,6,5,6,1,3,1,0,0,0,2,0,0,4,0,0,0,6,1,2,7,7,4,4,2,1,1,0,0,5,5,4,6,0,6,0,2,8,2,1,3,4,0,5,4,2,7,8,3,6,4,3,4,3,8,2,5,2,1,1,4,0,5,0,8,0,1,4,3,3,5,0,1,7,6,0,2,6},AG_ADULT,AS_HOME, 0},
                {"IRIS",{4,1,3,6,2,5,3,5,1,5,7,0,3,1,2,0,0,0,0,0,0,4,0,0,0,2,6,7,4,1,1,4,3,0,2,0,0,5,6,2,3,3,3,8,3,0,3,3,1,1,2,8,8,4,7,0,8,6,6,6,8,5,4,2,4,6,4,6,1,0,3,3,7,8,6,1,1,3,3,1,8,4,4,0,5,0},AG_TEEN,AS_GROW, 1},
                {"KOJI",{0,0,5,7,2,7,2,7,3,5,1,4,2,1,3,0,0,0,1,0,0,3,0,0,3,1,0,8,5,0,4,3,5,1,2,0,0,3,4,1,7,7,6,7,5,5,2,1,1,7,8,2,1,5,3,1,6,8,0,6,3,4,5,0,5,2,3,8,3,0,8,8,1,3,4,8,3,4,8,4,7,6,6,0,7,4},AG_ELDER,AS_KNOW, 2},
                {"LOLA",{1,3,5,7,2,6,2,0,2,6,4,8,0,2,1,0,0,0,1,0,0,0,0,0,0,5,3,5,5,4,1,1,3,2,3,0,0,7,7,2,7,2,7,2,8,8,1,2,1,2,5,0,4,6,0,3,2,4,6,3,0,8,1,8,8,2,7,6,1,0,6,3,0,2,3,3,5,2,2,1,2,5,0,7,6,7},AG_ADULT,AS_POP, 3} }},
    {"THE DIPPERS",4,{ {"VINCE",{4,3,5,2,1,8,1,7,6,2,7,6,2,0,1,0,0,0,2,0,0,1,0,0,0,3,7,0,5,0,8,8,7,2,0,0,0,5,0,0,7,0,5,4,2,8,0,2,2,3,7,8,1,0,3,4,1,5,8,7,8,7,7,2,5,4,0,0,7,0,6,4,5,8,1,3,2,8,1,8,6,1,3,7,0,1},AG_TEEN,AS_GROW, 4},
                {"MAE",{5,6,2,1,1,0,2,3,4,7,2,6,2,3,2,3,0,0,3,0,0,4,0,0,0,7,2,0,4,0,0,8,7,3,3,0,0,6,7,5,7,1,0,3,3,1,3,3,2,1,2,8,8,3,4,8,8,4,2,0,4,5,8,8,7,5,8,6,1,0,2,5,7,4,5,0,6,6,0,5,0,1,2,4,3,4},AG_TEEN,AS_GROW, 5},
                {"OZZY",{3,0,7,4,1,8,6,7,7,1,7,4,3,0,2,0,0,0,2,0,0,1,0,0,3,7,4,0,1,8,7,0,8,0,0,0,0,1,5,6,8,7,5,5,0,3,2,0,1,3,8,4,1,1,7,8,0,2,0,6,5,1,6,2,0,3,4,2,2,0,6,0,3,3,5,0,7,4,0,1,1,7,0,1,4,3},AG_TEEN,AS_GROW, 6},
                {"TESS",{4,6,8,5,2,7,0,1,1,8,2,2,3,3,0,3,0,0,2,0,0,2,0,0,0,7,3,6,2,6,6,0,5,0,1,0,0,6,6,1,7,1,2,0,1,7,1,1,3,6,6,8,2,7,0,3,2,5,2,8,4,7,0,0,3,4,3,7,8,0,7,7,1,3,4,6,5,2,2,4,0,5,5,8,8,6},AG_ELDER,AS_KNOW, 7} }},
};
#define HH_NFAM ((int)(sizeof(hhFams)/sizeof(hhFams[0])))

// ---- 16-bit sprite <-> 15 colours + clear, 4bpp tiles ----
// Colour lookups for the two quantisers: a small open-addressing hash from a 15-bit colour to a slot (key 0xFFFF = empty).
// The sprites hold about 40 colours, so a lookup is one or two probes instead of a walk through the list.
#define HQ_N 512
static u16 hqKey[HQ_N] EWRAM_BSS; static u8 hqVal[HQ_N] EWRAM_BSS;
static void hqClear(void){ for(int i=0;i<HQ_N;i++) hqKey[i]=0xFFFF; }
static inline int hqSlot(u16 c){ int h=(int)(((u32)c*40503u)>>7)&(HQ_N-1); while(hqKey[h]!=0xFFFF&&hqKey[h]!=c) h=(h+1)&(HQ_N-1); return h; }
static inline int hqDist(u16 a,u16 b){ int dr=(a&31)-(b&31), dg=((a>>5)&31)-((b>>5)&31), db=((a>>10)&31)-((b>>10)&31); return dr*dr*3+dg*dg*4+db*db*2; }
static void hhQuant(u16 (*src)[SPW*SPH],u8 (*dst)[OBJ_B],u16*pal){
    static u16 col[256] EWRAM_BSS, oc[256] EWRAM_BSS; static u32 cnt[256] EWRAM_BSS; static u8 ob[256] EWRAM_BSS; int n=0;
    hqClear();
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++){ u16 c=src[v][i]; if(c==SKY) continue; int h=hqSlot(c), k;
        if(hqKey[h]==0xFFFF){ if(n==256) continue; hqKey[h]=c; hqVal[h]=(u8)n; col[n]=c; cnt[n]=0; k=n++; } else k=hqVal[h];
        cnt[k]++; }
    int n0=n; for(int k=0;k<n0;k++) oc[k]=col[k];   // the colours as found (hqVal points into this list)
    while(n>15){   // merge the two closest colours (weighted by how often they appear) until 15 are left
        int ba=0, bb=1, bd=1<<30;
        for(int a=0;a<n;a++)for(int b=a+1;b<n;b++){ int d=hqDist(col[a],col[b])*(int)(cnt[a]<cnt[b]?cnt[a]:cnt[b]); if(d<bd){ bd=d; ba=a; bb=b; } }
        u32 w=cnt[ba]+cnt[bb]; if(!w) w=1;
        int r=(int)(((col[ba]&31)*cnt[ba]+(col[bb]&31)*cnt[bb])/w), g=(int)((((col[ba]>>5)&31)*cnt[ba]+((col[bb]>>5)&31)*cnt[bb])/w), bl=(int)((((col[ba]>>10)&31)*cnt[ba]+((col[bb]>>10)&31)*cnt[bb])/w);
        if(cnt[bb]>cnt[ba]) col[ba]=col[bb]; else if(cnt[ba]==cnt[bb]) col[ba]=(u16)(r|(g<<5)|(bl<<10));   // keep the commoner one exact (faces stay crisp)
        cnt[ba]=w; col[bb]=col[n-1]; cnt[bb]=cnt[n-1]; n--; }
    pal[0]=0; for(int k=0;k<15;k++) pal[k+1]=k<n?col[k]:0;
    for(int j=0;j<n0;j++){ u16 c=oc[j]; int best=1, bd=1<<30;   // the nearest palette entry, once per colour found (not once per pixel)
        for(int k=0;k<n;k++){ int d=hqDist(c,col[k]); if(d<bd){ bd=d; best=k+1; if(!d) break; } } ob[j]=(u8)best; }
    for(int v=0;v<4;v++){
        for(int i=0;i<OBJ_B;i++) dst[v][i]=0;
        for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++){ u16 c=src[v][y*SPW+x]; if(c==SKY) continue;
            int h=hqSlot(c), best;
            if(hqKey[h]==c) best=ob[hqVal[h]];
            else { int bd=1<<30; best=1; for(int k=0;k<n;k++){ int d=hqDist(c,col[k]); if(d<bd){ bd=d; best=k+1; if(!d) break; } } }   // (past 256 colours: not in the table)
            int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1); dst[v][o]|=(u8)(best<<((x&1)*4)); }
    }
}
static void hhQuantS(u16 (*src)[SPW*SPH],u8 (*dst)[STR_BN],const u16*pal){   // the stride band, in the palette the standing frame chose
    hqClear(); int used=0;
    for(int v=0;v<4;v++){
        for(int i=0;i<STR_BN;i++) dst[v][i]=0;
        for(int y=STR_Y0;y<STR_Y1&&y<SPH;y++)for(int x=0;x<SPW;x++){ u16 c=src[v][y*SPW+x]; if(c==SKY) continue;
            int h=hqSlot(c), best;
            if(hqKey[h]==c) best=hqVal[h];
            else { int bd=1<<30; best=1; for(int k=1;k<16;k++){ int d=hqDist(c,pal[k]); if(d<bd){ bd=d; best=k; if(!d) break; } }
                   if(used<HQ_N*3/4){ hqKey[h]=c; hqVal[h]=(u8)best; used++; } }   // remembered: the next pixel of this colour is one lookup
            int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1)-STR_B0; dst[v][o]|=(u8)(best<<((x&1)*4)); }
    }
}
static void hhUnquantS(u8 (*src)[STR_BN],const u16*pal,u16 (*dst)[SPW*SPH]){   // a stride band back over a copy of the standing frame
    for(int v=0;v<4;v++)for(int y=STR_Y0;y<STR_Y1&&y<SPH;y++)for(int x=0;x<SPW;x++){ int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1)-STR_B0, k=(src[v][o]>>((x&1)*4))&15; dst[v][y*SPW+x]=k?pal[k]:SKY; }
}
static void hhUnquant(u8 (*src)[OBJ_B],const u16*pal,u16 (*dst)[SPW*SPH]){   // back to 16-bit (when a member becomes the one you control)
    for(int v=0;v<4;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++){ int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1), k=(src[v][o]>>((x&1)*4))&15; dst[v][y*SPW+x]=k?pal[k]:SKY; }
}
static void spBounds(void){   // the box that holds every opaque pixel of the player's four views (blits and redraw rectangles stay inside it)
    spBx0=SPW; spBx1=0; spBy0=SPH; spBy1=0;
    for(int v=0;v<8;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++) if((v<4?spr4[v]:spr4s[v-4])[y*SPW+x]!=SKY){ if(x<spBx0) spBx0=x; if(x+1>spBx1) spBx1=x+1; if(y<spBy0) spBy0=y; if(y+1>spBy1) spBy1=y+1; }
    if(spBx0>=spBx1){ spBx0=0; spBx1=SPW; spBy0=0; spBy1=SPH; }
}
// ---- baking: render a member's look with the creator's own code, then put the player's creature back ----
static void hhRandLook(u8*lk,u8*stg){   // a made-up Sim: passers-by, and SELECT on the RELATIONSHIPS screen
    static const u8 shp[6]={0,1,3,4,5,6}, sg[4]={AG_ADULT,AG_ADULT,AG_TEEN,AG_ELDER};
    for(int i=0;i<LK_N;i++) lk[i]=0;
    lk[LK_SHAPE]=shp[rnd8()%6]; lk[LK_SKIN]=(u8)(rnd8()%NSW); lk[LK_EYES]=(u8)(rnd8()%NEYE); lk[LK_MOUTH]=(u8)(rnd8()%NMOUTH);
    lk[LK_EARS]=(u8)(1+(rnd8()&1)); lk[LK_HSTYLE]=(u8)(rnd8()%NHAIR); lk[LK_HCOL]=(u8)(rnd8()%NSW); lk[LK_TOP]=(u8)(rnd8()%NSW); lk[LK_BOT]=(u8)(rnd8()%NSW);
    lk[LK_TOPSTY]=(u8)(rnd8()&3); lk[LK_HAT]=(rnd8()&3)==0?(u8)(1+rnd8()%5):0; lk[LK_GLASS]=(rnd8()&3)==0?(u8)(1+rnd8()%3):0;
    lk[LK_BROW]=(u8)(rnd8()%6); lk[LK_EYECOL]=(u8)(rnd8()%NSW);
    *stg=sg[rnd8()&3];
}
// ---- bake cache: a sprite set is only baked again when something it is drawn from changed ----
// The key is a hash of everything the bake reads: the voxels, the face sprites, the colour tables, the look, the age stage, the hand-built
// flag, the unlock flag and every option. Equal key = an identical picture, so a Sim that did not change is not drawn again (coming back
// from the editor, a slot, the pause menu, growing up ...). Keys follow their sprites when members move (hhRemove); hhSwitch drops them.
static u32 hhKey[HH_MAX];   // per member: the key hhObj / hhObjS / hhPal were baked from (0 = unknown)
static u8 twKeep;           // the visitors already picked are kept (new faces when you move to another lot or start a new life)
static u8 twHas[TW_N], twOn[TW_N]; static short twWait[TW_N]={240,900}; static char twFrom[TW_N][12];   // visitor k: set up, on the lot (1 coming, 2 staying, 3 going), the lot they live on
static int nbVisitor(HhSim*s,char*from,int not);   // households.h: a Sim of another household of the town (s), the lot it lives on; not = a family to skip
static void twPick(void){   // who comes by while you are on this lot: Sims from the town's other households
    int f0=-1;
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); twHas[k]=0; twOn[k]=0; twWait[k]=(short)(240+k*700);
        if(v<hhN||!xo[XO_SIMPRE]) continue;
        HhSim*s=&hhM[v]; int f=nbVisitor(s,twFrom[k],f0); if(f==-2) continue; f0=f;
        s->uid=255; s->bubT=0; s->hp=HP_MAX; s->ltw=0; s->act=HA_IDLE; s->think=0; s->hd=0; s->pn=s->pi=0; for(int q=0;q<HN_N;q++) s->need[q]=80;
        twHas[k]=1; }
}
static void twDrop(int place){ for(int k=0;k<TW_N;k++) if(TW_V(k)==place){ twHas[k]=0; twOn[k]=0; hhKey[place]=0; } }   // a member moves into a visitor's place
static void hkAdd(u32*h,const void*p,int n){ const u8*b=(const u8*)p; u32 x=*h; for(int i=0;i<n;i++) x=(x^b[i])*16777619u; *h=x; }
static u32 bakeKey(void){   // call after buildLook() + setColors()
    u32 h=2166136261u; u8 t[4]={stage,(u8)custom,sUnlock,0};
    hkAdd(&h,vox,sizeof(vox)); hkAdd(&h,dec,sizeof(dec)); hkAdd(&h,sT,sizeof(sT)); hkAdd(&h,sL,sizeof(sL)); hkAdd(&h,sR,sizeof(sR));
    hkAdd(&h,dL,sizeof(dL)); hkAdd(&h,dR,sizeof(dR)); hkAdd(&h,look,sizeof(look)); hkAdd(&h,t,4); hkAdd(&h,xo,sizeof(xo));
    return h?h:1;
}
static void hhBakeAll(void){
    static u8 sv[H][D][W] EWRAM_BSS; static u16 sd[H][D][W] EWRAM_BSS; u8 sl[LK_N]; u8 sst=stage; int sc=custom;
    int scratch=0;   // spr4 / spr4s were used to bake someone else: the player has to be baked again
    for(int i=0;i<LK_N;i++) sl[i]=look[i];
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ sv[y][z][x]=vox[y][z][x]; sd[y][z][x]=dec[y][z][x]; }
    for(int m=0;m<hhN;m++){
        for(int i=0;i<LK_N;i++) look[i]=hhM[m].look[i]; stage=hhM[m].stage;
        buildLook(); setColors(); u32 k=bakeKey(); if(k==hhKey[m]) continue;   // unchanged since the last bake
        ldShow("GETTING THE SIMS READY",m,hhN+TW_N+1);
        bakeInto(spr4); hhQuant(spr4,hhObj[m],hhPal[m]);
        strideK=1; bakeInto(spr4s); strideK=0; hhQuantS(spr4s,hhObjS[m],hhPal[m]); hhKey[m]=k; scratch=1;
    }
    if(!twKeep) twPick();   // who visits (new faces on another lot or in a new life)
    twKeep=1;
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); if(!twHas[k]||v<hhN) continue;   // the visitors, in the free member places
        for(int i=0;i<LK_N;i++) look[i]=hhM[v].look[i]; stage=hhM[v].stage;
        buildLook(); setColors(); u32 kk=bakeKey(); if(kk==hhKey[v]) continue;
        ldShow("GETTING THE NEIGHBORS READY",hhN+k,hhN+TW_N+1);
        bakeInto(spr4); hhQuant(spr4,hhObj[v],hhPal[v]);
        strideK=1; bakeInto(spr4s); strideK=0; hhQuantS(spr4s,hhObjS[v],hhPal[v]); hhKey[v]=kk; scratch=1;
    }
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=sst;
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ vox[y][z][x]=sv[y][z][x]; dec[y][z][x]=sd[y][z][x]; }
    custom=sc; setColors();
    u32 pk=bakeKey();
    if(scratch||pk!=sprKey){
        ldShow("ALMOST THERE",hhN+TW_N,hhN+TW_N+1);
        bakeInto(spr4);   // the player (still drawn by the CPU, so walls and furniture in front cover it and the action cam can zoom it)
        strideK=1; bakeInto(spr4s); strideK=0; sprKey=pk;
    }
    spBounds();      // the blit box holds both frames
    ldEnd();         // the loading screen is over: the game's display mode (window 0 + sprites) comes back
    hhSlotsFree();   // new tiles and palettes: every slot is reloaded when its Sim is next on screen
}
// ---- where members can stand ----
static int hhWalk(int x,int y){ if(x<0||y<0||x>=MW||y>=MH) return 0; char c=lifeMap[y][x]; return c!='w'&&c!='W'&&tileH(x,y)<=3; }
static int hhTakenAt(int x,int y); static void hhTakenScan(const HhSim*self);
static void hhPlace(HhSim*s,int k){   // somewhere free near the spawn point, spread out a little (never on someone else's tile)
    s->act=HA_AWAY; hhTakenScan(s);
    for(int r=1;r<12;r++)for(int t=0;t<40;t++){ int x=spx+((rnd8()%(2*r+1))-r), y=spy+((rnd8()%(2*r+1))-r);
        if(hhWalk(x,y)&&(x!=spx||y!=spy)&&!hhTakenAt(x,y)){ s->fx=x*256+128; s->fy=y*256+128; s->act=HA_IDLE; return; } }
    s->act=HA_IDLE;
    s->fx=spx*256+128; s->fy=spy*256+128; (void)k;
}
static int hhFreeUid(void);
static const char* const hhNames[24]={"ALEX","SAM","JO","RILEY","MILO","NOVA","IVY","OTTO","LUNA","FINN","ZOE","RAY","SKYE","BO","CLEO","DEX","ARLO","JUDE","MAE","REMY","TESS","VIC","WREN","ZED"};
static void hhPickName(char*out){   // a first name nobody in the house has yet
    for(int t=0;t<48;t++){ const char*nm=hhNames[(rnd8()+t)%24]; int used=0; for(int i=0;nm[i]==hhPName[i];i++) if(!nm[i]){ used=1; break; }
        for(int m=0;m<hhN&&!used;m++){ int i=0; while(nm[i]&&nm[i]==hhM[m].name[i]) i++; if(!nm[i]&&!hhM[m].name[i]) used=1; }
        if(!used){ int i=0; for(;nm[i]&&i<HH_NM-1;i++) out[i]=nm[i]; out[i]=0; return; } }
    out[0]='S'; out[1]='I'; out[2]='M'; out[3]=0;
}
static void hhPlace(HhSim*s,int k);
static int hhAdd(const u8*lk,int stg,int asp,int ltw,const u8*tr){   // a new member of the family (CREATE-A-FAMILY): -1 when the house is full
    if(hhN>=HH_MAX) return -1;
    twDrop(hhN);
    HhSim*s=&hhM[hhN]; s->uid=(u8)hhFreeUid(); s->bubT=0; s->hp=HP_MAX;
    for(int i=0;i<LK_N;i++) s->look[i]=lk[i];
    s->stage=(u8)stg; s->asp=(u8)asp; s->ltw=(u8)ltw; for(int i=0;i<TR_N;i++) s->tr[i]=tr[i];
    hhPickName(s->name); for(int i=0;i<HH_NM;i++) s->last[i]=hhPLast[i];   // family: your last name
    for(int k=0;k<HN_N;k++) s->need[k]=(u8)(70+(rnd8()&15)); s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0;
    hhPlace(s,0);
    int a=s->uid;                                       // family: they know and like everyone at home already
    for(int u=0;u<HU_N;u++){ if(u==a) continue; relD[a][u]=relD[u][a]=40; relL[a][u]=relL[u][a]=50; relF[a][u]=relF[u][a]=0; }
    return hhN++;
}
static void hhRemove(int m){   // moves out: their sprites and relationships go with them
    if(m<0||m>=hhN) return;
    int a=hhM[m].uid; for(int u=0;u<HU_N;u++){ relD[a][u]=relD[u][a]=0; relL[a][u]=relL[u][a]=0; relF[a][u]=relF[u][a]=0; }
    for(int k=m;k<hhN-1;k++){ hhM[k]=hhM[k+1]; for(int v=0;v<4;v++){ for(int i=0;i<OBJ_B;i++) hhObj[k][v][i]=hhObj[k+1][v][i]; for(int i=0;i<STR_BN;i++) hhObjS[k][v][i]=hhObjS[k+1][v][i]; } for(int i=0;i<16;i++) hhPal[k][i]=hhPal[k+1][i]; }
    for(int k=m;k<hhN-1;k++) hhKey[k]=hhKey[k+1]; hhKey[hhN-1]=0;   // the keys move with the sprites
    hhN--; hhSlotsFree();
}
static void hhNew(HhSim*s,const HhPre*p){
    s->uid=(u8)hhFreeUid(); s->bubT=0; s->hp=HP_MAX;
    for(int i=0;i<LK_N;i++) s->look[i]=p->look[i];
    s->stage=p->stage; s->asp=p->asp; s->ltw=0; for(int i=0;i<TR_N;i++) s->tr[i]=signTr[p->sign][i];
    int i=0; for(;p->name[i]&&i<HH_NM-1;i++) s->name[i]=p->name[i]; s->name[i]=0; s->last[0]=0;
    for(int k=0;k<HN_N;k++) s->need[k]=(u8)(70+(rnd8()&15)); s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0;
    hhPlace(s,0);
}

// ---- path finding: BFS from the member's tile; the goal is any free tile next to furniture c (or a random free tile for c=0) ----
static int hhGX, hhGY;   // hhPlan(s,1): walk next to this tile (a person)
static int hhNextTo(int x,int y,char c){   // (a beanbag 'U' is as good as the sofa 'C')
    if(c==1){ int dx=x-hhGX, dy=y-hhGY; return (dx==0&&(dy==1||dy==-1))||(dy==0&&(dx==1||dx==-1)); } for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d]; if(nx>=0&&ny>=0&&nx<MW&&ny<MH&&(lifeMap[ny][nx]==c||(c=='C'&&lifeMap[ny][nx]=='U'))) return 1; } return 0; }
static u16 hhTk[HH_MAX+1]; static int hhTkN;   // the tiles that are someone's spot (filled by hhTakenScan)
static void hhTakenScan(const HhSim*self){   // the player's tile, and every other member's: where it stands, or where its walk ends
    hhTkN=0; hhTk[hhTkN++]=(u16)((int)(lfy>>8)*MW+(int)(lfx>>8));
    for(int m=0;m<hhN;m++){ const HhSim*o=&hhM[m]; if(o==self||o->act==HA_AWAY) continue;
        int tx, ty, k;
        if(o->gok){ tx=(int)(o->gx>>8); ty=(int)(o->gy>>8); k=o->pi+1; } else { tx=(int)(o->fx>>8); ty=(int)(o->fy>>8); k=o->pi; }
        if(o->act==HA_WALK||o->act==HA_WANDER||o->act==HA_SEEK||o->act==HA_LEAVE) for(;k<o->pn;k++){ tx+=hhDx[o->path[k]]; ty+=hhDy[o->path[k]]; }
        if(tx>=0&&ty>=0&&tx<MW&&ty<MH) hhTk[hhTkN++]=(u16)(ty*MW+tx); }
}
static int hhTakenAt(int x,int y){ int p=y*MW+x; for(int i=0;i<hhTkN;i++) if(hhTk[i]==p) return 1; return 0; }
static int hhTaken(int x,int y,const HhSim*self){ hhTakenScan(self); return hhTakenAt(x,y); }
static int hhPlan(HhSim*s,char c){   // fills s->path; returns its length+1 (1 = already there), 0 = no way
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if(!hhWalk(sx,sy)) return 0;
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;
    hhTakenScan(s);
    if(c==1&&((sx-hhGX)*(sx-hhGX)+(sy-hhGY)*(sy-hhGY))<=2) return 1;   // already next to them
    int qh=0, qt=0, goal=-1, pick=c?0:(rnd8()*4+rnd8())%400+40;   // wander: the pick-th tile the search reaches
    hhDist[sy*MW+sx]=0; hhQ[qt++]=(u16)(sy*MW+sx);
    while(qh<qt){ int p=hhQ[qh++], x=p%MW, y=p/MW;
        if((c?hhNextTo(x,y,c):(qh>=pick))&&(c==1||!hhTakenAt(x,y))){ goal=p; break; }   // (a spot someone already has is skipped: no two Sims on one tile)
        if(hhDist[p]>=HH_PATH) continue;
        for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d], np=ny*MW+nx; if(hhWalk(nx,ny)&&hhDist[np]==0xFFFF){ hhDist[np]=(u16)(hhDist[p]+1); hhQ[qt++]=(u16)np; } } }
    if(goal<0) return 0;
    int n=hhDist[goal], p=goal; s->pn=(u8)n; s->pi=0; s->gok=0;
    for(int k=n-1;k>=0;k--){ int x=p%MW, y=p/MW;   // walk back down the distances
        for(int d=0;d<4;d++){ int px=x-hhDx[d], py=y-hhDy[d], pp=py*MW+px; if(px>=0&&py>=0&&px<MW&&py<MH&&hhDist[pp]==hhDist[p]-1){ s->path[k]=(u8)d; p=pp; break; } } }
    return n+1;
}

// ---- free will ----
static int hhTilt(const HhSim*s,int n){   // traits: neat Sims shower sooner, lazy ones sit, playful ones skate (percent)
    switch(n){ case HN_CLEAN: return 70+s->tr[TR_NEAT]*6; case HN_COMFY: return 130-s->tr[TR_ACT]*6; case HN_FUN: return 70+s->tr[TR_PLAY]*6; case HN_SOC: return 60+s->tr[TR_OUT]*8; default: return 100; }
}
static void hhSeek(HhSim*s);   // social: pick someone and walk over (below)
static int hhUseT(const HhSim*s){ return (s->use==HN_REST&&simIsNight())?HH_USE*5:HH_USE; }   // a night in bed is a long one
static void hhDecide(HhSim*s){
    int best[2]={-1,-1}, bs[2]={0,0}, low=xo[XO_FREEWILL]==1?35:55;   // LOW free will waits until needs are lower
    for(int n=0;n<HN_N;n++){
        if(hnFurn[n]&&!(simHave&(n==HN_FOOD?SR_FRIDGE:n==HN_WC?SR_TOILET:n==HN_REST?SR_BED:n==HN_CLEAN?SR_SHOWER:SR_SOFA))) continue;   // no such furniture
        if(n==HN_SOC&&hhN<1) continue;
        int v=s->need[n]; if(n==HN_REST&&simIsNight()) v=v>45?v-45:0;   // night: bedtime comes first
        if(v>=low+30) continue;
        int u=(100-v)*(100-v)/100*hhTilt(s,n)/100;
        if(u>bs[0]){ bs[1]=bs[0]; best[1]=best[0]; bs[0]=u; best[0]=n; } else if(u>bs[1]){ bs[1]=u; best[1]=n; }
    }
    int n=best[0]; if(best[1]>=0&&bs[1]*4>=bs[0]*3&&(rnd8()&1)) n=best[1];   // close call: either of the two
    if((simHave&SR_PIPE)&&s->stage>=AG_ADULT&&s->act!=HA_LEAVE){   // grown-ups and the water pipe: for fun, and everyone at 4:20
        int t420=simMin>=16*60+20&&simMin<17*60+20;
        if((t420&&rnd8()<200)||(n==HN_FUN&&(rnd8()&1))){ int r=hhPlan(s,'G'); if(r==1){ s->act=HA_USE; s->use=HN_FUN; s->t=HH_USE; s->bub=IC_LEAF; s->bubT=90; return; } if(r>1){ s->act=HA_WALK; s->use=HN_FUN; return; } } }
    if(n==HN_SOC){ hhSeek(s); return; }
    if(n<0&&hhN>0&&(rnd8()*100>>8)<25+s->tr[TR_OUT]*5){ hhSeek(s); return; }   // nothing pressing: go and see someone (outgoing Sims more often)
    if(n<0){ if(hhPlan(s,0)>1){ s->act=HA_WANDER; s->use=HN_FUN; } else s->act=HA_IDLE; return; }
    int r=hhPlan(s,hnFurn[n]?hnFurn[n]:0);
    if(r==1&&hnFurn[n]){ s->act=HA_USE; s->use=(u8)n; s->t=hhUseT(s); }
    else if(r>1){ s->act=hnFurn[n]?HA_WALK:HA_WANDER; s->use=(u8)n; }
    else s->act=HA_IDLE;
}
static void hhArrive(int m);   // social: reached the person (below)
static void relTick(void);
static int hhStill;   // steps you have been standing still (hud.h only shows the thought bubble when you stop)
// ---- the day's routine: work and school off the lot, back in the evening; passers-by on the edges of the map ----
// Every reachable tile on the map's border is a way off the lot (found once, by a search from the spawn point). Adults go to work on
// weekdays and children and teens to school: each walks to the nearest way off and is gone (needs still drain), then walks back in at
// the end of the day, a little hungry and tired. If no border tile can be reached, they are picked up at the spawn point (a carpool).
// Two passers-by (made-up townies, their own hardware sprites) walk from one way off to another in the daytime.
static int hhExN, hhExX, hhExY; static u16 hhEx[16];
static void hhFindExits(void){
    hhExN=0; hhExX=spx; hhExY=spy; if(!hhWalk(spx,spy)) return;
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;
    int qh=0, qt=0, seen=0; hhDist[spy*MW+spx]=0; hhQ[qt++]=(u16)(spy*MW+spx);
    while(qh<qt){ int p=hhQ[qh++], x=p%MW, y=p/MW;
        if(x==0||y==0||x==MW-1||y==MH-1){
            if(!seen){ hhExX=x; hhExY=y; }   // the nearest way off: where the household comes and goes
            seen++; if(hhExN<16) hhEx[hhExN++]=(u16)p; else { int r=((rnd8()<<8)|rnd8())%seen; if(r<16) hhEx[r]=(u16)p; } }
        for(int d=0;d<4;d++){ int nx=x+hhDx[d], ny=y+hhDy[d], np=ny*MW+nx; if(hhWalk(nx,ny)&&hhDist[np]==0xFFFF){ hhDist[np]=(u16)(hhDist[p]+1); hhQ[qt++]=(u16)np; } } }
}
static int hhSched(const HhSim*s,int*from,int*to){   // 1 work, 2 school (weekdays), 0 nothing: everyone leaves and comes back a little apart
    if(!simWorkday()) return 0;
    int u=s->uid;
    if(s->stage==AG_ADULT){ *from=SIM_WORK_FROM-25-(u%4)*5; *to=SIM_WORK_TO+(u%5)*6; return 1; }
    if(s->stage==AG_CHILD||s->stage==AG_TEEN){ *from=8*60-15-(u%3)*5; *to=15*60+(u%4)*5; return 2; }
    return 0;
}
static char hhNoteB[40];
static void hhNote(const HhSim*s,const char*w){ if(lnoteT>0) return; char*e=simCat(hhNoteB,s->name); simCat(e,w); lnote=hhNoteB; lnoteT=110; }
static void hhStepAlong(HhSim*s){   // one step along the path, tile centre to tile centre
    int d=s->path[s->pi];
    if(!s->gok){ s->gx=((int)(s->fx>>8)+hhDx[d])*256+128; s->gy=((int)(s->fy>>8)+hhDy[d])*256+128; s->fx=(s->fx&~255)|128; s->fy=(s->fy&~255)|128; s->gok=1; }
    int sp=F_WALK*stSpd[s->stage]/100; if(sp<2) sp=2;
    if(hhDx[d]){ s->fx+=hhDx[d]*sp; if((hhDx[d]>0&&s->fx>=s->gx)||(hhDx[d]<0&&s->fx<=s->gx)){ s->fx=s->gx; s->pi++; s->gok=0; } }
    else { s->fy+=hhDy[d]*sp; if((hhDy[d]>0&&s->fy>=s->gy)||(hhDy[d]<0&&s->fy<=s->gy)){ s->fy=s->gy; s->pi++; s->gok=0; } }
    s->hd=(u8)(d*4);
}
static char twMsg[40];   // "ROXY STACK FROM MAPLE 2 DROPS BY"
static int twFar(void){   // where a visitor comes from / goes to: a way off the lot, or (walls all round) somewhere walkable well away from you. -1 = nowhere
    if(hhExN>0) return hhEx[rnd8()%hhExN];
    int px=(int)(lfx>>8), py=(int)(lfy>>8);
    for(int t=0;t<60;t++){ int x=(int)(((u32)rnd8()<<8|rnd8())%MW), y=(int)(((u32)rnd8()<<8|rnd8())%MH), d=(x>px?x-px:px-x)+(y>py?y-py:py-y);
        if(d>=10&&hhWalk(x,y)) return y*MW+x; }
    return -1;
}
static void twTick(int*planned){   // VISITORS: someone from another household walks in, stays a while near the house, and walks off again
    if(!xo[XO_SIMPRE]){ for(int k=0;k<TW_N;k++) twOn[k]=0; return; }   // PRE-MADE SIMS off: the town's families stay home
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); if(!twHas[k]||v<hhN){ twOn[k]=0; continue; } HhSim*s=&hhM[v];
        if(!twOn[k]){
            if(twWait[k]>0){ twWait[k]--; continue; }
            if(*planned||simIsNight()){ twWait[k]=60; continue; }
            int a=twFar(); if(a<0){ twWait[k]=120; continue; } s->fx=(a%MW)*256+128; s->fy=(a/MW)*256+128; *planned=1;
            hhGX=(int)(lfx>>8); hhGY=(int)(lfy>>8);
            if(hhPlan(s,1)>1){ twOn[k]=1; s->act=HA_WALK;   // walking in, over to you
                char*e=twMsg; for(const char*p=s->name;*p;) *e++=*p++; *e++=' '; for(const char*p=s->last;*p;) *e++=*p++;
                if(twFrom[k][0]){ const char*p=" FROM "; while(*p) *e++=*p++; for(p=twFrom[k];*p;) *e++=*p++; } else { const char*p=" DROPS BY"; while(*p) *e++=*p++; } *e=0;
                lnote=twMsg; lnoteT=90; }
            else twWait[k]=120;
            continue;
        }
        if(twOn[k]==2){ if(--twWait[k]>0) continue;   // staying a while, then off again
            if(*planned){ twWait[k]=30; continue; }
            int b=twFar(); if(b<0){ twOn[k]=0; twWait[k]=900; continue; } hhGX=b%MW; hhGY=b/MW; *planned=1;
            if(hhPlan(s,1)>1){ twOn[k]=3; s->act=HA_WALK; } else twWait[k]=60;
            continue; }
        if(s->pi>=s->pn){
            if(twOn[k]==1){ twOn[k]=2; s->act=HA_IDLE; twWait[k]=(short)(360+(rnd8()<<2)); }   // there: stays 6 to 23 seconds
            else { twOn[k]=0; twWait[k]=(short)(900+(rnd8()<<4)); }                      // gone: the next visit in a while
            continue; }
        hhStepAlong(s);
    }
}
static void hhTick(void){   // once per logic step in the life game
    if(curFl) return;   // upstairs: the household waits on the ground floor
    if(hhBubT) hhBubT--;
    if(lvx||lvy||lsp||lairF||lgrind) hhStill=0; else if(hhStill<1000) hhStill++;
    int planned=0;
    if(xo[XO_FREEWILL]) twTick(&planned);
    if(!hhN) return;
    relTick();
    int fe=oFoodEvery(), we=oWcEvery();
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m];
        // needs drain (gently: the Sims you do not watch should not be in constant crisis)
        if(fe&&lfr%(fe*2)==0&&s->need[HN_FOOD]>0) s->need[HN_FOOD]--;
        if(we&&lfr%(we*2)==0&&s->need[HN_WC]>0) s->need[HN_WC]--;
        if(lfr%300==m*7){ for(int n=HN_REST;n<HN_N;n++) if(s->need[n]>0) s->need[n]--; }
        if(s->bubT) s->bubT--;
        if(s->hp<HP_MAX&&lfr%HP_REGEN==(m*11)%HP_REGEN) s->hp++;   // health creeps back (knocked out Sims wake at 30)
        if(lfr%(150-s->tr[TR_OUT]*8)==0&&s->need[HN_SOC]>0) s->need[HN_SOC]--;   // lonely sooner when outgoing
        if(s->act==HA_SOC){ if(--s->t<=0){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); } continue; }   // standing in a conversation
        if(!xo[XO_FREEWILL]){ if(s->act==HA_AWAY) s->fx=hhExX*256+128, s->fy=hhExY*256+128; s->act=HA_IDLE; continue; }
        { int fr=0, to=0, k=hhSched(s,&fr,&to), due=k&&simMin>=fr&&simMin<to;   // the day's routine
          if(s->act==HA_AWAY){ if(due) continue;
              s->fx=hhExX*256+128; s->fy=hhExY*256+128; s->act=HA_IDLE; s->think=20; s->gok=0;   // home again: walks in from the edge
              static const u8 dn[HN_N]={25,15,20,15,10,20,0}; for(int n=0;n<HN_N;n++) s->need[n]=(u8)(s->need[n]>dn[n]?s->need[n]-dn[n]:0);
              hhNote(s,s->use==2?" IS BACK FROM SCHOOL":" IS HOME FROM WORK"); continue; }
          if(due&&s->act!=HA_LEAVE&&!planned){   // time to go: walk to the way off (one search per step, like every plan)
              s->use=(u8)k; hhGX=hhExX; hhGY=hhExY; planned=1;
              int r=hhPlan(s,1); if(r>1) s->act=HA_LEAVE; else { s->act=HA_AWAY; hhNote(s,k==2?" WENT TO SCHOOL":" LEFT FOR WORK"); }
              continue; }
          if(due&&s->act!=HA_LEAVE) continue; }
        if(s->act==HA_USE){   // using furniture: refill, then free again (a night's sleep lasts until it is over, rested or not)
            if(s->need[s->use]<100&&(lfr&1)) s->need[s->use]++;
            if(--s->t<=0||(s->need[s->use]>=100&&!(s->use==HN_REST&&simIsNight()))){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); }
            continue;
        }
        if(s->act==HA_WALK||s->act==HA_WANDER||s->act==HA_SEEK||s->act==HA_LEAVE){   // follow the path, tile centre to tile centre
            if(s->pi>=s->pn&&s->act==HA_SEEK){ hhArrive(m); continue; }
            if(s->pi>=s->pn&&s->act==HA_LEAVE){ s->act=HA_AWAY; hhNote(s,s->use==2?" WENT TO SCHOOL":" LEFT FOR WORK"); continue; }
            if(s->pi>=s->pn){ if(s->act==HA_WALK){ s->act=HA_USE; s->t=hhUseT(s); } else { s->act=HA_IDLE; if(s->need[HN_FUN]<90) s->need[HN_FUN]+=10; } continue; }
            hhStepAlong(s);
            continue;
        }
        if(s->act==HA_IDLE&&(lfr&15)==(m&15)&&hhTaken((int)(s->fx>>8),(int)(s->fy>>8),s)){   // standing on someone: one step aside
            int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8), d0=rnd8()&3;
            for(int k=0;k<4;k++){ int d=(d0+k)&3, nx=sx+hhDx[d], ny=sy+hhDy[d];
                if(hhWalk(nx,ny)&&!hhTakenAt(nx,ny)){ s->path[0]=(u8)d; s->pn=1; s->pi=0; s->gok=0; s->act=HA_WANDER; s->use=HN_FUN; break; } }
            if(s->act!=HA_IDLE) continue; }
        if(--s->think<=0&&!planned&&m==hhPlanNext%hhN){   // one plan per step (BFS is the costly part)
            hhDecide(s); planned=1; s->think=(short)(xo[XO_FREEWILL]==1?HH_THINK*2:HH_THINK)+(rnd8()&31);
        }
    }
    hhPlanNext++;
}

// ---- SOCIAL: interactions, acceptance, relationships ----
// The player presses R next to a household Sim (a menu like the Sims' pie menu); free will makes Sims start them too, with each other and
// with you. Whether it is ACCEPTED depends on how the target feels about the one asking (daily score), its traits, its mood and age.
// Accepted: both feel better about each other and their SOCIAL (and sometimes FUN) fills. Rejected: the asker is embarrassed, and likes the
// other a bit less. Mean ones (ARGUE, INSULT, SLAP) always land: the target likes the asker less.
// Statuses follow the scores: FRIEND (daily 50+), BEST FRIEND (daily and lifetime 70+), ENEMY (daily -50 or less), and the romance
// steps CRUSH (a flirt was accepted), IN LOVE (kissed, and lifetime 60+ both ways), STEADY (asked and said yes). Daily drifts back to
// lifetime over the hours, so friendships need keeping up.
enum { SA_ROM=1, SA_MEAN=2, SA_CRUSH=4, SA_LOVE=8, SA_KID=16, SA_PIPE=32 };   // SA_PIPE: grown-ups, with a water pipe in the house
typedef struct { const char* name; signed char dA,lA,dR,lR; u8 soc,fun; signed char minD,maxD; u8 base,tr,fl,icA,icR; const char*say,*yes,*no; } SocAct;
enum { SC_TALK, SC_JOKE, SC_COMPL, SC_HIGH5, SC_HUG, SC_TRICK, SC_FLIRT, SC_KISS, SC_STEADY, SC_SORRY, SC_ARGUE, SC_INSULT, SC_SLAP, SC_PUNCH, SC_PASS, SC_N };
static const SocAct socT[SC_N]={
  //  name           dA  lA  dR  lR soc fun minD maxD base trait   flags                icon yes  icon no     you say  they did        they did not
    {"TALK",          3,  1, -2,  0, 22,  0,-100, 100, 85,TR_OUT, SA_KID,              IC_TALK, IC_BAIL, "BLAH BLAH","CHATTED",     "IGNORED YOU"},
    {"JOKE",          6,  2, -4, -1, 14, 10, -20, 100, 55,TR_PLAY,SA_KID,              IC_STAR, IC_BAIL, "HA HA",   "LAUGHED",       "DID NOT LAUGH"},
    {"COMPLIMENT",    5,  2, -3, -1, 14,  0,   0, 100, 65,TR_NICE,SA_KID,              IC_STAR, IC_BAIL, "NICE",    "BLUSHED",       "SHRUGGED"},
    {"HIGH FIVE",     5,  1, -3,  0, 12,  6,  15, 100, 70,TR_ACT, SA_KID,              IC_HAND, IC_BAIL, "UP TOP",  "HIGH FIVED",    "LEFT YOU HANGING"},
    {"HUG",           9,  3, -8, -2, 20,  0,  35, 100, 55,TR_NICE,SA_KID,              IC_HEART,IC_BAIL, "HUG",     "HUGGED YOU",    "PUSHED AWAY"},
    {"SHOW A TRICK",  4,  1, -2,  0, 12, 12, -10, 100, 65,TR_PLAY,SA_KID,              IC_STAR, IC_GLASS,"WATCH",   "WAS IMPRESSED", "WAS BORED"},
    {"FLIRT",         7,  2, -7, -2, 16,  6,  20, 100, 45,TR_OUT, SA_ROM,              IC_HEART,IC_BAIL, "HEY YOU", "FLIRTED BACK",  "REJECTED YOU"},
    {"KISS",         12,  5,-12, -4, 22,  8,  55, 100, 55,TR_OUT, SA_ROM|SA_CRUSH,     IC_HEART,IC_BAIL, "MWAH",    "KISSED YOU",    "TURNED AWAY"},
    {"GO STEADY",    15, 10,-15, -6, 22,  0,  70, 100, 65,TR_NICE,SA_ROM|SA_LOVE,      IC_HEART,IC_BAIL, "BE MINE",  "SAID YES",      "SAID NO"},
    {"APOLOGIZE",    12,  4, -3,  0, 10,  0,-100,  -5, 55,TR_NICE,SA_KID,              IC_TALK, IC_ANGRY,"SORRY",   "FORGAVE YOU",   "IS STILL MAD"},
    {"ARGUE",        -8, -3,  0,  0,  6,  0,-100, 100,100,TR_NICE,SA_MEAN|SA_KID,      IC_ANGRY,IC_ANGRY,"GRR",     "ARGUED BACK",   ""},
    {"INSULT",      -10, -4,  0,  0,  4,  0,-100,  30,100,TR_NICE,SA_MEAN|SA_KID,      IC_SAD,  IC_SAD,  "LOSER",   "LOOKS HURT",    ""},
    {"SLAP",        -16, -6,  0,  0,  4,  0,-100, -20,100,TR_NICE,SA_MEAN,             IC_HURT, IC_HURT, "SMACK",   "GOT SLAPPED",   ""},
    {"PUNCH",       -20, -8,  0,  0,  4,  0,-100,   0,100,TR_NICE,SA_MEAN,             IC_HURT, IC_HURT, "TAKE THAT","GOT PUNCHED",  ""},   // teens and up; neutral or worse; takes HP (fightHit)
    {"PUFF PUFF PASS", 6,  2, -3,  0, 14, 14, -10, 100, 80,TR_PLAY,SA_PIPE,             IC_LEAF, IC_BAIL, "PASS IT", "TOOK A HIT",    "PASSED"},
};
static int hhFreeUid(void){ for(int u=0;u<HU_N;u++){ if(u==hhPUid) continue; int k=0; for(int m=0;m<hhN;m++) if(hhM[m].uid==u) k=1; if(!k) return u; } return 0; }
static int hhOthers(void){ return hhN>0; }
static int hhMemOf(int uid){ for(int m=0;m<hhN;m++) if(hhM[m].uid==uid) return m; return -1; }   // -1: the player (or nobody)
static int uStage(int u){ int m=hhMemOf(u); return m<0?stage:hhM[m].stage; }
static int uTr(int u,int t){ int m=hhMemOf(u); return m<0?pTr[t]:hhM[m].tr[t]; }
static int uMood(int u){ int m=hhMemOf(u); if(m<0) return moodHapPct(); const HhSim*s=&hhM[m]; int v=0; for(int n=0;n<HN_N;n++) v+=s->need[n]; return v/HN_N; }
static const char* uName(int u){ int m=hhMemOf(u); return m<0?hhPName:hhM[m].name; }
static int ageBand(int st){ return st<AG_TEEN?0:st==AG_TEEN?1:2; }   // romance only within a band: teens with teens, adults with adults and elders
static int romOk(int a,int b){ int sa=uStage(a), sb=uStage(b); return sa>=AG_TEEN&&sb>=AG_TEEN&&ageBand(sa)==ageBand(sb); }
static int hhRomanceOk(void){ for(int m=0;m<hhN;m++) if(romOk(hhPUid,hhM[m].uid)) return 1; return 0; }
static int clampR(int v){ return v<-100?-100:v>100?100:v; }
static const char* relWord(int a,int b){   // how a sees b
    u8 f=relF[a][b]; int d=relD[a][b], l=relL[a][b];
    if(f&RF_STEADY) return "STEADY"; if(f&RF_LOVE) return "IN LOVE"; if(f&RF_CRUSH) return "CRUSH";
    if(d>=70&&l>=70) return "BEST FRIEND"; if(d>=50) return "FRIEND"; if(d<=-50) return "ENEMY"; if(d<=-20) return "DISLIKE";
    if(d==0&&l==0) return "STRANGER"; return "ACQUAINTANCE";
}
static int uPipeOk(int u){ return u==hhPUid?pipeOk():uStage(u)>=AG_ADULT; }   // you: PIPE AGE applies; members: grown-ups (they keep no days-in-stage)
static int socAllowed(int a,int b,int i){   // may a do interaction i to b now?
    const SocAct*S=&socT[i]; int d=relD[a][b];
    if(d<S->minD||d>S->maxD) return 0;
    if(!(S->fl&SA_KID)&&(uStage(a)<AG_TEEN||uStage(b)<AG_TEEN)) return 0;
    if((S->fl&SA_ROM)&&!romOk(a,b)) return 0;
    if((S->fl&SA_PIPE)&&(!uPipeOk(a)||!uPipeOk(b)||!(simHave&SR_PIPE))) return 0;
    if((S->fl&SA_CRUSH)&&!(relF[a][b]&RF_CRUSH)) return 0;
    if((S->fl&SA_LOVE)&&(!(relF[a][b]&RF_LOVE)||(relF[a][b]&RF_STEADY))) return 0;
    if(i==SC_TRICK&&uStage(a)<AG_CHILD) return 0;
    return 1;
}
static void needAdd(int u,int n,int v){   // a need of anyone (n: HN_SOC or HN_FUN)
    int m=hhMemOf(u);
    if(m<0){ if(n==HN_SOC){ sSoc+=v; if(sSoc>100) sSoc=100; if(sSoc<0) sSoc=0; } else moodFun=moodClamp(moodFun+v*MOOD_ONE); return; }
    int x=hhM[m].need[n]+v; hhM[m].need[n]=(u8)(x<0?0:x>100?100:x);
}
static void relMilestones(int a,int b){   // statuses that just started: notes and (for you) wants and fears
    u8*f=&relF[a][b]; int d=relD[a][b], l=relL[a][b], you=(a==hhPUid);
    if(d>=50&&!(*f&RF_FRIEND)){ *f|=RF_FRIEND; if(you){ simEvent(SE_FRIEND); simCat(simCat(simMsg2,"NEW FRIEND: "),uName(b)); simQueue(simMsg2); } }
    if(d<40) *f&=~RF_FRIEND;
    if(d>=70&&l>=70&&!(*f&RF_BFF)){ *f|=RF_BFF; if(you){ simEvent(SE_BFF); simCat(simCat(simMsg2,"BEST FRIENDS: "),uName(b)); simQueue(simMsg2); } }
    if(l<60) *f&=~RF_BFF;
    if(d<=-50&&!(*f&RF_ENEMY)){ *f|=RF_ENEMY; if(you){ simEvent(SE_ENEMY); simCat(simCat(simMsg2,"NEW ENEMY: "),uName(b)); simQueue(simMsg2); } }
    if(d>-40) *f&=~RF_ENEMY;
    if((*f&RF_KISSED)&&!(*f&RF_LOVE)&&l>=60&&relL[b][a]>=60){ *f|=RF_LOVE; relF[b][a]|=RF_LOVE; if(you||b==hhPUid){ simEvent(SE_LOVE); simQueue("IN LOVE"); } }
}
static void hhSay(int u,int icon,const char* word){   // a balloon over someone's head: an icon for household Sims, a word for you
    int m=hhMemOf(u); if(m<0){ hhBubTxt=word; hhBubT=90; } else { hhM[m].bub=(u8)icon; hhM[m].bubT=90; }
}
static void hhFreeze(int u,int steps){ int m=hhMemOf(u); if(m<0){ if(lstun<steps) lstun=steps; lsp=0; lgrind=0; } else { hhM[m].act=HA_SOC; hhM[m].t=(short)steps; } }
static void socNote(int a,int b,int i,int ok){   // what you read when you are part of it: "REX LAUGHED +6"
    if(a!=hhPUid&&b!=hhPUid) return;
    const SocAct*S=&socT[i]; char*e=simMsg2;
    if(a==hhPUid){ e=simCat(e,uName(b)); *e++=' '; e=simCat(e,ok?S->yes:S->no); }
    else { e=simCat(e,uName(a)); *e++=' '; const char*w=S->name; char lw[16]; int k=0; for(;w[k]&&k<15;k++) lw[k]=w[k]; lw[k]=0;
        e=simCat(e,(S->fl&SA_MEAN)?(i==SC_PUNCH?"PUNCHED YOU":i==SC_SLAP?"SLAPPED YOU":i==SC_ARGUE?"PICKED A FIGHT":"INSULTED YOU"):i==SC_TALK?"CAME TO CHAT":i==SC_FLIRT?"FLIRTS WITH YOU":i==SC_KISS?"KISSED YOU":i==SC_HUG?"HUGS YOU":i==SC_STEADY?"ASKS YOU OUT":lw); }
    lnote=simMsg2; lnoteT=110;
}
// ---- FIGHTING: PUNCH takes HP from the one hit. Damage 14..26, more from active (TR_ACT) Sims. Nobody dies in a fight: at 0 HP the Sim is
// knocked out (a household Sim lies still for 10 s and wakes at 30 HP; you wake at 25, see fightHurt in main.c). ----
// What a Sim is made of (its creator parts and sliders; lk = its look, yours is the global `look`) decides how it fights:
//   HITS HARDER  horns (NUBS +2, HORNS +6: a HEADBUTT; HORN SIZE and HORN THICKNESS add -2..+2, a big head +2 more on a headbutt),
//                claws (CLAWS +2: a SCRATCH, PINCERS +6: a PINCH, BLADES +5: a SLASH that cuts through a quarter of the armour),
//                a tail (STUB +1, LONG +3, and TAIL LENGTH / THICKNESS -2..+2 on a long one), broad SHOULDERS up to +2,
//                a heavy build (WEIGHT) up to +4, big hands (HAND FOOT SIZE) up to +3
//   TAKES LESS   SPIKES -30% (and they prick the one that hits: 4 HP back), a helmet -15%, a thick skull (HORNS) -10% and a big head up to 4% more, a heavy build 1% a notch
//   DODGES       EYE STALKS 20% (+ ANT LENGTH), FEELERS 8% (+ ANT LENGTH), WINGS 15% (+ WING SIZE), a long tail 5% (+ TAIL LENGTH)
static const u8* fkLook(int u){ int m=hhMemOf(u); return m<0?look:hhM[m].look; }
static int fkAtk(const u8*lk){
    int cl=lk[LK_CLAWS], hn=lk[LK_HORNS], tl=lk[LK_TAIL];
    int d=(hn==2?6:hn==1?2:0)+(cl==3?5:cl==2?6:cl==1?2:0)+(tl==2?3:tl==1?1:0), h=slideEff(lk[LK_HANDFT]);
    if(hn){ int hs=(slideEff(lk[LK_HORNSZ])+slideEff(lk[LK_HORNTH]))/3; d+=hs; if(hn==2&&slideEff(lk[LK_HEADSZ])>0) d+=slideEff(lk[LK_HEADSZ])/2; }   // bigger, thicker horns on a bigger head
    if(tl==2) d+=(slideEff(lk[LK_TAILLEN])+slideEff(lk[LK_TAILTHK]))/3;                                                                              // a long, thick tail whips harder
    if(slideEff(lk[LK_SHOULW])>0) d+=slideEff(lk[LK_SHOULW])/2;                                                                                       // broad shoulders
    return d+slideEff(lk[LK_WEIGHT])+(h>3?3:h<-3?-3:h);
}
static int fkTaken(const u8*lk){ int hd=slideEff(lk[LK_HEADSZ]); int p=100-(lk[LK_BACK]==1?30:0)-(lk[LK_HAT]==5?15:0)-(lk[LK_HORNS]==2?10+(hd>0?hd:0):0)-slideEff(lk[LK_WEIGHT]); return p<30?30:p; }   // percent of a blow that gets through
static int fkDodge(const u8*lk){
    int d=(lk[LK_ANTENNA]==2?20+2*slideEff(lk[LK_ANTLEN]):lk[LK_ANTENNA]==1?8+slideEff(lk[LK_ANTLEN]):0)+(lk[LK_BACK]==2?15+2*slideEff(lk[LK_WINGSZ]):0)+(lk[LK_TAIL]==2?5+slideEff(lk[LK_TAILLEN])/2:0);
    return d<0?0:d>60?60:d;
}
static const char* fkMove(int u,const char*def){ const u8*lk=fkLook(u); return lk[LK_HORNS]==2?"HEADBUTT":lk[LK_CLAWS]==3?"SLASH":lk[LK_CLAWS]==2?"PINCH":lk[LK_CLAWS]==1?"SCRATCH":def; }   // what the blow is called
static void fkLose(int u,int n){ int m=hhMemOf(u); if(m<0){ lhp-=n; if(lhp<1) lhp=1; } else { int v=hhM[m].hp-n; hhM[m].hp=(u8)(v<1?1:v); } }   // recoil never knocks anyone out
static char fkB[28];   // what the one who lands a blow reads: "CRIT SLASH 27"
static void fightHit(int a,int b){
    const u8*la=fkLook(a), *lb=fkLook(b);
    if((rnd8()*100>>8)<fkDodge(lb)){ hhSay(b,IC_BAIL,"DODGED"); return; }   // eye stalks see it coming, wings flap clear
    int dmg=14+(uTr(a,TR_ACT)>>1)+(rnd8()>>5)+fkAtk(la);
    { int tk=fkTaken(lb); if(la[LK_CLAWS]==3) tk+=(100-tk)/4; dmg=dmg*tk/100; } if(dmg<1) dmg=1;   // BLADES cut through a quarter of the armour
    int crit=(rnd8()*100>>8)<8+(la[LK_CLAWS]==3?10:0)+(uTr(a,TR_ACT)>>2);   // a CRITICAL HIT: one blow in twelve or so (BLADES and active Sims more) lands half as hard again
    if(crit){ dmg=dmg*3/2; hhSay(b,IC_BAIL,"CRIT"); }
    if(lb[LK_BACK]==1&&dmg>3){ fkLose(a,4); if(a==hhPUid&&lnoteT<=0){ lnote="OUCH  SPIKES"; lnoteT=40; } }   // spikes prick the one that hits them
    int m=hhMemOf(b);
    if(m<0){ fightHurt(dmg); return; }
    HhSim*t=&hhM[m]; if(t->hp>dmg){ t->hp=(u8)(t->hp-dmg);
        if(a==hhPUid&&lnoteT<=0){ char*e=fkB; if(crit) e=simCat(e,"CRIT "); e=simCat(e,fkMove(a,"PUNCH")); *e++=' '; simCatN(e,dmg); lnote=fkB; lnoteT=45; } }   // you see how hard it landed
    else { t->hp=30; t->act=HA_SOC; t->t=600; t->bub=IC_SKULL; t->bubT=120; hhNote(t," IS KNOCKED OUT"); if(a==hhPUid) voxPlay(V_win_the_fight); }
}
// a does interaction i to b. Returns 1 if it was accepted (mean ones: 1 = it landed)
// The voice of the Sim you control in a social (you are a or b). One clip at a time; the newest wins.
static void voxSoc(int a,int b,int i,int ok){
    int ya=(a==hhPUid), yb=(b==hhPUid); if(!ya&&!yb) return;
    const SocAct*S=&socT[i]; int rom=((relF[a][b]|relF[b][a])&(RF_CRUSH|RF_LOVE|RF_STEADY))!=0, r=rnd8()&1;
    if(S->fl&SA_MEAN){   // a fight: you start it, or you are the one it is aimed at
        if(ya) voxPlay((i==SC_ARGUE||i==SC_PUNCH)?V_lets_fight:V_amgry); else voxPlay(r?V_angered:V_amgry);
        return; }
    switch(i){
        case SC_TALK: case SC_COMPL: case SC_SORRY: voxPlay(ok?V_agree:(r?V_not_agree:V_disagree)); break;
        case SC_HIGH5: voxPlay(ok?(r?V_yeha:V_yahoo):V_nah); break;
        case SC_TRICK: voxPlay(ok?V_yahoo:V_nah); break;
        case SC_HUG: if(!ok) voxPlay(V_nah); break;
        case SC_JOKE:
            if(rom&&ok) voxPlay(r?V_naughty_joke:V_naughty_jokw_2);                        // a joke between two who fancy each other
            else if(ya) voxPlay(ok?V_joke_good:V_joke_not_land);                           // you tell it
            else voxPlay(ok?(r?V_laughing:V_laughing_2):V_laughing_small_or_not);          // you hear it
            break;
        case SC_FLIRT: voxPlay(ok?(ya?V_flirt:V_flirt_2):V_nah); break;
        case SC_KISS: voxPlay(ok?V_flirt_2:(ya?V_burst_crying:V_nah)); break;
        case SC_STEADY: voxPlay(ok?V_serenade_good:V_serenade_bad); break;              // BE MINE: a serenade, good or bad
        default: break;   // PUFF PUFF PASS: the pipe sounds come from voxEvent
    }
}
static int socDo(int a,int b,int i){
    const SocAct*S=&socT[i]; int ok;
    if(S->fl&SA_MEAN) ok=1;
    else {
        int c=S->base+relD[b][a]/2+(uTr(b,S->tr)-5)*4+(uMood(b)-50)/5;
        if(S->fl&SA_ROM){ if(relF[b][a]&RF_CRUSH) c+=20; for(int u=0;u<HU_N;u++) if(u!=a&&(relF[b][u]&RF_STEADY)) c-=40; }   // taken: jealousy
        if(uTr(b,TR_OUT)<=2&&relD[b][a]<20) c-=10;   // shy with people it hardly knows
        c=c<5?5:c>95?95:c; ok=(rnd8()*100>>8)<c;
    }
    hhFreeze(a,80); hhFreeze(b,80);
    hhSay(a,S->icA,i==SC_PUNCH?fkMove(a,S->say):S->say);
    if(S->fl&SA_MEAN){
        relD[b][a]=(signed char)clampR(relD[b][a]+S->dA); relL[b][a]=(signed char)clampR(relL[b][a]+S->lA);
        relD[a][b]=(signed char)clampR(relD[a][b]+S->dA/2);
        needAdd(b,HN_SOC,-S->soc); needAdd(a,HN_SOC,S->soc); if(uTr(a,TR_NICE)<=3) needAdd(a,HN_FUN,8);   // grouchy Sims enjoy it a little
        hhSay(b,S->icR,i==SC_PUNCH?"OOF":i==SC_SLAP?"OW":i==SC_ARGUE?"GRR":"HEY");
        if(i==SC_PUNCH){   // the blow lands, and a Sim that is not out cold hits back (grouchy ones nearly always)
            fightHit(a,b);
            int bm=hhMemOf(b);
            if(bm>=0&&hhM[bm].t<500&&(rnd8()*100>>8)<70-uTr(b,TR_NICE)*5) fightHit(b,a);
        }
        if(b==hhPUid){ simEvent((i==SC_SLAP||i==SC_PUNCH)?SE_SLAPPED:SE_FIGHT); moodEventN(M_FEAR,(i==SC_SLAP||i==SC_PUNCH)?2:1); }
        if(a==hhPUid&&(i==SC_ARGUE||i==SC_PUNCH)) simEvent(SE_FIGHT);
    } else if(ok){
        relD[b][a]=(signed char)clampR(relD[b][a]+S->dA); relL[b][a]=(signed char)clampR(relL[b][a]+S->lA);
        relD[a][b]=(signed char)clampR(relD[a][b]+S->dA*2/3); relL[a][b]=(signed char)clampR(relL[a][b]+S->lA*2/3);
        needAdd(a,HN_SOC,S->soc); needAdd(b,HN_SOC,S->soc); if(S->fun){ needAdd(a,HN_FUN,S->fun); needAdd(b,HN_FUN,S->fun); }
        if(i==SC_FLIRT){ relF[a][b]|=RF_CRUSH; relF[b][a]|=RF_CRUSH; }
        if(i==SC_PASS&&(a==hhPUid||b==hhPUid)){ if(lchill<1200) lchill=1200; moodEvent(M_CHILL); simEvent(SE_PIPE); }   // passed round: you chill out too
        if(i==SC_KISS){ int first=!(relF[a][b]&RF_KISSED); relF[a][b]|=RF_KISSED; relF[b][a]|=RF_KISSED; if(first&&(a==hhPUid||b==hhPUid)) simEvent(SE_KISS); }
        if(i==SC_STEADY){ relF[a][b]|=RF_STEADY; relF[b][a]|=RF_STEADY; if(a==hhPUid||b==hhPUid){ simEvent(SE_STEADY); simQueue("GOING STEADY"); } }
        if(a==hhPUid||b==hhPUid){ simEvent(SE_TALK); if(i==SC_JOKE) simEvent(SE_LAUGH); if(i==SC_HUG) simEvent(SE_HUGGED); moodEvent(M_WANT); }
        hhSay(b,S->icA,i==SC_JOKE?"HA HA":i==SC_HUG||i==SC_KISS?"AWW":i==SC_STEADY?"YES":i==SC_PASS?"NICE":"YEAH");
    } else {
        relD[a][b]=(signed char)clampR(relD[a][b]+S->dR); relL[a][b]=(signed char)clampR(relL[a][b]+S->lR); relD[b][a]=(signed char)clampR(relD[b][a]+S->dR/2);
        needAdd(a,HN_SOC,-6);
        if(S->fl&SA_ROM){ relF[a][b]&=~RF_CRUSH; }
        if(a==hhPUid){ if(S->fl&SA_ROM) simEvent(SE_REJECT); moodEvent(M_BUMP); }
        hhSay(b,S->icR,"NO");
    }
    relMilestones(a,b); relMilestones(b,a);
    socNote(a,b,i,ok);
    voxSoc(a,b,i,ok);
    return ok;
}
static void relTick(void){   // every game hour (900 steps): daily scores drift one step back towards lifetime
    static int t; if(++t<900) return; t=0;
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ int d=relD[a][b], l=relL[a][b]; if(d>l) relD[a][b]--; else if(d<l) relD[a][b]++; }
}
// ---- free will socials ----
static int uTile(int u,int*x,int*y){ int m=hhMemOf(u); if(m<0){ *x=(int)(lfx>>8); *y=(int)(lfy>>8); return 1; } *x=(int)(hhM[m].fx>>8); *y=(int)(hhM[m].fy>>8); return hhM[m].act!=HA_USE&&hhM[m].act<HA_LEAVE; }
static void hhSeek(HhSim*s){   // pick someone to go and see: friends most, enemies when grouchy, anyone when lonely enough
    int best=-1, bs=-999, me=s->uid;
    for(int u=0;u<HU_N;u++){ if(u==me) continue; if(u!=hhPUid&&hhMemOf(u)<0) continue;
        int x,y; if(!uTile(u,&x,&y)) continue;
        int d=relD[me][u], sc=d+(rnd8()&31); if(s->tr[TR_NICE]<=3&&d<-20) sc=-d+(rnd8()&31);   // grouchy Sims go looking for trouble
        if(relF[me][u]&(RF_CRUSH|RF_LOVE|RF_STEADY)) sc+=30;
        if(relD[me][u]==0&&relL[me][u]==0) sc+=20+s->tr[TR_OUT]*4;   // someone new: go and say hello (outgoing Sims more)
        if(u==hhPUid) sc+=15;                                          // and they like to come and see you
        if(sc>bs){ bs=sc; best=u; } }
    if(best<0){ s->act=HA_IDLE; return; }
    uTile(best,&hhGX,&hhGY); s->tgt=(u8)best;
    int r=hhPlan(s,1); if(r==1){ s->act=HA_SEEK; s->pn=s->pi=0; } else if(r>1) s->act=HA_SEEK; else s->act=HA_IDLE;
}
static int socPick(int a,int b){   // what a free-will Sim says to b
    int d=relD[a][b], nice=uTr(a,TR_NICE), r=rnd8();
    if(d<-30||(nice<=2&&r<40)){ if(d<-30&&socAllowed(a,b,SC_PUNCH)&&r<50) return SC_PUNCH; if(socAllowed(a,b,SC_SLAP)&&r<70) return SC_SLAP; return (r&1)?SC_ARGUE:SC_INSULT; }
    if(d<-5&&nice>=6&&socAllowed(a,b,SC_SORRY)) return SC_SORRY;
    if(socAllowed(a,b,SC_STEADY)&&r<90) return SC_STEADY;
    if(socAllowed(a,b,SC_KISS)&&r<120) return SC_KISS;
    if(socAllowed(a,b,SC_FLIRT)&&uTr(a,TR_OUT)>=5&&r<70) return SC_FLIRT;
    int pool[8], n=0;
    pool[n++]=SC_TALK; if(socAllowed(a,b,SC_JOKE)&&uTr(a,TR_PLAY)>=4) pool[n++]=SC_JOKE; if(socAllowed(a,b,SC_COMPL)&&nice>=5) pool[n++]=SC_COMPL;
    if(socAllowed(a,b,SC_PASS)&&simMin>=16*60+20&&simMin<17*60+20) return SC_PASS;   // 4:20
    if(socAllowed(a,b,SC_PASS)&&uTr(a,TR_PLAY)>=4) pool[n++]=SC_PASS;
    if(socAllowed(a,b,SC_HIGH5)) pool[n++]=SC_HIGH5; if(socAllowed(a,b,SC_HUG)&&nice>=4) pool[n++]=SC_HUG; if(socAllowed(a,b,SC_TRICK)&&uTr(a,TR_ACT)>=5) pool[n++]=SC_TRICK;
    return pool[(r*n)>>8];
}
static void hhArrive(int m){   // a free-will Sim reached the one it wanted to see
    HhSim*s=&hhM[m]; int b=s->tgt, x, y; s->act=HA_IDLE; s->think=(short)HH_THINK;
    if(!uTile(b,&x,&y)) return;
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if((sx-x)*(sx-x)+(sy-y)*(sy-y)>4) return;   // they walked off
    int bm=hhMemOf(b); if(bm>=0&&(hhM[bm].act==HA_SOC||hhM[bm].act==HA_USE)) return;
    if(bm<0&&(lstun>0||simAct||lz>(s32)(surfH(lfx,lfy)<<8))) return;   // you are busy, or in the air
    s->hd=(u8)(x>sx?0:x<sx?8:y>sy?4:12); if(bm>=0) hhM[bm].hd=(u8)((s->hd+8)&15);
    socDo(s->uid,b,socPick(s->uid,b));
}
// ---- you: R next to a household Sim opens the social menu (furniture you stand at is offered first) ----
static int hhNearest(void){ if(curFl) return -1; int best=-1, bd=1<<30; for(int m=0;m<hhN;m++){ if(hhM[m].act==HA_AWAY) continue; s32 dx=hhM[m].fx-lfx, dy=hhM[m].fy-lfy; int d=(int)((dx*dx+dy*dy)>>8); if(d<bd){ bd=d; best=m; } } return bd<=(380*380>>8)?best:-1; }   // within 1.5 tiles
static void liveInvalidate(void);
static int hhSocR(int useLabel){   // 1 = handled (a social, or the menu was closed), 0 = go on and use the furniture
    int m=hhNearest(); if(m<0) return 0;
    HhSim*s=&hhM[m]; int b=s->uid, a=hhPUid;
    if(s->act==HA_USE){ lnote="THEY ARE BUSY"; lnoteT=50; return 0; }
    static const char* it[SC_N+1]; static char tl[40]; int id[SC_N+1], n=0;
    static const char* const useNm[6]={0,"USE THE FRIDGE","USE THE TOILET","SLEEP IN BED","TAKE A SHOWER","SIT ON SOFA"};
    if(useLabel>0&&useLabel<6){ it[n]=useNm[useLabel]; id[n++]=-1; }
    for(int i=0;i<SC_N;i++) if(socAllowed(a,b,i)){ it[n]=i==SC_PUNCH?fkMove(a,"PUNCH"):socT[i].name; id[n++]=i; }
    { char*e=simCat(tl,s->name); *e++=' '; *e++=' '; e=simCat(e,relWord(a,b)); e=simCat(e,"  HP "); simCatN(e,s->hp); }
    int c=menu(tl,it,n); liveInvalidate();
    while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]<0) return 0;
    int px=(int)(lfx>>8), py=(int)(lfy>>8), sx=(int)(s->fx>>8), sy=(int)(s->fy>>8);
    s->hd=(u8)(px>sx?0:px<sx?8:py>sy?4:12); lhd=(s->hd+8)&15;
    socDo(a,b,id[c]);
    return 1;
}
// ---- the RELATIONSHIPS screen (pause menu > HOUSEHOLD): how you feel about everyone, and how they feel about you ----
static void relBar(int x,int y,int v){   // -100..100 around a centre line, green above 0, red below
    rect(x,y,61,4,RGB(3,4,8)); rect(x+30,y-1,1,6,RGB(14,16,20));
    int w=v*30/100; if(w>0) rect(x+31,y,w,4,RGB(8,26,8)); else if(w<0) rect(x+30+w,y,-w,4,RGB(28,8,6));
}
static void hhInvite(void);   // (below, next to the redraw bookkeeping)
static void relScreen(void){
    u16 prev=keyNow(); int top=0;   // seven rows fit: UP / DOWN scroll a bigger household
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) return;
        if((pr&K_DOWN)&&top+7<hhN) top++; if((pr&K_UP)&&top>0) top--;
        if((pr&K_SEL)&&dbgOn){ hhInvite(); prev=keyNow(); }   // SELECT: a new Sim moves in (DEBUG CODE only)
        box(3,1,234,157); char t[44]; { char*e=simCat(simCat(t,"RELATIONSHIPS OF "),hhPName); if(hhPLast[0]){ *e++=' '; simCat(e,hhPLast); } } text(10,6,t,GOLD,1);
        text(84,16,"YOU TO THEM",DIMC,1); text(162,16,"THEM TO YOU",DIMC,1);
        if(!hhN) text(10,40,"NO ONE ELSE LIVES HERE",DIMC,1);
        for(int m=top;m<hhN&&m<top+7;m++){ int y=26+(m-top)*18, b=hhM[m].uid, a=hhPUid;
            text(10,y,hhM[m].name,WHITE,1); text(10,y+8,relWord(a,b),(relF[a][b]&(RF_LOVE|RF_STEADY|RF_CRUSH))?RGB(31,14,20):relD[a][b]<=-20?RGB(30,10,8):RGB(16,26,16),1);
            relBar(84,y+1,relD[a][b]); relBar(84,y+8,relL[a][b]); relBar(162,y+1,relD[b][a]); relBar(162,y+8,relL[b][a]);
            if(relF[a][b]&RF_STEADY) simIcon(226,y+2,IC_HEART,RGB(31,14,20)); }
        text(10,150,dbgOn?(hhN>7?"UP DOWN MORE  SELECT ADD A SIM":"TOP DAILY  LOW LIFETIME  SELECT ADD A SIM"):(hhN>7?"UP DOWN MORE  A OR B BACK":"TOP DAILY  LOW LIFETIME"),RGB(12,14,16),1);
        present();
    }
}
// ---- drawing: like the player, inside drawRoomRect's back-to-front walk. hhCalc (once a picture) works out where everyone is ----
static int hhX[HH_MAX], hhY[HH_MAX], hhB[HH_MAX], hhV[HH_MAX], hhH[HH_MAX];   // feet on screen, band (tile x+y), view, floor height
static void hhCalc(void){   // (the places above hhN that hold a visitor too)
    for(int m=0;m<HH_MAX;m++){ if(m>=hhN){ int k=HH_MAX-1-m; if(k>=TW_N||!twHas[k]||!twOn[k]) continue; } const HhSim*s=&hhM[m]; s32 rx,ry; rotPos(s->fx,s->fy,&rx,&ry);
        hhX[m]=LOX+(int)((rx-ry)>>5); hhY[m]=LOY+(int)((rx+ry)>>6); hhB[m]=(int)((rx>>8)+(ry>>8)); hhV[m]=faceView[(s->hd+4*cview)&15]; hhH[m]=surfH(s->fx,s->fy); }
}
static void hhDrawBand(int s0,int s1){   // the members whose band is in s0..s1
    for(int m=0;m<hhN;m++){ if(hhB[m]<s0||hhB[m]>s1||hhM[m].act==HA_AWAY) continue;
        if(sShad) rect(hhX[m]-3,hhY[m]-hhH[m]-1,7,2,RGB(10,8,5));   // (the Sim itself is a hardware sprite: hhObjUpdate)
        if(hhM[m].bubT){ int bx=hhX[m]-5, by=hhY[m]-hhH[m]-50; rect(bx,by,11,10,RGB(14,16,22)); rect(bx+1,by+1,9,8,WHITE);   // a balloon with an icon (Sims style)
            simIcon(bx+2,by+1,hhM[m].bub,hhM[m].bub==IC_HEART?RGB(28,6,12):hhM[m].bub==IC_ANGRY||hhM[m].bub==IC_HURT?RGB(26,4,4):RGB(4,4,10)); px(hhX[m],by+10,RGB(14,16,22)); } }
}
typedef struct { short x0,y0,x1,y1; } HhR;   // (hud.h's Rc comes later in main.c)
static void hhRc(int m,HhR*r){   // what a member puts INTO the picture: only its shadow (and a balloon); the body is a hardware sprite
    if(hhM[m].act==HA_AWAY){ r->x0=r->x1=r->y0=r->y1=0; return; }   // off the lot: nothing
    r->x0=(short)(hhX[m]-3); r->x1=(short)(hhX[m]+4); r->y0=(short)(hhY[m]-hhH[m]-1); r->y1=(short)(hhY[m]-hhH[m]+1);
    if(hhM[m].bubT){ int by=hhY[m]-hhH[m]-50; if(r->y0>by) r->y0=(short)by; if(r->x0>hhX[m]-5) r->x0=(short)(hhX[m]-5); if(r->x1<hhX[m]+6) r->x1=(short)(hhX[m]+6); } }
static unsigned hhSig(int m){ return (unsigned)(hhX[m]&0x3FF)|((unsigned)(hhY[m]&0x3FF)<<10)|((unsigned)(hhH[m]&15)<<22)|((unsigned)(hhM[m].bubT?1+(hhM[m].bub&31):0)<<26)^(hhM[m].act==HA_AWAY?0x80000000u:0); }
static int hhBehindAt(s32 fx,s32 fy){   // is a full-height wall in front of this spot (towards the camera)? then a Sim there is drawn see-through
    s32 rx,ry; rotPos(fx,fy,&rx,&ry); int x=(int)(rx>>8), y=(int)(ry>>8);
    static const signed char d[5][2]={{1,0},{0,1},{1,1},{2,1},{1,2}};
    for(int k=0;k<5;k++){ int wx=x+d[k][0], wy=y+d[k][1]; if(!wallAtR(wx,wy)||cellAt(wx,wy)!='W'||sWall==2) continue;
        if(sWall==1&&(wInAt(wx,wy-1)||wInAt(wx-1,wy))) continue;   // that wall is cut away
        return 1; }
    for(int k=0;k<3;k++){ int wx=x+d[k][0], wy=y+d[k][1]; if(wx<0||wy<0||wx>=MW||wy>=MH) continue; char c=cellAt(wx,wy); if(c=='F'||c=='H'||c=='#') return 1; }   // tall furniture right in front (a fridge, a shower)
    return 0;
}
static int hhPlayerFront(int x,int y){   // is the player (drawn into the picture, so under every sprite) standing in front of this Sim's sprite?
    s32 rx,ry; rotPos(lfx,lfy,&rx,&ry); int px=LOX+(int)((rx-ry)>>5), py=LOY+(int)((rx+ry)>>6), dx=px-(x+16);
    return py>y+SPF&&dx>-22&&dx<22&&py<y+SPF+44;
}
typedef struct { const HhSim*s; short x,y; int dd,dep; u8 id,key; } HhOv;   // a Sim in view: where its sprite goes, how far from the middle, how far back
static void hhObjUpdate(void){   // in vblank: hand out OBJ slots, load what changed into OBJ VRAM, write OAM (a Sim is two entries: 32x32 over 32x16), set the window that clips them
    volatile u16*oam=OAM; int i, nOam=0;
    *(volatile u16*)0x04000040=240; *(volatile u16*)0x04000044=(u16)((sbY0<<8)|sbY1);   // WIN0: the room view (the room rows of the screen)
    *(volatile u16*)0x04000048=0x34; *(volatile u16*)0x0400004A=0x04;                   // inside: BG2 + sprites + blend; outside: BG2 only
    *(volatile u16*)0x04000050=0x0400; *(volatile u16*)0x04000052=(6<<8)|10;            // see-through sprites blend 10/16 over the picture
    if(!hhSlotOk) hhSlotsFree();
    if(lcamF>0){ for(i=0;i<2*OBJ_SLOTS;i++) oam[i*4]=0x200; return; }   // the action cam: all off, the slots stay as they are
    if(curFl){ for(i=0;i<2*OBJ_SLOTS;i++) oam[i*4]=0x200; hhSlotsFree(); return; }   // upstairs: no Sim sprites
    // 1. who is in view
    HhOv w[HH_IDS]; int n=0, cx=SW/2, cy=(vpY0+vpY1)/2; u8 vis[HH_IDS]; int ddOf[HH_IDS];
    for(i=0;i<HH_IDS;i++) vis[i]=0;
    for(int id=0;id<HH_IDS;id++){
        const HhSim*s; int x,y,v,f,dep;
        if(id>=hhN){ int k=HH_MAX-1-id; if(k>=TW_N||!twHas[k]||!twOn[k]) continue;   // a visitor
            s=&hhM[id]; x=hhX[id]-16; y=hhY[id]-SPF-hhH[id]; v=hhV[id]; dep=hhB[id];
            f=twOn[k]!=2?((lfr+k*3)>>3)&1:0;   // walking in or out: stepping; staying: standing
        } else {
            if(hhM[id].act==HA_AWAY) continue;
            s=&hhM[id]; x=hhX[id]-16; y=hhY[id]-SPF-hhH[id]; v=hhV[id]; dep=hhB[id];
            int walk=(s->act==HA_WALK||s->act==HA_WANDER||s->act==HA_SEEK||s->act==HA_LEAVE)&&s->pi<s->pn;
            f=walk?((lfr+id*5)>>3)&1:0;   // walking: standing / mid-stride, every 8 frames (each Sim a little out of step)
        }
        if(x+32<=vpX0||x>=vpX1||y+SPH<=vpY0||y>=vpY1) continue;
        HhOv*o=&w[n++]; o->s=s; o->x=(short)x; o->y=(short)y; o->id=(u8)id; o->key=(u8)(v*2+f); o->dep=dep;
        int dx=x+16-cx, dy=y+40-cy; o->dd=(dx<0?-dx:dx)+(dy<0?-dy:dy); ddOf[id]=o->dd; vis[id]=1;
    }
    // 2. slots: free the ones whose Sim left the view, then give the nearest newcomers a slot (if the view holds more Sims than slots, the farthest wait)
    for(int sl=0;sl<OBJ_SLOTS;sl++){ int id=hhSlotId[sl]; if(id>=0&&!vis[id]){ hhSlotOf[id]=-1; hhSlotId[sl]=-1; hhSlotKey[sl]=-1; } }
    for(i=1;i<n;i++){ HhOv t=w[i]; int j=i; while(j>0&&w[j-1].dd>t.dd){ w[j]=w[j-1]; j--; } w[j]=t; }   // nearest first (insertion sort, n <= 16)
    for(i=0;i<n;i++){ int id=w[i].id; if(hhSlotOf[id]>=0) continue;
        int sl=-1; for(int t=0;t<OBJ_SLOTS;t++) if(hhSlotId[t]<0){ sl=t; break; }
        if(sl<0){ int fd=w[i].dd+24; for(int t=0;t<OBJ_SLOTS;t++){ int o=hhSlotId[t]; if(ddOf[o]>fd){ fd=ddOf[o]; sl=t; } }   // steal from the farthest, but only if it is clearly farther (no ping-pong)
                  if(sl<0) continue; hhSlotOf[hhSlotId[sl]]=-1; }
        hhSlotId[sl]=id; hhSlotOf[id]=(signed char)sl; hhSlotKey[sl]=-1;
        const u16*pl=hhPalOf(id); for(int c=0;c<16;c++) OBJ_PAL[sl*16+c]=pl[c];
    }
    // 3. depth order: the Sim nearest the camera gets the lowest OAM entry, so it is drawn over the ones behind it
    int ord[HH_IDS], no=0;
    for(i=0;i<n;i++) if(hhSlotOf[w[i].id]>=0) ord[no++]=i;
    for(i=1;i<no;i++){ int t=ord[i], j=i; while(j>0&&w[ord[j-1]].dep<w[t].dep){ ord[j]=ord[j-1]; j--; } ord[j]=t; }
    // 4. load what changed (a few fresh sprites per frame at most) and write OAM
    int budget=UP_BUDGET;
    for(int r=0;r<no;r++){ const HhOv*o=&w[ord[r]]; int id=o->id, sl=hhSlotOf[id], v=o->key>>1, f=o->key&1, key=o->key;
        if(hhSlotKey[sl]!=key){
            int ov=hhSlotKey[sl], full=ov<0||(ov>>1)!=v;
            if(full&&budget<=0){ if(ov<0) continue; }   // no time left: a new sprite appears next frame, one that only turned keeps its old view for a frame or two
            else { if(full) budget--;
                volatile u16*d=OBJ_VRAM+sl*(OBJ_B/2); hhSlotKey[sl]=(signed char)key;
                if(full){ const u16*sp=(const u16*)hhTiles(id,v); for(int k=0;k<OBJ_B/2;k++) d[k]=sp[k]; }
                const u16*sp=f?(const u16*)hhStrideB(id,v):(const u16*)(hhTiles(id,v)+STR_B0); for(int k=0;k<STR_BN/2;k++) d[STR_B0/2+k]=sp[k]; } }
        volatile u16*e=oam+nOam*4; int tile=512+sl*32, y=o->y, x=o->x, blend=(hhBehindAt(o->s->fx,o->s->fy)||hhPlayerFront(x,y))?0x400:0;
        if(zoomDma){   // ZOOM: scaled up by the hardware (affine, double size: a 64 x 128 box, matrix 0) and placed where its room pixels are on screen
            int X=(x+16-vpX0)*zoomNum/zoomDen-32, Y=sbY0+(y+32-vpY0)*zoomNum/zoomDen-64;
            e[0]=(u16)((Y&255)|blend|0x8000|0x300); e[1]=(u16)((X&511)|0xC000); e[2]=(u16)(tile|(sl<<12));
        } else { e[0]=(u16)((y&255)|blend|0x8000); e[1]=(u16)((x&511)|0xC000); e[2]=(u16)(tile|(sl<<12)); }   // one tall 32 x 64 sprite
        nOam++;
    }
    for(i=nOam;i<2*OBJ_SLOTS;i++) oam[i*4]=0x200;   // everything else off
    oam[3]=zoomPa; oam[7]=0; oam[11]=0; oam[15]=zoomPa;   // affine matrix 0 (the ZOOM's sprites): 1 / scale
}
static HhR hhOld[HH_MAX]; static unsigned hhOldSig[HH_MAX];
static void hhSave(void);
static void hhInvite(void){   // a made-up Sim moves in (pause menu > HOUSEHOLD, or SELECT on the RELATIONSHIPS screen)
    if(!xo[XO_SIMRAND]){ toast("MADE-UP SIMS ARE OFF"); return; }
    if(hhN>=HH_MAX){ toast("THE HOUSE IS FULL"); return; }
    u8 lk[LK_N], st, tr[TR_N]; hhRandLook(lk,&st); for(int i=0;i<TR_N;i++) tr[i]=(u8)(rnd8()%11);
    int m=hhAdd(lk,st,rnd8()%AS_PICK,rnd8()&1,tr); if(m<0) return;
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave();
    static char t[32]; char*e=simCat(t,hhM[m].name); simCat(e," MOVED IN"); toast(t);
}

static void lookTrueRandom(u8*lk,u8*stg);   // main.c: every slider, part and colour at random
static void hhInviteTrue(void){   // a truly random Sim moves in: any age from child to elder, every choice of the creator at random
    if(!xo[XO_SIMRAND]){ toast("MADE-UP SIMS ARE OFF"); return; }
    if(hhN>=HH_MAX){ toast("THE HOUSE IS FULL"); return; }
    u8 lk[LK_N], st=255, tr[TR_N]; lookTrueRandom(lk,&st); for(int i=0;i<TR_N;i++) tr[i]=(u8)(rnd8()%11);
    int m=hhAdd(lk,st,st<AG_ADULT?AS_GROW:rnd8()%AS_PICK,rnd8()&1,tr); if(m<0) return;
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave();
    static char t[32]; char*e=simCat(t,hhM[m].name); simCat(e," MOVED IN"); toast(t);
}
static void hhLoad(void);
static void hhStart(void){   // entering the life game: load the household and stand everyone somewhere free
    hhLoad(); hhSlotsFree(); hhFindExits(); for(int k=0;k<TW_N;k++){ twOn[k]=0; twWait[k]=(short)(240+k*700); }
    for(int m=0;m<hhN;m++){ hhPlace(&hhM[m],m); hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
}
// ---- switching who you control ----
static void hhSwap(HhSim*s);   // main.c: trades the player's position, needs, look and persona with s
static void hhSwitchFrom(int f);
static void hhSwitch(void){   // SELECT: control the next Sim of the household who is at home
    if(!hhN) return;
    int f=0; while(f<hhN&&hhM[f].act==HA_AWAY) f++;
    if(f>=hhN){ lnote="EVERYONE IS OUT"; lnoteT=90; return; }
    hhSwitchFrom(f);
}
static void hhSwitchTo(int m){   // pause menu > HOUSEHOLD > SWITCH TO A SIM: control member m (the ones before it go to the back of the line, as SELECT does)
    if(m<0||m>=hhN) return;
    if(hhM[m].act==HA_AWAY){ toast("THEY ARE OUT RIGHT NOW"); return; }
    if(custom){ toast("HAND BUILT SIMS CANNOT SWITCH"); return; }
    hhSwitchFrom(m);
}
static void hhSwitchFrom(int f){   // the first f members go to the back of the line, then you trade places with the one in front
    static u8 ob[4][OBJ_B] EWRAM_BSS, obs[4][STR_BN] EWRAM_BSS;   // one buffer pair for the whole switch (the rotation below and the player's sprite after it never overlap)
    while(f-->0){   // (with their sprites)
        HhSim t=hhM[0]; u16 p1[16];
        for(int v=0;v<4;v++){ for(int i=0;i<OBJ_B;i++) ob[v][i]=hhObj[0][v][i]; for(int i=0;i<STR_BN;i++) obs[v][i]=hhObjS[0][v][i]; } for(int i=0;i<16;i++) p1[i]=hhPal[0][i];
        for(int m=0;m<hhN-1;m++){ hhM[m]=hhM[m+1]; for(int v=0;v<4;v++){ for(int i=0;i<OBJ_B;i++) hhObj[m][v][i]=hhObj[m+1][v][i]; for(int i=0;i<STR_BN;i++) hhObjS[m][v][i]=hhObjS[m+1][v][i]; } for(int i=0;i<16;i++) hhPal[m][i]=hhPal[m+1][i]; }
        hhM[hhN-1]=t; for(int v=0;v<4;v++){ for(int i=0;i<OBJ_B;i++) hhObj[hhN-1][v][i]=ob[v][i]; for(int i=0;i<STR_BN;i++) hhObjS[hhN-1][v][i]=obs[v][i]; } for(int i=0;i<16;i++) hhPal[hhN-1][i]=p1[i];
    }
    HhSim t=hhM[0]; for(int m=0;m<hhN-1;m++) hhM[m]=hhM[m+1];   // the player goes to the back of the line, the first member steps in
    hhSwap(&t); hhM[hhN-1]=t;
    u16 pl[16];
    hhQuant(spr4,ob,pl);                   // the one you leave: down to a hardware sprite
    hhQuantS(spr4s,obs,pl);
    hhUnquant(hhObj[0],hhPal[0],spr4);     // the member you take over: back to a full 16-bit sprite
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) spr4s[v][i]=spr4[v][i];
    hhUnquantS(hhObjS[0],hhPal[0],spr4s);
    for(int m=0;m<hhN-1;m++){ for(int v=0;v<4;v++){ for(int i=0;i<OBJ_B;i++) hhObj[m][v][i]=hhObj[m+1][v][i]; for(int i=0;i<STR_BN;i++) hhObjS[m][v][i]=hhObjS[m+1][v][i]; } for(int i=0;i<16;i++) hhPal[m][i]=hhPal[m+1][i]; }
    for(int v=0;v<4;v++){ for(int i=0;i<OBJ_B;i++) hhObj[hhN-1][v][i]=ob[v][i]; for(int i=0;i<STR_BN;i++) hhObjS[hhN-1][v][i]=obs[v][i]; } for(int i=0;i<16;i++) hhPal[hhN-1][i]=pl[i];
    spBounds();
    for(int m=0;m<HH_MAX;m++) hhKey[m]=0; sprKey=0;   // sprites moved around and the player's came from a hardware sprite: bake them again next time
    hhSlotsFree();
}

// ---- saving (SRAM at HH_OFF): 'H' '2' count, your uid, then per member its look, stage, persona, name, needs and uid, then the
// relationships (daily, lifetime, flags for every pair of uids); checksum last ----
#define HH_REC (LKPK+3+TR_N+2*HH_NM+HN_N+1)   // ('H6' and older held a 10-byte first name and no last name)
#define HH_RELB (3*HU_N*HU_N)
static void hhSave(void){
    volatile u8*m=SRAM_BASE+HH_OFF; int k=3; u8 sum=0x48;
    m[0]='H'; m[1]='>'; m[2]=(u8)hhN; m[k++]=(u8)hhPUid;   // 'H:' = 'H9' for 8 Sims (8 uids); 'H;' adds the format 11 sliders
    for(int j=0;j<HH_NM;j++) m[k++]=(u8)hhPName[j]; for(int j=0;j<HH_NM;j++) m[k++]=(u8)hhPLast[j];   // your own name
    for(int i=0;i<hhN;i++){ const HhSim*s=&hhM[i];
        for(int j=0;j<LK_N;j++) if(!lkSlide(j)) m[k++]=s->look[j];   // 'H9': the picks as bytes, then the sliders (9 values) two to a byte
        { int h=-1; for(int j=0;j<LK_N;j++) if(lkSlide(j)){ int v=s->look[j]&15; if(h<0) h=v; else { m[k++]=(u8)(h|(v<<4)); h=-1; } } if(h>=0) m[k++]=(u8)h; }
        m[k++]=s->stage; m[k++]=s->asp; m[k++]=s->ltw;
        for(int j=0;j<TR_N;j++) m[k++]=s->tr[j]; for(int j=0;j<HH_NM;j++) m[k++]=(u8)s->name[j]; for(int j=0;j<HH_NM;j++) m[k++]=(u8)s->last[j]; for(int j=0;j<HN_N;j++) m[k++]=s->need[j]; m[k++]=s->uid; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ m[k++]=(u8)relD[a][b]; m[k++]=(u8)relL[a][b]; m[k++]=relF[a][b]; }
    for(int i=2;i<k;i++) sum+=m[i];
    m[k]=sum;
}
static int hhUidsOf(int ver){ return ver<'6'?HH_MAXOLD+1:ver<=':'-1?HH_MAX9+1:HU_N; }   // uids a saved household kept relationships for
static void hhLoad(void){
    volatile u8*m=SRAM_BASE+HH_OFF; u8 sum=0x48; hhN=0;
    if(m[0]!='H'||m[1]<'2'||m[1]>'>'||m[2]>(m[1]>=':'?HH_MAX:HH_MAX9)) return;
    int hu=hhUidsOf(m[1]);   // before 'H6': 10 uids; 'H6'..'H9': 14; 'H:': 8
    int v7=m[1]>='7', nb=v7?2*HH_NM:10, nl=m[1]>='>'?LKPK:m[1]>='='?LKPK13:m[1]>='<'?LKPK12:m[1]>=';'?LKPK11:m[1]>='9'?LKPK10:m[1]>='8'?LK_N9:v7?LK_N8:m[1]>='5'?LK_N7:m[1]=='4'?LK_N6:m[1]=='3'?LK_N5:LK_N4, rec=HH_REC-LKPK+nl-2*HH_NM+nb;   // 'H2' households were saved before the hats and clothes, 'H3' before the face details and sliders
    int n=m[2], hb=v7?2*HH_NM:0, k=4+hb+n*rec+3*hu*hu; for(int i=2;i<k;i++) sum+=m[i]; if(m[k]!=sum) return;
    if(m[3]>=hu) return;
    u8 nu[HH_MAX9+1]; for(int u=0;u<=HH_MAX9;u++) nu[u]=255;   // an older, bigger household: the uids are given out again (you 0, the members 1..), the first HH_MAX members stay
    k=4; int pu=m[3]; nu[pu]=0; hhPUid=0;
    if(v7){ for(int j=0;j<HH_NM;j++) hhPName[j]=(char)m[k++]; for(int j=0;j<HH_NM;j++) hhPLast[j]=(char)m[k++]; hhPName[HH_NM-1]=hhPLast[HH_NM-1]=0; if(!hhPName[0]){ hhPName[0]='Y'; hhPName[1]='O'; hhPName[2]='U'; hhPName[3]=0; } }
    int kept=0;
    for(int i=0;i<n;i++){ HhSim tmp, *s=i<HH_MAX?&hhM[i]:&tmp;
        if(m[1]>='9'){ int lim=m[1]>='>'?LK_N:m[1]>='='?LK_N13:m[1]>='<'?LK_N12:m[1]>=';'?LK_N11:LK_N10; for(int j=0;j<LK_N;j++) s->look[j]=0; for(int j=0;j<lim;j++) if(!lkSlide(j)) s->look[j]=m[k++];   // 'H:' and older: no format 11 looks
          int h=-1; for(int j=0;j<lim;j++) if(lkSlide(j)){ if(h<0){ int b=m[k++]; s->look[j]=(u8)(b&15); h=b>>4; } else { s->look[j]=(u8)h; h=-1; } } }
        else for(int j=0;j<LK_N;j++) s->look[j]=j<nl?m[k++]:0;
        s->stage=m[k++]; s->asp=m[k++]; s->ltw=m[k++];
        for(int j=0;j<TR_N;j++) s->tr[j]=m[k++]; if(v7){ for(int j=0;j<HH_NM;j++) s->name[j]=(char)m[k++]; for(int j=0;j<HH_NM;j++) s->last[j]=(char)m[k++]; } else { for(int j=0;j<10;j++) s->name[j]=(char)m[k++]; s->last[0]=0; s->name[9]=0; } s->name[HH_NM-1]=s->last[HH_NM-1]=0;
        for(int j=0;j<HN_N;j++){ s->need[j]=m[k++]; if(s->need[j]>100) s->need[j]=70; } s->uid=m[k++];
        if(s->stage>=AG_N||s->asp>=AS_N||s->uid>=hu||s->uid==pu) return;   // (AS_GROW, past AS_PICK, is the aspiration of the young)
        if(i>=HH_MAX) continue;   // (a member past the 8 Sims a household holds now: moved out)
        nu[s->uid]=(u8)(++kept); s->uid=(u8)kept;
        s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0; s->bubT=0; s->hp=HP_MAX; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; }
    for(int a=0;a<hu;a++)for(int b=0;b<hu;b++){ signed char d=(signed char)m[k++], l=(signed char)m[k++]; u8 f=m[k++];
        if(nu[a]<HU_N&&nu[b]<HU_N){ relD[nu[a]][nu[b]]=d; relL[nu[a]][nu[b]]=l; relF[nu[a]][nu[b]]=f; } }
    hhN=n<HH_MAX?n:HH_MAX;
}
// How many bytes the household block at m takes (its header, count, uids and checksum all check out), or 0 if it is not a good household
// or does not fit in avail bytes. The household slots (slots.h) use it to copy a household in and out of SRAM without touching hhM.
static int hhBlockLen(volatile u8*m,int avail){
    if(avail<4||m[0]!='H'||m[1]<'2'||m[1]>'>'||m[2]>(m[1]>=':'?HH_MAX:HH_MAX9)) return 0;
    int hu=hhUidsOf(m[1]);
    int v7=m[1]>='7', nb=v7?2*HH_NM:10, nl=m[1]>='>'?LKPK:m[1]>='='?LKPK13:m[1]>='<'?LKPK12:m[1]>=';'?LKPK11:m[1]>='9'?LKPK10:m[1]>='8'?LK_N9:v7?LK_N8:m[1]>='5'?LK_N7:m[1]=='4'?LK_N6:m[1]=='3'?LK_N5:LK_N4, rec=HH_REC-LKPK+nl-2*HH_NM+nb;
    int n=m[2], hb=v7?2*HH_NM:0, k=4+hb+n*rec+3*hu*hu; if(k+1>avail) return 0;
    u8 sum=0x48; for(int i=2;i<k;i++) sum+=m[i]; if(m[k]!=sum||m[3]>=hu) return 0;
    return k+1;
}
_Static_assert(HH_OFF+4+2*HH_NM+HH_MAX*HH_REC+HH_RELB+1<=SLOT_BASE,"the household must fit before the room slots");
_Static_assert(4+2*HH_NM+HH_MAX*HH_REC+HH_RELB+1<=SL_HH_LEN,"the household is bigger than the SRAM block reserved for it");

static int hhMoveIn(const HhFam*F){   // a pre-made family moves in (HOUSEHOLD, or a NEW GAME). Returns how many fit
    int add=0, first=hhN;
    for(int i=0;i<F->n&&hhN<HH_MAX;i++){ twDrop(hhN); hhNew(&hhM[hhN],&F->m[i]);
        { const char*f=F->fam; if(f[0]=='T'&&f[1]=='H'&&f[2]=='E'&&f[3]==' ') f+=4; int k=0; while(f[k]&&k<HH_NM-1){ hhM[hhN].last[k]=f[k]; k++; }   // THE MIDNIGHTS -> MIDNIGHT
          if(k>1&&hhM[hhN].last[k-1]=='S') k--; hhM[hhN].last[k]=0; }
        hhN++; add++; }
    for(int i=first;i<hhN;i++)for(int j=first;j<hhN;j++) if(i!=j){   // a family already knows and likes each other; couples (the first two adults) are in love
        int a=hhM[i].uid, b=hhM[j].uid; relD[a][b]=40; relL[a][b]=50; relF[a][b]=0;
        if(i<first+2&&j<first+2&&hhM[i].stage>=AG_ADULT&&hhM[j].stage>=AG_ADULT){ relD[a][b]=70; relL[a][b]=80; relF[a][b]=RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF; } }
    return add;
}
// ---- the pause menu's HOUSEHOLD screen ----
static void hhInviteTrue(void); static int hhMoveOut(int m);   // households.h
static void hhSwitchMenu(void){   // pick the Sim you control
    if(!hhN){ toast("NO ONE ELSE LIVES HERE"); return; }
    static char nm[HH_MAX][HH_NM+8] EWRAM_BSS; const char* who[HH_MAX];
    for(int m=0;m<hhN;m++){ char*e=simCat(nm[m],hhM[m].name); if(hhM[m].act==HA_AWAY) simCat(e,"  OUT"); who[m]=nm[m]; }
    int m=menu("WHO DO YOU PLAY",who,hhN); if(m<0) return;
    hhSwitchTo(m); lnote=hhPName; lnoteT=60;
}
static void hhMenu(void){
    if(!dbgOn){   // moving in / out and every other change to the household is the DEBUG CODE's (title screen, see main.c): without it, RELATIONSHIPS and who you control
        static const char* const it2[2]={"RELATIONSHIPS","SWITCH TO A SIM"}; int c=menu("HOUSEHOLD",it2,2);
        if(c==0) relScreen(); else if(c==1) hhSwitchMenu();
        return; }
    static const char* const it[7]={"RELATIONSHIPS","MOVE IN A FAMILY","INVITE A NEW SIM","TRULY RANDOM SIM","MOVE SOMEONE OUT","MOVE EVERYONE OUT","SWITCH TO A SIM"};
    char t[24]; { char*e=t; const char*p="HOUSEHOLD  "; while(*p) *e++=*p++; e+=numStr(e,hhN+1); p=" OF "; while(*p) *e++=*p++; e+=numStr(e,HH_MAX+1); *e=0; }
    int c=menu(t,it,7); if(c<0) return;
    if(c==6){ hhSwitchMenu(); return; }
    if(c==0){ relScreen(); return; }
    if(c==2){ hhInvite(); return; }
    if(c==3){ hhInviteTrue(); return; }
    if(c==4){ if(!hhN){ toast("NO ONE ELSE LIVES HERE"); return; }
        const char* who[HH_MAX]; for(int m=0;m<hhN;m++) who[m]=hhM[m].name;
        int m=menu("WHO MOVES OUT?",who,hhN); if(m<0) return;
        static const char* const yn[2]={"YES  GOODBYE","NO"}; if(menu("ARE YOU SURE?",yn,2)!=0) return;
        hhMoveOut(m); return; }   // (to a free lot of the town if there is one: they live there and come to visit)
    if(c==5){ hhN=0; for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; } hhSave(); toast("ONLY YOU LIVE HERE NOW"); return; }
    { const char* fm[HH_NFAM]; for(int f=0;f<HH_NFAM;f++) fm[f]=hhFams[f].fam;   // MOVE IN A FAMILY: the list of families
      c=menu("MOVE IN A FAMILY",fm,HH_NFAM); if(c<0) return; }
    if(!xo[XO_SIMPRE]){ toast("PRE-MADE SIMS ARE OFF"); return; }
    const HhFam*F=&hhFams[c]; int add=hhMoveIn(F);
    if(!add){ toast("THE HOUSE IS FULL"); return; }
    for(int m=0;m<hhN;m++){ hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave(); toast(add<F->n?"SOME DID NOT FIT":"WELCOME HOME");
}
