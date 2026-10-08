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
