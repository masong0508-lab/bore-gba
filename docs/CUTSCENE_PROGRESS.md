# Cutscene redo progress

8 of 8 scenes redone.

| Order | id | Scene | Status |
|---|---|---|---|
| 1 | 7 | OPENING  THE NIGHT | done (redo 1) |
| 2 | 0 | CH1 END  THE BARS | done (redo 2) |
| 3 | 1 | CH2 END  THE SWEATER | done (redo 3) |
| 4 | 2 | CH3 END  THE FALL | done (redo 4) |
| 5 | 3 | CH4 END  WAKING UP | done (redo 7) |
| 6 | 4 | CH5 START  THE NEWS | done (redo 6) |
| 7 | 5 | CH5 END  HERE TODAY | done (redo 9) |
| 8 | 6 | CH4 LOSS  THE PLUG | done (redo 8) |

Untested on a GBA or in an emulator.

## Animation pass (redo 10)

New poses for `pa` / `pb` (in `cutscene.h`, drawn in `csart.h`): CP_TALK (gestures, the mouth moves while the caption types), CP_LAUGH, CP_CRY (hands on the face, tears in close-ups), CP_POINT (at the other figure), CP_SHOCK (jolts back, arms up), CP_WALK (walks in over 60 frames), CP_LEAVE (stands 45 frames, then walks out), CP_SLUMP (sinks and hangs the head), CP_STIR (lying: the hand lifts and the fingers move). Everyone blinks, and a lying figure breathes. On the mirror beats (scene 1) `pa` is the reflection's pose.
Speakers who were CP_STAND now use CP_TALK automatically (scene 5 sung lines unchanged).

## Atmosphere pass (redo 12)

- A rain-streaked window in the hospital (scenes 3 and 6), with a lightning flash and thunder on "Missy Jeanne is gone." (scene 6, beat 9).
- Mood: the lights dim for the news (scene 4 from beat 21, the dressing-room bulbs go out) and the plug (scene 6 from beat 7, darker still from beat 9); the picture brightens when she wakes and hears the rain (scene 3 from beat 29). `csMood` in `cutscene.h` (-1 brighter .. 2 much dimmer), applied in `csVig`.
- Her phone lights up on the bar cart (scene 1, beats 4-5) and buzzes in her hand (scene 4, beats 17 and 19).
- The camera drifts a pixel or two on every held shot (undone each frame, so the shot lists are unaffected).

## Step 5 - close-up faces (redo 14)

In `csface.h` (only used when the camera is zoomed in, csCz >= 384):
- Head turn parallax: the head eases toward where the figure looks (csLook) plus a slow idle sway. Eyes, brows, cheeks and mouth slide the furthest, the nose a bit more, the face oval half as far, the hair a third. Lying figures keep still; crying and sad heads turn less.
- 3-phase blinks: half shut, shut, half open (6 frames, every 100).
- Mouth shape by syllable: ah (wide, tall), oo (small, round), ee (wide, flat), oh (round); teeth and tongue follow the width.
- State: csTn[8] and csTnFn[8] in EWRAM (48 B). No IWRAM change.

## Step 6 - walk and run cycle (redo 14)

In `csart.h` (`csFig`, new `csStepLift`): while a figure walks (CP_WALK / CP_LEAVE, only while it is moving) or runs (CP_RUN) the legs are two-segment limbs (thigh, knee, shin).
- The feet swing in step (walk: 14-frame cycle, run: 10), the swinging foot lifts (2 px walking, 4 running) and the knee bulges the way they are going. Shoes point that way.
- Arms counter-swing (the hand opposite the forward foot goes forward) and the hands rise with the swing; the run pumps harder and leans in.
- The body dips as the feet spread and rises as they pass (a bounce of about 1 px walking, 2 running).
- Standing, talking and every other pose keep the old straight legs. No new statics.

## Step 7 - small-sprite crying and laughing (redo 14)

In `csart.h` (small sprite only; close-ups already had their own):
- CP_CRY: sobs come in bursts (24 frames of quick heaves, then 24 of slow breaths): the shoulders heave up while the head is pulled down, the raised hands rise a pixel on the peaks. Eyes scrunched shut, two tears run down the cheeks and start again, the mouth quivers. Also used when a sad head (CP_HEAD / CP_SLUMP) is shaking with CF_SHAKE.
- CP_LAUGH: eyes shut as two upturned arcs, a wide open mouth with teeth on top and the jaw bobbing, and a tear of joy now and then. Missy keeps her glasses with the eyes simply shut.
- No new statics.

