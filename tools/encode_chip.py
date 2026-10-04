#!/usr/bin/env python3
"""Pack the chiptune loops of tools/make_chiptunes.py for the game: looping 4-bit IMA-ADPCM, mono, 18157 Hz (the game's music rate).

usage:  python3 tools/encode_chip.py [chiptune_out]       (run from the project root; needs numpy + scipy)
        reads <chiptune_out>/songs/NN_id.wav and <chiptune_out>/secret/NN_id.wav (run make_chiptunes.py first)
        writes source/music/chip_<id>.adp for each, and rewrites source/chips.h (the list the game builds its creator-menu music from)

How the loop stays seamless: the loop is band-limited and resampled with an FFT (which treats it as periodic, so the join is exact), then ADPCM-coded
starting from a decoder state that is iterated until the state at the END of the loop equals the state at the START. The file stores that state, and the
player starts every pass from it. File layout: u32 sample count | bit 30 (= loops) , u32 predictor | index << 16, then the 4-bit samples (low nibble first).
Loudness: peak 0.95 of full scale, which puts the loops at the level of the tracker songs (about 4000 RMS on the speaker, the songs sit between 3000 and 6600).
"""
import sys, os, wave, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
from encode_sfx import STEP, IDX
import make_chiptunes as MC

RATE = 18157
CHIPS_H = "source/chips.h"

def read_wav(path):
    w = wave.open(path); n = w.getnframes(); x = np.frombuffer(w.readframes(n), dtype='<i2').astype(float) / 32768.0
    return x, w.getframerate()

def to_game_rate(x, fs):
    n = int(round(len(x) * RATE / fs))
    X = np.fft.rfft(x); f = np.fft.rfftfreq(len(x), 1 / fs); X[f > 7500] = 0        # nothing above what 18 kHz can hold, exactly periodic
    x = np.fft.irfft(X, len(x))
    y = signal.resample(x, n)                                                        # FFT resample: the loop stays a perfect loop
    return y / (np.abs(y).max() + 1e-9) * 0.95

def enc(x, pred, idx):
    out = []
    for s in x:
        step = STEP[idx]; d = int(s) - pred; n = 0
        if d < 0: n = 8; d = -d
        if d >= step: n |= 4; d -= step
        if d >= step >> 1: n |= 2; d -= step >> 1
        if d >= step >> 2: n |= 1
        diff = step >> 3
        if n & 1: diff += step >> 2
        if n & 2: diff += step >> 1
        if n & 4: diff += step
        pred += -diff if n & 8 else diff
        pred = max(-32768, min(32767, pred)); idx = max(0, min(88, idx + IDX[n & 7]))
        out.append(n)
    return out, pred, idx

def dec(nib, pred, idx):
    y = []
    for v in nib:
        step = STEP[idx]; diff = step >> 3
        if v & 1: diff += step >> 2
        if v & 2: diff += step >> 1
        if v & 4: diff += step
        pred += -diff if v & 8 else diff
        pred = max(-32768, min(32767, pred)); idx = max(0, min(88, idx + IDX[v & 7])); y.append(pred)
    return np.array(y), pred, idx

def encode_loop(x16):
    s = (0, 0)
    for k in range(12):                                   # find the decoder state a pass can start from and end on
        nib, p, i = enc(x16, *s)
        if (p, i) == s: break
        s = (p, i)
    nib, p, i = enc(x16, *s)
    body = bytes((nib[k] | ((nib[k + 1] if k + 1 < len(nib) else 0) << 4)) for k in range(0, len(nib), 2))
    b = struct.pack('<II', len(x16) | 0x40000000, (s[0] & 0xFFFF) | (s[1] << 16)) + body
    return b + bytes((-len(b)) % 4), s, (p, i), nib

if __name__ == '__main__':
    out_dir = sys.argv[1] if len(sys.argv) > 1 else 'chiptune_out'
    rows = []; total = 0
    os.makedirs('source/music', exist_ok=True)
    for group, lst, secret in (('songs', MC.SONGS, 0), ('secret', MC.SECRET, 1)):
        for idx, (sid, path, title, opts) in enumerate(lst, 1):
            wav = os.path.join(out_dir, group, '%02d_%s.wav' % (idx, sid))
            if not os.path.exists(wav): print('  (missing %s: run make_chiptunes.py first)' % wav); continue
            x, fs = read_wav(wav); y = to_game_rate(x, fs); x16 = np.round(y * 32767).astype(int)
            b, s0, s1, nib = encode_loop(x16)
            dpath = 'source/music/chip_%s.adp' % sid; open(dpath, 'wb').write(b); total += len(b)
            d, _, _ = dec(nib, *s0); snr = 10 * np.log10(np.sum(x16.astype(float) ** 2) / max(1, np.sum((x16 - d).astype(float) ** 2)))
            seam = abs(int(d[0]) - int(d[-1])); typ = float(np.abs(np.diff(d)).mean())
            print('%-32s %5.1f s  %4d KB  SNR %4.1f dB  start state %s, end state %s%s' % (title, len(x16) / RATE, len(b) // 1024, snr, s0, s1, '  (settled)' if s0 == s1 else '  (NOT settled)'))
            rows.append((sid, title, secret))
    with open(CHIPS_H, 'w') as f:
        f.write('// chips.h - the chiptune loops of the creator menu: made by tools/make_chiptunes.py, packed by tools/encode_chip.py (do not edit)\n'
                '//   CHIP(id,"NAME",secret)   secret = 1: only after the title-screen code (UP UP DOWN DOWN LEFT LEFT RIGHT B A START)\n')
        for sid, title, secret in rows: f.write('CHIP(chip_%s,"%s",%d)\n' % (sid, title, secret))
    print('%d loops, %d KB of ROM in all -> %s' % (len(rows), total // 1024, CHIPS_H))
