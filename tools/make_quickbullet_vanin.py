#!/usr/bin/env python3
"""Builds tools/quickbullet.xm : "Mister Quickbullet", a VaninBlack rework of the Caustic project "Kw5 O" (tools/mister_quickbullet.mid,
its MIDI export at 121 BPM, and tools/kw5_o.caustic, the project itself). The first take, a Sk9m drum & bass version, is
tools/make_quickbullet_rework.py: this one keeps its notes, its form and most of its sounds and turns it into VaninBlack's dark,
heavy downgroove.   usage: python3 tools/make_quickbullet_vanin.py        (byte-identical on every run)

KEPT from the original (as in the first take): the BassLine (the F# pulses, the turnaround, the climbs and walks), the Modular (its
16th arpeggios and two-note chords, the G# pulse break, the falling runs) and both BeatBox parts; from the Caustic file the acid
BassLine and the growl one-shots cut from the unused third BeatBox's recordings.
VANINBLACK: 129 BPM (a touch quicker than the original) under half-time boom-bap drums (a dusty kick, a crushed snare on 3, rim
clicks, swung hats: every second 16th a 64th late), the second kit's 32nd-note rolls as trap hat and kick rolls, a long 808 that
holds the root under the acid, muffled Rhodes and dark strings, the Modular on a dark muted pluck, the melody on a low whine, vinyl
dust under everything, and the growls up front.
64th-note rows (speed 1, XM BPM 86 = 16 rows a beat = 129 BPM), 64 rows a bar, 128-row patterns = 2 bars.
Channels: 0 kick | 1 snare | 2 rim / perc | 3 hat | 4 shaker / open hat / vinyl | 5 808 | 6 acid / reese | 7 8 9 Rhodes / strings
          10 arp | 11 lead (whine) | 12 lead echo / harmony | 13 bell / acid answer | 14 growl / riser | 15 crash / impact.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import midi_read
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
MID = os.path.join(HERE, "mister_quickbullet.mid")
CAUSTIC = os.path.join(HERE, "kw5_o.caustic")
OUT = os.path.join(HERE, "quickbullet.xm")
SPEED, BPM, ROWS, NCH, BR, Q = 1, 86, 128, 16, 64, 4       # 16 rows a beat at 129 BPM; BR rows a bar; Q rows a 16th
SW = 1                                                       # the swing: every second 16th this many rows late
rng = np.random.default_rng(1291)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)

# ---------------------------------------------------------------- the original
div, TR = midi_read.parse(MID)
TB = div * 4; T16 = div // 4
def bars(track):
    out = {}
    for t0, ln, m, v, c in midi_read.notes(track): out.setdefault(t0 // TB, []).append(((t0 % TB) / T16, max(1, ln // T16), m, v))   # (16ths; the 32nd rolls land on half steps)
    return out
BASS, LEAD, KIT1, KIT2 = bars(TR[1]), bars(TR[2]), bars(TR[3]), bars(TR[4])
SCALE = {11, 1, 3, 4, 6, 8, 10}                                   # B major / G# minor, the key of the Modular

def voicing(root):
    """A rootless Rhodes voicing on the bass's root: its third, seventh and ninth (from the key; a root outside it, the A, gets a major 7th)."""
    s = SCALE | {root % 12}
    def pick(base, opts):
        for d in opts:
            if (root + d) % 12 in s: return base + d
        return base + opts[0]
    r = 48 + (root % 12)
    out = [pick(r, (4, 3)), pick(r, (11, 10)), pick(r, (14, 13))]
    return [m + 12 if m < 57 else m - 12 if m > 74 else m for m in out]
def roots(bar):
    """The bass's root for each half of an original bar (the first note of the half, or the last one heard)."""
    ns = BASS.get(bar, []); out = []; last = 30
    for h in (0, 8):
        hs = [m for s, l, m, v in ns if h <= s < h + 8]
        if hs: last = hs[0]
        out.append(last)
    return out