## Missy director's pass — 21 November 2026
- Mission continuity is saved: an interrupted chapter transition is replayed on loading the save, and a chapter-4 handover resumes rather than leaving the player controlling the wrong sister.
- The one-more-night medical review remains spent across reloads. A compact pressure band is stored in unused bits of the existing single-byte story memory; a reload no longer resets the hospital to the beginning of the appeal.
- Money targets were rebalanced to wages the game actually pays: 8k for the lawyer, 16k for the house, 8k for the hospital, 18k for the comeback. The story pays the corresponding contractual expenses when each relevant chapter ends.
- Film consistency: five days at the bedside (or five more after a successful extension), the whistle in the hospital before Missy recognizes it, medical caution after awakening, the no-camera recovery vow, a credible chain from rain and loss of control to the crash, no implication that she was texting while the car was moving, and an unambiguous official report in the dressing room.
- Film voice: Vance remains exploitative but Missy's responsibility is not erased. Her apology varies if recovery included slips; the original song timing, scene IDs and lyric beats are preserved.
- Camera: 16 additional beat-indexed shots in the offer, gate, drive, wings, and refill scenes. The shot-list lookup now caches its current shot and next cut; a full scan happens on entering a scene beat or a cut, not every rendered frame.
- Presentation: warmth in Missy's brief laugh and in Mamesy's letter, sombre clinic and dressing-room lighting, and removal of a redundant always-true expression in the close-up face renderer.
- Still required: GBA emulator playback at native 240x160, text readability, shot-by-shot continuity and full build/packaging checks. Static and host tests do not demonstrate cinematic frame rate.

## Regression coverage added
`tools/test_missyrules.c` checks pressure memory packing for every original
one-byte value and 0-100 pressure, including the spent one-more-night flag.
`tools/test_missy_story.py` rejects bad scene/shot indices, duplicate
camera shots at the same beat and frame, missing scene sequencing, and
accidental removal of the musical start/fade. The GBA workflow runs both
before building. A redundant, shadowed scene-6 beat-8 camera shot was removed.

### Hospital-patient AI coherence
The chapter-four Missy household member is flagged away before any household
AI decision. She cannot go to work, walk around the house, respond to talks,
or be selected while the film shows her unconscious in hospital. The guard
applies even when free will is disabled, because the normal HA_AWAY handler
otherwise sends absent Sims back inside instantly. On chapter five the
hospital flag ends and Missy's off-lot proxy is returned to a valid exit tile
before she is switched back into the player's control. A different manually
selected housemate cannot permanently block the hand-back. The goals cannot
complete while the player is controlling the wrong sister.

### Player-file continuity and camera montage invalidation
The player's story chunk is 12 bytes rather than eight: the original eight
bytes are unchanged, then comes one byte of lapse/medical-review memory and
three bytes of chapter-start cash. The reader accepts both 8 and 12 bytes;
older saves start a fair new earnings baseline and do not inherit the
previously loaded player's hospital status. This fixes switching between
saved players and restoring a chapter-four medical review from disk. The
initial handover and first story entry also hide comatose Missy immediately,
not merely after the first gameplay AI tick. The shot cache is invalidated
during a flashback montage so the shot following the memory cue is correct.

### Hospital review balancing
A hospital review now gains two points of pressure per ten seconds before
support; two friends, Mamesy's wellbeing and chapter-earned hospital money
each cancel a point. All three together slowly reduce pressure, and the
updated host test checks the full 0..100 range, including the zero-floor
saturation to prevent a negative value wrapping to 255. One-more-night
remains a last resort and is still consumed permanently once spent.

### Route 9 aftermath
Chapter five now also keeps Mamesy off the active lot after the road
accident. She no longer walks around, goes to work or turns up in a
household interaction despite the film treating her death as a fact.
Her uid remains in the story and family relationship data for continuity;
the existing chapter-five household goal explicitly does not count her.
Entering the scene from a saved game immediately hides her, just as
chapter four immediately hides unconscious Missy. This avoids altering
the household's binary save format or silently replacing a dead Sim.

### Starting or replaying the story
Starting a new story resets the prior chapter-card identity so replaying
TV SHOW & TELL in the same session still runs its opening film and initializes
the sobriety clock. A player with a hand-built (custom block geometry) body
must first rebuild a standard Sim before joining Missy's story: the cast
switching engine cannot safely transfer hand-built bodies, and silently
starting a story that would later deadlock in the hospital is worse than an
explicit entry guard. If the player customizes the body mid-story, the cast
handover displays the precise issue rather than retrying invisibly forever.

