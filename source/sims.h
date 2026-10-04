// sims.h - BORE life-sim layer, in the spirit of The Sims 2 on a handheld. Integer only, no allocation, no art files.
// Include AFTER mood.h, the persona (aspNow, pTr, pLtw) and the life globals (lfood, lbl, lstun, lsp, lgrind, lnote, lnoteT, lnear, lscore,
// lcBank, lifeMap) and BEFORE lifeStep.
//
//  NEEDS     REST (energy), CLEAN (hygiene), COMFY (comfort) and ROOM (the look of the place you stand in) next to FOOD and WC (main.c).
//            0..100, they drain by themselves. BED (S) sleeps, SHOWER (H) washes, SOFA (C) sits: stand next to it and press R, A/B/R gets you up.
//            ROOM rises when you stand among furniture (fridge, toilet, bed, shower, sofa) and sags in an empty place.
//            Everything feeds the HAPPY meter (mood.h -> simsComfort()).
//  CLOCK     see SIM_STEPS_MIN. A week is MON..SUN, the game starts MON 08:00. Sleeping runs the clock fast.
//  CAREER    Pro skater, Mon-Fri 09:00-17:00. Trick points you score during the shift count towards the day's QUOTA. At 17:00 you are paid:
//            full quota = full pay (and a step towards promotion), half quota = half pay, less = nothing and a strike. 3 good days = promotion,
//            3 strikes = demotion. Bills are taken at midnight; if you cannot pay, that is a fear coming true.
//  SKILL     SKATING skill 0..5, trained by tricks, combos and grinds; each level adds 8% to trick points.
//
//  ASPIRATION (The Sims 2 way)
//   - The creature has an aspiration (picked on the creator's ASPIRE tab; babies and children GROW UP). It decides which WANTS and FEARS roll:
//     most come from the aspiration's own pool, a few from anywhere, and the PERSONALITY (neat, outgoing, active, playful, nice) tilts the odds.
//   - FOUR WANTS and THREE FEARS are on show. A met want pays its points into the ASPIRATION METER and the same points into REWARD POINTS;
//     a fear that comes true takes its points off the meter. One want can be LOCKED (pause menu > ASPIRATION): it survives a reroll.
//   - Wants and fears REROLL when the creature wakes from a real sleep, when it has a birthday and when its aspiration changes.
//   - The METER (0..1000) drains slowly by itself (twice as fast when SAD). Its zones, from the bottom: FAILING, LOW, OK, GOOD, GOLD, PLATINUM.
//     Higher zones slow the needs and lift the mood; at 0 the creature has an ASPIRATION FAILURE (a breakdown, then a therapist puts it at LOW).
//   - The LIFETIME WANT (one of two per aspiration) is a big goal. Meeting it pays 500 reward points and keeps the meter in PLATINUM for good.
//   - REWARD POINTS buy aspiration rewards (pause menu > ASPIRATION > R): ENERGIZER, THINKING CAP, MONEY TREE, ELIXIR OF LIFE.
//  THOUGHT BUBBLE  over the head: an urgent need if there is one, otherwise a want.
//  SAVING    the life (needs, cash, meter, wants and fears, counters ...) is kept in SRAM (SIM_OFF, SIM_BLOCK bytes "SIM3"). An older 24 byte
//            "SIM2" life still loads (and becomes SIM3 at the next save). See simsPack() / simsUnpack().
//
// HOW TO ADD A WANT: add an SE_ event if it needs a new one, a row in simWants (name, event, points, furniture, icon, parameter, aspirations,
//   trait, minimum, who) and call simEvent(SE_X) where it happens (or map a mood event to it in simsMood). A FEAR is the same in simFears.
//   Rows are saved by index: only ever add rows at the END of the tables.
// HOW TO ADD A NEED: a variable (saved in simsPack/simsUnpack), a rate in simsTick, a use in simBegin, a bar in hud.h.
//
// TUNING
#define SIM_STEPS_MIN  15     // logic steps per game minute (60/s): 15 = a game day is 6 real minutes
#define SIM_RATE_NRG   7      // energy lost per step, 1/1024 pt (7 = 1 pt per 2.4 s, 100 to 0 in ~4 minutes)
#define SIM_RATE_HYG   5
#define SIM_RATE_COM   6
#define SIM_GAIN_SLEEP 175    // energy gained per step while sleeping, 1/1024 pt (~10 s for a full night)
#define SIM_GAIN_WASH  400    // hygiene per step in the shower (~4 s)
#define SIM_GAIN_SIT   300    // comfort per step on the sofa (~5.5 s)
#define SIM_LOW        20     // a need below this shows in the thought bubble
#define SIM_SLEEPY_TOP 20     // % of top speed lost when ENERGY is below SIM_LOW
#define SIM_WANT_GAP   240    // steps before an emptied want slot rolls a new want (a fear slot waits twice as long)
#define SIM_NIGHT_FROM 1320   // 22:00 ... 06:00: awake costs energy 50% faster, sleep restores 25% faster
#define SIM_NIGHT_TO   360
#define SIM_WORK_FROM  540    // 09:00
#define SIM_WORK_TO    1020   // 17:00
#define SIM_QUOTA0     600    // trick points needed per shift at job level 0 ...
#define SIM_QUOTA_LVL  500    // ... plus this per level
#define SIM_PAY0       70     // pay for a full shift at level 0 ...
#define SIM_PAY_LVL    40     // ... plus this per level (+30 when you score double the quota)
#define SIM_BILLS      40     // taken every midnight
#define SIM_CASH0      200    // starting cash
#define SIM_ROOM_R     5      // ROOM looks this many tiles around the skater
#define SIM_METER0     500    // aspiration meter at the start of a life (0..1000)
#define SIM_METER_K    5      // meter points per want / fear point
#define SIM_METER_DRAIN 60    // the meter loses 1 point every this many steps (500 to 100 in under 7 minutes of nothing)
#define SIM_THERAPY    300    // where the therapist leaves the meter after an aspiration failure
#define SIM_LTW_PTS    500    // reward points for the lifetime want
#define UL_CLOUDS      1      // unlock bit: the song WORTHLESS CLOUDS (unlocks.h) comes with the first lifetime want you meet
static int jbUnlock(int bit);   // (main.c) sets an unlock bit for good and rebuilds the jukebox list; 1 = it was locked before
#define SIM_GOOD_SLEEP 300    // steps of sleep (5 game hours: sleep runs the clock fast) that count as a real night: wants reroll on waking
#define SIM_TREE_PAY   25     // the MONEY TREE pays this every midnight
#define SIM_DNA_SKILL  15     // Spore DNA (main.c, spent on parts in the creator): a met want pays its points, and these
#define SIM_DNA_PROMO  25
#define SIM_DNA_BDAY   50     // (the BIRTHDAY note says the number)
#define SIM_DNA_LTW    200
static const short simZoneAt[5]={100,300,500,700,900};               // meter where LOW, OK, GOOD, GOLD and PLATINUM start
static const char* const simZoneNm[6]={"FAILING","LOW","OK","GOOD","GOLD","PLATINUM"};
static const unsigned char simDecayPct[6]={110,100,100,90,75,50};   // need decay in each zone
static const signed char simZoneMood[6]={-20,-10,0,5,10,20};         // added to the HAPPY target in each zone
static const char* const simJobNm[6]={"NEWBIE","AMATEUR","SPONSORED","PRO","TEAM RIDER","LEGEND"};
static const short simSkillAt[5]={12,35,70,120,200};      // skill points for skill levels 1..5
static const char* const simDayNm[7]={"MON","TUE","WED","THU","FRI","SAT","SUN"};

// furniture the room has (simsScan) and what a want needs
enum { SR_FRIDGE=1, SR_TOILET=2, SR_BED=4, SR_SHOWER=8, SR_SOFA=16, SR_RAIL=32, SR_RAMP=64, SR_PIPE=128 };
// things that happen (wants and fears are both made of these)
enum { SE_EAT, SE_PEE, SE_SLEEP, SE_SHOWER, SE_SOFA, SE_TRICK, SE_COMBO, SE_GRIND, SE_AIR, SE_SHOWOFF, SE_STOKED, SE_GREAT,
       SE_SHIFT, SE_ACE, SE_PROMO, SE_CASH, SE_BILLS, SE_SKILL, SE_PRACTICE, SE_ROOM, SE_GROWUP,
       SE_BAIL, SE_HURT, SE_ACCIDENT, SE_FAINT, SE_PASSOUT, SE_BROKE, SE_DEMOTE, SE_NOPAY, SE_STINKY, SE_BORED, SE_SAD, SE_DIE, SE_OLD, SE_SHABBY,
       SE_GLIDE, SE_CHARGE,
       SE_TALK, SE_FRIEND, SE_BFF, SE_KISS, SE_LOVE, SE_STEADY, SE_HUGGED, SE_LAUGH,   // social (house.h)
       SE_REJECT, SE_SLAPPED, SE_FIGHT, SE_ENEMY, SE_LONELY,
       SE_PIPE, SE_N };   // SE_PIPE: a puff on the water pipe, or PUFF PUFF PASS   // (event numbers are not saved: they can be put in any order; table ROWS are saved by index)
