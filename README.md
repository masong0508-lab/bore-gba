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
| MAKE CREATURE | build a voxel person (body, face, hair, clothes, parts, life stage) |
| BUILD ROOM | draw walls, floors, wallpaper and items |
| ROOM SLOTS | save and load rooms, people and lives |
| JUKEBOX | the Music Player (below) |
| OPTIONS | speed, gameplay, sound, buttons, and more |
| HOW TO PLAY | in-game help for every screen |

Every screen shows its own button hints. D-pad moves, **A** picks, **B** goes back.

## Music
- **JUKEBOX** is a two-tab Music Player (**L / R**). *Playlist*: SELECT checks or unchecks songs, and only checked songs are picked at random. *Interactive*: pick any song and play it. Opening it plays **one random checked song**; LEFT / RIGHT change the volume.
- **A random checked song also plays in the main menus** (OPTIONS > AUDIO > MENU MUSIC), and for GAME MUSIC while playing.
- **Add a tracker song:** put the `.xm` in `tools/`, add a `SONG_XM(...)` line to `source/songs.h` (and a name in `source/artists.h`), run `python3 tools/xm2gba.py` (needs numpy and scipy), commit `source/musicdata.h` and `source/music/xmdata.bin`.
- **Add a streamed song:** `python3 tools/encode_song.py "song.mp3"` (needs ffmpeg).
- Songs are listed in `source/songs.h` (up to 64). Songs added at the end keep everyone's checkmarks.

## Where things are
| Path | What |
|---|---|
| `source/main.c` | the game (screens, creator, life, audio mixer) |
| `source/songs.h`, `artists.h`, `musicdata.h` | the song list, artists, converted music |
| `source/music/` | music data (`xmdata.bin`) and streamed songs |
| `tools/` | song converters, music generators, font and art scripts |
| `docs/NOTES.md` | the full technical notes (memory maps, save layout, every feature) |
| `docs/NEXT.md` | what to build next |

Saves live in cartridge SRAM (32 KB); the layout is in `docs/NOTES.md`.