### Household UI and memorial status
In the household switch menu, Missy is labelled IN HOSPITAL during her
coma, and Mamesy is labelled REMEMBERED following Route 9 rather than
the generic OUT. Attempting to switch directly to the absent story
character explains why, without putting an unconscious or deceased
character back into player control. The menu-only name buffers are
widened modestly so the explicit labels cannot overrun their old
eight-character suffix allowance.

### Replay integrity
Starting a fresh story resets all hospital clock and review variables, not
just the saved story id. Replaying TV SHOW & TELL in an existing household
attempts to reuse the original sister by exact name and kinship before
adding a new cast member, preventing duplicate Mamesys and a potentially
unfinishable cast switch when the household is already full.

### ICU graphics director's cut
Missy now has a dedicated, frame-buffer-only three-quarter patient drawing
with anatomical side-on profile, hair silhouette, closed eyes, neckline,
gown, shoulder, blanket folds, legs and a visible hand that raises its
fingers when she wakes. The former 36-pixel reclining stick/bar shape
remains only as a fallback for non-hospital poses. At high zoom the camera
scales the actual reclining figure rather than drawing an upright face
on its side. The hospital is rebuilt with a layered mattress/pillow,
angled headboard, equipment shelf, ECG monitor with consistent heartbeat,
window/rain, IV stand and near rail that properly overlays the blanket.
The overlay uses existing clipping so it respects zoom and part redraws.
No external sprites/assets, heap memory or additional framebuffer.

### Broader stage and accident graphics
The building site now layers a city background, beams, X-braces and a
3-dimensional work platform with a visible ladder and collapsing plank at
the fall. The Route 9 cockpit has converging road lanes, near and far
tail lights, a legible truck with paired headlights and beams that grow
over the collision beat. Dressing-room staging gains a wardrobe and
backstage doorway; the finale stage has brighter grouped footlights.
The hospital near rail is lowered so it cannot cross Missy's face.
All shapes are in the existing GBA procedural renderer; no new images,
textures, music assets, framebuffer allocation or asset loading.

### Close-up face / micro-acting
At high zoom the bedside portrait now uses screen-space face anatomy:
temple, ear, cheek, nose silhouette, eyebrows, mouth and closed lids, all
aligned horizontally on the pillow. During her awakening one eyelid
opens with a brief blink, and a faint tear catches the monitor light.
Missy's round glasses rest on the nearby tray, rather than appearing to
levitate over her cheek. Only the hospital uses this new face painter;
other character models and memory budgets are unchanged. Screen-space
detail is bypassed in wide shots to preserve FPS.

### Direct-play hospital preview ROM
`make preview-missy` now builds a separate, non-release GBA ROM with
`CS_PREVIEW=4`. After the boot logo, it plays the hospital awakening
film repeatedly; use A to advance dialogue, START to skip. The normal
`bore.gba` and seven expansion packs are unchanged. GitHub Actions
uploads `bore-missy-hospital-preview` for direct mGBA testing and
checks that its GBA cartridge header is stamped MISSYWARD. To preview
the road sequence locally, use `make clean && make preview-missy CS_PREVIEW_SCENE=11`.
A preview builds the film directly; it is not a full gameplay validation.

### Native emulator smoke capture
The build workflow includes a second Linux job that downloads the real
hospital-preview GBA image, boots it in mGBA under a virtual X11 screen,
sends A/B input to the cutscene and uploads two genuine captured screens.
Basic dimensions and colour-variation checks catch a black/frozen boot.
These captures are a useful rendering sanity check, but they do not
establish frame-rate stability or full mission playthrough correctness.
If the emulator package or display setup fails, this QA job fails
openly instead of silently claiming a visual pass.

### Final coherence correction
The hospital support calculation now uses the chapter's actual 8,000
earned-money goal (previously 12,000), so meeting the displayed target
immediately affects the care-review pressure. The top construction platform
is aligned with the climbing pose's final foot height instead of floating
ten pixels above Missy's shoes. Both properties have static checks.

### Emulator capture repair
The first X11 smoke run compiled and booted mGBA, but Ubuntu's ImageMagick
`import -window` command failed before any PNG was saved. The capture now
uses `xwd -id` followed by `magick`, includes X11 diagnostics and preserves
`emulator.log` on failures. It also checks that screens actually change
between input sequences rather than accepting a frozen screenshot.
