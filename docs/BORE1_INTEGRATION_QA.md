# BORE 1 — Hospital rendering and story continuity acceptance

## Scope

PR #70 completes the hospital pose, bedside staging and camera correction, and protects the TV SHOW & TELL protagonist during the Mamesy handover. It is a targeted integration milestone. It does not certify a complete interactive playthrough of all five campaign chapters.

The native mGBA captures from baseline `824cadd` were rejected: Missy's face still read as upright on a reclining body, the patient dwarfed the standing cast, the stir animation separated the arm, and the fatal close-up cut the head offscreen. A successful emulator boot alone was not accepted as visual approval.

## Accepted rendering behavior

- One quarter-world silhouette is projected at every camera distance. The forehead, near eye, upward nose, lips and chin follow a supine profile. Hair/skull rest on a compressed pillow; the neck joins the collar. Only the chest breathes. The supported head does not bob.
- The gown, bent elbow, forearm and fingers remain connected while the hand stirs. The patient fits inside the shorter mattress (world x=115..183), with a shaped hip/knee/foot blanket.
- Mamesy approaches the far side of the bed and actually holds Missy's hand during the vigil, the fatal sequence and the corresponding memories. The opaque bed front hides her lower body correctly. The IV tube is routed below her face.
- The opening holds a readable full-room composition. Doctor entry/return use the same wider two-shot; hand shots follow the actual wrist; awakening close-ups retain the supported head/neck/shoulder. The fatal bedside shot preserves both heads.
- In `CB_FLAT`, the patient has no breathing, hand-stir, blink or speech animation. Deliberate scene-level camera shake remains separate.

No bitmap assets, heap allocations, additional framebuffer or new persistent rendering arrays were added. Fine geometry and lighting use the existing cutscene code overlay.

## Story identity and save behavior

TV SHOW & TELL records its original protagonist UID at story start and preserves it through household selection changes. Mamesy's family relationship and surname come from that protagonist. UID zero is valid; legacy sentinel 255 retains the selected-character fallback when original identity was never saved.

The shared cast lookup recognizes both inactive Mamesy and a currently selected Mamesy. The latter is stored outside the inactive household array: overlooking her previously duplicated the sister or stalled a full household. The handover now completes without a duplicate and makes the recorded patient unavailable immediately. A full house is allowed when the correct sister can be reused.

Household removal now notifies the TV story before an identity can be recycled. Removing the recorded protagonist, or Mamesy while she is still required before chapter five, cleanly ends only the unfinished TV story, saves that state and shows an explanation. Free play and the existing start-story flow remain available. Removing Mamesy after her narrative absence, or removing cast after story completion, clears only obsolete cast references; a replacement cannot inherit the hospital or remembered status. Returning to Missy depends on her recorded identity, not the continued existence of Mamesy's household slot.

The SRAM layout and save offsets are unchanged. Story save/load, pending handover, household remapping, capacity and retries are exercised with actual source functions rather than a reimplemented model.

## Runtime measurements

Measurements use unmodified preview ROMs under libmGBA 0.10.5. Hardware breakpoints measure emulated GBA CPU cycles between `csDraw` and `csPresent`; picture cadence uses successive `csDraw` entries. Each sample starts 20 hardware frames after entering the named beat and spans 120 hardware frames, with normal audio. These are bounded emulator measurements, not a hardware-wide or campaign-wide FPS guarantee.

The comparison is between the original curated shots and the revised compositions. The held opening and wider doctor shots are intentional staging changes; this is not an identical-camera comparison.

| Shot | Baseline draw time | Revised draw time | Baseline picture cadence | Revised picture cadence |
|---|---:|---:|---:|---:|
| Hospital opening, 3/0 | 34.137 ms | 25.449 ms | 22.333/s | 29.864/s |
| Doctor bedside, 3/3 | 36.186 ms | 36.348 ms | 21.273/s | 21.806/s |
| Awakening, 3/16 | 73.947 ms | 67.394 ms | 11.968/s | 13.056/s |

A controlled comparison using the new 4× camera with the old artwork measured 80.954 ms / 11.069 pictures per second; the new artwork with early clipping/batching measured 77.728 ms / 11.588 before the final lighting and line optimizations. Thus the tighter camera was distinguished from renderer cost during profiling.

The complete local game build uses 247,000 B EWRAM and 30,896 B IWRAM including the largest 7,296 B overlay, versus 247,008 B / 30,888 B at baseline. Eight existing bytes move between banks; combined static RAM remains 277,896 B. EWRAM has 6,952 B headroom beneath the reserved 8 KiB stack; IWRAM has 1,616 B below the BIOS reservation. The all-pack ROM is 28,704,768 B. No new framebuffer is hidden in these figures.

## Automated and visual validation

- Native preview ROMs boot and advance in real mGBA. Full 240×160 captures cover the opening, bedside handhold, finger movement, awakening, doctor return, closing, fatal handhold, flatline and fatal bedside.
- `tools/render_cutscene.sh` includes the actual cinematic headers and extracts drawing/font primitives from `main.c`. It is a development preview, not a replacement for emulator validation.
- `--check-patient` checks 1,086 rendered frames across lying/stirring motion at three zoom levels for stationary supported head, bedding contact, connected elbow/fingers, bed containment and framebuffer guards.
- `tools/test_missy_handover.py` compiles the actual story and household-removal functions. It covers active/inactive cast reuse, unrelated names, full-house retries, blocked switches, UID zero, saved/reloaded pending and completed handovers, UID remapping, protagonist/cast removal followed by identity reuse, and return to Missy.
- `tools/test_cslighting.py` passed 1,637,004 kernel calls against independent RGB15 channel math, covering all 32,768 colors, shifts 1–3, packed lanes, clipping, alignment and memory guards.
- Exact-pixel comparisons cover ellipse clipping/trapezoid batching, line clipping, incremental glow geometry, and axis-aligned line rectangles. The latter include 41,888 direct cases, reversed endpoints, degenerate points and zoom-thickness boundaries. Degenerate camera-line points retain the original horizontal stroke orientation.
- Existing story memory/pressure rules, all 129 camera entries, skate feel/jam, NPC targets and the full HERE TODAY song check pass.
- Full game, creator and both preview builds pass local budget checks. All 128 expansion-pack subsets and seven patch orders validate.

The X11 CI smoke no longer assumes boot completes after a fixed 14 seconds. It waits with a deadline for the real caption/picture signature, rejects blank/logo-only output, requires a live process/window and distinct captures, and still uploads failure evidence. `tools/budget.sh` converts the pack boundary to decimal before passing it to POSIX awk.

## Remaining campaign acceptance

A manual five-chapter interactive playthrough remains outstanding, including ordinary UI selection/reload during the handover, relapse, the final-night extension, the second terminal pressure event and the finale. The source-level scenario tests and cinematic captures do not replace that playthrough.
