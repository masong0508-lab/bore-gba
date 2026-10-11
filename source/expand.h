// expand.h - EXPANSION PACKS: six packs of MISSIONS you play inside your own life (the roadmap's 1st to 4th, 7th and 8th packs).
//   1 BORE-OUT MAN!  a space oddity    5 missions    a burnt-out desk jockey becomes an astronaut: the SPACEPORT, a launch, a spacewalk
//   2 BOREZIES!                        5 missions    the town got so bored it turned: Borezies shamble in, and only fun cures them
//   3 WORLD WAR BORE                  10 missions    the East Side trimmed your hedge: boot camp, the arms race, soldiers on your lawn
//   4 BORTUA FIGHTER                   5 missions + ARCADE MODE: challengers want a fight, the DOJO, the cabinet at the arcade
//   7 BOREagotchi!                    15 missions + THE HANDHELD: a pixel blob on a keychain, and the real pets
//   8 BOREpilogue                     20 missions    grown-ups only: retire, remember, settle scores, leave a legacy
// A MISSION is a scene like a story chapter: a title, two lines of what is going on, up to three TASKS that tick off in any order, a SCENARIO
// (what the mission does to your life while it runs) and its pay. A pack's missions open one after the other. A COMING SOON row is a PLACEHOLDER:
// its title is there, it cannot be played yet, and the mission after it opens without it.
// WRITING A MISSION: one row in the pack's table (see docs/NOTES.md, EXPANSION PACKS):
//   XM(title, line 1, line 2, scenario, pay in hundreds, up to three tasks)   or   XSOON(title)   for a placeholder.
//   Tasks: TA(CT_ place,"GO TO ...")    TE(SE_ / QE_ event, at least,"...")    TD(VA_ venue action,"...")    TS(SC_ social you do,"...")
//          TP(pet action,"...")         TG(GA_ handheld action,"...")          TK(QK_ kind, number,"...")    TL(SK_ skill, level,"...")
//          and story.h's own: {QK_HOME,0,0,"..."}, {QK_GUEST,0,0,"..."}, {QK_ROOM,0,0,"..."} ...
// SCENARIOS  XS_ZOMB   BOREZIES: every visitor turns up grey-green and groaning, day and night, shambles after you and bores you (FUN drains).
//                      A joke, a hug, a high five, a compliment or a trick cures one (the mission's counter); PUFF PUFF PASS cures them all.
//            XS_WAR    EAST SIDE SOLDIERS: visitors in khaki come to fight; they wind up (HIYAH: guard!) and swing. Knock them out (the counter).
//            XS_ARENA  CHALLENGERS: the same, the BORTUA way (a purse for every win). The DOJO's CALL OUT A CHALLENGER brings one any time.
//            XS_PUDDLE a pet had an accident: a puddle to mop when the mission starts.   XS_GHOST  a ghost turns up the first night.
// SAVED  EX_OFF: the mission you are on (it goes with your player save, chunk 'X'). EXD_OFF: the missions done, in every life (like the story
//        missions). BF_OFF: the arcade's record (bortua.h). GO_OFF: the handheld (gotchi.h).
// Needs before it: story.h (StT, QK_, stBack, stSparkle), venues.h (VA_), pets.h / petui.h, house.h (visitors, socials), fight.h, travel.h, the UI kit.
#define EX_OFF  7472
#define EX_LEN  9
#define EXD_OFF 7482
#define EXD_LEN 11
#define BF_OFF  7494   // bortua.h
#define GO_OFF  7502   // gotchi.h (up to 7559)
_Static_assert(WD_OFF+WD_LEN<=EX_OFF&&EX_OFF+EX_LEN<=EXD_OFF&&EXD_OFF+EXD_LEN<=BF_OFF&&BF_OFF+8<=GO_OFF&&GO_OFF<LG_OFF,"the expansion blocks sit between the stash and the life stats");
enum { XP_SPACE, XP_ZOMB, XP_WAR, XP_FIGHT, XP_GOTCHI, XP_EPI, XP_N };
enum { XS_NONE, XS_SOON, XS_ZOMB, XS_WAR, XS_ARENA, XS_PUDDLE, XS_GHOST };
enum { XMODE_NONE, XMODE_FIGHTER, XMODE_GOTCHI };
// task kinds the packs add to story.h's (QK_EV, QK_AT ... QK_FREE)
enum { QK_PETS=QK_FREE+1, QK_PLOVE, QK_PTRICKS, QK_POTTY, QK_STAGE, QK_CNT, QK_DAYS, QK_SECURE, QK_HAPPY, QK_BFW, QK_BFCHAMP, QK_GOTCHI,
       QK_DO, QK_SOC, QK_PETDO, QK_GDO };   // the last four tick on an event that names one thing (a venue action, a social, a pet action, a handheld action)
