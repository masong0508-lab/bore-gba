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
enum { HA_IDLE, HA_WALK, HA_USE, HA_WANDER, HA_SEEK, HA_SOC, HA_LEAVE, HA_AWAY, HA_STAIR };   // SEEK: walking to someone to talk to; SOC: in a conversation; LEAVE: off to work or school; AWAY: off the lot; STAIR: walking to the stairs to go up (hhUp)
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
    u8 ilast;                // item module 7: the item row + 1 of its last use (it tries something else next time)
    u8 item;                 // item use (item module 2): the iuT row + 1 this Sim is walking to or using (0 = none). Reset at every decision (hhDecide)
    short think, t;          // steps to the next decision, steps left in the action
    u8 path[HH_PATH];        // directions: 0 +x, 1 +y, 2 -x, 3 -y
} HhSim;
static HhSim hhM[HH_MAX] EWRAM_BSS; static int hhN;
static u16 hhUp[HH_MAX] EWRAM_BSS;   // steps a Sim still spends UPSTAIRS (act is HA_AWAY meanwhile, so nothing draws or picks it); 0 = not upstairs
static u8 hhFl[HH_MAX] EWRAM_BSS; static u8 hhCen[FLR_N] EWRAM_BSS; static u16 hhCenT;   // floors step 2: the floor each Sim is on (0 = ground), and per floor which need furniture it has
static u8 hhFlLd[HH_MAX] EWRAM_BSS;   // floors step 7: the floor each member was saved on (hhLoad), put back by hhStart
// ---- ITEM USE (item module 2): what a household Sim can do at the home pack and sound pack furniture, and what each use COSTS and NEEDS ----
// One row per item, all in ROM. need = the need it refills (the Sim gains 1 per 2 steps, up to t steps: a short use ends early when the need is full);
// dn[] = what the use does to the OTHER needs when it ends (items depend on each other: a run makes a Sim dirty and hungry, coffee fills the bladder);
// gate[] = the lowest value each need may have for a Sim to choose it (too hungry to run, too tired to read); seat = needs a sofa or beanbag on the floor.
enum { IU_TV, IU_SHELF, IU_AQUA, IU_TREAD, IU_STEREO, IU_COFFEE, IU_PHONE, IU_N };
typedef struct { char ch; u8 need, t, seat; signed char dn[HN_N]; u8 gate[HN_N]; const char*say; } ItemUse;
//                                    FOOD WC REST CLEAN COMFY FUN SOC            FOOD WC REST CLEAN COMFY FUN SOC
static const ItemUse iuT[IU_N]={
    {'v',HN_FUN, 200,1,{  0, 0,-5, 0, 8, 0, 0},{  0, 0,15, 0, 0, 0, 0}," IS WATCHING TV"},
    {'b',HN_FUN, 220,0,{  0, 0,-5, 0, 0, 0, 0},{  0, 0,25, 0, 0, 0, 0}," IS READING"},
    {'q',HN_FUN, 120,0,{  0, 0, 0, 0, 5, 0, 0},{  0, 0, 0, 0, 0, 0, 0}," IS FEEDING THE FISH"},
    {'m',HN_FUN, 160,0,{-12,-6,-10,-25, 0, 0, 0},{ 35, 0,35, 0, 0, 0, 0}," IS WORKING OUT"},
    {'A',HN_FUN, 180,0,{  0, 0, 0, 0, 4, 0, 0},{  0, 0, 0, 0, 0, 0, 0}," IS LISTENING TO MUSIC"},
    {'c',HN_REST, 60,0,{  0,-12, 0, 0, 0, 0, 0},{  0,20, 0, 0, 0, 0, 0}," IS HAVING COFFEE"},
    {'I',HN_SOC, 150,0,{  0, 0, 0, 0, 0, 0, 0},{  0, 0, 0, 0, 0, 0, 0}," IS ON THE PHONE"},
};
static u8 hhCenI[FLR_N] EWRAM_BSS;   // per floor: which iuT items it has (bit = row), kept with hhCen by hhCensus
static const u8 iuIc[IU_N]={IC_SOFA,IC_BOOK,IC_PUDDLE,IC_AIR,IC_STAR,IC_GLASS,IC_TALK};   // item module 8: the balloon over a Sim using it (TV, shelf, aquarium, treadmill, stereo, coffee, phone)
static const char* const iuTag[IU_N]={"  TV","  READ","  FISH","  RUN","  MUSIC","  COFFEE","  PHONE"};   // and the tag after its name in the household menu (short: the name buffer is small)
static inline int iuRow(char c){ for(int r=0;r<IU_N;r++) if(iuT[r].ch==c) return r; return -1; }
static inline int iuHasCallee(const HhSim*s){   // item module 5: is there someone to phone: a member parked on another floor (or upstairs)?
    for(int m=0;m<hhN;m++){ const HhSim*o=&hhM[m]; if(o!=s&&o->act==HA_AWAY&&hhUp[m]) return 1; }
    return 0;
}
static inline int iuBusy(const HhSim*s,int r){   // item module 7: a piece of furniture holds one Sim: are all of this kind on this floor taken?
    int u=0; for(int m=0;m<hhN;m++){ const HhSim*o=&hhM[m]; if(o!=s&&o->item==r+1&&(o->act==HA_USE||o->act==HA_WALK)&&(hhFl[m]==curFl||!xo[XO_MULTIFL])) u++; }
    if(!u) return 0;
    int n=0; for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]==iuT[r].ch) n++;
    return u>=n;
}
static inline int iuUsable(const HhSim*s,int r,int f){   // would this Sim choose item r on floor f right now? (it is there, the gates hold, the seat is there)
    const ItemUse*u=&iuT[r]; if(!(hhCenI[f]&(1<<r))) return 0;
    if(u->seat&&!(hhCen[f]&(1<<HN_COMFY))) return 0;
    if(r==IU_COFFEE&&simIsNight()) return 0;   // (item module 4) coffee is for the day
    if(r==IU_PHONE&&!iuHasCallee(s)) return 0;   // (item module 5) nobody to call
    if(f==curFl&&iuBusy(s,r)) return 0;   // (item module 7) someone has it
    for(int n=0;n<HN_N;n++) if(s->need[n]<u->gate[n]) return 0;
    return 1;
}
static int hmFishHungry(void); static void hmFishFed(void);   // skills.h (item module 13): the player's fish-fed flag
static inline int iuWeight(const HhSim*s,int r){   // item module 9: personality picks the item: active Sims run, quiet ones read, playful ones turn the music up, outgoing ones phone
    int a=s->tr[TR_ACT], o=s->tr[TR_OUT], p=s->tr[TR_PLAY], n=s->tr[TR_NICE], t=s->tr[TR_NEAT]; if(a>10) a=10; if(o>10) o=10; if(p>10) p=10; if(n>10) n=10; if(t>10) t=10;
    switch(r){ case IU_TV: return 4+(10-a)/2; case IU_SHELF: return 3+(10-o)/2+t/3; case IU_AQUA: return 3+n/2+(hmFishHungry()?12:0); case IU_TREAD: return 1+a;
               case IU_STEREO: return 2+(p+o)/2; case IU_COFFEE: return 4+a/2; default: return 2+o; }
}
static inline int iuPick(const HhSim*s,int need,int f){   // a usable item for this need on floor f: the row (one of them at random), or -1
    int c[IU_N], k=0; for(int r=0;r<IU_N;r++) if(iuT[r].need==need&&iuUsable(s,r,f)) c[k++]=r;
    if(k>1) for(int i=0;i<k;i++) if(c[i]+1==s->ilast){ c[i]=c[--k]; break; }   // (item module 7) not the same thing twice in a row
    if(!k) return -1;
    int w[IU_N], sum=0; for(int i=0;i<k;i++){ w[i]=iuWeight(s,c[i]); sum+=w[i]; }   // (item module 9) weighted by personality
    int x=(rnd8()*sum)>>8; for(int i=0;i<k;i++){ if(x<w[i]) return c[i]; x-=w[i]; }
    return c[k-1];
}
// ---- relationships (Sims 2 style): for every pair a DAILY and a LIFETIME score, -100..100, kept by uid and one-way (how a feels about b) ----
#define HU_N (HH_MAX+1)
#define TW_N 2                                   // visitors at a time (the neighbours who drop by: see VISITORS below)
// GUESTS: while a neighbour is on the lot, visitor k has a uid of its own, GU0+k, so every social, balloon and relationship rule works on them
// as on the household. Their rows of the tables come from TOWN RELATIONSHIPS (townrel.h) when they arrive and go back there (nrSync).
#define GU0  HU_N
#define RU_N (HU_N+TW_N)   // the tables are this big; the household block saves the first HU_N (guests are kept in the town table)
static signed char relD[RU_N][RU_N] EWRAM_BSS, relL[RU_N][RU_N] EWRAM_BSS; static u8 relF[RU_N][RU_N] EWRAM_BSS;
enum { RF_CRUSH=1, RF_LOVE=2, RF_STEADY=4, RF_KISSED=8, RF_FRIEND=16, RF_BFF=32, RF_ENEMY=64 };   // FRIEND/BFF/ENEMY: remembered so they fire once
// ---- KIN (CREATE-A-HOUSEHOLD, hhcreate.h): what each Sim IS to each other Sim. kin[a][b] = what a is TO b (a is b's MOTHER). Kept by uid like the
// relationships, saved right after them in the household block ('H?'), so room slots and the household bank carry it along. Roles from KN_MOTHER up
// are FAMILY (no romance between them, see socAllowed); NONE, ROOMMATE, PARTNER, WIFE and HUSBAND are not. ----
enum { KN_NONE, KN_ROOMMATE, KN_PARTNER, KN_WIFE, KN_HUSBAND, KN_MOTHER, KN_FATHER, KN_PARENT, KN_DAUGHTER, KN_SON, KN_CHILD, KN_SISTER, KN_BROTHER, KN_SIBLING,
       KN_GRANDMA, KN_GRANDPA, KN_GRANDDAU, KN_GRANDSON, KN_AUNT, KN_UNCLE, KN_NIECE, KN_NEPHEW, KN_COUSIN, KN_STEPMOM, KN_STEPDAD, KN_STEPDAU, KN_STEPSON,
       KN_SPOUSE,   // (appended with GENDER: a nonbinary Sim's husband or wife is their SPOUSE)
       KN_N };
static u8 kin[RU_N][RU_N] EWRAM_BSS;
static void kinClear(void){ for(int a=0;a<RU_N;a++)for(int b=0;b<RU_N;b++) kin[a][b]=0; }
static const char* const kinNm[]={"NONE","ROOMMATE","PARTNER","WIFE","HUSBAND","MOTHER","FATHER","PARENT","DAUGHTER","SON","CHILD","SISTER","BROTHER","SIBLING",
    "GRANDMOTHER","GRANDFATHER","GRANDDAUGHTER","GRANDSON","AUNT","UNCLE","NIECE","NEPHEW","COUSIN","STEPMOTHER","STEPFATHER","STEPDAUGHTER","STEPSON","SPOUSE"};
_Static_assert(sizeof(kinNm)/sizeof(kinNm[0])==KN_N,"kinNm needs a name for every KN_ role");
static const u8 kinInv[KN_N][3]={   // if a is the role, what b may be (the first choice when there is only one)
    {0,0,0}, {KN_ROOMMATE,0,0}, {KN_PARTNER,0,0},
    {KN_WIFE,KN_HUSBAND,KN_SPOUSE}, {KN_WIFE,KN_HUSBAND,KN_SPOUSE},
    {KN_DAUGHTER,KN_SON,KN_CHILD}, {KN_DAUGHTER,KN_SON,KN_CHILD}, {KN_DAUGHTER,KN_SON,KN_CHILD},      // MOTHER FATHER PARENT
    {KN_MOTHER,KN_FATHER,KN_PARENT}, {KN_MOTHER,KN_FATHER,KN_PARENT}, {KN_MOTHER,KN_FATHER,KN_PARENT}, // DAUGHTER SON CHILD
    {KN_SISTER,KN_BROTHER,KN_SIBLING}, {KN_SISTER,KN_BROTHER,KN_SIBLING}, {KN_SISTER,KN_BROTHER,KN_SIBLING},
    {KN_GRANDDAU,KN_GRANDSON,0}, {KN_GRANDDAU,KN_GRANDSON,0}, {KN_GRANDMA,KN_GRANDPA,0}, {KN_GRANDMA,KN_GRANDPA,0},
    {KN_NIECE,KN_NEPHEW,0}, {KN_NIECE,KN_NEPHEW,0}, {KN_AUNT,KN_UNCLE,0}, {KN_AUNT,KN_UNCLE,0}, {KN_COUSIN,0,0},
    {KN_STEPDAU,KN_STEPSON,0}, {KN_STEPDAU,KN_STEPSON,0}, {KN_STEPMOM,KN_STEPDAD,0}, {KN_STEPMOM,KN_STEPDAD,0}, {KN_WIFE,KN_HUSBAND,KN_SPOUSE} };
static inline int kinRom(int r){ return r==KN_PARTNER||r==KN_WIFE||r==KN_HUSBAND||r==KN_SPOUSE; }
static inline int kinWed(int r){ return r==KN_WIFE||r==KN_HUSBAND||r==KN_SPOUSE; }   // married (PARTNER: a couple, not married)
static int kinSex(int r){   // the gender a kin role says (-1: the role does not say)
    switch(r){ case KN_WIFE: case KN_MOTHER: case KN_DAUGHTER: case KN_SISTER: case KN_GRANDMA: case KN_GRANDDAU: case KN_AUNT: case KN_NIECE: case KN_STEPMOM: case KN_STEPDAU: return SX_FEMALE;
               case KN_HUSBAND: case KN_FATHER: case KN_SON: case KN_BROTHER: case KN_GRANDPA: case KN_GRANDSON: case KN_UNCLE: case KN_NEPHEW: case KN_STEPDAD: case KN_STEPSON: return SX_MALE;
               default: return -1; } }
static int kinSexOf(int u){ for(int v=0;v<HU_N;v++){ int x=kinSex(kin[u][v]); if(x>=0) return x; } return -1; }   // what u's own kin roles say about their gender
// FAMILY LIFE (family.h): each member's days in their life stage (by uid), and a baby on the way (days left, 0 = none; its parents' uids, 255 = nobody).
// Saved at the end of an 'H@' household.
static u8 famAge[HU_N] EWRAM_BSS, famDue EWRAM_BSS, famPa EWRAM_BSS, famPb EWRAM_BSS;
static void famReset(void){ for(int u=0;u<HU_N;u++) famAge[u]=0; famDue=0; famPa=famPb=255; }
// TOWN RELATIONSHIPS (townrel.h): how the Sims of this household and the Sims of the town's other households feel about each other. One entry per
// pair: who in the town (the lot they live on, which member of that household, a hash of their first name to catch a lot that changed hands) and
// whose relationship it is (own: a uid of this household), then both ways: own to them (d1 l1 f1), them to own (d2 l2 f2). Saved at the end of an
// 'HA' household (after the family tail), so room slots and the household bank carry it. nrKey: the town the entries belong to.
#define NR_N 32
#define NR_B 10   // bytes an entry takes in the household block
#define NR_LOST 0x80   // f2 bit: they MOVED IN with you (they no longer live in the town: nobody visits as them)
#define NR_MARK 0xFE   // own of a marker entry: that Sim of the town moved in with you (f2 = NR_LOST)
typedef struct { u8 lot, mem, nh, own; signed char d1, l1, d2, l2; u8 f1, f2; } NrE;
static NrE nrT[NR_N] EWRAM_BSS; static u8 nrN EWRAM_BSS; static u16 nrKey EWRAM_BSS;
static u8 twLot[TW_N] EWRAM_BSS, twMem[TW_N] EWRAM_BSS, twNh[TW_N] EWRAM_BSS;   // who visitor k is in the town (twLot 255: nobody we can remember)
static void nrSync(void);   // townrel.h: the guests' rows back into the town table
static void famForget(int u); static void famScreen(void);   // family.h
static void copCrime(int n); static void copTick(int*planned);   // npc.h: the police
static int prHeld(const HhSim*s); static int prHere(void); static void prSave(void); static int prSwitchHook(int m);   // prison.h
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
// SPRITE POOL: a member's 4 views used to be 4 KB (+3.5 KB for the walking frame) each, empty tiles and all. Now only the tiles that hold a pixel are
// kept, in one shared pool: per view a 32-bit mask says which of the 32 tiles (8x8, 32 bytes, in 1D order) exist, and the walking frame keeps only the
// tiles that differ from standing (the legs). A member's block in the pool is [standing views 0..3][walking tiles of views 0..3]. Moving members around
// (switching, moving out) only moves the small descriptors; the bytes stay. Index HH_MAX is a spare descriptor (the player on his way into the pool).
#define SP_POOL 30720
#define STR_T0 4                                 // the walking frame can differ from standing in tile rows 1..7 (tiles 4..31)
typedef struct { u32 sm[4], dm[4]; u16 off, len; } HhSpr;   // sm: standing tiles kept, dm: walking tiles kept; where the block is and how long
static u8 hhPool[SP_POOL] EWRAM_BSS;
static HhSpr hhSp[HH_MAX+1] EWRAM_BSS;
static int hhPoolTop;                            // the blocks are packed: the pool is used up to here
                                                 // (TW_N, the passers-by or townies, is defined with the relationship tables above)
#define TW_V(k) (HH_MAX-1-(k))                    // VISITORS (the old passers-by): visitor k lives in the empty member place TW_V(k) - its HhSim, sprites
                                                 // and palette are that place's - so they take no RAM of their own and only come while the house has room
static u16 hhPal[HH_MAX][16] EWRAM_BSS;                  // a palette per member (index 0 = clear)
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
#define INM_MAX 24   // inmates.h: the prison's population (one instance each; the looks they show are baked into the free member places)
#define HH_IDS    (HH_MAX+INM_MAX)   // sprite ids: 0..HH_MAX-1 the places, then one id per inmate (inmSetOf: the place whose baked sprites it shows)
static int inmSetOf(int id); static const u16* inmPalOf(int j); static int inmOn(int j); static int inmWalk(int j); static int inmNear(void); static void inmCalc(void);
static HhSim inmS[INM_MAX] EWRAM_BSS; static short inX[INM_MAX] EWRAM_BSS, inY[INM_MAX] EWRAM_BSS, inH[INM_MAX] EWRAM_BSS, inB[INM_MAX] EWRAM_BSS; static u8 inV[INM_MAX] EWRAM_BSS, inmSets EWRAM_BSS, inmSetPl[HH_MAX] EWRAM_BSS;
#define UP_BUDGET 5                             // fresh sprite uploads per vblank (a full one is ~700 halfword writes); the rest wait a frame
// CROWD (the prison): a sprite needs three separate things, and hhObjUpdate0 hands them out one by one, nearest Sim first:
//   an OAM entry   0..HH_OAM-1 (32 of them; fx.h starts after them)
//   a TILE BLOCK   1 KB of OBJ VRAM holding one body's view and walking frame (a "key": body*8 + view*2 + frame). Sims of the same body that face the same way and stride the same way
//                  SHARE one block. 8 blocks (OBJ VRAM slots 0..7); in the prison 6 more (slots 10..15; slot 8 is fx.h's, 9 is npc.h's)
//   a PALETTE BANK 16 colours: body 0..6 = its baked palette, 32+j = inmate j with its own skin and hair. Sims with the same palette share a bank. 8 banks (0..7); in the prison the
//                  banks that ghosts (8..10) and cops / skaters (12..15) are not using this very frame are lent out too (hhBanksBusy, npc.h), and handed back (hhExtraDone) when they are needed
// The tile block and the palette bank are chosen on their own: that is what lets one block (and so VRAM) serve several differently coloured inmates. A Sim that finds no block or bank waits a frame.
#define HH_OAM 32                               // OAM entries 0..31: the Sims (depth order: the nearest the camera gets the lowest)
_Static_assert(HH_IDS<=HH_OAM,"every sprite id must have an OAM entry");
#define HH_TB 14                                // tile blocks: 0..7 = OBJ VRAM slots 0..7, 8..13 = slots 10..15 (prison only)
static const u8 hhTvSlot[HH_TB]={0,1,2,3,4,5,6,7,10,11,12,13,14,15};
static signed char hhTcur[HH_TB] EWRAM_BSS;     // what block b holds: body*8+view*2+frame (-1: nothing loaded)
static signed char hhPkey[16] EWRAM_BSS;        // what OBJ palette bank b holds: body 0..6, 32+j an inmate (-1: nothing of ours)
static signed char hhIdBlk[HH_IDS] EWRAM_BSS;   // the block this sprite id drew from last frame (-1: none)
static u16 hhBorrow EWRAM_BSS;                  // palette banks we have overwritten that belong to fx.h / npc.h
static u8 hhSlotOk;
static int hhBanksBusy(void); static void hhExtraDone(void);   // npc.h
static void hhSlotsFree(void){ for(int i=0;i<HH_IDS;i++) hhIdBlk[i]=-1; for(int b=0;b<HH_TB;b++) hhTcur[b]=-1; for(int b=0;b<16;b++) hhPkey[b]=-1; hhSlotOk=1; }   // after anything that changes the baked sprites or who is who
static inline int hhPop(u32 x){ int n=0; while(x){ x&=x-1; n++; } return n; }
static void hhSprFree(int m){   // give a member's block back: everything above it slides down, so the pool stays packed
    HhSpr*s=&hhSp[m]; int off=s->off, len=s->len;
    if(len){ for(int i=off;i<hhPoolTop-len;i++) hhPool[i]=hhPool[i+len];
             for(int k=0;k<=HH_MAX;k++) if(k!=m&&hhSp[k].len&&hhSp[k].off>off) hhSp[k].off=(u16)(hhSp[k].off-len);
             hhPoolTop-=len; }
    for(int v=0;v<4;v++) s->sm[v]=s->dm[v]=0;
    s->off=0; s->len=0;
}
static void hhSprBegin(int m){ hhSprFree(m); hhSp[m].off=(u16)hhPoolTop; }   // a new block grows at the top of the pool
static const u8* hhStTile(int m,int v,int t){   // the standing tile t of view v, or 0 when it is empty
    const HhSpr*s=&hhSp[m]; if(!((s->sm[v]>>t)&1)) return 0;
    int n=0; for(int w=0;w<v;w++) n+=hhPop(s->sm[w]);
    n+=hhPop(s->sm[v]&((1u<<t)-1)); return hhPool+s->off+n*32;
}
// the tiles of view v (standing, or the walking frame when f) go into OBJ VRAM at d, from tile t0 on (STR_T0: only what the walking frame changes)
static void hhUpTiles(volatile u16*d,int id,int v,int f,int t0){
    const HhSpr*s=&hhSp[id]; u32 sm=s->sm[v], dm=f?s->dm[v]:0; int ns=0, nd=0;
    for(int w=0;w<4;w++) ns+=hhPop(s->sm[w]);
    for(int w=0;w<v;w++) nd+=hhPop(s->dm[w]);
    const u8*sp=hhPool+s->off; for(int w=0;w<v;w++) sp+=32*hhPop(s->sm[w]);
    const u8*dp=hhPool+s->off+32*ns+32*nd;
    for(int t=0;t<32;t++){
        int hs=(sm>>t)&1, hd=(dm>>t)&1; const u16*src=0;
        if(hd) src=(const u16*)dp; else if(hs) src=(const u16*)sp;
        if(hs) sp+=32;
        if(hd) dp+=32;
        if(t<t0) continue;
        volatile u16*q=d+t*16;
        if(src){ for(int k=0;k<16;k++) q[k]=src[k]; } else { for(int k=0;k<16;k++) q[k]=0; }
    }
}
static inline const u16* hhPalOf(int id){ return id>=HH_MAX?inmPalOf(id-HH_MAX):hhPal[id]; }   // (an inmate: its body's palette with its own skin and hair colour, inmates.h)

