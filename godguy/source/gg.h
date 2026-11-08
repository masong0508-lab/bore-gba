#ifndef GG_H
#define GG_H
#ifdef PCTEST
#include <stdint.h>
#include <stdio.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t s8; typedef int16_t s16;
#define iprintf printf
#define siprintf sprintf
#else
#include <nds.h>
#endif
#define WW 128
#define WH 128
#define MAXC 1500
#define SHAPE_N 16
#define ARCH_N 32
#define SP_N 400
enum { T_DEEP, T_WATER, T_SAND, T_GRASS, T_FOREST, T_HILL, T_MOUNT, T_SNOW, T_LAVA, T_DESERT, T_SWAMP, T_JUNGLE, T_DIRT, T_N };
typedef struct { u16 sp; u8 x, y; s8 ox, oy; u8 hp, food, age, t, dir, alive; } Cr;   // a creature: its kind, its tile and the offset it glides in from, health, hunger, age
typedef struct { u8 a, fam; } Sp;                                                     // a kind of creature: its archetype (shape and habits) and its colour family
extern u8 world[WH][WW];
u32 rnd(void);
#endif
