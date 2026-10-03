# BORE (tech demo)

GBA voxel creature creator, the seed of a later life sim. Pseudo-3D isometric view, humanoid-friendly build space:
**6 wide × 4 long × 8 high** blocks. Parts: HEAD, TORSO, ARM, LEG, EYE, MOUTH, EAR, HAIR, each in 3 sizes (S/M/L = 1×/2×/3× blocks).

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

**OPTIONS > AGES** (its own page): **AGING** (OFF / SLOW / NORMAL / FAST: slow doubles the days of every stage, fast halves them, off keeps the age you picked) and how many game days each stage lasts: **BABY** (default 2), **CHILD** (3), **TEEN** (3), **ADULT** (7, or FOREVER to never grow old); each is 1, 2, 3, 5, 7, 10, 14, 21, 30 or 60 days. The stage is saved in the person (room slot format 2; format 1 slots load as adults) and also in SRAM at 12416 so growth survives a power cycle.

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
Main menu -> **OPTIONS** (also in the pause menu and the map menu). Eight pages: **VIDEO** (the old settings: preset, auto tune, frame rate, walls, wallpaper, floors, shadows, performance info, ROM speed), **PLAY**, **AGES**, **AUDIO**, **INPUT**, **HUD**, **ROOMS**, **DATA**.
| Key | Action |
|---|---|
| L / R | change page |
| Up / Down | pick a row |
| Left / Right (or A) | change it |
| Select | put the row back to its normal value |
| B / Start | back (everything is saved) |
A gold dot marks a row that is not at its normal value. The row under the cursor explains itself in two lines.
- **PLAY**: NEEDS (off to brutal), FOOD AND WC, DAY LENGTH (3 / 6 / 12 / 24 min or stopped), CAREER on/off (off = no shifts, quota or bills), JOB QUOTA, BILLS, SCORE multiplier (x0.5 to x3), COMBO WINDOW, TOP SPEED (80 to 150 %), MOOD EFFECTS, HURT (normal / gentle / no death), AUTO SAVE LIFE.
- **AUDIO**: SOUND, SFX VOLUME, MUSIC VOLUME, **GAME MUSIC** (off by default: the jukebox songs play in their shuffled order while you play, the next song starts when one ends, it fades to half volume while the pause menu (or anything opened from it) is up, and sound effects play over the music; mixing runs in an interrupt, so it costs some speed on slow devices), TITLE MUSIC, JUKEBOX MODE.
- **INPUT**: BUTTONS (swap A/B, L/R or both, on every screen), CURSOR REPEAT speed of the editor, BUTTON TEST (shows the keys the game sees).
- **HUD**: INFO ON SCREEN, CLOCK (24 h / 12 h / hidden), THOUGHT BUBBLE, WANTS AND FEARS, ACTION CAM, ACCENT COLOUR (gold, mint, sky, pink, orange, lilac), MESSAGE TIME.
- **ROOMS**: EDITOR MINIMAP, SAVE ON EXIT, ASK BEFORE RESET, SLOTS SAVE (room / room + person / all three), ASK IN SLOTS, SAVE MAP TO SLOT, BOOT LOADS PERSON.
- **DATA**: SAVE LIFE NOW, ERASE LIFE, ERASE SAVED MAP, ERASE ALL SLOTS, ERASE EVERYTHING (main menu only, asks twice), SAVE MEMORY TEST (checks the cart or emulator keeps saves), RESET ALL OPTIONS.

**Adding an option** takes four small steps (written at the top of `source/opts.h`): add a name at the *end* of the `XO_` enum, add its choice count and default to `xoCnt[]` / `xoDef[]`, read `xo[XO_X]` where the game uses it, and add an `XR(...)` row to a page table in `source/optscreen.h`. Options are one byte each, saved with a checksum and a range check per value, so an older save simply gets the defaults for options it does not have.

## Room slots
Main menu (or pause menu, or map menu) -> **ROOM SLOTS**. Six named saves; each holds any of a **room** (walls, floors, wallpaper, items), the **person** (the creature, including hand built blocks) and the **life** (needs, cash, job, clock, skill). What a save stores is the SLOTS SAVE option.
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

