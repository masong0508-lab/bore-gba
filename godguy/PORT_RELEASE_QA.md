# GodGuy — Console port verification and release gates

Updated 21 November 2026. The New 3DS world and DS/DSi improvements from
#58, #67, #68 and #69 are integrated into `main` through #71, merge
`e0dbfe8a91b7213be1df3058ab3476c5618ee407`. The build workflows are
**not** hardware certification.

## Which binary is which

| File | Hardware / role | Automated checks | Required manual gate |
| --- | --- | --- | --- |
| `godguy.3dsx` | New Nintendo 3DS, New 3DS XL, New 2DS XL; playable software 3D world | devkitARM/libctru link, world/camera/cinematics/audio, host capture | New hardware startup, playable movement, touch, saves, film, music, stress |
| `godguy_hw.nds` | DS and DS-mode emulators/flashcarts, direct-hardware ARM9/ARM7 | real ARM SDK link, voice-bank integrity, zoom/simulation | Emulator startup, sound, cartridge-save handling or save states |
| `godguy_dsi.nds` | DSi in **DS-compatibility mode**, not 16 MiB DSi mode | full ROM, identical DS voice-bank integrity | On-device DSi DS-mode gameplay/save behavior |
| `godguy.nds` | Optional libnds/Calico DS build, conditional CI | compile when available; packaged only if build succeeded | melonDS/flashcart boot and SD save/reload |
| `godguy_dsi_mode.nds` | Experimental DSi 16 MiB extended mode | ARM9 image and homebrew DSi header validation | Dedicated DSi-mode loader test, title startup, memory, sound, input |
| `godguy_new3ds_gpu_probe.3dsx` | **Separate graphics experiment**, not the game | real PICA200 shader assembly, Citro3D link, vertex/depth sources | New-series hardware GPU screenshot, camera, near/far geometry and timing |
| `godguy_hw_safe.nds`, `*_audio_only.nds`, `*_save_only.nds` | DS troubleshooting | builds verified | Only use to isolate startup/audio/save failures |
| `godguy_hw_nocache.nds` | Optional diagnostic | best-effort compile | Only if diagnosing cache bugs |

The GBA CI workflow in `bore-gba` builds the repository's **BORE GBA**
project. Its green status is not proof of a standalone GodGuy GBA port
unless a separate GodGuy GBA build target is introduced and verified.

## How to obtain compact candidate bundles

The `Build GodGuy DS ROMs` GitHub Actions workflow now has a
`port-candidates` job that **requires both** the DS/DSi build and
independent 3DS cinema/SDK build to succeed. It fetches the two build
artifacts, validates all required compiled binaries and produces:

- `godguy_<commit>_playable.zip`: compact software-renderer 3DS and
  DS/DSi compatibility-mode candidates; the optional libnds file is
  included only when it actually compiled.
- `godguy_<commit>_experimental.zip`: DSi extended mode, separate
  hardware-GPU experiment and diagnostic fallback ROMs.

Each ZIP includes `README_PORTS.txt`, exact `PORTS_MANIFEST.json`
with file SHA-256 hashes/sizes, and `SHA256SUMS.txt`. No film clips
are placed in the ROM ZIP: they are optional CI inspection media and
unnecessarily increase download/storage size.

The verifier rejects absent core images, invalid Nintendo DS header
CRC or ARM segment bounds, a mislabeled 16 MiB DSi ROM, malformed 3DSX
headers and conflicting duplicate Nintendo DS builds. Fresh CI also
requires the two independent SDK jobs to produce byte-identical
`godguy.3dsx` executables before creating or publishing candidate bundles.
Both jobs update the SDK package database and print compiler/library
versions, the linked `libctru.a` hash and generated 3DS asset hashes.

The original merged candidate's alternate 3DS hash came from a specific
SDK mismatch: the mixed-port job upgraded to libctru 2.7.1 while the
cinema job retained 2.7.0. Its generated assets and compiler matched.
That historic manifest records the independent binary's hash and size;
only the mixed-port image was distributed. The packaging tool retains
that audit capability for older artifacts, but new CI rejects such a
mismatch rather than silently selecting a binary. Candidate releases
are labeled as GitHub prereleases and use a separate
`godguy-ports-ci-<full commit SHA>` tag for each source revision, so an
older tag never gains a mixture of old and new ROMs.

