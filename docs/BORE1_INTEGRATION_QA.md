# BORE 1 — Campaign continuity and rendering acceptance

## Scope

This acceptance record covers the patient/cast work in PR #70, the campaign and rendering corrections in PR #73, and the current follow-up based on `6aef481`. The current changes count town friendships in story goals, recover valid GROW UP saves, correct flash polling, clarify the bedside and completed-chapter counts, and reduce darkened-shot rendering cost. Hospital artwork, camera compositions and save offsets are unchanged.

PR #70 established the supported patient pose, bedside staging and original-protagonist handover rules below. This follow-up adds source-driven scenarios spanning all five chapters and native full-game save/reload checks. It does not certify a complete interactive five-chapter playthrough.

## Current gameplay and save corrections

All three story friendship predicates previously counted only household UIDs. A native hospital save listed IRIS and PIP as friends in MY SIM > PEOPLE while the chapter card still showed zero of two friends. The TV campaign goal, generic chapter goal and checklist goal now add the existing `nrFriends` result, matching career promotion requirements. That function synchronizes active visitors and checks remembered identities against the current town. Visiting friends count once, departed friends remain known, another household member's friends do not count, and missing or replaced neighbors do not satisfy a goal. The existing exclusions for the hospital patient and absent sister remain in the household count.

`tools/test_story_friends.py` compiles the actual three story predicates and town synchronization/identity functions together. It first demonstrated that the town subsystem counted a genuine fixture neighbor while the old story predicate returned zero. It then covers all three corrected paths, active and departed visitors, repeat synchronization, lost friendship, invalid name hashes and lots, ownership, full-house guest-slot reuse, cast exclusions, and the effect of two town friends on hospital pressure support. Fixture boundaries supply the small town, banks, mood and cash. This is source-level integration coverage, not an interactive playthrough.

Serialized personality validation previously used `AS_PICK`, the five choices exposed by the creator, as its aspiration limit. `AS_GROW` is also valid stored state: young household members use it, and the GOOD BOY story explicitly assigns it to its adult lead PIPPA. An ordinary native new-game/save/reset/load flow reproduced a listed PIPPA save that failed with COULD NOT READ SLOT. The file was structurally intact; its valid aspiration failed semantic decoding. The reader now accepts all values below `AS_N`. It still rejects out-of-range aspirations, wants and traits. The creator retains its five choices and 25-point budget. No save migration or format change is required.

The personality regression retains the exact native Mamesy person chunk and adds the exact 119-byte PIPPA chunk. It checks byte-exact person round trips and SRAM restoration across all five life stages, all 3,865,224 valid trait/aspiration/want field combinations, malformed fields, and the actual creator cycling/spending guards. The new PIPPA fixture failed against the old decoder before the bound correction.

The native finale test also exposed a separate flash-write failure. On mGBA, programming the byte `0x5F` made the existing equality poll accept the emulator's temporary `0x5F` busy response as completed data. The following `0x61` write then read that busy response instead of erased `0xFF`, failed the bit-preservation check and left a bad payload checksum. The game displayed SAVE NOT SUPPORTED HERE. A second control showed that the same problem could affect bytes copied back when preserving a neighboring half-sector. Waiting longer after the failed save did not repair it; this was a failed write, not an early export from the test harness.

`flProgRaw` now performs 128 volatile reads after programming `0x5F`, before the existing bounded data poll. The compiled loop outlasts mGBA's 650-cycle busy interval, including at the fastest SRAM wait setting. Other data values retain their ordinary path, with three extra cycles in the diagnostic. The helper leaves the caller's interrupt setting unchanged. The timeout/reset path and all save formats remain intact. The guard adds 16 B of IWRAM code and no persistent data; an isolated size-optimization experiment recovered no space and was rejected.

