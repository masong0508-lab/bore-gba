#!/usr/bin/env python3
"""Builds tools/emergency_hitech.xm : "Emergency On The Dance Floor" (the hi-tech rework). The earlier version
(tools/emergency_dance_floor.xm, reworked inside xm2gba.py's make_dance) stays in the game as a secret song (the title-screen code).

usage:  python3 tools/make_emergency_rework.py        (run from the project root; needs numpy + scipy)

The notes come from the original, read straight out of tools/emergency_dance_floor.xm: the bass riff (C .. Eb F .. G G Bb G G |
Bb Ab G . Bb Ab G . C C . . G A Bb . G A Bb . C) and the lead line over it, at the original 181 BPM. Everything else is new and
synthesised here: an FM growl bass with a sine sub under it, a supersaw pluck lead with an octave shadow and a ping-pong echo, glassy FM
arps, three-voice digital stabs, a vowel pad, a metallic hat, data blips, zaps, lasers and risers. 32nd-note resolution (speed 3) for
ratchets, stutters and glitch fills; all 16 voices, hand-panned (xm2gba.py: hitech_pan).
Channels: 0 kick | 1 snare | 2 hat | 3 glitch perc | 4 growl bass | 5 sub | 6 lead | 7 lead octave / echo | 8 arp | 9 10 11 stabs
          12 pad | 13 zap / counter | 14 fx | 15 data blips.     181 BPM speed 3, 48-row patterns (= one 24-row pattern of the original).
Re-running gives byte-identical output.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "emergency_hitech.xm")
BPM, SPEED, ROWS, NCH = 181, 3, 48, 16
ROW = 2.5 * SPEED / BPM
rng = np.random.default_rng(911)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)

# ---------------------------------------------------------------- sounds
n = int(.30 * SR); t = tt(n)                                                       # kick: tight, a click on top
ph = 2 * np.pi * np.cumsum(48 + 160 * np.exp(-t / .018)) / SR
k = np.sin(ph) * np.exp(-t / .11); k[:int(.003 * SR)] += hp(noise(int(.003 * SR)), 3000) * .6
add('kick', 'hitech kick', finish(np.tanh(2.2 * k), .98))
n = int(.20 * SR); t = tt(n)                                                       # snare: crack and body
x = bp(noise(n), 1800, 7000) * np.exp(-t / .05) + np.sin(2 * np.pi * 230 * t) * np.exp(-t / .03) * .7
add('snare', 'hitech snare', finish(np.tanh(1.8 * x), .85))
n = int(.07 * SR); t = tt(n)                                                       # hat: ring-modulated squares = metallic
m = square(3120, n) * square(4410, n); add('hat', 'metal hat', finish(hp(m + noise(n) * .5, 7000, 2) * np.exp(-t / .015), .38))
n = int(.05 * SR); t = tt(n)                                                       # glitch tick: a falling digital chirp
add('tick', 'glitch tick', finish(np.sign(np.sin(2 * np.pi * np.cumsum(6000 * np.exp(-t / .01) + 300) / SR)) * np.exp(-t / .012), .4))
n = int(.42 * SR); t = tt(n); f = mid2f(36)                                        # growl bass: FM with a moving index, then a low-pass
idx = 2.5 + 2.0 * np.sin(2 * np.pi * 7 * t)
x = np.sin(2 * np.pi * f * t + idx * np.sin(2 * np.pi * f * 2 * t)) + .4 * saw(f * 1.006, n)
add('growl', 'fm growl', finish(np.tanh(1.6 * lp(x, 1400)) * np.minimum(1, t / .004) * np.exp(-t / .5), .9), 36)
n = int(.5 * SR); t = tt(n); f = mid2f(36)
add('sub', 'sub', finish(np.sin(2 * np.pi * f * t) * np.minimum(1, t / .004) * np.exp(-t / .45), .95), 36)
n = int(.45 * SR2); t = tt(n, SR2); f = mid2f(72)                                  # supersaw pluck lead (C5)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-14, -6, 0, 6, 14)) / 4
env = np.exp(-t / .25) * np.minimum(1, t / .003); x = lp(x, 4200, 2, SR2) * env + .3 * np.sin(2 * np.pi * f * t) * env
add('lead', 'supersaw pluck', finish(x, .8), 72, rel=5)
n = int(.6 * SR); t = tt(n); f = mid2f(84)                                         # glass arp: FM bell (C6)
x = np.sin(2 * np.pi * f * t + 1.8 * np.exp(-t / .08) * np.sin(2 * np.pi * f * 3.5 * t)) * np.exp(-t / .22)
add('glass', 'fm glass', finish(x, .7), 84)
n = int(.3 * SR2); t = tt(n, SR2); f = mid2f(60)                                   # stab: detuned square + saw, bitcrushed a touch
x = (square(f, n, SR2) * .5 + saw(f * 1.01, n, SR2) * .5); x = np.round(lp(x, 3000, 2, SR2) * 12) / 12
add('stab', 'digital stab', finish(x * np.exp(-t / .12) * np.minimum(1, t / .002), .6), 60, rel=5)
n = int(1.4 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # vowel pad: saws through two moving formant bands
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-8, 0, 8)) / 3
fm = .5 + .5 * np.sin(2 * np.pi * .7 * t)
x = bp(x, 500, 900, 2, SR2) * (1 - fm) + bp(x, 1100, 1800, 2, SR2) * fm + lp(x, 400, 2, SR2) * .6
add('pad', 'vowel pad', finish(x * np.minimum(1, t / .25) * np.minimum(1, (t[-1] - t) / .3), .6, .05, SR2), 60, rel=5)
n = int(.25 * SR); t = tt(n)                                                       # zap: a pitch dive
add('zap', 'zap', finish(square(1, n) * 0 + np.sin(2 * np.pi * np.cumsum(2600 * np.exp(-t / .04) + 120) / SR) * np.exp(-t / .1), .6))
n = int(.9 * SR); t = tt(n)                                                        # laser: long falling sweep with an echo
sw = np.sin(2 * np.pi * np.cumsum(3000 * np.exp(-t / .2) + 80) / SR) * np.exp(-t / .35)
add('laser', 'laser', finish(sw + np.concatenate([np.zeros(int(.12 * SR)), sw[:-int(.12 * SR)]]) * .4, .6))
n = int(4 * 24 * ROW * 2 * SR2); t = tt(n, SR2); u = t / t[-1]; nz = noise(n)       # riser: 4 bars of rising noise + tone
add('riser', 'riser', finish((bp(nz, 400, 1200, 2, SR2) * (1 - u) + bp(nz, 2000, 5000, 2, SR2) * u) * u ** 2 + np.sin(2 * np.pi * np.cumsum(200 + 1800 * u ** 2) / SR2) * u ** 3 * .3, .55), None, rel=5)
n = int(.04 * SR); t = tt(n)                                                       # data blip (C6): a tiny square
add('blip', 'data blip', finish(square(mid2f(84), n) * np.exp(-t / .015), .35), 84)
n = int(1.0 * SR); t = tt(n)
add('crash', 'crash', finish(hp(noise(n), 5000, 3) * np.exp(-t / .35), .5))
KEYS = ['kick', 'snare', 'hat', 'tick', 'growl', 'sub', 'lead', 'glass', 'stab', 'pad', 'zap', 'laser', 'riser', 'blip', 'crash']
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the original's notes (24-row patterns; rows here are doubled)
src = xm.parse(os.path.join(HERE, "emergency_dance_floor.xm"))
def notes(pat, ch):
    return [(r, n + 11) for r, row in enumerate(src['pats'][pat]) for (n, i, v, e, ep) in [row[ch]] if 0 < n < 97]
BASS = {'A': notes(0, 0), 'B': notes(1, 0)}           # bass riff, two halves
LEAD = {'A': notes(2, 2), 'B': notes(3, 2)}           # the lead line over each half
CH = {'A': [(0, (63, 67, 70, 74)), (12, (62, 65, 70))], 'B': [(0, (63, 67, 72)), (12, (62, 65, 70))]}   # Cm9 | Gm7, Abmaj7 | Bb (voicings)
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    h = p.get('h', 'A')
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    d = p.get('drums', 0)
    for r in range(ROWS):
        q = r // 2                                                     # the original's row (a 16th)
        if d and r % 2 == 0:
            if q % 8 == 0 or (d >= 2 and q in (10, 18, 19, 22)): put(r, 0, 'kick', None, 62 if q % 8 == 0 else 46)
            if q % 8 == 4: put(r, 1, 'snare', None, 56)
            put(r, 2, 'hat', None, (36 if q % 2 else 22) if d >= 2 else (28 if q % 4 == 2 else 0))
        if d >= 2 and r % 2 == 1 and q % 4 == 3: put(r, 2, 'hat', None, 14)                        # 32nd ghosts
        if d >= 3 and q in (7, 15, 23) and r % 2 == 0: put(r, 3, 'tick', None, 30)
        if d >= 3 and r >= 44: put(r, 3, 'tick', None, 18 + (r - 44) * 8)                          # ratchet into the next bar
        if p.get('blip') and r % 3 == 0 and rng.random() < .55: put(r, 15, 'blip', 84 + int(rng.choice([0, 3, 7, 10, 12, 15])), p['blip'] - (r % 6) * 2)
    if p.get('bass'):
        for r, m in BASS[h]:
            put(2 * r, 4, 'growl', m - 24, p['bass']); put(2 * r, 5, 'sub', m - 24, p['bass'] - 6)
    if p.get('subonly'):
        for r, m in BASS[h]:
            put(2 * r, 5, 'sub', m - 24, p['subonly'])
    if p.get('lead'):
        for r, m in LEAD[h]:
            mm = m - 12 + p.get('oct', 0); put(2 * r, 6, 'lead', mm, p['lead'])
            if p.get('dbl'): put(2 * r, 7, 'lead', mm + 12, p['lead'] - 16)
            else: put(2 * r + 3, 7, 'lead', mm, p['lead'] * .4)                                   # echo, a dotted 32nd later
    if p.get('stab'):
        for r0, ch in CH[h]:
            for rr in (r0, r0 + 6) if p['stab'] > 0 else ():
                for j, m in enumerate(ch[:3]): put(2 * rr + 1, 9 + j, 'stab', m, p['stab'] - j * 4)
    if p.get('pad'):
        for r0, ch in CH[h]:
            put(2 * r0, 12, 'pad', ch[0], p['pad'])
    if p.get('arp'):
        for r in range(0, ROWS, 2):
            ch = [c for r0, c in CH[h] if r0 * 2 <= r][-1]
            put(r, 8, 'glass', ch[(r // 2) % len(ch)] + 12, p['arp'] - (0 if r % 8 == 0 else 10))
    if p.get('zap'):
        for r in (22, 46): put(r, 13, 'zap', None, p['zap'])
    if p.get('stutter'):                                              # the last half beat: the lead note repeated every 32nd
        if LEAD[h]:
            m = LEAD[h][-1][1] - 12
            for r in range(40, 48): put(r, 6, 'lead', m, p['stutter'] - (r - 40) * 4); 
    for (r, key, vol) in p.get('fx', ()): put(r, 14, key, None, vol)
    return P

# ---------------------------------------------------------------- arrangement: 46 patterns x 2 s, about 1:35
S = []
def sec(n, **kw):
    for i in range(n): S.append(dict(kw, h='AB'[i % 2]))
sec(2, pad=40, blip=30)                                                               # intro: vowel pad and data
sec(2, pad=40, blip=34, subonly=50, drums=1, fx=((0, 'laser', 44),))
sec(4, pad=36, blip=30, bass=46, drums=1, arp=34); S[-4]['fx'] = ((0, 'riser', 46),)   # build
S[-1]['stutter'] = 44
sec(8, bass=54, drums=2, lead=54, stab=40, zap=40); S[-8]['fx'] = ((0, 'crash', 52),)  # drop 1
sec(4, pad=44, arp=44, blip=28, subonly=44, lead=40, dbl=1)                           # break: glass arps over the pad
S[-4]['fx'] = ((0, 'laser', 50),); S[-2]['fx'] = ((0, 'riser', 50),); S[-1]['stutter'] = 50
sec(8, bass=56, drums=3, lead=56, dbl=1, stab=42, zap=44, arp=26); S[-8]['fx'] = ((0, 'crash', 56),)   # drop 2: octave lead, glitch perc
sec(4, pad=40, bass=44, drums=1, arp=40, blip=34, stab=30)                            # bridge
S[-1]['stutter'] = 50; S[-2]['fx'] = ((0, 'riser', 50),)
sec(8, bass=58, drums=3, lead=58, dbl=1, oct=12, stab=44, zap=46, arp=30, blip=24); S[-8]['fx'] = ((0, 'crash', 58),)   # final drop, lead up an octave
sec(2, pad=40, arp=36, subonly=44, drums=1, blip=26)                                  # outro
sec(2, pad=34, blip=22, fx=((0, 'laser', 40),))
S[-1]['fx'] = ((0, 'laser', 40),); S[-2]['fx'] = ()

def main():
    F.I.clear(); F.I.update(I)                                         # the shared XM writer reads its instrument table
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Emergency Hi-Tech'[:20].ljust(20) + b'\x1a' + b'make_emergency_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
