# GodGuy graphics overhaul — implementation and verification

This pass changes the live shared map renderer and the New-series 3DS
world renderer. It follows the playability/save/audio work merged in #72.
The maintained playable targets are `godguy_hw.nds`, `godguy_dsi.nds`
(DS compatibility mode), and `godguy.3dsx` (New-series hardware).
The enhanced DSi and optional libnds/Calico game builds inherit the shared
map changes but retain their experimental runtime status. Diagnostic and
standalone GPU probes are not substitutes for the playable game.

## Visible changes

The shared map has a single RGB5 terrain palette used by both the game and
native-world host captures, calmer teal water, legible surf contours,
clustered grass and forest forms, shaded building silhouettes, revised
roofs, fields, monuments and sprite contact outlines. CLOSE/MICRO citizens
use new 16×16 art with clothing, hair, hands and grounded boot poses.
Zoomed terrain uses
the same meadow, variant and shoreline decisions as the normal map.

The New 3DS mesh uses continuous biome material coordinates and physical
normal lighting instead of alternating tile/triangle colors on flat land.
Oak crowns, tiered conifers, shorter ground plants, flowers, coastal foam,
silt, chimneys, steep roofs and glazed windows enrich the same live world.
The sky is a row-graded RGB backdrop with a world-direction sun and slowly
moving cloud banks; it shares the existing target without a sky texture or
second framebuffer.

Procedural citizens retain deterministic identities while gaining more
coherent proportions, directional faces/hair and clothing, distinct garment
silhouettes and a planted-foot walking cycle. Their palette receives the
scene lighting and distance haze before compositing. Native animals use
bird/bat, fish, crawler, serpent, mammal and other bounded mesh families;
unmodeled iconic creatures retain their actual source sprite with physical
depth instead of becoming a generic quadruped. Narrow solid scenery is
placed away from central travel lanes using stable positions shared by
drawing and visibility selection; it does not jump when creatures arrive.

## Rendering faults fixed along the way

- The close-view creature palette formerly overlapped roof shades and
  aliased families 9–15. All 16 families now have distinct private entries.
  Terrain toolbar swatches use the existing terrain colors, leaving those
  creature entries available without introducing a second palette conflict.
- Transition cache entries were truncated to 16 bits despite 32-bit keys.
  High-numbered magic-biome transitions now reuse their real entries.
- Distant facade layers often quantized to the same inverse depth as their
  wall, hiding windows and doors. A scoped equality allowance draws these
  layers while storing the original physical depth; it cannot pass a
  nearer quantized depth or leak into later actors.
- Flat terrain no longer receives a second artificial four-sector lighting
  pattern on top of the rasterizer's real surface-normal light.

## Verification supplied with the code

| Regression | What it verifies |
| --- | --- |
| `pc_map_art.c` | Shared map/close/micro captures, palette ownership, all terrain/building colors, 16 creature families, transition cache reuse and matching zoom motifs. Run in DS, DSi16 and GG_BIG layouts. |
| `pc_3ds_world_art.c` | Flat-biome seams, real slope light, visible distant glazing, foreground occlusion, unbiased stored depth, crowded coast and bounded scenery. |
| `pc_3ds_sky.c` | Camera anchoring, continuous wind wrap, clipped sun/clouds, native and 2× coverage, framebuffer guards and untouched world depth. |
| `pc_3ds_inhabitants.c` | Actual live-population species selection, recognizable distinct silhouettes, iconic source-art fallback, camera directions, wall/near clipping and shared haze. |
| `pc_3ds_procedural_people.c` | Deterministic identity/palette, directional anatomy, gait/support-foot contact, and guarded generated cels. |
| `pc_3ds_art_tour.c` | Every terrain type, a full turn sampled at 32 headings, sub-cell camera positions, both native detail levels, all four map corners and mixed village/coast captures. |
| Existing mesh, material, interaction, cinema, zoom, save and platform checks | Preserve projection, target accuracy, shadows, gameplay, persistence, audio and platform behavior while changing the graphics. |

The cinematic workflow runs strict C11 checks and source integrations with
undefined-behavior/bounds instrumentation. The main port builder also gates
candidate packaging on map art, sky, inhabitants and facade checks.
Independent native 3DS builds must still produce identical executables.

## Resource and evidence limits

New 3DS keeps a 400×240 image at both runtime detail levels and the existing
192,000-byte depth allocation. People reuse the same 1,608-byte cel.
Scenery uses its existing fixed queue; detailed foliage and materials are
reduced by the existing quality controller. The facade context adds one
four-byte flag. The sky has no image buffer or heap allocation.

The shared DS map keeps its existing tile layout and display surfaces.
The local native DS ROM is 3,988,544 bytes, with ARM9 BSS of 1,824,028
bytes and 270,372 bytes below the linker stack guard, retaining its separate
32 KiB stack. Build size and the guard are checked for actual native ROMs,
in addition to host tests. Higher visual detail can change frame cost;
host CPU timings are recorded for comparisons and are not console FPS.

Before/after source captures use the same camera and fixture world. The
new art tour also writes native 400×240 mixed-village/coast images. DS ROM
runtime evidence comes from the actual ARM9/ARM7 executable in DeSmuME,
including gameplay, sound, cartridge save, cold Continue and a 6,000-frame
live-world soak. Source captures and emulator evidence are labeled
separately. Physical New 3DS, enhanced DSi and flashcart acceptance remain
as documented in [port verification](PORT_RELEASE_QA.md).