Three independent checks establish this cause: a native instruction trace of the failing game, a standalone test against the actual libmGBA flash implementation, and byte/checksum inspection of the saved payload. Native diagnostic ROMs compiled from the real save helpers test every possible successor byte after `0x5F` at two SRAM wait settings: the old code fails 386 of 512 cases, and the corrected code passes all 512. The ordinary-byte path, a 64 KB bank crossing, and both incoming interrupt states also pass. The committed host regression extracts the actual polling, byte-writing and slot-checksum functions with an explicit flash-I/O boundary; it adds repeatable coverage of the status collision and error handling.

After the hospital grants its one final night, the journal now shows elapsed bedside days against the extended six-day goal. For example, four elapsed days display as 4 OF 6 rather than 3 OF 5. The completion predicate, pressure rules, payment and saved state are unchanged; the display no longer appears to remove a day already spent.

The completed story card also retains the final playable chapter number after loading. The terminal END row no longer appears as an additional sixth chapter in a five-chapter story. This changes only the displayed label.

## Current rendering measurements

The native profile identified the vignette darkener as the largest sampled drawing cost in the awakening shot. The accepted change puts `csVig` in the existing ARM code overlay and always-inlines `csDk` into that loop. The compiler can specialize its fixed edge spans and shifts. Pixel arithmetic, clipping, loop bodies, artwork and cameras remain unchanged. Rejected experiments are not included.

The table compares the integrated build with PR #73 (`6aef481`), using the same libmGBA 0.10.5/GCC 16.1.0 setup, normal audio, 20 hardware frames of warm-up and a 120-hardware-frame sample. The closing shot uses a separate beat-29 preview built from the same integrated source. The integrated measurements match the isolated renderer candidate within measurement rounding.

| Shot | PR #73 draw time | Current draw time | PR #73 picture cadence | Current picture cadence |
|---|---:|---:|---:|---:|
| Hospital opening, 3/0 | 21.557 ms | 21.557 ms | 29.864/s | 29.864/s |
| Doctor bedside, 3/3 | 33.115 ms | 33.115 ms | 24.024/s | 24.024/s |
| Awakening, 3/16 | 62.287 ms | 58.181 ms | 14.144/s | 15.094/s |
| Bright closing, 3/29 | 54.319 ms | 50.713 ms | 15.747/s | 16.057/s |

Awakening and bright-closing draw time fall by 6.59% and 6.64%. Picture cadence improves by 6.72% and 1.97%; presentation follows hardware frame boundaries, so it does not scale continuously with draw time. Opening and doctor shots remain unchanged within measurement rounding. These bounded samples are not a whole-game frame-rate guarantee.

The renderer alone retains the previous total of 277,864 B static RAM; eight existing bytes move from EWRAM to IWRAM because the vignette accesses them from fast RAM. Including the necessary 16 B flash guard, the final full build uses 246,992 B EWRAM and 30,888 B IWRAM: 277,880 B combined, 16 B above PR #73. The lighting overlay occupies 6,480 B within the existing 7,296 B maximum allocation. There is no added persistent data, buffer, large array, framebuffer or heap allocation. EWRAM retains 6,960 B beneath the reserved stack, and IWRAM retains 1,624 B beneath the BIOS reservation. The all-pack ROM remains 28,704,768 B.

Against `6aef481`, all 1,629 actual-source rendered frames, including 506 partial redraws across nine shots/backgrounds, retain identical cumulative pixel hashes and final framebuffer bytes. The 1,637,004 darkener cases, 7,628,460 glow cases, 384 vignette cases and 1,086 patient frames also pass. Full game, creator, both standard previews, all 128 expansion-pack subsets and seven patch orders pass from the integrated source.

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

## PR #73 campaign corrections

The unsaved terminal-pressure retry previously reset the live pressure variables while leaving terminal pressure and the used final-night flag in story memory. The next restore brought both back. The retry now resets that persisted review state, restarts the chapter clock and retains earlier relapse history. A player with a saved game still follows the existing rollback-to-save path.

