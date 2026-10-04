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
Boot goes title -> **main menu** (PLAY, MAKE CREATURE, BUILD ROOM, JUKEBOX, SETTINGS, HOW TO PLAY). "MAIN MENU" is the last entry in the creature part list and in the pause menu.

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
- **TREE SWAYING ACTION** is the ambient version (`make_tree_eno`, xm2gba.py): a semitone down and Eno-like, all generated from the song's own pad,
  pluck and bass.
  - Swells, wandering 2-4 note phrases that sometimes come back changed, rare bass and bells, chords that drift.
  - About 11 minutes, then a 1.5-minute fade.
  - Every note has a **reverb trail** of quieter, spaced repeats. An allocator gives each note the channel whose tail has died away, using the real
    sample envelopes; only 12 of 1,213 notes take over a tail louder than -30 dB.
- **TREE SWAYING ACTION (ORIGINAL)**, hidden, is the drum rework. Its breeze pad uses a smooth echo in the calm parts and a gated stutter echo in the
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

**Menu music.** Whenever a main menu is open one random checked song plays (OPTIONS > AUDIO > MENU MUSIC, on by default). It carries on through the quiet screens (OPTIONS, ROOM SLOTS, HOW TO PLAY) and stops when PLAY, MAKE CREATURE, BUILD ROOM or the jukebox opens; back at the menu a NEW random song starts. When a song ends, another random one follows.

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
- **DONE tab > RANDOMIZE** (the Create-A-Sim dice): a whole new look, star sign and aspiration, only from what this life stage and your unlocked parts allow. Press it again for another.
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

## Floors (three per house)
A house has **three floors**. The floor you are on is the live map; the other two wait in memory (`flBuf`, `flGo` in `source/main.c`). Stairs are two items: **STAIRS UP** (`^`) and **STAIRS DOWN** (`~`), at the end of the item list. Step on `^` to go up and on `~` to come down; step off and on again to use them once more. If the other floor has no matching stairs yet, they appear where you came from. In the map editor **SELECT + UP / DOWN** changes the floor you are building on. Upstairs the household waits on the ground floor (Sims do not use stairs yet). The room kept in SRAM, and a ROOM slot, always hold the ground floor.
**Saving:** in ROOM SLOTS an empty slot offers **SAVE HOUSE** (all three floors, 3 or 4 slots in a row, KIND 1 in `source/slots.h`); on a house slot **LOAD ALL** loads it (you start on the ground floor), **SAVE HOUSE** overwrites it. Floors upstairs that are not saved to a house slot are lost when the console is switched off. Loading a single room slot replaces the house with one floor (the upper floors are emptied).

## Sim filter (OPTIONS > PLAY)
Three on/off rows decide which kinds of Sims may be added: **PRE-MADE SIMS** (the MOVE IN families), **USER-MADE SIMS** (made in the creator, added with ADD TO FAMILY) and **MADE-UP SIMS** (INVITE A NEW SIM, SELECT on the RELATIONSHIPS screen, and the passers-by). All are ON by default, so nothing changes until you switch one off. Turning a kind off only blocks adding more of it (with a short message): Sims already in the household stay and play as before.

## Loading screen
`source/loading.h` gives `ldShow("MESSAGE", done, total)`: a full loading screen with a progress bar and percent, one frame per call. It runs while the household is baked (entering the life game, moving in, growing up: one step per Sim) and while you move between lots or towns in the neighborhood. To use it in a slow job, call it between the steps with the number of steps done so far. It cannot move *inside* one flash write, so a single big step still holds the bar for that long.

## Loading screen fix
`ldShow` now switches the display window off while it is on screen (the game runs with window 0 on, and until the first game frame sets the window registers everything outside it is black, which hid the loading screen and made the game look frozen). `ldEnd()` puts the mode back; `hhBakeAll` calls it when the last Sim is baked. If you add `ldShow` to another slow job that runs in the game, call `ldEnd()` when it is done.