// ---- premade families (original characters) ----
typedef struct { const char* name; u8 look[LK_N]; u8 stage, asp, sign, sex; } HhPre;   // the whole look (the first families only set the base picks); traits come from a sign
typedef struct { const char* fam; u8 n; HhPre m[4]; } HhFam;
//                     look: SHAPE SKIN EYES MOUTH EARS HSTYLE HCOL TOP BOT  TONE EARSZ EARLF
static const HhFam hhFams[]={
    {"THE GRINDERS",3,{ {"REX", {5,2,2,1,1,0,0,1,1, 0,0,0},AG_ADULT,AS_POP,  0,SX_MALE},
                        {"DEE", {4,1,1,1,1,2,3,3,0, 0,0,0},AG_ADULT,AS_FORTUNE,9,SX_FEMALE},
                        {"PIP", {0,1,2,2,2,1,3,2,5, 0,0,0},AG_CHILD,AS_GROW,  2,SX_FEMALE} }},
    {"THE MIDNIGHTS",3,{ {"MORTIMER",{6,0,0,0,0,2,0,7,1, 4,0,0},AG_ADULT,AS_KNOW, 5,SX_MALE},
                        {"VESPER", {4,0,0,1,1,2,7,5,1, 4,0,0},AG_ADULT,AS_HOME, 7,SX_FEMALE},
                        {"WREN",   {0,0,0,0,1,1,0,7,7, 4,0,0},AG_TEEN, AS_PLEAS,10,SX_NB} }},
    {"THE FRESHLYS",2,{ {"BEN",   {0,3,1,1,1,0,1,4,3, 0,0,0},AG_ADULT,AS_FORTUNE,1,SX_MALE},
                        {"BEA",   {4,3,2,1,1,2,2,6,0, 0,0,0},AG_ADULT,AS_PLEAS, 6,SX_FEMALE} }},
    {"THE NOVAS",2,{   {"JUNO",  {5,4,5,3,1,5,6,2,3, 0,0,0},AG_ADULT,AS_POP,    3,SX_FEMALE},
                        {"KIT",   {0,2,3,1,2,4,1,5,2, 0,0,0},AG_TEEN, AS_GROW,   8,SX_MALE} }},
    // the 32 Sims of the bake test (tools: every slider and part random), now families of their own
    {"THE STACKS",4,{ {"ROXY",{6,4,4,0,1,1,6,4,0,0,2,2,2,3,3,0,0,0,0,0,0,1,0,0,0,2,4,0,2,7,0,1,1,1,1,0,0,0,1,3,4,2,7,1,1,2,3,0,2,2,3,3,1,5,1,1,4,8,6,6,5,3,7,0,5,3,2,3,5,0,5,7,8,0,7,7,1,2,1,0,4,8,1,5,5,0},AG_ELDER,AS_FORTUNE, 0,SX_FEMALE},
                {"DUKE",{5,7,6,6,2,4,1,3,2,2,5,7,0,1,2,0,0,0,0,0,0,0,0,0,3,4,1,7,6,1,0,0,1,3,3,0,0,0,3,5,8,8,8,0,6,3,3,1,3,7,2,3,3,0,6,5,0,0,4,8,0,5,5,4,6,2,5,1,8,0,7,7,8,5,7,5,8,5,0,7,8,2,7,0,6,1},AG_ADULT,AS_KNOW, 1,SX_MALE},
                {"MILO",{3,2,4,7,2,4,0,1,7,0,1,8,0,2,2,0,0,0,3,0,0,0,0,0,0,5,2,3,3,7,7,2,5,2,2,0,0,3,6,2,5,1,1,6,8,5,1,1,1,8,0,6,7,1,5,3,1,5,7,8,6,4,7,7,7,2,7,2,7,0,2,7,8,1,4,2,1,0,5,3,2,4,0,6,7,6},AG_ADULT,AS_POP, 2,SX_MALE},
                {"IVY",{0,5,1,5,1,0,6,0,1,0,5,6,2,0,2,0,0,0,2,0,0,0,0,0,0,7,3,1,7,5,8,5,1,3,1,0,0,7,4,2,8,2,7,8,1,6,3,3,2,3,7,5,6,5,7,2,3,8,3,7,6,6,0,3,5,0,1,2,4,0,5,0,0,7,6,0,2,0,3,5,0,0,2,3,7,5},AG_ADULT,AS_PLEAS, 3,SX_FEMALE} }},
    {"THE PIXELS",4,{ {"ZED",{1,0,6,6,1,7,6,4,4,1,4,1,2,3,1,0,0,0,2,0,0,5,0,0,0,7,5,0,3,4,2,0,8,1,3,0,0,0,7,3,2,3,4,6,4,5,0,3,0,2,6,5,3,0,3,6,1,5,4,1,8,5,5,5,5,6,2,8,5,0,2,0,7,0,6,3,2,2,2,4,3,6,4,1,8,8},AG_ADULT,AS_HOME, 4,SX_MALE},
                {"LULU",{5,0,1,8,1,8,2,1,1,8,7,3,2,3,1,0,0,0,1,0,0,0,0,0,0,3,8,4,4,1,8,6,1,1,1,0,0,6,0,7,2,4,6,5,1,7,1,1,2,8,2,6,8,5,5,2,3,8,7,6,1,7,1,8,5,1,4,0,5,0,8,1,5,8,5,4,1,2,6,2,8,8,1,7,5,8},AG_TEEN,AS_GROW, 5,SX_FEMALE},
                {"BYTE",{4,6,7,8,2,5,2,7,3,8,0,7,3,2,3,4,0,0,2,0,0,5,0,0,0,1,4,2,1,1,1,3,4,3,1,0,0,6,0,8,5,2,3,6,0,6,0,1,3,7,7,2,6,8,8,6,2,4,1,5,3,4,8,7,1,6,2,8,0,0,8,8,0,2,4,3,3,8,5,3,0,8,4,0,7,8},AG_ELDER,AS_KNOW, 6,SX_NB},
                {"PIXIE",{4,0,6,3,2,4,1,6,5,8,5,7,2,3,2,0,0,0,3,0,0,5,0,0,0,6,6,1,5,5,1,7,5,3,1,0,0,2,8,8,1,8,4,0,3,2,0,1,0,5,5,5,2,6,3,1,1,4,4,5,8,5,5,8,5,1,7,5,5,0,0,8,0,4,5,1,0,4,6,3,8,0,5,8,0,3},AG_TEEN,AS_GROW, 7,SX_FEMALE} }},
    {"THE VOXELLS",4,{ {"OTIS",{4,6,7,0,2,7,4,2,0,7,7,2,0,2,3,0,0,0,3,0,0,5,0,0,2,4,7,5,8,5,1,1,4,3,0,0,0,7,5,5,4,7,5,3,0,5,3,0,3,2,4,4,7,0,3,1,5,7,7,1,5,7,6,2,0,0,8,6,3,0,0,8,1,5,4,8,5,2,0,3,4,5,3,2,7,8},AG_ADULT,AS_PLEAS, 8,SX_MALE},
                {"JUNE",{5,2,3,5,1,8,6,4,5,3,7,4,3,0,2,0,0,0,0,0,0,1,0,0,0,0,6,3,7,6,6,1,2,2,1,0,0,2,3,2,3,2,5,7,4,5,1,1,1,6,4,3,3,3,5,8,5,2,6,7,3,6,7,7,2,6,5,3,7,0,5,4,8,1,4,5,1,5,3,4,2,7,1,2,7,0},AG_ADULT,AS_HOME, 9,SX_FEMALE},
                {"CASH",{1,7,2,0,1,3,1,5,1,1,0,0,2,1,2,4,0,0,1,0,0,4,0,0,0,5,7,8,0,6,2,3,7,1,0,0,0,0,8,5,6,4,4,5,2,2,1,3,3,5,3,8,2,1,4,5,5,3,7,5,8,4,6,7,3,8,5,7,8,0,7,2,1,6,7,7,8,4,6,6,3,3,3,2,4,4},AG_ADULT,AS_FORTUNE,10,SX_MALE},
                {"MAYA",{5,0,1,8,1,0,0,1,7,1,8,3,1,1,0,0,0,0,0,0,0,2,0,0,1,2,2,3,0,6,5,0,5,3,0,0,0,4,0,6,6,2,6,5,0,8,2,2,0,2,5,3,1,2,7,6,0,1,4,2,4,0,0,1,1,3,8,0,0,0,6,8,2,8,1,8,6,4,4,1,7,7,1,2,5,1},AG_ELDER,AS_KNOW,11,SX_FEMALE} }},
    {"THE LOWPOLYS",4,{ {"TOBY",{6,5,2,7,1,3,1,1,6,2,5,2,0,0,3,0,0,0,0,0,0,0,0,0,0,6,7,6,0,8,4,0,1,1,1,0,0,2,6,6,7,0,5,6,8,1,3,2,3,6,2,6,3,1,3,7,2,5,6,1,4,6,3,6,1,1,6,0,2,0,2,4,8,1,2,2,6,8,4,1,3,4,3,4,6,7},AG_TEEN,AS_GROW, 0,SX_MALE},
                {"ELLA",{3,7,6,1,1,5,0,5,2,6,0,2,3,3,0,0,0,0,2,0,0,3,0,0,3,4,3,7,1,3,7,8,8,3,2,0,0,4,5,5,3,4,6,0,7,0,3,1,0,7,1,1,8,4,2,2,5,7,1,0,7,4,7,7,7,3,1,3,2,0,0,8,2,7,0,6,8,1,1,5,0,7,6,7,8,5},AG_TEEN,AS_GROW, 1,SX_FEMALE},
                {"FINN",{5,6,2,8,1,3,4,3,3,0,3,4,1,3,0,3,0,0,3,0,0,2,0,0,3,3,6,6,1,6,4,7,3,2,2,0,0,7,6,3,7,5,2,5,3,7,0,0,1,4,2,6,1,7,6,0,0,3,3,2,0,2,5,0,3,3,1,8,6,0,6,1,1,0,3,8,5,8,4,3,3,5,6,0,4,7},AG_ELDER,AS_HOME, 2,SX_MALE},
                {"SAGE",{4,6,1,0,1,7,2,1,6,4,1,2,2,3,2,0,0,0,2,0,0,5,0,0,0,7,4,2,1,2,4,5,2,1,0,0,0,1,5,0,2,2,5,7,3,2,3,3,3,7,2,6,8,7,1,3,6,8,3,0,2,3,3,4,0,0,6,2,4,0,8,8,5,5,1,8,4,6,8,6,1,7,0,1,6,0},AG_ADULT,AS_FORTUNE, 3,SX_NB} }},
    {"THE KICKFLIPS",4,{ {"GUS",{6,1,6,2,1,2,6,1,3,0,1,2,0,3,1,0,0,0,3,0,0,1,0,0,0,0,8,8,2,8,3,7,2,0,2,0,0,2,4,5,7,7,7,6,7,4,0,1,3,6,8,1,0,7,2,6,3,4,6,8,4,3,0,2,6,2,5,8,4,0,6,1,6,8,3,5,6,6,5,4,5,6,1,6,8,0},AG_ADULT,AS_KNOW, 4,SX_MALE},
                {"NELL",{1,3,3,4,1,2,1,4,1,4,7,7,2,0,0,0,0,0,3,0,0,2,0,0,0,3,4,6,5,7,8,3,2,0,3,0,0,1,1,0,5,7,5,5,2,3,1,0,1,2,1,3,1,2,1,8,0,7,6,5,8,2,1,4,4,4,1,8,7,0,8,5,2,1,6,0,4,4,1,3,1,3,3,6,5,4},AG_ADULT,AS_POP, 5,SX_FEMALE},
                {"ACE",{3,5,0,1,1,6,3,3,6,8,3,6,1,0,2,4,0,0,3,0,0,1,0,0,0,3,2,5,3,6,1,6,8,3,3,0,0,6,0,2,6,3,2,4,0,6,0,0,3,6,5,8,1,4,0,1,5,7,1,2,1,4,0,3,8,3,2,0,0,0,6,3,4,5,0,4,8,2,3,3,5,1,2,5,7,7},AG_TEEN,AS_GROW, 6,SX_MALE},
                {"POPPY",{4,5,1,4,2,7,3,5,4,0,4,6,1,2,2,0,0,0,2,0,0,0,0,0,0,0,6,7,4,2,5,1,5,0,3,0,0,8,6,6,0,3,3,8,0,2,0,1,1,1,2,0,4,0,2,4,5,3,0,2,2,1,6,8,1,4,1,6,8,0,8,1,2,3,6,1,4,3,2,0,1,0,6,6,6,0},AG_ADULT,AS_HOME, 7,SX_FEMALE} }},
    {"THE BUFFERS",4,{ {"HANK",{4,7,6,2,1,1,2,3,5,4,8,5,3,0,2,0,0,0,0,0,0,5,0,0,1,4,0,3,8,8,7,4,1,3,1,0,0,8,7,0,4,6,1,7,0,4,3,1,3,0,4,2,3,0,2,3,8,3,4,3,3,4,7,3,7,8,7,2,6,0,0,6,7,5,4,5,4,7,2,0,4,1,2,8,1,5},AG_ADULT,AS_FORTUNE, 8,SX_MALE},
                {"DOT",{6,5,0,7,1,8,4,1,7,2,0,6,1,0,0,2,0,0,0,0,0,2,0,0,0,1,1,8,1,6,7,3,2,1,0,0,0,7,6,1,3,5,7,0,1,0,1,2,0,1,1,2,1,8,7,5,1,5,8,1,7,2,2,7,2,2,5,0,2,0,0,8,2,3,5,0,5,0,6,7,0,0,8,1,4,0},AG_ELDER,AS_KNOW, 9,SX_FEMALE},
                {"RIO",{6,1,7,2,2,5,2,6,4,3,8,0,1,2,0,0,0,0,2,0,0,1,0,0,2,6,0,5,8,1,4,2,2,2,1,0,0,4,2,2,7,0,0,2,2,4,3,3,3,2,2,6,1,3,3,5,2,7,4,2,5,7,1,6,7,0,1,1,1,0,6,1,1,6,1,4,4,1,6,8,1,6,4,8,3,1},AG_ADULT,AS_POP,10,SX_MALE},
                {"SUKI",{4,2,8,2,2,2,6,6,4,7,6,2,2,2,0,0,0,0,3,0,0,5,0,0,0,1,6,2,0,8,4,3,6,2,2,0,0,1,2,7,5,1,5,6,5,1,3,0,3,4,2,7,4,4,0,1,8,6,7,1,4,7,4,2,5,5,7,1,8,0,6,6,5,3,7,6,0,3,6,7,0,2,1,1,3,7},AG_ELDER,AS_PLEAS,11,SX_FEMALE} }},
    {"THE SPRITES",4,{ {"WADE",{3,2,4,7,1,5,2,3,5,6,5,6,1,3,1,0,0,0,2,0,0,4,0,0,0,6,1,2,7,7,4,4,2,1,1,0,0,5,5,4,6,0,6,0,2,8,2,1,3,4,0,5,4,2,7,8,3,6,4,3,4,3,8,2,5,2,1,1,4,0,5,0,8,0,1,4,3,3,5,0,1,7,6,0,2,6},AG_ADULT,AS_HOME, 0,SX_MALE},
                {"IRIS",{4,1,3,6,2,5,3,5,1,5,7,0,3,1,2,0,0,0,0,0,0,4,0,0,0,2,6,7,4,1,1,4,3,0,2,0,0,5,6,2,3,3,3,8,3,0,3,3,1,1,2,8,8,4,7,0,8,6,6,6,8,5,4,2,4,6,4,6,1,0,3,3,7,8,6,1,1,3,3,1,8,4,4,0,5,0},AG_TEEN,AS_GROW, 1,SX_FEMALE},
                {"KOJI",{0,0,5,7,2,7,2,7,3,5,1,4,2,1,3,0,0,0,1,0,0,3,0,0,3,1,0,8,5,0,4,3,5,1,2,0,0,3,4,1,7,7,6,7,5,5,2,1,1,7,8,2,1,5,3,1,6,8,0,6,3,4,5,0,5,2,3,8,3,0,8,8,1,3,4,8,3,4,8,4,7,6,6,0,7,4},AG_ELDER,AS_KNOW, 2,SX_MALE},
                {"LOLA",{1,3,5,7,2,6,2,0,2,6,4,8,0,2,1,0,0,0,1,0,0,0,0,0,0,5,3,5,5,4,1,1,3,2,3,0,0,7,7,2,7,2,7,2,8,8,1,2,1,2,5,0,4,6,0,3,2,4,6,3,0,8,1,8,8,2,7,6,1,0,6,3,0,2,3,3,5,2,2,1,2,5,0,7,6,7},AG_ADULT,AS_POP, 3,SX_FEMALE} }},
    {"THE DIPPERS",4,{ {"VINCE",{4,3,5,2,1,8,1,7,6,2,7,6,2,0,1,0,0,0,2,0,0,1,0,0,0,3,7,0,5,0,8,8,7,2,0,0,0,5,0,0,7,0,5,4,2,8,0,2,2,3,7,8,1,0,3,4,1,5,8,7,8,7,7,2,5,4,0,0,7,0,6,4,5,8,1,3,2,8,1,8,6,1,3,7,0,1},AG_TEEN,AS_GROW, 4,SX_MALE},
                {"MAE",{5,6,2,1,1,0,2,3,4,7,2,6,2,3,2,3,0,0,3,0,0,4,0,0,0,7,2,0,4,0,0,8,7,3,3,0,0,6,7,5,7,1,0,3,3,1,3,3,2,1,2,8,8,3,4,8,8,4,2,0,4,5,8,8,7,5,8,6,1,0,2,5,7,4,5,0,6,6,0,5,0,1,2,4,3,4},AG_TEEN,AS_GROW, 5,SX_FEMALE},
                {"OZZY",{3,0,7,4,1,8,6,7,7,1,7,4,3,0,2,0,0,0,2,0,0,1,0,0,3,7,4,0,1,8,7,0,8,0,0,0,0,1,5,6,8,7,5,5,0,3,2,0,1,3,8,4,1,1,7,8,0,2,0,6,5,1,6,2,0,3,4,2,2,0,6,0,3,3,5,0,7,4,0,1,1,7,0,1,4,3},AG_TEEN,AS_GROW, 6,SX_MALE},
                {"TESS",{4,6,8,5,2,7,0,1,1,8,2,2,3,3,0,3,0,0,2,0,0,2,0,0,0,7,3,6,2,6,6,0,5,0,1,0,0,6,6,1,7,1,2,0,1,7,1,1,3,6,6,8,2,7,0,3,2,5,2,8,4,7,0,0,3,4,3,7,8,0,7,7,1,3,4,6,5,2,2,4,0,5,5,8,8,6},AG_ELDER,AS_KNOW, 7,SX_FEMALE} }},
};
#define HH_NFAM ((int)(sizeof(hhFams)/sizeof(hhFams[0])))

