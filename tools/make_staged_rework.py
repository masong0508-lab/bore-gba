#!/usr/bin/env python3
"""Builds tools/staged.xm : "Staged (The Full Performance)", a DayBar hi-NRG rework of "The Dipper Man - Staged the Full Performance"
(tools/the_dipper_man_staged.xm, a 2-minute, 12-channel PixiTracker song in a triplet shuffle).
usage: python3 tools/make_staged_rework.py        (run from the project root; needs numpy + scipy; re-running gives byte-identical output)

KEPT, read straight out of the original XM. Its samples sound about a semitone under the written notes, so everything is taken down one:
you hear D minor. The original's shuffle (3 rows a beat) is straightened: the 1st, 2nd and 3rd triplet go to the 16ths 0, 1 and 3.
  THE PULSE   D F D F | Bb D C E   one note a beat, pulsed in 16ths (the original repeated it every row)
  THE CHORDS  Dm F Gm Am (two beats each)          THE INTRO BASS  D . C . Bb | Bb . C . D
  MELODY A    D A D A | G A G G F | G A F E D      MELODY B  D E F D E F G A G | G F D C D
  THE CLIMB   G/B C Dm C G/B (the breakdown)       THE END   the low D under a minor-sixth chord
NEW: the style, the form and every sound. Hi-NRG in the manner of Divine's "You Think You're a Man" (130 BPM, four on the floor, an
octave-bouncing 16th bass, claps on 2 and 4, open hats on the offbeats, Simmons toms, Fairlight-style orchestra hits, synth brass,
chimes) with DayBar's things on top: an acid line and a hoover in the dub, a piano / strings / "ahh" breakdown, risers, reverse swells
and impacts. The last two choruses go up a whole tone; the outro comes back down to D minor and fades out slowly.
Rows are 64ths (speed 1, XM BPM 87 = 130.5 BPM, 64 rows a bar), so some parts move in 32nds and 64ths: hat ratchets, snare rolls that
speed up into a section, tom fills, the riff stuttering at the ends of phrases, a sparkle arp in the last chorus, acid flicks.
Form (209 bars, about 6:25): intro 16 | riff 16 | verse 16 | pre 8 | chorus 16 | dub 16 | verse 16 | pre 8 | chorus 16 | breakdown 16 |
chorus (+2) 16 | chorus (+2) 16 | outro (back in D) 32 + a tail.
Channels: 0 kick | 1 clap / snare | 2 hat | 3 open hat / cowbell | 4 octave bass | 5 pulse riff | 6 7 8 brass chords / strings | 9 lead
          10 lead harmony / echo | 11 orchestra hit / piano | 12 choir / hoover | 13 chimes / arp / acid | 14 Simmons toms | 15 fx
"""
import os, sys, struct
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "the_dipper_man_staged.xm")
OUT = os.path.join(HERE, "staged.xm")
HIFI = int(os.environ.get('BORE_HIFI', '1'))   # 1 = the game's sample rates; tools/studio_render.py sets 4 to synthesise every sound at 4x the rate
SR = 16726 * HIFI; SR2 = 8363 * 2 ** (5 / 12) * HIFI; SR0 = 8363 * HIFI
BPM_XM, SPEED, NCH, ROWS = 87, 1, 16, 64
BAR = ROWS * SPEED * 2.5 / BPM_XM; BEAT = BAR / 4
rng = np.random.default_rng(1984)

# ---------------------------------------------------------------- synthesis helpers
def tt(n, sr=SR): return np.arange(n) / sr
def mid2f(m): return 440.0 * 2 ** ((m - 69) / 12)
def lp(x, fc, order=2, sr=SR): b, a = signal.butter(order, min(fc, sr * .45) / (sr / 2), 'low'); return signal.lfilter(b, a, x)
def hp(x, fc, order=2, sr=SR): b, a = signal.butter(order, fc / (sr / 2), 'high'); return signal.lfilter(b, a, x)
def bp(x, f1, f2, order=2, sr=SR): b, a = signal.butter(order, [f1 / (sr / 2), min(f2, sr * .45) / (sr / 2)], 'band'); return signal.lfilter(b, a, x)
def noise(n): return rng.uniform(-1, 1, n)
def saw(f, n, sr=SR, ph=0.0, vib=None):
    t = tt(n, sr); y = np.zeros(n); w = 2 * np.pi * (f * t if vib is None else np.cumsum(vib) / sr)
    for k in range(1, int(min(sr * .45 / max(f, 1), 40)) + 1): y += np.sin(k * w + ph * k) / k
    return y * (2 / np.pi)