# ---------------------------------------------------------------- the growl: one-shots from the unused BeatBox's recordings
def caustic_growls():
    d = open(CAUSTIC, 'rb').read()
    a = d.find(b'SPAT', d.find(b'SPAT', d.find(b'SPAT', d.find(b'SPAT', d.find(b'SPAT') + 4) + 4) + 4) + 4)   # the fifth pattern block: BBOX3's
    a = (a + 520 + 57344) & ~1; b = d.find(b'EFFX', a) & ~1
    x = np.frombuffer(d[a:b], dtype='<i2').astype(float) / 32768
    nz = np.flatnonzero(np.abs(x) > 1e-4); gaps = np.flatnonzero(np.diff(nz) > 2000)
    segs = [(s, e) for s, e in zip(np.r_[nz[0], nz[gaps + 1]], np.r_[nz[gaps], nz[-1]]) if e - s > 44100]
    out = []
    for s, e in segs[:2]:
        y = x[s:e]; env = np.convolve(np.abs(y), np.ones(441) / 441, 'same')
        on = [i for i in range(441, len(y) - 30000, 441) if env[i] > 2.2 * env[i - 441] and env[i] > .05]
        i = on[len(on) // 3] if on else len(y) // 3
        out.append(y[i:i + int(.42 * 44100)])
    return out
from scipy import signal as SG
for k, g in enumerate(caustic_growls()):
    g = SG.resample_poly(g, 16726, 44100)                                                        # to the sample rate of a "+12" sample
    t = tt(len(g)); g = np.tanh(2.2 * lp(g, 2600)) * np.minimum(1, t / .003) * np.minimum(1, (t[-1] - t) / .06)
    add('growl%d' % k, 'kw5 growl %d' % (k + 1), finish(g, .9), 45)                              # pitched as A2, where it sits

# ---------------------------------------------------------------- sounds
n = int(.3 * SR); t = tt(n)
add('kick', 'dnb kick', finish(np.tanh(1.7 * np.sin(2 * np.pi * np.cumsum(46 + 150 * np.exp(-t / .016)) / SR)) * np.exp(-t / .11)
                           + hp(noise(n), 3000) * np.exp(-t / .003) * .55, .97))
n = int(.26 * SR); t = tt(n)
x = bp(noise(n), 1300, 7800) * np.exp(-t / .085) + (np.sin(2 * np.pi * 205 * t) + .6 * np.sin(2 * np.pi * 345 * t)) * np.exp(-t / .038)
add('snare', 'crack snare', finish(np.tanh(2.2 * x), .93))
n = int(.08 * SR); t = tt(n)
add('ghost', 'ghost snare', finish(bp(noise(n), 1800, 7000) * np.exp(-t / .02) + np.sin(2 * np.pi * 215 * t) * np.exp(-t / .015) * .35, .5))
n = int(.045 * SR); add('hat', 'tight hat', finish(hp(noise(n) * np.exp(-tt(n) / .01), 8000, 4), .34))
n = int(.3 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .09), 7000, 4), .36))
n = int(.07 * SR); t = tt(n); add('shaker', 'shaker', finish(bp(noise(n), 5000, 9000) * np.sin(np.pi * t / t[-1]) ** 2, .28))
n = int(.16 * SR); t = tt(n)                                                       # the beatbox's toms / percs, one pitched hit
add('perc', 'perc', finish(np.sin(2 * np.pi * np.cumsum(170 + 120 * np.exp(-t / .02)) / SR) * np.exp(-t / .06) + bp(noise(n), 900, 3000) * np.exp(-t / .01) * .3, .6), 60)
n = int(1.0 * SR); add('crash', 'crash', finish(hp(noise(n), 4000, 3) * np.exp(-tt(n) / .4), .5))
n = int(.9 * SR2); t = tt(n, SR2)
add('impact', 'impact', finish(np.tanh(2 * np.sin(2 * np.pi * np.cumsum(34 + 62 * np.exp(-t / .08)) / SR2)) * np.exp(-t / .35)
                               + lp(noise(n), 900, 2, SR2) * np.exp(-t / .25) * .5, .9), None, 5)
