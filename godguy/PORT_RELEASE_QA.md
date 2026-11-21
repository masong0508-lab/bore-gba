# GodGuy — Console port verification and release gates

Updated 21 November 2026. The canonical code for the New 3DS world is
stacked across #58 -> #67 -> #68 -> #69 -> #71. The build workflows are
**not** hardware certification. No automatic merges are implied by this
document.

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
headers and conflicting duplicate executable builds. Independent
`GodGuy multiport image format and release packaging` CI exercises
corrupt synthetic fixtures before the large console builds finish.

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

## Main-branch merge dependency

The current Open PR chain is #58 -> #67 -> #68 -> #69 -> #71. When
this document was written, #58 had a merge conflict with a newer
`main`. Reconcile that foundation branch first, then require fresh
full checks on the reconciled head before merging the chain in order.
Do not force-update `main` or bypass release gates to make the
project appear finished.

**Certification status:** source/SDK builds and host tests passed on
GodGuy PR #71 commit `2b3abdce208bcf41cdd27a4add2ed3703506ead9`.
The independent port packaging workflow and later candidate commit
must pass before new candidate ZIPs can be treated as CI-qualified.
The hardware/emulator acceptance checklist remains outstanding.