// icons (7x7, simIconArt): drawn in the HUD cells, the aspiration panel and the creator
enum { IC_FOOD, IC_WC, IC_BED, IC_SHOWER, IC_SOFA, IC_BOARD, IC_COMBO, IC_RAIL, IC_AIR, IC_STAR, IC_BRIEF, IC_UP, IC_DOWN, IC_BOOK, IC_HOUSE,
       IC_COIN, IC_TROPHY, IC_CAKE, IC_HEART, IC_SKULL, IC_HURT, IC_PUDDLE, IC_SAD, IC_GLASS, IC_CANE, IC_STINK, IC_BAIL, IC_ZZZ,
       IC_TALK, IC_HAND, IC_ANGRY, IC_LEAF, IC_N };
static const char* const simIconArt[IC_N][7]={
  {"...#...","..#....",".##.##.","#######","#######","#######",".##.##."},   // food (apple)
  {"###....","###....","###....","#######",".#####.","..###..","..###.."},   // wc
  {".......","#......","#.##...","#######","#######","#.....#","......."},   // bed
  {"..###..",".#...#.",".......",".#.#.#.",".......","#.#.#.#","......."},   // shower
  {".......",".#####.",".#####.","#######","#######","#.....#","......."},   // sofa
  {".......",".......","#.....#",".#####.",".......",".#...#.","......."},   // skateboard
  {"...###.","..###..",".###...","#####..","..###..",".###...",".#....."},   // combo (lightning)
  {".......","#######",".#...#.",".#...#.",".#...#.",".#...#.","......."},   // rail
  {"...#...","..###..",".#####.","...#...","...#...",".......","#######"},   // air
  {"...#...","...#...","#######",".#####.","..###..",".##.##.","#.....#"},   // star
  {".......","..###..",".##.##.","#######","###.###","#######","......."},   // briefcase
  {"...#...","..###..",".#####.","#######","..###..","..###..","..###.."},   // up
  {"..###..","..###..","..###..","#######",".#####.","..###..","...#..."},   // down
  {"#.....#","##...##","###.###","###.###","###.###","#######","...#..."},   // book (open)
  {"...#...","..###..",".#####.","#######",".##.##.",".##.##.",".##.##."},   // house
  {"...#...",".#####.","##.#...",".#####.","...#.##",".#####.","...#..."},   // coin
  {"#######","#######",".#####.","..###..","...#...","..###..",".#####."},   // trophy
  {"..#.#..","..#.#..",".......","#######","#.#.#.#","#######","#######"},   // cake
  {".......",".##.##.","#######","#######",".#####.","..###..","...#..."},   // heart
  {".#####.","#######","#..#..#","#######",".##.##.",".#####.",".#.#.#."},   // skull
  {"..###..","..###..","#######","#######","#######","..###..","..###.."},   // hurt (cross)
  {"...#...","..###..","..###..",".......",".#####.","#######",".#####."},   // puddle
  {".#####.","#.....#","#.#.#.#","#.....#","#.###.#","#.....#",".#####."},   // sad
  {"#######",".#...#.","..#.#..","...#...","..#.#..",".#####.","#######"},   // hourglass (bored)
  {"...##..","..#..#.",".....#.",".....#.",".....#.",".....#.",".....#."},   // cane (old)
  {"#..#..#",".#..#..","#..#..#",".#..#..","#..#..#",".......","#######"},   // stink
  {"#.....#",".#...#.","..#.#..","...#...","..#.#..",".#...#.","#.....#"},   // bail (x)
  {"#####..","...#...","..#....",".#####.","....###",".....#.","....###"},   // zzz
  {".#####.","#.....#","#.#.#.#","#.....#",".#####.","..#....",".#....."},   // talk (speech balloon)
  {"..#.#..",".##.##.",".##.##.","######.","######.",".#####.","..###.."},   // hand (high five)
  {"#.....#",".#...#.","..###..",".#.#.#.","#######","#.###.#",".#...#."},   // angry
  {"...#...",".#.#.#.","#.###.#",".#####.","..###..","...#...","...#..."},   // leaf (chill)
};
static const unsigned char simAspIcon[AS_N]={IC_COIN,IC_BOOK,IC_TROPHY,IC_STAR,IC_HOUSE,IC_CAKE};
// a wish's parameter: none, a cash target, a skill point target, a combo bank target (chosen when the wish rolls)
enum { WP_NONE, WP_CASH, WP_SKILL, WP_COMBO };
// who may roll a wish
enum { WH_ANY=0, WH_JOB=1, WH_GROWS=2, WH_OLDING=4, WH_LEARN=8, WH_WINGS=16, WH_HORNS=32, WH_SOCIAL=64, WH_ROMANCE=128 };   // ... someone else lives here / teen or older and someone to love   // career on / can still grow up / an adult who will grow old / skill not maxed / has the Spore part
#define A(x) (1<<(x))
#define TP(t) ((t)+1)    // trait: high values favour it
#define TN(t) (-(t)-1)   // trait: low values favour it
typedef struct { const char* name; unsigned char ev, pts, req, icon, par, asp; signed char tr; unsigned char minv, who; } SimWish;
static const SimWish simWants[]={   // '#' in a name is replaced by the wish's parameter
    {"HAVE A SNACK",   SE_EAT,     8,SR_FRIDGE,IC_FOOD,  WP_NONE, A(AS_PLEAS)|A(AS_GROW)|A(AS_HOME), 0,         0,WH_ANY},
    {"USE THE WC",     SE_PEE,     5,SR_TOILET,IC_WC,    WP_NONE, A(AS_HOME)|A(AS_GROW),             TP(TR_NEAT),0,WH_ANY},
    {"TAKE A NAP",     SE_SLEEP,  12,SR_BED,   IC_BED,   WP_NONE, A(AS_HOME)|A(AS_PLEAS)|A(AS_GROW), TN(TR_ACT), 0,WH_ANY},
    {"GET CLEAN",      SE_SHOWER, 10,SR_SHOWER,IC_SHOWER,WP_NONE, A(AS_HOME)|A(AS_POP),              TP(TR_NEAT),0,WH_ANY},
    {"SIT ON SOFA",    SE_SOFA,    8,SR_SOFA,  IC_SOFA,  WP_NONE, A(AS_PLEAS)|A(AS_HOME),            TN(TR_ACT), 0,WH_ANY},
    {"LAND A TRICK",   SE_TRICK,   8,0,        IC_BOARD, WP_NONE, A(AS_GROW)|A(AS_POP)|A(AS_PLEAS),  TP(TR_ACT), 0,WH_ANY},
    {"TRICK COMBO",    SE_COMBO,  15,0,        IC_COMBO, WP_NONE, A(AS_POP)|A(AS_PLEAS),             TP(TR_PLAY),2,WH_ANY},
    {"5 TRICK COMBO",  SE_COMBO,  30,0,        IC_COMBO, WP_NONE, A(AS_POP)|A(AS_KNOW),              TP(TR_ACT), 5,WH_ANY},
    {"GRIND A RAIL",   SE_GRIND,  10,SR_RAIL,  IC_RAIL,  WP_NONE, A(AS_PLEAS)|A(AS_GROW),            TP(TR_ACT), 0,WH_ANY},
    {"GET AIR",        SE_AIR,    12,SR_RAMP,  IC_AIR,   WP_NONE, A(AS_PLEAS)|A(AS_POP)|A(AS_GROW),  TP(TR_PLAY),0,WH_ANY},
    {"BANK A # COMBO", SE_SHOWOFF,25,0,        IC_TROPHY,WP_COMBO,A(AS_POP)|A(AS_FORTUNE),           TP(TR_OUT), 0,WH_ANY},
    {"FEEL STOKED",    SE_STOKED, 15,0,        IC_STAR,  WP_NONE, A(AS_PLEAS)|A(AS_POP),             TP(TR_PLAY),0,WH_ANY},
    {"FEEL GREAT",     SE_GREAT,  20,0,        IC_HEART, WP_NONE, A(AS_HOME)|A(AS_PLEAS),            TP(TR_NICE),0,WH_ANY},
    {"FINISH A SHIFT", SE_SHIFT,  15,0,        IC_BRIEF, WP_NONE, A(AS_FORTUNE),                     0,          0,WH_JOB},
    {"ACE A SHIFT",    SE_ACE,    30,0,        IC_BRIEF, WP_NONE, A(AS_FORTUNE)|A(AS_KNOW),          TP(TR_ACT), 0,WH_JOB},
    {"GET PROMOTED",   SE_PROMO,  40,0,        IC_UP,    WP_NONE, A(AS_FORTUNE)|A(AS_POP),           0,          0,WH_JOB},
    {"HAVE # CASH",    SE_CASH,   20,0,        IC_COIN,  WP_CASH, A(AS_FORTUNE),                     0,          0,WH_JOB},
    {"PAY THE BILLS",  SE_BILLS,  10,0,        IC_HOUSE, WP_NONE, A(AS_FORTUNE)|A(AS_HOME),          TP(TR_NEAT),0,WH_JOB},
    {"LEARN A SKILL",  SE_SKILL,  30,0,        IC_BOOK,  WP_NONE, A(AS_KNOW)|A(AS_GROW),             TN(TR_PLAY),0,WH_LEARN},
    {"GAIN # SKILL",   SE_PRACTICE,15,0,       IC_BOOK,  WP_SKILL,A(AS_KNOW)|A(AS_GROW),             TN(TR_PLAY),0,WH_LEARN},
    {"NICE ROOM",      SE_ROOM,   15,SR_BED|SR_SOFA,IC_HOUSE,WP_NONE,A(AS_HOME)|A(AS_POP),           TP(TR_NEAT),0,WH_ANY},
    {"GROW UP",        SE_GROWUP, 40,0,        IC_CAKE,  WP_NONE, A(AS_GROW),                        0,          0,WH_GROWS},
    {"GO GLIDING",     SE_GLIDE,  15,0,        IC_AIR,   WP_NONE, A(AS_PLEAS)|A(AS_POP)|A(AS_GROW),  TP(TR_PLAY),0,WH_WINGS},   // Spore parts: only with WINGS ...
    {"CHARGE A WALL",  SE_CHARGE, 12,0,        IC_HURT,  WP_NONE, A(AS_POP)|A(AS_PLEAS),             TP(TR_ACT), 0,WH_HORNS},   // ... or HORNS
    {"TALK TO SOMEONE",SE_TALK,    8,0,        IC_TALK,  WP_NONE, A(AS_POP)|A(AS_HOME)|A(AS_GROW),   TP(TR_OUT), 0,WH_SOCIAL},  // social (house.h)
    {"MAKE A FRIEND",  SE_FRIEND, 30,0,        IC_HAND,  WP_NONE, A(AS_POP)|A(AS_HOME)|A(AS_GROW),   TP(TR_OUT), 0,WH_SOCIAL},
    {"BEST FRIENDS",   SE_BFF,    40,0,        IC_TROPHY,WP_NONE, A(AS_POP)|A(AS_HOME),              TP(TR_NICE),0,WH_SOCIAL},
    {"FIRST KISS",     SE_KISS,   30,0,        IC_HEART, WP_NONE, A(AS_PLEAS)|A(AS_POP),             TP(TR_OUT), 0,WH_ROMANCE},
    {"FALL IN LOVE",   SE_LOVE,   45,0,        IC_HEART, WP_NONE, A(AS_HOME)|A(AS_PLEAS),            TP(TR_NICE),0,WH_ROMANCE},
    {"GO STEADY",      SE_STEADY, 50,0,        IC_HEART, WP_NONE, A(AS_HOME),                        TP(TR_NICE),0,WH_ROMANCE},
    {"GET A HUG",      SE_HUGGED, 12,0,        IC_HEART, WP_NONE, A(AS_HOME)|A(AS_GROW),             TP(TR_NICE),0,WH_SOCIAL},
    {"SHARE A LAUGH",  SE_LAUGH,  12,0,        IC_STAR,  WP_NONE, A(AS_PLEAS)|A(AS_POP),             TP(TR_PLAY),0,WH_SOCIAL},
    {"PUFF PUFF PASS", SE_PIPE,   12,SR_PIPE, IC_LEAF,  WP_NONE, A(AS_PLEAS)|A(AS_POP),             TP(TR_PLAY),0,WH_ANY},     // grown-ups only (simWho2)
};
static const SimWish simFears[]={
    {"BAILING",        SE_BAIL,     8,0,IC_BAIL,  WP_NONE,A(AS_POP)|A(AS_GROW),          TN(TR_OUT), 0,WH_ANY},
    {"GETTING HURT",   SE_HURT,    10,0,IC_HURT,  WP_NONE,A(AS_GROW)|A(AS_KNOW),         TN(TR_ACT), 0,WH_ANY},
    {"AN ACCIDENT",    SE_ACCIDENT,15,0,IC_PUDDLE,WP_NONE,A(AS_POP)|A(AS_HOME),          TP(TR_NEAT),0,WH_ANY},
    {"FAINTING",       SE_FAINT,   12,0,IC_FOOD,  WP_NONE,A(AS_HOME)|A(AS_GROW),         0,          0,WH_ANY},
    {"PASSING OUT",    SE_PASSOUT, 12,0,IC_ZZZ,   WP_NONE,A(AS_KNOW)|A(AS_PLEAS),        TN(TR_ACT), 0,WH_ANY},
    {"BEING BROKE",    SE_BROKE,   20,0,IC_COIN,  WP_NONE,A(AS_FORTUNE)|A(AS_HOME),      0,          0,WH_JOB},
    {"DEMOTION",       SE_DEMOTE,  25,0,IC_DOWN,  WP_NONE,A(AS_FORTUNE)|A(AS_POP),       0,          0,WH_JOB},
    {"NO PAY TODAY",   SE_NOPAY,   15,0,IC_BRIEF, WP_NONE,A(AS_FORTUNE),                 0,          0,WH_JOB},
    {"BEING STINKY",   SE_STINKY,  10,0,IC_STINK, WP_NONE,A(AS_POP)|A(AS_HOME),          TP(TR_NEAT),0,WH_ANY},
    {"GETTING BORED",  SE_BORED,   10,0,IC_GLASS, WP_NONE,A(AS_PLEAS),                   TP(TR_PLAY),0,WH_ANY},
    {"FEELING SAD",    SE_SAD,     12,0,IC_SAD,   WP_NONE,A(AS_PLEAS)|A(AS_HOME),        0,          0,WH_ANY},
    {"DYING",          SE_DIE,     25,0,IC_SKULL, WP_NONE,A(AS_KNOW)|A(AS_HOME)|A(AS_GROW),0,        0,WH_ANY},
    {"GROWING OLD",    SE_OLD,     15,0,IC_CANE,  WP_NONE,A(AS_POP)|A(AS_PLEAS),         0,          0,WH_OLDING},
    {"A SHABBY ROOM",  SE_SHABBY,  10,0,IC_HOUSE, WP_NONE,A(AS_HOME),                    TP(TR_NEAT),0,WH_ANY},
    {"BEING REJECTED", SE_REJECT,  15,0,IC_BAIL,  WP_NONE,A(AS_POP)|A(AS_PLEAS),         TN(TR_OUT), 0,WH_SOCIAL},   // social (house.h)
    {"GETTING SLAPPED",SE_SLAPPED, 15,0,IC_HURT,  WP_NONE,A(AS_POP)|A(AS_HOME),          TN(TR_NICE),0,WH_SOCIAL},
    {"A FIGHT",        SE_FIGHT,   12,0,IC_ANGRY, WP_NONE,A(AS_HOME)|A(AS_GROW),         TP(TR_NICE),0,WH_SOCIAL},
    {"MAKING AN ENEMY",SE_ENEMY,   20,0,IC_ANGRY, WP_NONE,A(AS_POP),                     TP(TR_OUT), 0,WH_SOCIAL},
    {"BEING LONELY",   SE_LONELY,  12,0,IC_SAD,   WP_NONE,A(AS_POP)|A(AS_HOME),          TP(TR_OUT), 0,WH_SOCIAL},
};
#undef A
#define SIM_NW ((int)(sizeof(simWants)/sizeof(simWants[0])))
#define SIM_NF ((int)(sizeof(simFears)/sizeof(simFears[0])))
#define SIM_WS 4   // want slots
#define SIM_FS 3   // fear slots

