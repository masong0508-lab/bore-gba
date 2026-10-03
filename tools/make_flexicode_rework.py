#!/usr/bin/env python3
"""Builds tools/spanish_flexicode.xm : "An Ode to the Spanish Flexicode", a soft, wide 16-channel rework of "The Dipper Man - An Ode to Mexicode"
(tools/the_dipper_man_ode_to_mexicode.xm).

usage:  python3 tools/make_flexicode_rework.py        (run from the project root; needs numpy + scipy)

Kept from the original, note for note: the nylon stab figure (two voices, every eighth), the progression (Fm | Fm/C | Eb | Bb/D, and the
Db | Eb | Db/F | Eb/G bridge), the bass riff, the main melody (F Ab Ab C Ab Ab G F F Eb F G Bb F Bb F...), both counter-lines, the bridge
tune and the Picardy ending (Eb/G -> F major).  New: a 12/8 bembe groove (clave bell, rim, shaker with ghost notes), a breathy flute lead
with a ping-pong echo on its own channel, swelling pads breathing on every dotted beat, marimba arps, glass bells, every sound synthesised.
Pushes the player: all 16 voices, hand-panned stereo, velocity on every hit.  Uses only one-shot samples, note + volume column.
Channels: 0 kick | 1 rim / brush | 2 shaker | 3 clave bell | 4 sub bass | 5 6 7 pads | 8 9 nylon | 10 flute | 11 flute echo
          12 marimba | 13 vibes counter | 14 bell | 15 fx.      112 BPM speed 6, 24-row patterns = one bar of 12/8 (2 rows an eighth).
Re-running gives byte-identical output.
"""
import struct, os
import numpy as np
from scipy import signal

SR = 16726                       # a "relative note +12" sample plays at its natural pitch on C-4
SR2 = 8363 * 2 ** (5 / 12)       # relative note +5 (11163 Hz) for softer sounds with little top end
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "spanish_flexicode.xm")
rng = np.random.default_rng(1810)
BPM, SPEED, NCH, ROWS = 112, 6, 16, 24
ROW = 2.5 * SPEED / BPM
BEAT = 6 * ROW                   # one dotted beat (three eighths)

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


