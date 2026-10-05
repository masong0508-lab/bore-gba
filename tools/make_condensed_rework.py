#!/usr/bin/env python3
"""Builds tools/condensed_music.xm : "Condensed Music", a Sk9m rework of "The Dipper Man - Condensed Music"
(tools/the_dipper_man_condensed_music.xm, a 58-second two-channel PixiTracker sketch).
usage: python3 tools/make_condensed_rework.py        (run from the project root; needs numpy + scipy; re-running gives byte-identical output)

KEPT, read straight out of the original XM (its lead sample sounds about a semitone under the written notes, so everything is taken
down one semitone: the original is heard in F minor, and so is this):
  THE RIFF     Db E . Db E . D .   a 3-3-2 (tresillo) figure on the minor 6th, the major 7th and the 6th: the spy-film line over F minor
  HOOK A       F Ab Db | C Bb G    (the call; the riff answers it, bar for bar, as in the original)
  HOOK B       Db . . . . Db F G | Ab G F Eb | C      (the tune of the middle section) and its turnaround  G . . G | C/E Db/F C/E
  THE BREAK    F . . Eb . . C .    (the drumless bar before the middle section, again 3-3-2)
  THE STEPS    Eb F G .. Ab        (the tapped run under every half bar, here a bass pick-up and data blips)
  THE ENDING   the low F with a snare roll that dies away, then single taps bouncing left and right
NEW: the harmony (Dbmaj7 C7 | Fm under hook A, Dbmaj7 Eb | Fm  and  Abmaj7 / C7b9 under hook B), the form, the groove and every sound.
It is UK garage at 132 BPM: 2-step kicks, swung 16ths (every second 16th is a row late), a warm organ stab on the riff, Rhodes chords,
a sub bass that walks with the kick, a vowel lead for hook B with a dotted-8th echo, a glass pluck for hook A, strings, bells,
blips, risers and a reverse cymbal.  Rows are 1/64 of a bar (speed 1, XM BPM 88 = 132 BPM): a 16th is 4 rows, the swing is 1 row.
Form (104 bars, about 3:09): intro 8 | verse 16 | chorus 16 | verse 16 | breakdown 8 | chorus 16 | riff jam 8 | last chorus 8 | outro 8
Channels: 0 kick | 1 snare / clap | 2 hats | 3 open hat / shaker | 4 bass | 5 riff organ | 6 7 8 Rhodes | 9 lead | 10 lead echo / harmony
          11 pluck arp / bell | 12 13 strings | 14 fx | 15 rim / blips.   tools/xm2gba.py seats them (condensed_pan) and sets the level.
"""
import os, sys, struct
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "the_dipper_man_condensed_music.xm")
OUT = os.path.join(HERE, "condensed_music.xm")
HIFI = int(os.environ.get('BORE_HIFI', '1'))   # 1 = the game's sample rates; tools/studio_render.py sets 4 to synthesise every sound at 4x the rate
SR = 16726 * HIFI                # a "relative note +12" sample plays at its natural pitch on C-4
SR2 = 8363 * 2 ** (5 / 12) * HIFI # relative note +5 (11163 Hz) for sounds with little top end
SR0 = 8363 * HIFI                # relative note 0, for long dark sounds
BPM_XM, SPEED, NCH, ROWS = 88, 1, 16, 64
BAR = ROWS * SPEED * 2.5 / BPM_XM          # 1.818 s
BEAT = BAR / 4
SWG = 1                                     # the swing: odd 16ths are this many rows late
rng = np.random.default_rng(1320)

