#!/usr/bin/env python3
"""Builds tools/hotdamn_rave.xm : "Hot Damn", a 90s rave / IDM rework of the Caustic project "HoTdamn v050"
(tools/hotdamn_v050.caustic: 89 BPM, an organ bass and a modular lead).   usage: python3 tools/make_hotdamn_rework.py

The notes are read straight out of the .caustic file (its pattern chunks SPAT and the song sequence SEQN, see read_caustic below):
the organ's F# minor riff with its 64th-note chromatic runs (F# | C# | B | F#, the B-C# turn, the long 8-bar version and its
doubled take, the chord fall), the modular lead (F# B A B A, the C# answer, the D-E climb) and its octave pumps (B C# | D E | A B).
New, in the spirit of the Prodigy, Moby and Aphex Twin: 135 BPM breakbeats (an Amen-style break, a four-to-the-floor kick under it in
the drops), the organ riff as an ACID line (three 303 samples: closed, long and an open accented one, picked per note), a hoover,
organ stabs, a Moby-style piano / strings / "ahh" breakdown on the pumps, and an Aphex-style drill'n'bass section: every beat of the
break is cut up (pitched snare ratchets, kick 32nds, 64th hat rolls, gaps, reverse swells), with the lead's licks stuttered on a bell.
64th-note rows (speed 1, XM BPM 90 = 16 rows a beat), 128-row patterns = 2 bars = one 8-beat unit of the original.
Channels: 0 kick | 1 snare | 2 break ghost | 3 hat | 4 open hat | 5 acid | 6 sub | 7 hoover / organ stab | 8 9 10 piano / strings / vox
          11 lead | 12 lead echo / harmony | 13 bell / pumps | 14 riser / fx | 15 crash / impact.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "hotdamn_v050.caustic")
OUT = os.path.join(HERE, "hotdamn_rave.xm")
SPEED, BPM, ROWS, NCH, RB = 1, 90, 128, 16, 16      # 2.5/90 s a row, 16 rows a beat = 135 BPM; RB = rows per beat
rng = np.random.default_rng(1992)

# ---------------------------------------------------------------- the .caustic reader
def read_caustic(path):
    """Caustic 3 project: a RIFF-like rack. Each machine's SPAT chunk holds 64 patterns: their lengths in bars (64 ints), their note
    capacities (64 ints), then capacity x 56-byte note records (int flags, int dead, float start beat, float length, ..., int pitch at
    +24, ..., float velocity at +52; dead = -1 marks an empty slot), and near the end the note counts. SEQN lists the song:
    56-byte entries (int machine, int, float start beat, float length in beats, ..., int pattern at +24)."""
    d = open(path, 'rb').read()
    def chunks(tag):
        o = 0; out = []
        while True:
            o = d.find(tag, o)
            if o < 0: return out
            out.append(o); o += 4
    def spat(o):
        lens = struct.unpack('<64i', d[o + 8:o + 264]); cap = struct.unpack('<64i', d[o + 264:o + 520]); p = o + 520; pats = {}
        for i in range(64):
            notes = set()
            for k in range(cap[i]):
                r = d[p + 56 * k:p + 56 * k + 56]; ii = struct.unpack('<14i', r); f = struct.unpack('<14f', r)
                if ii[1] == 0 and 0 < ii[6] < 128 and f[2] < 1000: notes.add((round(f[2] * 16), max(1, round(f[3] * 16)), ii[6], round(f[13], 2)))
            p += 56 * cap[i]
            if notes: pats[i] = (lens[i], sorted(notes))
        return pats
    sp = chunks(b'SPAT')
    return spat(sp[0]), spat(sp[1])           # machine 0 = the modular (lead), machine 1 = the organ (bass)
MDLR, ORGN = read_caustic(SRC)                # notes: (start in 64ths, length in 64ths, midi, velocity)

def seg(P, pat, off=0, n=ROWS):
    """The notes of one 8-beat unit of a pattern: starting off rows in."""
    return [(s - off, l, m, v) for (s, l, m, v) in P[pat][1] if off <= s < off + n]

# ---------------------------------------------------------------- sounds
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
def svf(x, fc, q, sr):
    """A resonant state-variable low-pass (q = damping: small = more resonance), cutoff per sample."""
    y = np.zeros_like(x); low = band = 0.0
    for i in range(len(x)):
        f = 2 * np.sin(np.pi * min(fc[i], sr * .15) / sr)
        low += f * band; high = x[i] - low - q * band; band += f * high; y[i] = low
    return y

n = int(.3 * SR); t = tt(n)                                                        # 909-ish kick
add('kick', 'rave kick', finish(np.tanh(1.8 * np.sin(2 * np.pi * np.cumsum(50 + 160 * np.exp(-t / .02)) / SR)) * np.exp(-t / .16)
                            + hp(noise(n), 2500) * np.exp(-t / .004) * .6, .95))
n = int(.24 * SR); t = tt(n)                                                       # the break snare: gritty
x = bp(noise(n), 900, 7000) * np.exp(-t / .08) + (np.sin(2 * np.pi * 190 * t) + .7 * np.sin(2 * np.pi * 300 * t)) * np.exp(-t / .05)
x = np.round(np.tanh(2.4 * x) * 24) / 24                                           # a little crunch (5-bit), as off an old sampler
add('snare', 'break snare', finish(x, .93))
n = int(.09 * SR); t = tt(n)
add('ghost', 'break ghost', finish(bp(noise(n), 1500, 6000) * np.exp(-t / .025) + np.sin(2 * np.pi * 220 * t) * np.exp(-t / .02) * .4, .5))
n = int(.05 * SR); add('hat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .012), 7000, 4), .36))
n = int(.3 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .09), 6500, 4), .4))
n = int(.9 * SR2); add('crash', 'crash', finish(hp(noise(n), 3500, 3, SR2) * np.exp(-tt(n, SR2) / .35), .5), None, 5)
n = int(.9 * SR2); t = tt(n, SR2)
add('impact', 'impact', finish(np.tanh(2.2 * np.sin(2 * np.pi * np.cumsum(32 + 70 * np.exp(-t / .07)) / SR2)) * np.exp(-t / .3)
                               + lp(noise(n), 1200, 2, SR2) * np.exp(-t / .2) * .5, .9), None, 5)
def acid(dur, cut0, cut1, tau, q, drive, peak):
    n = int(dur * SR); t = tt(n); f = mid2f(42)                                     # TB-303 (F#2): saw, resonant filter, envelope
    x = saw(f, n) * .8 + square(f, n) * .2
    y = svf(x, cut1 + (cut0 - cut1) * np.exp(-t / tau), q, SR)
    return finish(np.tanh(drive * y) * np.minimum(1, t / .002) * np.minimum(1, (t[-1] - t) / .02), peak)
add('acids', 'acid short', acid(.2, 1900, 300, .05, .22, 2.2, .78), 42)
add('acidl', 'acid long', acid(.55, 1500, 450, .18, .2, 2.0, .74), 42)
add('acida', 'acid accent', acid(.35, 3300, 600, .08, .1, 3.0, .82), 42)
n = int(.8 * SR2); t = tt(n, SR2); f = mid2f(30)                                   # sub (F#1)
add('sub', 'sub', finish(np.sin(2 * np.pi * f * t) * np.minimum(1, t / .004) * np.minimum(1, (t[-1] - t) / .06), .95), 30, 5)
n = int(.7 * SR2); t = tt(n, SR2); f0 = mid2f(42)                                  # hoover (F#2): detuned saws gliding up into pitch
glide = f0 * 2 ** ((-5 * np.exp(-t / .05) + .15 * np.sin(2 * np.pi * 5.5 * t)) / 12)
x = sum(saw(f0, n, SR2, ph=rng.uniform(0, 6.28), vib=glide * 2 ** (c / 1200)) for c in (-22, -8, 8, 22)) / 4 + .5 * saw(f0 / 2, n, SR2, vib=glide / 2)
add('hoover', 'hoover', finish(np.tanh(1.5 * lp(x, 2600, 2, SR2)) * np.minimum(1, t / .01) * np.minimum(1, (t[-1] - t) / .2), .72), 42, 5)
n = int(.25 * SR); t = tt(n); f = mid2f(60)                                        # rave organ stab (C4): drawbars + key click
x = sum(a * np.sin(2 * np.pi * f * h * t) for h, a in ((1, 1), (2, .8), (3, .6), (4, .4), (6, .25), (8, .3)))
add('organ', 'organ stab', finish(x * np.exp(-t / .1) + hp(noise(n), 3000) * np.exp(-t / .003) * .3, .62), 60)
n = int(1.3 * SR2); t = tt(n, SR2); f = mid2f(60); x = np.zeros(n)                 # piano (C4): stretched partials, hammer
for k in range(1, 12):
    fk = k * f * np.sqrt(1 + .0004 * k * k)
    if fk < SR2 * .45: x += np.sin(2 * np.pi * fk * t) / k ** 1.1 * np.exp(-t / (.9 / k ** .7))
x += bp(noise(n), 1000, 4000, 2, SR2) * np.exp(-t / .01) * .4
add('piano', 'piano', finish(x * np.minimum(1, t / .002), .7), 60, 5)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # strings (C4): six saws, slow bow
vib = f * (1 + .004 * np.sin(2 * np.pi * 5 * t))
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28), vib=vib * 2 ** (c / 1200)) for c in (-14, -9, -3, 3, 9, 14)) / 6
add('strings', 'strings', finish(lp(x, 2400, 2, SR2) * np.minimum(1, t / .3) * np.minimum(1, (t[-1] - t) / .45), .55), 60, 5)
n = int(1.2 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # "ahh" (C4): a buzz through three formants
vib = f * (1 + .006 * np.sin(2 * np.pi * 5.2 * t)); src = saw(f, n, SR2, vib=vib)
x = bp(src, 600, 850, 2, SR2) + .6 * bp(src, 1050, 1350, 2, SR2) + .25 * bp(src, 2400, 2800, 2, SR2)
add('vox', 'ahh', finish(x * np.minimum(1, t / .12) * np.minimum(1, (t[-1] - t) / .35), .6), 60, 5)
n = int(.45 * SR); t = tt(n); f = mid2f(72)                                        # rave lead (C5): two squares and a saw, detuned
x = .5 * square(f * 2 ** (6 / 1200), n) + .5 * square(f * 2 ** (-6 / 1200), n) + .4 * saw(f * 2, n)
add('lead', 'rave lead', finish(lp(x, 5000) * np.minimum(1, t / .003) * np.exp(-t / .5), .6), 72)
n = int(.7 * SR); t = tt(n); f = mid2f(72)                                         # glass bell (C5)
add('bell', 'bell', finish(np.sin(2 * np.pi * f * t + 1.6 * np.exp(-t / .25) * np.sin(2 * np.pi * 3.5 * f * t)) * np.exp(-t / .3), .6), 72)
n = int(1.4 * SR2); t = tt(n, SR2)                                                 # riser
segs = [bp(noise(128), 250 + 4000 * (i / n) ** 2, 500 + 6000 * (i / n) ** 2, 2, SR2) for i in range(0, n, 128)]
add('riser', 'riser', finish(np.concatenate(segs)[:n] * (t / t[-1]) ** 1.5, .5), None, 5)
KEYS = ['kick', 'snare', 'ghost', 'hat', 'ohat', 'crash', 'impact', 'acids', 'acidl', 'acida', 'sub', 'hoover', 'organ', 'piano',
        'strings', 'vox', 'lead', 'bell', 'riser']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

# ---------------------------------------------------------------- harmony (the chords under each half of a unit)
CH = {'F#m': (61, 66, 69), 'C#m': (61, 64, 68), 'Bm': (62, 66, 71), 'D': (62, 66, 69), 'E': (64, 68, 71), 'A': (61, 64, 69)}
ROOT = {'F#m': 42, 'C#m': 37, 'Bm': 35, 'D': 38, 'E': 40, 'A': 33}
# a unit: (organ pattern, offset in rows, modular pattern, offset, the two chords)
U = {'A': (0, 0, 0, 0, ('F#m', 'F#m')), 'B': (1, 0, 1, 0, ('C#m', 'C#m')), 'C': (2, 0, 0, 0, ('Bm', 'Bm')), 'D': (3, 0, 1, 0, ('F#m', 'F#m')),
     'E': (6, 0, 4, 0, ('D', 'E')), 'E2': (4, 0, 4, 0, ('D', 'E')),
     'P1': (None, 0, 6, 0, ('Bm', 'C#m')), 'P2': (None, 0, 8, 0, ('D', 'E')), 'P3': (None, 0, 9, 0, ('A', 'Bm')),
     'S1': (8, 0, 0, 0, ('F#m', 'C#m')), 'S2': (9, 0, 1, 0, ('Bm', 'C#m')),
     'X0': (10, 0, 12, 0, ('Bm', 'C#m')), 'X1': (10, 128, 13, 0, ('D', 'E'))}
for k, (mp, ch) in enumerate(((0, ('F#m', 'F#m')), (1, ('C#m', 'C#m')), (4, ('D', 'E')), (0, ('F#m', 'F#m')))):
    U['BIG%d' % k] = (5, 128 * k, mp, 0, ch); U['DBL%d' % k] = (7, 128 * k, mp, 0, ch)

# ---------------------------------------------------------------- one 2-bar pattern
AMEN = [dict(K=(0, 2, 10, 11), S=(4, 12), G=(7, 9, 15)), dict(K=(0, 2, 10), S=(4, 14), G=(7, 9, 12))]   # 16th steps, per bar
def build(p, uidx):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    def put(r, ch, key, midi=None, vol=64, note=None):
        if 0 <= r < ROWS and vol > 0:
            P[r][ch] = (note if note else xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, int(vol))))
    def clear(r, chs):
        for ch in chs:
            if 0 <= r < ROWS: P[r][ch] = (0, 0, 0)
    op, oo, mp, mo, chords = U[p['u']]
    d = p.get('drums', 0)
    # ---- drums: 1 = hats and ghosts, 2 = the break, 3 = the break over a four-to-the-floor kick, 4 = drill'n'bass
    for bar in (0, 64):
        a = AMEN[bar // 64]
        if d >= 1:
            for s in range(0, 16, 2): put(bar + 4 * s, 3, 'hat', None, 26 if s % 4 == 2 else 16)
            for s in a['G']: put(bar + 4 * s, 2, 'ghost', None, 20)
        if d >= 2:
            for s in a['K']: put(bar + 4 * s, 0, 'kick', None, 58 if s == 0 else 46)
            for s in a['S']: put(bar + 4 * s, 1, 'snare', None, 58)
        if d == 3:
            for s in (0, 4, 8, 12): put(bar + 4 * s, 0, 'kick', None, 62)
            for s in (2, 6, 10, 14): put(bar + 4 * s, 4, 'ohat', None, 24)
            put(bar + 61, 3, 'hat', None, 14); put(bar + 62, 3, 'hat', None, 18)          # a 64th pickup into the bar
    if d == 4:                                                                           # drill'n'bass: every beat cut up
        for b in range(8):
            r0 = b * RB; g = np.random.default_rng(uidx * 31 + b * 7 + p.get('seed', 0)); c = int(g.integers(0, 7))
            if c == 1:                                                                    # pitched snare ratchet
                k = int(g.choice([4, 6, 8])); sp = int(g.choice([1, 2]))
                for j in range(k): put(r0 + 8 + j * sp, 1, 'snare', None, 22 + 4 * j, note=49 + j * int(g.choice([1, 2])))
            elif c == 2:                                                                  # kick 32nds
                for j in range(4): put(r0 + 2 * j, 0, 'kick', None, 58 - 8 * j)
                put(r0 + 12, 1, 'snare', None, 50)
            elif c == 3:                                                                  # a 64th hat roll
                for j in range(16): put(r0 + j, 3, 'hat', None, 8 + j * 2)
            elif c == 4:                                                                  # a gap with one low snare in it
                for r in range(r0, r0 + RB): clear(r, (0, 1, 2, 3, 4))
                put(r0 + 6, 1, 'snare', None, 46, note=44)
            elif c == 5:                                                                  # a reverse swell into the next beat
                for j in range(16): put(r0 + j, 2, 'ghost', None, 4 + j * 3, note=49 + j // 3)
            elif c == 6:                                                                  # triplet kicks against the 64ths
                for j in range(3): put(r0 + j * 5, 0, 'kick', None, 54)
                put(r0 + 15, 1, 'snare', None, 30, note=56)
    if p.get('roll'):                                                                    # a snare roll speeding up, pitch rising
        for r in range(ROWS):
            st = 16 if r < 32 else 8 if r < 64 else 4 if r < 96 else 2 if r < 112 else 1
            if r % st == 0: put(r, 1, 'snare', None, p['roll'] * (.3 + .7 * r / ROWS), note=49 + r // 24)
    if p.get('fill'):
        for r in range(112, ROWS, 2): put(r, 1, 'snare', None, 20 + (r - 112) * 2)
    if p.get('riser'): put(0, 14, 'riser', None, p['riser'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    # ---- the organ riff: an acid line (mono, picking the sample per note) and a sub on the beats
    if op is not None and (p.get('acid') or p.get('sub')):
        last = (-9, 0)
        for (s, l, m, v) in sorted(seg(ORGN, op, oo)):
            if s == last[0] or (m == last[1] and s - last[0] <= 2): continue              # mono; the doubled take's flams dropped
            last = (s, m)
            if p.get('acid'):
                key = 'acida' if v >= .95 and l >= 2 else 'acidl' if l >= 8 else 'acids'
                put(s, 5, key, m + 12 + p.get('acido', 0), p['acid'] * (.55 + .45 * v))
            if p.get('sub') and s % 8 == 0 and l >= 2: put(s, 6, 'sub', m if m < 42 else m - 12, p['sub'])
    elif p.get('sub'):
        for h, c in enumerate(chords): put(64 * h, 6, 'sub', ROOT[c] - 12 if ROOT[c] >= 42 else ROOT[c], p['sub'])
    # ---- harmony: hoover hits, organ stabs, piano / strings / vox chords
    for h, c in enumerate(chords):
        r0 = 64 * h
        if p.get('hoover'): put(r0, 7, 'hoover', ROOT[c] if ROOT[c] >= 40 else ROOT[c] + 12, p['hoover'])
        if p.get('organ'):
            for s in (6, 10, 14):
                for j, m in enumerate(CH[c][:1]): put(r0 + 4 * s, 7, 'organ', m + 12, p['organ'] - (s == 14) * 8)
        for key in ('piano', 'strings', 'vox'):
            if p.get(key):
                for j, m in enumerate(CH[c]): put(r0 + (8 * j if key == 'piano' and p.get('roll_ch') else 0), 8 + j, key, m - (12 if key == 'strings' else 0), p[key] - 3 * j)
        if p.get('pianopump'):                                                            # Moby: the chord again on the offbeats
            for s in (6, 10):
                for j, m in enumerate(CH[c]): put(r0 + 4 * s, 8 + j, 'piano', m, p['pianopump'] - 3 * j)
    # ---- the modular lead
    if p.get('lead'):
        o = p.get('oct', 0); key = p.get('lsnd', 'lead')
        for k, (s, l, m, v) in enumerate(seg(MDLR, mp, mo)):
            vol = p['lead'] * (.6 + .4 * v)
            if p.get('stutter') and k % 3 == 2:
                for j in range(4): put(s + j, 11, key, m + o, vol - 9 * j)
            else: put(s, 11, key, m + o, vol)
            if p.get('harm'): put(s, 12, key, m + o - (3 if (m + o) % 12 in (1, 4, 9) else 4), vol - 12)
            else: put(s + 12, 12, key, m + o, vol * .38)                                  # a dotted-8th echo
    if p.get('pumps'):                                                                    # the octave pumps on the bell / piano
        for (s, l, m, v) in seg(MDLR, mp if U[p['u']][2] in (6, 8, 9) else 6, 0): put(s, 13, p.get('psnd', 'bell'), m + 12, p['pumps'] * (.6 + .4 * v))
    if p.get('licks'):                                                                    # IDM: the little licks, on every beat, shifted
        for b in range(8):
            lk = (2, 3, 12, 13)[(b + uidx) % 4]; sh = (0, 12, 7, 12, 0, -5, 12, 19)[b]
            for (s, l, m, v) in seg(MDLR, lk, 0, RB): put(b * RB + s, 13, 'bell', m + sh, p['licks'] * (.6 + .4 * v))
    if p.get('fall'):                                                                     # the organ's chord fall, on the last beat
        nts = sorted(seg(ORGN, 11, 0, RB))
        for (s, l, m, v) in nts:
            top = [x for x in nts if x[0] == s]
            if m == min(x[2] for x in top): put(112 + s, 5, 'acids', m + 24, p['fall'])
            if m == max(x[2] for x in top): put(112 + s, 13, 'bell', m + 24, p['fall'] - 8)
    return P

# ---------------------------------------------------------------- the arrangement: 8-beat units (3.56 s), about 4:16
S = []
def sec(units, **kw):
    for u in units: d = dict(kw); d['u'] = u; S.append(d)
FULL = dict(drums=3, acid=50, sub=54, hoover=40, lead=48)
sec(['P1', 'P2', 'P3'], piano=42, strings=30, pumps=34, psnd='piano', sub=36)                               # intro: Moby
sec(['P1', 'P2', 'P3'], piano=40, pianopump=30, strings=32, pumps=34, psnd='piano', sub=44, drums=1)
S[-1].update(roll=52, riser=48)
sec(['A', 'B', 'C', 'D', 'A', 'B', 'C', 'D'], **FULL)                                                         # DROP 1: the riff as acid
S[-8]['fx'] = ((0, 'impact', 62), (1, 'crash', 52))
sec(['E', 'E2', 'E', 'E2', 'A', 'B', 'C', 'D'], **dict(FULL, organ=36, harm=1))
S[-8]['fx'] = ((0, 'crash', 48),); S[-1].update(fall=52, fill=1)
sec(['P1', 'P2', 'P3', 'P1', 'P2', 'P3'], strings=38, vox=34, piano=36, pumps=38, sub=40)                      # breakdown: strings, ahh
S[-6]['fx'] = ((0, 'crash', 40),)
sec(['S1', 'S2'], strings=36, vox=32, lead=42, lsnd='bell', sub=44, drums=1)
S[-1]['riser'] = 50
sec(['A', 'B'], drums=4, acid=40, lead=40, stutter=1, seed=3)                                                 # build: drill
S[-1].update(roll=58, fall=50)
sec(['BIG0', 'BIG1', 'BIG2', 'BIG3'], **dict(FULL, organ=36))                                                  # DROP 2: the long riff
S[-4]['fx'] = ((0, 'impact', 64), (1, 'crash', 54))
sec(['DBL0', 'DBL1', 'DBL2', 'DBL3'], **dict(FULL, organ=34, harm=1, acido=12, piano=30))
S[-4]['fx'] = ((0, 'crash', 50),); S[-1]['fill'] = 1
sec(['X0', 'X1', 'X0', 'X1', 'X0', 'X1'], drums=4, acid=46, sub=50, licks=40, strings=24)                      # IDM: drill'n'bass
S[-6]['fx'] = ((0, 'impact', 56),)
for k in range(1, 6): S[-6 + k]['seed'] = k * 11
S[-3]['lead'] = 38; S[-3]['stutter'] = 1
sec(['P2', 'P3'], drums=4, strings=34, vox=30, licks=34, pumps=36, seed=97)
S[-1].update(riser=52, roll=56, fall=50)
sec(['A', 'B', 'C', 'D', 'E', 'E2'], **dict(FULL, organ=36, harm=1, piano=30))                                # FINAL
S[-6]['fx'] = ((0, 'impact', 64), (1, 'crash', 56))
sec(['DBL0', 'DBL1', 'DBL2', 'DBL3', 'E', 'E2'], **dict(FULL, organ=36, harm=1, acido=12, strings=26))
S[-6]['fx'] = ((0, 'crash', 52),); S[-1].update(fall=54, fill=1)
sec(['P1', 'P2', 'P3'], piano=38, strings=32, vox=28, pumps=32, psnd='piano', sub=40)                         # outro
S[-3]['fx'] = ((0, 'crash', 44),)

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for ui, sp in enumerate(S):
        P = build(sp, ui); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    end = [[(0, 0, 0)] * NCH for _ in range(ROWS)]                                          # the last hit: F# minor, ringing out
    end[0][15] = (49, INST['impact'], 0x10 + 60); end[0][6] = (xmn(30, 'sub'), INST['sub'], 0x10 + 56)
    end[0][7] = (xmn(42, 'hoover'), INST['hoover'], 0x10 + 44); end[0][5] = (xmn(42, 'acida'), INST['acida'], 0x10 + 44)
    for j, m in enumerate(CH['F#m']): end[0][8 + j] = (xmn(m, 'strings') - 12, INST['strings'], 0x10 + 40)
    end[0][11] = (xmn(66, 'lead'), INST['lead'], 0x10 + 42)
    pats.append(end); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Hot Damn'[:20].ljust(20) + b'\x1a' + b'make_hotdamn_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
