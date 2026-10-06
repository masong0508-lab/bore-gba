// optscreen.h - the OPTIONS screen (settingsScreen). Eight pages, every row is one choice or one action. TIME is made of three sections (DAY, AGES, TIMERS).
//
//   L / R  change page        UP / DOWN  pick a row        LEFT / RIGHT (or A)  change it
//   TIME page: UP from its top row (or DOWN from its last) puts the cursor on the section strip, LEFT / RIGHT switch section, DOWN goes back
//   SELECT  put the row back to its normal value           B or START  back (everything is saved)
//   A gold dot on the right of a row means it is not at its normal value.
//
// HOW TO ADD AN OPTION: add it to opts.h (see the steps there), give it a label list below and an XR(...) row in the page it
// belongs to. HOW TO ADD A PAGE: add a row table, a line in optPages[] and raise NOPG.
// This file is included from main.c at the old SETTINGS screen's place: it needs presetNm/presetOf/setPreset/setDefaults/autoTune/
// measureDraw/capLevel/costCache from there, slots.h for the confirm menus, and tmStart()/R_TM2CNT.
enum { OR_VAR, OR_XO, OR_PRESET, OR_ACT };   // a plain u8 variable, an xo[] option, the preset, an action
enum { OA_TUNE, OA_LIFESAVE, OA_LIFEERASE, OA_ROOMERASE, OA_SLOTSERASE, OA_ALLERASE, OA_SRAMTEST, OA_RESET, OA_BTNTEST, OA_CLEAN };
static void cacheFlush(void);   // CLEAR CACHES (main.c, next to liveInvalidate: it needs the room, sprite-slot and path state)
typedef struct { u8 kind, idx, n, def; u8*v; const char*nm; const char* const* lab; const char*d0; const char*d1; } OptRow;
#define VR(var,n,def,nm,lab,d0,d1) {OR_VAR,0,n,def,&var,nm,lab,d0,d1}
#define XR(i,nm,lab,d0,d1) {OR_XO,i,0,0,0,nm,lab,d0,d1}
#define SR(i,nm,d0,d1) XR(i,nm,lbPct,d0,d1)   // a slider row: 0..10 steps, drawn as a bar, LEFT / RIGHT stop at the ends
#define AR(a,nm,d0,d1) {OR_ACT,a,0,0,0,nm,0,d0,d1}
// gInPlay (main.c): 1 while the life game runs (some actions only make sense, or are only safe, in one place)

static const char* const lbFps[4]={"60 FPS","30 FPS","20 FPS","15 FPS"}, *const lbWalls[3]={"FULL","CUTAWAY","LOW"}, *const lbPat[2]={"PLAIN","PATTERNS"},
    *const lbOnOff[2]={"OFF","ON"}, *const lbFree[3]={"OFF","LOW","HIGH"}, *const lbPipe[2]={"ADULTS ONLY","LATE TEENS"}, *const lbShow[3]={"OFF","FPS","DETAIL"}, *const lbWarn[2]={"ON","OFF"}, *const lbRom[2]={"FAST","SAFE"},
    *const lbCam[4]={"OFF","OVER 10000","OVER 5000","OVER 2000"}, *const lbHud[3]={"FULL","SLIM","OFF"};
static const char* const lbNeed[5]={"OFF","SLOW","NORMAL","FAST","BRUTAL"}, *const lbHunger[4]={"OFF","SLOW","NORMAL","FAST"},
    *const lbDay[5]={"3 MIN","6 MIN","12 MIN","24 MIN","STOPPED"}, *const lbBills[4]={"NONE","HALF","NORMAL","DOUBLE"},
    *const lbQuota[4]={"EASY","NORMAL","HARD","INSANE"}, *const lbScore[4]={"X0.5","X1","X2","X3"},
    *const lbCombo[4]={"1.5 SEC","2.5 SEC","4 SEC","6 SEC"}, *const lbSpeed[4]={"80 %","100 %","125 %","150 %"},
    *const lbDays[10]={"1 DAY","2 DAYS","3 DAYS","5 DAYS","7 DAYS","10 DAYS","14 DAYS","21 DAYS","30 DAYS","60 DAYS"},
    *const lbDaysA[11]={"1 DAY","2 DAYS","3 DAYS","5 DAYS","7 DAYS","10 DAYS","14 DAYS","21 DAYS","30 DAYS","60 DAYS","FOREVER"}, *const lbAging[4]={"OFF","SLOW","NORMAL","FAST"}, *const lbHurt[3]={"NORMAL","GENTLE","NO DEATH"}, *const lbBubble[3]={"OFF","URGENT","ALL"}, *const lbShown[2]={"HIDDEN","SHOWN"}, *const lbSaveMode[2]={"AUTO","MANUAL"};
static const char* const lbTut[3]={"OFFER","DONE","REPLAY"};
static const char* const lbNight[4]={"OFF","SOFT","NORMAL","DEEP"}, *const lbWeather[7]={"AUTO","CLEAR","CLOUDY","FOG","RAIN","STORM","SNOW"}, *const lbGhost[3]={"OFF","ON","HAUNTED"};
static const char* const lbPct[11]={"0 %","10 %","20 %","30 %","40 %","50 %","60 %","70 %","80 %","90 %","100 %"};   // the volume sliders (rows made with SR)
static const char* const lbBtn[4]={"NORMAL","A B SWAPPED","L R SWAPPED","BOTH SWAPPED"}, *const lbRep[3]={"SLOW","NORMAL","FAST"},
    *const lbClock[3]={"24 HOUR","12 HOUR","HIDDEN"}, *const lbToast[3]={"SHORT","NORMAL","LONG"},
    *const lbCont[3]={"ROOM","ROOM+PERSON","ALL THREE"}, *const lbZoom[5]={"OFF","1.25X","1.5X","1.7X","2X"}, *const lbMenuBg[3]={"RANDOM","TOWN","ACID RAINBOW"};

