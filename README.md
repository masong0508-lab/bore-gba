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

## Combos and the action cam
Clean tricks (spins, kickflips) and rail grinds now **chain**: each one adds to the chain and the chain multiplier equals the number of tricks. Land the next trick within 2.5 s (grinding keeps it alive) or the chain banks its bonus (points x (tricks - 1)). A bail or a hit loses the chain. The HUD shows `COMBO X5 2500` while it runs.
When a chain banks for more than the **ACTION CAM** threshold (Settings: OFF / over 10000 / over 5000 / over 2000, default 10000) the game holds still while the camera zooms in 1.3x on the skater and swings through all 4 sides of the room, then eases back. SELECT skips it. For reference, 14 kickflips in a row banks 19600.

## Big map, camera and minimap
The play map is now **40 x 40 tiles** (was 14 x 14), about 8x the floor space. The default map has a house (carpet, lino kitchen, pink-tile bathroom), a brick-and-steel factory with hazard lanes, and a rail park with long rails and crate boxes, joined by concrete roads and ringed by a low wall. The middle of the rail park is an open plaza.
- **Camera**: in play the view follows the skater, eased so it stays steady, and stops at the map edges. The action cam still spins round the skater.
- **Speed**: only the tiles on screen are drawn, so the bigger map costs far less than drawing all 1600 tiles. Use SETTINGS (AUTO TUNE) if your device needs it.
- **Map editor**: a dead-zone camera scrolls only when the cursor nears the edge of the screen, and a **minimap** (top right) shows the whole map, the area on screen and the blinking cursor.
- **Saves**: new maps save in a bigger SRAM block (settings moved to offset 8192). Older 14 x 14 saves and settings still load; an old room is placed into the plaza of the new map and re-saves in the new format.

## Title music
The title screen plays "The Dipper Man" (tools/the_dipper_man.xm). `python3 tools/xm2gba.py` converts the XM (and every other `SONG_XM` song listed in `source/songs.h`) to `source/musicdata.h`: note events per pattern (with per-note volume) plus the instrument samples that are actually used (8-bit, band-limited, down-sampled to the lowest rate that keeps them clean; the title song is about 70 KB in the ROM). A 10-voice mixer with linear interpolation plays it through Direct Sound B at 18157 Hz (exactly 304 samples per frame). The 7.7 s intro plays once, then the song loops from order 4; voices are never cut at the jump, so the last notes ring into the first ones. Music stops when you press START.

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
