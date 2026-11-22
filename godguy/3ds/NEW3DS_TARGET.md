# GodGuy — New Nintendo 3DS world target (21 November 2026)

## Hardware scope

The **3DS homebrew executable** built with `godguy/3ds/build.sh`
now targets **New Nintendo 3DS**, **New Nintendo 3DS XL**, and
**New Nintendo 2DS XL** only. Original Nintendo 3DS, 3DS XL and
original 2DS are intentionally unsupported by this specific 3DS build.
An unsupported machine shows a safe on-device explanation rather than
attempting the heavier triangle world.

Other DS/DSi ports are separate targets and are **not removed**. The
repository's GBA workflow builds BORE; it is not a GodGuy GBA build.
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

## PICA200 GPU migration experiment (separate preview binary)

Set `GG_BUILD_GPU_PROBE=1` when running `3ds/build.sh` to build
`godguy_new3ds_gpu_probe.3dsx` in addition to the unchanged playable
`godguy.3dsx`. This prototype is intentionally a **separate homebrew
binary** rather than an unreviewed renderer toggle inside the game.
Its test map uses the same GodGuy terrain/building tile encoding, but
**does not read live game saves or drive the actual campaign yet**.

The prototype has a working-source PICA vertex shader
(`3ds/gpu_world.v.pica`) assembled by `picasso`, with Citro3D
perspective, GPU vertex colors, GPU depth, and a bounded linear-RAM
vertex buffer. The SDK-free `3ds/gpu_world_batch3d.h` has a reusable
tile/building mesh contract accepting world and bld arrays directly.
`pc_3ds_gpu_batch.c` verifies determinism, map edits, height, camera
re-basing and hard bounds without SDK support. A New 3DS hardware build
checks shader assembly and linker compatibility.

**Memory ceilings:** 8192 vertices x 24 bytes = 192 KiB linear buffer.
No second 400x240 CPU RGB buffer in the GPU sidecar. GPU render targets
use Citro3D's VRAM, not the existing software depth buffer. The main
game's software rasterizer and saving remain unchanged.

**What the PICA probe DOES NOT yet provide:** on-device visual proof,
renderer parity, NPCs / full building grammar, texture atlas streaming,
cutscene composition, or gameplay integration. Shader and GPU render
calls are real, but compiling a probe does not by itself prove correct
camera orientation, depth conventions or acceptable console FPS.
A captured hardware walk-through is the release gate for this step.

**Next integration gate:**
1. Verify the shader, GPU geometry path and depth on New hardware with
   actual screen captures, benchmarked against software view.
2. Stream the game's authoritative `world[][]`/`bld[][]` into the
   same bounded GPU batch, without a duplicated world or save format.
3. Add NPCs, interaction HUD, lighting and collision parity; only then
   enable an explicit user-selectable PICA backend in the playable ROM.
4. Eventually replace CPU-built tile triangles with static terrain
   patches, geometric occlusion culling, materials and vertex-shader
   lighting, keeping strict VRAM and frame-time budgets.