## Households (up to 8 Sims)
**Pause menu -> HOUSEHOLD** moves in a premade family (original characters: THE GRINDERS, a skater family of three; THE MIDNIGHTS, a pale night-owl family; THE FRESHLYS, a young couple) or moves everyone out. A household is you plus up to 7 more Sims (`source/house.h`).
- **SELECT** (a tap, not SELECT+START) switches who you control: position, needs, look, persona and sprites trade places, and the camera jumps to the new Sim. Hand-built (block builder) creatures cannot switch yet.
- **Free will**: the Sims you do not control look after themselves. Each kind of furniture advertises a need (fridge FOOD, toilet WC, bed REST, shower CLEAN, sofa COMFY) and wandering about gives FUN. A Sim scores them (how low the need is, squared, tilted by its traits: neat Sims shower sooner, lazy ones sit, playful ones roam), picks one of the two best, finds a path (breadth-first search on the 40x40 tiles, one Sim plans per step), walks there and uses it. **OPTIONS > PLAY > FREE WILL**: OFF / LOW (waits until needs are lower, thinks half as often) / HIGH.
- **Hardware sprites**: the other Sims are GBA sprites (OBJ, 32x64, 16 colours each with their own palette), so their moving costs no drawing; the CPU only draws their shadows and talk balloons into the room. Their four views are baked like yours, cut down to 15 colours (closest colours merged, the common ones kept exact), and only the view on show sits in sprite memory (1 KB each, copied in vblank). A window keeps them inside the room view (never over the HUD), menus and other screens hide them. Sprites always sit on top of the picture, so a Sim standing behind a full-height wall is drawn see-through (an x-ray blend) instead of in front of it. You stay drawn by the CPU (furniture in front of you covers you, the action cam can zoom you); SELECT swaps sprites both ways.
- **Social life** (Sims 2 style). A SOCIAL need (HUD bar, a LONELY alert; outgoing Sims get lonely faster). Every pair of Sims has a one-way DAILY and LIFETIME relationship (-100..100): daily changes fast and drifts back to lifetime every game hour, lifetime moves a third as much. Statuses: STRANGER, ACQUAINTANCE, FRIEND (daily 50+), BEST FRIEND (daily and lifetime 70+), DISLIKE, ENEMY (daily -50 or less), and the romance steps CRUSH, IN LOVE, STEADY.
  - **R next to a household Sim** opens the social menu (the furniture you stand at is offered first): TALK, JOKE, COMPLIMENT, HIGH FIVE, HUG, SHOW A TRICK, FLIRT, KISS, GO STEADY, APOLOGIZE, ARGUE, INSULT, SLAP. What is on offer depends on the relationship (a hug needs daily 35, a kiss a crush, going steady being in love), age (romance only teen with teen or adult with adult/elder; no slapping for children) and mood.
  - **Acceptance** = the interaction's base chance + half of how the other feels about you + their matching trait (playful for jokes, nice for compliments and hugs, outgoing for flirts) + their mood; shy Sims are wary of people they hardly know, and a Sim going steady with someone else turns flirts down. Accepted: both like each other more and fill SOCIAL (jokes and tricks also FUN). Rejected: you are embarrassed and like them a little less. Mean ones always land.
  - **Free will socials**: lonely Sims (and idle ones, outgoing ones most) go and see someone: friends, crushes and partners first, strangers to say hello, and you. Grouchy Sims go looking for trouble. What they do follows the relationship: friends joke and hug, crushes flirt and kiss, couples in love ask to go steady, enemies argue and slap. They do it to you too.
  - Balloons over heads show what is said (a word over yours, an icon over theirs), and the note line says what happened ("REX LAUGHED").
  - Wants: TALK TO SOMEONE, MAKE A FRIEND, BEST FRIENDS, FIRST KISS, FALL IN LOVE, GO STEADY, GET A HUG, SHARE A LAUGH. Fears: BEING REJECTED, GETTING SLAPPED, A FIGHT, MAKING AN ENEMY, BEING LONELY.
  - **Pause menu > HOUSEHOLD > RELATIONSHIPS**: how you feel about everyone and how they feel about you, daily and lifetime. A family that moves in already knows each other, and its first two adults are a couple.
- **The thought bubble** only shows when you stand still (nothing flashes over your head while you walk), and by default only for urgent needs (OPTIONS > HUD > THOUGHT BUBBLE: ALL brings the wants back).
- **For now** the aspiration meter, wants, job, cash and skill belong to the household (whoever you control uses them), and the household is saved in SRAM at 18448 (one household, not per room slot).
- RAM: each member's baked sprites are 11 KB (EWRAM), the free will state about 150 bytes a Sim.

