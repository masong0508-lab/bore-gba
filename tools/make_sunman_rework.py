#!/usr/bin/env python3
"""Builds tools/sunman_sunrise.xm : "Sunman (Sunrise)", a sunrise nu-disco rework of "The Dipper Man - Sunman" (tools/the_dipper_man_sunman.xm).

usage:  python3 tools/make_sunman_rework.py        (run from the project root; needs numpy + scipy)

Kept from the original, note for note: the two-voice hook (channels 1+2 of the original, played an octave lower here), its syncopated rhythm
(16ths 0, 3, 6, 10, 12 of every half bar), the walking bass (F# G A C | D C Bb G) and the rising counter-line (F# G A D E D C Bb A).
New: the harmony under them (D/F# Gmaj7 Bbmaj7/A C | D C Bbmaj7#11 Gmaj7: every chord holds the original notes), the arrangement (bell
intro, four-on-the-floor groove, pumping pads, an arp, a breakdown, a last chorus lifted a whole tone) and every sound, synthesised here.
Uses only what the GBA player supports: one-shot samples, note + volume column, no effects. The "pump" of the pads is baked into the pad
sample (it swells over one beat) and the pad is struck on every beat.
Channels (0-based): 0 kick | 1 clap / snare | 2 hat | 3 open hat / shaker | 4 bass | 5 6 7 chords | 8 lead | 9 lead harmony | 10 arp / counter | 11 bell / fx
116 BPM, 64-row patterns = 4 bars (one turn of the hook). Re-running gives byte-identical output.
"""
import struct, os
import numpy as np
from scipy import signal

SR = 16726                       # a "relative note +12" sample plays at its natural pitch on C-4
SR2 = 8363 * 2 ** (5 / 12)       # relative note +5 (11163 Hz) for softer sounds with little top end
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sunman_sunrise.xm")
rng = np.random.default_rng(2026)
BPM, SPEED, NCH, ROWS = 116, 6, 12, 64
BEAT = 60 / BPM

# ---------------------------------------------------------------- synthesis helpers
def tt(n, sr=SR): return np.arange(n) / sr
def mid2f(m): return 440.0 * 2 ** ((m - 69) / 12)
def lp(x, fc, order=2, sr=SR): b, a = signal.butter(order, min(fc, sr * .45) / (sr / 2), 'low'); return signal.lfilter(b, a, x)
def hp(x, fc, order=2, sr=SR): b, a = signal.butter(order, fc / (sr / 2), 'high'); return signal.lfilter(b, a, x)
def bp(x, f1, f2, order=2, sr=SR): b, a = signal.butter(order, [f1 / (sr / 2), min(f2, sr * .45) / (sr / 2)], 'band'); return signal.lfilter(b, a, x)
def noise(n): return rng.uniform(-1, 1, n)
def saw(f, n, sr=SR, ph=0.0, vib=None):
    t = tt(n, sr); y = np.zeros(n); w = 2 * np.pi * f * (t if vib is None else np.cumsum(vib) / sr)
    for k in range(1, int(min(sr * .45 / f, 30)) + 1): y += np.sin(k * w + ph * k) / k
    return y * (2 / np.pi)
def square(f, n, sr=SR, vib=None):
    t = tt(n, sr); y = np.zeros(n); w = 2 * np.pi * f * (t if vib is None else np.cumsum(vib) / sr)
    for k in range(1, int(min(sr * .45 / f, 30)) + 1, 2): y += np.sin(k * w) / k
    return y * (4 / np.pi)
def finish(x, peak, tail=0.012, sr=SR):
    x = x / np.abs(x).max() * peak
    k = int(tail * sr); x[-k:] *= np.linspace(1, 0, k) ** 2
    return x

# ---------------------------------------------------------------- instruments
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
# drums: a round kick, a warm clap, crisp hats, a shaker
n = int(.32 * SR); t = tt(n)
ph = 2 * np.pi * np.cumsum(50 + 120 * np.exp(-t / .03)) / SR
k = np.sin(ph) * np.exp(-t / .13); k[:int(.002 * SR)] += lp(noise(int(.002 * SR)), 2500) * .4
add('kick', 'kick', finish(np.tanh(1.5 * k), .98))
n = int(.26 * SR); c = np.zeros(n)
for off, dec, a in ((0, .006, .7), (.010, .006, .8), (.021, .007, .8), (.030, .07, 1.0)):
    i = int(off * SR); m = n - i; c[i:] += noise(m) * np.exp(-tt(m) / dec) * a
