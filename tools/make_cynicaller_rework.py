#!/usr/bin/env python3
"""Builds tools/cynicaller_dnb.xm : "Cynicaller Madness", a fast liquid / tech drum & bass rework of "The Dipper Man - Cynicaller
Madness" (tools/cynicaller_madness.xm).   usage: python3 tools/make_cynicaller_rework.py

From the original (read out of the XM): the four chords (Bm | F#m | Am | Em, the last one turning to E major every other time), the
bass walk B F# A E, the lead (B A B A B A D  D B B D B B D/E), the falling counter-line (B A G# F#  E D C/C# B) and the outro's
E echoing across the voices. The original's 16ths land on every 4th row here.
New: 64th-note rows (speed 1, XM BPM 115 = 16 rows a beat at 172.5 BPM): two-step breaks with flams, ghosts pushed a 64th late
and 64th hat ratchets; a reese bass over a sine sub; 32nd-note arps with 64th flurries; snare rolls that speed up to 64ths; a
glitch section that stutters the lead; all 16 voices.
Channels: 0 kick | 1 snare | 2 ghost / break | 3 hat | 4 shaker / open hat | 5 sub | 6 reese | 7 8 9 pad | 10 arp | 11 lead
          12 lead echo / harmony | 13 bell (counter-line) | 14 stab / riser | 15 crash / impact.   128-row patterns = 2 bars.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "cynicaller_dnb.xm")
SPEED, BPM, ROWS, NCH = 1, 115, 128, 16        # 2.5/115 s a row, 16 rows a beat = 172.5 BPM
Q = 4                                          # rows per 16th
rng = np.random.default_rng(2049)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)

# ---------------------------------------------------------------- sounds
n = int(.32 * SR); t = tt(n)
add('kick', 'dnb kick', finish(np.tanh(1.6 * np.sin(2 * np.pi * np.cumsum(48 + 140 * np.exp(-t / .018)) / SR)) * np.exp(-t / .12)
                           + hp(noise(n), 3000) * np.exp(-t / .003) * .5, .95))
n = int(.26 * SR); t = tt(n)
x = bp(noise(n), 1200, 7500) * np.exp(-t / .09) + (np.sin(2 * np.pi * 200 * t) + .6 * np.sin(2 * np.pi * 340 * t)) * np.exp(-t / .04)
add('snare', 'crack snare', finish(np.tanh(2.0 * x), .92))
n = int(.08 * SR); t = tt(n)
add('ghost', 'ghost snare', finish(bp(noise(n), 1800, 7000) * np.exp(-t / .02) + np.sin(2 * np.pi * 210 * t) * np.exp(-t / .015) * .35, .5))
n = int(.045 * SR); add('hat', 'tight hat', finish(hp(noise(n) * np.exp(-tt(n) / .01), 8000, 4), .34))
n = int(.32 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .1), 7000, 4), .38))
n = int(.07 * SR); t = tt(n); add('shaker', 'shaker', finish(bp(noise(n), 5000, 9000) * np.sin(np.pi * t / t[-1]) ** 2, .28))
n = int(1.0 * SR); add('crash', 'crash', finish(hp(noise(n), 4000, 3) * np.exp(-tt(n) / .4), .5))
n = int(.9 * SR2); t = tt(n, SR2)
add('impact', 'impact', finish(np.tanh(2 * np.sin(2 * np.pi * np.cumsum(35 + 60 * np.exp(-t / .08)) / SR2)) * np.exp(-t / .35)
                               + lp(noise(n), 900, 2, SR2) * np.exp(-t / .25) * .5, .9), None, 5)
n = int(.9 * SR2); t = tt(n, SR2); f = mid2f(35)                                   # sub (B1): a pure sine
add('sub', 'sub', finish(np.sin(2 * np.pi * f * t) * np.minimum(1, t / .004) * np.minimum(1, (t[-1] - t) / .05), .95), 35, 5)
n = int(.9 * SR2); t = tt(n, SR2); f = mid2f(47)                                   # reese (B2): detuned saws, a slow filter wobble
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-14, 0, 13)) / 3
seg = [lp(x[i:i + 128], 380 + 700 * (.5 + .5 * np.sin(2 * np.pi * 2.9 * i / SR2)), 2, SR2) for i in range(0, n, 128)]
add('reese', 'reese', finish(np.tanh(1.8 * np.concatenate(seg)[:n]) * np.minimum(1, t / .006) * np.minimum(1, (t[-1] - t) / .06), .8), 47, 5)
n = int(1.4 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # pad (C4): soft supersaw, slow attack
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-11, -4, 4, 11)) / 4
add('pad', 'airy pad', finish(lp(x, 1800, 2, SR2) * np.minimum(1, t / .12) * np.minimum(1, (t[-1] - t) / .4), .55), 60, 5)
n = int(.14 * SR); t = tt(n); f = mid2f(72)                                        # arp pluck (C5): FM blip
add('pluck', 'fm pluck', finish(np.sin(2 * np.pi * f * t + 2.2 * np.exp(-t / .03) * np.sin(2 * np.pi * 2 * f * t)) * np.exp(-t / .05), .6), 72)
n = int(.5 * SR); t = tt(n); f = mid2f(72)                                         # chip lead (C5): 25% pulse, a touch of vibrato
vib = f * (1 + .006 * np.sin(2 * np.pi * 6 * t) * np.clip((t - .1) / .15, 0, 1))
x = saw(f, n, SR, vib=vib) - saw(f, n, SR, ph=np.pi / 2, vib=vib)
add('chip', 'chip lead', finish(lp(x, 6000) * np.minimum(1, t / .004) * np.exp(-t / .6), .62), 72)
n = int(.8 * SR); t = tt(n); f = mid2f(72)                                         # glass bell (C5): FM, long tail
add('bell', 'glass bell', finish(np.sin(2 * np.pi * f * t + 1.4 * np.exp(-t / .3) * np.sin(2 * np.pi * 3.5 * f * t)) * np.exp(-t / .35), .6), 72)
n = int(.22 * SR); t = tt(n); f = mid2f(60)                                        # stab (C4): bright saw chord-hit (one note, three voices make chords)
x = sum(saw(f * 2 ** (c / 1200), n, SR) for c in (-9, 9)) / 2
add('stab', 'stab', finish(lp(x, 4000) * np.exp(-t / .08), .6), 60)
n = int(1.4 * SR2); t = tt(n, SR2)                                                 # riser: noise sweeping up
seg = [bp(noise(128), 300 + 4200 * (i / n) ** 2, 600 + 6000 * (i / n) ** 2, 2, SR2) for i in range(0, n, 128)]
add('riser', 'riser', finish(np.concatenate(seg)[:n] * (t / t[-1]) ** 1.5, .5), None, 5)
KEYS = ['kick', 'snare', 'ghost', 'hat', 'ohat', 'shaker', 'crash', 'impact', 'sub', 'reese', 'pad', 'pluck', 'chip', 'bell', 'stab', 'riser']
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the original's material (16th k -> row 4k)
src = xm.parse(os.path.join(HERE, "cynicaller_madness.xm"))
def notes(pat, ch): return [(r * Q, n + 11) for r, row in enumerate(src['pats'][pat]) for (n, i, v, e, ep) in [row[ch]] if 0 < n < 97]
LEAD = {0: notes(2, 2), 1: notes(3, 2)}       # ...D / ...E endings
CTR = {0: notes(4, 6), 1: notes(5, 6)}        # the falling line (C / C#)
BASSL = notes(4, 7)                           # B F# A E
ECHO = notes(9, 4) + notes(9, 5) + notes(9, 6) + notes(9, 7)
CH_AT = (0, 32, 64, 88)                       # the chord changes (the last one anticipated, as in the original)
CHORDS = {0: [(62, 66, 71), (61, 66, 69), (60, 64, 69), (59, 64, 67)],
          1: [(62, 66, 71), (61, 66, 69), (60, 64, 69), (59, 64, 68)]}
ROOTS = (35, 30, 33, 28)                      # B1 F#1 A1 E1
SCALE = (11, 1, 2, 4, 6, 7, 9)                # B natural minor (G# borrowed on the E chord)
def chord_at(r): return sum(r % 128 >= c for c in CH_AT) - 1
def third_below(m):
    for d in (3, 4):
        if (m - d) % 12 in SCALE: return m - d
    return m - 3
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS and vol > 0: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    alt = p.get('alt', 0); d = p.get('drums', 0)
    # ---- drums: the two-step (kick 1 and the "and" of 2, snare 2 and 4), flams, late ghosts, 64th ratchets
    for bar in (0, 64):
        if d >= 1:
            for r in (0, 40) + ((54,) if d >= 3 else ()): put(bar + r, 0, 'kick', None, 60 if r == 0 else 50)
            for r in (16, 48):
                put(bar + r, 1, 'snare', None, 58)
                if d >= 3 and r == 48: put(bar + r - 1, 2, 'ghost', None, 22)                     # a flam
            for r in range(4, 64, 8): put(bar + r, 3, 'hat', None, 30 if r % 16 == 8 else 18)   # 8ths on the off
        if d >= 2:
            for r, gv in ((29, 22), (45, 16), (61, 19)): put(bar + r, 2, 'ghost', None, gv + (bar >> 4))   # ghosts a 64th late (swing)
            for r in range(0, 64, 4):
                if r % 8: put(bar + r, 4, 'shaker', None, 18 if r % 16 == 12 else 12)
            if d >= 3:
                rat = (26, 27, 58, 59, 60) if bar == 0 else (58, 59, 60, 61, 62, 63)                 # 64th ratchets
                for k, r in enumerate(rat): put(bar + r, 3, 'hat', None, 12 + 3 * k)
                put(bar + 24, 4, 'ohat', None, 26)
        if p.get('halftime') and bar == 0:
            put(0, 0, 'kick', None, 58); put(32, 1, 'snare', None, 52); put(64 + 40, 0, 'kick', None, 44); put(96, 1, 'snare', None, 56)
    if p.get('roll'):                                                                            # a snare roll speeding up: 8ths, 16ths, 32nds, 64ths
        lv = p['roll']
        for r in range(0, ROWS):
            step = 8 if r < 32 else 4 if r < 64 else 2 if r < 96 else 1
            if r % step == 0: put(r, 1, 'snare', None, lv * (.35 + .65 * r / ROWS))
    if p.get('riser'): put(0, 14, 'riser', None, p['riser'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    # ---- bass: the sub holds the root; the reese bounces in syncopation
    if p.get('sub'):
        for i, c in enumerate(CH_AT): put(c, 5, 'sub', ROOTS[i], p['sub'])
    if p.get('reese'):
        for i, c in enumerate(CH_AT):
            m = ROOTS[i] + 12
            for off, dv in ((0, 0), (10, -14), (20, -6)) if i < 3 else ((0, 0), (14, -8), (30, -16)):
                if c + off < (CH_AT[i + 1] if i < 3 else ROWS): put(c + off, 6, 'reese', m + (12 if off == 10 else 0), p['reese'] + dv)
    # ---- harmony
    if p.get('pad'):
        for i, c in enumerate(CH_AT):
            for j, m in enumerate(CHORDS[alt][i]): put(c, 7 + j, 'pad', m, p['pad'] - j * 2)
    if p.get('stabs'):
        for i, c in enumerate(CH_AT):
            for off in (0, 6, 20) if p.get('stutter') else (0, 20):
                for j, m in enumerate(CHORDS[alt][i]): put(c + off, 7 + j, 'stab', m + 12, p['stabs'] - (8 if off else 0) - j * 3)
    if p.get('arp'):                                                                             # 32nds through the chord, 64th flurries on the last beat
        for r in range(0, ROWS, 2):
            ch = CHORDS[alt][chord_at(r)]; tones = [ch[0], ch[1], ch[2], ch[0] + 12, ch[1] + 12, ch[2] + 12]
            k = (r // 2) % 10; m = tones[k if k < 6 else 10 - k] + p.get('arpo', 0)
            put(r, 10, 'pluck', m, p['arp'] - (0 if r % 8 == 0 else 8))
            if p.get('flurry') and (r % 64) >= 56: put(r + 1, 10, 'pluck', m + 7, p['arp'] - 14)
    # ---- the tunes
    if p.get('lead'):
        o = p.get('oct', 0)
        for k, (r, m) in enumerate(LEAD[alt]):
            if p.get('glitch') and k % 4 == 3:                                                     # stutter: 64th retriggers, fading
                for s in range(6): put(r + s, 11, 'chip', m + o, p['lead'] - s * 7)
            else: put(r, 11, 'chip', m + o, p['lead'])
            if p.get('harm'): put(r, 12, 'chip', third_below(m + o), p['lead'] - 10)
            else: put(r + 12, 12, 'chip', m + o, p['lead'] * .4)                                    # dotted-8th echo
    if p.get('ctr'):
        for r, m in CTR[alt]: put(r, 13, 'bell', m + p.get('ctro', 12), p['ctr'])
    if p.get('teaser'):
        for r, m in LEAD[alt][:7]: put(r, 13, 'bell', m, p['teaser'])
    if p.get('echo'):                                                                            # the original's outro: E bouncing across the voices
        for j, (r, m) in enumerate(ECHO): put(r * 2, (11, 12, 13, 10, 13)[j % 5], ('chip', 'chip', 'bell', 'pluck', 'bell')[j % 5], m, p['echo'] - j * 5)
    return P

# ---------------------------------------------------------------- arrangement: 2-bar patterns (2.78 s), about 4:20
S = []
def sec(n, **kw):
    for i in range(n): d = dict(kw); d['alt'] = i % 2; S.append(d)
sec(4, pad=40, teaser=40)                                                                    # intro: pads, the tune on a bell
sec(4, pad=40, teaser=44, arp=34, sub=40, drums=1); S[-4]['fx'] = ((0, 'crash', 40),)
sec(4, pad=38, arp=38, sub=48, drums=2, lead=40, oct=-12)
S[-1].update(riser=46, roll=50)
sec(16, sub=56, reese=46, drums=3, arp=36, lead=52, pad=30)                                  # DROP 1
S[-16]['fx'] = ((0, 'impact', 60), (2, 'crash', 50)); S[-1]['roll'] = 40
sec(8, sub=56, reese=46, drums=3, arp=34, flurry=1, lead=50, ctr=44, pad=28)                 # + the falling line
S[-8]['fx'] = ((0, 'crash', 48),)
sec(8, pad=44, sub=40, ctr=46, ctro=0, arp=28, halftime=1)                                    # breakdown
S[-4]['drums'] = 1; S[-2]['riser'] = 50; S[-1].update(roll=54, riser=0)
sec(16, sub=58, reese=48, drums=3, arp=38, arpo=12, flurry=1, lead=50, oct=12, harm=1, stabs=40)   # DROP 2: up an octave, in thirds, stabs
S[-16]['fx'] = ((0, 'impact', 62), (2, 'crash', 52))
sec(8, sub=56, reese=44, drums=3, arp=36, flurry=1, lead=50, glitch=1, stabs=38, stutter=1)  # glitch section
S[-8]['fx'] = ((0, 'crash', 46),); S[-2]['riser'] = 50; S[-1]['roll'] = 56
sec(12, sub=58, reese=48, drums=3, arp=38, flurry=1, lead=52, harm=1, ctr=44, pad=30)        # FINAL DROP
S[-12]['fx'] = ((0, 'impact', 64), (2, 'crash', 54))
sec(4, pad=36, sub=44, drums=1, echo=50, arp=26)                                             # outro: the original's echoing E
sec(2, pad=32, echo=40)
S[-2]['fx'] = ((0, 'crash', 36),)

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    end = [[(0, 0, 0)] * NCH for _ in range(ROWS)]                                           # the last hit, ringing out
    end[0][15] = (49, INST['impact'], 0x10 + 60); end[0][5] = (xmn(35, 'sub'), INST['sub'], 0x10 + 56)
    for j, m in enumerate(CHORDS[0][0]): end[0][7 + j] = (xmn(m, 'pad'), INST['pad'], 0x10 + 40)
    end[0][11] = (xmn(71, 'chip'), INST['chip'], 0x10 + 44)
    pats.append(end); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Cynicaller Madness'[:20].ljust(20) + b'\x1a' + b'make_cynicaller'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