## RAM budget (work RAM, not saves)
The GBA has 256 KB of EWRAM and 32 KB of IWRAM. Check the numbers with `arm-none-eabi-size -A` on the object or ELF: `.sbss` is EWRAM, `.bss` + `.data` + `.iwram` are IWRAM (the stack shares what is left of IWRAM).
| | EWRAM | IWRAM |
|---|---|---|
| before the audio rework | 237,000 B (90%) | 24,436 B |
| after the audio rework | 113,000 B (43%) | 24,812 B |
| with the 8-Sim household | 211,180 B (81%) | 25,452 B |
| household on hardware sprites | 168,348 B (64%) | 25,096 B |
| big users now | household sprite tiles `hhObj` 28 KB + bake buffer 11 KB, screen back buffer `fb` 76.8 KB, creature sprites `spr4` 11 KB, floor tiles `flTab` 8.6 KB, overlay `ovBuf` 5 KB, BFS queue + wall map 4.8 KB (wallpaper textures: ROM only) | mixer buffers, `irqStack` 1 KB, IWRAM code 14 KB |

**Audio driver.** Sound effects used to be decoded whole into a 124 KB buffer (the longest clip is 15 s) and played on their own, pausing the music. Now an effect is one more voice in the interrupt-driven music mixer: it is decoded a few samples at a time straight from the ROM and resampled from 6554 Hz to the mixer's 18157 Hz (`sfxMix`), so it needs no buffer and plays over the game music. When nothing plays, the mixer switches itself off (`audStart` / `audStop`). The title screen used to borrow that buffer for a whole-screen copy of its backdrop; it now keeps only the two areas it repaints (the smoke and PRESS START), about 10 KB, inside `spr4` before any sprite is baked.

**Room for more characters.** One baked character (4 views of 32 x 44 at 16 bits) is 11 KB, so the freed 124 KB holds about ten more at that size, or around twenty at 8 bits per pixel with a palette.

## Save memory map (32 KB SRAM)
| Offset | What |
|---|---|
| 0 | the room, "BM3" (4803 bytes; older 14 x 14 saves still load) |
| 8192 | settings (16 bytes) |
| 8448 | extended options (`opts.h`) |
| 12288 | jukebox order and mode |
| 12352 | active room slot |
| 12416 | life stage and days in it |
| 12432 | persona: aspiration, lifetime want, traits, DNA, unlocked parts |
| 16384 | the life (`sims.h`) |
| 18432 | 16 spare bytes for SAVE MEMORY TEST |
| 18448 | the household (up to 7 more Sims, `house.h`) |
| 20480 | six room slots of 2048 bytes (to the end of SRAM) |

## Combos and the action cam
Clean tricks (spins, kickflips) and rail grinds now **chain**: each one adds to the chain and the chain multiplier equals the number of tricks. Land the next trick within 2.5 s (grinding keeps it alive) or the chain banks its bonus (points x (tricks - 1)). A bail or a hit loses the chain. The HUD shows `COMBO X5 2500` while it runs.
When a chain banks for more than the **ACTION CAM** threshold (Settings: OFF / over 10000 / over 5000 / over 2000, default 10000) the game holds still while the camera zooms in 1.3x on the skater and swings through all 4 sides of the room, then eases back. SELECT skips it. For reference, 14 kickflips in a row banks 19600.

## Big map, camera and minimap
The play map is now **40 x 40 tiles** (was 14 x 14), about 8x the floor space. The default map has a house (carpet, lino kitchen, pink-tile bathroom), a brick-and-steel factory with hazard lanes, and a rail park with long rails and crate boxes, joined by concrete roads and ringed by a low wall. The middle of the rail park is an open plaza.
- **Camera**: in play the view follows the skater, eased so it stays steady, and stops at the map edges. The action cam still spins round the skater.
- **Speed**: only the tiles on screen are drawn, so the bigger map costs far less than drawing all 1600 tiles. Use SETTINGS (AUTO TUNE) if your device needs it.
- **Map editor**: a dead-zone camera scrolls only when the cursor nears the edge of the screen, and a **minimap** (top right) shows the whole map, the area on screen and the blinking cursor.
- **Saves**: new maps save in a bigger SRAM block (settings moved to offset 8192). Older 14 x 14 saves and settings still load; an old room is placed into the plaza of the new map and re-saves in the new format.

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