The chapter clock reserves byte 255 for "not started." Mapping a real day-255 start to 254 previously credited a free day immediately. TV SHOW & TELL now records the real day-255 start in its otherwise unused first saved task bit. The clock byte and all SRAM offsets stay unchanged. Day 255 starts at zero elapsed days and day 256 advances it to one. Legacy day-254 starts retain their meaning; the 255 sentinel still means not started. Other stories retain all three checklist bits. First starts, chapter transitions, relapses, retry, new-story setup and story exit exercise the marker lifecycle.

Story arrivals could receive a valid game-generated personality above the creator's 25-point spending budget. The real Mamesy handover produced traits `[7, 5, 9, 7, 4]` (32 points); saving succeeded, but the next load rejected the person chunk. Serialized personality validation now checks aspiration/want bounds and each trait's 0–10 range without applying the creator spending budget. Existing generated traits survive exactly, including in previously rejected files. The creator's point-allocation guard remains unchanged.

## PR #73 rendering baseline

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
- `tools/test_personality_save.py` exercises the actual person encoder/decoder, SRAM personality save/load, and creator editing functions. The exact 119-byte Mamesy and PIPPA person chunks that older ROMs rejected now decode and re-encode byte for byte. All 3,865,224 bounded aspiration/want/trait combinations validate across the expanded stored aspiration range; out-of-range fields, truncated data and corrupt SRAM checksums still reject. Creator edits continue to enforce the five aspiration choices and 25-point spending budget, including when opening a generated personality above that budget.
- `tools/test_cslighting.py` passes 1,637,004 darkener and 7,628,460 glow kernel calls against independent RGB15 channel mathematics. Cases include all 32,768 colors, shifts 1–3, nine channel masks, both packed lanes, bit 15, odd/even alignment, clipping and memory guards. Another 384 vignette cases cover moods, backgrounds and clip rectangles.
- Pixel comparison against `39412b3` passes 1,629 actual-source frames across nine shots/backgrounds, including 506 partial redraws. Both cumulative pixel hashes and final framebuffer bytes match. Coverage includes hospital opening/doctor/awakening/closing, fatal bedside, home glow, mirror, studio and car.
- Existing story memory/pressure rules, all 129 camera entries, skate feel/jam, NPC targets and the full HERE TODAY song check remain release gates. The earlier ellipse/trapezoid/line parity checks remain relevant because those primitives are unchanged in this pass.
- Full game, creator and both preview builds pass local budget checks. All 128 expansion-pack subsets and seven patch orders validate.

The X11 CI smoke waits with a deadline for a real caption/picture signature, rejects blank/logo-only output, requires a live process/window and distinct captures, and uploads failure evidence. Native CI captures must be inspected before acceptance.

## Full-game emulator scenarios

A fresh full-game save was created through the normal Players → New Player → Story Mode → TV SHOW & TELL flow. After observing and deliberately skipping the opening cinematic, the game entered chapter one. Saving through the pause menu, resetting the emulator and loading the player preserved the MISSY name, player slot, UID zero and chapter progress. The exported 128 KiB flash file also loaded in a fresh emulator running the exact integrated ROM; no savestate crossed builds.

A separately declared hospital fixture starts from that real player save and prepares chapter four at day 255. The game itself creates Mamesy as UID one, preserves Missy as the UID-zero patient and hands over control. The chapter card correctly displays zero of five bedside days. This prepared scenario is distinct from the natural new-player/save/reload flow above. The exact flash file that previously failed to load was cold-loaded by the corrected full-game ROM without editing its bytes. Mamesy's 32-point personality was preserved. Household UID compaction remapped the active sister to zero and the patient to one; names, roles, chapter progress and the day-255 marker remained correct. Saving again through Pause → Save produced the next player sequence, which loaded successfully in a separate fresh emulator process with the same traits, roles and zero elapsed bedside days.