// lifetime wants: two per aspiration (GROW UP has none: it is chosen as a teen)
enum { LT_JOB, LT_CASH, LT_SKILL, LT_TRICKS, LT_COMBO, LT_WANTS, LT_STOKED, LT_NIGHTS, LT_HOME };
typedef struct { const char* name; unsigned char kind; unsigned short goal; } SimLtw;
static const SimLtw simLtws[AS_PICK][2]={
    {{"BE A LEGEND",   LT_JOB,   5},   {"HAVE 3000 CASH", LT_CASH,  3000}},
    {{"MAX SKATE SKILL",LT_SKILL,5},   {"LAND 500 TRICKS",LT_TRICKS,500}},
    {{"20000 COMBO",   LT_COMBO, 20000},{"GO PRO",        LT_JOB,   3}},
    {{"MEET 100 WANTS",LT_WANTS, 100}, {"STOKED 20 MIN",  LT_STOKED,20}},
    {{"30 GOOD NIGHTS",LT_NIGHTS,30},  {"PERFECT HOME",   LT_HOME,  1}},
};
// aspiration rewards, bought with reward points
enum { RW_ENERGIZER, RW_CAP, RW_TREE, RW_ELIXIR, RW_N };
static const char* const simRewNm[RW_N]={"ENERGIZER","THINKING CAP","MONEY TREE","ELIXIR OF LIFE"};
static const short simRewCost[RW_N]={100,150,300,250};

