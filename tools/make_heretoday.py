#!/usr/bin/env python3
"""HERE TODAY (MISSY'S SONG): a chiptune rendition of the original melody hummed by the game's author (the melody comes from a voice recording, tracked
note by note; the lyrics in source/cutscene.h are original too). Run from the project root:   python3 tools/make_heretoday.py
  1. writes /tmp/here_today.wav   2. encode it with  python3 tools/encode_song.py /tmp/here_today.wav  (or use encode_to_adp below, which does both)
Edit PHRASES to change the tune: each phrase is [start s, end s, [[start s, length s, MIDI note], ...]]. The cutscene (cutscene.h, scene 5) shows one lyric line per
phrase and its beat lengths are worked out from these phrase starts, so if you move a phrase, change that beat's length too (see the comment on the lyric beats).
Sound (v2, cutscene redo 11): same melody and same timing (the lyric beats depend on it). The lead is a 25 % pulse one octave up, doubled by a slightly detuned 12.5 % pulse, with a short slide into each note, vibrato on held notes and a soft three-tap echo.
Under it, chords are worked out from the melody by a small search (best-fitting triad per slot, smooth motion between them): a triangle bass (root on the slot start, fifth halfway, a pickup on long slots), a quiet 12.5 % pulse
arpeggio and a held 50 % pulse pad. Everything swells toward the high section (about 32 s to 50 s) and thins out again for the last lines; the last chord rings out."""
import numpy as np, wave, sys
SR = 44100
PHRASES = [[0.15, 1.35, [[0.15, 1.07, 59],
  [1.22, 0.13, 56]]],
  [2.21, 3.25, [[2.21, 0.22, 56],
  [2.43, 0.25, 59],
  [3.02, 0.23, 60]]],
  [4.91, 7.22, [[4.91, 0.19, 62],
  [5.33, 0.13, 62],
  [5.84, 0.16, 58],
  [6.0, 0.13, 59],
  [6.28, 0.16, 57],
  [6.44, 0.25, 55],
  [6.89, 0.33, 55]]],
  [8.39, 10.03, [[8.39, 0.22, 55],
  [8.81, 0.21, 57],
  [9.25, 0.57, 55],
  [9.82, 0.21, 54]]],
  [11.02, 14.04, [[11.02, 0.16, 54],
  [11.18, 0.88, 55],
  [12.06, 1.07, 53],
  [13.13, 0.14, 51],
  [13.27, 0.77, 50]]],
  [15.09, 16.62, [[15.09, 0.26, 50],
  [15.54, 0.3, 51],
  [16.12, 0.5, 47]]],
  [18.82, 20.73, [[18.82, 0.14, 46],
  [19.17, 0.15, 48],
  [19.66, 0.64, 48],
  [20.57, 0.16, 52]]],
  [22.11, 23.89, [[22.11, 0.33, 50],
  [22.85, 0.5, 53],
  [23.74, 0.15, 50]]],
  [26.09, 27.54, [[26.09, 0.39, 50],
  [26.48, 0.18, 51],
  [26.94, 0.3, 53],
  [27.41, 0.13, 51]]],
  [28.3, 31.96, [[28.3, 0.3, 55],
  [28.79, 0.2, 51],
  [29.23, 0.28, 51],
  [29.88, 0.37, 55],
  [30.64, 0.13, 59],
  [31.15, 0.32, 59],
  [31.63, 0.19, 63],
  [31.82, 0.14, 64]]],
  [32.84, 33.84, [[32.84, 0.37, 67],
  [33.21, 0.2, 66],
  [33.62, 0.22, 67]]],
  [35.56, 38.01, [[35.56, 1.15, 67],
  [36.93, 0.15, 64],
  [37.3, 0.36, 64],
  [37.66, 0.35, 63]]],
  [40.21, 42.59, [[40.21, 0.27, 67],
  [40.48, 0.18, 69],
  [40.91, 0.17, 69],
  [41.08, 0.7, 71],
  [41.78, 0.31, 72],
  [42.35, 0.24, 71]]],
  [43.8, 45.44, [[43.8, 0.25, 69],
  [44.22, 0.2, 71],
  [44.67, 0.41, 72],
  [45.28, 0.16, 74]]],
  [47.64, 48.17, [[47.64, 0.14, 66],
  [47.78, 0.27, 67],
  [48.05, 0.12, 69]]],
  [48.88, 49.51, [[48.88, 0.33, 66],
  [49.37, 0.14, 64]]],
  [50.38, 55.09, [[50.38, 0.17, 65],
  [50.55, 1.19, 67],
  [52.15, 1.2, 65],
  [53.8, 0.62, 67],
  [54.42, 0.13, 65],
  [54.7, 0.39, 65]]],
  [55.6, 57.95, [[55.6, 0.53, 63],
  [56.13, 0.23, 62],
  [56.52, 0.58, 62],
  [57.1, 0.13, 60],
  [57.67, 0.28, 59]]],
  [60.15, 65.07, [[60.15, 0.19, 55],
  [60.62, 1.22, 55],
  [62.21, 1.32, 53],
  [63.99, 1.08, 51]]],
  [65.77, 70.65, [[65.77, 0.84, 50],
  [66.87, 1.09, 48],
  [68.16, 0.5, 47],
  [69.14, 0.23, 48],
  [69.7, 0.13, 47],
  [70.17, 0.48, 47]]]]