## Title music
The title screen plays "The Dipper Man" (tools/the_dipper_man.xm). `python3 tools/xm2gba.py` converts the XM (and every other `SONG_XM` song listed in `source/songs.h`) to `source/musicdata.h`: note events per pattern (with per-note volume) plus the instrument samples that are actually used (8-bit, band-limited, down-sampled to the lowest rate that keeps them clean; the title song is about 70 KB in the ROM). A 10-voice mixer with linear interpolation plays it through Direct Sound B at 18157 Hz (exactly 304 samples per frame). The 7.7 s intro plays once, then the song loops from order 4; voices are never cut at the jump, so the last notes ring into the first ones. Music stops when you press START.

**Sunman Sunrise**: `tools/make_sunman_rework.py` builds `tools/sunman_sunrise.xm`, a sunrise nu-disco rework of "The Dipper Man - Sunman" (the original is kept as `tools/the_dipper_man_sunman.xm`). The hook (both voices, note for note, an octave lower), its rhythm, the walking bass and the counter-line are the original's; the harmony (D/F# Gmaj7 Bbmaj7/A C | D C Bbmaj7#11 Gmaj7), the arrangement (bell intro, four-on-the-floor groove with pumping pads, arp, breakdown, last chorus a whole tone up) and every sound are new. 116 BPM, 3:04, 12 channels. Re-run the script, then `python3 tools/xm2gba.py`.

**An Ode to Mexicode**: `tools/make_mexicode_rework.py` builds `tools/ode_to_mexicode.xm`, a soft, wide rework of "The Dipper Man - An Ode to Mexicode" (the original is kept as `tools/the_dipper_man_ode_to_mexicode.xm`). It keeps the nylon stab figure, the F Dorian progression and its Db bridge, the bass riff, the melody, both counter-lines and the ending on F major from the original. New: a 12/8 bembe groove, a breathy flute with a ping-pong echo on its own channel, pads that breathe on every dotted beat, marimba, vibes and glass bells. It uses all 16 mixer voices, each one hand-panned (`mexicode_pan` in xm2gba.py). 112 BPM, 2:27. It is part one of a pair: the sequel, "An Ode to the Spanish Flexicode", will mirror its frame (12/8, the nylon figure, and the bell call F Ab C Eb G).

**Meltdown in Mars (90s house mix)**: `tools/make_meltdown_house.py` builds `tools/meltdown_in_mars_house.xm` (126 BPM, about 6 minutes, 10 channels, all sounds synthesised), it is listed in `source/songs.h`, and `python3 tools/xm2gba.py` bakes it into `source/musicdata.h`. `python3 tools/preview_xm.py meltdown_in_mars_house tools/meltdown_in_mars_house.xm out.wav` renders it the way the GBA mixer will play it.

## Jukebox
Main menu -> **JUKEBOX**. Opening it starts the song the playlist is on. The song list lives in `source/songs.h`.
| Key | Action |
|---|---|
| Up / Down | move the cursor through the playlist |
| A | play the song under the cursor |
| L / R | previous / next song |
| Start | stop, or play the song under the cursor |
| Left / Right | mode: SHUFFLE, IN ORDER, REPEAT ONE |
| Select | re-roll the shuffle (saved; the playing song stays first) |
| B | back to the menu |

When a song ends the next one starts (REPEAT ONE replays it). **The shuffled order is saved in SRAM (offset 12288), so the song set comes back in the same shuffled order every time the game starts**, and the playlist carries on from the last song played. It is re-rolled only by SELECT, or automatically when the number of songs in `songs.h` changes. The mode is a normal setting (SETTINGS -> JUKEBOX).

**Adding a tracker song (.xm):** copy it into `tools/`, add `SONG_XM(my_id,"MY SONG","tools/my_song.xm")` to `source/songs.h`, then run `python3 tools/xm2gba.py` (needs numpy + scipy) and commit the new `source/musicdata.h`. Tracker songs are tiny (tens of KB). The player handles up to 10 channels, 32 instruments, any pattern length, notes and the volume column; it ignores effects, panning, envelopes and note-off (the script warns if a song uses them). Speed/BPM must stay fixed in the song. `GAIN` in `tools/xm2gba.py` sets a song's loudness.