add('clap', 'clap', finish(bp(c, 900, 3200, 2) + lp(c, 600) * .3, .78))
n = int(.06 * SR); add('chat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .013), 6000, 4), .42))
n = int(.34 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .10), 5200, 4), .45))
n = int(.09 * SR); t = tt(n); add('shaker', 'shaker', finish(bp(noise(n), 4000, 7800, 2) * np.sin(np.pi * t / t[-1]) ** 2, .32))
n = int(.15 * SR); t = tt(n); add('snare', 'snare', finish(bp(noise(n), 1500, 6000, 2) * np.exp(-t / .045) + np.sin(2 * np.pi * 200 * t) * np.exp(-t / .03) * .6, .8))
# bass: a round plucked disco bass (G2 = midi 43)
n = int(.42 * SR); t = tt(n); f = mid2f(43)
b = saw(f, n) * .6 + np.sin(2 * np.pi * f * t) * 1.1 + np.sin(4 * np.pi * f * t) * .25
b = lp(b, 900) * np.exp(-t / .30) * np.minimum(1, t / .003); b = np.tanh(1.6 * b)
add('bass', 'disco bass', finish(b, .95), 43)
# pumping pad: one note, swells over one beat (struck on every beat = the sidechain feel). C4 = midi 60
n = int(BEAT * SR2); t = tt(n, SR2); f = mid2f(60)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-11, -4, 4, 11)) / 4
x = lp(x, 1700, 2, SR2) * (.18 + .82 * np.minimum(1, t / (BEAT * .55)) ** 1.6)
add('pad', 'pump pad', finish(x, .62, .03, SR2), 60, rel=5)
# electric piano: soft FM-ish tine (C4)
n = int(1.1 * SR); t = tt(n); f = mid2f(60)
mod = np.sin(2 * np.pi * f * 14 * t) * 1.3 * np.exp(-t / .05)
x = np.sin(2 * np.pi * f * t + np.sin(2 * np.pi * f * t) * .9 * np.exp(-t / .35) + mod * .4) * np.exp(-t / .55)
add('ep', 'e-piano', finish(x, .7), 60)
# the sun lead (C5): bright detuned saw + square, vibrato that blooms after 120 ms, gentle release
n = int(.70 * SR2); t = tt(n, SR2); f = mid2f(72)
vib = f * (1 + .006 * np.sin(2 * np.pi * 5.4 * t) * np.clip((t - .12) / .2, 0, 1))
x = saw(f, n, SR2, vib=vib) * .6 + saw(f * 1.004, n, SR2, vib=vib) * .5 + square(f / 2, n, SR2, vib=vib / 2) * .25
x = lp(x, 3200, 2, SR2) * np.minimum(1, t / .006) * (.62 + .38 * np.exp(-t / .12)) * np.minimum(1, (n / SR2 - t) / .18)
add('lead', 'sun lead', finish(x, .78), 72, rel=5)
# arp pluck (C5) and a glassy bell (C5)
n = int(.22 * SR); t = tt(n); f = mid2f(72)
add('pluck', 'pluck', finish(lp(square(f, n) * .6 + saw(f, n) * .4, 2600) * np.exp(-t / .07), .66), 72)
n = int(1.4 * SR); t = tt(n); f = mid2f(72)
x = np.sin(2 * np.pi * f * t) * np.exp(-t / .5) + .5 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t / .18) + .25 * np.sin(2 * np.pi * f * 5.4 * t) * np.exp(-t / .06)
add('bell', 'bell', finish(x, .7), 72)
# fx: a crash, a white-noise riser (2 bars), a soft sub drop
n = int(.9 * SR2); add('crash', 'crash', finish(hp(noise(n), 3500, 3, SR2) * np.exp(-tt(n, SR2) / .38), .55), None, rel=5)
n = int(BEAT * 8 * SR2); t = tt(n, SR2)
u = t / t[-1]; nz = noise(n)   # a rising whoosh: the noise moves from a low band to a high one
add('riser', 'riser', finish((bp(nz, 300, 1200, 2, SR2) * (1 - u) + bp(nz, 1800, 4800, 2, SR2) * u) * u ** 2, .55), None, rel=5)
n = int(1.2 * SR); t = tt(n); ph = 2 * np.pi * np.cumsum(40 + 60 * np.exp(-t / .2)) / SR
add('drop', 'sub drop', finish(np.sin(ph) * np.exp(-t / .5), .9))
KEYS = ['kick', 'clap', 'chat', 'ohat', 'shaker', 'snare', 'bass', 'pad', 'ep', 'lead', 'pluck', 'bell', 'crash', 'riser', 'drop']
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the music (rows are 16ths; a pattern is 4 bars = one turn of the hook)
NOTE = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}
def nm(s): return NOTE[s[:-1]] + 12 * (int(s[-1]) + 1)
def xmnote(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49
# the hook, from the original (2 voices; rows inside a 16-row half bar: 0 3 6 10 12), an octave lower than the original
HIT = (0, 3, 6, 10, 12)
HOOK = [  # per half bar: (voice 1 notes, voice 2 notes) for hits 0 3 | 6 10 12
    (('A4', 'F#4'), ('G4', 'A4')), (('Bb4', 'G4'), ('F4', 'C5')),
    (('F#4', 'E4'), ('D5', 'C5')), (('E5', 'D5'), ('G4', 'F#4'))]
BASSL = [  # (row, note) in a 4-bar pattern: the original walking bass, an octave lower
    (0, 'F#2'), (1, 'F#2'), (3, 'F#2'), (4, 'F#2'), (6, 'G2'), (10, 'G2'), (12, 'G2'), (16, 'A2'), (17, 'A2'), (19, 'A2'), (20, 'A2'), (22, 'C3'), (26, 'C3'), (28, 'C3'), (30, 'C#3'),
    (32, 'D3'), (33, 'D3'), (35, 'D3'), (36, 'D3'), (38, 'C3'), (42, 'C3'), (44, 'C3'), (48, 'Bb2'), (49, 'Bb2'), (51, 'Bb2'), (52, 'Bb2'), (54, 'G2'), (58, 'G2'), (60, 'G2'), (63, 'A2')]
COUNTER = [(4, 'F#4'), (8, 'G4'), (10, 'A4'), (12, 'D5'), (14, 'E5'), (18, 'D5'), (22, 'C5'), (26, 'Bb4'), (30, 'A4')]   # the original counter-line (2 bars), played twice
CHORDS = [  # (start row, three notes) the new harmony; every chord holds the hook's notes
    (0, ('D4', 'F#4', 'A4')), (6, ('D4', 'F#4', 'B4')), (16, ('D4', 'F4', 'A4')), (22, ('C4', 'E4', 'G4')),
    (32, ('D4', 'F#4', 'A4')), (38, ('C4', 'E4', 'G4')), (48, ('D4', 'F4', 'A4')), (54, ('D4', 'F#4', 'B4'))]
def chord_at(r): return [c for s, c in CHORDS if s <= r][-1]

def build(p):   # p: dict of what plays in this pattern
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    T = p.get('tr', 0)
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (xmnote(midi + T, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, vol)))
    for r in range(ROWS):
        b = r % 16
        if p.get('kick') and b % 4 == 0: put(r, 0, 'kick', None, 64)
        if p.get('clap') and b in (4, 12): put(r, 1, 'clap', None, 50)
        if p.get('hats') and b % 4 == 2: put(r, 2, 'chat', None, 40 if p['hats'] == 1 else 48)
        if p.get('hats') == 2 and b % 4 in (1, 3): put(r, 3, 'shaker', None, 30)
        if p.get('ohat') and b == 14 and (r // 16) % 2 == 1: put(r, 3, 'ohat', None, 38)
        if p.get('pad') and b % 4 == 0:   # struck on every beat: the pump
            for j, nt in enumerate(chord_at(r)): put(r, 5 + j, 'pad', nm(nt), p['pad'])
        if p.get('ep') and b in (0, 6, 10):   # off-beat e-piano chords
            for j, nt in enumerate(chord_at(r)): put(r, 5 + j, 'ep', nm(nt) + 12, p['ep'])
    if p.get('bass'):
        for r, nt in BASSL: put(r, 4, 'bass', nm(nt), 58 if r % 4 == 0 else 46)
    if p.get('hook'):
        key = p.get('hookinst', 'lead'); v1 = p['hook']
        # voice 1 on ch 8, voice 2 on ch 9 (the original has both voices hit the same rows)
        for h, (a, bb) in enumerate(HOOK):
            base = h * 16
            for r in (0, 3): put(base + r, 8, key, nm(a[0]), v1); put(base + r, 9, key, nm(bb[0]), v1 - 14)
            for r in (6, 10, 12): put(base + r, 8, key, nm(a[1]), v1); put(base + r, 9, key, nm(bb[1]), v1 - 14)
    if p.get('counter'):
        for rep in (0, 32):
            for r, nt in COUNTER: put(rep + r, 10, 'pluck', nm(nt), p['counter'])
    if p.get('arp'):   # rolling 16th arp through the chord, up an octave
        for r in range(ROWS):
            if P[r][10][0] == 0:
                c = chord_at(r); put(r, 10, 'pluck', nm(c[[0, 1, 2, 1][r % 4]]) + 12, p['arp'] - (0 if r % 4 == 0 else 10))
    if p.get('bell'):   # the hook's top voice on a bell, one note per hit pair (sparser, for the intro and breakdown)
        for h, (a, bb) in enumerate(HOOK): put(h * 16, 11, 'bell', nm(a[0]) + 12, p['bell']); put(h * 16 + 6, 11, 'bell', nm(a[1]) + 12, p['bell'] - 8)
    for (r, key, vol) in p.get('fx', ()): put(r, 11, key, None, vol)
    for r in p.get('roll', ()): put(r, 1, 'snare', None, 22 + (r % 16) * 2)
    return P

# ---------------------------------------------------------------- arrangement: 22 patterns x 4 bars = 88 bars, about 3:02
S = []
def sec(n, **kw): S.extend([dict(kw) for _ in range(n)])
sec(1, pad=46, bell=54)                                                           # intro: the dawn (pads + bell hook)
sec(1, pad=50, bell=56, hats=1, fx=((32, 'riser', 40),))
sec(2, kick=1, hats=1, pad=44, bass=1, counter=40)                                # the groove arrives with the original counter-line
S[-1]['fx'] = ((32, 'riser', 46),); S[-1]['roll'] = tuple(range(48, 64, 2))
sec(4, kick=1, clap=1, hats=2, ohat=1, pad=46, bass=1, hook=58, fx=((0, 'crash', 50),))   # chorus 1: the hook on the sun lead
for p in S[-3:]: p['fx'] = ()
sec(2, kick=1, clap=1, hats=2, ep=40, bass=1, counter=44)                  # interlude: e-piano and the counter-line
S[-1]['roll'] = tuple(range(48, 64, 1)); S[-1]['fx'] = ((32, 'riser', 48),)
sec(4, kick=1, clap=1, hats=2, ohat=1, pad=48, bass=1, hook=60, arp=36, fx=((0, 'crash', 52),))   # chorus 2: + arp
for p in S[-3:]: p['fx'] = ()
sec(2, pad=46, bell=54, fx=((0, 'drop', 56),))                                     # breakdown: the bells over the pads
S[-1]['fx'] = ((32, 'riser', 52),); S[-1]['roll'] = tuple(range(32, 64, 1)); S[-1]['pad'] = 42
sec(4, kick=1, clap=1, hats=2, ohat=1, pad=50, bass=1, hook=62, arp=38, tr=2, fx=((0, 'crash', 56),))   # last chorus, a whole tone up
for p in S[-3:]: p['fx'] = ()
sec(1, kick=1, hats=1, pad=40, bass=1, counter=40, tr=2)                          # outro
sec(1, pad=42, bell=50, tr=2)

# ---------------------------------------------------------------- XM writer (same layout as tools/make_meltdown_house.py)
def pat_bytes(P):
    b = bytearray()
    for row in P:
        for (n, i, v) in row: b += bytes([n, i, v, 0, 0])
    return struct.pack('<IBHH', 9, 0, len(P), len(b)) + bytes(b)
def inst_bytes(k):
    d = I[k]; q = np.clip(np.round(d['x'] * 127), -127, 127).astype(int)
    delta = np.diff(np.concatenate([[0], q])); dl = bytes((int(v) & 255) for v in delta)
    h = struct.pack('<I', 263) + d['name'].encode('latin1')[:22].ljust(22, b'\0') + b'\0' + struct.pack('<HI', 1, 40) + bytes(96) + bytes(48) + bytes(48)
    h += bytes(14) + struct.pack('<H', 0) + bytes(263 - len(h) - 14 - 2)
    sh = struct.pack('<IIIBbBBbB', len(q), 0, 0, 64, 0, 0, 128, d['rel'], 0) + b'\0' * 22
    assert len(h) == 263 and len(sh) == 40
    return h + sh + dl
def main():
    pats, order, seen = [], [], {}
    for sp in S:
        P = build(sp); key = pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    tail = [[(0, 0, 0)] * NCH for _ in range(16)]; pats.append(tail); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Sunman (Sunrise)'[:20].ljust(20) + b'\x1a' + b'make_sunman_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