# ---------------------------------------------------------------- synthesis helpers
def tt(n, sr=SR): return np.arange(n) / sr
def mid2f(m): return 440.0 * 2 ** ((m - 69) / 12)
def lp(x, fc, order=2, sr=SR): b, a = signal.butter(order, min(fc, sr * .45) / (sr / 2), 'low'); return signal.lfilter(b, a, x)
def hp(x, fc, order=2, sr=SR): b, a = signal.butter(order, fc / (sr / 2), 'high'); return signal.lfilter(b, a, x)
def bp(x, f1, f2, order=2, sr=SR): b, a = signal.butter(order, [f1 / (sr / 2), min(f2, sr * .45) / (sr / 2)], 'band'); return signal.lfilter(b, a, x)
def noise(n): return rng.uniform(-1, 1, n)
def saw(f, n, sr=SR, ph=0.0, vib=None):
    t = tt(n, sr); y = np.zeros(n); w = 2 * np.pi * (f * t if vib is None else np.cumsum(vib) / sr)
    for k in range(1, int(min(sr * .45 / f, 40)) + 1): y += np.sin(k * w + ph * k) / k
    return y * (2 / np.pi)
def square(f, n, sr=SR, vib=None):
    t = tt(n, sr); y = np.zeros(n); w = 2 * np.pi * (f * t if vib is None else np.cumsum(vib) / sr)
    for k in range(1, int(min(sr * .45 / f, 40)) + 1, 2): y += np.sin(k * w) / k
    return y * (4 / np.pi)
def verb(x, sr, t60, mix, pre=0.012, tone=4200):   # a small room / plate: the dry sound plus a decaying noise tail
    n = int(t60 * sr); t = tt(n, sr)
    ir = noise(n) * np.exp(-6.9 * t / t60); ir = lp(ir, tone, 2, sr); ir[:int(pre * sr)] = 0; ir /= np.sqrt((ir ** 2).sum())
    wet = np.concatenate([signal.fftconvolve(x, ir), [0.0]])
    y = np.concatenate([x, np.zeros(n)]); return y + wet * mix * np.abs(x).max() / (np.abs(wet).max() + 1e-9)
def finish(x, peak, tail=0.012, sr=SR):
    x = x / np.abs(x).max() * peak
    k = int(tail * sr); x[-k:] *= np.linspace(1, 0, k) ** 2
    k = len(x)
    while k > 64 and abs(x[k - 1]) < .004: k -= 1                  # drop the silent end
    return x[:k]

# ---------------------------------------------------------------- instruments
I = {}; KEYS = []
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel); KEYS.append(k)
# --- drums: a deep round kick, a garage snare with a clap in it, a rim, 808-ish metal hats, a shaker
n = int(.36 * SR); t = tt(n)
ph = 2 * np.pi * np.cumsum(47 + 105 * np.exp(-t / .032) + 260 * np.exp(-t / .004)) / SR
k = np.sin(ph) * np.exp(-t / .17) * np.minimum(1, t / .0015); k[:int(.003 * SR)] += hp(noise(int(.003 * SR)), 2000) * .35
add('kick', 'kick', finish(np.tanh(1.7 * k), .98))
n = int(.34 * SR); t = tt(n); c = np.zeros(n)
for off, dec, a in ((0, .005, .6), (.009, .005, .75), (.019, .006, .8), (.028, .085, 1.0)):
    i = int(off * SR); m = n - i; c[i:] += noise(m) * np.exp(-tt(m) / dec) * a
body = (np.sin(2 * np.pi * 182 * t) + .5 * np.sin(2 * np.pi * 331 * t)) * np.exp(-t / .045)
sn = bp(noise(n), 1800, 7500, 2) * np.exp(-t / .06)
add('snare', 'garage snare', finish(verb(bp(c, 900, 3600, 2) * .9 + sn * .7 + body * .55, SR, .35, .25), .82))
n = int(.09 * SR); t = tt(n)
add('rim', 'rim', finish(np.sin(2 * np.pi * 1690 * t) * np.exp(-t / .010) + .6 * np.sin(2 * np.pi * 820 * t) * np.exp(-t / .016) + hp(noise(n), 3000) * np.exp(-t / .003) * .5, .6))
def metal(n, sr=SR):   # six detuned squares, the classic cymbal metal
    t = tt(n, sr); return sum(np.sign(np.sin(2 * np.pi * f * t + rng.uniform(0, 6))) for f in (203, 298, 366, 431, 527, 802)) / 6
