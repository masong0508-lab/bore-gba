# BORE 1 — Campaign continuity and rendering acceptance

## Scope

This follow-up to PR #70 fixes campaign clock/retry edge cases, restores generated-character save compatibility, and reduces the cost of the existing cinematic renderer. The comparison baseline is committed `39412b3`. Hospital artwork, camera compositions and save offsets are unchanged by this pass.

PR #70 established the supported patient pose, bedside staging and original-protagonist handover rules below. This follow-up adds source-driven scenarios spanning all five chapters and native full-game save/reload checks. It does not certify a complete interactive five-chapter playthrough.

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

## Campaign corrections in this pass

The unsaved terminal-pressure retry previously reset the live pressure variables while leaving terminal pressure and the used final-night flag in story memory. The next restore brought both back. The retry now resets that persisted review state, restarts the chapter clock and retains earlier relapse history. A player with a saved game still follows the existing rollback-to-save path.

The chapter clock reserves byte 255 for "not started." Mapping a real day-255 start to 254 previously credited a free day immediately. TV SHOW & TELL now records the real day-255 start in its otherwise unused first saved task bit. The clock byte and all SRAM offsets stay unchanged. Day 255 starts at zero elapsed days and day 256 advances it to one. Legacy day-254 starts retain their meaning; the 255 sentinel still means not started. Other stories retain all three checklist bits. First starts, chapter transitions, relapses, retry, new-story setup and story exit exercise the marker lifecycle.

Story arrivals could receive a valid game-generated personality above the creator's 25-point spending budget. The real Mamesy handover produced traits `[7, 5, 9, 7, 4]` (32 points); saving succeeded, but the next load rejected the person chunk. Serialized personality validation now checks aspiration/want bounds and each trait's 0–10 range without applying the creator spending budget. Existing generated traits survive exactly, including in previously rejected files. The creator's point-allocation guard remains unchanged.

## Rendering changes and runtime measurements

The shared rectangle primitive uses size-optimized ARM code with the same clipping and pixel loop. Its compiled body is 248 B instead of 272 B. Darkening and brightening process eight pixels per inner iteration while retaining scalar handling of unaligned heads and tails. Packed glow arithmetic saturates each RGB15 channel without carrying into another channel or pixel. The vignette visits only rows that intersect the active clip. No persistent buffer or array is added.

Measurements use compiled preview ROMs under libmGBA 0.10.5 with devkitARM's GCC 16.1.0. Hardware breakpoints measure emulated GBA CPU cycles between `csDraw` and `csPresent`; picture cadence uses successive `csDraw` entries. Each sample starts 20 hardware frames after entering the named beat and spans 120 hardware frames, with normal audio. Both closing-shot previews start directly at beat 29 using the existing preview build option. These are bounded emulator measurements, not a hardware-wide or campaign-wide FPS guarantee.

Unlike the original PR #70 staging comparison, this pass measures identical artwork and camera compositions against `39412b3`.

| Shot | Baseline draw time | Revised draw time | Baseline picture cadence | Revised picture cadence |
|---|---:|---:|---:|---:|
| Hospital opening, 3/0 | 25.449 ms | 21.557 ms | 29.864/s | 29.864/s |
| Doctor bedside, 3/3 | 36.348 ms | 33.115 ms | 21.806/s | 24.024/s |
| Awakening, 3/16 | 67.394 ms | 62.287 ms | 13.056/s | 14.144/s |
| Bright closing, 3/29 | 75.806 ms | 54.319 ms | 11.391/s | 15.747/s |

Draw time falls by 15.3%, 8.9%, 7.6% and 28.3% respectively. The opening remains at its existing approximately 29.9-picture cadence; the other sampled shots deliver more pictures per second. These combined results include both the shared rectangle and lighting changes.

The complete local game build uses 247,000 B EWRAM and 30,864 B IWRAM, including the existing largest 7,296 B overlay. Combined static RAM is 277,864 B, 32 B less than `39412b3`. The larger lighting overlay remains smaller than that pre-existing maximum. EWRAM has 6,952 B headroom beneath the reserved 8 KiB stack; IWRAM has 1,648 B below the BIOS reservation. The all-pack ROM remains 28,704,768 B.

## Automated and visual validation