static const OptRow pgVideo[]={
 {OR_PRESET,0,0,0,0,"PRESET",0,"LOOKS BALANCED SPEED BATTERY  ONE TAP SETUP","CHANGING ANYTHING BELOW MAKES IT CUSTOM"},
 AR(OA_TUNE,"AUTO TUNE","PRESS A  TESTS YOUR SCREEN AND PICKS THE","PRETTIEST PRESET THAT STAYS SMOOTH"),
 VR(sFps,4,1,"FRAME RATE",lbFps,"HOW OFTEN THE PICTURE REDRAWS","LOWER IS FASTER  THE GAME KEEPS ITS PACE"),
 VR(sWall,3,1,"WALLS",lbWalls,"FULL SHOWS EVERY WALL  CUTAWAY LOWERS THE","WALLS IN FRONT  LOW DRAWS THEM ALL SHORT"),
 VR(sWp,2,1,"WALLPAPER",lbPat,"PATTERNED OR PLAIN COLOUR WALLS","PLAIN IS QUICKER TO DRAW"),
 VR(sFl,2,1,"FLOORS",lbPat,"PATTERNED OR PLAIN COLOUR FLOORS","PLAIN IS QUICKER TO DRAW"),
 VR(sShad,2,1,"SHADOWS",lbOnOff,"THE DARK SPOT UNDER YOUR FEET","OFF SAVES A LITTLE DRAWING"),
 VR(sShow,3,0,"PERFORMANCE INFO",lbShow,"SHOWS FPS WHILE YOU PLAY  DETAIL ALSO SHOWS","LOAD  100 MEANS A FRAME IS JUST FITTING"),
 VR(sNoWarn,2,0,"SPEED WARNING",lbWarn,"ON SHOWS TOO SLOW WHEN THE PICTURE","CANT KEEP UP  OFF HIDES THE WARNING"),
 XR(XO_ZOOM,"ZOOM",lbZoom,"A CLOSER VIEW WHILE YOU PLAY  ONLY THAT PART","IS DRAWN SO IT RUNS FASTER  SELECT+UP DOWN"),
 VR(sRom,2,0,"ROM SPEED",lbRom,"FAST IS RIGHT FOR MOST CARTS AND EMULATORS","SAFE IF A FLASH CART FREEZES OR GLITCHES"), AR(OA_CLEAN,"DEBUG CLEAR CACHES","KONAMI DEBUG  NOT A REAL CACHE DELETER","IT WONT SPEED UP THE GAME  A TO READ MORE"),   // LAST row of the page: hidden until the Konami code is on (pgRows)
};
// ---- SIM: everything about your Sim, the household and the score, in sections (like TIME) ----
static const OptRow pgSimNeeds[]={
 XR(XO_NEED,"NEEDS",lbNeed,"HOW FAST REST CLEAN AND COMFY RUN DOWN","OFF FREEZES THEM  BRUTAL IS TWICE AS FAST"),
 XR(XO_HUNGER,"FOOD AND WC",lbHunger,"HOW FAST HUNGER AND THE BLADDER BUILD","OFF MEANS NO ACCIDENTS AND NO FAINTING"),
 XR(XO_MOODFX,"MOOD EFFECTS",lbOnOff,"SAD SLOWS YOU  STOKED SPEEDS YOU UP AND","MOOD CHANGES TRICK POINTS  OFF IGNORES IT"),
 XR(XO_HURT,"HURT",lbHurt,"GENTLE HALVES FALL DAMAGE","NO DEATH MEANS A FALL CAN NEVER KILL"),
};
static const OptRow pgSimJob[]={
 XR(XO_JOB,"CAREER",lbOnOff,"OFF REMOVES SHIFTS QUOTAS PAY AND BILLS","A FREE PLAY LIFE WITH NO WORK"),
 XR(XO_QUOTA,"JOB QUOTA",lbQuota,"TRICK POINTS NEEDED IN A SHIFT","EASY 60  NORMAL 100  HARD 150  INSANE 200"),
 XR(XO_BILLS,"BILLS",lbBills,"WHAT THE BILL AT MIDNIGHT COSTS","NONE  HALF  NORMAL  OR DOUBLE"),
};
static const OptRow pgSimScore[]={
 XR(XO_SCORE,"SCORE",lbScore,"MULTIPLIES EVERY TRICK AND GRIND SCORE","QUOTAS AND PAY FOLLOW THE SCORE"),
 XR(XO_SPEED,"TOP SPEED",lbSpeed,"HOW FAST YOU WALK RUN AND SKATE","80 TO 150 % OF NORMAL"),
 VR(sCam,4,1,"ACTION CAM",lbCam,"A BIG COMBO ZOOMS AND SPINS THE CAMERA","ALL 4 VIEWS  PICK HOW BIG A COMBO TRIGGERS IT"),
};
static const OptRow pgSimSims[]={
 XR(XO_FREEWILL,"FREE WILL",lbFree,"SIMS YOU DO NOT CONTROL LOOK AFTER","THEMSELVES  LOW WAITS LONGER  OFF STANDS"),
 XR(XO_SIMPRE,"PRE-MADE SIMS",lbOnOff,"THE FAMILIES THAT MOVE IN FROM THE","HOUSEHOLD MENU  OFF BLOCKS THEM"),
 XR(XO_SIMUSER,"USER-MADE SIMS",lbOnOff,"SIMS YOU MAKE IN THE CREATOR AND ADD","TO THE FAMILY  OFF BLOCKS ADDING THEM"),
 XR(XO_SIMRAND,"MADE-UP SIMS",lbOnOff,"RANDOM SIMS THAT MOVE IN OR WALK PAST","OFF MEANS NONE OF THEM SHOW UP"),
 XR(XO_GHOSTS,"GHOSTS",lbGhost,"A GHOST STAYS WHERE YOU DIE AND HAUNTS","HAUNTED KEEPS ONE AROUND TO TEST WITH"),
};
static const char* const lbMcSl[2]={"NORMAL","DOUBLE"}, *const lbMcBox[2]={"BY AGE","LIMIT BREAK"};
static const OptRow pgSimMaster[]={   // CHEAT console (debug code only) (the section is hidden without it, and has no effect)
 XR(XO_MCSLIDE,"SIZE SLIDERS",lbMcSl,"DOUBLE  EVERY SIZE TONE AND LENGTH SLIDER","GOES TWICE AS FAR  PARTS STAY ATTACHED"),
 XR(XO_MCBOX,"BODY BOX",lbMcBox,"LIMIT BREAK  EVERY AGE GETS THE ADULT BOX","AND ADULTS STRETCH PAST 8 BLOCKS TALL"),
};
static const OptRow pgSimMind[]={
 XR(XO_BUBBLE,"THOUGHT BUBBLE",lbBubble,"THE BUBBLE OVER YOUR HEAD","URGENT SHOWS ONLY NEEDS  ALL ADDS WANTS"),
 XR(XO_WANTS,"WANTS AND FEARS",lbShown,"THE WANT AND FEAR CELLS IN THE HUD","THEY STILL COUNT WHEN HIDDEN"),
};
// ---- TIME: everything about time, in three sections (UP from the top row picks the section strip, LEFT / RIGHT switch it) ----
static const OptRow pgTimeDay[]={
 XR(XO_DAY,"DAY LENGTH",lbDay,"REAL MINUTES IN ONE GAME DAY  STOPPED","FREEZES THE CLOCK  SLEEP NO LONGER SKIPS TIME"),
 XR(XO_CLOCK,"CLOCK",lbClock,"HOW THE GAME CLOCK IS SHOWN","24 HOUR  12 HOUR  OR HIDDEN"),
 XR(XO_WEATHER,"WEATHER",lbWeather,"AUTO FOLLOWS THE SEASON AND THE DAY  OR","FORCES ONE KIND  RAIN FALLS OUTDOORS ONLY"),
 XR(XO_NIGHT,"NIGHT",lbNight,"HOW DARK THE ROOM GETS AT NIGHT  LAMPS KEEP","INDOORS BRIGHT  OFF KEEPS EVERY HOUR LIT"),
};
static const OptRow pgTimeAges[]={
 XR(XO_AGING,"AGING",lbAging,"OFF KEEPS THE AGE YOU PICKED  SLOW DOUBLES","EVERY STAGE BELOW  FAST HALVES THEM"),
 XR(XO_AGEB,"BABY LASTS",lbDays,"GAME DAYS AS A BABY  THE ONE STAGE YOU","CANNOT STEER"),
 XR(XO_AGEC,"CHILD LASTS",lbDays,"GAME DAYS AS A CHILD","BEFORE GROWING INTO A TEEN"),
 XR(XO_AGET,"TEEN LASTS",lbDays,"GAME DAYS AS A TEEN","THE CAREER STARTS AT THIS STAGE"),
 XR(XO_AGEA,"ADULT LASTS",lbDaysA,"GAME DAYS AS AN ADULT BEFORE BECOMING AN","ELDER  FOREVER NEVER GROWS OLD"),
 XR(XO_PIPEAGE,"PIPE AGE",lbPipe,"WHO MAY USE THE WATER PIPE  LATE TEENS IS","THE LAST QUARTER OF THE TEEN YEARS"),
 XR(XO_TUTOR,"TUTORIAL",lbTut,"OFFER ASKS ONCE AT YOUR FIRST PLAY","DONE IS QUIET  REPLAY STARTS THE LESSONS"),
};
static const OptRow pgTimeTimers[]={
 XR(XO_COMBO,"COMBO WINDOW",lbCombo,"TIME YOU HAVE TO LAND THE NEXT TRICK","BEFORE THE CHAIN IS BANKED"),
 XR(XO_TOAST,"MESSAGE TIME",lbToast,"HOW LONG POP UP MESSAGES STAY ON SCREEN","SHORT  NORMAL  LONG"),
};
static const OptRow pgAudio[]={
 VR(sSnd,2,1,"SOUND",lbOnOff,"SOUND OFF SKIPS SOUND DECODING","SAVES A LITTLE SPEED AND BATTERY"),
 SR(XO_MASTER,"MASTER VOLUME","HOW LOUD EVERYTHING IS  MUSIC AND EFFECTS","ARE EACH SET BELOW  THEN SCALED BY THIS"),
 SR(XO_MUSV,"MUSIC VOLUME","THE TITLE MUSIC  MENU MUSIC AND THE TOUKEBOX","0 SILENCES THEM  ALSO SET IN THE TOUKEBOX"),
 SR(XO_SFXV,"SFX VOLUME","LOUDNESS OF GRUNTS BONKS AND CRIES","0 SILENCES THEM"),
 XR(XO_GAMEMUS,"GAME MUSIC",lbOnOff,"RANDOM TOUKEBOX SONGS WHILE YOU PLAY","MIXING COSTS SPEED  SOUND EFFECTS DUCK IT"),
 XR(XO_GAMEXF,"GAME CROSSFADE",lbOnOff,"GAME MUSIC BLENDS INTO THE NEXT SONG","OFF STARTS EACH SONG AT ONCE"),
 XR(XO_TITLEMUS,"TITLE MUSIC",lbOnOff,"PLAY THE DIPPER MAN ON THE TITLE SCREEN","OFF KEEPS THE TITLE QUIET"),
 XR(XO_MENUMUS,"MENU MUSIC",lbOnOff,"GOTTCHO BARRACHO PLAYS IN THE","MAIN MENUS  OFF KEEPS THEM QUIET"),
 XR(XO_CREMUS,"CREATOR MUSIC",lbOnOff,"A CHIPTUNE LOOP PLAYS IN THE CREATOR","OFF KEEPS IT QUIET")
};
static const OptRow pgInput[]={
 XR(XO_BTN,"BUTTONS",lbBtn,"SWAP A AND B  OR L AND R  ON EVERY SCREEN","USE BUTTON TEST BELOW TO CHECK IT"),
 XR(XO_REPEAT,"CURSOR REPEAT",lbRep,"HOW FAST THE EDITOR CURSOR REPEATS WHEN","YOU HOLD THE D PAD"),
 AR(OA_BTNTEST,"BUTTON TEST","SHOWS WHICH BUTTONS THE GAME SEES","PRESS A  LEAVE WITH SELECT AND START"),
};
static const OptRow pgHud[]={
 VR(sHud,3,0,"INFO ON SCREEN",lbHud,"FULL SHOWS ALL  SLIM KEEPS SCORE AND BARS","OFF HIDES ALL OF IT  ALERTS STILL SHOW"),
 XR(XO_MENUBG,"MENU BACKDROP",lbMenuBg,"BEHIND THE MAIN MENU  YOUR TOWN AT THE TIME","OF DAY  THE ACID RAINBOW  OR EITHER"),
 XR(XO_ACCENT,"ACCENT COLOUR",accentNm,"COLOUR OF MENUS HEADINGS AND HUD NUMBERS","SEE IT CHANGE RIGHT HERE"),
};
static const OptRow pgRooms[]={
 XR(XO_BUYCOST,"BUILD COSTS",lbOnOff,"BUILD AND BUY PRICE WALLS AND ITEMS  SELLING","PAYS YOU BACK  OFF MAKES BUILDING FREE"),
 XR(XO_MINI,"EDITOR MINIMAP",lbOnOff,"THE SMALL MAP IN THE ROOM BUILDER","OFF GIVES A CLEARER VIEW"),
 XR(XO_EDSAVE,"SAVE ON EXIT",lbOnOff,"LEAVING THE ROOM BUILDER SAVES THE ROOM","OFF  USE SAVE MAP WHEN YOU WANT IT"),
 XR(XO_RESETASK,"ASK BEFORE RESET",lbOnOff,"ASK FIRST WHEN YOU CHOOSE RESET MAP","OFF RESETS AT ONCE"),
 XR(XO_SLOTCONT,"SLOTS SAVE",lbCont,"WHAT A SLOT STORES  THE ROOM  ROOM AND","PERSON  OR ROOM PERSON AND LIFE"),
 XR(XO_SLOTCONF,"ASK IN SLOTS",lbOnOff,"ASK BEFORE OVERWRITING LOADING OR DELETING","OFF  DOES IT AT ONCE"),
 XR(XO_SLOTSYNC,"SAVE MAP TO SLOT",lbOnOff,"SAVE MAP IN THE ROOM BUILDER ALSO WRITES","THE ACTIVE SLOT"),
 XR(XO_SLOTBOOT,"BOOT LOADS PERSON",lbOnOff,"AT POWER ON THE CREATURE OF THE ACTIVE","SLOT COMES BACK  THE ROOM IS NOT CHANGED"),
};
static const OptRow pgData[]={
 XR(XO_SAVEMODE,"SAVING",lbSaveMode,"AUTO  LEAVING PLAY SAVES YOUR PLAYER","MANUAL  ONLY SAVE GAME KEEPS PROGRESS"),
 XR(XO_AUTOSAVE,"AUTO SAVE LIFE",lbOnOff,"SAVES AT MIDNIGHT PAYDAY AND THE PAUSE MENU","OFF  ONLY SLOTS AND SAVE LIFE NOW SAVE IT"),
 AR(OA_LIFESAVE,"SAVE LIFE NOW","WRITES THE CURRENT LIFE TO SAVE MEMORY","USE IT FROM THE PAUSE MENU WHILE PLAYING"),
 AR(OA_LIFEERASE,"ERASE LIFE","DELETES THE SAVED LIFE  CASH JOB AND CLOCK","THE NEXT PLAY STARTS A NEW ONE"),
 AR(OA_ROOMERASE,"ERASE SAVED MAP","DELETES THE AUTO SAVED MAP  SLOTS STAY","THE DEFAULT MAP RETURNS AT NEXT POWER ON"),
 AR(OA_SLOTSERASE,"ERASE ALL SLOTS","DELETES EVERY ROOM SLOT","THIS CANNOT BE UNDONE"),
 AR(OA_ALLERASE,"ERASE EVERYTHING","WIPES ALL SAVE MEMORY  MAP SLOTS LIFE","OPTIONS AND TOUKEBOX  MAIN MENU ONLY"),
 AR(OA_SRAMTEST,"SAVE MEMORY TEST","CHECKS THAT THIS CART OR EMULATOR KEEPS","SAVES  A GOOD FIRST TEST ON NEW HARDWARE"),
 AR(OA_RESET,"RESET ALL OPTIONS","PUTS EVERY OPTION BACK TO NORMAL","PRESS A"),
};
typedef struct { const char*nm; const OptRow*r; u8 n; const char*d0; const char*d1; } OptSub;   // a section of a page with sections
typedef struct { const char*nm; const OptRow*r; u8 n; const OptSub*sub; u8 ns; } OptPage;   // ns > 0: the page is made of sections (sub[]), r / n are unused
#define NOPG 8
#define PG(nm,t) {nm,t,(u8)(sizeof(t)/sizeof(t[0])),0,0}
#define PGS(nm,t) {nm,0,0,t,(u8)(sizeof(t)/sizeof(t[0]))}
#define SUB(nm,t,d0,d1) {nm,t,(u8)(sizeof(t)/sizeof(t[0])),d0,d1}
static const OptSub simSubs[]={
 SUB("NEEDS",pgSimNeeds,"HOW FAST NEEDS RUN DOWN  MOOD AND HOW","MUCH A FALL HURTS"),
 SUB("JOB",pgSimJob,"THE CAREER  ITS QUOTA AND THE BILLS","AT MIDNIGHT"),
 SUB("SCORE",pgSimScore,"TRICK SCORES  YOUR TOP SPEED AND THE","ACTION CAM AFTER A BIG COMBO"),
 SUB("BORES",pgSimSims,"WHO MOVES IN OR WALKS PAST AND HOW","THE OTHERS LOOK AFTER THEMSELVES"),
 SUB("MIND",pgSimMind,"THE THOUGHT BUBBLE AND THE WANTS AND","FEARS IN THE HUD"),
 SUB("CHEAT",pgSimMaster,"DEBUG CHEAT CONSOLE  EXTREME BODY SLIDERS","AND A BIGGER BODY BOX  DEBUG CODE ONLY"),   // (keep it last: pgNs hides it)
};
static const OptSub timeSubs[]={
 SUB("DAY",pgTimeDay,"HOW LONG A GAME DAY IS AND HOW THE","CLOCK SHOWS IT"),
 SUB("AGES",pgTimeAges,"HOW FAST EVERY LIFE STAGE PASSES AND WHO","MAY USE THE WATER PIPE"),
 SUB("TIMERS",pgTimeTimers,"HOW LONG THE COMBO CHAIN WAITS AND HOW","LONG POP UP MESSAGES STAY"),
};
static const OptPage optPages[NOPG]={ PG("VIDEO",pgVideo), PGS("SIMU",simSubs), PGS("TIME",timeSubs), PG("AUDIO",pgAudio), PG("INPUT",pgInput), PG("HUD",pgHud), PG("ROOMS",pgRooms), PG("DATA",pgData) };
static int opPage, opFocus; static u8 opSel[NOPG][5], opSub[NOPG];   // opFocus: the cursor is on the section strip; opSel is kept per page and per section
static int pgNs(const OptPage*pg);
static const OptRow* pgRows(const OptPage*pg,int*n){ if(pg->ns){ if(opSub[pg-optPages]>=pgNs(pg)) opSub[pg-optPages]=0; const OptSub*u=&pg->sub[opSub[pg-optPages]]; *n=u->n; return u->r; } *n=pg->n; if(pg->r==pgVideo&&!sUnlock) (*n)--; return pg->r; }   // DEBUG CLEAR CACHES (the last VIDEO row) only shows while the Konami code (sUnlock) is on
static int pgNs(const OptPage*pg){ return pg->ns-((pg->sub==simSubs&&!mcOn())?1:0); }   // sections shown (SIM's MASTER only with the debug code)
static u8* pgSel(const OptPage*pg){ int i=(int)(pg-optPages); return &opSel[i][pg->ns?opSub[i]:0]; }