n = int(.055 * SR); t = tt(n)
add('chat', 'hat', finish(hp(metal(n) * .5 + noise(n), 7000, 4) * np.exp(-t / .011), .45))
n = int(.36 * SR); t = tt(n)
add('ohat', 'open hat', finish(hp(metal(n) * .55 + noise(n), 6200, 4) * np.exp(-t / .12), .45))
n = int(.085 * SR); t = tt(n)
add('shaker', 'shaker', finish(bp(noise(n), 4200, 8000, 2) * np.sin(np.pi * t / t[-1]) ** 2, .3))
# --- bass: a warm round garage bass (sine + a soft saw, slightly driven) at F2, long and short
for key, dur, rel_t in (('bass', .62, .16), ('bassS', .2, .05)):
    n = int(dur * SR); t = tt(n); f = mid2f(41)
    b = np.sin(2 * np.pi * f * t) * 1.15 + .3 * np.sin(4 * np.pi * f * t) + lp(saw(f, n), 520) * .55
    env = np.minimum(1, t / .003) * (.72 + .28 * np.exp(-t / .07)) * np.minimum(1, (dur - t) / rel_t)
    add(key, 'garage bass' if key == 'bass' else 'bass pop', finish(np.tanh(1.5 * b * env), .95), 41)
n = int(1.3 * SR0); t = tt(n, SR0); ph = 2 * np.pi * np.cumsum(32 + 62 * np.exp(-t / .25)) / SR0
add('drop', 'sub drop', finish(np.sin(ph) * np.exp(-t / .55) * np.minimum(1, t / .004), .95, .05, SR0), None, rel=0)
# --- the riff organ: a drawbar organ chop (C4), percussive, with a key click and a little chorus
n = int(.5 * SR); t = tt(n); f = mid2f(60)
org = sum(a * (np.sin(2 * np.pi * f * h * t) + np.sin(2 * np.pi * f * h * 1.0025 * t + 1)) for h, a in ((1, 1), (2, .75), (3, .45), (4, .35), (6, .18), (8, .12)))
org = lp(org, 4200) * np.minimum(1, t / .002) * (np.exp(-t / .13) + .18 * np.exp(-t / .5))
org[:int(.004 * SR)] += hp(noise(int(.004 * SR)), 2500) * .25
add('organ', 'organ stab', finish(verb(org, SR, .45, .22), .8), 60)
# --- Rhodes (C4): an FM tine; a long one for held chords, a short one for stabs, a muffled one for the intro
def rhodes(dur, dec, sr=SR):
    n = int(dur * sr); t = tt(n, sr); f = mid2f(60)
    x = np.sin(2 * np.pi * f * t + 1.1 * np.exp(-t / .3) * np.sin(2 * np.pi * f * t)) + .2 * np.sin(2 * np.pi * f * 14 * t) * np.exp(-t / .012)
    return hp(x * np.minimum(1, t / .002) * np.exp(-t / dec) * (1 + .06 * np.sin(2 * np.pi * 4.6 * t)), 170, 2, sr)   # thin the low end: the bass owns it
