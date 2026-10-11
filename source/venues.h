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
       VS_LAUNCH, VS_SPACEWALK, VS_SPAR, VS_FIGHTER, VS_BOARD, VS_PRODECK };   // (board.h: a skateboard, a pro deck)   // (the packs, expand.h: a seat on the rocket, spacewalk training, a challenger, the BORTUA FIGHTER cabinet)
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
    {"KARAOKE",0, 0,-5,0,0,15, M_COMBO,SK_CHARM,1,120,0,VS_NONE,"DONT STOP BELIEVING"},
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
    {"BORTUA FIGHTER",1, 0,-3,0,0,0, 0,NO,0,0,0,VS_FIGHTER,0},
    {"TEACH A CLASS",0, 0,-12,-6,0,6, 0,SK_BODY,1,200,0,VS_WORK,"BOW TO YOUR SENSEI"},
    // SKATE SHOP (77)
    {"A COMPLETE BOARD",120, 0,0,0,0,0, 0,NO,0,30,0,VS_BOARD,0},
    {"A PRO DECK",450, 0,0,0,0,0, 0,NO,0,30,0,VS_PRODECK,0},
    {"WATCH SKATE VIDEOS",0, 0,-2,0,5,0, M_COMBO,NO,0,120,0,VS_NONE,"THE NINETIES WERE A TIME"},
    {"GRIP TAPE ART",15, 0,-3,0,0,0, M_WANT,SK_CREAT,1,90,0,VS_NONE,"A MASTERPIECE  UNDERFOOT"},
    {"WORK THE REGISTER",0, 0,-10,0,0,5, 0,SK_CHARM,1,200,0,VS_WORK,"DUDE  NO WAY"},
};
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
    {CT_DOJO,    4, 4,RGB(20,5,4),RGB(30,26,18), "mmHHpl",72,5,"R THE SENSEI"},
    {CT_SKSHOP,  3, 2,RGB(20,20,22),RGB(31,22,4),"bbVQpl",77,5,"R THE SHOP DUDE"},
};
#define NVEN ((int)(sizeof(venues)/sizeof(venues[0])))
static const Venue* venueOf(int ct){ for(int i=0;i<NVEN;i++) if(venues[i].ct==ct) return &venues[i]; return 0; }
static const Venue* venueHere(void){ if(!nbOk||nbT.cur>=NB_LOTS) return 0; const NbLot*L=&nbT.lot[nbT.cur]; return L->on&&L->kind==LKIND_COMM?venueOf(L->type):0; }
// ---- the room ----
static void venueBuild(int x0,int y0,int x1,int y1,int ct){   // nbTemplate: a walled room, the counter across it, the props round the walls, you arrive outside the door
    const Venue*v=venueOf(ct); if(!v) return;
    int cx=(x0+x1)/2, rx0=x0+1, ry0=y0+1, rx1=x1-1, ry1=y1-3, my=(ry0+ry1)/2;
    gBox(x0,y0,x1,y1,v->fl==6?6:7); gRoom(rx0,ry0,rx1,ry1,v->fl,v->wp); gPut(cx,ry1,'D');
    for(int x=cx-2;x<=cx+2;x++) gPut(x,ry0+3,'@');
    const int sp[8][2]={{rx0+1,ry0+1},{rx1-1,ry0+1},{rx0+1,ry1-1},{rx1-1,ry1-1},{rx0+1,my},{rx1-1,my},{rx0+2,ry1-1},{rx1-2,ry1-1}};
    for(int i=0;v->props[i]&&i<8;i++){ int x=sp[i][0], y=sp[i][1]; if(x>rx0&&x<rx1&&y>ry0&&y<ry1&&lifeMap[y][x]=='.'&&!(x==cx&&y==ry1-1)) gPut(x,y,v->props[i]); }
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
static void venueDrawIcon(int ct,int sx,int sy,int k,int hw2){   // the town map: a box in the venue's colours, a sign in the other
    const Venue*v=venueOf(ct); if(!v) return;
    nbIsoBox(sx,sy,hw2,3*k,hw2/2,nbTint(v->a),nbTint(nbLit(v->a,4)),nbTint(v->b),nbTint(nbLit(v->b,-4)));
    rect(sx-hw2/3,sy-4*k,2*hw2/3+1,k,nbTint(v->b));
}
// ---- the menu ----
static char vMsg[48] EWRAM_BSS;
static int vNear(void){ int tx=(int)(lfx>>8), ty=(int)(lfy>>8); for(int dy=-1;dy<=1;dy++) for(int dx=-1;dx<=1;dx++){ int x=tx+dx, y=ty+dy; if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='@') return 1; } return 0; }
static const char* venueHint(void){ const Venue*v=venueHere(); return v&&vNear()?v->hint:0; }
static void vAdd(int*v,int d){ *v+=d; if(*v>100) *v=100; if(*v<0) *v=0; }
static u32 vWorkAt EWRAM_BSS;   // a shift: one every two game hours
static int exSeatFree(void); static void exSparCall(void);   // expand.h
static void venueDo(const VAct*a){
    int price=a->price; if(a->sp==VS_LAUNCH&&exSeatFree()) price=0;   // (BORE-OUT MAN!: the mission booked your seat)
    if(a->fl&VF_ADULT&&stage<AG_ADULT){ lnote="GROWN-UPS ONLY"; lnoteT=60; voxPlay(V_nah); return; }
    if(a->fl&VF_NIGHT&&simMin<18*60&&simMin>=4*60){ lnote="COME BACK AFTER 6 PM"; lnoteT=70; return; }
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
        case VS_FIGHTER: exTake=EXT_FIGHTER; lnote="INSERT COIN"; break;
        case VS_BOARD: if(bdOwn){ simMoney+=price; lnote="YOU ALREADY HAVE A BOARD"; break; } bdGot(1,"A BRAND NEW BOARD  RIDE IT OUTSIDE"); break;
        case VS_PRODECK: if(bdOwn>=2){ simMoney+=price; lnote="YOU ALREADY RIDE A PRO DECK"; break; } bdGot(2,"A PRO DECK  FASTER  MORE POP"); break;
        default: break; }
    stQEvent(QE_VACT,(int)(a-vAct));   // (the packs' missions: expand.h TD)
    simsSave();
}
static int venueUse(void){   // R next to a venue's counter. 1 = handled
    const Venue*v=venueHere(); if(!v||!vNear()) return 0;
    char tt[40]; const char* it[10]; char lb[10][32]; int id[10], n=0;
    { char*e=simCat(tt,ctNm[v->ct]); e=simCat(e,"  \xC2\xA7"); simCatMoney(e,simMoney,1); }
    for(int i=0;i<v->an&&n<10;i++){ const VAct*a=&vAct[v->a0+i]; char*e=simCat(lb[n],a->nm);
        if(a->price>0){ e=simCat(e,"  \xC2\xA7"); simCatN(e,a->price); } else if(a->price<0){ e=simCat(e,"  +\xC2\xA7"); simCatN(e,-a->price); }
        it[n]=lb[n]; id[n++]=v->a0+i; }
    int c=menu(tt,it,n); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
    if(c<0) return 1;
    venueDo(&vAct[id[c]]);
    return 1;
}
