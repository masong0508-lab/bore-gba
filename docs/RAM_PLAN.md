# RAM plan: getting room for more features

Numbers come from the CI job summary (EWRAM 236,352 B of 262,144 = 90.2%, IWRAM 29,032 of 32,768 = 88.6%, ROM only 18.8% used)
plus a host-side size count of every `EWRAM_BSS` symbol (no ARM compiler needed: `gcc -S` on a copy, read the `.size` lines).
Pointer-holding structs count a little high on the host, so trust the CI table for the final number.

## What the EWRAM is made of (about 232 KB)
| symbol | bytes | what it is |
|---|---|---|
| fb | 76,800 | the 240x160 16-bit frame buffer (mode 3) |
| hhObj + hhObjS (now hhPool) | 53,760 -> 30,720 | baked 4bpp hardware sprites for 7 household members, only the tiles that hold a pixel |
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

3. **Household sprites live in a trimmed pool (about 32 KB of EWRAM saved).** `hhObj` + `hhObjS` (53,760 B of fixed 32-tile views) are gone.
   `hhPool[30720]` holds, per member, only the 8x8 tiles that contain a pixel: a 32-bit mask per view says which of the 32 tiles exist
   (`HhSpr.sm`), and the walking frame keeps only the tiles that differ from standing (`HhSpr.dm`, the legs). A member's block is
   `[standing views 0..3][walking tiles of views 0..3]`. `hhUpTiles` writes the tiles to OBJ VRAM (empty ones as zeros), so what is on screen is byte-identical;
   OAM, the blend bits and the window (the x-ray look) were not touched. Moving members (SELECT switch, move out) only moves the 36-byte descriptors;
   `hhSprFree` keeps the pool packed. This also removed the 7.7 KB `ob/obs` scratch of `hhSwitchFrom` (the player goes into the spare descriptor `hhSp[HH_MAX]`),
   and the 2.3 KB `col/oc/cnt/ob` statics of `hhQuant` now live on `hhDist` (path-finding scratch, always refilled before a search; `HhQs`).
   If the pool is ever full, a view that does not fit is left empty (no crash): raise `SP_POOL` in house.h if you see a Sim with a missing view.
   Host test: 4,000 random bake / rebake / move-out / switch steps compared byte for byte against the old layout, peak use 26,976 of 30,720 B.
4. **IWRAM:** three cold menu buffers (`rb`, `lb`, the interaction menu `it`) moved to EWRAM (about 450 B). IWRAM is almost all hot code (mixers, blit, cube / wall drawing) and the
   audio mixer's buffers, so it is not touched further without a speed cost.

## Next, in order of payoff (not done yet: each needs testing on an emulator)
| # | idea | saves | risk |
|---|---|---|---|
| 1 | ~~Shrink hhObj/hhObjS with transparent-row trimming.~~ done (tile level, see above). The sprite is 32x64 but the bake is only 60 high and the creature is narrow: store each view as only its used tile rows (plus first/last row numbers) in one shared pool, and copy only those rows into OBJ VRAM (hhObjUpdate already copies per slot, in vblank). Typical Sims use about 60% of the box | about 15-20 KB | medium: touches hhQuant, hhUnquant, hhObjUpdate, the switch code |
| 2 | ~~Stride frame as a delta.~~ done. hhObjS is 25 KB; only the legs/bob rows differ from standing. Store just the changed tile rows | about 10 KB | medium, same code as 1 |
| 3 | **One shared scratch pool (the rest of it)** for buffers that are never live together: `udR` (builder only, cleared on entry), `nbTmp` (`ob/obs` and the hhQuant tables are done). Each user needs a check that nothing else running at the same time uses the pool | 10-12 KB | medium: aliasing bugs show up only at run time, so add an `assert`-style owner byte per user |
| 4 | **Drop `zoomBuf`** and point the zoom DMA straight at `zoomTab` in ROM (it is only read). ROM waits are 3/1 in the FAST mode, about the same as EWRAM, but ROM SPEED = SAFE is 4/2 without prefetch, so test the zoom in both | 2.6 KB | low-medium |
| 5 | **wpTab pre-shaded at build time** (3.5 KB + 0.9 KB IWRAM): bake in a PC-side tool like bake_items | about 4 KB | low, but needs a new generator |
| 6 | ~~Player sprite to 8bpp + palette~~ done (see above). Going further: the player as a hardware sprite like the Sims (walls in front need the x-ray trick, the zoom does not scale OBJ) | 15 KB more | high |
| 7 | **fb is the elephant** (76.8 KB). Mode 4 (8bpp, 2 pages) would halve it, but needs a 256-colour palette; skip unless the art is redone | 38 KB | very high |

Since ROM is 81% free (27 MB), anything that can be computed at build time or kept as a table should go to ROM first
(items 4, 5 and the earlier itemSpan). Anything that is only needed in one screen (builder, save menu, chooser) goes in the shared pool (3).

## Budget rule of thumb
Feature work that needs a new array: ask "can this live in ROM?", then "is it only used in one mode (pool)?", and only then `EWRAM_BSS`.