add('keys', 'rhodes', finish(verb(rhodes(1.25, .8), SR, 1.0, .3), .72, .05), 60)
add('keysS', 'rhodes stab', finish(verb(rhodes(.3, .12), SR, .5, .3), .72), 60)
add('keysM', 'rhodes muffled', finish(lp(verb(rhodes(1.25, .8, SR2), SR2, 1.0, .3, tone=2500), 850, 2, SR2), .72, .05, SR2), 60, rel=5)
# --- strings (C4): seven detuned saws, a slow swell, a long soft end
n = int(2.6 * SR0); t = tt(n, SR0); f = mid2f(60)
st = sum(saw(f * 2 ** (c / 1200), n, SR0, ph=rng.uniform(0, 6.28)) for c in (-14, -8, -3, 0, 3, 8, 14)) / 7
st = lp(st, 2300, 2, SR0) * np.minimum(1, t / .45) ** 1.5 * np.minimum(1, (t[-1] - t) / .9)
add('str', 'strings', finish(st, .6, .05, SR0), 60, rel=0)
# --- leads (C5): a vowel lead (saw + square through "ah" formants, vibrato that blooms), a glass pluck, a bell, an arp pluck, blips
n = int(1.5 * SR); t = tt(n); f = mid2f(72)
vib = f * (1 + .0055 * np.sin(2 * np.pi * 5.3 * t) * np.clip((t - .16) / .25, 0, 1))
src = saw(f, n, vib=vib) * .7 + saw(f * 1.003, n, vib=vib) * .45 + square(f, n, vib=vib) * .2
vox = bp(src, 650, 950, 2) * .85 + bp(src, 1050, 1350, 2) * .8 + bp(src, 2450, 2800, 2) * .45 + hp(src, 1600) * .15 + lp(src, 1800) * .35
vox *= np.minimum(1, t / .012) * (.8 + .2 * np.exp(-t / .1)) * np.exp(-t / 2.5) * np.minimum(1, (t[-1] - t) / .35)
add('vox', 'vowel lead', finish(verb(vox, SR, .8, .28), .78, .05), 72)
n = int(.8 * SR); t = tt(n); f = mid2f(72)
gl = np.sin(2 * np.pi * f * t + 2.2 * np.exp(-t / .07) * np.sin(2 * np.pi * 2 * f * t)) + .3 * np.sin(2 * np.pi * 4 * f * t) * np.exp(-t / .05)
add('glass', 'glass pluck', finish(verb(gl * np.exp(-t / .32) * np.minimum(1, t / .0015), SR, .9, .3), .7, .05), 72)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(72)
be = np.sin(2 * np.pi * f * t + 1.6 * np.exp(-t / .4) * np.sin(2 * np.pi * 3.5 * f * t)) * np.exp(-t / .7) + .25 * np.sin(2 * np.pi * 2.76 * f * t) * np.exp(-t / .15)
add('bell', 'bell', finish(verb(be * np.minimum(1, t / .002), SR2, 1.2, .3), .66, .05, SR2), 72, rel=5)
n = int(.22 * SR); t = tt(n); f = mid2f(72)
add('pluck', 'arp pluck', finish(lp(square(f, n) * .55 + saw(f * 1.004, n) * .45, 2900) * np.exp(-t / .065) * np.minimum(1, t / .001), .62), 72)
n = int(.13 * SR); t = tt(n); f = mid2f(72)
add('blip', 'blip', finish(np.sin(2 * np.pi * np.cumsum(f * (1 + .6 * np.exp(-t / .004))) / SR) * np.exp(-t / .045), .55), 72)
# --- fx: a crash, a reverse cymbal (one bar), a riser (two bars), an impact
n = int(1.5 * SR2); t = tt(n, SR2)
cr = hp(metal(n, SR2) * .4 + noise(n), 3200, 3, SR2) * np.exp(-t / .55)
add('crash', 'crash', finish(cr, .5, .05, SR2), None, rel=5)
n = int(BAR * SR2); t = tt(n, SR2)
sw = hp(metal(n, SR2) * .4 + noise(n), 3000, 3, SR2) * np.exp(-(t[-1] - t) / .5)
add('swell', 'reverse cymbal', finish(sw, .45, .01, SR2), None, rel=5)
n = int(2 * BAR * SR0); t = tt(n, SR0); u = t / t[-1]; nz = noise(n)
ri = bp(nz, 250, 900, 2, SR0) * (1 - u) + bp(nz, 1200, 3600, 2, SR0) * u + .25 * saw(1, n, SR0, vib=180 * 2 ** (3 * u)) * u
add('riser', 'riser', finish(ri * u ** 2.2, .5, .01, SR0), None, rel=0)
n = int(1.4 * SR2); t = tt(n, SR2); ph = 2 * np.pi * np.cumsum(30 + 70 * np.exp(-t / .08)) / SR2
im = np.sin(ph) * np.exp(-t / .5) + hp(noise(n), 1500, 2, SR2) * np.exp(-t / .25) * .45
add('impact', 'impact', finish(im, .9, .05, SR2), None, rel=5)
INST = {k: i + 1 for i, k in enumerate(KEYS)}

