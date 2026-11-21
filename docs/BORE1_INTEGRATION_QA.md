# BORE 1 — Integrated hospital/cinematic milestone

This PR combines the tested ICU anatomy and bed-plane improvements, stricter emulator smoke checks, validated camera-shot parsing, byte-for-byte save status roundtrips, and hospital recovery/failure rules. It does not certify the complete five-chapter interactive campaign.

## CI gates
- Strict C test with all 256 story byte values and pressure/night transition cases
- Python static scenario and camera shot checks (all 129 shot entries, including malformed entry detection)
- Full GBA, creator and all seven expansion pack builds
- Both real mGBA cutscene previews, with process/input/window validation and screenshot artifacts

## Manual acceptance
- Inspect hospital + awakening frames at 240×160 (face/neck/gown/blanket, doctor's framing, no clipping)
- Play chapters 1–5 from start to finish, including relapse, Mamesy's handover, hospital recovery, and finale
- Save/reload during pending chapter film and after positive hospital events
- Verify the final-night extension and second terminal pressure event
- Compare ROM size, EWRAM/IWRAM headroom and FPS against main; do not claim parity without measurements

## Integration
Source: PR #61, #62, #64 and #65 plus the camera parser already merged as #63.
No new assets or framebuffers. This document tracks unverified manual gates separately from automated CI.

## Low-pressure reload regression
Band 0 now restores a genuine 0..6 hospital pressure level rather than resetting it to 20. Newly entered chapter four persists its starting pressure before a save. Existing saves with zero band may resume slightly easier than before; SRAM layout and save-file decoding are unchanged.

## Screenshot-driven animation correction
Review of the actual mGBA ICU/awakening stills showed that the neck/gown were breathing on a different vertical phase than Missy’s face. The close-up and distant heads now follow the same 1px breath phase; this adds no new assets or buffers. Native capture should be reviewed again for head-to-neck continuity.

## Save-entry pressure restoration and screenshot scope
A chapter-four save load now invalidates the in-memory hospital clock and restores the persisted pressure before the first scene/goal check. The TQ_PLUG goal also defensively restores before evaluating completion, so a stale default PLUG_START cannot satisfy a multi-goal chapter on a load whose pressure is high. The mGBA smoke now crops the Qt menu row and checks the actual 3:2 GBA viewport only; a local crop check against the archived 480x340 capture produced a 480x320 game frame with 108 colours. CI must still rerun both current previews to validate the new head/neck motion and cropped artifact.

## TV protagonist identity & household capacity (follow-up milestone)
A story can now record Missy's actual original UID when TV SHOW & TELL begins, rather than assuming the currently selected household member at the eventual Mamesy handover is Missy. Mamesy's pre-existing sister relationship and her new friend/family link now use this saved UID. Legacy chapter-four saves with an unset 255 UID fall back to the selected household member, preserving previous behavior but not perfectly reconstructing missing historical identity. New ongoing-life TV story joins reserve one free household slot for Mamesy before the story starts. The chapter journal warns when no free slot is available and clarifies the 60-pressure threshold in Mamesy's chapter. Does not add SRAM bytes, actor slots, or runtime assets.

For replay in an already-full household, the reserved slot is waived only when an existing named Mamesy has the sister relationship with the recorded original protagonist. One shared cast lookup is used by both the story-join capacity gate and chapter-four handover, avoiding duplicate-Mamesy creation and false full-house rejection. The lookup occurs only when joining/reaching the handover, not each game frame.

The Mamesy handover now also verifies that the saved Missy UID still belongs to the playable household before switching. New Mamesy's surname is taken from that original protagonist (either the active Sim's surname or the stored household member's surname), not the unrelated Sim currently selected. Missing-patient state produces an actionable warning instead of silently making someone else the patient.

## Supine hospital pose correction — anatomy acceptance
The 21 November native mGBA screenshots showed an anatomically wrong hospitalized Missy: her face pointed sideways away from the body, with a protruding upper arm and a tall, stiff skull far above the pillow. The **real code renderer** (`csWardPatient` in `source/csart.h` and `csFaceBed` in `source/csface.h`) has been redrawn as a supported, upward-looking supine pose, sharing one x/y body plane: pillow beneath hair/skull, low connected neck/shoulders, horizontal gown, hips and legs beneath a contoured blanket, and bent forearm resting on the abdomen. Wide and closeup faces use the same head anchor; breathing still moves head, collar and torso together. The revised render uses no new assets, allocations, or framebuffers. The Python regression rejects the previous horizontal nose/old head coordinates and checks pillow, torso, blanket, rail and closeup anchors.

**Still required:** Build both native preview ROMs, capture new full 3:2 frames, inspect opening, Mamesy's bedside, awakening, closeup and fatal flatline, and correct remaining silhouette problems based on *those* images. The previous PR #66 screenshots cannot be used as visual acceptance for the new artwork.

## Second screenshot-driven bed-pose refinement
The first native mGBA captures of the supine pose (successful workflow 44959416209) demonstrated that the patient is now horizontal, but her head was still drawn as a very round bobblehead and her arm looked like a stiff horizontal plank. The second pass reduces the skull/hair radius and nose projection, rounds the shoulder, moves the eye/mouth to match the smaller face, and folds a shortened forearm onto the abdomen, using the real procedural renderer in `csart.h` and `csface.h`. The first set of captures is a baseline, not approval of the final proportions; the new changes require fresh mGBA previews.