n = int(.7 * SR2); t = tt(n, SR2); f = mid2f(30)                                   # sub (F#1): a pure sine
add('sub', 'sub', finish(np.sin(2 * np.pi * f * t) * np.minimum(1, t / .004) * np.minimum(1, (t[-1] - t) / .05), .95), 30, 5)
n = int(.7 * SR2); t = tt(n, SR2); f = mid2f(42)                                   # reese (F#2): detuned saws, a filter wobble
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-15, 0, 14)) / 3
seg = [lp(x[i:i + 128], 420 + 760 * (.5 + .5 * np.sin(2 * np.pi * 3.4 * i / SR2)), 2, SR2) for i in range(0, n, 128)]
add('reese', 'reese', finish(np.tanh(1.9 * np.concatenate(seg)[:n]) * np.minimum(1, t / .005) * np.minimum(1, (t[-1] - t) / .05), .8), 42, 5)
def svf(x, fc, q, sr):
    y = np.zeros_like(x); low = band = 0.0
    for i in range(len(x)):
        f = 2 * np.sin(np.pi * min(fc[i], sr * .15) / sr); low += f * band; high = x[i] - low - q * band; band += f * high; y[i] = low
    return y
for key, dec, acc in (('acid', .07, 0), ('acidA', .12, 1)):                        # the BassLine machine: a 303, closed and accented (F#2)
    n = int(.24 * SR); t = tt(n); f = mid2f(42)
    fc = 280 + (2600 if acc else 1500) * np.exp(-t / dec)
    add(key, '303 ' + ('accent' if acc else 'closed'), finish(np.tanh((2.6 if acc else 1.9) * svf(saw(f, n), fc, .16 if acc else .28, SR)) * np.minimum(1, (t[-1] - t) / .02), .8), 42)
def rhodes(dur, dec, sr=SR):
    n = int(dur * sr); t = tt(n, sr); f = mid2f(60)
    x = np.sin(2 * np.pi * f * t + 1.1 * np.exp(-t / .3) * np.sin(2 * np.pi * f * t)) + .2 * np.sin(2 * np.pi * f * 14 * t) * np.exp(-t / .012)
    return hp(x * np.minimum(1, t / .002) * np.exp(-t / dec) * (1 + .06 * np.sin(2 * np.pi * 4.6 * t)), 170, 2, sr)
