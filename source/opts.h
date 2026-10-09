// opts.h - the extended options. Every option here is one small number in xo[], so adding one is cheap:
//
//   1. add a name to the XO_ enum (ALWAYS at the end: saves are stored by position),
//   2. add its choice count to xoCnt[] and its default to xoDef[] (same position),
//   3. read xo[XO_X] (or a helper below) where the game uses it,
//   4. add a row for it in the page tables in optscreen.h (XR(...) line plus a label list).
//
// The values are saved to SRAM by optsSave() (main.c) next to the old settings block, and loaded with a range check on
// every value, so an older save (fewer options) just gets the defaults for the new ones.
// Needs before it: u8 / u16 (stdint) and RGB().
enum {
    // PLAY
    XO_NEED,      // how fast REST / CLEAN / COMFY drain: OFF SLOW NORMAL FAST BRUTAL
    XO_DAY,       // length of a game day: 3 / 6 / 12 / 24 real minutes, or the clock stopped
    XO_JOB,       // career on / off (off = no shifts, no quota, no bills)
    XO_BILLS,     // bills taken at midnight: NONE HALF NORMAL DOUBLE
    XO_QUOTA,     // trick-point quota of a shift: EASY NORMAL HARD INSANE
    XO_SCORE,     // trick points multiplier: X0.5 X1 X2 X3
    XO_COMBO,     // how long a combo chain waits for the next trick
    XO_MOODFX,    // SAD / STOKED / BORED change speed and points
    XO_HURT,      // NORMAL, GENTLE (half damage), NO DEATH (a bad fall can never kill)
    XO_HUNGER,    // FOOD and WC speed: OFF SLOW NORMAL FAST
    XO_WANTS,     // wants and fears shown in the HUD
    XO_BUBBLE,    // thought bubble: OFF, URGENT needs only, ALL
    XO_AUTOSAVE,  // the life saves itself (midnight, payday, pause menu, leaving)
    XO_SPEED,     // top speed 80 / 100 / 125 / 150 %
    // INPUT
    XO_BTN,       // NORMAL, A/B swapped, L/R swapped, both
    XO_REPEAT,    // D-pad auto-repeat of the editor cursor: SLOW NORMAL FAST
    // AUDIO
    XO_MUS,       // music volume: FULL HALF QUARTER OFF
    XO_SFX,       // sound effect volume: FULL HALF QUARTER
    XO_TITLEMUS,  // music on the title screen
    // HUD
    XO_CLOCK,     // 24 HOUR, 12 HOUR, HIDDEN
    XO_ACCENT,    // accent colour of menus and the HUD
    XO_TOAST,     // how long messages stay up: SHORT NORMAL LONG
    // ROOMS (editor and slots)
    XO_MINI,      // minimap in the editor
    XO_EDSAVE,    // leaving the editor saves the room
    XO_RESETASK,  // ask before RESET MAP
    XO_SLOTSYNC,  // SAVE MAP also writes into the active slot
    XO_SLOTCONF,  // ask before overwriting, loading over or deleting a slot
    XO_SLOTCONT,  // what a slot saves: ROOM, ROOM + PERSON, ALL (room, person, life)
    XO_SLOTBOOT,  // at power on, the person of the active slot is loaded (the creature is not kept anywhere else)
    XO_AGING,     // how fast the life stages pass: OFF SLOW NORMAL FAST (slow doubles the days of every stage, fast halves them)
    XO_AGEB,      // OPTIONS > TIME > AGES: days as a BABY  (see oStageDays)
    XO_AGEC,      // days as a CHILD
    XO_AGET,      // days as a TEEN
    XO_AGEA,      // days as an ADULT (the last choice is FOREVER: never grows old)
    XO_GAMEMUS,   // AUDIO: random jukebox songs (the checked ones) while you play (off / on)
    XO_FREEWILL,  // PLAY: household Sims you do not control look after themselves: OFF LOW HIGH (house.h)
    XO_PIPEAGE,   // PLAY: who may use the water pipe: ADULTS ONLY, or LATE TEENS (the last quarter of the teen stage, see pipeOk)
    XO_MENUMUS,   // AUDIO: a random jukebox song plays in the main menus (off / on)
    XO_CREMUS,    // AUDIO: a chiptune loop plays in the creature creator (off / on)
    XO_GAMEXF,    // AUDIO: GAME MUSIC crossfades into the next song (on) or starts it at once (off)
    XO_MASTER,    // AUDIO: master volume slider, 0..10 (music and effects)
    XO_MUSV,      // AUDIO: music volume slider, 0..10 (replaces XO_MUS, which is now unused: saves are stored by position)
    XO_SFXV,      // AUDIO: sound effect volume slider, 0..10 (replaces XO_SFX, now unused)
    XO_SIMPRE,    // PLAY: pre-made Sims (the MOVE IN families) allowed: OFF / ON
    XO_SIMUSER,   // PLAY: user-made Sims (made in the creator, added with ADD TO FAMILY) allowed: OFF / ON
    XO_SIMRAND,   // PLAY: made-up Sims (INVITE A NEW SIM, SELECT on RELATIONSHIPS, passers-by) allowed: OFF / ON
    XO_ZOOM,      // VIDEO: the room while you play: OFF, 1.25X, 1.5X, 1.7X, 2X (closer, and only that part of the room is drawn: faster)
    XO_MENUBG,    // HUD: the main menu's backdrop: RANDOM (a new pick each visit), TOWN (your town at the time of day), ACID (the acid rainbow)
    XO_MCSLIDE,   // SIM > MASTER (debug code only, a nod to The Sims' Master Controller): the size sliders go twice as far: NORMAL / DOUBLE
    XO_MCBOX,     // SIM > MASTER: every age builds in the adult box (6 x 4 x 8) and taller adults stretch further: BY AGE / LIMIT BREAK
    XO_TUTOR,     // PLAY: the tutorial: OFFER (asks once at the first PLAY), DONE, REPLAY (starts it at the next PLAY / when you leave the pause menu options)
    XO_WEATHER,   // TIME > DAY: AUTO (follows the season and the day), or force CLEAR / CLOUDY / FOG / RAIN / STORM / SNOW (fx.h)
    XO_GHOSTS,    // SIM > BORES: OFF / ON (a ghost stays where you die) / HAUNTED (one is always around) (fx.h)
    XO_BUYCOST,   // ROOMS: BUILD / BUY mode prices walls and items against the life's cash (selling gives it back); OFF = free (the DeadSet always costs)
    XO_SAVEMODE,  // SAVING: AUTO (leaving play saves the player) or MANUAL (default: only SAVE GAME keeps progress; quitting asks, a power cut loses it)
    XO_NIGHT,     // TIME > DAY: how dark the night is in play: OFF SOFT NORMAL DEEP (fx.h: the room dims by the hour; windows show sky, dusk and stars)
    XO_MULTIFL,   // SIM > BORES: household Sims use the stairs to reach the furniture their needs call for (house.h). OFF: they vanish upstairs for a while
    XO_ITEMUSE,   // SIM > BORES: household Sims use the TV, bookshelf, aquarium, treadmill, stereo, coffee maker and phone for their needs (house.h, item modules). OFF: only the five basic furniture needs
    XO_INMATES,   // SIM > BORES: how many inmates the prison holds: LOW 8, MEDIUM 16, HIGH 24 (inmates.h)
    XO_TWINS,     // SIM > BORES: how often a baby comes with a twin (family.h): NEVER SOMETIMES OFTEN ALWAYS
    XO_AMBV,      // AUDIO: NATURE SOUNDS slider 0..10, the outdoor ambience (main.c ambMix: birds by day, crickets at night, rain, wind)
    XO_N
};
static const u8 xoCnt[]={ 5,5,2,4,4,4,4,2,3,4,2,3,2,4,   4,3,   4,3,2,   3,6,3,   2,2,2,2,2,3,2, 4,10,10,10,11, 2, 3, 2, 2, 2, 2, 11, 11, 11, 2, 2, 2, 5, 3, 2, 2, 3, 7, 3 , 2, 2, 4, 2, 2, 3, 4, 11};
static const u8 xoDef[]={ 2,1,1,2,1,1,1,1,0,2,1,1,1,1,   0,1,   0,0,1,   0,0,1,   1,1,1,0,1,2,1, 2,1,2,2,4, 0, 2, 1, 1, 1, 1, 10, 10, 10, 1, 1, 1, 0, 0, 0, 0, 0, 0, 1 , 1, 1, 2, 1, 1, 1, 1, 6};
_Static_assert(sizeof(xoCnt)==XO_N&&sizeof(xoDef)==XO_N,"xoCnt / xoDef must have one entry per XO_ name");
static u8 xo[XO_N];
static void optsDefaults(void){ for(int i=0;i<XO_N;i++) xo[i]=xoDef[i]; }