// the handheld's actions (gotchi.h)
enum { GA_FEED, GA_SNACK, GA_PLAY, GA_CLEAN, GA_MED, GA_SCOLD, GA_LIGHT, GA_N };
// the pet actions of the pet's pie menu (petui.h petPie)
enum { PD_PET, PD_CUDDLE, PD_PLAY, PD_TRICK, PD_TRAIN, PD_FEED, PD_OUT, PD_POTTY, PD_SCOLD, PD_SHOO };
typedef struct { const char*nm; const char*br[2]; u8 sc, pay; StT t[3]; } XMs;
typedef struct { const char*nm,*sub,*bl[3]; const XMs*ms; u8 n, no, mode, adult; u16 a, b; const char*modeNm; } XPk;
#define XM(nm,b0,b1,sc,pay,...) {nm,{b0,b1},sc,pay,{__VA_ARGS__}}
#define XSOON(nm) {nm,{"COMING SOON","A MISSION STILL BEING WRITTEN"},XS_SOON,0,{{0,0,0,0}}}
#define TA(c,nm)   {QK_AT,c,0,nm}
#define TE(e,n,nm) {QK_EV,e,n,nm}
#define TD(a,nm)   {QK_DO,a,0,nm}
#define TS(a,nm)   {QK_SOC,a,0,nm}
#define TP(a,nm)   {QK_PETDO,a,0,nm}
#define TG(a,nm)   {QK_GDO,a,0,nm}
#define TK(k,n,nm) {k,0,n,nm}
#define TL(s,n,nm) {QK_LSKILL,s,n,nm}
// ---- 1 BORE-OUT MAN!  a space oddity ----
static const XMs xmSpace[]={
  XM("BURNT OUT","THE DESK  THE PRINTER  THE BOSS","A FLYER SAYS: WANTED  ASTRONAUT",XS_NONE,30,
     TE(SE_SHIFT,1,"WORK ONE LAST SHIFT"),TD(VA_STUDY,"STUDY AT THE LIBRARY"),TL(SK_LOGIC,2,"REACH LOGIC LEVEL 2")),
  XM("TAKE YOUR PROTEIN PILLS","THE SPACE PROGRAM SAYS YOU","ARE SOFT  GET TO THE GYM",XS_NONE,35,
     TD(VA_LIFT,"LIFT WEIGHTS"),TD(VA_SHAKE,"DOWN A PROTEIN SHAKE"),TL(SK_BODY,2,"REACH BODY LEVEL 2")),
  XM("PUT YOUR HELMET ON","TRAINING AT THE SPACEPORT","TRY NOT TO THROW UP",XS_NONE,40,
     TA(CT_SPACE,"GO TO THE SPACEPORT"),TD(VA_CENTRI,"RIDE THE CENTRIFUGE"),TE(QE_SPACEWALK,1,"PASS SPACEWALK TRAINING")),
  XM("FLOATING IN A TIN CAN","YOUR SEAT IS BOOKED  LIFT OFF","AND FIX THE ANTENNA OUT THERE",XS_NONE,50,
     TE(QE_LAUNCH,1,"LAUNCH INTO SPACE"),TE(QE_SPACEWALK,2,"FIX THE ANTENNA IN ORBIT")),
  XM("PLANET EARTH IS BLUE","BACK ON THE GROUND  NOBODY","NOTICED YOU WERE GONE  TELL THEM",XS_NONE,60,
     {QK_HOME,0,0,"GET BACK HOME"},TS(SC_HUG,"HUG SOMEONE"),{QK_GUEST,0,0,"HAVE SOMEONE OVER"}),
};
// ---- 2 BOREZIES! ----
static const XMs xmZomb[]={
  XM("NIGHT OF THE LIVING BORED","A NEIGHBOR SHUFFLES OVER","GROANING  BOOORE  CHEER THEM UP",XS_ZOMB,30,
     TK(QK_CNT,1,"CURE A BOREZIE"),TE(SE_LAUGH,1,"TELL A JOKE")),
  XM("DAWN OF THE BORED","THEY COME AT NIGHT NOW  STOCK","UP AND HOLD THE HOUSE",XS_ZOMB,35,
     TD(VA_GROCER,"BUY GROCERIES FOR HOME"),TK(QK_CNT,3,"CURE 3 BOREZIES"),TK(QK_DAYS,2,"HOLD OUT FOR 2 DAYS")),
  XM("SHAUN OF THE BORED","THE PLAN: GO TO THE BAR  EAT","SOMETHING  WAIT FOR IT TO BLOW OVER",XS_ZOMB,40,
     TA(CT_BAR,"GO TO THE BAR"),TD(VA_BARSNACK,"ORDER BAR SNACKS"),TK(QK_CNT,2,"CURE 2 BOREZIES")),
  XM("28 BONGS LATER","THE LAB SAYS ONE THING CURES","BOREDOM FOR GOOD  PASS IT ON",XS_ZOMB,45,
     TK(QK_CNT,4,"CURE 4 BOREZIES"),TK(QK_HAPPY,0,"FEEL HAPPY RIGHT NOW")),
  XM("BORED OF THE DEAD","PATIENT ZERO HIDES AT THE","ARCADE  BEAT HIS HIGH SCORE",XS_ZOMB,60,
     TA(CT_ARCADE,"GO TO THE ARCADE"),TD(VA_CABINET,"PLAY A CABINET"),TK(QK_CNT,5,"CURE 5 BOREZIES")),
};
// ---- 3 WORLD WAR BORE ----
static const XMs xmWar[]={
  XM("THE HEDGE INCIDENT","SOMEONE FROM THE EAST SIDE","TRIMMED YOUR HEDGE  UNFORGIVABLE",XS_NONE,30,
     TS(SC_ARGUE,"ARGUE WITH SOMEONE"),TK(QK_FRIENDS,2,"RECRUIT 2 FRIENDS")),
  XM("BOOT CAMP","THE WEST SIDE NEEDS YOU","DROP AND GIVE ME TWENTY",XS_NONE,35,
     TA(CT_GYM,"REPORT TO THE GYM"),TD(VA_SPIN,"SURVIVE SPIN CLASS"),TL(SK_BODY,2,"REACH BODY LEVEL 2")),
  XM("ARMS RACE","THEY HAVE A SUPER SOAKER","YOU NEED SOMETHING BIGGER",XS_NONE,40,
     TA(CT_ARMS,"GO TO THE ARMS SHOP"),TE(QE_BUY,1,"BUY A WEAPON")),
  XM("DIG IN","THE FRONT LINE IS YOUR LAWN","GATES  CAMERAS  HOLD THE LINE",XS_WAR,45,
     TK(QK_SECURE,2,"PUT UP 2 GATES OR CAMERAS"),TK(QK_DAYS,1,"HOLD THE LINE FOR A DAY")),
  XM("THE FRONT","THE EAST SIDE IS COMING OVER","THE FENCE  SEND THEM HOME",XS_WAR,50,
     TK(QK_CNT,3,"KNOCK OUT 3 SOLDIERS"),TE(QE_PARRY,1,"PARRY A BLOW")),
  XM("PROPAGANDA","WIN HEARTS AND MINDS ON THE","RADIO  AND DOWN AT THE PLAZA",XS_NONE,50,
     TE(SE_RADIO,1,"PLAY THE RADIO"),TA(CT_PLAZA,"GO TO THE PLAZA"),TK(QK_FRIENDS,4,"WIN 4 FRIENDS")),
  XM("BEHIND ENEMY LINES","SNEAK INTO OLD TOWN AND TAKE","OUT THEIR BEST  QUIETLY",XS_WAR,55,
     TA(CT_OLDTOWN,"SNEAK INTO OLD TOWN"),TK(QK_CNT,2,"KNOCK OUT 2 SOLDIERS")),
  XSOON("PRISONER OF BORE"),
  XSOON("THE BIG PUSH"),
  XM("ARMISTICE DAY","NOBODY REMEMBERS WHAT THE","HEDGE LOOKED LIKE  MAKE PEACE",XS_NONE,80,
     TS(SC_SORRY,"SAY SORRY"),TS(SC_HUG,"HUG IT OUT"),TK(QK_FRIENDS,6,"MAKE 6 FRIENDS")),
};
// ---- 4 BORTUA FIGHTER ----
static const XMs xmFight[]={
  XM("THE VIRAL VIDEO","SOMEONE FILMED YOU PUNCHING","A GUY  NOW EVERYONE WANTS A GO",XS_ARENA,30,
     TK(QK_CNT,1,"KNOCK OUT A CHALLENGER"),TE(QE_PARRY,1,"PARRY A BLOW")),
  XM("TRAINING MONTAGE","EVERY FIGHTER NEEDS A MONTAGE","GYM  POOL  REPEAT",XS_NONE,35,
     TD(VA_LIFT,"LIFT WEIGHTS"),TD(VA_SWIM,"SWIM LAPS"),TL(SK_BODY,3,"REACH BODY LEVEL 3")),
  XM("THE CABINET","THE ARCADE HAS A BORTUA","FIGHTER MACHINE  BEAT IT",XS_NONE,40,
     TA(CT_ARCADE,"GO TO THE ARCADE"),TK(QK_BFW,3,"WIN 3 ARCADE FIGHTS")),
  XM("SPECIAL MOVES","THE OLD MASTER AT THE DOJO","SAYS FIREBALLS ARE REAL",XS_ARENA,50,
     TA(CT_DOJO,"GO TO THE DOJO"),TE(QE_SPECIAL,1,"LAND A SPECIAL MOVE"),TK(QK_CNT,2,"KNOCK OUT 2 CHALLENGERS")),
  XM("BORTUA CHAMPION","THE FINAL BOSS IS MADE OF","CHROME  AND SHE IS WAITING",XS_ARENA,80,
     TK(QK_BFCHAMP,0,"BEAT ARCADE MODE"),TK(QK_CNT,3,"KNOCK OUT 3 CHALLENGERS")),
};
// ---- 7 BOREagotchi! ----
static const XMs xmGotchi[]={
  XM("IT HATCHED","AN EGG ON A KEYCHAIN BEEPS","IT WANTS YOU  CONSTANTLY",XS_NONE,30,
     TK(QK_GOTCHI,1,"HATCH YOUR BOREAGOTCHI"),TG(GA_FEED,"FEED IT A MEAL")),
  XM("A REAL ONE","THE BLOB CANNOT FETCH","ADOPT A PET WITH FUR",XS_NONE,35,
     TK(QK_PETS,1,"ADOPT A PET"),TP(PD_PET,"PET YOUR PET")),
  XM("PUPPY LOVE","IT FOLLOWS YOU EVERYWHERE","LOVE IT BACK",XS_NONE,35,
     TP(PD_CUDDLE,"CUDDLE YOUR PET"),TP(PD_PLAY,"PLAY WITH YOUR PET"),TK(QK_PLOVE,60,"A PET LOVES YOU 60")),
  XM("WHO PEED ON THE RUG","SOMEONE HAD AN ACCIDENT","MOP IT  THEN TEACH SOME MANNERS",XS_PUDDLE,40,
     TE(QE_PMOP,1,"MOP UP THE PUDDLE"),TP(PD_POTTY,"HOUSE TRAIN YOUR PET"),TK(QK_POTTY,30,"A PET 30% HOUSE TRAINED")),
  XM("SIT  STAY  GOOD","EVERY GOOD PET KNOWS A TRICK","OR AT LEAST PRETENDS TO",XS_NONE,40,
     TE(QE_PLEARN,1,"TEACH A NEW TRICK"),TP(PD_TRICK,"SHOW OFF A TRICK")),
  XM("THE BLOB GROWS UP","YOUR BOREAGOTCHI IS NOT A","BABY ANYMORE  PLAY WITH IT",XS_NONE,40,
     TK(QK_GOTCHI,2,"RAISE IT TO A CHILD"),TG(GA_PLAY,"PLAY A GAME WITH IT")),
  XM("PET SHOP SPREE","TREAT YOURSELF  TREAT THE","PETS  MOSTLY THE PETS",XS_NONE,45,
     TA(CT_PETSHOP,"GO TO THE PET SHOP"),TD(VA_TOY,"BUY A SQUEAKY TOY"),TD(VA_GROOM,"BOOK PET GROOMING")),
  XM("TWO'S COMPANY","ONE PET IS LONELY  SAYS THE","PET SHOP  WHO SELLS PETS",XS_NONE,45,
     TK(QK_PETS,2,"HAVE 2 PETS"),TP(PD_FEED,"FEED A PET")),
  XM("TEEN BLOB ANGST","IT SLAMS ITS LITTLE PIXEL","DOOR  SET SOME RULES",XS_NONE,50,
     TK(QK_GOTCHI,3,"RAISE IT TO A TEEN"),TG(GA_SCOLD,"DISCIPLINE IT")),
  XM("BEST IN SHOW","THE JUDGES WANT TRICKS","AND THEY WANT LOVE",XS_NONE,60,
     TK(QK_PTRICKS,3,"A PET KNOWS 3 TRICKS"),TK(QK_PLOVE,90,"A PET LOVES YOU 90")),
  XSOON("THE BLOB GOES ONLINE"),
  XSOON("PAWS AND CLAWS"),
  XSOON("THE GREAT ESCAPE"),
  XSOON("BLOB VERSUS DOG"),
  XM("FOREVER FRIENDS","THE BLOB IS ALL GROWN UP","AND SO ARE THE PETS",XS_NONE,90,
     TK(QK_GOTCHI,4,"RAISE IT TO AN ADULT"),TK(QK_PETS,1,"KEEP A PET"),TK(QK_PLOVE,100,"A PET LOVES YOU 100")),
};
// ---- 8 BOREpilogue ----
static const XMs xmEpi[]={
  XM("THE GOLD WATCH","FORTY YEARS AT ONE DESK","ONE LAST SHIFT  THEN FREEDOM",XS_NONE,30,
     TE(SE_SHIFT,1,"WORK ONE LAST SHIFT"),TK(QK_MONEY,20000,"SAVE 20000 SIMOLEONS")),
  XM("BUCKET LIST","THINGS TO DO BEFORE YOU KICK","IT  START WITH THE FUN ONES",XS_NONE,35,
     TD(VA_CANNON,"DO A CANNONBALL"),TD(VA_MOVIE,"WATCH A MOVIE"),TD(VA_KARAOKE,"SING KARAOKE")),
  XM("GREEN THUMB","EVERY RETIREE NEEDS A GARDEN","GROW SOMETHING  ANYTHING",XS_NONE,40,
     TE(QE_HARVEST,1,"HARVEST A PLANTER"),TE(SE_READ,1,"READ UP ON GARDENING")),
  XM("OLD FLAMES","AN OLD FLAME STILL LIVES IN","TOWN  CALL THEM UP",XS_NONE,40,
     {QK_GUEST,0,0,"HAVE SOMEONE OVER"},TS(SC_FLIRT,"FLIRT LIKE IT IS 1985")),
  XM("THE QUIET HOUSE","THE KIDS MOVED OUT  THE HOUSE","ECHOES  FILL IT BACK UP",XS_NONE,45,
     TK(QK_HOUSE,3,"3 SIMS LIVING HERE"),{QK_ROOM,0,0,"MAKE A ROOM LOOK GREAT"}),
  XM("MAKING AMENDS","THAT FEUD FROM 1997","TIME TO LET IT GO",XS_NONE,45,
     TS(SC_SORRY,"SAY SORRY"),TK(QK_FRIENDS,3,"MAKE 3 FRIENDS")),
  XM("MEMOIRS","WRITE IT ALL DOWN BEFORE YOU","FORGET  WHERE ARE YOUR GLASSES",XS_NONE,50,
     TD(VA_STUDY,"STUDY AT THE LIBRARY"),TD(VA_NOVEL,"READ A NOVEL FOR IDEAS"),TL(SK_CREAT,3,"REACH CREATIVITY LEVEL 3")),
  XM("SENIOR DISCOUNT","EVERYTHING IS CHEAPER AT YOUR","AGE  IF YOU ASK LOUD ENOUGH",XS_NONE,50,
     TD(VA_LATTE,"ORDER AN OAT LATTE"),TD(VA_FOURTH,"EAT FOURTH MEAL"),TD(VA_CHECKUP,"GET A CHECKUP")),
  XM("THE GHOST OF BORES PAST","SOMETHING GOES BUMP AT NIGHT","IT LOOKS LIKE YOUR OLD BOSS",XS_GHOST,55,
     TE(QE_SPOOK,1,"GET SPOOKED"),TE(SE_SLEEP,1,"SLEEP IT OFF")),
  XM("ONE LAST PARTY","INVITE EVERYONE YOU EVER","LOVED  AND A FEW YOU DID NOT",XS_NONE,70,
     TK(QK_FRIENDS,5,"MAKE 5 FRIENDS"),{QK_GUEST,0,0,"HAVE SOMEONE OVER"},TE(SE_RADIO,1,"CRANK THE STEREO")),
  XSOON("THE WILL"), XSOON("SECOND WIND"), XSOON("THE REUNION"), XSOON("SNOWBIRDS"), XSOON("ONE LAST SKATE"),
  XSOON("BINGO NIGHT"), XSOON("GRANDMA'S RECIPE"), XSOON("LEGACY"), XSOON("THE LONG GOODBYE"),
  XM("THE END  AND THE ONE AFTER","GROW OLD  GROW WISE  AND","LEAVE THE TOWN BETTER",XS_NONE,120,
     TK(QK_STAGE,AG_ELDER,"GROW INTO AN ELDER"),TK(QK_MONEY,100000,"LEAVE 100000 SIMOLEONS"),TK(QK_FRIENDS,8,"MAKE 8 FRIENDS")),
};
#undef XM
#undef XSOON
#undef TA
#undef TE
#undef TD
#undef TS
#undef TP
#undef TG
#undef TK
#undef TL
#define XN(a) ((u8)(sizeof(a)/sizeof(a[0])))
static const XPk xPk[XP_N]={
  {"BORE-OUT MAN!","A SPACE ODDITY",{"BURNT OUT AT A DESK JOB","ONE FLYER LATER YOU ARE","AN ASTRONAUT  SORT OF"},xmSpace,XN(xmSpace),1,XMODE_NONE,0,RGB(2,3,12),RGB(10,24,31),0},
  {"BOREZIES!","THE BORED WALK AMONG US",{"THE TOWN GOT SO BORED IT","TURNED  CURE THEM WITH FUN","JOKES  HUGS  PASS THE DUTCHIE"},xmZomb,XN(xmZomb),2,XMODE_NONE,0,RGB(3,9,3),RGB(14,30,10),0},
  {"WORLD WAR BORE","THE HEDGE THAT STARTED IT",{"EAST SIDE VERSUS WEST SIDE","A VERY SMALL  VERY STUPID","WAR  PICK A SIDE  YOURS"},xmWar,XN(xmWar),3,XMODE_NONE,0,RGB(9,9,3),RGB(27,23,9),0},
  {"BORTUA FIGHTER","THE 3D FIGHTING SENSATION",{"ONE VIRAL PUNCH AND NOW","EVERY BORE WANTS A PIECE","PLUS A WHOLE ARCADE MODE"},xmFight,XN(xmFight),4,XMODE_FIGHTER,0,RGB(13,2,2),RGB(31,22,4),"ARCADE MODE"},
  {"BOREAGOTCHI!","IT NEEDS YOU  CONSTANTLY",{"A PIXEL BLOB ON A KEYCHAIN","AND REAL PETS WHO ARE","JEALOUS  PLUS THE HANDHELD"},xmGotchi,XN(xmGotchi),7,XMODE_GOTCHI,0,RGB(14,3,12),RGB(31,20,27),"THE HANDHELD"},
  {"BOREPILOGUE","THE LAST CHAPTER",{"RETIRE  REMINISCE  SETTLE","OLD SCORES AND LEAVE A","LEGACY  GROWN-UPS ONLY"},xmEpi,XN(xmEpi),8,XMODE_NONE,1,RGB(7,5,10),RGB(25,22,29),0},
};
static const u8 xpBase[XP_N]={0,5,10,20,25,40};   // where each pack's missions start in the done bits
_Static_assert(XN(xmSpace)==5&&XN(xmZomb)==5&&XN(xmWar)==10&&XN(xmFight)==5&&XN(xmGotchi)==15&&XN(xmEpi)==20,"the packs have 5, 5, 10, 5, 15 and 20 missions");
_Static_assert(40+20<=64,"the done bits (8 bytes) are full");
#undef XN
// ---- state ----
static u8 exP, exM, exTk, exCnt, exDay=255;   // exP: pack + 1 (0: no mission), the mission, its tasks done (bits 0-2; bit 7: someone came over), its counter, the day it began
static u8 exShown, exLastP, exLastM;          // the mission whose card was shown (a card once per mission and power on); the one just finished (its card)
static u8 exHA[TW_N], exHS[TW_N], exHHp[TW_N], exSpar, exGhostDay=255;   // a hostile visitor's step counter, stun, last health; a challenger called; the ghost's night
static u16 exHPal[TW_N][16] EWRAM_BSS;        // a hostile visitor's own colours (put back when cured or beaten)
static u8 bfWins, bfChamp, goStage;           // the modes: arcade fights won, the ladder beaten, the handheld blob's stage (bortua.h, gotchi.h keep them)
static char exB[52] EWRAM_BSS;
static u8 exDay8(void){ u8 b=(u8)(simDay&255); return b==255?254:b; }
static const XMs* exMs(void){ return (exP>=1&&exP<=XP_N&&exM<xPk[exP-1].n)?&xPk[exP-1].ms[exM]:0; }
static int exTkN(const XMs*x){ int n=0; if(x) while(n<3&&x->t[n].k) n++; return n; }
static int exSoon(int p,int m){ return xPk[p].ms[m].sc==XS_SOON; }
// ---- saving ----
static u8 exSum(volatile const u8*m){ u8 s=0x58; for(int i=2;i<EX_LEN-1;i++) s=(u8)(s*3+m[i]); return s; }
static void exSave(void){ volatile u8*m=SRAM_BASE+EX_OFF; m[0]='X'; m[1]='P'; m[2]=1; m[3]=exP; m[4]=exM; m[5]=exTk; m[6]=exCnt; m[7]=exDay; m[8]=exSum(m); }
static void exClear(void){ exP=exM=exTk=exCnt=0; exDay=255; }
static int exLoadFrom(volatile const u8*m){   // 1 = a sound block
    if(m[0]!='X'||m[1]!='P'||m[2]!=1||m[8]!=exSum(m)||m[3]>XP_N) return 0;
    exP=m[3]; exM=m[4]; exTk=m[5]; exCnt=m[6]; exDay=m[7];
    if(exP&&(exM>=xPk[exP-1].n||exSoon(exP-1,exM))) exClear();
    return 1;
}
static void exLoad(void){ exClear(); if(!exLoadFrom(SRAM_BASE+EX_OFF)) exClear(); }
static int exDoneBit(int b){
    volatile u8*m=SRAM_BASE+EXD_OFF; if(m[0]!='X'||m[1]!='D') return 0;
    u8 x=0x3C; for(int i=0;i<8;i++) x^=m[2+i]; if(m[10]!=x) return 0;
    return (m[2+(b>>3)]>>(b&7))&1;
}
static void exDoneSet(int b){
    volatile u8*m=SRAM_BASE+EXD_OFF; u8 v[8], x=0x3C; int ok=m[0]=='X'&&m[1]=='D';
    for(int i=0;i<8;i++){ v[i]=ok?m[2+i]:0; x^=v[i]; }
    if(ok&&m[10]!=x) for(int i=0;i<8;i++) v[i]=0;   // (a damaged block starts again)
    v[b>>3]|=(u8)(1<<(b&7)); x=0x3C; for(int i=0;i<8;i++) x^=v[i];
    m[0]='X'; m[1]='D'; for(int i=0;i<8;i++) m[2+i]=v[i]; m[10]=x;
}
static int exDone(int p,int m){ return exDoneBit(xpBase[p]+m); }
static int exPackDone(int p){ int n=0; for(int i=0;i<xPk[p].n;i++) n+=exDone(p,i); return n; }
static int exPackPlay(int p){ int n=0; for(int i=0;i<xPk[p].n;i++) n+=!exSoon(p,i); return n; }
static int exOpen(int p,int m){   // can mission m be played: not a placeholder, and the playable one before it is done
    if(exSoon(p,m)) return 0; if(sUnlock) return 1;
    for(int i=m-1;i>=0;i--) if(!exSoon(p,i)) return exDone(p,i);
    return 1;
}
static int exNext(int p,int m){ for(int i=m+1;i<xPk[p].n;i++) if(!exSoon(p,i)) return i; return -1; }
// ---- the HOSTILE VISITORS: Borezies, soldiers and challengers (house.h twTick calls exHArrive and exHStay) ----
static int exHKind(void){ const XMs*x=exMs(); int sc=x?x->sc:0; return sc==XS_ZOMB?1:sc==XS_WAR?2:sc==XS_ARENA?3:exSpar?3:0; }   // what the next visitor is (0: a normal neighbour)
static int exHMode(void){ return exHKind()!=0; }
static void exPalBad(int v){ for(int b=0;b<16;b++) if(hhPkey[b]==v) hhPkey[b]=-1; }   // the bank holding that palette loads again next frame
static void exTint(int k,int kind){   // a Borezie goes grey-green, a soldier khaki (a challenger keeps their own colours)
    int v=TW_V(k); u16*p=hhPal[v]; for(int c=0;c<16;c++) exHPal[k][c]=p[c];
    if(kind==3) return;
    for(int c=1;c<16;c++){ int r=p[c]&31, g=(p[c]>>5)&31, b=(p[c]>>10)&31, l=(r*5+g*9+b*2)>>4;
        if(kind==1){ r=l*3/5; g=l*4/5+3; b=l/2; } else { r=(r+l+14)/3; g=(g+l+12)/3; b=(b+l)/4; }
        p[c]=RGB(r>31?31:r,g>31?31:g,b>31?31:b); }
    exPalBad(v);
}
static void exUntint(int k){ int v=TW_V(k); if(!exHT[k]) return; for(int c=0;c<16;c++) hhPal[v][c]=exHPal[k][c]; exPalBad(v); exHT[k]=0; }
static void exHClearAll(void){ for(int k=0;k<TW_N;k++) exUntint(k); exSpar=0; }
static void exGroan(int uid){ sfxNStp=(u32)SFX_STEP*62u/100u; voxPan(uid,&sfxNGL,&sfxNGR); sfxNSet=1; sfxPlay(VS(V_death_of_hunger)); if(!sSnd) sfxNSet=0; }   // a Borezie's BOOORE: the hunger groan, slow and low
static void exHMake(int k,int kind){   // visitor k turns hostile
    HhSim*s=&hhM[TW_V(k)]; exTint(k,kind); exHT[k]=(u8)kind; exHA[k]=0; exHS[k]=0; exHHp[k]=s->hp=HP_MAX;
    char*e=exB;
    if(kind==1){ e=simCat(e,"A BOREZIE SHAMBLES IN  "); simCat(e,s->name); exGroan(s->uid); }
    else if(kind==2){ e=simCat(e,"EAST SIDE SOLDIER  "); simCat(e,s->name); voxSay(s->uid,V_lets_fight); }
    else { e=simCat(e,s->name); simCat(e," WANTS A FIGHT"); voxSay(s->uid,V_lets_fight); exSpar=0; }
    lnote=exB; lnoteT=150;
}
static void exHArrive(int k){ int kind=exHKind(); if(kind) exHMake(k,kind); }   // twTick: visitor k starts walking in
static void exFace(HhSim*s){ s32 dx=lfx-s->fx, dy=lfy-s->fy; s->hd=(u8)((dx<0?-dx:dx)>(dy<0?-dy:dy)?(dx>0?0:8):(dy>0?4:12)); }
static void exBite(int k,HhSim*s){   // a Borezie gets to you: it is SO boring
    moodFun=moodClamp(moodFun-10*MOOD_ONE); if(sSoc>4) sSoc-=4; lstun=14; lsp=0; lgrind=0;
    static const char* const moan[4]={"BOOORE","BRAAAINS ARE BORING","TELL ME ABOUT TAXES","LET ME SHOW YOU MY SPREADSHEET"};
    hhSay(s->uid,IC_SAD,"BOOORE"); exGroan(s->uid);
    if(moodFun<=0){ lstun=150; lnote="BORED STIFF  YOU NEARLY TURNED"; lnoteT=150; moodEvent(M_SPOOK); }
    else { lnote=moan[(rnd8()+k)&3]; lnoteT=80; }
}
static int exHStay(int k,int*planned){   // a hostile visitor on the lot: chase you, then bite or swing. 0 = not hostile (the normal visit)
    if(!exHT[k]) return 0;
    int v=TW_V(k); HhSim*s=&hhM[v]; int kind=exHT[k];
    twWait[k]=600;   // (it does not go home while it is hostile)
    if(s->bubT) s->bubT--;
    if(s->act==HA_SOC){ if(s->t>0) s->t--; else s->act=HA_IDLE; return 1; }   // knocked down or talked to: it waits
    if(s->hp<exHHp[k]) exHS[k]=24;   // a blow landed: it reels
    exHHp[k]=s->hp; if(s->hp<HP_MAX&&(lfr&63)==0) s->hp++;
    if(exHS[k]){ exHS[k]--; return 1; }
    if(curFl||ldead||prIn()||simAct) return 1;   // (not while you are upstairs, down, inside or asleep)
    s32 dx=s->fx-lfx, dy=s->fy-lfy; int d2=(int)((dx*dx+dy*dy)>>8);
    if(d2>(340*340>>8)){   // far: walk at you (Borezies shamble at half speed)
        if(s->pi<s->pn){ s->act=HA_WALK; if(kind!=1||(lfr&1)) hhStepAlong(s); return 1; }
        if(!*planned&&++exHA[k]>=20){ exHA[k]=0; hhGX=(int)(lfx>>8); hhGY=(int)(lfy>>8); *planned=1; s->act=hhPlan(s,1)>1?HA_WALK:HA_IDLE; }
        else s->act=HA_IDLE;
        return 1; }
    s->pn=s->pi=0; s->act=HA_IDLE; exFace(s);
    if(++exHA[k]<(kind==1?70:kind==2?85:75)) return 1;
    exHA[k]=0;
    if(!wallClear(s->fx,s->fy,lfx,lfy)) return 1;
    if(kind==1){ exBite(k,s); return 1; }
    if(fgOk()){ if(!fgFoeT){ fgFoe=s->uid; fgFoeT=22; fgMode=300; } }   // it winds up (HIYAH at 14 steps: guard!) and swings (fight.h fgCounter)
    else socDo(s->uid,hhPUid,SC_SHOVE);   // (on a board, or too young for fists: a shove)
    return 1;
}
static int exCounts(void){ const XMs*x=exMs(); int sc=x?x->sc:0; return sc==XS_ZOMB?1:sc==XS_WAR?2:sc==XS_ARENA?3:0; }   // what the mission's counter counts
static void exCountUp(int kind){
    if(exCounts()!=kind) return;
    if(exCnt<250) exCnt++; exSave();
    const XMs*x=exMs(); int want=0; for(int i=0;i<exTkN(x);i++) if(x->t[i].k==QK_CNT&&(int)x->t[i].n>want) want=(int)x->t[i].n;
    char*e=simCat(exB,kind==1?"BOREZIE CURED  ":kind==2?"SOLDIER DOWN  ":"CHALLENGER BEATEN  "); e=simCatN(e,exCnt);
    if(want){ e=simCat(e," OF "); simCatN(e,want); } lnote=exB; lnoteT=130;
}
static void exCure(int k){   // a Borezie laughs and comes back to life
    HhSim*s=&hhM[TW_V(k)]; int u=s->uid;
    exUntint(k); twWait[k]=(short)(420+rnd8());
    if(hhPUid>=0&&hhPUid<RU_N&&u<RU_N){ relD[u][hhPUid]=(signed char)clampR(relD[u][hhPUid]+15); relL[u][hhPUid]=(signed char)clampR(relL[u][hhPUid]+8); }
    hhSay(u,IC_STAR,"I FEEL ALIVE"); moodEvent(M_WANT); stQEvent(QE_ZCURE,1); exCountUp(1);
}
static void exSoc(int a,int b,int i,int ok){   // house.h socDo: a social happened (you did it: the packs' social tasks; to a Borezie: maybe a cure)
    if(a!=hhPUid) return;
    if(ok) stQEvent(QE_SOC,i);   // (a mean one always lands)
    if(!uGuest(b)) return; int k=b-GU0; if(k<0||k>=TW_N||exHT[k]!=1) return;
    if(i==SC_PASS){ int n=0; for(int j=0;j<TW_N;j++) if(exHT[j]==1){ exCure(j); n++; } if(n>1){ lnote="PUFF PUFF PASS  THE WHOLE STREET MELLOWS OUT"; lnoteT=160; } return; }
    if(i==SC_JOKE||i==SC_HIGH5||i==SC_HUG||i==SC_COMPL||i==SC_TRICK||(i==SC_TALK&&(rnd8()&1))){ exCure(k); return; }
    if(socT[i].fl&SA_MEAN){ hhSay(b,IC_SAD,"BOOORE"); lnote="IT DOES NOT EVEN NOTICE  TRY FUN"; lnoteT=90; }
}
static void exKO(int uid){   // house.h fightHit: you knocked someone out
    stQEvent(QE_KO,1);
    if(!uGuest(uid)) return; int k=uid-GU0; if(k<0||k>=TW_N||!exHT[k]) return;
    HhSim*s=&hhM[TW_V(k)];
    if(exHT[k]==1){ s->act=HA_IDLE; s->t=0; s->hp=HP_MAX; exHHp[k]=HP_MAX; exHS[k]=40; lnote="IT GETS RIGHT BACK UP  TRY FUN"; lnoteT=110; exGroan(uid); return; }   // Borezies feel nothing
    int kind=exHT[k]; exUntint(k); twWait[k]=200;   // beaten: it lies there a while, then limps home
    voxSay(uid,V_lost_the_fight);
    if(kind==3){ simMoneyAdd(50); }   // the purse
    exCountUp(kind);
}
static void exSparCall(void){   // the DOJO's CALL OUT A CHALLENGER: a visitor here takes you on now, or the next one to come does
    for(int k=0;k<TW_N;k++) if(twHas[k]&&twOn[k]==2&&!exHT[k]&&TW_V(k)>=hhN){ exHMake(k,3); return; }
    exSpar=1; for(int k=0;k<TW_N;k++) if(twHas[k]&&!twOn[k]&&twWait[k]>60) twWait[k]=60;
    lnote="YOU CALL OUT THE ROOM  SOMEONE WILL ANSWER"; lnoteT=120;
}
// ---- the mission's tasks ----
static int exPetBest(int what){ int b=0; for(int i=0;i<petN;i++){ const Pet*p=&petM[i]; int v=what==0?p->love:what==1?p->potty:0; if(what==2){ for(int q=0;q<PET_TRK;q++) v+=(p->tricks>>q)&1; } if(v>b) b=v; } return b; }
static int exVal(const StT*t){   // the number a task counts (-1: it has none to show)
    int me=hhPUid;
    switch(t->k){
    case QK_CNT: return exCnt;
    case QK_DAYS: return exDay==255?0:((simDay&255)-exDay)&255;
    case QK_FRIENDS: { int n=0; for(int u=0;u<HU_N;u++) if(u!=me&&(relF[me][u]&RF_FRIEND)) n++; return n; }
    case QK_MONEY: return simMoneyI();
    case QK_PETS: return petN;
    case QK_PLOVE: return exPetBest(0);
    case QK_POTTY: return exPetBest(1);
    case QK_PTRICKS: return exPetBest(2);
    case QK_SECURE: return secCount('n')+secCount('j');
    case QK_BFW: return bfWins;
    case QK_HOUSE: return hhN+1;
    case QK_LSKILL: return skLvl(t->a);
    case QK_SKATE: return skillLvl;
    case QK_JOB: return jobLvl;
    case QK_TRICKS: return (int)simTricks;
    case QK_WANTS: return simDone;
    }
    return -1;
}
static int exEvKind(int k){ return k==QK_EV||k==QK_DO||k==QK_SOC||k==QK_PETDO||k==QK_GDO; }   // ticked by an event, never by a state
static int exTkNow(const StT*t){   // is a task's state met right now?
    switch(t->k){
    case QK_AT: return trvHere(t->a);
    case QK_HOME: return !nbOk||nbT.cur==nbT.home;
    case QK_GUEST: return (exTk>>7)&1;
    case QK_STAGE: return stage>=(int)t->n;
    case QK_HAPPY: return moodState()>=MS_HAPPY;
    case QK_BFCHAMP: return bfChamp!=0;
    case QK_GOTCHI: return goStage>=t->n;
    case QK_ROOM: return sRoom>=80;
    case QK_FREE: return !prDays&&!prIn();
    }
    { int v=exVal(t); return v>=0&&v>=(int)t->n; }
}
static void exTkDone(int i,const XMs*x){ exTk|=(u8)(1<<i); exSave(); char*e=simCat(exB,"DONE  "); simCat(e,x->t[i].nm); lnote=exB; lnoteT=150; sfxPlay(SFX_STICK); }
static void exQEvent(int ev,int v){   // story.h stQEvent passes every life event and story event on
    const XMs*x=exMs(); if(!x) return; int n=exTkN(x);
    for(int i=0;i<n;i++){ const StT*t=&x->t[i]; if((exTk>>i)&1) continue;
        int hit=(t->k==QK_EV&&t->a==ev&&v>=(int)t->n)||(t->k==QK_DO&&ev==QE_VACT&&v==t->a)||(t->k==QK_SOC&&ev==QE_SOC&&v==t->a)
              ||(t->k==QK_PETDO&&ev==QE_PET&&v==t->a)||(t->k==QK_GDO&&ev==QE_GOTCHI&&v==t->a);
        if(hit) exTkDone(i,x); }
}
static void exTkText(const StT*t,int ok,char*b){   // "+ CURE 3 BOREZIES" / "- CURE 3 BOREZIES  1/3"
    char*e=slCat(b,ok?"+ ":"- "); e=slCat(e,t->nm);
    if(ok||exEvKind(t->k)||t->k==QK_LSKILL||t->k==QK_STAGE) return;
    int v=exVal(t); if(v<0||t->n<2||t->n>9999) return;
    if(v>(int)t->n) v=(int)t->n; e=slCat(e,"  "); e=slNum(e,v); e=slCat(e,"/"); slNum(e,(int)t->n);
}
static void exTkList(const XMs*x,int x0,int y,int dy,int live){
    for(int i=0;i<exTkN(x);i++){ int ok=live&&((exTk>>i)&1); char l[60]; exTkText(&x->t[i],ok,l); if(!live) l[0]='-'; text(x0,y+i*dy,l,ok?RGB(10,28,12):WHITE,1); }
}
static int exPlace(int*ct){   // the place the mission still wants you at (*ct = its kind; -2 = home). 0 = none
    const XMs*x=exMs(); int n=exTkN(x);
    for(int i=0;i<n;i++){ if((exTk>>i)&1) continue; if(x->t[i].k==QK_AT){ *ct=x->t[i].a; return 1; } if(x->t[i].k==QK_HOME&&nbOk&&nbT.cur!=nbT.home){ *ct=-2; return 1; } }
    return 0;
}
// ---- scenarios ----
static void exScSetup(const XMs*x){   // a mission starts: its places are in the town, its scenario begins
    if(!x) return;
    for(int i=0;i<exTkN(x);i++) if(x->t[i].k==QK_AT) trvPlace(x->t[i].a);
    if(x->sc==XS_ZOMB||x->sc==XS_WAR||x->sc==XS_ARENA){
        int any=0; for(int k=0;k<TW_N;k++) any|=twHas[k];
        if(!any&&!curFl&&!prHere()){ twKeep=0; hhBakeAll(); ldEnd(); liveInvalidate(); }   // (no visitors set up here: PRE-MADE SIMS off): somebody has to come
        for(int k=0;k<TW_N;k++) if(twHas[k]&&!twOn[k]) twWait[k]=(short)(150+k*360);
    }
    if(x->sc==XS_PUDDLE&&pudN<PUD_N){   // somebody had an accident
        int tx=(int)(lfx>>8), ty=(int)(lfy>>8); if(petN){ tx=(int)(petM[0].w.fx>>8); ty=(int)(petM[0].w.fy>>8); }
        for(int r=1;r<4;r++){ int x2=tx+r, y2=ty; if(x2<MW-1&&lifeMap[y2][x2]=='.'){ pudX[pudN]=(u8)x2; pudY[pudN]=(u8)y2; pudN++; liveInvalidate(); break; } x2=tx-r; if(x2>0&&lifeMap[y2][x2]=='.'){ pudX[pudN]=(u8)x2; pudY[pudN]=(u8)y2; pudN++; liveInvalidate(); break; } }
    }
    exGhostDay=255;
}
static void exScTick(const XMs*x){   // once a second while a mission runs
    if(x->sc==XS_GHOST&&simIsNight()&&exGhostDay!=exDay8()&&!curFl&&!prIn()){   // the first night: your old boss comes back to haunt you
        exGhostDay=exDay8();
        if(xo[XO_GHOSTS]&&fxGN<GH_MAX){ fxGhostBorn(0); FxGhost*g=&fxG[fxGN-1];
            int hx=(int)(lfx>>8)+5, hy=(int)(lfy>>8)+2; if(hx>MW-2) hx=MW-2; if(hy>MH-2) hy=MH-2;
            g->hx=(u8)hx; g->hy=(u8)hy; g->fx=hx*256+128; g->fy=hy*256+128; g->tx=(s16)g->fx; g->ty=(s16)g->fy; g->age=255; g->cd=90;
            lnote="A CHILL  SOMETHING FAMILIAR IS HERE"; lnoteT=150; sfxPlay(SFX_GHOST); }
        else { lnote="SOMETHING WHISPERS  YOU ARE FIRED  BOO"; lnoteT=150; sfxPlay(SFX_GHOST); moodEvent(M_SPOOK); stQEvent(QE_SPOOK,1); }   // (GHOSTS off: it is only a voice)
    }
    if(exHMode()) for(int k=0;k<TW_N;k++) if(twHas[k]&&!twOn[k]&&twWait[k]>420) twWait[k]=(short)(240+rnd8());   // they keep coming
}
// ---- starting, finishing, leaving ----
static void exAnnounce(void){ const XMs*x=exMs(); if(!x) return; char*e=slCat(exB,xPk[exP-1].nm); e=slCat(e,"  "); slCat(e,x->nm); lnote=exB; lnoteT=240; }
static void exStart(int p,int m){
    exHClearAll(); exP=(u8)(p+1); exM=(u8)m; exTk=0; exCnt=0; exDay=exDay8(); exSave(); sgDirty=1;
    exScSetup(exMs()); exShown=(u8)(exP*32+exM+1); exModal=1;
}
static void exQuit(void){ exHClearAll(); exClear(); exSave(); sgDirty=1; }
static void exComplete(void){
    const XMs*x=exMs(); if(!x) return; int p=exP-1;
    simMoneyAdd((money_t)x->pay*100); dnaAdd(15); persSave(); simsSave();
    exDoneSet(xpBase[p]+exM); exHClearAll(); exLastP=(u8)p; exLastM=exM;
    int nx=exNext(p,exM);
    if(nx>=0){ exM=(u8)nx; exTk=0; exCnt=0; exDay=exDay8(); exSave(); exScSetup(exMs()); exShown=(u8)(exP*32+exM+1); exModal=2; }
    else { exClear(); exSave(); exModal=3; }
    sgDirty=1; voxPlay(V_yahoo);
}
static void exTick(void){   // once per logic step in the life game (main.c, next to stTick)
    if(!exP) return;
    static u8 c; if(++c<60) return; c=0;
    const XMs*x=exMs(); if(!x){ exQuit(); return; }
    if(exDay==255){ exDay=exDay8(); exSave(); }
    for(int k=0;k<TW_N;k++) if(twOn[k]==2&&!exHT[k]&&!(exTk&0x80)){ exTk|=0x80; exSave(); }   // someone came over (and is not here to bite)
    exScTick(x);
    int n=exTkN(x);
    for(int i=0;i<n;i++){ if((exTk>>i)&1) continue; const StT*t=&x->t[i]; if(!exEvKind(t->k)&&exTkNow(t)) exTkDone(i,x); }
    if(n&&(exTk&((1<<n)-1))==(1<<n)-1) exComplete();
}
static void exEnter(void){ exLoad(); for(int k=0;k<TW_N;k++) exHT[k]=0; exSpar=0; if(exP){ exAnnounce(); if(exShown!=(u8)(exP*32+exM+1)){ exShown=(u8)(exP*32+exM+1); exModal=1; } } }   // entering the life game
// ---- the look ----
static void exIcon(int p,int x,int y,int sc,u16 c){   // a picture per pack (9 x 8): a rocket, a zombie hand, a helmet, a fist, an egg, an hourglass
    static const char* const pic[XP_N][8]={
      {"....X....","...XXX...","...X.X...","...XXX...","...XXX...","..XXXXX..",".X.XXX.X.","...X.X..."},
      {".X.X.X...",".X.X.X.X.",".XXXXXXX.",".XXXXXXX.","..XXXXX..","..XXXXX..","...XXX...","...XXX..."},
      {"...XXX...",".XXXXXXX.","XXXXXXXXX","XXXXXXXXX","X.......X",".........","..X...X..","..XX.XX.."},
      {"..XXXX...",".XXXXXX..",".XXXXXXX.","XXXXXXXX.","XXXXXXXX.",".XXXXXX..","..XXXX...","..XXXX..."},
      {"...XXX...","..XXXXX..",".XX.X.XX.",".XXXXXXX.","XX.XXX.XX","XXXXXXXXX",".XXXXXXX.","..XXXXX.."},
      {"XXXXXXXXX",".X.....X.","..X...X..","...XXX...","...X.X...","..X...X..",".XXXXXXX.","XXXXXXXXX"}};
    for(int r=0;r<8;r++) for(int q=0;q<9;q++) if(pic[p][r][q]=='X') rect(x+q*sc,y+r*sc,sc,sc,c);
}
static u16 exMix(u16 a,u16 b,int t){ int r=(a&31)+(((b&31)-(a&31))*t>>4), g=((a>>5)&31)+((((b>>5)&31)-((a>>5)&31))*t>>4), bb=((a>>10)&31)+((((b>>10)&31)-((a>>10)&31))*t>>4); return RGB(r,g,bb); }
static void exCover(int p,int x,int y,int w,int h,int on){   // a pack's box: its colours, its picture, its roadmap number
    const XPk*P=&xPk[p];
    for(int j=0;j<h;j++) rect(x,y+j,w,1,exMix(P->a,P->b,j*10/h));
    rect(x,y,w,1,on?GOLD:P->b); rect(x,y+h-1,w,1,on?GOLD:RGB(2,2,4)); rect(x,y,1,h,on?GOLD:P->b); rect(x+w-1,y,1,h,on?GOLD:RGB(2,2,4));
    exIcon(p,x+(w-18)/2,y+8,2,on?WHITE:exMix(P->b,WHITE,8));
    rect(x+2,y+h-14,w-4,11,RGB(2,3,6)); { char b[4]; b[0]=(char)('0'+P->no); b[1]=0; text(x+(w-tw(b,1))/2,y+h-12,b,on?GOLD:DIMC,1); }
}
static int exModeRun(int p);   // the packs' modes (bortua.h, gotchi.h)
// ---- PACK: its missions ----
static void exPackScreen(int p,int ingame){
    const XPk*P=&xPk[p]; int hasMode=P->mode!=XMODE_NONE, rows=P->n+hasMode, sel=0, top=0;
    for(int i=0;i<P->n;i++) if(exOpen(p,i)&&!exDone(p,i)){ sel=i+hasMode; break; }   // (the first open mission not done yet)
    u16 prev=keyNow(); int dirty=1; u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)) return;
        if(pr&K_DOWN){ sel=(sel+1)%rows; dirty=1; } if(pr&K_UP){ sel=(sel+rows-1)%rows; dirty=1; }
        if(pr&K_R){ sel=sel+8<rows?sel+8:rows-1; dirty=1; } if(pr&K_L){ sel=sel>8?sel-8:0; dirty=1; }
        if(pr&K_A){
            if(hasMode&&sel==0){ exModeRun(p); prev=keyNow(); dirty=1; continue; }
            int m=sel-hasMode;
            if(exSoon(p,m)) toast("COMING SOON  A MISSION STILL BEING WRITTEN");
            else if(!exOpen(p,m)) toast("FINISH THE MISSION BEFORE IT FIRST");
            else if(!ingame) toast("PLAY YOUR LIFE  THEN PAUSE  STORY  PACKS");
            else if(P->adult&&stage<AG_ADULT) toast("THIS PACK IS FOR GROWN-UPS");
            else if(prIn()) toast("NOT FROM A PRISON CELL");
            else { static const char* const yn[2]={"START THIS MISSION","NOT NOW"}; const char*q=exP?"YOUR MISSION NOW ENDS":xPk[p].ms[m].nm;
                if(menu(q,yn,2)==0){ exStart(p,m); return; } }
            prev=keyNow(); dirty=1; continue; }
        if(!dirty&&(cnt&15)) { vsync(); continue; }
        dirty=0;
        if(sel<top) top=sel; if(sel>=top+9) top=sel-8;
        stBack(P->nm,(int)cnt);
        { char b[24]; char*e=slNum(b,exPackDone(p)); e=slCat(e," OF "); e=slNum(e,exPackPlay(p)); slCat(e," DONE"); text(231-tw(b,1),7,b,GOLD,1); }
        for(int r=0;r<9&&top+r<rows;r++){ int i=top+r, y=21+r*11, on=i==sel;
            if(on) s2grad(8,y-1,224,11,6,16,26,3,9,17);
            if(hasMode&&i==0){ rect(11,y+1,8,7,RGB(31,22,4)); text(23,y,P->modeNm,on?WHITE:GOLD,1); text(229-tw("PLAY",1),y,"PLAY",GOLD,1); continue; }
            int m=i-hasMode, done=exDone(p,m), soon=exSoon(p,m), open=exOpen(p,m), cur=exP==p+1&&exM==m;
            u16 bx=done?RGB(8,26,10):cur?GOLD:open?RGB(14,22,30):RGB(7,10,16); rect(11,y+1,8,7,bx); rect(12,y+2,6,5,RGB(2,6,13));
            if(done) text(12,y,"+",WHITE,1); else if(soon) text(13,y,"?",DIMC,1);
            { char b[48]; char*e=slNum(b,m+1); e=slCat(e,"  "); slCat(e,xPk[p].ms[m].nm); text(23,y,b,soon?(on?RGB(16,19,23):RGB(10,13,17)):done?RGB(10,22,12):open?WHITE:(on?RGB(20,24,28):RGB(12,16,22)),1); }
            const char*rt=soon?"SOON":cur?"NOW":!open?"LOCKED":0; char pb[12];
            if(!rt){ char*e=slCat(pb,"+"); e=slNum(e,xPk[p].ms[m].pay*100); rt=pb; }
            text(229-tw(rt,1),y,rt,cur?GOLD:soon||!open?RGB(12,16,22):RGB(14,24,16),1); }
        rect(8,121,224,1,RGB(14,26,31));
        if(hasMode&&sel==0){ text(12,125,"A MODE OF ITS OWN  NO LIFE NEEDED",RGB(20,26,31),1); text(12,134,P->bl[2],RGB(17,29,31),1); }
        else { const XMs*x=&P->ms[sel-hasMode]; text(12,125,x->br[0],RGB(20,26,31),1); text(12,134,x->br[1],RGB(20,26,31),1); }
        s2pill(5,147,tw("A START",1)+10,"A START"); s2pill(9+tw("A START",1)+10,147,tw("B BACK",1)+10,"B BACK");
        present();
    }
}
// ---- THE SHELF: every pack ----
static void exShelf(int ingame){
    int sel=0; if(exP) sel=exP-1;
    u16 prev=keyNow(); int dirty=1; u32 cnt=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)) return;
        if(pr&(K_RIGHT|K_R)){ sel=(sel+1)%XP_N; dirty=1; } if(pr&(K_LEFT|K_L)){ sel=(sel+XP_N-1)%XP_N; dirty=1; }
        if(pr&K_A){ exPackScreen(sel,ingame); if(ingame&&exModal) return; prev=keyNow(); dirty=1; continue; }
        if(!dirty){ vsync(); continue; }
        dirty=0;
        stBack("EXPANSION PACKS",(int)cnt);
        for(int p=0;p<XP_N;p++){ int on=p==sel; exCover(p,10+p*37,on?20:25,34,on?62:56,on); }
        const XPk*P=&xPk[sel];
        s2rr(8,88,224,54,RGB(10,20,30)); s2rr(9,89,222,52,RGB(2,6,13));
        exIcon(sel,15,96,3,P->b);
        text(48,93,P->nm,GOLD,1); text(48,102,P->sub,RGB(17,29,31),1);
        for(int i=0;i<3;i++) text(48,113+i*9,P->bl[i],RGB(20,26,31),1);
        { char b[32]; char*e=slNum(b,exPackDone(sel)); e=slCat(e,"/"); e=slNum(e,exPackPlay(sel)); slCat(e," DONE"); text(229-tw(b,1),93,b,WHITE,1);
          char*f=slNum(b,P->n); slCat(f," MISSIONS"); text(229-tw(b,1),102,b,DIMC,1); }
        if(P->mode) text(229-tw("+ A MODE",1),113,"+ A MODE",GOLD,1);
        { const char*a="A OPEN"; s2pill(5,147,tw(a,1)+10,a); s2pill(9+tw(a,1)+10,147,tw("LEFT RIGHT PICK",1)+10,"LEFT RIGHT PICK"); }
        present();
    }
}
// ---- the mission's cards and the journal ----
static void exCardBody(int p,int m,int y,int live){   // a mission's title, its two lines and its tasks
    const XMs*x=&xPk[p].ms[m];
    { char b[48]; char*e=slCat(b,"MISSION "); e=slNum(e,m+1); e=slCat(e,"  "); slCat(e,x->nm); text(16,y,b,GOLD,1); }
    text(16,y+10,x->br[0],RGB(20,26,31),1); text(16,y+19,x->br[1],RGB(20,26,31),1);
    exTkList(x,16,y+30,9,live);
}
static void exRunModal(void){   // the mission card (1 a mission starts, 2 one is done and the next starts, 3 the last one is done)
    int kind=exModal; exModal=0; u16 prev=keyNow(); u32 cnt=0; int p=kind==1?exP-1:exLastP;
    if(p<0||p>=XP_N) return;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_A|K_B|K_START)) return;
        stBack(kind==1?"EXPANSION PACK":"MISSION COMPLETE",(int)cnt);
        s2rr(8,22,224,118,RGB(16,27,31)); s2rr(9,23,222,116,RGB(2,6,13)); s2grad(10,24,220,114,3,9,19,1,4,10);
        exIcon(p,16,30,4,xPk[p].b);
        text(60,30,xPk[p].nm,GOLD,1); text(60,40,xPk[p].sub,RGB(17,29,31),1);
        if(kind==1){ char b[24]; char*e=slCat(b,"MISSION "); slNum(e,exM+1); text(60,52,b,WHITE,2); rect(14,72,212,1,RGB(14,26,31)); exCardBody(p,exM,78,1); }
        else {
            { char b[48]; char*e=slCat(b,"DONE  "); slCat(e,xPk[p].ms[exLastM].nm); text(60,52,b,WHITE,1); }
            { char b[40]; char*e=slCat(b,"+"); e=slNum(e,xPk[p].ms[exLastM].pay*100); slCat(e," SIMOLEONS  +15 JENES"); text(60,62,b,RGB(10,28,12),1); }
            stSparkle(cnt,12,26,216,100); rect(14,72,212,1,RGB(14,26,31));
            if(kind==2) exCardBody(p,exM,78,1);
            else if(exPackDone(p)>=exPackPlay(p)){ text(16,80,"PACK COMPLETE",GOLD,2); text(16,100,"EVERY MISSION IN THIS PACK IS DONE",WHITE,1); }
            else { text(16,80,"THAT IS ALL FOR NOW",GOLD,1); text(16,92,"MORE MISSIONS ARE COMING SOON",WHITE,1); }
        }
        s2pill(5,147,40,"A OK");
        { int ct; if(kind!=3&&exPlace(&ct)) text(50,149,"GO THERE  PAUSE  STORY  MISSION",RGB(17,29,31),1); }
        present();
    }
}
static void exJournal(void){   // pause menu > STORY > the mission: what it asks, live. A goes there, SELECT gives it up
    u16 prev=keyNow(); u32 cnt=0, lt=~0u;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(!exP) return;
        { int ct; if((pr&K_A)&&exPlace(&ct)){ int lot=ct==-2?nbT.home:trvPlace(ct); if(lot>=0&&trvGo(lot)){ sfxPlay(SFX_CASH); toast("THE CAB IS HERE"); } else if(lot<0) toast("NO SUCH PLACE IN TOWN"); return; } }
        if(pr&K_SEL){ static const char* const yn[2]={"KEEP GOING","GIVE IT UP"}; if(menu("GIVE UP THIS MISSION?",yn,2)==1){ exQuit(); toast("MISSION OVER  START IT AGAIN ANY TIME"); return; } prev=keyNow(); lt=~0u; continue; }
        if(pr&(K_A|K_B|K_START)) return;
        if(!pr&&(cnt>>4)==lt){ vsync(); continue; }
        lt=cnt>>4;
        int p=exP-1; stBack("MISSION",(int)cnt);
        exIcon(p,9,21,2,xPk[p].b); text(32,21,xPk[p].nm,GOLD,1); text(32,30,xPk[p].sub,RGB(17,29,31),1);
        { char b[24]; char*e=slNum(b,exM+1); e=slCat(e," OF "); slNum(e,xPk[p].n); text(233-tw(b,1),21,b,WHITE,1); }
        rect(14,42,212,1,RGB(14,26,31));
        exCardBody(p,exM,48,1);
        if(exCounts()){ char b[40]; char*e=slCat(b,exCounts()==1?"BOREZIES CURED  ":exCounts()==2?"SOLDIERS DOWN  ":"CHALLENGERS BEATEN  "); slNum(e,exCnt); text(16,112,b,RGB(17,29,31),1); }
        { int ct; if(exPlace(&ct)){ char b[34]; char*e=slCat(b,"A GO TO "); slCat(e,ct==-2?"HOME":ctNm[ct]); int w=tw(b,1)+10; s2pill(5,147,w,b); s2pill(9+w,147,tw("SEL GIVE UP",1)+10,"SEL GIVE UP"); }
          else { s2pill(5,147,50,"B BACK"); s2pill(59,147,tw("SEL GIVE UP",1)+10,"SEL GIVE UP"); } }
        present();
    }
}
static void exStoryTile(void){   // pause menu > STORY: the mission, the story journal, the packs
    for(;;){
        const char* it[3]; int id[3], n=0; static char mt[40] EWRAM_BSS;
        if(exP){ const XMs*x=exMs(); char*e=simCat(mt,"MISSION  "); simCat(e,x?x->nm:""); it[n]=mt; id[n++]=0; }
        it[n]="STORY JOURNAL"; id[n++]=1; it[n]="EXPANSION PACKS"; id[n++]=2;
        int c=menu("STORIES AND PACKS",it,n); if(c<0) return;
        if(id[c]==0) exJournal(); else if(id[c]==1){ storyScreen(); return; } else { exShelf(1); if(exModal) return; }
    }
}
// ---- a screen that takes over the life game (lifeModeRun plays it like the TV's clips) ----
static void exLaunch(void); static int exSpacewalk(int real);
static void exTakeRun(int t){
    if(t==EXT_LAUNCH) exLaunch();
    else if(t==EXT_SWTRAIN){ if(exSpacewalk(0)){ stQEvent(QE_SPACEWALK,1); toast("TRAINING PASSED  YOU ARE READY"); } else toast("MISSION CONTROL REELED YOU IN"); }
    else if(t==EXT_FIGHTER) exModeRun(XP_FIGHT);
    else if(t==EXT_GOTCHI) exModeRun(XP_GOTCHI);
    hhExtraDone();   // (the ghosts' and the cops' sprites go back into OBJ VRAM: a mode may have used it)
}
static int exSeatFree(void){ const XMs*x=exMs(); if(!x) return 0; for(int i=0;i<exTkN(x);i++) if(x->t[i].k==QK_EV&&x->t[i].a==QE_LAUNCH&&!((exTk>>i)&1)) return 1; return 0; }   // the mission booked your seat on the rocket
// ---- the player save chunk ----
static void sgEncExp(SlW*w){ exSave(); volatile u8*m=SRAM_BASE+EX_OFF; for(int i=0;i<EX_LEN;i++) slwPut(w,m[i]); }
