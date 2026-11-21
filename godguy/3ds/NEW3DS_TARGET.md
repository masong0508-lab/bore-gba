# GodGuy — New Nintendo 3DS world target (21 November 2026)

## Hardware scope

The **3DS homebrew executable** built with `godguy/3ds/build.sh`
now targets **New Nintendo 3DS**, **New Nintendo 3DS XL**, and
**New Nintendo 2DS XL** only. Original Nintendo 3DS, 3DS XL and
original 2DS are intentionally unsupported by this specific 3DS build.
An unsupported machine shows a safe on-device explanation rather than
attempting the heavier triangle world.

Other DS/DSi/GBA ports are separate targets and are **not removed**.
This is a target-policy change, not an assertion that all those ports can
render the New-series mesh at identical fidelity.

## Native optical and world contract

- Software polygon mesh uses the existing **400×240 RGB** top-screen image,
  with the unchanged **192,000-byte (187.5 KiB) depth buffer**.
- Frame pressure may reduce procedural surface sampling, scenery radius,
  foliage facet count and shadow fragments. It **must never halve the top
  viewport** or change the camera projection, FOV or world coordinates.
- The same live world, buildings, people, biome data, save format and
  cinematic character identities remain authoritative; no parallel game
  simulation is introduced.
- The original 200×120 path remains inside the source as a host-verified
  legacy path, but `-DGG_NEW3DS_ONLY` is enabled for the shipped 3DS ROM.
- New-series CPU speedup is requested through libctru. Geometry is still
  **software rasterized**; **PICA200/Citro3D hardware polygons are not
  implemented** by this milestone. Skyrim-equivalent visuals or measured
  on-device 30/60fps are not claimed.

## Regressions and release gate

Run source/geometry tests in `.github/workflows/godguy-3ds-smoke.yml`.
It compiles `pc_3ds_new3ds.c` with strict C11 warnings and runs
`pc_3ds_mesh_world.c` and `pc_3ds_cinema_capture.c` under
`-DGG_NEW3DS_ONLY`. The visual pipeline publishes native 400×240
images, source video, and host CPU metrics for review.

The existing Nintendo SDK workflow `.github/workflows/godguy-nds.yml`
must also build `godguy.3dsx`, without regressing companion ROM
formats. A live New-series console walkthrough is still needed to
measure render frame times, audio starvation, camera jitter, character
occlusion, and active-scene temperature/CPU load.

Prioritize **native frame stability** and reliable visuals over
unbounded asset counts. For future true GPU geometry, introduce a
separate PICA/Citro3D renderer with camera/material parity tests,
lifecycle and VRAM budgets; do not relabel existing software triangles
as GPU accelerated.
