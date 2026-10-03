#!/usr/bin/env python3
"""Builds tools/gottcho_barracho_ii.xm : "Gottcho Barracho" (the rework), the sister song of "An Ode to the Spanish Flexicode".
The original (tools/gottcho_barracho.xm) stays in the game as a secret jukebox song (the title-screen code unlocks it).

usage:  python3 tools/make_barracho_rework.py        (run from the project root; needs numpy + scipy)

The notes come from the original: its bass line, its lead line and its piano harmony are read out of tools/gottcho_barracho.xm bar by
bar (quantised to 16ths; the lead is folded into one singing octave), so the tune, the B-major riff and the chord changes are all kept.
New: the orchestration, shared with the Flexicode song (same nylon, flute + ping-pong echo, marimba, vibes, glass bells, breathing
pads, the bell call - here B D# F# A C#, a tritone away from Flexicode's F Ab C Eb G), a soft 4/4 groove at 32nd-note resolution
(hat ratchets, flams, ghost notes), all 16 voices hand-panned.  Uses only one-shot samples, note + volume column.
Channels: 0 kick | 1 rim / brush | 2 hat | 3 shaker | 4 sub bass | 5 6 7 pads | 8 9 nylon | 10 flute | 11 flute echo
          12 marimba | 13 vibes | 14 bell | 15 fx.        80 BPM speed 3: 32-row patterns = one bar of 4/4 (2 rows a 16th).
Re-running gives byte-identical output.
"""
import os, sys, collections
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F          # the shared sound palette (instruments, synthesis helpers, the XM writer)
from make_flexicode_rework import I, tt, noise, lp, hp, saw, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "gottcho_barracho_ii.xm")
BPM, SPEED, ROWS = 80, 3, 32
ROW = 2.5 * SPEED / BPM; BEAT = 8 * ROW
rng = np.random.default_rng(80)
F.NCH = NCH = 16; F.BPM = BPM; F.SPEED = SPEED

# ---------------------------------------------------------------- sounds: Flexicode's, plus a pad that breathes over one 4/4 beat and a hat
n = int(BEAT * SR2); t = tt(n, SR2); f = mid2f(60)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-9, -3, 3, 9)) / 4 + .5 * np.sin(2 * np.pi * f / 2 * t)
x = lp(x, 1300, 2, SR2) * (.35 + .65 * np.minimum(1, t / (BEAT * .6)) ** 1.4)
I['pad'] = dict(name='breath pad 4', x=finish(x, .6, .05, SR2), gen=60, rel=5)
n = int(.05 * SR); I['hat'] = dict(name='soft hat', x=finish(hp(noise(n) * np.exp(-tt(n) / .012), 6500, 4), .3), gen=None, rel=12)
n = int(ROWS * ROW * SR2); t = tt(n, SR2); u = t / t[-1]
I['swell'] = dict(name='reverse swell', x=finish(lp(hp(noise(n), 2000, 2, SR2), 5000, 2, SR2) * u ** 3, .45), gen=None, rel=5)
KEYS = ['kick', 'rim', 'brush', 'shaker', 'hat', 'sub', 'pad', 'nylon', 'flute', 'marimba', 'vibes', 'bell', 'swell', 'drop']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
F.KEYS = KEYS; F.INST = INST

# ---------------------------------------------------------------- read the original, bar by bar (its patterns are one bar of 96 rows)
src = xm.parse(os.path.join(HERE, "gottcho_barracho.xm"))
G = collections.defaultdict(list)                 # (bar, 16th, inst) -> [(midi, vol)]
for b, o in enumerate(src['order']):
    for r, row in enumerate(src['pats'][o]):
        for c, (n_, i_, v, e, ep) in enumerate(row):
            if 0 < n_ < 96:
                k = int(round(r / 6))
                if k < 16: G[(b, k, i_)].append((n_ + 11, v - 16 if v >= 16 else 64))
def bassline(b):                                  # the loudest bass note on each 16th, kept in octave 1-2
    out = []
    for k in range(16):
        x = G.get((b, k, 1))
        if x:
            m = max(x, key=lambda a: a[1])[0]
            while m > 52: m -= 12
            while m < 35: m += 12
            out.append((k, m))
    return out
def leadline(b, prev=76):                         # the loudest lead note per 16th, folded to the nearest octave of the last note (E4..D6)
    out = []
    for k in range(16):
        x = [a for a in G.get((b, k, 3), ()) if a[0] < 96]
        if not x: continue
        m = max(x, key=lambda a: a[1])[0]; pc = m % 12
        c = min((pc + 12 * o for o in range(4, 8)), key=lambda q: abs(q - prev))
        while c > 86: c -= 12
        while c < 64: c += 12
        out.append((k, c)); prev = c
    return out
def harmony(b, half):                             # the piano's three strongest pitch classes in a half bar, voiced in G3..F#4
    w = collections.Counter()
    for k in range(half * 8, half * 8 + 8):
        for m, v in G.get((b, k, 2), ()): w[m % 12] += v
    pcs = [p for p, _ in w.most_common(3)]
    return sorted(55 + (p - 7) % 12 for p in pcs)