// ---- state ----
static int sNrg, sHyg, sCom, sRoom, sSoc;    // needs, 0..100 (SOCIAL: refilled by talking to the household, house.h)
static int simCrS;                           // fractional SOCIAL drain
static int hhOthers(void); static int hhRomanceOk(void);   // house.h: is anyone else at home / anyone to love
static int simCrN, simCrH, simCrC;           // fractional need changes, 1/1024 pt
static int simAct, simActT, simActN;         // activity: 0 none, 1 sleep, 2 wash, 3 sit; steps left; steps done
static int simAsp, simDone, simMeter, simZone;   // reward points, wants met, aspiration meter 0..1000 and its zone 0..5
static int simW[SIM_WS], simWP[SIM_WS], simF[SIM_FS];   // slot contents (index into the tables, -1 empty) and want parameters
static int simLock;                          // bit s = want slot s is locked
static int simSlotT[SIM_WS+SIM_FS];          // refill countdown per slot
static int simAspUsed;                       // the aspiration the slots were rolled for (-1: none yet)
static int simFlags;                         // SF_ bits
enum { SF_LTW=1, SF_TREE=2, SF_HOME=4 };     // lifetime want met, owns a money tree, had a perfect home
static int simTricks, simBestCombo, simStokedS, simNights;   // lifetime want counters: tricks landed, best combo banked, seconds stoked, good nights
static int simHave, simPrevMood, simEdges, simStokedCr, simDrainCr;   // SR_ mask of furniture; last mood state; edge flags; counters
static unsigned simRng=12345u;
static const char* simQ, *simQ2; static int simQT;   // a note waiting for the note line to be free, and the one after it
static int simT;                             // steps since reset (drives the bubble)
static int simMoney, simDay, simMin, simClkCr;   // cash, days since the start (0 = MON), minute of day, step counter towards a minute
static int jobLvl, jobGood, jobBad, shiftPts, simLastScore, simNiceRoom;   // job level 0..5, good days towards promotion, strikes, points this shift
static int skillPts, skillLvl;               // SKATING skill
static char simClk[16], simMsg[40], simMsg2[40], simWTxt[SIM_WS][24] EWRAM_BSS;   // clock text, note buffers, want names with their parameter

static int simRnd(void){ simRng=simRng*1664525u+1013904223u; return (int)(simRng>>24); }
static void simQueue(const char* s){ simQ=s; simQ2=0; simQT=240; }
static char* simCat(char*d,const char*s){ while(*s) *d++=*s++; *d=0; return d; }
static char* simCatN(char*d,int n){ char t[8]; int k=0; if(n<0) n=0; if(n==0) t[k++]='0'; while(n>0&&k<7){ t[k++]=(char)('0'+n%10); n/=10; } while(k>0) *d++=t[--k]; *d=0; return d; }
static void simMsgPay(const char* pre,int n){ simCatN(simCat(simMsg,pre),n); }   // "SHIFT PAID 110" into simMsg

static void simsScan(void){   // what does this map have?
    simHave=0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ char c=lifeMap[y][x];
        if(c=='F') simHave|=SR_FRIDGE; else if(c=='T') simHave|=SR_TOILET; else if(c=='S') simHave|=SR_BED;
        else if(c=='H') simHave|=SR_SHOWER; else if(c=='C'||c=='U') simHave|=SR_SOFA; else if(c=='G') simHave|=SR_PIPE;
        else if(c=='='||c=='L'||c=='N'||c=='Z'||c=='K'||c=='J') simHave|=SR_RAIL; else if((c>='1'&&c<='<')) simHave|=SR_RAMP; }
}
static void simSkillCalc(void){ int l=0; for(int i=0;i<5;i++) if(skillPts>=simSkillAt[i]) l=i+1; skillLvl=l; }
static int simZoneOf(int m){ int z=0; for(int i=0;i<5;i++) if(m>=simZoneAt[i]) z=i+1; return z; }
static int simWorkday(void){ return (simDay%7)<5; }
static int simInShift(void){ return ojob()&&simWorkday()&&simMin>=SIM_WORK_FROM&&simMin<SIM_WORK_TO; }
static int simQuota(void){ return (SIM_QUOTA0+SIM_QUOTA_LVL*jobLvl)*oQuotaPct()/100; }
static int simIsNight(void){ return simMin>=SIM_NIGHT_FROM||simMin<SIM_NIGHT_TO; }
static int simWishes(void){ return stage!=AG_BABY; }   // babies have no wants or fears (and no aspiration meter)

// ---- rolling wants and fears ----
// ---- the water pipe: grown-ups, and (OPTIONS > PLAY > PIPE AGE: LATE TEENS) a teen in the last quarter of the teen stage. How long that
// stage is comes from OPTIONS > TIME > AGES (TEEN LASTS, scaled by AGING), so the quarter moves with it: with aging OFF or FOREVER a teen never
// gets there. ----
static int pipeTeenOk(void){
    static const u8 pct[4]={0,200,100,50};
    if(stage!=AG_TEEN||xo[XO_PIPEAGE]!=1||!xo[XO_AGING]||!oStageDays(AG_TEEN)) return 0;
    int need=oStageDays(AG_TEEN)*pct[xo[XO_AGING]]/100; if(need<1) need=1;
    return (ageDays+1)*4>need*3;   // (in whole days: the day that started in the last quarter counts)
}
static int pipeOk(void){ return stage>=AG_ADULT||pipeTeenOk(); }
static int simWho(int who){   // may this creature roll a wish with these WH_ flags?
    if((who&WH_JOB)&&!ojob()) return 0;
    if((who&WH_GROWS)&&(stage>=AG_ADULT||!xo[XO_AGING]||!oStageDays(stage))) return 0;
    if((who&WH_OLDING)&&(stage!=AG_ADULT||!xo[XO_AGING]||!oStageDays(stage))) return 0;
    if((who&WH_LEARN)&&skillLvl>=5) return 0;
    if((who&WH_WINGS)&&!(abPow()&PW_GLIDE)) return 0;
    if((who&WH_HORNS)&&!(abPow()&PW_CHARGE)) return 0;
    if((who&(WH_SOCIAL|WH_ROMANCE))&&!hhOthers()) return 0;
    if((who&WH_ROMANCE)&&(stage<AG_TEEN||!hhRomanceOk())) return 0;
    return 1;
}
static int simWeight(const SimWish*w){   // how likely a wish is to roll: aspiration first, then personality, then (for wants) how needed it is
    int a=aspNow(), wt=3;
    if(w->asp&(1<<a)) wt+=14;
    if(w->tr){ int t=(w->tr>0?w->tr:-w->tr)-1, v=w->tr>0?pTr[t]:10-pTr[t]; wt+=v>5?(v-5)*3:0; if(v<=2) wt=wt/2+1; }
    return wt;
}
static int simNeedBoost(int ev){   // wants for a need get likelier as the need runs low (the way a hungry Sim wants to eat)
    int v=ev==SE_EAT?lfood: ev==SE_PEE?100-lbl: ev==SE_SLEEP?sNrg: ev==SE_SHOWER?sHyg: ev==SE_SOFA?sCom: 100;
    return v<60?(60-v)/4:0;
}
static int simOnShow(int want,int i){   // is this row already in a slot?
    if(want){ for(int s=0;s<SIM_WS;s++) if(simW[s]==i) return 1; }
    else    { for(int s=0;s<SIM_FS;s++) if(simF[s]==i) return 1; }
    return 0;
}
static int simPick(int want){   // a weighted random wish the creature and the room can have, not already on show; -1 if none
    const SimWish*tab=want?simWants:simFears; int n=want?SIM_NW:SIM_NF, tot=0, wt[32];
    for(int i=0;i<n&&i<32;i++){
        const SimWish*w=&tab[i]; int v=0;
        if((w->req&simHave)==w->req&&simWho(w->who)&&(w->ev!=SE_PIPE||pipeOk())&&!simOnShow(want,i)){ v=simWeight(w); if(want) v+=simNeedBoost(w->ev); }
        wt[i]=v; tot+=v;
    }
    if(tot<=0) return -1;
    int r=((simRnd()<<8)|simRnd())%tot;
    for(int i=0;i<n&&i<32;i++){ if(r<wt[i]) return i; r-=wt[i]; }
    return -1;
}
static int simParam(int par,int minv){   // a parameter for a wish that is just rolling
    static const short comboAt[6]={1000,1500,2500,4000,6000,9000};
    switch(par){
      case WP_CASH:  { int t=(simMoney+100+(simRnd()%3)*100)/50*50; return t>9999?9999:t; }
      case WP_SKILL: return skillPts+8+(simRnd()%3)*4;
      case WP_COMBO: return comboAt[skillLvl];
      default:       return minv;
    }
}
static void simRollWant(int s){ int i=simPick(1); simW[s]=i; simWP[s]=i>=0?simParam(simWants[i].par,simWants[i].minv):0; }
static void simRollFear(int s){ simF[s]=simPick(0); }
static void simFill(void){
    if(!simWishes()) return;
    for(int s=0;s<SIM_WS;s++) if(simW[s]<0&&simSlotT[s]<=0) simRollWant(s);
    for(int s=0;s<SIM_FS;s++) if(simF[s]<0&&simSlotT[SIM_WS+s]<=0) simRollFear(s);
}
static void simReroll(void){   // a fresh set: every unlocked want and every fear (waking up, a birthday, a new aspiration)
    for(int s=0;s<SIM_WS;s++) if(!(simLock>>s&1)){ simW[s]=-1; simSlotT[s]=0; }
    for(int s=0;s<SIM_FS;s++){ simF[s]=-1; simSlotT[SIM_WS+s]=0; }
    if(!simWishes()){ for(int s=0;s<SIM_WS;s++) simW[s]=-1; simLock=0; return; }
    simFill();
}
static const char* simWantName(int s){   // the want in slot s, with its parameter filled in ("HAVE 450 CASH")
    if(simW[s]<0) return 0;
    const SimWish*w=&simWants[simW[s]]; const char*p=w->name; char*d=simWTxt[s];
    if(w->par==WP_NONE) return p;
    int v=w->par==WP_SKILL?simWP[s]-skillPts:simWP[s]; if(v<1) v=1;
    for(;*p;p++){ if(*p=='#') d=simCatN(d,v); else *d++=*p; } *d=0;
    return simWTxt[s];
}
static const char* simFearName(int s){ return simF[s]<0?0:simFears[simF[s]].name; }

