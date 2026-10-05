#!/usr/bin/env python3
"""Builds tools/tree_fast_old.xm: the SHORT first version (2:02) of the fast Breno Orêian (Eno parody) rework of tools/tree_swaying_action.xm, in E flat.
usage: python3 tools/make_tree_fast_old_rework.py   (needs numpy; byte-identical on re-run)

TRANSCRIPTION (detected, not guessed): every sample is analysed (autocorrelation for the plucked bass, spectral peaks for the pad chord);
each note then sounds at  detected pitch + (XM note - C-4)  ->  the original is a I IV V IV in G  (G C D C, bass G A B A, pad G6/9 C6/9 D6/9),
a sub pluck line (B B G E G C C A | B B G D E D C) and a plucked countermelody (G A A B | D C B).  All of it is moved to E FLAT (-3 semitones).
The drums keep the original's rhythm (kick-ish 12, snare-ish 11, cymbal 16) and are re-voiced soft and motorik; the new voices are
Eno's: a DX7 Rhodes, a marimba-ish pluck with a dotted-eighth tape echo, an E-bow lead, an "ooh" choir and an air pad.
Grid: 8 rows a beat (speed 3, 126 BPM): a 4/4 bar = 32 rows = one chord of the original.
Channels: 0 kick | 1 snare | 2 hat | 3 shaker / tom | 4 bass | 5 6 pad | 7 8 rhodes | 9 pluck | 10 e-bow | 11 echo / ooh
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__)); OUT = os.path.join(HERE, "tree_fast_old.xm")
SPEED, BPM, NCH, T = 3, 126, 12, -3          # T: the whole piece moves from G to E flat
src = xm.parse(os.path.join(HERE, "tree_swaying_action.xm"))
ROW = 2.5 * src['tempo'] / src['bpm']

# ---------------- detect the samples ----------------
def sr_of(sm): return 8363 * 2 ** ((sm['rel'] + sm['fine'] / 128) / 12)
def f2m(f): return 69 + 12 * np.log2(f / 440.0)
def fund(sm, lo=30, hi=700):
    """the pitch of a sample played at C-4: the strongest low spectral peak, refined"""
    x = np.array(sm['data']); sr = sr_of(sm); w = min(len(x), int(.6 * sr)); seg = x[:w] * np.hanning(w)
    sp = np.abs(np.fft.rfft(seg, 1 << 18)); fr = np.fft.rfftfreq(1 << 18, 1 / sr); m = (fr > lo) & (fr < hi)
    pk = fr[m][np.argmax(sp[m])]; return f2m(pk)
def chord(sm, t0=0, t1=2.4, lo=70, hi=400):
    """the pad's voicing: spectral peaks of its first 2.4 s -> MIDI notes"""
    x = np.array(sm['data']); sr = sr_of(sm); seg = x[int(t0 * sr):int(t1 * sr)]; seg = seg * np.hanning(len(seg))
    sp = np.abs(np.fft.rfft(seg, 1 << 19)); fr = np.fft.rfftfreq(1 << 19, 1 / sr); m = (fr > lo) & (fr < hi); sp = sp * m; mx = sp.max(); out = set()
    for _ in range(12):
        j = int(np.argmax(sp))
        if sp[j] < .12 * mx: break
        out.add(int(round(f2m(fr[j])))); sp[max(0, j - 40):j + 40] = 0
    return sorted(out)
PAD = chord(src['insts'][0]['samples'][0])
B_PLUCK = int(round(fund(src['insts'][2]['samples'][0], 30, 200)))     # sub pluck (instrument 3)
B_BASS = int(round(fund(src['insts'][6]['samples'][0], 100, 600)))     # bass pad (instrument 7)
# ---------------- transcribe the song ----------------
EV = []   # (t seconds, channel, instrument, sounding midi in E flat)
for oi, pi in enumerate(src['order']):
    for r, row in enumerate(src['pats'][pi]):
        for c, (n, i, v, e, ep) in enumerate(row):
            if n and n < 97: EV.append(((oi * 32 + r) * ROW, c, i, n - 49))
