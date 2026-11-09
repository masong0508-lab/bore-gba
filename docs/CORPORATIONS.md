# CORPORATIONS (YOU'RE WINNER): a song with vocals for the GBA

Built by `tools/make_corporations.py` from six isolated stems (bass, drums, other, guitar, piano, vocals; 6:04, 44.1 kHz stereo). Run it from the project root with ffmpeg, numpy and scipy;
`PREVIEW=1` also writes the two listening files /tmp/corporations_game.wav and /tmp/corporations_split.wav.

## What the stems showed
| stem | level | note |
|---|---|---|
| bass | -21.3 dBFS | carries the groove |
| drums | -22.0 dBFS | gives the tempo: **131.00 BPM**, first beat at 0.344 s |
| other | -29.4 dBFS | the synths / pads |
| guitar, piano | -45.7 / -44.6 dBFS | very quiet, texture only |
| vocals | -28.0 dBFS (digital silence between lines) | sung for 171 s of the 364 s, in 56 lines (longest 6 s) |

In the original balance the voice sits 6.2 dB BELOW the instruments while it sings, which a 4-bit, 18 kHz, 9 KB/s stream cannot carry (no headroom, nothing above 7.5 kHz).

## What is done to the singing
1. high-pass at 95 Hz, a presence lift (+3.5 dB at 2.8 kHz), a small cut at 300 Hz
2. a fast 8:1 compressor (2 ms attack, 60 ms release) then a slow 3:1 levelling compressor (12 / 220 ms), then a soft clipper: the voice stays at one loudness, crest factor 7.8 dB
3. the room: the instruments are split in three bands; while the voice sings the mid band (600 Hz - 5 kHz, where the voice lives) is pulled down by up to 5 dB and the top band by 1.5 dB
   (20 ms attack, 150 ms release, driven by the voice activity), and the voice ends up 2 dB BELOW the instruments (it was 6.2 dB below in the stems; the first version of this song had it 2.5 dB above and brighter)
4. the master is limited, low-passed at 7.5 kHz, levelled (loudnorm) and encoded by `tools/encode_song.py`'s own functions: mono 4-bit IMA-ADPCM at 18157 Hz -> `source/music/corporations.adp`, 3,227 KB,
   ADPCM SNR 26.7 dB; `source/songs.h` gets `SONG_ADP(corporations,"CORPORATIONS (YOU'RE WINNER)",...)`. It plays in the jukebox like any streamed song: no engine change.

The ROM still links with it (a real build: base game 14.0 MB of the 25.2 MB before the first pack slot).

## Vocal timing (theoretical split version)
`tools/corporations_vocal_timing.json` lists every line: start and end in seconds, in beats at 131 BPM, in sixteenths, in GBA music samples (18157 Hz, the mixer's clock) and in 60 Hz frames,
plus the clip's offset, length and a volume byte. The singing alone is stored as **2-bit adaptive delta** (IMA step table, a sign bit and one magnitude bit, index +3 / -1), 8 kHz, band-limited to 110 Hz - 3.6 kHz,
only where somebody sings: `tools/corporations_vocal2.bin`, 349 KB for 178.6 s (2 KB/s; the whole song as one stream is 8.9 KB/s).
* Timing: decoded clips put back at their start samples line up with the singing of the mix to within **1 sample** (median 0), so a mixer that starts clip n when the music clock reaches `gba_sample` keeps the voice in sync.
* 28 of the 56 line starts fall within 30 ms of a sixteenth note (median distance 31 ms): the singer swings, so do not quantise the starts to the grid, use the table.
* Quality: the 2-bit voice alone has 9.1 dB SNR (speech at 2 bits per sample is rough); the split mix correlates 0.76 - 0.83 with the single 4-bit stream. The single 4-bit stream (SNR 18.5 dB after level fit) is the better sounding one
  and costs about the same (3,227 KB against 3,228 + 349 KB), which is why that is what the jukebox plays.

How a GBA engine would play the split version: the effect-voice mixer already plays 4-bit ADPCM voices at their own rate next to the music; a 2-bit decoder is the same loop with a 4-entry code
and the same step table, so a vocal voice needs `{u32 startSample, u32 offset, u16 samples, u8 volume}` per line (the JSON), started when the music stream's sample counter passes `startSample`.

## Softer vocals (second pass)
The first version had the voice 2.5 dB above the instruments with a +3.5 dB lift at 2.8 kHz and no de-esser: bright and a little sharp in a 4-bit stream. Now (`vocal_chain` in tools/make_corporations.py, shared with
tools/corp_build.py): high-pass 110 Hz, +1.5 dB warmth at 200 Hz, -2 dB at 350 Hz (mud), +1.2 dB presence at 2.2 kHz, **-3 dB at 4.5 kHz** (the harsh band), a **de-esser** on 5-9 kHz (4:1 above its
usual level), the same two compressors, and the voice **2 dB under the instruments** while it sings (`VOICE_DB`). In the hybrid preview the vocal clips play at 0.55 of their old gain (`default_gain` in digital.json).

Third pass: the voice sits 3.5 dB under the instruments (`VOICE_DB = -3.5`).
