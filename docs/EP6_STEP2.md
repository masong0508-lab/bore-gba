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