// ---- saving (SRAM at SIM_OFF; main.c's map is 0..4802, the other small blocks sit in 4864..8191: see slots.h) ----
#ifndef SIM_SRAM
#define SIM_OFF 5136
#define SIM_SRAM (SRAM_BASE+SIM_OFF)
#endif
static void simPut16(volatile unsigned char*m,int i,int v){ m[i]=(unsigned char)(v&255); m[i+1]=(unsigned char)((v>>8)&255); }
static int  simGet16(volatile unsigned char*m,int i){ return m[i]|(m[i+1]<<8); }
#define SIM_BLOCK  52   // bytes of one saved life, "SIM3" (also stored inside a room slot, see slots.h)
#define SIM_BLOCK2 24   // the older "SIM2" life
// SIM3: 0 magic | 4 needs x4 | 8 cash | 10 reward points | 12 wants met | 14 day | 16 minute | 18 job level, good, bad | 21 skill points
//       23 meter | 25 flags | 26 tricks | 28 best combo | 30 seconds stoked | 32 good nights | 33 aspiration the slots are for (255 none)
//       34 wants (index+1) x4 | 38 fears (index+1) x3 | 41 lock mask | 42 want parameters x4 | 50 spare | 51 checksum
static void simsPack(volatile unsigned char*m){   // write the life into any SIM_BLOCK byte buffer
    unsigned sum=0x5A;
    m[0]='S'; m[1]='I'; m[2]='M'; m[3]='3';
    m[4]=(unsigned char)sNrg; m[5]=(unsigned char)sHyg; m[6]=(unsigned char)sCom; m[7]=(unsigned char)sRoom;
    simPut16(m,8,simMoney); simPut16(m,10,simAsp); simPut16(m,12,simDone); simPut16(m,14,simDay); simPut16(m,16,simMin);
    m[18]=(unsigned char)jobLvl; m[19]=(unsigned char)jobGood; m[20]=(unsigned char)jobBad; simPut16(m,21,skillPts);
    simPut16(m,23,simMeter); m[25]=(unsigned char)simFlags; simPut16(m,26,simTricks); simPut16(m,28,simBestCombo); simPut16(m,30,simStokedS);
    m[32]=(unsigned char)simNights; m[33]=(unsigned char)(simAspUsed<0?255:simAspUsed);
    for(int s=0;s<SIM_WS;s++){ m[34+s]=(unsigned char)(simW[s]+1); simPut16(m,42+s*2,simWP[s]); }
    for(int s=0;s<SIM_FS;s++) m[38+s]=(unsigned char)(simF[s]+1);
    m[41]=(unsigned char)simLock; m[50]=(unsigned char)(sSoc+1);   // (0 in an older SIM3 = not saved yet)
    for(int i=4;i<=50;i++) sum+=m[i];
    m[51]=(unsigned char)sum;
}
static void simsSaveNow(void){ simsPack(SIM_SRAM); persSave(); }   // the persona too: its DNA is earned here
static void simsSave(void){ if(xo[XO_AUTOSAVE]) simsSaveNow(); }   // AUTO SAVE LIFE option: off = only slots / SAVE LIFE NOW write it
static int simsVer(volatile unsigned char*m){ return (m[0]=='S'&&m[1]=='I'&&m[2]=='M')?(m[3]=='3'?3:m[3]=='2'?2:0):0; }
static int simsCheck(volatile unsigned char*m){   // 1 = the buffer holds a valid life, SIM3 or SIM2 (nothing is changed)
    int v=simsVer(m), last=v==3?SIM_BLOCK-1:SIM_BLOCK2-1; unsigned sum=0x5A;
    if(!v) return 0;
    for(int i=4;i<last;i++) sum+=m[i];
    if(m[last]!=(unsigned char)sum) return 0;
    if(m[4]>100||m[5]>100||m[6]>100||m[7]>100||m[18]>5||simGet16(m,16)>=1440) return 0;
    if(v==3){
        if(simGet16(m,23)>1000||m[41]>15||(m[33]>=AS_N&&m[33]!=255)) return 0;
        for(int s=0;s<SIM_WS;s++) if(m[34+s]>SIM_NW) return 0;
        for(int s=0;s<SIM_FS;s++) if(m[38+s]>SIM_NF) return 0;
    }
    return 1;
}
static int simsUnpack(volatile unsigned char*m){   // 1 = a valid life was read from the buffer
    if(!simsCheck(m)) return 0;
    sNrg=m[4]; sHyg=m[5]; sCom=m[6]; sRoom=m[7];
    simMoney=simGet16(m,8); simAsp=simGet16(m,10); simDone=simGet16(m,12); simDay=simGet16(m,14); simMin=simGet16(m,16);
    jobLvl=m[18]; jobGood=m[19]; jobBad=m[20]; skillPts=simGet16(m,21);
    if(simsVer(m)==3){
        simMeter=simGet16(m,23); simFlags=m[25]; simTricks=simGet16(m,26); simBestCombo=simGet16(m,28); simStokedS=simGet16(m,30);
        simNights=m[32]; simAspUsed=m[33]==255?-1:m[33];
        for(int s=0;s<SIM_WS;s++){ simW[s]=m[34+s]-1; simWP[s]=simGet16(m,42+s*2); }
        for(int s=0;s<SIM_FS;s++) simF[s]=m[38+s]-1;
        simLock=m[41]; sSoc=m[50]?(m[50]>101?70:m[50]-1):70;
    } else {   // SIM2: the old points carry over as reward points, the rest starts fresh
        simMeter=SIM_METER0; simFlags=0; simTricks=simBestCombo=simStokedS=simNights=0; simAspUsed=-1; simLock=0;
        for(int s=0;s<SIM_WS;s++){ simW[s]=-1; simWP[s]=0; } for(int s=0;s<SIM_FS;s++) simF[s]=-1;
    }
    return 1;
}
static int simsLoad(void){ return simsUnpack(SIM_SRAM); }   // 1 = loaded a valid save