- Native preview ROMs boot and advance in real mGBA. Full 240×160 captures retain the approved opening, supported awakening pose, bedside contact and bright closing composition. Earlier PR #70 captures also covered doctor return, fatal handhold, flatline and fatal bedside.
- `tools/render_cutscene.sh` includes the actual cinematic headers and extracts drawing/font primitives from `main.c`. It is a development preview, not a replacement for emulator validation. `--check-patient` passes 1,086 frames across lying/stirring motion at three zoom levels, checking stationary supported head, bedding contact, connected elbow/fingers, bed containment and framebuffer guards.
- `tools/test_missy_handover.py` compiles the actual story and household-removal functions. It covers active/inactive cast reuse, unrelated names, full-house retries, blocked switches, UID zero, pending/completed handover reloads, UID remapping, cast removal followed by identity reuse, and return to Missy.
- New `tools/test_missy_campaign.py` compiles actual campaign setup, goal, clock, pressure, reward, serialization, film-dispatch and handover functions plus household switch admission. Controlled fixtures supply world time, input, rendering, audio, sprite transfer and external reward storage. It covers all five chapters, ordinary household selection/reload during handover, pending scenes, relapse in chapters 2/3/5, one final-night extension, the second terminal event, saved/unsaved retry and finale reload without duplicate rewards. It also checks day starts 0 through 257, legacy clock states, marker cleanup and all eight checklist-bit combinations in other stories. CI runs this test alongside the existing handover regression.
- New `tools/test_personality_save.py` exercises the actual person encoder/decoder, SRAM personality save/load, and creator editing functions. The exact 119-byte Mamesy person chunk that the old ROM rejected now decodes and re-encodes byte for byte. All 3,221,020 bounded aspiration/want/trait combinations validate; out-of-range fields, truncated data and corrupt SRAM checksums still reject. Creator edits continue to enforce the 25-point spending budget, including when opening a generated personality above that budget.
- `tools/test_cslighting.py` passes 1,637,004 darkener and 7,628,460 glow kernel calls against independent RGB15 channel mathematics. Cases include all 32,768 colors, shifts 1–3, nine channel masks, both packed lanes, bit 15, odd/even alignment, clipping and memory guards. Another 384 vignette cases cover moods, backgrounds and clip rectangles.
- Pixel comparison against `39412b3` passes 1,629 actual-source frames across nine shots/backgrounds, including 506 partial redraws. Both cumulative pixel hashes and final framebuffer bytes match. Coverage includes hospital opening/doctor/awakening/closing, fatal bedside, home glow, mirror, studio and car.
- Existing story memory/pressure rules, all 129 camera entries, skate feel/jam, NPC targets and the full HERE TODAY song check remain release gates. The earlier ellipse/trapezoid/line parity checks remain relevant because those primitives are unchanged in this pass.
- Full game, creator and both preview builds pass local budget checks. All 128 expansion-pack subsets and seven patch orders validate.

The X11 CI smoke waits with a deadline for a real caption/picture signature, rejects blank/logo-only output, requires a live process/window and distinct captures, and uploads failure evidence. Native CI captures must be inspected before acceptance.

## Full-game emulator scenarios

A fresh full-game save was created through the normal Players → New Player → Story Mode → TV SHOW & TELL flow. After observing and deliberately skipping the opening cinematic, the game entered chapter one. Saving through the pause menu, resetting the emulator and loading the player preserved the MISSY name, player slot, UID zero and chapter progress. The exported 128 KiB flash file also loaded in a fresh emulator running the exact integrated ROM; no savestate crossed builds.

A separately declared hospital fixture starts from that real player save and prepares chapter four at day 255. The game itself creates Mamesy as UID one, preserves Missy as the UID-zero patient and hands over control. The chapter card correctly displays zero of five bedside days. This prepared scenario is distinct from the natural new-player/save/reload flow above. The exact flash file that previously failed to load was cold-loaded by the corrected full-game ROM without editing its bytes. Mamesy's 32-point personality was preserved. Household UID compaction remapped the active sister to zero and the patient to one; names, roles, chapter progress and the day-255 marker remained correct. Saving again through Pause → Save produced the next player sequence, which loaded successfully in a separate fresh emulator process with the same traits, roles and zero elapsed bedside days.

A separate same-ROM checkpoint prepares the unsaved terminal event with prior slip history, a used final night and day 255. The actual pressure tick enters the loss cinematic and failure card. Acknowledging retry clears the persisted final-night flag, restores the starting pressure band, retains the earlier slips, and restarts the clock at zero elapsed days. Mamesy remains controlled and the game continues without immediately repeating the loss.

## Remaining campaign acceptance

A complete natural five-chapter interactive playthrough remains outstanding. The source-driven scenarios, prepared hospital fixtures and cinematic captures exercise specific risks; they do not replace playing every chapter continuously through the ordinary UI.