# ---------------------------------------------------------------- instruments (soft: rounded attacks, little top end, nothing harsh)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
# drums: a padded kick, a woody rim, a brush, a shaker, a clave bell (the bembe line)
n = int(.36 * SR); t = tt(n)
ph = 2 * np.pi * np.cumsum(46 + 80 * np.exp(-t / .025)) / SR
add('kick', 'soft kick', finish(lp(np.sin(ph) * np.exp(-t / .16) * np.minimum(1, t / .002), 900), .95))
n = int(.12 * SR); t = tt(n)
x = np.sin(2 * np.pi * 820 * t) * np.exp(-t / .018) + np.sin(2 * np.pi * 1730 * t) * np.exp(-t / .01) * .5 + bp(noise(n), 1500, 5000) * np.exp(-t / .012) * .5
add('rim', 'rim', finish(x, .62))
n = int(.30 * SR); t = tt(n)
add('brush', 'brush', finish(bp(noise(n), 1200, 6500) * np.minimum(1, t / .015) * np.exp(-t / .07), .5))
n = int(.10 * SR); t = tt(n); add('shaker', 'shaker', finish(bp(noise(n), 3800, 7500, 2) * np.sin(np.pi * t / t[-1]) ** 3, .3))
n = int(.20 * SR); t = tt(n)
add('clave', 'clave bell', finish(np.sin(2 * np.pi * 2350 * t) * np.exp(-t / .04) + .4 * np.sin(2 * np.pi * 3790 * t) * np.exp(-t / .02), .4))
# sub bass: sine with a whisper of 2nd harmonic and a soft pluck on top (F2 = midi 41)
n = int(.9 * SR); t = tt(n); f = mid2f(41)
b = np.sin(2 * np.pi * f * t) + .22 * np.sin(4 * np.pi * f * t) + lp(saw(f, n), 500) * .35 * np.exp(-t / .06)
add('sub', 'sub bass', finish(np.tanh(1.3 * b * np.exp(-t / .55) * np.minimum(1, t / .004)), .95), 41)
# pad: breathes in over one dotted beat (struck on every dotted beat = a soft 12/8 pulse). C4 = midi 60
n = int(BEAT * SR2); t = tt(n, SR2); f = mid2f(60)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-9, -3, 3, 9)) / 4 + .5 * np.sin(2 * np.pi * f / 2 * t)
x = lp(x, 1300, 2, SR2) * (.35 + .65 * np.minimum(1, t / (BEAT * .6)) ** 1.4)
add('pad', 'breath pad', finish(x, .6, .05, SR2), 60, rel=5)
# nylon guitar: Karplus-Strong (G3 = midi 55)
f = mid2f(55); L = int(round(SR / f)); n = int(1.0 * SR); y = np.zeros(n); y[:L] = lp(noise(L), 3000)
for i in range(L, n): y[i] = .996 * .5 * (y[i - L] + y[i - L - 1] if i - L - 1 >= 0 else y[i - L])
add('nylon', 'nylon', finish(lp(y, 3200) * np.exp(-tt(n) / .5), .72), 55)
# flute lead (C5): sine body, a little 2nd/3rd, breath noise on the attack, vibrato that blooms after 150 ms
n = int(.9 * SR2); t = tt(n, SR2); f = mid2f(72)
vib = f * (1 + .005 * np.sin(2 * np.pi * 5.2 * t) * np.clip((t - .15) / .25, 0, 1)); w = 2 * np.pi * np.cumsum(vib) / SR2
x = np.sin(w) + .25 * np.sin(2 * w) + .08 * np.sin(3 * w) + bp(noise(n), 1500, 4500, 2, SR2) * (.25 * np.exp(-t / .05) + .05)
x *= np.minimum(1, t / .035) * (.7 + .3 * np.exp(-t / .2)) * np.minimum(1, (n / SR2 - t) / .25)
add('flute', 'flute', finish(x, .75), 72, rel=5)
# marimba (C5) and vibes (C5): soft mallets
n = int(.5 * SR); t = tt(n); f = mid2f(72)
add('marimba', 'marimba', finish(np.sin(2 * np.pi * f * t) * np.exp(-t / .16) + .3 * np.sin(2 * np.pi * f * 4 * t) * np.exp(-t / .03), .7), 72)
n = int(1.3 * SR); t = tt(n); f = mid2f(72)
x = (np.sin(2 * np.pi * f * t) + .2 * np.sin(2 * np.pi * f * 3.98 * t) * np.exp(-t / .15)) * np.exp(-t / .6) * (1 + .25 * np.sin(2 * np.pi * 5.5 * t))
add('vibes', 'vibes', finish(x, .7), 72)
n = int(1.6 * SR); t = tt(n); f = mid2f(72)
x = np.sin(2 * np.pi * f * t) * np.exp(-t / .6) + .45 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t / .2) + .2 * np.sin(2 * np.pi * f * 5.4 * t) * np.exp(-t / .07)
add('bell', 'glass bell', finish(x, .68), 72)
# fx: a reverse swell (one bar), a soft sub drop, a wind
n = int(ROWS * ROW * SR2); t = tt(n, SR2); u = t / t[-1]
add('swell', 'reverse swell', finish(lp(hp(noise(n), 2000, 2, SR2), 5000, 2, SR2) * u ** 3, .45), None, rel=5)
n = int(1.4 * SR); t = tt(n); ph = 2 * np.pi * np.cumsum(38 + 50 * np.exp(-t / .25)) / SR
add('drop', 'sub drop', finish(np.sin(ph) * np.exp(-t / .6), .85))
KEYS = ['kick', 'rim', 'brush', 'shaker', 'clave', 'sub', 'pad', 'nylon', 'flute', 'marimba', 'vibes', 'bell', 'swell', 'drop']
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the music: one 24-row pattern = one bar of 12/8 (2 rows per eighth)
NOTE = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}
def nm(s): return NOTE[s[:-1]] + 12 * (int(s[-1]) + 1)
def xmnote(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49
def L(s): return [(int(a), b) for a, b in (w.split(':') for w in s.split())]
H = {  # harmony per bar type: pads per dotted beat, bass riff, nylon voices (from the original's channels 1+2), marimba arp
 'A': dict(pads=[('Ab3', 'C4', 'G4'), ('Ab3', 'C4', 'Eb4'), ('G3', 'Bb3', 'F4'), ('F3', 'Bb3', 'D4')],
           bass=L('0:F2 4:C3 6:C2 10:F2 12:Eb2 16:Eb3 18:Bb1 22:F2'),
           ny1=L('0:F4 2:F4 4:F4 6:C4 8:F4 10:F4 12:Eb4 14:G4 16:G4 18:Bb3 20:F4 22:F4'),
           ny2=L('2:Ab4 4:Ab4 8:Ab4 10:Ab4 14:Eb4 16:Eb4 20:D4 22:D4'),
           arp=L('0:F5 2:F5 4:F5 6:F5 8:F5 10:F5 12:G5 14:G5 16:G5 18:Eb5 20:Eb5 22:Eb5')),
 'B': dict(pads=[('F3', 'Ab3', 'C4'), ('G3', 'Bb3', 'D4'), ('F3', 'Ab3', 'Db4'), ('G3', 'Bb3', 'Eb4')],
           bass=L('0:Db2 3:Db2 6:Eb2 12:F2 15:F2 18:G2'),
           ny1=L('0:Db4 2:Db4 4:Db4 6:Eb4 12:Db4 14:Db4 16:Db4 18:Eb4'),
           ny2=L('2:Ab4 4:Ab4 6:G4 14:Ab4 16:Ab4 18:Bb4'),
           arp=L('0:Db5 1:Ab5 2:Db5 3:Ab5 4:Db5 5:Ab5 6:Eb5 12:F5 13:C6 14:F5 15:C6 16:F5 17:C6 18:Bb5')),
 'E2': dict(pads=[('F3', 'A3', 'C4')] * 4, bass=L('0:F2'), ny1=L('0:F4'), ny2=L('0:A4'), arp=[]),
 'E1': dict(pads=[('G3', 'Bb3', 'Eb4')] * 2 + [('F3', 'A3', 'C4')] * 2, bass=L('0:G2 12:F2'), ny1=L('0:Eb4 12:F4'), ny2=L('0:Bb4 12:A4'), arp=[]),
}
MEL = {   # the original's tunes (the flute plays them; ch 5 and ch 3 of the original)
 'M1': L('0:F4 2:Ab4 4:Ab4 6:C5 7:Ab4 8:Ab4 9:G4 10:F4 11:F4 12:Eb4 14:F4 16:G4 18:Bb4 19:F4 20:Bb4 21:F4 22:Bb4 23:F4'),
 'M2': L('0:C5 8:Db5 10:D5 12:Eb5 16:Bb4 18:Db5 20:C5 22:Bb4'),
 'M3': L('0:C5 2:F4 12:Eb5 13:F5 14:G5 15:Ab5 16:G5 17:F5'),
 'MB': L('0:Db5 4:F5 6:Eb5 12:Ab5 16:G5 18:Bb5'),
 'ME': L('0:Bb4 12:C5'),
 'MF': L('0:F4 6:A4 12:C5'),
}
CALL = L('0:F5 2:Ab5 4:C6 6:Eb6 12:G6')   # the bell call (Fm9 rising) that opens and closes the song
CLAVE = (0, 4, 8, 10, 14, 18, 22)          # bembe: x.x.xx.x.x.x in eighths

def build(p):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    h = H[p.get('h', 'A')]
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS: P[r][ch] = (xmnote(midi, key) if midi is not None else 49, INST[key], 0x10 + max(0, min(64, int(vol))))
    for r in range(ROWS):
        if p.get('kick') and r in ((0, 12) if p['kick'] == 1 else (0, 9, 12, 21)): put(r, 0, 'kick', None, 60 if r in (0, 12) else 34)
        if p.get('rim') and r in (6, 18): put(r, 1, 'rim', None, 44)
        if p.get('rim') == 2 and r in (11, 23): put(r, 1, 'rim', None, 16)       # ghosts
        if p.get('shk') and r % 2 == 0: put(r, 2, 'shaker', None, (34 if r % 6 == 0 else 22) * p['shk'] / 2)
        if p.get('shk') == 2 and r % 2 == 1 and r % 6 == 5: put(r, 2, 'shaker', None, 12)
        if p.get('clave') and r in CLAVE: put(r, 3, 'clave', None, p['clave'] - (0 if r in (0, 14) else 8))
        if p.get('pad') and r % 6 == 0:
            for j, nt in enumerate(h['pads'][r // 6]): put(r, 5 + j, 'pad', nm(nt), p['pad'] - j * 3)
    if p.get('bass'):
        for r, nt in h['bass']: put(r, 4, 'sub', nm(nt), (60 if r % 6 == 0 else 42) * p['bass'] / 60)
    if p.get('ny'):
        for r, nt in h['ny1']: put(r, 8, 'nylon', nm(nt), p['ny'] - (0 if r % 6 == 0 else 8))
        for r, nt in h['ny2']: put(r, 9, 'nylon', nm(nt), p['ny'] - 10)
    if p.get('arp'):    # marimba: the original's ostinato, octave-jumping every other hit, with ghost notes between
        for r, nt in h['arp']:
            put(r, 12, 'marimba', nm(nt) - (12 if (r // 2) % 2 and len(h['arp']) == 12 else 0), p['arp'] - (0 if r % 6 == 0 else 10))
            if len(h['arp']) == 12 and r + 1 < ROWS and r % 6 == 4: put(r + 1, 12, 'marimba', nm(nt) + 7, 14)
    if p.get('mel'):
        mv = p.get('fv', 52)
        for r, nt in MEL[p['mel']]: put(r, 10, 'flute', nm(nt) + p.get('oct', 0), mv - (4 if r % 2 else 0))
        if p.get('echo', 1):   # ping-pong echo: a dotted eighth (3 rows) later, softer, on its own channel (panned opposite)
            for r, nt in MEL[p['mel']]: put(r + 3, 11, 'flute', nm(nt) + p.get('oct', 0), mv * .42)
    if p.get('ctr'):    # counter-lines on vibes
        for r, nt in MEL[p['ctr']]: put(r, 13, 'vibes', nm(nt), p['cv'])
    if p.get('call'):
        for r, nt in CALL: put(r, 14, 'bell', nm(nt) - 12 * p.get('callo', 0), p['call'] - r)
    if p.get('bmel'):   # a melody on the bell, an octave up, sparse
        for r, nt in MEL[p['bmel']]:
            if r % 2 == 0: put(r, 14, 'bell', nm(nt) + 12, p['bv'] - (r % 6))
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    for (r, vol) in p.get('roll', ()): put(r, 1, 'brush', None, vol)
    return P

# ---------------------------------------------------------------- arrangement (bars of 12/8, 3.2 s each), about 2:30
S = []
def bar(**kw): S.append(dict(kw))
G = dict(kick=1, rim=1, shk=1, bass=50, pad=40, ny=46)                       # the groove, light
F = dict(kick=2, rim=2, shk=2, clave=34, bass=58, pad=42, ny=48)             # the groove, full
ROLL = tuple((r, 10 + r) for r in range(12, 24, 1))
# intro: the bell call over breathing pads, then the nylon figure
bar(pad=36, call=50); bar(pad=38, bmel='M1', bv=40)
bar(pad=38, ny=40, call=44, callo=1); bar(pad=40, ny=44, shk=1, fx=((0, 'swell', 40),))
# A1: the melody arrives (original patterns 3 3 3 3)
for i in range(4): bar(**G, mel='M1', fv=50, fx=((0, 'drop', 44),) if i == 0 else ())
# bridge (4 4)
bar(**G, h='B', mel='MB', fv=52, arp=40); bar(**G, h='B', mel='MB', fv=54, arp=42, roll=ROLL)
# A2: the counter-lines (5 6 5 7 twice), vibes and marimba
for i, (c, m) in enumerate((('M2', None), (None, None), ('M2', None), ('M3', None)) * 2):
    kw = dict(F, arp=36)
    if i >= 4: kw['clave'] = 38
    if c: kw.update(ctr=c, cv=48)
    if i == 7: kw.update(fx=((0, 'swell', 44),))
    bar(**kw)
# A3: the peak (8 9 x4): melody + echo + arp + full drums
for i in range(8):
    kw = dict(F, mel='M1', fv=56, arp=34, ctr='M2' if i % 2 else None, cv=34)
    if i == 0: kw['fx'] = ((0, 'drop', 50),)
    if not kw['ctr']: del kw['ctr']
    bar(**kw)
# bridge again, higher flute
bar(**F, h='B', mel='MB', fv=56, arp=44, oct=12); bar(**F, h='B', mel='MB', fv=58, arp=46, oct=12, roll=ROLL)
# break (12 13 x2): drums out, the bell sings M2 over pads and sub, then the call
bar(pad=40, bass=46, bmel='M2', bv=46, fx=((0, 'drop', 46),)); bar(pad=40, bass=46, ny=38, ctr='M3', cv=40)
bar(pad=40, bass=48, bmel='M2', bv=48, shk=1); bar(pad=42, bass=50, ny=42, call=48, shk=1, fx=((0, 'swell', 46),), roll=ROLL)
# A4: last time, flute an octave up for the first four, then home
for i in range(8):
    kw = dict(F, mel='M1', fv=56, arp=36, oct=12 if i < 4 else 0)
    if i == 0: kw['fx'] = ((0, 'drop', 52),)
    if i % 4 == 3: kw.update(ctr='M3', cv=36)
    bar(**kw)
# outro (3 x n, 4, 14 15): thinning out, then the Picardy F major
bar(**G, mel='M1', fv=44, echo=1); bar(pad=38, ny=42, shk=1, bass=44, bmel='M1', bv=38)
bar(pad=36, ny=40, h='B', bass=44, arp=34); bar(pad=36, bass=44, ny=40, h='E1', mel='ME', fv=48, call=40, callo=1)
bar(pad=32, h='E2', ny=34, bass=40, bmel='MF', bv=40)                              # ...and it rings out on F major
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
    hdr = b'Extended Module: ' + b'Spanish Flexicode'[:20].ljust(20) + b'\x1a' + b'make_flexicode_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