Independent
`GodGuy multiport image format and release packaging` CI exercises
corrupt synthetic fixtures before the large console builds finish.

## Finishing-pass regression coverage

The source tests exercise failures as well as normal play:

- `pc_3ds_interaction.c`: exact terrain targets, building/inhabitant
  protection, all 256 headings at tile corners, interpolated hillside
  occlusion, honest status messages, native-depth preview and no camera
  bob while blocked. The preview is bounded to 32 translucent triangles
  and uses the existing RGB/depth surfaces.
- `pc_3ds_mesh.c` and `pc_3ds_mesh_world.c`: shared translucent edges
  blend once; five circular primitive generators wrap their final angle;
  both geometry detail levels run under undefined-behavior/bounds checks.
- `pc_save_safety.c`: failed save-and-title preserves the current world;
  malformed headers, identity records, truncated bodies and final records
  are refused before replacing live progress; backups still appear and
  load; successful save-and-title survives reloading. The read-only
  preflight uses 512-byte scratch and preserves versions 11–14. It checks
  structure, not every arbitrary payload bit flip; these formats have no
  whole-file checksum.
- `pc_save_hw_guard.c`: a write exceeding cartridge archive limits is
  latched and cannot commit a partial save; compressed data rewinds in
  its existing buffer without another cartridge read.
- `pc_3ds.c`: 400-frame framebuffer/stereo parity, twelve audio allocation
  failure positions, DSP failure, repeated initialization/shutdown,
  recovery, ordered stereo chunks and release of all linear audio buffers.

Local save regression runs also cover DS/DSi/GG_BIG record layouts,
12,000/38,000/199,999-inhabitant caps, 384/768/1024 worlds, legacy identity
fallback and DS-to-GG_BIG save migration. These are source/host checks;
actual device timing and the acceptance checklist below remain separate.

## On-device acceptance checklist

Record **device or emulator model, loader version, source SHA, target
FPS, audio health and visual screenshots**. Do not mark a platform
finished until these pass.

1. **Startup**: title screen shown within a reasonable device-specific
   time; no permanent black screen, white screen, red 16 MiB RAM
   warning on a compatible system, or lost touch/pad.
2. **Input and safety**: navigate menus; draw land; place and remove
   buildings; enter and exit 3D world; collision works on steep slopes.
3. **Memory and graphics**: 10-minute busy-settlement traversal and
   cinematic playback; no building-floor bleed, floating roofs,
   disappearing NPCs or missing textures during quality transitions.
4. **Living inhabitants**: two nearby residents walking in separate
   directions while camera follows; correct foot contact, stable
   identities, no snap or stuck animation on save and reload.
5. **Audio/video**: audible music and speech, no mixer stalls;
   story frame rate measured on actual hardware and not inferred from
   much faster x86 host frame-cost measurements.
6. **Save migration**: create a save, power cycle, restore it, check
   population/structures; explicitly test corrupted/truncated save.
   Record actual save medium (cart, SD, emulator state).
7. **DSi extended mode**: do not promote to normal DS release; verify
   mode-specific loader and 16 MiB RAM behavior first.
8. **New3DS GPU**: independently capture 3D probe output, inspect
   depth and camera orientation, then integrate the same world data
   into the actual game without duplicating the simulation. The GPU
   demo's success does **not** certify the game uses GPU rendering.

## Verified main-branch baseline

PR #71 merged the complete chain into `main`; #58 is merged and #67,
#68 and #69 were closed as superseded with their changes included.
All six postmerge workflows passed on
`e0dbfe8a91b7213be1df3058ab3476c5618ee407`, including both SDK builders,
PICA200, cinematic, procedural-people and packaging checks. Run
`44963948553` published the corresponding checksummed candidate bundles
at the `godguy-ports-ci-candidate` release tag.

**Certification status:** automated source/SDK and image-format checks
passed for that baseline. Every subsequent candidate must pass fresh
checks, including agreement between independent New 3DS builds. The
hardware/emulator acceptance checklist remains outstanding.
