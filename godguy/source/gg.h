#ifndef GG_H
#define GG_H
#ifdef PCTEST
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16;
#define iprintf printf
#define siprintf sprintf
static uint16_t pcBgPal[512], pcSprPal[512];      // (the palettes gfx2.h writes: stand-ins so the PC tests compile)
#define BG_PALETTE pcBgPal
#define SPRITE_PALETTE pcSprPal
#define RGB15(r,g,b) ((r)|((g)<<5)|((b)<<10))
#elif defined(NDSSHIM)
#include "ndsshim.h"
#elif defined(NDSHW)
#include "ndshw.h"
#else
#include <nds.h>
#endif
#if defined(GG_BIG)||defined(__3DS__)
#define GG_BIGMAP 1                // 3DS and PC: planets up to 1024 x 1024 and 100,000 creatures (the DS's 4 MB of RAM stops at 256 x 256)
#define MAXD 1024
typedef unsigned short coord_t; typedef unsigned int occ_t;
#else
#define MAXD 256                  // the biggest planet on a DS: 256 x 256 tiles
typedef unsigned char coord_t; typedef unsigned short occ_t;
#endif
extern int gW,gH;               // the planet in play: 128, 192 or 256 tiles a side (up to 1024 on the big builds)
#define WW gW
#define WH gH
#if defined(NDSHW)
#define MAXC 16000                // the DS's 4 MB of RAM: the creature list (20 bytes each), the save buffer and the picture all have to fit (arm9.ld stops the link if they do not)
#elif defined(GG_BIGMAP)
#define MAXC 100000
#else
#define MAXC 30000                // one creature to a tile, so this is also about the most a 256 x 256 planet can hold on its land
#endif
#ifdef GG_VILN
#define VIL_N GG_VILN              // (tests: pretend to be a build with another village count)
#elif defined(GG_BIGMAP)
#define VIL_N 200                  // villages (and cities) at once: a 1024 planet has room for hundreds
#else
#define VIL_N 64
#endif
#define VIL_OLDN 40                // saves made before the header said how many villages it holds
#define FAV_N (VIL_N>48?VIL_N:48)
#define SHAPE_OLD 20             // hand-drawn two-frame shapes
#define SHAPE_N 45               // plus the one-frame shapes drawn below (their second frame is made by swapping the feet) and the crown, rain and snow sprites, then the tornado (two pieces) and the black hole
#define ARCH_A 32                 // archetypes below this are people and animals
#define ARCH_N 52                 // 32..51 are monsters
#define SP_N 720
enum { T_DEEP, T_WATER, T_SAND, T_GRASS, T_FOREST, T_HILL, T_MOUNT, T_SNOW, T_LAVA, T_DESERT, T_SWAMP, T_JUNGLE, T_DIRT, T_N };
typedef struct { u16 sp, wep, arm; coord_t x, y; s8 ox, oy; u8 hp, food, age, t, dir, alive, tr, tr2, rk, sick; } Cr;   // a creature (tr, tr2: its subspecies traits; rk: 0 normal, 1 elite, 2 boss; sick: 0 healthy, 1 plagued, 2 recovered and immune. It sits in what was padding, so the struct is still 20 bytes and old saves load as healthy): its kind, its tile and the offset it glides in from, health, hunger, age; wep and arm: the item (+1) it carries
typedef struct { u8 a, fam; } Sp;                                                     // a kind of creature: its archetype (shape and habits) and its colour family
extern u8 world[MAXD][MAXD];
u32 rnd(void);
#endif
