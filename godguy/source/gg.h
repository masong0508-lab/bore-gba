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
#if defined(GG_BIG)||defined(__3DS__)
#define GG_BIGMAP 1                // 3DS and PC: planets up to 1024 x 1024 and 100,000 creatures (the DS's 4 MB of RAM stops at 256 x 256)
#define MAXD 1024
typedef unsigned short coord_t; typedef unsigned int occ_t;
#else
#define MAXD 256                  // the biggest planet: 256 x 256 tiles
typedef unsigned char coord_t; typedef unsigned short occ_t;
#endif
extern int gW,gH;               // the planet in play: 128, 192 or 256 tiles a side
#define WW gW
#define WH gH
#ifdef NDSHW
#define MAXC 22000                // the DS's 4 MB of RAM: the creature list, the save buffers and the picture all have to fit
#elif defined(GG_BIGMAP)
#define MAXC 100000
#else
#define MAXC 30000
#endif
                // one creature to a tile, so this is also about the most a 256 x 256 planet can hold on its land
#define SHAPE_OLD 20             // hand-drawn two-frame shapes
#define SHAPE_N 42               // plus the one-frame shapes drawn below (their second frame is made by swapping the feet) and the crown, rain and snow sprites
#define ARCH_A 32                 // archetypes below this are people and animals
#define ARCH_N 52                 // 32..51 are monsters
#define SP_N 720
enum { T_DEEP, T_WATER, T_SAND, T_GRASS, T_FOREST, T_HILL, T_MOUNT, T_SNOW, T_LAVA, T_DESERT, T_SWAMP, T_JUNGLE, T_DIRT, T_N };
typedef struct { u16 sp, wep, arm; coord_t x, y; s8 ox, oy; u8 hp, food, age, t, dir, alive, tr, tr2, rk; } Cr;   // a creature (tr, tr2: its subspecies traits; rk: 0 normal, 1 elite, 2 boss): its kind, its tile and the offset it glides in from, health, hunger, age; wep and arm: the item (+1) it carries
typedef struct { u8 a, fam; } Sp;                                                     // a kind of creature: its archetype (shape and habits) and its colour family
extern u8 world[MAXD][MAXD];
u32 rnd(void);
#endif
