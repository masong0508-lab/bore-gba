# CORPORATIONS: the transcribed version (v2, re-analysis)

The first hybrid conversion sounded cluttered and off in time. It was redone from a closer study of the stems. What was wrong, and what is different now:

| problem found | what v2 does |
|---|---|
| the bar line was an eighth note early and everything sat on a straight 1/32 grid | the real grid is 131.004 BPM with the bar line found from the drum fills (every 4th / 8th bar); the module keeps time in **tracker ticks** (1 tick = 1/6 sixteenth, 19.08 ms), one row per tick |
| the off sixteenths are swung (about 32 ms late) and were rounded to the straight grid | the swing is on the ticks (tick 2 of the off sixteenths) |
| the master has two edits (from bar 158 beat 3 everything is 104 ms later, from bar 174 19 ms later) | both are followed (+5 and +1 ticks) |
| a neural note model heard every overtone and echo as a note ("clusters") | the keys are a **harmonic factorisation** (fixed partial weights) of a semitone spectrum, notes only where an activation stands out; the bass is one pitch per tick from the stem itself |
| drums were three onset detectors | the drums are a **drum machine**: every hit of a drum is the same waveform (kick 0.99, snare 0.94 correlation), so the samples are recovered by aligned averaging and the part is taken apart by matched filters (kick, snare, tom, open and closed hat); gains are note volumes |
| instruments were guessed (piano / synth) | an audio tagger (PANNs) and a text-audio model (CLAP) were run on each stem section by section: the "piano" and the keys parts of "other" are a **Rhodes**, the "guitar" stem is the Rhodes' chorus layer in the intro, and the other stem's bars 75-110 and 142-196 are a **horn / slide-guitar / sax lead** |

## Which stem is what, and how it is stored
| stem | what it is | stored as |
|---|---|---|
| bass | a plucked sub bass, 45 cents flat, octave jumps (B1 B2 A1 A2 ...) | **tracked** (1,700 notes, 1 channel) |
| piano | Rhodes | **tracked** (985 notes, 5 voices) |
| other, bars 0-9 / 21-26 / 37-75 / 110-142 | Rhodes / vibes / pad | **tracked** (851 notes, 4 voices) |
| other, bars 75-111, 118-123, 130-134, 142-196 | horn / slide guitar / sax lead | **digital** 4-bit clips (`digital/lead.adp`) |
| vocals, guitar | voice (softened -3.5 dB), guitar stem | **digital** 4-bit clips (`digital/*.adp`, `digital.json`) |
| drums | drum machine: kick, snare, tom, open / closed hat | **tracked** kit samples cut from the stem (`kit/*.wav`) and 2,440 hits |

## Files (tools/corporations/)
`corporations_v2.xm` (16 channels, speed 1 / 131 BPM, 96 rows = 1 bar, bar 0 = first sound; cue lanes 14-16 mark where each digital clip starts), `corporations_v2.mid` (bass RPN-tuned 45 cents flat), `bass.json`, `keys_piano.json`, `keys_other.json`, `drums.json`, `kit/`, `digital/`, `digital.json`, `accuracy_v2.json`.

## Accuracy (against the original mix of the six stems; the old version measured the same way)
| | old hybrid | v2 |
|---|---|---|
| onset-envelope correlation (timing) | 0.10 | **0.75** |
| mean timing lag per section | 28 ms | **0 ms** |
| chroma (harmony) similarity | 0.81 | **0.93** |
| mel-spectrum similarity | 0.50 | **0.77** |
| Rhodes (piano stem) note vs stem, chroma / semitone | 0.76 / 0.48 | **0.89 / 0.78** |
| Rhodes (other stem) | 0.59 / 0.30 | **0.87 / 0.75** |

## Rebuild
```
python3 tools/corp_drums.py          # kit + hits
python3 tools/corp_bass.py           # bass notes
python3 tools/corp_keys.py piano; python3 tools/corp_keys.py other
python3 tools/corp_build2.py         # xm + mid + digital lead clips + preview (preview wav is not committed)
python3 tools/corp_eval2.py          # accuracy_v2.json
```
Needs ffmpeg, numpy, scipy, librosa, mido, soundfile (`STEM_DIR` = the stems directory). The tagging study used PANNs Cnn14 and `laion/larger_clap_music_and_speech` (not needed to rebuild).
Helper modules: `corp_lib.py` (grid / ticks), `corp_hnmf.py`, `corp_bass_f0.py`, `corp_score.py`.
The game itself still plays the single-stream version of the stems (`tools/make_corporations.py`, `source/music/corporations.adp`), which is the stems themselves and unchanged.

---
# DUCK & COVER (DNB REVISION)
`python3 tools/make_duckcover.py path/to/Duck__Cover.koala` rebuilds it: the Koala Sampler project (145 BPM; every note already on an eighth of a beat) re-written as a 174 BPM drum & bass track.
All 12 sample pads are spoken / rap vocal chops (CLAP tagging), pad 9 is a 62 Hz (B1) sub sample, pad 1 the QUOKKA filtered-saw pluck (E minor: pluck pitch 0 = B, bass notes E/G and D/F#).
Fixes: each chop's lead-in (40-136 ms of breath before the first syllable) is trimmed so the syllable lands on the beat; chops are time-stretched (pitch kept) to a whole number of half beats at 174 BPM, the break exactly by 145/174; unused parts of the recordings come back as swells / stabs; new kick, snare, ghost snares, hats, fills, risers, sidechain duck; sub + reese bass on the project's own bass rhythm.
Files: `tools/duckcover/duck_cover_dnb.xm` (16 channels, speed 3 / 174 BPM), `.mid`, `source/music/duck_cover.adp` (the game's stream, SONG_ADP in songs.h). 101 bars, 2:23.