def square(f, n, sr=SR, vib=None, duty=.5):
    t = tt(n, sr); w = 2 * np.pi * (f * t if vib is None else np.cumsum(vib) / sr); y = np.zeros(n)
    for k in range(1, int(min(sr * .45 / f, 40)) + 1): y += np.sin(np.pi * k * duty) / k * np.cos(k * w - np.pi * k * duty)
    return y * (4 / np.pi) * .5
def verb(x, sr, t60, mix, pre=0.015, tone=4200):
    n = int(t60 * sr); t = tt(n, sr)
    ir = noise(n) * np.exp(-6.9 * t / t60); ir = lp(ir, tone, 2, sr); ir[:int(pre * sr)] = 0; ir /= np.sqrt((ir ** 2).sum())
    wet = np.concatenate([signal.fftconvolve(x, ir), [0.0]])
    y = np.concatenate([x, np.zeros(n)]); return y + wet * mix * np.abs(x).max() / (np.abs(wet).max() + 1e-9)
def finish(x, peak, tail=0.012, sr=SR):
    x = x / np.abs(x).max() * peak
    k = int(tail * sr); x[-k:] *= np.linspace(1, 0, k) ** 2
    k = len(x)
    while k > 64 and abs(x[k - 1]) < .004: k -= 1
    return x[:k]
def env(n, att, dec, sr=SR, sus=0.0, rel=None):
    t = tt(n, sr); e = np.minimum(1, t / max(att, 1e-4)) * (sus + (1 - sus) * np.exp(-t / dec))
    if rel: e *= np.minimum(1, (t[-1] - t) / rel)
    return e
def acid(f, n, sr, cut0, cut1, res, acc):   # a 303-ish voice: saw through a resonant state-variable low-pass that closes fast
    x = saw(f, n, sr) * .9 + square(f, n, sr) * .15; y = np.zeros(n); lo = bd = 0.0
    for i in range(n):
        fc = cut1 + (cut0 - cut1) * np.exp(-i / sr / acc); F = 2 * np.sin(np.pi * min(fc, sr * .2) / sr)
        hi = x[i] - lo - res * bd; bd += F * hi; lo += F * bd; y[i] = lo
    return np.tanh(1.6 * y)

# ---------------------------------------------------------------- instruments
I = {}; KEYS = []
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel); KEYS.append(k)
# drums: a tight Linn-style kick, a big clap/snare with a gated room, hats, an open hat, a cowbell, Simmons toms
n = int(.3 * SR); t = tt(n)
ph = 2 * np.pi * np.cumsum(52 + 130 * np.exp(-t / .028) + 300 * np.exp(-t / .003)) / SR
add('kick', 'kick', finish(np.tanh(1.8 * np.sin(ph) * np.exp(-t / .14)) + 0 * t, .98), None)
n = int(.42 * SR); t = tt(n); c = np.zeros(n)
for off, dec, a in ((0, .005, .6), (.010, .005, .8), (.021, .006, .85), (.030, .09, 1.0)):
    i = int(off * SR); m = n - i; c[i:] += noise(m) * np.exp(-tt(m) / dec) * a
sn = bp(noise(n), 1500, 7000, 2) * np.exp(-t / .09) + (np.sin(2 * np.pi * 190 * t) + .5 * np.sin(2 * np.pi * 340 * t)) * np.exp(-t / .04) * .6
gate = np.where(t < .2, 1.0, np.maximum(0, 1 - (t - .2) / .05))   # the 80s gated room
add('clap', 'clap snare', finish(verb(bp(c, 900, 3800, 2) + sn * .8, SR, .5, .4)[:n] * gate, .84))
n = int(.13 * SR); t = tt(n)
add('snare', 'roll snare', finish(bp(noise(n), 1600, 7000, 2) * np.exp(-t / .045) + np.sin(2 * np.pi * 210 * t) * np.exp(-t / .025) * .5, .7))
def metal(n, sr=SR):
    t = tt(n, sr); return sum(np.sign(np.sin(2 * np.pi * f * t + rng.uniform(0, 6))) for f in (205, 304, 369, 437, 532, 812)) / 6
