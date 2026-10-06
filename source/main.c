// BORE - GBA voxel creature creator (tech demo)
// Build space: 6 wide (X) x 4 long (Z) x 8 high (Y). Mode 3, no libraries.
// EYE and MOUTH are 2D sprites painted straight onto the front (+Z) face of the voxel under the cursor.
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;

#define REG_DISPCNT (*(volatile u16*)0x04000000)
#define REG_VCOUNT  (*(volatile u16*)0x04000006)
#define REG_KEYINPUT (*(volatile u16*)0x04000130)
#define REG_DMA3SAD (*(volatile u32*)0x040000D4)
#define REG_DMA3DAD (*(volatile u32*)0x040000D8)
#define REG_DMA3CNT (*(volatile u32*)0x040000DC)
#define VRAM_ADDR 0x06000000u
#define EWRAM_BSS __attribute__((section(".sbss"), aligned(4)))
// Hot loops run as ARM code from IWRAM (32-bit, zero-wait bus) instead of Thumb from the 16-bit ROM bus.
#define IWRAM_CODE __attribute__((section(".iwram"), long_call))
#define IWRAM_ARM __attribute__((section(".iwram"), target("arm"), long_call))
#define IWRAM_THUMB __attribute__((section(".iwram"), long_call))   // fast RAM, Thumb code: about 2/3 the size of ARM, for work that is not the per-pixel hot path
#define REG_WAITCNT (*(volatile u16*)0x04000204)
#include "save.h"   // the save chip: 128 KB flash, or 32 KB SRAM as the fallback (SRAM_BASE, svRd / svWr / svErase / svCommit)

#define SW 240
#define SH 160
#define W 6
#define D 4
#define H 8
// Voxel size: change CA (and CC) to resize everything; the starter model, sprites and limbs all scale with them.
#define CA 8    // cube half width (was 10)
#define CB (CA/2)   // cube half height of top face
#define CC 8    // cube side height (was 10)
#define HUG ((CA*3+5)/10)   // limb inset toward the torso (was 3 at CA 10)
#define OX 50
#define OY 96
#define PANEL_X 124

enum { K_A=1, K_B=2, K_SEL=4, K_START=8, K_RIGHT=16, K_LEFT=32, K_UP=64, K_DOWN=128, K_R=256, K_L=512 };

#define RGB(r,g,b) ((u16)((r)|((g)<<5)|((b)<<10)))
static u16 fb[SW*SH] EWRAM_BSS;
#define SPW 32   // the Sims in the room: baked at 0.4 size (5 screen pixels to 2), so the skater is a little under 2 tiles tall
#define SPH 60   // and the sprite has room above for tall Sims and MASTER CONTROLLER giants (a 32 x 64 hardware sprite for the household)
#define SPF 56   // the row the feet stand on in a sprite
static u16 spr4[4][SPW*SPH] EWRAM_BSS;   // the creature's sprites, one per view (bakeSprites)
static u16 spr4s[4][SPW*SPH] EWRAM_BSS;  // the same with the legs mid-stride (walking alternates the two)
static u32 sprKey;   // what spr4 / spr4s hold: the bake key of the player they were baked from (0 = something else, house.h)
#define STR_Y0 8                         // the stride frame differs from the standing one only in half-size rows STR_Y0..STR_Y1-1
#define STR_Y1 64                        // (OBJ tile rows 1..7: what household sprites keep a second copy of)
// The title screen only has to repaint two small areas of its backdrop (the smoke and the PRESS START box), so it keeps just those, in
// spr4: the title shows once at power on, before any sprite is baked. (This used to be a whole-screen copy inside a 124 KB sound buffer.)
#define tfb (&spr4[0][0])

// ---------- settings (kept in SRAM; the SETTINGS screen edits them) ----------
static u8 sFps=0;    // frame rate: 0 = 60, 1 = 30, 2 = 20, 3 = 15 frames per second (game speed stays the same). 60 is a ceiling: a slow picture just takes two vblanks and the logic catches up
static u8 sWall=1;   // walls: 0 full height, 1 cutaway (walls in front drop low), 2 all low
static u8 sWp=1;     // wallpaper patterns on
static u8 sFl=1;     // floor patterns on
static u8 sSnd=1;    // sound on
static u8 sShad=1;   // shadows under the player
static u8 sHud=0;    // on-screen info: 0 full, 1 slim, 2 off
static u8 sRom=0;    // ROM waits: 0 fast (3/1 + prefetch), 1 safe (power-on default, for fussy flash carts)
static u8 sUnlock=0;  // 1 = the Konami code was entered on the title screen: START+SELECT in the creator swaps creator screens
static u8 sClassic=0; // 1 = the secret classic creature screen (toggled with UP UP DOWN DOWN in the creator)
static u8 sNoWarn=0; // 1 = hide the TOO SLOW FOR THIS FRAME RATE warning in settings
static u8 sShow=0;   // performance counter: 0 off, 1 fps, 2 fps + load
static int cview;    // room view while the action cam spins (0..3, quarter turns); always 0 in the editor
static int lcN, lcPts, lcT, lcBank, lcBankT, lcamPend, lcamF;   // combo chain: tricks, points, time left, banked total + display time, cam queued, cam frame
static u8 sCam=1;    // action cam after a big combo: 0 off, 1 over 10000, 2 over 5000, 3 over 2000
static const int camThr[4]={0,10000,5000,2000};
#define CAM_LEN 84    // action cam length in game steps (1.4 s)
#define CAM_ZOOM 62   // zoom in by 256/(256-62) = 1.3x
static int lloadV;   // work per drawn frame as a percent of its time budget (PERFORMANCE INFO: DETAIL)
#define NWP 14       // wallpapers: the old 8x8 patterns ...
#define WALL_H 24     // (full wall height in px, 3 blocks: the textures in wallart.h are this tall)
#include "wallart.h"  // ... and NWX textures from the KHLVH wallpaper set (ROM only), wallpapers NWP.. (see the walls section)
#define NWALL (NWP+NWX)
#define NFL 14       // floors
#include "opts.h"   // extended options (xo[]): gameplay, input, audio, HUD and room options; also defines GOLD (the accent colour)


// ---------- palette ----------
// Colour rows of the creature creator: 8 swatches each. Swatch 0 of every row is the starter creature's colour.
#define NSW 8
#define NSKIN 24   // skin colours: the first 8 are the old ones (saves keep their looks), then more natural tones from palest to darkest, then fantasy colours
static const u16 skinTones[NSKIN] = { RGB(30,23,17), RGB(24,16,10), RGB(19,12,7), RGB(13,8,5), RGB(14,26,10), RGB(10,19,29), RGB(22,13,27), RGB(31,17,19),
    RGB(31,27,22), RGB(29,21,15), RGB(27,19,12), RGB(23,15,9), RGB(20,15,9), RGB(16,10,6), RGB(10,6,4), RGB(7,4,3),
    RGB(31,26,6), RGB(31,14,6), RGB(28,6,6), RGB(8,25,24), RGB(20,28,12), RGB(16,16,30), RGB(18,18,20), RGB(28,28,29) };
static const u16 hairTones[NSW] = { RGB(5,3,2), RGB(14,8,4), RGB(27,21,6), RGB(28,8,4), RGB(21,21,22), RGB(10,22,12), RGB(8,12,28), RGB(30,14,22) };
static const u16 topTones[NSW]  = { RGB(8,20,22), RGB(28,8,6), RGB(30,24,6), RGB(10,24,8), RGB(8,10,26), RGB(22,10,26), RGB(30,30,30), RGB(5,5,8) };
static const u16 eyeTones[NSW]  = { RGB(3,3,6), RGB(12,7,3), RGB(15,13,5), RGB(6,17,8), RGB(7,13,26), RGB(15,17,19), RGB(24,5,5), RGB(17,8,24) };   // dark, brown, hazel, green, blue, grey, red, violet
static const u16 botTones[NSW]  = { RGB(8,9,20), RGB(5,5,8), RGB(18,12,6), RGB(14,15,16), RGB(24,20,12), RGB(8,16,8), RGB(26,6,6), RGB(30,30,30) };
// The look: one number per choice in the creature creator. 0 everywhere = the starter creature.
enum { LK_SHAPE, LK_SKIN, LK_EYES, LK_MOUTH, LK_EARS, LK_HSTYLE, LK_HCOL, LK_TOP, LK_BOT, LK_BASE,
       LK_TONE=LK_BASE, LK_EARSZ, LK_EARLF, LK_TAIL, LK_HORNS, LK_BACK,
       LK_HAT, LK_HATCOL, LK_BEARD, LK_TOPSTY, LK_BOTSTY, LK_SHOE,   // (these six came with person format 5; 0 everywhere = the old look)
       LK_BROW, LK_NOSE, LK_CHEEK, LK_GLASS, LK_EYECOL,                // face details (person format 6): 0 = none / dark eyes
       LK_HEIGHT, LK_WEIGHT, LK_EYESZ, LK_EYESP, LK_EYEHT, LK_MOUTHW, LK_MOUTHHT,   // body and face sliders (format 6)
       LK_CLAWS, LK_ANTENNA, LK_PATTERN, LK_PATCOL,                    // more Spore parts and body paint (format 7)
       LK_HTONE, LK_TTONE, LK_BTONE, LK_EYETONE, LK_BROWHT, LK_NOSEHT, LK_TORSO, LK_ARMS, LK_STANCE,   // more sliders (format 8)
       LK_FEARS, LK_MUZZLE, LK_FTAIL, LK_BUTT, LK_BUTTH, LK_BUTTW,
       LK_LEGW, LK_ARMW, LK_ANTLEN, LK_ANTSPR, LK_ANTTIP, LK_TAILLEN, LK_HORNSZ, LK_TAILCURL, LK_TAILTHK, LK_HORNSPR, LK_HORNCRV, LK_HORNHT, LK_EARFWD, LK_EARSPR, LK_HEADSZ, LK_HANDFT, LK_WINGSZ, LK_TAILTIP,
       LK_NECK, LK_HIPW, LK_WAISTW, LK_SHOULW, LK_THIGHW, LK_CALFW, LK_TAILHT, LK_TAILSW, LK_TAILTL, LK_HORNFB, LK_HORNTH, LK_WINGSP, LK_WINGHT, LK_ANTFB, LK_ANTGAP, LK_EARWID,   // format 10: neck, body widths, more tail / horn / wing / antenna / ear sliders (appended: older saves keep their positions)
       LK_CHESTW, LK_BELLYW, LK_UARMW, LK_FARMW, LK_TAILTONE, LK_HORNTONE,   // format 11: chest, belly, upper arm and forearm width, tail and horn shade (appended: older saves keep their positions)
       LK_TAILTAPER, LK_TAILFLUF, LK_TAILWAVE, LK_TIPTONE, LK_HORNTIP, LK_WINGDROOP, LK_WINGTONE,   // format 12: tail taper, fluff, wave and tip shade, horn tip length, wing droop and shade (appended: older saves keep their positions)
       LK_JAWW, LK_HANDSZ, LK_FOOTSZ,   // format 13: jaw width (the bottom row of the head), hand size and foot size on their own (appended: older saves keep their positions)
       LK_NECKW,   // format 14: neck width (the neck is its own narrow column between the shoulders and the head)
       LK_N };   // animal (furry) ears, a muzzle, a fur tail (format 8); LK_BUTT: a slider, the seat (teens and up); LK_LEGW: leg width slider (format 9)
#define LK_N13 (LK_FOOTSZ+1)   // looks a person format 13 slot (and an 'H=' household) holds
#define LK_N12 (LK_WINGTONE+1)   // looks a person format 12 slot (and an 'H<' household) holds
#define LK_N11 (LK_HORNTONE+1)   // looks a person format 11 slot (and an 'H;' household) holds
#define LK_N10 (LK_EARWID+1)   // looks a person format 10 slot (and an 'H:' household) holds
#define LK_N9 (LK_TAILTIP+1)   // looks a person format 9 slot (and an 'H8' household) holds
#define LK_NSL10 ((LK_EARLF-LK_BASE+1)+(LK_MOUTHHT-LK_HEIGHT+1)+(LK_STANCE-LK_HTONE+1)+(LK_WINGSZ-LK_BUTT+1)+(LK_EARWID-LK_NECK+1))   // sliders before format 11
#define LK_NSL11 (LK_NSL10+(LK_HORNTONE-LK_CHESTW+1))   // sliders before format 12
#define LK_NSL12 (LK_NSL11+(LK_WINGTONE-LK_TAILTAPER+1))   // sliders before format 13
#define LK_NSL13 (LK_NSL12+(LK_FOOTSZ-LK_JAWW+1))   // sliders before format 14
#define LK_NSL (LK_NSL13+1)   // how many looks are sliders (9 values each)
#define LKPK13 ((LK_N13-LK_NSL13)+(LK_NSL13+1)/2)   // the same, in an 'H=' household (before the format 14 slider)
#define LKPK12 ((LK_N12-LK_NSL12)+(LK_NSL12+1)/2)   // the same, in an 'H<' household (before the format 13 sliders)
#define LKPK11 ((LK_N11-LK_NSL11)+(LK_NSL11+1)/2)   // the same, in an 'H;' household (before the format 12 sliders)
#define LKPK10 ((LK_N10-LK_NSL10)+(LK_NSL10+1)/2)   // the same, in an 'H:' household (before the format 11 sliders)
#define LKPK ((LK_N-LK_NSL)+(LK_NSL+1)/2)   // bytes a look takes in the household save: sliders are packed two to a byte
#define LK_N8 (LK_BUTTW+1)   // looks a person format 8 slot holds
#define LK_N7 (LK_PATCOL+1)   // looks a person format 7 slot holds
#define LK_N6 (LK_MOUTHHT+1)   // looks a person format 6 slot holds
#define LK_N5 (LK_SHOE+1)    // looks a person format 5 slot holds
#define LK_N4 (LK_BACK+1)    // looks a person format 4 slot holds   // LK_TONE, LK_EARSZ, LK_EARLF are sliders: 0 = middle, then 1..4 up, 5..8 down (see slidePos)
#define LK_N3 (LK_EARLF+1)   // looks a person format 3 slot holds (the Spore parts TAIL, HORNS, BACK came with format 4)
static inline int lkSlide(int id){ return (id>=LK_BASE&&id<=LK_EARLF)||(id>=LK_HEIGHT&&id<=LK_MOUTHHT)||(id>=LK_HTONE&&id<=LK_STANCE)||(id>=LK_BUTT&&id<=LK_WINGSZ)||(id>=LK_NECK&&id<=LK_EARWID)||(id>=LK_CHESTW&&id<=LK_HORNTONE)||(id>=LK_TAILTAPER&&id<=LK_WINGTONE)||(id>=LK_JAWW&&id<=LK_NECKW); }
static inline int slidePos(int v){ return (v+4)%9; }      // 0..8 left to right, the middle (stored 0) is 4
static inline int slideVal(int p){ return (p+5)%9; }
static inline int slideEff(int v){ return slidePos(v)-4; }   // -4..4
// MASTER CONTROLLER (OPTIONS > SIMU > CHEAT, only with the debug code: sUnlock from the title's Konami code, or dbgOn): a debug cheat console for extreme body sliders.
// DOUBLE SLIDERS: the SIZE sliders (heights, widths, sizes, lengths, colour tones) go twice as far a notch. The sliders that PLACE a part (spread,
// height on the head, front / back) keep their range, so ears, horns, antennae and arms never come loose from the body. LIMIT BREAK: every
// age builds in the adult box and the HEIGHT / TORSO / NECK stretch may go much further: taller than the 8 block box, drawn as stretched rows,
// so the body stays in one piece (the sprite bake still eases a giant down to fit its 32 x 44 box in the room).
static u8 dbgOn;   // (the title-screen debug code, set further down)
static inline int mcOn(void){ return sUnlock||dbgOn; }
static inline int mcDbl(void){ return mcOn()&&xo[XO_MCSLIDE]; }
static inline int mcBig(void){ return mcOn()&&xo[XO_MCBOX]; }
__attribute__((noinline)) static int slideEffS(int v){ int e=slidePos(v)-4; return mcDbl()?2*e:e; }   // a SIZE slider (in ROM, out of line: the drawing code in IWRAM stays small)
static u8 look[LK_N];
// ---- life stages ----  BABY (cannot be steered, walks about by itself), CHILD, TEEN, ADULT, ELDER (the last stage, slower and stooped). The creator's room to build in is smaller
// when young and grows with the age: a box of stBW x stBD x stBH blocks centred on the floor, a biggest block size and a list of looks
// each part picker may use. The adult box is the whole 6x4x8 space, so every old person and save is an ADULT.
enum { AG_BABY, AG_CHILD, AG_TEEN, AG_ADULT, AG_ELDER, AG_N };
static u8 stage=AG_ADULT;   // current life stage
static u8 ageDays;          // game days lived in this stage (grows the creature when it reaches the days set on the OPTIONS > TIME > AGES section, saved with the person)
static const char* const stageNm[AG_N]={"BABY","CHILD","TEEN","ADULT","ELDER"};
static const u8 stBW[AG_N]={4,4,6,6,6}, stBD[AG_N]={4,4,4,4,4}, stBH[AG_N]={5,6,7,8,7};   // build box (width is always even: parts mirror around its centre)
static const u8 stMaxSz[AG_N]={2,2,3,3,3};          // biggest block size S/M/L the builder offers
static u8 bxSt, bxBig, bxLift=6;   // bxSync: the stage whose build box is used, LIMIT BREAK on, the most the HEIGHT stretch may add
#define BXW stBW[bxSt]   // this life stage's build box and biggest block (MASTER CONTROLLER LIMIT BREAK: the adult's, for every age)
#define BXH stBH[bxSt]
#define BXD stBD[bxSt]
#define BXS stMaxSz[bxSt]
static const u8 stLegs[AG_N]={0,1,2,3,2};   // (an elder is stooped: a block lower than an adult)           // leg blocks showing under the torso before the shape trims them
static const u8 stSpd[AG_N]={50,80,95,100,70};       // walking speed in percent
// allowed looks per stage: bit n set = option n may be picked. Shape: AVERAGE BROAD BIG-HEAD STUBBY SLIM ATHLETIC TALL. Ears: NONE SMALL BIG. Hair: CROP BOWL LONG BALD.
// BIG HEAD (bit 2) is only on offer while the Konami code is switched on (see shapeMask).
#define NSHAPE 29   // + CHUBBY PEAR LANKY STOCKY HUNCHED POTBELLY MUSCLE PETITE BARREL DIGITIGRADE (7..16), then the humanoid builds V-SHAPE CURVY RUNNER SOFT POWER LONG LEGS (17..22), then the kids' builds PUDGY TODDLER SPROUT SPORTY BELL STURDY (23..28)
enum { SH_AVG, SH_BROAD, SH_BIGHEAD, SH_STUBBY, SH_SLIM, SH_ATHL, SH_TALL, SH_CHUBBY, SH_PEAR, SH_LANKY, SH_STOCKY, SH_HUNCH, SH_POT, SH_MUSCLE, SH_PETITE, SH_BARREL, SH_DIGI, SH_VSHAPE, SH_CURVY, SH_RUNNER, SH_SOFT, SH_POWER, SH_LONGLEG, SH_PUDGY, SH_TODDLER, SH_SPROUT, SH_SPORTY, SH_BELL, SH_STURDY };
#define SHKID (SHB(SH_PUDGY)|SHB(SH_TODDLER)|SHB(SH_SPROUT)|SHB(SH_SPORTY)|SHB(SH_BELL)|SHB(SH_STURDY))   // the builds made for babies and children (a baby has no legs to lengthen: no SPROUT)
#define SHALL (((1u<<NSHAPE)-1)&~SHKID)   // everything a teen, adult or elder may pick
#define SHHUM (SHB(SH_VSHAPE)|SHB(SH_CURVY)|SHB(SH_SOFT)|SHB(SH_POWER)|SHB(SH_LONGLEG))   // the adult-frame builds: teens and up, and a baby or child only with the debug code
#define SHB(n) (1u<<(n))
static const u32 stMaskShape[AG_N]={   // every age gets a real choice of bodies (a baby has no legs to speak of, so no LANKY or DIGITIGRADE)
    SHB(SH_BIGHEAD)|SHB(SH_STUBBY)|SHB(SH_CHUBBY)|SHB(SH_PEAR)|SHB(SH_STOCKY)|SHB(SH_POT)|SHB(SH_PETITE)|SHB(SH_BARREL)|SHB(SH_MUSCLE)|(SHKID&~SHB(SH_SPROUT)),
    13|SHB(SH_CHUBBY)|SHB(SH_PEAR)|SHB(SH_LANKY)|SHB(SH_STOCKY)|SHB(SH_HUNCH)|SHB(SH_POT)|SHB(SH_MUSCLE)|SHB(SH_PETITE)|SHB(SH_BARREL)|SHB(SH_DIGI)|SHB(SH_RUNNER)|SHKID,
    SHALL&~SHB(SH_BROAD), SHALL, SHALL };
static const u8 stMaskEars[AG_N]={3,7,7,7,7};
#define NHAIR 9   // CROP BOWL LONG BALD + SPIKY AFRO FLAT TOP SIDE TAIL BUN
static const u16 stMaskHair[AG_N]={9,11|0x1F0,15|0x1F0,15|0x1F0,15|0x1F0};
static inline int shapeMask(void){ int m=(int)stMaskShape[stage]; if(!sUnlock) m&=~4; else if(stage<AG_TEEN) m|=(int)SHHUM; return m; }   // BIG HEAD, and V-SHAPE / CURVY / SOFT / POWER / LONG LEGS below teen, are only on offer with the debug code
static const u8 stSwatches[AG_N]={4,6,8,8,8};       // how many colours of each row are on offer
#define BX0 ((W-BXW)/2)
static u16 base[9+NWP], sT[9+NWP], sL[9+NWP], sR[9+NWP];   // slots 1..8 = body colours, 9.. = wallpaper average colours
static u16 wpEdge[NWP][3];   // wall block outline colours (top, left face, right face): set by setColors
static u16 dL[9], dR[9];   // face-sprite palette (k w r s i b g h l) pre-shaded for the left / right cube face
#define EDGE RGB(3,2,5)
#define SKY  RGB(20,26,31)
#define PANEL RGB(5,6,9)

static inline __attribute__((always_inline)) u16 shade(u16 c, int n) {
    int r=c&31, g=(c>>5)&31, b=(c>>10)&31;
    return RGB(r*n/16, g*n/16, b*n/16);
}
// ---------- wallpapers & floors: 8x8 texels, each char 0-3 picks one of the 4 colours ----------
typedef struct { const char*nm; u16 c[4]; const char*p[8]; } Tex;
#define PN_FLAT  "00000000","00000000","00000000","00000000","00000000","00000000","00000000","00000000"
#define PN_NOISE "01000200","00020010","20001000","00100020","01000100","00200001","10002000","00010200"
#define PN_CONC  "00000100","02000000","00001000","00000020","00100000","00000002","20000000","00020100"
#define PN_TILE  "11111111","12001200","10001000","10001000","11111111","12001200","10001000","10001000"
#define PN_STEEL "00000000","01110000","00220000","00000000","00000000","00000111","00000022","00000000"
#define PN_HAZ   "00110011","10011001","11001100","01100110","00110011","10011001","11001100","01100110"
// Wallpaper tiles repeat every block (8 px), so they line up across a whole wall. Colours: house first, factory after.
static const Tex wpTex[NWP]={
 {"TEAL PAINT",{RGB(8,20,22),0,0,0},{PN_FLAT}},
 {"FLORAL",{RGB(28,26,20),RGB(26,12,16),RGB(10,20,9),RGB(30,26,8)},{"00000000","00100000","01310020","00102200","00000000","00000100","20001310","02200100"}},
 {"PEACH STRIPE",{RGB(30,23,18),RGB(31,29,24),RGB(26,16,15),0},{"11102000","11102000","11102000","11102000","11102000","11102000","11102000","11102000"}},
 {"MEMPHIS",{RGB(30,30,28),RGB(4,22,22),RGB(29,9,18),RGB(31,27,5)},{"01000010","10100101","00000000","00022000","00222200","00000030","30000000","00000000"}},
 {"WOOD PANEL",{RGB(18,11,5),RGB(11,6,3),RGB(22,14,7),0},{"10201000","10001020","10021002","10001000","10201020","10001000","10021002","10001000"}},
 {"DIAMONDS",{RGB(20,13,17),RGB(25,18,21),RGB(14,8,12),0},{"00010000","00101000","01000100","10020010","01000100","00101000","00010000","00000000"}},
 {"GINGHAM",{RGB(29,29,29),RGB(18,22,29),RGB(10,15,26),0},{"22112211","22112211","11001100","11001100","22112211","22112211","11001100","11001100"}},
 {"CORRUGATED",{RGB(15,17,18),RGB(22,24,25),RGB(9,11,12),0},{"10201020","10201020","10201020","10201020","10201020","10201020","10201020","10201020"}},
 {"RED BRICK",{RGB(20,8,6),RGB(22,20,18),RGB(14,5,4),0},{"00010001","02010201","00010001","11111111","01000100","01020102","01000100","11111111"}},
 {"CINDER BLOCK",{RGB(17,17,17),RGB(11,11,11),RGB(21,21,20),0},{"10000000","10200000","10000020","11111111","00001000","02001000","00001020","11111111"}},
 {"HAZARD",{RGB(30,25,2),RGB(4,4,5),0,0},{PN_HAZ}},
 {"GREEN TILE",{RGB(10,20,14),RGB(22,24,22),RGB(14,25,18),0},{PN_TILE}},
 {"STEEL PLATE",{RGB(14,16,18),RGB(22,24,26),RGB(8,9,11),0},{PN_STEEL}},
 {"CONCRETE",{RGB(16,16,15),RGB(19,19,18),RGB(12,12,11),0},{PN_CONC}},
};
// Floor textures are mapped onto each iso tile in tile space (a = along +x, b = along +y).
static const Tex flTex[NFL]={
 {"TAN CHECK",{RGB(26,21,14),0,0,0},{PN_FLAT}},
 {"BEIGE CARPET",{RGB(24,21,16),RGB(21,18,13),RGB(27,24,19),0},{PN_NOISE}},
 {"TEAL CARPET",{RGB(6,16,16),RGB(4,12,13),RGB(8,19,19),0},{PN_NOISE}},
 {"CHECKER LINO",{RGB(29,29,28),RGB(5,5,8),0,0},{"00001111","00001111","00001111","00001111","11110000","11110000","11110000","11110000"}},
 {"WOOD PLANKS",{RGB(20,13,6),RGB(12,7,3),RGB(23,16,8),0},{"11111111","10020000","10000200","10002000","11111111","02001000","00201000","00001002"}},
 {"PINK TILE",{RGB(28,18,20),RGB(30,28,27),RGB(30,22,23),0},{PN_TILE}},
 {"BLUE TILE",{RGB(9,19,26),RGB(28,29,30),RGB(14,24,29),0},{PN_TILE}},
 {"CONCRETE",{RGB(15,15,14),RGB(18,18,17),RGB(12,12,11),0},{PN_CONC}},
 {"STEEL PLATE",{RGB(12,14,16),RGB(20,22,24),RGB(7,8,10),0},{PN_STEEL}},
 {"METAL GRATE",{RGB(4,5,7),RGB(17,18,20),RGB(10,11,13),0},{"11111111","10001000","10201020","10001000","11111111","10001000","10201020","10001000"}},
 {"HAZARD",{RGB(30,25,2),RGB(4,4,5),0,0},{"00000000","00000000","11111111","11111111","00000000","00000000","11111111","11111111"}},
 {"GREEN LINO",{RGB(12,19,10),RGB(14,22,12),RGB(10,16,8),0},{PN_NOISE}},
 {"OIL STAINED",{RGB(14,14,13),RGB(8,8,9),RGB(11,11,11),0},{"00000000","00022000","00211200","00022100","00002000","00000000","00000000","00000000"}},
 {"RED TILE",{RGB(22,8,5),RGB(14,6,4),RGB(25,11,7),0},{PN_TILE}},
};
static const u8 flVs[NFL]={14,15,15,16,14,15,15,15,15,15,15,15,15,15};   // shade (of 16) for the odd tiles of a checkerboard of tiles
static u16 wpAvg[NWP];   // average colour of each wallpaper: wall tops and the "wallpaper off" look


static u16 toneBy(u16 c,int e){   // a tone slider: darker to the left, lighter to the right (a step each notch)
    if(e<0) return shade(c,16+e*2);
    if(e>0){ int r=c&31,g=(c>>5)&31,b=(c>>10)&31; return RGB(r+(31-r)*e/8,g+(31-g)*e/8,b+(31-b)*e/8); }
    return c;
}
static void setColors(void) {
    base[1]=toneBy(skinTones[look[LK_SKIN]],slideEffS(look[LK_TONE]));   // skin tone
    base[2]=RGB(31,31,31); base[3]=RGB(3,3,6);
    base[4]=RGB(29,12,16);    base[5]=toneBy(hairTones[look[LK_HCOL]],slideEffS(look[LK_HTONE]));
    base[6]=toneBy(topTones[look[LK_TOP]],slideEffS(look[LK_TTONE])); base[7]=toneBy(botTones[look[LK_BOT]],slideEffS(look[LK_BTONE])); base[8]=RGB(31,30,16);
    for (int i=1;i<9;i++){ sT[i]=base[i]; sL[i]=shade(base[i],12); sR[i]=shade(base[i],9); }
    for (int i=0;i<NWP;i++){ int s=9+i; base[s]=wpAvg[i]; sT[s]=base[s]; sL[s]=shade(base[s],12); sR[s]=shade(base[s],9);
        wpEdge[i][0]=shade(sT[s],9); wpEdge[i][1]=shade(sL[s],9); wpEdge[i][2]=shade(sR[s],9); }
    u16 sk=base[1], bl=RGB(((sk&31)+31)/2,(((sk>>5)&31)+8)/2,(((sk>>10)&31)+12)/2);
    u16 dc[9]={ base[3], base[2], base[4], shade(base[1],11),        // k dark, w white, r red, s lid shadow (also the nose and freckles)
                toneBy(eyeTones[look[LK_EYECOL]%NSW],slideEffS(look[LK_EYETONE])), bl, RGB(6,6,8),         // i iris, b blush, g glasses frame
                shade(base[5],10), RGB(4,5,9) };                       // h brows (the hair colour, darker), l dark lenses
    for (int i=0;i<9;i++){ dL[i]=shade(dc[i],12); dR[i]=shade(dc[i],9); }
}

// ---------- drawing ----------
// Clip rectangle: every drawing primitive stays inside it. The life scene is redrawn a rectangle at a time (see drawRoomRect), so the
// rectangle is set around each piece of work and put back to the whole screen afterwards. cW / cH are unsigned so one compare tests a point.
static int cX0=0, cY0=0; static unsigned cW=SW, cH=SH;
#ifdef SELFTEST
static unsigned cntWB, cntFT, cntBI, cntTiles, cntWBcols;
#define CNT(v) (v)++
#else
#define CNT(v)
#endif
static inline void clipSet(int x0,int y0,int x1,int y1){ cX0=x0; cY0=y0; cW=(unsigned)(x1-x0); cH=(unsigned)(y1-y0); }
static inline void clipAll(void){ cX0=0; cY0=0; cW=SW; cH=SH; }
static inline __attribute__((always_inline)) void px(int x,int y,u16 c){ if((unsigned)(x-cX0)<cW && (unsigned)(y-cY0)<cH) fb[y*SW+x]=c; }
IWRAM_CODE static void vline(int x,int y0,int y1,u16 c){
    if((unsigned)(x-cX0)>=cW) return; int ye=cY0+(int)cH-1; if(y0<cY0)y0=cY0; if(y1>ye)y1=ye;
    u16*p=&fb[y0*SW+x]; for(;y0<=y1;y0++,p+=SW) *p=c;
}
IWRAM_CODE static void rect(int x,int y,int w,int h,u16 c){
    int x1=x+w, y1=y+h; if(x<cX0)x=cX0; if(y<cY0)y=cY0; if(x1>cX0+(int)cW)x1=cX0+(int)cW; if(y1>cY0+(int)cH)y1=cY0+(int)cH;
    if(x>=x1||y>=y1) return;
    u32 cc=(u32)c|((u32)c<<16);   // two pixels a store
    for(;y<y1;y++){ u16*p=&fb[y*SW+x]; int n=x1-x;
        if((uintptr_t)p&2){ *p++=c; n--; }
        u32*q=(u32*)p; for(int m=n>>1;m>0;m--) *q++=cc;
        if(n&1) *(u16*)q=c; }
}
IWRAM_THUMB static void line(int x0,int y0,int x1,int y1,u16 c){
    int dx=x1>x0?x1-x0:x0-x1, dy=y1>y0?y0-y1:y1-y0, sx=x0<x1?1:-1, sy=y0<y1?1:-1, e=dx+dy;
    for(;;){ px(x0,y0,c); if(x0==x1&&y0==y1)break; int e2=2*e;
        if(e2>=dy){e+=dy;x0+=sx;} if(e2<=dx){e+=dx;y0+=sy;} }
}

// Smooth proportional font (generated by tools/make_font.py from assets/font): 9 coverage steps blended over the screen.
#include "fontdata.h"
static signed char fAsc[128]; static u8 fAscOk;   // ASCII -> glyph number, built on first use (fIdx used to scan FNT_CHARS for every character drawn or measured)
_Static_assert(FNT_NASCII<128,"fAsc holds glyph numbers in a signed char");
static int fIdx(char ch){
    if(!fAscOk){ for(int i=0;i<128;i++) fAsc[i]=-1; const char*q=FNT_CHARS; for(int i=0;*q;q++,i++) if((unsigned char)*q<128&&fAsc[(int)*q]<0) fAsc[(int)*q]=(signed char)i; fAscOk=1; }
    if((unsigned char)ch<128) return fAsc[(int)ch];
    const char*p=FNT_CHARS; for(int i=0;*p;p++,i++) if(*p==ch) return i; return -1; }
// One character of UTF-8 text -> glyph index (advances *ps). ASCII is looked up in FNT_CHARS; accented letters, inverted marks and so on
// (FNT_EXTRA) by Unicode code point; characters the font has no glyph for fall back to a look-alike (capital accents -> the plain
// capital, curly quotes -> ' and ") and otherwise to a gap (the table is generated into fontdata.h). Strings in songs.h / artists.h can therefore hold real UTF-8.
static int fCode(unsigned cp){
    for(int k=0;k<FNT_NEXTRA;k++) if(FNT_EXTRA[k]==cp) return FNT_NASCII+k;
    for(int k=0;k<FNT_NFALLBACK;k++) if(FNT_FALLBACK[k].cp==cp) return fIdx(FNT_FALLBACK[k].as);
    return -1;
}
static int fNext(const char**ps){
    const u8*p=(const u8*)*ps; unsigned c=*p++;
    if(c>=0xC0){                                   // a multi-byte UTF-8 sequence
        int n=c<0xE0?1:c<0xF0?2:3; unsigned cp=c&(0x3Fu>>n);
        while(n-->0&&(*p&0xC0)==0x80) cp=(cp<<6)|(*p++&0x3Fu);
        *ps=(const char*)p; return fCode(cp);
    }
    *ps=(const char*)p; return fIdx((char)c);
}
// width in pixels of a string at scale sc (1 = small, 2 = medium, 3+ = large)
static int tw(const char*s,int sc){
    const u8*adv=sc<=1?fa_s:sc==2?fa_m:fa_l; int sp=sc<=1?FSP_s:sc==2?FSP_m:FSP_l, w=0;
    while(*s){ int i=fNext(&s); w+=(i<0)?sp:adv[i]; } return w;
}
IWRAM_THUMB static int text(int x,int y,const char*s,u16 c,int sc){
    const u32*fo=sc<=1?fo_s:sc==2?fo_m:fo_l; const u8*fw=sc<=1?fw_s:sc==2?fw_m:fw_l, *fa=sc<=1?fa_s:sc==2?fa_m:fa_l, *fp=sc<=1?fp_s:sc==2?fp_m:fp_l, *fx=sc<=1?fx_s:sc==2?fx_m:fx_l;
    int fh=sc<=1?FH_s:sc==2?FH_m:FH_l, sp=sc<=1?FSP_s:sc==2?FSP_m:FSP_l, ft=sc<=1?FTOP_s:sc==2?FTOP_m:FTOP_l;   // ft = extra rows above the capitals (room for accents on capitals); y is still the top of the capitals
    int cr=c&31, cg=(c>>5)&31, cb=(c>>10)&31;
    while(*s){
        int i=fNext(&s); if(i<0){ x+=sp; continue; }
        int w=fw[i]; const u8*g=fp+fo[i]; int r0=fx[i]?0:ft; g+=r0*w;   // no ink above the capitals: start at the capitals
        for(int r=r0;r<fh;r++){ int yy=y-ft+r; if((unsigned)(yy-cY0)>=cH){ g+=w; continue; }
            u16*d=&fb[yy*SW];
            for(int q=0;q<w;q++){ int a=g[q]; if(!a) continue; int xx=x+q; if((unsigned)(xx-cX0)>=cW) continue;
                if(a>=8){ d[xx]=c; continue; }
                u16 b=d[xx]; int ia=8-a;
                int R=((b&31)*ia+cr*a)>>3, G=(((b>>5)&31)*ia+cg*a)>>3, B=(((b>>10)&31)*ia+cb*a)>>3;
                d[xx]=(u16)(R|(G<<5)|(B<<10)); }
            g+=w; }
        x+=fa[i];
    }
    return x;
}
static void num(int x,int y,int n,u16 c){ char s[2]={(char)('0'+n),0}; text(x,y,s,c,1); }

static inline __attribute__((always_inline)) u16 lite(u16 c,int n){
    int r=(c&31)*n/16, g=((c>>5)&31)*n/16, b=((c>>10)&31)*n/16;
    if(r>31)r=31; if(g>31)g=31; if(b>31)b=31; return RGB(r,g,b);
}
// Soft voxel: tonal outline only on the silhouette (no seams between joined blocks), lit rim, shaded base.
// shape 0 block, 1 slim limb, 2 hand, 3 leg. f: 1 block above, 2 block below, 16/32 coplanar neighbour at left/right edge.
static const u8 rTab[4]={CA,CA*7/10,CA/2,CA*4/5};   // block, slim limb, hand, leg half widths
static u8 hhT[4][CA+1];   // hhT[shape][|t|] = (r/2)*(r-|t|)/r, filled once in initTables (no division in the hot loop)
static int cubeDR;   // WEIGHT slider: added to a block's half width (0 everywhere but the creature's body)
IWRAM_CODE static void cube(int sx,int sy,int ci,int shape,int f){
    int r=rTab[shape], ch=shape==2?CC*7/10:CC; const u8*hhp=hhT[shape];
    if(cubeDR){ static u8 hv[CA+8]; r+=cubeDR; if(r<3) r=3; if(r>CA+6) r=CA+6; for(int a=0;a<=r;a++) hv[a]=(u8)((r/2)*(r-a)/r); hhp=hv; }
    if(shape) f&=3;
    u16 T=sT[ci], L=sL[ci], R=sR[ci], eT=shade(T,9), eL=shade(L,9), eR=shade(R,9);
    u16 hiL=lite(L,19), hiR=lite(R,19), loL=shade(L,13), loR=shade(R,13);   // the lit rim and the shaded base, worked out once per block
    int t0=-r, t1=r; if(sx+t0<cX0) t0=cX0-sx; if(sx+t1>=cX0+(int)cW) t1=cX0+(int)cW-1-sx;
    for(int t=t0;t<=t1;t++){
        int at=t<0?-t:t, hh=hhp[at], x=sx+t, yt=sy+hh, yb=yt+ch-1;
        u16 sc=t<0?L:R, ec=t<0?eL:eR;
        vline(x,yt,yb,sc);
        vline(x,sy-hh,sy+hh,T);
        if(!(f&1)){ px(x,yt+1,t<0?hiL:hiR); px(x,sy-hh,eT); }
        if(!(f&2)){ px(x,yb-1,t<0?loL:loR); px(x,yb,ec); }
        if((t==-r&&!(f&16))||(t==r&&!(f&32))) vline(x,sy-hh,yb,ec);
    }
}

// ---------- textured walls and floors ----------
static u16 wpTab[NWP][2][8][8] EWRAM_BSS;              // [wallpaper][0 left face / 1 right face][column][row], pre-shaded
static u16 wpHi[NWP][2][8], wpLo[NWP][2][8];            // per column: the lit row under the top edge, and the shaded row above the bottom edge
static u16 flTab[NFL][2][2*CB+1][2*CA+1] EWRAM_BSS;    // [floor][odd tile][row][column] pre-sampled onto the iso diamond (row-major: drawn as horizontal spans)
static u8 rowHW[CB+1];   // rowHW[|y|] = half width of the diamond on that row
static u16 flFlat[NFL][2];                             // plain-colour fallback ("floor patterns off")
static u16 avgTex(const Tex*t){
    int r=0,g=0,b=0;
    for(int v=0;v<8;v++)for(int u=0;u<8;u++){ u16 c=t->c[t->p[v][u]-'0']; r+=c&31; g+=(c>>5)&31; b+=(c>>10)&31; }
    return RGB(r/64,g/64,b/64);
}
static void bakeTex(void){   // needs hhT (filled by initTables)
    for(int w=0;w<NWP;w++){
        const Tex*t=&wpTex[w]; wpAvg[w]=avgTex(t);
        for(int f=0;f<2;f++)for(int u=0;u<8;u++){ for(int v=0;v<8;v++) wpTab[w][f][u][v]=shade(t->c[t->p[v][u]-'0'],f?9:12); wpHi[w][f][u]=lite(wpTab[w][f][u][1],19); wpLo[w][f][u]=shade(wpTab[w][f][u][6],13); }
    }
    for(int fl=0;fl<NFL;fl++){
        const Tex*t=&flTex[fl]; u16 av=avgTex(t); int vs=flVs[fl];
        flFlat[fl][0]=av; flFlat[fl][1]=shade(av,vs);
        for(int var=0;var<2;var++)for(int tt=-CA;tt<=CA;tt++){
            int hh=hhT[0][tt<0?-tt:tt];
            for(int y=-hh;y<=hh;y++){
                int X=tt*CB+y*CA+CA*CB, Y=y*CA-tt*CB+CA*CB;      // tile-space position, 0..2*CA*CB
                int ta=X*8/(2*CA*CB), tb=Y*8/(2*CA*CB);
                if(ta<0)ta=0; if(ta>7)ta=7; if(tb<0)tb=0; if(tb>7)tb=7;
                u16 c=t->c[t->p[tb][ta]-'0']; if(var) c=shade(c,vs);
                flTab[fl][var][y+CB][tt+CA]=c;
            }
        }
    }
}
// One wall block with its wallpaper on both faces. Same silhouette and outline as cube(); f as for cube().
// The baseboard / crown lines come from cube's own edge rows, so they stay visible over the pattern.
IWRAM_CODE static void wallBlock(int sx,int sy,int wp,int f){
    CNT(cntWB);
    const u8*hhp=hhT[0]; int sl=9+wp;
    u16 T=sT[sl], eT=wpEdge[wp][0], eL=wpEdge[wp][1], eR=wpEdge[wp][2];
    int t0=-CA, t1=CA; if(sx+t0<cX0) t0=cX0-sx; if(sx+t1>=cX0+(int)cW) t1=cX0+(int)cW-1-sx;
    int ye=cY0+(int)cH-1;
    for(int t=t0;t<=t1;t++){
        int x=sx+t, hh=hhp[t<0?-t:t], ytop=sy-hh, yt=sy+hh, yb=yt+CC-1;
        int face=t<0?0:1, u=t<0?t+CA:(t&7);
        const u16*col=wpTab[wp][face][u]; u16 ec=t<0?eL:eR;
        int edge=(t==-CA&&!(f&16))||(t==CA&&!(f&32));
        if(ytop>=cY0&&yb<=ye){   // the whole column is inside the clip: no per-pixel tests
            u16*d=&fb[ytop*SW+x];
            if(edge){ for(int y=ytop;y<=yb;y++,d+=SW) *d=ec; continue; }
            for(int k=yt-ytop;k>=0;k--,d+=SW) *d=T;
            const u16*cp=col+1; for(int v=1;v<CC;v++,d+=SW,cp++) *d=*cp;
            if(!(f&1)){ fb[(yt+1)*SW+x]=wpHi[wp][face][u]; fb[ytop*SW+x]=eT; }
            if(!(f&2)){ fb[(yb-1)*SW+x]=wpLo[wp][face][u]; fb[yb*SW+x]=ec; }
        } else {                 // partly outside: clip every piece
            { int ya=yt<cY0?cY0:yt, yz=yb>ye?ye:yb; if(ya<=yz){ u16*d=&fb[ya*SW+x]; const u16*cp=col+(ya-yt); for(int y=ya;y<=yz;y++,d+=SW,cp++) *d=*cp; } }
            vline(x,ytop,yt,T);
            if(!(f&1)){ px(x,yt+1,wpHi[wp][face][u]); px(x,ytop,eT); }
            if(!(f&2)){ px(x,yb-1,wpLo[wp][face][u]); px(x,yb,ec); }
            if(edge) vline(x,ytop,yb,ec);
        }
    }
}
// Span helpers: four pixels a round (no loop branch per pixel). The loads come before the stores, so a span copied over itself still works.
static inline __attribute__((always_inline)) void cpyHW(u16*d,const u16*s,int n){
    while(n>=4){ u16 a=s[0],b=s[1],c=s[2],e=s[3]; d[0]=a; d[1]=b; d[2]=c; d[3]=e; d+=4; s+=4; n-=4; }
    while(n-->0) *d++=*s++;
}
static inline __attribute__((always_inline)) void fillHW(u16*d,int n,u16 c){
    while(n>=4){ d[0]=c; d[1]=c; d[2]=c; d[3]=c; d+=4; n-=4; }
    while(n-->0) *d++=c;
}
// One floor tile: copy the pre-sampled columns. tex = flTab[floor][odd][0][0].
IWRAM_CODE static void floorTile(int sx,int sy,const u16*tex){
    CNT(cntFT);   // one scanline span per row instead of one call per column
    int xa=cX0, xz=cX0+(int)cW-1;
    for(int ry=-CB;ry<=CB;ry++,tex+=2*CA+1){
        int y=sy+ry; if((unsigned)(y-cY0)>=cH) continue;
        int hw=rowHW[ry<0?-ry:ry], x0=sx-hw, x1=sx+hw; const u16*sp=tex+(CA-hw);
        if(x0<xa){ sp+=xa-x0; x0=xa; } if(x1>xz) x1=xz; if(x0>x1) continue;
        cpyHW(&fb[y*SW+x0],sp,x1-x0+1);
    }
}

IWRAM_CODE static void tileTop(int sx,int sy,u16 c){
    int xa=cX0, xz=cX0+(int)cW-1;
    for(int ry=-CB;ry<=CB;ry++){
        int y=sy+ry; if((unsigned)(y-cY0)>=cH) continue;
        int hw=rowHW[ry<0?-ry:ry], x0=sx-hw, x1=sx+hw;
        if(x0<xa) x0=xa; if(x1>xz) x1=xz; if(x0>x1) continue;
        fillHW(&fb[y*SW+x0],x1-x0+1,c);
    }
}


// ---------- face sprites ----------
// One cell = 9 x 8 px of art on a cube face. Wider sprites span cells: width = 10*cells-1 (the seam column is art too).
// Palette: k dark, w white, r red, s skin shadow, i iris (EYE COLOUR), b blush, g glasses frame, h brow (hair colour), l dark lens, . clear
#define B9  "........."
#define B19 "..................."
static const char* const aHalf[8] ={ B9, "..sssss..", ".kkkkkkk.", ".wwkikww.", "..wkikw..", "...www...", B9, B9 };
static const char* const aRound[8]={ B9, "..kkkkk..", ".kwwwwwk.", ".kwkikwk.", ".kwkikwk.", ".kwwwwwk.", "..kkkkk..", B9 };
static const char* const aHappy[8]={ B9, B9, "...kkk...", "..k...k..", ".k.....k.", B9, B9, B9 };
static const char* const aWide[8] ={ B9, ".kkkkkkk.", "kwwwwwwwk", "kwwiiiwwk", "kwwikiwwk", "kwwiiiwwk", ".kwwwwwk.", "..kkkkk.." };
static const char* const aAngry[8]={ B9, "kk.......", ".kkkk....", ".wwkkkkk.", ".wiikiww.", "..wiiiw..", "...www...", B9 };
static const char* const aCute[8] ={ B9, "..kkkkk..", ".kiiiiik.", ".kwiiiik.", ".kiiiwik.", ".kiiiiik.", "..kkkkk..", B9 };
static const char* const aCat[8]  ={ B9, "...kkk...", "..kiiik..", ".kiikiik.", ".kiikiik.", "..kiiik..", "...kkk...", B9 };
static const char* const aDot[8]  ={ B9, B9, B9, "...kkk...", "...kkk...", B9, B9, B9 };
static const char* const aLash[8] ={ B9, ".k.k.k.k.", "..kkkkk..", ".kwwiwwk.", ".kwikiwk.", "..kwwwk..", "...kkk...", B9 };
static const char* const mFlat[8] ={ B19, B19, B19, ".....kkkkkkkkk.....", B19, B19, B19, B19 };
static const char* const mSmile[8]={ B19, B19, "...k...........k...", "....k.........k....", ".....kkkkkkkkk.....", B19, B19, B19 };
static const char* const mOh[8]   ={ B19, ".......kkkkk.......", "......krrrrrk......", "......krrrrrk......", ".......kkkkk.......", B19, B19, B19 };
static const char* const mGrin[8] ={ B19, B19, "...kkkkkkkkkkkkk...", "...kwwwwwwwwwwwk...", "....kwwwwwwwwwk....", ".....kkkkkkkkk.....", B19, B19 };
static const char* const mSmirk[8]={ B19, B19, "...............k...", "..............k....", ".....kkkkkkkkk.....", B19, B19, B19 };
static const char* const mFrown[8]={ B19, B19, B19, ".....kkkkkkkkk.....", "....k.........k....", "...k...........k...", B19, B19 };
static const char* const mTongue[8]={ B19, B19, "....kkkkkkkkkkk....", ".....k.rrrrr.k.....", "......krrrrrk......", ".......kkkkk.......", B19, B19 };
static const char* const mFangs[8]={ B19, B19, "....kkkkkkkkkkk....", ".....ww.....ww.....", "......w.....w......", B19, B19, B19 };
static const char* const mCat[8]  ={ B19, B19, ".....k...k...k.....", "......k.k.k.k......", ".......k...k.......", B19, B19, B19 };
typedef struct { const char*name; u8 wc; const char* const*art; } Spr;   // wc = width in cells
#define NEYE 9
#define NMOUTH 9
#define NSPR (NEYE+NMOUTH)
static const Spr spr[NSPR]={ {"SLEEPY",1,aHalf},{"ROUND",1,aRound},{"HAPPY",1,aHappy},{"WIDE",1,aWide},{"ANGRY",1,aAngry},
                             {"CUTE",1,aCute},{"CAT",1,aCat},{"DOT",1,aDot},{"LASHES",1,aLash},
                             {"FLAT",2,mFlat},{"SMILE",2,mSmile},{"OH",2,mOh},{"GRIN",2,mGrin},{"SMIRK",2,mSmirk},
                             {"FROWN",2,mFrown},{"TONGUE",2,mTongue},{"FANGS",2,mFangs},{"KITTY",2,mCat} };
// Details drawn over the eyes (brows, glasses) and over the mouth (nose, cheeks), from the look: [style-1][row], art in the same grid.
static const char* const brArt[5][2]={ {"..hhhhh..",B9},{".hhhhhhh.",".hhhhhhh."},{".hhh.....","....hhh.."},{".....hhh.","..hhh...."},{"hhhhhhhhh","hhhhhhhhh"} };   // THIN THICK ANGRY SAD UNIBROW
static const char* const glArt[3][8]={ {B9,"..ggggg..",".g.....g.","gg.....gg",".g.....g.","..ggggg..",B9,B9},              // ROUND
                                       {B9,"ggggggggg","g.......g","g.......g","g.......g","ggggggggg",B9,B9},              // SQUARE
                                       {B9,B9,"ggggggggg",".lllllll.",".lllllll.","..lllll..",B9,B9} };                       // SHADES
static const char* const noArt[5][2]={ {"........sss........",B19},{".........ss........",".........kss......."},
                                       {".......sssss.......",".......s...s......."},{"........sss........","........k.k........"},
                                       {".......kkkkk.......","........kkk........"} };   // BUTTON POINTY WIDE PIG ANIMAL
static const char* const chArt[4][4]={ {".bbb...........bbb.",".bbb...........bbb.",B19,B19},{".s.s...........s.s.","..s.............s..",B19,B19},
                                       {B19,"kkk.............kkk",B19,"kkk.............kkk"},{"................r..","...............r...","..............r....",B19} };   // BLUSH FRECKLES WHISKERS SCAR
static int sty[2];                     // chosen style per kind: 0 = eye, 1 = mouth
#define SPRID(k) ((k)?NEYE+sty[1]:sty[0])
static inline int decSpr(u16 c){ return (c&7)|((c>>8)&0x78); }            // sprite+1: bits 0-2 and 11-14 of the code
static inline u16 decSprBits(int n){ return (u16)((n&7)|((n>>3)<<11)); }

// code (0 = none): bits 0-2 + 11-14 sprite+1, 3-5 cell column, 6-7 cell row (from top), 8-9 size-1, 10 mirrored
// face: 0 = left cube face (+Z seen from view 0), 1 = right cube face (+Z seen from view 3)
// The FACE sliders move and scale the art inside its footprint (eye size, spacing and height; mouth width and height), drawing a 2 px
// margin round it so a moved eye is not cut off at its own cell's edge.
static int decNose;   // 1: drawDeco draws only the nose (a raised nose is drawn again after the body, else the block above covers it)
static int decLook;   // 1: brows, glasses, nose, cheeks and the face sliders apply (a look-built creature)
__attribute__((noinline)) static void drawDeco(int sx,int sy,u16 code,int face,int tint){   // ROM: only the few face voxels call it
    int id=decSpr(code)-1; if(id<0||id>=NSPR) return;
    const Spr*sp=&spr[id]; int eye=id<NEYE;
    int ci=(code>>3)&7, cj=(code>>6)&3, sz=((code>>8)&3)+1, fl=(code>>10)&1, aw=10*sp->wc-1;   // aw = art width in chars
    int wp=CA*sp->wc*sz-1, hp=CC*sz-2;   // footprint size in px (scales with the voxel size)
    const u16*pal=face?dR:dL;
    int sh=0, sv=0, kx=64, ky=64, br=0, gl=0, no=0, ch=0, bsh=0, nsh=0;   // shift (px), scale (64 = 1x), details
    if(decLook){
        // every notch of a face slider moves the art by a pixel, or grows its footprint by two (per voxel size): no two notches look the same
        if(eye){ int e=(slideEffS(look[LK_EYESZ])+(stage==AG_BABY?1:0))*sz, gw; e=e>0?2*e:e<0?e-sz:0; gw=wp+e; int gh=hp+e; if(gw<1) gw=1; if(gh<1) gh=1; kx=64*gw/wp; ky=64*gh/hp;
                 sh=slideEff(look[LK_EYESP])*sz; if(fl) sh=-sh; sv=slideEff(look[LK_EYEHT])*sz;
                 br=look[LK_BROW]; gl=look[LK_GLASS]; bsh=slideEff(look[LK_BROWHT])*sz; }
        else { int gw=wp+(slideEffS(look[LK_MOUTHW])-(stage==AG_BABY?1:0))*sz*3; if(gw<1) gw=1; kx=64*gw/wp; sv=slideEff(look[LK_MOUTHHT])*sz; no=look[LK_NOSE]; ch=look[LK_CHEEK]; nsh=slideEff(look[LK_NOSEHT])*sz; }
    }
    int M=decLook?(eye?4:7):0, pc=wp/2, qc=hp/2;
    int lc0=(ci?-1:0)-M, lr0=(cj?-1:0)-M, lc1=CA-1+M, lr1=CC-2+M;
    for(int lr=lr0;lr<lr1;lr++){
        int Q=cj*CC+lr+sv; Q=qc+((Q-qc)*64)/ky;
        int Qb=Q+bsh, Qn=Q+nsh-sv;   // (the brows and nose may sit outside the art's own rows: they are checked on their own)
        if((Q<0||Q>=hp)&&(!br||Qb<0||Qb>=hp)&&(!no||Qn<0||Qn>=hp)) continue;
        int ay0=Q>=0&&Q<hp?(Q*8)/hp:9, ay1=Q>=0&&Q<hp?((Q+1)*8-1)/hp:9; if(ay0<9){ if(ay0>7) ay0=7; if(ay1>7) ay1=7; if(ay1<ay0) ay1=ay0; }   // the art rows this pixel covers   // the brows (BROW HEIGHT) and the nose (NOSE HEIGHT) move on their own
        int by0=Qb>=0&&Qb<hp?(Qb*8)/hp:9, by1=Qb>=0&&Qb<hp?((Qb+1)*8-1)/hp:9, ny0=Qn>=0&&Qn<hp?(Qn*8)/hp:9, ny1=Qn>=0&&Qn<hp?((Qn+1)*8-1)/hp:9;
        for(int lc=lc0;lc<lc1;lc++){
            int P=ci*CA+lc+sh; P=pc+((P-pc)*64)/kx; if(P<0||P>=wp) continue;
            int ax0=(P*aw)/wp, ax1=((P+1)*aw-1)/wp; if(ax0>aw-1) ax0=aw-1; if(ax1>aw-1) ax1=aw-1; if(ax1<ax0) ax1=ax0;
            // shrunk art: a pixel shows the art at its corner, or if that is clear any ink in the box it covers (else a one-row line, like the FLAT mouth, can fall
            // between the rows sampled and vanish)
            #define PICKR(A,rmax,Y0,Y1) ({ char t_=(Y0)<(rmax)?(A)[Y0][fl?aw-1-ax0:ax0]:'.'; for(int a_=(Y0);a_<=(Y1)&&a_<(rmax)&&t_=='.';a_++) for(int b_=ax0;b_<=ax1;b_++){ char u_=(A)[a_][fl?aw-1-b_:b_]; if(u_!='.'){ t_=u_; break; } } t_; })
            #define PICK(A,rmax) PICKR(A,rmax,ay0,ay1)
            char c=decNose?'.':PICK(sp->art,8);
            if(decNose){ if(!eye&&no) c=PICKR(noArt[no-1],2,ny0,ny1); }
            else if(eye){
                if(br){ char t=PICKR(brArt[br-1],2,by0,by1); if(t!='.') c=t; }
                if(gl){ char t=PICK(glArt[gl-1],8); if(t!='.') c=t; }
            } else if(c=='.'){
                if(no) c=PICKR(noArt[no-1],2,ny0,ny1);
                if(c=='.'&&ch) c=PICK(chArt[ch-1],4);
            }
            #undef PICK
            #undef PICKR
            if(c=='.') continue;
            static const char key[]="kwrsibghl"; int k=0; while(key[k]&&key[k]!=c) k++; if(!key[k]) k=0;
            u16 col=tint?(face?sR[8]:sL[8]):pal[k];
            if(!face) px(sx-CA+1+lc, sy+((1+lc)>>1)+1+lr, col);
            else      px(sx+1+lc,    sy+((CA-1-lc)>>1)+1+lr, col);
        }
    }
}

static int cx,cy,cz,part,size;
#define OXC 60
static int oycV=121;   // where the creator draws the creature's feet (the sprite bake moves it down: bakeInto)
#define OYC oycV
static int view=0;   // 0..3 = 90 degree turns
static int noGrid=0;   // sprite baking draws the character without the floor grid
static u8 bakeOn;      // bakeInto is drawing: only the capture window (the clip rectangle) is cleared and read
static void rotUW(int u,int w,int*ru,int*rw){
    switch(view){ case 0:*ru=u;*rw=w;break; case 1:*ru=-w;*rw=u;break; case 2:*ru=-u;*rw=-w;break; default:*ru=w;*rw=-u; }
}
// u,w = doubled grid coords relative to the build-space centre
static int headK, handK, liftK, liftL, liftT, liftTn, armK, stanceK, bakeCapH=99, bakeCapW=99, bakeCapT=99, bakeCapX=99, bakeCapL=99, bakeSh, bakeWk, strideK, neckK, exHip, exWst, exSho, exThi, exCal, exChe, exBel, exUAr, exFAr, exJaw, exHnd, exFt, bakeCapE=99, exMax;
#define EXC(v) ((v)>bakeCapE?bakeCapE:(v)<-bakeCapE?-bakeCapE:(v))   // NECK / HIP / WAIST / SHOULDER / THIGH / CALF extras, eased off for a sprite bake   // liftT: TORSO slider px per torso row (liftTn rows); armK, stanceK: ARMS and STANCE spread (px)
   // strideK: legs (shape 3) half a block forward / back, arms the other way   // HEIGHT slider: every one of the first liftL rows (the legs) is liftK px taller
static const signed char shpDraw[NSHAPE][4]={   // per body type, drawn: torso width, arm width, leg width (px added to the block's half width), leg lift (px per leg row)
    {0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},
    {2,1,1,0},{-1,-1,2,0},{-2,-1,-1,2},{3,2,2,0},{0,0,0,-1},{1,-1,-1,0},{2,3,1,0},{-2,-1,-1,-1},{1,0,0,0},{0,0,1,0},
    {1,0,0,0},{1,0,1,0},{-1,-1,-1,1},{1,1,0,0},{2,2,1,0},{0,-1,0,3},
    {3,1,1,0},{1,-1,0,-1},{-1,-1,-1,2},{1,1,0,0},{1,0,1,0},{2,2,2,0} };   // V-SHAPE CURVY RUNNER SOFT POWER LONG LEGS, then PUDGY TODDLER SPROUT SPORTY BELL STURDY
// the humanoid builds also shape the body rows themselves (px added on top of the sliders): hip row, waist (every torso row), shoulder row, thigh, calf, chest
static const signed char shpEx[NSHAPE][6]={
    {0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},{0},
    {-1,0,3,0,0,0},{3,0,1,0,0,2},{0,-1,0,-1,0,-1},{1,2,0,1,0,1},{1,0,3,2,1,2},{0,0,0,-1,-1,0},
    {1,1,0,0,0,1},{0},{0,-1,0,-1,0,0},{0,0,2,0,0,1},{3,0,-1,1,0,0},{0,1,1,1,1,0} };   // ... PUDGY TODDLER SPROUT SPORTY BELL STURDY
static void projC(int u,int w,int yy,int*ox,int*oy){
    int a,b; rotUW(u,w,&a,&b); *ox=OXC+(a-b)*CA/2; *oy=OYC+(a+b)*CB/2-yy*CC-liftK*(yy<liftL?yy:liftL)-liftT*(yy<liftL?0:yy-liftL<liftTn?yy-liftL:liftTn)-(yy>=liftL+liftTn?neckK:0);
}
// Wedge blocks (hair): the block's own diamond, with some top corners lowered by a full block height, so the top face slopes.
// shape 4..11: 4 edges (slope towards +x +z -x -z) and 4 corners (only the outer corner drops: ++ -+ -- +-). Slopes are given in grid space and
// turned with the view like everything else, so a wedge keeps pointing the same way as the creature spins.
static const u8 wMask[8]={10,12,5,3,8,4,1,2};   // bit k = grid corner (k&1 ? +x : -x, k&2 ? +z : -z)
IWRAM_THUMB static void wedgeCube(int sx,int sy,int ci,int shape,int f){
    int m=wMask[shape-4], o[4]={0,0,0,0};   // lowered corners as seen on screen: N, E, S, W
    for(int k=0;k<4;k++) if(m>>k&1){
        int a,b; rotUW((k&1)?1:-1,(k&2)?1:-1,&a,&b); int dx=a-b, dy=a+b;
        o[dx>0?1:dx<0?3:dy>0?2:0]=CC;
    }
    const u8*hhp=hhT[0]; int rr=CA;
    if(cubeDR){ static u8 hw2[CA+8]; rr+=cubeDR; if(rr<3) rr=3; if(rr>CA+6) rr=CA+6; for(int a=0;a<=rr;a++) hw2[a]=(u8)((rr/2)*(rr-a)/rr); hhp=hw2; }   // HEAD SIZE
    u16 T=sT[ci], L=sL[ci], R=sR[ci], eT=shade(T,9), eL=shade(L,9), eR=shade(R,9);
    int left=o[3]+o[2], right=o[1]+o[0];
    u16 S2=shade(T,left>right?13:left<right?11:12);   // the slope: a little darker than a flat top, tilted towards the light on the left
    for(int t=-rr;t<=rr;t++){
        int x=sx+t, hh=hhp[t<0?-t:t], k=t<0?t+rr:t, u0,u1,l0,l1;
        if(t<0){ u0=o[3];u1=o[0];l0=o[3];l1=o[2]; } else { u0=o[0];u1=o[1];l0=o[2];l1=o[1]; }
        int yu=sy-hh+u0+(u1-u0)*k/rr, yl=sy+hh+l0+(l1-l0)*k/rr, yb=sy+hh+CC-1;
        if(yl>yb) yl=yb;
        u16 sc=t<0?L:R, ec=t<0?eL:eR;
        if(yl<yb) vline(x,yl,yb,sc);
        if(yl>=yu) vline(x,yu,yl,S2);
        px(x,yu,eT);
        if(yl<yb-1){ px(x,yl+1,lite(sc,19)); } else px(x,yl,ec);
        if(yb>yl+2) px(x,yb-1,shade(sc,13));
        px(x,yb,ec);
        if(t==-rr||t==rr){ if(!(f&(t<0?16:32))) vline(x,yu,yb,ec); }
    }
}
static void moveView(int sx,int sz){   // screen-relative step -> grid step
    int gx,gz;
    switch(view){ case 0:gx=sx;gz=sz;break; case 1:gx=sz;gz=-sx;break; case 2:gx=-sx;gz=-sz;break; default:gx=-sz;gz=sx; }
    cx+=gx; cz+=gz;
}

static u8 vox[H][D][W], ghost[H][D][W] EWRAM_BSS;   // (the cursor preview is read once per voxel: EWRAM is fine)
static u16 dec[H][D][W] EWRAM_BSS, gdec[H][D][W] EWRAM_BSS;   // face sprites per voxel (+Z face) and their cursor preview
static int gAny;


// ---------- parts ----------
typedef struct { const char*name; u8 n,mirror,w,h,d; const u8 (*c)[4]; u8 dk; } Part;   // dk: 0 voxel part, 1 eye sprite, 2 mouth sprite
static const u8 cHead[][4]={{0,0,0,1},{1,0,0,1},{0,1,0,1},{1,1,0,1},{0,0,1,1},{1,0,1,1},{0,1,1,1},{1,1,1,1}};
static const u8 cTorso[][4]={{0,0,0,6},{1,0,0,6},{0,1,0,6},{1,1,0,6},{0,0,1,6},{1,0,1,6},{0,1,1,6},{1,1,1,6}};
static const u8 cArm[][4]={{0,0,0,1|(2<<4)},{0,1,0,1|(1<<4)},{0,2,0,6|(1<<4)}};   // hand, forearm, sleeve (high nibble = shape)
static const u8 cLeg[][4]={{0,0,0,7|(3<<4)},{0,1,0,7|(3<<4)},{0,2,0,7|(3<<4)}};
static const u8 cEar[][4]={{0,0,0,1},{0,1,0,1}};
static const u8 cHair[][4]={{0,0,0,5},{1,0,0,5},{0,0,1,5},{1,0,1,5},{0,1,0,5},{1,1,1,5}};
#define NPARTS 8
static const Part parts[NPARTS]={
 {"HEAD",8,0,2,2,2,cHead,0},{"TORSO",8,0,2,2,2,cTorso,0},{"ARM",3,1,1,3,1,cArm,0},{"LEG",3,1,1,3,1,cLeg,0},
 {"EYE",0,1,1,1,1,0,1},{"MOUTH",0,0,2,1,1,0,2},{"EAR",2,1,1,2,1,cEar,0},{"HAIR",6,0,2,2,2,cHair,0}};

// ---------- state ----------


static void apply(int x0,int y0,int z0,int flip,int act,int pi,int s){
    const Part*p=&parts[pi];
    for(int i=0;i<p->n;i++)
      for(int a=0;a<s;a++)for(int b=0;b<s;b++)for(int c=0;c<s;c++){
        int X=p->c[i][0]*s+a; if(flip) X=p->w*s-1-X;
        int x=x0+X, y=y0+p->c[i][1]*s+b, z=z0+p->c[i][2]*s+c;
        if(x<BX0||x>=BX0+BXW||y<0||y>=BXH||z<0||z>=BXD) continue;   // outside this life stage's build box
        if(act==0) ghost[y][z][x]=1; else if(act==1) vox[y][z][x]=p->c[i][3]; else { vox[y][z][x]=0; dec[y][z][x]=0; }
      }
}
// Sprites snap to the front-most solid voxel in the cursor's column, so Z does not matter.
static int snapZ(int x,int y){ for(int z=D-1;z>=0;z--) if(vox[y][z][x]) return z; return -1; }
static void applyDeco(int x0,int y0,int act,int pi,int s,int flip){
    int id=SPRID(parts[pi].dk-1), fw=spr[id].wc*s;   // footprint: fw cells wide, s cells tall
    for(int j=0;j<s;j++)for(int i=0;i<fw;i++){
        int x=x0+i, y=y0+s-1-j;
        if(x<BX0||x>=BX0+BXW||y<0||y>=BXH) continue;
        if(act==2){ for(int z=0;z<D;z++) dec[y][z][x]=0; continue; }
        int z=snapZ(x,y); if(z<0) continue;
        u16 code=(u16)(decSprBits(id+1)|(i<<3)|(j<<6)|((s-1)<<8)|(flip<<10));
        if(act==0){ gdec[y][z][x]=code; gAny=1; } else dec[y][z][x]=code;
    }
}
static void doDeco(int act,int pi,int s,int x,int y){
    int fw=spr[SPRID(parts[pi].dk-1)].wc*s;
    applyDeco(x,y,act,pi,s,0);
    if(parts[pi].mirror){ int mx=W-x-fw; if(mx!=x) applyDeco(mx,y,act,pi,s,1); }
}
static void doPart(int act,int pi,int s,int x,int y,int z){
    if(parts[pi].dk){ doDeco(act,pi,s,x,y); return; }
    apply(x,y,z,0,act,pi,s);
    if(parts[pi].mirror){ int mx=W-x-parts[pi].w*s; if(mx!=x) apply(mx,y,z,1,act,pi,s); }
}
static void clampCursor(void){
    if(part>=NPARTS) return;   // "GO LIVE LIFE!" entry has no cursor
    const Part*p=&parts[part];
    if(size>BXS) size=BXS;
    int mx=BX0+BXW-p->w*size, my=BXH-p->h*size, mz=BXD-p->d*size;
    if(mx<BX0)mx=BX0; if(my<0)my=0; if(mz<0)mz=0;
    if(cx>mx)cx=mx; if(cy>my)cy=my; if(cz>mz)cz=mz;
    if(cx<BX0)cx=BX0; if(cy<0)cy=0; if(cz<0)cz=0;
}
// ---------- the look -> blocks ----------
// The creature creator never asks for blocks: it asks for a look (shape, ears, hair style...) and this turns it into the 6x4x8 model.
// Skin, hair, top and bottom colours are only palette slots (setColors), eye and mouth styles only re-skin the face sprites (restyle),
// so those never touch the blocks. Shape, ears and hair style rebuild the whole model (the pickers ask first if you built by hand).
static int custom;   // 1 once the block builder has placed or erased something by hand
__attribute__((noinline)) static void bodyPlan(int*L,int*T,int*hs){   // legs showing, torso blocks showing, head scale: the body that fits this stage's box
    int sh=look[LK_SHAPE]; *hs=(sh==2)?2:1;
    int l=stLegs[stage], t=2, ht=2*(*hs);
    if(stage==AG_BABY){ l=1; t=1; }   // BABY: a big head on a tiny body: stubby legs, ONE block of torso (the head is half the height)
    if((sh==SH_BIGHEAD||sh==SH_STUBBY||sh==SH_STOCKY||sh==SH_PETITE||sh==SH_TODDLER)&&l>0) l--;   // BIG HEAD, STUBBY, STOCKY and PETITE: legs one block shorter
    int room=BXH-(*hs==1);                        // a normal head keeps a layer free above it for the hair (else the hair eats its top row, eyes and all)
    while(l+t+ht>room&&l>0) l--;                          // too tall for the box: shorten the legs, then the torso
    if(l+t+ht>room) t=1;
    *L=l; *T=t;
}
static void headBox(int*hx,int*hy,int*hz,int*hs){   // where the head sits (and how many blocks per head cell) for each body shape
    int L,T; bodyPlan(&L,&T,hs);
    *hx=BX0+(BXW-2*(*hs))/2; *hy=L+T; *hz=(BXD-2*(*hs))/2;
    if(look[LK_SHAPE]==SH_HUNCH&&*hz+2*(*hs)<BXD) (*hz)++;   // HUNCHED: the head pushed forward of the shoulders
}
static void hairW(int x,int y,int z,int xp,int xm,int zp,int zm){   // one hair block; the flags say which sides slope away (grid space)
    if(x<BX0||x>=BX0+BXW||y<0||y>=BXH||z<0||z>=BXD) return;
    int nx=xp+xm, nz=zp+zm, m=0;
    if(nx+nz==1) m=xp?10:zp?12:xm?5:3;
    else if(nx==1&&nz==1) m=1<<((xp?1:0)+(zp?2:0));
    for(int i=0;i<8;i++) if(wMask[i]==m&&m){ vox[y][z][x]=(u8)(5|((4+i)<<4)); return; }
    vox[y][z][x]=5;
}
static void vb(int x,int y,int z,int v){ if(x<BX0||x>=BX0+BXW||y<0||y>=BXH||z<0||z>=BXD) return; vox[y][z][x]=(u8)v; }
// Spore parts (the PARTS tab): a TAIL behind the hips, HORNS on the sides of the head, SPIKES or WINGS on the back. The back of the
// creature is z=0 (faces look towards +z). vw() is one block with a wedge top that slopes away on the sides flagged (grid space).
static void vw(int x,int y,int z,int col,int xp,int xm,int zp,int zm){
    if(x<BX0||x>=BX0+BXW||y<0||y>=BXH||z<0||z>=BXD) return;
    int nx=xp+xm, nz=zp+zm, m=0;
    if(nx+nz==1) m=xp?10:zp?12:xm?5:3;
    else if(nx==1&&nz==1) m=1<<((xp?1:0)+(zp?2:0));
    for(int i=0;i<8;i++) if(wMask[i]==m&&m){ vox[y][z][x]=(u8)(col|((4+i)<<4)); return; }
    vox[y][z][x]=(u8)col;
}
static void sporeParts(int tx,int ty,int hx,int hy,int hz,int hw,int hh,int top){   // tx,ty = torso left column, bottom row; hx..hh = the head; top = the hair layer
    int tail=look[LK_TAIL], horns=look[LK_HORNS], back=look[LK_BACK];
    // the tail and the horns are sprites now (drawTail / drawHorns): their length and size are sliders, and they cost no blocks
    if(back==1){                                                 // SPIKES: a ridge of wedges down the back, in the hair colour
        for(int x=tx;x<tx+2;x++){ vw(x,ty+1,0,5,x==tx+1,x==tx,0,1); if(!tail) vw(x,ty,0,5,x==tx+1,x==tx,0,1); }
        if(look[LK_HSTYLE]!=2&&hz>0) for(int x=hx;x<hx+hw;x++) vw(x,hy+hh-1,hz-1,5,x==hx+hw-1,x==hx,0,1);   // and up the back of the head (LONG hair is there already)
    }   // (WINGS are a sprite now: drawWings)
    (void)horns;
}
static void bxSync(void){ bxSt=(u8)(mcBig()?AG_ADULT:stage); bxBig=(u8)mcBig(); bxLift=(u8)(bxBig?18:mcDbl()?10:6); }   // (MASTER CONTROLLER: read once, not in the drawing code)
static void buildLook(void){
    bxSync();
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ vox[y][z][x]=0; dec[y][z][x]=0; }
    sty[0]=look[LK_EYES]; sty[1]=look[LK_MOUTH];
    int L,T,hs0; bodyPlan(&L,&T,&hs0);
    int tx=BX0+(BXW-2)/2, ty=L+T-2;                      // torso: left column and bottom row (the bottom row is clipped when T is 1)
    int sh=look[LK_SHAPE];
    if(stage==AG_BABY){                                          // BABY: stubby legs under one block of torso, little arms, and the big head above
        if(L>0) doPart(1,3,1,tx,L-3,1);                           // the legs (one block: the rows below the floor are cut off)
        for(int z=1;z<3;z++)for(int x=tx;x<tx+2;x++) vb(x,L,z,6);
        int sl=(look[LK_TOPSTY]==1||look[LK_TOPSTY]==3)?6:1;       // sleeves only for LONG SLEEVE and HOODIE
        for(int x=tx-1;x<tx+3;x+=3){ vb(x,L,1,sl|(1<<4)); if(L>0) vb(x,L-1,1,1|(2<<4)); }   // an arm hangs beside the body, with a little hand
    } else if(sh==1&&stage==AG_ADULT){                                  // BROAD: torso and legs two blocks wider each side
        doPart(1,3,1,1,0,1); doPart(1,3,1,2,0,1);
        doPart(1,1,1,1,3,1); doPart(1,1,1,3,3,1); doPart(1,2,1,0,2,1);
    } else if(sh==4&&stage>=AG_TEEN){                            // SLIM: a one block deep torso
        doPart(1,3,1,tx,L-3,1); doPart(1,2,1,tx-1,ty-1,1);
        for(int y=ty;y<ty+2;y++)for(int x=tx;x<tx+2;x++) vb(x,y,1,6);
    } else if(sh==5&&stage>=AG_TEEN){                            // ATHLETIC: wide shoulders, narrow waist (the arms hang clear of the waist)
        doPart(1,3,1,tx,L-3,1); doPart(1,2,1,tx-1,ty-2,2);   // the arms hang under the front of the wide chest, beside the waist (long arms)
        for(int z=1;z<3;z++){ for(int x=tx;x<tx+2;x++) vb(x,ty,z,6); for(int x=tx-1;x<tx+3;x++) vb(x,ty+1,z,6); }
    } else {                                                     // AVERAGE, BIG HEAD, STUBBY, TALL and the rest: legs (clipped to the stage), torso, arms
        doPart(1,3,1,tx,L-3,1); doPart(1,1,1,tx,ty,1); doPart(1,2,1,tx-1,ty-1,1);
    }
    {   // the newer body types: their own blocks on top of the standard body (the widths and heights are drawn: see shpDraw)
        int yb=L, yt=L+T-1, fz=3;                                 // torso rows yb..yt, it is two deep (z 1..2): fz is the row in front of it, z 0 behind
        if(sh==SH_CHUBBY) for(int x=tx;x<tx+2;x++) vw(x,yb,fz,6,0,0,1,0);                               // a soft belly
        if(sh==SH_POT) for(int x=tx;x<tx+2;x++){ vb(x,yb,fz,6); if(yt>yb) vw(x,yt,fz,6,0,0,1,0); }    // a round potbelly, sloping up to the chest
        if(sh==SH_BARREL) for(int x=tx;x<tx+2;x++){ for(int y=yb;y<=yt;y++) vb(x,y,fz,6); vw(x,yt,0,6,0,0,0,1); }   // a deep barrel chest
        if(sh==SH_HUNCH) for(int x=tx;x<tx+2;x++) vw(x,yt,0,6,0,0,0,1);                                 // a hump behind the shoulders (the head hangs forward)
        if(sh==SH_PEAR) for(int x=tx-1;x<tx+3;x+=3) if(!vox[yb][2][x]) vw(x,yb,2,7,x>tx,x<tx,1,0);       // hips flaring out in front of the hands
        if(sh==SH_LANKY) for(int y=1;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ u8 v=vox[y][z][x];  // long arms: the hand a block lower
            if((v&15)==1&&(v>>4)==2&&!vox[y-1][z][x]){ vox[y-1][z][x]=v; vox[y][z][x]=(u8)(1|(1<<4)); } }
        if(sh==SH_DIGI&&L>0) for(int x=tx;x<tx+2;x++){                                                  // DIGITIGRADE: animal legs, hocks back, paws forward
            if(L>=2&&vox[0][1][x]&&!vox[0][2][x]){ vox[0][2][x]=vox[0][1][x]; vox[0][1][x]=0; }
            if(L>=3&&vox[1][1][x]&&!vox[1][0][x]){ vox[1][0][x]=vox[1][1][x]; vox[1][1][x]=0; }
            int pz=L>=2?3:2; if(!vox[0][pz][x]) vw(x,0,pz,1,0,0,1,0); }                                // the paw: toes in the skin (fur) colour
    }
    int hx,hy,hz,hs; headBox(&hx,&hy,&hz,&hs);
    int hw=2*hs, hh=2*hs, hd=2*hs;
    doPart(1,0,hs,hx,hy,hz);                                     // head
    // ears are sprites now (drawEars in drawScene), not blocks
    int st=look[LK_HSTYLE], top=(BXH-(hy+hh)>=1)?hy+hh:hy+hh-1;    // hair: a cap on the head, or in place of its top layer when the head touches the ceiling
    if(st!=3){
        // every style starts with the same dome: the outer edges of the cap are wedges that slope away, down to the head's top
        for(int z=hz;z<hz+hd;z++)for(int x=hx;x<hx+hw;x++) hairW(x,top,z,x==hx+hw-1,x==hx,z==hz+hd-1,z==hz);
        // BOWL: full blocks down both sides (the dome slopes down onto them, so the profile stays smooth)
        if(st==1) for(int z=hz;z<hz+hd;z++)for(int y=top-1;y>=top-2&&y>=0;y--){ hairW(hx-1,y,z,0,0,0,0); hairW(hx+hw,y,z,0,0,0,0); }
        // LONG: full blocks down the back, from the dome to below the neck
        if(st==2){ int z0=hz>0?hz-1:hz; for(int x=hx;x<hx+hw;x++)for(int y=hy-1;y<top;y++) hairW(x,y,z0,0,0,0,0); }
        int zb=hz>0?hz-1:-1;   // the row behind the head (-1: none, the big head fills the box)
        if(st==4){ for(int z=hz;z<hz+hd;z++){ vw(hx-1,top-1,z,5,0,1,0,0); vw(hx+hw,top-1,z,5,1,0,0,0); } if(zb>=0) for(int x=hx;x<hx+hw;x++) vw(x,top-1,zb,5,0,0,0,1); }   // SPIKY: tufts out of every side
        if(st==5){ for(int z=hz;z<hz+hd;z++)for(int y=top-1;y>=top-2&&y>=0;y--){ hairW(hx-1,y,z,0,y==top-1,0,0); hairW(hx+hw,y,z,y==top-1,0,0,0); }   // AFRO: big and round
                   if(zb>=0) for(int x=hx-1;x<hx+hw+1;x++)for(int y=top-1;y>=top-2&&y>=0;y--) hairW(x,y,zb,x==hx+hw,x==hx-1,0,y==top-1); }
        if(st==6){ for(int z=hz;z<hz+hd;z++)for(int x=hx;x<hx+hw;x++) vb(x,top,z,5); }                                                        // FLAT TOP: square, no slopes
        if(st==7){ for(int y=top-1;y>=hy-1&&y>=0;y--) hairW(hx+hw,y,hz,0,0,0,0); vw(hx+hw,hy-2,hz,5,0,0,0,0); }                               // SIDE TAIL: down one side
        if(st==8&&zb>=0){ for(int x=hx;x<hx+hw;x++) vw(x,top-1,zb,5,x==hx+hw-1,x==hx,0,1); }                                                   // BUN: a knot at the back
    }
    {   // hats (in a colour slot the creature already has: top, bottom, white, black, red or gold)
        static const u8 hatSlot[6]={6,7,2,3,4,8}; int hat=look[LK_HAT], hc=hatSlot[look[LK_HATCOL]%6];
        if(hat==1){ for(int z=hz;z<hz+hd;z++)for(int x=hx;x<hx+hw;x++) vb(x,top,z,hc); for(int x=hx;x<hx+hw;x++) vw(x,top,hz+hd,hc,0,0,1,0); }   // CAP: a flat crown and a peak at the front
        if(hat==2){ for(int z=hz;z<hz+hd;z++)for(int x=hx;x<hx+hw;x++) vw(x,top,z,hc,x==hx+hw-1,x==hx,z==hz+hd-1,z==hz); }                      // BEANIE: a soft dome
        if(hat==3){ for(int x=hx;x<hx+hw;x++){ vb(x,top,hz+hd-1,hc); if(hz>0) vb(x,top,hz,hc); } }                                              // BAND: across the hair
        if(hat==4){ for(int z=hz;z<hz+hd;z++)for(int x=hx;x<hx+hw;x++) vb(x,top,z,hc); }                                                         // FEZ: a tall square cap
        if(hat==5){ for(int z=hz;z<hz+hd;z++){ for(int x=hx;x<hx+hw;x++) vw(x,top,z,hc,x==hx+hw-1,x==hx,0,z==hz); vb(hx-1,top-1,z,hc); vb(hx+hw,top-1,z,hc); } }   // HELMET: dome and sides
    }
    if(look[LK_BEARD]){   // beards grow ON the face, flush: the jaw (the front of the head's bottom layer) turns hair-coloured, the mouth sits on it;
        int zf=hz+hd-1;     // LONG also covers the top of the chest under the chin (no block sticks out in front of the face any more)
        for(int x=hx;x<hx+hw;x++){ u8 v=vox[hy][zf][x]; if(v) vox[hy][zf][x]=(u8)((v&0xF0)|5);
            if(look[LK_BEARD]==2&&hy>0){ u8 c=vox[hy-1][zf][x];
                if(c&&((c&15)==6||(c&15)==1)) vox[hy-1][zf][x]=(u8)((c&0xF0)|5);
                else if(!c&&zf+1<BXD) vw(x,hy-1,zf,5,0,0,1,0); } } }
    {   // FURRY: animal ears out of the top corners of the head (fur = the hair colour), a muzzle on the lower face, a fur tail
        int fe=look[LK_FEARS], mz=look[LK_MUZZLE], zb=hz, zf=hz+hd-1;
        for(int sd=0;sd<2;sd++){ int x=sd?hx+hw:hx-1, xp=sd, xm=!sd;   // just outside the head, at the hair line, at the back
            if(fe==1) vw(x,top,zb,5,xp,xm,0,1);                                                   // CAT: a point, sloping out and back
            if(fe==2){ vw(x,top,zb,5,xp,xm,0,1); if(top>0) vw(x,top-1,zb,5,0,0,0,0); vw(x,top,zb+1<zf+1?zb+1:zb,5,xp,xm,0,0); }   // FOX: bigger, deeper
            if(fe==3){ for(int y=top-1;y<=top;y++) if(y>=0) vw(x,y,zb,5,0,0,0,y==top); }          // BUNNY: tall and upright
            if(fe==4) vw(x,top,zb,5,0,0,0,1); }                                                    // BEAR: round
        if(mz&&zf+1<BXD) for(int x=hx;x<hx+hw;x++){                                         // the muzzle: the mouth goes onto its front
            if(mz==1) vw(x,hy,zf+1,1,0,0,1,0);                                                      // SNOUT: sloping down to the nose
            if(mz==2) vb(x,hy,zf+1,1);                                                              // MUZZLE: square
            if(mz==3) vw(x,hy,zf+1,8,x==hx+hw-1,x==hx,1,0); }                                       // BEAK: gold, pointed
        int ft=look[LK_FTAIL], yb=L;
        if(ft==1) for(int x=tx;x<tx+2;x++){ vb(x,yb,0,5); if(yb>0) vw(x,yb-1,0,2,x==tx+1,x==tx,0,1); }   // FOX: bushy, with a white tip
        if(ft==2){ vb(tx,yb,0,5); if(yb+1<BXH) vw(tx,yb+1,0,5,0,0,0,1); }                 // CAT: thin, curling up
        if(ft==3) vw(tx+1,yb,0,2,1,1,0,1);                                                          // BUNNY: a white puff
    }
    {   // clothes: the arms are the columns with a hand (skin) at the row below the torso
        int ts=look[LK_TOPSTY], bs=look[LK_BOTSTY];
        if(stage<AG_ADULT){ if(ts>=4) ts=0; if(bs>=3) bs=0; }   // whatever a save or a look says: only ADULT and ELDER are ever drawn bare
        if(ts==4) for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++) if((vox[y][z][x]&15)==6) vox[y][z][x]=(u8)((vox[y][z][x]&0xF0)|1);   // BARE top: the top colour is skin (arms and hands already are)
        for(int x=0;x<W;x++)for(int z=0;z<D;z++){ if(ty-1<0||(vox[ty-1][z][x]&15)!=1) continue;
            if(ts==1||ts==3){ if((vox[ty][z][x]&15)==1) vox[ty][z][x]=(u8)((vox[ty][z][x]&0xF0)|6); }        // LONG SLEEVE / HOODIE: forearms in the top colour
            if(ts==2&&ty+1<H&&(vox[ty+1][z][x]&15)==6) vox[ty+1][z][x]=(u8)((vox[ty+1][z][x]&0xF0)|1); }      // TANK: bare shoulders
        if(ts==3&&hz>0&&look[LK_HSTYLE]!=2){ for(int x=hx;x<hx+hw;x++)for(int y=hy;y<top;y++) vb(x,y,hz-1,6); }   // HOODIE: the hood hangs behind the head
        for(int y=0;y<L&&y<ty;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ u8 v=vox[y][z][x]; if((v&15)!=7) continue;
            if(bs==1&&y>0&&y<L-1) vox[y][z][x]=(u8)((v&0xF0)|1);                                                   // SHORTS: bare shins
            if(bs==3&&!(y==0&&look[LK_SHOE])) vox[y][z][x]=(u8)((v&0xF0)|1);                                       // BARE bottom: skin from the hips down (shoes, if any, stay on)
            if(y==0&&look[LK_SHOE]){ static const u8 shoeSlot[6]={7,2,3,4,8,6}; vox[y][z][x]=(u8)((v&0xF0)|shoeSlot[look[LK_SHOE]%6]); } }   // SHOES
        if(bs==2&&L>0){ int yk=L-1; for(int z=1;z<3&&z<D;z++){ vw(tx-1,yk,z,7,0,1,0,0); vw(tx+2,yk,z,7,1,0,0,0); } }                // SKIRT: flares out at the hips
    }
    sporeParts(tx,ty,hx,hy,hz,hw,hh,top);                               // tail, horns, spikes or wings (before the face: sprites snap to the front block)
    {   // HANDS: CLAWS (an ivory talon pointing forward out of each hand), PINCERS (a red claw in front of and under each hand) or BLADES (a white blade forward and down)
        int cl=look[LK_CLAWS];
        if(cl) for(int y=1;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ u8 v=vox[y][z][x]; if((v&15)!=1||(v>>4)!=2) continue;   // a hand block
            if(cl==1){ if(z+1<D&&!vox[y][z+1][x]) vw(x,y,z+1,8,0,0,1,0); else if(!vox[y-1][z][x]) vw(x,y-1,z,8,0,0,1,0); }
            else if(cl==2){ if(z+1<D&&!vox[y][z+1][x]) vw(x,y,z+1,4,0,0,1,0); if(z+1<D&&!vox[y-1][z+1][x]) vw(x,y-1,z+1,4,0,0,1,0); }
            else { if(z+1<D&&!vox[y][z+1][x]) vw(x,y,z+1,2,0,0,1,0); if(!vox[y-1][z][x]) vw(x,y-1,z,2,0,0,1,0); } }   // BLADES: a white blade out of the front AND under each hand
    }
    {   // PATTERN: Spore-style body paint over the skin (and the shirt for stripes and a belly), in a colour slot the creature has
        static const u8 pcs[6]={5,4,8,2,3,7}; int pt=look[LK_PATTERN], pc=pcs[look[LK_PATCOL]%6];
        if(pt) for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ u8 v=vox[y][z][x], c=v&15; if(c!=1&&c!=6) continue;
            int on=0;
            if(pt==1) on=(y&1)&&(c==1||y<hy);                                    // STRIPES: every other layer
            else if(pt==2) on=((x*7+y*13+z*5)%4==0);                             // SPOTS (skin and shirt)
            else if(pt==3) on=(z==D-1||!vox[y][z+1][x])&&y<hy&&y>=ty;           // BELLY: the front of the torso
            else if(pt==4) on=((x+y+z)%3==0)&&(c==1||y<hy);                    // TIGER: diagonal bands
            else if(pt==5) on=c==1&&(((v>>4)==2)||y==0||(y<L&&(v>>4)==3));       // SOCKS: the hands and the feet (paws)
            else on=0;
            if(on&&!(y>=hy&&y<hy+hh&&(z==hz+hd-1))) vox[y][z][x]=(u8)((v&0xF0)|pc); }   // (never the face itself)
        if(pt==6) for(int x=hx;x<hx+hw;x++){ int y=hy+hh-1-(hs>1), z=hz+hd-1; u8 v=vox[y][z][x]; if((v&15)==1) vox[y][z][x]=(u8)((v&0xF0)|pc); }   // MASK: a bandit band across the eyes
    }
    if(hs==1){ doPart(1,4,1,hx,hy+1,0); doPart(1,5,1,hx,hy,0); }          // eyes on the top row of the face, mouth on the bottom row
    else     { doPart(1,4,2,hx,hy+1,0); doPart(1,5,1,hx+1,hy,0); }        // big head: big eyes, mouth still one block
    custom=0;
}
static void restyle(int kind){   // change the style of every eye (0) or mouth (1) sprite already on the creature, built by hand or not
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){
        u16 c=dec[y][z][x]; if(!c) continue;
        int id=decSpr(c)-1; if((id>=NEYE)==kind) dec[y][z][x]=(u16)((c&~0x7807)|decSprBits(SPRID(kind)+1)); }
}
// ---- changing the stage ----
static int maskPick(int mask,int v,int n){ for(int i=0;i<n;i++){ int j=(v+i)%n; if(mask>>j&1) return j; } return 0; }   // the option at or after v that is allowed
static void fixLook(void){   // pull every choice into what this stage offers
    look[LK_SHAPE]=(u8)maskPick(shapeMask(),look[LK_SHAPE],NSHAPE);
    if(stage<AG_ADULT){ if(look[LK_TOPSTY]>=4) look[LK_TOPSTY]=0; if(look[LK_BOTSTY]>=3) look[LK_BOTSTY]=0; }   // BARE is for adults and elders only
    look[LK_EARS]=(u8)maskPick(stMaskEars[stage],look[LK_EARS],3);
    look[LK_HSTYLE]=(u8)maskPick(stMaskHair[stage],look[LK_HSTYLE],NHAIR);
    static const u8 sw[3]={LK_HCOL,LK_TOP,LK_BOT};   // (every skin colour is on offer at every age)
    for(int i=0;i<3;i++) if(look[sw[i]]>=stSwatches[stage]) look[sw[i]]=(u8)(look[sw[i]]%stSwatches[stage]);
}
static void clipCustom(void){   // hand-built blocks outside the stage's box are cut off
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++)
        if(x<BX0||x>=BX0+BXW||y>=BXH||z>=BXD){ vox[y][z][x]=0; dec[y][z][x]=0; }
}
static void ageSave(void);
static void setStage(int n){   // new stage: the look is fitted to it; a look-built creature is rebuilt, hand-built blocks stay (cut to size)
    stage=(u8)n; ageDays=0; fixLook(); if(size>BXS) size=BXS;
    if(n==AG_BABY&&!look[LK_CHEEK]) look[LK_CHEEK]=1;   // a new baby starts with rosy cheeks (pick NONE again if you like)
    if(custom) clipCustom(); else buildLook();
    setColors(); ageSave();
}
#define AGE_OFF 5008   // SRAM: 'A' 'G', stage, days in the stage, checksum (the creature itself is only kept in room slots, so its growth is remembered here)
static void ageSave(void){ volatile u8*m=SRAM_BASE+AGE_OFF; m[0]='A'; m[1]='G'; m[2]=stage; m[3]=ageDays; m[4]=(u8)(0x47+stage+ageDays); }
static void ageLoad(void){   // at power on, after the person came back from its slot: the grown-up stage wins over the stage the slot was saved at
    volatile u8*m=SRAM_BASE+AGE_OFF;
    if(m[0]!='A'||m[1]!='G'||m[2]>=AG_N||m[4]!=(u8)(0x47+m[2]+m[3])) return;
    ageDays=m[3];
    if(m[2]!=stage){ stage=m[2]; fixLook(); if(custom) clipCustom(); else buildLook(); setColors(); }
}
static int gGrow;   // set at midnight when the creature has lived long enough in its stage: the life loop grows it (setStage) and re-bakes its sprites
static inline int ojob(void){ return xo[XO_JOB]&&stage>=AG_TEEN&&stage<AG_ELDER; }   // career: shifts, quota and bills for teens and adults (an elder is retired)
static void starter(void){
    cx=2;cy=0;cz=1;part=0;size=1;
    buildLook();   // the starter creature is look 0 everywhere: legs, torso, arms, head, eyes, mouth and hair
}

// ---------- persona: aspiration, lifetime want and personality (the creator's ASPIRE tab) ----------
// Like Create-A-Sim in The Sims 2: pick what the creature dreams of (its ASPIRATION decides which wants and fears it rolls), a LIFETIME WANT
// for that aspiration, and a personality of five traits 0..10 that share 25 points (a star SIGN deals them out; moving a trait finds the sign
// that fits best). Babies and children always aspire to GROW UP; the chosen aspiration starts when the creature becomes a teen.
// sims.h reads all of it; it is saved with the person (room slot person format 4) and in SRAM at PERS_OFF.
enum { AS_FORTUNE, AS_KNOW, AS_POP, AS_PLEAS, AS_HOME, AS_GROW, AS_N };
#define AS_PICK 5   // the first five can be picked; GROW UP comes with being young
static const char* const aspNm[AS_N]={"FORTUNE","KNOWLEDGE","POPULARITY","PLEASURE","HOME","GROW UP"};
enum { TR_NEAT, TR_OUT, TR_ACT, TR_PLAY, TR_NICE, TR_N };
#define TR_POINTS 25   // personality points to share out
static const char* const trNm[TR_N]={"NEAT","OUTGOING","ACTIVE","PLAYFUL","NICE"};
static const char* const signNm[12]={"ARIES","TAURUS","GEMINI","CANCER","LEO","VIRGO","LIBRA","SCORPIO","SAGITTARIUS","CAPRICORN","AQUARIUS","PISCES"};
static const u8 signTr[12][TR_N]={ {5,8,6,3,3},{5,5,3,8,4},{4,7,8,3,3},{6,3,6,4,6},{4,10,4,4,3},{9,2,6,3,5},
                                   {2,8,2,6,7},{6,5,8,3,3},{2,3,9,7,4},{7,4,8,2,4},{4,4,4,7,6},{5,3,4,4,9} };   // each adds up to 25
static u8 pAsp=AS_FORTUNE, pLtw=0, pTr[TR_N]={5,8,6,3,3};   // starts as an ARIES who wants FORTUNE
static inline int aspNow(void){ return stage<AG_TEEN?AS_GROW:pAsp; }
static inline int trOf(int t){ return pTr[t]; }
static int trLeft(void){ int s=0; for(int i=0;i<TR_N;i++) s+=pTr[i]; return TR_POINTS-s; }
static int signOf(void){   // the sign whose traits are nearest to the creature's
    int best=0, bd=999;
    for(int s=0;s<12;s++){ int d=0; for(int i=0;i<TR_N;i++){ int e=pTr[i]-signTr[s][i]; d+=e<0?-e:e; } if(d<bd){ bd=d; best=s; } }
    return best;
}
static void setSign(int s){ for(int i=0;i<TR_N;i++) pTr[i]=signTr[s][i]; }
static int persValid(int asp,int ltw,const u8*tr){
    int s=0; if(asp>=AS_PICK||ltw>=2) return 0;
    for(int i=0;i<TR_N;i++){ if(tr[i]>10) return 0; s+=tr[i]; }
    return s<=TR_POINTS;
}
// ---- Spore: DNA, parts and abilities ----
// Like the Spore creature editor, the body is not just a look: every part changes what the creature can do. Five ABILITIES 0..5
// (SPEED, JUMP, GRIP, STYLE, STAMINA) come from the shape, face, hair and parts, and four parts carry a POWER:
//   LONG TAIL = BALANCE (lands spins further off straight), HORNS = CHARGE (skating into a wall does not hurt),
//   SPIKES = ARMOUR (falls and bails hurt less), WINGS = GLIDE (hold R in the air to float down).
// DNA points are earned by living (wants met, skill, promotions, birthdays, the lifetime want) and unlock the bigger parts.
static int lchill;   // steps left CHILLED OUT (a puff on the water pipe, or PUFF PUFF PASS): +1 STYLE, the munchies
enum { AB_SPEED, AB_JUMP, AB_GRIP, AB_STYLE, AB_STAMINA, AB_N };
static const char* const abNm[AB_N]={"SPEED","JUMP","GRIP","STYLE","STAMINA"};
enum { PW_BALANCE=1, PW_CHARGE=2, PW_ARMOUR=4, PW_GLIDE=8, PW_CLAMP=16, PW_SENSE=32, PW_SLASH=64 };
static const char* const tailNm[3]={"NONE","STUB","LONG"};
static const char* const hornNm[3]={"NONE","NUBS","HORNS"};
static const char* const backNm[3]={"NONE","SPIKES","WINGS"};
static const char* const clawNm[4]={"NONE","CLAWS","PINCERS","BLADES"};
static const char* const antNm[3]={"NONE","FEELERS","EYE STALKS"};
static const char* const patNm[7]={"NONE","STRIPES","SPOTS","BELLY","TIGER","SOCKS","MASK"};
static const char* const patColNm[6]={"AS THE HAIR","RED","GOLD","WHITE","BLACK","AS THE BOTTOM"};
#define NPART 5   // the parts DNA can buy: TAIL, HORNS, BACK, CLAWS, ANTENNAE (unlock bits part*3+option, 15 of pUnl's 16)
static const short partCost[NPART][4]={{0,0,60,0},{0,0,60,0},{0,40,120,0},{0,30,90,110},{0,0,70,0}};   // DNA to unlock each option
static const signed char abShape[NSHAPE][AB_N]={   // ability changes (SPEED JUMP GRIP STYLE STAMINA) per body type, in half bars
    {0,0,0,0,0},{-1,-1,1,0,2},{-1,0,0,2,0},{0,-1,2,0,1},{1,1,0,0,-1},{1,1,0,0,0},{2,1,-1,0,-1},   // AVERAGE BROAD BIG-HEAD STUBBY SLIM ATHLETIC TALL
    {-1,-1,1,0,1},{0,-1,1,1,0},{1,1,-1,0,-1},{-1,-1,2,0,2},{-1,0,1,1,0},                          // CHUBBY PEAR LANKY STOCKY HUNCHED
    {-1,-1,0,1,1},{0,0,2,1,1},{1,1,0,1,-1},{-1,0,1,0,2},{1,2,0,1,0},                             // POTBELLY MUSCLE PETITE BARREL DIGITIGRADE
    {1,1,0,1,0},{0,0,0,2,1},{2,1,-1,0,1},{-1,0,0,1,1},{0,-1,2,0,1},{1,2,-1,1,0},
    {-1,-1,0,1,1},{0,0,1,1,0},{1,1,-1,0,0},{1,1,0,0,1},{0,-1,1,1,0},{-1,0,1,0,2} };   // PUDGY TODDLER SPROUT SPORTY BELL STURDY
#define PARTBIT(p,v) ((v)<3?(p)*3+(v):15)   // unlock bit of option v of part p (BLADES, the 4th hand, takes the one spare bit 15)
static u16 pDna, pUnl;   // DNA points to spend; unlocked parts (bit = part*3 + option)
static inline int isPart(int id){ return (id>=LK_TAIL&&id<=LK_BACK)||id==LK_CLAWS||id==LK_ANTENNA; }
static inline int partOf(int id){ return id==LK_CLAWS?3:id==LK_ANTENNA?4:id-LK_TAIL; }   // 0 tail, 1 horns, 2 back, 3 claws, 4 antennae
static int partFree(int id,int v){ int p=partOf(id); return !partCost[p][v]||sUnlock||(pUnl>>PARTBIT(p,v)&1); }   // Konami: every part is free
static int abOf10(int a){   // 0..10, like a Sims skill bar: every part counts double, the body sliders add the odd points between
    int v=4+2*abShape[look[LK_SHAPE]<NSHAPE?look[LK_SHAPE]:0][a], hgt=slideEffS(look[LK_HEIGHT]), wgt=slideEffS(look[LK_WEIGHT]), tor=slideEffS(look[LK_TORSO]),
        arm=slideEff(look[LK_ARMS]), stn=slideEff(look[LK_STANCE]);
    switch(a){
      case AB_SPEED:   v+=(hgt>=2)-(wgt>=2)+(wgt<=-2); break;          // long legs and a light frame are quick
      case AB_JUMP:    v+=(hgt>=3)-(wgt>=3)+(tor<=-2); break;
      case AB_GRIP:    v+=(arm<=-2)+(wgt>=2)+(stn>=2); break;          // arms close in, weight down, feet apart: planted
      case AB_STYLE:   v+=(stn<=-2)+(arm>=3); break;                   // a narrow, cocky stance, arms out
      case AB_STAMINA: v+=(tor>=2)+(wgt>=1)-(wgt<=-3); break;
    }
    switch(a){
      case AB_SPEED:   v+=2*((look[LK_HSTYLE]==3)-(look[LK_HSTYLE]==2)-(look[LK_BACK]==2)); break;            // bald is quick, long hair and wings drag
      case AB_JUMP:    v+=2*((look[LK_BACK]==2)+(look[LK_EARS]==2)); break;                                    // wings and big (bunny) ears
      case AB_GRIP:    v+=2*((look[LK_TAIL]!=0)+(look[LK_HORNS]==1)+(look[LK_CLAWS]!=0)); break;              // a tail to steer with, claws to hold on
      case AB_STYLE:   v+=2*((look[LK_EYES]==2)+(look[LK_MOUTH]==1)+(look[LK_HSTYLE]==1||look[LK_HSTYLE]==2||look[LK_HSTYLE]>=4)+(look[LK_HORNS]==1)-(look[LK_EYES]==0)+(look[LK_HAT]&&look[LK_HAT]!=5)+(look[LK_PATTERN]!=0)+(look[LK_ANTENNA]==1)+(lchill>0)); break;   // hairdos, hats, body paint and feelers are stylish
      case AB_STAMINA: v+=2*((look[LK_BACK]==1)+(look[LK_HORNS]==2)+(look[LK_HAT]==5)); break;                  // armour plates, a thick skull, a helmet
    }
    return v<0?0:v>10?10:v;
}
static int abOf(int a){ return (abOf10(a)+1)/2; }   // 0..5: what the game reads
static int abPow(void){ return (look[LK_TAIL]==2?PW_BALANCE:0)|(look[LK_HORNS]==2?PW_CHARGE:0)|(look[LK_BACK]==1?PW_ARMOUR:0)|(look[LK_BACK]==2?PW_GLIDE:0)
                              |(look[LK_CLAWS]==2?PW_CLAMP:0)|(look[LK_CLAWS]==3?PW_SLASH:0)|(look[LK_ANTENNA]==2?PW_SENSE:0); }
// PINCERS = CLAMP (grinds score 2 more points each tick), BLADES = SLASH (a blow cuts through a quarter of the armour), EYE STALKS = SENSE (every DNA reward is a quarter bigger)
static int partPow(int id,int v){ return id==LK_TAIL?PW_BALANCE:id==LK_HORNS?PW_CHARGE:id==LK_BACK?(v==1?PW_ARMOUR:PW_GLIDE):id==LK_CLAWS?(v==3?PW_SLASH:PW_CLAMP):PW_SENSE; }
static inline int abPct(int a,int step){ return 100+(abOf10(a)-4)*step/2; }   // percent for an ability, 100 at 4 of 10
static int abGrindPts(void){ static const u8 t[6]={1,2,3,4,5,6}; return t[abOf(AB_GRIP)]+((abPow()&PW_CLAMP)?2:0); }   // grind points every 4 steps (3 was the old fixed value)
static int abBalance(void){ return (abPow()&PW_BALANCE)?10:0; }
static void dnaAdd(int n){ if(n>0&&(abPow()&PW_SENSE)) n+=(n+3)/4; int v=pDna+n; pDna=(u16)(v>9999?9999:v<0?0:v); }
static void partsSettle(void){   // leaving the creator: a part that was only being looked at (still locked) comes off
    int ch=0; for(int id=0;id<LK_N;id++) if(isPart(id)&&!partFree(id,look[id])){ look[id]=0; ch=1; }
    if(ch&&!custom) buildLook();
}
#define PERS_OFF 5024   // SRAM: 'P' 'S', aspiration, lifetime want, five traits, DNA (2), unlocked parts (2), checksum
#define PERS_LEN (4+TR_N+5)
static void persSave(void){
    volatile u8*m=SRAM_BASE+PERS_OFF; u8 sum=0x50;
    m[0]='P'; m[1]='S'; m[2]=pAsp; m[3]=pLtw; for(int i=0;i<TR_N;i++) m[4+i]=pTr[i];
    m[4+TR_N]=(u8)pDna; m[5+TR_N]=(u8)(pDna>>8); m[6+TR_N]=(u8)pUnl; m[7+TR_N]=(u8)(pUnl>>8);
    for(int i=2;i<PERS_LEN-1;i++) sum+=m[i];
    m[PERS_LEN-1]=sum;
}
static void persLoad(void){   // at power on, after the person of the active slot came back (the last edit wins: both are written together)
    volatile u8*m=SRAM_BASE+PERS_OFF; u8 tr[TR_N], sum=0x50;
    if(m[0]!='P'||m[1]!='S') return;
    for(int i=2;i<PERS_LEN-1;i++) sum+=m[i];
    for(int i=0;i<TR_N;i++) tr[i]=m[4+i];
    if(m[PERS_LEN-1]!=sum||!persValid(m[2],m[3],tr)) return;
    pAsp=m[2]; pLtw=m[3]; for(int i=0;i<TR_N;i++) pTr[i]=tr[i];
    pDna=(u16)(m[4+TR_N]|(m[5+TR_N]<<8)); pUnl=(u16)((m[6+TR_N]|(m[7+TR_N]<<8))&0xFFFF);
    if(pDna>9999) pDna=9999;
}
// ---------- scene ----------
static int solid(int x,int y,int z){ return x>=0&&x<W&&y>=0&&y<H&&z>=0&&z<D&&vox[y][z][x]; }
static const signed char dA[4][2]={{1,0},{0,-1},{-1,0},{0,1}}, dB[4][2]={{0,1},{1,0},{0,-1},{-1,0}};   // screen +a / +b in grid x,z per view
// The screen is two independent regions: the 3D scene (columns 0..PANEL_X-1) and the side panel.
// Both are cleared with 32-bit stores, and the panel is only redrawn when something it shows changed.
#define SCENE_W (PANEL_X/2)
#define ROW_W   (SW/2)
IWRAM_CODE static void fillCols(int w0,int w1,u16 c){
    u32 v=c|((u32)c<<16), *row=(u32*)fb;
    for(int y=0;y<SH;y++,row+=ROW_W) for(int w=w0;w<w1;w++) row[w]=v;
}
static u8 ord[4][W*D];   // per view: cells (x | z<<4) sorted back to front, so the draw loop needs no search
static void initTables(void){
    for(int sh=0;sh<4;sh++){ int r=rTab[sh]; for(int at=0;at<=r;at++) hhT[sh][at]=(u8)((r/2)*(r-at)/r); }
    for(int a=0;a<=CB;a++){ int w=0; for(int at=0;at<=CA;at++) if(hhT[0][at]>=a) w=at; rowHW[a]=(u8)w; }
    bakeTex();
    int sv=view;
    for(int v=0;v<4;v++){
        view=v; int key[W*D], n=0;
        for(int z=0;z<D;z++)for(int x=0;x<W;x++){
            int ru,rw; rotUW(2*x+1-W,2*z+1-D,&ru,&rw);
            int k=((ru+rw+8)<<8)|(z<<4)|x, j=n++;
            while(j>0&&key[j-1]>k){ key[j]=key[j-1]; j--; }
            key[j]=k;
        }
        for(int i=0;i<n;i++) ord[v][i]=(u8)(key[i]&0xFF);
    }
    view=sv;
}
// The creature creator's stage: a little house room built from the game's own wallpaper and floors, so the creature stands in the
// same kind of place it will live in. Floor tile (tx,ty) lines up with build cell x=tx, z=ty-1, so the creature stands on it exactly.
static int stageOn;   // 1: draw the stage under the creature; 0: the plain sky and build grid (block builder, game sprite baking)
#define ST_N 6                       // floor tiles per side
#define ST_WH 6                      // wall height in blocks
#define ST_WP 2                      // PEACH STRIPE
#define ST_Y0 (OYC-ST_N*CB)          // screen y of the floor's back corner
static void stageWall(int tx,int ty,int j32){   // one wall cell (tx or ty is -1); j32/16 = coplanar neighbour flags
    int sx=OXC+(tx-ty)*CA, sy=ST_Y0+(tx+ty+1)*CB;
    for(int j=1;j<=ST_WH;j++){
        int f=(j<ST_WH?1:0)|(j>1?2:0)|j32;
        if(sWp) wallBlock(sx,sy-j*CC,ST_WP,f); else cube(sx,sy-j*CC,9+ST_WP,0,f);
    }
}
__attribute__((noinline)) static void drawStage(void){   // ROM: the creator room, drawn once per redraw (its tiles and walls are IWRAM helpers)
    for(int y=0;y<SH;y++){ u16 c=RGB(3+y/45,4+y/34,10+y/16); u32 v=c|((u32)c<<16), *row=(u32*)fb+y*ROW_W; for(int w=0;w<SCENE_W;w++) row[w]=v; }
    for(int ty=0;ty<ST_N;ty++)for(int tx=0;tx<ST_N;tx++){
        int sx=OXC+(tx-ty)*CA, sy=ST_Y0+(tx+ty+1)*CB, v=(tx^ty)&1;
        int fl=(tx>=1&&tx<=ST_N-2&&ty>=1&&ty<=ST_N-2)?2:4;   // TEAL CARPET rug on WOOD PLANKS
        if(sFl) floorTile(sx,sy,&flTab[fl][v][0][0]); else tileTop(sx,sy,flFlat[fl][v]);
    }
    for(int k=-1;k<ST_N;k++){   // back to front: the corner first, then the two walls moving toward the viewer
        stageWall(-1,k,k>=0?32:0);
        if(k>=0) stageWall(k,-1,16);
    }
}
// ---- ears: drawn flat on the sides of the head (the +x and -x faces), in the same iso perspective as the blocks, standing a little
// proud of the face so they read as ears. The ear on the far side is drawn before the blocks (only its rim peeks out past the head),
// the near one after them. Each is an oval in the side face's plane: wide along the head's depth, tall up the head, with a rim and a hollow.
// ---- ANTENNAE: FEELERS (thin, a bead on top) or EYE STALKS (thicker, an eye on each), drawn from the top of the head like the ears ----
__attribute__((noinline)) static void drawAntennae(void){
    int an=look[LK_ANTENNA]; if(!an||custom) return;
    int hx,hy,hz,hs; headBox(&hx,&hy,&hz,&hs); int hw=2*hs, hd=2*hs, hh=2*hs, ty=((BXH-(hy+hh)>=1)?hy+hh:hy+hh-1)+1;   // just above the hair (or the head)
    static const u8 pcs[6]={5,4,8,2,3,7}; u16 col=shade(sL[1],an==1?14:12), tip=sT[an==1?pcs[look[LK_PATCOL]%6]:2];
    for(int sd=0;sd<2;sd++){
        int u=2*hx+(sd?2*hw-1:1)-W+(sd?1:-1)*(slideEff(look[LK_ANTGAP])/2), w=2*hz+2*hd-1-D-slideEff(look[LK_ANTFB])*hs;   // ANT GAP: wider / closer; ANT FRONT BACK                        // the front corners of the head's top
        int x0,y0,x1,y1,a,b; projC(u,w,ty,&x0,&y0); rotUW(sd?1:-1,0,&a,&b); int lean=(a-b)*(2+slideEff(look[LK_ANTSPR]));   // ANT SPREAD: how far they lean out
        int len=(an==1?11:12)*hs+slideEffS(look[LK_ANTLEN])*2*hs; if(len<3) len=3; x1=x0+lean;   // ANT LENGTH: two pixels a notch y1=y0-len;
        line(x0,y0,x1,y1,col); if(an==2){ line(x0+1,y0,x1+1,y1,col); }
        int ts=slideEffS(look[LK_ANTTIP])/2;   // ANT TIP: the bead / eye grows a pixel every two notches
        if(an==1){ int r=1+ts; if(r<0) r=0; rect(x1-r,y1-r,2*r+1,2*r+1,tip); px(x1-r,y1-r,lite(tip,19)); }
        else { int R=2+ts; if(R<1) R=1; rect(x1-R,y1-R,2*R+1,2*R+1,RGB(3,3,6)); rect(x1-R+1,y1-R+1,2*R-1,2*R-1,RGB(31,31,31)); px(x1,y1,RGB(3,3,6)); }
    }
}
// ---- TAIL and HORNS: sprites, so their length / size are sliders and they take no blocks (and no RAM: they are drawn each frame) ----
__attribute__((noinline)) static void drawTail(int near){   // STUB (a short wedge) or LONG (droops); TAIL LENGTH: a half block a notch
    int tl=look[LK_TAIL]; if(!tl||custom) return;
    int L,T,hs; bodyPlan(&L,&T,&hs);
    int u=2*(BX0+(BXW-2)/2)+2-W, w=-D, x0,y0; projC(u,w,L+1,&x0,&y0); y0+=(CC+liftT)/2; y0-=slideEff(look[LK_TAILHT])*2; if(T<=1) y0+=neckK;   // TAIL HEIGHT: two pixels a notch   // the middle of the back of the hips (the TORSO slider stretches that row: liftT px)
    int ua,ub; rotUW(0,-2,&ua,&ub); int bx=(ua-ub)*CA/2, by=(ua+ub)*CB/2;                  // px per block going backwards
    x0+=bx*bakeWk/(2*CA); y0+=by*bakeWk/(2*CA);   // WEIGHT: a heavier body is fatter front to back too, so the root moves out with its back face
    if((by>0)!=near) return;                                                                  // pointing away from you: behind the body
    int n=(tl==1?4:7)+slideEffS(look[LK_TAILLEN]); if(n<2) n=2;
    u16 tb=toneBy(sT[5],slideEffS(look[LK_TAILTONE])), col=shade(tb,13); int cu=slideEff(look[LK_TAILCURL]), tk=slideEffS(look[LK_TAILTHK])/2, sway=slideEff(look[LK_TAILSW]), tipn=n+(slideEffS(look[LK_TAILTL])*(n+2))/4;   // TAIL SWAY: leans left / right on the screen; TAIL TIP LEN: how much of the tail is the tip colour
      // TAIL CURL: negative droops, positive curls up; TAIL THICKNESS: a pixel every two notches
    static const u8 tipSlot[7]={0,2,4,8,3,6,7}; int tpS=tipSlot[look[LK_TAILTIP]%7]; u16 tpc=toneBy(tpS?shade(sT[tpS],13):col,slideEffS(look[LK_TIPTONE]));   // TAIL TIP: the last quarter in another colour (TIP SHADE: lighter / darker)
    int tpe=slideEff(look[LK_TAILTAPER]), fl=slideEff(look[LK_TAILFLUF]), wv=slideEff(look[LK_TAILWAVE]);   // TAIL TAPER: blunt .. pointed; TAIL FLUFF: slim .. bushy (a fuzzy rim); TAIL WAVE: an S wiggle
    static const signed char wvT[8]={0,5,7,5,0,-5,-7,-5};
    for(int j=0;j<=4*n;j++){
        int f=16*(4*n-j)/(4*n), mx=f;   // 16 at the root, 0 at the tip
        if(tpe>0) mx=f+(f*f/16-f)*tpe/4; else if(tpe<0) mx=f+((16-(16-f)*(16-f)/16)-f)*(-tpe)/4;   // pointed: thins early; blunt: stays thick to the end
        int t=1+(hs+1)*mx/16+tk+(bakeWk>0?bakeWk/4:0)+(fl*(f*(16-f)/16))/6; if(t<1) t=1;   // (and a bit thicker on a heavy body); FLUFF swells the middle
        int x=x0+bx*j/8+(j*sway)/(2*n), y=y0+by*j/8+(tl==2?j*j*(10-3*cu)/960:j*(2-cu)/16);
        if(wv) y+=(wv*wvT[(j*8/(2*n+1))&7]*j)/(8*4*n);   // WAVE grows from the root
        u16 c0=j>4*n-tipn?tpc:col, hi=lite(c0,19), lo=shade(c0,9);
        if(t<=2){ rect(x-t/2,y-t/2,t,t,c0); continue; }
        int r=t/2;   // a round, lit-from-above brush: highlight on top, shadow underneath, a dark rim at the sides
        for(int dy=-r;dy<=r;dy++){ int hw=r; while(hw>0&&hw*hw+dy*dy>r*r+r) hw--;
            u16 c=dy*3<-r?hi:dy*3>r?lo:c0; rect(x-hw,y+dy,2*hw+1,1,c); if(hw>=2){ px(x-hw,y+dy,lo); px(x+hw,y+dy,lo); } }
        if(fl>0&&(j&1)){ int e=r+1; px(x-e,y-r/2+(j&2),c0); px(x+e,y+r/2-(j&2),c0); }   // fluffy: a fuzzy rim
    }
}
__attribute__((noinline)) static void drawHorns(int near){   // NUBS or HORNS, one on each side of the head; HORN SIZE: 20% a notch
    int hn=look[LK_HORNS]; if(!hn||custom) return;
    int hx,hy,hz,hs; headBox(&hx,&hy,&hz,&hs); int hw=2*hs, hd=2*hs, hh=2*hs, top=(BXH-(hy+hh)>=1)?hy+hh:hy+hh-1;
    int len=(hn==1?5:11)*hs*(10+slideEffS(look[LK_HORNSZ])*2)/10; if(len<2) len=2;
    u16 hb=toneBy(sT[8],slideEffS(look[LK_HORNTONE])), col=shade(hb,13), tip=lite(hb,18); int ho=6+slideEff(look[LK_HORNSPR]), cv=4+slideEff(look[LK_HORNCRV]);   // HORN SPREAD: how far out they lean; HORN CURVE: how much the tip bends back in
    int hte=slideEff(look[LK_HORNTIP]), htn=hte<0?(hte==-4?0:1):1+hte*len/8;   // HORN TIP: how much of the horn is the light tip
    for(int sd=-1;sd<=1;sd+=2){
        int a,b; rotUW(sd,0,&a,&b); if(((a+b)>0)!=near) continue;
        int sx=(a-b)>0?1:-1, u=2*hx+hw-W+sd*(hw+1), w=2*(hz+hd-1)+1-D-slideEff(look[LK_HORNFB])*hs, x0,y0; projC(u,w,top+1,&x0,&y0);   // HORN FWD BACK: along the head
             y0+=CC/2-2*hs*slideEff(look[LK_HORNHT]); x0+=sx*headK;   // HORN HEIGHT: up or down the side of the head
        for(int i=0;i<=len;i++){
            int x=x0+sx*((i*ho)/4-(i*i*cv)/(4*(len+1))), t=2+(len-i)*hs/len+slideEffS(look[LK_HORNTH])/2; if(t<1) t=1;   // HORN THICKNESS   // out, then curving back in; thick at the root, a point at the tip
            rect(x-t/2,y0-i,t,1,i>len-htn?tip:col);
        }
    }
}
__attribute__((noinline)) static void drawWings(int near){   // WINGS: a feathered fan off each shoulder blade; WING SIZE: 20% a notch
    if(look[LK_BACK]!=2||custom) return;
    int L,T,hs; bodyPlan(&L,&T,&hs);
    int u=2*(BX0+(BXW-2)/2)+2-W, w=-D, x0,y0; projC(u,w,L+T,&x0,&y0); y0+=CC/2+liftT/2+neckK;
    int ua,ub; rotUW(0,-2,&ua,&ub); int bx=(ua-ub)*CA/2, by=(ua+ub)*CB/2;
    if((by>0)!=near) return;
    x0+=bx*bakeWk/(2*CA); y0+=by*bakeWk/(2*CA);
    int span=(14+4*hs)*(10+slideEffS(look[LK_WINGSZ])*2)/10; if(span<4) span=4;
    int wsp=slideEff(look[LK_WINGSP])*2; if(wsp<-2) wsp=-2; y0-=slideEff(look[LK_WINGHT])*2;   // WING SPREAD: out from the back; WING HEIGHT: up the back
    int a,b; rotUW(1,0,&a,&b); int rs=(a-b)>0?1:-1;
    int wd=slideEff(look[LK_WINGDROOP]), we=slideEffS(look[LK_WINGTONE]); u16 wa=toneBy(RGB(31,31,31),we), wb=toneBy(RGB(24,24,26),we); if(we>0){ wa=RGB(31,31,31-we*3); wb=RGB(24,24,26-we*3); }   // WING DROOP: the fan tilts down / up; WING SHADE: grey .. cream
    for(int sd=-1;sd<=1;sd+=2) for(int k=0;k<=span;k++){
        int x=x0+sd*rs*(k+2+wsp), top=y0-(k*2)/3-(k*k)/(3*span)+wd*k/5, bot=y0+k/4+3+wd*k/5; u16 c=(k%3==2)?wb:wa;
        vline(x,top,bot,c); px(x,top,RGB(20,20,24)); px(x,bot,RGB(20,20,24));
    }
}
__attribute__((noinline)) static void drawEars(int near){   // ROM, not inlined into the IWRAM drawScene
    int es=look[LK_EARS]; if(!es||custom) return;
    int hx,hy,hz,hs; headBox(&hx,&hy,&hz,&hs);
    int hw=2*hs, hd=2*hs, f=10+slideEffS(look[LK_EARSZ])*2;                       // ear size slider: 20% smaller or bigger per step
    int ry=((es==1?5:9)+(hs-1)*(es==1?2:4))*f/10, rz=((es==1?4:5)+(hs-1)*2)*f/10;  // half height and half depth in px; BIG ears are tall
    rz=rz*(10+slideEffS(look[LK_EARWID])*2)/10; if(ry<1) ry=1; if(rz<1) rz=1;   // EAR WIDTH: along the head
    for(int sd=-1;sd<=1;sd+=2){
        int a,b; rotUW(sd,0,&a,&b); int vis=(a+b)>0;                              // this side's face turns towards the camera
        if(vis!=near) continue;
        int xs=sd<0?hx-1:hx+hw, out=0;                                           // stand clear of hair or a helmet covering the side
        for(int z=hz;z<hz+hd;z++) if(xs>=0&&xs<W&&hy+hs<H&&(vox[hy+hs][z][xs]&15)) out=1;
        int u=2*hx+hw-W+sd*(hw+1+2*out), w=2*hz+hd-D;                           // just outside the middle of the side face
        int cxp,cyp; projC(u,w,hy,&cxp,&cyp);
        cyp-=(2*hs*CC*9)/20+slideEff(look[LK_EARLF]);                           // a little above the middle of the head's side
        int ua,ub; rotUW(0,2,&ua,&ub);                                           // screen step for one block of depth (along z)
        int dzx=(ua-ub)*CA/2, dzy=(ua+ub)*CB/2;                                  // px per block along the face
        { int sp=slideEff(look[LK_EARSPR])+headK, fw=slideEff(look[LK_EARFWD]);        // EAR SPREAD: out from the head (a pixel a notch); EAR FRONT BACK: a quarter block a notch along the face
          cxp+=(a-b)*sp+fw*dzx/4; cyp+=(a+b)*sp/2+fw*dzy/4; }
        int lft=(a-b)<0; u16 fill=shade(lft?sL[1]:sR[1],15), pit=shade(lft?sL[1]:sR[1],11), edge=shade(sT[1],5);
        int n=4*rz;                                                              // sample the oval finely enough to leave no holes
        for(int iy=-4*ry;iy<=4*ry;iy++)for(int iz=-n;iz<=n;iz++){
            long e=(long)iz*iz*ry*ry+(long)iy*iy*rz*rz, E=(long)16*rz*rz*ry*ry;  // (iz/4rz)^2+(iy/4ry)^2 <= 1
            if(e>E) continue;
            int rim=e*100>E*62, hol=!rim&&((long)(iz+n/4)*(iz+n/4)*ry*ry*4+(long)iy*iy*rz*rz*4<=E);
            int x=cxp+(iz*dzx)/(4*CA), y=cyp+(iz*dzy)/(4*CA)+iy/4;
            px(x,y,rim?edge:hol?pit:fill);
        }
    }
}
// The seat (BUTT slider, teens and up): two rounded cheeks in the colour of the hips (the bottom clothes, or skin when they are bare) on the back of the hips, drawn as shaded domes so they
// read at a glance from behind or the side: lit from above, a dark rim, the cleft where they meet and the crease under them. The slider
// takes them from nearly flat to full and round (each notch both bulges them out and grows them a little, so no two notches look alike).
__attribute__((noinline)) static void drawSeat(int x,int y,int u,int w,int rl,int slot){   // on the back of the top leg block (rl: that row's HEIGHT stretch; slot: that block's colour slot)
    int sx,sy,ax,ay,bx,by; projC(u,w,y+1,&sx,&sy); projC(u+2,w,y+1,&ax,&ay); projC(u,w+2,y+1,&bx,&by);
    int xX=ax-sx, xY=ay-sy, zX=bx-sx, zY=by-sy;              // one block along x, along z (towards the front), on screen
    int p=slidePos(look[LK_BUTT]), side=(x==BX0+(BXW-2)/2)?-1:1;
    int cx=sx-zX/2-side*xX/10, cy=sy+(CC+rl)/2-zY/2-side*xY/10;   // the back face of the block, a little high (up into the hips), nudged in to meet its twin
    int rxq=30+p+8*slideEffS(look[LK_BUTTW]), ry64=(CC+rl)*(40+2*p)+64*slideEff(look[LK_BUTTH]), bul=8+7*p;   // BUTT WIDTH / HEIGHT: a pixel a notch
    if(rxq<8) rxq=8; if(ry64<128) ry64=128;   // a little taller than wide, so they read round, not squashed                 // half width (64ths of a block), half height (px/64), bulge (64ths of a block)
    static const signed char cs[32]={64,63,59,53,45,36,24,12,0,-12,-24,-36,-45,-53,-59,-63,-64,-63,-59,-53,-45,-36,-24,-12,0,12,24,36,45,53,59,63};
    static const u8 dq[17]={64,64,63,62,62,60,58,56,53,50,46,41,36,30,22,13,0};   // the dome's height at ring r of 16
    u16 b=base[(slot>=1&&slot<=8)?slot:7];   // the seat is the same colour as what covers the hips: the bottom colour, or skin when they are bare
    for(int ri=16;ri>=0;ri--){ int na=ri?32:1, d=dq[ri];
        for(int a=0;a<na;a++){
            int cu=cs[a]*ri/16, cv=cs[(a+24)&31]*ri/16;       // where on the disc (64ths), outer rings first so the middle of the dome lands on top
            int X=cx+(xX*cu*rxq)/4096-(zX*d*bul)/4096, Y=cy+(cv*ry64)/4096+(xY*cu*rxq)/4096-(zY*d*bul)/4096;
            u16 c;
            if(ri>=13&&cu*side<-40) c=RGB(3,3,5);                          // the cleft: a clean dark line down the inner edge (survives the half-size bake)
            else if(ri>=15&&cv>-24) c=shade(b,7);                          // the rim (not along the top: it runs into the waist)
            else if(ri>=13&&cv>46) c=shade(b,8);                           // the crease underneath
            else { int f=10+(d*5)/64-(cv*2)/64+(cu*side>0?(cu*side)/48:0); c=f<=16?shade(b,f):lite(b,f); }   // lit from above and the outside, brightest where it sticks out most
            px(X,Y,c); px(X+1,Y,c);
        } }
}
// The neck (NECK LENGTH above 0): its own narrow skin column on the middle of the shoulders, up to the underside of the head, so a long neck reads as a neck and
// not as a tall block of shoulders. NECK WIDTH makes it thinner or thicker. Stacked blocks (a block every CC px, so a long neck has no gaps); the head is drawn over its top.
__attribute__((noinline)) static void drawNeck(int hyB){
    int sx,sy; projC(2*(BX0+(BXW-2)/2)+2-W,0,hyB,&sx,&sy);   // the middle of the torso, at the underside of the head (the shoulders' top is neckK lower)
    int top=sy-CC, bot=sy+neckK-CC; cubeDR=-2+slideEffS(look[LK_NECKW]);
    for(int Y=bot;Y>top;Y-=CC) cube(sx,Y,1,0,Y==bot?1:3);
    cube(sx,top,1,0,2); cubeDR=0;
}
static void bxSync(void);
// DEBUG CODE (title screen): HEIGHT also works for a BABY. A baby has one leg row (none for BIG HEAD / STUBBY / STOCKY / PETITE / TODDLER), so the leg stretch
// does little; with the debug code on, the slider stretches the baby's torso row as well. Out of line (ROM) so the IWRAM drawing code stays small.
__attribute__((noinline)) static int babyTall(void){ if(stage!=AG_BABY||!dbgOn) return 0; int h=slideEffS(look[LK_HEIGHT]); return h<-4?-4:h>bxLift?bxLift:h; }
IWRAM_THUMB static void drawScene(int blink){
    bxSync();   // (the bake swaps the stage between Sims without building the look again)
    if(stageOn&&!noGrid) drawStage(); else if(bakeOn) rect(cX0,cY0,(int)cW,(int)cH,SKY); else fillCols(0,SCENE_W,SKY);
    // floor grid
    u16 gc=RGB(13,18,22); int a,b,c,d;
    if(!noGrid&&!stageOn) for(int i=BX0;i<=BX0+BXW;i++){ projC(2*i-W,-D,0,&a,&b); projC(2*i-W,2*BXD-D,0,&c,&d); line(a,b,c,d,gc); }   // the grid shows only this stage's box
    if(!noGrid&&!stageOn) for(int j=0;j<=BXD;j++){ projC(2*BX0-W,2*j-D,0,&a,&b); projC(2*(BX0+BXW)-W,2*j-D,0,&c,&d); line(a,b,c,d,gc); }
    int fv=view==0?0:view==3?1:-1;   // which cube face shows the +Z (front) face, -1 = turned away
    // the body sliders (a look-built creature only): HEIGHT stretches the leg rows, WEIGHT widens (or slims) every block below the head
    int wk=0, hyB=H; decLook=!custom;
    int shA=0, shL=0;   // the body type's extra width for the arms and legs (the torso's goes into wk)
    if(decLook){ int L,T,hs; bodyPlan(&L,&T,&hs); liftL=L; const signed char*sd=shpDraw[look[LK_SHAPE]<NSHAPE?look[LK_SHAPE]:0];
        liftK=slideEffS(look[LK_HEIGHT])+((look[LK_SHAPE]==6&&stage>=AG_TEEN)?3:0)+sd[3]; if(liftK<-4) liftK=-4; if(liftK>bxLift) liftK=bxLift;   // TALL: longer legs (drawn taller, so the hair keeps its room)
        wk=slideEffS(look[LK_WEIGHT])+sd[0]; shA=sd[1]+slideEffS(look[LK_ARMW])/2; shL=sd[2]+slideEffS(look[LK_LEGW])/2; hyB=L+T;   // LEG WIDTH: half a pixel a notch (-2..+2), on top of the body type's legs
        headK=slideEffS(look[LK_HEADSZ])+(stage==AG_BABY?2:stage==AG_CHILD?1:0); handK=slideEffS(look[LK_HANDFT]);   // HEAD SIZE: px added to the head's half width; HAND FOOT SIZE: the same for the hands and the feet
        liftTn=T; liftT=slideEffS(look[LK_TORSO])+babyTall(); if(liftT<-4) liftT=-4; if(bxBig&&liftT>0) liftT+=liftT/2; armK=slideEff(look[LK_ARMS]); stanceK=slideEff(look[LK_STANCE]);   // TORSO px per torso row, ARMS and STANCE spread
        neckK=slideEffS(look[LK_NECK]); if(neckK<-2) neckK=-2; if(bxBig&&neckK>0) neckK+=neckK/2; exHip=slideEffS(look[LK_HIPW]); exWst=slideEffS(look[LK_WAISTW]); exSho=slideEffS(look[LK_SHOULW]); exThi=slideEffS(look[LK_THIGHW]); exCal=slideEffS(look[LK_CALFW]);
        exJaw=slideEffS(look[LK_JAWW]); exHnd=slideEffS(look[LK_HANDSZ]); exFt=slideEffS(look[LK_FOOTSZ]); exChe=slideEffS(look[LK_CHESTW]); exBel=slideEffS(look[LK_BELLYW]); exUAr=slideEffS(look[LK_UARMW]); exFAr=slideEffS(look[LK_FARMW]);   // CHEST, BELLY (torso rows), UPPER ARM, FOREARM width
        { const signed char*se=shpEx[look[LK_SHAPE]<NSHAPE?look[LK_SHAPE]:0]; exHip+=se[0]; exWst+=se[1]; exSho+=se[2]; exThi+=se[3]; exCal+=se[4]; exChe+=se[5]; }   // the body type's own build (V-SHAPE, CURVY, POWER...)
        { int m=neckK<0?-neckK:neckK; int e[12]={exHip,exWst,exSho,exThi,exCal,exChe,exBel,exUAr,exFAr,exJaw,exHnd,exFt}; for(int q=0;q<12;q++){ int a=e[q]<0?-e[q]:e[q]; if(a>m) m=a; } exMax=m; }
        if(noGrid){ if(liftK>bakeCapH) liftK=bakeCapH; if(wk>bakeCapW) wk=bakeCapW; if(liftT>bakeCapT) liftT=bakeCapT;
                    if(armK>bakeCapX) armK=bakeCapX; if(stanceK>bakeCapX) stanceK=bakeCapX; if(shA>bakeCapL) shA=bakeCapL; if(shL>bakeCapL) shL=bakeCapL; } if(noGrid){ neckK=EXC(neckK); exHip=EXC(exHip); exWst=EXC(exWst); exSho=EXC(exSho); exThi=EXC(exThi); exCal=EXC(exCal); exChe=EXC(exChe); exBel=EXC(exBel); exUAr=EXC(exUAr); exFAr=EXC(exFAr); exJaw=EXC(exJaw); exHnd=EXC(exHnd); exFt=EXC(exFt); } bakeWk=wk; bakeSh=shA>shL?shA:shL; }   // a sprite bake: only as tall / wide as its box holds
    else liftK=liftT=armK=stanceK=headK=handK=neckK=exHip=exWst=exSho=exThi=exCal=exChe=exBel=exUAr=exFAr=exJaw=exHnd=exFt=exMax=0;
    drawEars(0); drawTail(0); drawWings(0); drawHorns(0);
    int nsx=0, nsy=0, ntint=0; u16 ndc=0;   // where the mouth sprite went (for a raised nose)
    // voxels (back to front)
    for(int y=0;y<H;y++){ if(y==hyB&&decLook&&neckK>0) drawNeck(hyB);   // the neck goes in after the body and before the head
    for(int i=0;i<W*D;i++){
        int x=ord[view][i]&15, z=ord[view][i]>>4;
        int raw=vox[y][z][x], ci=raw&15, shape=raw>>4;
        if(ghost[y][z][x]&&blink) ci=8;
        if(gdec[y][z][x]&&blink&&fv<0) ci=8;   // face turned away: flag the target voxel instead
        if(!ci) continue;
        int u=2*x+1-W, w=2*z+1-D;
        if(strideK&&shape>=1&&shape<=3) w+=((x<W/2)==(shape==3))?strideK:-strideK;   // the walk: legs (shape 3) and arms (1, 2) swing, in either creator
        int onArm=0;                     // claws, pincers (anything wedge-shaped hanging off a hand) go where that arm goes
        if(shape>=4){
            #define ARMV(yy,zz) ((yy)<H&&(zz)>=0&&((vox[yy][zz][x]>>4)==1||(vox[yy][zz][x]>>4)==2))
            onArm=ARMV(y+1,z)||ARMV(y,z-1)||ARMV(y+1,z-1);
            #undef ARMV
            if(strideK&&onArm) w+=(x<W/2)?-strideK:strideK;   // the swing
        }
        int sx,sy; projC(u,w,y+1,&sx,&sy);   // top-face centre
        if(onArm){ int sg=u<0?1:-1, a2,b2, hg=HUG-wk-armK-exWst-exSho; rotUW(sg,0,&a2,&b2); sx+=hg*(a2-b2); sy+=(hg*(a2+b2))/2; }   // and the hug (else they float off the hand)
        int bw=(y<hyB&&shape<4)?(shape==1||shape==2?wk/2+shA:shape==3?wk-shpDraw[look[LK_SHAPE]<NSHAPE?look[LK_SHAPE]:0][0]+shL:wk):(y>=hyB&&shape<4?headK:0);
        if(y<hyB&&(shape==2||(shape==3&&y==0))) bw+=handK;   // bigger or smaller hands and feet
        if(y<hyB&&shape==2) bw+=exHnd; else if(y<hyB&&shape==3&&y==0) bw+=exFt;   // HAND SIZE and FOOT SIZE on their own
        if(y==hyB&&shape<4) bw+=exJaw;   // JAW WIDTH: the bottom row of the head
        if(y<hyB&&shape==0) bw+=exWst+(y==liftL?exHip:0)+(y==hyB-1?exSho:0)+(((y==hyB-2&&y>liftL)||(hyB-liftL<3&&y==hyB-1))?exChe:0)+(((hyB-liftL>=3&&y==liftL+1&&y<hyB-1)||(hyB-liftL<3&&y==liftL))?exBel:0);   // WAIST, HIP and SHOULDER width (the torso only), CHEST (the row under the shoulders) and BELLY (the row over the hips; on the usual 2 row torso: CHEST is the shoulder row and BELLY the hip row, else these sliders did nothing)
        else if(y<hyB&&(shape==1||shape==2)) bw+=(y>=(liftL+hyB)/2)?exUAr:exFAr;   // UPPER ARM (the top half of the arm) and FOREARM width
        else if(y<hyB&&shape==3) bw+=(y>=liftL/2)?exThi:exCal;                 // THIGH (upper half of the legs) and CALF (lower half) width
        if(shape==1||shape==2){ int sg=u<0?1:-1, a2,b2, hg=HUG-wk-armK-exWst-exSho; rotUW(sg,0,&a2,&b2); sx+=hg*(a2-b2); sy+=(hg*(a2+b2))/2; }   // hug the torso (a heavier torso, or the ARMS slider, pushes the arms out)
        else if(shape==3&&stanceK&&y<hyB){ int sg=u<0?-1:1, a2,b2; rotUW(sg,0,&a2,&b2); sx+=stanceK*(a2-b2); sy+=(stanceK*(a2+b2))/2; }   // STANCE: the legs apart or together
        int f=(solid(x,y+1,z)?1:0)|(solid(x,y-1,z)?2:0)
             |(solid(x-dA[view][0],y,z-dA[view][1])?16:0)|(solid(x-dB[view][0],y,z-dB[view][1])?32:0);
        if(shape>=4){ cubeDR=(y>=hyB&&!onArm)?headK:0; wedgeCube(sx,sy,ci,shape,f); cubeDR=0; }   // hair, furry ears and the like grow with the head
        else { cubeDR=bw;
            int rl=y<liftL?liftK:y<liftL+liftTn?liftT:0;   // this row stretched (HEIGHT: the legs, TORSO: the torso)
            int syc=sy; if(y==liftL+liftTn-1&&neckK>0) syc+=neckK;   // NECK LENGTH: the head sits higher; the top torso row stays at the shoulders and the neck (drawNeck) fills the gap
            for(int o=rl;o>0;o-=CC) cube(sx,syc+o,ci,shape,f|1);   // a stretched row: its lower part first (a block every CC pixels, so a long stretch has no gaps), then the block on top of it
            cube(sx,syc,ci,shape,rl>0?f|2:f); cubeDR=0;
            if(decLook&&(stage>=AG_TEEN||sUnlock)&&liftL>0&&y==liftL-1&&z==1&&shape==3&&(x==BX0+(BXW-2)/2||x==BX0+BXW/2)) drawSeat(x,y,u,w,rl,raw&15); }   // the seat, on the back of the top of the legs
        u16 dc=dec[y][z][x]; int tint=0;
        if(gdec[y][z][x]&&blink){ dc=gdec[y][z][x]; tint=1; }
        if(dc&&fv>=0){ drawDeco(sx,sy,dc,fv,tint); if(decSpr(dc)-1>=NEYE){ nsx=sx; nsy=sy; ndc=dc; ntint=tint; } }
    } }
    if(ndc&&decLook&&look[LK_NOSE]&&slideEff(look[LK_NOSEHT])>0){ decNose=1; drawDeco(nsx,nsy,ndc,fv,ntint); decNose=0; }   // a raised nose, over the block above the mouth
    drawEars(1); drawTail(1); drawWings(1); drawHorns(1); drawAntennae();
}
static volatile int mWantOff; static void audIdleStop(void);   // set by the mixer interrupt when nothing is left to play: vsync() then switches it off
static void ldTick(void);   // loading.h: once per frame, lets the music come back after a loading screen
// ZOOM (OPTIONS > VIDEO > ZOOM, SELECT + UP / DOWN while playing): the room is drawn only in a window in the middle (vpX0..vpX1, vpY0..vpY1)
// and BG2's own scaling stretches that window over the room rows; the HUD rows stay 1:1. An HBlank DMA writes every line's BG2 scaling
// from zoomDma (4 words a line, zoomtab.h, copied to EWRAM: DMA0 cannot read the cartridge); the vblank IRQ starts it again every frame.
static u8 zoomShow, zoomKeep, zoomNum=1, zoomDen=1; static u16 zoomPa=256; static const u32* zoomDma;   // zoomShow: the screen shows a zoomed room picture
IWRAM_ARM static void zoomArm(void){   // in vblank: line 0's scaling now, then the DMA writes each next line's in HBlank
    volatile u32*d0=(volatile u32*)0x040000B0, *bg=(volatile u32*)0x04000020; const u32*t=zoomDma;
    d0[2]=0; bg[0]=t[0]; bg[1]=t[1]; bg[2]=t[2]; bg[3]=t[3];
    d0[0]=(u32)(uintptr_t)(t+4); d0[1]=0x04000020u; d0[2]=4u|(3u<<21)|(1u<<25)|(1u<<26)|(2u<<28)|(1u<<31);   // 4 words, dest reload, repeat, 32 bit, HBlank, on
}
static void zoomOff(void){ volatile u32*d0=(volatile u32*)0x040000B0, *bg=(volatile u32*)0x04000020; zoomShow=0; d0[2]=0; bg[0]=0x100; bg[1]=0x1000000u; bg[2]=0; bg[3]=0; }   // BG2 back to 1:1
static void vsync(void){ while(REG_VCOUNT>=160); while(REG_VCOUNT<160); if(mWantOff) audIdleStop(); svTick(); ldTick(); }
static void present(void){
    if(zoomShow&&!zoomKeep) zoomOff();   // anything but a room picture (menus, messages) is shown 1:1
    vsync();
    REG_DMA3SAD=(u32)(uintptr_t)fb; REG_DMA3DAD=VRAM_ADDR;
    REG_DMA3CNT=(SW*SH/2)|0x84000000u;
}

// Copy only the scene columns (blink-only redraws leave the panel untouched).
static void presentScene(void){
    vsync();
    for(int y=0;y<SH;y++){
        REG_DMA3SAD=(u32)(uintptr_t)(fb+y*SW); REG_DMA3DAD=VRAM_ADDR+(u32)(y*SW*2); REG_DMA3CNT=SCENE_W|0x84000000u;
    }
}
// Row-by-row DMA of a rectangle (32-bit columns w0..w1-1, rows y0..y1-1): src buffer -> dst base address.
static void dmaRows(const u16*src,u32 dst,int w0,int w1,int y0,int y1){
    for(int y=y0;y<y1;y++){
        int o=y*SW+w0*2;
        REG_DMA3SAD=(u32)(uintptr_t)(src+o); REG_DMA3DAD=dst+(u32)(o*2); REG_DMA3CNT=(u32)(w1-w0)|0x84000000u;
    }
}

// ---------- title screen ----------
#include "titleimg.h"
#include "titlelogo.h"   // the BORE logo of the cover art (gold bubble letters, eyes in the B and R, a leaf in the O, a joint on the E; tools/make_logo.py)
#include "logo.h"
#define SM_W0 76   // smoke stays inside columns 152..203, rows 0..89 (checked over its whole 128-frame loop)
#define SM_W1 102
#define SM_Y1 90
#define TX_W0 47    // "PRESS START" box
#define TX_W1 76   // (was 70, which cut "PRESS START" off at x=140)
#define TX_Y0 141
#define TX_Y1 147
#define TB_TX ((SM_W1-SM_W0)*2*SM_Y1)   // where the PRESS START box starts in tfb
_Static_assert(TB_TX+(TX_W1-TX_W0)*2*(TX_Y1-TX_Y0)<=4*SPW*SPH,"the title backdrop pieces must fit in spr4");
static void titleKeep(int save,int w0,int w1,int y0,int y1,int at){   // copy a rectangle (32-bit columns w0..w1-1, rows y0..y1-1) fb <-> tfb+at
    int w=(w1-w0)*2; u16*t=tfb+at;
    for(int y=y0;y<y1;y++,t+=w){ u16*f=fb+y*SW+w0*2; if(save) for(int i=0;i<w;i++) t[i]=f[i]; else for(int i=0;i<w;i++) f[i]=t[i]; }
}
static void buildTitle(void){
    for(int y=0;y<80;y++)for(int x=0;x<120;x++){
        char c=titleArt[y][x]; u16 col=titlePal[c<='9'?c-'0':c-'a'+10];
        u16*o=&fb[(y*2)*SW+x*2]; o[0]=o[1]=o[SW]=o[SW+1]=col;
    }
    for(int y=118;y<SH;y++)for(int x=0;x<SW;x++){ u16 c=fb[y*SW+x]; fb[y*SW+x]=shade(c,7); }   // dim strip for the prompt
    u16 ink=RGB(4,3,6), grn=RGB(12,28,8);
    for(int y=0;y<LOGO_H;y++){ const char*r=logoArt[y]; u16*o=&fb[(y+2)*SW+4];   // the logo, top left (0 = see-through)
        for(int x=0;x<LOGO_W;x++){ char c=r[x]; if(c!='0') o[x]=logoPal[(c<='9'?c-'0':c-'a'+10)-1]; } }
    text(13,58,"A VOXEL LIFE SIM",ink,1); text(12,57,"A VOXEL LIFE SIM",grn,1);
    text(14,126,"PUFF PUFF PASS THE CONTROLLER",RGB(16,22,12),1);
    titleKeep(1,SM_W0,SM_W1,0,SM_Y1,0); titleKeep(1,TX_W0,TX_W1,TX_Y0,TX_Y1,TB_TX);
}
static void smoke(int frame){
    static const signed char wob[16]={0,1,2,3,3,3,2,1,0,-1,-2,-3,-3,-3,-2,-1};
    for(int i=0;i<9;i++){
        int t=(frame+i*14)&127;
        int x=164+wob[(t/4+i*5)&15]+t/5, y=82-t*7/8, r=1+t/24;
        for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++){
            if(dx*dx+dy*dy>r*r+1) continue;
            if(t>64&&((x+dx+y+dy)&1)) continue;       // fade out by dithering
            if(t>100&&((x+dx)&1)) continue;
            px(x+dx,y+dy,RGB(27,28,29));
        }
    }
}
// ---------- LIFE MODE: fixed isometric "sim" room + Tony-Hawk-style skating (placeholder) ----------
// Pick "GO LIVE LIFE!" in the part list and press A. SELECT+START returns to the editor.
// Controls: D-pad L/R steer (grounded) or spin (airborne) | hold A push | D-pad down brake | B ollie / kickflip in air
// Land spins in half-turns (180/360) for points, a bad angle is a bail. Land on a yellow rail to grind it.
typedef int32_t s32;
static void lifeMode(int ed);
static void mapEditor(void);
static u8 gInPlay;   // 1 while the life game runs (some actions only make sense, or are only safe, in one place)
static int edX0=0, edY0=0, edX1=9999, edY1=9999;   // where the room builder's cursor may go (neighborhood.h narrows it to the lot you are on)
static int nbPlaying;   // the game was started from the neighborhood: its pause menu goes back there
static int nbResetLot(void);
#define MW 40
#define MH 40    // keep MH == MW: the 4-way action cam rotates the square map
static int camX, camY, camSnap=1, camLastV;   // camera offset in px; the map's top corner is drawn at (120-camX, 24-camY)
#define LOX (120-camX)   // screen x of the map's top corner
#define LOY (24-camY)
#define OYCB 148            // the bake draws the feet here, so the capture window (80 x 150: SPW x SPH at 5 to 2) fits on the screen
#define SPX0 (OXC-40)
#define SPY0 (OYCB-SPF*5/2)   // capture window top; the feet land on row SPF of the sprite
#define MAPNAME "THE MAN BASE"   // name of the (placeholder) map
// w = low wall, W = wall, # = 2-block crate, = = grind rail, . = floor (the default map is built by mapGen below)
static const short cosT[16]={256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98,0,98,181,237};   // sin(a)=cosT[(a+12)&15]
static int spBx0, spBx1, spBy0, spBy1;
static s32 lfx,lfy,lz,lvz,lvx,lvy; static int lskate, lhave, lfr;   // lskate: 0 on foot, 1 skateboard; lhave: picked up the board
static u8 floorMap[MH][MW] EWRAM_BSS, wallMap[MH][MW] EWRAM_BSS;   // floor style and wallpaper per tile
static u8 wDirty;   // the map changed: the walls work out again which floor is inside a room (wallsScan)
static int lfpsV;   // measured frames per second (shown when SHOW FPS is on)
static char lifeMap[MH][MW+1] EWRAM_BSS;   // the room being played / edited (starts as mapDef, or the copy saved in SRAM)
static int bdx=10, bdy=4, spx=3, spy=6;   // skateboard tile and spawn tile, found by mapScan (B and P tiles)
#define BDX bdx
#define BDY bdy
static int lsp,lhd,lspin,lflip,lgrind,lscore,lstun,lairF,lpts,lnoteT,lglide; static const char*lnote;
static char lnBuf[24] EWRAM_BSS; static u8 lnPerf; static int lLand, lLandD;   // lnBuf: the name of the trick just landed ("KICK 360 GRAB"); lLand: frames of landing crouch left, lLandD: how far the fall was
static int stModal; static void stRunModal(void);   // story.h: a chapter card is waiting (1 chapter intro, 2 chapter done); lifeModeRun shows it like the pause menu
static int tutOn;   // 1 while the tutorial runs (tutorial.h): nobody dies, like the Sims 2 tutorial neighborhood

// SKATEBOARD ANIMATION: the board is drawn from these (set every picture by playerCalc, drawn by drawBoard under the sprite).
//   bdA heading + spin (256 = a turn), bdPitch nose up / down in px (ollie), bdRoll the flip (kickflip / heelflip roll about the long axis),
//   bdRaise a grab lifts it, bdSpk grind sparks; bFT / bFD count the flip (frames, +1 kickflip / -1 heelflip)
#define BFLIP_LEN 16
static int bFT, bFD, bFPrev, bdA, bdPitch, bdRoll, bdRaise, bdSpk;
static int lfood, lbl, lnear;   // hunger (100 = full), bladder (100 = bursting), what is in reach (1 fridge, 2 toilet)
static int lmaxz, lplay, ldead, lbumpCd;   // peak height this jump, air sound played, dead, bump cooldown
// HEALTH (HP, 0..100): the life meter. hurt() (falls, bails, wall hits) and punches (house.h) take it down, it creeps back while fed and
// standing, a meal gives a little and a night in bed a lot. At 0 from a fall you die; a punch only ever knocks you out (see fightHurt).
#define HP_MAX 100
#define HP_REGEN 90   // steps per +1 HP (1.5 s) while fed and not stunned: empty to full in about 2.5 minutes
static int lhp=HP_MAX;
static void hpHeal(int n){ lhp+=n; if(lhp>HP_MAX) lhp=HP_MAX; }
static void hpLose(int n){ lhp-=n; if(lhp<0) lhp=0; if(xo[XO_HURT]==2&&lhp<1) lhp=1; }   // HURT option NO DEATH: a fall can never empty it
#include "voices.h"   // the voice clips of the Sim you control (tools/encode_voices.py)
static void voxPlay(int v); static void voxNag(int v); static void voxChain(int a,int b,int c); static void voxEvent(int ev,int v);   // (defined after sfxPlay)
#include "mood.h"   // FUN + HAPPY meters: moodEvent(), moodTick(), moodTop(), moodPts()
#include "sims.h"   // life-sim layer: energy/hygiene/comfort, wants and fears, aspiration. simsTick(), simBegin(), simsHud()

// ---------- sound effects: 4-bit IMA-ADPCM @ 6554 Hz, mixed as one more voice by the music mixer (see the AUDIO notes further down) ----------
// source/sfx/*.adp (made by tools/encode_sfx.py) are baked into the ROM with .incbin; paths are relative to the project root.
// No RAM buffer: the mixer decodes a few samples ahead each frame, straight from the ROM, and resamples them to the mixer rate.
#define R_SNDCNT_L (*(volatile u16*)0x04000080)
#define R_SNDCNT_H (*(volatile u16*)0x04000082)
#define R_SNDCNT_X (*(volatile u16*)0x04000084)
#define R_DMA1SAD (*(volatile u32*)0x040000BC)
#define R_DMA1DAD (*(volatile u32*)0x040000C0)
#define R_DMA1CNT (*(volatile u32*)0x040000C4)
#define R_TM0D    (*(volatile u16*)0x04000100)
#define R_TM0CNT  (*(volatile u16*)0x04000102)
#define R_TM1D    (*(volatile u16*)0x04000104)
#define R_TM1CNT  (*(volatile u16*)0x04000106)
#define SFX_STEP 23655   // 6553.6 Hz source samples per 18157 Hz mixer sample, 16.16 fixed point
__asm__(".pushsection .rodata\n.balign 4\n"
 ".global sfx_hit\nsfx_hit:\n.incbin \"source/sfx/hit.adp\"\n.balign 4\n"
 ".global sfx_gasp\nsfx_gasp:\n.incbin \"source/sfx/gasp.adp\"\n.balign 4\n"
 ".global sfx_scream\nsfx_scream:\n.incbin \"source/sfx/scream.adp\"\n.balign 4\n"
 ".global sfx_cry\nsfx_cry:\n.incbin \"source/sfx/cry.adp\"\n.balign 4\n"
 ".global sfx_groan\nsfx_groan:\n.incbin \"source/sfx/groan.adp\"\n.balign 4\n"
 ".global sfx_instant\nsfx_instant:\n.incbin \"source/sfx/instant.adp\"\n.balign 4\n"
 ".global sfx_tick\nsfx_tick:\n.incbin \"source/sfx/tick.adp\"\n.balign 4\n"
 ".global sfx_pop\nsfx_pop:\n.incbin \"source/sfx/pop.adp\"\n.balign 4\n"
 ".global sfx_land\nsfx_land:\n.incbin \"source/sfx/land.adp\"\n.balign 4\n"
 ".global sfx_stick\nsfx_stick:\n.incbin \"source/sfx/stick.adp\"\n.balign 4\n"
 ".global sfx_grind\nsfx_grind:\n.incbin \"source/sfx/grind.adp\"\n.balign 4\n"
 ".global sfx_thunder\nsfx_thunder:\n.incbin \"source/sfx/thunder.adp\"\n.balign 4\n"
 ".global sfx_ghost\nsfx_ghost:\n.incbin \"source/sfx/ghost.adp\"\n.balign 4\n"
 ".popsection\n");
extern const u8 sfx_hit[],sfx_gasp[],sfx_scream[],sfx_cry[],sfx_groan[],sfx_instant[],sfx_tick[],sfx_pop[],sfx_land[],sfx_stick[],sfx_grind[],sfx_thunder[],sfx_ghost[];
enum { SFX_BONK, SFX_HIT, SFX_GASP, SFX_SCREAM, SFX_CRY, SFX_GROAN, SFX_NEARLY, SFX_DEATH, SFX_INSTANT, SFX_TICK, SFX_POP, SFX_LAND, SFX_STICK, SFX_GRIND, SFX_THUNDER, SFX_GHOST, SFX_VOICE0, SFX_N=SFX_VOICE0+VOICE_N };   // POP ollie, LAND a landing, STICK a trick landed, GRIND a rail caught (tools/make_skate_sfx.py)
#define VS(v) (SFX_VOICE0+(v))   // a voice clip's sound id (V_xxx from voices.h)
// effects that share a source file share one blob in the ROM
static const u8* const sfxTab[SFX_N]={ sfx_hit,sfx_hit,sfx_gasp,sfx_scream,sfx_cry,sfx_groan,sfx_scream,sfx_scream,sfx_instant,sfx_tick,sfx_pop,sfx_land,sfx_stick,sfx_grind,sfx_thunder,sfx_ghost, VOICE_TAB };
static const u16 stepT[89]={7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const signed char idxT[8]={-1,-1,-1,-1,2,4,6,8};
// The effect voice: ssrc/sn = the clip's nibbles and sample count, sPos + sFr/65536 = play position in clip samples, sRd = samples decoded so far,
// sS0/sS1 = the two decoded samples around sPos (for interpolation), spred/sidx = the ADPCM decoder. sfxV is cleared by the mixer at the end.
static const u8 *ssrc; static u32 sn, sPos, sFr, sRd; static int spred, sidx, sS0, sS1, sfxOn; static volatile int sfxV;
static volatile int sfxLoop, sfxFade=256, sfxFadeT=256;   // the effect voice can LOOP (the loading tick-tock) and has its own fade: sfxFade glides to sfxFadeT a little every frame (256 = full); a looping voice that has faded to 0 ends itself
static int gMusic;   // game music is switched on right now (an effect now plays over it instead of pausing it)
static volatile int mOn, mPlay;   // mOn: the mixer interrupts and sound DMA are running; mPlay: a song is part of the mix
static void audStart(void); static void audStop(void);
static void sfxStop(void){ vxQn=0; if(sfxLoop&&sfxV){ sfxFadeT=0; return; } sfxV=0; sfxOn=0; if(mOn&&!mPlay) audStop(); }   // (the loading tick-tock is never cut: it fades out and ends itself)
// ---------- tracker songs: note-based XM player (tools/xm2gba.py converts the .xm songs listed in songs.h) ----------
// A song is stored as notes (pattern/row/channel events, each with its own volume) plus small instrument samples (8-bit,
// band-limited and down-sampled in the converter). A 16-voice stereo mixer (each note has a pan bus, see tools/xm2gba.py) with linear interpolation renders 304 samples per frame
// into Direct Sound A (left, DMA1) and B (right, DMA2), both on Timer0, restarted every vblank. The title song plays its intro once, then orders loop.. repeat
// (loop = per-song loop order); the voices are never cut at the loop jump, so the last notes ring into the first ones.
typedef int8_t s8; typedef int16_t s16;
typedef struct {            // one converted tracker song (generated into musicdata.h)
    const u8*order; const u16*rows; const u32*patOff; const u8*ev;    // order list, rows per pattern, pattern start (byte offset) in ev, the note-event byte stream
    const u16*vt; const u32*anc; const u8*fx;                         // voice table (channel | instrument<<4 | pan bus<<9), per-instrument pitch anchor, pitch fix-ups
    const u32*len; const s8*const*data;                               // per instrument: sample length, sample data
    const u8*busL; const u8*busR;                                     // 7 pan buses: left / right gain (128 = 1.0), balanced per song by the converter
    int nord, loop, rowN, rfr;                                        // orders in the song, loop order, samples per row (+ fraction/256)
} XmSong;
#include "musicdata.h"
#define R_DMA2SAD (*(volatile u32*)0x040000C8)
#define R_DMA2DAD (*(volatile u32*)0x040000CC)
#define R_DMA2CNT (*(volatile u32*)0x040000D0)
#define MUS_N 304   // samples per frame at 18157 Hz (924 cycles each = exactly one frame)
#define MUS_VOICES 16   // tracker channels the mixer can play at once
typedef struct { const s8*d; u32 pos,step,len; int vl,vr,ol,orr; } MVoice;   // vl / vr = note volume x left / right pan-bus gain; len = samples left from d
// (d moves forward as a note plays, so pos (16.16) never needs more than 16 whole bits: samples longer than 65535 frames play to the end)
// ol / orr: DECLICK. A new note on a channel used to cut the old one mid-wave (a click). The jump between the old note's level and the new one's
// first sample is kept as an offset (x32) that is added to the mix and fades out over ~2 ms, so the wave never steps.
static MVoice mvc[MUS_VOICES];
// STEREO: Direct Sound A plays the left buffers, Direct Sound B the right ones; both are fed by Timer0 and restarted together at vblank.
static s8 mbufL[2][MUS_N] __attribute__((aligned(4))), mbufR[2][MUS_N] __attribute__((aligned(4)));
static s16 maccL[MUS_N], maccR[MUS_N];
static s8 mDly[256] __attribute__((aligned(4))); static int mDp, mLp;   // pseudo-stereo for streamed songs: 256-sample (14 ms) delay line + a low-pass state that keeps the bass centred
static int mOrd, mRow, mLeft, mFrac; static volatile int mCur, mFilled; static const XmSong*mSong;
static volatile int ldG=256, ldGT=256; static volatile int ldHold, ldRel;   // LOADING: the song steps aside while a slow job runs (loading.h). ldG = its own gain (256 full) gliding to ldGT; at 0 the song is FROZEN, not mixed at all (that is the CPU the job gets back). ldHold = a song is stepped aside; ldRel = frames until it comes back
static volatile int mGain=256, mGainT=256;   // music loudness 256 = full; mGain glides to mGainT a little every frame (half while a menu is open)
static int mKind, aTail; static volatile int mLaps, mDone;   // mKind 0 = tracker song, 1 = streamed ADPCM; mLaps = times the tracker song has wrapped; mDone = ADPCM song finished
static int aSlow, aPrv, aPh; static const u8 *aSrc; static u32 aN, aPos; static int aPred, aIdx;   // ADPCM stream: data, sample count, position, decoder state
static int aLoop, aPred0, aIdx0;   // a looping stream (the creator's chiptunes): it wraps to the start with the decoder state it was encoded from
// Playback step (16.16) of note nt (0..95) of instrument in: the instrument's anchor x 2^(nt/12) in integer maths (xmT = 2^(j/12) in Q30), plus the
// converter's rare +-1 fix-ups, so every step is exactly what the old 96-entry table per instrument held (tools/xm2gba.py checks that).
static u32 xmStep(const XmSong*s,int in,int nt){
    int q=(nt*683)>>13, j=nt-12*q, sh=46-q;   // q = nt/12 (exact for 0..95)
    u32 st=(u32)(((unsigned long long)s->anc[in]*xmT[j]+(1ull<<(sh-1)))>>sh);
    for(const u8*f=s->fx;*f!=255;f+=3) if(f[0]==in&&f[1]==nt) st+=(u32)(int)(s8)f[2];
    return st;
}
// Note events (tools/xm2gba.py): per pattern, row by row. A byte >=0x80 = that many (low 7 bits) empty rows; otherwise it is the number of events in
// the row, each 3 bytes (24 bits, little endian): voice index low 8 bits | note<<8 (7 bits) | volume<<15 (7 bits) | voice index high 2 bits<<22.
// The voice index picks a vt[] entry: channel(4) | instrument(5)<<4 | pan bus(3)<<9.
static void musTrigger(void){
    const XmSong*s=mSong; const u8*e=&s->ev[s->patOff[s->order[mOrd]]]; int r=mRow, n;
    for(;;){ int h=*e++;
        if(h&0x80){ h&=0x7F; if(r<h){ n=0; break; } r-=h; }
        else if(r--==0){ n=h; break; }
        else e+=3*h; }
    while(n--){ u32 w=e[0]|((u32)e[1]<<8)|((u32)e[2]<<16); e+=3;
        u32 t=s->vt[(w&255)|((w>>22)<<8)]; int ch=t&15, in=(t>>4)&31, nt=(w>>8)&127, vol=(w>>15)&127;   // channel, instrument, note, voice volume
        int bus=(t>>9)&7; if(bus>6) bus=3;   // pan bus 0 = hard left .. 3 = centre .. 6 = hard right
        MVoice*v=&mvc[ch]; int vl=vol*s->busL[bus], vr=vol*s->busR[bus];
        { int oL=0, oR=0;   // where the old note is right now
          if(v->d&&(v->pos>>16)<v->len){ const s8*d=v->d; int ix=(int)(v->pos>>16), fr=(int)((v->pos>>8)&255), x=d[ix]*256+(d[ix+1]-d[ix])*fr; oL=(x*v->vl)>>21; oR=(x*v->vr)>>21; }
          int x0=s->data[in][0]*256; v->ol+=(oL-((x0*vl)>>21))<<5; v->orr+=(oR-((x0*vr)>>21))<<5; }
        v->d=s->data[in]; v->pos=0; v->step=xmStep(s,in,nt); v->len=s->len[in]; v->vl=vl; v->vr=vr; }
}
// SOFT LIMIT: past +-96 the output bends smoothly towards the 8-bit edge instead of being cut flat there (a flat cut crackles). The curve is
// 96 + d*R/(d+R) (d = how far past 96, R = room left), so its slope is 1 at the knee and it never quite reaches the edge. tools/preview_xm.py: the same.
static inline __attribute__((always_inline)) int softClip(int x){
    if(x>96){ int d=x-96; return 96+d*31/(d+31); }
    if(x<-96){ int d=-96-x; return -96-d*32/(d+32); }
    return x;
}
IWRAM_ARM static void musMix(s8*outL,s8*outR){
    int done=0;
    while(done<MUS_N){
        if(mLeft==0){ musTrigger(); mFrac+=mSong->rfr; mLeft=mSong->rowN+(mFrac>>8); mFrac&=255;
            if(++mRow>=mSong->rows[mSong->order[mOrd]]){ mRow=0; if(++mOrd>=mSong->nord){ mOrd=mSong->loop; mLaps++; } } }
        int n=MUS_N-done; if(n>mLeft) n=mLeft;
        s16*a=maccL+done; s16*b=maccR+done;
        for(int i=0;i<n;i++){ a[i]=0; b[i]=0; }
        for(int vi=0;vi<MUS_VOICES;vi++){ MVoice*v=&mvc[vi]; if(!v->d) continue;
            u32 pos=v->pos, st=v->step; const s8*d=v->d; int vl=v->vl, vr=v->vr, i=0;
            if(pos>>16){ u32 k=pos>>16; d+=k; v->len-=k; pos&=0xFFFF; v->d=d; }   // keep pos small (see MVoice)
            u32 len=(v->len>0xFFFF?0xFFFFu:v->len)<<16;   // a frame moves a note far less than 65535 samples, so the cap never stops one early
            for(;i<n;i++){
                if(pos>=len){ v->d=0; break; }
                int ix=(int)(pos>>16), fr=(int)((pos>>8)&255), x0=d[ix], x1=d[ix+1];
                int x=x0*256+(x1-x0)*fr;                      // one interpolated sample, 16-bit scale
                a[i]=(s16)(a[i]+((x*vl)>>21)); b[i]=(s16)(b[i]+((x*vr)>>21)); pos+=st; }   // (>>21 = the old >>14 with the 1/128 bus gain folded in)
            v->pos=pos; }
        for(int vi=0;vi<MUS_VOICES;vi++){ MVoice*v=&mvc[vi]; int ol=v->ol, orr=v->orr; if(!(ol|orr)) continue;   // declick offsets fading out
            for(int i=0;i<n;i++){ a[i]=(s16)(a[i]+(ol>>5)); b[i]=(s16)(b[i]+(orr>>5)); ol-=ol>>5; orr-=orr>>5; }
            if(ol>-32&&ol<32) ol=0; if(orr>-32&&orr<32) orr=0; v->ol=ol; v->orr=orr; }
        mLeft-=n; done+=n;
    }
    for(int i=0;i<MUS_N;i++) outL[i]=(s8)softClip(maccL[i]>>2), outR[i]=(s8)softClip(maccR[i]>>2);
}
// Streamed ADPCM song (source/music/*.adp from tools/encode_song.py): 4-bit IMA-ADPCM, 18157 Hz, so one frame = 304 samples.
// Same format as the sound effects: u32 sample count, then nibbles (low first). Decoded straight into the DMA buffer.
IWRAM_ARM static void pseudoSt(s8*out,s8*outR);
IWRAM_ARM static void adpMix(s8*out,s8*outR){
    // bit 31 of the sample count = song stored at 2/3 rate (12105 Hz): every 2 stored samples become 3 output samples (linear interpolation)
    u32 p=aPos, e=aN; int pred=aPred, idx=aIdx, i=0; const u8*d=aSrc;
    int prv=aPrv, ph=aPh;
    for(;i<MUS_N;i++){
        if(aSlow){
            ph+=2;
            while(ph>=3){ if(p>=e){ if(!aLoop) break; p=0; pred=aPred0; idx=aIdx0; } ph-=3; prv=pred;
                int v=d[p>>1]; v=(p&1)?(v>>4):(v&15); p++;
                int step=stepT[idx], diff=step>>3;
                if(v&1) diff+=step>>2; if(v&2) diff+=step>>1; if(v&4) diff+=step;
                pred+=(v&8)?-diff:diff; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768;
                idx+=idxT[v&7]; if(idx<0) idx=0; if(idx>88) idx=88; }
            if(p>=e&&ph>=3) break;
            out[i]=(s8)((prv+(((pred-prv)*ph)/3))>>8);
        } else {
            if(p>=e){ if(!aLoop) break; p=0; pred=aPred0; idx=aIdx0; }
            int v=d[p>>1]; v=(p&1)?(v>>4):(v&15); p++;
            int step=stepT[idx], diff=step>>3;
            if(v&1) diff+=step>>2; if(v&2) diff+=step>>1; if(v&4) diff+=step;
            pred+=(v&8)?-diff:diff; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768;
            idx+=idxT[v&7]; if(idx<0) idx=0; if(idx>88) idx=88;
            out[i]=(s8)(pred>>8);
        }
    }
    for(;i<MUS_N;i++) out[i]=0;
    aPos=p; aPred=pred; aIdx=idx; aPrv=prv; aPh=ph;
    pseudoSt(out,outR);
    if(p>=e&&i<MUS_N&&++aTail>=3) mDone=1;   // 2 buffers are in flight, so wait for the last real samples to be heard
}
// Pseudo-stereo for a mono stream (complementary comb): L = 0.75x + 0.5z, R = 0.75x - 0.5z, where z is the high part of x delayed by 14 ms. L+R is
// exactly the original mono signal (so it also sounds right on the GBA's mono speaker); the ears get different comb patterns = width.
// The one-pole low-pass is subtracted from z so bass and kick stay in the middle.
IWRAM_ARM static void pseudoSt(s8*out,s8*outR){
    int dp=mDp, lp=mLp;
    for(int k=0;k<MUS_N;k++){ int x=out[k], z=mDly[dp]; mDly[dp]=(s8)x; dp=(dp+1)&255;
        lp+=(z*16-lp)>>3; int h=z-(lp>>4);                 // lp holds the low-passed delayed signal x16
        int l=(x*12+h*8)>>4, r=(x*12-h*8)>>4;
        out[k]=(s8)(l>127?127:l<-128?-128:l); outR[k]=(s8)(r>127?127:r<-128?-128:r); }
    mDp=dp; mLp=lp;
}
IWRAM_ARM static void sfxMix(s8*outL,s8*outR){   // add the effect voice to a finished buffer (both sides), clipped
    int fg=sfxFade;   // the voice's own fade (loading tick-tock): 8/256 a frame up, 16/256 down
    if(fg!=sfxFadeT){ fg+=(sfxFadeT>fg)?8:-16; if((sfxFadeT>sfxFade)?fg>sfxFadeT:fg<sfxFadeT) fg=sfxFadeT; sfxFade=fg;
        if(fg==0&&sfxFadeT==0&&sfxLoop){ sfxV=0; sfxLoop=0; return; } }
    const u8*d=ssrc; u32 n=sn, ip=sPos, fr=sFr, rd=sRd; int pred=spred, idx=sidx, s0=sS0, s1=sS1, sgain=(oSfxGain()*fg)>>8;   // SFX VOLUME and MASTER VOLUME options (x the voice's fade: 256 = unchanged)
    for(int i=0;i<MUS_N;i++){
        if(ip>=n){ if(!sfxLoop){ sfxV=0; break; } ip=0; rd=0; pred=0; idx=0; s0=s1=0; }   // a looping voice starts over (decoder and all; the loop is silent at its ends)
        while(rd<ip+2){   // decode up to the sample after ip (silence past the end)
            s0=s1;
            if(rd<n){ int v=d[rd>>1]; v=(rd&1)?(v>>4):(v&15);
                int step=stepT[idx], diff=step>>3;
                if(v&1) diff+=step>>2; if(v&2) diff+=step>>1; if(v&4) diff+=step;
                pred+=(v&8)?-diff:diff; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768;
                idx+=idxT[v&7]; if(idx<0) idx=0; if(idx>88) idx=88; s1=pred; }
            else s1=0;
            rd++; }
        int x=((s0+(((s1-s0)*(int)fr)>>16))*sgain)>>16;
        outL[i]=(s8)softClip(outL[i]+x); outR[i]=(s8)softClip(outR[i]+x);   // (the soft limit: an effect over a loud song bends, never cuts)
        fr+=SFX_STEP; ip+=fr>>16; fr&=0xFFFF;
    }
    sPos=ip; sFr=fr; sRd=rd; spred=pred; sidx=idx; sS0=s0; sS1=s1;
}
// ---- the creator's chiptune loops, played LIVE from note data (tools/chip_synth.py, which also holds an exact twin of chipMix) ----
// Four NES-style voices: two pulses and a triangle read 256-step wave tables that hold exactly the harmonics below 7.5 kHz for their pitch,
// the noise reads a band-limited recording of the NES noise at the drum's clock; two tables then apply the NES APU's non-linear mixer, a one-
// pole 40 Hz low cut follows, and the loop's gain. The note data changes the voices 120 times a second (a flags byte per step, see chip_synth.py).
#include "chipsyn.h"
__asm__(".pushsection .rodata\n.balign 4\n.global chipsyn\nchipsyn:\n.incbin \"source/music/chipsyn.bin\"\n.balign 4\n.popsection\n");
extern const u8 chipsyn[];
static const u32 csNzOff[CS_NNZ]=CS_NZ, csTabOff[3]=CS_TAB; static const u8 csHmin[3]=CS_HMIN, csHmax[3]=CS_HMAX;
typedef struct { const u8*s,*d; int nl,step,rep,acc,left; u32 ph[3],inc[3]; const s8*tab[3]; int lv[3],nlv,np,lp,g; const s8*nb; } CSyn;
static CSyn csy;   // the main deck's synth (deckSwap trades it with the other deck's)
static const s8* csTab(int v,u32 inc){ int h=inc?(int)(CS_KH/inc):127; if(h<1) h=1; if(h>127) h=127; if(h<csHmin[v]) h=csHmin[v]; if(h>csHmax[v]) h=csHmax[v];
    return (const s8*)(chipsyn+csTabOff[v]+(u32)(h-csHmin[v])*256); }
static void csInit(const u8*loop){   // header: u16 steps, u16 0, s32 low-cut state the loop starts from, u32 gain (Q20); then the steps
    CSyn*c=&csy; c->s=loop+12; c->d=c->s; c->nl=*(const u16*)loop; c->lp=*(const int*)(loop+4); c->g=(int)*(const u32*)(loop+8);
    c->step=0; c->rep=0; c->acc=0; c->left=0; c->nlv=0; c->np=0; c->nb=(const s8*)(chipsyn+csNzOff[0]);
    for(int v=0;v<3;v++){ c->ph[v]=0; c->inc[v]=0; c->lv[v]=0; c->tab[v]=csTab(v,0); }
}
__attribute__((noinline,long_call)) static void csStep(CSyn*c){   // the next control step (ROM, called from chipMix)
    if(c->step>=c->nl){ c->step=0; c->d=c->s; c->rep=0; }   // the loop starts again (the voices just carry on)
    c->step++;
    if(c->rep) c->rep--;
    else { int fl=*c->d++;
        if(fl&0x80) c->rep=fl&0x7F;
        else { for(int v=0;v<3;v++){
                   if(fl&(1<<(2*v))){ int cd=c->d[0]|(c->d[1]<<8); c->d+=2; c->inc[v]=cd?(u32)(((unsigned long long)csInc[cd>>8]*csFine[cd&255])>>15):0; c->tab[v]=csTab(v,c->inc[v]); }
                   if(fl&(2<<(2*v))) c->lv[v]=*c->d++; }
               if(fl&0x40){ int b=*c->d++; c->nlv=b&15; c->nb=(const s8*)(chipsyn+csNzOff[b>>4]); } } }
    int t=c->acc+37; c->left=151+(t>=120); c->acc=t>=120?t-120:t;   // 18157/120 = 151 r 37: 151 or 152 samples a step, exact on average
}
IWRAM_ARM static void chipMix(s8*out,s8*outR){   // in passes over maccL / maccR (free while this runs): few live values, no spills
    CSyn*c=&csy; int i=0;
    while(i<MUS_N){
        if(!c->left) csStep(c);
        int n=MUS_N-i; if(n>c->left) n=c->left;
        s16*P=maccL+i,*U=maccR+i;
        { u32 ph=c->ph[0],in=c->inc[0]; const s8*t=c->tab[0]; int l=c->lv[0];                        // pulse 1
          if(l){ _Pragma("GCC unroll 2") for(int k=0;k<n;k++){ ph+=in; P[k]=(s16)(t[ph>>24]*l); } } else { for(int k=0;k<n;k++) P[k]=0; ph+=in*(u32)n; } c->ph[0]=ph; }
        { u32 ph=c->ph[1],in=c->inc[1]; const s8*t=c->tab[1]; int l=c->lv[1];                        // pulse 2
          if(l){ _Pragma("GCC unroll 2") for(int k=0;k<n;k++){ ph+=in; P[k]=(s16)(P[k]+t[ph>>24]*l); } } else ph+=in*(u32)n; c->ph[1]=ph; }
        { u32 ph=c->ph[2],in=c->inc[2]; const s8*t=c->tab[2];                                        // triangle (level x the APU weight)
          if(c->lv[2]){ _Pragma("GCC unroll 2") for(int k=0;k<n;k++){ ph+=in; U[k]=(s16)((t[ph>>24]*CS_TRIMUL)>>4); } } else { for(int k=0;k<n;k++) U[k]=0; ph+=in*(u32)n; } c->ph[2]=ph; }
        { int np=c->np, l=c->nlv; const s8*nb=c->nb;                                                  // noise
          if(l){ _Pragma("GCC unroll 2") for(int k=0;k<n;k++){ U[k]=(s16)(U[k]+nb[np]*l); np=(np+1)&(CS_NB-1); } } else np=(np+n)&(CS_NB-1); c->np=np; }
        c->left-=n; i+=n;
    }
    { const s16*PM=(const s16*)(chipsyn+CS_PM),*TM=(const s16*)(chipsyn+CS_TM); int lp=c->lp, g=c->g;   // the APU mixer, the low cut, the gain
      _Pragma("GCC unroll 2") for(int k=0;k<MUS_N;k++){
          int x4=(PM[(maccL[k]+CS_POFF)>>2]+TM[(maccR[k]+CS_UOFF)>>2])<<4; lp+=((x4-lp)*CS_HPK)>>16;   // (the tables cover every value a voice can make)
          int y=(int)(((long long)(x4-lp)*g)>>20); out[k]=(s8)(y>127?127:y<-128?-128:y); }
      c->lp=lp; }
#ifdef CS_TEST
    { extern u8* csTestP; for(int k=0;k<MUS_N;k++) *csTestP++=(u8)out[k]; }   // (test build: the mono output, before the pseudo-stereo)
#endif
    pseudoSt(out,outR);
}

// ---- CROSSFADE: a second deck ----
// Everything a song keeps between frames (musMix / adpMix state) can be put aside in xdk and a new song started in the globals. While xfOn the interrupt
// mixes BOTH songs every frame (the old one by swapping its state in and out for a moment) and blends them equal-power over xfN frames: the new song
// rises, the old one falls. With no new song (fade out) the main deck is just silent. Asking for another crossfade while one runs drops the older song.
typedef struct {
    const XmSong*song; int ord,row,left,frac; MVoice vc[MUS_VOICES]; int kind,tail,laps,done,play;
    int aSlow,aPrv,aPh; const u8*aSrc; u32 aN,aPos; int aPred,aIdx,aLoop,aPred0,aIdx0; s8 dly[256]; int dp,lp; CSyn cs;
} MDeck;
static MDeck xdk EWRAM_BSS; static s8 xbufL[MUS_N] EWRAM_BSS, xbufR[MUS_N] EWRAM_BSS;
static volatile int xfOn, xfT, xfN, xdkG=256;   // crossfade running, frames done, frames in all, the old song's gain when it was put aside (256 = full)
static const u16 xfCurve[17]={0,25,50,74,98,121,142,162,181,198,213,226,237,245,251,255,256};   // sin(90 deg x k/16) x 256: equal power
static int xfGain(int t,int n){ int p=t*256/n; if(p<0) p=0; if(p>256) p=256; int i=p>>4, f=p&15, a=xfCurve[i], c=xfCurve[i<16?i+1:16]; return a+(((c-a)*f)>>4); }
#define XSW(T,A,B) { T t_=A; A=B; B=t_; }
static void deckSwap(MDeck*d){   // exchange the main deck (the globals) with d
    XSW(const XmSong*,mSong,d->song) XSW(int,mOrd,d->ord) XSW(int,mRow,d->row) XSW(int,mLeft,d->left) XSW(int,mFrac,d->frac) XSW(int,mKind,d->kind) XSW(int,aTail,d->tail)
    XSW(int,aSlow,d->aSlow) XSW(int,aPrv,d->aPrv) XSW(int,aPh,d->aPh) XSW(const u8*,aSrc,d->aSrc) XSW(u32,aN,d->aN) XSW(u32,aPos,d->aPos) XSW(int,aPred,d->aPred) XSW(int,aIdx,d->aIdx) XSW(int,aLoop,d->aLoop) XSW(int,aPred0,d->aPred0) XSW(int,aIdx0,d->aIdx0)
    XSW(int,mDp,d->dp) XSW(int,mLp,d->lp) XSW(CSyn,csy,d->cs)
    { int t=mLaps; mLaps=d->laps; d->laps=t; t=mDone; mDone=d->done; d->done=t; t=mPlay; mPlay=d->play; d->play=t; }
    { u32*a=(u32*)mvc,*b=(u32*)d->vc; for(unsigned i=0;i<sizeof(mvc)/4;i++){ u32 t=a[i]; a[i]=b[i]; b[i]=t; } }
    { u32*a=(u32*)mDly,*b=(u32*)d->dly; for(int i=0;i<64;i++){ u32 t=a[i]; a[i]=b[i]; b[i]=t; } }
}
IWRAM_ARM static void musMixAny(int b){
    if(!mPlay&&!xfOn){ for(int i=0;i<MUS_N;i++){ mbufL[b][i]=0; mbufR[b][i]=0; } if(sfxV) sfxMix(mbufL[b],mbufR[b]); else mWantOff=1; return; }   // only an effect (or nothing: switch the mixer off)
    if(ldG==0&&ldGT==0&&!xfOn){ for(int i=0;i<MUS_N;i++){ mbufL[b][i]=0; mbufR[b][i]=0; } if(sfxV) sfxMix(mbufL[b],mbufR[b]); return; }   // the song is stepped aside for a loading screen: frozen where it is, no decoding at all (only the tick-tock is mixed)
    if(mPlay){ if(mKind==2) chipMix(mbufL[b],mbufR[b]); else if(mKind) adpMix(mbufL[b],mbufR[b]); else musMix(mbufL[b],mbufR[b]); }
    else for(int i=0;i<MUS_N;i++){ mbufL[b][i]=0; mbufR[b][i]=0; }
    if(xfOn){   // blend the new song (main deck) with the old one
        int n=xfN>0?xfN:1, gi=xfGain(xfT,n), go=(xfGain(n-xfT,n)*xdkG)>>8;
        if(xdk.play){
            deckSwap(&xdk); if(mKind==2) chipMix(xbufL,xbufR); else if(mKind) adpMix(xbufL,xbufR); else musMix(xbufL,xbufR); deckSwap(&xdk);
            for(int i=0;i<MUS_N;i++){ int l=(mbufL[b][i]*gi+xbufL[i]*go)>>8, r=(mbufR[b][i]*gi+xbufR[i]*go)>>8;
                mbufL[b][i]=(s8)(l>127?127:l<-128?-128:l); mbufR[b][i]=(s8)(r>127?127:r<-128?-128:r); }
        } else if(gi<256) for(int i=0;i<MUS_N;i++){ mbufL[b][i]=(s8)((mbufL[b][i]*gi)>>8); mbufR[b][i]=(s8)((mbufR[b][i]*gi)>>8); }
        if(++xfT>=xfN){ xfOn=0; xdk.play=0; xdkG=256; }
    }
    int mg0=oMusGain();   // MUSIC VOLUME and MASTER VOLUME options (sliders, 0..256)
    if(mg0==0){ for(int i=0;i<MUS_N;i++){ mbufL[b][i]=0; mbufR[b][i]=0; } }
    else if(mg0<256){ for(int i=0;i<MUS_N;i++){ mbufL[b][i]=(s8)((mbufL[b][i]*mg0)>>8); mbufR[b][i]=(s8)((mbufR[b][i]*mg0)>>8); } }
    if(mGain!=mGainT){ int g=mGain+((mGainT>mGain)?16:-16); if((mGainT>mGain)?g>mGainT:g<mGainT) g=mGainT; mGain=g; }   // fade: 16 steps of 1/16 per frame
    if(mGain<256){ int g=mGain; for(int i=0;i<MUS_N;i++){ mbufL[b][i]=(s8)((mbufL[b][i]*g)>>8); mbufR[b][i]=(s8)((mbufR[b][i]*g)>>8); } }
    if(ldG!=ldGT){ int g=ldG+((ldGT>ldG)?6:-8); if((ldGT>ldG)?g>ldGT:g<ldGT) g=ldGT; ldG=g; }   // LOADING fade: out 8/256 a frame (~0.5 s), back in 6/256 (~0.7 s)
    if(ldG<256){ int g=ldG; for(int i=0;i<MUS_N;i++){ mbufL[b][i]=(s8)((mbufL[b][i]*g)>>8); mbufR[b][i]=(s8)((mbufR[b][i]*g)>>8); } }
    if(sfxV) sfxMix(mbufL[b],mbufR[b]);   // an effect plays on top of the song (it used to pause it)
}
// ---- Audio is driven by interrupts, NOT by the main loop ----
// Old design: the main loop mixed one buffer per frame right after vsync. Any frame whose drawing ran long (jukebox list
// redraw, equalizer...) missed the next vblank, so the DMA ran dry (crackle) and the song fell a frame behind (timing lag).
// Now: VBlank IRQ (line 160) only restarts the sound DMA on the buffer that is already filled (a few dozen cycles);
// VCount IRQ (line 0) mixes the next buffer. The song clock is therefore locked to the hardware, whatever the main loop does.
#define R_IE  (*(volatile u16*)0x04000200)
#define R_IF  (*(volatile u16*)0x04000202)
#define R_IME (*(volatile u16*)0x04000208)
#define R_DISPSTAT (*(volatile u16*)0x04000004)
#define R_IRQVEC (*(volatile u32*)0x03007FFC)
u32 irqStack[256] __attribute__((aligned(8)));   // private IRQ stack (the BIOS one is only 160 bytes)
extern void irqEntry(void);
__asm__(".pushsection .iwram,\"ax\",%progbits\n.arm\n.align 2\n.global irqEntry\nirqEntry:\n"
        "  push {r4-r11,lr}\n  mov r4,sp\n  ldr r0,=irqStack+1024\n  mov sp,r0\n  bl irqMain\n  mov sp,r4\n  pop {r4-r11,lr}\n  bx lr\n"
        ".ltorg\n.popsection\n");
__attribute__((used)) IWRAM_ARM void irqMain(void){
    u16 f=R_IF;
    if(f&1){   // vblank: start the buffer that was filled last frame, in step with the screen
        R_IF=1;
        if(zoomShow&&zoomDma) zoomArm();   // the ZOOM's per-line scaling, every frame
        if(mOn){
            if(!mFilled) mCur^=1;                      // (mix overran: replay the last buffer rather than a half-filled one)
            R_DMA1CNT=0; R_DMA2CNT=0; R_TM0CNT=0;
            R_DMA1SAD=(u32)(uintptr_t)mbufL[mCur]; R_DMA1DAD=0x040000A0u;   // left  -> Direct Sound A
            R_DMA2SAD=(u32)(uintptr_t)mbufR[mCur]; R_DMA2DAD=0x040000A4u;   // right -> Direct Sound B
            R_DMA1CNT=0xB6400000u; R_DMA2CNT=0xB6400000u;                   // enable, FIFO timing, repeat, 32-bit, fixed dest
            R_TM0D=(u16)(65536-924); R_TM0CNT=0x80;
            mCur^=1; mFilled=0;                        // mCur is now the idle buffer
        }
    }
    if(f&4){   // line 0: render the idle buffer, it plays at the next vblank
        R_IF=4;
        if(mOn&&!mFilled){ musMixAny(mCur); mFilled=1; }
    }
}
static void zoomIrq(void){ R_IRQVEC=(u32)(uintptr_t)irqEntry; R_DISPSTAT|=0x0008; R_IE|=1; R_IME=1; }   // the vblank IRQ on (for the ZOOM, also without sound)
static void irqOff(void){ R_IME=0; R_IE=0; R_DISPSTAT=0; R_IF=0xFFFF; if(zoomShow) zoomIrq(); }
// AUDIO: one mixer for everything. While a song or an effect plays, the interrupts above run it; with neither, they are switched off.
static void audStart(void){   // start the mixer (the caller has set up what plays)
    irqOff(); R_DMA1CNT=0; R_DMA2CNT=0; R_TM0CNT=0;
    mCur=0; musMixAny(0);   // buffer 0 is primed here and plays at the first vblank; the line-0 IRQ then renders buffer 1
    mFilled=1; mOn=1;
    R_SNDCNT_X=0x80; R_SNDCNT_L=0; R_SNDCNT_H=0x9A0C;   // stereo: Direct Sound A -> left only, B -> right only, both 100%, Timer0, FIFOs reset
    R_IRQVEC=(u32)(uintptr_t)irqEntry;
    R_DISPSTAT=0x0028;                // vblank IRQ (bit 3) + vcount IRQ (bit 5) at line 0
    R_IF=0xFFFF; R_IE=5; R_IME=1;
}
static void audStop(void){ irqOff(); mOn=0; R_DMA1CNT=0; R_DMA2CNT=0; R_TM0CNT=0; R_SNDCNT_H=0; }
// Start a song: kind 0 = the tracker song xm, kind 1 = the ADPCM data in adp, kind 2 = a chiptune loop (synth data in adp).
static void deckInit(int kind,const u8*adp,const XmSong*xm){   // set the main deck up for a song (the sound hardware is not touched)
    for(int i=0;i<MUS_VOICES;i++){ mvc[i].d=0; mvc[i].ol=mvc[i].orr=0; }
    mOrd=0; mRow=0; mLeft=0; mFrac=0; mLaps=0; mDone=0; aTail=0; mKind=kind; mSong=xm; mDp=0; mLp=0; for(int i=0;i<256;i++) mDly[i]=0;
    aLoop=0; aPred0=0; aIdx0=0;
    if(kind==2) csInit(adp);
    else if(kind){ u32 n0=*(const u32*)adp; aSlow=(int)(n0>>31); aLoop=(int)((n0>>30)&1); aN=n0&0x3FFFFFFFu; aSrc=adp+4; aPrv=0; aPh=0; aPos=0;
        if(aLoop){ u32 st=((const u32*)adp)[1]; aSrc=adp+8; aPred0=(s16)(st&0xFFFF); aIdx0=(int)((st>>16)&0xFF); }   // loop header: count|bit 30, then pred | idx<<16
        aPred=aPred0; aIdx=aIdx0; }
    mPlay=1;
}
// A song that is stepped aside for a loading screen (ldHold) and gets replaced or stopped by someone else is simply dropped: nothing of it comes back.
static void ldDrop(void){
    if(!ldHold) return;
    u16 ime=R_IME; R_IME=0;
    ldHold=0; ldRel=0; ldG=ldGT=256; xfOn=0; xdk.play=0; xdkG=256; mPlay=0;
    R_IME=ime;
}
static void musBeginRaw(int kind,const u8*adp,const XmSong*xm){
    irqOff(); mOn=0; R_DMA1CNT=0; R_DMA2CNT=0; R_TM0CNT=0;
    sfxV=0; sfxOn=0; sfxLoop=0; mCur=0; mFilled=0;
    deckInit(kind,adp,xm); audStart();
}
static void musBegin(int kind,const u8*adp,const XmSong*xm){ ldDrop(); xfOn=0; xdk.play=0; xdkG=256; musBeginRaw(kind,adp,xm); }   // a hard start: whatever played is cut
static void musStart(void){ musBegin(0,0,&xm_the_dipper_man); }   // the title music
static void musKick(void){}   // (kept so old call sites still compile: the interrupts do this now)
static void musFill(void){}
static void musStop(void){ ldDrop(); xfOn=0; xdk.play=0; xdkG=256; mPlay=0; if(mOn&&!sfxV) audStop(); }   // a hard stop. An effect still sounding keeps the mixer going (sfxTick stops it after)
static void audIdleStop(void){ mWantOff=0; if(mOn&&!mPlay&&!xfOn&&!sfxV) audStop(); }
// CROSSFADE to a new song: the one playing carries on under a falling gain while the new one rises (frames of 1/60 s). With nothing playing the song just fades in.
#define XF_SONG 90   // song to song: 1.5 s
#define XF_SCREEN 60 // from one screen's music to the next's
#define XF_OUT 45    // out to silence
static void musFadeTo(int kind,const u8*adp,const XmSong*xm,int frames){
    ldDrop();
    if(!mOn){ xdk.play=0; xdkG=256; xfT=0; xfN=frames; xfOn=1; musBeginRaw(kind,adp,xm); return; }   // (xfOn is set before the first buffer is mixed)
    u16 ime=R_IME; R_IME=0;   // the sound interrupts must not run while the songs are swapped over
    int g=xfOn?xfGain(xfT,xfN>0?xfN:1):256;   // a crossfade still running: the song that was rising starts its fall from where it got to
    deckSwap(&xdk); deckInit(kind,adp,xm); xdkG=g; xfT=0; xfN=frames; xfOn=1;
    R_IME=ime;
}
static void musFadeOut(int frames){   // fade the playing song out to silence (the mixer switches itself off when it is done)
    ldDrop();
    if(!mOn||!mPlay) return;
    u16 ime=R_IME; R_IME=0;
    int g=xfOn?xfGain(xfT,xfN>0?xfN:1):256;
    deckSwap(&xdk); mPlay=0; xdkG=g; xfT=0; xfN=frames; xfOn=1;
    R_IME=ime;
}
// Will the song in the main deck be over within `frames` frames? (the menus start the next song's crossfade shortly before the end)
static int musNearEnd(int frames){
    if(!mPlay) return 0;
    if(mKind==2) return 0;   // a chiptune loop never ends
    if(mKind) return !aLoop&&(mDone||(aSlow?(long)(aN-aPos)*3/2:(long)(aN-aPos))<(long)frames*MUS_N);   // (a slow song stores 2 samples for every 3 it plays; a loop never ends)
    const XmSong*s=mSong; long left=(long)s->rows[s->order[mOrd]]-mRow;
    for(int o=mOrd+1;o<s->nord;o++) left+=s->rows[s->order[o]];
    return left*s->rowN<(long)frames*MUS_N||mLaps>=1;
}
// ---------- jukebox song table: built from source/songs.h (edit that file, not this) ----------
// Pass 1 bakes every .adp into the ROM, pass 2 declares the data, pass 3 builds the table.
#define SONG_XM(id,n,f)
#define SONG_ADP(id,n,f) ".global jbs_" #id "\njbs_" #id ":\n.incbin \"" f "\"\n.balign 4\n"
__asm__(".pushsection .rodata\n.balign 4\n"
#include "songs.h"
".popsection\n");
#undef SONG_XM
#undef SONG_ADP
#define SONG_XM(id,n,f)
#define SONG_ADP(id,n,f) extern const u8 jbs_##id[];
#include "songs.h"
#undef SONG_XM
#undef SONG_ADP
typedef struct { const char*name; const u8*adp; const XmSong*xm; } Song;   // adp = 0 means a tracker song (xm)
#define SONG_XM(id,n,f) {n,0,&xm_##id},
#define SONG_ADP(id,n,f) {n,jbs_##id,0},
static const Song songs[]={
#include "songs.h"
};
#undef SONG_XM
#undef SONG_ADP
#define NSONGS ((int)(sizeof(songs)/sizeof(songs[0])))
// artists (source/artists.h): looked up by the song's name, a song without a line shows none
typedef struct { const char*song; const char*artist; } ArtistRow;
#define ARTIST(s,a) {s,a},
static const ArtistRow artistRows[]={
#include "artists.h"
{0,0}};
#undef ARTIST
static const char* songArtist(const Song*sg){
    for(const ArtistRow*r=artistRows;r->song;r++){ const char*x=r->song,*y=sg->name; while(*x&&*x==*y){x++;y++;} if(!*x&&!*y) return r->artist; }
    return 0;
}
_Static_assert(sizeof(songs)/sizeof(songs[0])<=64,"the jukebox holds at most 64 songs, secret ones included (see source/songs.h and JB_MAX in jukebox.h)");
// ---------- debug code: UP UP DOWN DOWN LEFT LEFT RIGHT B A START on the title screen ----------
// Reveals, for this session only (never saved): the PLACEHOLDER test tunes in the jukebox, the secret creator chiptunes, and everything that directly changes the HOUSEHOLD:
// pause menu > HOUSEHOLD (MOVE IN A FAMILY, INVITE A NEW SIM, TRULY RANDOM SIM, MOVE SOMEONE OUT, MOVE EVERYONE OUT; without the code it is just RELATIONSHIPS), SELECT on RELATIONSHIPS,
// the creator's ADD TO FAMILY and FAMILY (DONE tab), and SAVE / LOAD HOUSEHOLD in ROOM SLOTS.
static u8 dbgOn;
static void settingsSave(void);
static const u16 konSeq[11]={K_UP,K_UP,K_DOWN,K_DOWN,K_LEFT,K_RIGHT,K_LEFT,K_RIGHT,K_B,K_A,K_START};   // UP UP DOWN DOWN LEFT RIGHT LEFT RIGHT B A START
static u8 konMsg;   // 1 = the code just locked the classic creator, 2 = unlocked (main shows a toast once the title is gone)
static const u16 dbgSeq[10]={K_UP,K_UP,K_DOWN,K_DOWN,K_LEFT,K_LEFT,K_RIGHT,K_B,K_A,K_START};
static int titleScreen(void){
    sprKey=0; buildTitle();                // leaves the finished backdrop in fb, and the pieces it repaints in tfb (spr4: no sprite left in it)
    vsync(); dmaRows(fb,VRAM_ADDR,0,ROW_W,0,SH);
    int shown=0, frame, dbgI=0, konI=0; u16 dbgPrev=(u16)(~REG_KEYINPUT)&0x3FF; if(xo[XO_TITLEMUS]) musStart();
    for(frame=0;;frame++){
        u16 dk=(u16)(~REG_KEYINPUT)&0x3FF, dp=dk&(u16)~dbgPrev; dbgPrev=dk;
        if(dp){   // a fresh button press: right next key of the code, or start over
            if(dp==dbgSeq[dbgI]){ if(++dbgI==10){ dbgOn=1; dbgI=0; } }
            else dbgI=(dp==dbgSeq[0])?1:0;
            if(dp==konSeq[konI]){ if(++konI==11){ konI=0; sUnlock^=1; if(!sUnlock) sClassic=0; settingsSave(); konMsg=1+sUnlock; } }
            else konI=(dp==konSeq[0])?1:0;
        }
        if(dk&K_START) break;
        titleKeep(0,SM_W0,SM_W1,0,SM_Y1,0);   // wipe last frame's smoke only
        smoke(frame);
        int on=(frame>>4)&1, tx=(on!=shown);
        if(tx){ titleKeep(0,TX_W0,TX_W1,TX_Y0,TX_Y1,TB_TX); if(on) text(94,141,"PRESS START",RGB(31,31,31),1); shown=on; }
        vsync(); musKick();
        dmaRows(fb,VRAM_ADDR,SM_W0,SM_W1,0,SM_Y1);
        if(tx) dmaRows(fb,VRAM_ADDR,TX_W0,TX_W1,TX_Y0,TX_Y1);
        musFill();
    }
    while((~REG_KEYINPUT)&K_START) vsync();   // wait for release so START doesn't also change size   (the title song plays on: the main menu crossfades from it)
    return frame;   // how long the player sat on the title: stirs the random seed
}

static void sfxPlay(int id){   // a new sound replaces whatever effect is playing; the song (if any) keeps going under it
    if(!sSnd){ sfxStop(); return; }
    vxQn=0;   // (a new sound drops the clips waiting behind the old one)
    const u8*b=sfxTab[id];
    sfxV=0;   // (the interrupt does not touch the voice while sfxV is 0)
    ssrc=b+4; sn=*(const u32*)b; sPos=0; sFr=0; sRd=0; spred=0; sidx=0; sS0=sS1=0;
    sfxLoop=0; sfxFade=sfxFadeT=256;
    sfxOn=1; sfxV=1;
    if(!mOn) audStart();
}
static void sfxTick(void){   // call once per frame: switch the mixer off once the last effect is over and no song plays
    if(!sfxV&&vxQn>0){ int v=vxQ[0], n=vxQn-1; vxQ[0]=vxQ[1]; voxPlay(v); vxQn=n; }   // the next clip of a chain (lighter, inhale, cough)
    if(sfxOn&&!sfxV){ sfxOn=0; if(mOn&&!mPlay) audStop(); }
}
static u32 lrng=12345;
static int rnd8(void){ lrng=lrng*1664525u+1013904223u; return (int)(lrng>>24); }
// ---- voices: the Sim you control talks. voxPlay: always; voxNag: only when nothing else is sounding; voxChain: three clips one after the other ----
static void voxPlay(int v){ sfxPlay(VS(v)); }
static void voxNag(int v){ if(!sfxV&&!vxQn) voxPlay(v); }
static void voxChain(int a,int b,int c){ voxPlay(a); vxQ[0]=(signed char)b; vxQ[1]=(signed char)c; vxQn=2; }
static void voxEvent(int ev,int v){   // sims.h calls this for every life event (simEventV); the socials speak from house.h (voxSoc)
    switch(ev){
        case SE_SHIFT: voxPlay(V_finished); break;                                   // FINISH A SHIFT
        case SE_ACE: case SE_PROMO: case SE_GROWUP: voxPlay(V_yahoo); break;        // ace a shift, get promoted, grow up
        case SE_SKILL: voxPlay(V_yeha); break;                                       // skill up
        case SE_COMBO: if(v>=5) voxPlay(V_yeha); break;                              // a 5 trick combo
        case SE_ACCIDENT: voxPlay(V_peed_self); break;
        case SE_FAINT: voxPlay(V_death_of_hunger); break;                            // fainted from hunger
        case SE_PASSOUT: voxPlay(V_snoore); break;
        case SE_SAD: voxNag(V_cry); break;
        case SE_DEMOTE: case SE_BROKE: case SE_NOPAY: voxPlay(V_cry_after_bad_advent_2); break;
        case SE_BORED: voxNag(V_hey_i_need_something_h); break;
        case SE_LONELY: voxNag(V_needs_something); break;
        case SE_STINKY: voxNag(V_sniiize_2); break;
        case SE_PIPE: voxChain(V_lighter_spark,V_spark_inhale,(rnd8()&1)?V_after_smoke_cough:V_after_smoke_cough_2); break;   // spark, inhale, cough
        default: break;
    }
}

#define BAIL_STUN 34   // frames you lie there after an ordinary bail (was 45, then 60 in hurt()): back on the board in about half a second
// Getting hurt. sev grows with fall height, speed and a bad landing. kind: 0 clean landing, 1 bail, 2 wall hit.
// DEATH VARIANTS: what killed you decides the note on the dead screen (and a few the sound). why: 0 plain, 1 shock, 2 gravity, 3 a wall, 4 hunger, 5 worn out
static void fxGhostBorn(int why); static void fxTick(void); static void fxPlayStart(void);   // fx.h: ghosts and weather
static const char* const deathNote[6]={"YOU DIED","DIED OF SHOCK","GRAVITY WON","MET A WALL AT SPEED","DIED OF HUNGER","ONE HIT TOO MANY"};
static void die(int snd,int why){ if(tutOn){ lhp=HP_MAX; lstun=60; lsp=0; lgrind=0; lnote="TUTORIAL  NO DYING"; lnoteT=90; return; }   // (tutorial.h)
    if(why==4||(why==5&&lfood<10)) why=4;   // hit points ran out while starving: say so
    moodEvent(M_DIE); fxGhostBorn(why); ldead=1; lstun=2; lsp=0; lgrind=0; sfxPlay(snd); lnote=deathNote[why>5?0:why]; lnoteT=0x7fff; }
static void hurt(int sev,int kind){
    if(abPow()&PW_ARMOUR) sev=sev*7/10;                                    // SPIKES: armour plates take the edge off
    if(xo[XO_HURT]==1) sev/=2; else if(xo[XO_HURT]==2&&sev>=30) sev=29;   // HURT option: GENTLE halves it, NO DEATH keeps a fall survivable
    if(sev>=30) moodEvent(M_HURT_BIG); else if(sev>=18) moodEvent(M_HURT); else if(kind==2) moodEvent(M_BUMP);   // (40+ is death: die() logs it)
    hpLose(sev>=40?HP_MAX:kind==2?sev:sev*3/2);                                                     // HEALTH: a wall hit costs sev, a fall or bail 1.5 x sev
    if(sev>=40) die(kind==2?SFX_DEATH:kind==0?SFX_SCREAM:VS(V_die_of_shock),kind==2?3:kind==0?2:1);                                                                   // instant death
    else if(sev>=30){                                                                                // life or death
        if(rnd8()<128){ if(lhp>15) lhp=15; lstun=240; lsp=0; lgrind=0; sfxPlay(SFX_NEARLY); lnote="CLOSE CALL"; lnoteT=120; }
        else die(kind==2?SFX_DEATH:SFX_SCREAM,kind==2?3:2);
    }
    else if(sev>=18){ lstun=150; lsp=0; lgrind=0; sfxPlay(SFX_GROAN); lnote="OW"; lnoteT=90; }     // groaning, struggling up
    else if(kind==1){ lstun=BAIL_STUN; voxPlay((rnd8()&1)?V_cry:V_cry_after_bad_event); }                                                  // minor bail: crying
    else if(kind==2){ lstun=20; sfxPlay(SFX_HIT); lnote="OOF"; lnoteT=30; }                          // grunts and hits
    if(!ldead&&lhp<=0) die(SFX_DEATH,5);                                                              // the meter ran out (hits add up)
}
// A punch lands on the one you control (house.h calls this). Fights never kill: at 0 HP you are knocked out for 4 s and get up at 25.
static void fightHurt(int dmg){
    if(xo[XO_HURT]==1) dmg/=2;                                              // GENTLE
    static u8 vxLosing; if(lhp>=60) vxLosing=0;
    lhp-=dmg; lsp=0; lgrind=0; sfxPlay(SFX_HIT);
    if(lhp<=0){ lhp=25; lstun=240; sfxPlay(SFX_GROAN); voxPlay(V_lost_the_fight); lnote="KNOCKED OUT"; lnoteT=120; moodEvent(M_HURT_BIG); }
    else { if(lhp<35&&!vxLosing){ vxLosing=1; voxPlay(V_losing_the_fight); }   // still on your feet, but losing
        static char fhB[16] EWRAM_BSS; char*e=simCat(fhB,dmg>=30?"OUCH ":"OW "); *e++='-'; simCatN(e,dmg); if(lstun<30) lstun=30; lnote=fhB; lnoteT=40; }   // and how much
}

#include "ramps.h"
static int tileH(int tx,int ty){   // surface height in px (ramps: their highest point). Grind height is 6: rails, ledges and benches
    if(tx<0||ty<0||tx>=MW||ty>=MH) return 99;
    char c=lifeMap[ty][tx];
    return (c=='#'||c=='F'||c=='W'||c=='H')?2*CC: (c=='X'||c=='Y')?10: (c=='w'||c=='T'||c=='S'||c=='C'||c=='O'||c=='G'||c=='V'||c=='U'||c=='Q'||c=='A')?CC: (c=='='||c=='L'||c=='N'||c=='Z'||c=='K'||c=='J'||c=='I'||c=='R')?6: (c=='M')?3: isKicker(c)?KICKER_H: isLaunch(c)?LAUNCH_H: isQPipe(c)?qpH[7]: 0;   // pack 2: X funbox 10, Y trash can 10, O barrel 8, Z planter / K table / J jersey grind at 6, M manual pad 3
}
static int surfH(s32 fx,s32 fy){   // surface height at an exact position (1/256 tiles): same as tileH, but ramps slope
    int tx=(int)(fx>>8), ty=(int)(fy>>8); if(tx<0||ty<0||tx>=MW||ty>=MH) return 99;
    char c=lifeMap[ty][tx]; return isRamp(c)?rampH(c,(int)fx,(int)fy):tileH(tx,ty);
}
static void bakeShrink(u16 (*spr4)[SPW*SPH],int v){   // view v, just drawn at full size in fb, into the sprite set at 0.4 size
    // every sprite pixel covers 2 or 3 screen pixels each way: take the top left one, unless the cell holds a very dark one (eyes, mouth,
    // outline): those must survive the shrink
    for(int y=0;y<SPH;y++){ int sy0=(y*5)>>1, sy1=((y+1)*5)>>1;
        for(int x=0;x<SPW;x++){ int sx0=(x*5)>>1, sx1=((x+1)*5)>>1;
            u16 c=fb[(SPY0+sy0)*SW+SPX0+sx0]; int best=(c&31)+((c>>5)&31)+((c>>10)&31);
            if(best>14&&c!=SKY) for(int yy=sy0;yy<sy1;yy++){ const u16*r=&fb[(SPY0+yy)*SW+SPX0]; for(int xx=sx0;xx<sx1;xx++){ u16 q=r[xx]; int sm=(q&31)+((q>>5)&31)+((q>>10)&31); if(sm<=11&&sm<best){ best=sm; c=q; } } }
            spr4[v][y*SPW+x]=c; } }
    // seen from behind the head shows hair, not a face: repaint the head's skin in the hair colour so the way he is facing reads at a glance
    if(!custom&&(v==1||v==2)){
        int hx,hy,hz,hs; headBox(&hx,&hy,&hz,&hs);
        int ax=SW,az=SH,bx=0,bz=0;   // head box on screen (full size)
        for(int yy=hy;yy<hy+2*hs;yy++)for(int zz=hz;zz<hz+2*hs;zz++)for(int xx=hx;xx<hx+2*hs;xx++){
            int sx,sy; projC(2*xx+1-W,2*zz+1-D,yy+1,&sx,&sy);
            if(sx-CA<ax) ax=sx-CA; if(sx+CA>bx) bx=sx+CA; if(sy-CB<az) az=sy-CB; if(sy+CB+CC>bz) bz=sy+CB+CC; }
        int x0=(ax-SPX0)*2/5, x1=(bx-SPX0)*2/5+1, y0=(az-SPY0)*2/5, y1=(bz-SPY0)*2/5+1;
        for(int y=y0<0?0:y0;y<y1&&y<SPH;y++)for(int x=x0<0?0:x0;x<x1&&x<SPW;x++){
            u16*c=&spr4[v][y*SPW+x];
            if(*c==sT[1]) *c=sT[5]; else if(*c==sL[1]) *c=sL[5]; else if(*c==sR[1]) *c=sR[5]; }
    }
}
static int bakeClips(void){   // the drawing in fb reaches the two outer rows / columns of the capture window (the bake keeps every other pixel)
    for(int x=0;x<SPW*5/2;x++) if(fb[SPY0*SW+SPX0+x]!=SKY||fb[(SPY0+1)*SW+SPX0+x]!=SKY) return 1;
    for(int y=0;y<SPH*5/2;y++){ const u16*r=&fb[(SPY0+y)*SW+SPX0]; if(r[0]!=SKY||r[1]!=SKY||r[SPW*5/2-2]!=SKY||r[SPW*5/2-1]!=SKY) return 1; }
    return 0;
}
static void bakeInto(u16 (*spr4)[SPW*SPH]){   // render the built character once per view (4 turns) into a sprite set, then just blit it
    int sv=view; noGrid=1; bakeOn=1; oycV=OYCB;   // (drawn lower than in the creator: the tall capture window fits on the screen)
    int ox=cX0, oy=cY0; unsigned ow=cW, oh=cH;   // draw only inside the capture window: nothing outside it is ever read
    { int x0=SPX0>ox?SPX0:ox, y0=SPY0>oy?SPY0:oy, x1=SPX0+SPW*5/2, y1=SPY0+SPH*5/2;
      if(x1>ox+(int)ow) x1=ox+(int)ow; if(y1>oy+(int)oh) y1=oy+(int)oh; if(x1<x0) x1=x0; if(y1<y0) y1=y0; clipSet(x0,y0,x1,y1); }
    for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ ghost[y][z][x]=0; gdec[y][z][x]=0; }
    bakeCapH=99; bakeCapW=99; bakeCapT=99; bakeCapX=99; bakeCapL=99; bakeCapE=99; { view=0; drawScene(0); bakeCapE=exMax; bakeCapH=liftK; bakeCapW=bakeWk; bakeCapT=liftT; bakeCapX=armK>stanceK?armK:stanceK; bakeCapL=bakeSh; }   // the HEIGHT and WEIGHT sliders are eased off, a step at a time, until every view fits the capture window
    // The caps start at the values the probe measured, which change nothing, so the probe picture IS the first round's view 0.
    // Each view is shrunk as soon as it is drawn and found to fit, so a round that fits leaves all four done (nothing drawn twice).
    // A round stops at the first view that sticks out, and the next round tests that view first: one drawing per failed round, not four.
    // (Only WHETHER a round fits steers the caps, never which view failed, so the caps and the sprites come out exactly as before.)
    int fit=0, first=0, cl=-1; u8 done=0;
    for(int tries=0;tries<24;tries++){
        cl=-1; done=0;
        for(int j=0;j<4;j++){ int v=(first+j)&3;
            if(tries||j){ view=v; drawScene(0); }
            if(bakeClips()){ cl=v; break; }
            bakeShrink(spr4,v); done|=(u8)(1<<v); }
        if(cl<0){ fit=1; break; }
        first=cl;
        if(bakeCapE>0&&(tries&3)==2) bakeCapE--;
        else if(bakeCapX>0&&(tries&1)) bakeCapX--;
        else if(bakeCapL>0&&(tries&1)) bakeCapL--;
        else if(bakeCapW>0&&(bakeCapH<=0||(tries&1))) bakeCapW--;
        else if(bakeCapH>0) bakeCapH--;
        else if(bakeCapT>0) bakeCapT--;
        else if(bakeCapX>0) bakeCapX--;
        else if(bakeCapL>0) bakeCapL--;
        else if(bakeCapE>0) bakeCapE--;
        else { bakeShrink(spr4,cl); done|=(u8)(1<<cl); break; }   // nothing left to ease off: this round IS the final picture (view cl is still in fb)
        cl=-1; done=0;   // the caps changed: nothing drawn so far counts
    }
    if(!fit) for(int v=0;v<4;v++) if(!(done&(1<<v))){ view=v; drawScene(0); bakeShrink(spr4,v); }   // whatever the last caps still need
    clipSet(ox,oy,ox+(int)ow,oy+(int)oh); bakeOn=0; oycV=121;
    noGrid=0; view=sv;
    spBx0=SPW; spBx1=0; spBy0=SPH; spBy1=0;   // the box that holds every opaque pixel of all four views: blits and redraw rectangles stay inside it
    for(int v=0;v<4;v++)for(int y=0;y<SPH;y++)for(int x=0;x<SPW;x++) if(spr4[v][y*SPW+x]!=SKY){
        if(x<spBx0) spBx0=x; if(x+1>spBx1) spBx1=x+1; if(y<spBy0) spBy0=y; if(y+1>spBy1) spBy1=y+1; }
    if(spBx0>=spBx1){ spBx0=0; spBx1=SPW; spBy0=0; spBy1=SPH; }
}
static void hhBakeAll(void);
static void bakeSprites(void){ hhBakeAll(); }   // the player and every household member (house.h)
IWRAM_CODE static void blit(const u16*s,int x0,int y0){
    int ia=cX0-x0, ib=cX0+(int)cW-x0; if(ia<spBx0) ia=spBx0; if(ib>spBx1) ib=spBx1; if(ia>=ib) return;
    for(int y=spBy0;y<spBy1;y++){ int yy=y0+y; if((unsigned)(yy-cY0)>=cH) continue;
        const u16*sp=s+y*SPW+ia; u16*d=&fb[yy*SW+x0+ia];
        for(int x=ia;x<ib;x++,sp++,d++){ u16 c=*sp; if(c!=SKY) *d=c; } }
}
static int numStr(char*b,int n){ char t[8]; int k=0, i=0; if(n<=0) t[k++]='0'; while(n>0&&k<7){ t[k++]=(char)('0'+n%10); n/=10; } while(k>0) b[i++]=t[--k]; b[i]=0; return i; }   // n as text into b; returns its length
static int numText(int x,int y,int n,u16 c){
    char b[10]; int i=9; b[i]=0; if(n<=0) b[--i]='0';
    while(n>0&&i>0){ int q=n/10; b[--i]=(char)('0'+n-q*10); n=q; }
    return text(x,y,b+i,c,1);
}
// ---------- map data: reset / scan / save ----------
// lifeMap = what stands on each tile, floorMap = floor style under it, wallMap = wallpaper on it (for wall tiles).
enum { T_ROOM, T_WALL, T_FLOOR, T_ITEM, T_ERASE, NTOOL };
static int eTool, eAct, eAx, eAy, eFl, eWp, eOb;   // editor: tool, rectangle anchor set?, anchor tile, chosen floor / wallpaper / item
#define NOBJ 34
#define OB_LAUNCH 17   // launch ramp turns like the kicker: '9'..'<'
#define OB_KICKER 10   // palette slots whose char carries a turn (+eRot): kicker '1'..'4', quarter pipe '5'..'8'
#define OB_QPIPE 11
static int eRot;   // editor: which way the next ramp faces (0 S, 1 E, 2 N, 3 W)
static const char palCh[NOBJ]={'.','w','W','#','=','F','T','D','B','P','1','5','L','N','S','H','C','9','X','O','Y','Z','K','J','M','G','V','U','^','~','Q','I','R','A'};
static const char* const palNm[NOBJ]={"CLEAR","LOW WALL","WALL","CRATE","RAIL","FRIDGE","TOILET","DOOR","BOARD","SPAWN","KICKER","Q PIPE","LEDGE","BENCH","BED","SHOWER","SOFA","LAUNCH","FUNBOX","BARREL","TRASH CAN","PLANTER","PICNIC","JERSEY","MANUAL PAD","WATER PIPE","LAVA LAMP","BEANBAG","STAIRS UP","STAIRS DOWN","DEADSET 3THOUSAND VYBE","PHONE","RADIO","SOUND SYSTEM"};
static const u16 palCol[NOBJ]={RGB(26,21,14),RGB(8,20,22),RGB(10,22,24),RGB(8,9,20),RGB(31,30,16),RGB(31,31,31),RGB(30,28,18),RGB(14,9,5),RGB(26,10,6),RGB(28,10,8),RGB(24,17,9),RGB(27,19,11),RGB(20,20,22),RGB(25,18,9),RGB(10,14,28),RGB(22,28,30),RGB(26,18,9),RGB(8,14,24),RGB(18,16,24),RGB(24,6,5),RGB(12,18,14),RGB(20,10,6),RGB(25,18,9),RGB(22,22,24),RGB(30,26,5),RGB(10,24,14),RGB(24,8,26),RGB(18,8,22),RGB(24,22,18),RGB(12,11,10),RGB(6,20,31),RGB(26,6,6),RGB(20,20,22),RGB(12,13,16)};
static signed char palLut[256] EWRAM_BSS; static u8 palLutOk;   // tile char -> palette slot (or -1), built on first use: palIdx() runs for every tile of the minimap, so it must be O(1) even with 100+ items
static int palIdx(char c){
    if(!palLutOk){ for(int i=0;i<256;i++) palLut[i]=-1; for(int i=NOBJ-1;i>=0;i--) palLut[(u8)palCh[i]]=(signed char)i;
        for(int r=0;r<4;r++){ palLut[(u8)('1'+r)]=OB_KICKER; palLut[(u8)('5'+r)]=OB_QPIPE; palLut[(u8)('9'+r)]=OB_LAUNCH; } palLutOk=1; }
    return palLut[(u8)c];
}
static char edObjCh(void){ char c=palCh[eOb]; return (eOb==OB_KICKER||eOb==OB_QPIPE||eOb==OB_LAUNCH)?(char)(c+eRot):c; }   // the char the ITEM tool places
// ---- BUY mode: the catalog. Every palette item sits in one category and has a price (cash of the life; BUILD COSTS option) ----
#define DS_PRICE 5000
#define NCAT 7
static const char* const catNm[NCAT]={"SEAT","HOME","TECH","SKATE","DECOR","WALLS","MISC"};
static const u8 catN[NCAT]={4,4,6,10,2,5,3};
static const u8 catItems[NCAT][10]={ {13,16,27,22}, {5,6,14,15}, {31,32,33,30,26,25}, {3,4,10,11,12,17,18,19,23,24}, {20,21}, {1,2,7,28,29}, {0,8,9} };
static const u16 palPrice[NOBJ]={0,3,6,10,15,150,90,12,0,0,30,60,20,40,140,110,120,45,50,10,5,10,80,15,10,30,25,60,40,40,DS_PRICE,50,40,200};
static int edCatOf(int idx,int*pos){ for(int c=0;c<NCAT;c++) for(int j=0;j<catN[c];j++) if(catItems[c][j]==idx){ if(pos) *pos=j; return c; } if(pos) *pos=0; return 0; }
static void edItemStep(int d){ int p, c=edCatOf(eOb,&p); p=(p+d+catN[c])%catN[c]; eOb=catItems[c][p]; }   // L / R: the next item of this category
static void edCatStep(int d){ int c=(edCatOf(eOb,0)+d+NCAT)%NCAT; eOb=catItems[c][0]; }                  // SELECT + L / R: the next category
// ---- default big map: house (top left), factory (top right), rail park (bottom), roads of concrete between ----
static void gBox(int x0,int y0,int x1,int y1,int fl){ for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++) floorMap[y][x]=(u8)fl; }
static void gRoom(int x0,int y0,int x1,int y1,int fl,int wp){   // walled room with a floor
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){ floorMap[y][x]=(u8)fl;
        if(x==x0||x==x1||y==y0||y==y1){ lifeMap[y][x]='W'; wallMap[y][x]=(u8)wp; } } }
static void gLine(int x0,int y0,int x1,int y1,char c,int wp){ for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){ lifeMap[y][x]=c; wallMap[y][x]=(u8)wp; } }
static void gPut(int x,int y,char c){ lifeMap[y][x]=c; }
static void gFree(int x,int y,char c){ if(x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='.') lifeMap[y][x]=c; }   // put only onto empty floor
static void mapGen(void){
    wDirty=1;
    for(int y=0;y<MH;y++){ for(int x=0;x<MW;x++){ lifeMap[y][x]='.'; floorMap[y][x]=7; wallMap[y][x]=0; } lifeMap[y][MW]=0; }
    gLine(0,0,MW-1,0,'w',13); gLine(0,MH-1,MW-1,MH-1,'w',13); gLine(0,0,0,MH-1,'w',13); gLine(MW-1,0,MW-1,MH-1,'w',13);   // low wall round the edge
    // HOUSE: peach wallpaper, beige carpet, lino kitchen, pink-tile bathroom
    gRoom(2,2,17,17,1,NWP+61); gBox(11,11,16,16,3);   // (wallpapers NWP+n: the KHLVH set, wallart.h: PARLOR, OCEANIC, METAL DECK)
    gRoom(2,2,9,9,5,NWP+57); gPut(6,9,'D'); gPut(3,3,'T');
    gPut(9,17,'D'); gPut(17,13,'D');
    gPut(16,11,'F'); gPut(16,12,'F'); gPut(12,4,'#'); gPut(13,4,'#'); gPut(12,5,'#'); gPut(13,5,'#');
    gPut(5,12,'P'); gPut(7,14,'B');
    gPut(8,3,'H'); gPut(4,16,'S'); gPut(3,11,'C');                     // shower (bathroom), bed and sofa (lounge)
    gPut(3,10,'V'); gPut(5,10,'G'); gPut(3,13,'U'); gPut(6,12,'Q'); gPut(8,12,'I');   // + the DeadSet 3Thousand VYBE                     // the chill corner: lava lamp, water pipe, beanbag
    // FACTORY: red brick, steel plate, oil-stained and hazard lanes, grate corner, crates and a rail
    gRoom(22,2,37,19,8,NWP+53); gBox(23,10,36,11,10); gBox(23,14,27,18,9); gBox(30,3,36,8,12);
    gPut(29,19,'D'); gPut(22,10,'D'); gPut(37,10,'D');
    gLine(24,13,29,13,'=',8);
    gPut(25,4,'#'); gPut(26,4,'#'); gPut(25,5,'#'); gPut(26,5,'#'); gPut(31,15,'#'); gPut(32,15,'#'); gPut(31,16,'#'); gPut(32,16,'#'); gPut(34,5,'#'); gPut(34,6,'#');
    // RAIL PARK: oil-stained skate lanes, long rails, crate boxes. The middle (x 13-26, y 22-35) is a plaza (an old 14x14 saved room lands here)
    gBox(2,22,37,37,7); gBox(2,28,37,29,12);
    gLine(3,24,10,24,'=',0); gLine(3,31,10,31,'=',0); gLine(3,35,10,35,'=',0);
    gLine(29,24,36,24,'=',0); gLine(29,31,36,31,'=',0); gLine(29,35,36,35,'=',0);
    gLine(16,28,23,28,'=',0); gLine(16,33,23,33,'=',0);
    gPut(5,26,'#'); gPut(6,26,'#'); gPut(5,27,'#'); gPut(6,27,'#'); gPut(8,33,'#'); gPut(9,33,'#'); gPut(8,34,'#'); gPut(9,34,'#');
    gPut(31,26,'#'); gPut(32,26,'#'); gPut(31,27,'#'); gPut(32,27,'#'); gPut(34,33,'#'); gPut(35,33,'#'); gPut(34,34,'#'); gPut(35,34,'#');
    gPut(14,24,'#'); gPut(15,24,'#'); gPut(14,25,'#'); gPut(15,25,'#'); gPut(24,25,'#'); gPut(25,25,'#'); gPut(24,26,'#'); gPut(25,26,'#');
    gLine(12,21,27,21,'w',13);
    gPut(16,30,'4'); gPut(21,30,'2');                                  // two kickers facing each other: a gap jump
    gPut(17,22,'5'); gPut(18,22,'5');                                  // quarter pipes (face south) in front of the plaza wall
    gPut(13,33,'L'); gPut(14,33,'L'); gPut(15,33,'L'); gPut(25,33,'N'); gPut(26,33,'N');   // ledge and bench to grind
    // SKATE PACK 2 (only onto empty floor, so nothing above is overwritten): funbox with two launch ramps, barrels, jersey barriers, planters, picnic table, trash cans, manual pad
    gFree(18,30,'X'); gFree(19,30,'X'); gFree(18,31,'9'); gFree(19,31,'9');
    gFree(13,31,'O'); gFree(13,32,'O'); gFree(14,31,'O'); gFree(26,30,'O'); gFree(26,31,'O');
    for(int x=16;x<=21;x++) gFree(x,35,'J');
    gFree(13,22,'Z'); gFree(14,22,'Z'); gFree(25,22,'Z'); gFree(26,22,'Z');
    gFree(21,24,'K'); gFree(13,23,'Y'); gFree(26,23,'Y');
    for(int x=17;x<=20;x++) gFree(x,26,'M');
}
static void mapReset(void){ mapGen(); }
static void mapScan(void){   // find the skateboard (B) and the spawn point (P); fall back to sane defaults
    wDirty=1;
    int fx=-1, fy=-1; bdx=bdy=spx=spy=-1;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ char c=lifeMap[y][x];
        if(c=='B'){ bdx=x; bdy=y; } if(c=='P'){ spx=x; spy=y; }
        if(fx<0&&c=='.'){ fx=x; fy=y; } }
    if(spx<0){ if(fx<0){ lifeMap[1][1]='P'; fx=fy=1; } spx=fx; spy=fy; }
}
static const char sramTag[] __attribute__((used)) = "FLASH1M_V103";   // tells emulators / flash carts to give the game 128 KB of flash (save.h)
#define MSZ (MW*MH)
#define FLR_N 3   // floors a house has (house slots, slots.h): the floor you stand on is the live map, the other floors wait packed in flPool
// FLOOR STORAGE. A floor is mostly empty floor, so the floors are kept run-length packed instead of as 3 x 1600 bytes each: flPool holds floor 0's
// runs, then floor 1's, then floor 2's, back to back. One floor = three planes (tiles, floors, wallpapers), each a list of (count 1..255, value)
// pairs that covers the whole map. flLen[f] = bytes the floor takes (0 = a blank floor, flBlank, which takes no room at all); flPl[f][p] = where
// plane p starts inside it. The floor you stand on is the live map (lifeMap / floorMap / wallMap): its copy here is only the last one stored.
// FL_POOL is big enough for every house the save slots can hold (4 slots, see the _Static_assert in slots.h). A floor that would not fit is not
// stored: flStoreAs returns 0 and the old copy stays, flGo refuses the stairs.
#define FL_POOL 8192
static u8 flPool[FL_POOL] EWRAM_BSS; static u16 flLen[FLR_N] EWRAM_BSS, flPl[FLR_N][3] EWRAM_BSS; static int curFl; static u8 flArm, flInit;
static int flOff(int f){ int o=0; for(int g=0;g<f;g++) o+=flLen[g]; return o; }   // where floor f's runs start (flOff(FLR_N) = bytes in use)
static int flBlankV(int pl,int i){ int x=i%MW, y=i/MW, e=(x==0||y==0||x==MW-1||y==MH-1); return pl==0?(e?'w':'.'):pl==1?1:(e?13:0); }   // a blank floor: carpet and a low wall round the edge
static u8 flCf[3] EWRAM_BSS; static int flCi[3] EWRAM_BSS, flCn[3] EWRAM_BSS, flCv[3] EWRAM_BSS; static const u8*flCq[3] EWRAM_BSS;   // one read cursor per plane (floor + 1, last cell, run left, value, next run)
static int flGet(int f,int pl,int i){   // cell i of plane pl of stored floor f. Read a plane in order and it costs one step a cell
    if(!flLen[f]) return flBlankV(pl,i);
    if(flCf[pl]!=f+1||i<=flCi[pl]){ flCf[pl]=(u8)(f+1); flCq[pl]=flPool+flOff(f)+flPl[f][pl]; flCn[pl]=0; flCi[pl]=-1; }
    while(flCi[pl]<i){ if(!flCn[pl]){ flCn[pl]=*flCq[pl]++; flCv[pl]=*flCq[pl]++; } flCn[pl]--; flCi[pl]++; }
    return flCv[pl];
}
static int flPlaneAt(int f,int pl,int i){   // cell i of plane pl of floor f as it stands now: the live map for the floor you are on, else its stored copy
    if(f==curFl){ int y=i/MW, x=i%MW; return pl==0?(u8)lifeMap[y][x]:pl==1?floorMap[y][x]:wallMap[y][x]; }
    return flGet(f,pl,i);
}
#define SET_OFF 4864            // settings live here now (the big map takes bytes 0..4802)
#define OMW 14                  // old 14x14 saves
#define OMSZ (OMW*OMW)
#define LEG_X 13                // an old save is copied into the plaza at (13,22)
#define LEG_Y 22
#include "jukebox.h"   // which songs may play (the check boxes, saved in SRAM at JB_OFF = 5056) and picking one at random
// The SECRET songs: hidden from the jukebox, the menu music and the game music until the title-screen code (UP UP DOWN DOWN LEFT LEFT RIGHT B A START, dbgOn).
// They are picked by NAME, so a new one needs no change here: the old version of a reworked song is named "... (ORIGINAL)" in songs.h, the test tunes "PLACEHOLDER ...".
static int isDbgSong(int i){
    const char*n=songs[i].name; int len=0; while(n[len]) len++;
    static const char orig[]=" (ORIGINAL)"; int ol=(int)sizeof(orig)-1;
    if(len>ol){ const char*t=n+len-ol; int k=0; while(k<ol&&t[k]==orig[k]) k++; if(k==ol) return 1; }
    const char*p="PLACEHOLDER"; while(*p){ if(*n++!=*p++) return 0; } return 1;
}
static u16 jbNameHash(int upto){   // hash of the names of the first n songs: tells whether the saved on/off flags still belong to this list (jukebox.h)
    static const char* const was[2][2]={{"TREE-AGE IN ACTION","TREE SWAYING ACTION"},{"TREE-AGE IN ACTION (ORIGINAL)","TREE SWAYING ACTION (ORIGINAL)"}};   // renamed songs count by their old names (the checkmarks stay)
    u32 h=2166136261u; for(int i=0;i<upto&&i<NSONGS;i++){ const char*nm=songs[i].name;
        for(int r=0;r<2;r++){ const char*x=was[r][0],*y=nm; while(*x&&*x==*y){x++;y++;} if(!*x&&!*y) nm=was[r][1]; }
        for(const char*p=nm;*p;p++) h=(h^(u8)*p)*16777619u; h=(h^0x7C)*16777619u; }
    return (u16)(h^(h>>16));
}
// Songs that start LOCKED (source/unlocks.h): hidden until their bit is set in jbUl (a lifetime want met, see sims.h), or the title-screen code is entered.
typedef struct { const char*song; u8 bit; } UnlockRow;
#define UNLOCK(s_,b_) {s_,(u8)(b_)},
static const UnlockRow unlockRows[]={
#include "unlocks.h"
{0,0}};
#undef UNLOCK
static int isLockedSong(int i){
    for(const UnlockRow*r=unlockRows;r->song;r++){ const char*x=r->song,*y=songs[i].name; while(*x&&*x==*y){x++;y++;} if(!*x&&!*y) return !(jbUl&r->bit); }
    return 0;
}
static void jbSetup(void){   // build the list of songs the jukebox shows (no secret and no locked ones), then load the on/off flags
    jbUlLoad(); jbModeLoad();
    int n=0; for(int i=0;i<NSONGS&&n<JB_MAX;i++) if(songs[i].xm!=&xm_the_dipper_man&&(dbgOn||(!isDbgSong(i)&&!isLockedSong(i)))) jbMap[n++]=(u8)i;   // THE DIPPER MAN is the title music only: never listed
    jbInit(NSONGS,n,jbNameHash);
}
static int jbUnlock(int bit){   // 1 when the song was locked and is now free (saved for good; the list is rebuilt so it shows up at once)
    if(jbUl&bit) return 0;
    jbUl|=(u8)bit; jbUlSave(); jbSetup(); return 1;
}
static int jbDreamMet(int asp,int ltw){   // a lifetime dream was met (every life counts, saved for good). It is only recorded now: CLOSER TO THE END and TREE-AGE IN ACTION
    if(asp<0||asp>=AS_PICK) return 0;       // moved to the STORY MISSIONS (jbStoryDone); a life that already earned them with dreams keeps them. Always 0.
    u16 b=(u16)(1<<(asp*2+(ltw&1))); if(!(jbDr&b)){ jbDr|=b; jbDrSave(); }
    return 0;
}
// SRAM layout: 0..2 "BM3", then MSZ bytes each of tiles, floors, wallpapers. Settings at SET_OFF (see settingsSave).
// Old "BM1" / "BM2" saves (14x14, settings at 640) still load: the room is placed into the plaza of the new default map.
// (bytes 0..4095 sit in flash sector 0 on their own, so mapSave erases that sector and writes it again: svRd / svWr, not pointers)
static int mapSaved(void){ if(svRd(0)!='B'||svRd(1)!='M'||svRd(2)!='3') return 0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ int i=y*MW+x;
        if(curFl?(svRd(3+i)!=flGet(0,0,i)||svRd(3+MSZ+i)!=flGet(0,1,i)||svRd(3+2*MSZ+i)!=flGet(0,2,i)):(svRd(3+i)!=(u8)lifeMap[y][x]||svRd(3+MSZ+i)!=floorMap[y][x]||svRd(3+2*MSZ+i)!=wallMap[y][x])) return 0; }
    return 1; }
static void mapSave(void){
    if(mapSaved()) return;   // the same room: nothing to write (flash wears with every erase)
    svErase(0,SV_SEC); svWr(0,'B'); svWr(1,'M'); svWr(2,'3');
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ int i=y*MW+x; if(curFl){ svWr(3+i,flGet(0,0,i)); svWr(3+MSZ+i,flGet(0,1,i)); svWr(3+2*MSZ+i,flGet(0,2,i)); } else { svWr(3+i,(u8)lifeMap[y][x]); svWr(3+MSZ+i,floorMap[y][x]); svWr(3+2*MSZ+i,wallMap[y][x]); } }   // (upstairs: the room kept in SRAM is still the ground floor)
    svCommit(); }
static int mapLoad(void){   // returns 1 if a valid saved map was loaded
    wDirty=1;
    #define m(k) svRd(k)
    if(m(0)!='B'||m(1)!='M') return 0;
    if(m(2)=='3'){
        for(int i=0;i<MSZ;i++){ if(palIdx((char)m(3+i))<0||m(3+MSZ+i)>=NFL||m(3+2*MSZ+i)>=NWALL) return 0; }
        for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ int i=y*MW+x;
            lifeMap[y][x]=(char)m(3+i); floorMap[y][x]=m(3+MSZ+i); wallMap[y][x]=m(3+2*MSZ+i); }
        return 1; }
    if(m(2)!='1'&&m(2)!='2') return 0;
    int v2=(m(2)=='2');
    for(int i=0;i<OMSZ;i++){ if(palIdx((char)m(3+i))<0) return 0; if(v2&&(m(3+OMSZ+i)>=NFL||m(3+2*OMSZ+i)>=NWP)) return 0; }
    mapReset();
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]=='B'||lifeMap[y][x]=='P') lifeMap[y][x]='.';   // the old room brings its own
    for(int y=0;y<OMW;y++)for(int x=0;x<OMW;x++){ int i=y*OMW+x, X=LEG_X+x, Y=LEG_Y+y;
        lifeMap[Y][X]=(char)m(3+i); floorMap[Y][X]=v2?m(3+OMSZ+i):0; wallMap[Y][X]=v2?m(3+2*OMSZ+i):0; }
    #undef m
    return 1; }
static const char* dsMsg;   // set when something could not be bought (the room builder shows it)
static u8 edLife EWRAM_BSS, edTried EWRAM_BSS, edCashDirty EWRAM_BSS;   // the room builder works with the life's cash: loaded once (lazily when not in play), saved when you leave
static void edLoadLife(void){ if(edTried) return; edTried=1; simsDefaults(); edLife=simsLoad()?1:0; edCashDirty=0; }
static void edCashSave(void){ if(!gInPlay&&edLife&&edCashDirty){ simsSaveNow(); edCashDirty=0; } }
static int edCharged(void){ if(!gInPlay) edLoadLife(); return gInPlay||edLife; }   // is there a purse to pay from? (before any life is saved the room builder is free)
static int edCost(char c){ if(c=='Q') return DS_PRICE; if(!xo[XO_BUYCOST]) return 0; int i=palIdx(c); return i<0?0:palPrice[i]; }   // the DeadSet always costs; the rest with BUILD COSTS on
static int edSell(char c){ return c=='Q'?DS_PRICE:edCost(c)/2; }   // selling gives half back (the DeadSet: all of it, as before)
static int edPay(int net){   // net > 0 buys, net < 0 sells back. 0 = refused
    if(!net||!edCharged()) return 1;
    if(net>0&&simMoney<net){ dsMsg=net>=DS_PRICE?"THE DEADSET COSTS 5000":"NOT ENOUGH CASH"; return 0; }
    simMoney-=net; if(simMoney>9999) simMoney=9999; if(simMoney<0) simMoney=0; edCashDirty=1; return 1;
}
static int edAffordable(char c,char old){ int n=edCost(c)-edSell(old); return n<=0||!edCharged()||simMoney>=n; }
static void mapPlace(int x,int y,char c){
    char old=lifeMap[y][x];
    if(c!=old){ int net=edCost(c)-edSell(old); if(net&&!edPay(net)) return; }   // buying costs; replacing or removing sells the old one back (half)
    if(c=='B'||c=='P'){ for(int j=0;j<MH;j++)for(int i=0;i<MW;i++) if(lifeMap[j][i]==c) lifeMap[j][i]='.'; }
    lifeMap[y][x]=c; if(c=='w'||c=='W') wallMap[y][x]=(u8)eWp; wDirty=1; }

// ---------- floors: the live map is the floor you are on; the others wait packed in flPool. Stairs: '^' goes up, '~' comes down. House slots (slots.h) keep all of them ----------
static void hhSlotsFree(void); static void liveInvalidate(void);
static const char* const flNm[FLR_N]={"GROUND FLOOR","FLOOR 2","FLOOR 3"};
static int flBd[4];   // the board pickup and the spawn tile of the ground floor while you are upstairs
static void flBlankLive(void);
static int flLiveBlank(void){ for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ int e=(x==0||y==0||x==MW-1||y==MH-1); if(lifeMap[y][x]!=(e?'w':'.')||floorMap[y][x]!=1||wallMap[y][x]!=(e?13:0)) return 0; } return 1; }
static int flEnc(u8*out,u16*pl){   // the live map as runs: with out 0 only counts the bytes. pl gets where each plane starts
    int n=0;
    for(int p=0;p<3;p++){
        int run=0, cur=0; if(pl) pl[p]=(u16)n;
        for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){
            int v=p==0?(u8)lifeMap[y][x]:p==1?floorMap[y][x]:wallMap[y][x];
            if(run>0&&v==cur&&run<255){ run++; continue; }
            if(run){ if(out){ out[n]=(u8)run; out[n+1]=(u8)cur; } n+=2; }
            run=1; cur=v; }
        if(run){ if(out){ out[n]=(u8)run; out[n+1]=(u8)cur; } n+=2; }
    }
    return n;
}
static void flResize(int f,int n){   // floor f's runs become n bytes long (the floors after it slide along). The caller has checked that n fits
    int o=flOff(f), old=flLen[f], tail=flOff(FLR_N)-o-old;
    if(n<old) for(int k=0;k<tail;k++) flPool[o+n+k]=flPool[o+old+k];
    else if(n>old) for(int k=tail-1;k>=0;k--) flPool[o+n+k]=flPool[o+old+k];
    flLen[f]=(u16)n; flCf[0]=flCf[1]=flCf[2]=0;
}
static int flStoreAs(int f){   // keep the live map as floor f. 1 = done; 0 = it does not fit in flPool (then floor f's old copy is untouched)
    int n=flLiveBlank()?0:flEnc(0,0);
    if(flOff(FLR_N)-flLen[f]+n>FL_POOL) return 0;
    flResize(f,n); if(n) flEnc(flPool+flOff(f),flPl[f]);
    return 1;
}
static void flLoad(int f){   // floor f becomes the live map
    if(!flLen[f]){ flBlankLive(); return; }
    const u8*q=flPool+flOff(f)+flPl[f][0];
    for(int p=0;p<3;p++){ int x=0, y=0;
        for(int left=MSZ;left>0;){ int c=*q++, v=*q++; if(c>left) c=left; left-=c;
            while(c-->0){ if(p==0) lifeMap[y][x]=(char)v; else if(p==1) floorMap[y][x]=(u8)v; else wallMap[y][x]=(u8)v; if(++x==MW){ x=0; y++; } } } }
    wDirty=1;
}
static void flBlank(int f){ flResize(f,0); }   // an empty floor: carpet and a low wall round the edge (it takes no room)
static void flClear(void){ for(int f=0;f<FLR_N;f++) flLen[f]=0; flCf[0]=flCf[1]=flCf[2]=0; }   // every floor blank at once (a house is about to be loaded over them)
static void flBlankLive(void){ for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){ int e=(x==0||y==0||x==MW-1||y==MH-1); lifeMap[y][x]=e?'w':'.'; floorMap[y][x]=1; wallMap[y][x]=(u8)(e?13:0); } wDirty=1; }
static void flEnsure(void){ if(flInit) return; flInit=1; for(int f=1;f<FLR_N;f++) flBlank(f); }
static int flGoF(int n,int force){   // make floor n the live map (the one you leave is kept). 0 = it would not fit in flPool and nothing changed; with force the leaving floor falls back to its last stored copy
    if(n==curFl||n<0||n>=FLR_N) return 1;
    flEnsure(); if(!flStoreAs(curFl)&&!force) return 0;
    if(curFl==0){ flBd[0]=bdx; flBd[1]=bdy; flBd[2]=spx; flBd[3]=spy; }
    curFl=n; flLoad(n);
    if(n==0){ bdx=flBd[0]; bdy=flBd[1]; spx=flBd[2]; spy=flBd[3]; } else bdx=bdy=-1;   // no board pickup upstairs
    hhSlotsFree(); liveInvalidate(); camSnap=1;
    return 1;
}
static int flGo(int n){ return flGoF(n,0); }
static void flHome(void){ flArm=0; flGoF(0,1); }
static void flBlankUpper(void){ flEnsure(); for(int f=1;f<FLR_N;f++) flBlank(f); }
static void flStairs(int dir){   // you stepped on a stair tile: up (+1) or down (-1)
    int n=curFl+dir; if(n<0||n>=FLR_N){ lnote=dir>0?"NO FLOOR ABOVE":"NO FLOOR BELOW"; lnoteT=40; return; }
    int tx=(int)(lfx>>8), ty=(int)(lfy>>8); char want=dir>0?'~':'^';
    if(!flGo(n)){ lnote="TOO MUCH BUILT TO CLIMB"; lnoteT=60; return; }
    int fx=-1, fy=-1;
    for(int y=0;y<MH&&fx<0;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]==want){ fx=x; fy=y; break; }
    if(fx<0){ fx=tx; fy=ty; lifeMap[fy][fx]=want; wDirty=1; }   // no stairs there yet: they appear where you came from
    lfx=fx*256+128; lfy=fy*256+128; lz=lvz=0; flArm=0; lnote=flNm[n]; lnoteT=60;
}
// extended options (opts.h): one byte each at OPT_OFF, 'X' 'O', count, values, checksum. A save with fewer options (older game) leaves the new ones at their defaults.
#define OPT_OFF 4896
#define STORY_OFF 4968   // story.h: the STORY MODE block (8 bytes, after the options)
static void optsSave(void){
    volatile u8*m=SRAM_BASE+OPT_OFF; unsigned sum=0x3C;
    for(int i=0;i<XO_N;i++){ m[3+i]=xo[i]; sum+=xo[i]; }
    m[2]=XO_N; sum+=XO_N; m[3+XO_N]=(u8)sum; m[0]='X'; m[1]='O';
}
static void optsLoad(void){
    volatile u8*m=SRAM_BASE+OPT_OFF; optsDefaults();
    if(m[0]!='X'||m[1]!='O'||m[2]==0||m[2]>XO_N) return;
    int n=m[2]; unsigned sum=0x3C+n; for(int i=0;i<n;i++) sum+=m[3+i];
    if(m[3+n]!=(u8)sum) return;                                       // damaged: keep the defaults
    for(int i=0;i<n;i++) if(m[3+i]<xoCnt[i]) xo[i]=m[3+i];            // every value range checked
    if(n>XO_MUS&&n<=XO_MUSV){ static const u8 mm[4]={10,5,2,0}; xo[XO_MUSV]=mm[xo[XO_MUS]]; }   // a save from before the sliders: keep its old volume (full, half, quarter, off)
    if(n>XO_SFX&&n<=XO_SFXV){ static const u8 sm[3]={10,5,2}; xo[XO_SFXV]=sm[xo[XO_SFX]]; }
}
// settings (SRAM offset 8192)
static void settingsSave(void){ optsSave();
   volatile u8*m=SRAM_BASE+SET_OFF; m[0]='S'; m[1]='2'; m[2]=sFps; m[3]=sWall; m[4]=sWp; m[5]=sFl; m[6]=sSnd; m[7]=sShow; m[8]=sShad; m[9]=sHud; m[10]=sRom; m[11]=sCam; m[12]=2; m[13]=sNoWarn; m[14]=sClassic; m[15]=sUnlock; }
static void settingsLoad(void){ volatile u8*m=SRAM_BASE+SET_OFF;
    if(m[0]!='S'){ if(svType==SV_SRAM&&svRd(0)=='B'&&svRd(1)=='M'&&svRd(2)!='3') m=SRAM_BASE+640; else return; }   // old saves kept settings at 640
    if(m[0]!='S') return;
    if(m[1]=='1'){ if(m[2]>2||m[3]>2||m[4]>1||m[5]>1||m[6]>1||m[7]>1) return;   // older save: fewer settings
        sFps=m[2]; sWall=m[3]; sWp=m[4]; sFl=m[5]; sSnd=m[6]; sShow=m[7]; return; }
    if(m[1]!='2'||m[2]>3||m[3]>2||m[4]>1||m[5]>1||m[6]>1||m[7]>2||m[8]>1||m[9]>2||m[10]>1) return;
    sCam=(m[11]<=3)?m[11]:1; sNoWarn=(m[13]==1)?1:0; sClassic=(m[14]==1)?1:0; sUnlock=(m[15]==1)?1:0; if(!sUnlock) sClassic=0;
    sFps=m[2]; sWall=m[3]; sWp=m[4]; sFl=m[5]; sSnd=m[6]; sShow=m[7]; sShad=m[8]; sHud=m[9]; sRom=m[10];
    if(m[12]<2&&sFps==1) sFps=0; }   // revision 2: default frame rate 30 -> 60 (a save that chose 20 or 15 keeps it)

// ---------- small UI kit: one menu style, one help style, one toast ----------
#define DIMC RGB(18,20,22)
#define WHITE RGB(31,31,31)
static u16 keyNow(void){   // BUTTONS option: A/B and L/R can be swapped here, so every screen sees the swapped keys
    u16 k=(u16)(~REG_KEYINPUT)&0x3FF; int b=xo[XO_BTN];
    if(b&1){ u16 a=k&K_A, c=k&K_B; k=(u16)((k&~(K_A|K_B))|(a?K_B:0)|(c?K_A:0)); }
    if(b&2){ u16 l=k&K_L, r=k&K_R; k=(u16)((k&~(K_L|K_R))|(l?K_R:0)|(r?K_L:0)); }
    return k;
}
static void objHideAll(void){ for(int i=0;i<128;i++) ((volatile u16*)0x07000000)[i*4]=0x200; *(volatile u16*)0x04000050=0x0400; *(volatile u16*)0x04000054=0; }   /* all 128 OAM entries (fx.h uses 16..58), and no weather dimming behind a menu */    // household sprites off (menus, other screens)
static void box(int x,int y,int w,int h){ objHideAll(); rect(x-1,y-1,w+2,h+2,GOLD); rect(x,y,w,h,RGB(3,4,7)); }
static int menu(const char*title,const char*const*it,int n){   // UP/DOWN + A to choose, B or START to cancel (returns -1). Long lists scroll (L R jump a page).
    if(n<=0) return -1;
    int vis=n>9?9:n, w=tw(title,1)+40; for(int i=0;i<n;i++){ int q=tw(it[i],1)+30; if(q>w) w=q; } if(w<116) w=116; if(w>232) w=232;   // as wide as its longest line
    int h=32+vis*10, x=(SW-w)/2, y=(SH-h)/2, sel=0, top=0, dirty=1, hold=0; u16 prev=keyNow();
    for(int g=1;g<3;g++){ int gh=h*g/3; box(x,(SH-gh)/2,w,gh); present(); }   // it opens (it only grows, so nothing needs wiping)
    sfxPlay(SFX_POP);
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; int ps=sel;
        if(k&(K_UP|K_DOWN)){ if(++hold>24&&hold%6==0) pr|=k&(K_UP|K_DOWN); } else hold=0;   // hold UP or DOWN to run down a long list
        if(pr&K_DOWN) sel=(sel+1)%n;
        if(pr&K_UP) sel=(sel+n-1)%n;
        if(pr&K_R){ sel+=vis; if(sel>=n) sel=n-1; }
        if(pr&K_L){ sel-=vis; if(sel<0) sel=0; }
        if(sel!=ps){ dirty=1; sfxPlay(SFX_TICK); if(sel<top) top=sel; if(sel>=top+vis) top=sel-vis+1; }
        if(pr&K_A){ sfxPlay(SFX_POP); return sel; }
        if(pr&(K_B|K_START)) return -1;
        if(!dirty){ vsync(); continue; }   // nothing moved: the picture on the screen is still right
        dirty=0;
        box(x,y,w,h); rect(x,y,w,13,RGB(5,12,24)); rect(x,y+13,w,1,GOLD); text(x+6,y+4,title,GOLD,1);
        if(n>vis){ char c[12]; char*e=c; e+=numStr(e,sel+1); *e++='/'; numStr(e,n); text(x+w-6-tw(c,1),y+4,c,DIMC,1); }   // 3/12
        for(int j=0;j<vis;j++){ int i=top+j, yy=y+17+j*10;
            if(i==sel){ rect(x+3,yy-2,w-6-(n>vis?5:0),10,RGB(6,16,8)); rect(x+3,yy-2,2,10,GOLD); text(x+8,yy,">",WHITE,1); }
            text(x+16,yy,it[i],i==sel?WHITE:DIMC,1); }
        if(n>vis){ int th=vis*10*vis/n; if(th<6) th=6; rect(x+w-5,y+15,2,vis*10,RGB(8,12,22)); rect(x+w-5,y+15+(vis*10-th)*top/(n-vis),2,th,GOLD); }   // scroll bar
        text(x+6,y+h-9,"A OK  B BACK",RGB(12,14,16),1);
        present();
    }
}
#include "pie.h"   // the pie menu: contextual interaction (house.h: hhSocR)
static void helpScreen(const char*title,const char*const*ln,int n){   // lines starting with > are headings
    u16 prev=keyNow(); int dirty=1;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_A|K_B|K_START)) return;
        if(!dirty){ vsync(); continue; }   // a help page never changes
        dirty=0;
        box(3,1,234,157); text(14,10,title,GOLD,1);
        for(int i=0;i<n;i++){ const char*l=ln[i]; if(l[0]=='>') text(14,22+i*8,l+1,GOLD,1); else text(18,22+i*8,l,WHITE,1); }
        text(14,144,"PRESS A TO CLOSE",DIMC,1);
        present();
    }
}
static void toast(const char*msg){ int w=tw(msg,1)+16;
    box((SW-w)/2,66,w,22); text((SW-w)/2+8,74,msg,WHITE,1); for(int i=0,n=oToastLen();i<n;i++){ present(); } }
static const char* const lifeHelp[18]={">ON FOOT","DPAD WALK  B RUN  A HOP","L GET ON THE BOARD","R USE FRIDGE TOILET BED SHOWER SOFA",">ON THE BOARD","A PUSH  DPAD STEER  B OLLIE","IN AIR DPAD SPINS  B KICKFLIP  R GLIDES","GREEN MARK = SAFE LANDING  RED = BAIL",">KEEP YOURSELF GOING","WC IS THE TOILET BAR  HP UNDER THE FACE","A OR B GETS YOU UP FROM BED OR SOFA",">WORK  PICK A CAREER ON THE PHONE","TRICK POINTS BEAT THE QUOTA FOR PAY",">WANTS AND FEARS","WANTS FILL THE METER  FEARS DRAIN IT","A GOOD SLEEP ROLLS NEW WANTS AND FEARS","R BY A SIM TALK OR FIGHT  START MENU","SELECT+UP DOWN ZOOM IN OR OUT"};

static const char* const creatureHelp[15]={">PICK YOUR LOOK","L R CHANGE TAB   UP DOWN PICK A ROW","LEFT RIGHT CHANGE IT  A ALSO STEPS","SELECT TURNS THE CREATURE ROUND",">THE TABS","1 BODY  2 FACE  3 HAIR  4 CLOTHES","5 PARTS  TAIL HORNS SPIKES WINGS","  PARTS GIVE POWERS  AND FIGHT BONUSES","  BIG PARTS COST JENES  A BUYS ONE","6 ASPIRE  ASPIRATION  LIFETIME WANT  SIGN","  AND TRAITS THAT SHARE 25 POINTS",">FINISH","START JUMPS TO THE DONE TAB","GO LIVE LIFE PLAYS YOUR CREATURE","LIVING EARNS DNA FOR NEW PARTS"};
static const char* const mapHelp[14]={">BUILD A ROOM","ROOM TOOL  A CORNER  A BUILDS","WALL TOOL  A START  A DRAWS A LINE","FLOOR TOOL  A CORNER  A FILLS","ITEM TOOL  PLACE SINGLE TILES","ERASE TOOL  A CORNER  A CLEARS",">STYLES","L R PICK FLOOR OR ITEM","SEL+L R PICK WALLPAPER","SELECT TAP NEXT TOOL  B CANCELS",">KEEP IT","START OPENS PLAY TEST AND SAVE",">FLOORS","SEL+UP DOWN FLOOR  STAIRS ARE ITEMS"};

// ---------- settings screen ----------
static void drawRoom(int ed);
static void itemSpanInit(void);
// Timer2 (65536 Hz) is the clock for pacing, the speed meter and the load counter.
#define R_TM2D   (*(volatile u16*)0x04000108)
#define R_TM2CNT (*(volatile u16*)0x0400010A)
static void radioTune(int sys);   // (the radio and sound system items: defined with the game music, below)
static int jbLast=-1;   // the last song picked at random anywhere (menus, jukebox, game music): the next pick avoids it
static u32 uiTicks;     // counts frames in the menus: how long you sat there stirs the random numbers
static int pickSong(void){   // ONE random checked song (never the one picked last): visible song number, or -1 when there are no songs
    lrng^=((u32)R_TM2D<<8^uiTicks)*2654435761u; int v=jbPick(jbLast); if(v>=0) jbLast=v; return v;
}
#define TICKS_FRAME 1097   // 65536 / 59.7275 Hz
static void tmStart(void){ R_TM2CNT=0; R_TM2D=0; R_TM2CNT=0x82; }
// graphics fields per preset: fps wall wallpaper floors shadows hud
static const u8 presetTab[4][6]={ {0,0,1,1,1,0}, {0,1,1,1,1,0}, {1,2,0,0,0,1}, {2,2,0,0,0,1} };
static const char* const presetNm[5]={"LOOKS","BALANCED","SPEED","BATTERY","CUSTOM"};
static int sCost, sTunedMsg;   // measured cost of drawing one frame (timer ticks); 1 = just auto-tuned
static int presetOf(void){
    for(int p=0;p<4;p++){ const u8*t=presetTab[p];
        if(sFps==t[0]&&sWall==t[1]&&sWp==t[2]&&sFl==t[3]&&sShad==t[4]&&sHud==t[5]) return p; }
    return 4;
}
static void setPreset(int p){ const u8*t=presetTab[p]; sFps=t[0]; sWall=t[1]; sWp=t[2]; sFl=t[3]; sShad=t[4]; sHud=t[5]; }
static void applyRom(void){ REG_WAITCNT=sRom?0x0003:0x4317; }   // (bits 0-1 = 3: the save chip gets 8 waits, which flash needs)
static void setDefaults(void){ setPreset(1); sCam=1; sSnd=1; sRom=0; sShow=0; sNoWarn=0; applyRom(); }
// Time to draw the room once (timer ticks), averaged over 3 draws. Uses the editor view so it never touches the game state.
static int measureDraw(void){
    drawRoom(1); u16 t0=R_TM2D;
    for(int i=0;i<3;i++) drawRoom(1);
    return (int)(u16)(R_TM2D-t0)/3;
}
static int capLevelC(int cost){   // how many 60 Hz frames one picture really needs: 1 = holds 60 FPS ... 4 = 15 FPS (logic and copy get ~30%)
    int c=cost+TICKS_FRAME*3/10;
    return c<=TICKS_FRAME?1: c<=2*TICKS_FRAME?2: c<=3*TICKS_FRAME?3: 4;
}
static int capLevel(void){ return capLevelC(sCost); }
static void autoTune(void){
    int p; for(p=0;p<4;p++){ setPreset(p); sCost=measureDraw(); if(capLevelC(sCost*2/5)<=sFps+1) break; }   // (playing redraws patches, about 2/5 of this full-room cost)
    if(p==4){ setPreset(3); sCost=measureDraw(); }
    sTunedMsg=1;
}
#include "loading.h"    // LOADING screen with a progress bar (ldShow)
#include "slots.h"      // ROOM SLOTS: named saves of the room, the person and the life (header made ready for houses)
#include "optscreen.h"  // OPTIONS: seven pages of settings (replaces the old SETTINGS screen; settingsScreen() keeps its name)

static const signed char hdT[3][3]={{10,12,14},{8,-1,0},{6,4,2}};   // [sign dy+1][sign dx+1] -> heading (16 steps), -1 = keep
#include "feel.h"
// The name of the trick just landed, into lnBuf (and lnote): "KICKFLIP", "HEEL 540", "180 GRAB", ... A PERFECT landing shows it in gold (lnPerf, hud.h).
static void trickName(int hs,int grab,int perfect){
    char*p=lnBuf; const char*q;
    if(lflip){ q=hs?(bFD<0?"HEEL ":"KICK "):(bFD<0?"HEELFLIP ":"KICKFLIP "); while(*q) *p++=*q++; }
    if(hs){ int d=hs*180; if(d>=1000) *p++=(char)('0'+d/1000%10); *p++=(char)('0'+d/100%10); *p++=(char)('0'+d/10%10); *p++='0'; *p++=' '; }
    if(grab&&!(lflip&&hs)){ q="GRAB "; while(*q) *p++=*q++; }   // (a flip and a spin already fill the top bar)
    if(p==lnBuf){ q="NICE "; while(*q) *p++=*q++; }
    p[-1]=0; lnote=lnBuf; lnPerf=(u8)perfect;
}
static void hhStart(void); static void hhTick(void); static int hhSocR(int useLabel);   // house.h (included further down, next to the drawing it hooks into)
static void lifeInit(void){
    if(!(shapeMask()>>look[LK_SHAPE]&1)){ look[LK_SHAPE]=(u8)maskPick(shapeMask(),look[LK_SHAPE],NSHAPE); if(!custom) buildLook(); }
    flHome(); mapScan(); hhStart();
    bakeSprites(); camSnap=1;
    lfx=spx*256+128; lfy=spy*256+128; lz=lvz=0; lsp=0; lhd=0; lspin=0; lflip=0; lgrind=0; lscore=0; lstun=0; lairF=0; lpts=0; lnoteT=0; lnote=""; lchill=0; lskate=0; lhave=(bdx<0); lfr=0; lvx=lvy=0; ldead=0; lmaxz=0; lplay=0; lbumpCd=0; lfood=100; lbl=0; lhp=HP_MAX; lnear=0; moodReset(); simsReset(); sfxStop(); feelReset(0);
}
static int rampAvg, rampOn;   // px/step (8.8) the skater has been climbing a ramp, smoothed (heights are whole px, so single steps are lumpy); rampOn = rode a ramp last step
// BABY: cannot be steered. A caretaker keeps the needs up and the baby toddles about by itself: stops now and then, picks a new way
// every second or two, and turns round when it walks into something.
static const char* const growNote[AG_N]={"","NOW A CHILD","NOW A TEEN","NOW AN ADULT","NOW AN ELDER"};
static u16 babyPad(void){
    static const u16 dm[9]={0,K_RIGHT,K_LEFT,K_UP,K_DOWN,K_RIGHT|K_DOWN,K_LEFT|K_DOWN,K_RIGHT|K_UP,K_LEFT|K_UP};
    static int t, dir, still; static s32 ox, oy;
    if(lfx==ox&&lfy==oy&&dir) still++; else still=0;
    ox=lfx; oy=lfy;
    if(--t<=0||still>10){ int r=rnd8(); dir=(r&3)==0?0:1+((r>>2)&7); t=40+(rnd8()&63); still=0; }
    return dm[dir];
}
static void phoneMenu(void); static void phTick(void);   // households.h: the PHONE, and the food it ordered
static void storyScreen(void); static void stTick(void); static void stEnter(void); static void stOff(void);   // story.h
static void lifeStep(u16 k,u16 pr,int fr){
    if(stage==AG_BABY&&!ldead){ k=babyPad(); pr=0; }   // uncontrollable stage: the pad is ignored (the pause menu still works)
    int fh=surfH(lfx,lfy)<<8;
    { int tx=(int)(lfx>>8), ty=(int)(lfy>>8); char sc=(tx>=0&&ty>=0&&tx<MW&&ty<MH)?lifeMap[ty][tx]:'.';   // stairs: step on them to change floor (step off and on again to use them once more)
      if(sc!='^'&&sc!='~') flArm=1; else if(flArm&&lz<=fh&&!ldead){ flArm=0; flStairs(sc=='^'?1:-1); return; } }
    if(ldead){   // dead: frozen until A
        lstun=2;
        if(pr&K_A){ ldead=0; lstun=0; lfx=spx*256+128; lfy=spy*256+128; lz=0; lvz=0; lskate=0; lsp=0; lgrind=0; lspin=0; lflip=0; lairF=0; lmaxz=0; lplay=0; lnoteT=0; lfood=100; lbl=0; lhp=HP_MAX; moodReset(); simsRespawn(); sfxStop(); feelReset(0); }
    }
    if(lstun>0){ lstun--; lsp=0; lvx=lvy=0; }
    else {
        if((pr&K_L)&&!lhave){ lnote="FIND A BOARD"; lnoteT=40; }
        if((pr&K_L)&&lhave&&lz<=fh&&stage>=AG_CHILD){   // L: swap between on-foot (walk/run) and skateboard
            lskate=!lskate; lsp=0; lgrind=0; lspin=0; lflip=0; feelReset(lhd); lnote=lskate?"SKATE":"ON FOOT"; lnoteT=40;
        }
        feelSync(); feelTick(lz<=fh,(pr&K_B)!=0&&lskate&&(lz<=fh||F.coy>0));
        if(lskate){
            if(lz<=fh||(F.coy>0&&lvz<=0)){                     // on the ground (or a rail), incl. coyote frames
                feelSteer(k); feelPush(k,lgrind);
                if(F.buf>0){ lvz=feelOllie(); lgrind=0; sfxPlay(SFX_POP); }      // ollie (buffered, variable height) with its pop
            } else feelAir(k,pr,(lz-fh)<(8<<8));               // airborne
            feelVel(); lspin=F.spin>>4;
        } else {
            feelWalk(k,pr,lz<=fh);                             // D-pad relative to screen, B = run, A = hop
        }
    }
    int zp=(int)(lz>>8);
    s32 nx=lfx+lvx, ny=lfy+lvy;   // move per axis so walls slide
    int bump=0, sp0b=lsp;
    int tol=isRamp(lifeMap[lfy>>8][lfx>>8])?F_RAMP_TOL:3;   // a ramp climbs a few px per step without being a wall
    if(surfH(nx,lfy)<=zp+tol) lfx=nx; else bump=1;
    if(surfH(lfx,ny)<=zp+tol) lfy=ny; else bump=1;
    if(bump){
        lsp=(lsp*2)/3;
        if(lbumpCd==0&&sp0b>=(lskate?12:10)){ lbumpCd=40;   // skating into a wall hurts, running into one bonks
            if(lskate&&(abPow()&PW_CHARGE)){ sfxPlay(SFX_HIT); lnote="HORNS FIRST"; lnoteT=30; simEvent(SE_CHARGE); }   // HORNS: charge the wall, no harm done
            else if(lskate) hurt(sp0b*2/3+(rnd8()>>5),2); else sfxPlay(SFX_BONK); }   // (was speed + 0..15: a full speed wall was a coin flip for dying. Now 8..23, worst case a short OW)
    }
    if(lbumpCd>0) lbumpCd--;
    if(lLand>0) lLand--;
    fh=surfH(lfx,lfy)<<8;
    int wasOn=rampOn, onRamp=lskate&&isRamp(lifeMap[lfy>>8][lfx>>8]); rampOn=0;
    if(lz<=fh&&onRamp){ int rise=lz<fh?(int)(fh-lz):0; rampAvg=(rampAvg*3+rise)>>2; rampOn=1; }   // riding a ramp: remember how fast we are climbing
    if(lz<fh){ lz=fh; if(lvz<0) lvz=0; }
    else if(lz>fh&&wasOn&&!onRamp&&lskate&&lvz<=0&&(lz-fh)<(16<<8)){   // rolled off the lip: launch with the climb speed
        int v=rampAvg*F_RAMP_BOOST; if(v>F_RAMP_MAX) v=F_RAMP_MAX; if(v>0){ lvz=v; lnote="AIR"; lnoteT=20; moodEvent(M_LAUNCH); } }
    if(!rampOn) rampAvg=0;
    if(lz>fh||lvz>0){ lz+=lvz; lvz-=0x40;   // gravity
        if((abPow()&PW_GLIDE)&&(k&K_R)&&lvz<0){ lvz+=0x2C; if(lvz<-0xC0) lvz=-0xC0; if(!lglide){ lnote="GLIDE"; lnoteT=30; simEvent(SE_GLIDE); } lglide=1; } else lglide=0;   // WINGS: hold R to float down
        if(lz<=fh&&lvz<=0){ lz=fh; lvz=0; } }
    int air=lz>fh;
    if(air){
        int zz=(int)(lz>>8); if(zz>lmaxz) lmaxz=zz;
        if(!lplay&&lvz<0){ int hi=lmaxz-(int)(fh>>8);
            if(hi>=34){ voxPlay(V_shriek); lplay=1; }                 // falling from way up
            else if(hi>=10&&F.spin&&feelPredGrade()==0){ sfxPlay(SFX_GASP); lplay=1; }   // landing is going wrong (judged from where the spin will end up, not where it is now)
        }
    }
    if(lairF&&!air){                                   // just landed
        int g=feelGrade(), hs=feelHalfTurns(), gb=F.grab>=12, onRail=lskate&&tileH(lfx>>8,lfy>>8)==6;
        if(onRail&&g<2) g=2;                           // a rail catches the board whatever the angle: no bail for a crooked grind
        int pts=hs*180+(lflip?100:0)+feelGrabPts();
        int drop=lmaxz-(int)(lz>>8), sp0=lsp, bail=(g==0);
        lLand=7; lLandD=drop;                          // the landing crouch (playerCalc)
        if(bail){ lnote="BAIL"; lnoteT=60; lsp=0; lstun=BAIL_STUN; lgrind=0; moodEvent(M_BAIL); }
        else{
            if(g==1){ F.spd=F.spd*3/5; lsp=F.spd>>4; pts/=2; lnote="SKETCHY"; lnoteT=40; }   // landed, but crooked: you lose speed and the trick is worth half
            else if(g==3&&pts) pts+=pts/4;                                                      // PERFECT: +25%
            if(pts){ pts=moodPts(pts); lscore+=pts; lpts=pts; if(g!=1) trickName(hs,gb,g==3); lnoteT=60; lcN++; lcPts+=pts; lcT=oComboLen(); moodEvent(M_TRICK); }
            if(onRail){ lgrind=1; lnote="GRIND"; lnoteT=30; lcN++; lcT=oComboLen(); moodEvent(M_GRIND_ON); sfxPlay(SFX_GRIND); }
            else sfxPlay((pts&&g!=1)?SFX_STICK:SFX_LAND);   // the landing is heard: a thud, or the bright one for a trick
        }
        if(bail){ int sv=drop/3+sp0/3+(rnd8()>>6); if(drop<30&&sv>15) sv=15; hurt(sv,1); }   // bad landing: harder/faster/higher = worse (was drop/2+speed: a fast bail was OW + 2.5 s down, or even death)
        else if(drop>24) hurt(drop-24+(rnd8()>>5),0);   // big drops hurt even landed clean
        lspin=0; lflip=0; feelLandReset();
    }
    if(!air){ lmaxz=(int)(lz>>8); lplay=0; }
    lairF=air;
    if(lflip&&lskate&&air){ if(!bFPrev) bFD=(k&K_UP)?-1:1; if(bFT<BFLIP_LEN) bFT++; bFPrev=1; } else { bFT=0; bFPrev=0; }   // the flip: one full roll in BFLIP_LEN steps, then it is flat again
    if(lgrind){ if(air||tileH(lfx>>8,lfy>>8)!=6) lgrind=0; else if((fr&3)==0){ int g=abGrindPts(); lscore+=g; lnote="GRIND"; lnoteT=10; lcPts+=g; lcT=oComboLen(); } }   // GRIP ability
    if(!lhave&&lz<(8<<8)&&(lfx>>8)==BDX&&(lfy>>8)==BDY){ lhave=1; lnote="GOT A SKATEBOARD"; lnoteT=90; moodEvent(M_GOT_BOARD); }   // walk over it to pick it up
    fxTick();   // ghosts and weather (fx.h): every step, also while you lie dead
    if(!ldead){   // needs: hunger and bladder, then how they (and the skating) make the skater feel
        if(stage==AG_BABY){ if(lfood<70) lfood=70; if(lbl>30) lbl=30; if(sNrg<60) sNrg=60; if(sHyg<60) sHyg=60; if(sCom<60) sCom=60; }   // looked after
        moodTick(); simsTick(pr,(int)(lfx>>8),(int)(lfy>>8)); hhTick(); phTick(); stTick();
        if(gGrow){ gGrow=0; setStage(stage+1); bakeSprites(); lnote=growNote[stage]; lnoteT=120; lstun=lstun>30?lstun:30; lsp=0; }
        { int fe=oFoodEvery(), we=oWcEvery();   // FOOD AND WC option
          if(fe&&lfr%fe==0&&lfood>0) lfood--;
          if(lchill>0){ lchill--; if(fe&&lfr%fe==fe/2&&lfood>0) lfood--;   // the munchies: hunger twice as fast while chilled out
              if(lchill==900&&lnoteT<=0){ lnote="THE MUNCHIES"; lnoteT=60; } if(!lchill&&lnoteT<=0){ lnote="BACK TO NORMAL"; lnoteT=40; } }
          if(we&&lfr%we==0&&lbl<100) lbl++; }
        if(lhp<HP_MAX&&lfood>=25&&lstun<=0&&lfr%HP_REGEN==0) lhp++;   // HEALTH creeps back while you are fed and on your feet
        if(lfood==0&&lfr%300==0){ lfood=15; lstun=120; lsp=0; lgrind=0; sfxPlay(SFX_GROAN); lnote="FAINTED FROM HUNGER"; lnoteT=90; moodEvent(M_FAINT); }
        { static u8 vxH, vxP; if(lfood<SIM_LOW){ if(!vxH){ vxH=1; voxNag(V_im_hungryrururyry); } } else if(lfood>=40) vxH=0;   // the hunger and the bladder speak up once each time they run low
          if(lbl>=80){ if(!vxP){ vxP=1; voxNag(V_need_to_pee); } } else if(lbl<50) vxP=0; }
        if(lbl>=100){ lbl=0; lstun=90; lsp=0; lgrind=0; lscore=lscore>100?lscore-100:0; sfxPlay(SFX_CRY); lnote="ACCIDENT"; lnoteT=90; moodEvent(M_ACCIDENT); }
        int nf=0, nt=0, nb=0, nh=0, nc=0, np=0, nq=0, nph=0, nrd=0, nsy=0;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){ int tx=(lfx>>8)+dx, ty=(lfy>>8)+dy; if(tx<0||ty<0||tx>=MW||ty>=MH) continue;
            char c=lifeMap[ty][tx]; if(c=='F') nf=1; if(c=='T') nt=1; if(c=='S') nb=1; if(c=='H') nh=1; if(c=='C'||c=='U') nc=1; if(c=='G') np=1; if(c=='Q') nq=1; if(c=='I') nph=1; if(c=='R') nrd=1; if(c=='A') nsy=1; }
        lnear=nf?1:(nt?2:(nb?3:(nh?4:(np?6:(nq?7:(nph?8:(nrd?9:(nsy?10:(nc?5:0)))))))));   // 7 the DeadSet   // 1 fridge, 2 toilet, 3 bed, 4 shower, 6 water pipe, 5 sofa or beanbag
        if((pr&K_R)&&lstun<=0&&lz<=fh&&!simAct&&hhSocR(lnear)) pr&=~K_R;   // next to a household Sim: the social menu (it offers the furniture too)
        if((pr&K_R)&&lnear&&lstun<=0&&lz<=fh){
            if(lnear==1){   // fridge: eat
                if(lfood>=95){ lnote="FULL"; lnoteT=40; }
                else { lfood+=35; if(lfood>100) lfood=100; lbl+=10; if(lbl>99) lbl=99; lstun=30; lsp=0; lnote="YUM"; lnoteT=50; moodEvent(M_EAT); }
            } else if(lnear==7){   // the DeadSet 3Thousand VYBE: strap it on and vanish into virtual reality for a while
                lstun=150; lsp=0; lgrind=0; lnote="JACKED IN  VYBE 3000"; lnoteT=150; moodEvent(M_CHILL); moodEvent(M_COMBO);
            } else if(lnear==6){   // the water pipe (grown-ups only): CHILLED OUT for two game hours
                if(!pipeOk()){ lnote=stage==AG_TEEN&&xo[XO_PIPEAGE]?"NOT OLD ENOUGH YET":"GROWN-UPS ONLY"; lnoteT=50; }
                else if(lchill>0){ lnote="STILL CHILLIN"; lnoteT=40; }
                else { lchill=1800; lstun=80; lsp=0; lgrind=0; lnote="PUFF PUFF  CHILLED OUT"; lnoteT=80; moodEvent(M_CHILL); simEvent(SE_PIPE); }
            } else if(lnear==8){   // the telephone: invite someone, order food, pick a career
                phoneMenu(); liveInvalidate(); camSnap=1; while((~REG_KEYINPUT)&0x3FF) vsync();
            } else if(lnear==9||lnear==10){ radioTune(lnear==10);   // the radio / the sound system (sound pack): next station
            } else if(lnear>=3){ simBegin(lnear);   // bed / shower / sofa (sims.h)
            } else {        // toilet: relieve yourself
                if(lbl<15){ lnote="LATER"; lnoteT=40; }
                else { lbl=0; lstun=70; lsp=0; lgrind=0; lnote="AHH"; lnoteT=60; moodEvent(M_RELIEVE); }
            }
        }
    }
    if(lcN>0){
        if(lstun>0||ldead){ lcN=0; lcPts=0; lcT=0; }                      // a bail or hit loses the chain
        else if(!air&&!lgrind&&--lcT<=0){                                  // chain over: bank the multiplier bonus
            int tot=lcPts*lcN; if(lcN>=2) lscore+=lcPts*(lcN-1);
            lcBank=tot; lcBankT=120;
            if(lcN>=2&&sCam&&tot>camThr[sCam]) lcamPend=1;
            if(lcN>=2) moodEventN(M_COMBO,lcN-1);
            lcN=0; lcPts=0;
        }
    }
    if(lcBankT>0) lcBankT--;
    lfr++;
    if(lnoteT>0) lnoteT--;
    sfxTick();
}
static int ecx=6, ecy=6, efr;   // map editor cursor (tile) and frame counter
// The room can be viewed from 4 sides (action cam). (rx,ry) are screen-space tile coords for the current view, (tx,ty) the real map tile.
static void rotXY(int rx,int ry,int*tx,int*ty){
    switch(cview){ case 0:*tx=rx;*ty=ry;break; case 1:*tx=ry;*ty=MW-1-rx;break; case 2:*tx=MW-1-rx;*ty=MH-1-ry;break; default:*tx=MH-1-ry;*ty=rx; }
}
static void rotPos(s32 x,s32 y,s32*rx,s32*ry){   // same for a position in 1/256 tiles
    switch(cview){ case 0:*rx=x;*ry=y;break; case 1:*rx=MH*256-y;*ry=x;break; case 2:*rx=MW*256-x;*ry=MH*256-y;break; default:*rx=y;*ry=MW*256-x; }
}
static char cellAt(int rx,int ry){ int tx,ty; rotXY(rx,ry,&tx,&ty); return lifeMap[ty][tx]; }
static int wpAt(int rx,int ry){ int tx,ty; rotXY(rx,ry,&tx,&ty); return wallMap[ty][tx]; }
static int flAt(int rx,int ry){ int tx,ty; rotXY(rx,ry,&tx,&ty); return floorMap[ty][tx]; }
static int isWallCh(char c){ return c=='w'||c=='W'; }
// ---- camera: follows the player (play) or the cursor (editor); only the tiles on screen are drawn ----
static int vpY0=0, vpY1=SH, vpX0=0, vpX1=SW, sbY0=0, sbY1=SH;   // vpX0..vpX1: the columns drawn (all, or the ZOOM window); sbY0..sbY1: the room rows of the screen   // rows of the screen the scene lives in (life mode keeps the HUD panels above and below; the editor uses it all)
static void camClamp(int ed){
    int xl=120-MH*CA-vpX0, xh=120+MW*CA-vpX1, yl=24-vpY0-(ed?20:0), yh=24+(MW+MH)*CB-vpY1+(ed?20:0);
    if(camX<xl) camX=xl; if(camX>xh) camX=xh; if(camY<yl) camY=yl; if(camY>yh) camY=yh;
}
static void camFollow(int snap){   // keep the skater near the middle of the screen, eased so it stays steady
    s32 rfx,rfy; rotPos(lfx+lvx*(lskate?14:10),lfy+lvy*(lskate?14:10),&rfx,&rfy);   // look ahead of the skater (a little less on foot)
    int playY=vpY0+(vpY1-vpY0)*5/8;                                                  // screen row of the feet (100 on the full screen)
    int ox=camX, oy=camY; camX=(int)((rfx-rfy)>>5); camY=(int)((rfx+rfy)>>6)-(playY-24); camClamp(0);
    int tx=camX, ty=camY; camX=ox; camY=oy;
    if(cview!=camLastV){ camLastV=cview; snap=1; }
    if(snap){ camX=tx; camY=ty; return; }
    int ease=lskate?4:3, dx=tx-camX, dy=ty-camY, sx=dx/ease, sy=dy/ease;   // on foot the camera catches up a bit faster
    if(!sx) sx=(dx>0)-(dx<0); if(!sy) sy=(dy>0)-(dy<0);
    camX+=sx; camY+=sy;
}
static int fdiv(int a,int b){ return a>=0?a/b:-((-a+b-1)/b); }   // floor division, b > 0
// The diagonals (tx+ty) and, on each, the tiles whose art can touch the rectangle x0..x1 / y0..y1 (a little generous: a tile's art reaches
// 23 px above its centre, 5 below, 11 to each side). Drawing extra tiles is harmless, they are clipped.
static void bandRows(int y0,int y1,int*s0,int*s1){
    int lo=fdiv(y0-14-LOY,CB)-1, hi=fdiv(y1+26-LOY,CB)+1;
    if(lo<0) lo=0; if(hi>MW+MH-2) hi=MW+MH-2; *s0=lo; *s1=hi;
}
static void bandCols(int s,int x0,int x1,int*a,int*b){
    int kmin=fdiv(x0-12-LOX,CA)-1, kmax=fdiv(x1+12-LOX,CA)+1;
    int lo=(s+kmin)>>1, hi=(s+kmax+1)>>1, mn=s-(MH-1), mx=s<MW-1?s:MW-1;
    if(mn<0) mn=0; if(lo<mn) lo=mn; if(hi>mx) hi=mx; *a=lo; *b=hi;
}
// ---------- walls (The Sims style) ----------
// A wall tile is drawn as a thin, tall panel through the middle of the tile: half a segment towards every neighbouring wall tile, so walls
// join up into lines and corners, with the floor of the room drawn under them. Only the side the camera sees is drawn (the wallpaper,
// pre-shaded per face), with a light trim along the top. CUTAWAY (OPTIONS > VIDEO > WALLS): a segment that hides the inside of a room
// behind it drops to a low stub, the others stay full height. "Inside" = floor that cannot be reached from the edge of the map without
// crossing a wall or a doorway (a one-tile gap in a wall); wallsScan works it out again whenever the map changed (wDirty).
// Wallpapers 0..NWP-1 are the old 8x8 patterns (tiled up the wall), NWP.. are the textures in wallart.h (8 x WALL_H, ROM only).
#define WALL_CUT 5    // a cut-away segment
#define WALL_LOW 8    // a low wall ('w')
static u8 wInside[MH][MW] EWRAM_BSS; static u8 wDirty=1;
static u16 bfsQ[MH*MW] EWRAM_BSS;   // one queue for every breadth-first search (the walls' flood here, the Sims' paths in house.h)
static int wIsWall(int x,int y){ return x>=0&&y>=0&&x<MW&&y<MH&&lifeMap[y][x]=='W'; }   // rooms are closed by full walls (a low wall is a fence)
static int wDoor(int x,int y){ return (wIsWall(x-1,y)&&wIsWall(x+1,y))||(wIsWall(x,y-1)&&wIsWall(x,y+1)); }
static void wallsScan(void){   // flood the outside from the map edge; everything else that is not a wall is inside
    u16*q=bfsQ; int qh=0, qt=0;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) wInside[y][x]=1;
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if((x==0||y==0||x==MW-1||y==MH-1)&&!wIsWall(x,y)){ wInside[y][x]=0; q[qt++]=(u16)(y*MW+x); }
    while(qh<qt){ int p=q[qh++], x=p%MW, y=p/MW;
        for(int d=0;d<4;d++){ int nx=x+(d==0)-(d==1), ny=y+(d==2)-(d==3); if(nx<0||ny<0||nx>=MW||ny>=MH||!wInside[ny][nx]) continue;
            if(wIsWall(nx,ny)||wDoor(nx,ny)) continue; wInside[ny][nx]=0; q[qt++]=(u16)(ny*MW+nx); } }
    wDirty=0;
}
static int wInAt(int rx,int ry){ if(rx<0||ry<0||rx>=MW||ry>=MH) return 0; int tx,ty; rotXY(rx,ry,&tx,&ty); return wInside[ty][tx]&&!isWallCh(lifeMap[ty][tx]); }
static int wallAtR(int rx,int ry){ return rx>=0&&ry>=0&&rx<MW&&ry<MH&&isWallCh(cellAt(rx,ry)); }
static int wallFloorR(int rx,int ry){   // the floor to draw under a wall tile: a neighbour's (inside first)
    static const signed char nd[4][2]={{0,1},{1,0},{0,-1},{-1,0}}; int best=-1;
    for(int k=0;k<4;k++){ int x=rx+nd[k][0], y=ry+nd[k][1]; if(x<0||y<0||x>=MW||y>=MH||isWallCh(cellAt(x,y))) continue; if(wInAt(x,y)) return flAt(x,y); if(best<0) best=flAt(x,y); }
    return best<0?flAt(rx,ry):best;
}
static const char* wpName(int wp){ return wp<NWP?wpTex[wp].nm:wxName[wp-NWP]; }
static u16 wpAvgOf(int wp){ return wp<NWP?wpAvg[wp]:wxAvg[wp-NWP]; }
// one segment of wall: columns xa..xb of a tile whose centre (on the floor) is sx,sy. dir 0 runs along x (the camera sees its +y face),
// dir 1 along y (+x face). h = height in px. edge: bit 0 = column xa is an end or corner, bit 1 = column xb.
IWRAM_CODE static void wallSeg(int sx,int sy,int xa,int xb,int dir,int h,int wp,int edge){
    int ye=cY0+(int)cH-1, per, v0;
    u16 av=wpAvgOf(wp), flat=shade(av,dir?9:12), trim=lite(av,20), dark=shade(av,6);
    for(int x=xa;x<=xb;x++){
        if((unsigned)(x-cX0)>=cW) continue;
        int off=x-sx, base=dir?sy-(off>>1):sy+(off>>1), top=base-h, u=dir?(4-off)&7:(off+4)&7;
        const u16*col; if(wp<NWP){ col=wpTab[wp][dir][u]; per=8; v0=0; } else { col=wxTex[wp-NWP][dir][u]; per=WALL_H; v0=WALL_H-h; }
        int ya=top-1<cY0?cY0:top-1, yz=base>ye?ye:base; if(ya>yz) continue;
        u16*d=&fb[ya*SW+x];
        if(((edge&1)&&x==xa)||((edge&2)&&x==xb)){ for(int y=ya;y<=yz;y++,d+=SW) *d=dark; continue; }   // an end or a corner: an outline
        for(int y=ya;y<=yz;y++,d+=SW){
            if(y==top-1) *d=dark; else if(y==top) *d=trim;                 // the top of the wall: an outline and a light trim
            else if(!sWp) *d=flat;
            else { int v=v0+(y-top-1); if(per==8) v&=7; *d=col[v]; } }
    }
}
static void drawWall(int tx,int ty,int sx,int sy){   // tx,ty in screen-rotated tile coords
    if(wDirty) wallsScan();
    int low=cellAt(tx,ty)=='w', wp=wpAt(tx,ty); if(wp>=NWALL) wp=0;
    int nxm=wallAtR(tx-1,ty), nxp=wallAtR(tx+1,ty), nym=wallAtR(tx,ty-1), nyp=wallAtR(tx,ty+1);
    int hx=low?WALL_LOW:sWall==2?WALL_CUT:(sWall==1&&wInAt(tx,ty-1))?WALL_CUT:WALL_H;   // a wall along x hides what is at y-1
    int hy=low?WALL_LOW:sWall==2?WALL_CUT:(sWall==1&&wInAt(tx-1,ty))?WALL_CUT:WALL_H;   // a wall along y hides what is at x-1
    int cx=(nxm||nxp), cy=(nym||nyp), corner=cx&&cy;
    if(!cx&&!cy){ wallSeg(sx,sy,sx-3,sx+3,0,hx,wp,3); return; }   // a lone pillar
    if(nxm) wallSeg(sx,sy,sx-4,sx,0,hx,wp,corner?2:0);             // back halves first, then the front ones
    if(nym) wallSeg(sx,sy,sx,sx+4,1,hy,wp,corner?1:0);
    if(nxp) wallSeg(sx,sy,sx,sx+4,0,hx,wp,(corner?1:0)|(cy&&!nxm&&!nym?1:0));
    if(nyp) wallSeg(sx,sy,sx-4,sx,1,hy,wp,corner?2:0);
    if(cx&&!cy){ if(!nxm) wallSeg(sx,sy,sx,sx,0,hx,wp,1); if(!nxp) wallSeg(sx,sy,sx,sx,0,hx,wp,1); }   // a free end: a clean edge
    if(cy&&!cx){ if(!nym) wallSeg(sx,sy,sx,sx,1,hy,wp,1); if(!nyp) wallSeg(sx,sy,sx,sx,1,hy,wp,1); }
}
static void tileMark(int tx,int ty,u16 cc){   // diamond outline on a tile (editor cursor / preview)
    char c=lifeMap[ty][tx]; int hgt=isWallCh(c)?0:tileH(tx,ty);
    int sx=LOX+(tx-ty)*CA, sy=LOY+(tx+ty+1)*CB-hgt;
    if(sx<-CA-1||sx>SW+CA||sy<-CB-2||sy>SH+CB+2) return;   // off screen
    for(int t=-CA;t<=CA;t++){ int at=t<0?-t:t, hh=hhT[0][at]; px(sx+t,sy-hh,cc); px(sx+t,sy-hh-1,cc); px(sx+t,sy+hh,cc); px(sx+t,sy+hh+1,cc); }
}
static void eRect(int*x0,int*y0,int*x1,int*y1){   // anchor..cursor as an ordered rectangle; the WALL tool snaps to a straight line
    int ax=eAx, ay=eAy, bx=ecx, by=ecy;
    if(eTool==T_WALL){ int dx=bx>ax?bx-ax:ax-bx, dy=by>ay?by-ay:ay-by; if(dx>=dy) by=ay; else bx=ax; }
    *x0=ax<bx?ax:bx; *x1=ax<bx?bx:ax; *y0=ay<by?ay:by; *y1=ay<by?by:ay;
}
#include "items.h"
static int lpsx, lpsy;   // where the player is on screen (zoom centre)
// Which of the 4 baked views to show for a heading (16 steps, 0 = +x, 4 = +y ...). View v has its face on: 0 down-left, 3 down-right, 2 up-right, 1 up-left.
// The pad walks along the screen's up / down / left / right, which are the diagonals of the tile grid, so those four headings sit between two views:
// they pick the one that reads right (down and left show the face, right shows the face, up shows the back).
static const u8 faceView[16]={3,3,0,0,0,0,0,1,1,1,2,2,2,2,3,3};
#include "house.h"   // households: up to 7 more Sims with free will, SELECT switches who you control
static int plX, plY, plZ, plFh, plV, plBob;   // feet on screen, height above the floor, floor height under the feet, which baked view
static int plDip, plMk;   // plDip: px the skater crouches for a few frames after a landing; plMk: the landing mark under a spinning skater (0 none, 1 red = bail, 2 yellow = sketchy, 3 green = clean, 4 bright = perfect)
static void playerCalc(void){
    s32 rfx,rfy; rotPos(lfx,lfy,&rfx,&rfy);
    plX=LOX+(int)((rfx-rfy)>>5); plY=LOY+(int)((rfx+rfy)>>6);
    plFh=surfH(lfx,lfy); plZ=(int)(lz>>8); plV=faceView[(lhd+lspin+4*cview)&15];
    plBob=(!lskate&&plZ<=plFh&&(lvx|lvy)&&lstun<=2)?(int)((lfr>>3)&1):0;   // a little step bounce while he walks
    lpsx=plX; lpsy=plY-20;
    plDip=(lskate&&lLand>0&&plZ<=plFh)?(lLand>4?(lLandD>=10?3:2):1):0;   // landing crouch: the harder the drop the lower, easing back up over 7 frames
    plMk=0; if(lskate&&plZ>plFh&&(F.spinV||F.spin>=20||F.spin<=-20)){ int g=feelPredGrade(); plMk=g==0?1:g==1?2:g==2?3:4; }   // spinning: will it land?
    if(lskate){   // the board
        int air2=plZ>plFh, gp=F.grab>8?8:F.grab;
        bdA=((F.angF>>4)+F.spin+64*cview)&255;                        // heading + the spin of the trick, turned with the camera
        bdPitch=air2?(int)(lvz>>7):0; if(bdPitch>4) bdPitch=4; if(bdPitch<-3) bdPitch=-3;   // an ollie: nose up on the way up, nose down on the way down
        if(air2&&F.grab>0) bdPitch+=gp/4;                              // a grab pulls it up and tips it
        bdRaise=(air2&&F.grab>0)?gp*5/8:0;                             // ... towards the hand
        bdRoll=(bFT>0&&bFT<BFLIP_LEN)?((bFD*bFT*256/BFLIP_LEN)&255):0; // kickflip / heelflip
        bdSpk=lgrind?1+(lfr&3):0;                                      // grind sparks
    } else { bdA=bdPitch=bdRoll=bdRaise=bdSpk=0; }
    hhCalc();
}
// The board: a real deck, about as long as the rider is wide (scaled by life stage), drawn as a flat slab in the room's isometric grid under the feet.
// It points the way the rider faces and turns with spins, noses up and down in an ollie, rolls over for a KICKFLIP / HEELFLIP (the grip side is
// the red top, the underside shows trucks and wheels), lifts to the hand in a grab and throws sparks on a grind.
static void drawBoard(void){
    static const u8 bdSc[AG_N]={70,85,95,100,100};
    int sc=bdSc[stage<AG_N?stage:AG_ADULT], hl=270*sc/100, hw=56*sc/100;
    int c=fcos(bdA), s=fsin(bdA), cr=fcos(bdRoll), sr=fsin(bdRoll);
    int ex=((c-s)*hl)>>13, ey=((c+s)*hl)>>14;                         // nose offset on screen (map x -> 8,4 px per tile, y -> -8,4)
    int pxv=((-s-c)*hw)>>13, pyv=((c-s)*hw)>>14;                      // across the deck
    int lx=(pxv*cr)>>8, ly=((pyv*cr)>>8)-((2*sr)>>8);                 // the roll tips the across vector up out of the floor plane
    int wx=(pxv*sr)>>8, wy=((pyv*sr)>>8)+((2*cr)>>8);                 // which way the wheels hang (2 px under a flat deck)
    int cx=plX, cy=plY-plZ-bdRaise;
    int nx=cx+ex, ny=cy+ey-bdPitch, tx=cx-ex, ty=cy-ey+(bdPitch+1)/2;   // nose / tail
    int top=cr>=0;
    u16 deck=top?RGB(27,9,6):RGB(9,9,12), edge=top?RGB(19,5,4):RGB(5,5,7), hi=top?RGB(31,24,9):RGB(14,14,18);
    int wf=top?0:1;
    for(int pass=0;pass<2;pass++){
        if(pass==wf){   // wheels: two trucks, one pair at each end (under the deck when it is flat, over it when it is upside down)
            for(int e=-1;e<=1;e+=2) for(int sd=-1;sd<=1;sd+=2){
                int wxp=cx+ex*6*e/10+lx*sd+wx, wyp=cy+ey*6*e/10-(e>0?bdPitch:-(bdPitch+1)/2)*6/10+ly*sd+wy;
                rect(wxp-1,wyp-1,2,2,RGB(25,24,20)); px(wxp-1,wyp-1,RGB(31,31,28)); }
        } else {        // the deck: seven lines across, an edge line on each side, a stripe down the middle, a lighter nose
            for(int k=-3;k<=3;k++){ int ox=lx*k/3, oy=ly*k/3; line(tx+ox,ty+oy,nx+ox,ny+oy,(k==-3||k==3)?edge:deck); }
            line(tx+lx*3/3,ty+ly*3/3+1,nx+lx*3/3,ny+ly*3/3+1,edge); line(tx-lx*3/3,ty-ly*3/3+1,nx-lx*3/3,ny-ly*3/3+1,edge);   // the thickness
            line(tx+(nx-tx)/5,ty+(ny-ty)/5,nx-(nx-tx)/5,ny-(ny-ty)/5,hi);
            px(nx,ny,hi); px(nx+(lx>0?1:-1),ny,hi); }
    }
    if(bdSpk){ px(tx+bdSpk-2,ty-bdSpk,RGB(31,29,8)); px(tx-bdSpk,ty-(bdSpk>>1)-1,RGB(31,31,24)); px(tx+(bdSpk>>1),ty-bdSpk-2,RGB(31,20,4)); }
}
static void drawPlayerNow(void){
    if(sShad){ rect(plX-3,plY-plFh-1,7,2,RGB(10,8,5)); rect(plX-1,plY-plFh-2,3,4,RGB(10,8,5)); }   // shadow
    if(plMk){   // the landing mark: a bar under the shadow, red / yellow / green, wider when it is a perfect landing
        u16 mc=plMk==1?RGB(30,8,6):plMk==2?RGB(30,26,6):plMk==3?RGB(8,27,11):RGB(14,31,16); int w=plMk==4?6:4;
        rect(plX-w,plY-plFh+2,2*w+1,2,mc); }
    if(lskate) drawBoard();   // board under the feet
    blit((plBob&&!lskate)?spr4s[plV]:spr4[plV],plX-16,plY-SPF-plZ-plBob+plDip);   // walking: the stride frame on the up-step
}
// The room inside the rectangle x0..x1 / y0..y1 (end excluded), drawn back to front and clipped to it: the same pixels a whole-screen
// draw would put there. ed=1: editor view (no player).
static inline int isItemCh(char c){
    switch(c){ case '#': case 'F': case 'T': case '=': case 'D': case 'L': case 'N': case 'S': case 'H': case 'C': case 'X': case 'O': case 'Y': case 'Z': case 'K': case 'J': case 'M': case 'G': case 'V': case 'U': case 'Q': case 'I': case 'R': case 'A': case '^': case '~': return 1; }
    return isRamp(c);
}
static void drawRoomRect(int x0,int y0,int x1,int y1,int ed){
    clipSet(x0,y0,x1,y1);
    rect(x0,y0,x1-x0,y1-y0,RGB(4,5,8));
    int s0,s1; bandRows(y0,y1,&s0,&s1);
    for(int s=s0;s<=s1;s++){ int a,b; bandCols(s,x0,x1,&a,&b);
        for(int tx=a;tx<=b;tx++){ int ty=s-tx;
            int sx=LOX+(tx-ty)*CA, sy=LOY+(tx+ty+1)*CB;
            if(sx+CA<x0||sx-CA>=x1||sy+CB<y0||sy-CB>=y1) continue;   // the diamond does not reach the rectangle
            CNT(cntTiles); char c=cellAt(tx,ty); if(c=='#') continue;
            { int fl=(c=='w'||c=='W')?wallFloorR(tx,ty):flAt(tx,ty), v=(tx^ty)&1; if(sFl) floorTile(sx,sy,&flTab[fl][v][0][0]); else tileTop(sx,sy,flFlat[fl][v]); } } }   // (walls are thin now: the room's floor runs under them)
    int ss=0; if(!ed){ s32 rfx,rfy; rotPos(lfx,lfy,&rfx,&rfy); ss=(int)((rfx>>8)+(rfy>>8)); }
    for(int s=s0;s<=s1;s++){ int a,b; bandCols(s,x0,x1,&a,&b);
        for(int tx=a;tx<=b;tx++){ int ty=s-tx;
            int sx=LOX+(tx-ty)*CA, sy=LOY+(tx+ty+1)*CB;
            if(sx+11<=x0||sx-11>=x1||sy+6<=y0||sy-24>=y1) continue;   // art (walls, items, the pickup) is at most 11 px to a side, 24 above and 5 below the centre
            char c=cellAt(tx,ty); if(c=='.'&&(ed||lhave)) continue;   // plain floor: nothing stands there (but the board pickup might)
            int ox,oy; rotXY(tx,ty,&ox,&oy);
            if(c=='.'&&(ox!=BDX||oy!=BDY)) continue;
            if(c=='w'||c=='W') drawWall(tx,ty,sx,sy);
            if(isItemCh(c)) drawItemTile(c,sx,sy,ox,oy);
            if((ed&&c=='B')||(!ed&&!lhave&&ox==BDX&&oy==BDY)) blitItem(V_BOARD,sx,sy-(ed?0:((lfr>>4)&1)));   // the skateboard pickup, bobbing
            if(ed&&c=='P') drawSpawn(sx,sy+1);   // little person = spawn
        }
        if(!ed&&hhN&&!curFl) hhDrawBand(s,s);
        if(!ed&&s==ss) drawPlayerNow();
    }
    if(!ed&&hhN&&!curFl) hhDrawBand(s1+1,9999);
    if(!ed&&ss>s1) drawPlayerNow();   // the feet are below the rectangle but the head is inside it: nothing in front can reach it, so draw last
    clipAll();
}
static void drawRoom(int ed){   // the whole screen (editor, speed test)
    if(!ed) playerCalc();
    drawRoomRect(0,0,SW,SH,ed);
    if(ed){
        if(eAct&&eTool!=T_ITEM){   // preview of what the next A will build
            int x0,y0,x1,y1; eRect(&x0,&y0,&x1,&y1);
            u16 pc=eTool==T_ERASE?RGB(31,10,8):RGB(10,28,10);
            for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
                if(eTool==T_ROOM&&x!=x0&&x!=x1&&y!=y0&&y!=y1) continue;   // a room only outlines its walls
                tileMark(x,y,pc);
            }
        }
        if(eTool==T_ITEM){ char gc=edObjCh(); int hg=isWallCh(lifeMap[ecy][ecx])?0:tileH(ecx,ecy); drawItemTile(gc,LOX+(ecx-ecy)*CA,LOY+(ecx+ecy+1)*CB-hg,ecx,ecy); }   // BUY: the item you would place, standing on the tile
        tileMark(ecx,ecy,(efr&8)?WHITE:(eTool==T_ITEM&&!edAffordable(edObjCh(),lifeMap[ecy][ecx]))?RGB(31,10,8):GOLD);   // blinking diamond on the tile under the cursor (red: you cannot afford it)
    }
}

// Camera zoom: scale the finished picture up around (cx,cy) in place. zk = 256 / zoom. Source pixels are always nearer the
// centre than their destination, so working outward from the centre never reads a pixel that was already overwritten.
static short zxm[SW], zym[SH];
IWRAM_CODE static void zoomFb(int cx,int cy,int zk){   // only the scene rows (vpY0..vpY1-1) are zoomed: the HUD panels stay put
    for(int x=0;x<SW;x++) zxm[x]=(short)(cx+(((x-cx)*zk)>>8));
    for(int y=vpY0;y<vpY1;y++) zym[y]=(short)(cy+(((y-cy)*zk)>>8));
    for(int pass=0;pass<2;pass++){
        int y0=pass?vpY0:vpY1-1, y1=pass?cy:cy-1, st=pass?1:-1;
        int prevY=-1, prevS=-1;   // the row done just before and the source row it was made from
        for(int y=y0;y!=y1;y+=st){
            u16*d=fb+y*SW; const u16*s=fb+zym[y]*SW;
            if(zym[y]==prevS){   // same source row as the row before: the zoomed row is already there, copy it (8 words per round) instead of gathering 240 pixels
                const u32*q=(const u32*)(fb+prevY*SW); u32*o=(u32*)d;
                for(int i=0;i<SW/16;i++){ u32 a=q[0],b=q[1],c=q[2],e=q[3],f=q[4],g=q[5],h=q[6],j=q[7]; q+=8; o[0]=a; o[1]=b; o[2]=c; o[3]=e; o[4]=f; o[5]=g; o[6]=h; o[7]=j; o+=8; }
            } else {
                int x=SW-1;   // right half, outer edge inwards: two pixels per store (x stays odd, so the pair (x-1,x) is word aligned; both are read before either is written)
                for(;x-1>=cx;x-=2){ u32 a=s[zxm[x-1]], b=s[zxm[x]]; *(u32*)(d+x-1)=a|(b<<16); }
                for(;x>=cx;x--) d[x]=s[zxm[x]];
                x=0;      // left half, outer edge inwards: the pair (x,x+1) with x even
                for(;x+1<cx;x+=2){ u32 a=s[zxm[x]], b=s[zxm[x+1]]; *(u32*)(d+x)=a|(b<<16); }
                for(;x<cx;x++) d[x]=s[zxm[x]];
            }
            prevY=y; prevS=zym[y];
        }
    }
}
// a 7x7 face for the mood state (SAD, BORED, OK, HAPPY, STOKED); 'y' = skin, 'k' = features
static const char* const faceArt[5][7]={
 {".yyyyy.","yyyyyyy","yykykyy","yyyyyyy","yykkkyy","ykyyyky",".yyyyy."},   // sad
 {".yyyyy.","yyyyyyy","ykkykky","yyyyyyy","yykkkyy","yyyyyyy",".yyyyy."},   // bored: half-shut eyes, flat mouth
 {".yyyyy.","yyyyyyy","yykykyy","yyyyyyy","yykkkyy","yyyyyyy",".yyyyy."},   // ok
 {".yyyyy.","yyyyyyy","yykykyy","yyyyyyy","ykyyyky","yykkkyy",".yyyyy."},   // happy
 {".yyyyy.","yyyyyyy","ykkykky","yyyyyyy","ykkkkky","yykkkyy",".yyyyy."} }; // stoked
static void drawFace(int x,int y,int st){
    static const u16 skin[5]={RGB(14,18,28),RGB(22,22,20),RGB(30,26,8),RGB(26,30,10),RGB(31,20,6)};
    for(int j=0;j<7;j++)for(int i=0;i<7;i++){ char c=faceArt[st][j][i]; if(c=='.') continue; px(x+i,y+j,c=='k'?RGB(4,3,6):skin[st]); }
}
#include "hud.h"
// ---------- the life scene: scroll and patch ----------
// Redrawing the whole room every picture is far too slow for the GBA. Instead the screen itself (VRAM) is the picture: when the camera moves it is slid
// over by the camera's step (one DMA per row, during the vertical blank) and only what changed is drawn: the strip the slide uncovered, the rectangle
// round the player (where he was and where he is) and a bobbing pickup. Each rectangle is drawn into fb at its own place on screen (clipped, so it is
// the very same pixels a whole-screen draw would make) and copied to VRAM. fb therefore only holds the last patches, not the room.
// What floats over the room (thought bubble, plumbob) is not part of the room: the pixels under it are saved before it is drawn and put back before
// it moves, so it costs a copy, not a redraw of the room behind it.
#define NRC 28   // (room for the household's Sims: each can add its old and new rectangle)
static Rc rcs[NRC] EWRAM_BSS; static int nrc;
static int vpValid;                 // the screen holds the room as of pCamX/pCamY (cleared by anything that draws over it: menus, other screens)
static int pCamX, pCamY;            // camera of the last picture
static Rc actOld; static int actHas; static unsigned actSig;   // the player's rectangle last picture, and what it was made from
static int pBob;                    // the board pickup's frame last picture (-1 = not shown)
static int pHud=-1;
#define OV_CAP 2600
static u16 ovBuf[OV_CAP] EWRAM_BSS; // the room pixels under the overlay that is on screen
static Rc ovRc; static int ovOn; static unsigned ovSig;
static u16 lifeVs;                  // timer value when the picture was ready (before waiting for the vertical blank): the load meter counts work up to here
#ifdef SELFTEST
static int stBad, stPics, stFull, stArea, stRects, stMoved, stTop, stBot; static unsigned tRend, tOvl, tHud, tWait, tDma, tLogic; static volatile int stDbg[16];
#define TMARK(v) { u16 n_=R_TM2D; v+=(u16)(n_-tm0); tm0=n_; }
#endif
static void rcAdd(int x0,int y0,int x1,int y1){
    if(x0<vpX0) x0=vpX0; if(x1>vpX1) x1=vpX1; if(y0<vpY0) y0=vpY0; if(y1>vpY1) y1=vpY1; if(x0>=x1||y0>=y1) return;
    for(int i=0;i<nrc;i++){ Rc*r=&rcs[i];
        if(x0<=r->x1&&x1>=r->x0&&y0<=r->y1&&y1>=r->y0){
            int bx0=x0<r->x0?x0:r->x0, by0=y0<r->y0?y0:r->y0, bx1=x1>r->x1?x1:r->x1, by1=y1>r->y1?y1:r->y1;
            int ab=(bx1-bx0)*(by1-by0), aa=(x1-x0)*(y1-y0)+(r->x1-r->x0)*(r->y1-r->y0);
            if(ab*2<=aa*3){ r->x0=(short)bx0; r->y0=(short)by0; r->x1=(short)bx1; r->y1=(short)by1; return; } } }
    if(nrc<NRC){ Rc*r=&rcs[nrc++]; r->x0=(short)x0; r->y0=(short)y0; r->x1=(short)x1; r->y1=(short)y1; }
    else { Rc*r=&rcs[0]; if(x0<r->x0) r->x0=(short)x0; if(y0<r->y0) r->y0=(short)y0; if(x1>r->x1) r->x1=(short)x1; if(y1>r->y1) r->y1=(short)y1; }
}
static int rcHit(const Rc*a,int x0,int y0,int x1,int y1){ return a->x0<x1&&a->x1>x0&&a->y0<y1&&a->y1>y0; }
static void actorRc(Rc*r){   // everything the player puts on screen: sprite, shadow, board
    int sx=plX-16, sy=plY-SPF-plZ-plBob+plDip;
    int x0=sx+spBx0, x1=sx+spBx1, y0=sy+spBy0, y1=sy+spBy1;
    if(sShad){ if(plX-3<x0) x0=plX-3; if(plX+4>x1) x1=plX+4; if(plY-plFh+2>y1) y1=plY-plFh+2; }
    if(plMk){ if(plX-7<x0) x0=plX-7; if(plX+8>x1) x1=plX+8; if(plY-plFh+4>y1) y1=plY-plFh+4; }   // the landing mark on the floor
    if(lskate){ if(plX-19<x0) x0=plX-19; if(plX+20>x1) x1=plX+20; if(plY-plZ-17<y0) y0=plY-plZ-17; if(plY-plZ+14>y1) y1=plY-plZ+14; }   // the whole board, nose up, rolled or lifted
    r->x0=(short)x0; r->x1=(short)x1; r->y0=(short)y0; r->y1=(short)y1;
}
static unsigned actSigBase(void){ return (unsigned)(plX&0x3FF)|((unsigned)(plY&0x3FF)<<10)|((unsigned)(plZ&0x3F)<<20)|((unsigned)plV<<26)|((unsigned)lskate<<28)|((unsigned)sShad<<29)|((unsigned)(plFh&1)<<30)|((unsigned)plBob<<31); }
static unsigned actSigNow(void){ unsigned b=actSigBase(); if(lskate) b^=((unsigned)bdA|((unsigned)(bdPitch+4)<<8)|((unsigned)bdRaise<<12)|((unsigned)bdRoll<<16)|((unsigned)bdSpk<<24)|((unsigned)plDip<<27)|((unsigned)plMk<<29))*2654435761u; return b; }   // + the board's pose: any change redraws
// ---- getting pixels to the screen ----
static void dmaRows16(u32 src,u32 dst,int w,int rows,int sstride,int dstride){   // rows of w halfwords, strides in halfwords
    for(int j=0;j<rows;j++){ REG_DMA3SAD=src; REG_DMA3DAD=dst; REG_DMA3CNT=(u32)w|0x80000000u; src+=(u32)(sstride*2); dst+=(u32)(dstride*2); }
}
static void vramCopy(int x0,int y0,int x1,int y1){   // fb rectangle -> VRAM, one DMA per row (32 bit when the columns line up)
    int wide=((x0|x1)&1)==0;
    for(int y=y0;y<y1;y++){
        int o=y*SW+x0;
        REG_DMA3SAD=(u32)(uintptr_t)(fb+o); REG_DMA3DAD=VRAM_ADDR+(u32)(o*2);
        REG_DMA3CNT=wide?(u32)((x1-x0)/2)|0x84000000u:(u32)(x1-x0)|0x80000000u;
    }
}
static void vramScroll(int dx,int dy){   // the picture moves by (-dx,-dy) inside the scene rows: new(x,y)=old(x+dx,y+dy)
    int w=(vpX1-vpX0)-(dx<0?-dx:dx), sx0=vpX0+(dx>0?dx:0), dx0=vpX0+(dx>0?0:-dx);
    int y0=dy>0?vpY0:vpY0-dy, y1=dy>0?vpY1-dy:vpY1;   // destination rows [y0,y1)
    int back=(dy<0)||(dy==0&&dx<0);                    // copy order that never overwrites what is still to be read
    for(int n=0,cnt=y1-y0;n<cnt;n++){
        int y=back?y1-1-n:y0+n;
        u32 src=VRAM_ADDR+(u32)(((y+dy)*SW+sx0)*2), dst=VRAM_ADDR+(u32)((y*SW+dx0)*2);
        if(dy==0&&dx<0){ src+=(u32)((w-1)*2); dst+=(u32)((w-1)*2); REG_DMA3SAD=src; REG_DMA3DAD=dst; REG_DMA3CNT=(u32)w|(1u<<21)|(1u<<23)|0x80000000u; }
        else { REG_DMA3SAD=src; REG_DMA3DAD=dst; REG_DMA3CNT=(u32)w|0x80000000u; }
    }
}
// ---- the overlay (bubble / plumbob): save what is under it, draw it, put it back ----
static void ovSaveVram(const Rc*r){ dmaRows16(VRAM_ADDR+(u32)((r->y0*SW+r->x0)*2),(u32)(uintptr_t)ovBuf,r->x1-r->x0,r->y1-r->y0,SW,r->x1-r->x0); }
static void ovSaveFb(const Rc*r){ int w=r->x1-r->x0; for(int y=r->y0;y<r->y1;y++){ const u16*sp=fb+y*SW+r->x0; u16*d=ovBuf+(y-r->y0)*w; for(int x=0;x<w;x++) d[x]=sp[x]; } }
static void ovToFb(const Rc*r){ dmaRows16((u32)(uintptr_t)ovBuf,(u32)(uintptr_t)(fb+r->y0*SW+r->x0),r->x1-r->x0,r->y1-r->y0,r->x1-r->x0,SW); }
static void ovRestoreVram(const Rc*r){ dmaRows16((u32)(uintptr_t)ovBuf,VRAM_ADDR+(u32)((r->y0*SW+r->x0)*2),r->x1-r->x0,r->y1-r->y0,r->x1-r->x0,SW); }
static int ovNow(Rc*r,unsigned*sig){   // is there an overlay, where (clamped to the scene and the buffer), and what it is made of
    int x0,y0,x1,y1; if(!hudOverlayRc(&x0,&y0,&x1,&y1)) return 0;
    *sig=hudOverlaySig()*31u+(unsigned)(x0+64)*7u+(unsigned)(y0+64)*131u+(unsigned)(x1+64);   // from the unclamped box: the overlay can sit partly off the scene
    if(x0<vpX0) x0=vpX0; if(x1>vpX1) x1=vpX1; if(y0<vpY0) y0=vpY0; if(y1>vpY1) y1=vpY1; if(x0>=x1||y0>=y1||(x1-x0)*(y1-y0)>OV_CAP) return 0;
    r->x0=(short)x0; r->y0=(short)y0; r->x1=(short)x1; r->y1=(short)y1;
    return 1;
}
#include "zoomtab.h"
static ZLn zoomBuf[ZT_N] EWRAM_BSS;   // the ZOOM table in use
static void hudApplyLayout(void){   // the room rows between the HUD panels, and the ZOOM window inside them
    sbY0=HUD_TOPH; sbY1=sHud>=2?SH:HUD_BOTY; vpX0=0; vpX1=SW; vpY0=sbY0; vpY1=sbY1; zoomDma=0; zoomNum=zoomDen=1; zoomPa=256;
    int z=xo[XO_ZOOM], L=sHud>=2;
    if(z>=1&&z<=4){ const u8*w=zoomWin[L][z-1]; vpX0=w[0]; vpX1=w[1]; vpY0=w[2]; vpY1=w[3];
        for(int i=0;i<ZT_N;i++) zoomBuf[i]=zoomTab[L][z-1][i];
        zoomDma=(const u32*)zoomBuf; zoomNum=zoomND[z-1][0]; zoomDen=zoomND[z-1][1]; zoomPa=zoomBuf[sbY0].pa; }
    else if(zoomShow) zoomOff();   // (the DMA must not run on without a table)
}
static void vpFull(void){ vpX0=0; vpX1=SW; vpY0=0; vpY1=SH; sbY0=0; sbY1=SH; zoomOff(); }   // leaving the room view: the whole screen, 1:1
static void liveInvalidate(void){ vpValid=0; }
// CLEAR CACHES (OPTIONS > VIDEO, last row): throws away everything that is only a speed-up copy and gets rebuilt on its own, so
// a stale or glitched one is gone and the next frame starts clean. Nothing that is saved or that holds a Sim, a room or a song
// is touched, and nothing is re-baked (that would cost more than it frees).
static void cacheFlush(void){
    wDirty=1;                                        // the walls' inside / outside map is worked out again
    liveInvalidate();                                // the viewport cache: the next frame draws the whole room
    hhSlotsFree();                                   // OBJ sprite slots: each Sim's tiles and palette are loaded again when it is next on screen
    for(int i=0;i<MW*MH;i++) hhDist[i]=0xFFFF;       // the Sims' path-search scratch
    sfxStop();                                       // sound effects still being mixed
    REG_WAITCNT&=(u16)~0x4000; applyRom();           // cart prefetch off, then back on as the ROM SPEED option says: its buffer starts empty
}
static void liveHud(int all){   // bring the panels up to date (into fb); the pieces that changed are listed in hudRc
    hudRcN=0;
    if(all||pHud!=sHud){ all=1; }
    hudTopUpdate(all);
    if(sHud<2) hudBotUpdate(all);
    pHud=sHud;
#ifdef SELFTEST
    stTop+=hudRcN;
#endif
}
static void liveFull(void){   // the whole scene and both panels, from scratch
    drawRoomRect(vpX0,vpY0,vpX1,vpY1,0);
    Rc o; unsigned osg; ovOn=ovNow(&o,&osg);
    if(ovOn){ ovSaveFb(&o); ovRc=o; ovSig=osg; clipSet(vpX0,vpY0,vpX1,vpY1); hudOverlayDraw(); clipAll(); }
    if(lcamF>0){   // action cam: ease in, spin through all 4 views, ease out
        int f=lcamF, z=f<12?f:(f>CAM_LEN-12?CAM_LEN-f:12);   // 0..12 zoom amount
        if(z>0){ int cx=lpsx<vpX0?vpX0:lpsx>=vpX1?vpX1-1:lpsx, cy=lpsy<vpY0?vpY0:lpsy>=vpY1?vpY1-1:lpsy; zoomFb(cx,cy,256-z*(CAM_ZOOM)/12); }
    }
    liveHud(1);
    if(sHud>=2) rect(0,sbY1,SW,SH-sbY1,RGB(0,0,0));
    lifeVs=R_TM2D;
    if(zoomDma){ zoomShow=1; zoomIrq(); } else if(zoomShow) zoomOff();   // a room picture: zoomed if the ZOOM is on (else BG2 back to 1:1 at once)
    zoomKeep=1; present(); zoomKeep=0; hhObjUpdate();   // (the household sprites change in vblank, with the picture)
    pCamX=camX; pCamY=camY;
    Rc r; actorRc(&r); actOld=r; actHas=1; actSig=actSigNow();
    for(int m=0;m<hhN;m++){ hhRc(m,&hhOld[m]); hhOldSig[m]=hhSig(m); }
    pBob=(!lhave)?((lfr>>4)&1):-1;
    vpValid=(lcamF>0)?0:1;
#ifdef SELFTEST
    stFull++;
#endif
}
static void liveBoardRc(void){   // the pickup's tile on screen
    int sx=LOX+(BDX-BDY)*CA, sy=LOY+(BDX+BDY+1)*CB; rcAdd(sx-12,sy-26,sx+12,sy+7);
}
static void livePatch(int dx,int dy){
#ifdef SELFTEST
    u16 tm0=R_TM2D;
#endif
    nrc=0;
    if(dx>0) rcAdd(vpX1-dx,vpY0,vpX1,vpY1); else if(dx<0) rcAdd(vpX0,vpY0,vpX0-dx,vpY1);
    if(dy>0) rcAdd(vpX0,vpY1-dy,vpX1,vpY1); else if(dy<0) rcAdd(vpX0,vpY0,vpX1,vpY0-dy);
    Rc a; actorRc(&a); unsigned asg=actSigNow();
    if(dx||dy||asg!=actSig){   // the player moved, turned or jumped (or the picture slid under him): redraw where he was and where he is
        rcAdd(a.x0,a.y0,a.x1,a.y1);
        if(actHas) rcAdd(actOld.x0-dx,actOld.y0-dy,actOld.x1-dx,actOld.y1-dy);
    }
    for(int m=0;m<hhN;m++){ unsigned sg=hhSig(m);   // household members: same as the player
        if(dx||dy||sg!=hhOldSig[m]){ HhR r; hhRc(m,&r); rcAdd(r.x0,r.y0,r.x1,r.y1); if(hhOld[m].x1>hhOld[m].x0) rcAdd(hhOld[m].x0-dx,hhOld[m].y0-dy,hhOld[m].x1-dx,hhOld[m].y1-dy); hhOld[m]=r; hhOldSig[m]=sg; } }
    int bob=(!lhave)?((lfr>>4)&1):-1; if(bob!=pBob) liveBoardRc();
    for(int i=0;i<nrc;i++) drawRoomRect(rcs[i].x0,rcs[i].y0,rcs[i].x1,rcs[i].y1,0);
#ifdef SELFTEST
    TMARK(tRend)
    for(int i=0;i<nrc;i++) stArea+=(rcs[i].x1-rcs[i].x0)*(rcs[i].y1-rcs[i].y0); stRects+=nrc; if(dx||dy) stMoved++;
#endif
    // the overlay: is it the same as last picture, sitting on pixels that did not change?
    Rc o; unsigned osg; int on=ovNow(&o,&osg);
    int ovSame=on&&ovOn&&osg==ovSig&&!dx&&!dy;
    if(ovSame) for(int i=0;i<nrc;i++) if(rcHit(&rcs[i],o.x0,o.y0,o.x1,o.y1)){ ovSame=0; break; }
    liveHud(0);
#ifdef SELFTEST
    TMARK(tHud)
#endif
    lifeVs=R_TM2D;
    vsync(); hhObjUpdate();
#ifdef SELFTEST
    TMARK(tWait)
#endif
    if(!ovSame&&ovOn) ovRestoreVram(&ovRc);              // old overlay off the screen, the room under it is back
    if(dx||dy) vramScroll(dx,dy);
    for(int i=0;i<nrc;i++) vramCopy(rcs[i].x0,rcs[i].y0,rcs[i].x1,rcs[i].y1);
    if(on&&!ovSame){
        ovSaveVram(&o); ovToFb(&o); clipSet(o.x0,o.y0,o.x1,o.y1); hudOverlayDraw(); clipAll(); vramCopy(o.x0,o.y0,o.x1,o.y1);
        ovRc=o; ovSig=osg;
    }
    ovOn=on;
    for(int i=0;i<hudRcN;i++) vramCopy(hudRc[i].x0,hudRc[i].y0,hudRc[i].x1,hudRc[i].y1);
#ifdef SELFTEST
    TMARK(tDma)
#endif
    pCamX=camX; pCamY=camY; actOld=a; actHas=1; actSig=asg; pBob=bob;
}
static void lifeDraw(void){
    camFollow(camSnap||lcamF>0); camSnap=0;
    playerCalc();
    int dx=camX-pCamX, dy=camY-pCamY;
    if(zoomDma&&!zoomShow) vpValid=0;   // a message or menu was shown 1:1 in between: a whole zoomed picture again
    if(!vpValid||lcamF>0||dx>40||dx<-40||dy>40||dy<-40) liveFull(); else livePatch(dx,dy);
#ifdef SELFTEST
    if(lcamF==0){   // draw the whole thing again and compare it with what is on the screen
        stPics++; drawRoomRect(0,vpY0,SW,vpY1,0); { Rc o; unsigned sg; if(ovNow(&o,&sg)){ clipSet(0,vpY0,SW,vpY1); hudOverlayDraw(); clipAll(); } }
        int bad=0, bx0=999, by0=999, bx1=-1, by1=-1; const volatile u16*v=(const volatile u16*)VRAM_ADDR;
        for(int y=vpY0;y<vpY1;y++)for(int x=0;x<SW;x++) if(v[y*SW+x]!=fb[y*SW+x]){ bad++; if(x<bx0)bx0=x; if(x>bx1)bx1=x; if(y<by0)by0=y; if(y>by1)by1=y; }
        if(bad){ stBad++; if(stBad==1){ stDbg[0]=bad; stDbg[1]=bx0; stDbg[2]=by0; stDbg[3]=bx1; stDbg[4]=by1; stDbg[5]=dx; stDbg[6]=dy; stDbg[7]=ovOn; stDbg[8]=ovRc.x0; stDbg[9]=ovRc.y0; stDbg[10]=ovRc.x1; stDbg[11]=ovRc.y1; stDbg[12]=plX; stDbg[13]=plY; stDbg[14]=stPics; } }
    }
#endif
}
static void camStep(int steps,u16 k,u16 pr){   // action cam: the game holds still while the camera swings round the room
    lcamF+=steps;
    if(((k&K_SEL)&&(pr&K_SEL))||lcamF>=CAM_LEN){ lcamF=0; cview=0; lcBankT=120; }
    else { int f=lcamF; cview=(f<6||f>=60)?0:(f-6)/18+1; if(cview>3) cview=0; }
}
static int gToMenu;   // set when the player picks MAIN MENU in the pause menu, so every screen above returns to it
static void hhSwap(HhSim*s){   // trade places: the player becomes s, s becomes who the player was
    s32 x=lfx, y=lfy; lfx=s->fx; lfy=s->fy; s->fx=x; s->fy=y;
    { u8 h=(u8)(lhd&15); lhd=s->hd; s->hd=h; }
    { int v;
      v=lfood; lfood=s->need[HN_FOOD]; s->need[HN_FOOD]=(u8)v;
      v=100-lbl; lbl=100-s->need[HN_WC]; s->need[HN_WC]=(u8)v;
      v=sNrg; sNrg=s->need[HN_REST]; s->need[HN_REST]=(u8)v;
      v=sHyg; sHyg=s->need[HN_CLEAN]; s->need[HN_CLEAN]=(u8)v;
      v=sCom; sCom=s->need[HN_COMFY]; s->need[HN_COMFY]=(u8)v;
      v=moodFunPct(); moodFun=s->need[HN_FUN]*MOOD_ONE; s->need[HN_FUN]=(u8)v;
      v=lhp; lhp=s->hp; s->hp=(u8)v; }
    for(int i=0;i<LK_N;i++){ u8 t=look[i]; look[i]=s->look[i]; s->look[i]=t; }
    { u8 t=stage; stage=s->stage; s->stage=t; t=pAsp; pAsp=s->asp; s->asp=t; t=pLtw; pLtw=s->ltw; s->ltw=t; }
    for(int i=0;i<TR_N;i++){ u8 t=pTr[i]; pTr[i]=s->tr[i]; s->tr[i]=t; }
    for(int i=0;i<HH_NM;i++){ char t=hhPName[i]; hhPName[i]=s->name[i]; s->name[i]=t; t=hhPLast[i]; hhPLast[i]=s->last[i]; s->last[i]=t; }
    { int u=hhPUid; hhPUid=s->uid; s->uid=(u8)u; int v=sSoc; sSoc=s->need[HN_SOC]; s->need[HN_SOC]=(u8)v; s->bubT=0; hhBubT=0; }
    s->act=HA_IDLE; s->think=30; s->gok=0;
    lz=lvz=0; lsp=0; lskate=0; lgrind=0; lstun=0; lairF=0; feelReset(lhd);
    buildLook(); setColors(); ageSave(); persSave();
}
// ---- the ASPIRATION panel (pause menu): the Sims 2 wants and fears panel, the lifetime want, the reward shop, and the creature's Spore side ----
// UP DOWN pick a want | A lock it (one at a time: a locked want survives the reroll when you wake up) | R aspiration rewards | B back
static void aspRewards(void){
    static char rb[RW_N][24]; const char* it[RW_N];
    for(;;){
        for(int r=0;r<RW_N;r++){ char*e=rb[r]; const char*p=simRewNm[r]; while(*p) *e++=*p++; *e++=' '; *e++=' '; e+=numStr(e,simRewCost[r]); *e=0; it[r]=rb[r]; }
        char t[24]; { char*e=t; const char*p="REWARDS  POINTS "; while(*p) *e++=*p++; numStr(e,simAsp); }
        int c=menu(t,it,RW_N); if(c<0) return;
        toast(simBuy(c));
    }
}
static void aspPanel(void){
    u16 prev=keyNow(); int cur=0, dirty=1;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&(K_B|K_START)) return;
        if(pr) dirty=1;
        if(pr&K_DOWN) cur=(cur+1)%SIM_WS;
        if(pr&K_UP) cur=(cur+SIM_WS-1)%SIM_WS;
        if((pr&K_A)&&simW[cur]>=0){ simLock=(simLock>>cur&1)?0:(1<<cur); }   // one lock: locking another want moves it
        if(pr&K_R){ aspRewards(); prev=keyNow(); }
        if(!dirty){ vsync(); continue; }   // the game is paused: the panel only changes when a key does
        dirty=0;
        box(3,1,234,157);
        int a=aspNow(), wish=simWishes(); char b[24];
        simIcon(10,6,simAspIcon[a],GOLD); text(20,6,aspNm[a],GOLD,1);
        if(wish){ const char*z=simZoneNm[simZone]; text(230-tw(z,1),6,z,simZoneCol(simZone),1); simMeterBar(10,15,220,5); }
        else text(230-tw("BABIES HAVE NO WANTS",1),6,"BABIES HAVE NO WANTS",DIMC,1);
        // lifetime want
        { int y=24; text(10,y,"LIFETIME",DIMC,1); const SimLtw*L=simLtw(); text(52,y,L->name,stage<AG_TEEN?DIMC:WHITE,1);
          const char*st; if(simFlags&SF_LTW) st="MET"; else if(stage<AG_TEEN) st="AS A TEEN"; else { int v=simLtwVal(), g=L->goal; char*e=b; e+=numStr(e,v>g?g:v); *e++='/'; numStr(e,g); st=b; }
          text(230-tw(st,1),y,st,(simFlags&SF_LTW)?RGB(24,30,31):GOLD,1); }
        // wants
        text(10,35,"WANTS",RGB(12,28,12),1);
        for(int s=0;s<SIM_WS;s++){
            int y=44+s*11, on=simW[s]>=0, lk=simLock>>s&1, f=(s==cur);
            if(f){ rect(7,y-2,226,11,RGB(6,16,8)); rect(7,y-2,2,11,GOLD); }
            simCell(12,y-1,on?simWants[simW[s]].icon:0,0,lk,on);
            if(on){ text(25,y,simWantName(s),f?WHITE:RGB(22,28,22),1);
                b[0]='+'; numStr(b+1,simWants[simW[s]].pts); int x=230-tw(b,1); text(x,y,b,RGB(12,30,12),1);
                if(lk) text(x-6-tw("LOCKED",1),y,"LOCKED",GOLD,1); }
            else text(25,y,"...",DIMC,1);
        }
        text(10,89,"FEARS",RGB(30,10,8),1);
        for(int s=0;s<SIM_FS;s++){
            int y=98+s*11, on=simF[s]>=0;
            simCell(12,y-1,on?simFears[simF[s]].icon:0,1,0,on);
            if(on){ text(25,y,simFearName(s),RGB(30,18,16),1); b[0]='-'; numStr(b+1,simFears[simF[s]].pts); text(230-tw(b,1),y,b,RGB(30,10,8),1); }
            else text(25,y,"...",DIMC,1);
        }
        // points, DNA, sign and abilities
        { int y=132, x=text(10,y,"REWARD POINTS",DIMC,1)+3; numStr(b,simAsp); x=text(x,y,b,GOLD,1)+10;
          x=text(x,y,"DNA",DIMC,1)+3; numStr(b,pDna); x=text(x,y,b,RGB(12,30,24),1)+10;
          text(x,y,signNm[signOf()],RGB(20,22,30),1); }
        { int x=10, y=141; for(int ab=0;ab<AB_N;ab++){ x=text(x,y,abNm[ab],DIMC,1)+2; for(int q=0;q<10;q++) rect(x+q*2,y+1,1,4,q<abOf10(ab)?GOLD:RGB(4,6,12)); x+=20; } }
        text(10,150,"A LOCK WANT  R REWARDS  B BACK",RGB(12,14,16),1);
        present();
    }
}
// Timer2 (65536 Hz) is the clock (defined with the settings). The game logic always runs at 60 steps per second; the
// frame rate setting only says how often the picture is redrawn, so lower rates save work without slowing the game.
static const char* const yesNoLife[2]={"NO","YES ERASE IT"};
// ---- the PAUSE PANEL: Sims style (household funds, one icon tile per screen) instead of a plain list ----
// The phone is no longer a menu entry: it is a real item in the house (palette PHONE, tile 'I'): R next to it. A house without one gets one (phoneEnsure).
enum { PM_RESUME, PM_SAVE, PM_WANTS, PM_FAMILY, PM_STORY, PM_OPTS, PM_BUILD, PM_QUIT };
static const char* const pmArt[8][9]={
 {"..#......","..###....","..#####..","..#######","..#######","..#####..","..###....","..#......","........."},   // resume
 {"#########","#.#####.#","#.#####.#","#.......#","#.ooooo.#","#.ooooo.#","#.ooooo.#","#.ooooo.#","#########"},   // save (a disk)
 {"....#....","....#....","...###...","#########",".#######.","..#####..","..##.##..",".##...##.",".#.....#."},   // wants (a star)
 {".##...##.",".##...##.","####.####","####.####","####.####",".##...##.",".##...##.",".##...##.",".#.#.#.#."},   // family
 {"#########","#.......#","#.#####.#","#.......#","#.#####.#","#.......#","#.####..#","#.......#","#########"},   // story (a book)
 {"..#.#.#..",".#######.","#########","###...###","###...###","#########",".#######.","..#.#.#..","........."},   // options (a cog)
 {"....#....","...###...","..#####..",".#######.","#########",".##...##.",".##.o.##.",".##.o.##.",".#######."},   // build (a house)
 {"#######..","#.....#.#","#.....##.","#...#####","#.....##.","#.....#.#","#.....#..","#######..","........."}}; // quit (door and arrow)
static const u16 pmCol[8]={RGB(10,28,10),RGB(10,20,31),RGB(31,26,6),RGB(31,16,22),RGB(22,16,30),RGB(22,24,26),RGB(30,20,8),RGB(30,10,8)};
static const char* const pmNm[8]={"RESUME","SAVE","WANTS","FAMILY","STORY","OPTIONS","BUILD","QUIT"};
static const char* const pmTitle[8]={"RESUME","SAVE GAME","ASPIRATION","HOUSEHOLD","STORY","OPTIONS","BUILD AND HOUSES","MAIN MENU"};
static const char* const pmDesc[8]={"BACK TO YOUR LIFE","SAVES YOU AND YOUR HOUSE","WANTS  FEARS  REWARD SHOP","WHO LIVES HERE  HOW THEY FEEL","YOUR CHAPTERS","SETTINGS  SOUND  CONTROLS","EDIT MAP  BLUEPRINTS  NEW LIFE","SAVES AND LEAVES"};
static int pauseMenu(int mode){   // mode 0 life, 1 from the neighborhood, 2 test play from the editor. Returns a PM_ number, or -1 (resume)
    static const u8 full[8]={0,1,2,3,4,5,6,7}, edl[3]={PM_RESUME,PM_OPTS,PM_QUIT};
    const u8*ids=mode==2?edl:full; int n=mode==2?3:8, sel=0, dirty=1, lastB=-1; u16 prev=keyNow(); u32 t=0;
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; t++;
        int ps=sel;
        if(pr&K_RIGHT) sel=(sel+1)%n;
        if(pr&K_LEFT) sel=(sel+n-1)%n;
        if((pr&K_DOWN)&&sel+4<n) sel+=4;
        if((pr&K_UP)&&sel>=4) sel-=4;
        if(pr&K_A) return ids[sel]==PM_RESUME?-1:ids[sel];
        if(pr&(K_B|K_START)) return -1;
        { int bb=(int)((t>>4)&1); if(sel!=ps||bb!=lastB) dirty=1; lastB=bb; }   // redraw when the cursor moves or the icon bob changes (a few times a second), not every frame
        if(!dirty){ vsync(); continue; }
        dirty=0;
        box(6,4,228,152);
        rect(7,5,226,17,RGB(5,12,24)); rect(7,21,226,1,GOLD);
        text(12,9,mode==2?"TEST PLAY PAUSED":"PAUSED",GOLD,1);
        if(mode!=2){ char b[12]; char*e=b; *e++=(char)0xC2; *e++=(char)0xA7; numStr(e,simMoney); text(228-tw(b,1),9,b,RGB(14,30,14),1); }
        for(int i=0;i<n;i++){ int id=ids[i], x=10+(i&3)*56, y=27+(i>>2)*43, on=(i==sel);
            rect(x-1,y-1,54,40,on?GOLD:RGB(10,16,30)); rect(x,y,52,38,on?RGB(6,18,10):RGB(7,10,20));
            u16 col=on?pmCol[id]:(u16)((pmCol[id]>>1)&0x3DEF); int ib=(on&&((t>>4)&1))?-1:0;
            for(int r=0;r<9;r++)for(int q=0;q<9;q++){ char ch=pmArt[id][r][q]; if(ch!='.') rect(x+17+q*2,y+5+r*2+ib,2,2,ch=='o'?WHITE:col); }
            const char*nm=(id==PM_QUIT&&mode==1)?"TOWN":(id==PM_QUIT&&mode==2)?"EDITOR":pmNm[id];
            text(x+(52-tw(nm,1))/2,y+28,nm,on?WHITE:DIMC,1); }
        { int id=ids[sel]; const char*ti=pmTitle[id], *ds=pmDesc[id];
          if(id==PM_QUIT&&mode==1){ ti="NEIGHBORHOOD"; ds="BACK TO THE TOWN"; } else if(id==PM_QUIT&&mode==2){ ti="BACK TO EDITOR"; ds="LEAVE THE TEST PLAY"; }
          rect(7,113,226,1,RGB(10,16,30)); text(12,118,ti,GOLD,1); text(12,128,ds,RGB(22,25,28),1); }
        text(12,142,"LEFT RIGHT UP DOWN PICK  A OK  B BACK",RGB(12,14,16),1);
        present();
    }
}
static const char* const buildItems[3]={"EDIT MAP","BLUEPRINTS","NEW LIFE"};
static void phoneEnsure(void){   // a house must have a phone (careers, food and visitors live there now): when the map has none, one goes on the free tile nearest the spawn
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++) if(lifeMap[y][x]=='I') return;
    for(int r=1;r<=8;r++)for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++){
        if((dx<0?-dx:dx)!=r&&(dy<0?-dy:dy)!=r) continue;
        int x=spx+dx, y=spy+dy; if(x<1||y<1||x>=MW-1||y>=MH-1) continue;
        if(lifeMap[y][x]=='.'){ lifeMap[y][x]='I'; return; } }
}

// ---- game music: the jukebox songs in their shuffled order while you play (OPTIONS > AUDIO > GAME MUSIC) ----
static int gmCur;   // visible number of the song that plays
static u8 radioSt EWRAM_BSS;   // 0 = off, else the tuned station + 1
static int menuOn, creOn, musCtx;   // who owns the music: the main menus' song, the creator's chiptune loop (musCtx = the screen the game was started from: 0 menu, 1 creator)
static void creatorMusStart(void); static void menuMusStart(void);
static void gmPlay(void){ const Song*sg=&songs[jbMap[gmCur]]; if(xo[XO_GAMEXF]) musFadeTo(sg->adp?1:0,sg->adp,sg->xm,XF_SONG); else musBegin(sg->adp?1:0,sg->adp,sg->xm); }   // OPTIONS > AUDIO > GAME CROSSFADE: off = a hard start
static void gmStart(void){   // entering the game: crossfade into a random checked song (OPTIONS > AUDIO > GAME MUSIC), or fade out the last screen's music
    if(gMusic) return;
    menuOn=0; creOn=0;
    if(!xo[XO_GAMEMUS]||!sSnd||jbN<=0){ musFadeOut(XF_OUT); return; }
    gmCur=pickSong(); if(gmCur<0){ musFadeOut(XF_OUT); return; }
    gMusic=1; mGain=mGainT=256; gmPlay();
}
static void gmStop(void){ mGain=mGainT=256; gMusic=0; radioSt=0; }   // leaving the game: nothing is cut, the next screen's music crossfades over the song
static void gmSync(void){ if(xo[XO_GAMEMUS]&&sSnd) gmStart(); else if(gMusic&&!(radioSt&&sSnd)){ gMusic=0; radioSt=0; musFadeOut(XF_OUT); } }   // (a tuned radio keeps playing)   // after the pause menu: the option or SOUND may have changed
// ---- RADIO and SOUND SYSTEM (items 'R' and 'A', sound pack): R next to one tunes the next station; every station is a slice of the jukebox ----
// A station plays the visible (unlocked, not secret) songs of one artist, one after the other at random; ALL SONGS FM is the whole jukebox. After the
// last station the radio goes off and the normal GAME MUSIC comes back. Leaving the game switches the radio off (gmStop).
static const struct { const char*nm; const char*art; } radioStn[]={{"ALL SONGS FM",0},{"DAYBAR FM","DayBar"},{"SK9M BASS RADIO","Sk9m"},{"DANNY STEELE FM","Danny"},{"BRENO FM","Breno"},{"SINGHS RADIO","Singh"}};
#define RADIO_N 6
static int radioMatch(int st,int v){   // does visible song v belong to station st (the artist's name starts with the station's key)
    const char*k=radioStn[st].art; if(!k) return 1;
    const char*a=songArtist(&songs[jbMap[v]]); if(!a) return 0;
    while(*k&&*k==*a){ k++; a++; } return !*k;
}
static int radioPick(int st){   // a random song of the station, never the one that plays while there is another; -1 = none
    int c=0; for(int v=0;v<jbN;v++) if(radioMatch(st,v)&&v!=gmCur) c++;
    int any=(c==0); if(any) for(int v=0;v<jbN;v++) if(radioMatch(st,v)) c++;
    if(c==0) return -1;
    lrng^=((u32)R_TM2D<<8^uiTicks)*2654435761u;
    int r=(int)((((unsigned)rnd8()<<8|(unsigned)rnd8())*(unsigned)c)>>16);
    for(int v=0;v<jbN;v++){ if(!radioMatch(st,v)||(!any&&v==gmCur)) continue; if(r--==0) return v; }
    return -1;
}
static int gmPick(void){ if(radioSt){ int v=radioPick(radioSt-1); if(v>=0){ jbLast=v; return v; } } return pickSong(); }   // the next song: the station's, else a random checked one
static void radioTune(int sys){   // R at the radio (sys=0) or the sound system (sys=1)
    if(!sSnd||jbN<=0){ lnote="SOUND IS OFF"; lnoteT=50; return; }
    int st=radioSt;   // next station that has a song to play; past the last one: off
    do{ st++; } while(st<=RADIO_N&&radioPick(st-1)<0);
    lstun=20; lsp=0; lgrind=0;
    if(st>RADIO_N){ radioSt=0; lnote="RADIO OFF"; lnoteT=60;
        if(xo[XO_GAMEMUS]){ gMusic=0; gmStart(); } else { gMusic=0; musFadeOut(XF_OUT); }   // back to the normal game music (or silence)
        return; }
    int v=radioPick(st-1); radioSt=(u8)st; gmCur=v; jbLast=v; gMusic=1; mGain=mGainT=256; gmPlay();
    lnote=radioStn[st-1].nm; lnoteT=70;
    if(sys){ moodEvent(M_CHILL); }   // the big speakers feel better than the little radio
}
static void gmTick(void){   // once per frame: when the song is over, another random one
    if(!gMusic||!mPlay) return;
    if((xo[XO_GAMEXF]&&musNearEnd(XF_SONG))||(mKind?mDone:mLaps>=1)){ gmCur=gmPick(); if(gmCur<0){ gMusic=0; return; } gmPlay(); }   // crossfade on: the next song blends in before this one ends. Off: it starts right at the end
}
#include "tutorial.h"   // the TUTORIAL: pop-up lessons in the Sims 2 style (tutTick / tutRunModal, called from lifeModeRun)
static void lifeModeRun(int ed);
static void lifeMode(int ed){ int back=musCtx; gInPlay=1; lifeModeRun(ed); gInPlay=0; if(!gToMenu){ if(back==1) creatorMusStart(); else menuMusStart(); } }   // back from the game: the screen it was started from gets its music back (a crossfade)   // gInPlay: some option actions are only allowed while playing / only outside it
static void lifeModeRun(int ed){   // ed=1: test play started from the map editor
    objHideAll(); winFull(); REG_DISPCNT=0x3443; fxPlayStart();   // mode 3 + sprites (1D tiles) + window 0 (the household's hardware sprites, house.h)
    // (the passers-by of this lot are kept until you move to another lot or start a new life: twKeep, house.h)
    lifeInit(); if(!ed) phoneEnsure(); lcamF=0; cview=0; lcN=lcPts=lcT=lcBank=lcBankT=lcamPend=0; u16 prev=keyNow(); gmStart(); hudApplyLayout(); liveInvalidate(); camSnap=1;
    stModal=0; if(!ed) stEnter();   // (the chapter card of the story waits for the first frame)
    tutOn=0; tutModal=TM_NONE;   // the tutorial: replay now, or offer it once (first PLAY, not in the test play of the editor)
    if(!ed){ if(xo[XO_TUTOR]==2) tutBegin(); else if(xo[XO_TUTOR]==0&&!tutAsked){ tutAsked=1; tutModal=TM_OFFER; } }
    tmStart(); u16 tl=R_TM2D; int acc=0, fpsN=0, fr=0; u32 fpsT=0, workT=0; lfpsV=0; lloadV=0;
    for(;;){
        int need=(sFps+1)*TICKS_FRAME-100;
        for(;;){ u16 now=R_TM2D, dt=(u16)(now-tl); tl=now; acc+=dt; fpsT+=dt; if(acc>=need) break; vsync(); }
        u16 w0=R_TM2D;
        int steps=(acc+110)/TICKS_FRAME; if(steps>6){ steps=6; acc=0; } else acc-=steps*TICKS_FRAME;
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if((k&K_SEL)&&(k&K_START)) break;
        { static int selArm;   // SELECT tapped on its own (not SELECT+START, not during the action cam): control the next Sim of the household
          if((pr&K_SEL)&&!(k&K_START)&&lcamF==0) selArm=1; if(k&K_START) selArm=0;
          if((k&K_SEL)&&(pr&(K_UP|K_DOWN))){ selArm=0;   // SELECT + UP / DOWN: zoom in / out (the ZOOM option)
              int z=xo[XO_ZOOM]+((pr&K_UP)?1:-1); if(z>=0&&z<=4&&lcamF==0){ xo[XO_ZOOM]=(u8)z; optsSave(); hudApplyLayout(); liveInvalidate(); camSnap=1; }
              { static const char* const zn[5]={"ZOOM OFF","ZOOM 1.25X","ZOOM 1.5X","ZOOM 1.7X","ZOOM 2X"}; lnote=zn[xo[XO_ZOOM]]; } lnoteT=50; }
          if(k&K_SEL){ k&=(u16)~(K_UP|K_DOWN); pr&=(u16)~(K_UP|K_DOWN); }   // (SELECT held: UP / DOWN do not walk)
          if(selArm&&!(k&K_SEL)){ selArm=0;
              if(!hhN){ lnote="NO ONE ELSE LIVES HERE"; lnoteT=60; }
              else if(custom){ lnote="HAND BUILT SIMS CANNOT SWITCH"; lnoteT=60; }
              else { hhSwitch(); lnote=hhPName; lnoteT=60; liveInvalidate(); camSnap=1; } } }
        if(pr&K_START){   // pause menu
            tutSawPause=1;   // (the tutorial's pause menu lesson)
            mGainT=128; sfxStop(); simsSave(); hhSave(); objHideAll(); REG_DISPCNT=0x0403;   // (no sprites over the menus, options or the editor)   // the music fades to half while a menu is open   // the pause menu is also a save point
            { u8 zz=xo[XO_ZOOM]; xo[XO_ZOOM]=0; hudApplyLayout(); camSnap=1; liveInvalidate(); lifeDraw(); xo[XO_ZOOM]=zz; }   // a whole picture behind the menu (the screen itself only holds patches), not zoomed
            int c=pauseMenu(ed?2:nbPlaying?1:0);
            if(c==PM_SAVE){ if(!sgPid) toast("PICK A PLAYER ON THE PLAY SCREEN"); else { int se=sgSave(); toast(se?slErrMsg(se):"GAME SAVED"); } }
            else if(c==PM_WANTS) aspPanel();
            else if(c==PM_FAMILY) hhMenu();
            else if(c==PM_STORY) storyScreen();
            else if(c==PM_OPTS){ settingsScreen(); if(!ed&&xo[XO_TUTOR]==2) tutBegin(); }
            else if(c==PM_BUILD){   // EDIT MAP, BLUEPRINTS (the old room slots) and NEW LIFE share one entry
                int b=menu("BUILD AND HOUSES",buildItems,3);
                if(b==0){ vpFull(); mapEditor(); lifeInit(); }
                else if(b==1){ simsSaveNow(); hhSave(); if(slotScreen()){ lifeInit(); phoneEnsure(); } }   // a blueprint was loaded: start again in the loaded room (the life was written first, so nothing is lost)
                else if(b==2){ if(menu("START A NEW LIFE",yesNoLife,2)==1){ twKeep=0; simsNewLife(); moodReset(); lscore=0; simLastScore=0; stOff(); lnote="NEW LIFE"; lnoteT=60; } } }
            else if(c==PM_QUIT){ if(!ed&&!nbPlaying) gToMenu=1; break; }   // (from the neighborhood: back there)
            winFull(); REG_DISPCNT=0x3443; hudApplyLayout(); liveInvalidate(); camSnap=1; mGainT=256; gmSync(); prev=keyNow(); tmStart(); tl=R_TM2D; acc=0; lcamF=0; cview=0; continue;
        }
        if(!ed&&lcamF==0){ tutTick(k,pr);
            if(tutModal||stModal){   // a tutorial pop-up: the game holds still behind it, like the pause menu
                mGainT=128; sfxStop(); objHideAll(); REG_DISPCNT=0x0403;
                { u8 zz=xo[XO_ZOOM]; xo[XO_ZOOM]=0; hudApplyLayout(); camSnap=1; liveInvalidate(); lifeDraw(); xo[XO_ZOOM]=zz; }
                if(stModal) stRunModal(); else tutRunModal();
                winFull(); REG_DISPCNT=0x3443; hudApplyLayout(); liveInvalidate(); camSnap=1; mGainT=256; gmSync(); prev=keyNow(); tmStart(); tl=R_TM2D; acc=0; lcamF=0; cview=0; continue;
            } }
        if(lcamF>0) camStep(steps,k,pr);
        else {
            for(int s=0;s<steps;s++) lifeStep(k,s?0:pr,fr++);   // catch up if a frame took long; button presses count once
            if(lcamPend){ lcamPend=0; if(sCam){ lcamF=1; cview=0; } }
        }
        gmTick(); lifeDraw(); workT+=(u16)(lifeVs-w0);
        fpsN++; if(fpsT>=65536){ lfpsV=fpsN; lloadV=(int)(workT/(u32)fpsN*100/(u32)((sFps+1)*TICKS_FRAME)); workT=0; fpsN=0; fpsT-=65536; }
    }
    tutOn=0; tutModal=TM_NONE; stModal=0;
    objHideAll(); REG_DISPCNT=0x0403;
    simsSave(); hhSave(); R_TM2CNT=0; gmStop(); sfxStop(); lcamF=0; cview=0; vpFull(); clipAll(); liveInvalidate();   // leaving the life game saves it
    while((~REG_KEYINPUT)&0x3FF) vsync();   // wait for release so the caller doesn't see the exit keys
}


// ---------- map editor ----------
// Tools: ROOM (two corners -> walls + floor + a door), WALL (a straight line), FLOOR (fill an area), ITEM (single tiles), ERASE (clear an area).
static const char* const mapItems[6]={"PLAY TEST","SAVE MAP","BLUEPRINTS","OPTIONS","RESET MAP","BACK"};
static const char* const yesNo[2]={"NO","YES RESET"};
static const char* const toolNm[NTOOL]={"ROOM","WALL","FLOOR","BUY","SELL"};
static const u8 toolNext[NTOOL]={T_WALL,T_FLOOR,T_ERASE,T_ROOM,T_ITEM};   // SELECT: BUILD tools (room, wall, floor, sell), then BUY, then round again
static const char* const toolHint[NTOOL][2]={
 {"A CORNER  A AGAIN BUILDS THE ROOM  B CANCEL","L R FLOOR  SEL+L R WALLPAPER  SEL TOOL"},
 {"A START  A AGAIN DRAWS A WALL  B CANCEL","L R WALLPAPER  SEL TOOL  START MENU"},
 {"A CORNER  A AGAIN FILLS THE AREA  B CANCEL","L R FLOOR  SEL TOOL  START MENU"},
 {"A BUY  B SELL  HOLD AND MOVE TO PAINT","L R ITEM  SEL+L R TYPE  SEL+A TURN"},
 {"A CORNER  A AGAIN SELLS THE AREA  B CANCEL","SEL MODE  START MENU"} };
static void texSwatch(const Tex*t,int x,int y);
static void wallSwatch(int wp,int x,int y){   // 8x8: an old pattern, or a new wallpaper squeezed (every 3rd row)
    if(wp<NWP){ texSwatch(&wpTex[wp],x,y); return; }
    for(int r=0;r<8;r++)for(int u=0;u<8;u++) px(x+u,y+r,wxTex[wp-NWP][0][u][r*WALL_H/8]);
}
static void texSwatch(const Tex*t,int x,int y){   // the 8x8 pattern itself, 1:1
    rect(x-1,y-1,10,10,WHITE);
    for(int v=0;v<8;v++)for(int u=0;u<8;u++) px(x+u,y+v,t->c[t->p[v][u]-'0']);
}
static int eNet(int x0,int y0,int x1,int y1){   // what the ROOM / WALL / SELL rectangle would cost (negative: it pays you back): walls and items are priced, floors are free
    int net=0; if(eTool==T_FLOOR) return 0;
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){ char o=lifeMap[y][x], n=o;
        if(eTool==T_ROOM){ n=(x==x0||x==x1||y==y0||y==y1)?'W':isWallCh(o)?'.':o; if(y==y1&&x==(x0+x1)/2) n='D'; }
        else if(eTool==T_WALL) n='W'; else n='.';
        if(n!=o) net+=edCost(n)-edSell(o); }
    return net;
}
static int eApply(void){   // second A of ROOM / WALL / FLOOR / ERASE. 0 = refused
    int x0,y0,x1,y1; eRect(&x0,&y0,&x1,&y1);
    if(eTool==T_ROOM&&(x1-x0<2||y1-y0<2)) return 0;   // needs at least 3 x 3
    { int net=eNet(x0,y0,x1,y1); if(net&&!edPay(net)) return 0; }   // (not enough cash: dsMsg says so)
    wDirty=1;
    if(eTool==T_ROOM){
        for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
            floorMap[y][x]=(u8)eFl;
            if(x==x0||x==x1||y==y0||y==y1){ lifeMap[y][x]='W'; wallMap[y][x]=(u8)eWp; }
            else if(isWallCh(lifeMap[y][x])) lifeMap[y][x]='.';   // old walls inside are cleared, furniture stays
        }
        lifeMap[y1][(x0+x1)/2]='D';   // doorway in the front wall; move or remove it with the ITEM tool
    } else for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
        if(eTool==T_WALL){ lifeMap[y][x]='W'; wallMap[y][x]=(u8)eWp; }
        else if(eTool==T_FLOOR) floorMap[y][x]=(u8)eFl;
        else { lifeMap[y][x]='.'; floorMap[y][x]=0; wallMap[y][x]=0; }
    }
    return 1;
}
static void edShadeBand(int y0,int y1){ for(int i=y0*SW;i<y1*SW;i++){ u16 c=fb[i]; fb[i]=(u16)((c>>2)&0x1CE7); } }   // the room behind HUD text, at a quarter brightness
static char* edMoney(char*b,int v){ char*e=b; *e++=(char)0xC2; *e++=(char)0xA7; numStr(e,v); return b; }   // "§123"
static void drawEditorHud(const char*msg){
    int x=2, buy=(eTool==T_ITEM);
    edShadeBand(0,18); { int y0=buy?100:124; edShadeBand(y0,SH); rect(0,y0,SW,1,RGB(9,11,15)); }   // dark bands top and bottom: the text stays readable over any floor
    for(int m=0;m<2;m++){ int on=(m==buy), w=tw(m?"BUY":"BUILD",1)+4;   // the two modes, like the Sims: BUILD (rooms, walls, floors, selling) and BUY (the catalog)
        rect(x,1,w,8,on?GOLD:RGB(3,4,7)); text(x+2,1,m?"BUY":"BUILD",on?RGB(4,3,6):DIMC,1); x+=w+1; }
    x+=4;
    if(!buy) for(int i=0;i<NTOOL;i++){ if(i==T_ITEM) continue; int w=tw(toolNm[i],1)+4;   // the BUILD tools
        rect(x,1,w,8,i==eTool?WHITE:RGB(3,4,7)); text(x+2,1,toolNm[i],i==eTool?RGB(4,3,6):DIMC,1); x+=w+1; }
    if(msg[0]) text(2,11,msg,WHITE,1);
    else if(eTool!=T_ITEM){
        if(eAct){ int x0,y0,x1,y1; eRect(&x0,&y0,&x1,&y1); int w=x1-x0+1, h=y1-y0+1;
            if(eTool==T_WALL){ numText(text(2,11,"LENGTH",GOLD,1)+3,11,w+h-1,WHITE); }
            else { int xx=numText(text(2,11,"SIZE",GOLD,1)+3,11,w,WHITE)+3; xx=text(xx,11,"X",GOLD,1)+3; numText(xx,11,h,WHITE); } }
        else text(2,11,"PICK A START POINT",GOLD,1);
    }
    if(eTool==T_ITEM){
        { int cc=edCatOf(eOb,0), xx=2;   // the category tabs, then this category's items
          for(int c=0;c<NCAT;c++){ int w=tw(catNm[c],1)+4; rect(xx,102,w,8,c==cc?GOLD:RGB(3,4,7)); text(xx+2,102,catNm[c],c==cc?RGB(4,3,6):DIMC,1); xx+=w+1; }
          for(int j=0;j<catN[cc];j++){ int id=catItems[cc][j], x2=2+j*14; rect(x2,112,13,10,id==eOb?WHITE:RGB(3,4,7)); rect(x2+1,113,11,8,palCol[id]); } }
        { char b[16]; int xx=text(2,134,"PRICE",DIMC,1)+3; edMoney(b,edCost(palCh[eOb])); xx=text(xx,134,b,WHITE,1)+8;
          if(xo[XO_BUYCOST]&&edCharged()){ text(xx,134,"CASH",DIMC,1); edMoney(b,simMoney); text(xx+26,134,b,edAffordable(edObjCh(),lifeMap[ecy][ecx])?RGB(14,30,14):RGB(31,10,8),1); } }
        { static const char*const faceNm[4]={"FACES S","FACES E","FACES N","FACES W"};
          int xx=text(2,124,palNm[eOb],WHITE,1)+4; if(eOb==OB_KICKER||eOb==OB_QPIPE||eOb==OB_LAUNCH) text(xx,124,faceNm[eRot],GOLD,1); }
        if(eOb>=3){ rect(204,114,34,36,RGB(4,5,8)); tileTop(221,141,RGB(14,14,18));   // preview of the picked item
            switch(eOb){ case 3:blitItem(V_CRATE,221,141);break; case 4:blitItem(V_RAILU,221,141);break; case 5:blitItem(V_FRIDGE,221,141);break;
                case 6:blitItem(V_TOILET,221,141);break; case 7:blitItem(V_DOOR,221,141);break; case 8:blitItem(V_BOARD,221,141);break;
                case OB_KICKER:blitItem(V_KICKER+((eRot-cview)&3),221,141);break; case OB_QPIPE:blitItem(V_QPIPE+((eRot-cview)&3),221,141);break;
                case 12:blitItem(V_LEDGEU,221,141);break; case 13:blitItem(V_BENCHU,221,141);break;
                case OB_LAUNCH:blitItem(V_LAUNCH+((eRot-cview)&3),221,141);break; case 18:blitItem(V_FUNBOX,221,141);break; case 19:blitItem(V_BARREL,221,141);break;
                case 20:blitItem(V_TRASH,221,141);break; case 21:blitItem(V_PLANTER,221,141);break; case 22:blitItem(V_PICNIC,221,141);break;
                case 23:blitItem(V_JERSEYU,221,141);break; case 24:blitItem(V_MPAD,221,141);break;
                case 14:blitItem(V_BED,221,141);break; case 15:blitItem(V_SHOWER,221,141);break; case 16:blitItem(V_SOFA,221,141);break; case 25:blitItem(V_PIPE,221,141);break; case 26:blitItem(V_LAVA,221,141);break; case 27:blitItem(V_BEANBAG,221,141);break; case 28:case 29:drawStairs(221,141,eOb==28);break; case 30:blitItem(V_DEADSET,221,141);break; case 31:blitItem(V_PHONE,221,141);break; case 32:blitItem(V_RADIO,221,141);break; case 33:blitItem(V_STEREO,221,141);break; default:drawSpawn(221,142); } }
        if(eOb==1||eOb==2){ wallSwatch(eWp,212,137); }
    } else if(eTool!=T_ERASE){
        int xx=2;   // label, swatch, name: each placed after the one before, so nothing covers a label
        if(eTool!=T_WALL){ xx=text(2,139,"FLOOR",DIMC,1)+3; texSwatch(&flTex[eFl],xx,137); xx=text(xx+12,139,flTex[eFl].nm,WHITE,1)+10; }
        if(eTool!=T_FLOOR){ xx=text(xx,139,"WALL",DIMC,1)+3; wallSwatch(eWp,xx,137); text(xx+12,139,wpName(eWp),WHITE,1); }
    } else text(2,139,"SELLS WALLS ITEMS AND FLOORS",DIMC,1);
    if(!buy){ char b[16]; int xx=2;
        if(xo[XO_BUYCOST]&&edCharged()){ xx=text(2,126,"CASH",DIMC,1)+3; edMoney(b,simMoney); xx=text(xx,126,b,RGB(14,30,14),1)+10; }
        else xx=text(2,126,xo[XO_BUYCOST]?"NO LIFE YET  FREE":"FREE BUILD",DIMC,1)+10;
        if(eAct&&eTool!=T_FLOOR){ int x0,y0,x1,y1; eRect(&x0,&y0,&x1,&y1); int n=eNet(x0,y0,x1,y1);
            if(n&&xo[XO_BUYCOST]){ xx=text(xx,126,n>0?"COST":"REFUND",DIMC,1)+3; edMoney(b,n>0?n:-n); text(xx,126,b,(n>0&&edCharged()&&n>simMoney)?RGB(31,10,8):WHITE,1); } } }
    text(2,147,toolHint[eTool][0],RGB(16,18,21),1); text(2,153,toolHint[eTool][1],RGB(16,18,21),1);
}
static void edCamSnap(void){ camX=(ecx-ecy)*CA; camY=24+(ecx+ecy+1)*CB-80; camClamp(1); }
static int edCamStep(void){   // dead-zone camera: the view only scrolls when the cursor nears the edge of the screen
    int sx=LOX+(ecx-ecy)*CA, sy=LOY+(ecx+ecy+1)*CB, dx=0, dy=0;
    if(sx<76) dx=sx-76; else if(sx>164) dx=sx-164;
    int lo=eTool==T_ITEM?94:112;   // (BUY has a taller panel at the bottom)
    if(sy<48) dy=sy-48; else if(sy>lo) dy=sy-lo;
    if(!dx&&!dy) return 0;
    if(dx>10) dx=10; if(dx<-10) dx=-10; if(dy>5) dy=5; if(dy<-5) dy=-5;
    int ox=camX, oy=camY; camX+=dx; camY+=dy; camClamp(1);
    return camX!=ox||camY!=oy;
}
static void mmPx(int x,int y,u16 c){ if((unsigned)x<MW&&(unsigned)y<MH) px(SW-MW-3+x,2+y,c); }
static void miniMap(void){   // whole map at 1 px per tile, top right: colours by tile, the camera's view outlined, cursor blinking
    int X0=SW-MW-3, Y0=2;
    rect(X0-1,Y0-1,MW+2,MH+2,RGB(3,4,7));
    for(int y=0;y<MH;y++)for(int x=0;x<MW;x++){
        char c=lifeMap[y][x]; u16 col;
        if(c=='.') col=shade(flFlat[floorMap[y][x]][0],10);
        else { int i=palIdx(c); col=i>=0?palCol[i]:0; }
        px(X0+x,Y0+y,col);
    }
    int ca=(120-LOX)/CA, cb=(80-LOY)/CB-1;   // screen centre as (tx-ty, tx+ty); the screen is a tilted box on the map
    for(int t=-15;t<=15;t++){ int a=ca+t, b=cb-20; mmPx((a+b)>>1,(b-a)>>1,RGB(20,22,24)); b=cb+20; mmPx((a+b)>>1,(b-a)>>1,RGB(20,22,24)); }
    for(int t=-20;t<=20;t++){ int b=cb+t, a=ca-15; mmPx((a+b)>>1,(b-a)>>1,RGB(20,22,24)); a=ca+15; mmPx((a+b)>>1,(b-a)>>1,RGB(20,22,24)); }
    u16 cc=(efr&8)?WHITE:GOLD;
    mmPx(ecx,ecy,cc); mmPx(ecx-1,ecy,cc); mmPx(ecx+1,ecy,cc); mmPx(ecx,ecy-1,cc); mmPx(ecx,ecy+1,cc);
}
static void mapEditor(void){
    int hold[4]={0}, comboUsed=0, dirty=1, lastBl=-1, msgT=0; const char*msg=""; u16 prev=keyNow();
    static const u16 dirK[4]={K_RIGHT,K_LEFT,K_UP,K_DOWN};
    edTried=0; edLife=0; edCashDirty=0; if(xo[XO_BUYCOST]&&!gInPlay) edLoadLife();   // the life's cash, for the prices
    eAct=0; if(ecx<edX0)ecx=edX0; if(ecy<edY0)ecy=edY0; if(ecx>edX1)ecx=edX1; if(ecy>edY1)ecy=edY1; edCamSnap();
    for(efr=0;;efr++){
        u16 k=keyNow(), pr=k&~prev, rel=prev&~k; prev=k;
        int tr[4];
        for(int i=0;i<4;i++){ hold[i]=(k&dirK[i])?hold[i]+1:0; tr[i]=(hold[i]==1)||(hold[i]>oRepDelay()&&(hold[i]&oRepMask())==0); }
        int ux=tr[0]-tr[1], uy=tr[3]-tr[2];
        if((k&K_SEL)&&(tr[2]||tr[3])){   // SELECT + UP / DOWN: the floor above / below (stairs: the ^ and ~ items)
            int nf=curFl+(tr[2]?1:-1); comboUsed=1; ux=uy=0; dirty=1;
            if(nf>=0&&nf<FLR_N){ if(flGo(nf)){ eAct=0; msg=flNm[nf]; msgT=60; } else { msg="TOO MUCH BUILT TO CHANGE FLOOR"; msgT=60; } } else { msg=nf<0?"NO FLOOR BELOW":"NO FLOOR ABOVE"; msgT=40; }
        }
        if(ux||uy){   // screen-relative like walking: up = away from the camera
            int dx=ux+uy, dy=uy-ux; dx=(dx>0)-(dx<0); dy=(dy>0)-(dy<0);
            ecx+=dx; ecy+=dy; if(ecx<edX0)ecx=edX0; if(ecy<edY0)ecy=edY0; if(ecx>edX1)ecx=edX1; if(ecy>edY1)ecy=edY1; if(ecx>=MW)ecx=MW-1; if(ecy>=MH)ecy=MH-1;
            if(eTool==T_ITEM){ if(k&K_A) mapPlace(ecx,ecy,edObjCh()); else if(k&K_B) mapPlace(ecx,ecy,'.'); }
            dirty=1;
        }
        if(pr|rel) dirty=1;
        if(pr&(K_L|K_R)){
            int d=(pr&K_R)?1:-1;
            if(k&K_SEL){ if(eTool==T_ITEM) edCatStep(d); else eWp=(eWp+d+NWALL)%NWALL; comboUsed=1; }   // BUY: the next category; BUILD: the wallpaper
            else if(eTool==T_ITEM) edItemStep(d);
            else if(eTool==T_WALL) eWp=(eWp+d+NWALL)%NWALL;
            else if(eTool!=T_ERASE) eFl=(eFl+d+NFL)%NFL;
        }
        if(rel&K_SEL){ if(!comboUsed){ eTool=toolNext[eTool]; eAct=0; } comboUsed=0; }
        if(pr&K_A){
            if(eTool==T_ITEM&&(k&K_SEL)){ eRot=(eRot+1)&3; comboUsed=1; msg="TURNED"; msgT=20; }   // SEL+A: turn the next ramp
            else if(eTool==T_ITEM){ int m0=simMoney; mapPlace(ecx,ecy,edObjCh()); if(xo[XO_BUYCOST]&&simMoney!=m0){ msg=simMoney<m0?"BOUGHT":"SOLD"; msgT=30; } }
            else if(!eAct){ eAct=1; eAx=ecx; eAy=ecy; }
            else if(eApply()){ eAct=0; msg=eTool==T_ROOM?"ROOM BUILT":eTool==T_WALL?"WALL BUILT":eTool==T_FLOOR?"FLOOR LAID":"SOLD"; msgT=70; }
            else { msg="ROOM NEEDS 3 X 3 OR BIGGER"; msgT=70; }
        }
        if(pr&K_B){ if(eAct) eAct=0; else { int m0=simMoney; mapPlace(ecx,ecy,'.'); if(xo[XO_BUYCOST]&&simMoney>m0){ msg="SOLD"; msgT=30; } } }
        if(dsMsg){ msg=dsMsg; msgT=90; dsMsg=0; dirty=1; }
        if(pr&K_START){
            int c=menu("MAP MENU",mapItems,6);
            if(c==0){ edCashSave(); mapScan(); lifeMode(1); }
            else if(c==1){ edCashSave(); mapSave();
                if(!mapSaved()) toast("SAVE NOT SUPPORTED HERE");
                else if(xo[XO_SLOTSYNC]&&slotSyncActive()) toast("MAP AND SLOT SAVED");   // MAP SAVE TO SLOT option
                else toast("MAP SAVED"); }
            else if(c==2) slotScreen();
            else if(c==3){ settingsScreen(); if(xo[XO_BUYCOST]&&!gInPlay) edLoadLife(); }
            else if(c==4){ if(!xo[XO_RESETASK]||menu("RESET THE MAP",yesNo,2)==1){ if(curFl) flBlankLive(); else if(!nbResetLot()) mapReset(); eAct=0; toast("MAP RESET"); } }
            else if(c==5){ edCashSave(); if(xo[XO_EDSAVE]) mapSave(); break; }
            prev=keyNow(); edCamSnap(); dirty=1; continue;
        }
        if(edCamStep()) dirty=1;
        if(msgT>0&&--msgT==0){ msg=""; dirty=1; }
        int bl=(efr>>3)&1;   // the editor only redraws when something changed or the cursor blinks
        if(dirty||bl!=lastBl){
            drawRoom(1); drawEditorHud(msgT>0?msg:""); if(xo[XO_MINI]) miniMap();
            present(); dirty=0; lastBl=bl;
        } else vsync();
    }
    while((~REG_KEYINPUT)&0x3FF) vsync();
}


// ---------- SLIDER LOCKS (roadmap #5) ----------
// Most creator sliders start LOCKED and are bought in packs with jenes (A on a locked slider, like a part). The ESSENTIALS stay free:
// HEIGHT, WEIGHT, SKIN TONE, EYE SIZE, EYE SHADE and the HAIR / TOP / BOTTOM tones. The Konami code (sUnlock) opens everything.
// A look keeps whatever its sliders already hold (old saves and Sims made before this carry over untouched); a lock only stops EDITING, and the
// dice (ROLL THE DICE / TRUE RANDOM, for you) leave a locked slider in the middle.
// Saved for good, for every life, in the jukebox block: JB_OFF+32 'S' 'K', the unlocked packs (one bit each), the bits xor 0x5A (appended: nothing moved).
// Add a pack: raise NSLK (8 at most for the one byte), add a name and a cost below and the sliders to slkPack().
#define NSLK 6
static const char* const slkNm[NSLK]={"BODY SHAPE","BODY DETAIL","BUTT","FACE DETAIL","EAR SLIDERS","PART SLIDERS"};
static const short slkCost[NSLK]={40,80,50,40,30,60};   // jenes (pDna) for each pack
static u8 slkUl;   // unlocked packs
static void slkSave(void){ volatile u8*m=SRAM_BASE+JB_OFF; m[32]='S'; m[33]='K'; m[34]=slkUl; m[35]=(u8)(slkUl^0x5A); }
static void slkLoad(void){ volatile u8*m=SRAM_BASE+JB_OFF; slkUl=(m[32]=='S'&&m[33]=='K'&&(u8)(m[34]^0x5A)==m[35])?(u8)(m[34]&((1<<NSLK)-1)):0; }
static int slkPack(int id){   // which pack a slider is in: -1 = an essential (always free) or not a slider
    switch(id){
      case LK_HEIGHT: case LK_WEIGHT: case LK_TONE: case LK_EYESZ: case LK_EYETONE: case LK_HTONE: case LK_TTONE: case LK_BTONE: return -1;
      case LK_TORSO: case LK_ARMS: case LK_STANCE: case LK_LEGW: case LK_ARMW: case LK_HEADSZ: case LK_HANDFT: return 0;
      case LK_NECK: case LK_NECKW: case LK_HIPW: case LK_WAISTW: case LK_SHOULW: case LK_THIGHW: case LK_CALFW: case LK_CHESTW: case LK_BELLYW:
      case LK_UARMW: case LK_FARMW: case LK_JAWW: case LK_HANDSZ: case LK_FOOTSZ: return 1;
      case LK_BUTT: case LK_BUTTH: case LK_BUTTW: return 2;
      case LK_EYESP: case LK_EYEHT: case LK_BROWHT: case LK_NOSEHT: case LK_MOUTHW: case LK_MOUTHHT: return 3;
      case LK_EARSZ: case LK_EARLF: case LK_EARFWD: case LK_EARSPR: case LK_EARWID: return 4;
      default: return lkSlide(id)?5:-1;   // every other slider is a part slider (ANT, TAIL, HORN, WING)
    }
}
static int slkFree(int id){ int p=slkPack(id); return p<0||sUnlock||(slkUl>>p&1); }
static void slkMiddle(void){ for(int id=0;id<LK_N;id++) if(lkSlide(id)&&!slkFree(id)) look[id]=0; }   // (0 = the middle notch)
static int slkBuy(int id){   // A on a locked slider: spend jenes on its pack. 1 = bought
    int p=slkPack(id), c=slkCost[p]; char t[32]; static const char* const it[2]={"YES  UNLOCK IT","NO"};
    if(pDna<c){ char*e=t; const char*q="NEED "; while(*q) *e++=*q++; e+=numStr(e,c); q=" JENES"; while(*q) *e++=*q++; *e=0; toast(t); return 0; }
    { char*e=t; const char*q=slkNm[p]; while(*q) *e++=*q++; *e++=' '; e+=numStr(e,c); q=" JENES?"; while(*q) *e++=*q++; *e=0; }
    if(menu(t,it,2)!=0) return 0;
    pDna=(u16)(pDna-c); slkUl|=(u8)(1<<p); slkSave(); persSave(); return 1;
}

// ---------- creature creator ----------
// Pick a look from numbered tabs (like a character creator): the body, the face, the hair, the clothes, Spore-style PARTS that change
// what the creature can do, and (like Create-A-Bore) its ASPIRATION, lifetime want and personality.
// L R change tab | UP DOWN pick a row | LEFT RIGHT change it | SELECT turns the creature | START jumps to DONE | B leaves.
enum { TB_BODY, TB_FACE, TB_HAIR, TB_CLOTHES, TB_PARTS, TB_ASPIRE, TB_DONE, NTAB };
enum { RK_PICK, RK_SWATCH, RK_ACT, RK_SLIDE, RK_PERS, RK_TRAIT, RK_DUO };   // RK_DUO: two buttons side by side in one row (LEFT RIGHT picks the side; id = left action, n = right action, lab/sub = what each side says it does)   // a row picks from named options, picks a colour, is a button, a slider, a persona choice or a trait
enum { AC_PLAY, AC_MAP, AC_MENU, AC_RAND, AC_ADD, AC_FAM, AC_FNAME, AC_LNAME, AC_TRAND, AC_HOUSE };
enum { PS_ASP, PS_LTW, PS_SIGN };
typedef struct { const char*lab,*sub; u8 kind,id,n; } Row;   // sub = second line of a button
static const char* const tabNm[NTAB]={"BODY","FACE","HAIR","CLOTHES","PARTS","ASPIRE","DONE"};
static const char* const shapeNm[NSHAPE]={"AVERAGE","BROAD","BIG HEAD","STUBBY","SLIM","ATHLETIC","TALL","CHUBBY","PEAR","LANKY","STOCKY","HUNCHED","POTBELLY","MUSCLE","PETITE","BARREL","DIGITIGRADE","V-SHAPE","CURVY","RUNNER","SOFT","POWER","LONG LEGS","PUDGY","TODDLER","SPROUT","SPORTY","BELL","STURDY"};
static const char* const eyeNm[NEYE]={"SLEEPY","ROUND","HAPPY","WIDE","ANGRY","CUTE","CAT","DOT","LASHES"};
static const char* const mouthNm[NMOUTH]={"FLAT","SMILE","OH","GRIN","SMIRK","FROWN","TONGUE","FANGS","KITTY"};
static const char* const browNm[6]={"NONE","THIN","THICK","ANGRY","WORRIED","UNIBROW"};
static const char* const noseNm[6]={"NONE","BUTTON","POINTY","WIDE","PIG","ANIMAL"};
static const char* const fearNm[5]={"NONE","CAT","FOX","BUNNY","BEAR"};
static const char* const muzNm[4]={"NONE","SNOUT","MUZZLE","BEAK"};
static const char* const ftailNm[4]={"NONE","FOX","CAT","BUNNY"};
static const char* const cheekNm[5]={"NONE","BLUSH","FRECKLES","WHISKERS","SCAR"};
static const char* const glassNm[4]={"NONE","ROUND","SQUARE","SHADES"};
static const char* const earNm[3]={"NONE","SMALL","BIG"};
static const char* const hairNm[NHAIR]={"CROP","BOWL","LONG","BALD","SPIKY","AFRO","FLAT TOP","SIDE TAIL","BUN"};
static const char* const hatNm[6]={"NONE","CAP","BEANIE","BAND","FEZ","HELMET"};
static const char* const hatColNm[6]={"AS THE TOP","AS THE BOTTOM","WHITE","BLACK","RED","GOLD"};
static const char* const beardNm[3]={"NONE","BEARD","LONG BEARD"};
static const char* const topStyNm[5]={"TEE","LONG SLEEVE","TANK","HOODIE","BARE"};   // BARE: adults only (see lkAllowed, fixLook and the clothes code in buildLook)
static const char* const botStyNm[4]={"PANTS","SHORTS","SKIRT","BARE"};
static const char* const tipNm[7]={"NONE","WHITE","RED","GOLD","BLACK","AS THE TOP","AS THE BOTTOM"};   // TAIL TIP colours (colour slots 5.. see tipSlot)
static const char* const shoeNm[6]={"AS THE BOTTOM","WHITE","BLACK","RED","GOLD","AS THE TOP"};
#define LK_AGE LK_N   // the AGE row is not part of look[]: it picks the life stage
static const char* const* const lookNm[LK_N+1]={shapeNm,0,eyeNm,mouthNm,earNm,hairNm,0,0,0,0,0,0,tailNm,hornNm,backNm,hatNm,hatColNm,beardNm,topStyNm,botStyNm,shoeNm,
                                                browNm,noseNm,cheekNm,glassNm,0,0,0,0,0,0,0,0,clawNm,antNm,patNm,patColNm,0,0,0,0,0,0,0,0,0,fearNm,muzNm,ftailNm,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,tipNm,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,stageNm};
_Static_assert(LK_N==103,"lookNm / lookCol / cnt need a slot for every look");
static const u16* const lookCol[LK_N+1]={0,skinTones,0,0,0,0,hairTones,topTones,botTones,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,eyeTones,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#define TROWS 40   // most rows a tab holds; the card shows 5 at a time and scrolls
static const Row tabRow[NTAB][TROWS]={
  {{"FIRST NAME","LAST NAME",RK_DUO,AC_FNAME,AC_LNAME},{"AGE",0,RK_PICK,LK_AGE,AG_N},{"SHAPE",0,RK_PICK,LK_SHAPE,NSHAPE},{"HEIGHT",0,RK_SLIDE,LK_HEIGHT,9},{"WEIGHT",0,RK_SLIDE,LK_WEIGHT,9},
   {"TORSO",0,RK_SLIDE,LK_TORSO,9},{"ARMS",0,RK_SLIDE,LK_ARMS,9},{"STANCE",0,RK_SLIDE,LK_STANCE,9},{"LEG WIDTH",0,RK_SLIDE,LK_LEGW,9},{"ARM WIDTH",0,RK_SLIDE,LK_ARMW,9},{"HEAD SIZE",0,RK_SLIDE,LK_HEADSZ,9},{"HAND FOOT SIZE",0,RK_SLIDE,LK_HANDFT,9},{"NECK LENGTH",0,RK_SLIDE,LK_NECK,9},{"NECK WIDTH",0,RK_SLIDE,LK_NECKW,9},{"HIP WIDTH",0,RK_SLIDE,LK_HIPW,9},{"WAIST WIDTH",0,RK_SLIDE,LK_WAISTW,9},{"SHOULDERS",0,RK_SLIDE,LK_SHOULW,9},{"THIGH WIDTH",0,RK_SLIDE,LK_THIGHW,9},{"CALF WIDTH",0,RK_SLIDE,LK_CALFW,9},{"CHEST",0,RK_SLIDE,LK_CHESTW,9},{"BELLY",0,RK_SLIDE,LK_BELLYW,9},{"UPPER ARM",0,RK_SLIDE,LK_UARMW,9},{"FOREARM",0,RK_SLIDE,LK_FARMW,9},{"JAW WIDTH",0,RK_SLIDE,LK_JAWW,9},{"HAND SIZE",0,RK_SLIDE,LK_HANDSZ,9},{"FOOT SIZE",0,RK_SLIDE,LK_FOOTSZ,9},
   {"SKIN",0,RK_SWATCH,LK_SKIN,NSKIN},{"SKIN TONE",0,RK_SLIDE,LK_TONE,9},{"BUTT",0,RK_SLIDE,LK_BUTT,9},{"BUTT HEIGHT",0,RK_SLIDE,LK_BUTTH,9},{"BUTT WIDTH",0,RK_SLIDE,LK_BUTTW,9}},   // (the BUTT rows last: cut from the tab below teen)
  {{"EYES",0,RK_PICK,LK_EYES,NEYE},{"EYE COLOUR",0,RK_SWATCH,LK_EYECOL,NSW},{"EYE SHADE",0,RK_SLIDE,LK_EYETONE,9},{"EYE SIZE",0,RK_SLIDE,LK_EYESZ,9},{"EYE SPACING",0,RK_SLIDE,LK_EYESP,9},
   {"EYE HEIGHT",0,RK_SLIDE,LK_EYEHT,9},{"BROWS",0,RK_PICK,LK_BROW,6},{"BROW HEIGHT",0,RK_SLIDE,LK_BROWHT,9},{"GLASSES",0,RK_PICK,LK_GLASS,4},{"NOSE",0,RK_PICK,LK_NOSE,6},
   {"NOSE HEIGHT",0,RK_SLIDE,LK_NOSEHT,9},
   {"MOUTH",0,RK_PICK,LK_MOUTH,NMOUTH},{"MOUTH WIDTH",0,RK_SLIDE,LK_MOUTHW,9},{"MOUTH HEIGHT",0,RK_SLIDE,LK_MOUTHHT,9},{"CHEEKS",0,RK_PICK,LK_CHEEK,5},
   {"EARS",0,RK_PICK,LK_EARS,3},{"EAR SIZE",0,RK_SLIDE,LK_EARSZ,9},{"EAR HEIGHT",0,RK_SLIDE,LK_EARLF,9},{"EAR FRONT BACK",0,RK_SLIDE,LK_EARFWD,9},{"EAR SPREAD",0,RK_SLIDE,LK_EARSPR,9},{"EAR WIDTH",0,RK_SLIDE,LK_EARWID,9}},
  {{"STYLE",0,RK_PICK,LK_HSTYLE,NHAIR},{"COLOUR",0,RK_SWATCH,LK_HCOL,NSW},{"HAIR TONE",0,RK_SLIDE,LK_HTONE,9},{"BEARD",0,RK_PICK,LK_BEARD,3},{"HAT",0,RK_PICK,LK_HAT,6},{"HAT COLOUR",0,RK_PICK,LK_HATCOL,6}},
  {{"TOP",0,RK_SWATCH,LK_TOP,NSW},{"TOP TONE",0,RK_SLIDE,LK_TTONE,9},{"BOTTOM",0,RK_SWATCH,LK_BOT,NSW},{"BOTTOM TONE",0,RK_SLIDE,LK_BTONE,9},{"TOP STYLE",0,RK_PICK,LK_TOPSTY,5},{"BOTTOM STYLE",0,RK_PICK,LK_BOTSTY,4},{"SHOES",0,RK_PICK,LK_SHOE,6}},
  {{"TAIL",0,RK_PICK,LK_TAIL,3},{"HORNS",0,RK_PICK,LK_HORNS,3},{"BACK",0,RK_PICK,LK_BACK,3},{"HANDS",0,RK_PICK,LK_CLAWS,4},{"ANTENNAE",0,RK_PICK,LK_ANTENNA,3},
   {"PATTERN",0,RK_PICK,LK_PATTERN,7},{"PAINT",0,RK_PICK,LK_PATCOL,6},{"ANIMAL EARS",0,RK_PICK,LK_FEARS,5},{"MUZZLE",0,RK_PICK,LK_MUZZLE,4},{"FUR TAIL",0,RK_PICK,LK_FTAIL,4},
   {"ANT LENGTH",0,RK_SLIDE,LK_ANTLEN,9},{"ANT SPREAD",0,RK_SLIDE,LK_ANTSPR,9},{"ANT TIP SIZE",0,RK_SLIDE,LK_ANTTIP,9},
   {"TAIL LENGTH",0,RK_SLIDE,LK_TAILLEN,9},{"TAIL CURL",0,RK_SLIDE,LK_TAILCURL,9},{"TAIL THICKNESS",0,RK_SLIDE,LK_TAILTHK,9},{"TAIL TIP",0,RK_PICK,LK_TAILTIP,7},{"WING SIZE",0,RK_SLIDE,LK_WINGSZ,9},
   {"HORN SIZE",0,RK_SLIDE,LK_HORNSZ,9},{"HORN SPREAD",0,RK_SLIDE,LK_HORNSPR,9},{"HORN CURVE",0,RK_SLIDE,LK_HORNCRV,9},{"HORN HEIGHT",0,RK_SLIDE,LK_HORNHT,9},
   {"TAIL HEIGHT",0,RK_SLIDE,LK_TAILHT,9},{"TAIL SWAY",0,RK_SLIDE,LK_TAILSW,9},{"TAIL TIP LEN",0,RK_SLIDE,LK_TAILTL,9},{"HORN FWD BACK",0,RK_SLIDE,LK_HORNFB,9},{"HORN THICKNESS",0,RK_SLIDE,LK_HORNTH,9},
   {"WING SPREAD",0,RK_SLIDE,LK_WINGSP,9},{"WING HEIGHT",0,RK_SLIDE,LK_WINGHT,9},{"ANT FRONT BACK",0,RK_SLIDE,LK_ANTFB,9},{"ANT GAP",0,RK_SLIDE,LK_ANTGAP,9},{"TAIL SHADE",0,RK_SLIDE,LK_TAILTONE,9},{"HORN SHADE",0,RK_SLIDE,LK_HORNTONE,9},
   {"TAIL TAPER",0,RK_SLIDE,LK_TAILTAPER,9},{"TAIL FLUFF",0,RK_SLIDE,LK_TAILFLUF,9},{"TAIL WAVE",0,RK_SLIDE,LK_TAILWAVE,9},{"TAIL TIP SHADE",0,RK_SLIDE,LK_TIPTONE,9},{"HORN TIP",0,RK_SLIDE,LK_HORNTIP,9},{"WING DROOP",0,RK_SLIDE,LK_WINGDROOP,9},{"WING SHADE",0,RK_SLIDE,LK_WINGTONE,9}},
  {{"ASPIRATION",0,RK_PERS,PS_ASP,AS_PICK},{"LIFETIME WANT",0,RK_PERS,PS_LTW,2},{"SIGN",0,RK_PERS,PS_SIGN,12},
   {"NEAT",0,RK_TRAIT,TR_NEAT,11},{"OUTGOING",0,RK_TRAIT,TR_OUT,11},{"ACTIVE",0,RK_TRAIT,TR_ACT,11},{"PLAYFUL",0,RK_TRAIT,TR_PLAY,11},{"NICE",0,RK_TRAIT,TR_NICE,11}},
  {{"GO LIVE LIFE!","PLAY IT NOW",RK_ACT,AC_PLAY,0},{"ROLL THE DICE","EVERYTHING ROLLS",RK_DUO,AC_RAND,AC_TRAND},{"EDIT MAP","BUILD ROOMS",RK_ACT,AC_MAP,0},{"MAIN MENU","LOOK IS KEPT",RK_ACT,AC_MENU,0},{"HOUSEHOLD","FAMILY AND KIN",RK_ACT,AC_HOUSE,0}} };   // (HOUSEHOLD: hhcreate.h. It holds ADD TO FAMILY, FAMILY and the relations)
static const u8 tabN0[NTAB]={31,21,6,7,40,8,5};
static int tabRows(int t){ return tabN0[t]-(t==0&&stage<AG_TEEN&&!sUnlock?3:0); }   // babies and children: no BUTT rows
#define tabN(t) tabRows(t)
static int tabNext(int t,int d){ return (t+d+NTAB)%NTAB; }

// layout (the panel is x 124..239): tabs down the left edge, the card of rows beside them, key legend under both
#define TBX 128
#define TBW 18
#define TBH 15
#define TBP 17
#define TBY 7
#define CDX 149
#define CDY 5
#define CDW 91
#define CDH 124
#define RW0 (CDY+26)      // first row
#define RHT 19            // row pitch
#define CARD   RGB(8,11,21)
#define CARDED RGB(14,17,30)
#define FOCUS  RGB(6,16,8)
#define GOLD2  RGB(14,11,3)

static void roundRect(int x,int y,int w,int h,u16 c){ rect(x+1,y,w-2,h,c); rect(x,y+1,w,h-2,c); }
static void tri(int x,int y,int dir,u16 c){   // dir 0 left, 1 right (3 wide, 5 tall); 2 up, 3 down (5 wide, 3 tall); x,y = top left
    if(dir<2){ for(int r=0;r<5;r++){ int hf=2-(r<2?2-r:r-2); if(dir==0) rect(x+2-hf,y+r,hf+1,1,c); else rect(x,y+r,hf+1,1,c); } }
    else for(int r=0;r<3;r++){ if(dir==2) rect(x+2-r,y+r,2*r+1,1,c); else rect(x+r,y+r,5-2*r,1,c); }
}
static int kcap(int x,int y,const char*t){   // a little key cap with a label in it; returns the x after it
    int w=tw(t,1)+5; rect(x,y-2,w,10,RGB(22,18,5)); rect(x+1,y-1,w-2,8,RGB(8,9,15)); text(x+3,y,t,GOLD,1); return x+w+2;
}
static int kcapAr(int x,int y,int vert){   // a key cap showing two arrows: up/down or left/right
    int w=vert?17:14; rect(x,y-2,w,10,RGB(22,18,5)); rect(x+1,y-1,w-2,8,RGB(8,9,15));
    if(vert){ tri(x+3,y+1,2,GOLD); tri(x+9,y+2,3,GOLD); } else { tri(x+3,y,0,GOLD); tri(x+8,y,1,GOLD); }
    return x+w+2;
}
static int klab(int x,int y,const char*t){ return text(x,y,t,RGB(20,22,26),1)+6; }
static void disc(int x0,int y0,int r,u16 c){ for(int dy=-r;dy<=r;dy++)for(int dx=-r;dx<=r;dx++) if(dx*dx+dy*dy<=r*r) px(x0+dx,y0+dy,c); }

static const char* const iconArt[6][9]={
  {"...###...","...###...","...###...","..#####..",".#.###.#.",".#.###.#.","...#.#...","...#.#...","...#.#..."},   // body
  {"..#####..",".#.....#.","#.#...#.#","#.#...#.#","#.......#","#.#...#.#","#..###..#",".#.....#.","..#####.."},   // face
  {"..#####..",".#######.","#########","##.....##","#.......#","#.......#",".#.....#.","..#...#..","........."},   // hair
  {".##...##.","####.####","#########","#.#####.#","..#####..","..#####..","..#####..","..#####..","........."},   // clothes
  {"#.......#","##.....##",".#.###.#.","..#####..","..#.#.#..","..#####..","...###...",".........","........."},   // parts (a horned head)
  {"....#....","...###...","..#####..",".#######.","#########",".#######.","..#####..","...###...","....#...."} }; // aspire (the plumbob)
static void drawIcon(int x,int y,int id,u16 c){
    if(id==TB_DONE){ line(x,y+4,x+3,y+7,c); line(x+3,y+7,x+9,y+1,c); line(x,y+3,x+3,y+6,c); line(x+3,y+6,x+9,y,c); return; }
    for(int r=0;r<9;r++)for(int q=0;q<9;q++) if(iconArt[id][r][q]=='#') px(x+q,y+r,c);
}
static void panelBg(void){
    for(int y=0;y<SH;y++){ u16 c=RGB(2+y/55,3+y/38,9+y/14); u32 v=c|((u32)c<<16), *row=(u32*)fb+y*ROW_W; for(int w=SCENE_W;w<ROW_W;w++) row[w]=v; }
    rect(PANEL_X,0,2,SH,GOLD2);
}
static void drawTabs(int tab){
    roundRect(CDX-1,CDY-1,CDW+2,CDH+2,CARDED); roundRect(CDX,CDY,CDW,CDH,CARD);   // the card first, tabs on top of its edge
    for(int i=0;i<NTAB;i++){
        int j=i, y=TBY+j*TBP, sel=(i==tab); u16 ink=sel?WHITE:RGB(14,17,22);
        if(sel){ roundRect(TBX-1,y-1,CDX-TBX+3,TBH+2,GOLD); roundRect(TBX,y,CDX-TBX+3,TBH,CARD); rect(CDX-1,y,3,TBH,CARD); }   // open into the card
        else   { roundRect(TBX,y,TBW,TBH,RGB(4,6,13)); roundRect(TBX+1,y+1,TBW-2,TBH-2,RGB(9,12,22)); }
        if(i==TB_DONE) drawIcon(TBX+4,y+4,TB_DONE,sel?RGB(14,30,12):RGB(8,18,8));
        else { char d[2]={(char)('1'+j),0}; text(TBX+(TBW-tw(d,2))/2,y+1,d,ink,2); }
    }
}
static const char* const lookName(int id,int v){ return lookNm[id][v]; }
static int lkAllowed(int id,int v){   // may this stage pick option v of row id?
    switch(id){
      case LK_SHAPE: return shapeMask()>>v&1;
      case LK_EARS:  return stMaskEars[stage]>>v&1;
      case LK_HSTYLE:return stMaskHair[stage]>>v&1;
      case LK_SKIN: return v<NSKIN;
      case LK_HCOL: case LK_TOP: case LK_BOT: return v<stSwatches[stage];
      case LK_TOPSTY: return v<4||stage>=AG_ADULT;   // BARE (nudity) is for ADULT and ELDER only: never a baby, child or teen
      case LK_BOTSTY: return v<3||stage>=AG_ADULT;
      case LK_TAIL: case LK_HORNS: case LK_BACK: case LK_CLAWS: case LK_ANTENNA: return 1;   // every part can be looked at; a locked one is bought with DNA (or comes off when you leave)
      default: return 1;
    }
}
static int lkCount(int id,int n,int*rank){   // options on offer, and the 1-based place of the current one among them
    int c=0, cur=id==LK_AGE?stage:look[id]; *rank=1;
    for(int v=0;v<n;v++) if(lkAllowed(id,v)){ if(v==cur) *rank=c+1; c++; }
    return c;
}
static void drawPip(int x,int y,int w,int h,int on,int f){ rect(x,y,w,h,on?(f?GOLD:RGB(20,17,6)):RGB(4,6,12)); }
static const char* const powNm[7]={"BALANCE","CHARGE","ARMOUR","GLIDE","CLAMP","SENSE","SLASH"};
static void drawAbilities(int sel){   // PARTS tab: the Spore ability chart under the part rows, then DNA and the focused part's cost or power
    int y0=RW0+3*RHT-2, pw=abPow();
    rect(CDX+5,y0-2,CDW-10,1,GOLD2);
    for(int a=0;a<AB_N;a++){ int y=y0+a*6, v=abOf10(a);
        text(CDX+6,y,abNm[a],RGB(16,19,24),1);
        for(int q=0;q<10;q++) drawPip(CDX+48+q*4,y+1,3,4,q<v,1); }   // ten slots, like a Sims skill bar
    int y=y0+AB_N*6+2; char b[12]; numStr(b,pDna);
    int x=text(CDX+6,y,"JENES",DIMC,1)+3; text(x,y,b,GOLD,1);
    const Row*r=&tabRow[TB_PARTS][sel]; int v=look[r->id];
    if(isPart(r->id)&&!partFree(r->id,v)){ numStr(b,partCost[partOf(r->id)][v]); int w=tw(b,1); text(CDX+CDW-6-w,y,b,RGB(31,12,8),1); text(CDX+CDW-9-w-tw("BUY",1),y,"BUY",RGB(31,12,8),1); }
    else if(isPart(r->id)){ int bit=partPow(r->id,v);
        if(v&&(pw&bit)){ int q=0; while(!(bit>>q&1)) q++; const char*nm=powNm[q]; text(CDX+CDW-6-tw(nm,1),y,nm,RGB(12,30,24),1); }
        else if(v){ char c[12]; c[0]=0;   // no power: say what it does in a fight
            if(v==1&&(r->id==LK_HORNS||r->id==LK_CLAWS)) simCatN(simCat(c,"HIT +"),2); else if(v==1&&r->id==LK_TAIL) simCatN(simCat(c,"HIT +"),1); else if(v==1&&r->id==LK_ANTENNA) simCatN(simCat(c,"DODGE "),8);
            if(c[0]) text(CDX+CDW-6-tw(c,1),y,c,RGB(30,22,10),1); } }
}
static void drawAspire(int sel){   // ASPIRE tab: aspiration, lifetime want and sign as rows, then the personality as five tracks of ten pips
    static const char* const lab[3]={"ASPIRATION","LIFETIME","SIGN"};
    for(int i=0;i<3;i++){
        int y=RW0+i*18, f=(i==sel); char b[8];
        if(f){ rect(CDX+3,y-2,CDW-6,17,FOCUS); rect(CDX+3,y-2,2,17,GOLD); }
        text(CDX+9,y,lab[i],f?WHITE:DIMC,1);
        const char*nm; int cur, cnt;
        if(i==PS_ASP){ nm=aspNm[pAsp]; cur=pAsp; cnt=AS_PICK; }
        else if(i==PS_LTW){ nm=simLtw()->name; cur=pLtw; cnt=2; }
        else { cur=signOf(); nm=signNm[cur]; cnt=12; }
        if(i==PS_ASP&&stage<AG_TEEN) text(CDX+CDW-6-tw("TEEN",1),y,"TEEN",RGB(12,20,26),1);   // babies and children GROW UP first: this starts as a teen
        else { int k=numStr(b,cur+1); b[k]='/'; numStr(b+k+1,cnt); text(CDX+CDW-6-tw(b,1),y,b,f?DIMC:RGB(10,12,16),1); }
        u16 ink=f?GOLD:RGB(10,12,16); int w=tw(nm,1)+(i==PS_ASP?9:0), x=CDX+CDW/2-w/2+1;
        if(x<CDX+14) x=CDX+14; if(x+w>CDX+CDW-14) x=CDX+CDW-14-w;   // long names (KNOWLEDGE and its icon) stay clear of the arrows
        tri(CDX+9,y+9,0,ink); tri(CDX+CDW-12,y+9,1,ink);
        if(i==PS_ASP){ simIcon(x,y+8,simAspIcon[pAsp],f?GOLD:DIMC); x+=9; }
        text(x,y+9,nm,f?WHITE:DIMC,1);
    }
    int y=RW0+3*18-1; char b[8];
    rect(CDX+5,y,CDW-10,1,GOLD2);
    text(CDX+6,y+2,"TRAITS",RGB(16,19,24),1);
    { int k=numStr(b,trLeft()); b[k]=0; int x=CDX+CDW-6-tw("LEFT",1); text(x,y+2,"LEFT",DIMC,1); text(x-3-tw(b,1),y+2,b,trLeft()?GOLD:DIMC,1); }
    for(int t=0;t<TR_N;t++){
        int ty=y+10+t*7, f=(sel==3+t);
        if(f){ rect(CDX+3,ty-1,CDW-6,7,FOCUS); rect(CDX+3,ty-1,2,7,GOLD); }
        text(CDX+7,ty,trNm[t],f?WHITE:DIMC,1);
        for(int q=0;q<10;q++) drawPip(CDX+48+q*4,ty,3,5,q<pTr[t],f);   // ten slots per trait, as in The Sims
    }
}
static u8 duoHalf[NTAB];   // which side (0 left, 1 right) of the tab's duo row LEFT RIGHT has picked
static const u16 pipMask[7]={0,16,257,273,325,341,365};   // die faces: bit = cell of a 3x3 grid (corners, centre, sides)
static void drawDie(int x,int y,int n,u16 face,u16 pip){   // a little 12x12 die showing n
    roundRect(x,y,12,12,RGB(3,4,9)); roundRect(x+1,y+1,10,10,face);
    for(int c=0;c<9;c++) if(pipMask[n]>>c&1) rect(x+2+(c%3)*3,y+2+(c/3)*3,2,2,pip);
}
static void drawDuo(int tab,int y,int f,const Row*r){   // two half-width buttons in one row; the picked side lights up when the row is focused
    for(int sd=0;sd<2;sd++){
        int x=CDX+3+sd*43, on=f&&duoHalf[tab]==sd, act=sd?r->n:r->id; u16 face=on?RGB(31,31,29):RGB(23,25,28);
        rect(x,y-2,42,17,on?FOCUS:f?RGB(7,11,21):RGB(5,8,16)); if(on) rect(x,y+13,42,2,GOLD);
        if(act==AC_RAND) drawDie(x+15,y,5,face,RGB(3,4,9));   // RANDOMIZE: one die
        else if(act==AC_TRAND){ drawDie(x+10,y+2,3,face,RGB(28,5,5)); drawDie(x+20,y-1,6,face,RGB(5,10,28)); }   // TRUE RANDOM: two tumbling dice with coloured pips
        else {   // the name buttons: FIRST / LAST over the name (cut to fit)
            char nb[HH_NM]; const char*nm=act==AC_FNAME?hhPName:(hhPLast[0]?hhPLast:"NONE"); int n=0;
            while(nm[n]&&n<HH_NM-1){ nb[n]=nm[n]; n++; } nb[n]=0; while(n>0&&tw(nb,1)>36) nb[--n]=0;
            text(x+4,y,act==AC_FNAME?"FIRST":"LAST",on?GOLD:DIMC,1); text(x+4,y+8,nb,on?WHITE:DIMC,1);
        }
    }
}
static void drawDoneRows(int sel){   // DONE tab: one slim line per button, the dice duo in the middle; the focused one explains itself underneath
    int y=RW0-2;
    for(int i=0;i<tabN(TB_DONE);i++){
        const Row*r=&tabRow[TB_DONE][i];
        if(r->kind==RK_DUO){ drawDuo(TB_DONE,y,i==sel,r); y+=19; continue; }
        if(i==sel){ rect(CDX+3,y-2,CDW-6,11,FOCUS); rect(CDX+3,y-2,2,11,GOLD); }
        else if(i==0) rect(CDX+3,y-2,CDW-6,11,RGB(5,8,16));
        text(CDX+9,y,r->lab,i==sel?(i==0?GOLD:WHITE):(i==0?RGB(24,20,6):DIMC),1);
        y+=12;
    }
    const Row*q=&tabRow[TB_DONE][sel]; const char*sb=q->kind==RK_DUO?(duoHalf[TB_DONE]?q->sub:q->lab):q->sub;
    if(sb){ rect(CDX+5,CDY+CDH-14,CDW-10,1,GOLD2); text(CDX+CDW/2-tw(sb,1)/2,CDY+CDH-10,sb,DIMC,1); }
}
static void drawRowSet(int tab,int sel){
    if(tab==TB_ASPIRE){ drawAspire(sel); return; }
    if(tab==TB_PARTS) drawAbilities(sel);
    if(tab==TB_DONE){ drawDoneRows(sel); return; }
    int vis=tab==TB_PARTS?3:5, first=sel>vis-1?sel-(vis-1):0;   // five rows fit on the card (three over the PARTS chart): it scrolls to keep the focused one in view
    if(first>0) tri(CDX+CDW/2-2,RW0-6,2,GOLD);
    if(first+vis<tabN(tab)) tri(CDX+CDW/2-2,RW0+vis*RHT-(vis>3?1:3),3,GOLD);   // (five rows: the arrow sits in the gap under the last row, not on its slider knob)
    for(int i=first;i<tabN(tab)&&i<first+vis;i++){
        const Row*r=&tabRow[tab][i]; int y=RW0+(i-first)*RHT, f=(i==sel);
        if(r->kind==RK_DUO){ drawDuo(tab,y,f,r); continue; }
        if(r->kind==RK_ACT){
            rect(CDX+3,y-2,CDW-6,17,f?FOCUS:RGB(5,8,16)); if(f){ rect(CDX+3,y-2,2,17,GOLD); }
            const char*sb=r->sub;
            text(CDX+9,y,r->lab,f?GOLD:WHITE,1); text(CDX+9,y+8,sb,f?WHITE:DIMC,1); continue;
        }
        if(f){ rect(CDX+3,y-2,CDW-6,RHT-1,FOCUS); rect(CDX+3,y-2,2,RHT-1,GOLD); }
        text(CDX+9,y,r->lab,f?WHITE:DIMC,1);
        int cur=r->id==LK_AGE?stage:look[r->id], rk, cnt=lkCount(r->id,r->n,&rk);
        if(r->kind==RK_SLIDE){   // a slider: a track with a notch for each step and a knob on the current one
            if(!slkFree(r->id)){   // locked: the pack's price where the knob would be, a padlock after it (A buys the pack)
                char b[14]; numStr(b,slkCost[slkPack(r->id)]); u16 lc=RGB(28,10,8);
                int x=text(CDX+11,y+10,b,lc,1)+3; x=text(x,y+10,"JENES",lc,1)+4;
                rect(x,y+11,5,4,lc); rect(x+1,y+9,3,2,lc); px(x+2,y+10,f?FOCUS:CARD);
                continue; }
            int pos=slidePos(look[r->id]); u16 ink=f?GOLD:RGB(10,12,16);
            rect(CDX+11,y+12,65,1,f?DIMC:RGB(8,10,16));
            for(int q=0;q<9;q++) rect(CDX+11+q*8,y+(q==4?9:10),1,q==4?7:5,f?DIMC:RGB(8,10,16));
            rect(CDX+11+pos*8-2,y+9,5,7,f?WHITE:RGB(16,18,22)); rect(CDX+11+pos*8-1,y+10,3,5,ink);
            tri(CDX+3,y+10,0,ink); tri(CDX+CDW-6,y+10,1,ink);
            continue;
        }
        { char b[10]; char*e=b; e+=numStr(e,rk); *e++='/'; e+=numStr(e,cnt); *e=0; text(CDX+CDW-6-tw(b,1),y,b,f?DIMC:RGB(10,12,16),1); }   // (two digits for the long colour rows)
        if(r->kind==RK_PICK){
            const char*nm=lookName(r->id,cur); int mx=CDX+CDW/2, lk=isPart(r->id)&&!partFree(r->id,cur);
            tri(CDX+9,y+9,0,f?GOLD:RGB(10,12,16)); tri(CDX+CDW-12,y+9,1,f?GOLD:RGB(10,12,16));
            text(mx-tw(nm,1)/2,y+9,nm,lk?RGB(28,10,8):f?WHITE:DIMC,1);
            if(lk){ int lx=mx+tw(nm,1)/2+3; rect(lx,y+11,5,4,RGB(28,10,8)); rect(lx+1,y+9,3,2,RGB(28,10,8)); px(lx+2,y+10,f?FOCUS:CARD); }   // a little padlock
        } else {
            const u16*pal=lookCol[r->id];
            if(cnt<=8){
                for(int q=0;q<cnt;q++){
                    int x=CDX+8+q*10, on=(q==look[r->id]);
                    if(on){ rect(x-1,y+8,11,11,f?WHITE:RGB(16,18,22)); }
                    rect(x,y+9,9,9,pal[q]);
                }
            } else {   // a long colour row (SKIN): seven swatches scroll along it with the pick, arrows show there is more to either side
                int w0=look[r->id]-3; if(w0<0) w0=0; if(w0>cnt-7) w0=cnt-7;
                u16 ink=f?GOLD:RGB(10,12,16);
                for(int q=0;q<7;q++){
                    int x=CDX+11+q*10, on=(w0+q==look[r->id]);
                    if(on){ rect(x-1,y+8,11,11,f?WHITE:RGB(16,18,22)); }
                    rect(x,y+9,9,9,pal[w0+q]);
                }
                if(w0>0) tri(CDX+3,y+13,0,ink); if(w0<cnt-7) tri(CDX+CDW-6,y+13,1,ink);
            }
        }
    }
}
static void drawCreatorPanel(int tab,int sel){
    panelBg(); drawTabs(tab);
    drawIcon(CDX+6,CDY+6,tab,GOLD); text(CDX+20,CDY+4,tabNm[tab],GOLD,2);
    rect(CDX+5,CDY+20,CDW-10,1,GOLD2);
    drawRowSet(tab,sel);
    const Row*rs=&tabRow[tab][sel]; int act=(rs->kind==RK_ACT||rs->kind==RK_DUO), buy=rs->kind==RK_PICK&&isPart(rs->id)&&!partFree(rs->id,look[rs->id]), lkS=rs->kind==RK_SLIDE&&!slkFree(rs->id), x;
    x=kcap(128,132,"L"); x=kcap(x,132,"R"); x=klab(x,132,"TABS"); x=kcapAr(x,132,1); klab(x,132,"ROW");
    if(buy){ x=kcap(128,142,"A"); x=klab(x,142,"BUY"); x=kcapAr(x,142,0); klab(x,142,"CHANGE"); }
    else if(lkS){ x=kcap(128,142,"A"); klab(x,142,"UNLOCK THE PACK"); }
    else { x=act?kcap(128,142,"A"):kcapAr(128,142,0); x=klab(x,142,act?"CHOOSE":"CHANGE"); if(rs->kind==RK_DUO){ x=kcapAr(x,142,0); klab(x,142,"SIDE"); } }
    x=kcap(128,152,"START"); x=klab(x,152,"DONE"); x=kcap(x,152,"B"); klab(x,152,"BACK");
}
static void drawDial(void){   // the creature's compass: the needle points the way it faces on screen (view 0 = down-left, then clockwise)
    static const signed char ddx[4]={-1,-1,1,1}, ddy[4]={1,-1,-1,1};
    int x0=18, y0=143;
    disc(x0,y0,12,GOLD2); disc(x0,y0,11,RGB(4,6,12));
    for(int i=0;i<4;i++) rect(x0+ddx[i]*7-1,y0+ddy[i]*7-1,2,2,RGB(10,12,18));
    int tx=x0+ddx[view&3]*7, ty=y0+ddy[view&3]*7;
    line(x0,y0,tx,ty,GOLD); rect(tx-1,ty-1,3,3,GOLD); rect(x0-1,y0-1,3,3,WHITE);
    int x=kcap(34,152,"SELECT"); klab(x,152,"TURN");
}
static void drawCreatorScene(void){
    stageOn=1; drawScene(0);
    text(7,6,"CREATE A BORE",RGB(3,3,6),1); text(6,5,"CREATE A BORE",GOLD,1);
    drawDial();
}

// ---- tab actions ----
static int confirmRebuild(void){ static const char* const it[2]={"YES  REBUILD","NO  KEEP BLOCKS"}; return menu("REPLACE YOUR BLOCKS?",it,2)==0; }
static void lookStep(int id,int n,int d){
    if(id==LK_AGE){   // the life stage: a look-built creature is rebuilt, hand-built blocks stay but are cut to a smaller box
        int ns=(stage+d+AG_N)%AG_N;
        if(custom&&ns<stage){ static const char* const it[2]={"YES  CUT THEM","NO  KEEP AGE"}; if(menu("CUT BLOCKS TO FIT?",it,2)!=0) return; }
        setStage(ns); return;
    }
    if(lkSlide(id)){   // sliders: one step along the track, no wrap round, never rebuilds the blocks
        int p=slidePos(look[id])+d; if(p<0||p>8) return;
        look[id]=(u8)slideVal(p); if(id==LK_TONE||(id>=LK_HTONE&&id<=LK_EYETONE)) setColors(); return;   // the colour sliders repaint at once
    }
    int nv=look[id];
    for(int t=0;t<n;t++){ nv=(nv+d+n)%n; if(lkAllowed(id,nv)) break; }   // skip what this stage cannot have
    if(nv==look[id]) return;
    if((id==LK_SHAPE||id==LK_EARS||id==LK_HSTYLE||(id>=LK_TAIL&&id<LK_BROW)||id>=LK_CLAWS)&&id!=LK_TAILTIP&&custom&&!confirmRebuild()) return;   // declined: keep the hand-built blocks
    look[id]=(u8)nv;
    switch(id){
      case LK_SKIN: case LK_HCOL: case LK_TOP: case LK_BOT: case LK_EYECOL: setColors(); break;
      case LK_BROW: case LK_NOSE: case LK_CHEEK: case LK_GLASS: case LK_TAILTIP: break;   // drawn over the face sprites: nothing to rebuild
      case LK_EYES:  sty[0]=nv; restyle(0); break;
      case LK_MOUTH: sty[1]=nv; restyle(1); break;
      default: buildLook(); break;
    }
}

// ---- persona rows (ASPIRE tab) ----
static void persStep(const Row*r,int d){
    if(r->kind==RK_TRAIT){ int t=r->id, v=pTr[t]+d; if(v<0||v>10||(d>0&&trLeft()<=0)) return; pTr[t]=(u8)v; }
    else if(r->id==PS_ASP) pAsp=(u8)((pAsp+d+AS_PICK)%AS_PICK);
    else if(r->id==PS_LTW) pLtw=(u8)((pLtw+d+2)%2);
    else setSign((signOf()+d+12)%12);
    persSave();
}
static int buyPart(int id){   // A on a locked part: spend DNA on it. 1 = bought
    int v=look[id], c=partCost[partOf(id)][v]; char t[24]; static const char* const it[2]={"YES  BUY IT","NO"};
    if(pDna<c){ char*e=t; const char*p="NEED "; while(*p) *e++=*p++; e+=numStr(e,c); p=" JENES"; while(*p) *e++=*p++; *e=0; toast(t); return 0; }
    { char*e=t; const char*p="SPEND "; while(*p) *e++=*p++; e+=numStr(e,c); p=" JENES?"; while(*p) *e++=*p++; *e=0; }
    if(menu(t,it,2)!=0) return 0;
    pDna=(u16)(pDna-c); pUnl|=(u16)(1<<PARTBIT(partOf(id),v)); persSave(); return 1;
}

// ---------- the secret classic creator ----------
// Title screen: UP UP DOWN DOWN LEFT RIGHT LEFT RIGHT B A START unlocks (or locks again) the classic creature screen.
// Creator: once unlocked, pressing START and SELECT together swaps between the new creator and the classic one. Remembered in SRAM.
static int comboSS(u16 k,u16 pressed){ return (k&K_START)&&(k&K_SEL)&&(pressed&(K_START|K_SEL)); }
#define NENT (NPARTS+5)   // classic list: the parts, then AGE, SHAPE (the four original body shapes), GO LIVE LIFE, EDIT MAP, MAIN MENU

// ---- CREATE-A-FAMILY: the creator makes the whole household. ADD TO FAMILY puts a new Sim with the look on screen (and its persona) into
// the household, then you can change the look and add the next one; FAMILY lists them: EDIT swaps one into the creator (you become them,
// the Sim you were takes their place in the family) or MOVE OUT. ----
// ---- naming: an on-screen keyboard with every character the font has (capitals, lowercase, digits and symbols) ----
static const char* const kbRow[7]={"ABCDEFGHIJKLM","NOPQRSTUVWXYZ","abcdefghijklm","nopqrstuvwxyz","0123456789.-'","!?&@#*\"_~$:;,","+/()=%<[]^"};
enum { KB_SPACE, KB_DEL, KB_CLEAR, KB_DONE };
static int nameEdit(char*nm,int max,const char*title,int mayEmpty){   // returns 1 if the name was accepted (nm then holds it)
    char b[24]; int n=0; while(nm[n]&&n<max){ b[n]=nm[n]; n++; } b[n]=0;
    if(n==3&&b[0]=='Y'&&b[1]=='O'&&b[2]=='U'){ n=0; b[0]=0; }   // the stand-in name: start from a clean slate
    int r=0, c=0, blink=0; u16 prev=keyNow(); int hold[10]={0};
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; blink++; for(int i=0;i<10;i++) hold[i]=(k>>i&1)?hold[i]+1:0;
        #define KREP(m,i) ((pr&(m))||(hold[i]>14&&(hold[i]&3)==0))
        int rows=8, cols=r<7?(int)__builtin_strlen(kbRow[r]):4;
        if(KREP(K_DOWN,7)){ r=(r+1)%rows; } if(KREP(K_UP,6)){ r=(r+rows-1)%rows; }
        cols=r<7?(int)__builtin_strlen(kbRow[r]):4; if(c>=cols) c=cols-1;
        if(KREP(K_RIGHT,4)) c=(c+1)%cols; if(KREP(K_LEFT,5)) c=(c+cols-1)%cols;
        #undef KREP
        int key=-1; if(pr&K_A) key=r<7?(u8)kbRow[r][c]:256+c;
        if(pr&K_B){ if(n>0) b[--n]=0; else return 0; }                     // B rubs out a letter; on an empty name it backs out
        if(pr&K_START) key=256+KB_DONE;
        if(pr&K_SEL){ r=7; c=KB_DONE; }
        if(key>=0&&key<256){ if(n<max){ b[n++]=(char)key; b[n]=0; } }
        else if(key==256+KB_SPACE){ if(n<max&&n>0&&b[n-1]!=' '){ b[n++]=' '; b[n]=0; } }
        else if(key==256+KB_DEL){ if(n>0) b[--n]=0; }
        else if(key==256+KB_CLEAR){ n=0; b[0]=0; }
        else if(key==256+KB_DONE){ while(n>0&&b[n-1]==' ') b[--n]=0;
            if(n>0||mayEmpty){ for(int i=0;i<=n;i++) nm[i]=b[i]; return 1; } }
        box(6,6,228,150); text(120-tw(title,1)/2,12,title,GOLD,1);
        { char t[8]; char*e=t; e+=numStr(e,n); *e++='/'; e+=numStr(e,max); *e=0; text(226-tw(t,1),12,t,DIMC,1); }
        rect(14,22,212,14,RGB(5,6,10)); rect(14,35,212,1,GOLD2); int x=text(18,25,b,WHITE,1); if((blink>>4)&1) rect(x+1,25,4,6,GOLD);   // the name so far, and the caret
        for(int rr=0;rr<7;rr++){ const char*row=kbRow[rr]; for(int cc=0;row[cc];cc++){
            int kx=16+cc*16, ky=42+rr*13, f=(rr==r&&cc==c); char t[2]={row[cc],0};
            rect(kx,ky,14,11,f?RGB(6,16,8):RGB(5,7,14)); if(f) rect(kx,ky+10,14,1,GOLD); text(kx+5-(tw(t,1)>3),ky+2,t,f?GOLD:WHITE,1); } }
        static const char* const sk[4]={"SPACE","DEL","CLEAR","DONE"};
        for(int i=0;i<4;i++){ int kx=16+i*52, ky=134, f=(r==7&&c==i); rect(kx,ky,48,11,f?RGB(6,16,8):RGB(5,7,14)); if(f) rect(kx,ky+10,48,1,GOLD); text(kx+24-tw(sk[i],1)/2,ky+2,sk[i],f?GOLD:WHITE,1); }
        text(120-tw("A TYPE  B RUB OUT  START DONE",1)/2,147,"A TYPE  B RUB OUT  START DONE",DIMC,1);
        present();
    }
}
static void famAdd(void){
    if(!dbgOn) return;   // (DEBUG CODE only: the row is hidden without it)
    if(!xo[XO_SIMUSER]){ toast("USER-MADE SIMS ARE OFF"); return; }
    if(custom){ toast("BLOCK-BUILT BODIES STAY YOURS"); return; }
    hhLoad(); int m=hhAdd(look,stage,pAsp,pLtw,pTr);
    if(m<0){ toast("THE HOUSE IS FULL"); return; }
    hhSave(); static char t[36] EWRAM_BSS; char*e=simCat(t,hhM[m].name); e=simCat(e," JOINS  "); e=simCatN(e,hhN+1); e=simCat(e," OF "); simCatN(e,HH_MAX+1); toast(t);
}
static void famMenu(void){
    hhLoad(); if(!hhN){ toast("ONLY YOU SO FAR"); return; }
    static char lb[HH_MAX][32]; const char* it[HH_MAX];
    for(int m=0;m<hhN;m++){ char*e=simCat(lb[m],hhM[m].name); e=simCat(e,"  "); simCat(e,stageNm[hhM[m].stage<AG_N?hhM[m].stage:AG_ADULT]); it[m]=lb[m]; }
    int m=menu("THE FAMILY",it,hhN); if(m<0) return;
    static const char* const act[3]={"EDIT  PLAY AS THEM","MOVE OUT","BACK"}; int c=menu(hhM[m].name,act,3);
    if(c==1){ static const char* const yn[2]={"YES  GOODBYE","NO"}; if(menu("ARE YOU SURE?",yn,2)==0){ hhRemove(m); hhSave(); toast("MOVED OUT"); } return; }
    if(c!=0) return;
    if(custom&&!confirmRebuild()) return;
    HhSim*s=&hhM[m];   // swap: their look, age, persona, name and place in the relationships come to the creator, yours go to them
    for(int i=0;i<LK_N;i++){ u8 t=look[i]; look[i]=s->look[i]; s->look[i]=t; }
    { u8 t=stage; stage=s->stage; s->stage=t; } { u8 t=pAsp; pAsp=s->asp; s->asp=t; } { u8 t=pLtw; pLtw=s->ltw; s->ltw=t; }
    for(int i=0;i<TR_N;i++){ u8 t=pTr[i]; pTr[i]=s->tr[i]; s->tr[i]=t; }
    for(int i=0;i<HH_NM;i++){ char t=hhPName[i]; hhPName[i]=s->name[i]; s->name[i]=t; t=hhPLast[i]; hhPLast[i]=s->last[i]; s->last[i]=t; }
    { int t=hhPUid; hhPUid=s->uid; s->uid=(u8)t; }
    custom=0; ageDays=0; fixLook(); buildLook(); setColors(); ageSave(); persSave(); hhSave();
    static char t[32] EWRAM_BSS; simCat(simCat(t,"NOW EDITING "),hhPName); toast(t);
}
#include "hhcreate.h"   // CREATE-A-HOUSEHOLD: the DONE tab's HOUSEHOLD row (add Sims, who is whose mother / sister / roommate)
static const u8 lkCnt[LK_N]={NSHAPE,NSKIN,NEYE,NMOUTH,3,NHAIR,NSW,NSW,NSW,9,9,9,3,3,3,6,6,3,4,3,6, 6,6,5,4,NSW, 9,9,9,9,9,9,9, 4,3,7,6, 9,9,9,9,9,9,9,9,9, 5,4,4, 9,9,9, 9,9,9,9,9,9,9, 9,9,9,9,9,9,9, 9,9,9,7, 9,9,9, 9};   // how many options each look row has (sliders: 9)
static void lookRandom(void){   // the dice (like Create-A-Bore): a whole new look and personality, only from what this stage and your unlocked parts allow
    const u8*cnt=lkCnt;
    for(int id=0;id<LK_N;id++){
        if(lkSlide(id)){ look[id]=slkFree(id)?(u8)slideVal(rnd8()%5+rnd8()%5):0; continue; }   // most land near the middle (a locked slider stays in the middle)
        for(int t=0;t<20;t++){ int v=rnd8()%cnt[id];
            if(isPart(id)&&(!partFree(id,v)||(rnd8()&1))) v=0;
            if((id==LK_PATTERN||id==LK_FEARS||id==LK_MUZZLE||id==LK_FTAIL||id==LK_TAILTIP)&&(rnd8()%3)) v=0;   // parts: half the time none, never a locked one (and the animal bits now and then)
            if((id==LK_HAT||id==LK_BEARD||id==LK_GLASS||id==LK_CHEEK)&&(rnd8()&1)) v=0;
            if(id==LK_BEARD&&stage<AG_ADULT) v=0;
            if(lkAllowed(id,v)){ look[id]=(u8)v; break; } }
    }
    setSign(rnd8()%12); pAsp=(u8)(rnd8()%AS_PICK); pLtw=(u8)(rnd8()&1); persSave();
    custom=0; fixLook(); buildLook(); setColors();
}
// TRUE RANDOM: every slider (0-8, evenly), every pick, part and colour at random, nothing nudged towards the middle - only what the age
// allows (lkAllowed, fixLook) and no beard on the young. *stg: a stage from child to elder is rolled when it comes in as 255, else kept.
static void lookTrueRandom(u8*lk,u8*stg){
    u8 sl[LK_N], ss=stage; for(int i=0;i<LK_N;i++) sl[i]=look[i];
    if(*stg==255) *stg=(u8)(AG_CHILD+rnd8()%(AG_N-AG_CHILD));
    stage=*stg;
    for(int id=0;id<LK_N;id++){
        if(lkSlide(id)){ look[id]=(u8)(rnd8()%9); continue; }
        int v=0; for(int t=0;t<20;t++){ v=rnd8()%lkCnt[id]; if(lkAllowed(id,v)&&(!isPart(id)||partFree(id,v))) break; v=0; }   // never a part you have not unlocked
        if(id==LK_BEARD&&stage<AG_ADULT) v=0;
        look[id]=(u8)v;
    }
    fixLook(); for(int i=0;i<LK_N;i++) lk[i]=look[i];
    for(int i=0;i<LK_N;i++) look[i]=sl[i]; stage=ss;
}
static void lookTrueRandomMe(void){   // the creator's TRUE RANDOM row: the same for you (your age stays), and a new personality
    slkLoad(); u8 lk[LK_N], st=stage; lookTrueRandom(lk,&st); for(int i=0;i<LK_N;i++) look[i]=lk[i]; slkMiddle();   // (a locked slider stays in the middle)
    setSign(rnd8()%12); pAsp=(u8)(rnd8()%AS_PICK); pLtw=(u8)(rnd8()&1); persSave();
    custom=0; fixLook(); buildLook(); setColors();
}
#define TRIG(m,i) ((pressed&(m))||(hold[i]>14&&(hold[i]&3)==0))   // pressed now, or held long enough to repeat
static int creatorNew(void){   // returns 1 when the secret code switched screens, 0 when leaving
    int tab=0, rs[NTAB]={0}, dirty=3, hold[10]={0}; u16 prev=keyNow(); slkLoad();
    if(!(shapeMask()>>look[LK_SHAPE]&1)){ look[LK_SHAPE]=(u8)maskPick(shapeMask(),look[LK_SHAPE],NSHAPE); if(!custom) buildLook(); }   // BIG HEAD goes away when the Konami code is off
    for(;;){
        u16 k=keyNow(), pressed=k&~prev; prev=k;
        for(int i=0;i<10;i++) hold[i]=(k>>i&1)?hold[i]+1:0;
        if(sUnlock&&comboSS(k,pressed)){ partsSettle(); sClassic=1; settingsSave(); stageOn=0; return 1; }   // the classic block screen: Konami on, START and SELECT held together
        if(pressed&K_R){ tab=tabNext(tab,1); dirty|=2; }
        if(pressed&K_L){ tab=tabNext(tab,-1); dirty|=2; }
        if(rs[tab]>=tabN(tab)) rs[tab]=tabN(tab)-1;   // (a row can go when the age changes)
        if(pressed&K_DOWN){ rs[tab]=(rs[tab]+1)%tabN(tab); dirty|=2; }
        if(pressed&K_UP){ rs[tab]=(rs[tab]+tabN(tab)-1)%tabN(tab); dirty|=2; }
        if(pressed&K_SEL){ view=(view+1)&3; dirty=3; }
        if((pressed&K_START)&&!(k&K_SEL)){ tab=TB_DONE; rs[tab]=0; dirty|=2; }
        if(pressed&K_B){ partsSettle(); stageOn=0; return 0; }
        const Row*r=&tabRow[tab][rs[tab]];
        int d=TRIG(K_RIGHT,4)?1:TRIG(K_LEFT,5)?-1:0;
        if(r->kind==RK_ACT||r->kind==RK_DUO){
            int aid=r->id;
            if(r->kind==RK_DUO){ if(d>0&&!duoHalf[tab]){ duoHalf[tab]=1; dirty|=2; } else if(d<0&&duoHalf[tab]){ duoHalf[tab]=0; dirty|=2; } if(duoHalf[tab]) aid=r->n; }   // LEFT RIGHT picks the side, A presses it
            if(pressed&K_A){
                partsSettle();   // a part still locked comes off before the creature leaves the creator
                switch(aid){
                    case AC_PLAY:  lifeMode(0); if(gToMenu){ stageOn=0; return 0; } break;
                    case AC_MAP:   mapEditor(); break;
                    case AC_RAND:  lookRandom(); break;
                    case AC_TRAND: lookTrueRandomMe(); break;
                    case AC_ADD:   famAdd(); break;
                    case AC_FAM:   famMenu(); break;
                    case AC_HOUSE: hcMenu(); break;
                    case AC_FNAME: if(nameEdit(hhPName,HH_NM-1,"FIRST NAME",0)) hhSave(); break;
                    case AC_LNAME: if(nameEdit(hhPLast,HH_NM-1,"LAST NAME",1)) hhSave(); break;
                    default:       stageOn=0; return 0;   // MAIN MENU
                }
                prev=keyNow(); for(int i=0;i<10;i++) hold[i]=0; dirty=3;
            }
        } else if(r->kind==RK_PERS||r->kind==RK_TRAIT){
            if(d||(pressed&K_A)){ persStep(r,d?d:1); prev=keyNow(); for(int i=0;i<10;i++) hold[i]=0; dirty|=2; }
        } else if((pressed&K_A)&&isPart(r->id)&&!partFree(r->id,look[r->id])){
            buyPart(r->id); prev=keyNow(); for(int i=0;i<10;i++) hold[i]=0; dirty=3;
        } else if(r->kind==RK_SLIDE&&!slkFree(r->id)){   // a locked slider: only A (buy the pack) does anything
            if(pressed&K_A){ slkBuy(r->id); prev=keyNow(); for(int i=0;i<10;i++) hold[i]=0; dirty=3; }
        } else if(d||(pressed&K_A)){
            lookStep(r->id,r->n,d?d:1); prev=keyNow(); for(int i=0;i<10;i++) hold[i]=0; dirty=3;
        }
        if(dirty){
            if(dirty&1) drawCreatorScene();
            drawCreatorPanel(tab,rs[tab]);
            present(); dirty=0;
        } else vsync();
    }
}

__attribute__((noinline)) static void drawClassicPanel(void){
    fillCols(SCENE_W,ROW_W,PANEL);
    text(130,5,"BORE",RGB(31,26,6),2);
    text(130,17,"VOXEL DEMO",RGB(14,16,18),1);
    for(int i=0;i<NENT;i++){
        int y=24+i*6, go=(i>=NPARTS);
        if(i==part){ rect(128,y-1,108,6,go?RGB(16,10,2):RGB(6,16,8)); text(130,y,">",RGB(31,31,31),1); }
        text(137,y,go?(i==NPARTS?"AGE":i==NPARTS+1?"SHAPE":i==NPARTS+2?"GO LIVE LIFE!":i==NPARTS+3?"EDIT MAP":"MAIN MENU"):parts[i].name,i==part?(go?RGB(31,26,6):RGB(31,31,31)):(go?RGB(24,20,6):RGB(18,20,22)),1);
        if(i==NPARTS) text(190,y,stageNm[stage],RGB(31,26,6),1);
        if(i==NPARTS+1) text(190,y,shapeNm[look[LK_SHAPE]<4?look[LK_SHAPE]:0],RGB(31,26,6),1);
        if(i==part&&!go&&parts[i].dk) text(190,y,spr[SPRID(parts[i].dk-1)].name,RGB(31,26,6),1);
    }
    text(130,105,"SIZE",RGB(18,20,22),1);
    const char*sn[3]={"S","M","L"};
    for(int i=0;i<3;i++){
        int x=156+i*16; rect(x,103,12,9,i==size-1?RGB(6,16,8):RGB(2,3,5));
        text(x+4,105,sn[i],i<BXS?RGB(31,31,31):RGB(8,9,11),1);
    }
    text(130,115,"X",RGB(18,20,22),1); num(136,115,cx,RGB(31,31,31));
    text(148,115,"Y",RGB(18,20,22),1); num(154,115,cy,RGB(31,31,31));
    text(166,115,"Z",RGB(18,20,22),1); num(172,115,cz,(part<NPARTS&&parts[part].dk)?RGB(12,14,16):RGB(31,31,31));   // sprites ignore Z
    u16 hc=RGB(12,14,16);
    if(part==NPARTS){ text(130,123,"A OR LEFT RIGHT",RGB(31,26,6),1); text(130,131,"CHANGES THE AGE",RGB(31,26,6),1); text(130,141,"SELECT NEXT ENTRY",hc,1); }
    else if(part==NPARTS+1){ text(130,123,"A OR LEFT RIGHT",RGB(31,26,6),1); text(130,131,"CHANGES THE SHAPE",RGB(31,26,6),1); text(130,141,"SELECT NEXT ENTRY",hc,1); }
    else if(part>=NPARTS){ text(130,123,"PRESS A TO OPEN",RGB(31,26,6),1); text(130,131,"SELECT NEXT ENTRY",hc,1); }
    else {
    text(130,123,"DPAD X Z  L R UP DN",hc,1); text(130,129,"A PLACE B ERASE",hc,1);
    text(130,135,"START SIZE  SEL PART",hc,1); text(130,141,"SEL+UP DN FACE",hc,1);
    text(130,147,"SEL+A SKIN  B HAIR",hc,1); text(130,153,"SEL+L R TURN VIEW",hc,1);
    }
}


// ---------- the classic creature screen (the original editor, kept as a secret) ----------
static int creatorClassic(void){   // returns 1 when the secret code switched screens, 0 when leaving
    stageOn=0;
    u16 prev=keyNow(); int hold[10]={0}, frame=0, dirty=1, lastBlink=-1, comboUsed=(keyNow()&K_SEL)?1:0;
    for(;;){
        u16 k=keyNow(), pressed=k&~prev, released=prev&~k; prev=k;
        for(int i=0;i<10;i++) hold[i]=(k>>i&1)?hold[i]+1:0;
        if(sUnlock&&comboSS(k,pressed)){ sClassic=0; settingsSave(); return 1; }
        int sel=k&K_SEL;
        if(sel){
            if(pressed&K_A){ look[LK_SKIN]=(look[LK_SKIN]+1)%NSKIN; setColors(); comboUsed=1; dirty=1; }
            if(pressed&K_B){ look[LK_HCOL]=(look[LK_HCOL]+1)%NSW; setColors(); comboUsed=1; dirty=1; }
            if(pressed&K_R){ part=(part+1)%NENT; comboUsed=1; dirty=1; }
            if(pressed&K_L){ part=(part+NENT-1)%NENT; comboUsed=1; dirty=1; }
            if(pressed&K_RIGHT){ view=(view+1)&3; comboUsed=1; dirty=1; }
            if(pressed&K_LEFT){ view=(view+3)&3; comboUsed=1; dirty=1; }
            if(pressed&(K_UP|K_DOWN)){
                comboUsed=1;
                if(part<NPARTS&&parts[part].dk){ int kd=parts[part].dk-1; int nk=kd?NMOUTH:NEYE; sty[kd]=(sty[kd]+((pressed&K_UP)?1:nk-1))%nk; look[LK_EYES]=(u8)sty[0]; look[LK_MOUTH]=(u8)sty[1]; dirty=1; }
            }
        } else {
            if(part==NPARTS){   // the AGE entry: LEFT / RIGHT step the life stage backwards / forwards, A steps forwards
                if(pressed&(K_LEFT|K_RIGHT|K_A)){ lookStep(LK_AGE,AG_N,(pressed&K_LEFT)?-1:1); prev=keyNow(); dirty=1; }
            } else if(part==NPARTS+1){   // the SHAPE entry: the four original body shapes (the new creator has the newer ones)
                if(pressed&(K_LEFT|K_RIGHT|K_A)){ if(look[LK_SHAPE]>=4) look[LK_SHAPE]=0; lookStep(LK_SHAPE,4,(pressed&K_LEFT)?-1:1); prev=keyNow(); dirty=1; }
            } else {
            if(TRIG(K_RIGHT,4)){moveView(1,0);dirty=1;} if(TRIG(K_LEFT,5)){moveView(-1,0);dirty=1;}
            if(TRIG(K_UP,6)){moveView(0,-1);dirty=1;}     if(TRIG(K_DOWN,7)){moveView(0,1);dirty=1;}
            if(TRIG(K_R,8)){cy++;dirty=1;}      if(TRIG(K_L,9)){cy--;dirty=1;}
            }
            if((pressed&K_A)&&part!=NPARTS&&part!=NPARTS+1){
                if(part==NPARTS+2){ lifeMode(0); if(gToMenu) return 0; }
                else if(part==NPARTS+3) mapEditor();
                else if(part==NPARTS+4) return 0;   // MAIN MENU
                else { doPart(1,part,size,cx,cy,cz); custom=1; }
                prev=keyNow(); dirty=1;
            }
            if(pressed&K_B){ if(part<NPARTS){ doPart(2,part,size,cx,cy,cz); custom=1; } dirty=1; }
        }
        if(released&K_SEL){ if(!comboUsed){ part=(part+1)%NENT; dirty=1; } comboUsed=0; }
        if(pressed&K_START){ size=size%BXS+1; dirty=1; }
        clampCursor();
        if(dirty) frame=16;   // restart blink with the ghost visible
        int blink=(frame>>4)&1;
        if(dirty||blink!=lastBlink){
            for(int y=0;y<H;y++)for(int z=0;z<D;z++)for(int x=0;x<W;x++){ ghost[y][z][x]=0; gdec[y][z][x]=0; }
            gAny=0;
            if(part<NPARTS){ doPart(0,part,size,cx,cy,cz);
            if(parts[part].dk&&!gAny) ghost[cy][cz][cx]=1; }   // nothing solid under the cursor: show a marker cube
            drawScene(blink); if(dirty){ drawClassicPanel(); present(); } else presentScene();   // blink-only: scene columns only
            dirty=0; lastBlink=blink;
        } else vsync();
        frame++;
    }
}


static void creatureEditor(void){ creatorMusStart(); for(;;){ int sw=(sUnlock&&sClassic)?creatorClassic():creatorNew(); if(!sw) break; } }   // main menu entry: the new creator, or the classic one once the code has been entered

// ---------- jukebox: the MUSIC PLAYER ----------
// ONE screen: a NOW PLAYING card (song, artist, elapsed / total time, progress bar, equalizer, play mode) above the song list. Every row shows a check
// box, the song name, its artist and its LENGTH. Only CHECKED songs are picked at random (jukebox.h): when the jukebox opens, in the main menus, for
// GAME MUSIC and when a song ends in SHUFFLE mode. Opening the jukebox plays ONE random checked song.
//   UP / DOWN browse   A play the song (A again on the playing song stops it)   L / R previous / next song   LEFT / RIGHT volume
//   SELECT check / uncheck   START play mode: SHUFFLE (random checked song) -> IN ORDER (next checked song down the list) -> REPEAT (same song)   B back
// The mode is saved (jukebox.h). Audio runs from interrupts (see above), so the screen may take as long as it likes to redraw; it redraws only what changed.
#define JB_ROWS 7                // visible list rows, 9 px each
#define JB_PX0 2                 // the panel
#define JB_PX1 238
#define JB_PY0 3
#define JB_PY1 157
#define JB_CY0 19                // the NOW PLAYING card: y 19 .. 58
#define JB_CY1 58
#define JB_HY1 59                // the head (title bar + card) ends here; the column header row follows
#define JB_LY 70                 // first list row
#define JB_LY1 (JB_LY+JB_ROWS*9)
#define JB_NX 30                 // song name column (a longer name is cut with .. and scrolls on the cursor row)
#define JB_NW 106
#define JB_AX 140                // artist column (the longest artist today is 57 px)
#define JB_AW 56
#define JB_TR 231                // right edge of the TIME column (times are right-aligned)
#define JB_BODY  RGB(3,4,7)
#define JB_HEAD  RGB(5,7,11)
#define JB_CARD  RGB(5,7,12)
#define JB_EDGE  RGB(13,15,20)
#define JB_TXT   RGB(20,24,29)   // list text: bright enough to read on the small screen
#define JB_DIM   RGB(14,17,22)   // a song that is switched off, labels
#define JB_BAR   RGB(11,24,31)   // volume dashes and progress, like the blue of the original
static int jbPlaying, jbCur=-1, jbMsgT;   // jbCur = visible number of the song that plays (or played last)
static const char*jbMsg; static u8 jbEq[8];
static u8 jbHist[8]; static int jbHN;      // the songs played before this one (L goes back through them)
static u16 jbLenC[JB_MAX] EWRAM_BSS;                 // length of each songs[] entry in seconds + 1 (0 = not worked out yet)
static const char* const jbModeName[3]={"SHUFFLE","IN ORDER","REPEAT"};
static const char* const jbModeInfo[3]={"RANDOM CHECKED SONGS","CHECKED SONGS DOWN THE LIST","THE SAME SONG AGAIN AND AGAIN"};
static void fillBox(int x0,int x1,int y0,int y1,u16 c){   // x0, x1 must be even (32-bit stores)
    u32 v=c|((u32)c<<16);
    for(int y=y0;y<y1;y++){ u32*row=(u32*)fb+y*ROW_W; for(int w=x0>>1;w<(x1>>1);w++) row[w]=v; }
}
static int numAt(int x,int y,int n,u16 c){ return numText(x,y,n,c); }
// ---- lengths (everything in output samples at 18157 Hz, the rate the mixer runs at) ----
static u32 jbRowsTo(const XmSong*s,int ord){ u32 r=0; for(int i=0;i<ord&&i<s->nord;i++) r+=s->rows[s->order[i]]; return r; }   // rows in the first `ord` orders
static u32 jbRowSamp(const XmSong*s,u32 rows){ return rows*(u32)s->rowN+((rows*(u32)s->rfr)>>8); }                          // what the player spends on them
static u32 jbTotalSamp(const Song*sg){   // one pass through the song: that is how long the jukebox plays it
    if(sg->adp){ u32 n0=*(const u32*)sg->adp, n=n0&0x3FFFFFFFu; return (n0>>31)?n*3/2:n; }   // bit 31 = stored at 2/3 rate: 2 stored samples make 3
    return jbRowSamp(sg->xm,jbRowsTo(sg->xm,sg->xm->nord));
}
static int jbSecs(int si){ if(!jbLenC[si]) jbLenC[si]=(u16)(1+(jbTotalSamp(&songs[si])+9078)/18157); return jbLenC[si]-1; }   // rounded to the nearest second
static u32 jbElapsedSamp(const Song*sg){   // how far the song in the main deck has got
    u32 tot=jbTotalSamp(sg), e;
    if(!mPlay) return 0;
    if(mKind){ if(mDone) return tot; e=aSlow?aPos*3/2:aPos; }
    else{ if(mLaps>=1) return tot; e=jbRowSamp(mSong,jbRowsTo(mSong,mOrd)+(u32)mRow); }
    return e>tot?tot:e;
}
static void jbTimeStr(char*b,int s){ int m=s/60; s%=60; if(m>99) m=99; int i=0; if(m>=10) b[i++]=(char)('0'+m/10); b[i++]=(char)('0'+m%10); b[i++]=':'; b[i++]=(char)('0'+s/10); b[i++]=(char)('0'+s%10); b[i]=0; }
static int jbTimeR(int xr,int y,int s,u16 c){ char b[8]; jbTimeStr(b,s); text(xr-tw(b,1),y,b,c,1); return xr-tw(b,1); }   // right-aligned time, returns its left edge
// ---- playing ----
static void jbStart(int v,int fade){   // play visible song v. fade=1 (only when the jukebox opens): crossfade from the menu music. fade=0: the song starts at once, nothing is crossfaded
    jbCur=v; if(!sSnd||v<0){ jbPlaying=0; return; }
    menuOn=0; creOn=0; const Song*sg=&songs[jbMap[v]];
    if(fade) musFadeTo(sg->adp?1:0,sg->adp,sg->xm,XF_SONG); else musBegin(sg->adp?1:0,sg->adp,sg->xm);
    jbPlaying=1;
}
static void jbGo(int v){   // start v and remember the song it replaces, so that L can come back to it
    if(jbPlaying&&jbCur>=0&&jbCur!=v){ if(jbHN==8){ for(int i=0;i<7;i++) jbHist[i]=jbHist[i+1]; jbHN=7; } jbHist[jbHN++]=(u8)jbCur; }
    jbStart(v,0);
}
static int jbNextOrder(int from){   // the next CHECKED song down the list after `from` (with none checked, every song counts); wraps round
    int any=(jbCount()==0);
    for(int i=1;i<=jbN;i++){ int v=(from+i)%jbN; if(any||jbOnVis(v)) return v; }
    return from<0?0:from;
}
static int jbNextSong(int from,int manual){   // the song after `from` for the current mode (REPEAT only repeats on its own: the R button still moves on)
    if(jbMode==2&&!manual) return from;
    if(jbMode==0){ int v=pickSong(); return v<0?from:v; }
    return jbNextOrder(from);
}
// ---- drawing ----
static void jbOutline(int x,int y,int w,int h,u16 c){ rect(x,y,w,1,c); rect(x,y+h-1,w,1,c); rect(x,y,1,h,c); rect(x+w-1,y,1,h,c); }
// text in a column w wide: as is when it fits; else cut with ".." (or, with a scroll offset >= 0, scrolled inside the column)
static void jbCol(int x,int y,int w,const char*s,u16 c,int scroll){
    int fw=tw(s,1);
    if(fw<=w){ text(x,y,s,c,1); return; }
    if(scroll>=0){ clipSet(x,0,x+w,SH); text(x-scroll,y,s,c,1); clipAll(); return; }
    char b[48]; int n=0, ew=tw("..",1); const char*p=s;
    for(;;){ const char*q=p; if(!*q) break; fNext(&q); int k=(int)(q-s); if(k>=(int)sizeof b-3) break; for(int i=0;i<k;i++) b[i]=s[i]; b[k]=0; if(tw(b,1)+ew>w) break; n=k; p=q; }
    for(int i=0;i<n;i++) b[i]=s[i]; b[n]='.'; b[n+1]='.'; b[n+2]=0; text(x,y,b,c,1);
}
static int jbMq(int fw,int w,int t){   // scroll offset for a text fw wide in a column w wide: waits, scrolls, waits, starts over (t counts about 15 per second)
    int r=fw-w; if(r<=0) return 0; int p=t%(r+60); return p<30?0:(p-30<r?p-30:r);
}
static void jbSpeaker(int x,int y,u16 c){ rect(x,y+2,3,3,c); rect(x+3,y+1,1,5,c); rect(x+4,y,1,7,c); rect(x+6,y+2,1,3,c); rect(x+8,y+1,1,5,c); }
static void jbBox(int x,int y,int on,u16 c){   // the check box: 7 x 7, with a tick when the song is on
    jbOutline(x,y,7,7,c);
    if(on){ px(x+1,y+3,c); px(x+2,y+4,c); px(x+3,y+3,c); px(x+4,y+2,c); px(x+5,y+1,c); px(x+1,y+4,c); px(x+2,y+5,c); px(x+3,y+4,c); px(x+4,y+3,c); px(x+5,y+2,c); }
}
static int jbKey(int x,int y,const char*k){   // a key cap, returns the x after it
    int w=tw(k,1)+5; rect(x,y-2,w,10,RGB(20,16,5)); rect(x+1,y-1,w-2,8,RGB(7,8,14)); text(x+3,y,k,GOLD,1); return x+w+2;
}
static int jbLab(int x,int y,const char*t){ return text(x,y,t,RGB(22,25,29),1)+7; }
static void jbPaintHead(int fr){
    rect(JB_PX0+1,JB_PY0+1,JB_PX1-JB_PX0-2,JB_HY1-JB_PY0-1,JB_BODY); rect(JB_PX0+1,JB_PY0+1,JB_PX1-JB_PX0-2,15,JB_HEAD);
    // title bar: name, how many songs are checked, volume
    text(9,8,"TOUKEBOX",RGB(1,2,4),1); text(8,7,"TOUKEBOX",WHITE,1); text(9,7,"TOUKEBOX",WHITE,1);   // a little bold
    int x=numAt(62,8,jbCount(),JB_DIM); x=numAt(text(x,8," OF ",JB_DIM,1),8,jbN,JB_DIM); text(x,8," CHECKED",JB_DIM,1);
    int v=xo[XO_MUSV]; jbSpeaker(148,8,v?WHITE:JB_DIM);
    for(int i=0;i<10;i++){ int h=2+i*6/9; rect(160+i*7,15-h,5,h,i<v?JB_BAR:RGB(6,8,13)); }
    // the NOW PLAYING card
    rect(JB_PX0+3,JB_CY0,JB_PX1-JB_PX0-6,JB_CY1-JB_CY0,JB_CARD); jbOutline(JB_PX0+3,JB_CY0,JB_PX1-JB_PX0-6,JB_CY1-JB_CY0,JB_EDGE);
    int nx=48, nr=232, nw=nr-nx; int live=(sSnd&&jbPlaying&&jbCur>=0);
    for(int b=0;b<8;b++){   // equalizer (flat when nothing plays)
        int e=jbEq[b], t=live?2+((rnd8()*11)>>8):1; e=(t>e)?t:(e>1?e-1:1); jbEq[b]=(u8)e; int ex=11+b*4;
        for(int k=0;k<e;k++) rect(ex,54-k*2,3,1,!live?RGB(6,8,13):k<7?RGB(8,26,8):k<10?RGB(28,26,5):RGB(30,9,6));
    }
    const char*mn=jbModeName[jbMode]; int mw=tw(mn,1)+8;   // the play mode chip, START changes it
    rect(nr-mw,21,mw,10,RGB(8,10,16)); rect(nr-mw,21,mw,1,GOLD); text(nr-mw+4,23,mn,GOLD,1);
    if(!sSnd) text(nx,23,"SOUND IS OFF IN OPTIONS",RGB(30,12,8),1);
    else text(nx,23,live?"NOW PLAYING":jbCur>=0?"STOPPED":"PICK A SONG",live?RGB(14,26,13):JB_DIM,1);
    if(jbCur>=0){
        const Song*ns=&songs[jbMap[jbCur]]; int fw=tw(ns->name,1); u16 nc=live?WHITE:JB_TXT;
        jbCol(nx,31,nw,ns->name,nc,fw>nw?jbMq(fw,nw,fr>>2):-1);
        const char*ar=songArtist(ns);
        if(jbMsgT>0) text(nx,40,jbMsg,GOLD,1); else if(ar) text(nx,40,ar,live?GOLD:JB_DIM,1);
        int tot=jbSecs(jbMap[jbCur]), el=0; u32 es=0, ts=jbTotalSamp(ns);
        if(live){ es=jbElapsedSamp(ns); el=(int)(es/18157); }
        int tl=jbTimeR(nr,49,tot,JB_DIM);                       // total, right
        int bx=nx+30, bw=tl-4-bx; char eb[8]; jbTimeStr(eb,el); text(nx,49,eb,live?WHITE:JB_DIM,1);   // elapsed, left; the bar between
        int fx=(int)(es/(ts/(u32)bw+1)); if(fx>bw) fx=bw;
        rect(bx,52,bw,2,RGB(6,8,13)); rect(bx,52,fx,2,JB_BAR); if(live&&fx>0) rect(bx+fx-1,51,2,4,WHITE);
    } else {
        if(jbMsgT>0) text(nx,31,jbMsg,GOLD,1); else text(nx,31,"UP DOWN BROWSE  A PLAYS",RGB(16,19,24),1);
        text(nx,40,jbModeInfo[jbMode],JB_DIM,1);
        rect(nx+30,52,nw-60,2,RGB(6,8,13));
    }
}
static void jbPaintList(int cur,int fr){
    rect(JB_PX0+1,JB_HY1,JB_PX1-JB_PX0-2,JB_LY1-JB_HY1+1,JB_BODY);
    rect(JB_PX0+1,JB_HY1+1,JB_PX1-JB_PX0-2,10,RGB(7,9,14));
    text(17,JB_HY1+2,"ON",RGB(17,21,27),1); text(JB_NX,JB_HY1+2,"SONG",RGB(17,21,27),1); text(JB_AX,JB_HY1+2,"ARTIST",RGB(17,21,27),1);
    text(JB_TR-tw("TIME",1),JB_HY1+2,"TIME",RGB(17,21,27),1);
    int top=cur-JB_ROWS/2; if(top>jbN-JB_ROWS) top=jbN-JB_ROWS; if(top<0) top=0;
    for(int r=0;r<JB_ROWS&&top+r<jbN;r++){
        int v=top+r, y=JB_LY+r*9, sel=(v==cur), pl=(v==jbCur&&jbPlaying), on=jbOnVis(v);
        const Song*sg=&songs[jbMap[v]]; const char*ar=songArtist(sg);
        if(sel){ rect(JB_PX0+4,y,JB_PX1-JB_PX0-8,9,RGB(8,12,20)); rect(JB_PX0+4,y,2,9,GOLD); }
        else if(r&1) rect(JB_PX0+4,y,JB_PX1-JB_PX0-8,9,RGB(4,5,9));
        u16 tc=(sel||pl)?WHITE:on?JB_TXT:JB_DIM;
        if(pl) jbSpeaker(7,y+1,GOLD);
        jbBox(18,y+1,on,sel?WHITE:on?JB_TXT:JB_DIM);
        int fw=tw(sg->name,1);
        jbCol(JB_NX,y+1,JB_NW,sg->name,tc,(sel&&fw>JB_NW)?jbMq(fw,JB_NW,fr>>2):-1);
        if(ar) jbCol(JB_AX,y+1,JB_AW,ar,sel?GOLD:pl?WHITE:on?RGB(15,19,25):JB_DIM,-1);
        jbTimeR(JB_TR,y+1,jbSecs(jbMap[v]),sel?WHITE:pl?GOLD:on?RGB(15,19,25):JB_DIM);
    }
    if(jbN>JB_ROWS){   // scroll bar
        int th=JB_ROWS*9*JB_ROWS/jbN; if(th<4) th=4; int ty=JB_LY+(JB_ROWS*9-th)*top/(jbN-JB_ROWS);
        rect(JB_PX0+1,JB_LY,2,JB_ROWS*9,RGB(7,9,14)); rect(JB_PX0+1,ty,2,th,GOLD);
    }
}
static void jbPaintFoot(int cur){
    rect(JB_PX0+1,JB_LY1+1,JB_PX1-JB_PX0-2,JB_PY1-JB_LY1-1,JB_BODY); rect(JB_PX0+1,JB_LY1+1,JB_PX1-JB_PX0-2,1,RGB(7,9,14));
    int x=8, y1=137, y2=148;
    x=jbKey(x,y1,"A"); x=jbLab(x,y1,(cur==jbCur&&jbPlaying)?"STOP":"PLAY");
    x=jbKey(x,y1,"SELECT"); x=jbLab(x,y1,"CHECK");
    x=jbKey(x,y1,"START"); x=jbLab(x,y1,"MODE");
    x=jbKey(x,y1,"B"); jbLab(x,y1,"BACK");
    x=8; x=jbKey(x,y2,"UP"); x=jbKey(x,y2,"DOWN"); x=jbLab(x,y2,"BROWSE");
    x=jbKey(x,y2,"L"); x=jbKey(x,y2,"R"); x=jbLab(x,y2,"SKIP");
    x=jbKey(x,y2,"<"); x=jbKey(x,y2,">"); jbLab(x,y2,"VOLUME");
}
static void jbPaintAll(int cur,int fr){
    for(int y=0;y<SH;y++) fillBox(0,SW,y,y+1,RGB(1+y/70,2+y/45,5+y/22));
    rect(JB_PX0-1,JB_PY0-1,JB_PX1-JB_PX0+2,JB_PY1-JB_PY0+2,RGB(1,1,3)); rect(JB_PX0,JB_PY0,JB_PX1-JB_PX0,JB_PY1-JB_PY0,JB_BODY);
    jbOutline(JB_PX0,JB_PY0,JB_PX1-JB_PX0,JB_PY1-JB_PY0,JB_EDGE);
    jbPaintHead(fr); jbPaintList(cur,fr); jbPaintFoot(cur);
}
static void jbSay(const char*m,int t){ jbMsg=m; jbMsgT=t; }
static void jukeboxScreen(void){
    int cur=0, dH=1, dL=1, dF=1, fr=0, held=0; u16 prev=keyNow();
    jbMsgT=0; jbPlaying=0; jbCur=-1; jbHN=0;
    if(jbN<=0){ toast("NO SONGS"); return; }
    if(sSnd){ jbStart(pickSong(),1); if(jbCur>=0) cur=jbCur; }              // opening the jukebox plays ONE random checked song
    jbPaintAll(cur,0); vsync(); dmaRows(fb,VRAM_ADDR,0,ROW_W,0,SH); dH=dL=dF=0;
    int lastStop=(cur==jbCur&&jbPlaying);
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; fr++; uiTicks++;
        if(pr&K_B) break;
        int mv=0; if(pr&K_DOWN) mv=1; if(pr&K_UP) mv=-1;
        if(k&(K_UP|K_DOWN)){ if(++held>=22&&(held&3)==0) mv=(k&K_DOWN)?1:-1; } else held=0;   // hold to scroll through a long list
        if(mv){ cur=(cur+mv+jbN)%jbN; dL=1; }
        if(pr&K_A){   // play the song under the cursor; A on the song that is playing stops it
            if(!sSnd) jbSay("SOUND IS OFF IN OPTIONS",60);
            else if(cur==jbCur&&jbPlaying){ musFadeOut(XF_OUT); jbPlaying=0; }
            else jbGo(cur);
            dH=dL=dF=1;
        }
        if(pr&K_R){ if(!sSnd) jbSay("SOUND IS OFF IN OPTIONS",60); else{ jbGo(jbNextSong(jbCur>=0?jbCur:cur,1)); if(jbCur>=0) cur=jbCur; } dH=dL=dF=1; }   // next song (by the mode)
        if(pr&K_L){   // previous song: the one played before this, else the one above it in the list
            if(!sSnd) jbSay("SOUND IS OFF IN OPTIONS",60);
            else{ int v=jbHN>0?(int)jbHist[--jbHN]:((jbCur>=0?jbCur:cur)+jbN-1)%jbN; jbStart(v,0); if(jbCur>=0) cur=jbCur; }
            dH=dL=dF=1;
        }
        if(pr&K_SEL){ jbToggle(cur); dL=dH=1; if(jbCount()==0) jbSay("NONE CHECKED: ALL PLAY",90); }
        if(pr&(K_LEFT|K_RIGHT)){ int v=xo[XO_MUSV]; if(pr&K_RIGHT){ if(v<10) v++; } else if(v>0) v--; xo[XO_MUSV]=(u8)v; dH=1; }
        if(pr&K_START){ jbMode=(u8)((jbMode+1)%3); jbModeSave(); jbSay(jbModeInfo[jbMode],90); dH=1; }   // SHUFFLE -> IN ORDER -> REPEAT
        if(jbPlaying&&(mKind?mDone:mLaps>=1)){   // song over (no crossfade: the next one starts right at the end): the mode picks the next one
            jbGo(jbNextSong(jbCur,0)); if(jbCur>=0) cur=jbCur; dH=dL=dF=1; }
        if((fr&3)==0){ if(jbPlaying) dH=1; { int fw=tw(songs[jbMap[cur]].name,1); if(fw>JB_NW) dL=1; } }   // now playing card + equalizer; a long name on the cursor row scrolls
        if(jbMsgT>0&&--jbMsgT==0) dH=1;
        { int st=(cur==jbCur&&jbPlaying); if(st!=lastStop){ lastStop=st; dF=1; } }   // A says STOP while the cursor is on the playing song
        int y0=SH, y1=0;
        if(dH){ jbPaintHead(fr); y0=0; y1=JB_HY1; dH=0; }
        if(dL){ jbPaintList(cur,fr); if(JB_HY1<y0) y0=JB_HY1; y1=JB_LY1+1; dL=0; }
        if(dF){ jbPaintFoot(cur); if(JB_LY1+1<y0) y0=JB_LY1+1; y1=SH; dF=0; }
        vsync();
        if(y1>y0) dmaRows(fb,VRAM_ADDR,0,ROW_W,y0,y1);
    }
    jbPlaying=0; settingsSave();   // (the volume you set is an option: saved with the others; the song plays on, the main menu crossfades from it)
    while(keyNow()) vsync();
}
// ---------- main menu music ----------
// GOTTCHO BARRACHO (the borracho rework) plays, and only it, whenever a main menu is open (MENU MUSIC option). It carries on through the quiet screens
// (OPTIONS, ROOM SLOTS, HOW TO PLAY), stops when the game, the creator, the room builder or the jukebox opens, and crossfades back in when you return.
// To use another song for the menu, change the xm_ name in menuSong().
static const Song* menuSong(void){ for(int i=0;i<NSONGS;i++) if(songs[i].xm==&xm_gottcho_barracho_ii) return &songs[i]; return 0; }
static void menuMusStart(void){
    musCtx=0; if(menuOn) return;
    creOn=0;
    const Song*sg=menuSong();
    if(!sSnd||!xo[XO_MENUMUS]||!sg){ musFadeOut(XF_OUT); return; }
    mGain=mGainT=256; musFadeTo(sg->adp?1:0,sg->adp,sg->xm,XF_SONG); menuOn=1;   // crossfades from whatever played before
}
static void menuMusStop(void){ if(!menuOn) return; menuOn=0; musFadeOut(XF_OUT); }
static void menuMusSync(void){ if(sSnd&&xo[XO_MENUMUS]) menuMusStart(); else menuMusStop(); }   // after OPTIONS: SOUND or MENU MUSIC may have changed
static void menuMusTick(void){ if(menuOn&&mPlay&&(musNearEnd(XF_SONG)||(mKind?mDone:mLaps>=1))){ menuOn=0; menuMusStart(); } }   // the song is nearly over: it crossfades into itself again
// ---------- creator music: the chiptune loops (source/chips.h: voiced by tools/make_chiptunes.py, stored as synth data by tools/chip_synth.py) ----------
// Played live by chipMix, one random loop each time the creator opens (the secret ones only after the title-screen code); entering from the menu or the game crossfades.
typedef struct { const char*name; u32 off; u8 secret; } Chip;
#define CHIP(id,n,sec,off) {n,off,sec},
static const Chip chips[]={
#include "chips.h"
};
#undef CHIP
#define NCHIPS ((int)(sizeof(chips)/sizeof(chips[0])))
static int chipLast=-1;
static void creatorMusStart(void){
    musCtx=1; if(creOn) return;
    menuOn=0;
    int ok[NCHIPS], n=0; for(int i=0;i<NCHIPS;i++) if((dbgOn||!chips[i].secret)&&i!=chipLast) ok[n++]=i;
    if(!sSnd||!xo[XO_CREMUS]||n==0){ musFadeOut(XF_OUT); return; }
    lrng^=((u32)R_TM2D<<8^uiTicks)*2654435761u; int k=ok[(((unsigned)rnd8()<<8|(unsigned)rnd8())*(unsigned)n)>>16];
    chipLast=k; mGain=mGainT=256; musFadeTo(2,chipsyn+chips[k].off,0,XF_SONG); creOn=1;
}

// ---------- main menu ----------
static const char* const jbHelp[15]={">PLAYING","UP DOWN PICK A SONG  A PLAYS IT","A ON THE PLAYING SONG STOPS IT","L R GO TO THE PREVIOUS OR NEXT SONG",">CHECK BOXES","SELECT CHECKS OR UNCHECKS A SONG","ONLY CHECKED SONGS ARE PICKED AT RANDOM:","HERE  IN THE MENUS  AND FOR GAME MUSIC",">PLAY MODE","START CHANGES IT:  SHUFFLE  IN ORDER  REPEAT","WHEN A SONG ENDS THE MODE PICKS THE NEXT",">OTHER","LEFT RIGHT CHANGE THE VOLUME","OPENING IT PLAYS ONE RANDOM CHECKED SONG","B GOES BACK TO THE MENU"};
#include "acid.h"   // ACID RAINBOW: a live plasma, one of the main menu's backdrops
// ---- the Sims 3 look: glossy rounded panels and pill buttons ----
static u16 s3Mix(u16 a,u16 b,int t,int n){ if(n<=1) return a; int m=n-1;
    int r=(a&31)+((int)(b&31)-(int)(a&31))*t/m, g=((a>>5)&31)+((int)((b>>5)&31)-(int)((a>>5)&31))*t/m, c=((a>>10)&31)+((int)((b>>10)&31)-(int)((a>>10)&31))*t/m; return RGB(r,g,c); }
static int s3Sq(int v){ int k=0; while((k+1)*(k+1)<=v) k++; return k; }
static int s3In(int j,int h,int r){ int e=j<r?2*(r-j)-1:j>=h-r?2*(j-(h-r))+1:0; return e?r-s3Sq(4*r*r-e*e)/2:0; }   // how far row j of a rounded box is pulled in
static void s3Box(int x,int y,int w,int h,int r,u16 top,u16 bot){ if(2*r>h) r=h/2; for(int j=0;j<h;j++){ int in=s3In(j,h,r); rect(x+in,y+j,w-2*in,1,s3Mix(top,bot,j,h)); } }
static void s3Panel(int x,int y,int w,int h){ s3Box(x,y,w,h,12,RGB(3,8,19),RGB(2,5,13)); s3Box(x+2,y+2,w-4,h-4,10,RGB(23,29,31),RGB(13,22,31)); rect(x+12,y+3,w-24,1,RGB(29,31,31)); }
static void s3Well(int x,int y,int w,int h){ s3Box(x,y,w,h,5,RGB(13,21,30),RGB(16,24,31)); s3Box(x+1,y+1,w-2,h-2,4,RGB(25,30,31),RGB(21,28,31)); }   // a sunken well (the town card, the tiles)
static void s3Pill(int x,int y,int w,int h,int on,const char*s){   // the focused one is green, like the game's
    s3Box(x,y,w,h,h/2,on?RGB(4,10,2):RGB(6,11,20),on?RGB(3,8,1):RGB(5,9,17));
    s3Box(x+1,y+1,w-2,h-2,(h-2)/2,on?RGB(21,30,9):RGB(29,31,31),on?RGB(10,22,2):RGB(18,25,31));
    s3Box(x+h/2,y+2,w-h,(h-4)/2,2,on?RGB(25,31,15):RGB(31,31,31),on?RGB(21,30,9):RGB(27,30,31));   // the gloss
    text(x+(w-tw(s,1))/2,y+(h-6)/2,s,on?RGB(1,4,0):RGB(2,5,11),1);
}
static void s3Round(int x,int y,int on,const char*glyph){ disc(x,y,7,on?RGB(4,10,2):RGB(3,7,16)); disc(x,y,6,on?RGB(14,27,6):RGB(9,16,27)); text(x-tw(glyph,1)/2+1,y-3,glyph,on?RGB(1,4,0):WHITE,1); }
static void s3Tip(const char*t){ rect(0,150,SW,10,RGB(2,5,12)); rect(0,150,SW,1,RGB(8,14,26)); text((SW-tw(t,1))/2,152,t,RGB(26,29,31),1); }
#include "neighborhood.h"   // THE NEIGHBORHOOD: a town of lots to live in, visit and build on (main menu)
#include "households.h"     // THE TOWN'S HOUSEHOLDS: who lives where, the household bank, visitors, the phone
#include "fx.h"             // GHOSTS and WEATHER: hardware sprites on the spare OBJ slots (see the top of the file)
#include "story.h"          // STORY MODE: chapters with goals (NEW GAME > STORY MODE)
#include "career.h"         // CAREER TRACKS: the screen on the phone (the tracks are in sims.h)
// ---------- main menu (The Sims 3 look): a glossy panel over your town, lit for the time of day of your life's clock ----------
#define MM_N 7
static const char* const mmName[MM_N]={"Play","Create a Bore","Build Mode","Toukebox","Room Slots","Options","?"};
static const char* const mmDesc[MM_N]={"YOUR LIFE  YOUR TOWNS  OR A NEW GAME","DESIGN YOUR OWN VOXEL CHARACTER","BUILD WALLS AND LAY FLOORS AND WALLPAPER","LISTEN  PICK  OR SHUFFLE THE SONGS","SAVE AND LOAD ROOMS  PEOPLE AND LIVES","SPEED  GAMEPLAY  SOUND  BUTTONS AND MORE","HOW TO PLAY  LEARN THE CONTROLS"};
static u8 mmTod, mmAcid; static s8 mmLot=-1;   // the time of day, and the acid rainbow in place of the town (picked on each visit)
static int mmPickTod(void){   // 5-8 dawn, 8-17 day, 17-20 dusk, else night (no life yet: any)
    int m=simsCheck(SIM_SRAM)?simGet16(SIM_SRAM,16):-1; if(m<0) return rnd8()&3;
    return m<300?2:m<480?3:m<1020?0:m<1200?1:2;
}
static void mmPick(void){ mmTod=(u8)mmPickTod(); mmLot=-1; mmAcid=(u8)(xo[XO_MENUBG]==2||(xo[XO_MENUBG]==0&&(rnd8()&1))); }   // a new view (MENU BACKDROP option)
static void mmBackdrop(void){   // your town close up around a random lot (no town yet: BOREVILLE as it will look), or the acid rainbow
    if(mmAcid){ acidBg(acT); return; }
    int had=nbOk; if(!had){ nbGen(NS_SUBURB,nsTown[NS_SUBURB]); nbT.cur=0; }
    u8 st=nbT.tod, sz=nbT.zoom; nbT.tod=mmTod; nbT.zoom=1;
    int on[NB_LOTS], n=0; for(int i=0;i<NB_LOTS;i++) if(nbT.lot[i].on) on[n++]=i;
    int cx=NB_W/2, cy=NB_H/2;
    if(n){ if(mmLot<0||!nbT.lot[(int)mmLot].on) mmLot=(s8)on[rnd8()%n]; const NbLot*L=&nbT.lot[(int)mmLot]; cx=L->x+L->w/2; cy=L->y+L->h/2; }
    nbDrawTown(cx,cy,-2,0,0,0);
    nbT.tod=st; nbT.zoom=sz; if(!had) nbT.tag[0]=0;
}
static void mmLogo(int x,int y){ for(int j=0;j<LOGO_SH;j++){ const char*r=logoSmallArt[j]; u16*o=&fb[(y+j)*SW+x]; for(int i=0;i<LOGO_SW;i++){ char c=r[i]; if(c!='0') o[i]=logoPal[(c<='9'?c-'0':c-'a'+10)-1]; } } }
static void drawMainMenu(int sel,int full){
    if(full) mmBackdrop();
    s3Panel(60,20,120,128);
    for(int i=0;i<6;i++) s3Pill(70,i?52+(i-1)*15:31,100,i?13:17,i==sel,mmName[i]);
    s3Round(73,136,sel==6,"?");   // HOW TO PLAY
    s3Box(84,129,86,14,7,RGB(6,12,24),RGB(3,8,18)); s3Round(108,136,1,"A"); text(118,133,"Select",WHITE,1);
    mmLogo(SW/2-LOGO_SW/2,1); s3Tip(mmDesc[sel]);
}
// ---------- HOW TO PLAY: a Sims 2 style control panel (main menu only) ----------
// Glossy blue panels, rounded tabs along the top, a bobbing green plumbob next to the title, a scrolling text pane with a thumb,
// and a button strip along the bottom.  L R (or LEFT RIGHT, or A) change the tab, UP DOWN scroll, B or START close.
static void s2rr(int x,int y,int w,int h,u16 c){ rect(x+1,y,w-2,h,c); rect(x,y+1,w,h-2,c); }   // a rounded rectangle
static void s2grad(int x,int y,int w,int h,int r0,int g0,int b0,int r1,int g1,int b1){        // a vertical gradient
    for(int i=0;i<h;i++){ int t=h>1?i*256/(h-1):0; rect(x,y+i,w,1,RGB(r0+(r1-r0)*t/256,g0+(g1-g0)*t/256,b0+(b1-b0)*t/256)); } }
static void s2plumbob(int cx,int y){   // the green diamond: dark left half, light right half, a glint
    for(int i=0;i<7;i++){ int hw=i<4?i:6-i; rect(cx-hw,y+i*2,hw+1,2,i<3?RGB(3,20,6):RGB(2,14,4)); rect(cx+1,y+i*2,hw,2,i<3?RGB(14,31,16):RGB(8,26,10)); }
    rect(cx+1,y+3,1,2,RGB(26,31,26)); }
static void s2pill(int x,int y,int w,const char*s){ s2rr(x,y,w,11,RGB(10,20,30)); s2grad(x+1,y+1,w-2,9,6,15,25,3,9,17); text(x+(w-tw(s,1))/2,y+2,s,RGB(20,27,31),1); }
static void howToPlay(void){
    static const signed char bob[8]={0,1,2,2,1,0,-1,-1};
    static const char* const tn[7]={"PLAY","MAKE","BUILD","MUSIC","PLANS","OPTS","TOWN"};
    static const char* const tt[7]={"PLAYING","CREATE A BORE","BUILD ROOMS","TOUKEBOX","BLUEPRINTS","OPTIONS","NEIGHBORHOOD"};
    const char* const* ln[7]={lifeHelp,creatureHelp,mapHelp,jbHelp,slotHelp,optHelp,nbHelp};
    static const unsigned char nn[7]={18,15,14,15,13,12,16};
    enum { VIS=13, LY=34, LH=104 };
    int tab=0, sc=0; u32 cnt=0, lt=~0u; u16 prev=keyNow();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k; cnt++;
        if(pr&(K_B|K_START)) return;
        if(pr&(K_R|K_RIGHT|K_A)){ tab=(tab+1)%7; sc=0; }
        if(pr&(K_L|K_LEFT)){ tab=(tab+6)%7; sc=0; }
        int n=nn[tab], mx=n>VIS?n-VIS:0;
        if((pr&K_DOWN)&&sc<mx) sc++;
        if((pr&K_UP)&&sc>0) sc--;
        if(!pr&&(cnt>>3)==lt){ vsync(); continue; }   // idle: the picture on the screen is still right (the whole backdrop used to be redrawn every frame, so taps landed between polls and were lost)
        lt=cnt>>3;
        objHideAll();
        s2grad(0,0,SW,SH,1,4,10,2,9,17);                                       // the backdrop: deep Sims blue
        for(int y=0;y<SH;y+=8) for(int x=(y&8)?4:0;x<SW;x+=8) rect(x,y,1,1,RGB(3,9,17));   // a faint diamond lattice
        s2rr(1,1,238,158,RGB(10,20,30)); s2rr(2,2,236,156,RGB(2,6,13));        // the frame
        s2grad(3,3,234,14,8,18,28,3,10,19); rect(3,17,234,1,RGB(14,26,31));    // the title bar
        s2plumbob(11,2+bob[(cnt>>3)&7]); text(21,7,"HOW TO PLAY",WHITE,1);
        { const char*t=tt[tab]; text(233-tw(t,1),7,t,RGB(17,29,31),1); }
        for(int i=0;i<7;i++){ int x=4+i*33, on=i==tab;                          // the tabs
            s2rr(x,19,32,12,on?RGB(16,27,31):RGB(7,14,22));
            if(on) s2grad(x+1,20,30,11,10,22,31,5,14,25); else s2grad(x+1,20,30,11,3,9,18,2,6,13);
            text(x+(32-tw(tn[i],1))/2,22,tn[i],on?WHITE:RGB(12,18,24),1); }
        s2rr(3,31,234,113,RGB(10,20,30)); s2rr(4,32,232,111,RGB(2,6,13));      // the text pane
        for(int i=0;i<VIS&&sc+i<n;i++){ const char*l=ln[tab][sc+i]; int y=LY+i*8;
            if(l[0]=='>'){ s2grad(6,y-1,222,9,6,16,26,3,9,17); rect(9,y+2,3,3,RGB(8,28,10)); text(15,y,l+1,RGB(17,29,31),1); }
            else text(11,y,l,RGB(27,30,31),1); }
        rect(232,LY,4,LH,RGB(4,10,18));                                          // the scroll thumb
        if(mx>0){ int th=LH*VIS/n; if(th<8) th=8; int ty=LY+(LH-th)*sc/mx; s2grad(232,ty,4,th,12,24,31,6,16,26); }
        else s2grad(232,LY,4,LH,6,14,22,4,10,18);
        s2pill(5,147,50,"L R TAB"); s2pill(59,147,86,"UP DOWN SCROLL"); s2pill(149,147,46,"B BACK");   // the button strip
        { char pg[8]; pg[0]=(char)('1'+tab); pg[1]='/'; pg[2]='7'; pg[3]=0; s2rr(199,147,36,11,RGB(10,20,30)); s2rr(200,148,34,9,RGB(2,6,13)); text(199+(36-tw(pg,1))/2,149,pg,RGB(17,29,31),1); }
        present();
    }
}

// ---------- NEW GAME: a fresh life in the chosen town, started three ways (the story mode can start from here later) ----------
static const char* const ngIt[4]={"CREATE A BORE","A PRE-MADE FAMILY","A TRULY RANDOM SIM","STORY MODE"};
static int newGame(int slot){   // 1 = it started (and ended: back to the main menu)
    int c=menu("HOW DO YOU START?",ngIt,4); if(c<0) return 0;
    int story=0; if(c==3){ story=storyPick(); if(!story) return 0; }
    int f=0; if(c==1){ const char* fm[HH_NFAM]; for(int i=0;i<HH_NFAM;i++) fm[i]=hhFams[i].fam; f=menu("WHICH FAMILY?",fm,HH_NFAM); if(f<0) return 0; }
    static const char* const yn[2]={"YES  NEW LIFE","NO"}; if(!sgWant&&menu("START OVER?",yn,2)!=0) return 0;   // (a NEW PLAYER has nothing to start over: the player in play was saved first)
    if(slot>=0){ if(!nbSwitch(slot)){ nbOk=nbLoad(); toast(nbErr); return 0; } nbOk=1; nbBounds(); }
    if(sgWant){ sgPickHome(); sgPid=sgWant; sgWant=0; } else sgPid=0;   // a NEW PLAYER gets a home lot and a save file of their own; a new life started elsewhere belongs to no save file
    twKeep=0; simsNewLife(); moodReset(); lscore=0; simLastScore=0;
    hhN=0; for(int a=0;a<HU_N;a++)for(int b=0;b<HU_N;b++){ relD[a][b]=relL[a][b]=0; relF[a][b]=0; } kinClear();   // the old household moves out
    stOff();
    if(c==1&&hhMoveIn(&hhFams[f])>0){ hhSwap(&hhM[0]); hhRemove(0); }   // you are the family's first Sim (who you were leaves)
    else if(c==2) lookTrueRandomMe();
    else if(c==3) storySetup(story);   // STORY MODE: who you live with, and chapter 1
    hhSave(); sprKey=0; if(sgPid) sgSave();   // (the save file exists from the first minute)
    if(c==0||c==3) creatureEditor();   // make your Sim, then GO LIVE LIFE
    else lifeMode(0);
    return 1;
}

// ---------- PLAY (The Sims 3 New Game panel): pick a town, then CONTINUE your life, VISIT the town, or a NEW GAME there ----------
static void plDraw(const int*l,int n,int sel,int act,int foc,int tile,int full){   // full 0: only what the cursor changes (the dropdown, the tiles, the tip)
    int ok=n&&nbRead(l[sel],&nbTmp);
    if(full){
    s3Panel(8,16,224,124); text(22,24,"Play",RGB(3,9,20),1);
    s3Well(16,31,208,55);
    rect(20,35,74,47,RGB(3,8,18));
    if(ok) nbThumb(&nbTmp,21,36,72,45);
    text(100,34,"Select a Town:",RGB(3,9,20),1);
    }
    s3Box(100,43,118,13,4,foc==0?RGB(12,20,31):RGB(8,14,27),foc==0?RGB(6,12,26):RGB(4,9,20));
    rect(100,56,118,1,foc==0?RGB(14,27,6):RGB(23,29,31));
    text(106,46,ok?nbTmp.name:"NO TOWNS",WHITE,1); text(196,46,"<>",RGB(20,26,31),1);
    if(ok&&full){ int lots=0,homes=0,wat=0,sand=0; for(int i=0;i<NB_LOTS;i++) if(nbTmp.lot[i].on){ lots++; if(nbTmp.lot[i].kind==LKIND_RES&&nbTmp.lot[i].slot>=0) homes++; }
        for(int y=0;y<NB_H;y++)for(int x=0;x<NB_W;x++){ int g=NB_GR(nbTmp.cell[y][x]); wat+=g==NT_WATER; sand+=g==NT_SAND; }
        char b[40]; char*e=slNum(b,lots); e=slCat(e," LOTS  "); e=slNum(e,homes); slCat(e,homes==1?" HOUSE":" HOUSES");
        text(102,60,sand>100?"OUT IN THE DESERT":wat>40?"A TOWN BY THE WATER":"A QUIET GREEN SUBURB",RGB(3,9,20),1);
        text(102,69,b,RGB(5,12,22),1); text(102,78,l[sel]==act?"YOU LIVE HERE":seasNm[nbTmp.season&3],l[sel]==act?RGB(4,16,2):RGB(5,12,22),1); }
    s3Well(16,89,208,42);
    for(int i=0;i<2;i++){ int x=22+i*72, y=92, on=foc==1&&tile==i;
        s3Box(x,y,66,36,5,on?RGB(14,27,6):RGB(9,15,25),on?RGB(8,20,3):RGB(7,12,22)); s3Box(x+1,y+1,64,34,4,on?RGB(26,31,20):RGB(27,30,31),on?RGB(20,29,12):RGB(20,26,31));
        if(i==0) nbIsoBox(x+33,y+17,11,7,7,RGB(20,18,14),RGB(28,26,20),RGB(14,4,4),RGB(20,6,5));
        else if(ok) nbThumb(&nbTmp,x+13,y+4,40,18);   // (small: cheap enough to draw again)
        text(x+33-tw(i?"Visit Town":"Continue",1)/2,y+25,i?"Visit Town":"Continue",RGB(2,5,11),1); }
    { int on=foc==1&&tile==2; disc(190,106,10,on?RGB(4,10,2):RGB(5,10,20)); disc(190,106,9,on?RGB(14,27,6):RGB(18,25,31)); rect(185,105,11,2,on?RGB(1,4,0):WHITE); rect(189,101,2,11,on?RGB(1,4,0):WHITE);
      text(190-tw("New Game",1)/2,119,"New Game",RGB(2,5,11),1); }
    if(full){ disc(120,140,9,RGB(3,8,19)); disc(120,140,7,RGB(10,18,30)); for(int d=0;d<2;d++){ line(116,140+d,119,143+d,WHITE); line(119,143+d,125,136+d,WHITE); }   // the check button (A)
    mmLogo(SW/2-LOGO_SW/2,0); }
    if(foc==0) s3Tip("LEFT RIGHT TOWN  SELECT NEW  START RENAME");
    else if(tile==0) s3Tip("PLAY ON WHERE YOU LEFT OFF");
    else if(tile==1) s3Tip("THE TOWN MAP  LOTS  MOVE IN AND BUILD");
    else s3Tip("A FRESH START IN THIS TOWN");
}
static void playScreen(void){
    int l[SLOT_MAX], n=nbFirstTowns(l);
    nbOk=nbLoad(); int act=nbTS, sel=0; for(int i=0;i<n;i++) if(l[i]==act) sel=i;
    int foc=1, tile=0, dirty=6; u16 prev=keyNow();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_UP){ foc=0; dirty|=1; }
        if(pr&K_DOWN){ foc=1; dirty|=1; }
        int d=(pr&K_RIGHT)?1:(pr&K_LEFT)?-1:0, dt=(pr&K_R)?1:(pr&K_L)?-1:0;
        if(foc==0&&d) dt=d; else if(d){ tile=(tile+3+d)%3; dirty|=1; }
        if(dt&&n){ sel=(sel+n+dt)%n; dirty|=4; }   // another town: its picture and description too
        if(pr&K_B) break;
        if(pr&K_A){
            if(foc==0){ foc=1; dirty|=1; }
            else if(tile==0){ lifeMode(0); break; }
            else if(tile==1&&n){ if(!nbSwitch(l[sel])){ nbOk=nbLoad(); toast(nbErr); } else { nbOk=1; neighborhoodScreen(); if(gToMenu) break; } }
            else if(tile==2){ if(!dbgOn) toast("USE NEW PLAYER ON THE PLAYERS SCREEN"); else if(newGame(n?l[sel]:-1)) break; }   // (the old new game on the old lot is a secret: debug code)
            n=nbTownList(l,SLOT_MAX); act=nbTS; if(sel>=n) sel=n?n-1:0; prev=keyNow(); dirty=2; mmPick();
        }
        if(pr&K_SEL){
            int st=menu("A NEW NEIGHBORHOOD",nsNm,NS_N);
            if(st>=0){ char nm[SLOT_NAME+1]; int i=0; for(;nsTown[st][i];i++) nm[i]=nsTown[st][i]; nm[i]=0;
                if(slEditName(nm)){ nbGen(st,nm); nbTS=-1; int e=nbSave(); toast(e?"NO FREE SLOT FOR A TOWN":"NEIGHBORHOOD MADE"); }
                nbOk=nbLoad(); n=nbTownList(l,SLOT_MAX); act=nbTS; }
            prev=keyNow(); dirty=2;
        }
        if((pr&K_START)&&n&&nbRead(l[sel],&nbTmp)){
            static const char* const it[2]={"RENAME","DELETE"};
            int c=menu(nbTmp.name,it,2);
            if(c==0&&nbRead(l[sel],&nbT)){ char nm[SLOT_NAME+1]; for(int i=0;i<=NB_NAME;i++) nm[i]=nbT.name[i];
                if(slEditName(nm)){ for(int i=0;i<=NB_NAME;i++) nbT.name[i]=nm[i]; nbTS=l[sel]; nbSave(); } nbOk=nbLoad(); }
            else if(c==1){
                if(l[sel]==act) toast("YOU LIVE THERE");
                else { const char*yn[2]={"NO","YES"};
                    if(menu("DELETE IT AND ITS HOUSES",yn,2)==1&&nbRead(l[sel],&nbTmp)){
                        for(int i=0;i<NB_LOTS;i++) if(nbTmp.lot[i].on&&nbTmp.lot[i].slot>=0) slDelete(nbTmp.lot[i].slot);
                        slDelete(l[sel]); toast("NEIGHBORHOOD DELETED"); n=nbTownList(l,SLOT_MAX); if(sel>=n) sel=n-1; if(sel<0) sel=0; } } }
            nbOk=nbLoad(); act=nbTS; prev=keyNow(); dirty=2;
        }
        if(dirty){ if(dirty&2) mmBackdrop(); plDraw(l,n,sel,act,foc,tile,(dirty&6)!=0); present(); dirty=0; } else vsync();   // (the acid rainbow holds still here: the panel is too much to draw every frame)
        uiTicks++; menuMusTick();
    }
    nbOk=nbLoad(); nbBounds();
}

#include "savegame.h"    // PLAYERS: a save file per player, picked on the PLAY screen
static void mainMenu(void){
    int sel=0, dirty=3; u16 prev=keyNow();
    menuMusStart();   // a random checked song plays while a main menu is open (MENU MUSIC option)
    acidInit(); mmPick();
    for(;;){
        u16 k=keyNow(), pr=k&~prev; prev=k;
        if(pr&K_DOWN){ sel=(sel+1)%MM_N; dirty|=1; }
        if(pr&K_UP){ sel=(sel+MM_N-1)%MM_N; dirty|=1; }
        if(pr&(K_A|K_START)){
            if(sel==0) playerScreen();
            else if(sel==1) creatureEditor();
            else if(sel==2) mapEditor();   // (the menu song plays on in the room builder)
            else if(sel==3) jukeboxScreen();
            else if(sel==4){ slotScreen(); if(nbOk) nbBoot(); }   // (a slot screen can delete or replace the town)
            else if(sel==5) settingsScreen();
            else howToPlay();
            gToMenu=0; prev=keyNow(); dirty=2; if(sel<=1||sel==4||sel==5) mmPick();   // back from a game (or OPTIONS): a new view and the time of day again
            menuMusSync();   // the menu's song comes back (a crossfade) if the screen took the music; OPTIONS may have switched SOUND or MENU MUSIC
            continue;
        }
        if(dirty){ drawMainMenu(sel,dirty&2); present(); dirty=0; }
        else if(mmAcid){   // the acid rainbow moves: only the plasma around the panel is worked out again and copied (the panel stays put on screen)
            acT+=2; acidRect(acT,0,60,0,10); acidRect(acT,0,15,10,74); acidRect(acT,45,60,10,74); acidRect(acT,0,60,74,75); mmLogo(SW/2-LOGO_SW/2,1);
            vsync(); vramCopy(0,0,SW,20); vramCopy(0,20,60,148); vramCopy(180,20,SW,148); vramCopy(0,148,SW,150); }
        else vsync();
        uiTicks++; menuMusTick();
    }
}

#ifdef CS_TEST
u8* csTestP;
#endif
#include "chipguard.h"   // the save chip guard (warns when the save chip got smaller)
// THE STACK LIVES IN EWRAM. IWRAM (32 KB) is almost all code + .bss (about 31 KB), which left the stack only ~1.5 KB (a different compiler version
// made it ~0.7 KB) while PLAY alone needs ~2.6 KB: the stack then ran over the globals at the top of .bss (slN got a saved pointer, a loop ran
// 50 million times, START on the title froze). main() is only this stub: it points sp at the top 8 KB of EWRAM (8-byte aligned) and enters boreMain.
// The Makefile fails the build if EWRAM statics ever grow into those 8 KB (STACK_EWRAM). Interrupts use their own stack (irqStack, IWRAM), the BIOS its own.
int boreMain(void);
__attribute__((naked, used)) int main(void){
    __asm__ volatile("ldr r0,=0x02040000\n\tmov sp,r0\n\tldr r1,=boreMain\n\tbx r1\n\t.ltorg\n");
}
int boreMain(void){
#ifdef CS_TEST
    { csTestP=(u8*)fb; csInit(chipsyn+chips[CS_TEST].off); static s8 l[MUS_N],r[MUS_N];
      REG_WAITCNT=0x4317; volatile u16*t=(volatile u16*)0x04000108; t[1]=0; t[3]=0; t[0]=0; t[2]=0; t[3]=0x84; t[1]=0x80;   // timer 2 + 3 cascaded: cycles
      for(int f=0;f<100;f++) chipMix(l,r);
      u32 cyc=t[0]|((u32)t[2]<<16); ((u32*)fb)[32000/4]=cyc; for(;;); }   // test build (-DCS_TEST=n): 100 frames of loop n at fb, then the cycles they took
#endif
    REG_WAITCNT=0x4317;  // ROM 3/1 waits + prefetch (power-on default is 4/2, no prefetch)
    *(volatile unsigned int*)0x04000800=0x0E000020;   // EWRAM 1 wait state (faster fb, sprites, stack)
    logo_play();         // the DippInn Productions boot logo (source/logo.c, ~8 s; leaves a black screen, its DMA and sprites off)
    { volatile u16*io=(volatile u16*)0x04000000; for(int r=0x08/2;r<0x20/2;r++) io[r]=0; for(int r=0x40/2;r<0x56/2;r++) io[r]=0; }   // undo its BG control, scroll, windows and blend (BG2's affine registers are left alone: mode 3 needs them)
    { static volatile u32 zero; zero=0; REG_DMA3SAD=(u32)(uintptr_t)&zero; REG_DMA3DAD=VRAM_ADDR; REG_DMA3CNT=(SW*SH/2)|0x85000000u; }   // (the zero must sit in RAM: a DMA from cartridge ROM always steps its source, "fixed" or not, and used to paint ROM data on screen)   // clear its tiles out of the bitmap (else mode 3 shows them as noise until the title is drawn)
    REG_DISPCNT=0x0403;  // mode 3, BG2 on
    initTables(); setColors(); svInit(); slInitN(); slMigrate(); bkInit(); chipGuard(); settingsLoad(); optsLoad(); applyRom();   // slMigrate: carries a layout 1 save over to layout 2 first (slots.h)
    lrng^=(u32)titleScreen()*2654435761u;   // time spent on the title seeds the random numbers (first shuffle)
    if(konMsg) toast(konMsg==2?"DEBUG UNLOCKED":"DEBUG LOCKED");
    jbSetup();                              // load the saved shuffled order (or make a new one), placeholders hidden
    starter();
    mapReset(); mapLoad();   // default room, or the one saved to SRAM
    itemSpanInit();          // item sprite spans (the room builder draws items too, so this cannot wait for the first PLAY)
    nbBoot();                // the town, if one was made (the room builder keeps to the lot you are on)
    slotBoot();              // BOOT LOADS PERSON option: the creature of the active room slot
    ageLoad();               // ...grown to the stage it had reached
    persLoad();              // ...with its aspiration, personality, DNA and unlocked parts
    hhLoad();                // ...and the household, which holds your first and last name
    mainMenu();
    return 0;
}

