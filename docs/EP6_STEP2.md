# EP6 part two (Phase 2: kingdom data and saves), bit a: the realm and its bytes

`source/kingdom.h` is the REALM as data and how it is packed for a save slot. Pure functions: no screen, no save memory, no EWRAM.
Nothing in the game calls them yet (every function is `static inline`, so they cost no ROM until something does).
`tools/test_kingdom.c` tests it on a PC:

    gcc -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -o /tmp/tk tools/test_kingdom.c && /tmp/tk

## What is saved
The terrain is made again from the seed (kgCell), so a save holds only: the header (seed, size, giant, castle stage, day, random state, 8 goods),
the buildings (7 bytes each; workers are a count), and the cells the player changed, as runs (gap, length, value). Hauling jobs are not saved.
Sizes measured on random but plausible realms (a slot is 2 KB; a save also has a 32 B header and 4 B of chunk):
hamlet 172 B to 1.2 KB (1 slot); realm 239 B to 3.7 KB (1 to 2 slots); kingdom 982 B to 7.7 KB (1 to 4 slots). Every cell changed: refused as too big.
So the 8 slot limit of the roadmap is generous: 4 would do for now.

## Rules that must never be broken
- kgCell (the made land) must give the same land for the same seed for ever. A change is a new genVer with the old one kept. The test holds fingerprints.
- A load checks first (apply=0), then loads (apply=1): a bad save never half-replaces a realm in play.

## Next bits of part two
b) the slot kind (SLK_KING) in slots.h: list, protect, delete, save and load a realm like the household block  c) the KINGDOM screen listing realms
d) found a kingdom: giant, size, name  e) rename, quota, the power-cut test, and a bigger done-bits block for the ten missions (see EP6_STEP1.md)

---
# Bit b: the slot kind (SLK_KING) and the realm save layer

`source/kingslot.h` puts a realm in the save slots; `slots.h` learned the new kind (`SLK_KING = 5`). Nothing calls it yet (the KINGDOM screen is bit c).

## What changed in slots.h (small, additive)
- `SLK_KING` is a known kind (`slInfo` used to reject anything above `SLK_PLAYER`: an old base would see a realm as an empty slot and could overwrite it, risk 18).
- `slLocked`: a realm is shown in the BLUEPRINTS list as A KINGDOM / KEPT BY THE KINGDOM and can never be picked, saved over or deleted from there (like players and the town).
- The labels in the list, the slot card and the info lines know the kind.

## kingslot.h
- Slot payload: `'K'`, length (u16), the `kgEncode` bytes, `0`. Header byte 28 is the REALM'S NUMBER (1 to 250); the slot name is the realm's name.
- `kgsSave(id, name, R, B, cells, buf, cap, &idOut)`: id 0 makes a new realm. Writes a FRESH copy into free slots, reads it back, and only then drops the old copy. No save over itself: no free run big enough gives `SLE_NOROOM` and touches nothing (bit e reserves room so a realm can always be saved). At most 4 slots.
- `kgsLoad(slot, R, B, cells, tmp, cap)`: checksum, then `kgDecode` with apply 0, then apply 1: a bad copy never half-replaces a realm in play.
- `kgsList`, `kgsFind` (newest good copy, wrap-safe), `kgsHeal` (drops an older copy left by a power cut), `kgsPeek` (tier, giant, castle, day without loading), `kgsDelete`, `kgsNewId`.
- The caller owns the scratch buffers (the realm's map lives in a shared scratch, roadmap 5.1): kingslot.h adds no static RAM.

## Tests (PC): `sh tools/test_kingslot.sh`
Runs the REAL header, scan and verify code cut out of slots.h on a fake flash chip that can lose power at any write or erase (an erase cut half way leaves garbage).
- A cut at every write of a first save: 1016 of 1017 left nothing, 1 left the whole realm, never a damaged one.
- A cut at every write over an existing realm (and sampled over a 4 slot realm): the old realm or the new, whole; the other realm untouched; both copies whole only for one moment, and `kgsHeal` fixes that; the realm can be saved again after any cut.
- A full flash refuses the save and keeps the old copy; damaged copies are refused and change nothing; a room slot is never read as a realm.
Also run `tools/test_kingdom.c` (bit a) as before.

## Not tested
Nothing here was built with devkitARM or run on a cart or in an emulator. Host parse baseline unchanged: 14 errors (all `target("arm")`), 131 warnings.

## Next: bit c (the KINGDOM screen listing realms), d (found a kingdom), e (rename, quota and reserve, power-cut test on the real flash code, bigger done-bits block).