n = int(.05 * SR); t = tt(n); add('chat', 'hat', finish(hp(metal(n) * .5 + noise(n), 7200, 4) * np.exp(-t / .010), .45))
n = int(.3 * SR); t = tt(n); add('ohat', 'open hat', finish(hp(metal(n) * .6 + noise(n), 6000, 4) * np.exp(-t / .11), .45))
n = int(.3 * SR); t = tt(n)
cb = np.sign(np.sin(2 * np.pi * 540 * t)) + np.sign(np.sin(2 * np.pi * 800 * t))
add('cowbell', 'cowbell', finish(bp(cb, 450, 3500, 2) * (np.exp(-t / .03) * .7 + np.exp(-t / .16) * .3), .5))
n = int(.5 * SR); t = tt(n)   # Simmons: a sine that drops in pitch fast, with a noise edge (A2)
f = mid2f(45) * (1 + 1.2 * np.exp(-t / .045)); add('tom', 'simmons tom', finish(np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / .2) + bp(noise(n), 1500, 5000) * np.exp(-t / .015) * .4, .8), 45)
# bass: a punchy octave bass (Juno saw + square, short filtered envelope), D2; a long one for held notes
for key, dur, dec in (('obass', .16, .07), ('lbass', .9, .5)):
    n = int(dur * SR); t = tt(n); f = mid2f(38)
    b = saw(f, n) * .7 + square(f, n, duty=.3) * .5
    b = lp(b, 1500) * .5 + lp(b, 420) * .9 + np.sin(2 * np.pi * f * t) * .5
    add(key, 'octave bass' if key == 'obass' else 'long bass', finish(np.tanh(1.4 * b * env(n, .002, dec, sus=.25, rel=.03)), .95), 38)
# the pulse riff: a bright sequencer pluck (C5)
n = int(.2 * SR); t = tt(n); f = mid2f(72)
add('pulse', 'pulse synth', finish(lp(square(f, n, duty=.25) * .6 + saw(f * 1.005, n) * .5, 3600) * env(n, .001, .07, sus=.1), .62), 72)
# synth brass (C4): detuned saws, a filter that opens on the attack; short (stabs) and long (chords) / strings (C4)
def brass(dur, sr=SR2):
    n = int(dur * sr); t = tt(n, sr); f = mid2f(60)
    x = sum(saw(f * 2 ** (c / 1200), n, sr, ph=rng.uniform(0, 6)) for c in (-9, 0, 9)) / 3
    bright = lp(x, 3200, 2, sr); dark = lp(x, 900, 2, sr); w = np.exp(-t / .18)
    return (bright * w + dark * (1 - w)) * np.minimum(1, t / .025) * (.75 + .25 * np.exp(-t / .3))
