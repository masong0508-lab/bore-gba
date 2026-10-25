// venues.h - PLACES WITH THINGS TO DO: fifteen more kinds of community lot, each with its own room, its own counter and its own menu.
//   TACO BORE (the munchies' natural enemy), GYM, ARCADE, NIGHTCLUB, GROCERY, BANK, HOSPITAL, CAFE, BAR, SALON, PET SHOP, LIBRARY, CASINO,
//   CINEMA, POOL. Table driven: a Venue says how the place looks (wallpaper, floor, props, the town icon's colours) and which actions its
//   counter offers; a VAct is one action: its price, what it does to your needs, a skill, how long it keeps you busy, who may do it, and a
//   SPECIAL for the few that do more (adopt a pet, a movie, a bet, a hairdo, a shift of work, robbing the bank ...).
//   THE COUNTER  one tile, '@', drawn in code in the venue's colours with its own little sign (venueDrawCounter). R next to it opens the menu.
//   The room   venueBuild lays it out when the lot opens (nbTemplate): a walled room, the counter across it, the props round the walls.
#define VF_ADULT 1   // grown-ups only
#define VF_NIGHT 2   // only after 6 PM
enum { VS_NONE, VS_MUNCH, VS_ADOPT, VS_MOVIE, VS_GAMBLE, VS_HEAL, VS_HAIR, VS_WORK, VS_ROB, VS_GROCER, VS_PETFOOD, VS_PETTOY, VS_PETGROOM,
       VS_LAUNCH, VS_SPACEWALK, VS_SPAR, VS_FIGHTER, VS_BOARD, VS_PRODECK, VS_CLASS, VS_BAG, VS_BELT, VS_FUND, VS_HOME, VS_PHOTO, VS_TOUR, VS_ROCK, VS_SYSJOB, VS_SATJOB, VS_GAUGE, VS_REFUEL10, VS_REFUEL35 };   // (board.h: a skateboard, a pro deck)   // (the packs, expand.h: a seat on the rocket, spacewalk training, a challenger, the BORTUA FIGHTER cabinet)
typedef struct { const char*nm; short price; signed char food, nrg, hyg, com, soc; u8 mood, sk, skn, stun, fl, sp; const char*say; } VAct;   // (VS_GAMBLE: sk = payout x, skn = win %)
#define NO 255
static const VAct vAct[]={
    // TACO BORE (0)
    {"CRUNCHY TACO",3, 20,0,0,0,0, M_EAT,NO,0,40,0,VS_MUNCH,"CRUNCH"},
    {"BORE-RITO SUPREME",6, 40,0,0,0,0, M_EAT,NO,0,50,0,VS_MUNCH,"SO MUCH CHEESE"},
    {"LOADED NACHOS",5, 30,0,-2,0,0, M_EAT,NO,0,50,0,VS_MUNCH,"CHEESE ON EVERYTHING"},
    {"BAJA BLAST-OFF",2, 0,12,0,0,0, M_WANT,NO,0,30,0,VS_NONE,"IT IS BLUE  NOBODY KNOWS WHY"},
    {"THE MUNCHIES BOX",12, 80,0,0,8,0, M_EAT,NO,0,80,0,VS_MUNCH,"THE WHOLE BOX"},
    {"FOURTH MEAL",8, 60,0,0,5,0, M_EAT,NO,0,60,VF_NIGHT,VS_MUNCH,"IT IS 2 AM  WORTH IT"},
    {"WORK THE DRIVE-THRU",0, 0,-15,-8,0,5, 0,SK_COOK,1,200,0,VS_WORK,"WOULD YOU LIKE SAUCE"},
    // GYM (7)
    {"LIFT WEIGHTS",5, -5,-15,-15,0,0, M_WANT,SK_BODY,2,150,0,VS_NONE,"DO YOU EVEN LIFT"},
    {"SPIN CLASS",8, -8,-20,-20,0,8, M_COMBO,SK_BODY,2,180,0,VS_NONE,"THE INSTRUCTOR YELLS"},
    {"PROTEIN SHAKE",4, 15,5,0,0,0, M_EAT,NO,0,30,0,VS_NONE,"TASTES LIKE CHALK"},
    {"SAUNA",3, 0,5,10,20,0, M_SOFA,NO,0,120,0,VS_NONE,"SO STEAMY"},
    {"PERSONAL TRAINER",0, 0,-15,-10,0,5, 0,SK_BODY,1,200,0,VS_WORK,"ONE MORE REP"},
    // ARCADE (12)
    {"PLAY A CABINET",1, 0,-3,0,0,0, M_COMBO,SK_LOGIC,1,90,0,VS_NONE,"NEW HIGH SCORE"},
    {"TICKET GAME",2, 0,-2,0,0,0, 0,3,40,60,0,VS_GAMBLE,0},
    {"DANCE MACHINE",2, -3,-8,-6,0,4, M_COMBO,SK_BODY,1,100,0,VS_NONE,"PERFECT PERFECT GREAT"},
    {"BORTUA FIGHTER",1, 0,-3,0,0,0, 0,NO,0,0,0,VS_FIGHTER,0},
    {"FIX THE MACHINES",0, 0,-10,-5,0,0, 0,SK_LOGIC,1,200,0,VS_WORK,"IT WAS UNPLUGGED"},
    // NIGHTCLUB (17)
    {"DANCE",0, -4,-10,-8,0,15, M_COMBO,SK_BODY,1,120,0,VS_NONE,"THE BASS DROPS"},
    {"BUY A ROUND",30, 0,0,0,0,30, M_WANT,SK_CHARM,1,60,VF_ADULT,VS_NONE,"EVERYONE LOVES YOU NOW"},
    {"VIP LOUNGE",50, 0,5,0,30,20, M_SOFA,NO,0,120,VF_ADULT,VS_NONE,"VELVET ROPES"},
    {"DJ A SET",0, 0,-12,0,0,10, M_COMBO,SK_CREAT,2,200,VF_NIGHT,VS_WORK,"THE CROWD GOES WILD"},
    // GROCERY (21)
    {"GROCERIES FOR HOME",40, 20,0,0,0,0, M_PAY,SK_COOK,1,60,0,VS_GROCER,0},
    {"A SNACK",3, 15,0,0,0,0, M_EAT,NO,0,30,0,VS_NONE,"CHIPS"},
    {"MUNCHIES AISLE",8, 30,0,0,5,0, M_EAT,NO,0,40,0,VS_MUNCH,"ONE OF EVERYTHING"},
    {"PET FOOD",10, 0,0,0,0,0, 0,NO,0,30,0,VS_PETFOOD,0},
    {"STOCK SHELVES",0, 0,-12,-6,0,0, 0,SK_BODY,1,200,0,VS_WORK,"CLEANUP ON AISLE 3"},
    // BANK (26)
    {"INVEST 100",100, 0,0,0,0,0, 0,2,48,60,VF_ADULT,VS_GAMBLE,0},
    {"ROB THE BANK",0, 0,-10,0,0,0, 0,NO,0,90,VF_ADULT,VS_ROB,0},
    {"WORK THE TELLER",0, 0,-10,0,0,5, 0,SK_LOGIC,1,200,0,VS_WORK,"NEXT IN LINE"},
    // HOSPITAL (29)
    {"GET PATCHED UP",50, 0,0,0,10,0, M_SOFA,NO,0,100,0,VS_HEAL,"GOOD AS NEW"},
    {"A CHECKUP",20, 0,0,0,10,0, M_WANT,NO,0,60,0,VS_HEAL,"SAY AHH"},
    {"DONATE BLOOD",-30, 10,-20,0,0,0, M_WANT,NO,0,90,VF_ADULT,VS_NONE,"YOU GET A COOKIE"},
    {"VOLUNTEER",0, 0,-12,0,0,10, 0,SK_CHARM,1,200,0,VS_WORK,"YOU HELPED"},
    // CAFE (33)
    {"ESPRESSO",3, 0,25,0,0,0, M_WANT,NO,0,30,0,VS_NONE,"WIDE AWAKE"},
    {"OAT LATTE",4, 5,18,0,5,0, M_WANT,NO,0,40,0,VS_NONE,"VERY ARTISANAL"},
    {"A PASTRY",3, 15,0,0,0,0, M_EAT,NO,0,30,0,VS_MUNCH,"FLAKY"},
    {"OPEN MIC",0, 0,-5,0,0,12, M_COMBO,SK_CHARM,2,120,0,VS_NONE,"SNAPS ALL ROUND"},
    {"BARISTA SHIFT",0, 0,-12,-4,0,6, 0,SK_COOK,1,200,0,VS_WORK,"NAME FOR THE CUP"},
    // BAR (38)
    {"A DRINK",6, 0,-3,0,5,10, M_CHILL,NO,0,60,VF_ADULT,VS_NONE,"CHEERS"},
    {"KARAOKE",0, 0,-5,0,0,15, M_COMBO,SK_CHARM,1,120,0,VS_NONE,"DON'T STOP BELIEVING"},
    {"DARTS",2, 0,-3,0,0,8, M_WANT,SK_LOGIC,1,80,0,VS_NONE,"BULLSEYE  ALMOST"},
    {"BAR SNACKS",4, 15,0,0,0,0, M_EAT,NO,0,30,0,VS_MUNCH,"PRETZELS"},
    {"TEND BAR",0, 0,-12,-4,0,8, 0,SK_CHARM,1,200,VF_ADULT,VS_WORK,"WHAT CAN I GET YOU"},
    // SALON (43)
    {"NEW HAIRDO",40, 0,0,5,5,0, M_WANT,NO,0,120,0,VS_HAIR,0},
    {"MANI PEDI",20, 0,0,10,20,0, M_SOFA,NO,0,90,0,VS_NONE,"FABULOUS"},
    {"SPA DAY",60, 0,20,40,40,0, M_SOFA,NO,0,200,0,VS_NONE,"CUCUMBERS ON YOUR EYES"},
    // PET SHOP (46)
    {"ADOPT A PET",0, 0,0,0,0,0, 0,NO,0,0,0,VS_ADOPT,0},
    {"PET FOOD",10, 0,0,0,0,0, 0,NO,0,30,0,VS_PETFOOD,0},
    {"SQUEAKY TOY",8, 0,0,0,0,0, 0,NO,0,30,0,VS_PETTOY,0},
    {"PET GROOMING",25, 0,0,0,0,0, 0,NO,0,40,0,VS_PETGROOM,0},
    // LIBRARY (50)
    {"STUDY",0, -3,-8,0,0,0, M_WANT,SK_LOGIC,2,150,0,VS_NONE,"KNOWLEDGE"},
    {"READ A NOVEL",0, 0,0,0,10,0, M_SOFA,SK_CREAT,1,120,0,VS_NONE,"PLOT TWIST"},
    {"USE A COMPUTER",0, 0,-3,0,0,5, M_WANT,SK_LOGIC,1,90,0,VS_NONE,"YOU FELL DOWN A WIKI HOLE"},
    {"SHELVE BOOKS",0, 0,-10,0,0,0, 0,SK_LOGIC,1,200,0,VS_WORK,"SHHH"},
    // CASINO (54)
    {"SLOTS",5, 0,-2,0,0,0, 0,3,30,60,VF_ADULT,VS_GAMBLE,0},
    {"BLACKJACK",20, 0,-2,0,0,3, 0,2,45,80,VF_ADULT,VS_GAMBLE,0},
    {"HIGH ROLLER TABLE",200, 0,-4,0,0,5, 0,2,42,100,VF_ADULT,VS_GAMBLE,0},
    {"DEAL CARDS",0, 0,-12,0,0,5, 0,SK_LOGIC,1,200,VF_ADULT,VS_WORK,"HIT OR STAND"},
    // CINEMA (58)
    {"WATCH A MOVIE",10, 0,0,0,10,5, M_SOFA,NO,0,60,0,VS_MOVIE,0},
    {"POPCORN",4, 15,0,0,0,0, M_EAT,NO,0,30,0,VS_MUNCH,"EXTRA BUTTER"},
    {"THE LATE SHOW",8, 0,-5,0,10,5, M_SOFA,NO,0,60,VF_NIGHT,VS_MOVIE,0},
    {"USHER SHIFT",0, 0,-10,0,0,4, 0,SK_CHARM,1,200,0,VS_WORK,"ENJOY THE SHOW"},
    // POOL (62)
    {"SWIM LAPS",2, -5,-10,15,0,0, M_WANT,SK_BODY,2,150,0,VS_NONE,"SPLASH"},
    {"CANNONBALL",0, 0,-3,10,0,8, M_COMBO,NO,0,60,0,VS_NONE,"HUGE SPLASH"},
    {"SUNBATHE",0, 0,5,0,15,0, M_SOFA,NO,0,120,0,VS_NONE,"SO WARM"},
    {"LIFEGUARD SHIFT",0, 0,-12,0,0,4, 0,SK_BODY,1,200,0,VS_WORK,"NO RUNNING"},
    // SPACEPORT (66)
    {"ASTRONAUT ICE CREAM",4, 15,0,0,0,0, M_EAT,NO,0,30,0,VS_NONE,"FREEZE DRIED  BUT WHY"},
    {"RIDE THE CENTRIFUGE",10, -10,-8,-10,0,0, M_COMBO,SK_BODY,1,120,0,VS_NONE,"YOU SAW YOUR LUNCH AGAIN"},
    {"FLIGHT SIMULATOR",0, 0,-6,0,0,0, M_WANT,SK_LOGIC,2,150,0,VS_NONE,"YOU CRASHED INTO THE MOON"},
    {"SPACEWALK TRAINING",0, 0,-5,0,0,0, 0,NO,0,0,0,VS_SPACEWALK,0},
    {"A SEAT ON THE ROCKET",2500, 0,0,0,0,0, 0,NO,0,0,0,VS_LAUNCH,0},
    {"MISSION CONTROL SHIFT",0, 0,-10,0,0,4, 0,SK_LOGIC,1,200,0,VS_WORK,"HOUSTON  WE HAVE A NAP"},
    // DOJO (72)
    {"KATA",0, -4,-12,-8,0,0, M_WANT,SK_BODY,2,150,0,VS_NONE,"WAX ON  WAX OFF"},
    {"MEDITATE",0, 0,5,0,20,0, M_SOFA,NO,0,150,0,VS_NONE,"OMMMMMM"},
    {"CALL OUT A CHALLENGER",5, 0,0,0,0,0, 0,NO,0,30,0,VS_SPAR,0},
    {"TEACH A CLASS",0, 0,-12,-6,0,6, 0,SK_BODY,1,200,0,VS_WORK,"BOW TO YOUR SENSEI"},
    {"ATTEND CLASS",10, 0,0,0,0,0, 0,NO,0,0,0,VS_CLASS,0},
    {"DRILL THE BAG",0, 0,0,0,0,0, 0,NO,0,0,0,VS_BAG,0},
    {"BELT AND SCHEDULE",0, 0,0,0,0,0, 0,NO,0,0,0,VS_BELT,0},
    // SKATE SHOP (76)
    {"A COMPLETE BOARD",120, 0,0,0,0,0, 0,NO,0,30,0,VS_BOARD,0},
    {"A PRO DECK",450, 0,0,0,0,0, 0,NO,0,30,0,VS_PRODECK,0},
    {"WATCH SKATE VIDEOS",0, 0,-2,0,5,0, M_COMBO,NO,0,120,0,VS_NONE,"THE NINETIES WERE A TIME"},
    {"GRIP TAPE ART",15, 0,-3,0,0,0, M_WANT,SK_CREAT,1,90,0,VS_NONE,"A MASTERPIECE  UNDERFOOT"},
    {"WORK THE REGISTER",0, 0,-10,0,0,5, 0,SK_CHARM,1,200,0,VS_WORK,"DUDE  NO WAY"},
    // LUNRADO (the last rows: VA_LUN) and the spaceport's funding
    {"LUNAR RATIONS",8, 45,0,0,0,0, M_EAT,NO,0,60,0,VS_NONE,"FREEZE DRIED  BUT ON THE MOON"},
    {"LOW GRAVITY BOUNCE",0, -4,-10,-8,0,0, M_COMBO,SK_BODY,2,150,0,VS_NONE,"YOU BOUNCE  FOREVER"},
    {"MOONBASE SHIFT",0, 0,-12,-6,0,6, 0,SK_LOGIC,1,200,0,VS_WORK,"MOON ROCKS ARE HEAVY ANYWAY"},
    {"RETURN TO EARTH",0, 0,0,0,0,0, 0,NO,0,0,0,VS_HOME,0},
    {"EARTHRISE PHOTO",5, 0,0,0,0,0, 0,NO,0,0,0,VS_PHOTO,0},
    {"CRATER TOUR",15, 0,0,0,0,0, 0,NO,0,0,0,VS_TOUR,0},
    {"MOON ROCK SOUVENIR",25, 0,0,0,0,0, 0,NO,0,0,0,VS_ROCK,0},
    {"SYSTEMS CHECK JOB",0, 0,0,0,0,0, 0,NO,0,0,0,VS_SYSJOB,0},
    {"SATELLITE REPAIR JOB",0, 0,0,0,0,0, 0,NO,0,0,0,VS_SATJOB,0},
    {"FUND THE SPACEPORT",0, 0,0,0,0,0, 0,NO,0,0,0,VS_FUND,0},
    {"FUEL GAUGE",0, 0,0,0,0,0, 0,NO,0,0,0,VS_GAUGE,0},
    {"REFUEL  10 TRIPS",0, 0,0,0,0,0, 0,NO,0,0,0,VS_REFUEL10,0},
    {"REFUEL  35 TRIPS",0, 0,0,0,0,0, 0,NO,0,0,0,VS_REFUEL35,0},
};
#define VA_LUN ((int)(sizeof(vAct)/sizeof(vAct[0]))-13)   // the nine moonbase rows (four, the tourist three, the two jobs), then the Earth counter's FUND, FUEL GAUGE, REFUEL 10 and REFUEL 35 (VA_LUN+9 to +12)
// the rows the packs' missions name (expand.h TD): keep them in step with the table above
enum { VA_FOURTH=5, VA_LIFT=7, VA_SPIN=8, VA_SHAKE=9, VA_CABINET=12, VA_BORTUA=15, VA_GROCER=21, VA_CHECKUP=30, VA_LATTE=34, VA_KARAOKE=39, VA_BARSNACK=41,
       VA_TOY=48, VA_GROOM=49, VA_STUDY=50, VA_NOVEL=51, VA_MOVIE=58, VA_SWIM=62, VA_CANNON=63, VA_CENTRI=67, VA_SPACEWALK=69, VA_SEAT=70, VA_KATA=72, VA_SPAR=74 };