static void simsDefaults(void){   // a brand new life (nothing is written to SRAM)
    sNrg=100; sHyg=100; sCom=80; sRoom=40; sSoc=70; simMoney=SIM_CASH0; simAsp=0; simDone=0; simDay=0; simMin=480;
    jobLvl=0; jobGood=0; jobBad=0; skillPts=0;
    simMeter=SIM_METER0; simFlags=0; simTricks=simBestCombo=simStokedS=simNights=0; simAspUsed=-1; simLock=0;
    for(int s=0;s<SIM_WS;s++){ simW[s]=-1; simWP[s]=0; } for(int s=0;s<SIM_FS;s++) simF[s]=-1;
}
static void simsTransient(void){
    simCrN=simCrH=simCrC=0; simAct=simActT=simActN=0; for(int s=0;s<SIM_WS+SIM_FS;s++) simSlotT[s]=0;
    simPrevMood=-1; simEdges=0; simStokedCr=0; simDrainCr=0; simQ=0; simQ2=0; simQT=0; simT=0; simClkCr=0; shiftPts=0; simLastScore=lscore; simNiceRoom=0;
    simSkillCalc(); simsScan(); simZone=simZoneOf(simMeter);
    for(int s=0;s<SIM_WS;s++) if(simW[s]>=0&&((simWants[simW[s]].req&simHave)!=simWants[simW[s]].req||!simWho(simWants[simW[s]].who))){ simW[s]=-1; simLock&=~(1<<s); }   // a different room, or a stage that cannot
    for(int s=0;s<SIM_FS;s++) if(simF[s]>=0&&!simWho(simFears[simF[s]].who)) simF[s]=-1;
    if(simAspUsed!=aspNow()){ simLock=0; simReroll(); simAspUsed=aspNow(); }   // the aspiration was changed in the creator
    else simFill();
}
static void simsReset(void){   // entering the life game: pick up the saved life if there is one
    simsDefaults(); simsLoad(); simsTransient();
}
static void simsRespawn(void){   // after dying: the needs come back, the life (cash, job, skill, aspiration, wants, clock) goes on
    sNrg=60; sHyg=60; sCom=60; simAct=simActT=0; simQ=0; shiftPts=0; simLastScore=lscore;
}
static void simsNewLife(void){   // pause menu: NEW LIFE
    simsDefaults(); simsTransient(); simsSaveNow();
}

// ---- wants, fears and the aspiration meter ----
static void simMeterAdd(int d){
    simMeter+=d; if(simMeter>1000) simMeter=1000; if(simMeter<0) simMeter=0;
    if((simFlags&SF_LTW)&&simMeter<simZoneAt[4]) simMeter=simZoneAt[4];   // the lifetime want keeps it in platinum for good
}
static void simMeet(int s){   // the want in slot s came true
    const SimWish*w=&simWants[simW[s]];
    simAsp+=w->pts; if(simAsp>9999) simAsp=9999; if(simDone<9999) simDone++; dnaAdd(w->pts);   // Spore: living earns DNA
    simMeterAdd(w->pts*SIM_METER_K);
    simW[s]=-1; simLock&=~(1<<s); simSlotT[s]=SIM_WANT_GAP;
    moodEvent(M_WANT); simCatN(simCat(simMsg2,"WANT MET +"),w->pts); simQueue(simMsg2);
}
static void simDread(int s){   // the fear in slot s came true
    const SimWish*w=&simFears[simF[s]];
    simMeterAdd(-w->pts*SIM_METER_K);
    simF[s]=-1; simSlotT[SIM_WS+s]=SIM_WANT_GAP*2;
    moodEventN(M_FEAR,pTr[TR_NICE]<=3?2:1);   // a grouchy creature takes it twice as hard
    simCatN(simCat(simMsg2,"FEAR CAME TRUE -"),w->pts); simQueue(simMsg2);
}
// something happened (v = how much of it: tricks in a combo, a combo's points, cash, skill points): pay a want, or let a fear come true
static void simEventV(int ev,int v){
    if(!simWishes()) return;
    for(int s=0;s<SIM_WS;s++) if(simW[s]>=0&&simWants[simW[s]].ev==ev&&v>=simWP[s]) simMeet(s);
    for(int s=0;s<SIM_FS;s++) if(simF[s]>=0&&simFears[simF[s]].ev==ev) simDread(s);
}
static void simEvent(int ev){ simEventV(ev,1); }
static void simSkillAdd(int n){
    int old=skillLvl; skillPts+=n; if(skillPts>9999) skillPts=9999; simSkillCalc();
    if(skillLvl>old){ simQueue("SKILL UP"); dnaAdd(SIM_DNA_SKILL); moodEvent(M_SKILL); simEvent(SE_SKILL); }
    simEventV(SE_PRACTICE,skillPts);
}
// mood.h calls this from every moodEvent: the game events that wants, fears and skill care about
static void simsMood(int ev,int n){
    switch(ev){
        case M_TRICK: simEvent(SE_TRICK); simSkillAdd(1); if(simTricks<65535) simTricks++; break;
        case M_COMBO:   // n = tricks - 1; lcBank holds the points the chain banked
            simEventV(SE_COMBO,n+1); simEventV(SE_SHOWOFF,lcBank); simSkillAdd(1);
            if(lcBank>simBestCombo) simBestCombo=lcBank>65535?65535:lcBank;
            if(pTr[TR_OUT]>=7) moodFun=moodClamp(moodFun+4*MOOD_ONE);   // outgoing: showing off is a thrill
            break;
        case M_GRIND_ON: simEvent(SE_GRIND); simSkillAdd(1); break;    case M_LAUNCH: simEvent(SE_AIR); break;
        case M_EAT: simEvent(SE_EAT); hpHeal(5); break;           case M_RELIEVE: simEvent(SE_PEE); break;
        case M_BAIL: simEvent(SE_BAIL); if(pTr[TR_OUT]<=3) moodHap=moodClamp(moodHap-3*MOOD_ONE); break;   // shy: bailing is embarrassing
        case M_HURT: case M_HURT_BIG: simEvent(SE_HURT); break;
        case M_ACCIDENT: simEvent(SE_ACCIDENT); break; case M_FAINT: simEvent(SE_FAINT); break;
        case M_PASSOUT: simEvent(SE_PASSOUT); break;   case M_DIE: simEvent(SE_DIE); break;
        case M_SLEEP: hpHeal(40); break;   // a night in bed heals
        default: break;   // M_SLEEP/M_SHOWER/M_SOFA/M_WANT/M_FEAR/M_SKILL/M_PAY/M_PROMO/M_BROKE/M_DEMOTE are raised by sims.h itself
    }
}
static inline int simMin3(int a,int b,int c){ return a<b?(a<c?a:c):(b<c?b:c); }
static int simsComfort(void){ return (sNrg*25+sHyg*20+sCom*20+sRoom*15+sSoc*20)/100; } // blended, used by the HAPPY target in mood.h
static int simsAspMood(void){ return simWishes()?simZoneMood[simZone]:0; }     // the aspiration zone lifts or sinks the HAPPY target
static int simsFunPct(void){ return 70+pTr[TR_PLAY]*6; }                       // playful creatures get bored faster (FUN drains 70..130%)
static int simsTop(int top){ if(sNrg<SIM_LOW) top-=top*SIM_SLEEPY_TOP/100; return top*stSpd[stage]/100*abPct(AB_SPEED,5)/100; }   // too tired: slower; SPEED ability +-5% a point
static int simsPts(int pts){ pts+=pts*skillLvl*8/100; return pts*abPct(AB_STYLE,6)/100; }   // SKATING skill: +8% trick points per level; STYLE ability +-6% a point
static const char* simsAlert(void){   // most urgent need, or 0
    if(lbl>80) return "NEED THE TOILET";   // (WC)
    if(lfood<SIM_LOW) return "EAT";
    if(sNrg<SIM_LOW) return "ZZZ";
    if(sHyg<SIM_LOW) return "STINKY";
    if(sCom<SIM_LOW) return "SIT";
    if(sSoc<SIM_LOW&&hhOthers()) return "LONELY";
    return 0;
}
// lifetime want: progress towards the goal of the chosen one
static const SimLtw* simLtw(void){ return &simLtws[pAsp][pLtw]; }
static int simLtwVal(void){
    switch(simLtw()->kind){
      case LT_JOB: return jobLvl;           case LT_CASH: return simMoney;     case LT_SKILL: return skillLvl;
      case LT_TRICKS: return simTricks;     case LT_COMBO: return simBestCombo; case LT_WANTS: return simDone;
      case LT_STOKED: return simStokedS/60; case LT_NIGHTS: return simNights;  default: return (simFlags&SF_HOME)?1:0;
    }
}
static void simLtwCheck(void){
    if(simFlags&SF_LTW){ if(jbUnlock(UL_CLOUDS)) simQueue("SONG UNLOCKED"); return; }   // (a life that met its want before songs could be unlocked gets it now)
    if(stage<AG_TEEN) return;   // the lifetime want starts with the chosen aspiration
    if(simLtwVal()<simLtw()->goal) return;
    simFlags|=SF_LTW; simAsp+=SIM_LTW_PTS; if(simAsp>9999) simAsp=9999; simMeterAdd(1000); dnaAdd(SIM_DNA_LTW);
    moodEvent(M_PROMO); simQueue("LIFETIME WANT MET");
    if(jbUnlock(UL_CLOUDS)) simQ2="SONG UNLOCKED";   // the dream pays with a song
}
static void simZoneTick(void){   // the meter drains; say it when the zone changes; a meter at 0 is an aspiration failure
    if(!simWishes()) return;
    if(++simDrainCr>=(moodState()==MS_SAD?SIM_METER_DRAIN/2:SIM_METER_DRAIN)){ simDrainCr=0; simMeterAdd(-1); }
    int z=simZoneOf(simMeter);
    if(z!=simZone){
        if(z>simZone&&z>=4) simQueue(z==5?"PLATINUM MOOD":"ASPIRATION GOLD");
        else if(z<simZone&&z==1) simQueue("ASPIRATION LOW");
        simZone=z;
    }
    if(simMeter<=0){   // ASPIRATION FAILURE: a breakdown, then a therapist sorts things out
        lstun=300; lsp=0; lgrind=0; lnote="BREAKDOWN"; lnoteT=150; moodEventN(M_FEAR,3);
        simMeter=SIM_THERAPY; simZone=simZoneOf(simMeter); simReroll(); simQueue("A THERAPIST HELPED");
    }
}

