# BORE

A Game Boy Advance game: a **voxel creature creator** that is growing into a **life sim** (Sims-style rooms, a household, needs, a career, skate tricks). Pseudo-3D isometric view, everything drawn by the CPU, no libraries.

## Build and run
- **GitHub:** push; the *Build GBA ROM* workflow makes `bore.gba` (download it from the run's artifacts; tag a commit to get a release).
- **Locally:** install devkitARM, then `make`. Play `bore.gba` in mGBA or on a flash cart.

## Playing
Power on, then START on the title screen. The **main menu** has:

| Entry | What it does |
|---|---|
| PLAY | live in your room: needs, wants, job, skating |
| NEIGHBORHOOD | choose a town (or make one), then pick a lot to play, build or move into; paint land, lay roads, plant trees, add lots (START = town menu) |
| MAKE CREATURE | build a voxel person (body, face, hair, clothes, parts, life stage) |
| BUILD ROOM | draw walls, floors, wallpaper and items |
| ROOM SLOTS | save and load rooms, people and lives |
| JUKEBOX | the Music Player (below) |
| OPTIONS | speed, gameplay, sound, buttons, and more |
| HOW TO PLAY | in-game help for every screen |

Every screen shows its own button hints. D-pad moves, **A** picks, **B** goes back.

## Music
- **JUKEBOX** is a Music Player: a NOW PLAYING card (song, artist, elapsed / total time, progress bar) above one song list that shows every song's **length**. A plays a song (A on the playing song stops it), **L / R** skip to the previous / next song, SELECT checks or unchecks a song (only checked songs are picked at random), **START** changes the play mode (*Shuffle*, *In order*, *Repeat*; saved), LEFT / RIGHT change the volume. Opening it plays **one random checked song**.
- **GOTTCHO BARRACHO plays in the main menus** (OPTIONS > AUDIO > MENU MUSIC); opening the jukebox crossfades into its first song, after that every jukebox track starts directly with no crossfade. A random checked song still plays for GAME MUSIC while playing.
- **Add a tracker song:** put the `.xm` in `tools/`, add a `SONG_XM(...)` line to `source/songs.h` (and a name in `source/artists.h`), run `python3 tools/xm2gba.py` (needs numpy and scipy), commit `source/musicdata.h` and `source/music/xmdata.bin`.
- **Add a streamed song:** `python3 tools/encode_song.py "song.mp3"` (needs ffmpeg).
- Songs are listed in `source/songs.h` (up to 64). Songs added at the end keep everyone's checkmarks.
- **Creator music** is 12 chiptune loops the game plays **live** from note data (`tools/chip_synth.py`; 152 KB instead of 1.5 MB of recordings).
- **TREE SWAYING ACTION** is now a 13-minute ambient piece; the drum version is the hidden TREE SWAYING ACTION (ORIGINAL).
- **New:** CONDENSED MUSIC by Sk9m, a UK garage rework of The Dipper Man - Condensed Music (see docs/NOTES.md).
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

**New item:** the DeadSet 3Thousand VYBE (5000), a parody VR headset on a display bust. Stand next to it and press R to jack in.

**Saves:** 128 KB flash (58 room slots). Carts without flash fall back to 32 KB SRAM (12 slots). Older saves carry over. Layout: `docs/NOTES.md`.
