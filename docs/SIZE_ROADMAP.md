# SIZE ROADMAP: status on 25 Oct 2026 (read this first; the roadmap below is the original, written at commit 95ee19b)

Measured on a real devkitARM build of the tree (`tools/budget.sh` prints the first three lines after every link; the CI job summary has the "Biggest users" step):

| budget | limit | now | free |
|---|---|---|---|
| EWRAM statics | 253,952 B (262,144 minus the 8 KB stack) | 232,212 B (91.4%) | 21,740 B |
| IWRAM | 32,512 B | 29,160 B (89.7%; the overlays add 6,872) | 3,352 B |
| ROM, base game (before the first pack slot at 0x09800000) | 25,165,824 B | 20,749,240 B (82.5%) | **4,416,584 B** (was 3.3 MB before the item sprites were packed) |
| bore.gba / bore-base.gba | 33,554,432 B | 28,704,768 B | the image is padded to the END OF THE LAST SLOT (0x09B60000), so it does not shrink when base data shrinks (see "the padding" below) |
| packs (slot use) | | EP1 13 of 256 KB, EP2 29 of 384, EP3 6 of 256, EP4 1,045 of 1,280, EP7 15 of 256, EP8 13 of 256, EP6 59 of 768 | |

## What is done, what is not (checked against the tree, not copied from the text below)
| item | status |
|---|---|
| 0. CI "Biggest users" step | done (`.github/workflows/build.yml`), and every link now prints `BUDGET` lines (`tools/budget.sh`, called by the Makefile): a NOTE above 92% (EWRAM, IWRAM) or 95% (ROM), an ERROR if the base game runs into the packs' slots |
| 1.1 slots to the top of the ROM | **not done, on purpose**: it would make the file 32 MB (it is 28.7 MB now) and the cartridge limit is what you are watching. The base game has 4.4 MB of room under the slots as they are. If the base ever needs more, change `SLOTS` in `tools/make_packs.py` |
| 1.2 placeholder songs | done earlier (173 KB), `songs.h` says so |
| 1.3 sample dedupe | done: `xm2gba.py` stores identical samples once for the whole ROM (`SHARED`), near-duplicates for one song (`NEARDUP`) |
| 1.4 `here_today_old` (664 KB) | a design call, still open |
| 2. `itemrom.h` | **done**: 1,252,860 B of sprites -> 328,762 B + 3,140 B of offsets. Only the opaque span of each row is stored (`itemPix`, `itemOff`; `itemSpan` as before), `blitItemO` (items.h) walks it. **Saves 921,560 B of ROM**; screenshots of three furnished houses, normal and zoomed, are byte-identical to the old ROM's. The generator proves every pixel outside a span is see-through (`tools/bake_items.c`) |
| 2. `here_today_gone_tomorrow` (6.5 MB), `here_today_full` (2.5), `here_today` (1.3) | open, and it is the big one (10.5 MB of the 19.8 MB base). Lower rates or bit depths change the sound, which needs your ears; a tracker rebuild is a larger job |
| 3.1 `zoomBuf` | **not possible as written**: the zoom's per-line table is read by DMA0, and DMA0 cannot read the cartridge (the note in main.c at zoomArm); DMA1 to DMA3 are the sound's or the picture's |
| 3.2 `wpTab` | already ROM (3,584 B, `r`) |
| 3.3 `wFq` | not possible: it packs four floors of 8 bits (NFL is 80) per tile in a u32 |
| 3.4 `svShad` + `svLow` | not possible: they are not scratch: `svLow` IS sector 1 of the flash save kept in RAM, `svShad` its read-back copy; both live all the time |
| 3.5 builder pool | `udR` and `bakeBuf` no longer exist as statics; what is left that only the builder uses is `ovBuf` (5.2 KB) |
| 3.6 `bfsQ` / `hhDist` | one symbol each (3,200 B), the host scan counted them twice |
| 3.7 `SP_POOL` | open, medium risk, 3 KB at best |
| 4. `xpPool` | not needed now: the pack modes' own buffers are small (a few hundred bytes each in `nm`), and the realm borrows `hhPool` |
| 5. IWRAM | `bakeBuf` and `itemTmp` are not in IWRAM; the biggest IWRAM statics are `macc` (1.2 KB) and the code |