static u8* rowVar(const OptRow*r){ return r->kind==OR_XO?&xo[r->idx]: r->v; }
static int rowN(const OptRow*r){ return r->kind==OR_XO?xoCnt[r->idx]: r->n; }
static int rowDef(const OptRow*r){ return r->kind==OR_XO?xoDef[r->idx]: r->def; }
static int rowChanged(const OptRow*r){
    if(r->kind==OR_PRESET) return presetOf()!=1;
    if(r->kind==OR_ACT) return 0;
    return *rowVar(r)!=rowDef(r);
}
static int rowCostKey(const OptRow*r){ u8*v=r->v; return r->kind==OR_PRESET||(r->kind==OR_VAR&&(v==&sWall||v==&sWp||v==&sFl||v==&sShad)); }   // rows that change how much a frame costs
static void rowSet(const OptRow*r,int val){   // one place for the side effects of a change
    u8*v=rowVar(r); if(!v) return;
    *v=(u8)val; if(v==&sSnd&&!sSnd) sfxStop(); if(v==&sRom) applyRom();
}
static void rowChange(const OptRow*r,int d){
    sTunedMsg=0;
    if(r->kind==OR_PRESET){ int p=presetOf(); p=(p==4)?(d>0?0:3):(p+d+4)%4; setPreset(p); return; }
    int n=rowN(r);
    if(r->lab==lbPct){ int v=*rowVar(r)+d; if(v<0) v=0; if(v>n-1) v=n-1; rowSet(r,v); return; }   // sliders stop at the ends
    rowSet(r,(*rowVar(r)+d+n)%n);
}
static void rowReset(const OptRow*r){ sTunedMsg=0; if(r->kind==OR_PRESET) setPreset(1); else if(r->kind!=OR_ACT) rowSet(r,rowDef(r)); }
static const char* rowVal(const OptRow*r){
    if(r->kind==OR_PRESET) return presetNm[presetOf()];
    if(r->kind==OR_ACT) return "PRESS A";
    return r->lab[*rowVar(r)];
}
static int rowHeat(const OptRow*r){   // VIDEO page colours: 0 light (green), 1 medium (yellow), 2 heavy (red), 3 neutral
    if(r->kind!=OR_VAR) return 3; u8*v=r->v;
    if(v==&sFps) return sFps==0?2:sFps==1?1:0;
    if(v==&sWall) return sWall==0?2:sWall==1?1:0;
    if(v==&sWp) return sWp?1:0;
    if(v==&sFl) return sFl?1:0;
    if(v==&sShad) return sShad?1:0;
    if(v==&sRom) return sRom?1:0;
    if(v==&sShow) return sShow==2?1:0;
    return 3;
}
static s16 costCache[24] EWRAM_BSS;   // draw cost per walls/wallpaper/floors/shadows combo, 0 = not measured yet
static inline int costKey(void){ return sWall*8+sWp*4+sFl*2+sShad; }