# ---------------------------------------------------------------- the original's notes
S0 = xm.parse(SRC)
T0 = -1   # the original lead plays about a semitone flat: what you hear is a semitone under the written notes
def notes_of(pat, insts=(6,), chans=(0, 1)):
    """[(row 0..7, midi)] of one 8-row pattern of the original (a pattern there is half a bar), transposed to what is heard"""
    out = []
    for r, row in enumerate(S0['pats'][pat]):
        for ch in chans:
            n, i = row[ch][0], row[ch][1]
            if 0 < n < 97 and i in insts: out.append((r, n + 11 + T0))
    return out
RIFF = notes_of(5, insts=(7,), chans=(0,))                 # Db E . Db E . D .
HOOKA = [notes_of(3, chans=(0,)), notes_of(4, chans=(0,))]  # F Ab . . Db | C . Bb G
HOOKB = [notes_of(p, chans=(0,)) for p in (7, 8, 9)]       # Db . . . . Db F G | Ab . G . F . Eb . | C
TURN = notes_of(11, chans=(0,))                            # G . . G
STING = notes_of(12)                                       # C/E . Db/F C/E
BREAK = notes_of(6, chans=(0,))                            # F . . Eb . . C .
ENDF = notes_of(13, chans=(0,))[0][1]                      # the low F
STEPS = [n + 11 for r, (n, i, *_) in enumerate(row[1] for row in S0['pats'][5]) if i == 8]   # Eb F G: the tapped run (that sample has no
assert [m % 12 for _, m in RIFF] == [1, 4, 1, 4, 2] and [m % 12 for _, m in BREAK] == [5, 3, 0]   # clear pitch, so its notes are kept as written)
assert [m % 12 for _, m in HOOKA[0] + HOOKA[1]] == [5, 8, 1, 0, 10, 7] and STEPS[:3] == [51, 53, 55]

# ---------------------------------------------------------------- harmony (per half bar), voicings and bass roots
CH = {'Dbmaj7': (37, (53, 56, 60)), 'C7': (36, (52, 55, 58)), 'C7b9': (36, (52, 58, 61)), 'Fm': (41, (56, 60, 65)), 'Fm9': (41, (56, 63, 67)),
      'Fm7': (41, (56, 60, 63)), 'Eb': (39, (55, 58, 63)), 'Ab': (44, (55, 60, 63)), 'Bbm7': (46, (53, 56, 61)), 'Gsus': (43, (53, 55, 60))}
STR = {'Dbmaj7': (60, 65), 'C7': (58, 64), 'C7b9': (58, 64), 'Fm': (60, 68), 'Fm9': (60, 67), 'Fm7': (60, 63), 'Eb': (58, 67), 'Ab': (60, 67), 'Bbm7': (61, 65), 'Gsus': (60, 62)}

