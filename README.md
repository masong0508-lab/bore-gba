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
Boot goes title -> **main menu** (PLAY, MAKE CREATURE, BUILD ROOM, SETTINGS, HOW TO PLAY). "MAIN MENU" is the last entry in the creature part list and in the pause menu.

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
The title screen plays "The Dipper Man" (tools/the_dipper_man.xm). `python3 tools/xm2gba.py` converts the XM to `source/musicdata.h`: note events per pattern plus 16 small instrument samples (8-bit, band-limited, down-sampled to the lowest rate that keeps them clean, about 94 KB in the ROM). A 10-voice mixer with linear interpolation plays it through Direct Sound B at 18157 Hz (exactly 304 samples per frame). The 7.7 s intro plays once, then the song loops from order 4; voices are never cut at the jump, so the last notes ring into the first ones. Music stops when you press START.