// ---- the look: the Sims 3 style of the main menu (glossy rounded panels, green focus, icon tabs, switches and sliders) ----
// s3Box / s3Panel / s3Pill / s3Tip / disc live further down in main.c (the main menu draws with them too).
static u16 s3Mix(u16 a,u16 b,int t,int n); static void s3Box(int x,int y,int w,int h,int r,u16 top,u16 bot); static void s3Panel(int x,int y,int w,int h);
static void s3Pill(int x,int y,int w,int h,int on,const char*s); static void s3Tip(const char*t);
#define OPN RGB(2,5,11)        // navy ink on the light panel
#define OPF RGB(1,4,0)         // dark green ink on the focused (green) row
static const char* const opIcon[NOPG][7]={   // the page tabs, 7 x 7: VIDEO monitor, SIMU a person, TIME clock, AUDIO speaker, INPUT d-pad, HUD frame, ROOMS house, DATA disk
 {"#######","#.....#","#.....#","#.....#","#######","..###..",".#####."},
 {"..###..",".#####.",".#####.","..###..",".#####.","#######","#######"},
 {".#####.","#..#..#","#..#..#","#..##.#","#.....#","#.....#",".#####."},
 {"...#...","..##.#.","####..#","####..#","..##.#.","...#...","......."},
 {"..###..","..###..","#######","#######","#######","..###..","..###.."},
 {"#######","#######","#.....#","#.###.#","#.....#","#.##..#","#######"},
 {"...#...","..###..",".#####.","#######",".##.##.",".##.##.",".#####."},
 {"#######","#.###.#","#.###.#","#.....#","#.###.#","#.###.#","#######"} };