# ---------------------------------------------------------------- the bar builder
def R(s, off=0): return s * 4 + (SWG if s % 2 else 0) + off      # 16th s (0..15) of the bar -> row, swung
def xmnote(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49
def build(b):
    P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
    def put(r, ch, key, midi=None, vol=64):
        if 0 <= r < ROWS and vol > 0: P[r][ch] = (xmnote(midi, key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, int(vol))))
    ch2 = b.get('chords', ('Fm', 'Fm'))
    # drums
    d = b.get('drums')
    if d:
        kicks = {'2step': (0, 10), '2stepB': (0, 7, 10), '4': (0, 4, 8, 12), '4x': (0, 4, 8, 12, 14), 'half': (0,), 'build': (0, 4, 8, 12)}[d]
        for s in kicks: put(R(s), 0, 'kick', None, 64 if s % 4 == 0 else 56)
        if d != 'half' and d != 'build': put(R(4), 1, 'snare', None, 52); put(R(12), 1, 'snare', None, 54)
        if b.get('ghost'): put(R(15), 1, 'snare', None, 16); put(R(9), 1, 'snare', None, 12)
        if b.get('hats'):
            for s in range(16):
                if s % 4 == 2: put(R(s), 2, 'chat', None, 44)
                elif s % 2 == 1 and b['hats'] >= 2: put(R(s), 2, 'chat', None, 22 if s % 4 == 1 else 28)
                elif s % 4 == 0 and b['hats'] >= 3: put(R(s), 2, 'chat', None, 18)
        if b.get('ohat'): put(R(14), 3, 'ohat', None, 30); put(R(6), 3, 'ohat', None, 24)
        if b.get('shaker'):
            for s in (1, 3, 5, 9, 11, 13): put(R(s), 3, 'shaker', None, 22)
        if b.get('rim'): put(R(7), 15, 'rim', None, 26); put(R(13), 15, 'rim', None, 20)
    for s, v in b.get('roll', ()): put(R(s // 2) + (2 if s % 2 else 0), 1, 'snare', None, v)
    # bass: a 2-step line that sits with the kick, one root per half bar; 'steps' adds the original's tapped run as a pick-up
    if b.get('bass'):
        for h in range(2):
            root = CH[ch2[h]][0]; s0 = h * 8
            for s, iv, v, k in ((0, 0, 52, 'bass'), (3, 0, 40, 'bassS'), (6, 12, 30, 'bassS')) if b['bass'] == 1 else \
                               ((0, 0, 54, 'bass'), (3, 0, 40, 'bassS'), (5, 12, 32, 'bassS'), (6, 0, 44, 'bass')):
                put(R(s0 + s), 4, k, root + iv, v)
        if b.get('steps'):
            for j, m in enumerate(STEPS[:3]): put(R(13 + j), 4, 'bassS', m - 12, 34 + j * 4)
    # the riff (Db E . Db E . D .) on the organ, twice a bar, the original's rows as 16ths
    if b.get('riff'):
        v = b['riff']
        for h in range(2):
            for r, m in RIFF: put(R(h * 8 + r), 5, b.get('riffinst', 'organ'), m + 12, v - (4 if r % 3 else 0))
    # Rhodes: a held chord on each change, 2-step stabs between them
    if b.get('keys'):
        v = b['keys']; inst = b.get('keysinst', 'keys')
        for h in range(2):
            for j, m in enumerate(CH[ch2[h]][1]):
                put(R(h * 8), 6 + j, inst, m, v - j * 3)
                if b.get('stabs'):
                    put(R(h * 8 + 3), 6 + j, 'keysS', m, v - 10 - j * 3); put(R(h * 8 + 6), 6 + j, 'keysS', m, v - 14 - j * 3)
    if b.get('strings'):
        for h in range(2):
            if h == 0 or ch2[1] != ch2[0]:
                for j, m in enumerate(STR[ch2[h]]): put(R(h * 8), 12 + j, 'str', m, b['strings'] - j * 4)
    # melodies: (pairs of half bars) on the lead channel, with a dotted-8th echo (3 16ths later) or a harmony a third/sixth below
    for h, notes in enumerate(b.get('mel', ((), ()))):
        for r, m in notes:
            s = h * 8 + r; v = b.get('melv', 56); k = b.get('lead', 'vox'); sh = b.get('oct', 12)
            put(R(s), 9, k, m + sh, v)
            if b.get('echo') and s + 3 < 16 and not any(rr == r + 3 for rr, _ in notes):
                put(R(s + 3), 10, k, m + sh, v * .42)
            if b.get('harm'): put(R(s), 10, k, m + sh - (4 if m % 12 in (0, 5, 7) else 3), v * .6)
    for s, m, v in b.get('arp', ()): put(R(s), 11, 'pluck', m, v)
    for s, m, v in b.get('bells', ()): put(R(s), 11, 'bell', m, v)
    for s, m, v in b.get('blips', ()): put(R(s), 15, 'blip', m, v)
    for r, key, v in b.get('fx', ()): put(r, 14, key, None, v)
    for r, ch, key, m, v in b.get('raw', ()): put(r, ch, key, m, v)
    return P

def arp_for(chords, v, octv=12, pattern=(0, 1, 2, 1)):
    out = []
    for s in range(16):
        tones = CH[chords[s // 8]][1]
        out.append((s, tones[pattern[s % len(pattern)]] + octv, v - (0 if s % 4 == 0 else 8)))
    return out

# ---------------------------------------------------------------- the arrangement (one dict per bar)
BARS = []
def bar(**kw): BARS.append(kw); return kw
VA = (('Dbmaj7', 'C7'), ('Fm', 'Fm'))                       # verse: hook bar, riff bar
HB = [('Dbmaj7', 'Eb'), ('Fm', 'Fm7'), ('Dbmaj7', 'Eb'), ('Ab', 'Fm'), ('Dbmaj7', 'Eb'), ('Fm', 'Fm9'), ('Dbmaj7', 'Eb'), ('Gsus', 'C7b9')]
HBM = [(HOOKB[0], HOOKB[1]), (HOOKB[2], ()), (HOOKB[0], HOOKB[1]), (HOOKB[1], HOOKB[2]),
       (HOOKB[0], HOOKB[1]), (HOOKB[2], HOOKB[2][:1]), (HOOKB[0], HOOKB[1]), (TURN, ())]
def verse(n, lead, melv, **kw):
    for i in range(n):
        hook = i % 2 == 0
        b = bar(chords=VA[i % 2], drums=kw.get('drums', '2step'), hats=kw.get('hats', 2), shaker=1, rim=kw.get('rim', 0), bass=kw.get('bass', 1),
                keys=kw.get('keys', 35), stabs=1, strings=kw.get('strings', 0), ohat=kw.get('ohat', 0), ghost=1,
                riff=0 if hook else kw.get('riff', 50), steps=not hook)
        if hook: b.update(mel=(HOOKA[0], HOOKA[1]), lead=lead, melv=melv, oct=12, echo=kw.get('echo', 1))
        if not hook and kw.get('blips'): b['blips'] = [(9, STEPS[0] + 24, 22), (11, STEPS[1] + 24, 24), (13, STEPS[2] + 24, 26)]
def chorus(n, harm=0, arp=0, drums='2step', fx0=True):
    for i in range(n):
        j = i % 8; ch = HB[j]
        b = bar(chords=ch, drums=drums if j != 7 else '2stepB', hats=3, ohat=1, shaker=1, ghost=1, bass=2, keys=33, stabs=1, strings=40,
                mel=HBM[j], lead='vox', melv=60, oct=12, echo=1 if not harm else 0, harm=harm, steps=j in (3, 5))
        if arp: b['arp'] = arp_for(ch, arp)
        if j == 7:   # the turnaround: G . . G, then the original's sting (C/E Db/F C/E) on Rhodes and organ, and a fill
            b['raw'] = [(R(8 + r), 6 + k % 2, 'keysS', m, 52) for k, (r, m) in enumerate(STING)] + \
                       [(R(8 + r), 5, 'organ', m, 50) for k, (r, m) in enumerate(STING) if k % 2 == 0]
            b['keys'] = 0; b['roll'] = tuple((s, 20 + (s - 24) * 3) for s in range(24, 32))
        if i == 0 and fx0: b['fx'] = ((0, 'crash', 50),)

# intro: muffled Rhodes and the riff, then the drums come up
for i in range(8):
    b = bar(chords=VA[i % 2], keys=44, keysinst='keysM', strings=30 if i >= 4 else 0)
    if i % 2: b.update(riff=40 if i >= 2 else 0, blips=[(9, STEPS[0] + 24, 18), (11, STEPS[1] + 24, 20), (13, STEPS[2] + 24, 22)] if i >= 2 else ())
    else: b.update(mel=(HOOKA[0], HOOKA[1]), lead='glass', melv=44 if i >= 4 else 0, oct=12, echo=1)
    if i >= 4: b.update(drums='half', hats=1, shaker=1)
    if i == 6: b['fx'] = ((0, 'riser', 46),)
    if i == 7: b['fx'] = ((0, 'swell', 44),); b['roll'] = tuple((s, 14 + (s - 16) * 2) for s in range(16, 32))
verse(16, 'glass', 52)
BARS[-1].update(drums=None, bass=0, riff=0, keys=0, steps=0, mel=(BREAK, ()), lead='vox', melv=56, oct=12, echo=1, strings=36,
                fx=((0, 'swell', 46),))                                                   # the break: F . . Eb . . C, no drums
chorus(16)
verse(16, 'vox', 50, bass=2, strings=34, ohat=1, rim=1, blips=1, hats=3)
BARS[-16]['fx'] = ((0, 'impact', 52),)
# breakdown: no drums; bells sing hook B over the strings, the riff on muffled Rhodes, then a build
for i in range(8):
    j = i % 8; ch = HB[j]
    b = bar(chords=ch, strings=44, keys=34 if i < 4 else 30, keysinst='keysM', bells=[(r + h * 8, m + 12, 46) for h, ns in enumerate(HBM[j]) for r, m in ns])
    if i == 0: b['fx'] = ((0, 'drop', 58),)
    if i >= 4: b.update(drums='build' if i >= 6 else 'half', bass=1 if i >= 6 else 0)
    if i == 6: b['fx'] = ((0, 'riser', 52),); b['roll'] = tuple((s, 12 + s // 2) for s in range(0, 32, 2))
    if i == 7: b['roll'] = tuple((s, 18 + s) for s in range(32)); b['fx'] = ((0, 'swell', 48),); b['bells'] = (); b['raw'] = []
chorus(16, harm=1, arp=34)
for b in BARS[-8:]: b['drums'] = '4' if b['drums'] == '2step' else b['drums']   # the second half on four to the floor
# riff jam: the riff over Fm with the organ doubled by the bass, hook A on the pluck as a counter
for i in range(8):
    b = bar(chords=('Fm', 'Fm') if i % 2 else ('Dbmaj7', 'C7'), drums='2stepB' if i % 4 == 3 else '2step', hats=3, ohat=1, shaker=1, rim=1, ghost=1,
            bass=2, riff=54, keys=30, stabs=1, steps=i % 2 == 1)
    if i % 2 == 0: b['arp'] = [(h * 8 + r, m + 12, 40) for h, ns in enumerate(HOOKA) for r, m in ns]
    if i == 0: b['fx'] = ((0, 'crash', 46),)
    if i == 7: b['fx'] = ((0, 'riser', 40),)
chorus(8, harm=1, arp=36, drums='4')
# outro: hook A twice more over the verse chords, then the original ending: the low F with a dying snare roll, taps bouncing away
for i in range(4):
    hook = i % 2 == 0
    b = bar(chords=VA[i % 2], drums='2step', hats=2, shaker=1, bass=1, keys=36, stabs=1, strings=34, riff=0 if hook else 44)
    if hook: b.update(mel=(HOOKA[0], HOOKA[1]), lead='glass', melv=48, echo=1)
for i in range(2):
    b = bar(chords=VA[i % 2], keys=32, keysinst='keysM', strings=30, riff=0 if i == 0 else 34, riffinst='organ')
    if i == 0: b.update(mel=(HOOKA[0], HOOKA[1]), lead='glass', melv=44, echo=1)
bar(chords=('Fm', 'Fm'), strings=34, keys=34, keysinst='keysM', mel=(((0, ENDF),), ()), lead='vox', melv=50, oct=12,
    roll=tuple((s, max(3, 40 - s)) for s in range(0, 32, 2)), bass=0)
bar(chords=('Fm9', 'Fm9'), raw=[(R(s), 14 if s % 4 == 0 else 15, 'rim', None, max(4, 30 - s * 2)) for s in range(0, 14, 2)] + [(0, 11, 'bell', 77, 34)])
# a quiet bar to let the tails ring before the song starts again
bar()

# ---------------------------------------------------------------- XM writer
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
    for sp in BARS:
        P = build(sp); key = pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    assert len(order) <= 256 and len(KEYS) <= 32
    hdr = b'Extended Module: ' + b'Condensed Music'[:20].ljust(20) + b'\x1a' + b'make_condensed_rewrk'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM_XM) + bytes(order).ljust(256, b'\0')
    body = b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    s = len(order) * BAR
    print("wrote %s: %d bars, %d patterns, %d instruments, %.1f s (%d:%02d), samples %d KB" % (
        OUT, len(order), len(pats), len(KEYS), s, s // 60, s % 60, sum(len(I[k]['x']) for k in KEYS) // 1024))
if __name__ == '__main__': main()