def midi_hz(m): return 440.0 * 2 ** ((m - 69) / 12.0)
TAIL = 1.8                                           # seconds the last chord rings after the last melody note
SWELL = [(0, 0.55), (12, 0.70), (28, 0.80), (32, 1.00), (50, 1.05), (58, 0.80), (66, 0.60), (72, 0.50)]   # the accompaniment's level over the song

def swell(t): return np.interp(t, [p[0] for p in SWELL], [p[1] for p in SWELL])

def slots():
    """Chord slots: one per phrase, a phrase longer than 3.2 s is cut in two at the note nearest its middle. -> [(start, end, [(len, midi), ...])]"""
    out = []
    for k, (ps, pe, notes) in enumerate(PHRASES):
        nxt = PHRASES[k + 1][0] if k + 1 < len(PHRASES) else pe + TAIL
        if pe - ps > 3.2 and len(notes) > 3:
            mid = (ps + pe) / 2; j = min(range(1, len(notes)), key=lambda i: abs(notes[i][0] - mid))
            out.append((ps, notes[j][0], [(n[1], n[2]) for n in notes[:j]]))
            out.append((notes[j][0], nxt, [(n[1], n[2]) for n in notes[j:]]))
        else:
            out.append((ps, nxt, [(n[1], n[2]) for n in notes]))
    return out

CH = [(r, q) for r in range(12) for q in (0, 1)]       # (root pitch class, 0 major / 1 minor)
def tones(c): r, q = c; return [r, (r + (3 if q else 4)) % 12, (r + 7) % 12]
def fit(c, notes):
    t = tones(c); s = 0.0
    for i, (ln, m) in enumerate(notes):
        w = ln * (1.5 if i == 0 else 1.0); pc = m % 12
        s += w * ((2.0 if pc == t[0] else 1.6 if pc == t[1] else 1.3 if pc == t[2] else -1.0))
    return s
def link(a, b):
    ta, tb = set(tones(a)), set(tones(b)); s = 0.4 * len(ta & tb) - (0.35 if a == b else 0.0)
    d = (b[0] - a[0]) % 12; s += 0.5 if d in (5, 7) else 0.1 if d in (2, 3, 4, 8, 9, 10) else 0.0
    return s
def key_bonus(c):   # the tune sits around G minor / Bb: favour its own chords a little
    return 0.6 if c in [(7, 1), (0, 1), (2, 0), (2, 1), (3, 0), (5, 0), (10, 0), (7, 0)] else 0.0
def harmonise(sl):
    n = len(sl); best = [{c: fit(c, sl[0][2]) + key_bonus(c) for c in CH}]; back = []
    for k in range(1, n):
        cur = {}; bk = {}
        for c in CH:
            p = max(CH, key=lambda a: best[-1][a] + link(a, c)); cur[c] = best[-1][p] + link(p, c) + fit(c, sl[k][2]) + key_bonus(c) + (3.0 if k == n - 1 and c == (7, 0) else 0.0); bk[c] = p
        best.append(cur); back.append(bk)
    c = max(CH, key=lambda a: best[-1][a]); path = [c]
    for bk in reversed(back): c = bk[c]; path.append(c)
    return path[::-1]

def add(out, i0, seg):
    if i0 >= len(out): return
    seg = seg[:len(out) - i0]; out[i0:i0 + len(seg)] += seg
def pulse(ph, duty): return np.where((ph % 1.0) < duty, 1.0, -1.0)