#undef NO
typedef struct { u8 ct, wp, fl; u16 a, b; const char*props; u8 a0, an; const char*hint; } Venue;   // a, b: the counter's and the town icon's two colours
static const Venue venues[]={
    {CT_TACO,    3,13,RGB(18,6,24),RGB(31,26,4), "KhKhpl", 0,7,"R ORDER"},
    {CT_GYM,     9, 7,RGB(10,10,12),RGB(30,8,6), "mmmHHb", 7,5,"R THE FRONT DESK"},
    {CT_ARCADE,  3, 2,RGB(6,4,14),RGB(4,28,30),  "QvQvVV",12,5,"R THE PRIZE COUNTER"},
    {CT_CLUB,    5, 3,RGB(8,2,10),RGB(30,6,28),  "AAVVUU",17,4,"R THE BAR"},
    {CT_GROCER,  0, 3,RGB(24,24,24),RGB(6,22,8), "bbFFbb",21,5,"R THE CHECKOUT"},
    {CT_BANK,    4, 1,RGB(16,12,6),RGB(28,24,10),"ddhhpl",26,3,"R THE TELLER"},
    {CT_HOSPITAL,0, 6,RGB(28,28,30),RGB(28,4,4), "SSSHpi",29,4,"R THE NURSE"},
    {CT_CAFE,    4, 4,RGB(14,9,5),RGB(26,20,12), "KhKhcp",33,5,"R ORDER"},
    {CT_BAR,     8, 4,RGB(10,5,3),RGB(30,20,6),  "hhhyKA",38,5,"R THE BAR"},
    {CT_SALON,   1, 5,RGB(30,20,24),RGB(28,10,20),"hhiipl",43,3,"R THE STYLIST"},
    {CT_PETSHOP, 6,11,RGB(12,20,10),RGB(30,24,8),"qqZZpr",46,4,"R THE COUNTER"},
    {CT_LIBRARY, 4, 1,RGB(12,8,5),RGB(20,14,8),  "bbbbdh",50,4,"R THE LIBRARIAN"},
    {CT_CASINO,  5, 2,RGB(4,12,6),RGB(30,26,4),  "KKvAVh",54,4,"R THE CASHIER"},
    {CT_CINEMA,  5,13,RGB(6,4,8),RGB(30,28,20),  "vCCCCv",58,4,"R THE BOX OFFICE"},
    {CT_POOL,    0, 6,RGB(24,26,28),RGB(6,18,30),"HHNNpZ",62,4,"R THE LIFEGUARD"},
    {CT_SPACE,   5, 2,RGB(4,6,14),RGB(28,28,30), "QvVQpl",66,6,"R MISSION CONTROL"},
    {CT_DOJO,    4, 4,RGB(20,5,4),RGB(30,26,18), "mmHHpl",72,7,"R THE SENSEI"},
    {CT_SKSHOP,  3, 2,RGB(20,20,22),RGB(31,22,4),"bbVQpl",79,5,"R THE SHOP DUDE"},
};
#define NVEN ((int)(sizeof(venues)/sizeof(venues[0])))
static const Venue* venueOf(int ct){ for(int i=0;i<NVEN;i++) if(venues[i].ct==ct) return &venues[i]; return 0; }
static const Venue* venueHere(void){ if(!nbOk||nbT.cur>=NB_LOTS) return 0; const NbLot*L=&nbT.lot[nbT.cur]; return L->on&&L->kind==LKIND_COMM?venueOf(L->type):0; }
// ---- the room ----
static int catFind(const char*nm){ for(int i=0;i<CAT_N;i++){ const char*a=catTab[i].nm,*b=nm; while(*a&&*a==*b){ a++; b++; } if(!*a&&!*b) return catTab[i].lk; } return 0; }   // a catalog look by name (catalog.h), as the wallMap byte
static int venueLook(int ct,char c){   // the props of a place wear the catalog looks that fit it (TACO BORE's stools and neon, the arcade's cabinets ...)
    switch(ct){
        case CT_TACO: return c=='h'?catFind("BAR STOOL"):c=='p'?catFind("PINK FLAMINGO"):c=='l'?catFind("NEON ARROW"):0;
        case CT_ARCADE: return c=='v'?catFind("ARCADE CABINET"):0;
        case CT_CAFE: return c=='h'?catFind("WICKER CHAIR"):c=='c'?catFind("KETTLE STAND"):c=='p'?catFind("FICUS"):0;
        case CT_BAR: return c=='h'?catFind("BAR STOOL"):c=='y'?catFind("BAR COUNTER"):0;
        case CT_GYM: return c=='m'?catFind("WEIGHT BENCH"):c=='H'?catFind("GLASS SHOWER"):0;
        case CT_DOJO: return c=='m'?catFind("PUNCHING BAG"):c=='p'?catFind("BONSAI"):c=='l'?catFind("TOTEM POLE"):0;
        case CT_CASINO: return c=='v'?catFind("SLOT MACHINE"):c=='h'?catFind("BAR STOOL"):0;
        case CT_LIBRARY: return c=='b'?catFind("TALL BOOKSHELF"):c=='d'?catFind("COMPUTER DESK"):c=='h'?catFind("OFFICE CHAIR"):0;
        case CT_HOSPITAL: return c=='S'?catFind("CAMP COT"):c=='i'?catFind("CABINET"):c=='p'?catFind("FERN"):0;
        case CT_POOL: return c=='N'?catFind("PARK BENCH"):c=='H'?catFind("HOT TUB"):c=='p'?catFind("POTTED PALM"):0;
        case CT_SALON: return c=='h'?catFind("OFFICE CHAIR"):c=='i'?catFind("WHITE WARDROBE"):c=='l'?catFind("MARBLE BUST"):0;
        case CT_CINEMA: return c=='v'?catFind("FLAT TV"):c=='C'?catFind("RED COUCH"):0;
        case CT_SPACE: return c=='v'?catFind("ROBOT BUDDY"):c=='p'?catFind("TALL CACTUS"):c=='l'?catFind("GLOBE"):0;
        case CT_BANK: return c=='d'?catFind("RECEPTION DESK"):c=='h'?catFind("OFFICE CHAIR"):c=='l'?catFind("TALL VASE"):0;
        case CT_CLUB: return c=='A'?catFind("BIG SPEAKERS"):c=='U'?catFind("BALL PIT"):0;
        case CT_GROCER: return c=='F'?catFind("VENDING MACHINE"):c=='b'?catFind("WINE RACK"):0;
        case CT_PETSHOP: return c=='q'?catFind("FOUNTAIN"):c=='p'?catFind("TEDDY BEAR"):0;
        case CT_SKSHOP: return c=='b'?catFind("BOARD RACK"):c=='p'?catFind("TRAFFIC CONE"):c=='l'?catFind("NEON BORE SIGN"):0;
    }
    return 0;
}
// THE ARCADE'S HALL (venueBuild calls it): a prize counter with the prizes on the shelf behind it (the ticket desk), a vending machine for change and
// snacks, rows of cabinets and pinball with a walking lane between them, a jukebox, and a real bathroom (two toilets, two sinks) in the back corner.
// Everything is placed relative to the room, so it fits the small and the big lots; gFree-style guards keep it off walls and each other.
static int vRs[4][4], vNRs;   // rooms inside the hall that other pieces keep out of (the restrooms): x0, y0, x1, y1
static int vResHit(int x,int y){ for(int i=0;i<vNRs;i++) if(x>=vRs[i][0]&&x<=vRs[i][2]&&y>=vRs[i][1]&&y<=vRs[i][3]) return 1; return 0; }
static void vPut(int x,int y,char c,const char*nm){ if(x>0&&y>0&&x<MW-1&&y<MH-1&&lifeMap[y][x]=='.'&&!vResHit(x,y)){ gPut(x,y,c); wallMap[y][x]=(u8)catFind(nm); } }
static void arcadeBuild(int rx0,int ry0,int rx1,int ry1,int cx,int wp){
    // the bathroom: back right corner, 4 tiles wide inside, its door opens onto the hall
    int bx0=rx1-5, by1=ry0+3;
    gLine(bx0,ry0,bx0,by1,'W',wp); gLine(bx0,by1,rx1,by1,'W',wp); gBox(bx0+1,ry0+1,rx1-1,by1-1,5); gPut(bx0+2,by1,'D');
    vPut(bx0+1,ry0+1,'T',"PINK TOILET"); vPut(bx0+2,ry0+1,'T',"BLACK TOILET"); vPut(bx0+3,ry0+1,'H',"BATHROOM SINK"); vPut(bx0+4,ry0+1,'H',"BATHROOM SINK");
    // the prize counter (ticket desk) along the back wall, left: counter tiles in front, the prizes on the shelf behind it
    for(int x=rx0+1;x<=rx0+4;x++){ gPut(x,ry0+2,'@'); }
    vPut(rx0+1,ry0+1,'p',"TEDDY BEAR"); vPut(rx0+2,ry0+1,'l',"TROPHY CASE"); vPut(rx0+3,ry0+1,'b',"GIFT PILE"); vPut(rx0+4,ry0+1,'p',"PINATA");
    vPut(rx0+5,ry0+1,'F',"VENDING MACHINE"); vPut(rx0+6,ry0+1,'l',"NEON BORE SIGN"); vPut(rx0+7,ry0+1,'l',"DISCO BALL");   // change and snacks, the sign, the ball
    vPut(rx0+1,ry0+3,'R',"JUKEBOX");
    // the floor: cabinets in a row, pinball and the dartboard in the next, a lane (every third row) to walk in
    for(int y=ry0+4;y<=ry1-2;y+=3){
        for(int x=rx0+1;x<=rx1-1;x++){ if(x==cx||x==cx+1||((x-rx0)%4)==0||(y==ry0+4&&x>=bx0+1&&x<=bx0+3)) continue;   // gaps to walk through, and the way from the door
            vPut(x,y,'v',"ARCADE CABINET");
            if(y+1<=ry1-2) vPut(x,y+1,'v',((x-rx0)&1)?"PINBALL":"CONSOLE AND TV"); }
    }
    vPut(rx1-1,ry1-1,'p',"BALL PIT"); vPut(rx0+1,ry1-1,'p',"TEDDY BEAR");
}
// ---- THE BAR, renovated: a back bar of bottles and kegs, the long counter with stools, a pool and dart corner, a stage with a drum kit and speakers, tables and booths. The middle aisle from the door to the counter stays clear ----
static void barBuild(int rx0,int ry0,int rx1,int ry1,int cx){
#define BP(x,y,c,nm) do{ int _x=(x), _y=(y); if(_x>rx0&&_x<rx1&&_y>ry0&&_y<ry1) vPut(_x,_y,c,nm); }while(0)
    int b=ry0+1, cy=ry0+3;   // the back shelf row, the counter row
    for(int d=-4;d<=4;d+=2){ BP(cx+d,b,'b',(d&2)?"TALL BOOKSHELF":"WHITE BOOKSHELF"); }
    BP(cx,b,'l',"NEON BEER SIGN"); BP(cx-2,b,'F',"WINE FRIDGE"); BP(cx+2,b,'F',"WINE FRIDGE"); BP(cx-4,b,'F',"BEER KEG"); BP(cx+4,b,'F',"BEER KEG"); BP(cx-3,b,'b',"CIGAR HUMIDOR"); BP(cx+3,b,'l',"BRASS LAMP");
    for(int x=cx-2;x<=cx+2;x++) gPut(x,cy,'@');   // the counter you serve at (R)
    BP(cx-3,cy,'y',"BAR COUNTER"); BP(cx-4,cy,'y',"BAR COUNTER"); BP(cx+3,cy,'y',"BAR COUNTER"); BP(cx+4,cy,'y',"BAR COUNTER");
    BP(cx-4,cy+1,'h',"BAR STOOL"); BP(cx-3,cy+1,'h',"BAR STOOL"); BP(cx+3,cy+1,'h',"BAR STOOL"); BP(cx+4,cy+1,'h',"BAR STOOL");
    BP(rx0+1,b,'R',"JUKEBOX"); BP(rx0+3,b,'l',"NEON BORE SIGN"); BP(rx0+1,cy+1,'v',"DARTBOARD");   // the left corner: music and darts
    BP(rx0+3,cy+4,'t',"POOL TABLE"); BP(rx0+4,cy+4,'t',"POOL TABLE"); BP(rx0+2,cy+6,'h',"BAR STOOL"); BP(rx0+5,cy+6,'h',"BAR STOOL");
    BP(rx1-1,b,'R',"DRUM KIT"); BP(rx1-3,b,'R',"GUITAR STAND"); BP(rx1-1,cy,'A',"BIG SPEAKERS"); BP(rx1-5,b,'A',"BIG SPEAKERS"); BP(rx1-2,cy+1,'l',"DISCO BALL");   // the right corner: the stage
    for(int y=ry0+8;y<ry1-1;y+=4) for(int s=-1;s<=1;s+=2) for(int o=5;o<=9;o+=4){ int tx=cx+s*o;   // tables, a chair each side, a row every four
        BP(tx,y,'t',"ROUND TABLE"); BP(tx-1,y,'h',s<0?"RED ARMCHAIR":"GREEN ARMCHAIR"); BP(tx+1,y,'h',s<0?"GREEN ARMCHAIR":"RED ARMCHAIR"); }
    for(int y=ry0+8;y<ry1-2;y+=3){ BP(rx0+1,y,'C',"VELVET SOFA"); BP(rx1-1,y,'C',"VELVET SOFA"); }   // booths along the walls
    BP(rx0+1,ry1-1,'p',"POTTED PALM"); BP(rx1-1,ry1-1,'p',"FICUS"); BP(rx0+1,ry1-2,'l',"BRASS LAMP"); BP(rx1-1,ry1-2,'l',"BRASS LAMP");
#undef BP
}
// ---- THE HALLS: every place has its own room (the arcade's and the bar's are above). Each builder lays out the inside of a lot's room with the helpers below; the room is vL..vR by vT..vB
// inside its walls, the counter's row is vC (two rows of back room above it, the customers' floor below), the door is in column vX. vIn never puts anything in the way from the door to the
// counter. The back room behind a counter that runs wall to wall (vWings) is for show: fridges, safes, lockers, shelves. tools/lots_preview.py draws every hall at every lot shape on a PC. ----
static int vL, vT, vR, vB, vC, vX;
typedef struct { char c; const char*nm; } VI;
static void vIn(int x,int y,char c,const char*nm){ if(x>=vL&&x<=vR&&y>=vT&&y<=vB&&!(y>vC&&x>=vX-1&&x<=vX+1)&&!(y==vC+1&&x>vL&&x<vR)) vPut(x,y,c,nm); }   // (the door's aisle and the row in front of the counter stay open)
static void vRun(int x0,int x1,int y,char c,const char*nm,int st){ for(int x=x0;x<=x1;x+=st) vIn(x,y,c,nm); }
static void vCol(int x,int y0,int y1,char c,const char*nm,int st){ for(int y=y0;y<=y1;y+=st) vIn(x,y,c,nm); }
static void vSeq(int x0,int x1,int y,const VI*l,int n,int st){ int k=0; for(int x=x0;x<=x1;x+=st,k++) vIn(x,y,l[k%n].c,l[k%n].nm); }   // a run of different pieces, repeating
static void vWings(char c,const char*nm){ vRun(vL,vX-3,vC,c,nm,1); vRun(vX+3,vR,vC,c,nm,1); }                                                  // the counter goes wall to wall
static void vTable(int x,int y,const char*tn,const char*ch,int sides){ vIn(x,y,'t',tn); if(sides&1) vIn(x-1,y,'h',ch); if(sides&2) vIn(x+1,y,'h',ch); if(sides&4) vIn(x,y-1,'h',ch); if(sides&8) vIn(x,y+1,'h',ch); }
static void vWindows(int rx0,int ry0,int rx1,int ry1,int cx,char w){   // windows in the outer walls: in the front wall either side of the door, down the sides, along the back
    for(int x=rx0+2;x<=rx1-2;x+=3){ if(x<cx-1||x>cx+1){ if(lifeMap[ry1][x]=='W') lifeMap[ry1][x]=w; } if(lifeMap[ry0][x]=='W') lifeMap[ry0][x]=w; }
    for(int y=ry0+3;y<=ry1-2;y+=3){ if(lifeMap[y][rx0]=='W') lifeMap[y][rx0]=w; if(lifeMap[y][rx1]=='W') lifeMap[y][rx1]=w; }
}
static void vFront(int x0,int y0,int x1,int y1,int ry1,int cx,const char*bench,const char*bush){   // the front yard: a bench and a flower bed either side of the door, lamps at the corners
    int y=ry1+1; vPut(cx-4,y,'N',bench); vPut(cx+5,y,'N',bench); vPut(cx-2,y,'Z',bush); vPut(cx+3,y,'Z',bush); vPut(x0,y,'l',"STREET LAMP"); vPut(x1,y,'l',"STREET LAMP");
    vPut(x0,y0,'p',"BOXWOOD"); vPut(x1,y0,'p',"BOXWOOD"); vPut(x0,ry1,'p',"OAK TREE"); vPut(x1,ry1,'p',"OAK TREE");
}
// THE WALK-THROUGH CHECK (as the houses have): after a hall is laid out, anything that cannot be walked to is taken away again. A floor tile in the customers' part (below the counter) that
// cannot be reached from the door means something walled it in: the piece beside it that is least missed (a chair, a plant, a lamp, then the rest) goes. A piece with no reachable tile
// beside it goes too. The search borrows bfsQ, which no search is using while a lot is built.
static u8 vSeen[MH][MW] EWRAM_BSS; static int vRemoved;
static int vReach(int sx,int sy,int x0,int y0,int x1,int y1){   // marks every tile of the room that can be walked to from (sx, sy); returns how many
    for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) vSeen[y][x]=0;
    u16*q=bfsQ; int qh=0, qt=0, n=0; q[qt++]=(u16)(sy*MW+sx); vSeen[sy][sx]=1;
    while(qh<qt){ int p=q[qh++], x=p%MW, y=p/MW; n++;
        static const s8 d[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
        for(int k=0;k<4;k++){ int nx=x+d[k][0], ny=y+d[k][1]; if(nx<x0||nx>x1||ny<y0||ny>y1||vSeen[ny][nx]) continue; char c=lifeMap[ny][nx]; if(c!='.'&&c!='P'&&c!='B') continue; vSeen[ny][nx]=1; q[qt++]=(u16)(ny*MW+nx); } }
    return n;
}
static int vRank(char c){ return c=='h'?0:c=='p'?1:c=='l'?2:c=='Z'?3:c=='t'?4:c=='N'?5:6; }   // what is missed least
static void vRepair(int rx0,int ry0,int rx1,int ry1,int cx){
    int x0=rx0+1, x1=rx1-1, y0=ry0+1, y1=ry1-1, cy=ry0+3; vRemoved=0;
    for(int pass=0;pass<60;pass++){
        vReach(cx,y1,x0,y0,x1,y1);   // (from the first tile inside the door)
        int bx=-1, by=-1, br=99;
        for(int y=cy+1;y<=y1;y++) for(int x=x0;x<=x1;x++){ char c=lifeMap[y][x];
            if(c=='.'&&!vSeen[y][x]){   // a walled-in tile: the cheapest piece beside it that borders somewhere reachable (or any piece)
                for(int k=0;k<4;k++){ int nx=x+(k==0)-(k==1), ny=y+(k==2)-(k==3); if(nx<x0||nx>x1||ny<=cy||ny>y1) continue; char e=lifeMap[ny][nx]; if(e=='.'||e=='W'||e=='D'||e=='@'||e=='E'||e=='f'||e=='e') continue;
                    int r=vRank(e)+((vSeen[ny][nx])?0:0); int touch=0; for(int j=0;j<4;j++){ int tx=nx+(j==0)-(j==1), ty=ny+(j==2)-(j==3); if(tx>=x0&&tx<=x1&&ty>cy&&ty<=y1&&vSeen[ty][tx]) touch=1; }
                    if(touch) r-=3; if(r<br){ br=r; bx=nx; by=ny; } } }
            else if(c!='.'&&c!='W'&&c!='D'&&c!='@'&&c!='E'&&c!='f'&&c!='e'&&c!='P'&&c!='B'){   // a piece nobody can reach
                int touch=0; for(int j=0;j<4;j++){ int tx=x+(j==0)-(j==1), ty=y+(j==2)-(j==3); if(tx>=x0&&tx<=x1&&ty>cy&&ty<=y1&&vSeen[ty][tx]) touch=1; }
                if(!touch){ int r=vRank(c)-10; if(r<br){ br=r; bx=x; by=y; } } } }
        if(bx<0) break;
        lifeMap[by][bx]='.'; wallMap[by][bx]=0; vRemoved++;
    }
}
static void vFloorPatch(int x0,int y0,int x1,int y1,int fl){ for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) if(x>=vL&&x<=vR&&y>vC&&y<=vB&&lifeMap[y][x]=='.') floorMap[y][x]=(u8)fl; }
static void vSeats(int x,int y0,int y1,int dir,char c,const char*nm,int st){ for(int y=y0;y<=y1;y+=st) vIn(x,y,c,nm); (void)dir; }
static void vRestroom(int side){   // a restroom in the bottom corner of the hall (side 0 left, 1 right): four tiles wide, three deep, walled on the hall's sides with a doorway; toilets on the front wall, sinks on the back
    int x0=side?vR-3:vL, x1=side?vR:vL+3, wx=side?vR-4:vL+4, ty=vB-3;
    if(ty<=vC+2) return;
    for(int x=(side?vR-4:vL);x<=(side?vR:vL+4);x++){ lifeMap[ty][x]='W'; wallMap[ty][x]=7; }
    for(int y=ty;y<=vB;y++){ lifeMap[y][wx]='W'; wallMap[y][wx]=7; }
    lifeMap[vB-1][wx]='.'; wallMap[vB-1][wx]=0;                                                    // the doorway
    for(int y=ty+1;y<=vB;y++) for(int x=x0;x<=x1;x++){ lifeMap[y][x]='.'; wallMap[y][x]=0; floorMap[y][x]=(u8)(side?6:5); }
    vPut(x0,vB,'T',side?"BLACK TOILET":"PINK TOILET"); vPut(x0+1,vB,'T',side?"BLACK TOILET":"PINK TOILET"); vPut(x1,vB,'p',"FERN");
    vPut(x0+1,ty+1,'H',"BATHROOM SINK"); vPut(x0+2,ty+1,'H',"BATHROOM SINK");
    if(vNRs<4){ vRs[vNRs][0]=x0; vRs[vNRs][1]=ty+1; vRs[vNRs][2]=x1; vRs[vNRs][3]=vB; vNRs++; }   // (nothing else goes in there)
    if(vNRs<4){ vRs[vNRs][0]=side?wx-2:wx; vRs[vNRs][1]=vB-2; vRs[vNRs][2]=side?wx:wx+2; vRs[vNRs][3]=vB; vNRs++; }   // (nor in the little lobby before its doorway)
}
static int vHasRestrooms(int ct){ return ct==CT_TACO||ct==CT_CAFE||ct==CT_CLUB||ct==CT_CINEMA||ct==CT_CASINO||ct==CT_LIBRARY||ct==CT_BAR||ct==CT_GROCER||ct==CT_SALON||ct==CT_DOJO||ct==CT_ARCADE; }
static void tacoBuild(void){
    static const VI line[]={{'F',"STEEL FRIDGE"},{'y',"STOVE"},{'y',"OVEN"},{'y',"STOVE"},{'y',"KITCHEN SINK"},{'F',"MINT FRIDGE"},{'y',"MICROWAVE STAND"}};
    vSeq(vL,vR,vT,line,7,1); vWings('y',"OAK COUNTER"); vRun(vL+1,vR-1,vT+1,'y',"BUTCHER BLOCK",4); vIn(vX,vT+1,'l',"NEON BORE SIGN"); vIn(vX-4,vT+1,'l',"NEON ARROW"); vIn(vX+4,vT+1,'l',"NEON ARROW");
    for(int y=vC+3;y<=vB-1;y+=3) for(int d=4;vX-d>=vL+3;d+=4){ vTable(vX-d,y+((d/4)&1),"ROUND TABLE","BAR STOOL",3); vTable(vX+d,y+((d/4)&1),"ROUND TABLE","BAR STOOL",3); }   // the dining room: clusters either side of the aisle
    vIn(vR,vC+1,'F',"VENDING MACHINE"); vIn(vR,vC+2,'F',"VENDING MACHINE"); vIn(vL,vC+1,'p',"PINK FLAMINGO"); vIn(vL,vC+2,'p',"PINK FLAMINGO"); vIn(vR,vB,'l',"NEON ARROW");
    for(int y=vB-2;y<=vB;y++) for(int x=vL;x<=vL+2;x++) vIn(x,y,'U',"BALL PIT");                                                                                           // the play area
    vIn(vL+3,vB,'h',"ROCKING HORSE"); vIn(vL+3,vB-1,'p',"TEDDY BEAR"); vIn(vL+3,vB-2,'p',"BLOCKS TOWER");
}
static void gymBuild(void){
    vWings('i',"PINE DRESSER"); vRun(vL,vR,vT,'i',"LOCKER",1); vIn(vX,vT,'l',"TROPHY CASE"); vIn(vX-3,vT,'l',"TROPHY CASE"); vIn(vX+3,vT,'l',"TROPHY CASE");
    vRun(vL+1,vR-1,vT+1,'b',"WHITE BOOKSHELF",3);
    vCol(vL,vC+1,vC+3,'H',"BLUE SHOWER",1); vIn(vL,vC+4,'H',"BATHROOM SINK"); vIn(vL,vC+5,'T',"PINK TOILET"); vIn(vL,vC+6,'T',"BLACK TOILET");   // the locker-room wall
    vCol(vR,vC+2,vB-1,'i',"LOCKER",1);
    for(int y=vC+3;y<=vB-3;y+=2){ for(int x=vL+3;x<=vX-3;x+=2) vIn(x,y,'m',"WEIGHT BENCH"); for(int x=vX+3;x<=vR-3;x+=2) vIn(x,y,'m',((x+y)&2)?"WEIGHT BENCH":"PUNCHING BAG"); }
    if(vB-vC>=9){ vFloorPatch(vL+1,vB-3,vL+5,vB,1); vIn(vL+1,vB,'p',"FERN"); vIn(vL+5,vB,'p',"FERN"); vIn(vL+3,vB-3,'l',"BRASS LAMP"); }                      // a yoga corner on the carpet
    if(vR-vL>=14&&vB-vC>=9){ vFloorPatch(vR-5,vB-3,vR-2,vB,13); vIn(vR-5,vB-3,'J',"RED BARRIER"); vIn(vR-2,vB-3,'J',"RED BARRIER"); vIn(vR-5,vB,'J',"RED BARRIER"); vIn(vR-2,vB,'J',"RED BARRIER"); }   // a boxing ring
    vIn(vX-3,vB,'N',"PARK BENCH"); vIn(vX+3,vB,'N',"PARK BENCH"); vIn(vL+1,vC+1,'F',"VENDING MACHINE"); vIn(vR-1,vC+1,'v',"FLAT TV"); vIn(vR,vC+1,'v',"FLAT TV"); vIn(vL+8,vB,'A',"BIG SPEAKERS");
}
static void clubBuild(void){
    vWings('y',"BAR COUNTER");
    vRun(vL,vR,vT,'A',"BIG SPEAKERS",5); vIn(vX-1,vT,'d',"DJ BOOTH"); vIn(vX,vT,'d',"DJ BOOTH"); vIn(vX+1,vT,'d',"DJ BOOTH"); vIn(vX-2,vT,'R',"TURNTABLE"); vIn(vX+2,vT,'R',"TURNTABLE");
    vIn(vX,vT+1,'l',"DISCO BALL"); vIn(vL+2,vT,'l',"NEON ARROW"); vIn(vR-2,vT,'l',"NEON ARROW"); vIn(vX-3,vT,'l',"NEON BORE SIGN"); vIn(vX+3,vT,'l',"NEON BORE SIGN");
    vIn(vL,vC+1,'h',"BAR STOOL"); vIn(vR,vC+1,'h',"BAR STOOL");
    int fx0=vX-4, fx1=vX+4; if(fx0<vL+3) fx0=vL+3; if(fx1>vR-3) fx1=vR-3; vFloorPatch(fx0,vC+2,fx1,vB-1,6);                                                        // the dance floor, lit
    for(int y=vC+3;y<=vB-1;y+=3){ vIn(vL,y,'C',"VELVET SOFA"); vIn(vL+1,y,'t',"ROUND TABLE"); vIn(vL+2,y,'h',"RED ARMCHAIR"); vIn(vR,y,'C',"VELVET SOFA"); vIn(vR-1,y,'t',"ROUND TABLE"); vIn(vR-2,y,'h',"GREEN ARMCHAIR"); }   // the VIP booths
    vIn(fx0,vC+2,'l',"DISCO BALL"); vIn(fx1,vC+2,'l',"DISCO BALL"); vIn(fx0,vB-1,'l',"DISCO BALL"); vIn(fx1,vB-1,'l',"DISCO BALL");
    vIn(vL,vB,'A',"BIG SPEAKERS"); vIn(vR,vB,'A',"BIG SPEAKERS"); vIn(vL+4,vB,'p',"POTTED PALM"); vIn(vR-4,vB,'p',"POTTED PALM");
    if(vB-vC>=8){ vIn(vL+4,vC+3,'U',"BALL PIT"); vIn(vR-4,vC+3,'U',"BALL PIT"); }
}
static void grocerBuild(void){
    static const VI cold[]={{'F',"MINT FRIDGE"},{'F',"STEEL FRIDGE"},{'F',"RED FRIDGE"},{'b',"WINE RACK"}};
    vSeq(vL,vR,vT,cold,4,1); vWings('b',"WHITE BOOKSHELF"); vRun(vL+1,vR-1,vT+1,'y',"MARBLE COUNTER",5);
    vCol(vL,vC+2,vB-1,'F',"STEEL FRIDGE",1);                                                                                  // the dairy and drinks wall
    for(int y=vC+2;y<=vB-1;y++) vIn(vR,y,'Z',(y&1)?"TULIP BED":(y&2)?"RED FLOWER BED":"YELLOW FLOWER BED");                  // the produce crates
    for(int y=vC+3;y<=vB-2;y+=3){ vRun(vL+2,vX-3,y,'b',"WHITE BOOKSHELF",1); vRun(vX+3,vR-2,y,'b',"WHITE BOOKSHELF",1); vRun(vL+3,vX-4,y+1,'b',"WHITE BOOKSHELF",2); vRun(vX+4,vR-3,y+1,'b',"WHITE BOOKSHELF",2); }   // the aisles
    vIn(vL+1,vB,'F',"VENDING MACHINE"); vIn(vR-1,vB,'p',"FICUS"); vIn(vX-3,vC+1,'y',"SNACK CART"); vIn(vX+3,vC+1,'y',"SNACK CART");
    vIn(vL+1,vC+1,'y',"SNACK CART"); vIn(vR-1,vC+1,'p',"FICUS");
}
static void bankBuild(void){
    vWings('d',"RECEPTION DESK"); vRun(vL,vR,vT,'i',"SAFE",3); vRun(vL+1,vR-1,vT,'i',"TRUNK",3); vRun(vL+1,vR-1,vT+1,'d',"COMPUTER DESK",2); vRun(vL+1,vR-1,vT+1,'h',"OFFICE CHAIR",2);
    vIn(vL,vT,'i',"CABINET"); vIn(vR,vT,'i',"CABINET");
    vIn(vX-2,vC+3,'Y',"BOLLARD"); vIn(vX+2,vC+3,'Y',"BOLLARD"); vIn(vX-2,vC+4,'Y',"BOLLARD"); vIn(vX+2,vC+4,'Y',"BOLLARD");                // the queue rope
    for(int y=vC+3;y<=vB-1;y+=3){ vIn(vL,y,'C',"LEATHER SOFA"); vIn(vL,y+1,'C',"LEATHER SOFA"); vIn(vL+2,y,'t',"MARBLE TABLE"); vIn(vL+2,y+1,'p',"FICUS"); }   // the waiting area
    int ox=vR-5;                                                                                                                  // the manager's office, walled in, its door facing the lobby
    if(vR-vX>=7&&vB-vC>=6){ for(int y=vC+1;y<=vC+5;y++){ lifeMap[y][ox]='W'; wallMap[y][ox]=53; } for(int x=ox;x<=vR;x++){ lifeMap[vC+5][x]='W'; wallMap[vC+5][x]=53; } lifeMap[vC+5][ox+1]='D'; lifeMap[vC+5][ox+2]='.'; lifeMap[vC+5][ox+1]='.';
        vIn(vR-1,vC+2,'d',"OAK DESK"); vIn(vR-2,vC+3,'h',"OFFICE CHAIR"); vIn(vR,vC+2,'b',"TALL BOOKSHELF"); vIn(vR,vC+4,'p',"FICUS"); vIn(vR-4,vC+2,'l',"BRASS LAMP"); vIn(vR-3,vC+2,'h',"OFFICE CHAIR"); vIn(vR-3,vC+4,'p',"BONSAI"); }
    for(int x=vL+1;x<=vR-1;x+=4) if(!(x>=vX-3&&x<=vX+3)&&!(x>=vR-6)) vIn(x,vB,'p',(x&4)?"FICUS":"TALL VASE");
    vIn(vX-5,vB-1,'d',"COMPUTER"); vIn(vX+4,vB-1,'d',"COMPUTER"); vIn(vX-5,vB,'h',"OFFICE CHAIR"); vIn(vX+4,vB,'h',"OFFICE CHAIR");
}
static void hospitalBuild(void){
    static const VI sup[]={{'i',"CABINET"},{'i',"CABINET"},{'F',"STEEL FRIDGE"},{'i',"LOCKER"}};
    vSeq(vL,vR,vT,sup,4,1); vWings('d',"LAB BENCH"); vRun(vL+1,vR-1,vT+1,'d',"COMPUTER DESK",3); vRun(vL+1,vR-1,vT+1,'h',"OFFICE CHAIR",3); vIn(vX,vT+1,'l',"TABLE LAMP");
    for(int y=vC+3;y<=vB-1;y+=2){ for(int x=vL;x<=vX-5;x+=3){ vIn(x,y,'S',"CAMP COT"); vIn(x+1,y,'t',"SIDE TABLE"); } }                                  // the ward: cots, a side table between each pair
    for(int y=vC+3;y<=vB-1;y+=2){ for(int x=vR;x>=vX+5;x-=3){ vIn(x,y,'S',(x&1)?"CAMP COT":"CAMP COT"); vIn(x-1,y,'t',"SIDE TABLE"); } }
    for(int y=vC+3;y<=vB-3;y+=2){ vIn(vX-3,y,'h',"WHITE CHAIR"); vIn(vX-4,y,'h',"WHITE CHAIR"); vIn(vX+3,y,'h',"WHITE CHAIR"); vIn(vX+4,y,'h',"WHITE CHAIR"); }   // the waiting rows
    vIn(vX-3,vC+2,'t',"COFFEE TABLE"); vIn(vX+3,vC+2,'F',"VENDING MACHINE"); vIn(vL,vC+1,'p',"FERN"); vIn(vR,vC+1,'p',"FERN"); vIn(vX-6,vC+2,'p',"FICUS"); vIn(vX+6,vC+2,'p',"FICUS");
    if(vB-vC>=8){ vIn(vR,vB,'T',"PINK TOILET"); vIn(vR-1,vB,'H',"BATHROOM SINK"); vIn(vL,vB,'p',"FERN"); vIn(vX-3,vB,'p',"FICUS"); vIn(vX+3,vB,'p',"FICUS"); }
}
static void cafeBuild(void){
    static const VI bar[]={{'c',"KETTLE STAND"},{'F',"STEEL FRIDGE"},{'c',"KETTLE STAND"},{'y',"OVEN"},{'F',"MINT FRIDGE"}};
    vSeq(vL,vR,vT,bar,5,1); vWings('y',"MARBLE COUNTER"); vRun(vL+1,vR-1,vT+1,'y',"OAK COUNTER",3); vIn(vX,vT+1,'l',"BRASS LAMP");
    for(int y=vC+3;y<=vB-3;y+=3) for(int d=4;vX-d>=vL+3;d+=4){ vTable(vX-d,y+(d/4&1),"ROUND TABLE","WICKER CHAIR",3); vTable(vX+d,y+(d/4&1),"ROUND TABLE","WICKER CHAIR",3); }   // tables, staggered
    for(int y=vC+2;y<=vB-4;y+=2){ vIn(vL,y,'C',"BLUE COUCH"); vIn(vL+1,y,'t',"COFFEE TABLE"); }                                                                                      // the sofa wall
    vIn(vR,vC+2,'b',"WHITE BOOKSHELF"); vIn(vR,vC+3,'b',"TALL BOOKSHELF"); vIn(vR,vC+4,'b',"WHITE BOOKSHELF"); vIn(vR-1,vC+4,'h',"RED ARMCHAIR"); vIn(vR-1,vC+2,'l',"BRASS LAMP");   // the book nook
    vIn(vR,vB,'R',"GUITAR STAND"); vIn(vR-1,vB,'A',"BIG SPEAKERS"); vIn(vR,vB-1,'A',"BIG SPEAKERS"); vIn(vR-2,vB,'R',"SYNTHESIZER"); vIn(vR-3,vB,'R',"DRUM KIT");                      // the open-mic stage
    vIn(vL,vC+1,'p',"FICUS"); vIn(vR,vC+1,'p',"MONSTERA"); vIn(vL,vB,'p',"FERN"); vIn(vL+1,vB,'p',"FICUS"); vIn(vX-4,vB,'l',"BRASS LAMP"); vIn(vX+4,vB,'l',"BRASS LAMP");
}
static void salonBuild(void){
    vWings('d',"VANITY"); vRun(vL,vR,vT,'H',"BATHROOM SINK",2); vRun(vL+1,vR-1,vT,'i',"WHITE WARDROBE",2); vRun(vL+1,vR-1,vT+1,'l',"MARBLE BUST",7);
    for(int y=vC+3;y<=vB-3;y+=2){ vIn(vL,y,'d',"VANITY"); vIn(vL+1,y,'h',"OFFICE CHAIR"); vIn(vR,y,'d',"VANITY"); vIn(vR-1,y,'h',"OFFICE CHAIR");
        for(int d=5;vX-d>=vL+2;d+=4){ vIn(vX-d,y,'d',"VANITY"); vIn(vX-d+1,y,'h',"OFFICE CHAIR"); vIn(vX+d,y,'d',"VANITY"); vIn(vX+d-1,y,'h',"OFFICE CHAIR"); } }                          // the styling stations
    vIn(vL,vB,'p',"FICUS"); vIn(vR,vB,'p',"FICUS");
    if(vB-vC>=7){ vIn(vX+3,vB,'C',"PINK LOVESEAT"); vIn(vX+4,vB,'C',"PINK LOVESEAT"); vIn(vX+3,vB-1,'t',"SIDE TABLE"); vIn(vX-3,vB,'b',"COMIC RACK"); vIn(vX-4,vB,'b',"COMIC RACK"); vIn(vX-3,vB-1,'p',"POTTED PALM"); }
    vIn(vL,vC+2,'p',"FICUS"); vIn(vR,vC+2,'p',"FICUS"); vIn(vX-3,vC+2,'l',"MARBLE BUST"); vIn(vX+3,vC+2,'l',"MARBLE BUST");
}
static void petBuild(void){
    static const VI kennel[]={{'p',"DOG HOUSE"},{'b',"TOY BOX"},{'p',"DOG HOUSE"},{'p',"TEDDY BEAR"}};
    vSeq(vL,vR,vT,kennel,4,1); vWings('b',"WHITE BOOKSHELF"); vRun(vL+1,vR-1,vT+1,'p',"BIRD FEEDER",4); vIn(vX,vT+1,'q',"FOUNTAIN");
    for(int y=vC+2;y<=vC+4;y++) for(int x=vL;x<=vL+2;x++) vIn(x,y,'U',"SANDBOX"); vIn(vL+3,vC+2,'p',"DOG HOUSE"); vIn(vL+3,vC+3,'p',"TEDDY BEAR");    // the play pen
    for(int y=vC+4;y<=vB-2;y+=3){ vRun(vL+3,vX-3,y,'b',"WHITE BOOKSHELF",1); vRun(vX+3,vR-3,y,'b',"WHITE BOOKSHELF",1); }                                      // food and toy aisles
    for(int y=vC+2;y<=vB-1;y+=2){ vIn(vR,y,'p',"KIDDIE POOL"); vIn(vR,y+1,'q',"FOUNTAIN"); }                                                                  // the fish and bird side
    vIn(vL,vB,'p',"TEDDY BEAR"); vIn(vL+1,vB,'p',"PINK FLAMINGO"); vIn(vX-3,vB,'p',"FERN"); vIn(vX+3,vB,'p',"TOPIARY BALL"); vIn(vR-1,vB,'p',"BIRD BATH"); vIn(vL,vC+1,'p',"DOG HOUSE");
}
static void libraryBuild(void){
    vWings('d',"OAK DESK"); vRun(vL,vR,vT,'b',"TALL BOOKSHELF",1); vRun(vL+1,vR-1,vT+1,'d',"COMPUTER",5); vIn(vX,vT+1,'l',"GLOBE");
    for(int x=vL;x<=vX-4;x+=3) vCol(x,vC+2,vB-3,'b',(x&1)?"TALL BOOKSHELF":"WHITE BOOKSHELF",1);                                                              // the stacks
    for(int x=vR;x>=vX+4;x-=3) vCol(x,vC+2,vB-3,'b',(x&1)?"WHITE BOOKSHELF":"TALL BOOKSHELF",1);
    vTable(vX-5,vB-1,"OAK DINING TABLE","OFFICE CHAIR",11); vTable(vX+5,vB-1,"OAK DINING TABLE","OFFICE CHAIR",11);                                              // the reading tables
    vIn(vX-3,vB,'l',"GLOBE"); vIn(vX+3,vB,'d',"LECTERN"); vIn(vL,vB,'h',"RED ARMCHAIR"); vIn(vL+1,vB,'l',"BRASS LAMP"); vIn(vR,vB,'h',"GREEN ARMCHAIR"); vIn(vR-1,vB,'l',"BRASS LAMP");
    vIn(vL+1,vB-1,'t',"KIDS TABLE"); vIn(vL+2,vB-1,'b',"TOY BOX"); vIn(vL+1,vB,'p',"BLOCKS TOWER");
}
static void casinoBuild(void){
    vWings('y',"MARBLE COUNTER"); vRun(vL,vR,vT,'i',"SAFE",4); vRun(vL+1,vR-1,vT,'l',"TALL VASE",4); vIn(vX,vT+1,'l',"CHANDELIER"); vIn(vX-5,vT+1,'l',"CHANDELIER"); vIn(vX+5,vT+1,'l',"CHANDELIER");
    vCol(vL,vC+2,vB-1,'v',"SLOT MACHINE",1); vCol(vR,vC+2,vB-1,'v',"SLOT MACHINE",1);
    for(int y=vC+3;y<=vB-3;y+=3){ vIn(vL+3,y,'v',"SLOT MACHINE"); vIn(vL+4,y,'v',"SLOT MACHINE"); vIn(vR-3,y,'v',"SLOT MACHINE"); vIn(vR-4,y,'v',"SLOT MACHINE"); }  // slot islands
    int ty=vC+3+((vB-vC-5)/2); vTable(vX-5,ty,"POKER TABLE","BAR STOOL",11); vTable(vX+5,ty,"POKER TABLE","BAR STOOL",11);
    if(vB-vC>=11){ vTable(vX-5,ty+4,"CARD TABLE","BAR STOOL",11); vTable(vX+5,ty+4,"CARD TABLE","BAR STOOL",11); }
    for(int x=vL+1;x<=vL+4;x++) vIn(x,vB,'y',"BAR COUNTER"); for(int x=vL+1;x<=vL+4;x+=2) vIn(x,vB-1,'h',"BAR STOOL");                                                // the bar
    vIn(vR-1,vB,'p',"POTTED PALM"); vIn(vL+1,vC+1,'p',"POTTED PALM"); vIn(vR-1,vC+1,'p',"POTTED PALM"); vIn(vX-3,vB,'l',"CHANDELIER"); vIn(vX+3,vB,'l',"CHANDELIER");
}
static void cinemaBuild(void){
    vRun(vL,vR,vT,'v',"FLAT TV",1); vWings('y',"BREAKFAST BAR"); vRun(vL,vR,vT+1,'l',"ABSTRACT ART",5);                                          // the screen wall (flat panels side by side)
    for(int y=vC+3;y<=vB-1;y+=2){ for(int x=vL+1;x<=vX-3;x++) vIn(x,y,'C',(x&1)?"RED COUCH":"VELVET SOFA"); for(int x=vX+3;x<=vR-1;x++) vIn(x,y,'C',(x&1)?"VELVET SOFA":"RED COUCH"); }   // the rows of seats
    vIn(vL,vC+1,'F',"VENDING MACHINE"); vIn(vL,vC+2,'F',"VENDING MACHINE"); vIn(vR,vC+1,'y',"SNACK CART"); vIn(vR,vC+2,'y',"SNACK CART"); vIn(vL,vB,'p',"POTTED PALM"); vIn(vR,vB,'p',"FICUS");
    vIn(vX-3,vC+2,'l',"NEON BORE SIGN"); vIn(vX+3,vC+2,'l',"NEON ARROW");
}
static void spaceBuild(void){
    vRun(vL,vR,vT,'v',"BIG TV",2); vWings('d',"LAB BENCH"); vRun(vL+1,vR-1,vT+1,'d',"COMPUTER",2); vRun(vL,vR,vT+1,'h',"OFFICE CHAIR",2);                                    // mission control: the big screens and a console a seat
    for(int y=vC+3;y<=vB-3;y+=3){ vRun(vL+1,vX-3,y,'d',"COMPUTER DESK",2); vRun(vX+3,vR-1,y,'d',"COMPUTER DESK",2); vRun(vL+1,vX-3,y+1,'h',"OFFICE CHAIR",2); vRun(vX+3,vR-1,y+1,'h',"OFFICE CHAIR",2); }
    vIn(vL,vB,'l',"GLOBE"); vIn(vR,vB,'v',"ROBOT BUDDY"); vIn(vL+2,vB,'p',"MANNEQUIN"); vIn(vR-2,vB,'p',"MANNEQUIN"); vIn(vX-3,vB,'l',"GLOBE"); vIn(vX+3,vB,'v',"ROBOT BUDDY");
    vIn(vL,vC+1,'l',"GLOBE"); vIn(vR,vC+1,'v',"ROBOT BUDDY");
}
static void dojoBuild(void){
    vWings('b',"BOARD RACK"); vRun(vL,vR,vT,'l',"TOTEM POLE",5); vRun(vL+2,vR-2,vT,'p',"BONSAI",5); vIn(vX,vT+1,'l',"TROPHY CASE");
    int mx0=vL+3, mx1=vR-3, my0=vC+3, my1=vB-2; if(mx1-mx0>=4&&my1-my0>=2) vFloorPatch(mx0,my0,mx1,my1,13);                                          // the mat
    for(int y=vC+2;y<=vB;y+=2){ vIn(vL,y,'m',"PUNCHING BAG"); vIn(vR,y,'m',"PUNCHING BAG"); }
    vIn(vL,vC+1,'b',"BOARD RACK"); vIn(vR,vC+1,'b',"BOARD RACK"); vIn(vL+1,vB,'p',"BONSAI"); vIn(vR-1,vB,'p',"BONSAI"); vIn(vL+2,vB,'l',"STONE LANTERN"); vIn(vR-2,vB,'l',"STONE LANTERN");
    vTable(vX-5,vB,"ROUND TABLE","FOLDING CHAIR",3); vTable(vX+5,vB,"ROUND TABLE","FOLDING CHAIR",3);
}
static void skshopBuild(void){
    vWings('b',"BOARD RACK"); vRun(vL,vR,vT,'b',"BOARD RACK",1); vIn(vX,vT+1,'l',"NEON BORE SIGN"); vRun(vL+1,vR-1,vT+1,'b',"COMIC RACK",4);
    vCol(vL,vC+2,vC+7,'b',"BOARD RACK",1); vIn(vR,vC+1,'F',"VENDING MACHINE"); vIn(vR,vC+2,'F',"VENDING MACHINE");
    int gx=vR-7; if(gx<vX+3) gx=vX+3;                                                                                                              // a little demo street
    vIn(gx,vB-1,'L',"GRIND BOX"); vIn(gx+1,vB-1,'L',"GRIND BOX"); vRun(gx-1,gx+4,vB-3,'=',"SHORT RAIL",1); vIn(gx+4,vB-1,'Y',"TRAFFIC CONE"); vIn(gx-2,vB-1,'Y',"TRAFFIC CONE"); vIn(gx+2,vB-1,'X',"A FRAME");
    vIn(vL+1,vB,'C',"GREY COUCH"); vIn(vL+2,vB,'C',"GREY COUCH"); vIn(vL+1,vB-2,'v',"FLAT TV"); vIn(vL+3,vB,'t',"COFFEE TABLE"); vIn(vL,vB,'p',"TALL CACTUS");
    for(int x=vX-4;x>=vL+2;x-=3){ vCol(x,vC+3,vC+4,'b',"BOARD RACK",1); vCol(x,vC+7,vC+8,'b',"BOARD RACK",1); }   // islands of decks
    for(int x=vX+4;x<=vR-2;x+=3){ vCol(x,vC+3,vC+4,'b',"BOARD RACK",1); }
    vRun(vL+1,vR-1,vB,'l',"NEON ARROW",9);
}
static void poolBuild(int rx0,int ry0,int rx1,int ry1,int cx){   // THE COMMUNITY POOL: changing rooms along the back (showers, toilets, towels), the water in the middle with its tiled edge, a ladder, the diving board, loungers and umbrellas, a lifeguard chair, a kiddie pool and a snack bar
    int L=rx0+1, R=rx1-1, T=ry0+1, B=ry1-1;
    for(int x=L;x<=L+3;x++) vPut(x,T,'H',"OUTDOOR SHOWER"); for(int x=R-4;x<=R-3;x++) vPut(x,T,'H',"BATHROOM SINK"); vPut(R-1,T,'T',"PINK TOILET"); vPut(R,T,'T',"BLACK TOILET"); vPut(L+4,T,'p',"TOWEL RACK"); vPut(L+5,T,'p',"TOWEL RACK"); vPut(R-2,T,'i',"CABINET");
    vPut(L,T+1,'p',"POTTED PALM"); vPut(R,T+1,'p',"POTTED PALM"); vPut(cx-4,T+1,'p',"BEACH UMBRELLA"); vPut(cx+4,T+1,'p',"BEACH UMBRELLA"); vPut(cx-3,T+1,'p',"TOWEL RACK"); vPut(cx+3,T+1,'y',"SNACK CART");
    int px0=L+3, px1=R-3, py0=T+4, py1=B-3; if(px1-px0<2||py1-py0<1) return; if(px1>px0+9) px1=px0+9; if(py1>py0+4) py1=py0+4;   // (at most 10 x 5 of water)
    for(int y=py0-1;y<=py1+1;y++)for(int x=px0-1;x<=px1+1;x++){ int in=x>=px0&&x<=px1&&y>=py0&&y<=py1; lifeMap[y][x]='.'; floorMap[y][x]=(u8)(in?FL_W0:FL_COPING); }
    vPut(px1+1,py0,'p',"POOL LADDER"); vPut(px0-1,py1,'p',"DIVING BOARD");
    for(int x=px0-1;x<=px1+1;x+=2) if(py1+2<=B&&lifeMap[py1+2][x]=='.'){ vPut(x,py1+2,'N',(x&2)?"SUN LOUNGER":(x&4)?"STRIPED LOUNGER":"PINK LOUNGER"); vPut(x+1,py1+2,'p',"BEACH UMBRELLA"); }
    for(int y=py0;y<=py1+1;y+=2){ vPut(px1+3,y,'N',"SUN LOUNGER"); }
    vPut(px1+3,py0-1,'h',"LIFEGUARD CHAIR"); vPut(px0-3,py0-1,'p',"BEACH UMBRELLA"); vPut(px0-3,py0+1,'N',"STRIPED LOUNGER"); vPut(px0-3,py0+3,'N',"PINK LOUNGER");
    vPut(L,B,'p',"KIDDIE POOL"); vPut(L+1,B,'p',"KIDDIE POOL"); vPut(L,B-1,'p',"KIDDIE POOL"); vPut(L+1,B-1,'p',"POOL FLOAT"); vPut(R,B,'y',"TIKI BAR"); vPut(R-1,B,'y',"TIKI BAR"); vPut(R,B-1,'h',"BAR STOOL"); vPut(R-1,B-1,'h',"BAR STOOL"); vPut(R-3,B,'l',"TIKI TORCH"); vPut(L+3,B,'l',"TIKI TORCH");
}
// ---- THE LOUNGE, THE PLAZA AND THE PARK: the community lots that are not shops. The lounge is a hangout with a stage, a bar corner, couches round low tables, hookahs and a ball pit; the plaza has a fountain,
// benches, flower beds, a market with carts and stalls, trees and lamps; the park has paths, a pond, trees, a playground, picnic tables and grills. ----
static void vGroup(int x,int y,const char*tn){   // a low table with a sofa either side and an armchair at one end
    vIn(x,y,'t',tn); vIn(x-1,y,'C',(x&1)?"VELVET SOFA":"RED COUCH"); vIn(x+1,y,'C',(x&1)?"RED COUCH":"VELVET SOFA"); vIn(x,y+1,'h',(x&2)?"RED ARMCHAIR":"GREEN ARMCHAIR");
}
static void loungeBuild(int x0,int y0,int x1,int y1){
    int rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3, cx=(x0+x1)/2;
    gRoom(rx0,ry0,rx1,ry1,2,NWP+57); gPut(cx,ry1,'D');
    vL=rx0+1; vT=ry0+1; vR=rx1-1; vB=ry1-1; vC=vT-3; vX=cx; vNRs=0;   // (no counter: nothing is held back for it)
    if(vR-vL>=15&&vB-vT>=8){ vRestroom(0); vRestroom(1); }
    vRun(vL,vR,vT,'A',"BIG SPEAKERS",7); vIn(vX-1,vT,'d',"DJ BOOTH"); vIn(vX,vT,'d',"DJ BOOTH"); vIn(vX+1,vT,'d',"DJ BOOTH"); vIn(vX-2,vT,'R',"TURNTABLE"); vIn(vX+2,vT,'R',"TURNTABLE"); vIn(vX-4,vT,'l',"PINK LAVA LAMP"); vIn(vX+4,vT,'l',"GREEN LAVA LAMP");
    vIn(vX,vT+1,'l',"DISCO BALL");
    for(int x=vL;x<=vL+4;x++) vIn(x,vT,'y',"BAR COUNTER"); for(int x=vL;x<=vL+4;x+=2) vIn(x,vT+1,'h',"BAR STOOL"); vIn(vL+5,vT,'F',"WINE FRIDGE");                       // the bar corner
    vIn(vR,vT,'F',"VENDING MACHINE"); vIn(vR-1,vT,'F',"BEER KEG");
    for(int y=vT+3;y<=vB-3;y+=4){ for(int d=5;vX-d>=vL+2;d+=6){ vGroup(vX-d,y,"COFFEE TABLE"); vGroup(vX+d,y,"COFFEE TABLE"); } }                                          // the lounging groups
    for(int y=vT+5;y<=vB-3;y+=4){ vIn(vL,y,'G',"HOOKAH"); vIn(vL+1,y,'h',"BAR STOOL"); vIn(vR,y,'G',"BONG"); vIn(vR-1,y,'h',"BAR STOOL"); }                                       // hookahs and a bong or two
    vIn(vL,vB,'U',"BALL PIT"); vIn(vL+1,vB,'U',"BALL PIT"); vIn(vR,vB,'U',"BALL PIT"); vIn(vR-1,vB,'U',"BALL PIT"); vIn(vL+3,vB,'p',"POTTED PALM"); vIn(vR-3,vB,'p',"MONSTERA");
    vIn(vX-3,vB,'l',"BRASS LAMP"); vIn(vX+3,vB,'l',"BRASS LAMP"); vIn(vX-3,vT+2,'l',"DISCO BALL"); vIn(vX+3,vT+2,'l',"DISCO BALL");
    vRepair(rx0,ry0,rx1,ry1,cx); vWindows(rx0,ry0,rx1,ry1,cx,'f');
    vFront(x0,y0,x1,y1,ry1,cx,"PARK BENCH","TULIP BED");
}
static void plazaBuild(int x0,int y0,int x1,int y1){
    int cx=(x0+x1)/2, my=(y0+y1)/2;
    gBox(x0,y0,x1,y1,3); gBox(cx-4,my-3,cx+4,my+3,13); gBox(cx-3,my-2,cx+3,my+2,3);   // the checker paving with a red tiled ring round the fountain
    vL=x0+1; vT=y0+1; vR=x1-1; vB=y1-2; vC=-5; vX=cx; vNRs=0;
    vPut(cx,my,'q',"FOUNTAIN");
    for(int d=-3;d<=3;d+=3){ vPut(cx+d,my-3,'N',"PARK BENCH"); vPut(cx+d,my+3,'N',"PARK BENCH"); } vPut(cx-4,my,'N',"PARK BENCH"); vPut(cx+4,my,'N',"PARK BENCH");
    for(int x=x0+3;x<=x1-1;x+=3){ vPut(x,y0+1,'Z',(x&1)?"TULIP BED":"RED FLOWER BED"); vPut(x,y1-2,'Z',(x&1)?"YELLOW FLOWER BED":"TULIP BED"); }
    for(int y=y0+3;y<=y1-4;y+=4){ vPut(x0+1,y,'p',"CHERRY BLOSSOM"); vPut(x1-1,y,'p',"BIRCH TREE"); vPut(x0+2,y,'l',"STREET LAMP"); vPut(x1-2,y,'l',"STREET LAMP"); }
    vPut(x0+3,my-1,'y',"SNACK CART"); vPut(x0+5,my-1,'y',"SNACK CART"); vPut(x0+3,my+1,'t',"CARD TABLE"); vPut(x0+5,my+1,'t',"CARD TABLE"); vPut(x0+4,my+1,'h',"FOLDING CHAIR");   // the market end
    vPut(x1-3,my-1,'y',"TIKI BAR"); vPut(x1-5,my-1,'y',"SNACK CART"); vPut(x1-3,my+1,'t',"CARD TABLE"); vPut(x1-5,my+1,'t',"CARD TABLE"); vPut(x1-4,my+1,'h',"FOLDING CHAIR");
    vPut(cx-2,my-5,'l',"GARDEN STATUE"); vPut(cx+2,my-5,'p',"SUNDIAL"); vPut(cx,y0+2,'K',"WISHING WELL"); vPut(cx+2,my+5,'R',"BIG SPEAKERS");
}
static void parkBuild(int x0,int y0,int x1,int y1){
    int cx=(x0+x1)/2, my=(y0+y1)/2;
    for(int y=y0;y<=y1;y++) for(int x=x0;x<=x1;x++) floorMap[y][x]=11;
    for(int y=y0+1;y<=y1-1;y++) floorMap[y][cx]=7; for(int x=x0+2;x<=x1-2;x++) floorMap[my][x]=7;                                   // a path from the gate to the middle and across
    for(int y=my-3;y<=my+3;y++) for(int x=cx-5;x<=cx+5;x++){ int d=(x-cx)*(x-cx)*4/9+(y-my)*(y-my); if(d==9||d==10) floorMap[y][x]=7; }    // a ring round the middle
    vL=x0+1; vT=y0+1; vR=x1-1; vB=y1-2; vC=-5; vX=cx; vNRs=0;
    int px0=x1-7, py0=y0+3, px1=x1-2, py1=y0+7; if(px0>cx+2&&py1<my-1){ for(int y=py0-1;y<=py1+1;y++) for(int x=px0-1;x<=px1+1;x++){ int in=x>=px0&&x<=px1&&y>=py0&&y<=py1; lifeMap[y][x]='.'; floorMap[y][x]=(u8)(in?FL_W0:FL_COPING); } }   // a pond
    static const char* const tr[6]={"OAK TREE","BIRCH TREE","PINE TREE","CHERRY BLOSSOM","AUTUMN MAPLE","WILLOW"};
    for(int x=x0+1,k=0;x<=x1-1;x+=3,k++){ vPut(x,y0+1,'p',tr[k%6]); if(x<cx-1||x>cx+1) vPut(x+1,y1-2,'p',tr[(k+3)%6]); }
    for(int y=y0+3,k=0;y<=y1-4;y+=4,k++){ vPut(x0+1,y,'p',tr[(k+1)%6]); vPut(x1-1,y,'p',tr[(k+4)%6]); }
    vPut(cx-2,my-1,'N',"PARK BENCH"); vPut(cx+2,my-1,'N',"PARK BENCH"); vPut(cx-2,my+1,'N',"PARK BENCH"); vPut(cx+2,my+1,'N',"PARK BENCH"); vPut(cx,my-2,'q',"BIRD BATH"); vPut(cx,my+2,'Y',"HYDRANT");
    vPut(x0+3,my-3,'t',"OAK DINING TABLE"); vPut(x0+2,my-3,'h',"FOLDING CHAIR"); vPut(x0+4,my-3,'h',"FOLDING CHAIR"); vPut(x0+3,my-4,'y',"BBQ GRILL");              // a picnic spot with a grill
    vPut(x0+3,my+3,'t',"OAK DINING TABLE"); vPut(x0+2,my+3,'h',"FOLDING CHAIR"); vPut(x0+4,my+3,'h',"FOLDING CHAIR"); vPut(x0+5,my+3,'y',"BBQ GRILL");
    vPut(x1-4,my+3,'U',"SWING SET"); vPut(x1-5,my+3,'U',"SWING SET"); vPut(x1-3,my+3,'U',"SANDBOX"); vPut(x1-2,my+3,'U',"SANDBOX"); vPut(x1-4,my+4,'h',"ROCKING HORSE"); vPut(x1-2,my+4,'U',"BOUNCY CASTLE");   // the playground
    for(int x=x0+2;x<=x1-2;x+=5){ vPut(x,my+1,'Z',(x&1)?"TULIP BED":"RED FLOWER BED"); }
    vPut(cx-5,y1-3,'l',"STREET LAMP"); vPut(cx+5,y1-3,'l',"STREET LAMP"); vPut(x0+2,y1-3,'p',"GARDEN GNOME"); vPut(x1-2,y1-3,'p',"BIRD FEEDER");
}
static void venueBuild(int x0,int y0,int x1,int y1,int ct){   // nbTemplate: a walled room, the counter across it, restrooms, then the hall of that kind of place (above), windows, and a front yard; you arrive outside the door
    vRemoved=0;
    if(ct==CT_LOUNGE){ loungeBuild(x0,y0,x1,y1); return; }   // (the lots that are not shops: nbTemplate calls these three for those kinds too)
    if(ct==CT_PLAZA){ plazaBuild(x0,y0,x1,y1); return; }
    if(ct==CT_PARK){ parkBuild(x0,y0,x1,y1); return; }
    const Venue*v=venueOf(ct); if(!v) return;
    int cx=(x0+x1)/2, rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3;
    gBox(x0,y0,x1,y1,ct==CT_POOL?6:7); gRoom(rx0,ry0,rx1,ry1,v->fl,v->wp); gPut(cx,ry1,'D');
    if(ct!=CT_ARCADE&&ct!=CT_BAR) for(int x=cx-2;x<=cx+2;x++) gPut(x,ry0+3,'@');   // (the arcade's and the bar's halls lay their own counters)
    vL=rx0+1; vT=ry0+1; vR=rx1-1; vB=ry1-1; vC=ry0+3; vX=cx; vNRs=0;
    if(vHasRestrooms(ct)&&vR-vL>=15&&vB-vC>=8){ vRestroom(0); if(ct!=CT_ARCADE) vRestroom(1); }   // (the arcade has its hall's own too)
    switch(ct){
        case CT_TACO: tacoBuild(); break;           case CT_GYM: gymBuild(); break;           case CT_CLUB: clubBuild(); break;         case CT_GROCER: grocerBuild(); break;
        case CT_BANK: bankBuild(); break;           case CT_HOSPITAL: hospitalBuild(); break; case CT_CAFE: cafeBuild(); break;         case CT_SALON: salonBuild(); break;
        case CT_PETSHOP: petBuild(); break;         case CT_LIBRARY: libraryBuild(); break;   case CT_CASINO: casinoBuild(); break;     case CT_CINEMA: cinemaBuild(); break;
        case CT_SPACE: spaceBuild(); break;         case CT_DOJO: dojoBuild(); break;         case CT_SKSHOP: skshopBuild(); break;
        case CT_ARCADE: arcadeBuild(rx0,ry0,rx1,ry1,cx,v->wp); break;   // the arcade has its own hall (above): it lays its own counter
        case CT_BAR: barBuild(rx0,ry0,rx1,ry1,cx); break;               // the bar too
        default: break; }
    if(ct==CT_POOL) poolBuild(rx0,ry0,rx1,ry1,cx);
    vRepair(rx0,ry0,rx1,ry1,cx);
    vWindows(rx0,ry0,rx1,ry1,cx,ct==CT_SKSHOP||ct==CT_CLUB||ct==CT_SPACE?'f':'E');
    vFront(x0,y0,x1,y1,ry1,cx,"PARK BENCH",ct==CT_BANK||ct==CT_LIBRARY?"ROSE BUSH":"RED FLOWER BED");
}
// ---- the counter, drawn by code ----
static void venueDrawCounter(int sx,int sy,int t){
    const Venue*v=venueHere(); u16 a=v?v->a:RGB(14,10,6), b=v?v->b:RGB(26,22,10); int ct=v?v->ct:-1;
    rect(sx-8,sy-9,17,8,a); rect(sx-8,sy-10,17,2,b); rect(sx-8,sy-2,17,1,RGB(3,3,4));   // the counter, its trim on top
    int ix=sx-2, iy=sy-16;   // the little sign on it
    switch(ct){
        case CT_TACO: rect(ix-2,iy+2,9,3,RGB(31,24,6)); rect(ix-1,iy+1,7,1,RGB(31,24,6)); px(ix,iy+3,RGB(8,24,6)); px(ix+3,iy+2,RGB(30,6,4)); px(ix+5,iy+3,RGB(8,24,6)); break;   // a taco
        case CT_GYM: rect(ix-2,iy+2,9,1,RGB(20,20,22)); rect(ix-3,iy,2,5,RGB(8,8,10)); rect(ix+6,iy,2,5,RGB(8,8,10)); break;                                                  // a dumbbell
        case CT_ARCADE: rect(ix-2,iy-1,8,6,RGB(4,4,6)); rect(ix-1,iy,6,4,((t>>2)&1)?RGB(4,28,30):RGB(30,6,28)); break;                                                          // a flashing screen
        case CT_CLUB: rect(ix,iy-2,4,4,((t>>1)&3)==0?RGB(30,6,28):((t>>1)&3)==1?RGB(4,28,30):((t>>1)&3)==2?RGB(31,28,6):WHITE); break;                                       // a disco light
        case CT_GROCER: rect(ix,iy,5,4,RGB(28,4,4)); px(ix+2,iy-1,RGB(8,22,6)); break;                                                                                          // an apple
        case CT_BANK: rect(ix,iy-2,1,7,RGB(31,28,6)); rect(ix-2,iy,5,1,RGB(31,28,6)); rect(ix-2,iy+3,5,1,RGB(31,28,6)); break;                                                  // a dollar-ish sign
        case CT_HOSPITAL: rect(ix-1,iy,7,2,RGB(30,4,4)); rect(ix+2,iy-3,2,8,RGB(30,4,4)); break;                                                                                 // a red cross
        case CT_CAFE: rect(ix,iy+1,5,4,WHITE); px(ix+5,iy+2,WHITE); if((t>>3)&1) px(ix+2,iy-1,RGB(24,24,24)); break;                                                            // a cup, steaming
        case CT_BAR: rect(ix,iy,4,5,RGB(30,22,6)); rect(ix,iy,4,1,WHITE); break;                                                                                                // a pint
        case CT_SALON: line(ix,iy,ix+5,iy+5,RGB(26,26,28)); line(ix+5,iy,ix,iy+5,RGB(26,26,28)); break;                                                                       // scissors
        case CT_PETSHOP: rect(ix+1,iy+2,4,3,RGB(26,16,8)); px(ix,iy,RGB(26,16,8)); px(ix+2,iy-1,RGB(26,16,8)); px(ix+4,iy-1,RGB(26,16,8)); px(ix+6,iy,RGB(26,16,8)); break;   // a paw
        case CT_LIBRARY: rect(ix-1,iy,7,5,RGB(8,12,24)); rect(ix+2,iy,1,5,RGB(28,28,24)); break;                                                                                 // a book
        case CT_CASINO: rect(ix,iy,5,5,WHITE); px(ix+1,iy+1,RGB(4,4,6)); px(ix+3,iy+3,RGB(4,4,6)); px(ix+2,iy+2,RGB(4,4,6)); break;                                           // a die
        case CT_CINEMA: rect(ix-1,iy,7,5,RGB(6,6,8)); px(ix,iy+1,WHITE); px(ix+4,iy+1,WHITE); px(ix,iy+3,WHITE); px(ix+4,iy+3,WHITE); break;                                  // a film reel
        case CT_POOL: rect(ix-2,iy+2,9,3,RGB(6,18,30)); if((t>>2)&1) px(ix+2,iy+1,WHITE); break;                                                                                // water
        case CT_SPACE: rect(ix+1,iy-1,3,5,WHITE); px(ix+2,iy-2,RGB(30,6,4)); px(ix,iy+3,RGB(30,6,4)); px(ix+4,iy+3,RGB(30,6,4)); px(ix+2,iy+4,((t>>1)&1)?RGB(31,28,6):RGB(31,14,4)); break;   // a rocket, its flame flickering
        case CT_SKSHOP: rect(ix-2,iy+1,9,2,RGB(26,14,6)); rect(ix-2,iy+1,9,1,RGB(31,22,4)); px(ix-1,iy+3,RGB(4,4,6)); px(ix+5,iy+3,RGB(4,4,6)); break;   // a skateboard
        case CT_DOJO: rect(ix-2,iy+2,9,2,RGB(3,3,4)); rect(ix+1,iy+1,3,4,RGB(3,3,4)); px(ix+1,iy+5,RGB(3,3,4)); px(ix+3,iy+5,RGB(3,3,4)); break;                              // a black belt
        default: break; }
}
// ---- the spacecraft: drawn by code, in the live room and on the town map ----
static int lnrHere(void);   // lunrado.h
static void vnRocket(int bx,int by,int t){   // the rocket on its pad, bx = its middle, by = where it stands (the live room: standing behind the counter, 52 px tall)
    for(int r=0;r<14;r++){ int w=1+r*5/13; rect(bx-w,by-50+r,w*2,1,RGB(30,6,4)); }   // the nose cone
    rect(bx-6,by-36,12,36,RGB(29,29,30)); rect(bx-6,by-36,2,36,RGB(21,21,23)); rect(bx+3,by-36,2,36,RGB(31,31,31));   // the body, shaded left, lit right
    rect(bx-6,by-20,12,3,RGB(30,6,4)); rect(bx-6,by-36,12,1,RGB(18,18,20));            // a stripe, the seam under the nose
    rect(bx-2,by-32,4,4,RGB(4,10,22)); px(bx-1,by-31,RGB(14,24,31)); rect(bx-2,by-26,4,4,RGB(4,10,22)); px(bx-1,by-25,RGB(14,24,31));   // two portholes
    for(int r=0;r<12;r++){ rect(bx-6-r/2-1,by-12+r,2+r/3,1,RGB(26,5,3)); rect(bx+5+r/2-r/3,by-12+r,2+r/3,1,RGB(26,5,3)); }   // the fins
    rect(bx-3,by-2,6,2,RGB(8,8,10));   // the nozzle
    rect(bx+9,by-46,2,46,RGB(12,12,14)); for(int y=by-44;y<by;y+=6) rect(bx+9,y,2,1,RGB(20,20,22)); rect(bx+6,by-34,4,2,RGB(16,16,18));   // the gantry, its arm to the hatch
    if((t>>2)&1) px(bx,by-52,RGB(31,6,4)); if((t>>3)&1) px(bx+10,by-47,RGB(31,28,6));   // the beacons
}
static void vnLander(int bx,int by,int t){   // LUNRADO: a lunar lander on its legs and a flag on the regolith (26 px tall)
    line(bx-8,by-8,bx-15,by,RGB(20,20,22)); line(bx+8,by-8,bx+15,by,RGB(20,20,22)); line(bx-4,by-6,bx-5,by,RGB(18,18,20)); line(bx+4,by-6,bx+5,by,RGB(18,18,20));   // the legs
    rect(bx-17,by,6,1,RGB(26,26,27)); rect(bx+11,by,6,1,RGB(26,26,27));                // the foot pads
    rect(bx-9,by-18,18,11,RGB(27,20,6)); rect(bx-9,by-18,18,1,RGB(31,26,10)); rect(bx-9,by-8,18,1,RGB(18,13,3));   // the foil-wrapped descent stage
    rect(bx-7,by-26,14,8,RGB(23,23,24)); rect(bx-7,by-26,2,8,RGB(16,16,18)); rect(bx-3,by-24,6,3,RGB(4,10,22)); px(bx-2,by-23,RGB(14,24,31));   // the crew cabin and its window
    line(bx+3,by-26,bx+8,by-34,RGB(24,24,25)); rect(bx+6,by-36,5,2,RGB(28,28,29));       // the antenna and its dish
    rect(bx-19,by-22,1,22,RGB(24,24,25)); rect(bx-18,by-22,6,4,RGB(30,6,4)); rect(bx-18,by-18,6,2,WHITE);   // a flag on a pole
    if((t>>2)&1) px(bx,by-27,RGB(31,6,4));
}
static void venueDrawShip(int sx,int sy,int ox,int oy,int t){   // the middle tile of the SPACEPORT's counter: its spacecraft stands behind it (before the counter is drawn, so the counter hides its foot)
    const Venue*v=venueHere(); if(!v||v->ct!=CT_SPACE) return;
    int mid=(ox>=2&&ox+2<MW&&lifeMap[oy][ox-2]=='@'&&lifeMap[oy][ox+2]=='@')||(oy>=2&&oy+2<MH&&lifeMap[oy-2][ox]=='@'&&lifeMap[oy+2][ox]=='@'); if(!mid) return;
    if(lnrHere()) vnLander(sx,sy-6,t); else vnRocket(sx,sy-6,t);
}
static void vnShipIcon(int sx,int sy,int k,int hw2){   // the town map: a launch pad, the gantry and the rocket standing on it
    nbIsoBox(sx,sy,hw2,k,hw2/2,nbTint(RGB(16,16,18)),nbTint(RGB(21,21,23)),nbTint(RGB(10,10,12)),nbTint(RGB(13,13,15)));   // the pad
    int by=sy-k, w=k+k/2+1;   // the rocket: base, half width
    rect(sx-w,by-12*k,w*2,10*k,nbTint(RGB(29,29,30))); rect(sx-w,by-12*k,k,10*k,nbTint(RGB(21,21,23)));   // body, shaded left
    for(int r=0;r<4*k;r++){ int ww=(r*w)/(4*k)+(r>0); rect(sx-ww,by-16*k+r,ww*2+1,1,nbTint(RGB(30,6,4))); }   // the nose
    rect(sx-w,by-7*k,w*2,k,nbTint(RGB(30,6,4))); rect(sx-k/2,by-10*k,k+1,k,nbTint(RGB(4,10,22)));        // a stripe, a porthole
    rect(sx-w-k,by-5*k,k,3*k,nbTint(RGB(26,5,3))); rect(sx+w,by-5*k,k,3*k,nbTint(RGB(26,5,3)));           // the fins
    int gx=sx+w+2*k; rect(gx,by-14*k,k,14*k,nbTint(RGB(12,12,14))); rect(gx-k,by-9*k,k,k,nbTint(RGB(16,16,18)));   // the gantry and its arm
    if((uiTicks>>4)&1) px(sx,by-17*k,nbTint(RGB(31,6,4)));
}
static void venueDrawIcon(int ct,int sx,int sy,int k,int hw2){   // the town map: a box in the venue's colours, a sign in the other
    const Venue*v=venueOf(ct); if(!v) return;
    if(ct==CT_SPACE){ vnShipIcon(sx,sy,k,hw2); return; }   // (the spaceport: its rocket on the pad)
    nbIsoBox(sx,sy,hw2,3*k,hw2/2,nbTint(v->a),nbTint(nbLit(v->a,4)),nbTint(v->b),nbTint(nbLit(v->b,-4)));
    rect(sx-hw2/3,sy-4*k,2*hw2/3+1,k,nbTint(v->b));
}
// ---- the menu ----
static char vMsg[48] EWRAM_BSS;
static int vNear(void){ int tx=(int)(lfx>>8), ty=(int)(lfy>>8); for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=tx+dx, y=ty+dy; if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='@') return 1; } return 0; }
static const char* venueHint(void){ const Venue*v=venueHere(); return v&&vNear()?v->hint:0; }
static void vAdd(int*v,int d){ *v+=d; if(*v>100) *v=100; if(*v<0) *v=0; }
static u32 vWorkAt EWRAM_BSS;   // a shift: one every two game hours
static int lnrCanDo(int sp); static void lnrDo(int sp); static int lnrHere(void); static int lnrShut(void); static int lnrFuel(void);   // lunrado.h
static int djCanDo(int sp); static void djDo(int sp);   // dojo.h
static int exSeatFree(void); static void exSparCall(void);   // expand.h
// ARCADE TICKETS: the cabinets, the ticket game and the dance machine pay out tickets; the PRIZE COUNTER spends them. (Saved: the TK_OFF block below, and the player save's chunk 'I'.)
static u16 vTix EWRAM_BSS;
#define TK_OFF 7545   // SRAM: 'T' 'K', tickets (2 bytes), checksum. The free end of the handheld's block (after the HORDE record, before the board: horde.h checks it)
#define TK_LEN 5
static u8 tkSum(volatile const u8*m){ return (u8)(0x54+m[2]*3+m[3]*7); }
static void tkSave(void){ volatile u8*m=SRAM_BASE+TK_OFF; m[0]='T'; m[1]='K'; m[2]=(u8)vTix; m[3]=(u8)(vTix>>8); m[4]=tkSum(m); }
static int tkLoadFrom(volatile const u8*m){ if(m[0]!='T'||m[1]!='K'||m[4]!=tkSum(m)) return 0; int v=m[2]|(m[3]<<8); if(v>9999) return 0; vTix=(u16)v; return 1; }
static void tkLoad(void){ vTix=0; tkLoadFrom(SRAM_BASE+TK_OFF); }   // (no block yet: no tickets)
static void sgEncTix(SlW*w){ tkSave(); volatile u8*m=SRAM_BASE+TK_OFF; for(int i=0;i<TK_LEN;i++) slwPut(w,m[i]); }   // the player save's chunk 'I' (savegame.h)
typedef struct { const char*nm; u16 cost; signed char food, nrg, com, soc; u8 mood; const char*say; } VPrize;
static const VPrize vPrz[]={
    {"CANDY",             10, 15,0,0,0, M_EAT,  "SUGAR RUSH"},
    {"BOUNCY BALL",       25, 0,5,10,5, M_WANT, "BOING BOING BOING"},
    {"PLUSH TEDDY",       60, 0,0,25,5, M_COMBO,"IT HAS A TINY HAT"},
    {"MEGA TROPHY",      150, 0,0,10,30,M_WANT, "NUMBER ONE  AT SOMETHING"},
    {"GOLDEN TICKET",    400, 0,0,0,0,  M_PAY,  "TRADED FOR 500 SIMOLEONS"},
};
#define NPRZ ((int)(sizeof(vPrz)/sizeof(vPrz[0])))
static void vTixEarn(int i){   // i: the action's row in vAct (cabinet 12, ticket game 13, dance machine 14)
    int t=i==13?(lnote==vMsg?18:4):3+(int)(rnd8()%5); vTix=(u16)(vTix+t>9999?9999:vTix+t); tkSave(); sgDirty=1;
    char*e; if(lnote==vMsg){ e=vMsg; while(*e) e++; } else e=simCat(vMsg,lnote?lnote:"");
    e=simCat(e,"  +"); e=simCatN(e,t); simCat(e," TIX"); lnote=vMsg; lnoteT=110;
}
static void arcadePrizes(void){
    for(;;){
        char tt[32]; const char* it[6]; char lb[6][30]; int n=0;
        { char*e=simCat(tt,"PRIZES  "); e=simCatN(e,vTix); simCat(e," TIX"); }
        for(int i=0;i<NPRZ;i++){ char*e=simCat(lb[n],vPrz[i].nm); e=simCat(e,"  "); e=simCatN(e,vPrz[i].cost); simCat(e," TIX"); it[n]=lb[n]; n++; }
        int c=menu(tt,it,n); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
        if(c<0) return;
        const VPrize*p=&vPrz[c];
        if(vTix<p->cost){ lnote="NOT ENOUGH TICKETS"; lnoteT=60; voxPlay(V_nah); continue; }
        vTix=(u16)(vTix-p->cost); tkSave(); sgDirty=1;
        vAdd(&lfood,p->food); vAdd(&sNrg,p->nrg); vAdd(&sCom,p->com); vAdd(&sSoc,p->soc); if(p->mood) moodEvent(p->mood);
        if(c==NPRZ-1) simMoneyAdd(500);   // the golden ticket
        lnote=p->say; lnoteT=110; voxPlay(V_yahoo); simsSave(); return;
    }
}
#include "arcgames.h"   // the arcade's MINI GAMES (arcadeGames, arcGameRun)
static void venueDo(const VAct*a){
    int price=a->price; if(a->sp==VS_LAUNCH&&exSeatFree()) price=0;   // (BORE-OUT MAN!: the mission booked your seat)
    if(a->fl&VF_ADULT&&stage<AG_ADULT){ lnote="GROWN-UPS ONLY"; lnoteT=60; voxPlay(V_nah); return; }
    if(a->fl&VF_NIGHT&&simMin<18*60&&simMin>=4*60){ lnote="COME BACK AFTER 6 PM"; lnoteT=70; return; }
    if(!djCanDo(a->sp)||!lnrCanDo(a->sp)) return;
    if(price>0&&simMoney<price){ lnote="NOT ENOUGH SIMOLEONS"; lnoteT=60; voxPlay(V_nah); return; }
    if((a->sp==VS_LAUNCH||a->sp==VS_SPACEWALK||a->sp==VS_SPAR)&&stage<AG_TEEN){ lnote="KIDS WAIT OUTSIDE"; lnoteT=60; voxPlay(V_nah); return; }
    if(a->sp==VS_WORK){ u32 now=(u32)simDay*1440u+(u32)simMin; if(vWorkAt&&now<vWorkAt){ lnote="YOUR SHIFT IS NOT UP YET"; lnoteT=60; return; } if(sNrg<20){ lnote="TOO TIRED TO WORK"; lnoteT=60; return; } vWorkAt=now+120; }
    if(price>0) simMoney-=price; else if(price<0) simMoneyAdd(-price);
    vAdd(&lfood,a->food); vAdd(&sNrg,a->nrg); vAdd(&sHyg,a->hyg); vAdd(&sCom,a->com); vAdd(&sSoc,a->soc);
    if(a->mood) moodEvent(a->mood);
    if(a->sk<SK_N&&a->sp!=VS_GAMBLE&&a->skn) skGain(a->sk,a->skn);
    lstun=a->stun; lsp=0; lgrind=0; lnote=a->say?a->say:a->nm; lnoteT=110;
    switch(a->sp){
        case VS_MUNCH: if(lchill>0){ moodEventN(M_EAT,3); lnote="THE MUNCHIES  DEFEATED"; lnoteT=130; voxPlay(V_yahoo); } else sfxPlay(SFX_MUNCH); break;
        case VS_ADOPT: lstun=0; petAdopt(); break;
        case VS_MOVIE: tvClip=1; break;   // (the life loop plays the clip like a pause menu: main.c)
        case VS_GAMBLE: { int win=((int)rnd8()*100>>8)<a->skn; if(win){ int p=(a->price?a->price:2)*a->sk; simMoneyAdd(p); char*e=simCat(vMsg,"YOU WON \xC2\xA7"); simCatN(e,p); lnote=vMsg; moodEvent(M_PAY); voxPlay(V_yahoo); }
            else { static const char* const lose[4]={"THE HOUSE WINS","SO CLOSE","NOTHING","DOUBLE OR NOTHING"}; lnote=lose[rnd8()&3]; voxPlay(V_laughing_small_or_not); } lnoteT=110; break; }
        case VS_HEAL: lhp=HP_MAX; break;
        case VS_HAIR: { int ns=0; for(int k=0;k<12;k++){ int h=(int)(rnd8()%NHAIR); if((stMaskHair[stage]>>h)&1){ look[LK_HSTYLE]=(u8)h; ns=1; break; } } (void)ns;
            look[LK_HCOL]=(u8)(rnd8()%stSwatches[stage]); fixLook(); buildLook(); setColors(); persSave(); bakeSprites(); liveInvalidate(); lnote="A WHOLE NEW YOU"; lnoteT=110; voxPlay(V_yeha); break; }
        case VS_WORK: { int pay=25+skLvl(a->sk<SK_N?a->sk:SK_CHARM)*6; simMoneyAdd(pay); char*e=simCat(vMsg,a->say?a->say:"SHIFT DONE"); e=simCat(e,"  \xC2\xA7"); simCatN(e,pay); lnote=vMsg; moodEvent(M_PAY); break; }
        case VS_ROB: if(((int)rnd8()*100>>8)<35){ int p=1500+(int)rnd8()*12; simMoneyAdd(p); copCrime(6); char*e=simCat(vMsg,"YOU GOT \xC2\xA7"); e=simCatN(e,p); simCat(e,"  RUN"); lnote=vMsg; voxPlay(V_yahoo); }
            else { copCrime(8); lnote="THE ALARM WENT OFF"; voxPlay(V_shriek); } lnoteT=130; break;
        case VS_GROCER: for(int m=0;m<hhN;m++) hhM[m].need[HN_FOOD]=(u8)(hhM[m].need[HN_FOOD]>60?100:hhM[m].need[HN_FOOD]+40); lnote="STOCKED UP FOR THE HOUSE"; break;
        case VS_PETFOOD: if(!petN){ lnote="YOU HAVE NO PETS"; simMoney+=a->price; break; } for(int i=0;i<petN;i++){ petM[i].need[PN_FOOD]=100; if(petM[i].love<98) petM[i].love+=2; } lnote="THE PETS WILL EAT LIKE KINGS"; break;
        case VS_PETTOY: if(!petN){ lnote="YOU HAVE NO PETS"; simMoney+=a->price; break; } for(int i=0;i<petN;i++){ petM[i].need[PN_FUN]=(u8)(petM[i].need[PN_FUN]>60?100:petM[i].need[PN_FUN]+40); } lnote="SQUEAK SQUEAK SQUEAK"; break;
        case VS_PETGROOM: if(!petN){ lnote="YOU HAVE NO PETS"; simMoney+=a->price; break; } for(int i=0;i<petN;i++) petM[i].love=(u8)(petM[i].love>90?100:petM[i].love+10); lnote="FLUFFY AND FABULOUS"; break;
        case VS_LAUNCH: exTake=EXT_LAUNCH; lnote=price?"STRAP IN":"YOUR SEAT IS BOOKED  STRAP IN"; break;
        case VS_SPACEWALK: exTake=EXT_SWTRAIN; lnote="SUIT UP"; break;
        case VS_SPAR: exSparCall(); break;
        case VS_CLASS: case VS_BAG: case VS_BELT: djDo(a->sp); break;
        case VS_FUND: case VS_HOME: case VS_PHOTO: case VS_TOUR: case VS_ROCK: case VS_SYSJOB: case VS_SATJOB: case VS_GAUGE: case VS_REFUEL10: case VS_REFUEL35: lnrDo(a->sp); break;
        case VS_FIGHTER: exTake=EXT_FIGHTER; lnote="INSERT COIN"; break;
        case VS_BOARD: if(bdOwn){ simMoney+=price; lnote="YOU ALREADY HAVE A BOARD"; break; } bdGot(1,"A BRAND NEW BOARD  RIDE IT OUTSIDE"); break;
        case VS_PRODECK: if(bdOwn>=2){ simMoney+=price; lnote="YOU ALREADY RIDE A PRO DECK"; break; } bdGot(2,"A PRO DECK  FASTER  MORE POP"); break;
        default: break; }
    if(a->nm&&(a-vAct==VA_CABINET||a-vAct==13||a-vAct==14)) vTixEarn((int)(a-vAct));
    stQEvent(QE_VACT,(int)(a-vAct));   // (the packs' missions: expand.h TD)
    simsSave();
}
static int venueUse(void){   // R next to a venue's counter. 1 = handled
    const Venue*v=venueHere(); if(!v||!vNear()) return 0;
    char tt[40]; const char* it[10]; char lb[10][32]; int id[10], n=0;
    { char*e=simCat(tt,ctNm[v->ct]); e=simCat(e,"  \xC2\xA7"); simCatMoney(e,simMoney,1); }
    int a0=v->a0, an=v->an; if(v->ct==CT_SPACE&&lnrHere()){ a0=VA_LUN; an=9; }
    for(int i=0;i<an&&n<10;i++){ const VAct*a=&vAct[a0+i]; char*e=simCat(lb[n],a->nm);
        if(a->price>0){ e=simCat(e,"  \xC2\xA7"); simCatN(e,a->price); } else if(a->price<0){ e=simCat(e,"  +\xC2\xA7"); simCatN(e,-a->price); }
        it[n]=lb[n]; id[n++]=a0+i; }
    if(v->ct==CT_SPACE&&!lnrHere()&&lnrShut()&&n<10){ it[n]="FUND THE SPACEPORT  \xC2\xA7" "300000"; id[n++]=VA_LUN+9; }
    if(v->ct==CT_SPACE&&!lnrHere()){   // the rocket's tank: the gauge, and the two ways to fill it
        if(n<10){ int f=lnrFuel(); char*e=simCat(lb[n],"FUEL GAUGE  "); e=simCatN(e,f); simCat(e,f==1?" TRIP":" TRIPS"); it[n]=lb[n]; id[n++]=VA_LUN+10; }
        if(n<10){ it[n]="REFUEL  10 TRIPS  \xC2\xA7" "50000"; id[n++]=VA_LUN+11; }
        if(n<10){ it[n]="REFUEL  35 TRIPS  \xC2\xA7" "250000"; id[n++]=VA_LUN+12; } }
    if(v->ct==CT_ARCADE&&n<10){ it[n]="MINI GAMES"; id[n++]=-2; }
    if(v->ct==CT_ARCADE&&n<10){ char*e=simCat(lb[n],"PRIZE COUNTER  "); e=simCatN(e,vTix); simCat(e," TIX"); it[n]=lb[n]; id[n++]=-1; }
    int c=menu(tt,it,n); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    if(id[c]==-2){ arcadeGames(); return 1; }
    if(id[c]<0){ arcadePrizes(); return 1; }
    venueDo(&vAct[id[c]]);
    return 1;
}
