#!/usr/bin/env python3
"""Builds tools/aim_and_shoot.xm : "Aim and Shoot", a full arrangement of the Caustic sketch "Aimandshoot 430 2021 V11"
(tools/aimandshoot_v11.caustic: 81 BPM, a beatbox, a sampled ostinato and an FM synth, 53 seconds).
usage: python3 tools/make_aimandshoot_rework.py

The notes are read out of the .caustic file (read_caustic, as in make_hotdamn_rework.py): the ostinato (a pulsing tonic with its
half-step neighbours, moved up a semitone so that it sits on the FM part's C#), the FM melody (G# B C# | D ... B C D C C#), its
descending progression (C# B A G# | F# G# A B | F# G# A F# C# | E B E A D G#, with the Neapolitan D turning back to C#), its second
line (F# A B C C# D) and the beatbox pattern (played on congas).
The colour is Damascus without the postcard: the mode (C# Hijaz) is already in the sketch, so it is left to speak. A small takht
(oud, qanun, ney and a string section that plays the tune in unison and octaves, heterophony: the oud and qanun ornament the same
line a 64th ahead, with tremolo on the long notes) sits on a modern half-time groove; the darbuka keeps to a quiet maqsum under
the kick and snare. No finger cymbals, no snake-charmer runs.
64th-note rows at 81 BPM (speed 2, XM BPM 108 = 16 rows a beat), 128-row patterns = 2 bars, about 2:50.
Channels: 0 kick / dum | 1 snare | 2 darbuka | 3 riq | 4 congas | 5 bass | 6 oud | 7 oud / qanun tremolo | 8 9 10 strings
          11 ney | 12 strings melody | 13 qanun | 14 glockenspiel / electone | 15 swell / cymbal.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "aimandshoot_v11.caustic")
OUT = os.path.join(HERE, "aim_and_shoot.xm")
SPEED, BPM, ROWS, NCH, RB = 2, 108, 128, 16, 16     # 2*2.5/108 s a row, 16 rows a beat = 81 BPM
rng = np.random.default_rng(1516)

# ---------------------------------------------------------------- the .caustic reader (see make_hotdamn_rework.py for the layout)
def read_caustic(path):
    d = open(path, 'rb').read(); out = []; o = 0
    while True:
        o = d.find(b'SPAT', o)
        if o < 0: return out
        lens = struct.unpack('<64i', d[o + 8:o + 264]); cap = struct.unpack('<64i', d[o + 264:o + 520]); p = o + 520; pats = {}
        for i in range(64):
            notes = set()
            for k in range(cap[i]):
                r = d[p + 56 * k:p + 56 * k + 56]; ii = struct.unpack('<14i', r); f = struct.unpack('<14f', r)
                if ii[1] == 0 and 0 < ii[6] < 128 and f[2] < 1000 and f[3] > 0: notes.add((round(f[2] * 16), max(1, round(f[3] * 16)), ii[6], round(f[13], 2)))
            p += 56 * cap[i]
            if notes: pats[i] = (lens[i], sorted(notes))
        out.append(pats); o += 4
M = read_caustic(SRC)                          # machines: 0 beatbox, 1 the ostinato (PCM), 2 3 empty PCM, 4 the FM synth
BBOX, OST, FM = M[0], M[1], M[4]
def seg(P, pat, off=0, n=ROWS, tr=0): return [(s - off, l, m + tr, v) for (s, l, m, v) in P[pat][1] if off <= s < off + n]

# ---------------------------------------------------------------- sounds
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
def ks(f, dur, sr, damp, bright, seed):
    """Karplus-Strong pluck."""
    g = np.random.default_rng(seed); L = int(round(sr / f)); n = int(dur * sr); y = np.zeros(n)
    y[:L] = lp(g.uniform(-1, 1, L), bright, 2, sr)
    for i in range(L, n): y[i] = damp * .5 * (y[i - L] + y[i - L - 1])
    return y
n = int(.4 * SR); t = tt(n)                                                        # kick: round, a little long (half-time)
add('kick', 'kick', finish(np.tanh(1.5 * np.sin(2 * np.pi * np.cumsum(46 + 90 * np.exp(-t / .03)) / SR)) * np.exp(-t / .2)
                           + hp(noise(n), 2500) * np.exp(-t / .003) * .35, .95))
n = int(.3 * SR); t = tt(n)                                                        # snare: dusty, warm
x = bp(noise(n), 800, 6000) * np.exp(-t / .11) + (np.sin(2 * np.pi * 175 * t) + .5 * np.sin(2 * np.pi * 280 * t)) * np.exp(-t / .05)
add('snare', 'snare', finish(lp(np.tanh(1.6 * x), 7000), .9))
n = int(.35 * SR); t = tt(n)                                                        # dum: the darbuka's low stroke
add('dum', 'dum', finish(np.sin(2 * np.pi * np.cumsum(95 + 40 * np.exp(-t / .02)) / SR) * np.exp(-t / .18) + lp(noise(n), 900) * np.exp(-t / .01) * .3, .85))
n = int(.12 * SR); t = tt(n)                                                        # tek: the rim, bright and short
add('tek', 'tek', finish(bp(noise(n), 2500, 8000) * np.exp(-t / .012) + np.sin(2 * np.pi * 760 * t) * np.exp(-t / .03) * .7, .62))
n = int(.18 * SR); t = tt(n); m_ = square(4300, n) * square(6100, n) * square(7900, n)   # riq: jingles
add('riq', 'riq', finish(hp(m_ * .6 + noise(n) * .4, 5000, 2) * (np.exp(-t / .03) + .3 * np.exp(-t / .09)), .34))
for nm_, f0, d0 in (('congah', 330, .11), ('congal', 210, .16)):
    n = int(.3 * SR); t = tt(n)
    add(nm_, nm_, finish(np.sin(2 * np.pi * np.cumsum(f0 * (1 + .25 * np.exp(-t / .01))) / SR) * np.exp(-t / d0) + bp(noise(n), 1500, 5000) * np.exp(-t / .006) * .4, .7))
n = int(1.2 * SR2); t = tt(n, SR2)                                                  # a soft air swell (reverse cymbal)
add('swell', 'air swell', finish(hp(noise(n), 3000, 2, SR2) * (t / t[-1]) ** 2.5, .42), None, 5)
n = int(1.0 * SR2); add('cym', 'cymbal', finish(hp(noise(n), 3500, 3, SR2) * np.exp(-tt(n, SR2) / .4), .42), None, 5)
n = int(.8 * SR2); t = tt(n, SR2); f = mid2f(37)                                   # bass (C#2): round, a little woody
b = np.sin(2 * np.pi * f * t) + .25 * np.sin(4 * np.pi * f * t) + .1 * np.sin(6 * np.pi * f * t)
add('bass', 'bass', finish(np.tanh(1.2 * b) * np.minimum(1, t / .004) * np.exp(-t / .5), .92), 37, 5)
f = mid2f(49); y = ks(f, .9, SR, .996, 3500, 7)                                     # oud (C#3): fretless pluck, a nasal bridge
y = y + .5 * bp(y, 700, 1600); y = y + hp(noise(len(y)), 3000) * np.exp(-tt(len(y)) / .004) * .25
add('oud', 'oud', finish(np.tanh(1.3 * y) * np.exp(-tt(len(y)) / .7), .66), 49)
f = mid2f(61); y = ks(f, .7, SR, .997, 7000, 11)                                    # qanun (C#4): bright, many strings ringing
y = y + .3 * ks(f * 2.003, .7, SR, .996, 7000, 12)
add('qanun', 'qanun', finish(y * np.exp(-tt(len(y)) / .5), .6), 61)
n = int(1.2 * SR2); t = tt(n, SR2); f = mid2f(73)                                  # ney (C#5): breath, a scoop into the note, late vibrato
fr = f * 2 ** ((-.3 * np.exp(-t / .06) + .12 * np.sin(2 * np.pi * 5 * t) * np.clip((t - .25) / .3, 0, 1)) / 12); w = 2 * np.pi * np.cumsum(fr) / SR2
x = np.sin(w) + .12 * np.sin(2 * w) + .05 * np.sin(3 * w) + bp(noise(n), 1200, 4500, 2, SR2) * (.35 * np.exp(-t / .07) + .1)
add('ney', 'ney', finish(x * np.minimum(1, t / .07) * np.minimum(1, (t[-1] - t) / .3), .66), 73, 5)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(61)                                  # string section (C#4): soft bow, for chords
vib = f * (1 + .004 * np.sin(2 * np.pi * 5.3 * t))
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28), vib=vib * 2 ** (c / 1200)) for c in (-11, -5, 0, 6, 12)) / 5
add('strings', 'strings', finish(lp(x, 2600, 2, SR2) * np.minimum(1, t / .25) * np.minimum(1, (t[-1] - t) / .4), .55), 61, 5)
n = int(1.0 * SR2); t = tt(n, SR2); f = mid2f(61)                                  # strings in unison (C#4): the tune, faster bow, vibrato
vib = f * (1 + .007 * np.sin(2 * np.pi * 5.8 * t) * np.clip((t - .1) / .2, 0, 1))
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28), vib=vib * 2 ** (c / 1200)) for c in (-7, 0, 7)) / 3
add('violins', 'violins', finish(lp(x, 3400, 2, SR2) * np.minimum(1, t / .05) * np.minimum(1, (t[-1] - t) / .25), .6), 61, 5)
n = int(.9 * SR); t = tt(n); f = mid2f(84)                                          # glockenspiel (C6), as in the sketch
add('glock', 'glockenspiel', finish((np.sin(2 * np.pi * f * t) + .3 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t / .1)) * np.exp(-t / .45), .55), 84)
n = int(1.0 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # electone (C4): a soft home organ, as in the sketch
x = sum(a * np.sin(2 * np.pi * f * h * t * (1 + .003 * np.sin(2 * np.pi * 6 * t))) for h, a in ((1, 1), (2, .5), (3, .3), (4, .15)))
add('electone', 'electone', finish(x * np.minimum(1, t / .02) * np.minimum(1, (t[-1] - t) / .2), .5), 60, 5)
KEYS = ['kick', 'snare', 'dum', 'tek', 'riq', 'congah', 'congal', 'swell', 'cym', 'bass', 'oud', 'qanun', 'ney', 'strings', 'violins', 'glock', 'electone']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

# ---------------------------------------------------------------- the mode and the harmony
HIJAZ = (1, 2, 5, 6, 8, 9, 11)                 # C# D E# F# G# A B
def up_step(m):
    for k in range(1, 4):
        if (m + k) % 12 in HIJAZ: return m + k
    return m + 1
TRI = {1: (61, 65, 68), 11: (59, 63, 66), 9: (57, 61, 64), 8: (56, 60, 63), 6: (54, 57, 61), 4: (56, 59, 64), 2: (57, 62, 66)}   # C# B A G# F#m E D
def chords_of(notes):
    """A chord per bass note that lands on a beat (the passing notes between are left out)."""
    out = []
    for (s, l, m, v) in notes:
        if s % RB == 0 and (m % 12) in TRI and m < 50 and (not out or out[-1][1] != m % 12): out.append((s, m % 12))
    return out
# the material, in 8-beat units
OSTU = [seg(OST, 0, k * ROWS, tr=1) for k in range(4)]                          # the ostinato, on C#
MELA = [seg(FM, 0, 0), seg(FM, 0, ROWS)]                                         # the tune
PROG = [seg(FM, 0, 2 * ROWS), seg(FM, 0, 3 * ROWS), seg(FM, 2, 0), seg(FM, 2, ROWS)]   # the progression (and its variant with the run)
MELB = seg(FM, 1, 0, 2 * ROWS)                                                   # the second line (12 beats)
BEAT = seg(BBOX, 0, 0)

# ---------------------------------------------------------------- one 2-bar pattern
def build(p, ui):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    def put(r, ch, key, midi=None, vol=64, note=None):
        if 0 <= r < ROWS and vol > 0:
            P[r][ch] = (note if note else xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, int(vol))))
    d = p.get('drums', 0)
    # ---- drums: 1 the sketch's beat on congas, 2 + kick and snare (half-time), 3 + darbuka maqsum and riq, 4 + fills
    if d >= 1:
        for (s, l, m, v) in BEAT: put(s, 4, 'congal' if m == 55 else 'congah', None, 44 * (.35 + .65 * v))
    for bar in (0, 64):
        if d >= 2:
            for s, vv in ((0, 60), (10, 46), (11 if bar else 14, 40)): put(bar + 4 * s, 0, 'kick', None, vv)
            for s in (4, 12): put(bar + 4 * s, 1, 'snare', None, 54)
        if d >= 3:
            for s, k, vv in ((0, 'dum', 30), (2, 'tek', 24), (6, 'tek', 26), (8, 'dum', 28), (12, 'tek', 24)): put(bar + 4 * s, 2, k, None, vv)   # maqsum
            for s in (3, 5, 9, 13, 15): put(bar + 4 * s, 2, 'tek', None, 9, note=53)                                                             # ka
            for s in range(1, 16, 2): put(bar + 4 * s, 3, 'riq', None, 16 if s % 4 == 3 else 11)
            put(bar + 58, 3, 'riq', None, 10); put(bar + 60, 3, 'riq', None, 14); put(bar + 62, 3, 'riq', None, 18)
    if d >= 4 and p.get('fill', 1):
        for r in range(112, ROWS, 2): put(r, 2, 'tek', None, 14 + (r - 112) * 2, note=49 + (r - 112) // 4)
    if p.get('swell'): put(0, 15, 'swell', None, p['swell'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    # ---- the ostinato on the oud, the bass under it
    if 'ost' in p:
        for (s, l, m, v) in OSTU[p['ost']]:
            if p.get('oud'): put(s, 6, 'oud', m + 12, p['oud'] * (.6 + .4 * v))
            if p.get('qost'): put(s + 2, 7, 'qanun', m + 24, p['qost'])
            if p.get('bass') and (s % RB == 0 or m % 12 != 1): put(s, 5, 'bass', m, p['bass'] - (0 if s % 64 == 0 else 8))
        if p.get('pad'):
            for h in (0, 64):
                for j, m in enumerate(TRI[1]): put(h, 8 + j, 'strings', m - 12, p['pad'] - 3 * j)
    # ---- the progression: the bass from the sketch, string chords, the qanun spelling the chord out
    if 'prog' in p:
        notes = PROG[p['prog']]; ch = chords_of(notes)
        for (s, l, m, v) in notes:
            if m < 50 and p.get('bass'): put(s, 5, 'bass', m + (12 if m < 33 else 0), p['bass'] * (.7 + .3 * v))
            if m >= 50 and p.get('runs'): put(s, 13, 'qanun', m + 12, p['runs'])            # the sketch's high notes and its run
        for k, (s, pc) in enumerate(ch):
            tri = TRI[pc] if not (p.get('picardy') and k == len(ch) - 1) else TRI[1]
            if p.get('pad'):
                for j, m in enumerate(tri): put(s, 8 + j, 'strings', m - 12, p['pad'] - 3 * j)
            if p.get('qarp'):                                                                 # 16ths through the chord
                e = ch[k + 1][0] if k + 1 < len(ch) else ROWS
                for i, r in enumerate(range(s, e, 4)):
                    put(r, 13, 'qanun', (tri + (tri[0] + 12, tri[1] + 12))[(i * 2) % 5] + 12, p['qarp'] - (0 if i % 4 == 0 else 8))
            if p.get('elec'): put(s, 14, 'electone', tri[0], p['elec'])
    # ---- the tune: ney (or the violins) leads; the oud and qanun play it too, a 64th ahead, ornamented (heterophony)
    if 'mel' in p:
        notes = MELA[p['mel']] if p['mel'] < 2 else MELB if p['mel'] == 2 else seg(FM, 1, ROWS, ROWS)
        o = p.get('oct', 24)
        for k, (s, l, m, v) in enumerate(notes):
            vol = (.65 + .35 * v)
            if p.get('ney'):
                put(s, 11, 'ney', m + o, p['ney'] * vol)
                if l >= 12 and k % 2 == 0:                                                    # a turn on a long note
                    put(s + 4, 11, 'ney', up_step(m + o), p['ney'] * vol * .8); put(s + 6, 11, 'ney', m + o, p['ney'] * vol * .9)
            if p.get('vln'): put(s, 12, 'violins', m + o - 12, p['vln'] * vol)
            if p.get('het'):
                put(s - 1, 6 if 'ost' not in p else 7, 'oud', m + o - 12, p['het'] * vol)
                if l >= 8:                                                                     # tremolo on the long notes
                    for r in range(s + 3, s + l, 3): put(r, 7, 'qanun', m + o, p['het'] * .5)
            if p.get('glk') and k % 2 == 0: put(s, 14, 'glock', m + 24, p['glk'] * vol)
    if p.get('final'):                                                                        # the end: C# major, a qanun sweep, everyone
        for j, m in enumerate((49, 53, 56)): put(0, 8 + j, 'strings', m, 44 - 3 * j)
        put(0, 5, 'bass', 37, 58); put(0, 0, 'dum', None, 50); put(0, 11, 'ney', 73, 44); put(0, 12, 'violins', 61, 40); put(0, 14, 'glock', 85, 34)
        sweep = [m for m in range(49, 86) if m % 12 in HIJAZ]
        for i, m in enumerate(sweep): put(1 + i, 13, 'qanun', m, 20 + i)
        for r in range(4, 96, 3): put(r, 7, 'oud', 61, 26 * (1 - r / 110))
        put(0, 15, 'cym', None, 36)
    return P

# ---------------------------------------------------------------- the arrangement: 8-beat units (5.93 s), about 2:50
S = []
def u(**kw): S.append(kw)
u(ost=0, oud=34, drums=1, ney=0, swell=30)                                                     # intro: the ostinato alone, the congas
u(ost=1, oud=36, drums=1, bass=40, pad=26)
u(ost=2, oud=38, drums=1, bass=44, pad=28, mel=0, glk=26, swell=40)                            # the glockenspiel hints at the tune
u(ost=3, oud=40, drums=4, bass=48, pad=28, qost=18)
for k in range(4): u(ost=k, oud=42, drums=3, bass=52, qost=18 if k % 2 else 0)                 # the groove
S[-4]['fx'] = ((0, 'cym', 34),); S[-1]['drums'] = 4
for k in range(4): u(ost=k, oud=36, drums=3, bass=52, mel=k % 2, ney=50, het=28, pad=24)         # theme: the ney, the oud shadowing it
S[-4]['fx'] = ((0, 'cym', 30),); S[-1]['drums'] = 4
for k in range(4): u(prog=k, bass=56, drums=3, pad=36, qarp=30, runs=40, elec=22)                 # the progression
S[-4]['fx'] = ((0, 'cym', 36),)
S[-2].update(mel=2, ney=44); S[-1].update(mel=3, ney=44, drums=4)
u(ost=0, oud=38, drums=1, mel=2, het=34, pad=24, swell=0)                                  # break: the oud takes the second line
u(ost=1, oud=34, drums=1, mel=3, het=34, pad=26, swell=44)
for k in range(4): u(ost=k, oud=34, drums=3, bass=54, mel=k % 2, ney=48, vln=44, het=30, pad=28, glk=18 if k == 3 else 0)   # theme, in full
S[-4]['fx'] = ((0, 'cym', 40),); S[-1]['drums'] = 4
for k in range(4): u(prog=k, bass=56, drums=3, pad=38, qarp=28, runs=40, mel=(0, 1, 2, 3)[k], ney=44, vln=40, picardy=(k == 3))
S[-4]['fx'] = ((0, 'cym', 36),)
u(ost=0, oud=34, bass=46, drums=1, pad=26, mel=0, ney=36)                                         # outro
u(ost=3, oud=28, bass=40, pad=22, qost=14, swell=34)
u(final=1)

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for ui, sp in enumerate(S):
        P = build(sp, ui); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    hdr = b'Extended Module: ' + b'Aim and Shoot'[:20].ljust(20) + b'\x1a' + b'make_aimandshoot'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
