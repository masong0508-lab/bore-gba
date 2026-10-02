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

**Settings** (FPS savers, saved to SRAM): presets LOOKS / BALANCED / SPEED, frame rate 60/30/20 (game logic always runs at 60 steps/s), walls FULL / CUTAWAY / LOW, wallpaper on/off, floor patterns on/off, sound on/off, SHOW FPS.
