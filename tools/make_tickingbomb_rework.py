#!/usr/bin/env python3
"""Builds tools/the_ticking_bomb.xm : "The Ticking Bomb", an El B.D'ees Latin house rework of tools/the_dipper_man_ticking_bomb.xm.
Kept from the original: the offbeat riff (F# F# F# F# B B E E), the A/D bass and the G-E-G stinger.  New: dusty breakbeat drums under a
four-on-the-floor kick, congas and timbales, a low menacing bass, and a half-time "downgroove" in the middle (the fuse and the blast of
the original become the way in and out of it).  El B.D'ees's palette: nylon, flute, marimba, vibes, bell, pads (shared with Flexicode).
Channels: 0 kick | 1 snare / rim | 2 hat | 3 conga | 4 sub | 5 6 pad | 7 timbale | 8 nylon | 9 marimba | 10 flute | 11 flute echo
          12 vibes | 13 bell | 14 low whine | 15 fx.      124 BPM speed 3: 32-row patterns = one bar (2 rows a 16th).
usage:  python3 tools/make_tickingbomb_rework.py     Re-running gives byte-identical output.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
from make_flexicode_rework import I, tt, noise, lp, hp, bp, saw, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "the_ticking_bomb.xm")
BPM, SPEED, ROWS = 124, 3, 32
ROW = 2.5 * SPEED / BPM; BEAT = 8 * ROW
rng = np.random.default_rng(124)
F.NCH = NCH = 16; F.BPM = BPM; F.SPEED = SPEED

# ---------------------------------------------------------------- sounds: a dusty snare, a hat, congas, a timbale, a dark pad, a whine
n = int(.22 * SR); t = tt(n)
x = .55 * np.sin(2 * np.pi * 190 * t) * np.exp(-t / .05) + lp(bp(noise(n), 900, 7000), 5000) * np.exp(-t / .07)
I['snare'] = dict(name='dusty snare', x=finish(np.tanh(2.2 * x) * .8, .8), gen=None, rel=12)
n = int(.06 * SR); I['hat'] = dict(name='hat', x=finish(hp(noise(n) * np.exp(-tt(n) / .015), 7000, 4), .35), gen=None, rel=12)
n = int(.3 * SR); t = tt(n); f = mid2f(57)
x = np.sin(2 * np.pi * f * t * (1 + .25 * np.exp(-t / .01))) * np.exp(-t / .12) + .2 * bp(noise(n), 300, 2000) * np.exp(-t / .01)
I['conga'] = dict(name='conga', x=finish(x, .8), gen=57, rel=12)
n = int(.35 * SR); t = tt(n)
x = (np.sin(2 * np.pi * 520 * t) + .6 * np.sin(2 * np.pi * 1340 * t)) * np.exp(-t / .09) + .3 * hp(noise(n), 3000) * np.exp(-t / .02)
I['timbale'] = dict(name='timbale', x=finish(x, .7), gen=None, rel=12)
n = int(BEAT * 2 * SR2); t = tt(n, SR2); f = mid2f(60)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-11, -4, 5, 12)) / 4
I['pad'] = dict(name='dark pad', x=finish(lp(x, 900, 2, SR2) * np.minimum(1, t / .3), .5, .05, SR2), gen=60, rel=5)
n = int(BEAT * 2 * SR2); t = tt(n, SR2); f = mid2f(72)
x = np.sin(2 * np.pi * f * t + 2.5 * np.sin(2 * np.pi * 5.5 * t)) * np.minimum(1, t / .4)     # a thin, bending sine: the "insane in the brain" whine
I['whine'] = dict(name='low whine', x=finish(x, .45, .05, SR2), gen=72, rel=5)
n = int(ROWS * ROW * SR2); t = tt(n, SR2); u = t / t[-1]
I['swell'] = dict(name='reverse swell', x=finish(lp(hp(noise(n), 2000, 2, SR2), 5000, 2, SR2) * u ** 3, .45), gen=None, rel=5)
KEYS = ['kick', 'snare', 'hat', 'conga', 'timbale', 'sub', 'pad', 'whine', 'nylon', 'flute', 'marimba', 'vibes', 'bell', 'swell', 'drop']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
F.KEYS = KEYS; F.INST = INST

# ---------------------------------------------------------------- the original's material, transposed down a semitone to concert pitch
RIFF = [(4, 65), (12, 65), (20, 65), (28, 65), (36, 70), (44, 70), (52, 63), (60, 63)]   # 2 bars, rows at 16 a beat-pair: F F F F Bb Bb Eb Eb
BASS = [(0, 32), (8, 37)]                          # Ab / Db (the original's A / D, a semitone down) alternating each half bar
CHORD = {0: (56, 60, 63, 65), 1: (58, 61, 65, 68)}  # Fm7 / Bbm7-ish colour over the two-bar cycle
STING = [(0, 78), (8, 75), (16, 78)]               # G-E-G, down a semitone
def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    half = p.get('half', 0); h = p.get('bar', 0) % 2
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (F.xmnote(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    d = p.get('drums', 0)
    for r in range(ROWS):
        if half:   # downgroove: a slow, heavy boom-bap at half the pulse
            if d and r in (0, 22): put(r, 0, 'kick', None, 62 if r == 0 else 50)
            if d and r == 16: put(r, 1, 'snare', None, 58)
            if d and r % 4 == 0: put(r, 2, 'hat', None, 20 if r % 8 else 28)
            if d >= 2 and r in (6, 14, 26): put(r, 3, 'conga', 50 + 7 * (r == 14), 30)
            continue
        if d and r % 8 == 0: put(r, 0, 'kick', None, 60)
        if d >= 2 and r in (8, 24): put(r, 1, 'snare', None, 40)
        if d >= 2 and r in (14, 30): put(r, 1, 'snare', None, 14)                           # ghost notes: the break under the house kick
        if d and r % 8 == 4: put(r, 2, 'hat', None, 34)
        if d >= 2 and r % 4 == 2: put(r, 2, 'hat', None, 12)
        if p.get('perc') and r in (2, 6, 10, 18, 22, 26, 28): put(r, 3, 'conga', 57 + 7 * (r in (6, 22)) - 5 * (r == 28), p['perc'] - 6 * (r % 4 == 2))
        if p.get('timb') and r in (12, 15, 29): put(r, 7, 'timbale', None, p['timb'])
    if p.get('bass'):
        for r0, m in BASS:
            m = m + (5 if h else 0)
            if half: put(r0 * 2, 4, 'sub', m, p['bass'])
            else:
                for r in range(2 * r0, 2 * r0 + 16, 2): put(r, 4, 'sub', m + 12 * (r % 4 == 2), p['bass'] - 10 * (r % 4 == 2))   # octave-jumping 8ths
    if p.get('pad') and (h == 0 or half):
        for j, m in enumerate(CHORD[h][:2]): put(0, 5 + j, 'pad', m - 12 * half, p['pad'] - 4 * j)
    if p.get('ny'):
        for r in (4, 12, 20, 28) if not half else (8, 24):
            put(r, 8, 'nylon', CHORD[h][-1 - (r // 8) % 2], p['ny'] - 6 * (r % 16 != 4))
    if p.get('riff'):
        key = p.get('rk', 'marimba'); ch = 9 if key == 'marimba' else 10
        for r, m in RIFF:
            r -= 32 * h
            if 0 <= r < 32:
                rr = r * 2 if half else r
                if rr < 32: put(rr, ch, key, m + p.get('oct', 0), p['riff'])
                if key == 'flute' and rr + 3 < 32: put(rr + 3, 11, 'flute', m + p.get('oct', 0), p['riff'] * .4)
    if p.get('vib') and h == 1:
        for r, m in STING: put(r, 12, 'vibes', m, p['vib'])
    if p.get('bell'):
        for r, m in STING: put(r + 2, 13, 'bell', m + 12, p['bell'] - r)
    if p.get('whine'): put(0, 14, 'whine', 70 - 2 * h, p['whine'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    for (r, vol) in p.get('roll', ()): put(r, 1, 'snare', None, vol)
    return P

S = []
def sec(n, **kw):
    for b in range(n): S.append(dict(kw, bar=b))
FUSE = tuple((r, 10 + r) for r in range(0, 32, 2)) + tuple((r, 30 + r) for r in range(17, 32, 2))
sec(4, pad=34, ny=36, bell=40, fx=((0, 'swell', 30),))                                       # intro: the stinger on bells
sec(4, drums=1, bass=52, pad=34, ny=38, riff=44)                                             # the kick and the riff
sec(8, drums=2, bass=56, pad=34, ny=40, riff=48, perc=34, vib=40)                            # house: breakbeat, congas, stinger on vibes
sec(8, drums=2, bass=56, pad=36, ny=40, riff=50, rk='flute', perc=36, timb=30, vib=40, whine=18)
sec(2, drums=1, bass=54, pad=36, riff=44, perc=30, roll=FUSE[:16]); S[-1]['roll'] = FUSE       # the fuse
sec(8, half=1, drums=2, bass=60, pad=40, whine=26, ny=34, riff=40, vib=36)                     # the downgroove: half time, heavy
sec(4, half=1, drums=2, bass=60, pad=40, whine=30, riff=44, rk='flute', vib=40, bell=30)
S[-1]['fx'] = ((0, 'swell', 44),)
sec(8, drums=2, bass=58, pad=38, ny=42, riff=50, rk='flute', oct=12, perc=38, timb=32, vib=42, fx=((0, 'drop', 52),))   # the blast: back to house
sec(8, drums=2, bass=58, pad=36, ny=40, riff=48, perc=36, timb=30, vib=40, whine=18)
sec(4, drums=1, bass=50, pad=34, ny=36, riff=40, perc=28)                                    # outro
sec(2, pad=30, ny=32, bell=34)
for i, p in enumerate(S): p['bar'] = i

def main():
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'The Ticking Bomb'[:20].ljust(20) + b'\x1a' + b'make_tickingbomb'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
