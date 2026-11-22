# WINNER / CORPORATIONS — direct Koala Sampler source in BORE 1

The active jukebox song **CORPORATIONS (YOU'RE WINNER)** is now rebuilt
directly from the user's original `Winner.koala`, **not** from guesses
based on isolated audio stems. The full Koala project is not committed (43 MB);
the exact compact 339,248-byte compressed XM module is tracked instead as
`tools/winner_koala.xm.xz`, with the user's 23 original playable pad samples.
Run `make` and `tools/install_winner_koala.py` installs just that one new
module into BORE's existing music bank before compiling.

## Source/arrangement inventory

- Original Koala project checksum SHA256:
  `98ab503f5e2ff1c0712c5c8a7ffd67bbf56e9f6f79e65b75a045685afbc62b5b`.
- Koala BPM **127.565125**, swing **0.620229**. Seven nonempty sequence sections,
  **9 + 25 + 4 + 16 + 16 + 8 + 128 = 206 bars**, **8,813 note events**.
- Converted standard XM: 16 playback channels, 23 real pad instruments,
  206 patterns, 64 tracker rows/bar, speed 2, BPM 170. Timing varies by
  less than 0.1% from the original; swing is mapped at note-event level.
- XM checksum SHA256:
  `b3d5d2cda538f1987c34292f2e94d2218f217c3811bfe0321cd0ab33e61279a4`.
  1,386,911 bytes uncompressed, 339,248 compressed.
- Original 48-kHz pad WAVs were trimmed to their Koala start/end boundaries,
  mixed to mono with phase-cancellation safeguards and antialiased to 16 kHz;
  no harsh 3-kHz lowpass or synth sound reconstruction. 23 samples total.
- Playback on BORE is **not** bit-perfect to Koala: the GBA player has only 16
  channels, 32 instruments, and one-shot 8-bit PCM, and does not implement
  Koala's flanger, delay, reverb, sampler envelopes, or compression.
  Under heavy polyphony 666 active-channel voice tails were reallocated;
  **no note-on events were dropped**. The XM has a playable 6:28 arrangement.

## Jukebox visibility

**CORPORATIONS (YOU'RE WINNER)** is the new default, visible in the jukebox,
genre radio, game shuffle, and menu music. Existing older interpretations
remain stored, but **both are secrets**, accessible only for the current
debug-code session via `UP UP DOWN DOWN LEFT LEFT RIGHT B A START`
on the title screen:

- `YOU'RE WINNER (ORIGINAL)` — the stem-derived synthesized XM arrangement.
- `CORPORATIONS (ORIGINAL)` — the prior 4-bit ADPCM master built from stems.

Neither secret is in `source/unlocks.h`; neither is earnable from gameplay
and neither appears on radio or shuffled playlists normally.

## Rebuild and verify

From a clean checkout with devkitARM, Python 3, numpy, scipy installed:

```sh
python3 tools/test_winner_koala.py
python3 tools/install_winner_koala.py
make
make creator
```

The importer is available for refreshing from the original source:

```sh
python3 tools/import_winner_koala.py Winner.koala
# Re-compress the XM with LZMA/XZ and update the expected SHA256
# in tools/install_winner_koala.py and tools/test_winner_koala.py.
```

The installation script is idempotent: later `make` invocations reuse the
prepared bank. It preserves every old XM bank offset, appending only the Koala
song and its samples. `tools/test_winner_koala.py` independently checks all
**8,813 XM note events**, 206 patterns, 16 channels, 23 instruments, and the
secret-versus-unlockable naming rules. Native ROM build, audio playthrough,
RAM, ROM, and emulator checks must be accepted separately.

Other games using a Winner recording are **not modified** in this milestone.

## Vocal recovery and authentic synth releases — shuffle-timing pass

The original song mix (131 BPM) and two user-sung timing demonstrations
(normal 4.18 seconds, shuffle 4.22 seconds) guide vocal placement. The
**existing isolated original vocal clips** in `tools/corporations/digital/`
are restored alongside the Koala arrangement; all 56 phrases retain
their original pitch, sample rate and intra-phrase phrasing. Only each
line's *entrance* is placed on the Koala 127.565 BPM / 62% swing grid,
with a modest 45ms pickup/lag. This intentionally avoids globally
speeding/slowing the singer or slurring consonants. Check the audible
phrases against the original mix: finer syllabic retiming, where needed,
remains a listening decision, not blind quantization.

Instrument 6 in the XM corresponds to original Koala pad 6
(`voice-C2_hum_long.flac`), a ~9s sampled sustained tone played by
**716 short note events**. Previously, BORE's tracker ignored note-off;
the flute-like sound therefore kept ringing long after the score.
`tools/winner_flute_gates.b64` is a tiny, LZMA-packed map of those
**exact** note lengths from `Winner.koala`. The build inserts 715
declicked note-off events; one is naturally preempted by the next note
on its channel. The original 8,813 attacks, their pitches, original
23 samples, 206 bars, and 16 playback channels are unchanged.

The vocals use the already versioned 4-bit ADPCM isolated vocal data
(~1.6MB) as a two-voice overlay synchronized to the XM's own row clock.
Ordinary sound effects remain an independent voice and are not stolen
for singing; the song's crossfade transports its vocal state with it.
The old (ORIGINAL) songs remain debug-code-only secrets. The source
Koala XM archive is preserved byte-for-byte: installer applies the
releases only to its generated copy, then appends the rebuilt event
bank. The 4-bit legacy vocal source cannot be magically restored to
lossless quality; note entry placement and audio taste must still be
reviewed at actual GBA audio output. No global song speed, song pitch,
voice stretch, extra framebuffer, or large RAM allocation is introduced.