**Adding streamed songs:** `python3 tools/encode_song.py "my song.mp3"` (needs ffmpeg + numpy). It writes `source/music/<id>.adp` and adds a `SONG_ADP(...)` line to `source/songs.h`. Song spec: 4-bit IMA-ADPCM, mono, 18157 Hz, about 9 KB per second, up to 32 songs. Song titles use capitals, digits and spaces (the font has no punctuation). The three `PLACEHOLDER` songs and the tracker songs (`SONG_XM`) are there so shuffle can be heard from day one: delete the placeholder lines in `songs.h` and the files in `source/music/` when you add real songs.
The jukebox plays only on its own screen for now: music during gameplay needs a vblank interrupt (game frames can run longer than a sound buffer).


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
- **CLOTHES tab**: TOP and BOTTOM colours, **TOP STYLE** (TEE, LONG SLEEVE, TANK, HOODIE with a hood behind the head), **BOTTOM STYLE** (PANTS, SHORTS, SKIRT) and **SHOES** (as the bottom, white, black, red, gold, as the top).
- Hats and the new hairdos add STYLE; a helmet adds STAMINA.
- **DONE tab > RANDOMIZE** (the Create-A-Sim dice): a whole new look, star sign and aspiration, only from what this life stage and your unlocked parts allow. Press it again for another.
- Saved as person format 5 (older slots still load; the new looks start at their first option), households as 'H3' (an 'H2' household still loads).

### Tab 5: PARTS (Spore style)
Like the Spore creature editor, the body decides what the creature can do. Parts are built as blocks on the model:
| Part | Options | Power |
|---|---|---|
| TAIL | NONE, STUB, LONG (furry, hair colour) | LONG = **BALANCE**: spins land clean further off straight |
| HORNS | NONE, NUBS, HORNS (ivory, out of the sides of the head) | HORNS = **CHARGE**: skating into a wall does not hurt |
| BACK | NONE, SPIKES, WINGS | SPIKES = **ARMOUR** (falls and bails hurt 30% less), WINGS = **GLIDE** (hold R in the air to float down) |

Under the rows is the **ability chart**: SPEED, JUMP, GRIP, STYLE, STAMINA, 0 to 5 each (2 is normal). Shape, face, hair and parts move them (TALL is fast, BROAD tough, BIG HEAD stylish, bald is quick, wings help jumps but drag, a tail helps grip...). In play: SPEED +-5% top speed per point, JUMP +-6% ollie and hop, GRIP more grind points and faster rails, STYLE +-6% trick points, STAMINA -8% need drain per point. All of it is in `abOf()` / `abPow()` in `main.c`.

**DNA.** Big parts (LONG tail 60, HORNS 60, SPIKES 40, WINGS 120) are locked until bought with DNA. You can still look at a locked part (red, with a padlock): A buys it, and it comes off again when you leave the creator if you did not. DNA is earned by living: a met want pays its points, a skill level 15, a promotion 25, a birthday 50, the lifetime want 200. The Konami code makes every part free.

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

**Saving.** Needs, cash, the aspiration meter and reward points, the wants and fears (and the lock), clock, job level and progress, skill and the lifetime want counters are saved to SRAM (offset 16384, 52 bytes "SIM3"; an older 24 byte "SIM2" life still loads, its points become reward points) at every midnight, every payday, when you open the pause menu and when you leave the life game, with a checksum so a bad save is ignored. Dying only resets the needs: the life goes on. **Pause menu -> NEW LIFE** erases it and starts over.

**Adding things.** A want or fear: add an `SE_` name if it needs a new event, a row **at the end** of `simWants` / `simFears` (rows are saved by index: name, event, points, furniture, icon, parameter, aspirations, trait, minimum, who), and map the game event in `simsMood()` (or call `simEvent(SE_X)` yourself). A need: a variable (and in `simsSave`/`simsLoad`), a rate in `simsTick()`, a use in `simBegin()`, a bar in `simsHud()`. A piece of furniture: art in `simart.h`, a slot in `palCh`/`palNm`/`palCol` and a `drawItemTile` line.

**Not done yet:** SOCIAL need, other people to talk to, relationships.