// ---- 16-bit sprite <-> 15 colours + clear, 4bpp tiles ----
// Colour lookups for the two quantisers: a small open-addressing hash from a 15-bit colour to a slot (key 0xFFFF = empty).
// The sprites hold about 40 colours, so a lookup is one or two probes instead of a walk through the list.
// (the hash helpers hqKey / hqVal / hqClear / hqSlot / hqDist are in main.c, next to the sprite palette they also serve)
// scratch for the quantisers, on the path-finding table (hhDist is always filled again before a search, and nothing searches while a Sim is baked)
typedef struct { u16 col[256], oc[256], cnt[256]; u8 ob[256]; u8 tv[OBJ_B]; } HhQs;   // tv: one view as 32 tiles
_Static_assert(sizeof(HhQs)<=sizeof(hhDist),"HhQs must fit on hhDist");
#define hhQs (*(HhQs*)hhDist)
static void hhPutTiles(int m,u32 mk,const u8*tv,int stride,int v){   // append the tiles in mk (from tv, 32 bytes each) to member m's block; too full: nothing is kept
    int n=hhPop(mk);
    if(hhPoolTop+n*32>SP_POOL){ mk=0; n=0; }
    else { u8*d=hhPool+hhPoolTop; for(int t=0;t<32;t++) if((mk>>t)&1){ const u8*q=tv+t*32; for(int i=0;i<32;i++) *d++=q[i]; }
           hhPoolTop+=n*32; hhSp[m].len=(u16)(hhSp[m].len+n*32); }
    if(stride) hhSp[m].dm[v]=mk; else hhSp[m].sm[v]=mk;
}
static void hhQuant(u8 (*src)[SPW*SPH],int m,u16*pal){   // the 4 standing views of the sprite set into member m's block (m = HH_MAX: the spare)
    u16*col=hhQs.col, *oc=hhQs.oc, *cnt=hhQs.cnt; u8*ob=hhQs.ob; u8*tv=hhQs.tv; int n=0;
    hqClear();
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++){ u16 c=spC(src[v][i]); if(c==SKY) continue; int h=hqSlot(c), k;
        if(hqKey[h]==0xFFFF){ if(n==256) continue; hqKey[h]=c; hqVal[h]=(u8)n; col[n]=c; cnt[n]=0; k=n++; } else k=hqVal[h];
        cnt[k]++; }
    int n0=n; for(int k=0;k<n0;k++) oc[k]=col[k];   // the colours as found (hqVal points into this list)
    while(n>15){   // merge the two closest colours (weighted by how often they appear) until 15 are left
        int ba=0, bb=1, bd=1<<30;
        for(int a=0;a<n;a++)for(int b=a+1;b<n;b++){ int d=hqDist(col[a],col[b])*(int)(cnt[a]<cnt[b]?cnt[a]:cnt[b]); if(d<bd){ bd=d; ba=a; bb=b; } }
        u32 w=(u32)cnt[ba]+cnt[bb]; if(!w) w=1;
        int r=(int)(((col[ba]&31)*(u32)cnt[ba]+(col[bb]&31)*(u32)cnt[bb])/w), g=(int)((((col[ba]>>5)&31)*(u32)cnt[ba]+((col[bb]>>5)&31)*(u32)cnt[bb])/w), bl=(int)((((col[ba]>>10)&31)*(u32)cnt[ba]+((col[bb]>>10)&31)*(u32)cnt[bb])/w);
        if(cnt[bb]>cnt[ba]) col[ba]=col[bb]; else if(cnt[ba]==cnt[bb]) col[ba]=(u16)(r|(g<<5)|(bl<<10));   // keep the commoner one exact (faces stay crisp)
        cnt[ba]=(u16)w; col[bb]=col[n-1]; cnt[bb]=cnt[n-1]; n--; }
    pal[0]=0; for(int k=0;k<15;k++) pal[k+1]=k<n?col[k]:0;
    for(int j=0;j<n0;j++){ u16 c=oc[j]; int best=1, bd=1<<30;   // the nearest palette entry, once per colour found (not once per pixel)
        for(int k=0;k<n;k++){ int d=hqDist(c,col[k]); if(d<bd){ bd=d; best=k+1; if(!d) break; } } ob[j]=(u8)best; }
    hhSprBegin(m);
    for(int v=0;v<4;v++){
        for(int i=0;i<OBJ_B;i++) tv[i]=0;
        for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++){ u16 c=spC(src[v][y*SPW+x]); if(c==SKY) continue;
            int h=hqSlot(c), best;
            if(hqKey[h]==c) best=ob[hqVal[h]];
            else { int bd=1<<30; best=1; for(int k=0;k<n;k++){ int d=hqDist(c,col[k]); if(d<bd){ bd=d; best=k+1; if(!d) break; } } }   // (past 256 colours: not in the table)
            int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1); tv[o]|=(u8)(best<<((x&1)*4)); }
        u32 mk=0; for(int t=0;t<32;t++){ const u8*q=tv+t*32; for(int i=0;i<32;i++) if(q[i]){ mk|=1u<<t; break; } }   // the tiles that hold a pixel
        hhPutTiles(m,mk,tv,0,v);
    }
}
static void hhQuantS(u8 (*src)[SPW*SPH],int m,const u16*pal){   // the walking frame, in the palette the standing frame chose: only the tiles that differ from standing are kept
    u8*tv=hhQs.tv; hqClear(); int used=0;
    for(int v=0;v<4;v++){
        for(int i=0;i<OBJ_B;i++) tv[i]=0;
        for(int y=STR_Y0;y<STR_Y1&&y<SPH;y++)for(int x=0;x<SPW;x++){ u16 c=spC(src[v][y*SPW+x]); if(c==SKY) continue;
            int h=hqSlot(c), best;
            if(hqKey[h]==c) best=hqVal[h];
            else { int bd=1<<30; best=1; for(int k=1;k<16;k++){ int d=hqDist(c,pal[k]); if(d<bd){ bd=d; best=k; if(!d) break; } }
                   if(used<HQ_N*3/4){ hqKey[h]=c; hqVal[h]=(u8)best; used++; } }   // remembered: the next pixel of this colour is one lookup
            int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1); tv[o]|=(u8)(best<<((x&1)*4)); }
        u32 mk=0;   // tile row 0 is as standing; below it, keep a tile only if it is not what standing has there
        for(int t=STR_T0;t<32;t++){ const u8*a=tv+t*32, *b=hhStTile(m,v,t); int df=0; for(int i=0;i<32;i++) if(a[i]!=(b?b[i]:0)){ df=1; break; } if(df) mk|=1u<<t; }
        hhPutTiles(m,mk,tv,1,v);
    }
}
// one view of member m as a full 32-tile picture in out: the standing frame, or (f) the walking frame
static void hhViewImg(int m,int v,int f,u8*out){
    for(int i=0;i<OBJ_B;i++) out[i]=0;
    for(int t=0;t<32;t++){ const u8*q=hhStTile(m,v,t); if(q) for(int i=0;i<32;i++) out[t*32+i]=q[i]; }
    if(f){ const HhSpr*s=&hhSp[m]; int ns=0, nd=0; for(int w=0;w<4;w++) ns+=hhPop(s->sm[w]); for(int w=0;w<v;w++) nd+=hhPop(s->dm[w]);
           const u8*dp=hhPool+s->off+32*ns+32*nd;
           for(int t=0;t<32;t++) if((s->dm[v]>>t)&1){ for(int i=0;i<32;i++) out[t*32+i]=dp[i]; dp+=32; } }
}
static void hhUnquantS(int m,const u16*pal,u8 (*dst)[SPW*SPH]){   // the walking frame back over a copy of the standing frame
    (void)pal; u8*tv=hhQs.tv;
    for(int v=0;v<4;v++){ hhViewImg(m,v,1,tv);
        for(int y=STR_Y0;y<STR_Y1&&y<SPH;y++)for(int x=0;x<SPW;x++){ int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1), k=(tv[o]>>((x&1)*4))&15; dst[v][y*SPW+x]=(u8)k; } }   // (the sprite palette is the member's own: see hhUnquant)
}
static void hhUnquant(int m,const u16*pal,u8 (*dst)[SPW*SPH]){   // back to full sprites (when a member becomes the one you control): the 15 colours become the sprite palette
    sprPal[0]=SKY; for(int k=1;k<16;k++) sprPal[k]=pal[k]; sprN=16; u8*tv=hhQs.tv;
    for(int v=0;v<4;v++){ hhViewImg(m,v,0,tv);
        for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++){ int t=(y>>3)*4+(x>>3), o=t*32+(y&7)*4+((x&7)>>1), k=(tv[o]>>((x&1)*4))&15; dst[v][y*SPW+x]=(u8)k; } }
}
static void spBounds(void){   // the box that holds every opaque pixel of the player's four views (blits and redraw rectangles stay inside it)
    spBx0=SPW; spBx1=0; spBy0=SPH; spBy1=0;
    for(int v=0;v<8;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++) if(sprBuf[v][y*SPW+x]){ if(x<spBx0) spBx0=x; if(x+1>spBx1) spBx1=x+1; if(y<spBy0) spBy0=y; if(y+1>spBy1) spBy1=y+1; }
    if(spBx0>=spBx1){ spBx0=0; spBx1=SPW; spBy0=0; spBy1=SPH; }
}
// ---- baking: render a member's look with the creator's own code, then put the player's creature back ----
static void hhRandLook(u8*lk,u8*stg){   // a made-up Sim: passers-by, and SELECT on the RELATIONSHIPS screen
    static const u8 shp[12]={0,1,3,4,5,6,17,18,19,20,21,22}, sg[4]={AG_ADULT,AG_ADULT,AG_TEEN,AG_ELDER};
    for(int i=0;i<LK_N;i++) lk[i]=0;
    lk[LK_SHAPE]=shp[rnd8()%12]; lk[LK_SKIN]=(u8)(rnd8()%NSKIN); lk[LK_EYES]=(u8)(rnd8()%NEYE); lk[LK_MOUTH]=(u8)(rnd8()%NMOUTH);
    lk[LK_EARS]=(u8)(1+(rnd8()&1)); lk[LK_HSTYLE]=(u8)(rnd8()%NHAIR); lk[LK_HCOL]=(u8)(rnd8()%NSW); lk[LK_TOP]=(u8)(rnd8()%NSW); lk[LK_BOT]=(u8)(rnd8()%NSW);
    lk[LK_TOPSTY]=(u8)(rnd8()&3); lk[LK_HAT]=(rnd8()&3)==0?(u8)(1+rnd8()%5):0; lk[LK_GLASS]=(rnd8()&3)==0?(u8)(1+rnd8()%3):0;
    lk[LK_BROW]=(u8)(rnd8()%6); lk[LK_EYECOL]=(u8)(rnd8()%NSW); lk[LK_SEX]=sexRoll();
    *stg=sg[rnd8()&3];
}
// ---- bake cache: a sprite set is only baked again when something it is drawn from changed ----
// The key is a hash of everything the bake reads: the voxels, the face sprites, the colour tables, the look, the age stage, the hand-built
// flag, the unlock flag and every option. Equal key = an identical picture, so a Sim that did not change is not drawn again (coming back
// from the editor, a slot, the pause menu, growing up ...). Keys follow their sprites when members move (hhRemove); hhSwitch drops them.
static u32 hhKey[HH_MAX];   // per member: the key the pool block and hhPal were baked from (0 = unknown)
static u8 twKeep; static u8 inmN EWRAM_BSS; static void inmPick(void); static void inmDress(int j); static void inmColors(int j);  static void inmTick(int*planned);   // inmates.h: the prison population           // the visitors already picked are kept (new faces when you move to another lot or start a new life)
static u8 twCall[TW_N];     // visitor k was CALLED OVER on the phone: they come at night too
static u8 twWel[TW_N];      // visitor k is the WELCOME visit: the first neighbour of a new home rings the bell soon after you move in (twPick), at night too, and brings a gift
static u8 twHas[TW_N], twOn[TW_N]; static short twWait[TW_N]={240,900}; static char twFrom[TW_N][12];   // visitor k: set up, on the lot (1 coming, 2 staying, 3 going), the lot they live on
static int nbVisitor(HhSim*s,char*from,int not);   // households.h: a Sim of another household of the town (s), the lot it lives on; not = a family to skip
static u8 nbMem;   // households.h: which member of that household it was (nbSimFrom / nbVisitor set it)
static void nrGuestIn(int k); static void nrForget(int u); static u8 nrHash(const char*s); static void nrGuestChat(int k); static int nrPull(int k);   // townrel.h
static void twWho(int k,int lot){ HhSim*s=&hhM[TW_V(k)]; twLot[k]=(u8)(lot<0?255:lot>=100?0x80|(lot-100):lot); twMem[k]=nbMem; twNh[k]=nrHash(s->name); s->uid=(u8)(GU0+k); nrGuestIn(k); }   // visitor k is that Sim of the town: their uid and rows (townrel.h)
static void twPick(void){   // who comes by while you are on this lot: Sims from the town's other households
    int f0=-1; nrSync();   // (the visitors who were here keep what they feel)
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); twHas[k]=0; twOn[k]=0; twWait[k]=(short)(240+k*700);
        if(v<hhN||!xo[XO_SIMPRE]) continue;
        HhSim*s=&hhM[v]; int f=nbVisitor(s,twFrom[k],f0); if(f==-2) continue; f0=f;
        s->bubT=0; s->hp=HP_MAX; s->ltw=0; s->act=HA_IDLE; s->think=0; s->hd=0; s->pn=s->pi=0; for(int q=0;q<HN_N;q++) s->need[q]=80;
        twWho(k,f); twHas[k]=1; }
    for(int k=0;k<TW_N;k++) twWel[k]=0;
    for(int k=0;k<TW_N;k++) if(twHas[k]){ twWel[k]=1; twWait[k]=180; break; }   // SCRIPTED ARRIVAL: the first neighbour walks in about 3 seconds after you move in
}
static void twDrop(int place){ for(int k=0;k<TW_N;k++) if(TW_V(k)==place){ twHas[k]=0; twOn[k]=0; hhKey[place]=0; } }   // a member moves into a visitor's place
static void hkAdd(u32*h,const void*p,int n){ const u8*b=(const u8*)p; u32 x=*h; for(int i=0;i<n;i++) x=(x^b[i])*16777619u; *h=x; }
static u32 bakeKey(void){   // call after buildLook() + setColors()
    u32 h=2166136261u; u8 t[4]={stage,(u8)custom,sUnlock,0};
    hkAdd(&h,vox,sizeof(vox)); hkAdd(&h,dec,sizeof(dec)); hkAdd(&h,sT,sizeof(sT)); hkAdd(&h,sL,sizeof(sL)); hkAdd(&h,sR,sizeof(sR));
    hkAdd(&h,dL,sizeof(dL)); hkAdd(&h,dR,sizeof(dR)); hkAdd(&h,look,sizeof(look)); hkAdd(&h,t,4); hkAdd(&h,xo,sizeof(xo));
    return h?h:1;
}
static void poseBakeAll(void); static void poseWiden(void);
static void hhBakeAll(void){
    static u8 sv[H][D][W] EWRAM_BSS; static u16 sd[H][D][W] EWRAM_BSS; u8 sl[LK_N]; u8 sst=stage; int sc=custom;
    int scratch=0;   // spr4 / spr4s were used to bake someone else: the player has to be baked again
    for(int i=0;i<LK_N;i++) sl[i]=look[i];
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ sv[y][z][x]=vox[y][z][x]; sd[y][z][x]=dec[y][z][x]; }
    for(int m=0;m<hhN;m++){
        for(int i=0;i<LK_N;i++) look[i]=hhM[m].look[i];
        stage=hhM[m].stage;
        if(prHere()&&prHeld(&hhM[m])) continue;   // (in the prison the OUT members' places show inmates: not baked now, they are when you go home)
        buildLook(); setColors(); u32 k=bakeKey(); if(k==hhKey[m]) continue;   // unchanged since the last bake
        ldShow("GETTING THE SIMS READY",m,hhN+TW_N+1);
        bakeInto(spr4); hhQuant(spr4,m,hhPal[m]);
        strideK=1; bakeInto(spr4s); strideK=0; hhQuantS(spr4s,m,hhPal[m]); hhKey[m]=k; scratch=1;
    }
    inmPick();   // inmates.h: who is in the prison (and no neighbours there)
    if(!twKeep) twPick();   // who visits (new faces on another lot or in a new life)
    twKeep=1;
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); if(!twHas[k]||v<hhN) continue;   // the visitors, in the free member places
        for(int i=0;i<LK_N;i++) look[i]=hhM[v].look[i];
        stage=hhM[v].stage;
        buildLook(); setColors(); u32 kk=bakeKey(); if(kk==hhKey[v]) continue;
        ldShow("GETTING THE NEIGHBORS READY",hhN+k,hhN+TW_N+1);
        bakeInto(spr4); hhQuant(spr4,v,hhPal[v]);
        strideK=1; bakeInto(spr4s); strideK=0; hhQuantS(spr4s,v,hhPal[v]); hhKey[v]=kk; scratch=1;
    }
    for(int k=0;k<inmSets;k++){ int v=inmSetPl[k];   // the prison: one baked look per free place (prison clothes: inmDress, inmColors); the inmates show them
        for(int i=0;i<LK_N;i++) look[i]=inmS[k].look[i];
        stage=inmS[k].stage;
        buildLook(); inmDress(k); setColors(); inmColors(k); u32 kk=bakeKey(); if(kk==hhKey[v]) continue;
        ldShow("GETTING THE INMATES READY",hhN+k,hhN+TW_N+inmSets+1);
        bakeInto(spr4); hhQuant(spr4,v,hhPal[v]);
        strideK=1; bakeInto(spr4s); strideK=0; hhQuantS(spr4s,v,hhPal[v]); hhKey[v]=kk; scratch=1;
    }
    for(int i=0;i<LK_N;i++) look[i]=sl[i];
    stage=sst;
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ vox[y][z][x]=sv[y][z][x]; dec[y][z][x]=sd[y][z][x]; }
    custom=sc; setColors();
    u32 pk=bakeKey();
    if(scratch||pk!=sprKey){
        ldShow("ALMOST THERE",hhN+TW_N,hhN+TW_N+1);
        bakeInto(spr4);   // the player (still drawn by the CPU, so walls and furniture in front cover it and the action cam can zoom it)
        poseBakeAll(); strideK=1; bakeInto(spr4s); strideK=0; sprKey=pk;
    }
    spBounds(); poseWiden();      // the blit box holds both frames and the poses
    ldEnd();         // the loading screen is over: the game's display mode (window 0 + sprites) comes back
    hhSlotsFree();   // new tiles and palettes: every slot is reloaded when its Sim is next on screen
}
// ---- where members can stand ----
static int hhWalk(int x,int y){ if(x<0||y<0||x>=MW||y>=MH) return 0; char c=lifeMap[y][x]; return c!='w'&&c!='W'&&tileH(x,y)<=3; }
static int hhTakenAt(int x,int y); static void hhTakenScan(const HhSim*self);
static void hhPlace(HhSim*s,int k){   // somewhere free near the spawn point, spread out a little (never on someone else's tile)
    if(s>=hhM&&s<hhM+HH_MAX){ hhFl[s-hhM]=xo[XO_MULTIFL]?(u8)curFl:0; hhUp[s-hhM]=0; }
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
static void hhJoinCash(void);   // main.c: +SIM_JOIN_CASH for a Sim who joins
static int hhAdd(const u8*lk,int stg,int asp,int ltw,const u8*tr){   // a new member of the family (CREATE-A-FAMILY): -1 when the house is full
    if(hhN>=HH_MAX) return -1;
    twDrop(hhN);
    HhSim*s=&hhM[hhN]; s->uid=(u8)hhFreeUid(); s->bubT=0; s->hp=HP_MAX;
    for(int i=0;i<LK_N;i++) s->look[i]=lk[i];
    s->stage=(u8)stg; s->asp=(u8)asp; s->ltw=(u8)ltw; for(int i=0;i<TR_N;i++) s->tr[i]=tr[i];
    hhPickName(s->name); for(int i=0;i<HH_NM;i++) s->last[i]=hhPLast[i];   // family: your last name
    for(int k=0;k<HN_N;k++) s->need[k]=(u8)(70+(rnd8()&15));
    s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0;
    hhPlace(s,0);
    int a=s->uid;                                       // family: they know and like everyone at home already
    for(int u=0;u<HU_N;u++){ if(u==a) continue; relD[a][u]=relD[u][a]=40; relL[a][u]=relL[u][a]=50; relF[a][u]=relF[u][a]=0; kin[a][u]=kin[u][a]=0; }
    hhJoinCash();
    return hhN++;
}
static void hhRemove(int m){   // moves out: their sprites and relationships go with them
    if(m<0||m>=hhN) return;
    int a=hhM[m].uid; for(int u=0;u<RU_N;u++){ relD[a][u]=relD[u][a]=0; relL[a][u]=relL[u][a]=0; relF[a][u]=relF[u][a]=0; kin[a][u]=kin[u][a]=0; }
    famForget(a); nrForget(a);   // (their town friendships go with them)
    hhSprFree(m);   // their sprites go back to the pool; the others only move their descriptors
    for(int k=m;k<hhN-1;k++){ hhM[k]=hhM[k+1]; hhFl[k]=hhFl[k+1]; hhUp[k]=hhUp[k+1]; hhSp[k]=hhSp[k+1]; for(int i=0;i<16;i++) hhPal[k][i]=hhPal[k+1][i]; }
    if(hhN-1>m){ HhSpr*z=&hhSp[hhN-1]; for(int v=0;v<4;v++) z->sm[v]=z->dm[v]=0; z->off=0; z->len=0; }   // (no second descriptor for the same block)
    for(int k=m;k<hhN-1;k++) hhKey[k]=hhKey[k+1];
    hhKey[hhN-1]=0;   // the keys move with the sprites
    hhFl[hhN-1]=0; hhUp[hhN-1]=0; hhN--; hhSlotsFree();
}
static void hhNew(HhSim*s,const HhPre*p){
    s->uid=(u8)hhFreeUid(); s->bubT=0; s->hp=HP_MAX;
    for(int i=0;i<LK_N;i++) s->look[i]=p->look[i]; s->look[LK_SEX]=p->sex;
    s->stage=p->stage; s->asp=p->asp; s->ltw=0; for(int i=0;i<TR_N;i++) s->tr[i]=signTr[p->sign][i];
    int i=0; for(;p->name[i]&&i<HH_NM-1;i++) s->name[i]=p->name[i]; s->name[i]=0; s->last[0]=0;
    for(int k=0;k<HN_N;k++) s->need[k]=(u8)(70+(rnd8()&15));
    s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0;
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
        if(o->act==HA_WALK||o->act==HA_WANDER||o->act==HA_SEEK||o->act==HA_LEAVE||o->act==HA_STAIR) for(;k<o->pn;k++){ tx+=hhDx[o->path[k]]; ty+=hhDy[o->path[k]]; }
        if(tx>=0&&ty>=0&&tx<MW&&ty<MH) hhTk[hhTkN++]=(u16)(ty*MW+tx); }
}
static int hhTakenAt(int x,int y){ int p=y*MW+x; for(int i=0;i<hhTkN;i++) if(hhTk[i]==p) return 1; return 0; }
static int hhTaken(int x,int y,const HhSim*self){ hhTakenScan(self); return hhTakenAt(x,y); }
static int hhOut(HhSim*s){   // standing where nobody can stand (a wall was built on the spot): out to the nearest free tile around it. 1 = moved
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); hhTakenScan(s);
    for(int r=1;r<=4;r++)for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++){ if((dx<0?-dx:dx)!=r&&(dy<0?-dy:dy)!=r) continue;
        int x=sx+dx, y=sy+dy; if(hhWalk(x,y)&&!hhTakenAt(x,y)){ s->fx=x*256+128; s->fy=y*256+128; s->pn=s->pi=0; s->gok=0; return 1; } }
    return 0;
}
static int hhPlan(HhSim*s,char c){   // fills s->path; returns its length+1 (1 = already there), 0 = no way
    int sx=(int)(s->fx>>8), sy=(int)(s->fy>>8); if(!hhWalk(sx,sy)){ if(!hhOut(s)) return 0; sx=(int)(s->fx>>8); sy=(int)(s->fy>>8); }   // (inside a wall: step out of it first, never stuck there)
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
static int hhUseT(const HhSim*s){ if(s->item) return iuT[s->item-1].t; return (s->use==HN_REST&&simIsNight())?HH_USE*5:HH_USE; }   // (item module 3: an item has its own time)   // a night in bed is a long one
static int hhHasStairs(void){ if(curFl>=FLR_N-1) return 0; for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]=='^') return 1; return 0; }   // a way up on the ground floor
static int hhHasCh(char c){ for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]==c) return 1; return 0; }
static int hhFloorFor(int n){ for(int d=1;d<FLR_N;d++){ int u=curFl+d, w=curFl-d; if(u<FLR_N&&(hhCen[u]&(1<<n))) return u; if(w>=0&&(hhCen[w]&(1<<n))) return w; } return -1; }   // floors step 5: the nearest other floor with need n's furniture
static int hhElse(int n){ if(!xo[XO_MULTIFL]) return 0; int g=hhFloorFor(n); if(g<0) return 0; return hhHasCh(g>curFl?'^':'~')?(g>curFl?1:-1):0; }   // +1 the furniture is up the stairs, -1 down the stairs, 0 no way
// item module 6: NEED CHAINS. A finished use moves the OTHER needs: iuT[].dn for the items, iuBc[] for the five basics (eating fills the bladder, a night's sleep
// leaves a Sim hungry, a shower feels good, a sit-down is a little fun), so one need leads to the next and the household keeps moving.
static inline void iuMove(HhSim*s,const signed char*d){ for(int n=0;n<HN_N;n++){ int v=s->need[n]+d[n]; s->need[n]=(u8)(v<0?0:v>100?100:v); } }
static const signed char iuBc[HN_FUN][HN_N]={   //  FOOD WC REST CLEAN COMFY FUN SOC
    {  0,-15, 0, 0, 0, 0, 0},   // ate
    {  0,  0, 0, 0, 0, 0, 0},   // toilet
    {-10,-10, 0, 0, 0, 0, 0},   // slept
    {  0,  0, 0, 0, 5, 0, 0},   // showered
    {  0,  0, 0, 0, 0, 5, 0} }; // sat