// ---- aspiration rewards ----
static const char* simBuy(int r){   // returns the note to show
    if(simAsp<simRewCost[r]) return "NOT ENOUGH POINTS";
    switch(r){
      case RW_ENERGIZER: if(sNrg>=95) return "NOT TIRED"; sNrg=100; break;
      case RW_CAP: if(skillLvl>=5) return "SKILL IS MAXED"; skillPts=simSkillAt[skillLvl]-1; break;   // the skill point below the next level ...
      case RW_TREE: if(simFlags&SF_TREE) return "YOU HAVE ONE"; simFlags|=SF_TREE; break;
      case RW_ELIXIR: if(stage>=AG_ELDER||!xo[XO_AGING]||!oStageDays(stage)) return "NOT AGING NOW"; if(!ageDays) return "ALREADY YOUNG"; ageDays=0; ageSave(); break;
      default: return "";
    }
    simAsp-=simRewCost[r];
    if(r==RW_CAP) simSkillAdd(1);   // ... and the one that takes it there (SKILL UP, the LEARN A SKILL want)
    simsSave();
    return r==RW_ENERGIZER?"FULL OF ENERGY":r==RW_CAP?"SKILL UP":r==RW_TREE?"MONEY TREE PLANTED":"YOUNGER AGAIN";
}

// ---- clock and career ----
static void simShiftEnd(void){   // 17:00 on a workday
    int q=simQuota(), p=shiftPts, pay=0;
    if(p>=q){ pay=SIM_PAY0+SIM_PAY_LVL*jobLvl+(p>=2*q?30:0); jobBad=0;
        if(++jobGood>=3){ jobGood=0; if(jobLvl<5){ jobLvl++; dnaAdd(SIM_DNA_PROMO); moodEvent(M_PROMO); simEvent(SE_PROMO); simQueue("PROMOTED"); } } }
    else if(p>=q/2){ pay=(SIM_PAY0+SIM_PAY_LVL*jobLvl)/2; }
    else { if(++jobBad>=3){ jobBad=0; jobGood=0; if(jobLvl>0){ jobLvl--; moodEvent(M_DEMOTE); simEvent(SE_DEMOTE); simQueue("DEMOTED"); } } }
    if(pay>0){ simMoney+=pay; if(simMoney>9999) simMoney=9999; moodEvent(M_PAY); simMsgPay(p>=q?"SHIFT PAID ":"HALF PAY ",pay); simQueue(simMsg); }
    else { simEvent(SE_NOPAY); if(!simQ) simQueue("NO PAY TODAY"); }
    if(p>=q/2) simEvent(SE_SHIFT);
    if(p>=2*q) simEvent(SE_ACE);
    simEventV(SE_CASH,simMoney);
    shiftPts=0; simsSave();
}
static void ageTick(void){   // once per game day: each stage lasts the days set on OPTIONS > TIME > AGES (the AGING option scales them); the life loop does the growing
    static const u8 pct[4]={0,200,100,50};
    if(stage>=AG_ELDER||!xo[XO_AGING]||!oStageDays(stage)) return;   // an elder is the last stage; OFF or FOREVER never grows
    int need=oStageDays(stage)*pct[xo[XO_AGING]]/100; if(need<1) need=1;
    if(++ageDays>=need){ gGrow=1; dnaAdd(SIM_DNA_BDAY); simQueue("BIRTHDAY +50 DNA"); simEvent(SE_GROWUP); if(stage+1==AG_ELDER) simEvent(SE_OLD); } else ageSave();
}
static void simMinute(void){   // once per game minute
    simMin++;
    if(simMin==16*60+20&&(simHave&SR_PIPE)&&pipeOk()) simQueue("IT IS 4:20  PUFF PUFF PASS");   // the house gathers at the water pipe (house.h)
    if(simMin>=1440){   // midnight: new day, bills, autosave
        simMin=0; simDay++; if(simDay>30000) simDay=0;
        ageTick();
        if(simFlags&SF_TREE){ simMoney+=SIM_TREE_PAY; if(simMoney>9999) simMoney=9999; }   // the money tree
        int bill=ojob()?SIM_BILLS*oBillsPct()/100:0;   // no career = no bills; BILLS option scales them
        if(bill>0){ if(simMoney>=bill){ simMoney-=bill; simEvent(SE_BILLS); }
            else { simMoney=0; moodEvent(M_BROKE); simEvent(SE_BROKE); simQueue("BILLS UNPAID"); } }
        simEventV(SE_CASH,simMoney);
        simsSave();
    }
    if(ojob()&&simMin==SIM_WORK_FROM-60&&simWorkday()&&!simQ) simQueue("WORK AT 9");
    if(ojob()&&simMin==SIM_WORK_FROM&&simWorkday()){ shiftPts=0; if(!simQ) simQueue("SHIFT STARTS"); }
    if(ojob()&&simMin==SIM_WORK_TO&&simWorkday()) simShiftEnd();
}
static const char* simsClock(void){   // "MON 14:05"
    int h=simMin/60, m=simMin%60, i=0; const char*d=simDayNm[simDay%7];
    simClk[i++]=d[0]; simClk[i++]=d[1]; simClk[i++]=d[2]; simClk[i++]=' ';
    if(xo[XO_CLOCK]==1){ int h12=h%12; if(h12==0) h12=12; if(h12>=10) simClk[i++]='1'; simClk[i++]=(char)('0'+h12%10); }   // CLOCK option: 12 HOUR
    else { simClk[i++]=(char)('0'+h/10); simClk[i++]=(char)('0'+h%10); }
    simClk[i++]=':'; simClk[i++]=(char)('0'+m/10); simClk[i++]=(char)('0'+m%10);
    if(xo[XO_CLOCK]==1) simClk[i++]=(h>=12)?'P':'A';
    simClk[i]=0;
    return simClk;
}

