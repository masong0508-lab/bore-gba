# BORE: full technical notes (the long README)

GBA voxel creature creator, the seed of a later life sim. Pseudo-3D isometric view, humanoid-friendly build space:
**6 wide × 4 long × 8 high** blocks. Parts: HEAD, TORSO, ARM, LEG, EYE, MOUTH, EAR, HAIR, each in 3 sizes (S/M/L = 1×/2×/3× blocks).

## Boot logo
At power on the game plays the **DippInn Productions** logo, about 8 s long. It comes from `source/logo.c`, taken from the danny-steel project. It draws a dusk scene with parallax, then a grey scan line, then the text with a rope underline. All of it is drawn in code with tiled mode 0 and HBlank DMA, so it needs no image files. Its tables live in EWRAM. Press A or START to skip it. Afterwards the game resets the display registers and goes to the title screen.

## Controls
| Key | Action |
|---|---|
| D-pad | move cursor (left/right = X, up/down = Z) |
| L / R | cursor down / up |
| A / B | place / erase part |
| Start | cycle size S, M, L |
| Select (tap) | next part |
| Select + L/R | previous / next part |
| Select + Left/Right | turn the view 90° (D-pad then moves relative to the screen) |
| Select + A / B | cycle skin / hair colour (recolours everything) |

Arm, leg, eye and ear are mirrored automatically across the body.

## Build
Push to GitHub: the **Build GBA ROM** workflow produces `bore.gba` as an artifact (tag a commit to get it as a release).
Locally with devkitARM installed: `make`. Run in mGBA or on hardware.

## Life stages (BABY, CHILD, TEEN, ADULT, ELDER)
The creature has an **AGE**: pick it on the BODY tab of the creator (first row), or, in the classic creator, on the AGE entry of the list (A or Left/Right). The creative space grows with the age (and an elder is a little stooped):
| Stage | Build box (W x D x H) | Biggest block | Looks on offer |
|---|---|---|---|
| BABY | 4 x 4 x 5 | M | BIG HEAD / STUBBY, no big ears, CROP or BALD, 4 colours per row |
| CHILD | 4 x 4 x 6 | M | no BROAD, no LONG hair, 6 colours |
| TEEN | 6 x 4 x 7 | L | everything but BROAD, 8 colours |
| ADULT | 6 x 4 x 8 | L | everything (old people and saves are adults) |
| ELDER | 6 x 4 x 7 | L | everything |
The picker rows skip what a stage cannot have, the block builder only places inside the stage's box (its grid shows the box) and greys out sizes that do not fit. A look-built creature is rebuilt for the new stage; hand-built blocks stay (and are cut to the box if you go to a smaller stage, after asking).

In the life: the **BABY cannot be steered**: it toddles about by itself and a caretaker keeps its needs up. Child, teen and elder walk slower than an adult; the career (shifts, quota, bills) is for TEEN and ADULT only (an elder is retired). At midnight the days in the stage count up and the creature grows to the next stage (a "NOW A CHILD" note, and its sprites are re-baked). ELDER is the last stage.

**OPTIONS > TIME > AGES** (a section of the TIME page): **AGING** (OFF / SLOW / NORMAL / FAST: slow doubles the days of every stage, fast halves them, off keeps the age you picked) and how many game days each stage lasts: **BABY** (default 2), **CHILD** (3), **TEEN** (3), **ADULT** (7, or FOREVER to never grow old); each is 1, 2, 3, 5, 7, 10, 14, 21, 30 or 60 days. The stage is saved in the person (room slot format 2; format 1 slots load as adults) and also in SRAM at 5008 so growth survives a power cycle.

## Main menu, room builder, settings
Boot goes title -> **main menu** (PLAY, CREATE A BORE, BUILD ROOM, JUKEBOX, SETTINGS, HOW TO PLAY). "MAIN MENU" is the last entry in the creature part list and in the pause menu.

**Build Room** tools (Select taps to the next tool): ROOM (A corner, A again = walls + floor + doorway, min 3x3), WALL (straight line), FLOOR (fill area), ITEM (single tiles), ERASE (clear area). L/R picks floor (or item); Select+L/R picks wallpaper. 14 wallpapers and 14 floors, 90s house (floral, peach stripe, Memphis, wood panel, gingham, teal carpet, checker lino...) and factory (corrugated, red brick, cinder block, hazard, steel plate, grate, oil-stained concrete...). Floors, wallpaper and tiles are saved to SRAM.

**Settings** (saved to SRAM; open from the main menu or the pause menu). Rows are colour-coded green (fast) / yellow (mid) / red (slow), and a live **DRAW COST** meter measures how long a frame really takes to draw on your device, with 60 / 30 / 20 marks showing which frame rate it can hold (it warns when your frame rate is too high).
- PRESET: LOOKS / BALANCED / SPEED / BATTERY (CUSTOM once you change anything)
- AUTO TUNE (press A): measures your screen and picks the prettiest preset that stays smooth
- FRAME RATE 60 / 30 / 20 / 15 (logic always runs at 60 steps/s, only the redraw rate drops)
- WALLS full / cutaway / low, WALLPAPER and FLOORS patterns or plain, SHADOWS on/off
- INFO ON SCREEN full / slim / off (alerts always show), SOUND on/off
- ROM SPEED fast / safe (safe = power-on waits, for fussy flash carts)
- PERFORMANCE INFO off / FPS / DETAIL (FPS + LOAD, 100 = a frame is just fitting)
- RESET ALL

## Options (the SETTINGS screen, rebuilt)
Main menu -> **OPTIONS** (also in the pause menu and the map menu). Eight pages: **VIDEO** (the old settings: preset, auto tune, frame rate, walls, wallpaper, floors, shadows, performance info, ROM speed), **PLAY**, **TIME**, **AUDIO**, **INPUT**, **HUD**, **ROOMS**, **DATA**.
| Key | Action |
|---|---|
| L / R | change page |
| Up / Down | pick a row |
| Left / Right (or A) | change it |
| Select | put the row back to its normal value |
| B / Start | back (everything is saved) |
A gold dot marks a row that is not at its normal value. The row under the cursor explains itself in two lines.
- **PLAY**: NEEDS (off to brutal), FOOD AND WC, CAREER on/off (off = no shifts, quota or bills), JOB QUOTA, BILLS, SCORE multiplier (x0.5 to x3), TOP SPEED (80 to 150 %), MOOD EFFECTS, HURT (normal / gentle / no death), AUTO SAVE LIFE, FREE WILL. (PIPE AGE moved to TIME > AGES.)
- **TIME**: everything about time, in three sections. UP from the top row (or DOWN from the last row) puts the cursor on the section strip, LEFT / RIGHT (or A) switch section, DOWN or UP goes back to the rows. A gold dot on a section tab means a row inside it is not at its normal value. **DAY**: DAY LENGTH (3 / 6 / 12 / 24 min or stopped), CLOCK (24 h / 12 h / hidden). **AGES**: AGING plus BABY / CHILD / TEEN / ADULT LASTS (see Life stages) and PIPE AGE. **TIMERS**: COMBO WINDOW (1.5 to 6 s) and MESSAGE TIME.
- **AUDIO**: SOUND, SFX VOLUME, MUSIC VOLUME, **GAME MUSIC** (off by default: random checked jukebox songs play while you play, another starts when one ends, it fades to half volume while the pause menu (or anything opened from it) is up, and sound effects play over the music; mixing runs in an interrupt, so it costs some speed on slow devices), TITLE MUSIC, **MENU MUSIC** (on by default: a random checked jukebox song plays in the main menus, see **Jukebox**).
- **INPUT**: BUTTONS (swap A/B, L/R or both, on every screen), CURSOR REPEAT speed of the editor, BUTTON TEST (shows the keys the game sees).
- **HUD**: INFO ON SCREEN, THOUGHT BUBBLE, WANTS AND FEARS, ACTION CAM, ACCENT COLOUR (gold, mint, sky, pink, orange, lilac).
- **ROOMS**: EDITOR MINIMAP, SAVE ON EXIT, ASK BEFORE RESET, SLOTS SAVE (room / room + person / all three), ASK IN SLOTS, SAVE MAP TO SLOT, BOOT LOADS PERSON.
- **DATA**: SAVE LIFE NOW, ERASE LIFE, ERASE SAVED MAP, ERASE ALL SLOTS, ERASE EVERYTHING (main menu only, asks twice), SAVE MEMORY TEST (checks the cart or emulator keeps saves), RESET ALL OPTIONS.

**Adding an option** takes four small steps (written at the top of `source/opts.h`): add a name at the *end* of the `XO_` enum, add its choice count and default to `xoCnt[]` / `xoDef[]`, read `xo[XO_X]` where the game uses it, and add an `XR(...)` row to a page table in `source/optscreen.h`. Options are one byte each, saved with a checksum and a range check per value, so an older save simply gets the defaults for options it does not have.

## Room slots
Main menu (or pause menu, or map menu) -> **ROOM SLOTS**. Twelve named saves (the list scrolls); each holds any of a **room** (walls, floors, wallpaper, items), the **person** (the creature, including hand built blocks) and the **life** (needs, cash, job, clock, skill). What a save stores is the SLOTS SAVE option.
| Key | Action |
|---|---|
| Up / Down | pick a slot (the room is previewed on the right) |
| A | the slot's list: SAVE HERE / SAVE OVER IT, LOAD ALL, LOAD ROOM ONLY, LOAD PERSON ONLY, RENAME, COPY TO, INFO, DELETE |
| Select | help |
| B / Start | back |
The slot you saved to or loaded last is the **active slot**. With SAVE MAP TO SLOT on, the editor's SAVE MAP also writes it; with BOOT LOADS PERSON on (default) the person of the active slot comes back at power on. Loading is checked first (checksum, map size, every value) and only then applied, a save only touches its own slot, and a damaged slot is shown as DAMAGED instead of being loaded.

**Format** (all in `source/slots.h`, made so a whole house can be added later without breaking any save): a 32 byte header (magic, version, kind ROOM or HOUSE, span = how many consecutive slots it covers, what it holds, payload length and checksum, map size, save counter, name) and then a list of tagged chunks (`R` room as run-length packed tiles, `C` person, `L` life, `H` reserved for the house plan). Readers skip tags they do not know. A house is KIND 1 with a span of several slots; the slot screen already lists, protects and deletes those. To add houses: write `houseSave()` / `houseLoad()` and set `SLOT_HOUSE_READY` to 1.

## Walls (The Sims style) and 103 wallpapers
Walls are drawn like The Sims: a wall tile is a **thin, tall panel** (24 px, 3 blocks) through the middle of the tile, joined to its neighbours into lines and corners, with the room's floor running under it. Only the face the camera sees is drawn, with the wallpaper on it and a light trim along the top. **Cutaway** (OPTIONS > VIDEO > WALLS): a wall segment that hides the inside of a room drops to a low stub, back walls stay full height. "Inside" is any floor you cannot reach from the edge of the map without crossing a full wall or a doorway (a one-tile gap in a wall); low walls ('w') are fences and never close a room. Collision is unchanged (a wall still fills its tile).
- **Cheaper to draw**: a full wall tile now writes about 290 pixels (with the floor under it) against about 408 for the old wall block, a cut-away one about 118 against 272.
- **Wallpapers**: the 14 old patterns plus **89 wallpapers converted from a Sims 2 custom-content set by KHLVH** (ModTheSims, 2005). `tools/make_wallpapers.py FOLDER` reads the `.package` files (DBPF, QFS decompression and the DXT textures in `tools/sims2tex.py`), shrinks each to one tile of wall (8 x 24), pre-shades it for both visible faces and writes `source/wallart.h` (66 KB, ROM only: **no RAM**), and a preview sheet `assets/preview/wallpapers.png`. The room editor's WALL tool (L/R, or SELECT+L/R in other tools) cycles through all 103 and shows the name. The default house uses PARLOR, OCEANIC and METAL DECK (reset the map to see them).
- Saves: the map in SRAM already kept a byte per tile; room slots now use room format 2 (a byte for the floor and one for the wallpaper; format 1 slots still load).
- **Credits**: the converted wallpapers are KHLVH's work (the "KHLVH 06162005" wallpaper set on ModTheSims); they are in this repo only as the shrunk 8 x 24 versions. Check the creator's terms before you distribute a ROM with them.

## Chill pack (the PUFF PUFF PASS side of BORE)
- **New furniture** in the room editor's ITEM tool and in the starter lounge's chill corner, baked like every item (ROM only):
  - **WATER PIPE** ('G'): a green glass pipe.
  - **LAVA LAMP** ('V'): purple fluid with orange blobs. It counts toward a nicer room.
  - **BEANBAG** ('U'): four turns. It works as a sofa for COMFY, for you and for free will.
- **Using the pipe** (R next to it):
  - **Who can use it:** grown-ups, and late teens when **OPTIONS > TIME > AGES > PIPE AGE** is LATE TEENS (the default; ADULTS ONLY turns it off). "Late" is the last quarter of the teen stage in whole days, so it follows OPTIONS > TIME > AGES (TEEN LASTS) and the AGING speed. For example, a 7-day teen stage at NORMAL opens it on days 6 and 7; with aging OFF or FOREVER a teen never gets there. A teen who is too young sees "NOT OLD ENOUGH YET", children "GROWN-UPS ONLY". The rule also covers the want and PUFF PUFF PASS. Household members keep no day count, so they still need to be adults.
  - **CHILLED OUT** for two game hours: a mood boost (M_CHILL) and +1 STYLE, so tricks score more.
  - **The munchies:** hunger drains twice as fast until it wears off.
- **PUFF PUFF PASS:** a new social interaction for two grown-ups when the house has a water pipe. Free will uses it too. If you are in it, you chill out as well.
- **4:20:** at 16:20 the HUD says "IT IS 4:20". The grown-ups in the house drift over to the pipe for the next hour, and PASS is what they pick when they talk.
- **New want:** "PUFF PUFF PASS" (leaf icon) for grown-ups in a house with a pipe.

## Households (up to 14 Sims)
**Pause menu -> HOUSEHOLD** moves in a premade family (original characters: THE GRINDERS, a skater family of three; THE MIDNIGHTS, a pale night-owl family; THE FRESHLYS, a young couple; THE NOVAS, a mother and her teen) or moves everyone out. A household is you plus up to 13 more Sims (`source/house.h`, `HH_MAX`). Only the Sims in view hold a hardware sprite: OBJ VRAM (16 KB in the bitmap modes) and the OBJ palettes make 16 slots, a slot is 24 tiles (768 B, the 32x48 a Sim really fills, drawn as a 32x32 plus a 32x16 sprite) and its own palette, and `hhObjUpdate` hands slots out each frame, nearest the middle of the screen first, and takes them back when a Sim leaves the view, goes to work or school, or a passer-by walks off. So the number of Sims living in the house is limited by EWRAM (about 5.8 KB a member) and the SRAM block, not by sprites; if more than 16 are in view at once the farthest wait. A fresh sprite upload is limited to 5 per vblank, so a view turn shows the old view for a frame or two instead of overrunning vblank. Sims are in OAM in depth order, so the nearer one is drawn over the one behind. Only one Sim plans a path per step, so fourteen cost no more CPU per frame than eight. The RELATIONSHIPS screen scrolls with UP and DOWN when more than seven live there.
- **Create-A-Family:**
  - **In the creator:** the DONE tab's **ADD TO FAMILY** puts a Sim with the look and persona on screen into the household and gives them a name. Change the look and add the next one, up to 10. **FAMILY** lists them: **EDIT** swaps one into the creator, so you become them and the Sim you were takes their place, and **MOVE OUT** removes one.
  - **In play:** the pause menu's **HOUSEHOLD** has **INVITE A NEW SIM** (a made-up Sim) and **MOVE SOMEONE OUT**, and **SELECT on the RELATIONSHIPS screen** invites someone too.
  - New members arrive as family: they already like everyone at home.