add('keys', 'rhodes', finish(rhodes(1.0, .7), .72, .05), 60)
add('keysS', 'rhodes stab', finish(rhodes(.25, .1), .72), 60)
n = int(2.0 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # strings (C4)
st = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-12, -5, 0, 5, 12)) / 5
add('str', 'strings', finish(lp(st, 2200, 2, SR2) * np.minimum(1, t / .35) ** 1.5 * np.minimum(1, (t[-1] - t) / .6), .55, .05, SR2), 60, 5)
n = int(1.0 * SR); t = tt(n); f = mid2f(72)                                        # vowel lead (C5)
vib = f * (1 + .0055 * np.sin(2 * np.pi * 5.5 * t) * np.clip((t - .14) / .2, 0, 1))
src = saw(f, n, vib=vib) * .7 + saw(f * 1.003, n, vib=vib) * .45 + square(f, n, vib=vib) * .2
vox = bp(src, 650, 950, 2) * .85 + bp(src, 1050, 1350, 2) * .8 + bp(src, 2450, 2800, 2) * .45 + lp(src, 1800) * .35
add('vox', 'vowel lead', finish(vox * np.minimum(1, t / .01) * np.exp(-t / 1.6) * np.minimum(1, (t[-1] - t) / .25), .78, .05), 72)
n = int(.14 * SR); t = tt(n); f = mid2f(72)                                        # arp pluck (C5): FM blip
add('pluck', 'fm pluck', finish(np.sin(2 * np.pi * f * t + 2.2 * np.exp(-t / .03) * np.sin(2 * np.pi * 2 * f * t)) * np.exp(-t / .05), .6), 72)
n = int(.4 * SR); t = tt(n); f = mid2f(72)                                         # chip lead (C5): 25% pulse
x = saw(f, n, SR) - saw(f, n, SR, ph=np.pi / 2)
add('chip', 'chip lead', finish(lp(x, 6000) * np.minimum(1, t / .003) * np.exp(-t / .35), .6), 72)
n = int(.8 * SR); t = tt(n); f = mid2f(72)                                         # glass bell (C5)
add('bell', 'glass bell', finish(np.sin(2 * np.pi * f * t + 1.4 * np.exp(-t / .3) * np.sin(2 * np.pi * 3.5 * f * t)) * np.exp(-t / .35), .6), 72)
n = int(1.4 * SR2); t = tt(n, SR2)                                                 # riser
seg = [bp(noise(128), 300 + 4200 * (i / n) ** 2, 600 + 6000 * (i / n) ** 2, 2, SR2) for i in range(0, n, 128)]
add('riser', 'riser', finish(np.concatenate(seg)[:n] * (t / t[-1]) ** 1.5, .5), None, 5)
# ---- VaninBlack: a dusty boom-bap kit, an 808, vinyl, darker keys and a low whine
n = int(.42 * SR); t = tt(n)
k = np.sin(2 * np.pi * np.cumsum(44 + 90 * np.exp(-t / .03)) / SR) * np.exp(-t / .2) + lp(noise(n), 1500) * np.exp(-t / .006) * .4
add('kick', 'dusty kick', finish(lp(np.tanh(2.4 * k), 2200), .97))
n = int(.3 * SR); t = tt(n)
x = .6 * np.sin(2 * np.pi * 185 * t) * np.exp(-t / .05) + lp(bp(noise(n), 800, 6500), 4800) * np.exp(-t / .1)
add('snare', 'crushed snare', finish(np.round(np.tanh(2.6 * x) * 20) / 20, .9))                # 5-bit, off an old sampler
n = int(.07 * SR); t = tt(n)
add('rim', 'rim click', finish(np.sin(2 * np.pi * 1650 * t) * np.exp(-t / .009) + .5 * np.sin(2 * np.pi * 760 * t) * np.exp(-t / .015), .5))
n = int(.05 * SR); add('hat', 'dusty hat', finish(lp(hp(noise(n) * np.exp(-tt(n) / .012), 6500, 4), 9000), .3))
n = int(2.2 * SR2); t = tt(n, SR2); f = mid2f(30)                                  # 808 (F#1): a long sine with a little drive and a pitch dip in
ph = 2 * np.pi * np.cumsum(f * (1 + .35 * np.exp(-t / .03))) / SR2
add('808', '808', finish(np.tanh(1.6 * np.sin(ph)) * np.minimum(1, t / .003) * np.exp(-t / 1.4) * np.minimum(1, (t[-1] - t) / .1), .95, .05, SR2), 30, 5)
n = int(1.6 * SR2); t = tt(n, SR2); x = np.zeros(n)                                 # vinyl: dust, crackle, a little hiss
for i in rng.integers(0, n - 40, 120): x[i:i + 3] += rng.uniform(-1, 1, 3)
add('vinyl', 'vinyl', finish(lp(x, 4000, 2, SR2) + .05 * lp(noise(n), 3000, 2, SR2), .4, .05, SR2), None, 5)
add('keysD', 'rhodes muffled', finish(lp(rhodes(1.0, .8), 1500), .72, .05), 60)
n = int(.2 * SR); t = tt(n); f = mid2f(72)
add('pluckD', 'muted pluck', finish(lp(np.sin(2 * np.pi * f * t + 1.6 * np.exp(-t / .03) * np.sin(2 * np.pi * 2 * f * t)), 2400) * np.exp(-t / .07), .62), 72)
n = int(1.2 * SR); t = tt(n); f = mid2f(72)                                         # the low whine (C5): a thin saw through a narrow band, slow vibrato
x = bp(saw(f, n, vib=f * (1 + .009 * np.sin(2 * np.pi * 4.2 * t))), 600, 1400, 2)
add('whine', 'low whine', finish(x * np.minimum(1, t / .06) * np.exp(-t / 1.2) * np.minimum(1, (t[-1] - t) / .3), .7, .05), 72)
KEYS = ['kick', 'snare', 'ghost', 'hat', 'ohat', 'shaker', 'perc', 'crash', 'impact', 'sub', 'reese', 'acid', 'acidA', 'keys', 'keysS', 'str',
        'vox', 'pluck', 'chip', 'bell', 'riser', 'growl0', 'growl1', 'rim', '808', 'vinyl', 'keysD', 'pluckD', 'whine']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