def at(t): return int(round(t / (ROW * 8)))     # which 8-row chord of the original
CHORDS = sorted({(round(t, 3), d) for t, c, i, d in EV if i == 1 and c == 0})
PADN = {at(t): [p + d + T for p in PAD] for t, d in CHORDS}                         # chord by bar of the original
BASSN = {}; SUB = []; PL = []; HITS = []
for t, c, i, d in EV:
    if i == 7: BASSN.setdefault(at(t), []).append(B_BASS + d + T)
    elif i == 3 and c == 6: SUB.append((t, B_PLUCK + d + T))
    elif i == 3 and c == 8: PL.append((t, B_PLUCK + d + T + 24))
    elif i in (11, 12, 16): HITS.append((t, i))
# the original's drum rhythm, one 16-row bar of it (rows 0..15 of 0.3 s): 12 = kick, 11 = snare, 16 = cymbal
DR = {}
for t, i in HITS:
    rr = int(round(t / ROW)) % 16; DR.setdefault(rr, set()).add(i)
DRUM_STEPS = sorted((rr * 2, sorted(v)) for rr, v in DR.items())      # onto my 32-row bar

# ---------------- sounds ----------------
rng = np.random.default_rng(1977); I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
n = int(.28 * SR); t = tt(n); add('kick', 'soft kick', finish(np.sin(2 * np.pi * np.cumsum(46 + 70 * np.exp(-t / .03)) / SR) * np.exp(-t / .14), .8))
n = int(.25 * SR); t = tt(n); add('snare', 'rim snare', finish(np.tanh(1.2 * (bp(noise(n), 1500, 6000) * np.exp(-t / .07) + np.sin(2 * np.pi * 210 * t) * np.exp(-t / .04))), .7))
n = int(.06 * SR); add('hat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .012), 7500, 4), .25))
n = int(.5 * SR); t = tt(n); add('ohat', 'open hat', finish(hp(noise(n), 6500, 3) * np.exp(-t / .17), .3))
n = int(.12 * SR); t = tt(n); add('shaker', 'shaker', finish(bp(noise(n), 5000, 9000) * np.minimum(1, t / .02) * np.exp(-t / .045), .25))
n = int(.4 * SR); t = tt(n); add('tom', 'tom', finish(np.sin(2 * np.pi * np.cumsum(110 + 70 * np.exp(-t / .05)) / SR) * np.exp(-t / .2), .7), 50)
n = int(.8 * SR); t = tt(n); f = mid2f(40); add('bass', 'round bass', finish(np.tanh(1.3 * (np.sin(2 * np.pi * f * t) + .25 * np.sin(4 * np.pi * f * t))) * np.minimum(1, t / .004) * np.exp(-t / .55), .9), 40)
n = int(2.0 * SR2); t = tt(n, SR2); f = mid2f(60); u = np.minimum(1, t / .5)
x = sum(np.sin(2 * np.pi * f * 2 ** (c / 1200) * t + rng.uniform(0, 6.28)) for c in (-9, -3, 3, 9)) / 4 + .3 * np.sin(2 * np.pi * f * 2 * t) * np.sin(2 * np.pi * .35 * t)
add('pad', 'air pad', finish(x * u * np.exp(-t / 2.2), .55, .1, SR2), 60, 5)
n = int(1.4 * SR2); t = tt(n, SR2); f = mid2f(60); x = np.sin(2 * np.pi * f * t + 1.6 * np.sin(2 * np.pi * f * 14 * t) * np.exp(-t / .04)) + .5 * np.sin(2 * np.pi * f * t + .5 * np.sin(2 * np.pi * f * t) * np.exp(-t / .6))
add('rhodes', 'dx7 rhodes', finish(x * np.exp(-t / .75) * np.minimum(1, t / .003), .65, .06, SR2), 60, 5)
n = int(1.0 * SR); t = tt(n); f = mid2f(72); x = np.sin(2 * np.pi * f * t) + .4 * np.sin(2 * np.pi * f * 4 * t) * np.exp(-t / .08) + .15 * np.sin(2 * np.pi * f * 9.2 * t) * np.exp(-t / .04)
add('pluck', 'marimba pluck', finish(x * np.exp(-t / .32), .65), 72)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(72); w = 2 * np.pi * np.cumsum(f * (1 + .004 * np.sin(2 * np.pi * 5 * t) * np.clip((t - .3) / .5, 0, 1))) / SR2
x = np.sin(w) + .35 * np.sin(2 * w) + .12 * np.sin(3 * w)
add('ebow', 'e-bow lead', finish(x * np.minimum(1, t / .45) * np.exp(-t / 1.5), .6, .08, SR2), 72, 5)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(60); w = 2 * np.pi * np.cumsum(f * (1 + .005 * np.sin(2 * np.pi * 4.7 * t))) / SR2
x = saw(f, n, SR2) * 0; ph = (w / (2 * np.pi)) % 1; x = 2 * ph - 1; x = bp(x, 400, 1100, 2, SR2) + .6 * bp(x, 700, 1300, 2, SR2)
add('ooh', 'ooh choir', finish(x * np.minimum(1, t / .4) * np.exp(-t / 1.4), .55, .08, SR2), 60, 5)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(60); x = sum(np.sin(2 * np.pi * f * k * t) / k ** 1.3 * np.exp(-t * (.9 + k * .55)) for k in range(1, 9))
add('piano', 'meadow piano', finish(x * np.minimum(1, t / .002), .7, .05, SR2), 60, 5)
n = int(1.1 * SR2); t = tt(n, SR2); f = mid2f(72); w = 2 * np.pi * np.cumsum(f * (1 + .005 * np.sin(2 * np.pi * 5.3 * t) * np.clip((t - .1) / .25, 0, 1))) / SR2
x = np.sin(w) + .18 * np.sin(2 * w) + .05 * np.sin(3 * w) + lp(noise(n), 4500, 2, SR2) * .06
add('flute', 'wooden flute', finish(x * np.minimum(1, t / .06) * np.exp(-t / .85), .65, .08, SR2), 72, 5)
n = int(.45 * SR); t = tt(n); y = np.zeros(n); L = int(round(SR / mid2f(48))); y[:L] = noise(L)
for i in range(L, n): y[i] = .993 * .5 * (y[i - L] + y[i - L - 1])
add('pizz', 'pizzicato', finish(lp(y, 3500), .7), 48)
KEYS = ['kick', 'snare', 'hat', 'ohat', 'shaker', 'tom', 'bass', 'pad', 'rhodes', 'pluck', 'ebow', 'ooh']   # (the longer version also has the meadow's piano, flute and pizzicato)
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return max(1, min(96, 49 + (m - g))) if g else 49

# ---------------- the arrangement (64 bars: the original's I IV V IV in four-bar turns) ----------------
LOOP = [0, 1, 2, 1]                                                  # bar index of the original's chords: G C D C -> Eb Ab Bb Ab
def padc(k): return PADN[[0, 1, 2, 3][k % 4] * 1] if False else PADN[(0, 1, 2, 3)[k % 4]]
BASSD = {0: BASSN.get(19.2 and at(19.2)), }
SUBL = [[(round((t - 37.8) / ROW), m) for (t, m) in SUB if 37.8 <= t < 57.6]]
def bar_events(lst, t0, t1): return [(int(round((t - t0) / (ROW / 2 * 1))), m) for (t, m) in lst if t0 <= t < t1]
def build(p):
    k = p['k']; P = [[(0, 0, 0)] * NCH for _ in range(32)]; ch = PADN[at(0) if k % 4 == 0 else at(2.4) if k % 4 in (1, 3) else at(4.8)]
    def put(r, c, key, m=None, v=64):
        if 0 <= r < 32: P[r][c] = (xmn(m, key) if m is not None else 49, INST[key], 0x10 + max(0, min(64, int(v))))
    bs = sorted(BASSN[at(19.2 + 2.4 * (k % 4))]);
    if p.get('pad'): put(0, 5, 'pad', ch[1] + 12, p['pad']); put(0, 6, 'pad', ch[3], p['pad'] - 6); put(16, 5, 'pad', ch[2] + 12, p['pad'] - 8); put(16, 6, 'pad', ch[4], p['pad'] - 12)
    if p.get('bass'): put(0, 4, 'bass', bs[0] + 12, p['bass']); put(12, 4, 'bass', bs[-1] + 12, p['bass'] - 10); put(16, 4, 'bass', bs[0] + 12, p['bass'] - 4); put(28, 4, 'bass', bs[-1] + 12, p['bass'] - 12)
    if p.get('rh'):                                                  # two Rhodes loops of 3 and 4 eighths, drifting like tape loops
        up = [ch[0] + 12, ch[1] + 12, ch[2] + 12, ch[3] + 12, ch[4] + 12]
        for j, r in enumerate(range(0, 32, 4)): put(r, 7, 'rhodes', up[(0, 2, 1, 3, 4, 2, 3, 1)[j % 8]], p['rh'] - (j % 2) * 8)
        for j, r in enumerate(range(2, 32, 6)): put(r, 8, 'rhodes', up[(4, 2, 3)[j % 3]] + 12, p['rh'] - 14)
    if p.get('pno'):                                                 # the meadow: rolling piano arpeggios and pizzicato on the off beats
        up = [ch[0] + 12, ch[1] + 12, ch[2] + 12, ch[3] + 12, ch[4] + 12]
        for j, r in enumerate(range(0, 32, 4)): put(r, 7, 'piano', up[(0, 2, 3, 2, 1, 3, 4, 3)[j % 8]], p['pno'] - (j % 4 != 0) * 7)
        for r in (8, 24): put(r, 3, 'pizz', bs[0] + 24, 40); put(r + 4, 3, 'pizz', bs[-1] + 24, 30) if r == 8 else None
    for (r, m) in p.get('mel', ()): put(r, 10, 'flute', m, p.get('mv', 44)); put(r + 6, 11, 'flute', m, 18)
    d = p.get('dr')
    if d:
        for r, s in DRUM_STEPS:
            if 12 in s and d >= 1: put(r, 0, 'kick', None, 46)
            if 11 in s and d >= 2: put(r, 1, 'snare', None, 40)
            if 16 in s and d >= 2: put(r, 2, 'ohat', None, 28)
        for r in range(0, 32, 4):
            if d >= 1: put(r + 2, 3, 'shaker', None, 22 + (r % 8 == 0) * 6)
            if d >= 3: put(r, 2, 'hat', None, 22 if r % 8 else 30)
        if d >= 3:
            for r in (0, 8, 16, 24): put(r, 0, 'kick', None, 52)
        if p.get('fill'):
            for j, r in enumerate(range(20, 32, 2)): put(r, 3, 'tom', 52 - (j // 2) * 3, 24 + j * 5)
    ln = p.get('sub', 0)
    if ln:
        for (r, m) in p.get('subn', ()): put(r, 9, 'pluck', m + 24, ln); put(r + 6, 11, 'pluck', m + 24, ln * .42)
    for (r, m) in p.get('cm', ()): put(r, 10, 'ebow', m, p.get('cv', 40)); put(r + 6, 11, 'ooh', m - 12, 20) if p.get('oo') else None
    if p.get('ooh'): put(0, 11, 'ooh', ch[2] + 12, p['ooh'])
    return P

# original plucked bars: the sub pluck line and the countermelody, cut into 32-row bars (1 original chord = 8 rows of 0.3 s = 1 bar)
def line_bars(lst, t0):
    out = {}
    for (t, m) in lst:
        b = int((t - t0) // (ROW * 8)); r = int(round(((t - t0) % (ROW * 8)) / (ROW * 8) * 32)); out.setdefault(b, []).append((r, m))
    return out
SUBB = line_bars([(t, m) for (t, m) in SUB if 37.8 <= t < 57.6], 38.4 - 0.0) ; PLB = line_bars(PL, 38.4)
S = []
def sec(n, **kw):
    for i in range(n): S.append(dict(kw, k=len(S)))
def put_lines(a, n, sub=True, cm=True):
    for i in range(n):
        b = i % 8
        if sub: S[a + i]['subn'] = SUBB.get(b, [])
        if cm: S[a + i]['cm'] = [(r, m) for (r, m) in PLB.get(b, [])]
# A intro, B groove, C full, D tape-loop break, E full + lead, F outro (64 bars)
a = len(S); sec(8, pad=34, rh=34, ooh=22)
a = len(S); sec(8, pad=34, rh=36, bass=44, dr=1, ooh=22, sub=0)
a = len(S); sec(16, pad=34, rh=34, bass=46, dr=2, sub=30, cv=40); put_lines(a, 16)
a = len(S); sec(8, pad=36, rh=40, ooh=30)
a = len(S); sec(16, pad=34, rh=36, bass=48, dr=3, sub=34, cv=46, oo=1); put_lines(a, 16)
for i in (7, 15): S[a + i]['fill'] = 1
a = len(S); sec(8, pad=30, rh=30, bass=36, ooh=24)

def main():
    F.I.clear(); F.I.update(I); pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    pats.append([[(0, 0, 0)] * NCH for _ in range(16)]); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Tree fast'[:20].ljust(20) + b'\x1a' + b'make_tree_fast'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    open(OUT, 'wb').write(hdr + b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS))
    s = sum(len(pats[o]) for o in order) * SPEED * 2.5 / BPM
    print("pad voicing", PAD, "sub", B_PLUCK, "bass", B_BASS)
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