A separate same-ROM checkpoint prepares the unsaved terminal event with prior slip history, a used final night and day 255. The actual pressure tick enters the loss cinematic and failure card. Acknowledging retry clears the persisted final-night flag, restores the starting pressure band, retains the earlier slips, and restarts the clock at zero elapsed days. Mamesy remains controlled and the game continues without immediately repeating the loss.

### Current native save recovery

The final full ROM has SHA-256 `ab2f40e145b567f0ef056a2833d6d7f79f813771979f91fd71986a1d9ca077d6`. It cold-loads the unchanged PIPPA flash that the prior release rejected, entering GOOD BOY chapter one with the correct name, adult stage, GROW UP aspiration and traits `[4, 7, 8, 3, 3]`. The accepted save implementation also passes an ordinary pause-menu save advancing the player count from two saves to three, followed by a separate fresh-process reload with the same persona and story state. This reproduction and recovery flow uses ordinary UI input, without RAM edits or cross-build savestates.

### Current native campaign scenarios

The game spawned IRIS and PIP as genuine town visitors. A declared fixture set friendship scores on those real identities; opening the People panel ran the game's own synchronization and the pause menu saved those records. The prior release listed both friends in People while reporting 0 OF 2 in the hospital. The final ROM cold-loads that exact unchanged flash and reports 2 OF 2 friends and 4 OF 6 bedside days. No identity, name hash or town population was fabricated to obtain the result.

The terminal-pressure scenarios exercise the actual pressure tick and film dispatch. The first event consumes the single reprieve, charges $1,000, reduces pressure from 100 to 50 and preserves the real-day-255 marker. A second terminal event plays the loss sequence and returns to the saved file, with byte-identical flash and remembered relationships. These checks establish the existing saved rollback behavior; the earlier unsaved-retry regression remains in place.

For the hospital success path, prepared time, funds and wellbeing satisfy the other requirements while five elapsed days still holds the chapter. At six elapsed days the actual completion code runs films 3, 10 and 4, clears pending state and hands control back to Missy. The $8,000 hospital payment and $8,000 chapter reward net correctly, and 25 DNA is granted once. Saving and cold loading preserve Missy and Mamesy through UID compaction, keep Mamesy absent, and do not grant a second reward. A subsequent actual hunger faint resets the comeback clean-day clock to zero.

For the finale, the game's existing debug family-move-in UI creates real MORTIMER, VESPER and WREN records. Declared goal fixtures supply time, earned funds, wellbeing and friendships on those residents plus a genuine IVY town visitor. Nine clean days holds the chapter; ten completes through films 11 and 5, awards $9,000 and 25 DNA, and unlocks HERE TODAY. Films are observed and deliberately skipped with START. Chapter indices and household counts are not directly advanced to cause completion.

The decisive flash comparison cold-loads the same pre-finale file in the uncorrected and corrected builds, then uses identical goal values, menu actions and timing. The old first Save fails verification and damages bytes in the neighboring copy during partial-sector preservation. The corrected first Save displays GAME SAVED, clears the error/unsaved flags, and produces a valid new player checksum. Every neighboring byte is preserved before the older record's magic bytes are intentionally retired following successful verification. A separate emulator process loads that first corrected save with completed chapter index 5, Missy controlled, Mamesy absent, four housemates, $67,000, 50 DNA and the song unlock. It neither replays the ending nor duplicates rewards. The final display-only correction is separately checked by cold-loading this verified file and observing the chapter-five card.

These are actual-ROM scenarios with explicit time, goal, relationship, hunger and debug-family setup. They do not establish natural completion of all five mission bits or the all-five-missions reward. Cross-build comparisons use exported flash files, never emulator savestates.

## Remaining campaign acceptance

A complete natural five-chapter interactive playthrough remains outstanding. The source-driven scenarios, prepared hospital fixtures and cinematic captures exercise specific risks; they do not replace playing every chapter continuously through the ordinary UI.