# ---------------------------------------------------------------- one bar
CALL = [(0, 71), (4, 75), (8, 78), (12, 81), (20, 85)]      # the bell call: B D# F# A C# (Flexicode's is F Ab C Eb G)
def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    sb = p.get('src')
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (F.xmnote(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    d = p.get('drums', 0)
    for r in range(ROWS):
        if d and r in ((0, 20) if d == 1 else (0, 12, 20)): put(r, 0, 'kick', None, 60 if r == 0 else 44)
        if d and r in (8, 24): put(r, 1, 'rim', None, 46)
        if d >= 2 and r in (7, 23): put(r, 1, 'rim', None, 14)                           # flams before the backbeat
        if d and r % 4 == 0: put(r, 2, 'hat', None, 30 if r % 8 == 0 else 22)
        if d >= 2 and r >= 28: put(r, 2, 'hat', None, 12 + (r - 28) * 5)                 # the 32nd-note ratchet into the next bar
        if d >= 2 and r % 2 == 0 and r % 4 == 2: put(r, 3, 'shaker', None, 18 if r % 8 == 6 else 10)
        if p.get('pad') and r % 8 == 0 and sb is not None:
            for j, m in enumerate(harmony(sb, r // 16)): put(r, 5 + j, 'pad', m, p['pad'] - 3 * j)
    if sb is None: sb = -1
    if p.get('bass'):
        for k, m in bassline(sb): put(2 * k, 4, 'sub', m, p['bass'] - (0 if k % 4 == 0 else 12))
    if p.get('ny') and sb >= 0:   # nylon: the piano's harmony as off-beat strums, two voices
        for r in (4, 10, 20, 26):
            h = harmony(sb, r // 16)
            if h: put(r, 8, 'nylon', h[-1], p['ny'] - (0 if r in (4, 20) else 8)); put(r + 1, 9, 'nylon', h[0], p['ny'] - 12)
    if p.get('mel'):
        mv = p['mel']; prev = p.get('prev', 76)
        for k, m in leadline(sb, prev):
            m += p.get('oct', 0)
            put(2 * k, 10, 'flute', m, mv - (0 if k % 2 == 0 else 6))
            if p.get('echo', 1): put(2 * k + 3, 11, 'flute', m, mv * .4)                 # ping-pong echo, a dotted 16th later
    if p.get('mar') and sb >= 0:   # marimba: the bass riff two octaves up, as an answering ostinato (16ths 1, 3, 5... between bass hits)
        bl = dict(bassline(sb))
        for k in range(1, 16, 2):
            m = bl.get(k - 1)
            if m: put(2 * k, 12, 'marimba', m + 24 + 7 * (k % 4 == 3), p['mar'] - (k % 4) * 3)
    if p.get('vib') and sb >= 0:   # vibes: the lead's strongest notes, sparse, an octave up
        for k, m in leadline(sb)[::3]: put(2 * k, 13, 'vibes', m, p['vib'])
    if p.get('call'):
        for r, m in CALL: put(r, 14, 'bell', m - 12 * p.get('callo', 0), p['call'] - r // 2)
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    for (r, vol) in p.get('roll', ()): put(r, 1, 'brush', None, vol)
    return P

# ---------------------------------------------------------------- arrangement: follows the original's form, about 3:00
S = []
ROLL = tuple((r, 8 + r) for r in range(16, 32, 2))
def sec(bars, **kw):
    for b in bars: S.append(dict(kw, src=b))
S.append(dict(src=27, pad=36, call=50)); S.append(dict(src=28, pad=38, ny=40, call=44, callo=1, fx=((0, 'swell', 40),)))   # intro: the call
sec([3], drums=1, bass=54, pad=38, ny=42, mel=50, fx=((0, 'drop', 44),))                         # verse 1: the riff and the tune
sec(range(4, 10), drums=1, bass=54, pad=38, ny=42, mel=50)
sec([10], drums=2, bass=54, pad=40, ny=44, mel=52, mar=0, roll=ROLL)                            # pre
sec(range(11, 15), drums=2, bass=56, pad=42, ny=44, mel=54, vib=36)                             # bridge
sec(range(15, 22), drums=2, bass=58, pad=40, ny=46, mel=54, mar=34)                             # verse 2: + marimba
sec([22], drums=2, bass=56, pad=42, ny=44, mel=54, roll=ROLL)
sec(range(23, 27), drums=2, bass=58, pad=44, ny=46, mel=56, vib=38, mar=30)
sec(range(27, 32), pad=42, ny=40, vib=44, mel=46, echo=1, fx=((0, 'drop', 48),))                # interlude: the piano section, drums out
S[-1]['fx'] = ((0, 'swell', 46),); S[-1]['roll'] = ROLL
sec(range(34, 42), drums=2, bass=60, mar=38, pad=34)                                            # break: the bass riff with marimba
S[-1]['roll'] = ROLL
sec([51], drums=2, bass=58, pad=44, ny=46, mel=56, mar=34, vib=34, fx=((0, 'drop', 52),))       # last chorus (the original's 51-66)
sec(range(52, 67), drums=2, bass=58, pad=44, ny=46, mel=56, mar=34, vib=34)
for p in S[-8:-4]: p['oct'] = 12                                                                  # flute lifts an octave for four bars
sec(range(67, 71), pad=40, ny=40, mel=46, vib=38)                                                # outro
sec([71], pad=36, ny=36, call=40, callo=1)
sec([72], pad=32, call=34)

def main():
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    import struct
    hdr = b'Extended Module: ' + b'Gottcho Barracho II'[:20].ljust(20) + b'\x1a' + b'make_barracho_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
