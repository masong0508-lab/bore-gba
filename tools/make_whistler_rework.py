#!/usr/bin/env python3
"""Builds tools/whistler_shuffle.xm : "Whistler Man", a steely, jazz-rock rework of "The Dipper Man - Whistler Man"
(tools/the_dipper_man_whistler_man.xm) on a half-time shuffle (the Purdie shuffle).   usage: python3 tools/make_whistler_rework.py

From the original (read out of the XM): the bass walk G | C | F | D, the horn dyads (Bb/G Bb/G A/F | F/C ... G/Bb), the arp
(G D G D F C F C), the whistled tune (D G Bb G G G C C D Bb) and the trill lick (G A G  C D C  F G F  G A G). Its straight 16ths are
swung into shuffle triplets: every eighth is three rows, its second 16th lands on the third row.
New: the groove (the PURDIE SHUFFLE: hats on the 1st and 3rd triplet of every beat, the snare on 3 in half time, ghost notes on the
middle triplets, kick on 1 and the pickups, triplet fills), the harmony (Gm9 | C9 | Fadd9 | D7#9, Rhodes voicings), a clav, a horn
section that swells, a breathy whistle with an echo, jazz-guitar fills, all 16 voices. About 5:36.
Channels: 0 kick | 1 snare | 2 ghost snare | 3 hat | 4 open hat / tambourine | 5 bass | 6 7 8 rhodes | 9 clav | 10 11 horns
          12 whistle | 13 whistle echo / harmony | 14 guitar | 15 ride / crash.     6 rows a beat (triplet 16ths), 48-row patterns = 2 bars.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "whistler_shuffle.xm")
SPEED, BPM, ROWS, NCH = 3, 75, 48, 16          # 2.5*3/75 = 0.1 s a row, 6 rows a beat = 100 BPM
rng = np.random.default_rng(1977)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)

# ---------------------------------------------------------------- sounds
n = int(.3 * SR); t = tt(n)
add('kick', 'round kick', finish(np.sin(2 * np.pi * np.cumsum(55 + 70 * np.exp(-t / .025)) / SR) * np.exp(-t / .14) * np.minimum(1, t / .002), .95))
n = int(.28 * SR); t = tt(n)
x = bp(noise(n), 1500, 6500) * np.exp(-t / .07) + (np.sin(2 * np.pi * 185 * t) + .5 * np.sin(2 * np.pi * 330 * t)) * np.exp(-t / .045) * .8
add('snare', 'fat snare', finish(np.tanh(1.4 * x), .9))
n = int(.09 * SR); t = tt(n)
add('ghost', 'ghost snare', finish(bp(noise(n), 2000, 7000) * np.exp(-t / .025) + np.sin(2 * np.pi * 200 * t) * np.exp(-t / .02) * .3, .5))
n = int(.06 * SR); add('hat', 'tight hat', finish(hp(noise(n) * np.exp(-tt(n) / .014), 7500, 4), .36))
n = int(.38 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .12), 6500, 4), .4))
n = int(.18 * SR); t = tt(n)
add('tamb', 'tambourine', finish(hp(noise(n), 6000, 3) * (np.exp(-t / .03) + .5 * np.exp(-((t - .05) ** 2) / .0004)), .32))
n = int(1.1 * SR); t = tt(n); m = square(3900, n) * square(5700, n)
add('ride', 'ride', finish(hp(m * .45 + noise(n) * .55, 4500, 2) * np.exp(-t / .4), .32))
n = int(1.1 * SR); add('crash', 'crash', finish(hp(noise(n), 4000, 3) * np.exp(-tt(n) / .45), .48))
n = int(.6 * SR); t = tt(n); f = mid2f(43)                                         # fingered bass (G2): round, a little thump
b = np.sin(2 * np.pi * f * t) + .3 * np.sin(4 * np.pi * f * t) + lp(saw(f, n), 900) * .35 * np.exp(-t / .05)
add('bass', 'finger bass', finish(np.tanh(1.3 * b) * np.minimum(1, t / .003) * np.exp(-t / .45), .92), 43)
n = int(1.6 * SR); t = tt(n); f = mid2f(60)                                        # Rhodes (C4): tine bell + body, slow tremolo
x = np.sin(2 * np.pi * f * t + .9 * np.exp(-t / .4) * np.sin(2 * np.pi * f * t)) + .5 * np.sin(2 * np.pi * f * 7.02 * t) * np.exp(-t / .03)
x *= np.exp(-t / .9) * (1 + .18 * np.sin(2 * np.pi * 4.5 * t))
add('rhodes', 'rhodes', finish(x, .62), 60)
n = int(.25 * SR); t = tt(n); f = mid2f(60)                                        # clav (C4): a bright pulse through a closing filter
x = square(f, n) * .5 + saw(f, n) * .5; x = bp(x, 900, 4500) * np.exp(-t / .07) + lp(x, 600) * np.exp(-t / .1) * .4
add('clav', 'clav', finish(x * np.minimum(1, t / .001), .6), 60)
n = int(.55 * SR2); t = tt(n, SR2); f = mid2f(72)                                  # horn section (C5): saws that swell open ("bwah")
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-7, 0, 7)) / 3
env = np.minimum(1, t / .04) * np.exp(-t / .5); cut = 700 + 2600 * np.minimum(1, t / .08)
seg = [lp(x[i:i + 256], cut[min(i + 128, n - 1)], 2, SR2) for i in range(0, n, 256)]
add('horn', 'horn section', finish(np.concatenate(seg)[:n] * env, .7), 72, rel=5)
n = int(.8 * SR2); t = tt(n, SR2); f = mid2f(84)                                   # the whistle (C6): pure, breathy, vibrato after a moment
vib = f * (1 + .008 * np.sin(2 * np.pi * 5.6 * t) * np.clip((t - .12) / .2, 0, 1)); w = 2 * np.pi * np.cumsum(vib) / SR2
x = np.sin(w) + .04 * np.sin(2 * w) + bp(noise(n), 2500, 4500, 2, SR2) * (.12 * np.exp(-t / .05) + .03)
add('whistle', 'whistle', finish(x * np.minimum(1, t / .025) * np.minimum(1, (t[-1] - t) / .2), .72), 84, rel=5)
f = mid2f(64); L = int(round(SR / f)); n = int(.7 * SR); y = np.zeros(n); y[:L] = noise(L)   # jazz guitar (E4): plucked string
for i in range(L, n): y[i] = .995 * .5 * (y[i - L] + (y[i - L - 1] if i - L - 1 >= 0 else 0))
add('guitar', 'jazz guitar', finish(np.tanh(1.6 * lp(y, 3500)) * np.exp(-tt(n) / .4), .66), 64)
KEYS = ['kick', 'snare', 'ghost', 'hat', 'ohat', 'tamb', 'ride', 'crash', 'bass', 'rhodes', 'clav', 'horn', 'whistle', 'guitar']
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the original's material, swung (16th k -> row 3*(k//2) + 2*(k%2))
src = xm.parse(os.path.join(HERE, "the_dipper_man_whistler_man.xm"))
def notes(pat, ch): return [(r, n + 11) for r, row in enumerate(src['pats'][pat]) for (n, i, v, e, ep) in [row[ch]] if 0 < n < 97]
def sw(k): return 3 * (k // 2) + 2 * (k % 2)
BASSL = notes(2, 2)                      # G C F D, one per half bar
HORN1, HORN2 = notes(1, 3), notes(1, 4)  # the dyads
ARP = notes(2, 1)                        # G D G D F C F C
TUNE = notes(3, 2)                       # the whistled tune
LICK = notes(5, 8)                       # the trill lick
CHORDS = [(0, (58, 62, 65, 69)), (12, (64, 70, 74)), (24, (57, 60, 67)), (36, (54, 60, 65))]   # Gm9 | C9 | Fadd9 | D7#9 (rows)
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    d = p.get('drums', 0)
    for r in range(ROWS):
        b, t3 = (r % 24) // 6, r % 6                                     # beat in the bar, row in the beat
        if d:
            if t3 in (0, 4): put(r, 3 if not p.get('rideon') else 15, 'hat' if not p.get('rideon') else 'ride', None, (34 if t3 == 0 else 26) if not p.get('rideon') else 22)   # the shuffle: 1st and 3rd triplet
            if t3 == 2 and d >= 2: put(r, 3, 'hat', None, 10)                                    # the soft middle one
            if r % 24 == 12: put(r, 1, 'snare', None, 56)                                         # backbeat on 3 (half time)
            if d >= 2 and t3 == 2 and r % 24 != 14: put(r, 2, 'ghost', None, 14 + (rng.random() < .3) * 6)   # ghost notes on the middle triplet
            if r % 24 in (0, 10, 16) or (d >= 3 and r % 24 == 22): put(r, 0, 'kick', None, 58 if r % 24 == 0 else 44)
            if d >= 3 and r % 24 == 20: put(r, 4, 'tamb', None, 22)
        if p.get('fill') and r >= 36 and t3 % 2 == 0: put(r, 1, 'snare', None, 20 + (r - 36) * 3)  # a triplet fill into the next section
    if p.get('ohat'): put(46, 4, 'ohat', None, 30)
    if p.get('bass'):
        for k, m in BASSL:
            r = sw(k); put(r, 5, 'bass', m - 12, p['bass'])
            put(r + 4, 5, 'bass', m - 12 + 12, p['bass'] - 18); put(r + 9, 5, 'bass', m - 12 + 7, p['bass'] - 12)   # octave and fifth pickups
    if p.get('rhodes'):
        for r0, ch in CHORDS:
            for off in (0, 7) if p.get('comp') else (0,):
                for j, m in enumerate(ch[:3]): put(r0 + off, 6 + j, 'rhodes', m, p['rhodes'] - j * 3 - (8 if off else 0))
    if p.get('clav'):
        for k, m in ARP: put(sw(k) + 1, 9, 'clav', m, p['clav'])
        for k, m in ARP: put(sw(k) + 3, 9, 'clav', m + 12, p['clav'] - 14)
    if p.get('horns'):
        for (k, m) in HORN1: put(sw(k), 10, 'horn', m, p['horns'])
        for (k, m) in HORN2: put(sw(k), 11, 'horn', m, p['horns'] - 6)
    if p.get('tune'):
        o = p.get('oct', 0)
        for k, m in TUNE:
            put(sw(k), 12, 'whistle', m + o, p['tune'])
            if p.get('harm'): put(sw(k), 13, 'whistle', m + o - (4 if (m % 12) in (2, 7, 10) else 3), p['tune'] - 12)   # a third under, Steely style
            else: put(sw(k) + 5, 13, 'whistle', m + o, p['tune'] * .38)
    if p.get('lick'):
        for k, m in LICK: put(sw(k), 14, 'guitar', m + p.get('lo', 0), p['lick'] - (k % 4) * 3)
    if p.get('hits'):                                                   # bridge: the D7#9 hit, horns and Rhodes together
        for r in (0, 9, 24, 33):
            for j, m in enumerate((54, 60, 65)): put(r, 6 + j, 'rhodes', m, p['hits'])
            put(r, 10, 'horn', 66, p['hits']); put(r, 11, 'horn', 72, p['hits'] - 4)
    if p.get('teaser'):
        for k, m in TUNE[:4]: put(sw(k) + 24, 12, 'whistle', m, p['teaser'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    return P

# ---------------------------------------------------------------- arrangement: 70 patterns x 4.8 s, about 5:36
S = []
def sec(n, **kw):
    for i in range(n): S.append(dict(kw))
sec(2, rhodes=40, teaser=46)                                                            # intro: Rhodes and a hint of the whistle
sec(2, rhodes=42, comp=1, bass=50, drums=1, teaser=48); S[-1]['fill'] = 1
sec(8, rhodes=40, comp=1, bass=56, drums=2, clav=40); S[-8]['fx'] = ((0, 'crash', 48),)   # the groove: the Purdie shuffle
S[-1]['fill'] = 1; S[-1]['ohat'] = 1
sec(8, rhodes=38, comp=1, bass=56, drums=3, clav=34, horns=52)                          # the horn hook
sec(8, rhodes=38, comp=1, bass=56, drums=3, clav=30, tune=54); S[-8]['fx'] = ((0, 'crash', 50),)   # the whistled tune
S[-1]['fill'] = 1
sec(8, rhodes=42, comp=1, bass=54, drums=2, rideon=1, lick=46, lo=12)                   # solo: guitar licks over the ride
sec(4, hits=50, drums=1, bass=50); S[-1]['fill'] = 1                                   # bridge: D7#9 hits
sec(8, rhodes=38, comp=1, bass=58, drums=3, clav=30, tune=54, oct=12, harm=1); S[-8]['fx'] = ((0, 'crash', 54),)   # tune again, up, harmonised
sec(8, rhodes=40, comp=1, bass=58, drums=3, horns=54, tune=50, lick=36); S[-1]['fill'] = 1   # shout chorus
sec(8, rhodes=38, comp=1, bass=52, drums=2, clav=34, horns=40)                          # outro vamp
sec(4, rhodes=34, comp=1, bass=46, drums=1, teaser=40)
sec(2, rhodes=36)

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Whistler Man'[:20].ljust(20) + b'\x1a' + b'make_whistler_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
