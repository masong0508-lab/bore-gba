# Music quality control (all 168 tracks)

`tools/gm_qc.py` measures every track exactly as it is packed; `tools/gm_up.py` is the pass every track goes through before packing (`gm2_compose.write_header`);
`tools/gm_loud.json` is the per-track loudness trim, measured by rendering every track (`pc_music`). Run `python3 tools/gm_qc.py` (before the upgrade) and `python3 tools/gm_qc.py up` (after).

| measure (mean over 168 tracks) | before | after |
|---|---|---|
| notes outside the key and outside the chord under them | 5.9 % | 1.0 % |
| lead against bass a semitone / major seventh apart | 5.5 % | 0.1 % |
| bars that repeat an earlier bar note for note (lead + bass) | 74.6 % | 34.3 % |
| phrases (4 bars) that end in a drum fill | 42 % | 52 % (every phrase of every track that has a kit and is not ambient) |
| loudness spread between tracks (std dev of RMS, jingles and the war track excluded) | 2.0 dB | 0.7 dB |
| tracks that clip / peak over 0.95 | 0 / 0 | 0 / 0 |

What the pass does (details in the head of `gm_up.py`): repairs (off-key notes, bass clashes), a different treatment for every repeat of a bar (neighbour-note turns, passing tones, an octave pickup,
a third below the lead on the harmony voice, bass octave jumps, fifths and approach notes, a rising sparkle arpeggio), a fill at the end of every phrase and a crash on repeated eights, a thin
introduction (no counter-line, harmony or sparkle for eight bars), a snare build and a breath at the end of every sixteen, a thin ending, and a guarantee that no bar is left note for note like an earlier one.
The two MIDI-read songs (CORPORATIONS, DUCK & COVER) keep their original melodic MIDI notes and timing. A further 2026 vocal-arrangement pass replaces hard honking effects with warm, rhythm-locked vocal cuts and smooth melodic responses. Off-line mastering reduces nasal formants, de-rumbles phrases, reduces oversized plate echoes, and eases the pitch-quantization on DUCK & COVER without changing lyrics. New assets are produced by tools/gm_vox.py and tools/gm2_compose.py; the 3DS, DS, and DSi build workflows regenerate them from sources before compiling.