## The padding
The base game ends at 20,749,240 B but the first pack slot starts at 25,165,824 B, so `bore.gba` carries 4.4 MB of 0xFF between them. That is the cheapest way to make the FILE smaller: lower the slots' start (for example to 0x09500000, 21 MB) and the image drops to about 25.3 MB, at the price of a base game ceiling of 21 MB (1.25 MB of room) that the build now watches (`tools/budget.sh`). It is a one-line change to `SLOTS` in `tools/make_packs.py`; I did not make it because the cartridge ceiling is not the problem yet and the room matters more for the BORTUA 3D mode.

---

# SIZE ROADMAP: room for new features and for finishing the expansion packs

Written 22 Oct 2026 from the repo at commit 95ee19b. **Nothing here was built or measured on an ARM toolchain** (none was available), so every size below is either a file size, a host-side scan of the `EWRAM_BSS` declarations (rough: structs are not counted), or a figure copied from `docs/RAM_PLAN.md`. Phase 0 replaces the guesses with real numbers.

## 1. Where you actually stand

| budget | limit | last measured (docs/RAM_PLAN.md, stale) | what the repo says today |
|---|---|---|---|
| EWRAM | 253,952 B (262,144 minus the 8 KB stack) | 236,352 B (90.2%), before the hhPool/sprBuf work was counted | unknown: run Phase 0 |
| IWRAM | 32,512 B | 29,032 B (88.6%) | unknown, about 3.5 KB free at best |
| ROM, base game | **24 MB** (the packs start at 0x09800000) | "18.8% used, 27 MB free" | **about 20.5 MB or more**, see below |
| ROM, packs | 4.25 MB in six fixed slots | n/a | slots are 512 KB, 1 MB, 512 KB, 1.25 MB, 512 KB, 512 KB, each with a linker ASSERT |

**The big correction is ROM.** `docs/RAM_PLAN.md` says ROM is 81% free. That was true before the songs went in. The files the ROM embeds now add up to **19.0 MB**, plus `itemrom.h` (785 sprites x 1,596 B = 1.25 MB) plus all the code. The base game has to fit under 24 MB because the pack slots sit at 24 MB. Real headroom is roughly 2.5 to 3.5 MB, not 27 MB. The cartridge ceiling is 32 MB, and the slots end at 28.25 MB, so the slots can move (Phase 1).

