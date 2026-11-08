# CORPORATIONS, hybrid version: transcribed bass, piano and drums + digital guitar, vocals and synths

The six stems (6:04) split into two groups so the song can be remixed and revised:

| group | stems | how it is kept |
|---|---|---|
| **notes** (editable) | bass, piano, drums | transcribed: `tools/corporations/corporations_hybrid.xm` (tracker), `corporations_performance.mid` (exact times) and `corporations_grid.mid` (1/32 grid) |
| **digital** (4-bit samples) | vocals, guitar (and the other synths, which cannot be transcribed) | IMA-ADPCM 4-bit, mono, 18157 Hz (the game's song format), cut into clips with exact start samples: `tools/corporations/digital/*.adp` + `digital.json` |

Rebuild with `python3 tools/corp_transcribe.py && python3 tools/corp_build.py && python3 tools/corp_eval.py` (needs ffmpeg, numpy, scipy, librosa, mido, soundfile and `basic-pitch[onnx]`; the stems directory is `STEM_DIR`).

## What the analysis found
* **Key: E major** (Krumhansl correlation 0.90 on the bass and piano notes; E minor only 0.59).
* **Tempo 131.005 BPM**, downbeat at 0.152 s (found where the kick lands most often in the bar); mean distance of the bass and kick onsets to the 1/32 grid 12.7 ms.
* **The track is 40 cents flat** (A = 430 Hz): every bass note's f0 sits 40 cents under the semitone. The tuning is measured (circular mean of the fractional pitch) and removed before the piano is
  transcribed, and it is written into the files: the MIDI files carry it as RPN fine tuning (-40 cents), the tracker instruments carry it in their finetune. The notes themselves are the written
  pitches (the key of E), so a remix can leave the tuning or retune it to 440.
* **Why the bass is not just "basic-pitch"**: the neural transcriber lists the harmonics of this sub bass as extra notes (octave, twelfth ...). The bass is therefore built from the fundamental:
  pYIN f0 -> tuning removed -> 50 ms median filter -> cut into notes where the pitch changes and where the loudness re-strikes. basic-pitch stays as a cross-check:
  of its 2,113 bass notes 878 are the same pitch as ours, 988 are harmonics of our note under them and only 247 (12 %) disagree.

## Accuracy (the module played back with its own instruments, against the stems; `accuracy.json`)
| part | result |
|---|---|
| bass pitch | pYIN on the stem vs pYIN on the module's bass: **94.7 % of the frames within 50 cents, 98.9 % within a semitone, 0 % an octave off** (27,386 frames) |
| bass | log-spectrum cosine 0.89, loudness-curve correlation 0.81, note starts vs the stem's own onsets F1 0.83 (recall 0.92) |
| piano | chroma cosine 0.69 (it was 0.64 at the model's default threshold; lower thresholds give more notes for little more: 0.70 at twice the notes), loudness correlation 0.80. The spectrum cosine (0.28) mostly measures the timbre of the stand-in piano, and the onset precision (0.09) the fact that the piano stem is quiet and sparse (-44 dBFS, sounding in 29 % of the song): the transcription follows the model, the stem's own onset detector finds far fewer attacks. Treat the piano as a good draft, not a score. |
| kick / snare / hat | onsets F1 0.77 / 0.81 / 0.88; band loudness correlation 0.54 / 0.55 / 0.66 (a break with very busy hits; thresholds chosen by how well the hits rebuild each band's loudness curve) |
| timing | quantising to 1/32 rows moves a note by 14 ms on average (28.6 ms at most); the MIDI "performance" file keeps the exact times |

## The module (`corporations_hybrid.xm`)
Speed 3 at 131 BPM = one row per 1/32 note, 64 rows per pattern (two bars), 101 patterns, 15 channels, 6 instruments (bass, piano, kick, snare, hat, cue) in 150 KB:
channel 1 bass (one voice, key-offs between notes), 2-9 piano (first free voice), 10-12 kick / snare / hat, **13-15 CUE lanes**: a note on the lane = "clip number N of the vocals / guitar /
other stem starts here" (the exact start sample is in `digital.json`: use it, the row is only 1/32 accurate). The instruments are stand-ins (a warm saw for the bass, an additive piano, synthesised
drums) made so the module plays on its own; swap them for real samples when remixing. The module is plain FastTracker II format (it also opens in OpenMPT / MilkyTracker) and passes the
repo's own parser (tools/xm.py). It uses 15 channels and 6 instruments, inside the limits of tools/xm2gba.py (16 / 32); the game's player ignores note-off, so on the GBA the bass and piano
notes ring until the next note on their channel.

## The digital stems
| stem | clips | size | notes |
|---|---|---|---|
| vocals | 56 | 1,569 KB | after the compression and the room carving of docs/CORPORATIONS.md; clips are the sung lines, each with its own peak (`vol_peak`) |
| guitar | 5 | 167 KB | only 19 s of the song |
| other | 11 | 3,237 KB | the synths: 365 s in clips of at most 40 s cut at the quietest moment; drop it if you want the song thinner |

Playing it on the GBA would need the music mixer to run the tracker and an ADPCM voice together (the effect voice already plays 4-bit clips beside the music; the clip table gives the sample at which
each clip must start). The song that plays in the jukebox today is still the single 4-bit stream of docs/CORPORATIONS.md.