add('brass', 'synth brass', finish(verb(brass(1.3) * np.minimum(1, (1.3 - tt(int(1.3 * SR2), SR2)) / .25), SR2, .7, .25), .6, .05, SR2), 60, rel=5)
add('brassS', 'brass stab', finish(verb(brass(.22) * np.exp(-tt(int(.22 * SR2), SR2) / .12), SR2, .5, .3), .62, .02, SR2), 60, rel=5)
n = int(2.4 * SR0); t = tt(n, SR0); f = mid2f(60)
st = sum(saw(f * 2 ** (c / 1200), n, SR0, ph=rng.uniform(0, 6)) for c in (-12, -5, 0, 5, 12)) / 5
add('str', 'strings', finish(lp(st, 2600, 2, SR0) * np.minimum(1, t / .35) * np.minimum(1, (t[-1] - t) / .8), .55, .05, SR0), 60, rel=0)
# lead (C5): a synth-brass lead with vibrato that blooms
n = int(1.3 * SR); t = tt(n); f = mid2f(72)
vib = f * (1 + .006 * np.sin(2 * np.pi * 5.6 * t) * np.clip((t - .15) / .2, 0, 1))
ld = saw(f, n, vib=vib) * .6 + saw(f * 1.004, n, vib=vib) * .45 + square(f, n, vib=vib, duty=.4) * .3
ld = lp(ld, 3600) * np.minimum(1, t / .01) * (.8 + .2 * np.exp(-t / .12)) * np.minimum(1, (t[-1] - t) / .3)
add('lead', 'brass lead', finish(verb(ld, SR, .6, .22), .74, .04), 72)
# orchestra hit (C4): stacked octaves and fifths of saws plus a noise burst, gone in half a second
n = int(.6 * SR2); t = tt(n, SR2); f = mid2f(60)
oh = sum(a * saw(f * r, n, SR2, ph=rng.uniform(0, 6)) for r, a in ((0.5, .8), (1, 1), (1.5, .6), (2, .7), (3, .3)))
oh = lp(oh, 4000, 2, SR2) * np.exp(-t / .16) + bp(noise(n), 800, 4000, 2, SR2) * np.exp(-t / .03) * .7
add('orch', 'orchestra hit', finish(verb(oh, SR2, .8, .3), .82, .05, SR2), 60, rel=5)
# piano (C4): a bright FM piano for the breakdown
n = int(1.6 * SR); t = tt(n); f = mid2f(60)
pn = np.sin(2 * np.pi * f * t + 1.6 * np.exp(-t / .25) * np.sin(2 * np.pi * f * t)) + .35 * np.sin(4 * np.pi * f * t) * np.exp(-t / .3)
add('piano', 'piano', finish(verb(pn * np.exp(-t / .7) * np.minimum(1, t / .002), SR, .9, .3), .7, .05), 60)
# choir "ahh" (C4) and the hoover (C3)
n = int(1.8 * SR2); t = tt(n, SR2); f = mid2f(60)
vb = f * (1 + .005 * np.sin(2 * np.pi * 5 * t))
src = sum(saw(f * 2 ** (c / 1200), n, SR2, vib=vb * 2 ** (c / 1200)) for c in (-8, 0, 8)) / 3
ch = bp(src, 650, 950, 2, SR2) + bp(src, 1050, 1350, 2, SR2) * .7 + bp(src, 2400, 2800, 2, SR2) * .2
add('choir', 'choir ahh', finish(verb(ch * np.minimum(1, t / .2) * np.minimum(1, (t[-1] - t) / .4), SR2, 1.0, .3), .6, .05, SR2), 60, rel=5)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(48) * (1 - .25 * np.exp(-t / .07))   # the hoover: detuned saws that dive in, a whoosh of PWM
hv = sum(saw(f[0] * 2 ** (c / 1200), n, SR2, vib=f * 2 ** (c / 1200)) for c in (-25, -12, 0, 12, 25)) / 5
add('hoover', 'hoover', finish(lp(np.tanh(2 * hv), 3000, 2, SR2) * np.minimum(1, t / .02) * np.minimum(1, (t[-1] - t) / .4), .62, .05, SR2), 48, rel=5)
# chimes (C6) and the acid voice (C3)
n = int(1.2 * SR2); t = tt(n, SR2); f = mid2f(84)
cm = np.sin(2 * np.pi * f * t) * np.exp(-t / .5) + .5 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t / .18) + .3 * np.sin(2 * np.pi * f * 5.4 * t) * np.exp(-t / .06)
add('chime', 'chime', finish(cm * np.minimum(1, t / .001), .55, .05, SR2), 84, rel=5)
n = int(.2 * SR); add('acid', 'acid', finish(acid(mid2f(48), n, SR, 3200, 380, 1.15, .06) * env(n, .001, .1, sus=.3, rel=.02), .7), 48)
# fx: riser (2 bars), reverse cymbal (1 bar), crash, impact
n = int(2 * BAR * SR0); t = tt(n, SR0); u = t / t[-1]; nz = noise(n)
add('riser', 'riser', finish((bp(nz, 250, 900, 2, SR0) * (1 - u) + bp(nz, 1200, 3600, 2, SR0) * u) * u ** 2, .5, .01, SR0), None, rel=0)
n = int(BAR * SR2); t = tt(n, SR2)
add('swell', 'reverse cymbal', finish(hp(metal(n, SR2) * .4 + noise(n), 3000, 3, SR2) * np.exp(-(t[-1] - t) / .5), .45, .01, SR2), None, rel=5)
n = int(1.4 * SR2); t = tt(n, SR2)
add('crash', 'crash', finish(hp(metal(n, SR2) * .4 + noise(n), 3200, 3, SR2) * np.exp(-t / .55), .5, .05, SR2), None, rel=5)
n = int(1.3 * SR2); t = tt(n, SR2); ph = 2 * np.pi * np.cumsum(30 + 80 * np.exp(-t / .08)) / SR2
add('impact', 'impact', finish(np.sin(ph) * np.exp(-t / .5) + hp(noise(n), 1500, 2, SR2) * np.exp(-t / .25) * .45, .9, .05, SR2), None, rel=5)
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the original's notes (a semitone down; triplets -> 16ths 0 1 3)
S0 = xm.parse(SRC)
TRP = -1; QQ = {0: 0, 1: 1, 2: 3}
def notes(pat, inst, ch=None, pulse=False):
    out = []
    for r, row in enumerate(S0['pats'][pat]):
        for c, (n, i, v, e, ep) in enumerate(row):
            if 0 < n < 97 and i == inst and (ch is None or c == ch):
                s = (r // 3) * 4 + QQ[r % 3]; m = n + 11 + TRP
                if pulse and out and out[-1][1] == m and out[-1][0] // 4 == s // 4: continue   # (the riff: one note a beat)
                out.append((s, m))
    return out
MEL_A = notes(5, 10, 8) + [(s + 32, m) for s, m in notes(6, 10, 8)]     # 4 bars, in 16ths
MEL_B = notes(7, 10, 8) + [(s + 32, m) for s, m in notes(8, 10, 8)]
RIFF = notes(2, 9, 2, pulse=True)                                         # 8 beats: (16th, note)
ROOTS = [m for s, m in notes(2, 6, 4)][::2]                               # D F G A (a root per half bar)
IBASS = notes(0, 4) + [(s + 32, m) for s, m in notes(1, 4)]               # D . C . Bb | Bb . C . D  (4 bars)
WALK = notes(2, 3, 6)                                                     # D A D A G . A (2 bars)
CLIMB = [m for s, m in notes(9, 6)] + [m for s, m in notes(10, 6)]        # B C | D C B   (the pads of the breakdown)
assert [m % 12 for s, m in RIFF] == [2, 5, 2, 5, 10, 2, 0, 4] and [m % 12 for m in ROOTS] == [2, 5, 7, 9]
assert [m % 12 for s, m in MEL_A][:4] == [2, 9, 2, 9] and [m % 12 for m in CLIMB] == [11, 0, 2, 0, 11]
CHQ = {2: ('m', 'D'), 5: ('M', 'F'), 7: ('m', 'G'), 9: ('m', 'A')}        # i III iv v
def voicing(root, kind):   # three brass voices around C4-A4
    third = 3 if kind == 'm' else 4
    pcs = [(root + d) % 12 for d in (0, third, 7)]
    out = []
    for pc in pcs:
        m = 57 + ((pc - 57) % 12)
        out.append(m)
    return sorted(out)
CLIMBV = {11: ((59, 62, 67), 47), 0: ((60, 64, 67), 48), 2: ((62, 65, 69), 50)}   # G/B, C, Dm: (voices, bass)

# ---------------------------------------------------------------- the bar builder
def R(s16, off=0): return s16 * 4 + off      # 16th -> row (a row is a 64th)
def xmnote(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49
def build(b):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    T = b.get('tr', 0); G = b.get('g', 1.0); h = b.get('half', 0)   # transpose, overall level (the fade), which half of a 2-bar figure
    def put(r, ch, key, midi=None, vol=64):
        v = int(round(vol * G))
        if 0 <= r < ROWS and v > 0:
            P[r][ch] = (xmnote(midi + (T if key not in ('tom',) else 0), key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, v)))
    roots = ROOTS[2 * h: 2 * h + 2]
    d = b.get('drums')
    if d:
        for s in range(0, 16, 4): put(R(s), 0, 'kick', None, 62 if s == 0 else 58)
        if d >= 2: put(R(4), 1, 'clap', None, 54); put(R(12), 1, 'clap', None, 56)
        for s in range(16):
            if b.get('hats', 0) >= 1 and s % 2 == 0 and s % 4 == 2: put(R(s), 3, 'ohat', None, 42)   # open hats on the offbeats (hi-NRG)
            if b.get('hats', 0) >= 2 and s % 4 != 2: put(R(s), 2, 'chat', None, 38 if s % 2 else 28)
        if b.get('cowbell'): [put(R(s), 3, 'cowbell', None, 26) for s in (3, 6, 11, 14)]
        if b.get('ratchet'):   # the last beat's hats in 32nds, then 64ths
            for k, r in enumerate((48, 50, 52, 54, 56, 57, 58, 59, 60, 61, 62, 63)): put(r, 2, 'chat', None, 16 + k * 2)
    for r, v in b.get('roll', ()): put(r, 1, 'snare', None, v)
    for r, m, v in b.get('toms', ()): put(r, 14, 'tom', m, v)
    # bass: octave bounce in 16ths on the chord root (low, high, low, high ...)
    if b.get('bass'):
        for s in range(16):
            rt = roots[s // 8]
            put(R(s), 4, 'obass', rt + (12 if s % 2 else 0), 50 if s % 2 == 0 else 40)
    for s, m, v, k in b.get('lbass', ()): put(R(s), 4, k, m, v)
    # the pulse riff: each beat's note in 16ths, the first accented; 'stutter' cuts the last beat into 32nds
    if b.get('riff'):
        v = b['riff']
        for s16, m in RIFF[4 * h: 4 * h + 4]:
            bs = s16 - 16 * h
            for k in range(4): put(R(bs + k), 5, 'pulse', m + b.get('riffoct', 0), v if k == 0 else v - 12)
        if b.get('stutter'):
            m = RIFF[4 * h + 3][1] + b.get('riffoct', 0)
            for k in range(8): put(R(12) + 2 * k, 5, 'pulse', m, v - 14 + k)
    # brass chords on each root (held) and stabs on the and-of-2 / and-of-4
    if b.get('brass'):
        for half in range(2):
            rt = roots[half]; vs = voicing(rt % 12, CHQ[rt % 12][0])
            for j, m in enumerate(vs): put(R(8 * half), 6 + j, 'brass', m, b['brass'] - 3 * j)
            if b.get('stabs'):
                for j, m in enumerate(vs): put(R(8 * half + 6), 6 + j, 'brassS', m, b['brass'] - 6 - 3 * j)
    if b.get('strings'):
        for half in range(2):
            rt = roots[half]; vs = voicing(rt % 12, CHQ[rt % 12][0])
            for j, m in enumerate(vs): put(R(8 * half), 6 + j, 'str', m, b['strings'] - 3 * j)
    if b.get('climb'):   # the breakdown chords: strings + piano + choir, the bass under them
        for half, (pc, vol) in enumerate(b['climb']):
            vs, bm = CLIMBV[pc]
            for j, m in enumerate(vs): put(R(8 * half), 6 + j, 'str', m, vol - 3 * j)
            put(R(8 * half), 11, 'piano', vs[-1] + 12, vol - 4); put(R(8 * half + 3), 11, 'piano', vs[1] + 12, vol - 14); put(R(8 * half + 6), 11, 'piano', vs[0] + 12, vol - 16)
            put(R(8 * half), 12, 'choir', vs[-1], vol - 6)
            if b.get('climbbass'): put(R(8 * half), 4, 'lbass', bm, vol + 4)
    # melodies (a 4-bar phrase, this is bar b['mbar'] of it), on the lead with an echo or a harmony, or on the choir / chimes
    mel = b.get('mel')
    if mel:
        line, key, v = mel; mb = b.get('mbar', 0)
        for s, m in line:
            if mb * 16 <= s < mb * 16 + 16:
                ss = s - mb * 16; put(R(ss), 9 if key == 'lead' else 12 if key == 'choir' else 13, key, m, v)
                if b.get('echo') and key == 'lead' and ss + 3 < 16: put(R(ss + 3), 10, 'lead', m, v * .4)
                if b.get('harm') and key == 'lead': put(R(ss), 10, 'lead', m - (4 if m % 12 in (2, 7, 9) else 3), v * .6)
                if b.get('choirdbl') and key == 'lead': put(R(ss), 12, 'choir', m - 12, v * .55)
    for r, m, v in b.get('orch', ()): put(r, 11, 'orch', m, v)
    for r, m, v in b.get('chimes', ()): put(r, 13, 'chime', m, v)
    if b.get('arp'):   # the sparkle: the chord in 32nds, very soft
        for k in range(32):
            rt = roots[k // 16]; vs = voicing(rt % 12, CHQ[rt % 12][0]); put(2 * k, 13, 'chime', vs[k % 3] + 12, b['arp'] - (0 if k % 4 == 0 else 6))
    if b.get('acid'):   # the walking bass on the acid voice, 16ths, with 32nd flicks at the end of the bar
        for s, m in WALK:
            if 16 * h <= s < 16 * h + 16: put(R(s - 16 * h), 13, 'acid', m + 12, b['acid'])
        for s in (2, 6, 10, 14): put(R(s), 13, 'acid', roots[s // 8] + (0 if s % 4 == 2 else 12), b['acid'] - 10)
        if b.get('flick'): [put(R(14) + 2 * k, 13, 'acid', roots[1] + 12 + (3 if k % 2 else 0), b['acid'] - 6) for k in range(4)]
    if b.get('hoover'): put(0, 12, 'hoover', roots[0] - 12 + 12, b['hoover'])
    for r, key, v in b.get('fx', ()): put(r, 15, key, None, v)
    for r, ch, key, m, v in b.get('raw', ()): put(r, ch, key, m, v)
    return P

# ---------------------------------------------------------------- the form
BARS = []
def bar(**kw): BARS.append(kw); return kw
def tomfill(beat0=12, start=50):   # Simmons 32nds from high to low over the last beat(s)
    out = []; k = 0
    for r in range(R(beat0), ROWS, 2):
        out.append((r, 57 - (k // 2) * 3, start - k)); k += 1
    return tuple(out)
def roll(r0=0, r1=ROWS, v0=14, v1=46):   # snare roll: 16ths, then 32nds, then 64ths in the last beat
    out = []; r = r0
    while r < r1:
        q = (r - r0) / max(1, r1 - r0); out.append((r, int(v0 + (v1 - v0) * q)))
        r += 4 if q < .5 else 2 if q < .75 else 1
    return tuple(out)
def section(n, **kw):
    for i in range(n):
        b = bar(half=i % 2, mbar=i % 4, **kw); yield i, b
# intro (16): kick + the intro bass, hats, claps, then the riff in the distance; tom fill
for i, b in section(16, drums=1):
    b['drums'] = 1 if i < 8 else 2; b['hats'] = 0 if i < 4 else 1 if i < 8 else 2
    s4 = i % 4
    b['lbass'] = tuple((s - s4 * 16, m, 42, 'lbass') for s, m in IBASS if s4 * 16 <= s < s4 * 16 + 16)
    if i >= 8: b.update(riff=34)
    if i == 0: b['orch'] = ((0, 62, 56),); b['fx'] = ((0, 'impact', 50),)
    if i == 14: b['fx'] = ((0, 'riser', 44),)
    if i == 15: b['toms'] = tomfill(8); b['fx'] = ((0, 'swell', 46),)
    if i in (7, 11): b['ratchet'] = 1
# riff (16): octave bass, brass, the riff in full
for i, b in section(16, drums=2, hats=2, bass=1, riff=48, brass=34, stabs=1):
    if i == 0: b['fx'] = ((0, 'crash', 50),); b['orch'] = ((0, 62, 52),)
    if i % 4 == 3: b['ratchet'] = 1
    if i == 15: b['toms'] = tomfill(12)
def verse(n, line, **kw):
    for i, b in section(n, drums=2, hats=2, bass=1, riff=30, brass=28, mel=(line, 'lead', 54), echo=1, **kw):
        if i % 4 == 3: b['ratchet'] = 1
        if i == 0: b['fx'] = ((0, 'crash', 44),)
def pre():
    for i, b in section(8, drums=2, hats=2, bass=1, brass=36, stabs=1):
        b['riff'] = 34 + 2 * i   # the riff comes up through the build
        s4 = i % 4   # the intro bass as orchestra hits, climbing
        b['orch'] = tuple((R(s - s4 * 16), m + 12, 50) for s, m in IBASS if s4 * 16 <= s < s4 * 16 + 16)
        if i >= 4: b['roll'] = roll(0 if i == 7 else 32, ROWS, 12 + (i - 4) * 6, 30 + (i - 4) * 6)
        if i == 6: b['fx'] = ((0, 'riser', 50),)
        if i == 7: b['fx'] = ((0, 'swell', 48),); b['toms'] = tomfill(12, 46)
def chorus(n, tr=0, sparkle=0, harm=0):
    for i, b in section(n, drums=2, hats=2, bass=1, riff=56, brass=38, stabs=1, mel=(MEL_A, 'lead', 60), choirdbl=1, tr=tr, harm=harm):
        b['orch'] = ((0, ROOTS[2 * (i % 2)] + 12, 46), (32, ROOTS[2 * (i % 2) + 1] + 12, 40))
        b['chimes'] = tuple((R(s - 16 * (i % 2)), m + 12, 30) for s, m in RIFF if 16 * (i % 2) <= s < 16 * (i % 2) + 16 and (s // 4) % 2 == 0)
        if i % 2 == 1 and i % 4 == 3: b['stutter'] = 1
        if i % 4 == 3: b['ratchet'] = 1
        if sparkle: b['arp'] = sparkle
        if i == 0: b['fx'] = ((0, 'crash', 54),)
        if i == n - 1: b['toms'] = tomfill(12)
verse(16, MEL_A); BARS[-8:] = []; verse(8, MEL_B)      # verse 1: melody A (8 bars), melody B (8 bars)
pre()
chorus(16)
# dub (16): DayBar's acid line and hoover, cowbell; the riff comes back in stutters
for i, b in section(16, drums=2, hats=2, bass=1, cowbell=1, acid=50):
    b['flick'] = i % 2 == 1
    if i % 4 == 0: b['hoover'] = 40
    if i >= 8: b.update(riff=36, stutter=(i % 2 == 1))
    if i == 0: b['fx'] = ((0, 'impact', 48),)
    if i % 4 == 3: b['ratchet'] = 1
    if i == 15: b['toms'] = tomfill(8); b['fx'] = ((0, 'swell', 44),)
verse(8, MEL_B, harm=1); verse(8, MEL_A, harm=1)      # verse 2, with the harmony a third below
pre()
chorus(16)
# breakdown (16): no drums at first; strings, piano and choir climb G/B C Dm C G/B, the melody on the chimes; then the build
CL = [(11, 11), (0, 0), (2, 0), (11, 11)]   # per bar: the two half-bar chords, as the original's pads go: G/B | C | Dm C | G/B
for i in range(16):
    c = CL[i % 4]; b = bar(half=i % 2, mbar=i % 4, climb=((c[0], 44), (c[1], 42)), climbbass=1, mel=(MEL_A, 'chime', 40))
    if i == 0: b['fx'] = ((0, 'impact', 54),)
    if i >= 8: b.update(drums=1 if i >= 12 else 0, hats=1 if i >= 12 else 0)
    if i >= 12: b['roll'] = roll(0, ROWS, 10 + (i - 12) * 8, 26 + (i - 12) * 8)
    if i == 14: b['fx'] = ((0, 'riser', 54),)
    if i == 15: b['fx'] = ((0, 'swell', 50),); b['toms'] = tomfill(8, 52)
chorus(16, tr=2, harm=1)                 # up a whole tone
chorus(16, tr=2, sparkle=26, harm=1)     # and once more, with the sparkle arp
# outro (32): back down to D minor, the riff and the drums; everything thins out and fades slowly
for i, b in section(32, riff=48, stabs=1):
    b.update(drums=2 if i < 24 else 1, hats=2 if i < 16 else 1, bass=1 if i < 28 else 0, brass=30 if i < 16 else 0)
    b['g'] = 1.0 if i < 8 else max(0.08, 1 - (i - 8) / 26)
    if i == 0: b['fx'] = ((0, 'impact', 52),); b['orch'] = ((0, 62, 54),)
    if i < 16 and i % 4 == 3: b['stutter'] = 1
    if i % 8 == 7 and i < 24: b['ratchet'] = 1
# the end: the low D under the minor-sixth chord (D F A B, from the original's last pattern), an orchestra hit and a crash
bar(raw=[(0, 4, 'lbass', 38, 40), (0, 6, 'str', 57, 34), (0, 7, 'str', 59, 32), (0, 8, 'str', 65, 30), (0, 11, 'orch', 62, 40), (0, 12, 'choir', 62, 30)],
    fx=((0, 'crash', 34),), g=0.7)
bar(); bar()

# ---------------------------------------------------------------- XM writer
def pat_bytes(P):
    b = bytearray()
    for row in P:
        for (n, i, v) in row: b += bytes([n, i, v, 0, 0])
    return struct.pack('<IBHH', 9, 0, len(P), len(b)) + bytes(b)
def inst_bytes(k):
    d = I[k]; q = np.clip(np.round(d['x'] * 127), -127, 127).astype(int)
    delta = np.diff(np.concatenate([[0], q])); dl = bytes((int(v) & 255) for v in delta)
    h = struct.pack('<I', 263) + d['name'].encode('latin1')[:22].ljust(22, b'\0') + b'\0' + struct.pack('<HI', 1, 40) + bytes(96) + bytes(48) + bytes(48)
    h += bytes(14) + struct.pack('<H', 0) + bytes(263 - len(h) - 14 - 2)
    sh = struct.pack('<IIIBbBBbB', len(q), 0, 0, 64, 0, 0, 128, d['rel'], 0) + b'\0' * 22
    assert len(h) == 263 and len(sh) == 40
    return h + sh + dl
def main():
    pats, order, seen = [], [], {}
    for sp in BARS:
        P = build(sp); key = pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    assert len(order) <= 256 and len(KEYS) <= 32 and len(pats) <= 256, (len(order), len(KEYS), len(pats))
    hdr = b'Extended Module: ' + b'Staged'[:20].ljust(20) + b'\x1a' + b'make_staged_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM_XM) + bytes(order).ljust(256, b'\0')
    body = b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    s = len(order) * BAR
    print("wrote %s: %d bars, %d patterns, %d instruments, %.1f s (%d:%02d), samples %d KB" % (
        OUT, len(order), len(pats), len(KEYS), s, s // 60, s % 60, sum(len(I[k]['x']) for k in KEYS) // 1024))
if __name__ == '__main__': main()