static const char* const opTitle[NOPG]={"VIDEO AND SPEED","THE SIM","TIME","SOUND","BUTTONS","ON SCREEN","ROOMS","SAVING AND DATA"};
static int pgAnyChanged(const OptPage*pg){
    if(pg->ns){ for(int u=0;u<pg->ns;u++) for(int q=0;q<pg->sub[u].n;q++) if(rowChanged(&pg->sub[u].r[q])) return 1; return 0; }
    for(int q=0;q<pg->n;q++) if(rowChanged(&pg->r[q])) return 1; return 0;
}
static void opSwitch(int x,int y,int on,int foc){   // 22 x 8: a switch, green when on
    s3Box(x,y,22,8,4,foc?RGB(2,8,1):RGB(4,8,16),foc?RGB(1,5,0):RGB(3,6,13));
    s3Box(x+1,y+1,20,6,3,on?RGB(14,28,6):RGB(14,19,26),on?RGB(8,21,3):RGB(10,15,22));
    s3Box(on?x+13:x+1,y+1,8,6,3,RGB(31,31,31),RGB(21,27,31));
}
static void opSlider(int x,int y,int lv,int foc){   // 10 steps over 60 px: a groove, a green fill and a round knob
    s3Box(x,y+3,60,3,1,foc?RGB(2,8,1):RGB(4,8,16),foc?RGB(1,5,0):RGB(8,13,22));
    if(lv>0) s3Box(x+1,y+3,lv*5+4,3,1,RGB(18,30,8),RGB(8,21,3));
    int kx=x+lv*5; s3Box(kx,y,9,8,3,RGB(3,7,15),RGB(3,7,15)); s3Box(kx+1,y+1,7,6,2,RGB(31,31,31),RGB(18,25,31));
}
static void opValueBox(int x,int y,int w,const char*v,int foc){   // a sunken value box, < > at the ends while its row is lit
    s3Box(x,y,w,8,4,foc?RGB(2,8,1):RGB(4,8,16),foc?RGB(1,5,0):RGB(8,13,22));
    s3Box(x+1,y+1,w-2,6,3,RGB(31,31,31),RGB(22,28,31));
    text(x+(w-tw(v,1))/2,y+2,v,OPN,1);
    if(foc){ text(x+3,y+2,"<",OPF,1); text(x+w-7,y+2,">",OPF,1); }
}
static void drawOptions(void){
    static const u16 heat[4]={ RGB(12,28,10), RGB(31,26,6), RGB(30,10,8), RGB(22,24,26) };
    objHideAll();
    for(int y=0;y<SH;y++) rect(0,y,SW,1,s3Mix(RGB(2,5,12),RGB(5,11,24),y,SH));   // the night-blue backdrop
    s3Panel(2,1,236,148);
    s3Box(74,0,92,13,6,RGB(3,8,19),RGB(2,5,13)); text(120-tw("OPTIONS",1)/2,3,"OPTIONS",RGB(26,29,31),1);   // the title cap on the top edge
    for(int i=0;i<NOPG;i++){ int x=13+i*27;   // the icon tabs (a gold dot: something on that page is not at its normal value)
        s3Pill(x,15,25,14,i==opPage,"");
        for(int r=0;r<7;r++)for(int q=0;q<7;q++) if(opIcon[i][r][q]=='#') px(x+9+q,18+r,i==opPage?OPF:OPN);
        if(pgAnyChanged(&optPages[i])) rect(x+20,17,2,2,RGB(31,20,2)); }
    const OptPage*pg=&optPages[opPage]; int nr; const OptRow*rows=pgRows(pg,&nr); int sel=*pgSel(pg), y0=47, vis=7, foc=(pg->ns&&opFocus);
    text(12,34,opTitle[opPage],OPN,1);   // the page header with its rule, like the Sims 3 options
    if(pg->ns){   // the sections (DAY | AGES | TIMERS): small pills on the right of the header. UP from the top row picks them, LEFT / RIGHT switch
        int x=226, cs=opSub[opPage];
        for(int u=pgNs(pg)-1;u>=0;u--){ const OptSub*su=&pg->sub[u]; int w=tw(su->nm,1)+10, chg=0; x-=w;
            for(int q=0;q<su->n;q++) if(rowChanged(&su->r[q])) chg=1;
            if(u==cs&&!foc){ s3Box(x,31,w,10,5,RGB(3,8,19),RGB(2,5,13)); text(x+5,33,su->nm,RGB(26,29,31),1); }
            else s3Pill(x,31,w,10,u==cs&&foc,su->nm);
            if(chg) rect(x+w-4,32,2,2,RGB(31,20,2));
            x-=2; }
    }
    rect(10,43,220,1,OPN); rect(10,44,220,1,RGB(29,31,31));
    if(opPage==0){   // the speed meter in a dark well: how much of the frame the picture needs. 60 / 30 / 20 marks show which frame rate it can hold.
        int cap=capLevel(), want=sFps+1, fill=sCost*100/(3*TICKS_FRAME); if(fill>100) fill=100;
        u16 mc=cap==1?heat[0]:cap<=2?heat[1]:heat[2];
        s3Box(8,47,218,17,5,RGB(3,8,19),RGB(2,5,13));
        text(13,50,"DRAW COST",RGB(18,24,30),1);
        rect(62,50,88,5,RGB(8,10,14)); rect(62,50,fill*88/100,5,mc);
        static const int mk[3]={20,50,79}; static const char* const ml[3]={"60","30","20"};
        for(int i=0;i<3;i++){ rect(62+mk[i],49,1,7,WHITE); text(62+mk[i]-3,56,ml[i],RGB(18,24,30),1); }
        if(cap>want){ if(!sNoWarn){ text(157,49,"TOO SLOW FOR",heat[2],1); text(157,56,"THIS FRAME RATE",heat[2],1); } }
        else { text(157,49,cap==1?"HOLDS 60 FPS":cap==2?"HOLDS 30 FPS":cap==3?"HOLDS 20 FPS":"HOLDS 15 FPS",heat[0],1); text(157,56,sTunedMsg?"TUNED FOR YOU":"SMOOTH",sTunedMsg?heat[0]:RGB(18,24,30),1); }
        y0=67; vis=5;
    }
    int top=sel-vis/2; if(top>nr-vis) top=nr-vis; if(top<0) top=0;
    for(int n=0;n<vis&&top+n<nr;n++){
        int i=top+n, y=y0+n*11; const OptRow*r=&rows[i]; int cs=(i==sel&&!foc), ch=rowChanged(r);   // (no row is lit while the cursor is on the sections)
        if(cs){ s3Box(9,y,216,10,5,RGB(4,10,2),RGB(3,8,1)); s3Box(10,y+1,214,8,4,RGB(21,30,9),RGB(10,22,2)); s3Box(14,y+1,206,3,1,RGB(25,31,15),RGB(21,30,9)); }
        else rect(12,y+10,210,1,RGB(17,24,30));
        u16 ink=cs?OPF:OPN;
        if(ch) rect(11,y+4,2,2,cs?RGB(24,12,0):RGB(31,20,2));
        text(16,y+2,r->nm,ink,1);
        if(opPage==0&&rowHeat(r)<3){ u16 hc=heat[rowHeat(r)]; s3Box(116,y+2,6,6,3,RGB(2,5,11),RGB(2,5,11)); s3Box(117,y+3,4,4,2,hc,hc); }   // an LED: green fast, yellow mid, red slow
        if(r->kind==OR_ACT){ int a=r->idx, bad=(a==OA_LIFEERASE||a==OA_ROOMERASE||a==OA_SLOTSERASE||a==OA_ALLERASE);
            s3Box(163,y+1,58,8,4,bad?RGB(12,2,2):RGB(4,8,16),bad?RGB(8,1,1):RGB(3,6,13));
            s3Box(164,y+2,56,6,3,bad?RGB(31,18,14):RGB(31,31,31),bad?RGB(27,8,6):RGB(18,25,31));
            text(163+(58-tw("PRESS A",1))/2,y+3,"PRESS A",bad?RGB(10,1,1):OPN,1); }
        else if(r->lab==lbPct){ int lv=*rowVar(r); opSlider(127,y+1,lv,cs); text(221-tw(rowVal(r),1),y+2,rowVal(r),ink,1); }
        else if(r->kind!=OR_PRESET&&rowN(r)==2){ opSwitch(199,y+1,*rowVar(r)==1,cs); text(194-tw(rowVal(r),1),y+2,rowVal(r),ink,1); }
        else opValueBox(125,y+1,96,rowVal(r),cs);
    }
    if(nr>vis){ int th=(vis*11-1)*vis/nr; if(th<8) th=8; int ty=y0+((vis*11-1)-th)*top/(nr-vis);   // the scroll thumb
        s3Box(229,y0,3,vis*11-1,1,RGB(4,8,16),RGB(8,13,22)); s3Box(229,ty,3,th,1,RGB(31,31,31),RGB(18,25,31)); }
    s3Box(8,126,224,19,5,RGB(3,8,19),RGB(2,5,13));   // the description well
    if(foc){ const OptSub*su=&pg->sub[opSub[opPage]]; text(13,129,su->d0,RGB(26,29,31),1); text(13,137,su->d1,RGB(15,21,28),1); }
    else{ text(13,129,rows[sel].d0,RGB(26,29,31),1); text(13,137,rows[sel].d1,RGB(15,21,28),1); }
    s3Tip(foc?"LEFT RIGHT SECTION  DOWN ROWS  L R PAGE  B BACK":pg->ns?"L R PAGE  UP AT TOP FOR SECTIONS  B BACK":"L R PAGE  < > CHANGE  SELECT RESET  B BACK");
}

