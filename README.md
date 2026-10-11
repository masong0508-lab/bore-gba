# BORE

A Game Boy Advance game: a **voxel creature creator** that is growing into a **life sim** (Sims-style rooms, a household, needs, a career, skate tricks). Pseudo-3D isometric view, everything drawn by the CPU, no libraries.

## Build and run
- **GitHub:** push; the *Build GBA ROM* workflow makes `bore.gba` (download it from the run's artifacts; tag a commit to get a release).
- **Locally:** install devkitARM, then `make`. Play `bore.gba` in mGBA or on a flash cart.
- **The Creator Demo:** `make creator` builds `bore-creator.gba` (the workflow uploads it too): only the creature creator, like Spore's Creature Creator. Every part and slider is free; save your Bores to **MY CREATIONS** and flip through them, and every Bore has a **SHARE CODE**.

## Share codes
A Bore (its look, age, aspiration and personality) as up to about 72 letters and digits, e.g. `AG9E FK8H W7R8 ...`. The code alphabet has no I, O, 0 or 1, and a mistyped code is refused rather than loaded as some other Bore.
- In the demo: hub > ENTER A SHARE CODE, or a creation's START in MY CREATIONS to see its code.
- In the game: the creator's DONE tab > SHARE CODE / ENTER A CODE (the code's Bore becomes the one on screen), or DONE > HOUSEHOLD > ADD FROM A CODE (the code's Bore joins your household).

## Playing
Power on, then START on the title screen. The **main menu** has:

| Entry | What it does |
|---|---|
| PLAY | live in your room: needs, wants, job, skating |
| NEIGHBORHOOD | choose a town (or make one), then pick a lot to play, build or move into; paint land, lay roads, plant trees, add lots (START = town menu) |
| CREATE A BORE | build a voxel person (body, face, hair, clothes, parts, life stage) |
| BUILD ROOM | draw walls, floors, wallpaper and items |
| ROOM SLOTS | save and load rooms, people and lives |
| JUKEBOX | the Music Player (below) |
| OPTIONS | speed, gameplay, sound, buttons, and more |
| HOW TO PLAY | in-game help for every screen |

Every screen shows its own button hints. D-pad moves, **A** picks, **B** goes back.

In the game: **SELECT + L / R** turns the view a quarter turn left / right and keeps it (like turning the camera in a Sims game; the D-pad still means up the screen). **SELECT + UP / DOWN** zooms. A tap of **SELECT** switches Sim. The ACTION CAM still spins the room after a big combo.

## Building
In the room builder (BUILD ROOM, the pause menu's EDIT MAP, or BUILD on a lot in the town): **SELECT + B is UNDO and SELECT + START is REDO**. Both are also in the MAP MENU (START), which shows how many steps are left. A step is everything between pressing A (or B) and letting go: a whole room, a wall line, a floor fill or a held-down stroke of items. Undo gives back what the step cost and redo charges it again. It keeps the last 3 steps, so you can undo three times (up to 1,000 tiles in all) and is cleared when you leave the builder, reset the map or load a blueprint. Details: `docs/NOTES.md`, *Undo and redo*.

**Community lots** (park, skate park, plaza, lounge, old town) are visited, not built, while you stand in them: the pause menu's EDIT MAP, BUILD ROOM in the main menu, the creator's BUILD button and loading a blueprint over the lot all say no, and the pause panel says VISITING. Build a community lot from the **town view** (A on the lot, then BUILD), like the Sims neighbourhood screen. Any lot that is not your home and has no household can be switched with **MAKE COMMUNITY / MAKE RESIDENTIAL** in the same menu.

## Music
- **JUKEBOX** is a Music Player: a NOW PLAYING card (song, artist, elapsed / total time, progress bar) above one song list that shows every song's **length**. A plays a song (A on the playing song stops it), **L / R** skip to the previous / next song, SELECT checks or unchecks a song (only checked songs are picked at random), **START** changes the play mode (*Shuffle*, *In order*, *Repeat*; saved), LEFT / RIGHT change the volume. Opening it plays **one random checked song**.
- **GOTTCHO BARRACHO plays in the main menus** (OPTIONS > AUDIO > MENU MUSIC); opening the jukebox crossfades into its first song, after that every jukebox track starts directly with no crossfade. A random checked song still plays for GAME MUSIC while playing.
- **Add a tracker song:** put the `.xm` in `tools/`, add a `SONG_XM(...)` line to `source/songs.h` (and a name in `source/artists.h`), run `python3 tools/xm2gba.py` (needs numpy and scipy), commit `source/musicdata.h` and `source/music/xmdata.bin`.
- **Add a streamed song:** `python3 tools/encode_song.py "song.mp3"` (needs ffmpeg).
- Songs are listed in `source/songs.h` (up to 64). Songs added at the end keep everyone's checkmarks.
- **Creator music** is 12 chiptune loops the game plays **live** from note data (`tools/chip_synth.py`; 152 KB instead of 1.5 MB of recordings).
- **TREE-AGE IN ACTION** (renamed from TREE SWAYING ACTION) is a 13-minute ambient piece; the drum version is the hidden TREE-AGE IN ACTION (ORIGINAL).
- **New:** CONDENSED MUSIC by Sk9m (a UK garage rework of The Dipper Man - Condensed Music), COCAINE COLA (its original hidden) and STAGED THE FULL PERFORMANCE by DayBar, a hi-NRG rework (see docs/NOTES.md).
- **Locked and hidden songs:** WORTHLESS CLOUDS starts locked and unlocks for good when a Sim meets their **lifetime want**. Songs named `... (ORIGINAL)` stay hidden. The title code (UP UP DOWN DOWN LEFT LEFT RIGHT B A START) shows both. To lock another song, add one line to `source/unlocks.h`.

## Where things are
| Path | What |
|---|---|
| `source/main.c` | the game (screens, creator, life, audio mixer) |
| `source/songs.h`, `artists.h`, `musicdata.h` | the song list, artists, converted music |
| `source/music/` | music data (`xmdata.bin`) and streamed songs |
| `tools/` | song converters, music generators, font and art scripts |
| `docs/NOTES.md` | the full technical notes (memory maps, save layout, every feature) |
| `docs/NEXT.md` | what to build next |

**The commercial:** `assets/trailer/bore_commercial.mp4`, a 65-second late-night TV spot cut from the game itself (`tools/make_trailer.py`; `tools/trailer/README.md` has the pipeline). Rated M for munchies.

**THE CATALOG:** 200 more things to buy, in 18 new BUY tabs (chairs, sofas, beds, tables, desks, storage, lights, plants, kitchen, bath, tech, art, outdoor, kids, skate 2, grown-ups, seasonal, misc), from a 60 oak chair to a 5000 gold toilet. Each is a new look on a thing the game already knows how to use, so it is sat on, slept in, eaten from and saved like the rest. Generated houses and the town's venues wear them too. Details: `docs/NOTES.md`, *THE CATALOG*.

**New items:** a RADIO and a SOUND SYSTEM (R next to one tunes a station by genre (HOUSE & DANCE, BREAKS & BASS, CHILL & WORLD, LATIN, ROCK & PROG, HIP HOP, or ALL SONGS; genres live in source/genres.h); see docs/NOTES.md, Sound pack). Older: the DeadSet 3Thousand VYBE (5000), a parody VR headset on a display bust. Stand next to it and press R to jack in.

**TV:** R at the TV changes channel. MACHINI-TV is the first one and plays a 3 second clip (drawn by code, no video in the ROM; A or B skips it). More channels get their own clips later (source/tvclip.h).

**Wants:** wants met soon after each other make a **STREAK** (5 more points each, up to +20); a want nobody meets for about 3 minutes is swapped for a new one unless you locked it; there are four lifetime wants per aspiration (pick yours on the ASPIRE tab) and new wants for switch tricks, wall taps, manuals, the radio and 8 trick combos.

**Saves:** 128 KB flash (58 room slots). Carts without flash fall back to 32 KB SRAM (12 slots). Older saves carry over. Layout: `docs/NOTES.md`.

**Lot flags:** in BUILD (MISC) place a **COMMUNITY FLAG** (visitors come and go there) and/or a **SKATE FLAG** (a skater starts there). Building a lot with flags makes it a community lot: skate flags = SKATE PARK, community flags = PARK, both = PARK + SKATE. When a lot opens, the camera now flies over its open goals first (A / B / START skips).

