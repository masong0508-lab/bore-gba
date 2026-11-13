#ifndef GG_H
#define GG_H
#ifdef PCTEST
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16;
#define iprintf printf
#define siprintf sprintf
#elif defined(NDSSHIM)
#include "ndsshim.h"
#elif defined(NDSHW)
#include "ndshw.h"
#else
#include <nds.h>
#endif
#define MAXD 256                  // the biggest planet: 256 x 256 tiles
extern int gW,gH;               // the planet in play: 128, 192 or 256 tiles a side
#define WW gW
#define WH gH
#define MAXC 30000                // one creature to a tile, so this is also about the most a 256 x 256 planet can hold on its land
#define SHAPE_OLD 20             // hand-drawn two-frame shapes
#define SHAPE_N 42               // plus the one-frame shapes drawn below (their second frame is made by swapping the feet) and the crown, rain and snow sprites
#define ARCH_A 32                 // archetypes below this are people and animals
#define ARCH_N 52                 // 32..51 are monsters
#define SP_N 720
enum { T_DEEP, T_WATER, T_SAND, T_GRASS, T_FOREST, T_HILL, T_MOUNT, T_SNOW, T_LAVA, T_DESERT, T_SWAMP, T_JUNGLE, T_DIRT, T_N };
typedef struct { u16 sp, wep, arm; u8 x, y; s8 ox, oy; u8 hp, food, age, t, dir, alive, tr, tr2, rk; } Cr;   // a creature (tr, tr2: its subspecies traits; rk: 0 normal, 1 elite, 2 boss): its kind, its tile and the offset it glides in from, health, hunger, age; wep and arm: the item (+1) it carries
typedef struct { u8 a, fam; } Sp;                                                     // a kind of creature: its archetype (shape and habits) and its colour family
extern u8 world[MAXD][MAXD];
u32 rnd(void);
#endif