static void buttonTest(void){
    static const char* const nm[10]={"A","B","SELECT","START","RIGHT","LEFT","UP","DOWN","R","L"};   // same order as the K_ bits
    while((~REG_KEYINPUT)&0x3FF) vsync();
    for(;;){
        u16 k=keyNow(); if((k&K_SEL)&&(k&K_START)) break;
        fillCols(0,ROW_W,RGB(3,4,8)); box(4,2,232,156); text(12,7,"BUTTON TEST",GOLD,1); text(12,18,"THE KEYS THE GAME SEES",DIMC,1);
        for(int i=0;i<10;i++){ int x=14+(i%5)*44, y=40+(i/5)*30, on=(k>>i)&1;
            rect(x,y,40,20,on?GOLD:RGB(5,6,10)); text(x+4,y+7,nm[i],on?RGB(4,3,6):DIMC,1); }
        text(12,112,"PRESS SELECT AND START TOGETHER TO LEAVE",WHITE,1);
        present();
    }
    while((~REG_KEYINPUT)&0x3FF) vsync();
}
static void optAction(int a,int*remeasure){
    switch(a){
        case OA_TUNE: autoTune(); costCache[costKey()]=(s16)sCost; *remeasure=0; break;
        case OA_RESET: if(menu("RESET ALL OPTIONS",slYesNo,2)==1){ optsDefaults(); setDefaults(); sTunedMsg=0; *remeasure=1; toast("OPTIONS RESET"); } break;
        case OA_BTNTEST: buttonTest(); break;
        case OA_CLEAN:{
            if(!sUnlock) break;   // (the row is hidden without the Konami code; this is only a second lock)
            static const char* const dis[11]={">NOT A REAL CACHE DELETER","THE GBA HAS NO CACHE PILE UP TO CLEAR","AND THE GAME KEEPS NO HIDDEN ASSETS IN RAM","IT ONLY DROPS SMALL SPEED UP COPIES THAT","THE GAME BUILDS AGAIN BY ITSELF",">WHAT IT CAN DO","FIX A GLITCHED SPRITE OR A STALE REDRAW",">WHAT IT CANNOT DO","RAISE YOUR FRAME RATE OR FREE UP RAM","FOR REAL SPEED USE FRAME RATE WALLS","WALLPAPER FLOORS AND SHADOWS"};
            helpScreen("DEBUG CLEAR CACHES",dis,11);
            if(menu("RUN IT ANYWAY",slYesNo,2)!=1) break; }
            cacheFlush(); for(int i=0;i<24;i++) costCache[i]=0; sTunedMsg=0; *remeasure=1; toast("CACHES CLEARED"); break;   // the speed meter measures again by itself a moment later
        case OA_LIFESAVE:
            if(!gInPlay) toast("USE THIS FROM THE PAUSE MENU");
            else { simsSaveNow(); toast("LIFE SAVED"); } break;
        case OA_LIFEERASE: if(menu("ERASE THE SAVED LIFE",slYesNo,2)==1){ SIM_SRAM[0]=0; svCommit(); toast("SAVED LIFE ERASED"); } break;
        case OA_ROOMERASE: if(menu("ERASE THE SAVED MAP",slYesNo,2)==1){ svWr(0,0); toast("SAVED MAP ERASED"); } break;
        case OA_SLOTSERASE: if(menu("ERASE ALL ROOM SLOTS",slYesNo,2)==1){ slEraseAll(); toast("ALL SLOTS ERASED"); } break;
        case OA_ALLERASE:
            if(gInPlay){ toast("USE THIS FROM THE MAIN MENU"); break; }
            if(menu("ERASE ALL SAVE MEMORY",slYesNo,2)==1 && menu("REALLY ERASE EVERYTHING",slYesNo,2)==1){
                svEraseAll();
                optsDefaults(); setDefaults(); settingsSave(); mapReset(); for(int i=0;i<LK_N;i++) look[i]=0; stage=AG_ADULT; ageDays=0; starter(); setColors(); jbSetup(); *remeasure=1;
                toast("EVERYTHING ERASED"); }
            break;
        case OA_SRAMTEST:{
            volatile u8*m=SRAM_BASE+SRAM_TEST; int ok=1; svErr=0;
            for(int i=0;i<16;i++) m[i]=(u8)(i*37+0x5A);
            svCommit(); for(int i=0;i<16;i++) if(svChip(SRAM_TEST+i)!=(u8)(i*37+0x5A)) ok=0;   // read back from the chip itself
            for(int i=0;i<16;i++) m[i]=0;
            svCommit(); if(svErr) ok=0;
            static char tb[32] EWRAM_BSS; char*e=tb; for(const char*p=svName();*p;) *e++=*p++; for(const char*p=" WORKS";*p;) *e++=*p++; *e=0;
            toast(ok?tb:"NO SAVE MEMORY HERE"); } break;
    }
}

