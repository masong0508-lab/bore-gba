#!/usr/bin/env python3
"""Builds tools/youre_winner.xm : "You're Winner (You Rule)" by Dipper, transcribed from the song's stems.
The notes live in tools/youre_winner_notes.json (drums, bass, sax, the vocal melody played on a soft lead, pads), on a grid of
6 steps a beat (the song swings in triplets), 24 steps a bar, at the song's own 131 BPM. The bass keeps the record's tuning (45 cents flat).
Channels: 0 kick | 1 snare | 2 hat | 3 crash | 4 bass | 5 sax | 6 lead | 7-10 pads.   speed 3 / BPM 98 = one row a step.
usage:  python3 tools/make_winner.py      then  python3 tools/xm2gba.py"""
import os, sys, struct, json
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, finish, mid2f, SR, SR2
HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "youre_winner.xm")
BPM, SPEED, ROWS, NCH = 98, 3, 24, 11
rng = np.random.default_rng(131)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
# ---- sounds
n = int(.32 * SR); t = tt(n); ph = 2 * np.pi * np.cumsum(45 + 110 * np.exp(-t / .03)) / SR
add('kick', 'kick', finish(np.tanh(1.6 * np.sin(ph) * np.exp(-t / .16)), .95))
n = int(.24 * SR); t = tt(n)
add('snare', 'snare', finish(np.tanh(2 * (.5 * np.sin(2 * np.pi * 200 * t) * np.exp(-t / .04) + lp(bp(noise(n), 1200, 7500), 6000) * np.exp(-t / .08))), .85))
n = int(.07 * SR); add('hat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .018), 7000, 4), .4))
n = int(1.6 * SR); add('crash', 'crash', finish(hp(noise(n), 4000, 2) * np.exp(-tt(n) / .55), .45))
n = int(2.2 * SR); t = tt(n); f = mid2f(36 - 0.45)                    # reese bass, 45 cents flat like the record
x = (saw(f * 2 ** (-.1 / 12), n) + saw(f * 2 ** (.1 / 12), n, ph=1.3)) * .5 + .9 * np.sin(2 * np.pi * f * t)
add('bass', 'reese bass', finish(np.tanh(1.5 * lp(x, 700, 2)) * np.exp(-t / 2.5), .8), gen=36)
n = int(2.2 * SR); t = tt(n); f = mid2f(73); vib = 1 + .006 * np.sin(2 * np.pi * 5.2 * t) * np.minimum(1, t / .3)
x = saw(f, n, vib=vib) ; x = bp(x, 500, 3200) + .04 * bp(noise(n), 1500, 5000)
add('sax', 'sax', finish(np.tanh(1.8 * x) * np.minimum(1, t / .03) * np.exp(-t / 3), .7), gen=73)
n = int(2.4 * SR2); t = tt(n, SR2); f = mid2f(62); vib = 1 + .008 * np.sin(2 * np.pi * 5 * t) * np.minimum(1, t / .4)
pha = 2 * np.pi * np.cumsum(f * vib) / SR2
x = sum(a * np.sin(h * pha) for h, a in ((1, 1), (2, .45), (3, .25), (4, .08)))
add('lead', 'soft lead', finish(x * np.minimum(1, t / .05) * np.exp(-t / 4), .6, .05, SR2), gen=62, rel=5)
n = int(3.6 * SR2); t = tt(n, SR2); f = mid2f(60)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-9, -3, 4, 10)) / 4
add('pad', 'pad', finish(lp(x, 1400, 2, SR2) * np.minimum(1, t / .25), .45, .1, SR2), gen=60, rel=5)
KEYS = ['kick', 'snare', 'hat', 'crash', 'bass', 'sax', 'lead', 'pad']; INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return max(1, min(96, 49 + (m - g))) if g else 49
# ---- notes
D = json.load(open(os.path.join(HERE, 'youre_winner_notes.json')))
total = max([a for k in ('bass', 'sax', 'voice', 'pads') for a, z, p, v in D[k]] + [s for k in D['drums'] for s, v in D['drums'][k]]) + 48
nbars = total // ROWS + 1
G = [[(0, 0, 0)] * NCH for _ in range(nbars * ROWS)]
def put(r, ch, n, key, vol):
    if 0 <= r < len(G): G[r][ch] = (n, INST[key], 0x10 + max(0, min(64, int(vol))))
for ch, (k, key, lo) in enumerate((('kick', 'kick', 40), ('snare', 'snare', 30), ('hat', 'hat', 24), ('crash', 'crash', 30))):
    for s, v in D['drums'][k]: put(s, ch, 49, key, lo + (64 - lo) * v)
def mono(ev, ch, key, lo, hi):
    ev = sorted(ev)
    for i, (a, z, p, v) in enumerate(ev):
        put(a, ch, xmn(p, key), key, lo + (hi - lo) * v)
        nxt = ev[i + 1][0] if i + 1 < len(ev) else 1 << 30
        if z < nxt: put(z, ch, xmn(p, key), key, 0)          # a silent note ends it
mono(D['bass'], 4, 'bass', 44, 62); mono(D['sax'], 5, 'sax', 34, 54); mono(D['voice'], 6, 'lead', 36, 56)
busy = [0] * 4
for a, z, p, v in sorted(D['pads']):                         # four pad voices, each note to a free one
    c = next((i for i in range(4) if busy[i] <= a), min(range(4), key=lambda i: busy[i]))
    put(a, 7 + c, xmn(p, 'pad'), 'pad', 24 + 16 * v); busy[c] = z
    put(z, 7 + c, xmn(p, 'pad'), 'pad', 0)
# ---- XM
def pat_bytes(P):
    b = bytearray()
    for row in P:
        for (n, i, v) in row: b += bytes([n, i, v, 0, 0])
    return struct.pack('<IBHH', 9, 0, len(P), len(b)) + bytes(b)
def inst_bytes(k):
    d = I[k]; q = np.clip(np.round(d['x'] * 127), -127, 127).astype(int)
    dl = bytes((int(v) & 255) for v in np.diff(np.concatenate([[0], q])))
    h = struct.pack('<I', 263) + d['name'].encode('latin1')[:22].ljust(22, b'\0') + b'\0' + struct.pack('<HI', 1, 40) + bytes(96) + bytes(48) + bytes(48)
    h += bytes(14) + struct.pack('<H', 0) + bytes(263 - len(h) - 14 - 2)
    sh = struct.pack('<IIIBbBBbB', len(q), 0, 0, 64, 0, 0, 128, d['rel'], 0) + b'\0' * 22
    return h + sh + dl
pats, order, seen = [], [], {}
for b in range(nbars):
    P = G[b * ROWS:(b + 1) * ROWS]; key = pat_bytes(P)
    if key not in seen: seen[key] = len(pats); pats.append(P)
    order.append(seen[key])
assert len(pats) <= 256 and len(order) <= 256, (len(pats), len(order))
hdr = b'Extended Module: ' + b"You're Winner"[:20].ljust(20) + b'\x1a' + b'make_winner'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
open(OUT, 'wb').write(hdr + b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(k) for k in KEYS))
s = nbars * ROWS * SPEED * 2.5 / BPM
print("wrote %s: %d bars, %d patterns, %d:%02d" % (OUT, nbars, len(pats), s // 60, s % 60))