// ---- ROOM need: what is around the skater ----
static void simRoomTick(int tx,int ty){
    int kinds=0, items=0;
    for(int y=ty-SIM_ROOM_R;y<=ty+SIM_ROOM_R;y++)for(int x=tx-SIM_ROOM_R;x<=tx+SIM_ROOM_R;x++){
        if(x<0||y<0||x>=MW||y>=MH) continue; char c=lifeMap[y][x]; int b=0;
        if(c=='F') b=1; else if(c=='T') b=2; else if(c=='S') b=4; else if(c=='H') b=8; else if(c=='C'||c=='U') b=16; else if(c=='V'||c=='G') b=32;   // a lava lamp (or the pipe) makes it a den
        if(b){ kinds|=b; items++; } }
    int k=0; for(int b=1;b<64;b<<=1) if(kinds&b) k++;
    int target=k*16+(items>5?5:items)*4; if(target>100) target=100;
    if(target>sRoom){ sRoom+=2; if(sRoom>target) sRoom=target; }
    else if(target<sRoom&&(simT%90)<30) sRoom--;       // sags slowly
    if(sRoom<0) sRoom=0;
    if(sRoom>=80){ if(!simNiceRoom){ simNiceRoom=1; simEvent(SE_ROOM); } } else if(sRoom<70) simNiceRoom=0;
}
// ---- states that wants and fears watch: each one fires once when it starts ----
enum { SG_STINKY=1, SG_SHABBY=2, SG_GREAT=4 };
static void simEdge(int bit,int on,int ev){ if(on&&!(simEdges&bit)){ simEdges|=bit; simEvent(ev); } else if(!on) simEdges&=~bit; }
static void simStateTick(void){
    simEdge(SG_STINKY,sHyg<10,SE_STINKY);
    simEdge(SG_SHABBY,sRoom<15,SE_SHABBY);
    int great=lfood>=70&&lbl<=30&&sNrg>=70&&sHyg>=70&&sCom>=70;
    if(sRoom>=90&&lfood>=80&&lbl<=20&&sNrg>=80&&sHyg>=80&&sCom>=80) simFlags|=SF_HOME;   // a perfect home moment (the HOME lifetime want)
    simEdge(SG_GREAT,great,SE_GREAT);
    int ms=moodState();
    if(ms!=simPrevMood){
        if(simPrevMood>=0){ if(ms==MS_STOKED) simEvent(SE_STOKED); else if(ms==MS_BORED) simEvent(SE_BORED); else if(ms==MS_SAD) simEvent(SE_SAD); }
        simPrevMood=ms;
    }
    simEventV(SE_CASH,simMoney);
    simLtwCheck();
}

// ---- using furniture ----
// kind: 3 bed, 4 shower, 5 sofa (the lnear values). Returns 1 if the skater started.
static int simBegin(int kind){
    if(kind==3){ if(sNrg>=95){ lnote="NOT TIRED"; lnoteT=40; return 0; } simAct=1; simActT=900; lnote="ZZZ"; lnoteT=40; }
    else if(kind==4){ if(sHyg>=95){ lnote="ALREADY CLEAN"; lnoteT=40; return 0; } simAct=2; simActT=420; lnote="SPLASH"; lnoteT=40; }
    else if(kind==5){ if(sCom>=95){ lnote="COMFY ALREADY"; lnoteT=40; return 0; } simAct=3; simActT=480; lnote="AHH SOFA"; lnoteT=40; }
    else return 0;
    simActN=0; lsp=0; lgrind=0; lstun=2; return 1;
}
static void simEnd(void){
    if(simAct==1){
        moodEvent(M_SLEEP); simEvent(SE_SLEEP);
        if(simActN>=SIM_GOOD_SLEEP){   // a real night's sleep: a fresh set of wants and fears, like waking up in The Sims
            if(sNrg>=90&&simNights<255) simNights++;
            simReroll(); simQueue("NEW WANTS AND FEARS");
        }
    }
    else if(simAct==2){ moodEvent(M_SHOWER); simEvent(SE_SHOWER); }
    else if(simAct==3){ moodEvent(M_SOFA); simEvent(SE_SOFA); }
    simAct=0; simActT=0; simActN=0;
}
// once per logic step while alive. pr = keys pressed this step, tx,ty = the tile the skater stands on.
static void simsTick(unsigned pr,int tx,int ty){
    simT++;
    int pct=simDecayPct[simWishes()?simZone:2]*oNeedPct()/100*abPct(AB_STAMINA,-8)/100, night=simIsNight();   // STAMINA ability: -8% need drain a point
    if(simAct==0){   // needs drain only when not being refilled; NEAT keeps you clean longer, ACTIVE needs the sofa less
        simCrN+=SIM_RATE_NRG*pct/100*(night?3:2)/2;
        simCrH+=SIM_RATE_HYG*pct/100*(130-6*pTr[TR_NEAT])/100;
        simCrC+=SIM_RATE_COM*pct/100*(130-6*pTr[TR_ACT])/100;
        if(hhOthers()){ simCrS+=4*pct/100*(70+6*pTr[TR_OUT])/100; while(simCrS>=1024){ simCrS-=1024; if(sSoc>0){ sSoc--; if(sSoc==SIM_LOW) simEvent(SE_LONELY); } } }   // outgoing Sims get lonely faster
        while(simCrN>=1024){ simCrN-=1024; if(sNrg>0) sNrg--; }
        while(simCrH>=1024){ simCrH-=1024; if(sHyg>0) sHyg--; }
        while(simCrC>=1024){ simCrC-=1024; if(sCom>0) sCom--; }
    } else {
        int *n=simAct==1?&sNrg:simAct==2?&sHyg:&sCom, *cr=simAct==1?&simCrN:simAct==2?&simCrH:&simCrC;
        int g=simAct==1?SIM_GAIN_SLEEP:simAct==2?SIM_GAIN_WASH:SIM_GAIN_SIT;
        if(simAct==1&&night) g=g*5/4;
        if(simAct==3) g=g*(70+6*(10-pTr[TR_ACT]))/100;   // a lazy creature sinks into the sofa
        *cr+=g; while(*cr>=1024){ *cr-=1024; if(*n<100) (*n)++; }
        lstun=lstun>2?lstun:2; lsp=0; lgrind=0;       // stay put while busy
        simActT--; simActN++;
        if(*n>=100||simActT<=0||(pr&(K_A|K_B|K_R))) simEnd();
    }
    // the clock: sleeping runs it one game minute per step
    int spm=oStepsMin();   // DAY LENGTH option (0 = the clock is stopped)
    if(spm>0){ simClkCr+=(simAct==1)?spm:1; while(simClkCr>=spm){ simClkCr-=spm; simMinute(); } }
    // the day's quota counts trick points scored during the shift
    if(lscore>simLastScore&&simInShift()) shiftPts+=lscore-simLastScore;
    simLastScore=lscore;
    if(simT%30==0){ simRoomTick(tx,ty); simStateTick(); }
    if(sNrg==0&&simAct==0){ sNrg=25; lstun=300; lsp=0; lgrind=0; lnote="PASSED OUT"; lnoteT=90; moodEvent(M_PASSOUT); }   // like the old FAINT, from tiredness
    if(moodState()==MS_STOKED&&++simStokedCr>=60){ simStokedCr=0; if(simStokedS<65535) simStokedS++; }
    simZoneTick();
    if(simAspUsed!=aspNow()){   // grew into the chosen aspiration (a child becoming a teen)
        simAspUsed=aspNow(); simLock=0; simReroll(); simCat(simCat(simMsg,"ASPIRES TO "),aspNm[aspNow()]); simQueue(simMsg);
    }
    for(int s=0;s<SIM_WS+SIM_FS;s++) if(simSlotT[s]>0) simSlotT[s]--;
    simFill();
    if(simQ){ if(lnoteT<=0){ lnote=simQ; lnoteT=60; simQ=simQ2; simQ2=0; simQT=240; } else if(--simQT<=0){ simQ=0; simQ2=0; } }
}

// ---- drawing helpers (hud.h and the aspiration panel use them) ----
static u16 simZoneCol(int z){ static const u16 c[6]={RGB(31,4,4),RGB(28,10,6),RGB(24,24,10),RGB(8,26,8),RGB(31,25,6),RGB(24,30,31)}; return c[z]; }
static void simIcon(int x,int y,int ic,u16 c){ for(int r=0;r<7;r++)for(int q=0;q<7;q++) if(simIconArt[ic][r][q]=='#') px(x+q,y+r,c); }
static void simCell(int x,int y,int ic,int fear,int locked,int on){   // a 9x9 want (green) or fear (red) cell, gold when locked; on=0 empty
    u16 edge=locked?RGB(31,26,7):fear?RGB(18,5,4):RGB(5,16,6), fill=fear?RGB(10,2,2):RGB(2,9,3);
    rect(x,y,9,9,edge); rect(x+1,y+1,7,7,on?fill:RGB(2,3,6));
    if(on) simIcon(x+1,y+1,ic,fear?RGB(31,20,18):RGB(22,31,20));
}
static void simMeterBar(int x,int y,int w,int h){   // the aspiration meter, Sims colours: the zones as a dim track, the fill in the zone's colour
    rect(x,y,w,h,RGB(1,2,5));
    for(int z=0;z<6;z++){ int a=z==0?0:simZoneAt[z-1], b=z==5?1000:simZoneAt[z]; rect(x+a*w/1000,y+h-1,(b-a)*w/1000,1,shade(simZoneCol(z),6)); }
    int f=simMeter*w/1000; if(f>0){ u16 c=simZoneCol(simZoneOf(simMeter)); rect(x,y,f,h-1,c); rect(x,y,f,1,lite(c,22)); }
}