static inline void iuBasic(HhSim*s){ iuMove(s,iuBc[s->use]); }
static inline void iuCouple(HhSim*s,int r){ iuMove(s,iuT[r].dn); }   // what a finished item use does to the other needs
static void hhNote(const HhSim*s,const char*w);
static void hhCallDone(HhSim*s){   // item module 5: the call ends: the closest friend who is parked on another floor picks up; both are less lonely and like each other a little more
    int m=-1, bs=-999, v;
    for(int k=0;k<hhN;k++){ const HhSim*q=&hhM[k]; if(q==s||q->act!=HA_AWAY||!hhUp[k]) continue; int d=(q->uid<HU_N&&s->uid<HU_N?relD[s->uid][q->uid]:0)+(rnd8()&15); if(d>bs){ bs=d; m=k; } }
    if(m<0) return;
    HhSim*o=&hhM[m]; int a=s->uid, b=o->uid;
    if(a<HU_N&&b<HU_N){ v=relD[a][b]+3; relD[a][b]=(signed char)(v>100?100:v); v=relD[b][a]+3; relD[b][a]=(signed char)(v>100?100:v); v=relL[a][b]+1; relL[a][b]=(signed char)(v>100?100:v); v=relL[b][a]+1; relL[b][a]=(signed char)(v>100?100:v); }
    v=o->need[HN_SOC]+25; o->need[HN_SOC]=(u8)(v>100?100:v); hhNote(s," HAD A PHONE CALL");
}
static void iuStart(HhSim*s){   // item module 10: a Sim starts an item: its balloon, and "NAME IS WATCHING TV" when it is within 8 tiles of you (hhNote waits while another note is up)
    int r=s->item-1; if(r<0) return;
    s->bub=iuIc[r]; s->bubT=90;
    s32 dx=(s->fx-lfx)>>8, dy=(s->fy-lfy)>>8; if(dx<0) dx=-dx; if(dy<0) dy=-dy;
    if(dx<=8&&dy<=8) hhNote(s,iuT[r].say);
}
static int iuFloorFor(int r){ for(int d=1;d<FLR_N;d++){ int u=curFl+d, w=curFl-d; if(u<FLR_N&&(hhCenI[u]&(1<<r))) return u; if(w>=0&&(hhCenI[w]&(1<<r))) return w; } return -1; }   // the nearest other floor with item r
static int hhItemGo(HhSim*s,int need){   // item module 3: serve 'need' with an item. On this floor: walk there and use it. On another floor (SIMS ON FLOORS): to the stairs. 1 = the Sim has a plan
    int r=iuPick(s,need,curFl);
    if(r>=0){ int q=hhPlan(s,iuT[r].ch);
        if(q==1){ s->item=(u8)(r+1); s->act=HA_USE; s->use=iuT[r].need; s->t=hhUseT(s); iuStart(s); return 1; }
        if(q>1){ s->item=(u8)(r+1); s->act=HA_WALK; s->use=iuT[r].need; return 1; } }
    if(xo[XO_MULTIFL]) for(int d=1;d<FLR_N;d++) for(int sg=0;sg<2;sg++){ int g=sg?curFl-d:curFl+d; if(g<0||g>=FLR_N) continue;
        int r2=iuPick(s,need,g); if(r2<0||!hhHasCh(g>curFl?'^':'~')) continue;
        int q=hhPlan(s,g>curFl?'^':'~'); if(q<1) continue;
        if(q==1){ s->pn=s->pi=0; s->gok=0; } s->act=HA_STAIR; s->use=(u8)need; s->item=(u8)(r2+1); return 1; }
    return 0;
}
static void hhDecide(HhSim*s){
    s->item=0;
    int best[2]={-1,-1}, bs[2]={0,0}, low=xo[XO_FREEWILL]==1?35:55;   // LOW free will waits until needs are lower
    for(int n=0;n<HN_N;n++){
        if(hnFurn[n]&&!(simHave&(n==HN_FOOD?SR_FRIDGE:n==HN_WC?SR_TOILET:n==HN_REST?SR_BED:n==HN_CLEAN?SR_SHOWER:SR_SOFA))&&!hhElse(n)) continue;   // no such furniture on this floor or above
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
    if(xo[XO_ITEMUSE]&&n==HN_REST&&!simIsNight()&&s->need[HN_REST]>=25&&(rnd8()&1)&&hhItemGo(s,HN_REST)) return;   // item module 4: tired but not wrecked, in the daytime: a coffee first (really tired: bed)
    if(xo[XO_ITEMUSE]&&(n==HN_FUN||(n<0&&rnd8()<70))&&hhItemGo(s,HN_FUN)) return;   // item module 3: FUN = the TV, bookshelf, aquarium, treadmill, stereo (nothing pressing: a hobby now and then)
    if(n==HN_SOC){ if(xo[XO_ITEMUSE]&&rnd8()<90&&hhItemGo(s,HN_SOC)) return; hhSeek(s); return; }   // item module 5: sometimes lonely means the phone
    if(n<0&&rnd8()<30&&hhHasStairs()){   // nothing pressing: sometimes up the stairs for a while
        int r=hhPlan(s,'^'); if(r>=1){ if(r==1){ s->pn=s->pi=0; s->gok=0; } s->act=HA_STAIR; s->use=HN_FUN; return; } }
    if(n<0&&hhN>0&&(rnd8()*100>>8)<25+s->tr[TR_OUT]*5){ hhSeek(s); return; }   // nothing pressing: go and see someone (outgoing Sims more often)
    if(n<0){ if(hhPlan(s,0)>1){ s->act=HA_WANDER; s->use=HN_FUN; } else s->act=HA_IDLE; return; }
    if(hnFurn[n]&&!(hhCen[curFl]&(1<<n))&&hhElse(n)){ int q=hhPlan(s,hhElse(n)>0?'^':'~'); if(q>=1){ if(q==1){ s->pn=s->pi=0; s->gok=0; } s->act=HA_STAIR; s->use=(u8)n; } else s->act=HA_IDLE; return; }   // floors step 3: not on this floor: up the stairs
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
static char hhNoteB[40] EWRAM_BSS;
static void hhNote(const HhSim*s,const char*w){ if(lnoteT>0) return; char*e=simCat(hhNoteB,s->name); simCat(e,w); lnote=hhNoteB; lnoteT=110; }
static void hhStepAlong(HhSim*s){   // one step along the path, tile centre to tile centre
    int d=s->path[s->pi];
    if(!s->gok&&!hhWalk((int)(s->fx>>8)+hhDx[d],(int)(s->fy>>8)+hhDy[d])){ s->pi=s->pn; return; }   // a wall was built across the way since the plan: stop here (the next plan goes round it)
    if(!s->gok){ s->gx=((int)(s->fx>>8)+hhDx[d])*256+128; s->gy=((int)(s->fy>>8)+hhDy[d])*256+128; s->fx=(s->fx&~255)|128; s->fy=(s->fy&~255)|128; s->gok=1; }
    int sp=F_WALK*stSpd[s->stage]/100; if(sp<2) sp=2;
    if(hhDx[d]){ s->fx+=hhDx[d]*sp; if((hhDx[d]>0&&s->fx>=s->gx)||(hhDx[d]<0&&s->fx<=s->gx)){ s->fx=s->gx; s->pi++; s->gok=0; } }
    else { s->fy+=hhDy[d]*sp; if((hhDy[d]>0&&s->fy>=s->gy)||(hhDy[d]<0&&s->fy<=s->gy)){ s->fy=s->gy; s->pi++; s->gok=0; } }
    s->hd=(u8)(d*4);
}
static char twMsg[40] EWRAM_BSS;   // "ROXY STACK FROM MAPLE 2 DROPS BY"
static int twFar(void){   // where a visitor comes from / goes to: a way off the lot, or (walls all round) somewhere walkable well away from you. -1 = nowhere
    if(FLG(0)){ int f=rnd8()%flgN[0]; return flgY[0][f]*MW+flgX[0][f]; }   // a COMMUNITY FLAG on the lot: visitors walk in from it and out to it (main.c, FLAGS)
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
            if(*planned||(simIsNight()&&!twWel[k]&&!twCall[k])){ twWait[k]=60; continue; }
            int a=twFar(); if(a<0){ twWait[k]=120; continue; } s->fx=(a%MW)*256+128; s->fy=(a/MW)*256+128; *planned=1;
            hhGX=(int)(lfx>>8); hhGY=(int)(lfy>>8);
            if(hhPlan(s,1)>1){ twOn[k]=1; s->act=HA_WALK; twCall[k]=0;   // walking in, over to you
                char*e=twMsg; for(const char*p=s->name;*p;) *e++=*p++; *e++=' '; for(const char*p=s->last;*p;) *e++=*p++;
                if(twWel[k]){ const char*p=" SAYS WELCOME"; while(*p) *e++=*p++; }   // (the welcome visit)
                else if(twFrom[k][0]){ const char*p=" FROM "; while(*p) *e++=*p++; for(p=twFrom[k];*p;) *e++=*p++; } else { const char*p=" DROPS BY"; while(*p) *e++=*p++; }
                *e=0;
                lnote=twMsg; lnoteT=90; }
            else twWait[k]=120;
            continue;
        }
        if(twOn[k]==2){ if(--twWait[k]>0){ if(((twWait[k]+k*170)&511)==0) nrGuestChat(k); continue; }   // staying a while (now and then they come to you with something to say), then off again
            if(*planned){ twWait[k]=30; continue; }
            int b=twFar(); if(b<0){ twOn[k]=0; twWait[k]=900; continue; } hhGX=b%MW; hhGY=b/MW; *planned=1;
            if(hhPlan(s,1)>1){ twOn[k]=3; s->act=HA_WALK; } else twWait[k]=60;
            continue; }
        if(s->pi>=s->pn){
            if(twOn[k]==1){ twOn[k]=2; s->act=HA_IDLE; twWait[k]=(short)(360+(rnd8()<<2));
                if(twWel[k]){ twWel[k]=0; simMoneyAdd(500); moodEvent(M_PAY); lnote="WELCOME GIFT  500"; lnoteT=90; }   // there: the housewarming gift   // there: stays 6 to 23 seconds
                if(!sfxV&&!curFl) sfxPlay(SFX_BELL); }   // the doorbell (when nothing else is sounding)
            else { twOn[k]=0; twWait[k]=(short)(nrPull(k)?600+(rnd8()<<3):900+(rnd8()<<4)); nrSync(); }   // gone: the next visit in a while (sooner when they like you)
            continue; }
        hhStepAlong(s);
    }
}
// ---- SIMS ON FLOORS (floors step 2): where each Sim is, and what each floor has ----
static void hhCensus(void){   // which need furniture each floor has (bit = need number). flPlaneAt reads the live map for this floor and the packed copy for the others
    hhCenT=300;
    for(int f=0;f<FLR_N;f++){ u8 b=0, bi=0;
        for(int y=0,i=0;y<MH;y++)for(int x=0;x<MW;x++,i++){   // (row by row: flPlaneAt works the row and column out of i with two divisions a cell, which made this a 2-frame hitch every 5 seconds)
            int c=f==curFl?(u8)lifeMap[y][x]:flLen[f]?flGet(f,0,i):((x==0||y==0||x==MW-1||y==MH-1)?'w':'.');   // (the live map, the stored floor, or a blank floor's carpet and low wall: flBlankV)
            if(c=='F') b|=1<<HN_FOOD; else if(c=='T') b|=1<<HN_WC; else if(c=='S') b|=1<<HN_REST; else if(c=='H') b|=1<<HN_CLEAN; else if(c=='C'||c=='U') b|=1<<HN_COMFY;
            else if(c!='.'&&c!='w'&&c!='W'){ int r=iuRow((char)c); if(r>=0) bi|=1<<r; } }   // (plain floor and walls hold no item: most cells skip the item table)
        hhCen[f]=b; hhCenI[f]=bi; }
}
static void hhStairSpot(HhSim*s,char c){   // floors step 4: stand on a free tile next to the stairs c ('^' up, '~' down), where a Sim arrives from another floor
    int sx=-1, sy=-1;
    for(int y=0;y<MH&&sx<0;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]==c){ sx=x; sy=y; break; }
    if(sx<0){ sx=(int)(lfx>>8); sy=(int)(lfy>>8); }
    hhTakenScan(s);
    for(int r=1;r<6;r++)for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++){ if((dx<0?-dx:dx)!=r&&(dy<0?-dy:dy)!=r) continue;
        int x=sx+dx, y=sy+dy; if(hhWalk(x,y)&&!hhTakenAt(x,y)){ s->fx=x*256+128; s->fy=y*256+128; return; } }
    s->fx=sx*256+128; s->fy=sy*256+128;
}
// ---- floors step 8: a Sim on a floor you are not on still lives (coarsely, no walking): when its timer ends it counts the need it was serving as done if its floor has the
// furniture, then looks after its worst need: furniture here = it uses it; furniture on another floor = one floor toward it; nothing pressing = drifts down to the ground floor.
// A Sim that reaches the floor you are on steps out of the stairs (the sync block in hhTick). hhUp is the timer and also the "parked" mark, so every branch leaves it at 2 or more.
static const char* hhWhere(int m){ static const char*const t[]={"  GROUND","  FLOOR 2","  FLOOR 3"}; return xo[XO_MULTIFL]&&hhFl[m]<3?t[hhFl[m]]:"  UPSTAIRS"; }
static void hhOffStep(int m){
    HhSim*s=&hhM[m]; int f=hhFl[m], any=0, low=xo[XO_FREEWILL]==1?35:55, best=-1, bv=101;
    for(int g=0;g<FLR_N;g++) any|=hhCen[g];
    if(xo[XO_ITEMUSE]&&s->item){ int r=s->item-1;   // item module 3: bound for an item on another floor: it uses it when it gets there, else one floor toward it
        if(hhCenI[f]&(1<<r)){ int v=s->need[iuT[r].need]+iuT[r].t/2; s->need[iuT[r].need]=(u8)(v>100?100:v); iuCouple(s,r); s->item=0; s->use=HN_FUN; }
        else { int g=-1; for(int d=1;d<FLR_N&&g<0;d++){ if(f+d<FLR_N&&(hhCenI[f+d]&(1<<r))) g=f+1; else if(f-d>=0&&(hhCenI[f-d]&(1<<r))) g=f-1; }
            if(g>=0){ hhFl[m]=(u8)g; hhUp[m]=90; return; } s->item=0; } }
    if(s->use<HN_FUN&&(hhCen[f]&(1<<s->use))){ s->need[s->use]=100; if(xo[XO_ITEMUSE]) iuBasic(s); }   // it used the furniture while it waited
    for(int n=0;n<HN_FUN;n++){ int v=s->need[n]; if(!(any&(1<<n))) continue; if(n==HN_REST&&simIsNight()) v=v>45?v-45:0; if(v<low+30&&v<bv){ bv=v; best=n; } }
    if(best>=0){ s->use=(u8)best;
        if(hhCen[f]&(1<<best)){ hhUp[m]=(u16)(hhUseT(s)+2); return; }   // here: it uses it
        int g=-1; for(int d=1;d<FLR_N&&g<0;d++){ if(f+d<FLR_N&&(hhCen[f+d]&(1<<best))) g=f+1; else if(f-d>=0&&(hhCen[f-d]&(1<<best))) g=f-1; }
        if(g>=0){ hhFl[m]=(u8)g; hhUp[m]=90; return; } }   // one floor toward it
    s->use=HN_FUN;
    if(f>0){ hhFl[m]=(u8)(f-1); hhUp[m]=90; return; }   // nothing pressing: drifts back down
    hhUp[m]=60;   // on the ground floor while you are upstairs: waits for you
}
static void hhAmbient(void){   // item module 12: while a Sim has the stereo on, everyone else awake on the floor enjoys it (+1 FUN every 2 seconds)
    if(!xo[XO_ITEMUSE]||lfr%120) return;
    int on=0; for(int m=0;m<hhN;m++){ const HhSim*s=&hhM[m]; if(s->act==HA_USE&&s->item&&iuT[s->item-1].ch=='A'&&(hhFl[m]==curFl||!xo[XO_MULTIFL])) on=1; }
    if(!on) return;
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m]; if(s->act==HA_AWAY||s->need[HN_FUN]>=100||(s->act==HA_USE&&s->item&&iuT[s->item-1].ch=='A')) continue; s->need[HN_FUN]++; }
}
#include "panic.h"   // PANIC: Sims run from gunfire, blasts and shot Sims (hhScare)
static void hhTick(void){   // once per logic step in the life game
    if(xo[XO_MULTIFL]||xo[XO_ITEMUSE]){ if(hhCenT) hhCenT--; else hhCensus(); }   // floors step 2: keep the per-floor furniture census fresh (item module 2: item use reads it too)
    if(curFl&&!xo[XO_MULTIFL]) return;   // upstairs: the household waits on the ground floor (SIMS ON FLOORS: the Sims up here carry on)
    if(hhBubT) hhBubT--;
    if(lvx||lvy||lsp||lairF||lgrind) hhStill=0; else if(hhStill<1000) hhStill++;
    int planned=0;
    if(xo[XO_FREEWILL]&&!curFl) twTick(&planned);
    if(!curFl){ copTick(&planned); inmTick(&planned); }   // npc.h: the police (a cop comes after you when you hurt Sims)
    if(!hhN) return;
    relTick(); hhAmbient();
    int fe=oFoodEvery(), we=oWcEvery();
    for(int m=0;m<hhN;m++){ HhSim*s=&hhM[m];
        // needs drain (gently: the Sims you do not watch should not be in constant crisis)
        if(fe&&lfr%(fe*2)==0&&s->need[HN_FOOD]>0) s->need[HN_FOOD]--;
        if(we&&lfr%(we*2)==0&&s->need[HN_WC]>0) s->need[HN_WC]--;
        if(lfr%300==m*7){ for(int n=HN_REST;n<HN_N;n++) if(s->need[n]>0) s->need[n]--; }
        if(s->bubT) s->bubT--;
        if(s->hp<HP_MAX&&lfr%HP_REGEN==(m*11)%HP_REGEN) s->hp++;   // health creeps back (knocked out Sims wake at 30)
        if(lfr%(150-s->tr[TR_OUT]*8)==0&&s->need[HN_SOC]>0) s->need[HN_SOC]--;   // lonely sooner when outgoing
        if(prHeld(s)){ s->act=HA_AWAY; continue; }   // prison.h: the prisoner is out while you are at home, everyone else while you are in the cell
        if(xo[XO_MULTIFL]){   // floors step 4: Sims who are not on your floor wait (parked), the ones whose floor you came to step out of the stairs
            if(hhFl[m]!=curFl){
                if(xo[XO_FREEWILL]){ int fr=0, to=0, k=hhSched(s,&fr,&to); if(k&&simMin>=fr&&simMin<to&&!(s->act==HA_AWAY&&!hhUp[m])){ s->act=HA_AWAY; hhFl[m]=0; hhUp[m]=0; s->use=(u8)k; s->item=0; hhNote(s,k==2?" WENT TO SCHOOL":" LEFT FOR WORK"); continue; } }   // floors step 9: a Sim parked on another floor still leaves for work or school on time
                if(s->act!=HA_AWAY){ s->act=HA_AWAY; s->use=HN_FUN; s->item=0; hhUp[m]=(u16)(300+(rnd8()<<2)); }   // you left their floor: they stay there a while
                if(hhUp[m]>1) hhUp[m]--; else if(hhUp[m]) hhOffStep(m);   // floors step 8: a Sim on another floor lives coarsely (hhOffStep)
                continue; }
            else if(s->act==HA_AWAY&&hhUp[m]){ s->use=HN_FUN; s->item=0; hhUp[m]=0; hhStairSpot(s,curFl?'~':'^'); s->act=HA_IDLE; s->think=20; s->gok=0; s->pn=s->pi=0; continue; } }
        if(hhUp[m]){   // upstairs: gone from the ground floor until the time is up, then back down the stairs (waits if someone stands there)
            if(s->act!=HA_AWAY){ hhUp[m]=0; }
            else { if(!xo[XO_FREEWILL]&&hhUp[m]>1) hhUp[m]=1;
                if(hhUp[m]>1){ hhUp[m]--; continue; }
                if(hhTaken((int)(s->fx>>8),(int)(s->fy>>8),s)){ hhUp[m]=30; continue; }
                hhUp[m]=0; if(hhFl[m]){ hhFl[m]=0; hhStairSpot(s,'^'); } if(xo[XO_MULTIFL]&&s->use<HN_FUN) s->need[s->use]=100; s->act=HA_IDLE; s->think=20; s->gok=0; s->pn=s->pi=0; hhNote(s," CAME DOWNSTAIRS"); continue; } }
        if(hhPanT[m]){ if(s->act==HA_SOC&&s->bub==IC_SKULL) hhPanT[m]=0; else { hhPanicStep(m,s,&planned); continue; } }   // PANIC: running away
        if(s->act==HA_SOC){ if(--s->t<=0){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); } continue; }   // standing in a conversation
        if(!xo[XO_FREEWILL]){ if(s->act==HA_AWAY) s->fx=hhExX*256+128, s->fy=hhExY*256+128; s->act=HA_IDLE; continue; }
        { int fr=0, to=0, k=hhSched(s,&fr,&to), due=k&&simMin>=fr&&simMin<to;   // the day's routine
          if(s->act==HA_AWAY){ if(due) continue;
              s->fx=hhExX*256+128; s->fy=hhExY*256+128; s->act=HA_IDLE; s->think=20; s->gok=0;   // home again: walks in from the edge
              static const u8 dn[HN_N]={25,15,20,15,10,20,0}; for(int n=0;n<HN_N;n++) s->need[n]=(u8)(s->need[n]>dn[n]?s->need[n]-dn[n]:0);
              hhNote(s,s->use==2?" IS BACK FROM SCHOOL":" IS HOME FROM WORK"); continue; }
          if(due&&curFl&&s->act!=HA_AWAY){ s->act=HA_AWAY; hhFl[m]=0; hhUp[m]=0; s->use=(u8)k; hhNote(s,k==2?" WENT TO SCHOOL":" LEFT FOR WORK"); continue; }
          if(due&&s->act!=HA_LEAVE&&!planned){   // time to go: walk to the way off (one search per step, like every plan)
              s->use=(u8)k; hhGX=hhExX; hhGY=hhExY; planned=1;
              int r=hhPlan(s,1); if(r>1) s->act=HA_LEAVE; else { s->act=HA_AWAY; hhNote(s,k==2?" WENT TO SCHOOL":" LEFT FOR WORK"); }
              continue; }
          if(due&&s->act!=HA_LEAVE) continue; }
        if(s->act==HA_USE){   // using furniture: refill, then free again (a night's sleep lasts until it is over, rested or not)
            if(s->need[s->use]<100&&(lfr&1)) s->need[s->use]++;
            if(--s->t<=0||(s->need[s->use]>=100&&!(s->use==HN_REST&&simIsNight()))){ s->act=HA_IDLE; s->think=(short)(HH_THINK/2); if(s->item){ if(iuT[s->item-1].ch=='I') hhCallDone(s); else if(iuT[s->item-1].ch=='q') hmFishFed(); iuCouple(s,s->item-1); s->ilast=s->item; s->item=0; } else if(xo[XO_ITEMUSE]&&s->use<HN_FUN) iuBasic(s); }
            continue;
        }
        if(s->act==HA_WALK||s->act==HA_WANDER||s->act==HA_SEEK||s->act==HA_LEAVE||s->act==HA_STAIR){   // follow the path, tile centre to tile centre
            if(s->pi>=s->pn&&s->act==HA_SEEK){ hhArrive(m); continue; }
            if(s->pi>=s->pn&&s->act==HA_STAIR){ s->act=HA_AWAY; hhUp[m]=(u16)(xo[XO_MULTIFL]&&(s->use<HN_FUN||s->item)?hhUseT(s)+90:600+(rnd8()<<3)); if(xo[XO_MULTIFL]){ int g=s->item?iuFloorFor(s->item-1):s->use<HN_FUN?hhFloorFor(s->use):-1; hhFl[m]=(u8)((g>=0&&g<curFl)||curFl+1>=FLR_N?curFl-1:curFl+1); } hhNote(s,xo[XO_MULTIFL]&&hhFl[m]<curFl?" WENT DOWNSTAIRS":" WENT UPSTAIRS"); continue; }
            if(s->pi>=s->pn&&s->act==HA_LEAVE){ s->act=HA_AWAY; hhNote(s,s->use==2?" WENT TO SCHOOL":" LEFT FOR WORK"); continue; }
            if(s->pi>=s->pn){ if(s->act==HA_WALK){ s->act=HA_USE; s->t=hhUseT(s); iuStart(s); } else { s->act=HA_IDLE; if(s->need[HN_FUN]<90) s->need[HN_FUN]+=10; } continue; }
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
// other a bit less. Mean ones (ARGUE, INSULT, TEASE, SHOVE, SLAP, PUNCH) can be done to anyone at any time, friend, family or stranger (the physical ones
// teens and up), and always land: the target likes the asker less.
// Statuses follow the scores: FRIEND (daily 50+), BEST FRIEND (daily and lifetime 70+), ENEMY (daily -50 or less), and the romance
// steps CRUSH (a flirt was accepted), IN LOVE (kissed, and lifetime 60+ both ways), STEADY (asked and said yes). Daily drifts back to
// lifetime over the hours, so friendships need keeping up.
enum { SA_ROM=1, SA_MEAN=2, SA_CRUSH=4, SA_LOVE=8, SA_KID=16, SA_PIPE=32, SA_WED=64, SA_FAM=128 };   // SA_WED: going steady, not married; SA_FAM: a couple, room for a baby (family.h)   // SA_PIPE: grown-ups, with a water pipe in the house
typedef struct { const char* name; signed char dA,lA,dR,lR; u8 soc,fun; signed char minD,maxD; u8 base,tr,fl,icA,icR; const char*say,*yes,*no; } SocAct;
enum { SC_TALK, SC_JOKE, SC_COMPL, SC_HIGH5, SC_HUG, SC_TRICK, SC_FLIRT, SC_KISS, SC_STEADY, SC_SORRY, SC_ARGUE, SC_INSULT, SC_SLAP, SC_PUNCH, SC_PASS, SC_PROPOSE, SC_BABY, SC_TEASE, SC_SHOVE, SC_N };
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
    {"INSULT",      -10, -4,  0,  0,  4,  0,-100, 100,100,TR_NICE,SA_MEAN|SA_KID,      IC_SAD,  IC_SAD,  "LOSER",   "LOOKS HURT",    ""},
    {"SLAP",        -16, -6,  0,  0,  4,  0,-100, 100,100,TR_NICE,SA_MEAN,             IC_HURT, IC_HURT, "SMACK",   "GOT SLAPPED",   ""},
    {"PUNCH",       -20, -8,  0,  0,  4,  0,-100, 100,100,TR_NICE,SA_MEAN,             IC_HURT, IC_HURT, "TAKE THAT","GOT PUNCHED",  ""},   // teens and up; takes HP (fightHit)
    {"PUFF PUFF PASS", 6,  2, -3,  0, 14, 14, -10, 100, 80,TR_PLAY,SA_PIPE,             IC_LEAF, IC_BAIL, "PASS IT", "TOOK A HIT",    "PASSED"},
    {"PROPOSE",      16, 12,-14, -6, 24,  6,  75, 100, 60,TR_NICE,SA_ROM|SA_WED,       IC_HEART,IC_BAIL, "MARRY ME","SAID YES",      "SAID NOT YET"},   // adults going steady: a wedding (family.h)
    {"TRY FOR A BABY",8,  4, -6, -2, 20, 10,  60, 100, 70,TR_NICE,SA_ROM|SA_FAM,       IC_HEART,IC_BAIL, "A BABY?", "WANTS ONE TOO", "NOT NOW"},        // a couple: maybe a baby in 3 days (family.h)
    {"TEASE",        -5, -1,  0,  0,  6,  4,-100, 100,100,TR_NICE,SA_MEAN|SA_KID,      IC_ANGRY,IC_SAD,  "NYAH NYAH","GOT TEASED",   ""},   // the mildest one: any age
    {"SHOVE",       -12, -5,  0,  0,  4,  0,-100, 100,100,TR_NICE,SA_MEAN,             IC_ANGRY,IC_HURT, "MOVE IT", "GOT SHOVED",    ""},   // teens and up: no HP, just rude
};
static void famWed(int a,int b); static void famTry(int a,int b); static void famForget(int u);   // family.h
static int nrWedIn(int g);   // townrel.h: a neighbour said yes to PROPOSE: they move in (their new uid, -1 = they could not)
static int nrCanAsk(int a,int g); static void nrAskIn(int a,int g); static void nrBye(int g);   // townrel.h: ASK TO MOVE IN, SAY GOODBYE
static int hhFreeUid(void){ for(int u=0;u<HU_N;u++){ if(u==hhPUid) continue; int k=0; for(int m=0;m<hhN;m++) if(hhM[m].uid==u) k=1; if(!k) return u; } return 0; }
static int hhOthers(void){ return hhN>0; }
static int hhMemOf(int uid){ if(uid>=GU0){ int k=uid-GU0; return k<TW_N?TW_V(k):-1; } for(int m=0;m<hhN;m++) if(hhM[m].uid==uid) return m; return -1; }   // -1: the player (or nobody). A guest: the place it is staying in
static inline int uGuest(int u){ return u>=GU0&&u<RU_N; }   // a neighbour who is on the lot (GUESTS above)
static int uStage(int u){ int m=hhMemOf(u); return m<0?stage:hhM[m].stage; }
static int uSex(int u){ int m=hhMemOf(u); return m<0?look[LK_SEX]:hhM[m].look[LK_SEX]; }   // GENDER (SX_*)
static void kinParent(int p,int c){   // p is c's parent: MOTHER / FATHER / PARENT, and c is p's DAUGHTER / SON / CHILD (by their GENDER)
    static const u8 par[SX_N]={KN_MOTHER,KN_FATHER,KN_PARENT}, kid[SX_N]={KN_DAUGHTER,KN_SON,KN_CHILD};
    int a=uSex(p), b=uSex(c); kin[p][c]=par[a<SX_N?a:SX_NB]; kin[c][p]=kid[b<SX_N?b:SX_NB]; }
static void kinSib(int a,int b){ static const u8 sib[SX_N]={KN_SISTER,KN_BROTHER,KN_SIBLING}; int x=uSex(a), y=uSex(b); kin[a][b]=sib[x<SX_N?x:SX_NB]; kin[b][a]=sib[y<SX_N?y:SX_NB]; }
static void uSetSex(int u,int x){ int m=hhMemOf(u); if(m<0) look[LK_SEX]=(u8)x; else hhM[m].look[LK_SEX]=(u8)x; }
static int uTr(int u,int t){ int m=hhMemOf(u); return m<0?pTr[t]:hhM[m].tr[t]; }
static int uMood(int u){ int m=hhMemOf(u); if(m<0) return moodHapPct(); const HhSim*s=&hhM[m]; int v=0; for(int n=0;n<HN_N;n++) v+=s->need[n]; return v/HN_N; }
static const char* uName(int u){ int m=hhMemOf(u); return m<0?hhPName:hhM[m].name; }
static int ageBand(int st){ return st<AG_TEEN?0:st==AG_TEEN?1:2; }   // romance only within a band: teens with teens, adults with adults and elders
static int romOk(int a,int b){ int sa=uStage(a), sb=uStage(b); return sa>=AG_TEEN&&sb>=AG_TEEN&&ageBand(sa)==ageBand(sb); }
static int hhRomanceOk(void){ for(int m=0;m<hhN;m++) if(romOk(hhPUid,hhM[m].uid)) return 1; return 0; }
static int clampR(int v){ return v<-100?-100:v>100?100:v; }
static const char* relWord(int a,int b){   // how a sees b
    u8 f=relF[a][b]; int d=relD[a][b], l=relL[a][b];
    if(kinRom(kin[b][a])) return kinNm[kin[b][a]];   // married (or partners): WIFE / HUSBAND / PARTNER
    if(f&RF_STEADY) return sexWord(SW_DATE,uSex(b));   // going steady: GIRLFRIEND / BOYFRIEND / PARTNER
    if(f&RF_LOVE) return "IN LOVE";
    if(f&RF_CRUSH) return "CRUSH";
    if(d>=70&&l>=70) return "BEST FRIEND";
    if(d>=50) return "FRIEND";
    if(d<=-50) return "ENEMY";
    if(d<=-20) return "DISLIKE";
    if(d==0&&l==0) return "STRANGER";
    return "ACQUAINTANCE";
}
static int uPipeOk(int u){ return u==hhPUid?pipeOk():uStage(u)>=AG_ADULT; }   // you: PIPE AGE applies; members: grown-ups (they keep no days-in-stage)
static int socAllowed(int a,int b,int i){   // may a do interaction i to b now?
    const SocAct*S=&socT[i]; int d=relD[a][b];
    if(d<S->minD||d>S->maxD) return 0;
    if(!(S->fl&SA_KID)&&(uStage(a)<AG_TEEN||uStage(b)<AG_TEEN)) return 0;
    if((S->fl&SA_ROM)&&!romOk(a,b)) return 0;
    if((S->fl&SA_ROM)&&(kin[a][b]>=KN_MOTHER||kin[b][a]>=KN_MOTHER)) return 0;   // family (CREATE-A-HOUSEHOLD): friends, never lovers
    if((S->fl&SA_PIPE)&&(!uPipeOk(a)||!uPipeOk(b)||!(simHave&SR_PIPE))) return 0;
    if((S->fl&SA_CRUSH)&&!(relF[a][b]&RF_CRUSH)) return 0;
    if((S->fl&SA_LOVE)&&(!(relF[a][b]&RF_LOVE)||(relF[a][b]&RF_STEADY))) return 0;
    if(i==SC_TRICK&&uStage(a)<AG_CHILD) return 0;
    if((S->fl&SA_FAM)&&(uGuest(a)||uGuest(b))) return 0;   // babies: with someone who lives here
    if((S->fl&SA_WED)&&uGuest(a)) return 0;   // (a guest never proposes: you ask them, and a yes moves them in, townrel.h)
    if((S->fl&SA_WED)&&uGuest(b)&&hhN>=HH_MAX) return 0;   // ... so there has to be room for them
    if((S->fl&(SA_WED|SA_FAM))&&(hhMemOf(b)<0&&b!=hhPUid)) return 0;
    if((S->fl&SA_WED)&&(!(relF[a][b]&RF_STEADY)||kinWed(kin[a][b])||uStage(a)<AG_ADULT||uStage(b)<AG_ADULT)) return 0;
    if((S->fl&SA_FAM)&&(!((relF[a][b]&RF_STEADY)||kinRom(kin[a][b]))||uStage(a)!=AG_ADULT||uStage(b)!=AG_ADULT||famDue||hhN>=HH_MAX)) return 0;
    return 1;
}
static void needAdd(int u,int n,int v){   // a need of anyone (n: HN_SOC or HN_FUN)
    int m=hhMemOf(u);
    if(m<0){ if(n==HN_SOC){ if(v>0){ v+=v*skLvl(SK_CHARM)*10/100; skGain(SK_CHARM,1); } sSoc+=v; if(sSoc>100) sSoc=100; if(sSoc<0) sSoc=0; } else moodFun=moodClamp(moodFun+v*MOOD_ONE); return; }
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
static void hhFreeze(int u,int steps){ int m=hhMemOf(u); if(m<0){ if(lstun<steps) lstun=steps; lsp=0; lgrind=0; }
    else if(uGuest(u)){ int k=u-GU0; if(twOn[k]==2&&twWait[k]<steps+300) twWait[k]=(short)(steps+300); }   // a guest: a good talk makes them stay a while longer
    else { hhM[m].act=HA_SOC; hhM[m].t=(short)steps; } }
static void socNote(int a,int b,int i,int ok){   // what you read when you are part of it: "REX LAUGHED +6"
    if(a!=hhPUid&&b!=hhPUid) return;
    const SocAct*S=&socT[i]; char*e=simMsg2;
    if(a==hhPUid){ e=simCat(e,uName(b)); *e++=' '; e=simCat(e,ok?S->yes:S->no); }
    else { e=simCat(e,uName(a)); *e++=' '; const char*w=S->name; char lw[16]; int k=0; for(;w[k]&&k<15;k++) lw[k]=w[k]; lw[k]=0;
        e=simCat(e,(S->fl&SA_MEAN)?(i==SC_PUNCH?"PUNCHED YOU":i==SC_SLAP?"SLAPPED YOU":i==SC_ARGUE?"PICKED A FIGHT":i==SC_TEASE?"TEASED YOU":i==SC_SHOVE?"SHOVED YOU":"INSULTED YOU"):i==SC_TALK?"CAME TO CHAT":i==SC_FLIRT?"FLIRTS WITH YOU":i==SC_KISS?"KISSED YOU":i==SC_HUG?"HUGS YOU":i==SC_STEADY?"ASKS YOU OUT":i==SC_PROPOSE?"PROPOSED TO YOU":i==SC_BABY?"WANTS A BABY WITH YOU":lw); }
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
static char fkB[28] EWRAM_BSS;   // what the one who lands a blow reads: "CRIT SLASH 27"
static void fightHit(int a,int b){
    const u8*la=fkLook(a), *lb=fkLook(b);
    if(a==hhPUid) copCrime(1);   // hitting someone is a crime (npc.h)
    if((rnd8()*100>>8)<fkDodge(lb)){ hhSay(b,IC_BAIL,"DODGED"); return; }   // eye stalks see it coming, wings flap clear
    int dmg=14+(uTr(a,TR_ACT)>>1)+(rnd8()>>5)+fkAtk(la);
    { int tk=fkTaken(lb); if(la[LK_CLAWS]==3) tk+=(100-tk)/4; dmg=dmg*tk/100; } if(fgMul) dmg=dmg*fgMul/100; if(dmg<1) dmg=1;   // BLADES cut through a quarter of the armour
    int crit=(rnd8()*100>>8)<8+(la[LK_CLAWS]==3?10:0)+(uTr(a,TR_ACT)>>2);   // a CRITICAL HIT: one blow in twelve or so (BLADES and active Sims more) lands half as hard again
    if(crit){ dmg=dmg*3/2; hhSay(b,IC_BAIL,"CRIT"); }
    if(lb[LK_BACK]==1&&dmg>3){ fkLose(a,4); if(a==hhPUid&&lnoteT<=0){ lnote="OUCH  SPIKES"; lnoteT=40; } }   // spikes prick the one that hits them
    int m=hhMemOf(b);
    if(m<0){ fightHurt(dmg); return; }
    HhSim*t=&hhM[m]; if(t->hp>dmg){ t->hp=(u8)(t->hp-dmg);
        if(a==hhPUid&&lnoteT<=0){ char*e=fkB; if(crit) e=simCat(e,"CRIT "); e=simCat(e,fkMove(a,"PUNCH")); *e++=' '; simCatN(e,dmg); lnote=fkB; lnoteT=45; } }   // you see how hard it landed
    else { t->hp=30; t->act=HA_SOC; t->t=600; t->bub=IC_SKULL; t->bubT=120; hhNote(t," IS KNOCKED OUT"); if(a==hhPUid){ voxPlay(V_win_the_fight); copCrime(2); } }
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
        case SC_PROPOSE: voxPlay(ok?V_serenade_good:V_serenade_bad); break;             // popping the question: the serenade, good or bad (family.h)
        case SC_BABY: voxPlay(ok?V_yahoo:V_nah); break;                                  // yahoo: getting ready to woohoo
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
        hhSay(b,S->icR,i==SC_PUNCH?"OOF":i==SC_SLAP?"OW":i==SC_ARGUE?"GRR":i==SC_TEASE?"QUIT IT":"HEY");
        if(uGuest(b)&&(i==SC_SLAP||i==SC_PUNCH||i==SC_SHOVE)){ int k=b-GU0; if(twOn[k]==2) twWait[k]=60; }   // a guest who gets hit has had enough: home they go
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
        if(i==SC_PROPOSE){ int w=uGuest(b)?nrWedIn(b):b; if(w>=0){ b=w; famWed(a,b); } }   // a wedding (a neighbour who says yes moves in first: townrel.h)
        if(i==SC_BABY) famTry(a,b);      // maybe a baby on the way
        if(a==hhPUid||b==hhPUid){ simEvent(SE_TALK); if(i==SC_JOKE) simEvent(SE_LAUGH); if(i==SC_HUG) simEvent(SE_HUGGED); moodEvent(M_WANT); }
        hhSay(b,S->icA,i==SC_JOKE?"HA HA":i==SC_HUG||i==SC_KISS?"AWW":i==SC_STEADY||i==SC_PROPOSE?"YES":i==SC_PASS?"NICE":"YEAH");
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
    for(int a=0;a<RU_N;a++)for(int b=0;b<RU_N;b++){ int d=relD[a][b], l=relL[a][b]; if(d>l) relD[a][b]--; else if(d<l) relD[a][b]++; }   // (the guests' rows too)
}
static int guestHere(int u){ int k=u-GU0; return uGuest(u)&&twHas[k]&&twOn[k]==2&&TW_V(k)>=hhN&&!curFl&&hhM[TW_V(k)].uid==u; }   // guest u is staying on the lot (not walking in or off)
// ---- free will socials ----
static int uTile(int u,int*x,int*y){ int m=hhMemOf(u); if(m<0){ *x=(int)(lfx>>8); *y=(int)(lfy>>8); return 1; } *x=(int)(hhM[m].fx>>8); *y=(int)(hhM[m].fy>>8); return hhM[m].act!=HA_USE&&hhM[m].act<HA_LEAVE; }
static void hhSeek(HhSim*s){   // pick someone to go and see: friends most, enemies when grouchy, anyone when lonely enough
    int best=-1, bs=-999, me=s->uid;
    for(int u=0;u<RU_N;u++){ if(u==me) continue; if(uGuest(u)?!guestHere(u):(u!=hhPUid&&hhMemOf(u)<0)) continue;   // (the neighbours staying over too)
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
    if(d<-30||(nice<=2&&r<40)){ if(d<-30&&socAllowed(a,b,SC_PUNCH)&&r<50) return SC_PUNCH; if(d<=-20&&socAllowed(a,b,SC_SLAP)&&r<70) return SC_SLAP;   // (anyone can be mean to anyone now; free will still only hits when it is bad between them)
        if(d<=-20&&socAllowed(a,b,SC_SHOVE)&&r<110) return SC_SHOVE; return (r&2)?SC_TEASE:(r&1)?SC_ARGUE:SC_INSULT; }
    if(d<-5&&nice>=6&&socAllowed(a,b,SC_SORRY)) return SC_SORRY;
    if(socAllowed(a,b,SC_PROPOSE)&&!uGuest(b)&&r<40) return SC_PROPOSE;   // (a free-will Sim never tries for a baby: that is yours to choose; nor proposes to a guest)
    if(socAllowed(a,b,SC_STEADY)&&r<90) return SC_STEADY;
    if(socAllowed(a,b,SC_KISS)&&r<120) return SC_KISS;
    if(socAllowed(a,b,SC_FLIRT)&&uTr(a,TR_OUT)>=5&&r<70) return SC_FLIRT;
    int pool[8], n=0;
    pool[n++]=SC_TALK; if(socAllowed(a,b,SC_JOKE)&&uTr(a,TR_PLAY)>=4) pool[n++]=SC_JOKE; if(socAllowed(a,b,SC_COMPL)&&nice>=5) pool[n++]=SC_COMPL;
    if(socAllowed(a,b,SC_PASS)&&simMin>=16*60+20&&simMin<17*60+20) return SC_PASS;   // 4:20
    if(socAllowed(a,b,SC_PASS)&&uTr(a,TR_PLAY)>=4) pool[n++]=SC_PASS;
    if(socAllowed(a,b,SC_HIGH5)) pool[n++]=SC_HIGH5;
    if(socAllowed(a,b,SC_HUG)&&nice>=4) pool[n++]=SC_HUG;
    if(socAllowed(a,b,SC_TRICK)&&uTr(a,TR_ACT)>=5) pool[n++]=SC_TRICK;
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
static int hhVisitorHere(int m){ if(m<hhN||curFl) return 0; int k=HH_MAX-1-m; return k>=0&&k<TW_N&&twHas[k]&&twOn[k]; }   // slot m (past the household) holds a neighbour on the lot
static int hhNearest(void){ if(xo[XO_MULTIFL]?pkHome>=0:curFl) return -1; int best=-1, bd=1<<30; for(int m=0;m<HH_MAX;m++){ if(m<hhN){ if(hhM[m].act==HA_AWAY) continue; } else if(!hhVisitorHere(m)) continue; s32 dx=hhM[m].fx-lfx, dy=hhM[m].fy-lfy; int d=(int)((dx*dx+dy*dy)>>8); if(d<bd&&d<=(380*380>>8)&&wallClear(lfx,lfy,hhM[m].fx,hhM[m].fy)){ bd=d; best=m; } } return best; }   // within 1.5 tiles (household Sims and the neighbours who drop by), never through a wall
static void liveInvalidate(void);
static int hhVisitorTalk(int m,int useLabel){   // R next to a neighbour who dropped by (or an inmate): TALK / JOKE / COMPLIMENT / HIGH FIVE, and the mean ones: TEASE / INSULT, SHOVE / SLAP for teens and up (needs and mood only: they are not in the relationship tables)
    HhSim*s=m>=HH_MAX?&inmS[m-HH_MAX]:&hhM[m];
    static const char* it[10] EWRAM_BSS; static char tl[32] EWRAM_BSS, nt[28] EWRAM_BSS; int id[10], cat[10], n=0;
    static const char* const useNm[6]={0,"USE THE FRIDGE","USE THE TOILET","SLEEP IN BED","TAKE A SHOWER","SIT ON SOFA"}; static const char* const useNm2[5]={"WATCH TV","READ A BOOK","MAKE COFFEE","FEED THE FISH","RUN ON TREADMILL"};
    static const char* const vn[8]={"TALK","JOKE","COMPLIMENT","HIGH FIVE","TEASE","INSULT","SHOVE","SLAP"}; static const u8 vcat[8]={0,1,0,0,3,3,3,3};
    if((useLabel>0&&useLabel<6)||useLabel>=8){ it[n]=useLabel==8?"USE THE PHONE":useLabel>=11?useNm2[useLabel-11]:useLabel>=9?"TUNE THE RADIO":useNm[useLabel]; cat[n]=4; id[n++]=-1; }
    int big=stage>=AG_TEEN&&s->stage>=AG_TEEN;   // (shoving and slapping: teens and up, both of you)
    for(int i=0;i<8;i++) if(i<6||big){ it[n]=vn[i]; cat[n]=vcat[i]; id[n++]=i; }
    { char*e=simCat(tl,s->name); simCat(e,m>=HH_MAX?"  INMATE":"  NEIGHBOR"); }
    int c=pieCats(tl,it,cat,n); liveInvalidate();
    while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]<0) return 0;
    int px=(int)(lfx>>8), py=(int)(lfy>>8), sx=(int)(s->fx>>8), sy=(int)(s->fy>>8);
    s->hd=(u8)(px>sx?0:px<sx?8:py>sy?4:12); lhd=(s->hd+8)&15;
    int i=id[c], gain=i==1?10:i==0?8:6;
    if(i>=4){   // a mean one: it lands. They are hurt or angry; a neighbour shoved or slapped goes home, an inmate may hit back
        static const char* const said[4]={" GOT TEASED"," LOOKS HURT"," GOT SHOVED"," GOT SLAPPED"};
        hhSay(hhPUid,IC_ANGRY,i==4?"NYAH NYAH":i==5?"LOSER":i==6?"MOVE IT":"SMACK"); hhFreeze(hhPUid,60);
        s->bub=(u8)(i>=6?IC_HURT:i==5?IC_SAD:IC_ANGRY); s->bubT=90;
        needAdd(hhPUid,HN_SOC,2); if(pTr[TR_NICE]<=3) needAdd(hhPUid,HN_FUN,6);   // grouchy Sims enjoy it a little
        voxPlay(i>=6?V_lets_fight:V_amgry); simEvent(SE_FIGHT);
        { char*e=simCat(nt,s->name); simCat(e,said[i-4]); } lnote=nt; lnoteT=60;
        if(i>=6){ if(m>=HH_MAX){ if((rnd8()&1)||i==7){ fightHurt(6+(rnd8()&7)); simCat(simCat(nt,s->name)," HIT BACK"); } }   // an inmate does not take it
                  else { int k=HH_MAX-1-m; if(k>=0&&k<TW_N&&twWait[k]>40) twWait[k]=40; } }                                         // a neighbour has had enough: home they go
        return 1; }
    needAdd(hhPUid,HN_SOC,gain); if(i==1) needAdd(hhPUid,HN_FUN,8);
    simEvent(SE_TALK); if(i==1) simEvent(SE_LAUGH); moodEvent(M_WANT);
    voxPlay(i==3?V_yeha:i==1?V_joke_good:V_agree);
    { char*e=simCat(nt,s->name); simCat(e,i==1?" LAUGHED":i==2?" SAYS THANKS":i==3?" HIGH FIVES":" CHATTED"); }
    lnote=nt; lnoteT=60;
    return 1;
}
static int hhSocR(int useLabel){   // 1 = handled (a social, or the menu was closed), 0 = go on and use the furniture
    { int q=inmNear(); if(q>=0) return hhVisitorTalk(HH_MAX+q,useLabel); }   // an inmate (inmates.h)
    int m=hhNearest(); if(m<0) return 0;
    if(m>=hhN){ int k=HH_MAX-1-m; if(k<0||k>=TW_N||hhM[m].uid!=GU0+k) return hhVisitorTalk(m,useLabel); }   // a neighbour is a GUEST (their own uid, every social); only one we cannot name gets the small menu
    HhSim*s=&hhM[m]; int b=s->uid, a=hhPUid;
    if(s->act==HA_USE){ lnote="THEY ARE BUSY"; lnoteT=50; return 0; }
    static const char* it[SC_N+4] EWRAM_BSS; static char tl[40] EWRAM_BSS; int id[SC_N+4], cat[SC_N+4], n=0;   // (cat: the pie's category, 0 FRIENDLY 1 FUN 2 ROMANTIC 3 MEAN 4 USE)
    static const char* const useNm[6]={0,"USE THE FRIDGE","USE THE TOILET","SLEEP IN BED","TAKE A SHOWER","SIT ON SOFA"}; static const char* const useNm2[5]={"WATCH TV","READ A BOOK","MAKE COFFEE","FEED THE FISH","RUN ON TREADMILL"};
    static const u8 socCat[SC_N]={0,1,0,0,0,1,2,2,2,0,3,3,3,3,1,2,2,3,3};   // TALK JOKE COMPL HIGH5 HUG TRICK FLIRT KISS STEADY SORRY ARGUE INSULT SLAP PUNCH PASS PROPOSE BABY TEASE SHOVE
    if((useLabel>0&&useLabel<6)||useLabel>=8){ it[n]=useLabel==8?"USE THE PHONE":useLabel>=11?useNm2[useLabel-11]:useLabel>=9?"TUNE THE RADIO":useNm[useLabel]; cat[n]=4; id[n++]=-1; }
    it[n]="YOUR ACTIONS"; cat[n]=4; id[n++]=-2;   // self.h: the pie of what you can do on your own
    for(int i=0;i<SC_N;i++) if(socAllowed(a,b,i)){ it[n]=i==SC_PUNCH?fkMove(a,"PUNCH"):socT[i].name; cat[n]=socCat[i]; id[n++]=i; }
    if(uGuest(b)){   // a neighbour: they can be asked to live here, or sent home (townrel.h)
        if(nrCanAsk(a,b)){ it[n]="ASK TO MOVE IN"; cat[n]=0; id[n++]=-3; }
        it[n]="SAY GOODBYE"; cat[n]=0; id[n++]=-4; }
    { char*e=simCat(tl,s->name); *e++=' '; *e++=' '; e=simCat(e,relWord(a,b)); e=simCat(e,"  HP "); simCatN(e,s->hp); }
    if(!n){ lnote="NOTHING TO DO HERE"; lnoteT=50; return 1; }
    pieFaceU=(u8)(b+1); int c=pieCats(tl,it,cat,n); pieFaceU=0; liveInvalidate();   // the PIE MENU (pie.h): the Sims way, their face in the hub
    while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]==-2) return slfMenu();   // self.h
    if(id[c]==-3){ nrAskIn(a,b); return 1; }
    if(id[c]==-4){ nrBye(b); return 1; }
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
static int pplCount(void); static void pplRow(int r,int y,int b1,int b2,int hx);   // townrel.h: the household, then the town Sims you know
static void relScreen(void){
    u16 prev=keyNow(); int top=0, n=pplCount();   // seven rows fit: UP / DOWN scroll a bigger list
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; if(pr&(K_A|K_B|K_START)) return;
        if((pr&K_DOWN)&&top+7<n) top++;
        if((pr&K_UP)&&top>0) top--;
        if((pr&K_SEL)&&dbgOn){ hhInvite(); prev=keyNow(); n=pplCount(); }   // SELECT: a new Sim moves in (DEBUG CODE only)
        box(3,1,234,157); char t[44]; { char*e=simCat(simCat(t,"RELATIONSHIPS OF "),hhPName); if(hhPLast[0]){ *e++=' '; simCat(e,hhPLast); } } text(10,6,t,GOLD,1);
        text(84,16,"YOU TO THEM",DIMC,1); text(162,16,"THEM TO YOU",DIMC,1);
        if(!n){ text(10,40,"NO ONE ELSE LIVES HERE",DIMC,1); text(10,52,"TALK TO THE NEIGHBORS WHO DROP BY",DIMC,1); text(10,62,"OR CALL SOMEONE OVER ON THE PHONE",DIMC,1); }
        for(int r=top;r<n&&r<top+7;r++) pplRow(r,26+(r-top)*18,84,162,226);
        text(10,150,dbgOn?(n>7?"UP DOWN MORE  SELECT ADD A SIM":"TOP DAILY  LOW LIFETIME  SELECT ADD A SIM"):(n>7?"UP DOWN MORE  BLUE LIVE IN TOWN":"TOP DAILY  LOW LIFETIME  BLUE LIVE IN TOWN"),RGB(12,14,16),1);
        present();
    }
}
// ---- drawing: like the player, inside drawRoomRect's back-to-front walk. hhCalc (once a picture) works out where everyone is ----
static int hhX[HH_MAX], hhY[HH_MAX], hhB[HH_MAX], hhV[HH_MAX], hhH[HH_MAX];   // feet on screen, band (tile x+y), view, floor height
static int hhLook(int m){   // ALIVE tier 1: an idle member now and then glances a quarter turn one way or the other (the four views are already baked). 0 = looks where they face
    if(m>=hhN||hhM[m].act!=HA_IDLE) return 0;
    unsigned g=((((unsigned)(lfr>>7))+(unsigned)m*7u)*2654435761u)>>29;   // a new glance every 128 steps, different for each member
    return g==0?4:g==1?12:0;   // (heading units: 4 and 12 are the two quarter turns)
}
static void hhCalc(void){   // (the places above hhN that hold a visitor too)
    for(int m=0;m<HH_MAX;m++){ if(m>=hhN){ int k=HH_MAX-1-m; if(k>=TW_N||!twHas[k]||!twOn[k]||curFl) continue; } const HhSim*s=&hhM[m]; s32 rx,ry; rotPos(s->fx,s->fy,&rx,&ry);
        hhX[m]=LOX+(int)((rx-ry)>>5); hhY[m]=LOY+(int)((rx+ry)>>6); hhB[m]=(int)((rx>>8)+(ry>>8)); hhV[m]=faceView[(s->hd+hhLook(m)+4*cview)&15]; hhH[m]=surfH(s->fx,s->fy); }
}
// FOE HEALTH BAR (fight.h sets it): a small bar floats over the Sim you last hit for about 2.5 s. Green / yellow / red like the HUD; a pale chunk
// (the ghost) holds where the health was and drains away, so you can see what the last blow took. A knocked out Sim shows an empty bar. 4 bytes of EWRAM.
#define HB_W 18      // inner width in pixels
#define HB_UP 44     // the bar sits this far above the Sim's feet (a balloon moves up by HB_BAL to make room)
#define HB_BAL 7
static u8 hhBarT EWRAM_BSS, hhBarM EWRAM_BSS, hhBarG EWRAM_BSS, hhBarH EWRAM_BSS;   // steps left, whose bar (member), ghost health, steps the ghost holds
static int hhBarOn(int m){ return hhBarT&&hhBarM==m&&m<hhN; }
static int hhBarHp(int m){ return (hhM[m].act==HA_SOC&&hhM[m].t>=500)?0:hhM[m].hp; }   // (a knocked out Sim is kept at 30 HP inside: show it empty)
static void hhBarHit(int m,int hp0){   // a blow landed on member m that had hp0 before it
    if(hhBarT&&hhBarM==m&&hhBarG>hp0) hp0=hhBarG;   // the same foe again: the ghost keeps the whole combo's damage
    hhBarM=(u8)m; hhBarG=(u8)hp0; hhBarH=24; hhBarT=150;
}
static void hhBarTick(void){
    if(!hhBarT) return;
    hhBarT--; if(hhBarM>=hhN){ hhBarT=0; return; }
    int cur=hhBarHp(hhBarM);
    if(hhBarG<cur) hhBarG=(u8)cur;
    else if(hhBarH) hhBarH--;
    else if(hhBarG>cur){ int g=hhBarG-2; hhBarG=(u8)(g<cur?cur:g); }
}
static void hhBarDraw(int m){
    int x=hhX[m]-HB_W/2-1, y=hhY[m]-hhH[m]-HB_UP, cur=hhBarHp(m), w=(cur*46)>>8, g=(hhBarG*46)>>8; if(cur>0&&w<1) w=1;
    u16 col=cur>=55?RGB(9,27,8):cur>=28?RGB(29,25,5):RGB(30,7,6);
    rect(x,y,HB_W+2,5,RGB(3,3,6)); rect(x+1,y+1,HB_W,3,RGB(9,4,4));
    if(g>w) rect(x+1+w,y+1,g-w,3,RGB(31,30,24));
    if(w>0){ rect(x+1,y+1,w,3,col); rect(x+1,y+1,w,1,(u16)(col|0x2108)); }
}
static void hhDrawBand(int s0,int s1){   // the members whose band is in s0..s1
    for(int m=0;m<hhN;m++){ if(hhB[m]<s0||hhB[m]>s1||hhM[m].act==HA_AWAY) continue;
        if(sShad) rect(hhX[m]-3,hhY[m]-hhH[m]-1,7,2,RGB(10,8,5));   // (the Sim itself is a hardware sprite: hhObjUpdate)
        if(hhBarOn(m)) hhBarDraw(m);
        if(hhM[m].bubT){ int bx=hhX[m]-5, by=hhY[m]-hhH[m]-50-(hhBarOn(m)?HB_BAL:0); rect(bx,by,11,10,RGB(14,16,22)); rect(bx+1,by+1,9,8,WHITE);   // a balloon with an icon (Sims style)
            simIcon(bx+2,by+1,hhM[m].bub,hhM[m].bub==IC_HEART?RGB(28,6,12):hhM[m].bub==IC_ANGRY||hhM[m].bub==IC_HURT?RGB(26,4,4):RGB(4,4,10)); px(hhX[m],by+10,RGB(14,16,22)); } }
}
typedef struct { short x0,y0,x1,y1; } HhR;   // (hud.h's Rc comes later in main.c)
static void hhRc(int m,HhR*r){   // what a member puts INTO the picture: only its shadow (and a balloon); the body is a hardware sprite
    if(hhM[m].act==HA_AWAY){ r->x0=r->x1=r->y0=r->y1=0; return; }   // off the lot: nothing
    r->x0=(short)(hhX[m]-3); r->x1=(short)(hhX[m]+4); r->y0=(short)(hhY[m]-hhH[m]-1); r->y1=(short)(hhY[m]-hhH[m]+1);
    if(hhM[m].bubT){ int by=hhY[m]-hhH[m]-50-(hhBarOn(m)?HB_BAL:0); if(r->y0>by) r->y0=(short)by; if(r->x0>hhX[m]-5) r->x0=(short)(hhX[m]-5); if(r->x1<hhX[m]+6) r->x1=(short)(hhX[m]+6); }
    if(hhBarOn(m)){ int by=hhY[m]-hhH[m]-HB_UP; if(r->y0>by) r->y0=(short)by; if(r->x0>hhX[m]-HB_W/2-1) r->x0=(short)(hhX[m]-HB_W/2-1); if(r->x1<hhX[m]+HB_W/2+2) r->x1=(short)(hhX[m]+HB_W/2+2); } }
static unsigned hhSig(int m){ return (((unsigned)(hhX[m]&0x3FF)|((unsigned)(hhY[m]&0x3FF)<<10)|((unsigned)(hhH[m]&15)<<22)|((unsigned)(hhM[m].bubT?1+(hhM[m].bub&31):0)<<26))^(hhM[m].act==HA_AWAY?0x80000000u:0))
    +(hhBarOn(m)?((unsigned)(hhBarHp(m)+1)+(unsigned)hhBarG*257u)*0x9E3779B1u:0u); }   // (the bar adds its own health and ghost, so any change redraws it)
static int hhBehindAt(s32 fx,s32 fy){   // is a full-height wall in front of this spot (towards the camera)? then a Sim there is drawn see-through
    s32 rx,ry; rotPos(fx,fy,&rx,&ry); int x=(int)(rx>>8), y=(int)(ry>>8);
    static const signed char d[5][2]={{1,0},{0,1},{1,1},{2,1},{1,2}};
    for(int k=0;k<5;k++){ int wx=x+d[k][0], wy=y+d[k][1]; if(!wallAtR(wx,wy)||(cellAt(wx,wy)!='W'&&!isWinCh(cellAt(wx,wy)))||sWall==2) continue;
        if(sWall==1&&(wInAt(wx,wy-1)||wInAt(wx-1,wy))) continue;   // that wall is cut away
        return 1; }
    for(int k=0;k<3;k++){ int wx=x+d[k][0], wy=y+d[k][1]; if(wx<0||wy<0||wx>=MW||wy>=MH) continue; char c=cellAt(wx,wy); if(c=='F'||c=='H'||c=='#') return 1; }   // tall furniture right in front (a fridge, a shower)
    return 0;
}
static int hhPlayerFront(int x,int y){   // is the player (drawn into the picture, so under every sprite) standing in front of this Sim's sprite?
    s32 rx,ry; rotPos(lfx,lfy,&rx,&ry); int px=LOX+(int)((rx-ry)>>5), py=LOY+(int)((rx+ry)>>6), dx=px-(x+16);
    return py>y+SPF&&dx>-22&&dx<22&&py<y+SPF+44;
}
typedef struct { const HhSim*s; short x,y; int dd,dep; u8 id,key,pk; signed char tb,pb; } HhOv;   // a Sim in view: where its sprite goes, how far from the middle, how far back; key = body*8+view*2+frame, pk = its palette; tb, pb = the tile block and palette bank it got (-1: none yet)
// RUNNING LEGS: the stride is driven by real movement (world position since last frame), not only by the walking actions, so a Sim that is pushed,
// scared or sent running always moves its legs. Faster than 1.5 x walking speed = running: the legs swap every 4 frames instead of 8. Other code can force
// a run by setting hhRunT[id] (steps). Teleports (a jump of a tile or more in one frame) do not count.
static int hhLfx[HH_MAX], hhLfy[HH_MAX]; static u8 hhMvT[HH_MAX], hhRunT[HH_MAX];
static int hhLegs(int id,const HhSim*s){   // 0 standing, 1 walking, 2 running
    int dx=(int)(s->fx-hhLfx[id]), dy=(int)(s->fy-hhLfy[id]); hhLfx[id]=(int)s->fx; hhLfy[id]=(int)s->fy; if(dx<0) dx=-dx; if(dy<0) dy=-dy; int d=dx+dy;
    if(d>0&&d<200){ hhMvT[id]=8; if(d>F_WALK*3/2) hhRunT[id]=8; } else if(hhMvT[id]) hhMvT[id]--;
    if(hhRunT[id]&&d==0&&!hhMvT[id]) hhRunT[id]=0; else if(hhRunT[id]) hhRunT[id]--;
    return hhMvT[id]?(hhRunT[id]?2:1):0;
}
static void fxObjUpdate(void); static void hhObjUpdate0(void){   // in vblank: hand out OAM entries, tile blocks and palette banks, load what changed into OBJ VRAM, write OAM (a Sim is one 32x64 sprite), set the window that clips them
    volatile u16*oam=OAM; int i, nOam=0;
    *(volatile u16*)0x04000040=240; *(volatile u16*)0x04000044=(u16)((sbY0<<8)|sbY1);   // WIN0: the room view (the room rows of the screen)
    *(volatile u16*)0x04000048=0x34; *(volatile u16*)0x0400004A=0x04;                   // inside: BG2 + sprites + blend; outside: BG2 only
    *(volatile u16*)0x04000050=0x0400; *(volatile u16*)0x04000052=(6<<8)|10;            // see-through sprites blend 10/16 over the picture
    if(!hhSlotOk) hhSlotsFree();
    if(lcamF>0){ for(i=0;i<HH_OAM;i++) oam[i*4]=0x200; return; }   // the action cam: all off, the blocks and banks stay as they are
    if(xo[XO_MULTIFL]?pkHome>=0:curFl){ for(i=0;i<HH_OAM;i++) oam[i*4]=0x200; hhSlotsFree(); return; }   // upstairs: no Sim sprites
    // 1. who is in view
    HhOv w[HH_IDS]; int n=0, cx=SW/2, cy=(vpY0+vpY1)/2;
    for(int id=0;id<HH_IDS;id++){
        const HhSim*s; int x,y,v,f,dep;
        if(id>=HH_MAX){ int j=id-HH_MAX; if(!inmOn(j)) continue; s=&inmS[j]; x=inX[j]-16; y=inY[j]-SPF-inH[j]; v=inV[j]; dep=inB[j]; f=inmWalk(j); }   // an inmate
        else if(id>=hhN){ int k=HH_MAX-1-id; if(k>=TW_N||!twHas[k]||!twOn[k]||curFl) continue;   // a visitor
            s=&hhM[id]; x=hhX[id]-16; y=hhY[id]-SPF-hhH[id]; v=hhV[id]; dep=hhB[id];
            f=twOn[k]!=2?((lfr+k*3)>>3)&1:0;   // walking in or out: stepping; staying: standing
        } else {
            if(hhM[id].act==HA_AWAY) continue;
            s=&hhM[id]; x=hhX[id]-16; y=hhY[id]-SPF-hhH[id]; v=hhV[id]; dep=hhB[id];
            int walk=(s->act==HA_WALK||s->act==HA_WANDER||s->act==HA_SEEK||s->act==HA_LEAVE||s->act==HA_STAIR)&&s->pi<s->pn;
            { int mv=hhLegs(id,s); f=(walk||mv)?((lfr+id*5)>>(mv==2?2:3))&1:0; }   // walking: standing / mid-stride, every 8 frames (running: every 4); each Sim a little out of step
        }
        if(x+32<=vpX0||x>=vpX1||y+SPH<=vpY0||y>=vpY1) continue;
        HhOv*o=&w[n++]; o->s=s; o->x=(short)x; o->y=(short)y; o->id=(u8)id; o->key=(u8)(inmSetOf(id)*8+v*2+f); o->pk=(u8)(id>=HH_MAX?32+(id-HH_MAX):id); o->dep=dep; o->tb=o->pb=-1;
        int dx=x+16-cx, dy=y+40-cy; o->dd=(dx<0?-dx:dx)+(dy<0?-dy:dy);
    }
    // 2. nearest first
    for(i=1;i<n;i++){ HhOv t=w[i]; int j=i; while(j>0&&w[j-1].dd>t.dd){ w[j]=w[j-1]; j--; } w[j]=t; }   // (insertion sort, n <= 31)
    // 3. which palette banks and tile blocks may be used this frame
    u16 allow=0x00FF; int nTb=8;
    if(prHere()){ allow|=(u16)(0xF700&~hhBanksBusy()); nTb=HH_TB; }   // the prison: the banks ghosts and cops are not drawing with are lent too (11 is the weather's)
    { u16 lost=(u16)(hhBorrow&~allow); if(lost){ for(int b=8;b<16;b++) if((lost>>b)&1) hhPkey[b]=-1; hhBorrow&=(u16)~lost; hhExtraDone(); } }   // a ghost or a cop wants its bank back: its art and colours are loaded again right after this (fx.h, npc.h)
    // 4. hand out banks and blocks, load what changed (a few fresh sprites per frame at most)
    u8 tcnt[HH_TB], pcnt[16]; for(i=0;i<HH_TB;i++) tcnt[i]=0; for(i=0;i<16;i++) pcnt[i]=0;
    int budget=UP_BUDGET;
    for(i=0;i<n;i++){ HhOv*o=&w[i]; int id=o->id, set=o->key>>3, v=(o->key>>1)&3, f=o->key&1, key=o->key, pk=o->pk, pb=-1, tb=-1, b;
        for(int a=0;a<2&&pb<0;a++){   // a palette bank: the one that already holds this palette, else a free one (a crowd that is too colourful falls back to the body's own colours)
            int kk=a?set:pk; if(a&&pk<32) break;
            for(b=0;b<16;b++) if(((allow>>b)&1)&&hhPkey[b]==kk){ pb=b; break; }
            if(pb<0) for(b=0;b<16;b++) if(((allow>>b)&1)&&!pcnt[b]){ pb=b; break; }
            if(pb>=0&&hhPkey[pb]!=kk){ const u16*pl=kk>=32?inmPalOf(kk-32):hhPal[kk]; for(int c=0;c<16;c++) OBJ_PAL[pb*16+c]=pl[c]; hhPkey[pb]=(signed char)kk; if(pb>=8) hhBorrow|=(u16)(1u<<pb); } }
        if(pb<0) continue;
        for(b=0;b<nTb;b++) if(hhTcur[b]==key){ tb=b; break; }   // a block that already holds exactly this (shared by every Sim that looks the same way)
        if(tb<0) for(b=0;b<nTb;b++) if(!tcnt[b]&&hhTcur[b]>=0&&(hhTcur[b]>>1)==(key>>1)){ tb=b; break; }   // the same view in the other stride: only the legs change
        if(tb<0&&hhIdBlk[id]>=0&&hhIdBlk[id]<nTb&&!tcnt[(int)hhIdBlk[id]]) tb=hhIdBlk[id];   // else the block this Sim had
        if(tb<0) for(b=0;b<nTb;b++) if(!tcnt[b]&&hhTcur[b]<0){ tb=b; break; }
        if(tb<0) for(b=0;b<nTb;b++) if(!tcnt[b]){ tb=b; break; }
        if(tb<0) continue;
        if(hhTcur[tb]!=key){
            int ov=hhTcur[tb], full=ov<0||(ov>>1)!=(key>>1);
            if(full&&budget<=0){ if(ov<0||(ov>>3)!=set) continue; }   // no time left: a new sprite appears next frame, one that only turned keeps its old view for a frame or two
            else { if(full) budget--; hhTcur[tb]=(signed char)key; hhUpTiles(OBJ_VRAM+hhTvSlot[tb]*(OBJ_B/2),set,v,f,full?0:STR_T0); } }   // the whole view, or only the tiles the walking frame can change
        tcnt[tb]++; pcnt[pb]++; hhIdBlk[id]=(signed char)tb; o->tb=(signed char)tb; o->pb=(signed char)pb;
    }
    // 5. depth order: the Sim nearest the camera gets the lowest OAM entry, so it is drawn over the ones behind it
    int ord[HH_IDS], no=0;
    for(i=0;i<n;i++) if(w[i].tb>=0) ord[no++]=i;
    for(i=1;i<no;i++){ int t=ord[i], j=i; while(j>0&&w[ord[j-1]].dep<w[t].dep){ ord[j]=ord[j-1]; j--; } ord[j]=t; }
    // 6. OAM
    for(int r=0;r<no;r++){ const HhOv*o=&w[ord[r]];
        volatile u16*e=oam+nOam*4; int tile=512+hhTvSlot[(int)o->tb]*32, pal=o->pb, y=o->y, x=o->x, blend=(hhBehindAt(o->s->fx,o->s->fy)||hhPlayerFront(x,y))?0x400:0;
        if(zoomDma){   // ZOOM: scaled up by the hardware (affine, double size: a 64 x 128 box, matrix 0) and placed where its room pixels are on screen
            int X=(x+16-vpX0)*zoomNum/zoomDen-32, Y=sbY0+(y+32-vpY0)*zoomNum/zoomDen-64;
            e[0]=(u16)((Y&255)|blend|0x8000|0x300); e[1]=(u16)((X&511)|0xC000); e[2]=(u16)(tile|(pal<<12));
        } else { e[0]=(u16)((y&255)|blend|0x8000); e[1]=(u16)((x&511)|0xC000); e[2]=(u16)(tile|(pal<<12)); }   // one tall 32 x 64 sprite
        nOam++;
    }
    for(i=nOam;i<HH_OAM;i++) oam[i*4]=0x200;   // everything else off
    oam[3]=zoomPa; oam[7]=0; oam[11]=0; oam[15]=zoomPa;   // affine matrix 0 (the ZOOM's sprites): 1 / scale
}
static void hhObjUpdate(void){ hhObjUpdate0(); fxObjUpdate(); }   // fx.h: the ghosts and the weather are sprites too
static HhR hhOld[HH_MAX] EWRAM_BSS; static unsigned hhOldSig[HH_MAX] EWRAM_BSS;
static void hhSave(void);
static void stUidMap(const u8*nu,int n);   // story.h: the story's partner and kid uids follow a household load that gave the uids out again
static void hhInvite(void){   // a made-up Sim moves in (pause menu > HOUSEHOLD, or SELECT on the RELATIONSHIPS screen)
    if(!xo[XO_SIMRAND]){ toast("MADE-UP SIMS ARE OFF"); return; }
    if(hhN>=HH_MAX){ toast("THE HOUSE IS FULL"); return; }
    u8 lk[LK_N], st, tr[TR_N]; hhRandLook(lk,&st); for(int i=0;i<TR_N;i++) tr[i]=(u8)(rnd8()%11);
    int m=hhAdd(lk,st,rnd8()%AS_PICK,rnd8()&1,tr); if(m<0) return;
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave();
    static char t[32] EWRAM_BSS; char*e=simCat(t,hhM[m].name); simCat(e," MOVED IN"); toast(t);
}

static void lookTrueRandom(u8*lk,u8*stg);   // main.c: every slider, part and colour at random
static void hhInviteTrue(void){   // a truly random Sim moves in: any age from child to elder, every choice of the creator at random
    if(!xo[XO_SIMRAND]){ toast("MADE-UP SIMS ARE OFF"); return; }
    if(hhN>=HH_MAX){ toast("THE HOUSE IS FULL"); return; }
    u8 lk[LK_N], st=255, tr[TR_N]; lookTrueRandom(lk,&st); for(int i=0;i<TR_N;i++) tr[i]=(u8)(rnd8()%11);
    int m=hhAdd(lk,st,st<AG_ADULT?AS_GROW:rnd8()%AS_PICK,rnd8()&1,tr); if(m<0) return;
    for(int k=0;k<hhN;k++){ hhOld[k].x0=hhOld[k].x1=0; hhOldSig[k]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave();
    static char t[32] EWRAM_BSS; char*e=simCat(t,hhM[m].name); simCat(e," MOVED IN"); toast(t);
}
static void hhLoad(void);
static void hhStart(void){   // entering the life game: load the household and stand everyone somewhere free
    for(int m=0;m<HH_MAX;m++){ hhFl[m]=0; hhUp[m]=0; } hhCenT=0;   // floors step 2: everyone starts on the ground floor
    hhLoad(); hhSlotsFree(); hhFindExits(); for(int k=0;k<TW_N;k++){ twOn[k]=0; twWait[k]=(short)(240+k*700); }
    for(int m=0;m<hhN;m++){ hhPlace(&hhM[m],m); hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
    if(xo[XO_MULTIFL]&&!curFl) for(int m=0;m<hhN;m++) if(hhFlLd[m]>0&&hhFlLd[m]<FLR_N){ hhFl[m]=hhFlLd[m]; hhM[m].act=HA_AWAY; hhM[m].use=HN_FUN; hhUp[m]=(u16)(300+(rnd8()<<2)); }   // floors step 7: whoever was upstairs when you saved still is
}
// ---- switching who you control ----
static void hhSwap(HhSim*s);   // main.c: trades the player's position, needs, look and persona with s
static void hhSwitchFrom(int f);
static int hhOnOtherFloor(int m){ return xo[XO_MULTIFL]&&hhM[m].act==HA_AWAY&&hhUp[m]>0&&hhFl[m]<FLR_N&&hhFl[m]!=curFl&&!prHeld(&hhM[m]); }   // floors fix: parked on another floor (not out at work): you can still switch to them, you go to their floor
static int hhCallFloors(void){   // floors step 9: R next to the stairs with nothing else in reach shouts for the household: every Sim parked on another floor comes to yours and steps out of the stairs (1 = handled)
    if(!xo[XO_MULTIFL]||!hhN) return 0;
    int st=0; for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){ int tx=(int)(lfx>>8)+dx, ty=(int)(lfy>>8)+dy; if(tx<0||ty<0||tx>=MW||ty>=MH) continue; char c=lifeMap[ty][tx]; if(c=='^'||c=='~') st=1; }
    if(!st) return 0;
    int n=0; for(int m=0;m<hhN;m++) if(hhOnOtherFloor(m)){ hhFl[m]=(u8)curFl; hhUp[m]=2; n++; }   // (hhTick sees a parked Sim whose floor is now yours and places it at the stairs)
    lstun=20; lsp=0; lgrind=0; lnote=n?(n==1?"CALLED SOMEONE DOWN":"CALLED THE HOUSEHOLD"):"NOBODY ON OTHER FLOORS"; lnoteT=70; return 1;
}
static void hhSwitch(void){   // SELECT: control the next Sim of the household who is at home
    if(!hhN) return;
    int f=0; while(f<hhN&&hhM[f].act==HA_AWAY) f++;
    if(f>=hhN){ f=0; while(f<hhN&&!hhOnOtherFloor(f)) f++; }   // nobody else on this floor: a Sim parked on another floor (you go to them)
    if(f>=hhN){ lnote="EVERYONE IS OUT"; lnoteT=90; return; }
    hhSwitchFrom(f);
}
static void hhSwitchTo(int m){   // pause menu > HOUSEHOLD > SWITCH TO A SIM: control member m (the ones before it go to the back of the line, as SELECT does)
    if(m<0||m>=hhN) return;
    if(prSwitchHook(m)) return;   // prison.h: between the cell and home
    if(hhM[m].act==HA_AWAY&&!hhOnOtherFloor(m)){ toast("THEY ARE OUT RIGHT NOW"); return; }
    if(custom){ toast("HAND BUILT SIMS CANNOT SWITCH"); return; }
    hhSwitchFrom(m);
}
static void hhSwitchFrom(int f){   // the first f members go to the back of the line, then you trade places with the one in front
    int swX=-1, swY=-1, swFrom; if(pkHome>=0) peekEnd(); swFrom=curFl;   // (looking at another floor: back on your own first)
    if(f>=0&&f<hhN&&hhOnOtherFloor(f)){   // floors fix: switching to a Sim parked on another floor. You go to their floor first (nothing changes if it will not fit) and arrive at the stairs, as when you climb them
        int tf=hhFl[f], tx=(int)(lfx>>8), ty=(int)(lfy>>8), fd=-1; char from=tf>curFl?'^':'~', want=tf>curFl?'~':'^';
        for(int y=0;y<MH&&fd<0;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]==from){ tx=x; ty=y; fd=y; break; }
        if(!flGo(tf)){ toast("TOO MUCH BUILT TO CHANGE FLOOR"); return; }
        swX=tx; swY=ty; fd=-1;
        for(int y=0;y<MH&&fd<0;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]==want){ swX=x; swY=y; fd=y; break; }
        if(lifeMap[swY][swX]!=want){ lifeMap[swY][swX]=want; wDirty=1; }   // no stairs there yet: they appear where you came from (as flStairs does)
    }
    while(f-->0){   // (with their sprites: only the descriptors move, the pool bytes stay where they are)
        HhSim t=hhM[0]; HhSpr s0=hhSp[0]; u16 p1[16];
        for(int i=0;i<16;i++) p1[i]=hhPal[0][i];
        for(int m=0;m<hhN-1;m++){ hhM[m]=hhM[m+1]; hhSp[m]=hhSp[m+1]; for(int i=0;i<16;i++) hhPal[m][i]=hhPal[m+1][i]; }
        hhM[hhN-1]=t; hhSp[hhN-1]=s0; { u8 a=hhFl[0]; u16 b=hhUp[0]; for(int m=0;m<hhN-1;m++){ hhFl[m]=hhFl[m+1]; hhUp[m]=hhUp[m+1]; } hhFl[hhN-1]=a; hhUp[hhN-1]=b; } for(int i=0;i<16;i++) hhPal[hhN-1][i]=p1[i];
    }
    HhSim t=hhM[0]; for(int m=0;m<hhN-1;m++) hhM[m]=hhM[m+1];   // the player goes to the back of the line, the first member steps in
    hhSwap(&t); hhM[hhN-1]=t; for(int m=0;m<hhN-1;m++){ hhFl[m]=hhFl[m+1]; hhUp[m]=hhUp[m+1]; } hhFl[hhN-1]=xo[XO_MULTIFL]?(u8)swFrom:0; hhUp[hhN-1]=0;
    u16 pl[16];
    hhQuant(spr4,HH_MAX,pl);               // the one you leave: down to a hardware sprite (in the spare descriptor: the member stepping in is still in the pool)
    hhQuantS(spr4s,HH_MAX,pl);
    hhUnquant(0,hhPal[0],spr4);            // the member you take over: back to a full sprite
    for(int v=0;v<4;v++)for(int i=0;i<SPW*SPH;i++) spr4s[v][i]=spr4[v][i];
    hhUnquantS(0,hhPal[0],spr4s);
    hhSprFree(0);                          // (their block is no longer needed)
    for(int m=0;m<hhN-1;m++){ hhSp[m]=hhSp[m+1]; for(int i=0;i<16;i++) hhPal[m][i]=hhPal[m+1][i]; }
    hhSp[hhN-1]=hhSp[HH_MAX]; for(int i=0;i<16;i++) hhPal[hhN-1][i]=pl[i];
    { HhSpr*z=&hhSp[HH_MAX]; for(int v=0;v<4;v++) z->sm[v]=z->dm[v]=0; z->off=0; z->len=0; }   // the spare is empty again
    spBounds();
    for(int m=0;m<HH_MAX;m++) hhKey[m]=0;
    sprKey=0;   // sprites moved around and the player's came from a hardware sprite: bake them again next time
    if(swX>=0){ lfx=swX*256+128; lfy=swY*256+128; lz=lvz=0; flArm=0; lnote=flNm[curFl]; lnoteT=60; camSnap=1; liveInvalidate(); }   // floors fix: you arrive at the stairs of the Sim's floor
    hhSlotsFree();
}

