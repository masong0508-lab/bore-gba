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


## Standard 4 MiB Nintendo DS lyric fidelity and playback

The normal hardware DS and the DS-compatible DSi ROMs now include **all 56
Corporations sung events and 18 Duck & Cover vocal pads** in two resident
ADPCM banks. The ARM7 sample bank is 370,752 bytes and the ARM9 second bank
is 421,348 bytes before the adaptive fidelity pass below; check generated
run logs for current sizes. Save compression uses incremental RLE in a
cache-synchronized ARM9 BSS buffer, with the old save format retained.

The original Corporations 4.5-kHz fixed sampling has been replaced by a
**phrase-specific 4.4–5.2-kHz rate**. Short lyrics use better high-frequency
detail while long ones reduce only as needed to stay inside the two existing
0x9800-byte PCM decode slots. Reduced wet reverb tails reclaim several
dozen KiB of resident storage for the actual singer. The MIDI pitch, timing,
words and vocal identities are unchanged. All vocal samples must remain
present and all sample lengths below the scratch-slot limit; CI checks both
the generated sound banks and byte-for-byte copies inside each released
hardware DS ROM.

The optional Calico/libnds `godguy.nds` build retains a smaller bank until
its different memory/save layout has an independently validated full-voice
solution. For complete 4-MiB-DS audio, use `godguy_hw.nds`.
