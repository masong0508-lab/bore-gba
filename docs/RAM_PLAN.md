# RAM plan: getting room for more features

Numbers come from the CI job summary (EWRAM 236,352 B of 262,144 = 90.2%, IWRAM 29,032 of 32,768 = 88.6%, ROM only 18.8% used)
plus a host-side size count of every `EWRAM_BSS` symbol (no ARM compiler needed: `gcc -S` on a copy, read the `.size` lines).
Pointer-holding structs count a little high on the host, so trust the CI table for the final number.

## What the EWRAM is made of (about 232 KB)
| symbol | bytes | what it is |
|---|---|---|
| fb | 76,800 | the 240x160 16-bit frame buffer (mode 3) |
| hhObj + hhObjS | 53,760 | baked 4bpp hardware sprites for 7 household members (4 views + stride frames) |
| spr4 + spr4s | 30,720 | the player's 16-bit baked sprites (CPU drawn) |
| flPool | 8,192 | packed floors of the house |
| ob / obs (in hhSwitchFrom) | 7,680 | scratch for switching Sims |
| ovBuf | 5,200 | pixels under an overlay |
| svLow | 4,096 | flash save: the small-blocks sector |
| udR | 4,000 | room builder undo history (builder only) |
| wpTab | 3,584 | pre-shaded wallpapers |
| hhDist, bfsQ | 6,400 | path-finding |
| zoomBuf | 2,576 | copy of one zoom table from ROM |
| everything else | about 25,000 | maps, towns (nbT / nbTmp 1 KB each), save slots, ... |

## Done in this pass
1. **itemSpan (4,144 B) moved to ROM.** It was filled at boot from `romSpr`. `tools/bake_items.c` now writes it into `itemrom.h`
   as a `const` table, `itemSpanInit()` is an empty stub. New items: run `tools/bake_items.sh` as before, the table follows.
   Measured: EWRAM statics 232,132 -> 227,988 B. `romSpr` is byte-identical to before.
2. **Player sprites are 8-bit palette indices (15,356 B of EWRAM saved).** `spr4` / `spr4s` are now views into one `u8 sprBuf[8][SPW*SPH]`;
   `sprPal[256]` (512 B, IWRAM) is the player's palette, built while baking (`sprIdx`), and `blit` does `*d = sprPal[c]` for every opaque pixel.
   When you switch to a household member, their 15 colours become the palette (`hhUnquant`). Past 255 colours a new colour snaps to the nearest one.
   The title screen still borrows the same buffer (`tfb`). The hash helpers (`hqKey` ...) moved from house.h to main.c.

## Next, in order of payoff (not done yet: each needs testing on an emulator)
| # | idea | saves | risk |
|---|---|---|---|
| 1 | **Shrink hhObj/hhObjS with transparent-row trimming.** The sprite is 32x64 but the bake is only 60 high and the creature is narrow: store each view as only its used tile rows (plus first/last row numbers) in one shared pool, and copy only those rows into OBJ VRAM (hhObjUpdate already copies per slot, in vblank). Typical Sims use about 60% of the box | about 15-20 KB | medium: touches hhQuant, hhUnquant, hhObjUpdate, the switch code |
| 2 | **Stride frame as a delta.** hhObjS is 25 KB; only the legs/bob rows differ from standing. Store just the changed tile rows | about 10 KB | medium, same code as 1 |
| 3 | **One shared scratch pool** (`static u8 pool[8192]` + `#define` views) for buffers that are never live together: hhSwitchFrom `ob/obs` (7.7 KB), hhQuant `col/oc/cnt/ob` (2.3 KB), `udR` (builder only, cleared on entry), `nbTmp`. Each user needs a check that nothing else running at the same time uses the pool | 10-12 KB | medium: aliasing bugs show up only at run time, so add an `assert`-style owner byte per user |
| 4 | **Drop `zoomBuf`** and point the zoom DMA straight at `zoomTab` in ROM (it is only read). ROM waits are 3/1 in the FAST mode, about the same as EWRAM, but ROM SPEED = SAFE is 4/2 without prefetch, so test the zoom in both | 2.6 KB | low-medium |
| 5 | **wpTab pre-shaded at build time** (3.5 KB + 0.9 KB IWRAM): bake in a PC-side tool like bake_items | about 4 KB | low, but needs a new generator |
| 6 | ~~Player sprite to 8bpp + palette~~ done (see above). Going further: the player as a hardware sprite like the Sims (walls in front need the x-ray trick, the zoom does not scale OBJ) | 15 KB more | high |
| 7 | **fb is the elephant** (76.8 KB). Mode 4 (8bpp, 2 pages) would halve it, but needs a 256-colour palette; skip unless the art is redone | 38 KB | very high |

Since ROM is 81% free (27 MB), anything that can be computed at build time or kept as a table should go to ROM first
(items 4, 5 and the earlier itemSpan). Anything that is only needed in one screen (builder, save menu, chooser) goes in the shared pool (3).

## Budget rule of thumb
Feature work that needs a new array: ask "can this live in ROM?", then "is it only used in one mode (pool)?", and only then `EWRAM_BSS`.