// ---- saving (SRAM at HH_OFF): 'H' '2' count, your uid, then per member its look, stage, persona, name, needs and uid, then the
// relationships (daily, lifetime, flags for every pair of uids); checksum last ----
#define HH_REC (LKPK+3+TR_N+2*HH_NM+HN_N+1)   // ('H6' and older held a 10-byte first name and no last name)
static int hhPlaceOf(int u){ if(u==hhPUid) return 0; for(int i=0;i<hhN;i++) if(hhM[i].uid==u) return i+1; return -1; }   // a uid as the family and town tails keep it (0 you, 1.. members by place)
#define HH_RELB (4*HU_N*HU_N)   // daily, lifetime, flags and (from 'H?') kin, for every pair of uids
static void hhSave(void){
    volatile u8*m=SRAM_BASE+HH_OFF; int k=3; u8 sum=0x48;
    m[0]='H'; m[1]='A'; m[2]=(u8)hhN; m[k++]=(u8)hhPUid;   // 'H:' = 'H9' for 8 Sims (8 uids); 'H;' adds the format 11 sliders; 'H?' adds the kin (who is whose mother, sister, roommate); 'H@' adds GENDER and the family tail; 'HA' the town relationships
    for(int j=0;j<HH_NM;j++) m[k++]=(u8)hhPName[j];
    for(int j=0;j<HH_NM;j++) m[k++]=(u8)hhPLast[j];   // your own name
    for(int i=0;i<hhN;i++){ const HhSim*s=&hhM[i];
        for(int j=0;j<LK_N;j++) if(!lkSlide(j)) m[k++]=s->look[j];   // 'H9': the picks as bytes, then the sliders (9 values) two to a byte
        { int h=-1; for(int j=0;j<LK_N;j++) if(lkSlide(j)){ int v=s->look[j]&15; if(h<0) h=v; else { m[k++]=(u8)(h|(v<<4)); h=-1; } } if(h>=0) m[k++]=(u8)h; }
        m[k++]=(u8)(s->stage|((xo[XO_MULTIFL]&&hhFl[i]<FLR_N?hhFl[i]:0)<<6)); m[k++]=s->asp; m[k++]=s->ltw;
        for(int j=0;j<TR_N;j++) m[k++]=s->tr[j];
        for(int j=0;j<HH_NM;j++) m[k++]=(u8)s->name[j];
        for(int j=0;j<HH_NM;j++) m[k++]=(u8)s->last[j];
        for(int j=0;j<HN_N;j++) m[k++]=s->need[j];
        m[k++]=s->uid; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ m[k++]=(u8)relD[a][b]; m[k++]=(u8)relL[a][b]; m[k++]=relF[a][b]; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++) m[k++]=kin[a][b];
    { int pl[2]={famPa,famPb};   // 'H@': the family tail (family.h): each member's days in their stage, the days until the baby, its parents (0 you, 1.. members by place, 255 nobody)
      for(int i=0;i<hhN;i++) m[k++]=famAge[hhM[i].uid];
      m[k++]=famDue; for(int q=0;q<2;q++){ int u=pl[q], v=255; if(famDue&&u!=255){ if(u==hhPUid) v=0; else for(int i=0;i<hhN;i++) if(hhM[i].uid==u) v=i+1; } m[k++]=(u8)v; } }
    nrSync();   // 'HA': the town relationships (townrel.h), whose they are kept as a place like the family tail
    { int c=k+2, n=0; m[k++]=(u8)nrKey; m[k++]=(u8)(nrKey>>8); k++;
      for(int e=0;e<nrN;e++){ const NrE*q=&nrT[e]; int p=q->own==NR_MARK?NR_MARK:hhPlaceOf(q->own); if(p<0) continue;
          m[k++]=q->lot; m[k++]=q->mem; m[k++]=q->nh; m[k++]=(u8)p; m[k++]=(u8)q->d1; m[k++]=(u8)q->l1; m[k++]=(u8)q->d2; m[k++]=(u8)q->l2; m[k++]=q->f1; m[k++]=q->f2; n++; }
      m[c]=(u8)n; }
    for(int i=2;i<k;i++) sum+=m[i];
    m[k]=sum;
    prSave();   // prison.h: who is doing time is kept as a place in this household (a load gives the uids out again)
}
static int hhUidsOf(int ver){ return ver<'6'?HH_MAXOLD+1:ver<=':'-1?HH_MAX9+1:HU_N; }   // uids a saved household kept relationships for
// Where the checksum of the household block at m sits (= its length less one), from its version and counts; -1 when the counts make no sense
// or it would not fit in avail bytes. ('HA': the town tail's own count says how long it is.)
static int hhSumAt(volatile u8*m,int avail){
    int hu=hhUidsOf(m[1]);   // before 'H6': 10 uids; 'H6'..'H9': 14; 'H:': 8
    int v7=m[1]>='7', nb=v7?2*HH_NM:10, nl=m[1]>='@'?LKPK:m[1]>='>'?LKPK14:m[1]>='='?LKPK13:m[1]>='<'?LKPK12:m[1]>=';'?LKPK11:m[1]>='9'?LKPK10:m[1]>='8'?LK_N9:v7?LK_N8:m[1]>='5'?LK_N7:m[1]=='4'?LK_N6:m[1]=='3'?LK_N5:LK_N4, rec=HH_REC-LKPK+nl-2*HH_NM+nb;   // 'H2' households were saved before the hats and clothes, 'H3' before the face details and sliders
    int n=m[2], hb=v7?2*HH_NM:0, k=4+hb+n*rec+3*hu*hu+(m[1]>='?'?hu*hu:0)+(m[1]>='@'?n+3:0);
    if(m[1]>='A'){ if(k+3>avail||m[k+2]>NR_N) return -1; k+=3+NR_B*m[k+2]; }
    return k+1>avail?-1:k;
}
// THE FAMILY-LIFE TEST BUILD (pull request 26 before it took in main) wrote its own 'H=': GENDER as the last pick after the format 12 looks, and its
// family in a block of its own at 4976 ('F' 'Y', where the gap goal lives now: goals.h). That 'H=' is exactly as long as main's, so it is told apart by
// that block: still there, sound, and made for this very household (its checksum and Sim count). Its weddings were a relationship flag (128).
#define HH_FY_OFF 4976
static int hhFyOk(u8 sum,int n){ volatile u8*f=SRAM_BASE+HH_FY_OFF; u8 s=0x46; for(int i=2;i<15;i++) s=(u8)(s*3+f[i]); return f[0]=='F'&&f[1]=='Y'&&f[15]==s&&f[12]==sum&&f[13]==n; }
static void hhLoad(void){
    volatile u8*m=SRAM_BASE+HH_OFF; u8 sum=0x48; hhN=0; for(int j=0;j<HH_MAX;j++) hhFlLd[j]=0;
    famReset(); nrN=0; nrKey=0;
    if(m[0]!='H'||m[1]<'2'||m[1]>'A'||m[2]>(m[1]>=':'?HH_MAX:HH_MAX9)) return;
    int hu=hhUidsOf(m[1]), n=m[2], k=hhSumAt(m,SL_HH_LEN); if(k<0) return;
    for(int i=2;i<k;i++) sum+=m[i]; if(m[k]!=sum) return;
    if(m[3]>=hu) return;
    int fy=m[1]=='='&&hhFyOk(sum,n);   // a household of the family-life test build (above)
    int v7=m[1]>='7', nl=m[1]>='8'?LK_N9:v7?LK_N8:m[1]>='5'?LK_N7:m[1]=='4'?LK_N6:m[1]=='3'?LK_N5:LK_N4;   // (looks of a household from before 'H9': bytes, not packed)
    u8 nu[HH_MAX9+1]; for(int u=0;u<=HH_MAX9;u++) nu[u]=255;   // an older, bigger household: the uids are given out again (you 0, the members 1..), the first HH_MAX members stay
    k=4; int pu=m[3]; nu[pu]=0; hhPUid=0;
    if(v7){ for(int j=0;j<HH_NM;j++) hhPName[j]=(char)m[k++]; for(int j=0;j<HH_NM;j++) hhPLast[j]=(char)m[k++]; hhPName[HH_NM-1]=hhPLast[HH_NM-1]=0; if(!hhPName[0]){ hhPName[0]='Y'; hhPName[1]='O'; hhPName[2]='U'; hhPName[3]=0; } }
    int kept=0;
    for(int i=0;i<n;i++){ HhSim tmp, *s=i<HH_MAX?&hhM[i]:&tmp;
        if(m[1]>='9'){ int lim=m[1]>='@'?LK_N:m[1]>='>'?LK_N14:fy?LK_N12:m[1]>='='?LK_N13:m[1]>='<'?LK_N12:m[1]>=';'?LK_N11:LK_N10; for(int j=0;j<LK_N;j++) s->look[j]=0; for(int j=0;j<lim;j++) if(!lkSlide(j)) s->look[j]=m[k++];   // 'H:' and older: no format 11 looks
          if(fy) s->look[LK_SEX]=m[k++];   // (the test build's GENDER: the last pick, before the sliders)
          int h=-1; for(int j=0;j<lim;j++) if(lkSlide(j)){ if(h<0){ int b=m[k++]; s->look[j]=(u8)(b&15); h=b>>4; } else { s->look[j]=(u8)h; h=-1; } } }
        else for(int j=0;j<LK_N;j++) s->look[j]=j<nl?m[k++]:0;
        { int sg=m[k++]; if(i<HH_MAX) hhFlLd[i]=(u8)(sg>>6); s->stage=(u8)(sg&63); } s->asp=m[k++]; s->ltw=m[k++];
        for(int j=0;j<TR_N;j++) s->tr[j]=m[k++];
        if(v7){ for(int j=0;j<HH_NM;j++) s->name[j]=(char)m[k++]; for(int j=0;j<HH_NM;j++) s->last[j]=(char)m[k++]; } else { for(int j=0;j<10;j++) s->name[j]=(char)m[k++]; s->last[0]=0; s->name[9]=0; } s->name[HH_NM-1]=s->last[HH_NM-1]=0;
        for(int j=0;j<HN_N;j++){ s->need[j]=m[k++]; if(s->need[j]>100) s->need[j]=70; } s->uid=m[k++];
        if(s->stage>=AG_N||s->asp>=AS_N||s->uid>=hu||s->uid==pu) return;   // (AS_GROW, past AS_PICK, is the aspiration of the young)
        if(i>=HH_MAX) continue;   // (a member past the 8 Sims a household holds now: moved out)
        nu[s->uid]=(u8)(++kept); s->uid=(u8)kept;
        s->act=HA_IDLE; s->think=(short)(rnd8()&63); s->hd=0; s->bubT=0; s->hp=HP_MAX; }
    for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; }
    kinClear();
    for(int a=0;a<hu;a++)for(int b=0;b<hu;b++){ signed char d=(signed char)m[k++], l=(signed char)m[k++]; u8 f=m[k++];
        if(nu[a]<HU_N&&nu[b]<HU_N){ relD[nu[a]][nu[b]]=d; relL[nu[a]][nu[b]]=l; relF[nu[a]][nu[b]]=f; } }
    if(m[1]>='?') for(int a=0;a<hu;a++)for(int b=0;b<hu;b++){ u8 v=m[k++]; if(nu[a]<HU_N&&nu[b]<HU_N&&v<KN_N) kin[nu[a]][nu[b]]=v; }   // 'H?': kin follow the same uid mapping
    hhN=n<HH_MAX?n:HH_MAX;
    if(m[1]>='@'){ for(int i=0;i<n;i++){ u8 d=m[k++]; if(i<hhN) famAge[hhM[i].uid]=d; }   // the family tail (the places are the new uids: you 0, members 1..)
        int due=m[k++], pa=m[k++], pb=m[k++]; if(due&&due<=9&&(pa<=hhN||pb<=hhN)){ famDue=(u8)due; famPa=(u8)(pa<=hhN?pa:255); famPb=(u8)(pb<=hhN?pb:255); }
        if(m[1]>='A'){ nrKey=(u16)(m[k]|m[k+1]<<8); int c=m[k+2]; k+=3;   // 'HA': the town relationships (whose: a place, so the new uid)
            for(int e=0;e<c;e++,k+=NR_B){ int p=m[k+3]; if((p>hhN&&p!=NR_MARK)||nrN>=NR_N) continue; NrE*q=&nrT[nrN++];
                q->lot=m[k]; q->mem=m[k+1]; q->nh=m[k+2]; q->own=(u8)p; q->d1=(signed char)m[k+4]; q->l1=(signed char)m[k+5]; q->d2=(signed char)m[k+6]; q->l2=(signed char)m[k+7]; q->f1=m[k+8]; q->f2=m[k+9]; } } }
    else if(fy){   // the family-life test build: genders came with the looks; its weddings become kin, its family block the family tail
        static const u8 wr[SX_N]={KN_WIFE,KN_HUSBAND,KN_SPOUSE};
        for(int i=0;i<hhN;i++) if(hhM[i].look[LK_SEX]>=SX_N) hhM[i].look[LK_SEX]=SX_NB;
        for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++) if(relF[a][b]&128){ relF[a][b]=(u8)((relF[a][b]&127)|RF_STEADY); int x=uSex(a); kin[a][b]=wr[x<SX_N?x:SX_NB]; }
        volatile u8*f=SRAM_BASE+HH_FY_OFF;
        for(int i=0;i<hhN&&i<7;i++) famAge[hhM[i].uid]=f[2+i];   // (by place, members only: your own days are the age block's)
        int due=f[9], pa=f[10], pb=f[11]; if(due&&due<=9&&(pa<=hhN||pb<=hhN)){ famDue=(u8)due; famPa=(u8)(pa<=hhN?pa:255); famPb=(u8)(pb<=hhN?pb:255); } }
    else {   // a household from before GENDER: kin roles say it first (a MOTHER is a woman), then the look
        for(int i=0;i<hhN;i++){ int x=kinSexOf(hhM[i].uid); hhM[i].look[LK_SEX]=(u8)(x>=0?x:sexGuess(hhM[i].look)); }
        { int x=kinSexOf(hhPUid); if(x>=0) look[LK_SEX]=(u8)x; } }
    { int same=1; for(int u=0;u<hu&&u<=HH_MAX9;u++) if(nu[u]!=255&&nu[u]!=u) same=0;   // the uids were given out again: the story's partner and kid follow them (once: the household is saved with the new uids)
      if(!same){ stUidMap(nu,hu<=HH_MAX9?hu:HH_MAX9+1); hhSave(); } }
    for(int k=0;k<TW_N;k++){ int v=TW_V(k); if(twHas[k]&&v>=hhN&&hhM[v].uid==GU0+k) nrGuestIn(k); }   // the neighbours already over: their rows as THIS household knows them
}
// How many bytes the household block at m takes (its header, count, uids and checksum all check out), or 0 if it is not a good household
// or does not fit in avail bytes. The household slots (slots.h) use it to copy a household in and out of SRAM without touching hhM.
static int hhBlockLen(volatile u8*m,int avail){
    if(avail<4||m[0]!='H'||m[1]<'2'||m[1]>'A'||m[2]>(m[1]>=':'?HH_MAX:HH_MAX9)) return 0;
    int hu=hhUidsOf(m[1]), k=hhSumAt(m,avail); if(k<0) return 0;
    u8 sum=0x48; for(int i=2;i<k;i++) sum+=m[i]; if(m[k]!=sum||m[3]>=hu) return 0;
    return k+1;
}
#define HH_MAXB (4+2*HH_NM+HH_MAX*HH_REC+HH_RELB+HH_MAX+3+3+NR_N*NR_B+1)   // the biggest household block: 8 Sims, the family tail, a full town table
_Static_assert(HH_OFF+HH_MAXB<=SLOT_BASE,"the household must fit before the room slots");
_Static_assert(HH_MAXB<=SL_HH_LEN,"the household is bigger than the SRAM block reserved for it");
_Static_assert(HH_OFF+HH_MAXB<=7560,"the household runs into the life stats (lifestats.h LG_OFF)");

