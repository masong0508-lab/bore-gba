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

---
# Bit c: the KINGDOM screen

`source/kingscreen.h`: PACKS shelf > THE LAND BOREFORE TIME now has a first row, KINGDOM MODE (PLAY), like the other packs' modes. It opens the KINGDOM screen:
- the realms in the save slots, one row each (name, castle stage), and a first row FOUND A NEW KINGDOM (founding is bit d: it only says so for now);
- under the list, what the cursor is on: name, size, giant, day, castle stage, times saved, bytes and slots used (read with `kgsPeek`, no realm is loaded);
- A on a realm: DELETE THIS KINGDOM, then a second YES, then `kgsDelete` (every copy). Leftover older copies from a power cut are dropped when the screen opens (`kgsHeal`);
- a cart with fewer than 26 slots (SRAM only) shows NEEDS A SAVE CHIP WITH MORE ROOM and does not open (roadmap 5.4).
Changes: `XMODE_KING` in expand.h (the pack's mode row and "KINGDOM MODE"), `exModeRun` in xmodes.h, one include in main.c. The code is in the base ROM for now; it moves into the pack's slot when there is something to play. Giant names are placeholders (GIANT A to D, YOUR BORE).

Not tested: this is a screen, so it was only parsed on the PC (host baseline unchanged: 14 errors, 131 warnings). It needs a devkitARM build and a look in Pizza Boy: the shelf with the new MODE badge, the list with 0, 1 and several realms (none can be made yet except by a test build), delete, and B to go back. The save layer under it is the part tested by `tools/test_kingslot.sh`.

Next: bit d, found a kingdom (giant, size, name; saves it with `kgsSave`), then bit e.

---
# Bit d: found a kingdom

`kingscreen.h` `ksFound`: A on the first row of the KINGDOM screen asks for the giant (GIANT A to D or YOUR OWN BORE), the size (HAMLET 48, REALM 96, KINGDOM 128) and a name (the slot name editor, 10 letters, default REALM n), makes a seed (the menu time stirs `lrng`), and saves the new realm through `kgsSave`. Then the cursor lands on it.
- A new realm has no buildings and no edits: `kgEncode` takes `cells = 0` for "the made land, untouched" and the save is 40 bytes (one slot), so founding needs only a 64 byte scratch on the stack and no map in RAM (`tools/test_kingdom.c` check 7 holds that `0` gives the same bytes as the full map; `tools/test_kingslot.sh` check 10 saves and loads one in every size).
- It starts with 10 of each raw good (`KS_START`, [ASSUMED], tunable). The first settlement comes with the simulation (Phase 3).
- Refused cleanly: more than 16 realms (the list's size) and no free slot (NOT ENOUGH FREE SLOTS). The proper quota and the reserve of room for players and towns are bit e.
- The giant's names are still placeholders; there is no stat screen yet.
Not run on a cart: the menus and name editor are the existing ones, but the flow needs an emulator look (giant, size, name, the toast, the new row, then delete it again).

---
# Bit e (first half): rename, the quota and the reserve

`kingslot.h` `kgsRenameI` / `kgsRename`: renames a realm the same safe way as a save (a fresh copy into free slots, read back, and only then the old copy is dropped), but it copies the payload bytes as they stand, so it needs no map in RAM and decodes nothing. Only the header's name and seq change. It refuses a damaged realm (it is not listed, so it reads as "not there") and says `SLE_NOROOM` and touches nothing when no free run is big enough.
`kingscreen.h`: A on a realm now opens RENAME / DELETE / LEAVE IT (delete is still asked twice). After a rename the cursor follows the realm to its new slot.
- Quota, all [ASSUMED] and tunable: at most 4 realms on a 128 KB flash (`SLOT_N` >= 58), 2 on a 64 KB flash (`ksQuota`). The small cart only offers HAMLET in the size menu (`ksTiers`). The SRAM-only cart still has no KINGDOM MODE (`KS_MINSLOTS`).
- Reserve: founding needs `KS_KEEP + 1` free slots: `KS_RESERVE` (8) stay free for players and towns, and `KGS_MAXSPAN` (4) more stay free so any realm can always be saved as a fresh copy. Refused with "NOT ENOUGH FREE SLOTS" or "KINGDOM LIMIT  DELETE ONE". The FOUND panel shows "n OF 4 KINGDOMS" and the free slots, and the reason when it is refused.
- Tests (`tools/test_kingslot.sh`, checks 11 and 12): rename keeps the bytes, id and count, bumps seq, keeps the old name for an empty one, is refused with no room or a damaged copy and touches nothing; a power cut at every write of a rename (every point for a realm, every point near the ends and a stride between for a 4 slot realm) always leaves the old name or the new, the realm whole, the other realm untouched, and it can be renamed again.
- Host parse baseline unchanged: 14 errors (all `target("arm")`), 131 warnings.

Not done in bit e yet (so Phase 2 is not finished):
- The bigger done-bits block. `EXD_OFF` (7482, 11 bytes) sits right before `BF_OFF` (7494) with one spare byte, so growing it means moving a block or finding free SRAM, which is a layout and save-compatibility decision. The compile-time check still stops EP6 at 4 missions.
- The power-cut test on the REAL flash code. The slot tests run the real `slots.h` header, scan and verify code on a fake chip; the real `svWr` / `svCommit` / `svErase` are still only read, not run.
- Not run on a cart: the rename flow and the FOUND panel text need an emulator look (rename, the new name in the list, a refused rename, founding at the quota).
