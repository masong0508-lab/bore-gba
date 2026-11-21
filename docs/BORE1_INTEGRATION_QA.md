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
