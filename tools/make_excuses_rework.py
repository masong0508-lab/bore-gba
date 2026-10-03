#!/usr/bin/env python3
"""Builds tools/excuses_house.xm : "Excuses", an expansive house / ambient rework of "The Dipper Man - Excuses"
(tools/the_dipper_man_excuses.xm).   usage:  python3 tools/make_excuses_rework.py   (from the project root; numpy + scipy)

From the original (read straight out of the XM): the chord pairs (F | Dm | F/C | C), the climbing bass (F F F G A A | Bb Bb Bb B C C)
and the melody over them (C C C D C D E E | E D C D E D# D A), and the A-major-ish ending. Each 24-row pattern of the original becomes two
bars of 4/4 at 124 BPM (its 16-row first chord gets a bar, its 8-row second chord gets a bar, notes stretched to fit).
New: deep house (four on the floor, offbeat open hats, a rolling bass, organ stabs), wide pads that "open" over the song (three baked
pad colours, dark to bright, so the filter sweep costs nothing), a chiptune sine lead (a 4-bit stepped sine) and a glass-bell lead with a ping-pong echo, FM bell
arps, a vinyl / air texture, risers, reverse swells and sub drops. 32nd-note rows (speed 3) for ratchets and stutters; all 16 voices.
Channels: 0 kick | 1 clap | 2 hat | 3 open hat / shaker / ride | 4 bass | 5 6 7 stabs | 8 9 10 pads | 11 lead | 12 echo | 13 arp
          14 fx | 15 texture / bell.        124 BPM, speed 3, 64-row patterns (two bars).  About 6:45.   Byte-identical on re-runs.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "excuses_house.xm")
BPM, SPEED, ROWS, NCH = 124, 3, 64, 16
ROW = 2.5 * SPEED / BPM; BAR = 32 * ROW; BEAT = 8 * ROW
rng = np.random.default_rng(124)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)

# ---------------------------------------------------------------- sounds
n = int(.42 * SR); t = tt(n)
ph = 2 * np.pi * np.cumsum(44 + 110 * np.exp(-t / .03)) / SR
add('kick', 'house kick', finish(np.tanh(1.8 * np.sin(ph) * np.exp(-t / .2)), .98))
n = int(.3 * SR); c = np.zeros(n)
for off, dec, a in ((0, .006, .7), (.011, .007, .8), (.022, .008, .8), (.032, .09, 1.0)):
    i = int(off * SR); m = n - i; c[i:] += noise(m) * np.exp(-tt(m) / dec) * a
add('clap', 'clap', finish(bp(c, 800, 4000) + lp(c, 500) * .25, .75))
n = int(.05 * SR); add('hat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .012), 7000, 4), .38))
n = int(.4 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .14), 6000, 4), .42))
n = int(.09 * SR); t = tt(n); add('shaker', 'shaker', finish(bp(noise(n), 4500, 8000, 2) * np.sin(np.pi * t / t[-1]) ** 2, .3))
n = int(.9 * SR); t = tt(n); m = square(4400, n) * square(6100, n)
add('ride', 'ride', finish(hp(m * .4 + noise(n) * .6, 5000, 2) * np.exp(-t / .35), .3))
n = int(.38 * SR); t = tt(n); f = mid2f(41)                                        # rolling house bass (F2)
b = np.sin(2 * np.pi * f * t) * 1.1 + lp(saw(f, n), 700) * .6 * np.exp(-t / .12)
add('bass', 'house bass', finish(np.tanh(1.5 * b) * np.minimum(1, t / .004) * np.exp(-t / .3), .92), 41)
n = int(.35 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # organ stab (C4): drawbar sines + a click
x = np.sin(2 * np.pi * f * t) + .6 * np.sin(4 * np.pi * f * t) + .35 * np.sin(6 * np.pi * f * t) + .2 * np.sin(8 * np.pi * f * t)
add('stab', 'organ stab', finish(x * np.minimum(1, t / .003) * np.exp(-t / .16), .6), 60, rel=5)
def pad(cut, name):                                                                 # two bars long, swells in, three colours
    n = int(BAR * 2 * SR2); t = tt(n, SR2); f = mid2f(60)
    x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-12, -5, 0, 5, 12)) / 5 + .4 * np.sin(np.pi * f * t)
    x = lp(x, cut, 2, SR2) * np.minimum(1, t / .6) * np.minimum(1, (t[-1] - t) / .5) * (1 + .12 * np.sin(2 * np.pi * .25 * t))
    return finish(x, .55, .05, SR2)
add('padD', 'pad dark', pad(700, 'dark'), 60, rel=5); add('padM', 'pad mid', pad(1500, 'mid'), 60, rel=5); add('padB', 'pad bright', pad(3200, 'bright'), 60, rel=5)
n = int(.9 * SR); t = tt(n); f = mid2f(72)                                         # chip lead (C5): a sine stepped to 4 bits, like a
vib = f * (1 + .006 * np.sin(2 * np.pi * 6 * t) * np.clip((t - .1) / .15, 0, 1))     # sound chip's wavetable channel, with a delayed vibrato
x = np.round(np.sin(2 * np.pi * np.cumsum(vib) / SR) * 7.5) / 7.5                    # (16 levels: the soft "chiptune sine" edge)
add('vox', 'chip sine lead', finish(x * np.minimum(1, t / .004) * (.75 + .25 * np.exp(-t / .1)) * np.minimum(1, (t[-1] - t) / .15), .7), 72)
n = int(1.4 * SR); t = tt(n); f = mid2f(84)                                        # glass bell (C6)
x = np.sin(2 * np.pi * f * t + 1.2 * np.exp(-t / .15) * np.sin(2 * np.pi * f * 3.01 * t)) * np.exp(-t / .55) + .25 * np.sin(2 * np.pi * f * 4.2 * t) * np.exp(-t / .12)
add('bell', 'glass bell', finish(x, .7), 84)
n = int(.3 * SR); t = tt(n); f = mid2f(84)                                         # arp pluck (C6)
add('pluck', 'fm pluck', finish(np.sin(2 * np.pi * f * t + 2.2 * np.exp(-t / .03) * np.sin(4 * np.pi * f * t)) * np.exp(-t / .09), .6), 84)
n = int(BAR * 2 * SR2); t = tt(n, SR2)                                             # air / vinyl texture: two bars, loops by retrigger
x = lp(noise(n), 2500, 2, SR2) * .15 + (rng.random(n) < .0008) * rng.uniform(-1, 1, n) * 1.2
add('air', 'air texture', finish(lp(x, 4000, 2, SR2) * np.minimum(1, t / .3) * np.minimum(1, (t[-1] - t) / .3), .35, .05, SR2), None, rel=5)
n = int(BAR * 4 * SR2); t = tt(n, SR2); u = t / t[-1]; nz = noise(n)               # riser: four bars
add('riser', 'riser', finish((bp(nz, 300, 1100, 2, SR2) * (1 - u) + bp(nz, 1800, 5000, 2, SR2) * u) * u ** 2 + np.sin(2 * np.pi * np.cumsum(150 + 1500 * u ** 2) / SR2) * u ** 3 * .25, .55), None, rel=5)
n = int(BAR * SR2); t = tt(n, SR2); u = t / t[-1]
add('swell', 'reverse swell', finish(lp(hp(noise(n), 1500, 2, SR2), 6000, 2, SR2) * u ** 3, .45), None, rel=5)
n = int(1.4 * SR); t = tt(n); ph = 2 * np.pi * np.cumsum(36 + 60 * np.exp(-t / .25)) / SR
add('drop', 'sub drop', finish(np.sin(ph) * np.exp(-t / .6), .9))
n = int(1.2 * SR); add('crash', 'crash', finish(hp(noise(n), 4500, 3) * np.exp(-tt(n) / .45), .5))
KEYS = ['kick', 'clap', 'hat', 'ohat', 'shaker', 'ride', 'bass', 'stab', 'padD', 'padM', 'padB', 'vox', 'bell', 'pluck', 'air', 'riser', 'swell', 'drop', 'crash']
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the original (24-row patterns: first chord rows 0-15, second 16-23)
src = xm.parse(os.path.join(HERE, "the_dipper_man_excuses.xm"))
def notes(pat, ch): return [(r, n + 11) for r, row in enumerate(src['pats'][pat]) for (n, i, v, e, ep) in [row[ch]] if 0 < n < 97]
def stretch(r): return 2 * (r if r < 16 else 16 + (r - 16) * 2)        # original row -> 32nd row in the new two-bar pattern
BASSL = {'A': notes(3, 4), 'B': notes(4, 4)}
MEL = {'A': notes(3, 6), 'B': notes(4, 6)}
CH = {'A': [(0, (57, 60, 64, 67)), (32, (57, 60, 62, 65))],    # Fmaj9 (A C E G over F) | Dm9 (A C D F)
      'B': [(0, (58, 62, 65, 69)), (32, (58, 60, 64, 67))]}    # Bbmaj7 (Bb D F A) | C7sus-ish (Bb C E G)
ROOT = {'A': (41, 38), 'B': (46, 48)}
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    h = p.get('h', 'A')
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    d = p.get('drums', 0)
    for r in range(ROWS):
        q = r % 32                                                      # position in the bar (32nds)
        if d and q % 8 == 0: put(r, 0, 'kick', None, 62)
        if d >= 2 and q in (8, 24): put(r, 1, 'clap', None, 50)
        if d >= 2 and q % 4 == 2 and q % 8 != 6: put(r, 2, 'hat', None, 26)
        if d >= 1 and q % 8 == 4: put(r, 3, 'ohat', None, 36 if d >= 2 else 26)          # offbeat open hat
        if d >= 3 and q % 8 in (2, 6): put(r, 2, 'hat', None, 30 if q % 8 == 6 else 18)
        if d >= 3 and q % 4 == 1: put(r, 3, 'shaker', None, 14)
        if d >= 4 and r >= 56 and r % 2 == 0 and r < 64 and p.get('fill'): put(r, 1, 'clap', None, 24 + (r - 56) * 4)
    if p.get('ride'):
        for r in range(0, ROWS, 8): put(r + 4, 15, 'ride', None, p['ride'])
    if p.get('bass'):
        bl = BASSL[h]
        for r, m in bl: put(stretch(r), 4, 'bass', m - 24, p['bass'])
        for r, m in bl:                                                # a rolling offbeat pickup between the original's notes
            rr = stretch(r) + 4
            if rr < ROWS and P[rr][4][0] == 0: put(rr, 4, 'bass', m - 24 + 12, p['bass'] - 16)
    if p.get('root'):                                                  # intro / breakdown: just the roots, long
        for k, rt in enumerate(ROOT[h]): put(32 * k, 4, 'bass', rt - 12 if rt > 45 else rt, p['root'])
    if p.get('stab'):
        for r0, ch in CH[h]:
            for off in (6, 14, 22, 28):
                for j, m in enumerate(ch[1:]): put(r0 + off, 5 + j, 'stab', m, p['stab'] - j * 3 - (6 if off in (14, 28) else 0))
    if p.get('pad'):
        key = p['pad']
        for r0, ch in CH[h]:
            for j, m in enumerate(ch[:3]): put(r0, 8 + j, key, m, p.get('pv', 44) - j * 3)
    if p.get('lead'):
        key = p.get('lk', 'vox'); o = p.get('oct', 0)
        for r, m in MEL[h]:
            mm = m - 12 + o if key == 'vox' else m + o
            put(stretch(r), 11, key, mm, p['lead']); put(stretch(r) + 6, 12, key, mm, p['lead'] * .4)   # echo a dotted 8th later
    if p.get('arp'):
        for r in range(0, ROWS, 4):
            ch = [c for r0, c in CH[h] if r0 <= r][-1]
            put(r, 13, 'pluck', ch[(r // 4) % len(ch)] + 12 + (12 if (r // 16) % 2 else 0), p['arp'] - (0 if r % 8 == 0 else 8))
    if p.get('bells'):
        for r, m in MEL[h][::2]: put(stretch(r), 15, 'bell', m + 12, p['bells'])
    if p.get('air'): put(0, 15, 'air', None, p['air'])
    if p.get('stutter'):
        for r in range(56, 64): put(r, 11, p.get('lk', 'vox'), MEL[h][-1][1] - 12, p['stutter'] - (r - 56) * 3)
    for (r, key, vol) in p.get('fx', ()): put(r, 14, key, None, vol)
    return P

# ---------------------------------------------------------------- arrangement (two bars a pattern, 3.87 s): about 6:45
S = []
def sec(n, **kw):
    for i in range(n): S.append(dict(kw, h='AB'[i % 2]))
sec(4, pad='padD', pv=36, air=40, root=40)                                                 # intro: dark pads, air, roots
sec(6, pad='padD', pv=40, air=38, root=44, bells=34, drums=1)
S[-1]['fx'] = ((0, 'swell', 44),)
sec(4, pad='padM', pv=40, bass=50, drums=2, arp=30); S[-4]['fx'] = ((0, 'crash', 46),)     # the groove arrives
sec(4, pad='padM', pv=40, bass=52, drums=3, stab=40, arp=28)
S[-2]['fx'] = ((0, 'riser', 46),); S[-1]['fill'] = 1; S[-1]['drums'] = 4
sec(8, pad='padM', pv=42, bass=56, drums=3, stab=42, lead=54, ride=20); S[-8]['fx'] = ((0, 'drop', 50),)   # theme 1: the vowel lead
sec(8, pad='padB', pv=40, bass=56, drums=3, stab=40, lead=54, lk='bell', arp=30, ride=24) # theme 1b: glass bell lead + arps
S[-1]['stutter'] = 50
sec(6, pad='padD', pv=44, air=44, bells=40, root=40, fx=())                                # first breakdown: ambient
S[-6]['fx'] = ((0, 'drop', 46),)
sec(6, pad='padM', pv=44, air=40, arp=36, root=42, bells=34, drums=1)
S[-2]['fx'] = ((0, 'riser', 50),); S[-1]['stutter'] = 46
sec(8, pad='padB', pv=44, bass=58, drums=3, stab=44, lead=58, ride=26, arp=26); S[-8]['fx'] = ((0, 'crash', 54),)   # drop 2
sec(8, pad='padB', pv=44, bass=58, drums=3, stab=44, lead=56, lk='bell', oct=12, bells=36, ride=26)
S[-1]['fill'] = 1; S[-1]['drums'] = 4
sec(12, pad='padD', pv=46, air=46, root=40, bells=36, arp=28)                              # the long ambient breakdown
S[-12]['fx'] = ((0, 'drop', 48),); S[-5]['fx'] = ((0, 'swell', 44),)
sec(4, pad='padM', pv=44, air=40, root=44, lead=46, drums=1)
S[-2]['fx'] = ((0, 'riser', 54),); S[-1]['stutter'] = 52; S[-1]['fill'] = 1
sec(16, pad='padB', pv=46, bass=60, drums=3, stab=46, lead=60, arp=30, ride=28, bells=34)  # final drop, everything
S[-16]['fx'] = ((0, 'crash', 58),); S[-1]['drums'] = 4; S[-1]['fill'] = 1
sec(4, pad='padM', pv=42, bass=54, drums=2, stab=36, arp=30)                               # outro
sec(5, pad='padD', pv=40, air=42, root=40, bells=32, drums=1)
sec(2, pad='padD', pv=34, air=36)

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Excuses (House)'[:20].ljust(20) + b'\x1a' + b'make_excuses_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