static int hhMoveIn(const HhFam*F){   // a pre-made family moves in (HOUSEHOLD, or a NEW GAME). Returns how many fit
    int add=0, first=hhN;
    for(int i=0;i<F->n&&hhN<HH_MAX;i++){ twDrop(hhN); hhNew(&hhM[hhN],&F->m[i]);
        { const char*f=F->fam; if(f[0]=='T'&&f[1]=='H'&&f[2]=='E'&&f[3]==' ') f+=4; int k=0; while(f[k]&&k<HH_NM-1){ hhM[hhN].last[k]=f[k]; k++; }   // THE MIDNIGHTS -> MIDNIGHT
          if(k>1&&hhM[hhN].last[k-1]=='S') k--;
          hhM[hhN].last[k]=0; }
        hhN++; add++; }
    for(int i=first;i<hhN;i++)for(int j=first;j<hhN;j++) if(i!=j){   // a family already knows and likes each other; couples (the first two adults) are in love
        int a=hhM[i].uid, b=hhM[j].uid; relD[a][b]=40; relL[a][b]=50; relF[a][b]=0; kin[a][b]=0;
        if(i<first+2&&j<first+2&&hhM[i].stage>=AG_ADULT&&hhM[j].stage>=AG_ADULT){ relD[a][b]=70; relL[a][b]=80; relF[a][b]=RF_CRUSH|RF_LOVE|RF_STEADY|RF_KISSED|RF_FRIEND|RF_BFF; kin[a][b]=KN_PARTNER; } }
    for(int i=0;i<add;i++) hhJoinCash();   // (every Sim who moves in brings SIM_JOIN_CASH)
    return add;
}
// ---- the pause menu's HOUSEHOLD screen ----
static void hhInviteTrue(void); static int hhMoveOut(int m);   // households.h
static void hhSwitchMenu(void){   // pick the Sim you control
    if(!hhN){ toast("NO ONE ELSE LIVES HERE"); return; }
    char nm[HH_MAX][HH_NM+8]; const char* who[HH_MAX];
    for(int m=0;m<hhN;m++){ char*e=simCat(nm[m],hhM[m].name); if(hhM[m].act==HA_AWAY) simCat(e,prHeld(&hhM[m])?(prHere()?"  AT HOME":"  IN PRISON"):hhUp[m]?hhWhere(m):"  OUT"); else if(xo[XO_ITEMUSE]&&hhM[m].act==HA_USE&&hhM[m].item) simCat(e,iuTag[hhM[m].item-1]); who[m]=nm[m]; }
    int m=menu("WHO DO YOU PLAY",who,hhN); if(m<0) return;
    hhSwitchTo(m); lnote=hhPName; lnoteT=60;
}
static void hhMenu(void){
    if(!dbgOn){   // moving in / out and every other change to the household is the DEBUG CODE's (title screen, see main.c): without it, RELATIONSHIPS and who you control
        static const char* const it2[3]={"RELATIONSHIPS","FAMILY","SWITCH TO A SIM"}; int c=menu("HOUSEHOLD",it2,3);
        if(c==0) relScreen(); else if(c==1) famScreen(); else if(c==2) hhSwitchMenu();
        return; }
    static const char* const it[8]={"RELATIONSHIPS","MOVE IN A FAMILY","INVITE A NEW SIM","TRULY RANDOM SIM","MOVE SOMEONE OUT","MOVE EVERYONE OUT","SWITCH TO A SIM","FAMILY"};
    char t[24]; { char*e=t; const char*p="HOUSEHOLD  "; while(*p) *e++=*p++; e+=numStr(e,hhN+1); p=" OF "; while(*p) *e++=*p++; e+=numStr(e,HH_MAX+1); *e=0; }
    int c=menu(t,it,8); if(c<0) return;
    if(c==6){ hhSwitchMenu(); return; }
    if(c==7){ famScreen(); return; }
    if(c==0){ relScreen(); return; }
    if(c==2){ hhInvite(); return; }
    if(c==3){ hhInviteTrue(); return; }
    if(c==4){ if(!hhN){ toast("NO ONE ELSE LIVES HERE"); return; }
        const char* who[HH_MAX]; for(int m=0;m<hhN;m++) who[m]=hhM[m].name;
        int m=menu("WHO MOVES OUT?",who,hhN); if(m<0) return;
        static const char* const yn[2]={"YES  GOODBYE","NO"}; if(menu("ARE YOU SURE?",yn,2)!=0) return;
        hhMoveOut(m); return; }   // (to a free lot of the town if there is one: they live there and come to visit)
    if(c==5){ for(int m=0;m<hhN;m++) famForget(hhM[m].uid); hhN=0; for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; } kinClear(); hhSave(); toast("ONLY YOU LIVE HERE NOW"); return; }
    { const char* fm[HH_NFAM]; for(int f=0;f<HH_NFAM;f++) fm[f]=hhFams[f].fam;   // MOVE IN A FAMILY: the list of families
      c=menu("MOVE IN A FAMILY",fm,HH_NFAM); if(c<0) return; }
    if(!xo[XO_SIMPRE]){ toast("PRE-MADE SIMS ARE OFF"); return; }
    const HhFam*F=&hhFams[c]; int add=hhMoveIn(F);
    if(!add){ toast("THE HOUSE IS FULL"); return; }
    for(int m=0;m<hhN;m++){ hhOld[m].x0=hhOld[m].x1=0; hhOldSig[m]=0xFFFFFFFFu; }
    toast("PLEASE WAIT  MOVING IN"); hhBakeAll(); hhSave(); toast(add<F->n?"SOME DID NOT FIT":"WELCOME HOME");
}
