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
