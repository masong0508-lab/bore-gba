#!/usr/bin/env python3
"""Builds tools/closer_to_the_end_old.xm : "Closer to the End" (the first, slower version; the secret CLOSER TO THE END (ORIGINAL)), a Danny Steele prog rework of "The Dipper Man - Closer to the End"
(tools/the_dipper_man_closer_to_the_end.xm).   usage: python3 tools/make_closer_rework.py   (needs numpy + scipy; byte-identical on re-run)

Old prog (Trespass / Foxtrot: 12-string arpeggios, mellotron strings and flute, Hammond, a bell chime, a 12/8 pastoral) that grows into
new prog (Dream Theater: 7/8 palm-muted riffs on double kick, unison runs, organ and guitar solos, a stadium finale).
KEPT from the original (read out of the XM): the bass roots bar by bar, and the tresillo lead Bb Ab Gb (3+3+2) with its B-natural turn.
DANNY STEELE MOTIFS, moved to C minor: from WHISTLER MAN the whistled tune (G C Eb C C C F F G Eb), the trill lick (C D C F G F Bb C Bb C D C),
the arp (C G C G Bb F Bb F) under the 12-string and the bass walk (C F Bb G); from NURSERY TIME the music-box chime (C C C C D C | Bb Bb Bb Bb C Bb
| Ab Ab Ab Ab Bb Ab | G G G C Eb F G), the tune (Bb Bb Bb C Bb | A D D | Ab Ab Ab Bb Ab | G C) and the run (C D Eb F G A Bb C).
Grid: 12 rows a beat (speed 2, 84 BPM): 4/4 = 48 rows, 7/8 = 42, 12/8 = 72.
Channels: 0 kick | 1 snare | 2 hat / ride | 3 toms / crash | 4 bass | 5 6 mellotron strings | 7 8 12-string | 9 piano / bell | 10 11 guitars
          12 lead | 13 lead 2 | 14 organ | 15 fx.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__)); OUT = os.path.join(HERE, "closer_to_the_end_old.xm")
SPEED, BPM, NCH = 2, 84, 16
rng = np.random.default_rng(1971); I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
def ks(f, n, sr, damp=.996):
    L = int(round(sr / f)); y = np.zeros(n); y[:L] = noise(L)
    for i in range(L, n): y[i] = damp * .5 * (y[i - L] + y[i - L - 1])
    return y
# ---- drums
n = int(.3 * SR); t = tt(n); add('kick', 'prog kick', finish(np.sin(2 * np.pi * np.cumsum(50 + 90 * np.exp(-t / .02)) / SR) * np.exp(-t / .12) + .15 * noise(n) * np.exp(-t / .004), .95))
n = int(.3 * SR); t = tt(n); add('snare', 'big snare', finish(np.tanh(1.5 * (bp(noise(n), 1200, 7000) * np.exp(-t / .09) + np.sin(2 * np.pi * 190 * t) * np.exp(-t / .05))), .9))
n = int(.07 * SR); add('hat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .015), 7000, 4), .3))
n = int(.9 * SR); t = tt(n); add('ride', 'ride', finish(hp(square(3900, n) * square(5700, n) * .45 + noise(n) * .55, 4500, 2) * np.exp(-t / .35), .3))
n = int(1.2 * SR); add('crash', 'crash', finish(hp(noise(n), 3800, 3) * np.exp(-tt(n) / .5), .5))
n = int(.4 * SR); t = tt(n); add('tom', 'tom', finish(np.sin(2 * np.pi * np.cumsum(120 + 90 * np.exp(-t / .05)) / SR) * np.exp(-t / .16), .9), 50)
# ---- pitched
n = int(.7 * SR); t = tt(n); f = mid2f(40); b = np.sin(2 * np.pi * f * t) + .4 * np.sin(4 * np.pi * f * t) + lp(saw(f, n), 1400) * .5
add('bass', 'rick bass', finish(np.tanh(1.8 * b) * np.minimum(1, t / .003) * np.exp(-t / .5), .9), 40)
n = int(1.1 * SR); y = ks(mid2f(64), n, SR) + ks(mid2f(64) * 1.003, n, SR) * .8 + ks(mid2f(76), n, SR, .993) * .5
add('twelve', '12-string', finish(lp(y, 5000), .7), 64)
n = int(1.7 * SR2); t = tt(n, SR2); f = mid2f(60); u = np.minimum(1, t / .35)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-14, -5, 5, 14)) / 4 * (1 + .1 * np.sin(2 * np.pi * 5.5 * t))
add('strings', 'mellotron strings', finish(lp(x, 1500, 2, SR2) * u * np.exp(-t / 1.6), .6, .08, SR2), 60, 5)
n = int(1.0 * SR2); t = tt(n, SR2); f = mid2f(72); w = 2 * np.pi * np.cumsum(f * (1 + .006 * np.sin(2 * np.pi * 5.2 * t) * np.clip((t - .1) / .2, 0, 1))) / SR2
x = np.sin(w) + .25 * np.sin(2 * w) + .08 * np.sin(3 * w) + lp(noise(n), 3000, 2, SR2) * .05
add('flute', 'mellotron flute', finish(x * np.minimum(1, t / .05) * np.exp(-t / .9), .7, .1, SR2), 72, 5)
n = int(1.2 * SR2); t = tt(n, SR2); f = mid2f(60)
x = sum(np.sin(2 * np.pi * f * m * t) * a for m, a in ((.5, 1), (1, 1), (1.5, .6), (2, .8), (3, .5), (4, .4), (6, .2))) * (1 + .22 * np.sin(2 * np.pi * 6.5 * t))
add('organ', 'hammond', finish(x * np.minimum(1, t / .01) * np.exp(-t / 1.2), .6, .08, SR2), 60, 5)
n = int(.3 * SR); t = tt(n); y = ks(mid2f(40), n, SR, .985); add('chug', 'palm chug', finish(np.tanh(5 * lp(y, 2500)) * np.exp(-t / .16), .85), 40)
n = int(1.0 * SR2); t = tt(n, SR2); f = mid2f(69); w = 2 * np.pi * np.cumsum(f * (1 + .006 * np.sin(2 * np.pi * 5.5 * t) * np.clip((t - .15) / .2, 0, 1))) / SR2
add('glead', 'dist lead', finish(np.tanh(3.5 * (np.sin(w) + .5 * np.sin(2 * w) + .3 * np.sin(3 * w))) * np.minimum(1, t / .01) * np.exp(-t / .9), .75, .08, SR2), 69, 5)
n = int(.7 * SR); t = tt(n); f = mid2f(69); x = saw(f * 1.004, n) + saw(f * .996, n) + square(f / 2, n) * .5
add('synth', 'moog lead', finish(lp(x, 2400, 2) * np.minimum(1, t / .01) * np.exp(-t / .6), .7), 69)
n = int(1.2 * SR); t = tt(n); f = mid2f(84); x = np.sin(2 * np.pi * f * t) + .35 * np.sin(2 * np.pi * f * 4 * t) * np.exp(-t / .2) + .15 * np.sin(2 * np.pi * f * 6.3 * t) * np.exp(-t / .1)
add('bell', 'glass bell', finish(x * np.exp(-t / .7), .6), 84)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(60); x = sum(np.sin(2 * np.pi * f * k * t) / k ** 1.2 * np.exp(-t * (1 + k * .6)) for k in range(1, 8))
add('piano', 'grand piano', finish(x, .7, .05, SR2), 60, 5)
KEYS = ['kick', 'snare', 'hat', 'ride', 'crash', 'tom', 'bass', 'twelve', 'strings', 'flute', 'organ', 'chug', 'glead', 'synth', 'bell', 'piano']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return max(1, min(96, 49 + (m - g))) if g else 49

# ---- the original's lead (tresillo Bb Ab Gb), bar by bar
src = xm.parse(os.path.join(HERE, "the_dipper_man_closer_to_the_end.xm"))
def LP(pos): return [[(r % 16 * 3, row[7][0] + 11) for r, row in enumerate(src['pats'][pos]) if row[7][0] and r // 16 == b] for b in (0, 1)]
LEAD = {p: LP(p) for p in range(6, 19)}
CH = {'Cm': (36, [60, 63, 67, 70, 74]), 'Dd': (38, [62, 65, 68, 72]), 'Db': (37, [61, 65, 68, 72, 75]), 'Eb': (39, [63, 67, 70, 74]),
      'Fm': (41, [65, 68, 72, 75, 79]), 'G7': (43, [67, 71, 74, 77, 80]), 'Ab': (44, [68, 72, 75, 79])}
CHIME = [[72, 72, 72, 72, 74, 72], [70, 70, 70, 70, 72, 70], [68, 68, 68, 68, 70, 68], [67, 67, 67, 72, 75, 77, 79]]
TUNE = [(0, 0, 67), (0, 12, 72), (0, 24, 75), (0, 36, 72), (1, 0, 72), (1, 12, 72), (1, 24, 77), (1, 36, 77), (2, 0, 79), (2, 24, 75)]   # Whistler tune
LICK = [67 + k for k in (-7, -5, -7, -2, 0, -2, 3, 5, 3, 5, 7, 5)]   # placeholder replaced below
LICK = [72, 74, 72, 77, 79, 77, 82, 84, 82, 84, 86, 84]               # Whistler trill lick: C D C F G F Bb C Bb C D C
NTUNE = [[70, 70, 70, 72, 70], [69, 74, 74], [68, 68, 68, 70, 68], [67, 72]]                  # Nursery tune
RUN = [72, 74, 75, 77, 79, 81, 82, 84]                                                       # Nursery run
ARP = [0, 4, 0, 4, 3, 2, 3, 2]                                                               # Whistler arp C G C G Bb F Bb F as chord-tone picks (see build)
WALK = [36, 41, 34, 43]

def build(p):
    L = p['L']; P = [[(0, 0, 0)] * NCH for _ in range(L)]; c = p['ch']; bs, tones = CH[c]
    def put(r, ch, key, m=None, v=64):
        if 0 <= r < L: P[r][ch] = (xmn(m, key) if m is not None else 49, INST[key], 0x10 + max(0, min(64, int(v))))
    d = p.get('drums')
    if d:
        for r in range(L):
            if d == 'soft':
                if r % 12 == 0: put(r, 2, 'ride', None, 24)
                if r % 12 == 6: put(r, 2, 'hat', None, 14)
                if r == 0: put(r, 0, 'kick', None, 40)
                if r == L // 2: put(r, 1, 'snare', None, 22)
            else:
                if r % 6 == 0: put(r, 2, 'hat' if d != 'ride' else 'ride', None, 30 if r % 12 == 0 else 20)
                if d == 'metal':
                    if r % 3 == 0 and r < L - 6: put(r, 0, 'kick', None, 44 if r % 6 == 0 else 34)
                    if r == 12 or r == 36 or (L == 42 and r == 30): put(r, 1, 'snare', None, 58)
                else:
                    if r == 0 or (L == 48 and r == 30) or (L == 72 and r == 36): put(r, 0, 'kick', None, 52)
                    if (L == 48 and r in (12, 36)) or (L == 72 and r == 36) or (L == 42 and r in (12, 30)): put(r, 1, 'snare', None, 54)
        if p.get('crash'): put(0, 3, 'crash', None, 50)
        if p.get('fill'):
            for k, r in enumerate(range(L - 12, L, 3)): put(r, 1 if k % 2 else 3, 'snare' if k % 2 else 'tom', 45 - 3 * k if k % 2 == 0 else None, 30 + k * 8)
    b = p.get('bass')
    if b == 'sus': put(0, 4, 'bass', bs, 52); put(L // 2, 4, 'bass', bs + 7, 40)
    elif b == 'eighth':
        for r in range(0, L, 6): put(r, 4, 'bass', bs + (12 if r % 12 == 6 else 0), 54 if r % 12 == 0 else 38)
    elif b == 'walk':
        for k, m in enumerate(WALK): put(k * 12, 4, 'bass', m, 56)
    if p.get('str'):
        for r in (0, L // 2): put(r, 5, 'strings', tones[1], p['str']); put(r, 6, 'strings', tones[2], p['str'] - 4)
    if p.get('org'):
        for r in range(0, L, 24): put(r, 14, 'organ', tones[0] - 12, p['org']); put(r, 13, 'organ', tones[3], p['org'] - 8) if not p.get('lead') else None
    if p.get('arp'):
        for k, r in enumerate(range(0, L, 6)):
            m = tones[ARP[k % 8] % len(tones)] + (12 if k % 8 in (1, 3) else 0); put(r, 7, 'twelve', m, p['arp'] - (k % 2) * 6); put(r + 1, 8, 'twelve', m + 12, p['arp'] - 14)
    if p.get('chug'):
        pat = (0, 3, 6, 12, 15, 18, 24, 27, 30, 36) if L == 42 else (0, 3, 6, 12, 18, 21, 24, 30, 36, 42)
        for r in pat: put(r, 10, 'chug', bs + 12, p['chug'] - (r % 6 and 6)); put(r, 11, 'chug', bs + 19, p['chug'] - 10)
    for (r, m) in p.get('chime', ()): put(r, 9, 'bell', m, p.get('bv', 40))
    for (r, m) in p.get('pno', ()): put(r, 9, 'piano', m, 44)
    k = p.get('lk')
    for (r, m) in p.get('lead', ()): put(r, 12, k or 'flute', m + p.get('lo', 0), p.get('lv', 46)); (put(r, 13, p['l2'], m + p.get('lo', 0) - 12, p.get('lv', 46) - 14) if p.get('l2') else None)
    if p.get('stab'):
        for r in (0, 18, 36) if L == 48 else (0, 12, 24):
            for j, m in enumerate(tones[:3]): put(r, 10 + (j % 2), 'chug', m - 12, 58)
            put(r, 3, 'crash', None, 40)
    return P

S = []
def sec(prog, L=48, **kw):
    for i, c in enumerate(prog):
        d = dict(kw); d['ch'] = c; d['L'] = L; d['i'] = i
        for key in list(d):
            if isinstance(d[key], list) and key in ('leadbars', 'chimebars', 'pnobars', 'tunebars'): pass
        S.append(d)
def lead_from(pos0, n=4): return [dict(lead=LEAD[pos0 + k // 2][k % 2]) for k in range(n)]
def apply(start, items):
    for k, it in enumerate(items): S[start + k].update(it)
PA = ['Cm', 'Dd', 'Fm', 'Cm']; PB = ['Dd', 'Db', 'Eb', 'Db']; PD = ['Cm', 'G7', 'G7', 'Dd', 'Cm', 'Eb', 'G7', 'G7']
# 1 intro: 12-string, bell chime, strings
a = len(S); sec(['Cm', 'Eb', 'Fm', 'Ab'], arp=30, str=0); apply(a, [dict(chime=[(6 * k, m) for k, m in enumerate(CHIME[i])], bv=38, str=26 if i > 1 else 0) for i in range(4)])
# 2 verse: bass, soft drums, strings, organ, the Whistler tune on the flute (3 bars)
a = len(S); sec(PA * 2, arp=34, str=30, org=26, bass='sus', drums='soft')
S[a + 3]['fill'] = 1
for (b, r, m) in TUNE: S[a + 4 + b].setdefault('lead', []).append((r, m))
for k in range(4, 8): S[a + k].update(lv=44, lk='flute')
# 3 main: the original tresillo lead, rock drums, bass in eighths
a = len(S); sec(PA + PB, arp=30, str=34, org=30, bass='eighth', drums='rock', crash=0, lk='synth', lv=42); apply(a, lead_from(6, 8)); S[a]['crash'] = 1; S[a + 7]['fill'] = 1
# 4 heavy: 7/8 riff on double kick, then 4/4 unison hits and the Nursery run
a = len(S); sec(['Fm', 'G7', 'Fm', 'G7'], L=42, chug=58, bass='eighth', drums='metal', str=26, crash=1)
b = len(S); sec(['Fm', 'G7', 'Ab', 'G7'], chug=58, bass='walk', drums='metal', stab=1, org=34); S[b + 3]['fill'] = 1
for i in range(4): S[b + i]['lead'] = [(3 * k + 12 * i if False else 3 * k + 24 * (k >= 4) + (0 if k < 4 else 0), m) for k, m in enumerate(RUN)] if i == 3 else []
S[b + 3]['lead'] = [(24 + 3 * k, m + 12) for k, m in enumerate(RUN)]; S[b + 3]['lk'] = 'glead'; S[b + 3]['lv'] = 50
# 5 solo: organ then guitar on the Whistler trill lick
a = len(S); sec(PA + PA, arp=24, str=30, bass='walk', drums='ride', org=34, lk='glead', lv=52)
for i in range(8): S[a + i]['lead'] = [(3 * k + 0, m - (12 if i < 4 else 0)) for k, m in enumerate(LICK)] if i % 2 == 0 else [(3 * k + 6, m + (0 if i < 4 else 12) - 12 * (i < 4)) for k, m in enumerate(LICK[:8])]
S[a]['crash'] = 1; S[a + 3]['fill'] = 1; S[a + 7]['fill'] = 1
for i in range(4): S[a + i]['lk'] = 'synth'
# 6 pastoral 12/8: strings, 12-string, the Nursery tune on the flute, piano answer
a = len(S); sec(['Cm', 'G7', 'Cm', 'Dd', 'Cm', 'Eb', 'G7', 'G7'], L=72, arp=34, str=36, bass='sus', drums='soft', org=22)
for i, bar in enumerate(NTUNE * 2): S[a + i]['lead'] = [(6 * k, m) for k, m in enumerate(bar)]; S[a + i]['lk'] = 'flute'; S[a + i]['lv'] = 46
for i in range(4, 8): S[a + i]['pno'] = [(6 * k + 3, m - 12) for k, m in enumerate(NTUNE[i - 4])]
# 7 finale: the tune on synth + guitar, everything in
a = len(S); sec(PD, arp=24, str=36, org=36, bass='eighth', drums='metal', chug=54, crash=0, lk='synth', lv=50, l2='glead')
for (b2, r, m) in TUNE: S[a + b2].setdefault('lead', []).append((r, m + 12))
apply(a + 4, [dict(lead=LEAD[18][0] if False else [(r, m + 12) for (r, m) in LEAD[6][k % 2]]) for k in range(4)])
S[a]['crash'] = 1; S[a + 4]['crash'] = 1; S[a + 3]['fill'] = 1; S[a + 7]['fill'] = 1
# 8 outro: the chime again, over a held Cm
a = len(S); sec(['Cm', 'Cm'], arp=26, str=34, org=24, bass='sus'); apply(a, [dict(chime=[(6 * k, m) for k, m in enumerate(CHIME[i * 3 % 4])], bv=36) for i in range(2)])

def main():
    F.I.clear(); F.I.update(I); pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Closer to the End'[:20].ljust(20) + b'\x1a' + b'make_closer_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    open(OUT, 'wb').write(hdr + b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS))
    s = sum(len(pats[o]) for o in order) * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