### Where the ROM goes (embedded files, bytes)
| file | size | what it is |
|---|---|---|
| music/here_today_gone_tomorrow.adp | 6,533,272 | 12 minute instrumental suite, streamed 4-bit ADPCM at 18,157 Hz |
| music/xmdata.bin | 6,041,914 | 36 tracker songs (events + samples). Top: over_my_voice 1.14 MB, magic_act 455 KB, nursery_time 365 KB, cocaine_cola_ii 333 KB, excuses_house 329 KB, earth_and_the_space_citizens 313 KB |
| music/here_today_full.adp | 2,570,960 | the full version, locked until the story is done |
| music/here_today.adp | 1,380,704 | Missy's song (CI checks this one is 150 s or longer) |
| art/bortua_fighters.lz | 962,792 | BORTUA FIGHTER art (pack 4's slot is only 1.25 MB) |
| music/here_today_old.adp | 664,100 | the secret chiptune placeholder |
| sfx/amb_*.abd | 1,042,752 | four ambience loops |
| music/chipsyn.bin | 179,573 | chiptunes |
| music/placeholder_a/b/c.adp | 173,208 | three placeholders; songs.h says to delete them |
| sfx voices (v_ x43) and x_ | about 239,000 | |

## 2. The roadmap, in order

### Phase 0: measure first (30 minutes, no risk)
1. Add this step to `.github/workflows/build.yml` after `make size`. It puts the real top users in every job summary, so you stop guessing:
```yaml
      - name: Biggest users
        run: |
          export PATH=/opt/devkitpro/devkitARM/bin:$PATH
          { echo "### Top EWRAM"; echo '```'; arm-none-eabi-nm -S --size-sort -r bore.elf | awk '$1 ~ /^02/' | head -30; echo '```'
            echo "### Top IWRAM"; echo '```'; arm-none-eabi-nm -S --size-sort -r bore.elf | awk '$1 ~ /^03/' | head -20; echo '```'
            echo "### Top ROM symbols"; echo '```'; arm-none-eabi-nm -S --size-sort -r bore.elf | awk '$1 ~ /^0[89]/' | head -30; echo '```'
            echo "bore-base.gba: $(stat -c %s bore-base.gba) B   bore.gba: $(stat -c %s bore.gba) B"; } >> "$GITHUB_STEP_SUMMARY"
```
2. Copy the numbers into a table at the top of `docs/RAM_PLAN.md` with the date and commit, and fix its "ROM 81% free" line. Every later phase records before/after there.
3. Read how `tools/make_packs.py` decides which functions go into a pack slot. The moonbase code (`lunrado.h`) is not in the pack-0 slot list, so it sits in the base ROM. Moving pack-only code into its slot frees base ROM and costs nothing at run time.

### Phase 1: ROM quick wins (low risk, biggest effect per hour)
| # | change | frees | notes |
|---|---|---|---|
| 1.1 | **Move the six pack slots to the top of the 32 MB ROM** (start 0x09BC0000 instead of 0x09800000; `tools/make_packs.py` generates `packslots.h` and `packs.ld`) | raises the base ceiling from 24 MB to about 27.75 MB: **+3.75 MB** | the ROM is padded to 32 MB; check your flash cart and emulator accept that. Pack patches are rebuilt on every build |
| 1.2 | Delete the three placeholder songs (`songs.h` lines 22 to 24 and the files) | 173 KB | the file already says to delete them |
| 1.3 | Dedupe samples in `xm2gba.py`: hash every sample and store each once across the 36 songs | unknown, maybe 5 to 15% of 6 MB | PC-side only, byte-identical playback |
| 1.4 | Decide about `here_today_old` (secret placeholder, 664 KB) | 664 KB | a design call, not a technical one |

### Phase 2: the two big ROM items (medium effort, pick what you need)
- **here_today_gone_tomorrow (6.5 MB, 32% of all embedded data).** Options, cheapest first:
  a. Re-encode at a lower sample rate (about 11 kHz saves about 40%; ADPCM at 18 kHz is already a lot for a 1970 band joke song). Check the sound by ear.
  b. 2-bit ADPCM or a smaller bitrate for this one song only.
  c. Rebuild it as tracker data through the existing `xm2gba.py` pipeline instead of a rendered stream. `tools/make_heretoday_instrumental.py` already generates it from notes. This is the real fix (6.5 MB down to a few hundred KB) but it needs a tracker version of the instruments.
- **itemrom.h (1.25 MB).** Almost every one of the 785 sprites is mostly `0x8000` (transparent). Store each sprite cropped to its bounding box (or row-run-length coded) and expand into the existing `bakeBuf`/`itemTmp` when it is drawn. Expect to save about two thirds. Medium risk: it touches the item draw path, so test the catalog, the builder and the shop.
- Do the same bounding-box idea for `petart.h` and `catalogart.h` only after Phase 0 shows they are large in ROM (their source files are big, but source size is not ROM size).

### Phase 3: EWRAM easy wins (each needs an emulator check)
From the host-side scan, EWRAM is led by `fb` 76,800, `hhPool` 30,720, `sprBuf` 15,360, `flPool` 8,192, `wFq` 6,400, `ovBuf` 5,200, `winColT` 4,608, `svShad` + `svLow` 8,192, then three 3,200 B path-finding tables.

| # | change | frees (estimate) | risk |
|---|---|---|---|
| 3.1 | `zoomBuf` dropped: point the zoom DMA at the table in ROM (RAM_PLAN item 4) | 2.6 KB | low, test both ROM SPEED settings |
| 3.2 | `wpTab` pre-shaded by a PC tool like `bake_items` (RAM_PLAN item 5) | about 4 KB (plus 0.9 KB IWRAM) | low |
| 3.3 | `wFq` is `u32 [MH][MW]` (6,400 B): if it holds tile indices it fits in `u16` | 3.2 KB | low, check what it stores |
| 3.4 | `svShad` + `svLow` (8 KB) are only needed while saving: alias them into a shared scratch that is never live at the same time | up to 8 KB | medium: aliasing bugs show up only at run time |
| 3.5 | Builder-only buffers (`udR` undo history, `ovBuf`, and any others Phase 0 shows) share one **builder pool** with an owner byte, cleared on entry | 10 to 14 KB | medium |
| 3.6 | `bfsQ`/`hhDist` appear several times in the scan; confirm with `nm` they are one symbol each, not duplicates | up to 6 KB | low |
| 3.7 | Trim `SP_POOL` (hhPool): the 4,000-step test peaked at 26,976 of 30,720 | about 3 KB | medium: a full pool leaves a view empty |

Phases 3.1 to 3.7 together are roughly **25 to 35 KB, about 10 to 13% of EWRAM**.

### Phase 4: one shared pool for the packs' modes (the "finish the packs" enabler)
BORTUA FIGHTER, BOREagotchi, the horde and arenas, the spacewalk and the launch all take the screen over while the life game holds still. Give them **one `xpPool`** (start at 12 KB, size it to the biggest mode) that every mode uses for its working buffers, with an owner byte and a clear on entry, the same rule RAM_PLAN item 3 gives the builder. New pack work then adds ROM only, not EWRAM. Rule for every new array: ROM first, pool second, `EWRAM_BSS` only if it must live across screens.

### Phase 5: IWRAM (about 3.5 KB free)
IWRAM is hot code and the audio mixer, so evict, do not shrink:
- Check `bakeBuf` (`[NIV][IH][IW]`) and `itemTmp` in `itembake.h`. They are not marked `EWRAM_BSS`; if `nm` shows them at 0x03..., they are the first things to move.
- Use the existing overlays (`IWRAM_OVL0`/`IWRAM_OVL1`, 16 uses) for code that is never live together; the six `save.h` flash routines must stay in RAM.
- `IWRAM_ARM` is 14 functions. Any that are not per-pixel work can become `IWRAM_THUMB` (about two thirds the size).

### Phase 6: only if the above is not enough (high risk)
- Mode 4 frame buffer (8 bpp, two pages): frees about 38 KB but needs a 256-colour palette, so the art would be redone.
- The player as a hardware sprite like the Sims: about 15 KB, but walls in front and the zoom both break.

## 3. What this buys the packs

Pack slots total 4.25 MB and the base game's ceiling is the real limit today. After Phase 1 (+3.75 MB) and Phase 2 (a few MB), there is room to add missions and modes without touching the music. The slots themselves are tight in one place: BORTUA FIGHTER's art (`bortua_fighters.lz` 963 KB + arena 77 KB + the other two blobs) is about 1.05 MB inside a 1.25 MB slot, so any new art there needs the slot enlarged (a one-line change in `make_packs.py`, easy once the slots sit at the top of the ROM).

## 4. Guardrails so it does not creep back
- Keep the Makefile's EWRAM and IWRAM hard limits. Add a warning band: print a note when EWRAM is over 92% or IWRAM over 92%.
- Add an equivalent check for the base ROM (fail if the base game plus assets goes over the ceiling), so an oversized song fails the build, not the pack slots.
- Every PR that adds an array puts a line in the commit message: where it lives and why.