// ---- accent colour (the GOLD of menus, HUD numbers and the editor) ----
#define NACC 6
static const u16 accentTab[NACC]={ RGB(31,26,6), RGB(12,28,16), RGB(14,24,31), RGB(31,16,24), RGB(31,18,4), RGB(24,18,31) };
static const char* const accentNm[NACC]={"GOLD","MINT","SKY","PINK","ORANGE","LILAC"};

// ---- helpers: the number each choice stands for ----
static inline int oNeedPct(void){ static const u8 t[5]={0,50,100,150,200}; return t[xo[XO_NEED]]; }
static inline int oStepsMin(void){ static const u8 t[5]={7,15,30,60,0}; return t[xo[XO_DAY]]; }   // logic steps per game minute, 0 = stopped
static inline int oStageDays(int st){   // game days a stage lasts before the next one (0 = for ever); ELDER is the last stage and has no row
    static const u8 t[10]={1,2,3,5,7,10,14,21,30,60};
    if(st<0||st>3) return 0;
    int i=xo[XO_AGEB+st]; return i<10?t[i]:0; }
static inline int oBillsPct(void){ static const u8 t[4]={0,50,100,200}; return t[xo[XO_BILLS]]; }
static inline int oQuotaPct(void){ static const u8 t[4]={60,100,150,200}; return t[xo[XO_QUOTA]]; }
static inline int oScorePct(void){ static const u16 t[4]={50,100,200,300}; return t[xo[XO_SCORE]]; }
static inline int oComboLen(void){ static const u16 t[4]={90,150,240,360}; return t[xo[XO_COMBO]]; }   // steps (60 per second)
static inline int oSpeedPct(void){ static const u8 t[4]={80,100,125,150}; return t[xo[XO_SPEED]]; }
static inline int oFoodEvery(void){ static const u16 t[4]={0,240,120,60}; return t[xo[XO_HUNGER]]; }  // steps per FOOD point, 0 = never
static inline int oWcEvery(void){ static const u16 t[4]={0,200,100,50}; return t[xo[XO_HUNGER]]; }
static inline int oToastLen(void){ static const u8 t[3]={25,45,90}; return t[xo[XO_TOAST]]; }
static inline int oRepDelay(void){ static const u8 t[3]={12,7,4}; return t[xo[XO_REPEAT]]; }       // frames held before the cursor repeats
static inline int oRepMask(void){ static const u8 t[3]={3,1,0}; return t[xo[XO_REPEAT]]; }          // repeats when (held & mask) == 0
// Volume sliders: 0..10 steps on a curve that sounds even (a straight line would be too loud too early). Music and effects are each
// multiplied with the master slider. The result is a gain 0..256 (256 = full): a sample is scaled by (sample*gain)>>8.
static const u16 volTab[11]={0,8,17,28,42,60,84,114,150,198,256};
static inline int oMusGain(void){ return (volTab[xo[XO_MUSV]]*volTab[xo[XO_MASTER]])>>8; }
static inline int oSfxGain(void){ return (volTab[xo[XO_SFXV]]*volTab[xo[XO_MASTER]])>>8; }
static inline int oAmbGain(void){ return (volTab[xo[XO_AMBV]]*volTab[xo[XO_MASTER]])>>8; }
#define GOLD (accentTab[xo[XO_ACCENT]])