static void settingsScreen(void){   // the OPTIONS screen (the name stays so every caller keeps working)
    int dirty=1, remeasure=1, idle=0; u16 prev=keyNow(); tmStart(); sTunedMsg=0; opFocus=0;
    for(int i=0;i<24;i++) costCache[i]=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_L|K_R)){ opPage=(opPage+((pr&K_R)?1:NOPG-1))%NOPG; opFocus=0; dirty=1; if(opPage==0){ int c=costCache[costKey()]; if(c){ sCost=c; remeasure=0; } else remeasure=1; } }
        const OptPage*pg=&optPages[opPage]; int nr; const OptRow*rows=pgRows(pg,&nr); u8*sel=pgSel(pg);
        if(pg->ns&&opFocus){   // on the section strip: LEFT / RIGHT (or A) switch the section, DOWN / UP go back to the rows
            if(pr&(K_LEFT|K_RIGHT|K_A)){ int ns=pgNs(pg); opSub[opPage]=(u8)((opSub[opPage]+((pr&K_LEFT)?ns-1:1))%ns); dirty=1; }
            if(pr&(K_DOWN|K_UP)){ opFocus=0; rows=pgRows(pg,&nr); sel=pgSel(pg); *sel=(pr&K_DOWN)?0:(u8)(nr-1); dirty=1; }
        } else {
            if(pr&K_DOWN){ if(pg->ns&&*sel==nr-1) opFocus=1; else *sel=(u8)((*sel+1)%nr); dirty=1; }
            if(pr&K_UP){ if(pg->ns&&*sel==0) opFocus=1; else *sel=(u8)((*sel+nr-1)%nr); dirty=1; }
            if(!opFocus){
                const OptRow*r=&rows[*sel];
                if(r->kind==OR_ACT){ if(pr&K_A){ optAction(r->idx,&remeasure); prev=keyNow(); dirty=1; } }
                else {
                    int d=((pr&K_RIGHT)?1:0)-((pr&K_LEFT)?1:0); if(pr&K_A) d=1;
                    if(d) rowChange(r,d); else if(pr&K_SEL) { rowReset(r); d=1; }
                    if(d){ dirty=1; idle=0;
                        if(rowCostKey(r)){ int c=costCache[costKey()]; if(c){ sCost=c; remeasure=0; } else remeasure=1; } }
                }
            }
        }
        if(pr&(K_B|K_START)){ settingsSave(); R_TM2CNT=0; return; }
        if(dirty){ drawOptions(); present(); dirty=0; idle=0; }
        else { vsync(); if(remeasure&&opPage==0&&++idle>=20){ sCost=measureDraw(); costCache[costKey()]=(s16)sCost; remeasure=0; dirty=1; } }
    }
}
static const char* const optHelp[12]={">OPTIONS",">PAGES  L AND R","VIDEO  SPEED AND LOOKS   PLAY  THE LIFE SIM","TIME  SECTIONS  UP AT THE TOP ROW PICKS ONE",">ROWS","UP DOWN PICK  LEFT RIGHT CHANGE","SELECT PUTS A ROW BACK TO NORMAL","A GOLD DOT MARKS A CHANGED ROW",">SAFE TO TRY","RESET ALL OPTIONS IS ON THE DATA PAGE","B OR START GOES BACK AND SAVES",""};