- **A living day:**
  - **Work and school.** On weekdays the adults leave for work around 08:30 and the children and teens for school around 07:45. Each walks to the nearest way off the lot (a reachable tile on the map's border, or the spawn point if there is none) and is gone: no sprite, needs still draining. They walk back in at the end of the day (about 15:00 from school, 17:00 to 17:30 from work), hungry and tired, and the HUD says who left or came home.
  - **Night.** From 22:00 to 06:00, bed comes first, and a night in bed lasts until the night is over.
  - **Passers-by.** Two made-up townies (their own hardware sprites, OAM 9 and 10, with new looks every time the life game starts) walk from one way off the map to another in the daytime.
  - **Switching.** SELECT skips Sims who are out. Elders and babies stay home.
- **SELECT** (a tap, not SELECT+START) switches who you control: position, needs, look, persona and sprites trade places, and the camera jumps to the new Sim. Hand-built (block builder) creatures cannot switch yet.
- **Free will**: the Sims you do not control look after themselves. Each kind of furniture advertises a need (fridge FOOD, toilet WC, bed REST, shower CLEAN, sofa COMFY) and wandering about gives FUN. A Sim scores them (how low the need is, squared, tilted by its traits: neat Sims shower sooner, lazy ones sit, playful ones roam), picks one of the two best, finds a path (breadth-first search on the 40x40 tiles, one Sim plans per step), walks there and uses it. **OPTIONS > PLAY > FREE WILL**: OFF / LOW (waits until needs are lower, thinks half as often) / HIGH.
- **Walking animation.** Every Sim has a second, mid-stride frame for each view: legs half a block forward and back, arms swinging the other way. It alternates with the standing frame every 8 frames while the Sim walks (your Sim on foot, members following a path, passers-by), each one a little out of step.
  - Claws and pincers swing with the hand they hang from.
  - It works for both creators: legs are the leg-shaped blocks and arms the arm-shaped ones, whether built from a look or by hand in the classic block builder.
  - Household sprites keep only the part that changes: OBJ tile rows 1..5, 640 bytes per view. Your Sim keeps a full second set (`spr4s`).
- **Hardware sprites**: the other Sims are GBA sprites (OBJ, 32x64, 16 colours each with their own palette), so their moving costs no drawing; the CPU only draws their shadows and talk balloons into the room. Their four views are baked like yours, cut down to 15 colours (closest colours merged, the common ones kept exact), and only the view on show sits in sprite memory (1 KB each, copied in vblank). A window keeps them inside the room view (never over the HUD), menus and other screens hide them. Sprites always sit on top of the picture, so a Sim standing behind a full-height wall is drawn see-through (an x-ray blend) instead of in front of it. You stay drawn by the CPU (furniture in front of you covers you, the action cam can zoom you); SELECT swaps sprites both ways.
- **Social life** (Sims 2 style). A SOCIAL need (HUD bar, a LONELY alert; outgoing Sims get lonely faster). Every pair of Sims has a one-way DAILY and LIFETIME relationship (-100..100): daily changes fast and drifts back to lifetime every game hour, lifetime moves a third as much. Statuses: STRANGER, ACQUAINTANCE, FRIEND (daily 50+), BEST FRIEND (daily and lifetime 70+), DISLIKE, ENEMY (daily -50 or less), and the romance steps CRUSH, IN LOVE, STEADY.
  - **R next to a household Sim** opens the social menu (the furniture you stand at is offered first): TALK, JOKE, COMPLIMENT, HIGH FIVE, HUG, SHOW A TRICK, FLIRT, KISS, GO STEADY, APOLOGIZE, ARGUE, INSULT, SLAP. What is on offer depends on the relationship (a hug needs daily 35, a kiss a crush, going steady being in love), age (romance only teen with teen or adult with adult/elder; no slapping for children) and mood.
  - **Acceptance** = the interaction's base chance + half of how the other feels about you + their matching trait (playful for jokes, nice for compliments and hugs, outgoing for flirts) + their mood; shy Sims are wary of people they hardly know, and a Sim going steady with someone else turns flirts down. Accepted: both like each other more and fill SOCIAL (jokes and tricks also FUN). Rejected: you are embarrassed and like them a little less. Mean ones always land.
  - **Free will socials**: lonely Sims (and idle ones, outgoing ones most) go and see someone: friends, crushes and partners first, strangers to say hello, and you. Grouchy Sims go looking for trouble. What they do follows the relationship: friends joke and hug, crushes flirt and kiss, couples in love ask to go steady, enemies argue and slap. They do it to you too.
  - Balloons over heads show what is said (a word over yours, an icon over theirs), and the note line says what happened ("REX LAUGHED").
  - Wants: TALK TO SOMEONE, MAKE A FRIEND, BEST FRIENDS, FIRST KISS, FALL IN LOVE, GO STEADY, GET A HUG, SHARE A LAUGH. Fears: BEING REJECTED, GETTING SLAPPED, A FIGHT, MAKING AN ENEMY, BEING LONELY.
  - **Pause menu > HOUSEHOLD > RELATIONSHIPS**: how you feel about everyone and how they feel about you, daily and lifetime. A family that moves in already knows each other, and its first two adults are a couple.
- **The thought bubble** only shows when you stand still (nothing flashes over your head while you walk), and by default only for urgent needs (OPTIONS > HUD > THOUGHT BUBBLE: ALL brings the wants back).
- **For now** the aspiration meter, wants, job, cash and skill belong to the household (whoever you control uses them), and the household is saved in SRAM at 5216 (one household, not per room slot).
- RAM: each member's baked sprites are 11 KB (EWRAM), the free will state about 150 bytes a Sim.

**No more Sims on top of each other:**
- A Sim never picks a spot (furniture, a wander target, a spawn point) that another Sim stands on or is walking to.
- An idle Sim that someone walks onto steps aside.
- Household Sims go see-through when tall furniture (a fridge, a shower) or the player stands in front of them, as they already did behind full-height walls.

## RAM budget (work RAM, not saves)
The GBA has 256 KB of EWRAM and 32 KB of IWRAM. Every GitHub build prints the numbers in the job summary (`make size` does the same locally). By hand: `arm-none-eabi-size -A` on the object or ELF: `.sbss` is EWRAM, `.bss` + `.data` + `.iwram` are IWRAM (the stack shares what is left of IWRAM).
| | EWRAM | IWRAM |
|---|---|---|
| before the audio rework | 237,000 B (90%) | 24,436 B |
| after the audio rework | 113,000 B (43%) | 24,812 B |
| with the 8-Sim household | 211,180 B (81%) | 25,452 B |
| household on hardware sprites | 168,348 B (64%) | 25,096 B |
| boot logo + face parts, before the IWRAM diet | 172,812 B (66%) | 26,060 B |
| after the IWRAM diet | 174,980 B (67%) | 20,496 B (12 KB left for the stack) |
| 10-Sim household | 183,548 B (70%) | 20,752 B |
| + routines, passers-by, Spore parts, walk frames | 240,800 B (92%) | 20,964 B |
| 14-Sim household, 24-tile sprites, OBJ slots (`make size`) | 247,352 B (94%) | 22,324 B |
| big users now | household sprite tiles `hhObj` 28 KB + bake buffer 11 KB, screen back buffer `fb` 76.8 KB, creature sprites `spr4` 11 KB, floor tiles `flTab` 8.6 KB, overlay `ovBuf` 5 KB, BFS queue + wall map 4.8 KB (wallpaper textures: ROM only) | mixer buffers, `irqStack` 1 KB, IWRAM code 14 KB |

**IWRAM diet** (5.5 KB freed, the per-pixel hot paths untouched):
- **Cold buffers moved to EWRAM.** These are the save screens' decode and list buffers (`slTmp`, `slI`, `slLn`, `slCopyNm`), the creator's cursor preview and face-sprite grids (`ghost`, `gdec`, `dec`, each read once per voxel), and the want names (`simWTxt`).
- **Taken out of the IWRAM `drawScene`.** The ears, face sprites, body plan and creator room are now ROM functions instead of being inlined into it.
- **Compiled as Thumb.** `drawScene`, `wedgeCube`, `text` and `line` are still in IWRAM but compiled as Thumb (`IWRAM_THUMB`), which is about 2/3 the size of ARM.
- **Kept as ARM IWRAM.** The mixer, `cube`, walls, floors and blits, and the mixer buffers stay as they were.

**Audio driver.** Sound effects used to be decoded whole into a 124 KB buffer (the longest clip is 15 s) and played on their own, pausing the music. Now an effect is one more voice in the interrupt-driven music mixer: it is decoded a few samples at a time straight from the ROM and resampled from 6554 Hz to the mixer's 18157 Hz (`sfxMix`), so it needs no buffer and plays over the game music. When nothing plays, the mixer switches itself off (`audStart` / `audStop`). The title screen used to borrow that buffer for a whole-screen copy of its backdrop; it now keeps only the two areas it repaints (the smoke and PRESS START), about 10 KB, inside `spr4` before any sprite is baked.

**Room for more characters.** One baked character (4 views of 32 x 44 at 16 bits) is 11 KB, so the freed 124 KB holds about ten more at that size, or around twenty at 8 bits per pixel with a palette.

## Save memory: 128 KB flash, 32 KB SRAM as the fallback (`source/save.h`)
The ROM asks for **FLASH1M** (128 KB). At power on `svInit` sends the flash ID command first (mGBA picks the save type from the first access), and a
64 KB answer is asked to switch to bank 1 and back, which turns mGBA's 64 KB flash into 128 KB. A known flash ID means flash; anything else is
treated as **32 KB SRAM**, which works exactly as before. (Asking writes two bytes of an SRAM chip, at 0x2AAA and 0x5555; `svWr` keeps a copy of
them at 4840 and `svInit` puts them back.)
- Flash bytes can only be **programmed** from 0xFF, and only whole 4 KB sectors can be **erased**. So **sector 0** (0..4095) holds only the head of the
  room being played, and `mapSave` rewrites it when the room changed. **Sector 1** (4096..8191, the small blocks below) lives in RAM (`svLow`, 4 KB).
  All the old `SRAM_BASE+offset` code reaches it unchanged, and `svCommit` writes it back when it differs. `svTick`, called from `vsync`, compares 128
  bytes a frame, so a change reaches the chip within half a second even without a commit.
- **Slots** (8192 up) are erased (`slOpen` / `svErase`), then written byte by byte (`svWr`), header last. Two slots share a sector, so erasing one
  first copies its neighbour to the **scratch sector** (the last 4 KB) and back. Deleting a slot only clears bits, so no erase is needed. A save
  spanning several slots never crosses the 64 KB bank line (`slFits`), so read pointers (`svPtr`) stay valid.
- **Slots per chip:** 58 on 128 KB, 26 on 64 KB, 12 on SRAM. **Old saves carry over**: on mGBA a 32 KB SRAM save becomes the first 32 KB of the flash,
  so every block and slot is where the game expects it (tested).
- **Tested in mGBA:**
  - 128 KB detected; save, copy and save-over on two slots sharing a sector (the scratch sector used, the neighbour intact).
  - A slot in bank 1 (slot 58) works.
  - The active slot and the other cached blocks survive a power cycle.
  - An old 32 KB save opens with its slot intact.
  - Every slot header and checksum was checked straight from the `.sav`.
- **Not supported:** Atmel 64 KB flash chips, which use page writes.

## Save memory map (32 KB SRAM), layout 2
| Offset | What |
|---|---|
| 0 | the room being played, "BM3" (4803 bytes; older 14 x 14 saves still load) |
| 4808 | layout marker `LY2` (set once the upgrade below has run) |
| 4864 | settings (16 bytes) |
| 4896 | extended options (`opts.h`) |
| 4992 | active room slot |
| 5008 | life stage and days in it |
| 5024 | persona: aspiration, lifetime want, traits, DNA, unlocked parts |
| 5056 | jukebox song on / off flags (check boxes), unlock bits (5072), play mode (5076) |
| 5136 | the life (`sims.h`) |
| 5200 | 16 spare bytes for SAVE MEMORY TEST |
| 5216 | the household (up to 13 more Sims, `house.h`, format 'H6'; 2048 bytes reserved; 'H5' households load too) |
| 8192 | room slots of 2048 bytes: **twelve** on SRAM, **58** on 128 KB flash (to 126975; the last 4 KB are the scratch sector) |

The full map, with the compile-time checks that keep the blocks from overlapping, is at the top of `source/slots.h`.

**Upgrading a layout 1 save.** Layout 1 had six slots from 20480 and the small blocks in between. The first start of this version copies the small blocks down (`slMigrate`, `slots.h`), moves the active-slot number on by six, writes the marker and only then clears the old blocks, so a power cut at any point loses nothing. The six old slots are never touched: their bytes are now **slots 7 to 12**, with the same names and contents.

**Room format 3.** New saves store the tiles, floors and wallpapers as three separate runs lists (floors and wallpapers change far less often than furniture), about a quarter smaller than the old combined runs on a furnished room. Formats 1 and 2 still load. The default 40 x 40 map takes about 1.4 KB of a slot's 2 KB; a room covered in scattered furniture can still be too big (the game says TOO BIG FOR A SLOT and leaves the slot as it was). The slot screen preview now draws every room format (it used to draw only the oldest).

## Combos and the action cam
Clean tricks (spins, kickflips) and rail grinds now **chain**: each one adds to the chain and the chain multiplier equals the number of tricks. Land the next trick within 2.5 s (grinding keeps it alive) or the chain banks its bonus (points x (tricks - 1)). A bail or a hit loses the chain. The HUD shows `COMBO X5 2500` while it runs.
When a chain banks for more than the **ACTION CAM** threshold (Settings: OFF / over 10000 / over 5000 / over 2000, default 10000) the game holds still while the camera zooms in 1.3x on the skater and swings through all 4 sides of the room, then eases back. SELECT skips it. For reference, 14 kickflips in a row banks 19600.

## Big map, camera and minimap
The play map is now **40 x 40 tiles** (was 14 x 14), about 8x the floor space. The default map has a house (carpet, lino kitchen, pink-tile bathroom), a brick-and-steel factory with hazard lanes, and a rail park with long rails and crate boxes, joined by concrete roads and ringed by a low wall. The middle of the rail park is an open plaza.
- **Camera**: in play the view follows the skater, eased so it stays steady, and stops at the map edges. The action cam still spins round the skater.
- **Speed**: only the tiles on screen are drawn, so the bigger map costs far less than drawing all 1600 tiles. Use SETTINGS (AUTO TUNE) if your device needs it.
- **Map editor**: a dead-zone camera scrolls only when the cursor nears the edge of the screen, and a **minimap** (top right) shows the whole map, the area on screen and the blinking cursor.
- **Saves**: new maps save in a bigger SRAM block (settings moved, now at offset 4864). Older 14 x 14 saves and settings still load; an old room is placed into the plaza of the new map and re-saves in the new format.

## Skate objects and ramps
New skate-park pieces, all in the room editor's ITEM tool (L/R to pick, **Select+A turns a ramp** to face S / E / N / W):
| Item | Height | Notes |
|---|---|---|
| RAIL | 6 px | grind (links up with neighbouring rails) |
| LEDGE | 6 px | concrete box with steel edges, grind it; runs along its neighbours like a rail |
| BENCH | 6 px | slatted seat on legs, grind it |
| KICKER | 0 to 8 px | wedge ramp; the lip is on the side opposite the direction it faces |
| Q PIPE | 0 to 14 px | quarter pipe, steel coping on the lip |
Ramps have a real slope: ride up a kicker or quarter pipe on the board and you are launched off the lip, harder the faster you were going. Walking up one just works. Tuning is at the top of `source/ramps.h` (`F_RAMP_BOOST`, `F_RAMP_MAX`, `F_RAMP_TOL`). Saved maps keep working; to see the new pieces in the default park, reset the map (the default plaza now has two kickers facing each other, two quarter pipes under the wall, a ledge and a bench).

The sprite art is authored in `tools/make_skate_items.py`, which writes `source/skateart.h` (sprites), `source/rampdata.h` (matching physics heights) and a preview sheet `assets/preview/skate_items.png`. Edit the art there and re-run `python3 tools/make_skate_items.py`.

## Skate pack 2 (more objects)
Eight more pieces in the room editor's ITEM tool (the palette bar now scrolls, 25 entries). **Select+A turns the LAUNCH ramp** like the kicker. Tuning: `LAUNCH_H` in `source/rampdata.h`, the heights in `tileH()` in `main.c`.
| Item | Tile | Height | Notes |
|---|---|---|---|
| LAUNCH | `9 : ; <` (S E N W) | 0 to 12 px | taller blue wedge; launches harder than a kicker (capped by `F_RAMP_MAX`) |
| FUNBOX | `X` | 10 px | solid platform: ollie onto it, land on top |
| BARREL | `O` | 8 px | oil drum, solid (bumping at speed hurts) |
| TRASH CAN | `Y` | 10 px | solid |
| PLANTER | `Z` | 6 px | brick box, **grinds** |
| PICNIC | `K` | 6 px | table top, **grinds** |
| JERSEY | `J` | 6 px | concrete barrier with red/white top, **grinds**, links up with neighbours like the ledge |
| MANUAL PAD | `M` | 3 px | low painted pad, rolls straight on with no jump (the spot for a future manual mechanic) |
The default plaza now has a funbox with two launch ramps in line with the rail above it, barrels, jersey barriers, planters, a picnic table, trash cans and a manual pad (they only go on empty floor). A map you saved earlier keeps its layout: place them yourself or RESET MAP. Art is in `tools/make_skate_items.py` (pack 2 section), preview sheet `assets/preview/skate_items2.png`.

## Mood meters (FUN and HAPPY)
Two new HUD bars under FOOD and WC, plus a face and a mood word (SAD, BORED, OK, HAPPY, STOKED). All the logic is in `source/mood.h`, with every tuning number in the MOOD block at the top.
- **FUN** is fast: tricks, combos, grinds, air time and ramp launches fill it; it drains by itself, and twice as fast after 15 s of nothing fun (that is the BORE in BORE). Cruising on the board only slows the drain.
- **HAPPY** is slow: it drifts toward a target made of comfort (fed, bladder ok) and fun, falls faster than it rises, and takes instant knocks from bails, hurts, accidents and fainting.
- **Effects so far:** SAD cuts top speed by 15%, STOKED adds 6%; trick points are +25% when STOKED and -25% when BORED. The skater announces it when they slip into SAD, BORED or STOKED.
- **Adding a mechanic:** add a name to `MoodEv` and a row to `moodTab`, call `moodEvent(M_X)` where it happens; per-step things go in `moodTick()`; things the mood changes go in the effect functions (`moodTop`, `moodPts`) at the bottom, or read `moodState()`.
Meters are not saved to SRAM yet.

## Health (HP) and fighting
**Glossary: WC** is short for *water closet*, the toilet. The **WC bar** in the HUD is the bladder need: a full bar is good (empty bladder), an empty bar means you are about to have an ACCIDENT. In the code the player's bladder is `lbl` (100 = bursting, so the bar shows `100-lbl`); a household Sim's `need[HN_WC]` is the other way round (100 = empty bladder, like every other need). The over-head bubble says NEED THE TOILET when it is nearly full.

**Health meter.** `lhp` (0..100, `HP_MAX` in main.c) is the life meter, shown as a **2 px bar under the face** in the bottom panel (green, yellow, red). `hurt()` takes `sev` HP for a wall hit and 1.5 x `sev` for a fall or bad landing (a 40+ fall takes it all); at 0 HP you die. OPTIONS > HURT still applies (GENTLE halves it, NO DEATH keeps 1 HP). It heals +1 every 1.5 s while fed and on your feet, +5 for a meal, +40 for a night in bed. Not saved to SRAM. SELECT swaps HP with the household Sim.

**Fighting.** Next to a household Sim press **R**: teens and up get **PUNCH** (when the Sim feels neutral or worse about you). 14 to 26 damage (more from active Sims); the target hits back 20 to 70% of the time. Free-will Sims that dislike someone (daily score under -30) punch too. Nobody dies in a fight: at 0 HP a household Sim is knocked out for 10 s and wakes at 30, you are out for 4 s and wake at 25. Code: `PUNCH` row and `fightHit()` in house.h, `HhSim.hp` (not saved), `fightHurt()` in main.c, the bar in hud.h (`HK_HP`).

**Creator parts in a fight.** What a Sim is made of decides how it fights, for you and for every household Sim (`fkAtk`, `fkTaken`, `fkDodge`, `fkMove` in house.h read the Sim's own look): HITS HARDER with HORNS (+6, the blow is a HEADBUTT; NUBS +2), PINCERS (+6, a PINCH; CLAWS +2), a LONG tail (+3), a heavy build (WEIGHT slider, up to +4) and big hands (HAND SIZE, up to +3). TAKES LESS with SPIKES (-30%, and the one who hits you loses 4 HP), a helmet (-15%), a thick skull (HORNS, -10%) and a heavy build (1% a notch, never below 30%). DODGES with EYE STALKS (20%), WINGS (15%) and a LONG tail (5%). The menu entry reads HEADBUTT or PINCH when you have those parts.

## Creator chiptunes, played live (`tools/chip_synth.py`)
The 12 creator loops used to be 1,519 KB of ADPCM recordings. They are now **152 KB**: the note data that `make_chiptunes.py` voices plus shared
tables, and `chipMix` (main.c) renders them live.
- **What the 152 KB holds:**
  - 256-step wave tables for the two pulses and the triangle, one per harmonic count, so a note has exactly the harmonics below 7.5 kHz (the band
    the recordings were cut to).
  - A band-limited recording of the NES noise for each drum.
  - Two tables for the APU's non-linear mixer.
- **How `chipMix` renders:** a 40 Hz low cut, then the loop's gain. The creator loops are now **23 % quieter** (`VOL = 0.77`). The data is a flags
  byte per 1/120 s step, 4 to 7 KB per loop.
- **Checks:**
  - `chip_synth.py` holds an exact integer twin of `chipMix`. A test build (`-DCS_TEST=n`) renders 100 frames on the GBA, and they match the twin
    byte for byte.
  - Matched by spectrum, the melody and harmony voices sit closer to the approved renders than the old ADPCM did. The drums are noise, so they are
    compared by ear.
- **Cost:** 18 % of a frame while a loop plays, 4.5 % of it the pseudo-stereo that the ADPCM loops paid too. `chipMix` is 1.6 KB of IWRAM.
- **Remake:** `python3 tools/chip_synth.py [preview_dir]`.

## Tree Swaying Action
- **TREE-AGE IN ACTION** (renamed from TREE SWAYING ACTION; the jukebox hashes it by its old name, so checkmarks stay) is the ambient version (`make_tree_eno`, xm2gba.py): a semitone down and Eno-like, all generated from the song's own pad,
  pluck and bass.
  - Swells, wandering 2-4 note phrases that sometimes come back changed, rare bass and bells, chords that drift.
  - About 11 minutes, then a 1.5-minute fade.
  - Every note has a **reverb trail** of quieter, spaced repeats. An allocator gives each note the channel whose tail has died away, using the real
    sample envelopes; only 12 of 1,213 notes take over a tail louder than -30 dB.
- **TREE-AGE IN ACTION (ORIGINAL)**, hidden, is the drum rework. Its breeze pad uses a smooth echo in the calm parts and a gated stutter echo in the
  drops and risers (`TREE_ECHO = 'mix'`).

## Condensed Music (`tools/make_condensed_rework.py`)
- **CONDENSED MUSIC** (Sk9m) reworks The Dipper Man's 58-second, two-channel PixiTracker sketch (`tools/the_dipper_man_condensed_music.xm`) into
  3:10 of 2-step UK garage at 132 BPM.
- **Kept, read straight out of the sketch:** the 3-3-2 riff (Db E . Db E . D), hook A (F Ab Db | C Bb G) answered by the riff, hook B
  (Db . . . . Db F G | Ab G F Eb | C) with its turnaround and C/E - Db/F sting, the drumless F Eb C break, the tapped Eb F G run and the
  ending (a low F with a dying snare roll).
  - The sketch's lead sample sounds a semitone under its written notes, so everything is taken down one: you hear F minor in both.
- **New:** the harmony (Dbmaj7 C7 | Fm, then Dbmaj7 Eb | Fm and Abmaj7 / C7b9), the form, and every sound:
  - drums: 2-step kicks and swung 16ths (every second 16th is one 1/64-bar row late), four on the floor in the last choruses;
  - an organ stab on the riff, Rhodes chords, a garage sub bass;
  - a vowel lead with a dotted-8th echo, a glass pluck, strings, bells, blips and fx.
- **Mix:** levelled against SUNMAN SUNRISE; a hand-made stereo plan (`condensed_pan` in xm2gba.py). 205 KB of samples.
- **Remake:** `python3 tools/make_condensed_rework.py`, then `python3 tools/xm2gba.py`.

## Cocaine Cola (`tools/make_cocaine_cola_rework.py`)
- Committed on Oct 3 with its script and the stem transcription (`tools/cocaine_cola.xm`), but never added to `songs.h`, so it was not in the game.
- **COCAINE COLA** is the rework (`tools/cocaine_cola_ii.xm`, 7:22). **COCAINE COLA (ORIGINAL)**, the transcription, is hidden.
- Levels: GAIN 1.8 (rework) and 3.0 (original). No artist set yet (`source/artists.h`).

## Room builder HUD
- The text at the top and bottom of BUILD ROOM sits on the room shaded to a quarter brightness (`edShadeBand`), so it reads on any floor.

## Neighborhood (`source/neighborhood.h`)
- **The town** is a 24 x 24 grid. Each cell has a terrain (grass, dirt, sand, water, plaza, road) and a decoration (tree, pine, bush, flowers, rock,
  lamp, bench, fountain).
- **Lots:** up to 16, from 4 to 10 cells a side. A cell is 4 x 4 room tiles, so lots run from 16 x 16 to 40 x 40.
  - **Residential**, one of which is your HOME, or **community**: park, skate park, plaza, lounge, or old town (the default map).
- **The live room always belongs to one lot.** Going to another lot stores this one as a HOUSE slot with all its floors, named after the lot. It then
  loads the other lot's slot, or builds that lot's starting layout.
- **The room builder** keeps its cursor inside the lot, and RESET rebuilds the lot's layout.
- **MOVE IN** pays the lot's value (land plus everything built on it) and gets back your old home's value, from the life's cash.
- **Tools** (L / R): LOTS, PAINT, ROADS, DECOR, NEW LOT. SELECT picks the kind, A applies, and holding A paints. B goes back.
- **START** opens the town menu: zoom, season, time of day, rename, help, a new town.
- **Saved** in a TOWN slot (kind 3). The first town is BOREVILLE: your place, four empty lots and five community lots.
- **Hook:** `nbDrawLotModel()` draws each lot's building as an icon. The real house can be drawn there later.
- **Tested in mGBA:** the town draws in both zooms; visiting another lot saved YOUR PLACE as a 2-slot house and wrote the town slot.

## Undo and redo (`source/undo.h`)
- **Keys:** SELECT + B = UNDO, SELECT + START = REDO (in the room builder). The MAP MENU lists UNDO n and REDO n too, and the builder shows `UNDO n  REDO n` at the top right when there is something to step through.
- **A step** = everything between pressing A / B and letting go (`udEnd` runs when both are up, before a menu, before a floor change and when the builder closes). A tile put back exactly as it was is not a step.
- **How:** before a tile changes, `udRec(x, y)` stores its three bytes (item char, floor, wallpaper) packed in a u32 (11 bits position, 4 floor, 8 wallpaper, 8 item). UNDO swaps the stored tiles with the live ones, in reverse order; REDO swaps them again, in order. Only recorded tiles are touched, so a play test in between cannot corrupt the history. Callers: `mapPlace` (BUY / SELL, including the old board / spawn tile it clears), `eApply` through `edSet` (ROOM, WALL, FLOOR, SELL area).
- **Money:** `edPay` adds what it charged to `udCashD`; the step keeps it. UNDO refunds it, REDO charges it. A step that cannot be paid for is refused (`NOT ENOUGH CASH TO UNDO / REDO`). No purse (the builder is free) means nothing to settle.
- **Floors:** a step remembers its floor; UNDO / REDO on another floor goes there first (`flGo`), and refuses if the floor pool cannot hold the change.
- **Limits:** `UD_ACT` 3 steps (you can undo three times), `UD_REC` 1,000 tiles. The oldest steps fall off. One step bigger than `UD_REC` (a 40 x 40 fill) clears the history and shows TOO BIG TO UNDO. Cleared on RESET MAP, after a blueprint loads, and when the builder opens.
- **Memory:** about 4.2 KB EWRAM (`udR` 4,000 B, `udA` 136 B, a few bytes of flags and two 12 B menu labels). No IWRAM. The last measured build had about 6.6 KB of EWRAM free: run `make size` and lower `UD_REC` if it is tight.
- **Tested:** the logic (steps, redo cut-off, cash refunds, refused undo, repeated tiles, the 16-step and 1,000-tile limits, floors) in a host-side C harness. **Not run on the GBA or in an emulator.**

## Community lots: visiting and building
- **The rule:** while the live room is a community lot, nothing may build it except the town view. `nbBarred()` (`neighborhood.h`) is true when the town is loaded, the live lot (`nbT.cur`) is a community lot and `nbEditPass` is not set. The town view's BUILD sets `nbEditPass` around its `mapEditor()` call; nothing else does.
- **What it blocks** (all with the toast COMMUNITY LOT BUILD IN THE TOWN, `edGate()` in `main.c`): the pause menu's BUILD > EDIT MAP, BUILD ROOM in the main menu, the creator's BUILD button and its BUILD ROOM row. Loading a room or a house slot over the lot says NO BUILDING WHILE VISITING (`SLE_VISIT` in `slLoad`; loading a Sim only is still fine, and BLUEPRINTS can still save). The pause panel shows PAUSED VISITING and the BUILD tile reads NO BUILDING WHILE VISITING.
- **Staying on a community lot:** after VISIT the live lot stays the community lot until you play another one (as before), so BUILD ROOM in the main menu keeps saying no until you PLAY your home from the neighborhood. A later version could send you home when the visit ends.
- **MAKE COMMUNITY / MAKE RESIDENTIAL** (lot menu): any lot that is not your HOME and has no household. Making one a community lot asks for the kind (park, skate park, plaza, lounge, old town). An empty live lot gets the starting layout of its new kind; a lot with something built keeps it. Saved with the town.
- **Not done:** per-lot opening hours, community lots with their own rules, an automatic walk home. **Untested on hardware.**

## Choose a neighborhood (the screen before the town)
- **The screen:** main menu NEIGHBORHOOD opens a chooser with a panel of town thumbnails. Each thumbnail is an aerial view drawn from the town's save.
  The picked town is drawn darkened behind the panel.
  - A plays the town, LEFT / RIGHT choose, SELECT makes a new town, START renames or deletes one.
- **Every town is its own TOWN slot.** One of them is yours (`pad[0]`): the live room is a lot there. Picking another town first stores your lot
  there as a house slot, then opens the new town on its last lot (or its home lot).
- **The first visit** makes three towns: BOREVILLE (where the room you already had becomes YOUR PLACE), MESA FLATS (desert) and PINE COVE (lakeside).
  On a 32 KB SRAM chip it makes BOREVILLE only.
  - SELECT makes a new town from a style: suburb, desert, lakeside, or empty land to build up yourself.
- **Delete** removes a town and its house slots. It is refused for the town you live in.
- **Test build:** `-DSV_FORCE_SRAM` makes the game use plain 32 KB SRAM, to test the fallback in an emulator.

## DeadSet 3Thousand VYBE (item `Q`)
- **What it is:** a parody VR headset shown on a display bust. The sprite is hand-traced pixel art, not boxes (`dsArt` in `itembake.h`): a bearded
  mannequin with the headset strapped over its eyes, a blue light and a green shirt. A mirrored copy covers the other facing.
- **Price:** **5000**. Placing one in the room builder takes it from the life's cash (free before any life is saved); removing it sells it back.
  It also counts 5000 toward a lot's value in the neighborhood.
- **In play:** stand next to it and press **R** to JACK IN. You are frozen in VR for a few seconds, with a big FUN boost.
- **Room score:** it counts as its own kind of item for the ROOM need.
- **Where to find one:** the default house has one in the chill corner.
- **Fix that came with it:** item sprites are set up at power on (`itemSpanInit`). The room builder showed no furniture until you had played once.

## Music data format (compact, lossless)
`python3 tools/xm2gba.py` writes two files: `source/musicdata.h` (about 80 KB of text: per song the order list, pattern lengths, voice table, pitch anchors and the XmSong struct, plus the `.incbin` lines) and `source/music/xmdata.bin` (every song's note events and every sample byte). Note events are a byte stream: runs of empty rows cost one byte, a row with notes is a count byte plus 3 bytes per note (voice index into a per-song table of channel / instrument / pan bus, note, volume). Each instrument stores one 32-bit pitch anchor instead of 96 playback steps; `xmStep()` in `main.c` rebuilds every step with integer maths and the converter checks that it equals the old table exactly (and keeps a fix-up list for the rare note that would differ; none do today). The samples are stored as they always were. This cut the ROM by about 500 KB and `musicdata.h` from 7.4 MB to 80 KB **with identical audio**: the old and new ROMs were run in an emulator and every mixed audio buffer of all 23 songs, one pass plus the loop point, hashed to the same values.

## Title music
The title screen plays "The Dipper Man" (tools/the_dipper_man.xm). `python3 tools/xm2gba.py` converts the XM (and every other `SONG_XM` song listed in `source/songs.h`) to `source/musicdata.h`: note events per pattern (with per-note volume) plus the instrument samples that are actually used (8-bit, band-limited, down-sampled to the lowest rate that keeps them clean; the title song is about 70 KB in the ROM). A 10-voice mixer with linear interpolation plays it through Direct Sound B at 18157 Hz (exactly 304 samples per frame). The 7.7 s intro plays once, then the song loops from order 4; voices are never cut at the jump, so the last notes ring into the first ones. Music stops when you press START.

**Sunman Sunrise**: `tools/make_sunman_rework.py` builds `tools/sunman_sunrise.xm`, a sunrise nu-disco rework of "The Dipper Man - Sunman" (the original is kept as `tools/the_dipper_man_sunman.xm`). The hook (both voices, note for note, an octave lower), its rhythm, the walking bass and the counter-line are the original's; the harmony (D/F# Gmaj7 Bbmaj7/A C | D C Bbmaj7#11 Gmaj7), the arrangement (bell intro, four-on-the-floor groove with pumping pads, arp, breakdown, last chorus a whole tone up) and every sound are new. 116 BPM, 3:04, 12 channels. Re-run the script, then `python3 tools/xm2gba.py`.

**An Ode to the Spanish Flexicode**: `tools/make_flexicode_rework.py` builds `tools/spanish_flexicode.xm`, a soft, wide rework of "The Dipper Man - An Ode to Mexicode" (the original is kept as `tools/the_dipper_man_ode_to_mexicode.xm`). It keeps the nylon stab figure, the F Dorian progression and its Db bridge, the bass riff, the melody, both counter-lines and the ending on F major from the original. New: a 12/8 bembe groove, a breathy flute with a ping-pong echo on its own channel, pads that breathe on every dotted beat, marimba, vibes and glass bells. It uses all 16 mixer voices, each one hand-panned (`flexicode_pan` in xm2gba.py). 112 BPM, 2:27.

**Gottcho Barracho (rework)**: `tools/make_barracho_rework.py` builds `tools/gottcho_barracho_ii.xm`, the sister song of the Flexicode rework. It reads the bass line, the lead line and the piano harmony straight out of the original `tools/gottcho_barracho.xm` bar by bar, so the tune, the B-major riff and the changes are the original's, and plays them with the Flexicode palette: nylon, flute with a ping-pong echo, marimba, vibes, glass bells and breathing pads. Its bell call (B D# F# A C#) sits a tritone from Flexicode's (F Ab C Eb G). A soft 4/4 groove at 32nd-note resolution adds hat ratchets, flams and ghost notes, across all 16 voices hand-panned. 80 BPM, 3:04. The original transcription is still in the game as the secret song **GOTTCHO BARRACHO (ORIGINAL)**, which the title-screen code (UP UP DOWN DOWN LEFT LEFT RIGHT B A START) reveals in the jukebox.

**Mi Cora Zone**: `tools/mi_cora_zone.xm`, a rework of "The Dipper Man - Mi Cora Zone", built from the original's own samples. It runs intro, drop, hook, breakdown, second drop, half-time bridge, a final section lifted 2 semitones, and outro. 133 BPM, 2:50. The generator is not in the repo.

**Emergency On The Dance Floor (hi-tech)**: `tools/make_emergency_rework.py` builds `tools/emergency_hitech.xm`. It reads the bass riff and lead line straight from the original at the original 181 BPM. The sounds are all new: an FM growl bass over a sine sub, a supersaw pluck lead with an octave shadow and a ping-pong echo, glassy FM arps, digital stabs, a vowel pad, a ring-modulated hat, glitch ticks, data blips, zaps, lasers and risers. It runs at 32nd-note resolution for ratchets and stutters, uses all 16 voices hand-panned (`hitech_pan`), and lasts 1:28. The earlier rework is now the secret song **EMERGENCY (ORIGINAL)**, which shows up after the title-screen code.

**Excuses (house)**: `tools/make_excuses_rework.py` builds `tools/excuses_house.xm` from "The Dipper Man - Excuses", an expansive house / ambient track. It runs 6:47 at 124 BPM.
- **Kept from the original:** the F, Dm, Bb and C chord pairs, the climbing bass and the melody.
- **New:**
  - Deep house drums with offbeat open hats, a rolling bass and organ stabs.
  - Pads that open from dark to bright over the song (three baked pad colours).
  - A chiptune sine lead (a 4-bit stepped sine) and a glass-bell lead with ping-pong echoes.
  - FM arps, an air texture, risers, swells and two long ambient breakdowns.
- **Technical:** 32nd-note rows, all 16 voices (`excuses_pan`).

**Whistler Man (shuffle)**: `tools/make_whistler_rework.py` builds `tools/whistler_shuffle.xm` from "The Dipper Man - Whistler Man", a steely jazz-rock take. It runs 5:37 at 100 BPM.
- **Kept from the original:** the bass walk, horn dyads, arp, whistled tune and trill lick, with their straight 16ths swung into triplets.
- **The groove is the Purdie shuffle:** hats on the first and third triplet of each beat, the snare on 3 in half time, ghost notes on the middle triplets, kick on 1 plus pickups, and triplet fills.
- **Harmony and band:** Gm9 | C9 | Fadd9 | D9 on Rhodes, plus clav, a swelling horn section, a breathy whistle with an echo or a harmony a third under it, jazz-guitar licks, and a bridge of D9 hits.
- **Technical:** all 16 voices, panned like a live band (`whistler_pan`).
- **Secret:** the first version is still in the game as **WHISTLER MAN (ORIGINAL)** (`tools/whistler_shuffle_old.xm`), shown in the jukebox after the title-screen code.
- **Smoother take:** the bridge's band hits now follow the bass walk (Gm9, C9, Fadd9, D9) instead of hitting D over it, and the tune after the bridge stays in its own octave (an octave up it went shrill). The whistle, horns and hats are rounder and darker. The first version is still the secret **WHISTLER MAN (ORIGINAL)**.

**Worthless Clouds**: rebuilt at load time by `make_clouds` in `tools/xm2gba.py` from `tools/worthless_clouds.xm`. It is a swung funk / house arrangement at 115 BPM over all 16 voices (`clouds_pan`) and runs 11:24.
- **Second act:** after the original arrangement comes a build into a FINALE, where the lead is doubled an octave up on a free voice.
- **Expanded ending:** the run-in plays first. The outro riff follows four times, stepping down from house to funk to groove to hats only. Then come a soft afterglow, the original coda, and a last hit that rings out.

**The Cynical Syndication (drum & bass)**: `tools/make_cynicaller_rework.py` builds `tools/cynicaller_dnb.xm` from "The Dipper Man - Cynicaller Madness" (`tools/cynicaller_madness.xm`), a liquid / tech drum & bass track. It runs 4:06 at 172.5 BPM.
- **From the original:** the Bm / F#m / Am / Em changes, the B F# A E bass walk, the lead, the falling counter-line and the echoing E of the outro.
- **New:** a two-step break with flams, ghosts a 64th late and 64th hat ratchets; a reese bass over a sine sub; 32nd-note arps with 64th flurries; and snare rolls that speed up to 64ths.
- **Arrangement:** two drops, a breakdown, a third drop with the lead an octave up in thirds, a glitch section that stutters the lead, and a final drop.
- **Technical:** it uses 64th-note rows (speed 1, XM BPM 115, 16 rows a beat), all 16 voices (`cynic_pan`) and 88 KB of samples.

**Hot Damn (90s rave / IDM)**: `tools/make_hotdamn_rework.py` reads the Caustic project `tools/hotdamn_v050.caustic` directly and builds `tools/hotdamn_rave.xm`. The reader parses the SPAT pattern chunks and the SEQN song sequence. The track runs 3:48 at 135 BPM.
- **From the original:** the organ's F# minor riff with its 64th-note chromatic runs, its long and doubled takes, and its chord fall; the modular lead; and its octave pumps (B C# | D E | A B).
- **Prodigy:** an Amen-style break over a four-to-the-floor kick, and the riff as a 303 acid line (closed, long and accented samples, picked per note). It also has a hoover and organ stabs.
- **Moby:** a piano / strings / "ahh" breakdown on the pumps.
- **Aphex Twin:** a drill'n'bass section where every beat of the break is cut up (pitched snare ratchets, kick 32nds, 64th hat rolls, gaps, reverse swells), with the lead's licks stuttered on a bell.
- **Technical:** it uses 64th-note rows (speed 1, XM BPM 90) and all 16 voices (`hotdamn_pan`).
- **Second take:** less busy and more syncopated, ending in a long fade as the band plays off one by one (the drums and acid first, then the bass and lead, the strings and piano last). The first take is still in the game as the secret **HOT DAMN (ORIGINAL)**. The break is a sparse two-step with the kicks off the beat and hats only on the offbeats. The organ stabs land on the a of 1 and the and of 3, and the acid line keeps its 64th runs for the ends of phrases. The IDM licks play every other beat, and a soft string bed under the drops keeps the space wide.

**Aim and Shoot**: `tools/make_aimandshoot_rework.py` reads the Caustic sketch `tools/aimandshoot_v11.caustic` (81 BPM, 53 seconds) and builds `tools/aim_and_shoot.xm`, a complete 2:52 piece.
- **From the sketch:** the pulsing ostinato (moved up a semitone to sit on the FM part's C#), the FM tune, its descending progression (C# B A G# | F# G# A B | E B E A D G#), its second line and its beatbox pattern.
- **The colour:** Damascus, kept understated. The C# Hijaz mode is already in the sketch, so it is left to speak. A small takht plays it: oud, qanun, ney and a string section in unison and octaves. The oud and qanun ornament the tune a 64th ahead (heterophony), with tremolo on the long notes.
- **The groove:** a modern half-time beat, with a quiet maqsum on the darbuka underneath. The ending resolves to C# major.
- **Technical:** 64th-note rows at 81 BPM (speed 2, XM BPM 108) and all 16 voices (`aim_pan`).

**The Magic Act**: `tools/make_magicact_rework.py` reworks "The Dipper Man - The Magic Act" (`tools/the_dipper_man_the_magic_act.xm`: F minor, 125 BPM, 2:31) into `tools/magic_act.xm`, a 6:47 suite credited to Singhs.
- **From the original:** the arpeggio F G Ab C F C Ab G (the "Wheel"), the hook (Ab Ab G G F | Eb Eb D D C | Db Db Eb Eb F | Eb Eb F F G | E E F G), the stab rhythm, the bridge's two voices in thirds (Fm Cm Bb Eb), the four-on-the-floor drive and the lone F at the end.
- **How it travels:** the Wheel is always degrees 1-2-3-5 of the scale it sits in, and the hook is stored as scale degrees, so both change colour with the mode: F Nahawand, C Hijaz (E E Db Db C ...), Db Lydian, A Hijaz, G Nahawand, and at the very end F major.
- **The form (Brian Wilson-style modules, nothing played the same way twice):** Curtain (a free-time ney taqsim over a tanpura; the Wheel speeds up until it becomes the tempo) | Wheel I and II (the original A section, then everything moved) | Ladder (the bridge thirds climbing Eb, F, Ab) | Bazaar (7/8: the Wheel without its last note is one bar) | Duet (oud and ney; the ney's last Db becomes the next key) | Floating (12/8 Db Lydian: the hook at half speed on an ondes-like voice, the Wheel 8-against-12 on a metallophone) | Sleight of Hand (fast A Hijaz on pizzicato, a trill, a bar of silence, a gong for the reveal) | Voices (a cappella in seven parts) | Return (F minor, then lifted to G) | Procession (9/8: the Wheel plus its tonic is one bar) | Climb (the thirds with the band, Bb then C) | Reveal (C7(b9) to F major, the hook in major, a sung tag, a lone F).
- **The colour:** the Singhs takht (oud, qanun, ney, duduk, strings, frame drum, a quiet darbuka, riq) with an otherworldly layer (choir, glass bowl, an ondes-like sine, gong, tanpura, metallophone, crystal), and no music-box twinkle.
- **Technical:** 0.04 s rows (speed 2, XM BPM 125), with a pulse of 8, 12, 16 or 24 rows for the metric changes; all 16 voices (`magic_pan`); 29 instruments, of which the drums and bass are Aim and Shoot's own samples (stored once in ROM). Every build checks that each pitched note belongs to its bar's scale or chord. Re-running the script gives a byte-identical file.

**Nursery Time**: `tools/make_nursery_rework.py` builds `tools/nursery_time.xm` (a Singhs track, about 6 minutes, 16 channels, all synthesised): the original music-box ditty is a chalkboard techno-diddy that turns into hard rock, thrash, nu-groove, a Hijaz break, a solo, an anthem and false endings, all from the original tune. Listed in `source/songs.h`; `python3 tools/xm2gba.py` bakes it.

**Meltdown in Mars (90s house mix)**: `tools/make_meltdown_house.py` builds `tools/meltdown_in_mars_house.xm` (126 BPM, about 6 minutes, 10 channels, all sounds synthesised), it is listed in `source/songs.h`, and `python3 tools/xm2gba.py` bakes it into `source/musicdata.h`. `python3 tools/preview_xm.py meltdown_in_mars_house tools/meltdown_in_mars_house.xm out.wav` renders it the way the GBA mixer will play it.

## Extended font
The game font (`assets/font/bore_font.png`, built into `source/fontdata.h` by `tools/make_font.py`) holds A-Z, 0-9, the ASCII punctuation (`! + - . , : ; ' ? / ( ) = > % & @ # * " _ ~ $ < [ ] ^ { } | \``  and backslash) and a-z, and now also, in all three sizes:
- **Latin-1 and Latin Extended-A letters with their marks, capitals and lowercase**: acute, grave, circumflex, diaeresis, tilde, ring, caron, breve, dot, double acute, macron, ogonek, cedilla and comma below, plus the apostrophe of d', l', t'. That covers French, Spanish, Portuguese, German, the Nordic languages, Polish, Czech, Slovak, Hungarian, Romanian, Turkish, Croatian and the Baltic ones (`Å Ç É Ñ Ö Ü ą ć ę ł ń ś ź ż č ď ě ň ř š ť ů ž ő ű ğ ı İ ș ț` ...).
- `ß æ Æ œ Œ ø Ø ł Ł đ Đ ¿ ¡ « » ‹ › ° ± × ÷ £ € ¥ ¢ § • … – —`.
- Anything else with a mark (for example `Ĉ` or `Ŭ`) falls back to its plain letter, curly quotes to `'` and `"`; characters with no look-alike leave a gap.

Strings in C may hold the characters directly (UTF-8): `text()` decodes them, and `tw()` measures them. A capital with a mark has its accent in extra rows above the capitals (`FTOP_s/m/l`, 2 / 4 / 8 rows): `text(x,y,...)` still means "the top of the capitals is at y", so nothing else on screen moved; glyphs with no ink up there skip those rows, so plain text costs what it always did. An accent on a capital at the very top of the screen is simply clipped.

To change or add glyphs: edit the mark shapes, the glyph lists (`MARKED`, `LIGS`, `SMALL`) in `tools/font_ext.py`, run `python3 tools/make_font.py --extend` (it rebuilds every non-ASCII glyph from the plain ones, so the original glyphs are never touched) and commit `assets/font/` and `source/fontdata.h`. The `{code point, letter}` fallback table is generated from Unicode.

## Jukebox
The **MUSIC PLAYER** (main menu -> JUKEBOX) is one screen: a title bar (how many songs are checked, volume), a **NOW PLAYING card** (state, play mode, song, artist, elapsed / total time, progress bar, equalizer) and one song list. Each row shows a **check box**, the song name, its artist and its **length** (m:ss). **Only checked songs are ever picked at random**: when the jukebox opens, in the main menus (MENU MUSIC), for GAME MUSIC, and in SHUFFLE mode when a song ends. With nothing checked every song counts.

**Opening the jukebox plays ONE random checked song** (never the one picked last) and puts the cursor on it. A name too long for its column is cut with `..` and scrolls on the cursor row. Volume is the MUSIC VOLUME option (LEFT / RIGHT change it here too).
| Key | Action |
|---|---|
| Up / Down | move the cursor (hold to scroll a long list) |
| A | play the song under the cursor; A on the song that is playing stops it |
| L / R | previous song (the ones played before, else the one above) / next song (by the mode) |
| Select | check / uncheck the song (the check box) |
| Start | play mode: **SHUFFLE** (random checked song) -> **IN ORDER** (next checked song down the list) -> **REPEAT** (same song) |
| Left / Right | music volume: quieter / louder |
| B | back to the menu |

The play mode is saved in SRAM at 5076 (`'M'`, mode, mode xor 0x5A, inside the 80-byte jukebox block). **Song lengths** are worked out from the song data, never stored: a tracker song is the rows of its whole order list x samples per row, a streamed song is its sample count (x 3/2 when stored at 2/3 rate), all at 18157 Hz, rounded to the nearest second (`jbSecs()` in main.c). The elapsed time uses the same maths on the main deck's position.

**Menu music.** Whenever a main menu is open one random checked song plays (OPTIONS > AUDIO > MENU MUSIC, on by default). It carries on through the quiet screens (OPTIONS, ROOM SLOTS, HOW TO PLAY) and stops when PLAY, CREATE A BORE, BUILD ROOM or the jukebox opens; back at the menu a NEW random song starts. When a song ends, another random one follows.

**Locked songs (`source/unlocks.h`).** `UNLOCK("SONG NAME",bit)` keeps a song out of the jukebox, the menu music and the game music until its bit is set. The bits live in SRAM at 5072 (`'U' 'L'`, the bits, the bits xor 0x5A, inside the 80-byte jukebox block, so an old save just reads as all locked) and are set by `jbUnlock(bit)`, which rebuilds the list at once. WORTHLESS CLOUDS (`UL_CLOUDS`, defined in `sims.h`) unlocks when `simLtwCheck()` sees the lifetime want met: the HUD shows LIFETIME WANT MET, then SONG UNLOCKED. A life saved with its want already met unlocks it on the next check. ERASE EVERYTHING locks it again; the title-screen code shows it.

The check boxes are saved in SRAM (offset 5056, 14 bytes): one bit per entry of `songs[]`, plus a hash of the song names. **Songs added at the end of `songs.h` come in checked**; the saved boxes are reset (everything checked) only if the songs in front of them were reordered, renamed or removed. The jukebox holds up to 64 songs (secret ones included) and scrolls. The artist of a song lives in `source/artists.h` (`ARTIST("SONG NAME","Artist")`, the name as written in `songs.h`); a song with no line there shows no artist. Titles and artists may hold punctuation and real UTF-8 letters (see **Extended font** below).

**Adding a tracker song (.xm):** copy it into `tools/`, add `SONG_XM(my_id,"MY SONG","tools/my_song.xm")` to `source/songs.h`, then run `python3 tools/xm2gba.py` (needs numpy + scipy) and commit the new `source/musicdata.h` **and `source/music/xmdata.bin`** (the note events and samples of every song, pulled into the ROM with `.incbin`). A tracker song costs about 30 to 250 KB of ROM, mostly its samples. The player handles up to 10 channels, 32 instruments, any pattern length, notes and the volume column; it ignores effects, panning, envelopes and note-off (the script warns if a song uses them). Speed/BPM must stay fixed in the song. `GAIN` in `tools/xm2gba.py` sets a song's loudness.

**Adding streamed songs:** `python3 tools/encode_song.py "my song.mp3"` (needs ffmpeg + numpy). It writes `source/music/<id>.adp` and adds a `SONG_ADP(...)` line to `source/songs.h`. Song spec: 4-bit IMA-ADPCM, mono, 18157 Hz, about 9 KB per second, up to 64 songs. Song titles use capitals, digits and spaces (the font has no punctuation). The three `PLACEHOLDER` songs and the tracker songs (`SONG_XM`) are there so shuffle can be heard from day one: delete the placeholder lines in `songs.h` and the files in `source/music/` when you add real songs.


## Creature creator (rebuilt)
The creature screen is now a character-creator: a live preview in a little house room (the game's own wallpaper and floor) on the left, a card of numbered tabs on the right.
| Key | Action |
|---|---|
| L / R | change tab (1 BODY, 2 FACE, 3 HAIR, 4 CLOTHES, 5 PARTS, 6 ASPIRE, tick = DONE) |
| Up / Down | pick a row |
| Left / Right (or A) | change it: named options (shape, eyes, mouth, ears, hair style) or 8 colour swatches (skin, hair, top, bottom) |
| Select | turn the creature (compass bottom left) |
| Start | jump to DONE (GO LIVE LIFE, EDIT MAP, MAIN MENU) |
| B | back to the main menu |

The old BUILD tab (block builder) is gone; the classic block screen is still behind the Konami code (START+SELECT in the creator). Changing shape, ears, hair style or a part after hand-building asks before replacing your blocks.

**More looks** (all built from blocks, like the rest of the creature):
- **HAIR tab**: STYLE (CROP, BOWL, LONG, BALD, SPIKY, AFRO, FLAT TOP, SIDE TAIL, BUN; babies only CROP and BALD), COLOUR, **BEARD** (NONE, BEARD, LONG BEARD: adults and elders pick it in the dice, the mouth sits on the beard), **HAT** (NONE, CAP, BEANIE, BAND, FEZ, HELMET) and **HAT COLOUR** (as the top, as the bottom, white, black, red, gold).
- **CLOTHES tab**: TOP and BOTTOM colours, **TOP STYLE** (TEE, LONG SLEEVE, TANK, HOODIE with a hood behind the head, **BARE**), **BOTTOM STYLE** (PANTS, SHORTS, SKIRT, **BARE**) and **SHOES** (as the bottom, white, black, red, gold, as the top).
- Hats and the new hairdos add STYLE; a helmet adds STAMINA.
- **DONE tab > RANDOMIZE** (the Create-A-Bore dice): a whole new look, star sign and aspiration, only from what this life stage and your unlocked parts allow. Press it again for another.
- **More face parts and real sliders** (person save format 6, household save H4; older saves still load):
  - **FACE tab** (it scrolls) has 9 eye styles and 9 mouths, plus EYE COLOUR, BROWS, GLASSES, NOSE and CHEEKS (blush, freckles, whiskers, scar).
  - **Face sliders:** EYE SIZE, EYE SPACING, EYE HEIGHT, MOUTH WIDTH and MOUTH HEIGHT move and scale the face art on the block.
  - **Body sliders:** HEIGHT draws the legs longer or shorter, and WEIGHT makes every block below the head wider or slimmer, pushing the arms out to match. The in-game sprite eases these off a step at a time if the creature would not fit its sprite box.
  - **Ears** sit on the sides of the head in the same iso perspective as the blocks. The far ear only peeks out.
  - **Fixes:** TALL no longer pushes the eyes up onto the hair (a normal head keeps a free layer above it, and TALL now draws longer legs instead of adding a block). ATHLETIC arms hang under the front of the wide chest instead of sticking out sideways.
- Saved as person format 5 (older slots still load; the new looks start at their first option), households as 'H3' (an 'H2' household still loads).

**Creator overhaul** (person save format 8, household save H7; older saves still load):
- **Every slider notch counts:** each step moves or grows the art by at least a pixel. Before, EYE SIZE, EYE SPACING and MOUTH WIDTH needed two steps to change anything, and the FLAT mouth vanished at normal size.
- **New sliders:**
  - BODY: TORSO (longer or shorter torso), ARMS (in or out) and STANCE (feet apart or together).
  - FACE: EYE SHADE, BROW HEIGHT and NOSE HEIGHT.
  - HAIR: HAIR TONE.
  - CLOTHES: TOP TONE and BOTTOM TONE.
  - BUTT, BUTT HEIGHT and BUTT WIDTH: teens, adults and elders only; the rows are not offered to babies or children.
- **The seat:** BUTT draws two shaded, rounded cheeks on the back of the hips, with a cleft and a crease, from nearly flat to full. **They take the colour of the block they sit on**: the bottom colour in pants, shorts or a skirt, skin when the legs are bare (or a paint colour if that block is painted).
- **BARE (nudity), adults and elders only:** BARE as TOP STYLE turns the top colour to skin, BARE as BOTTOM STYLE turns the legs and hips to skin (shoes stay on if chosen); both together is an unclothed figure, a plain skin-coloured voxel body with no anatomy, and the seat is skin too. It is never offered to a BABY, CHILD or TEEN (`lkAllowed`), `fixLook()` puts clothes back when the stage goes below ADULT, and `buildLook()` ignores BARE below ADULT whatever a save or a household member's look says. The dice (RANDOMIZE) and the made-up passers-by never pick it.
- **Seventeen body types:**
  - AVERAGE, BROAD, BIG HEAD, STUBBY, SLIM, ATHLETIC and TALL were already there.
  - New: CHUBBY (soft belly), PEAR (wide hips), LANKY (long and thin, longer arms), STOCKY (short and wide), HUNCHED (head forward, a hump), POTBELLY, MUSCLE (heavy arms), PETITE, BARREL (deep chest) and DIGITIGRADE (animal legs with paws).
  - Every age gets a real choice: babies have nine bodies to pick from.
- **Furry parts (PARTS tab):**
  - ANIMAL EARS: CAT, FOX, BUNNY, BEAR, in the fur (hair) colour.
  - MUZZLE: SNOUT, MUZZLE, BEAK. The mouth moves onto its front.
  - FUR TAIL: FOX, CAT, BUNNY.
  - New PATTERNs: SOCKS (paws and hands) and MASK (a bandit band across the eyes).
  - NOSE gains ANIMAL, and CHEEKS has WHISKERS.
- **Beards and claws:**
  - Beards grow flush on the jaw (a LONG BEARD also covers the top of the chest) instead of a block sticking out of the face.
  - CLAWS are talons pointing forward out of the hand.
  - Claws and pincers follow the arm when it is drawn in against the torso and when it swings.
- **Ten-slot meters:** the ability chart is now 0 to 10 per ability, like a Sims skill bar. The body sliders add the odd points. The personality traits use the same bigger 10-slot meter.
- **Names:** DONE > FIRST NAME and LAST NAME open an on-screen keyboard with capitals, lowercase, digits and symbols (`. - ' ! ? & @ # * " _ ~ $ : ; , + / ( ) = % < [ ] ^`), up to 11 characters each.
  - The font gained lowercase letters and these symbols: `tools/make_font.py --add` adds glyphs without touching the existing ones.
  - Premade families take their surname (THE MIDNIGHTS → MIDNIGHT), and new family members take yours.

### Tab 5: PARTS (Spore style)
Like the Spore creature editor, the body decides what the creature can do. Parts are built as blocks on the model:
| Part | Options | Power |
|---|---|---|
| TAIL | NONE, STUB, LONG (furry, hair colour) | LONG = **BALANCE**: spins land clean further off straight |
| HORNS | NONE, NUBS, HORNS (ivory, out of the sides of the head) | HORNS = **CHARGE**: skating into a wall does not hurt |
| BACK | NONE, SPIKES, WINGS | SPIKES = **ARMOUR** (falls and bails hurt 30% less), WINGS = **GLIDE** (hold R in the air to float down) |
| HANDS | NONE, CLAWS (ivory talons), PINCERS (red claws) | both add GRIP; PINCERS = **CLAMP**: grinds score 2 more points every tick |
| ANTENNAE | NONE, FEELERS (a bead on each, in the paint colour), EYE STALKS | FEELERS add STYLE; EYE STALKS = **SENSE**: every DNA reward is a quarter bigger |
| PATTERN + PAINT | NONE, STRIPES, SPOTS, BELLY, TIGER, in hair, red, gold, white, black or bottom colour | body paint over the skin and shirt (never the face); any pattern adds STYLE |

The PARTS tab scrolls: three rows show above the ability chart. Saved as person format 7 and household H5; older saves still load.

Under the rows is the **ability chart**: SPEED, JUMP, GRIP, STYLE, STAMINA, 0 to 5 each (2 is normal). Shape, face, hair and parts move them (TALL is fast, BROAD tough, BIG HEAD stylish, bald is quick, wings help jumps but drag, a tail helps grip...). In play: SPEED +-5% top speed per point, JUMP +-6% ollie and hop, GRIP more grind points and faster rails, STYLE +-6% trick points, STAMINA -8% need drain per point. All of it is in `abOf()` / `abPow()` in `main.c`.

**DNA.** Big parts (LONG tail 60, HORNS 60, SPIKES 40, WINGS 120, CLAWS 30, PINCERS 90, EYE STALKS 70) are locked until bought with DNA. You can still look at a locked part (red, with a padlock): A buys it, and it comes off again when you leave the creator if you did not. DNA is earned by living: a met want pays its points, a skill level 15, a promotion 25, a birthday 50, the lifetime want 200. The Konami code makes every part free.

### Tab 6: ASPIRE (Sims 2 Create-A-Sim)
- **ASPIRATION**: FORTUNE, KNOWLEDGE, POPULARITY, PLEASURE or HOME. Babies and children always aspire to **GROW UP**; the one you pick starts when the creature becomes a teen (the row says TEEN).
- **LIFETIME**: one of two lifetime wants for that aspiration (BE A LEGEND / HAVE 3000 CASH, MAX SKATE SKILL / LAND 500 TRICKS, 20000 COMBO / GO PRO, MEET 100 WANTS / STOKED 20 MIN, 30 GOOD NIGHTS / PERFECT HOME).
- **SIGN** and **TRAITS**: NEAT, OUTGOING, ACTIVE, PLAYFUL, NICE share 25 points (0 to 10 each). A sign deals out its set of points; moving a trait shows the sign that fits best. Traits tilt which wants and fears roll and change the life: neat creatures stay clean longer, active ones need the sofa less (lazy ones sink into it), playful ones get bored faster, outgoing ones get a thrill from banked combos and shy ones are embarrassed by bails, grouchy ones (NICE 3 or less) take a fear coming true twice as hard.

## Life sim layer (Sims 2, handheld edition)
The life part of the game has a Sims 2 style loop on top of the skating. All the logic is in `source/sims.h` (art in `source/simart.h`, preview in `assets/preview/sims_furniture.png`); every tuning number is in the TUNING block at the top of `sims.h`. Everything except other people (social needs, relationships) is in.

**Furniture** (room editor, ITEM tool, 3 slots at the end of the list so old saves still load): **BED** (S), **SHOWER** (H), **SOFA** (C). They face away from the wall like the fridge. Stand next to one and press **R**; **A, B or R** gets you up early. The default house now has a shower in the bathroom, and a bed and sofa in the lounge. (A map you saved earlier keeps its old layout: place them yourself, or RESET MAP.)

**Needs.** FOOD and WC were already there. REST, CLEAN, COMFY and ROOM join them in the right-hand column. They drain on their own (REST empties in about 4 minutes awake, faster at night). Bed refills REST (~10 s), shower CLEAN (~4 s), sofa COMFY (~6 s). **ROOM** is the look of the place you stand in: it rises near furniture (fridge, toilet, bed, shower, sofa; five different kinds in 5 tiles = 100) and sags slowly in an empty place. At 0 REST the skater **passes out** (blackout, REST back to 25, a mood knock); below 20 REST top speed drops 20%. All needs feed the HAPPY meter.

**Clock.** A game day is 6 real minutes (`SIM_STEPS_MIN`), a week is MON..SUN, the game starts MON 08:00. The clock is top right and turns gold during your shift. Sleeping runs the clock fast (one game minute per step); between 22:00 and 06:00 sleep restores 25% faster and staying awake costs REST 50% faster.

**Career: pro skater**, Mon to Fri 09:00 to 17:00. Trick points you score during the shift count towards the day's quota (600 at level 0, +500 per level). At 17:00 you are paid: full quota = full pay (70, +40 per level, +30 for double the quota) and a step to promotion; half quota = half pay; less = nothing and a strike. **3 good days = promotion, 3 strikes = demotion.** Levels: NEWBIE, AMATEUR, SPONSORED, PRO, TEAM RIDER, LEGEND. A reminder shows at 08:00 and when the shift starts. **Bills** of 40 are taken every midnight; if you cannot pay you are BROKE (cash to 0, a mood knock, and it ticks the BEING BROKE fear). You start with 200.

**Skill.** SKATING skill 0 to 5 is trained by tricks, combos and grinds (12 / 35 / 70 / 120 / 200 points). Each level adds 8% to trick points. It shows as SK on the job line.

**Thought bubble.** A thought bubble over the head shows the most urgent need (WC, EAT, ZZZ, STINKY, SIT), otherwise it alternates between your wants.

**Wants and fears (The Sims 2 way).** **Four wants and three fears** are on show at the bottom right of the HUD as icon cells (green wants, red fears, a gold edge on a locked want); the line under them spotlights one at a time with its points. They roll from the creature's **aspiration** pool first, a few from anywhere, tilted by its **traits**, and wants for a need get likelier as the need runs low. A want only rolls if the map and the creature can do it (no bed, no nap want; no job, no shift wants; GO GLIDING needs wings, CHARGE A WALL needs horns). Some carry a target that is set when they roll (HAVE 450 CASH, BANK A 2500 COMBO, GAIN 12 SKILL). **A real night's sleep rerolls them**, and so do a birthday and a new aspiration; one want can be **locked** so it survives. Babies have no wants.
Wants: snack, WC, nap, get clean, sofa, land a trick, trick combo, 5 trick combo, grind, get air, bank a combo, feel stoked, feel great, finish a shift, ace a shift, get promoted, have cash, pay the bills, learn a skill, gain skill, nice room, grow up, go gliding, charge a wall. Fears: bailing, getting hurt, an accident, fainting, passing out, being broke, demotion, no pay today, being stinky, getting bored, feeling sad, dying, growing old, a shabby room.

**Aspiration meter.** A met want adds its points x5 to the meter (0 to 1000) and the same points to **reward points**; a fear coming true takes its points x5 off. The meter drains slowly by itself (twice as fast when SAD). Zones: FAILING, LOW, OK, GOOD, GOLD, PLATINUM, shown in their colours in the HUD. Higher zones slow the needs (PLATINUM halves them) and lift the mood; LOW and FAILING sink it. At 0 the creature has an **aspiration failure**: a breakdown, then a therapist puts the meter back at LOW and rolls new wants. Meeting the **lifetime want** pays 500 reward points and keeps the meter in PLATINUM for good.

**Pause menu -> ASPIRATION.** The full panel: the meter, the lifetime want and its progress, the wants (UP/DOWN and A locks one) and fears with their points, reward points, DNA, sign and abilities. **R opens the aspiration rewards**: ENERGIZER (100, REST to full), THINKING CAP (150, the next skill level), MONEY TREE (300, pays 25 every midnight), ELIXIR OF LIFE (250, resets the days in the life stage).

**Saving.** Needs, cash, the aspiration meter and reward points, the wants and fears (and the lock), clock, job level and progress, skill and the lifetime want counters are saved to SRAM (offset 5136, 52 bytes "SIM3"; an older 24 byte "SIM2" life still loads, its points become reward points) at every midnight, every payday, when you open the pause menu and when you leave the life game, with a checksum so a bad save is ignored. Dying only resets the needs: the life goes on. **Pause menu -> NEW LIFE** erases it and starts over.

**Adding things.** A want or fear: add an `SE_` name if it needs a new event, a row **at the end** of `simWants` / `simFears` (rows are saved by index: name, event, points, furniture, icon, parameter, aspirations, trait, minimum, who), and map the game event in `simsMood()` (or call `simEvent(SE_X)` yourself). A need: a variable (and in `simsSave`/`simsLoad`), a rate in `simsTick()`, a use in `simBegin()`, a bar in `simsHud()`. A piece of furniture: art in `simart.h`, a slot in `palCh`/`palNm`/`palCol` and a `drawItemTile` line.

**Not done yet:** SOCIAL need, other people to talk to, relationships.

## Household slots
**Pause menu -> ROOM SLOTS** (and the main menu's slot screen) can now keep several households. On an empty slot pick **SAVE HOUSEHOLD**; on a household slot pick **LOAD HOUSEHOLD** (replaces the Sims living with you and their relationships; your own look and life stay), **SAVE HOUSEHOLD** (overwrite it with the current one), **RENAME**, **COPY TO**, **INFO** or **DELETE**. They share the twelve room slots (slot KIND 2 in `source/slots.h`); a big household takes two neighbouring free slots. A household slot never becomes the active slot, and saving a room never overwrites one.

**CREATE-A-HOUSEHOLD** (creator > DONE tab > HOUSEHOLD, `source/hhcreate.h`): ADD THIS SIM puts the look on screen into the household, asks for a name, then who it is related to. SET A RELATION picks two Sims and what the first IS to the second: ROOMMATE, MOTHER, FATHER, DAUGHTER, SON, SISTER, BROTHER, WIFE, HUSBAND, PARTNER, then MORE KIN (grandparents, grandchildren, aunt, uncle, niece, nephew, cousin, step family, PARENT / CHILD / SIBLING); when the other side can be two or three things (a mother's child is a DAUGHTER, a SON or a CHILD) it asks that too. WHO IS WHO shows one Sim's household (LEFT RIGHT change whose); EDIT OR MOVE OUT is the old FAMILY screen; SAME LAST NAME gives everyone yours. Ages are checked (a baby cannot be a mother, a child cannot be a wife), a relation lifts the two Sims' scores to fit (never lowers them; partners and spouses start as a couple), and family never romance each other (`socAllowed`). The RELATIONSHIPS screen shows what each Sim is to you. `HC_GATE` in `hhcreate.h` makes the row need the debug code again. The old ADD TO FAMILY / FAMILY rows moved inside this hub.
- **Save format 'H?'**: the household block ('H>' before) adds `kin[HU_N][HU_N]` (u8, 64 bytes, what a is TO b, by uid) right after the relationships, before the checksum. Older blocks ('H2'..'H>') still load (no kin). Because kin lives inside the household block, household slots, the household bank and room slot copies carry it with no other change. Kin is cleared wherever the relationships are (new game, new household on a lot, MOVE EVERYONE OUT) and when a Sim moves out.

## Floors (three per house)
A house has **three floors**. The floor you are on is the live map; the other two wait in memory (packed in `flPool`, `flGo` in `source/main.c`). Stairs are two items: **STAIRS UP** (`^`) and **STAIRS DOWN** (`~`), at the end of the item list. Step on `^` to go up and on `~` to come down; step off and on again to use them once more. If the other floor has no matching stairs yet, they appear where you came from. In the map editor **SELECT + UP / DOWN** changes the floor you are building on. Upstairs the household waits on the ground floor (Sims do not use stairs yet). The room kept in SRAM, and a ROOM slot, always hold the ground floor.
**Saving:** in ROOM SLOTS an empty slot offers **SAVE HOUSE** (all three floors, 3 or 4 slots in a row, KIND 1 in `source/slots.h`); on a house slot **LOAD ALL** loads it (you start on the ground floor), **SAVE HOUSE** overwrites it. Floors upstairs that are not saved to a house slot are lost when the console is switched off. Loading a single room slot replaces the house with one floor (the upper floors are emptied).

## Sim filter (OPTIONS > PLAY)
Three on/off rows decide which kinds of Sims may be added: **PRE-MADE SIMS** (the MOVE IN families), **USER-MADE SIMS** (made in the creator, added with ADD TO FAMILY) and **MADE-UP SIMS** (INVITE A NEW SIM, SELECT on the RELATIONSHIPS screen, and the passers-by). All are ON by default, so nothing changes until you switch one off. Turning a kind off only blocks adding more of it (with a short message): Sims already in the household stay and play as before.

## Loading screen
`source/loading.h` gives `ldShow("MESSAGE", done, total)`: a full loading screen with a progress bar and percent, one frame per call. It runs while the household is baked (entering the life game, moving in, growing up: one step per Sim) and while you move between lots or towns in the neighborhood. To use it in a slow job, call it between the steps with the number of steps done so far. It cannot move *inside* one flash write, so a single big step still holds the bar for that long.

## Loading screen fix
`ldShow` now switches the display window off while it is on screen (the game runs with window 0 on, and until the first game frame sets the window registers everything outside it is black, which hid the loading screen and made the game look frozen). `ldEnd()` puts the mode back; `hhBakeAll` calls it when the last Sim is baked. If you add `ldShow` to another slow job that runs in the game, call `ldEnd()` when it is done.
`ldEnd` and the life game's start also set window 0 to the whole screen (`winFull`), so the loading screen stays up until the game's first frame replaces it (no black gap).

## Loading screen sound (the music steps aside, a tick-tock takes its place)
The music mixer runs in an interrupt and decoding a song is a big part of what slows the household bake down, so while a loading screen is up the song is simply not played.
- **First `ldShow` of a job** (`source/loading.h`, `ldBegin`): the song fades out completely (about 0.5 s) while a quiet **tick-tock** fades in over the same 0.5 s. Once the song has reached silence it is **frozen**, not decoded at all (`musMixAny` skips it), so the job gets that CPU back. The song has its own gain for this (`ldG`/`ldGT` in `main.c`), so the pause menu's half volume, the MUSIC VOLUME slider and the crossfades are untouched.
- **`ldEnd()`** (`ldBack`): the tick-tock fades out (about 0.25 s) and, `LD_GRACE` (8) frames later, the song comes back **from the exact spot it stopped at** (about 0.7 s fade in). A second loading screen inside those 8 frames cancels the comeback (PLAY from the neighbourhood loads the lot and then the household: one quiet stretch, not two dips).
- **Someone else takes the music over** (PLAY starts the game music or silence, the jukebox opens, and so on): `musBegin` / `musFadeTo` / `musFadeOut` / `musStop` call `ldDrop()`, which throws the stepped-aside song away, so no menu song blips back just before the game music.
- A job that forgets `ldEnd` is let go of after `LD_STALE` (90) normal frames without an `ldShow`. `nbGo` / `nbSwitch` (`neighborhood.h`) now call `ldEnd` on every way out.
- With **SOUND off** there is no tick-tock; with no song playing there is nothing to step aside (the tick-tock still plays, it is an effect: SFX VOLUME and MASTER VOLUME scale it).
- The tick-tock is `source/sfx/tick.adp`, made by `python3 tools/make_tick.py [preview.wav]`: one second, a TICK at 0.0 s and a lower, softer TOCK at 0.5 s, silent at both ends so it loops cleanly. The effect voice got two small features for it: **looping** (`sfxLoop`) and its **own fade** (`sfxFade`/`sfxFadeT`). `sfxStop()` never cuts a looping tick-tock, it asks it to fade out (`lifeInit` calls `sfxStop` right after the bake), and `sfxPlay` resets both, so every other effect behaves as before.
- Tuning: fade speeds are the `8`, `6`, `8` / `16` in `musMixAny` and `sfxMix` (steps of 1/256 per frame), `LD_GRACE` and `LD_STALE` are at the top of `loading.h`.

## Household changes are in the debug code
Everything that directly changes who lives in the house now needs the **hidden debug code** (title screen: UP UP DOWN DOWN LEFT LEFT RIGHT B A START, `dbgOn` in `main.c`; it lasts until power off and is never saved, the same as the secret songs):
- **Pause menu > HOUSEHOLD:** without the code it opens the **RELATIONSHIPS** screen directly (viewing is not changing). With the code you get the full menu: MOVE IN A FAMILY, INVITE A NEW SIM, TRULY RANDOM SIM, MOVE SOMEONE OUT, MOVE EVERYONE OUT.
- **SELECT on RELATIONSHIPS** (invite a Sim) works only with the code, and the hint line under the list drops "SELECT ADD A SIM" without it.
- **Creator, DONE tab:** ADD TO FAMILY and FAMILY are now the **last two rows** and are hidden (`tabRows`) without the code; `famAdd` / `famMenu` also refuse to run.
- **ROOM SLOTS:** SAVE HOUSEHOLD and LOAD HOUSEHOLD are missing from a slot's menu without the code (a household slot can still be renamed, copied, inspected and deleted).
- **Not gated:** the neighborhood's MOVE IN (it buys a lot with your cash and changes where you live, not who lives with you), SELECT to switch which household Sim you control, and the three PRE-MADE / USER-MADE / MADE-UP SIMS options (made-up Sims still walk past as passers-by). The lines above about ADD TO FAMILY, INVITE A NEW SIM and MOVE IN A FAMILY describe what the debug code unlocks.

## Faster loading (the household bake)
Entering the game bakes every Sim into sprites: the creator's renderer draws each of 4 views at full size and halves them. Measured on the GBA a draw costs about 110 ms, so the number of draws is the loading time.
- **Each view is drawn once.** The fit check (eases the HEIGHT / WEIGHT sliders off until all four views fit the capture window) used to draw the views and then draw them all again for the sprites; now a view that fits is shrunk on the spot, the probe drawing doubles as the first check, a failed round tests the view that stuck out first (one drawing, not up to four), and when nothing is left to ease off the last round is kept. 9 draws per bake became 4 for a normal Sim.
- **Only the capture window** is cleared and drawn (clip rectangle), and `cube()` works out its rim / base colours once per block.
- **Colour reduction** (`hhQuant`, `hhQuantS`): a hash table instead of list walks, and each colour matched to the palette once instead of once per pixel (2x faster).
- **Nothing is baked twice.** Each sprite set remembers a hash of everything its drawing reads (voxels, face sprites, colour tables, look, age, flags and every option, `bakeKey`); an unchanged Sim is skipped. Coming back from the editor, a slot, the pause menu, or pressing PLAY again is instant. The passers-by stay until you move to another lot or start a new life. `hhSwitch` drops the keys (the sprites move around); `hhRemove` moves them with the sprites.
- All of it is exact: a test build baked 32 random Sims (every slider and part random) with the old and the new code and every sprite, tile and palette matched byte for byte.
- Result (mGBA, fresh save): PLAY 9.2 s -> 4.4 s; PLAY again, or back from the editor: no loading screen at all.

## Long samples play to the end
The music mixer kept each voice's position and length as 16.16 numbers, so a sample longer than 65535 frames (3.6 s) wrapped to a short length and
stopped early: TREE SWAYING ACTION's pads (72 k frames) played only their first 6.7 k, which made it sound chopped, and a long sample each in
EMERGENCY ON THE DANCE FLOOR and EXCUSES was cut too. Each voice now moves its data pointer forward as it plays (`MVoice`), so pos never needs more
than 16 whole bits; shorter samples come out exactly as before. Checked in mGBA: Tree follows the preview render (envelope correlation 0.995).

## Smoother sound (declick + soft limit)
- **Declick:** a new note on a channel used to cut the old one mid-wave, and the jump is a click (Sunman Sunrise had 833 audible ones, Hot Damn
  2,702, The Cynical Syndication 6,839). The jump is now kept as an offset on the voice (`MVoice.ol / orr`) that is added to the mix and fades out
  over ~2 ms, so the wave never steps.
- **Soft limit:** the mix used to be cut flat at the 8-bit edge on loud peaks (a crackle). Past +-96 it now bends towards the edge
  (`softClip`: 96 + d*R/(d+R), slope 1 at the knee); sound effects on top of a song use it too.
- `tools/preview_xm.py` does both, so previews still match the game (mGBA capture vs preview: envelope correlation 0.98).

## Household kept on reload
`hhLoad` refused a household holding a child or teen (their GROW UP aspiration is past the pickable ones), so the whole family vanished after the editor, a slot load or a power cycle. It now accepts every aspiration; the lifetime-want lookup falls back to learning for GROW UP.

## Title logo = the cover logo
The title screen and the main menu draw the logo of the cover art (`source/titlelogo.h`, made by `tools/make_logo.py`: gold bubble letters, outline and
drop shadow, half-lidded eyes in the B and R, a leaf in the O, a lit joint on the E), full size on the title and half size in the menu (`logoSmallArt`,
the same 2x2 reduction in the generator). The generator used to write `source/logo.h`, the name the boot logo took later, so the game never showed it.
With the big "BORE" text gone the large font is no longer linked: the ROM is 186 KB smaller.

## Staged The Full Performance (`tools/make_staged_rework.py`)
- **STAGED THE FULL PERFORMANCE** (DayBar) reworks The Dipper Man's 2-minute, 12-channel shuffle into 6:28 of hi-NRG in the manner of Divine's
  "You Think You're a Man": 130 BPM four on the floor, an octave-bouncing 16th bass, claps, open hats on the offbeats, Simmons toms,
  orchestra hits, synth brass and chimes, with DayBar's things on top (an acid line and a hoover in the dub, a piano / strings / "ahh"
  breakdown, risers, reverse swells, impacts).
- **Kept, read out of the XM** (a semitone down: its samples play flat, so you hear D minor; the shuffle's triplets go to the 16ths 0, 1, 3):
  the pulse riff (D F D F | Bb D C E), the chords (Dm F Gm Am), the intro bass (D C Bb | Bb C D), melodies A and B, the breakdown climb
  (G/B C Dm C G/B) and the ending (low D under a minor-sixth chord).
- **Form:** intro, riff, verse, pre-chorus, chorus, dub, verse, pre-chorus, chorus, breakdown, two choruses a whole tone up, then the outro
  back in D minor fading out over 24 bars.
- **32nds and 64ths** (rows are 64ths): hat ratchets at phrase ends, snare rolls that speed up from 16ths to 64ths into a section, Simmons
  tom fills, the riff stuttering on the last beat of a phrase, a soft sparkle arp in the last chorus, acid flicks.
- Level matched to HOT DAMN; stereo plan `staged_pan` in xm2gba.py. Remake: `python3 tools/make_staged_rework.py`, then `python3 tools/xm2gba.py`.

## Pre-made families and the true randomizer
- Eight more pre-made families (THE STACKS, PIXELS, VOXELLS, LOWPOLYS, KICKFLIPS, BUFFERS, SPRITES, DIPPERS): the 32 random Sims the bake
  test harness uses, now with their whole look (`HhPre.look` holds all `LK_N` values). They live in the ROM: no save space is used until
  a family moves in.
- Pause > HOUSEHOLD: MOVE IN A FAMILY opens the list of all twelve families; TRULY RANDOM SIM invites a Sim of any age from child to elder
  with every slider (0-8, evenly), pick, part and colour rolled (`lookTrueRandom`; only unlocked parts, no beard on the young).
- Creator, last tab: TRUE RANDOM does the same to you (your age stays) and rolls a new personality. RANDOMIZE keeps its gentler dice.
- Passers-by keep the gentle `hhRandLook`: a fully random look bakes about three times slower.


## Main menu and PLAY (The Sims 3 look)
- The main menu is a glossy rounded panel with pill buttons (the focused one turns green), the cover logo on top, a round `?` (HOW TO PLAY)
  and an A SELECT footer. Behind it: your own town close up around a random lot (`nbDrawTown` tool -2), lit for the time of day of your
  life's clock (5-8 dawn, 8-17 day, 17-20 dusk, else night; no life yet: any). Dawn is a new light (`nbTint` tod 3), the menu only.
  Or the ACID RAINBOW (`acid.h`, a live plasma): OPTIONS > HUD > MENU BACKDROP picks RANDOM (either, a new pick each visit), TOWN or ACID.
- PLAY opens the New Game panel: the town picture, Select a Town (LEFT RIGHT / L R), a description, then CONTINUE (your life), VISIT TOWN
  (the old NEIGHBORHOOD chooser's job; SELECT makes a town, START renames or deletes one) and NEW GAME.
- NEW GAME: a fresh life (cash, job, clock) and household in the chosen town, started as CREATE A BORE (the creator, then GO LIVE LIFE),
  A PRE-MADE FAMILY (you become its first Sim; `hhMoveIn` + `hhSwap`), or A TRULY RANDOM SIM. The story mode can hook in here.

## Floors are packed in RAM (flPool)
A house's floors used to sit in `flBuf`, 3 floors x 3 planes x 1600 bytes = 14,400 bytes of EWRAM. They are now run-length packed in `flPool` (8,192 bytes): each floor is three planes (tiles, floors, wallpapers) of (count, value) runs, the same runs a house save uses, one after the other. A blank floor costs nothing, an ordinary furnished floor a few hundred bytes. EWRAM: about 6 KB freed.
- `flStoreAs(f)` packs the live map as floor `f` and returns 0 if it does not fit (the old copy stays). `flLoad(f)` unpacks it. `flGet(f, plane, i)` reads one cell of a stored floor (cheap when a plane is read in order), `flPlaneAt` reads the live map for the floor you are on.
- `FL_POOL` holds every house the save slots can hold (a house is at most 4 slots, 8,160 bytes, and the packing is the same, so a saved house always loads: a `_Static_assert` in `slots.h` guards that). Only a floor plan too crowded to ever be saved can fill it: then the stairs say TOO MUCH BUILT TO CLIMB and you stay where you are, nothing is lost. Going home (`flHome`: starting play, a new lot, loading a house) always works: if the floor you leave does not fit, it goes back to its last stored copy.
- Lowering `FL_POOL` frees more RAM but the `_Static_assert` will stop the build: a house near the 4-slot limit could then fail to load.

## Zoom (OPTIONS > VIDEO > ZOOM, SELECT + UP / DOWN while playing)
- OFF, 1.5X or 2X. Zoomed, the room is drawn only in a window in the middle (`vpX0..vpX1`, `vpY0..vpY1`; 160 x 80 or 120 x 60 with
  the panels on), so there is a half or a quarter as much to draw, and the GBA's own BG2 scaling stretches it over the room rows
  (`sbY0..sbY1`). The HUD rows stay 1:1: an HBlank DMA (DMA0) writes every line's BG2PA..PD / BG2X / BG2Y from a table
  (`source/zoomtab.h`, made by `tools/make_zoomtab.py`, copied to `zoomBuf` in EWRAM because DMA0 cannot read the cartridge); the
  vblank IRQ starts it again every frame (`zoomArm`; the IRQ is switched on for it even with the sound off, `zoomIrq`).
- Household Sims (sprites) are scaled by the hardware too: affine, double size, matrix 0 (`hhObjUpdate`), placed where their room
  pixels are shown. WIN0 keeps them to the room rows.
- Anything that is not a room picture (`present()` without `zoomKeep`: menus, messages) puts BG2 back to 1:1 at once (`zoomOff`); the
  next room frame is a whole one again. The pause menu draws the room unzoomed behind it.

## MASTER CONTROLLER (OPTIONS > SIM > MASTER, debug code only)
A nod to the Master Controller mod for The Sims. The MASTER section only shows (and only works) after the title's Konami code
(`sUnlock`) or the debug code (`dbgOn`): `mcOn()`.
- **SIZE SLIDERS: DOUBLE.** Every SIZE slider (heights, widths, sizes, lengths, colour tones: `slideEffS`) goes twice as far a notch.
  The sliders that PLACE a part (ear spread, horn height, antenna gap, arm spread, ...) keep their range, so parts never come loose.
- **BODY BOX: LIMIT BREAK.** Every age builds in the adult box, and the HEIGHT / TORSO / NECK stretch may go much further (`bxLift`),
  so an adult can stand well past the 8 block box. A stretched row is filled with a block every CC pixels, so a long stretch has no gap
  and nothing floats. In the room the sprite bake still eases a giant down to fit its 32 x 44 sprite.
- The box and the limits are read once into globals (`bxSync`, called by `buildLook` and `drawScene`): IWRAM is nearly full (code +
  .bss about 30.7 KB of 32 KB, the stack lives in the rest), and a check inlined into the drawing code was enough to crash the game.

## STORY MODE (PLAY > NEW GAME > STORY MODE; pause menu > STORY)
`source/story.h`. Three stories, each a start and six chapters the game checks by itself every second (`stTick`):
- **ROOMMATES** (romance): you and a roommate you barely know: friends, in love, steady, a promotion, a child comes home.
- **NEWLYWEDS**: you and your love (already steady): a promotion, save §1000, a child comes home, become your kid's friend, save §2500.
- **SINGLE PARENT**: you and your kid (who takes after you): a promotion, your kid's friend, save §800, have a neighbor over, a second promotion.
A chapter pays §250 and 25 jenes. The child who comes home mixes your look and your partner's (`stMixLook`). The creator opens first to
make you. Saved at `STORY_OFF` (8 bytes after the options); the household bank keeps each household's story. NEW LIFE, another NEW GAME,
a fresh pre-made family or NEW HOUSEHOLD HERE end the story (`stOff`).

## Sprites: 32 x 60, models at 0.4 size; the bubble over your head
- The Sims in the room are baked at 0.4 size (5 screen pixels to 2: `bakeShrink` keeps the top left pixel of every 2-3 pixel cell, or
  its darkest pixel when that is very dark, so eyes and outlines survive), a little smaller than the old half size.
- The sprite box grew from 32 x 44 to 32 x 60 (`SPH`, feet on row `SPF` 56): tall Sims and MASTER CONTROLLER giants have room. The bake
  draws the creature lower (`OYCB`, `oycV`) so the 80 x 150 capture window fits on the screen. Household Sims are one tall 32 x 64
  hardware sprite (32 tiles, `OBJ_B` 1024) instead of 32 x 32 over 32 x 16; 8 sprite slots (a household is 8 Sims).
- About 25 KB more EWRAM (spr4 / spr4s and the household tiles); the bake time hardly changes (the creature is drawn at full size as before).
- Over your head: no plumbob any more, and the thought bubble only when it should: an urgent need for 3 seconds when it starts (again
  every 30 seconds while it lasts), with THOUGHT BUBBLE: ALL a want for 3 seconds every 45. Talking bubbles are as before.

## THE TICKING BOMB (VanInBlack)
Latin house rework of The Dipper Man - The Ticking Bomb, built by tools/make_tickingbomb_rework.py (124 BPM). Keeps the offbeat riff, the A/D bass and the G-E-G stinger (a semitone down). Breakbeat under a house kick, congas and timbales, a low whine, and an 12-bar half-time downgroove after the fuse, then the blast back into house.

## VOICES
The Sim you control talks. 43 clips (tools/voices_src/*.wav, cleaned and trimmed) are encoded by `python3 tools/encode_voices.py` into source/sfx/v_*.adp (4-bit ADPCM, 6554 Hz, 296 KB)
and source/voices.h (X-macro list, ROM blobs, V_<name> ids). They play on the one effect voice, so the newest sound wins. `voxPlay(V_x)` always plays, `voxNag(V_x)` only when nothing else sounds,
`voxChain(a,b,c)` plays three in a row (the pipe: lighter, inhale, cough). Who plays what: `voxEvent` (main.c, called from sims.h simEventV) for life events, `voxSoc` (house.h) for socials,
and spots in main.c: falls (shriek), bails (cry), instant death (die of shock), fights (lets fight / losing / lost / win), hunger and bladder nags, sleep (snore), new wants (thinking).

## Slider locks (roadmap #5)
Most creator sliders start locked and are bought in six packs with jenes (BODY SHAPE 40, BODY DETAIL 80, BUTT 50, FACE DETAIL 40, EAR SLIDERS 30, PART SLIDERS 60): press A on a locked slider. Free essentials: HEIGHT, WEIGHT, SKIN TONE, EYE SIZE, EYE SHADE, HAIR / TOP / BOTTOM tone. The Konami code (sUnlock) opens everything. Looks keep the values they already hold (old saves carry over); the lock only stops editing, and ROLL THE DICE / TRUE RANDOM for you leave a locked slider in the middle. Saved in the jukebox block at JB_OFF+32: 'S' 'K', the pack bits, the bits xor 0x5A (appended; nothing moved, nothing resized). Code: "SLIDER LOCKS" above the creator in main.c.
**Earning jenes** (pDna, spent on parts and slider packs): a met want pays its points, SKILL UP 15, a promotion 25, a birthday 50, a lifetime want 200, a story chapter 25, and since the slider locks: a GOOD SHIFT 8 (16 on a double-quota shift, shown on the pay note) and every 5th trick landed 1 ("+1 JENE"). Tune SIM_DNA_SHIFT / SIM_DNA_ACE / SIM_DNA_TRICKS in sims.h.
**Hidden songs and story missions** (roadmap #4): CLOSER TO THE END and TREE-AGE IN ACTION no longer come from lifetime dreams. Every story chapter you finish (5 per story, the END card is not one; any life) is counted once in the jukebox block at JB_OFF+40 ('M' 'S', 8 bytes of mission bits, check byte), and half of all of them (18 of 35 with 7 stories) unlocks both songs. `jbStoryDone()` (story.h) is called from stTick. Dreams are still recorded (jbDreamMet) but unlock nothing; songs already unlocked stay unlocked. Missions finished before this patch are not counted. Adding a story: SM_PER stays 5, the total follows STY_N.

## Sound pack: RADIO and SOUND SYSTEM (items `R` and `A`)
Two room items at the END of the item list (palette slots 32 and 33, so old rooms and saves load unchanged). Art is hand-drawn pixel art in `itembake.h` (`rdArt`, `syArt`), baked into `itemrom.h` by `tools/bake_items.sh` (V_RADIO, V_STEREO). The radio is a low grindable item (height 6 like the phone); the sound system is one block tall.
- **R next to one** (`lnear` 9 / 10, `radioTune` in main.c) tunes the next station. Stations (`radioStn`): ALL SONGS FM, DAYBAR FM, SK9M BASS RADIO, DANNY STEELE FM, BRENO FM, SINGHS RADIO. A station plays the visible (unlocked, non-secret) songs whose artist name starts with its key, at random, never the same song twice in a row; after the last station the radio goes OFF and the normal GAME MUSIC comes back. A station with no visible song is skipped.
- It rides on the game-music player: `gmPick` (instead of `pickSong`) chooses the next song while `radioSt` is set, `gmSync` leaves a tuned radio alone when the pause menu closes, `gmStop` switches it off when you leave the game. To add a station, add a row to `radioStn` and raise `RADIO_N`.
- The sound system also gives a CHILL mood event. Both count as furniture for the ROOM need (the "den" bit with the DeadSet, lamp and pipe), cost §40 / §150 in the town's house value, and show in the build room palette with a preview.
- They stand in the default house's lounge (mapGen) and, since phase 1, on the pre-made community lots (nbTemplate: LOUNGE gets both, PLAZA / PARK / SKATE PARK a radio). Lots that already have a layout keep it; RESET a lot to get them.

## Death variants
`die(snd, why)` in main.c: the dead screen's note says what killed you (`deathNote`): 0 YOU DIED, 1 DIED OF SHOCK (a bail that is 40+), 2 GRAVITY WON (a fall: also a failed life-or-death roll), 3 MET A WALL AT SPEED, 4 DIED OF HUNGER (hit points ran out while FOOD < 10), 5 ONE HIT TOO MANY. Falls play the scream. Ghosts are not in yet.

## Cameos (DAYBAR and SK9M)
After a good shift (`simCameo`, sims.h): a 1 in 6 chance, then a coin toss between DAYBAR and SK9M. Sk9m's catchphrase ("IM KIND OF A BIG DEAL" ... "YEAHHHH") fills both spare note slots (only three notes fit after the pay note); his other lines come after his name. Edit `simSk9mLn` / `simCameoLn`.

## Welcome visit (scripted arrival)
`twPick` (house.h) marks the first neighbour `twWel` and sets his wait to 3 seconds, so on every new lot or new life someone walks in soon after you arrive. The welcome ignores the night rule, says "<NAME> SAYS WELCOME" and pays a housewarming gift of §25 on arrival. Later visits are as before.


## Pie menu + scrolling menu (IWRAM safe)
- `source/pie.h`: Sims-style pie menu. Social interactions (R next to a Sim) show a ring of up to 8 chips; D-pad picks by direction (two keys = diagonal), L R step round, A confirms, B backs out. More than 8 interactions go in two levels (FRIENDLY / FUN / ROMANTIC / MEAN / USE), B steps back up.
- `menu()` now opens with a short grow animation, sizes to its longest line, scrolls long lists (scroll bar, n/m counter, L R page) and plays tick/pop sounds.
- IWRAM: pie.h is ROM code with no statics (arrays live on the EWRAM stack); the static symbol set (names + sizes) is identical to before. The Makefile now fails the build if `.bss + .data + .iwram` > `IWRAM_MAX` (32512 B), printing the figure on every build.

## Ghosts and weather (`source/fx.h`)
Both are hardware sprites on the OBJ slots the household does not use (slot 8 = tiles 768..799, OBJ palettes 8..11, OAM entries 16..58), semi-transparent like the household's x-ray sprites, clipped to the room view by the same window. They add no frame buffer and no drawing time; the cost is about 330 bytes of EWRAM (ghosts 60, particles 120, state) and about 1.3 KB of ROM for the ghost art plus 12.8 KB for the two sounds. Not drawn while the ZOOM is on, during the action cam, or upstairs. *Untested on hardware.*
- **Ghosts.** `die()` calls `fxGhostBorn(why)`: a ghost rises where you fell and stays (3 at most, the oldest goes). Its colour says how it died (plain, shock, gravity, a wall, hunger, worn out). It drifts through walls near its home tile, flickers by day and is solid at night. Close to you it says BOO (mood event `M_SPOOK`, a wail); close to a household member it puts a skull balloon over them. Saved with the life: 12 bytes at SRAM 5188..5199 (`'G'`, count, x y how x3, checksum; the 64 byte life block only used 52). Room slots do not carry ghosts. OPTIONS > SIM > BORES > GHOSTS: OFF / ON / HAUNTED (one is always around, for testing).
- **Weather.** CLEAR, CLOUDY, FOG, RAIN, STORM, SNOW. Nothing is stored: the kind is rolled from the day, the six-hour block of it and the season (the town's `nbT.season`, else the calendar), so a day always has the same weather. Rain and snow are up to 40 sprite particles that land on outdoor tiles only (`wInside`). The room view is dimmed (cloudy, rain, storm) or washed out (fog, snow) with the hardware blend (`BLDCNT` / `BLDY`), half as much when you stand indoors; a storm adds lightning (a brighten flash) and thunder. Outside in rain or snow your mood slowly drops (`M_SOAKED`). The HUD clock shows a small weather sign instead of the sun or moon. OPTIONS > TIME > DAY > WEATHER: AUTO or force one kind. Menus reset the blend registers (`objHideAll`), so nothing dims a menu.
- **Sounds** `thunder.adp` and `ghost.adp` are synthesised by `tools/make_fx_sfx.py` (no recordings) and encoded like every other effect.
- **Seeds for later:** weather could change top speed or grip outdoors (wet ground), make Sims go indoors in a storm, and let the bored ghost possess the radio. See `docs/RAM_AUDIT.md` for the RAM left for them.

## Jobs: PRO SKATER and the normal jobs
- `JT_SKATE` ("PRO SKATER", track 8 in `jobTr`) is the new default job. It is the **only** job whose quota is trick points (`shiftPts` grows with `lscore` while `simInShift()`); the OPTIONS > JOB page's **SKATER QUOTA** scales it.
- Every other track is a normal job with nothing to do with skating: `shiftPts` counts **work minutes** (`simMinute`: +1 while up and about, +2 while STOKED, 0 asleep / washing / sitting / dead). `jobQuotaOf` makes the quota a share of the shift (55% + 3% a level, times the track's `quota` %, +10% in branch B, clamped 30..90%). Pay, strikes and promotions work as before (`simShiftEnd`).
- Promotions want a skill per track (`jobSk`): SKATING for PRO SKATER, a life skill for the rest.
- Save: the track number needs 4 bits. Bits 3-5 of byte 18 keep the low three; the high bit (track 8) is bit 7 of byte 41, the lock mask byte (the mask only uses bits 0-3). Old saves read as before: their track is below 8.

## Lot flags (community and skate spawns) and the intro flyover
- **Two builder items, MISC category:** the COMMUNITY FLAG (`a`, blue) and the SKATE FLAG (`k`, orange). Drawn by code (`drawFlag`, items.h), no baked sprite, walkable, up to 4 of each count (`flgN / flgX / flgY`, found by `mapScan`). They are saved as ordinary map tiles, so older saves are untouched.
- **Town build only.** Flags can only be placed while the town view's BUILD has a free lot open (`nbFlagOk`): not in your home, not in BUILD ROOM or EDIT MAP during play ("FLAGS ARE BUILT FROM THE TOWN"). They only draw and act on a community lot (`nbFlagsOn`), so normal play never sees them.
- **They spawn the lot's crowd.** Visitors walk in from and out to a community flag (`twFar`, house.h; without a flag the old lot-edge exits are used). Each skate flag spawns one AI skater, up to 3 (`npcSkSpawn`, npc.h), even on a lot with fewer than NPC_PARK things to skate (it still needs at least one).
- **They set what kind of place the lot is.** After BUILD in the town view (`nbFlagSync`, neighborhood.h): skate flags = SKATE PARK, community flags = a community lot (a skate park turns into a PARK, any other kind stays), both = the new **PARK + SKATE** type (`CT_BOTH`). A free residential lot with flags becomes a community lot; your home and lots where a household lives keep their kind (the flags are decoration there). No flags: nothing changes. New PARK, SKATE PARK and PARK + SKATE lots start with their flags in the corner.
- **Intro flyover** (`introFly`, goals.h): when a lot opens the camera pans to the goals still open on it (hidden tape, first and last SKATE letter), then back to you. A, B or START skips it. It plays once per lot per session (`flySeen`, reset at power on). The flag items only show in the palette during town BUILD (`catCnt`).


## Mood portrait = the Sim's own face
`hudFaceDraw()` (source/hudface.h, included by hud.h) paints the creator's eye and mouth sprites (`spr[]`) onto a head in the controlled Sim's skin tone, with their hair, iris colour, brows, glasses, nose and cheeks. Each mood swaps the expression (`hudExpr`: eye sprite, mouth sprite, brows, tears): SAD = CUTE eyes, SAD brows, FROWN and tears; BORED = SLEEPY + FLAT; OK = ROUND + FLAT; HAPPY = HAPPY eyes + SMILE; STOKED = WIDE + GRIN. Edit one row of `hudExpr` to change a mood's face. The face sliders (EYE SIZE, SPACING, HEIGHT, MOUTH WIDTH, HEIGHT, BROW HEIGHT, NOSE HEIGHT and the master controller's double sliders) apply too: the sprites are sampled through the same scale and shift as `drawDeco` (`hudRng` / `hudPick`), with shifts rescaled from the block's 7x6 / 15x6 px footprints to the portrait's 9x8 / 19x8 art. Redrawn when the mood, Sim or look changes (`hudFaceKey`). The old 7x7 smiley (`faceArt` / `drawFace`) is gone.

## BROAD and SPIDER
**BROAD** is a four block wide torso with thick arms on **two** thick legs (teen and up; the legs and arms are thickened when drawn, `shpDraw` row 1). The old BROAD, whose torso stood on four thin legs side by side, is kept as **SPIDER** (shape 29, `SH_SPIDER`): it is only on offer with the debug code (the title's Konami code, `sUnlock`, like BIG HEAD). A person saved as BROAD now loads with the new body.

## Item use: household Sims and the home / sound pack (`source/house.h`, OPTIONS > SIM > BORES > SIMS USE ITEMS)
Built as ten small patch modules (`bore-items.zip`, one `apply.sh`); each leaves a marker comment `item module N` in the source. Every behaviour is behind `xo[XO_ITEMUSE]`: OFF gives exactly the old game (the five basic furniture needs, a wander for FUN).
- **The table (`iuT[]`, ROM).** One row per item: the tile char, the need it refills, how many steps a use takes, whether it needs a seat (sofa or beanbag) on the floor, what it does to the OTHER needs (`dn[]`), and the lowest value each need may have before a Sim will choose it (`gate[]`). To add an item: one enum name before `IU_N`, one row, one entry in `iuIc[]` and `iuTag[]`, and a weight in `iuWeight()`.
- **Which item.** `iuPick` takes the items for a need that are on the floor and usable (gates, seat, daytime for coffee, someone to call for the phone, not taken by another Sim, not the same thing as last time), then chooses by personality (`iuWeight`: ACTIVE runs, quiet reads, PLAYFUL and OUTGOING turn the music up, NICE feeds the fish, OUTGOING phones).
- **FUN** = TV, bookshelf, aquarium, treadmill, stereo (also a hobby now and then when nothing is pressing). **REST in the daytime** (tired, not wrecked) = the coffee maker first. **SOCIAL** = sometimes the phone: the call goes to a member who is on another floor, both get less lonely and like each other a little more. An item on another floor is reached over the stairs (SIMS ON FLOORS): `hhItemGo`, `iuFloorFor`, and the coarse offscreen step in `hhOffStep`.
- **Need chains (`iuMove`).** Items: a run costs FOOD, REST and CLEAN, coffee fills the bladder, TV on a sofa is comfy. Basics (`iuBc[]`): eating fills the bladder, a night's sleep leaves a Sim hungry, a shower feels good, a sit-down is a little fun. So one need leads to the next.
- **Showing it.** A balloon over the Sim (`iuIc[]`), `NAME IS WATCHING TV` when it is within 8 tiles of you, and a short tag after the name in the household menu (`iuTag[]`).
- **Stereo and fish (modules 12, 13).** While a Sim has the stereo on, the others awake on the floor gain FUN. Sims share your fish-fed flag: hungry fish make the aquarium far more attractive, and a Sim feeding them counts as fed today.
- **Memory.** Two bytes in each `HhSim` (`item`, `ilast`), one byte per floor (`hhCenI`), the table in ROM. Nothing is saved: after a load everyone starts fresh.
- **Not done:** Sims do not gain skills from the items (skills.h comes after house.h and the household has no skill table), Sims on a floor you are not on only use items by the trip logic (they are not drawn).

## Floors: switching to a Sim on another floor (phase 1 fix)
With SIMS ON FLOORS on, a Sim parked on another floor (the household list says GROUND / FLOOR 2 / FLOOR 3) can now be played: SELECT picks the next Sim on your floor and, when nobody else is here, one on another floor; the household menu (WHO DO YOU PLAY) lets you pick any of them. You change floor first (`flGo`; if the floors do not fit in flPool nothing changes and it says TOO MUCH BUILT TO CHANGE FLOOR) and arrive on the matching stairs, like climbing them. The Sim you leave stays on the floor you left. Sims out at work or school, and the prisoner, are still out (`hhOnOtherFloor`, house.h). Switching while FLOOR PEEK is on ends the peek first.


## Floors step 9: call the household, and work on time
- **R next to the stairs** (nothing else in reach, SIMS ON FLOORS on) shouts for the household: every Sim parked on another floor comes to your floor and steps out of the stairs (`hhCallFloors`, house.h). The note says CALLED SOMEONE DOWN, CALLED THE HOUSEHOLD or NOBODY ON OTHER FLOORS. No new RAM.
- **Work and school on time:** a Sim parked on another floor now checks its schedule too (`hhTick`, the parked branch). Before, it only left once it had drifted down to your floor.