# ---------------------------------------------------------------- one bar (64 rows) from one bar of the original and a few switches
def build_bar(ob, p, P, base, bi):
    def put(r, ch, key, midi=None, vol=64):
        r = int(round(r)); r += SW if r % (2 * Q) == Q else 0; r += base   # (swing)
        if base <= r < base + BR and vol > 0: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, int(vol))))
    d = p.get('drums', 0); last = p.get('last', False)
    # ---- VaninBlack's half-time boom-bap: a kick on 1, a push on the "a" of 2, the snare on 3; swung hats; rims; the original kit's hats on top
    if d:
        put(0, 4, 'vinyl', None, 26)
    if d == 1:
        put(0, 0, 'kick', None, 52)
        for s in range(0, 16, 2): put(s * Q, 3, 'hat', None, 18 if s % 4 else 22)
    if d >= 2:
        put(0, 0, 'kick', None, 62); put(28, 0, 'kick', None, 50); put(32, 1, 'snare', None, 62)
        if bi % 2: put(44, 0, 'kick', None, 46)
        put(56, 2, 'rim', None, 24); put(60, 2, 'ghost', None, 14)
        for s in range(0, 16, 2): put(s * Q, 3, 'hat', None, 24 if s % 4 == 0 else 16)
        for s, l, m, v in KIT1.get(ob, []):
            if m == 49 and int(s) % 2: put(s * Q, 3, 'hat', None, 12)
            elif m in (51, 53, 54, 55) and int(s) % 4 == 3: put(s * Q, 2, 'rim', None, 18)
        if d >= 3:
            put(40, 0, 'kick', None, 44); put(52, 4, 'ohat', None, 20)
            if bi % 4 == 3:
                for k, r in enumerate(range(48, 64, 2)): put(r, 3, 'hat', None, 12 + k * 2)     # a trap hat roll into the next bar
    if d == 9:                                                                                    # the breakdown: just the kick, the snare and dust
        put(0, 0, 'kick', None, 54); put(32, 1, 'snare', None, 48)
    if p.get('drill'):                                                                            # the second kit's 32nd rolls as trap hats; its accents kick
        for s, l, m, v in KIT2.get(ob, []):
            if m == 48:
                put(s * Q, 3, 'hat', None, 26 if v > 100 else 14)
                if v > 100 and int(s) % 4 == 0: put(s * Q, 0, 'kick', None, 54)
            elif m == 49: put(s * Q, 2, 'rim', None, 30)
            elif m == 53: put(s * Q, 2, 'perc', 55, 26)
        put(32, 1, 'snare', None, 60); put(0, 4, 'vinyl', None, 22)
    if p.get('roll'):                                                                             # a snare roll speeding up to 64ths
        for r in range(BR):
            step = 8 if r < 16 else 4 if r < 32 else 2 if r < 48 else 1
            if r % step == 0: put(r, 1, 'snare', None, p['roll'] * (.35 + .65 * r / BR))
    if p.get('riser'): put(0, 14, 'riser', None, p['riser'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    # ---- bass
    bn = BASS.get(ob, []); rt = roots(ob)
    if p.get('sub'):
        prev = None
        for s, l, m, v in bn:
            if m != prev or s % 8 == 0: put(s * Q, 5, '808', m, p['sub']); prev = m
    if p.get('acid'):
        for s, l, m, v in bn: put(s * Q, 6, 'acidA' if (s % 4 == 0 or m != 30) else 'acid', m + 12, p['acid'] - (0 if s % 4 == 0 else 8))
    elif p.get('reese'):
        prev = None
        for s, l, m, v in bn:
            if m != prev or s % 4 == 0: put(s * Q, 6, 'reese', m + 12, p['reese'] - (0 if s % 8 == 0 else 6)); prev = m
    if p.get('answer'):                                                                           # the acid answering an octave up, a 16th late, on the off beats
        for s, l, m, v in bn:
            if s % 4 == 2: put(s * Q + Q, 13, 'acid', m + 24, p['answer'])
    if p.get('growl'):                                                                            # the Kw5 growls on the bass's root
        for k, r in enumerate((0, 24) if p.get('growl2') else (0,)):
            put(r, 14, 'growl%d' % ((bi + k) % 2), rt[0 if r < 32 else 1] + 12, p['growl'])
    # ---- harmony: rootless Rhodes voicings on the bass's root, held, or stabbed on the two-step
    if p.get('keys') or p.get('stabs') or p.get('str'):
        for h in (0, 1):
            for j, m in enumerate(voicing(rt[h])):
                if p.get('str'): put(h * 32, 7 + j, 'str', m, p['str'] - j * 2)
                elif p.get('keys'): put(h * 32 + (j if p.get('roll_keys') else 0), 7 + j, 'keysD', m, p['keys'] - j * 2)
                if p.get('stabs'):
                    for off in (0, 22) if h == 0 else (8, 26):
                        if not p.get('keys') and not p.get('str') or off: put(h * 32 + off, 7 + j, 'keysS', m, p['stabs'] - j * 3 - (6 if off else 0))
    # ---- the Modular
    ln = LEAD.get(ob, [])
    if p.get('arp'):                                                                              # its 16ths as an FM pluck (the upper note of a pair on 12)
        seen = {}
        for s, l, m, v in ln: seen.setdefault(s, []).append(m)
        for s, ms in seen.items():
            ms = sorted(ms); put(s * Q, 10, p.get('arpkey', 'pluckD'), ms[0] + p.get('arpo', 0), p['arp'] - (0 if s % 4 == 0 else 8))
            if len(ms) > 1 and not p.get('lead'): put(s * Q, 12, p.get('arpkey', 'pluckD'), ms[-1] + p.get('arpo', 0), p['arp'] - 12)
            if p.get('flurry') and s % 8 == 7: put(s * Q + 2, 10, p.get('arpkey', 'pluckD'), ms[0] + 12 + p.get('arpo', 0), p['arp'] - 16)   # 32nd flicks
    if p.get('lead'):                                                                             # the vowel lead: the top notes, held, with an echo or a third below
        seen = {}
        for s, l, m, v in ln: seen[s] = max(m, seen.get(s, 0))
        steps = sorted(seen.items())
        for k, (s, m) in enumerate(steps):
            if p.get('sparse') and k % 2: continue
            o = p.get('oct', 0)
            if p.get('glitch') and k % 6 == 5:
                for g in range(4): put(s * Q + g, 11, 'whine', m + o, p['lead'] - g * 9)          # a 64th stutter
            else: put(s * Q, 11, 'whine', m + o, p['lead'])
            if p.get('harm'):
                h = m + o - 3
                while (h % 12) not in SCALE: h -= 1
                put(s * Q, 12, 'whine', h, p['lead'] - 12)
            else: put(s * Q + 12, 12, 'whine', m + o, p['lead'] * .38)                             # dotted-8th echo
    if p.get('bell'):                                                                             # the bell: the Modular's downbeats only, an octave up
        for s, l, m, v in ln:
            if s % 4 == 0: put(s * Q, 13, 'bell', m + 12, p['bell'])

# ---------------------------------------------------------------- arrangement
S = []                                                    # one entry a bar: (original bar, switches)
def sec(obs, **kw):
    for i, ob in enumerate(obs): S.append((ob, dict(kw)))
sec(range(24, 32), str=34, bell=34)                                                       # intro: strings and the tune on a bell
S[-8][1]['fx'] = ((0, 'impact', 40),); S[-2][1]['riser'] = 40; S[-1][1]['roll'] = 34
sec(range(8, 16), drums=1, acid=40, keys=34, sub=40)                                      # build: the acid line, the Rhodes
S[-8][1]['fx'] = ((0, 'crash', 40),); S[-1][1]['roll'] = 44; S[-1][1]['riser'] = 44
sec(range(16, 24), drums=2, acid=44, sub=48, stabs=36, arp=34)                            # groove: the break comes in, the Modular joins
sec(list(range(24, 34)) + list(range(24, 30)), drums=3, reese=48, sub=56, arp=40, flurry=1, stabs=38, answer=26)   # DROP
S[-16][1]['fx'] = ((0, 'impact', 62), (2, 'crash', 52)); S[-1][1]['roll'] = 40
sec(range(34, 38), keys=40, lead=46, sub=40)                                              # break: the G# pulses on the vowel lead
S[-2][1]['riser'] = 50; S[-1][1]['roll'] = 54
sec(range(38, 46), drill=1, acid=48, sub=56, arp=38, growl=46)            # drill: the second kit's rolls, growls
S[-8][1]['fx'] = ((0, 'impact', 60), (2, 'crash', 50))
sec(range(46, 54), drums=3, sub=56, reese=44, lead=50, keys=34, flurry=1)                # the runs: vowel lead, Rhodes
S[-8][1]['fx'] = ((0, 'crash', 48),)
sec(range(54, 62), drums=3, sub=56, acid=46, arp=40, bell=36, stabs=36, answer=24)        # bridge
S[-1][1]['roll'] = 50; S[-1][1]['riser'] = 46
sec(range(24, 34), drums=3, reese=50, sub=58, lead=48, harm=1, stabs=40, arp=34, arpo=12)  # second drop: the tune in thirds, up an octave on the arp
S[-10][1]['fx'] = ((0, 'impact', 62), (2, 'crash', 52))
sec(range(66, 74), drums=9, str=40, acid=34, sub=46, growl=40)                            # breakdown: halftime, strings, growls
S[-2][1]['riser'] = 52; S[-1][1]['roll'] = 56
sec(range(74, 82), drums=3, acid=50, sub=58, keys=36, growl=42, growl2=1, answer=28)       # roller
S[-8][1]['fx'] = ((0, 'impact', 60), (2, 'crash', 50))
sec(range(128, 152), drums=3, reese=52, sub=60, arp=40, flurry=1, lead=50, harm=1, stabs=38, growl=40)   # LAST DROP
S[-24][1]['fx'] = ((0, 'impact', 64), (2, 'crash', 56))
for k in range(-24, 0, 6): S[k + 4][1]['glitch'] = 1
S[-1][1]['roll'] = 48
sec(range(144, 152), drums=1, str=36, bell=36, sub=40)                                    # outro
S[-8][1]['fx'] = ((0, 'crash', 44),); S[-4][1]['drums'] = 0; S[-3][1]['drums'] = 0; S[-2][1]['drums'] = 0; S[-1][1]['drums'] = 0

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for k in range(0, len(S), 2):
        P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
        for j in range(2):
            if k + j < len(S): ob, p = S[k + j]; build_bar(ob, p, P, j * BR, k + j)
        key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    end = [[(0, 0, 0)] * NCH for _ in range(ROWS)]                                         # the last hit, ringing out on F#
    end[0][15] = (49, INST['impact'], 0x10 + 60); end[0][5] = (xmn(30, '808'), INST['808'], 0x10 + 58); end[0][14] = (xmn(42, 'growl0'), INST['growl0'], 0x10 + 48)
    for j, m in enumerate(voicing(30)): end[0][7 + j] = (xmn(m, 'keysD'), INST['keysD'], 0x10 + 40)
    end[0][11] = (xmn(68, 'whine'), INST['whine'], 0x10 + 40)
    pats.append(end); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Mister Quickbullet'[:20].ljust(20) + b'\x1a' + b'make_quickbullet_vb'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d bars, %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(S), len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
