# Actual DS ROM persistence check

This small libretro frontend runs the compiled `godguy_hw.nds` ARM9/ARM7
program in DeSmuME. It drives touch calibration, the title, the opening
film, the first chapter, painting an island, explicit saving, SAVE & TITLE,
same-session Continue, and a completely new emulator process. The new
process loads the cartridge `.dsv` and runs a further 6,000 frames so that
autosave can be checked. No emulator save state is used by this test.

## Run

Requirements: Linux/POSIX, a C compiler, Python 3, a DeSmuME libretro core,
and that core's `libretro.h`. The core, ROM, header, and any BIOS are not
bundled here. The frontend selects DeSmuME's built-in firmware/BIOS and
software rendering; no external BIOS file is needed for this scenario.

From the `godguy` directory, provide existing local paths and a **new**
output directory:

```sh
python3 tools/ds_emu/run_persistence.py \
  --core /absolute/path/desmume_libretro.so \
  --include /absolute/path/desmume/desmume/src/libretro-common/include \
  --rom /absolute/path/godguy_hw.nds \
  --output /absolute/path/new-ds-evidence
```

The runner builds the frontend into the evidence directory, then starts
two separate core processes. It waits for each scripted step to complete
before taking save snapshots. It refuses to reuse an existing evidence
directory so a prior save cannot make a fresh-save test pass accidentally.

The tested core source is
[`libretro/desmume` at `95b4d798731caa809125b6c3c11d17cc332ff6ef`](https://github.com/libretro/desmume/tree/95b4d798731caa809125b6c3c11d17cc332ff6ef),
using its headless software renderer and JIT. The test build disables
OpenGL. The source's no-PCAP build required `defined(NO_PCAP)` to be added
to the existing platform condition near the top of `wifi.cpp`, selecting
its existing PCAP stubs; this does not change emulated game logic.
Use `GG_EMU_CPU=interpreter` to select the slower interpreter when debugging
a core/JIT issue.

## Evidence and acceptance

`runtime_acceptance.json` records the ROM, core, and frontend hashes; the
decoded archive identity; and video/audio activity. The runner checks:

- A nonempty, valid compressed GodGuy archive is written by the game.
- The saved first-chapter world contains the scripted power use.
- Cold Continue leaves the persisted archive unchanged before autosave.
- The same world seed and chapter/progress survive the process restart.
- The live world advances and writes a later autosave.
- Both soak intervals produce changing video and nonzero stereo audio.
- Both screens retain their native combined 256 × 384 dimensions.
- The prior stuck WRITE ENABLE (`06`) error does not recur.
- No emulator `.state` file is present.

Review these captures as well: `first_painted_world.ppm` should show the
new island and objective 1/6; `first_save_complete.ppm` should show SAVED;
`first_saved_title.ppm` and `restart_cold_boot_title.ppm` should offer
Continue; `restart_cold_reloaded_world.ppm` should retain the island and
progress; `restart_live_soak_second.ppm` should show an active world.
The JSON explicitly leaves visual review to the reviewer.

Each session also writes a WAV recording, a step-by-step JSONL log, an
emulator log, and PPM screenshots. The runner retains the archive before
restart and immediately after cold Continue, so persistence is reviewable
without relying on screenshots alone.

This scenario checks the direct-hardware **DS-mode** ROM. It does not
validate enhanced DSi mode, New3DS hardware, all cartridge devices, or an
entire campaign playthrough. Emulator speed and the time taken by the host
are not measurements of physical console frame rate.

## Input format

The frontend accepts `run FRAMES LABEL BUTTON_MASK TOUCH_X TOUCH_Y` and
`quit`. Button masks are hexadecimal libretro joypad bits: A `100`,
B `1`, START `8`, UP `10`, DOWN `20`, LEFT `40`, RIGHT `80`.
Touch coordinates are native bottom-screen pixels; `-1 -1` releases touch.
The generic frontend also has debugging state commands, but the
persistence runner rejects them and the supplied scripts never use them.
