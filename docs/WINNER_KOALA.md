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