def render():
    sl = slots(); ch = harmonise(sl)
    end = PHRASES[-1][1] + TAIL + 0.7
    out = np.zeros(int(SR * end)); acc = np.zeros_like(out)
    # ---- the lead (melody), with a double, a slide into each note and a three-tap echo
    for ps, pe, notes in PHRASES:
        for ns, nl, m in notes:
            i0 = int(ns * SR); n = int(max(nl, 0.12) * SR); t = np.arange(n) / SR
            vib = 1 + (0.007 * np.sin(2 * np.pi * 5.6 * t) * np.clip((t - 0.22) * 4, 0, 1) if nl > 0.4 else 0)
            slide = 2 ** ((-0.7 * np.exp(-t / 0.025)) / 12.0)
            f = midi_hz(m + 12) * vib * slide; ph = np.cumsum(f) / SR
            duty = 0.25 + 0.03 * np.sin(2 * np.pi * 2.2 * t)
            lead = pulse(ph, duty)
            ph2 = np.cumsum(f * 1.004) / SR; dbl = pulse(ph2, 0.125)
            env = np.minimum(t / 0.006, 1.0) * (np.exp(-t * 1.5) * 0.65 + 0.35); env *= np.clip((n / SR - t) / 0.07, 0, 1)
            seg = (lead * 0.26 + dbl * 0.08) * env
            add(out, i0, seg)
            for d, g in ((0.23, 0.22), (0.46, 0.10), (0.69, 0.045)): add(out, i0 + int(d * SR), seg * g)
    # ---- the accompaniment, slot by slot
    for (s0, s1, notes), c in zip(sl, ch):
        r, q = c; third = 3 if q else 4; L = s1 - s0
        a = lambda p, lo: lo + ((p - lo) % 12)              # the pitch p (any octave) moved up into the octave starting at lo
        bass_r = a(r, 43); bass_5 = a(r + 7, 43)
        i0 = int(s0 * SR); n = int(L * SR); t = np.arange(n) / SR
        last = (s1 - s0) > 0 and sl[-1][0] == s0
        g = swell(s0 + t)
        tail_fade = np.clip((L - t) / 2.0, 0, 1) if last else 1.0
        # pad: three held tones, soft 50 % pulses, a slow tremolo, a short crossfade at each end
        padenv = np.minimum(t / 0.12, 1.0) * np.clip((L - t) / (0.20 if not last else 2.0), 0, 1) * (0.8 + 0.2 * np.sin(2 * np.pi * 0.7 * t))
        pad = 0.0
        for p in (a(r, 50), a(r + third, 50), a(r + 7, 50)):
            pad = pad + pulse(np.cumsum(np.full(n, midi_hz(p))) / SR, 0.5) * 0.034
        add(acc, i0, pad * padenv * g * tail_fade)
        # bass: root on the slot start, the fifth halfway on a long slot, a short octave pickup at the end of a very long one
        def tri(p, ln, vel):
            m = int(ln * SR); tt = np.arange(m) / SR; ph = np.cumsum(np.full(m, midi_hz(p))) / SR
            e = np.minimum(tt / 0.012, 1.0) * np.exp(-tt * 0.9) * np.clip((ln - tt) / 0.08, 0, 1)
            return (4 * np.abs((ph % 1.0) - 0.5) - 1) * e * vel
        bl = min(L * (0.55 if L > 2.0 else 0.95), 2.6) if not last else 4.0
        add(acc, i0, tri(bass_r, bl, 0.30) * swell(s0))
        if L > 2.0 and not last: add(acc, i0 + int(L * 0.55 * SR), tri(bass_5, min(L * 0.4, 1.6), 0.26) * swell(s0 + L * 0.55))
        if L > 3.4 and not last: add(acc, i0 + int((L - 0.5) * SR), tri(bass_r + 12, 0.45, 0.18) * swell(s1))
        # arpeggio: a quiet 12.5 % pulse pluck every 0.25 s, up and down the triad, the last chord only twice
        pat = [0, 1, 2, 3, 2, 1]; step = 0.25
        for k in range(int(L / step) if not last else 8):
            tt0 = k * step; p = [a(r, 60), a(r + third, 60), a(r + 7, 60), a(r, 60) + 12][pat[k % 6]]
            m = int(0.28 * SR); tt = np.arange(m) / SR; ph = np.cumsum(np.full(m, midi_hz(p))) / SR
            e = np.minimum(tt / 0.004, 1.0) * np.exp(-tt * 11.0)
            add(acc, i0 + int(tt0 * SR), pulse(ph, 0.125) * e * 0.065 * swell(s0 + tt0) * (1.0 if not last else 0.7))
    out += acc
    out /= max(1e-9, np.abs(out).max()) / 0.9
    return out
def write(path):
    x = render(); w = wave.open(path, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes((x * 32767).astype('<i2').tobytes()); w.close(); return len(x) / SR
if __name__ == '__main__':
    p = sys.argv[1] if len(sys.argv) > 1 else '/tmp/here_today.wav'
    print('wrote', p, round(write(p), 1), 's')
    if '--chords' in sys.argv:
        names = 'C C# D D# E F F# G G# A A# B'.split()
        for (s0, s1, _), (r, q) in zip(slots(), harmonise(slots())): print('%6.2f-%6.2f  %s%s' % (s0, s1, names[r], 'm' if q else ''))
