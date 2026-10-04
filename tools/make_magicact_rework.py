#!/usr/bin/env python3
"""Builds tools/magic_act.xm : "The Magic Act", a long Singhs-style reworking of "The Dipper Man - The Magic Act"
(tools/the_dipper_man_the_magic_act.xm).
usage: python3 tools/make_magicact_rework.py        (run from the project root; needs numpy + scipy; re-running gives byte-identical output)

WHAT IS KEPT from the original, and what it becomes (the original is F minor, 125 BPM, four on the floor, ten channels):
  THE WHEEL   the arpeggio F G Ab C F C Ab G.  It is always degrees 1-2-3-5 of whatever scale it sits in, so it can change colour:
              F G Ab C (Nahawand), Eb F G Bb, C Db E G (Hijaz: the augmented second), Db Eb F Ab (Lydian), A Bb C# E (A Hijaz) ...
              Without its last note it has seven, which is exactly one bar of 7/8.
  THE HOOK    Ab Ab G G F | Eb Eb D D C | Db Db Eb Eb F | Eb Eb F F G | E E F G.  Written here as scale degrees, so the same tune
              can be heard in Hijaz (E E Db Db C ...), in Lydian, slowed, doubled, in the choir.
  THE LADDER  the bridge: two voices in parallel thirds, Fm-Cm-Bb-Eb (ii vi V I in Eb).  Here it climbs: Eb, F, Ab (Wilson-style lifts).
  THE CADENCE F Nahawand -> C with an E natural (= C Hijaz).  The original stops on a lone F; this one ends on F MAJOR (A natural).
The colour is Singhs': a soft takht (oud, qanun, ney, strings, a frame drum, a quiet darbuka) plus an otherworldly layer made of voices,
glass bowls, an ondes-like sine, a gong and a tanpura, and no twinkle (no music box, no major-pentatonic sparkle).
The form is a suite of modules that never repeat themselves (a Brian Wilson song is built like that: a refrain that keeps coming back
in a new key, a new meter, a new colour): Curtain (free time) | Wheel I, II (4/4) | Ladder (up) | Bazaar (7/8, Hijaz) | Floating (12/8,
Lydian) | Duet (oud and ney, slow) | Sleight of hand (fast, A Hijaz) | Voices (a cappella) | Return (F, then up to G) | Procession (9/8) |
Climb | Reveal (F major), about 6:47.
The grid is 0.04 s a row: a pulse is 12 rows (125 BPM), cut in 2 (8ths, 6 rows), 3 (triplets, 4 rows), 4 (16ths, 3 rows); other modules
use a pulse of 8, 16 or 24 rows (metric modulation by 3:2, 3:4, 1:2).
Channels: 0 kick / gong | 1 frame drum | 2 darbuka | 3 riq / small percussion | 4 bass | 5 oud | 6 kora / qanun (the Wheel) | 7 the Wheel's partner
          8 9 10 pad voices (strings, choir) | 11 lead | 12 second lead | 13 counter-line / cello | 14 bells / glass | 15 tanpura / fx
          (the a cappella part spreads its seven voices over 4 8 9 10 11 12 13).  tools/xm2gba.py seats them (magic_pan) and sets the level.
Checks run on every build: every pitched note must belong to its bar's scale or chord (else it is printed), and notes far outside an
instrument's sampled range are reported.
"""
import os, sys, struct, math
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
import make_aimandshoot_rework as A          # its drums, oud and qanun: the same hands, the same room
from make_flexicode_rework import tt, lp, hp, bp, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "magic_act.xm")
SPEED, BPM, NCH = 2, 125, 16               # 2 * 2.5 / 125 = 0.04 s a row
ROW = SPEED * 2.5 / BPM
rng = np.random.default_rng(2610)
def noise(n): return rng.uniform(-1, 1, n)

# ================================================================ synthesis helpers (correct pitch: the phase is the integral of the frequency)
def vibf(f, t, rate=5.0, cents=8.0, delay=.2, ramp=.4, ph=0.0, bend=0.0, bend_t=.08):
    """frequency track: f with a vibrato that blooms after `delay`, and an optional scoop (bend cents -> 0 in bend_t seconds)"""
    d = np.clip((t - delay) / ramp, 0, 1)
    c = cents * np.sin(2 * np.pi * rate * t + ph) * d + bend * np.exp(-t / bend_t)
    return f * 2 ** (c / 1200)
def saw_f(freq, sr, ph0=0.0, kmax=40):
    """band-limited saw from an instantaneous frequency array"""
    ph = 2 * np.pi * np.cumsum(freq) / sr + ph0; K = max(1, min(kmax, int(.45 * sr / float(np.max(freq))))); y = np.zeros(len(freq))
    for k in range(1, K + 1): y += np.sin(k * ph) / k
    return y * (2 / np.pi)
def sine_f(freq, sr, ph0=0.0): return np.sin(2 * np.pi * np.cumsum(freq) / sr + ph0)
def fadein(t, a, p=1.0): return np.minimum(1, t / a) ** p
def fadeout(t, dur, r): return np.minimum(1, np.maximum(0, (dur - t) / r))
def level(x, rms, win=.35, sr=SR, peak=.95):
    """scale x so that its loudest `win` seconds have the given RMS (equal volume numbers then sound about equally loud); if that would pass
    `peak`, a soft limiter (tanh) carries the rest, so a spiky pluck keeps its body instead of becoming a click"""
    w = max(8, int(win * sr)); x = np.asarray(x, float)
    def r(y): return math.sqrt((np.convolve(y ** 2, np.ones(w) / w, 'valid') if len(y) > w else np.array([np.mean(y ** 2)])).max() + 1e-12)
    g = rms / r(x); y = x * g
    for _ in range(10):
        if np.abs(y).max() <= peak: break
        y = peak * np.tanh(g * x / peak); g *= (rms / r(y)) ** .8
    return y * min(1.0, peak / np.abs(y).max())
def shape_tail(x, sr, tail=.015):
    k = max(8, int(tail * sr)); x = x.copy(); x[-k:] *= np.linspace(1, 0, k) ** 2; x[:3] *= np.array([.2, .5, .8]); return x
def ks(f, dur, sr, damp, bright, seed):
    """Karplus-Strong pluck, tuned: the loop delay is L + 1/2 samples, so L is rounded down and the result is resampled onto the exact pitch"""
    g = np.random.default_rng(seed); L = int(sr / f - .5); n = int(dur * sr * 1.03); y = np.zeros(n); y[:L] = lp(g.uniform(-1, 1, L), bright, 2, sr)
    for i in range(L, n): y[i] = damp * .5 * (y[i - L] + y[i - L - 1])
    ratio = f / (sr / (L + .5)); idx = np.arange(int(n / ratio)) * ratio
    return np.interp(idx, np.arange(n), y)[:int(dur * sr)]

# ================================================================ the instruments
I = {}
def add(k, name, x, gen=None, rel=12, pre=0.0, rms=.16, win=.35, peak=.95, sr=None):
    """x at SR (rel 12) or SR2 (rel 5).  gen = the midi pitch the sample sounds at (None: unpitched).  pre = seconds the sound needs to reach its
    body (pads that bloom slowly are scheduled that much early so that they land on the beat)"""
    sr = sr or (SR if rel == 12 else SR2)
    I[k] = dict(name=name, x=shape_tail(level(x, rms, win, sr, peak), sr), gen=gen, rel=rel, pre=pre)
RELSR = {12: SR, 5: SR2}

# ---- percussion: the takht's (Aim and Shoot's own samples, so the room is the same) + a frame drum, a gong
for k in ('kick', 'dum', 'tek', 'riq', 'swell', 'bass'):
    d = A.I[k]; I[k] = dict(name=d['name'], x=d['x'], gen=d['gen'], rel=d['rel'], pre=0.0)
I['swell']['pre'] = 1.2
n = int(.55 * SR); t = tt(n)                                                                   # frame drum, centre stroke (low, round, a ring of skin)
x = (np.sin(2 * np.pi * np.cumsum(104 * (1 + .45 * np.exp(-t / .025))) / SR) * np.exp(-t / .17)
     + (.22 * np.sin(2 * np.pi * 205 * t) + .12 * np.sin(2 * np.pi * 318 * t)) * np.exp(-t / .11)
     + bp(noise(n), 600, 3200) * np.exp(-t / .014) * .45 + hp(noise(n), 6000) * np.exp(-t / .07) * (t > .012) * .07)
add('daf', 'frame drum', x, None, rms=.17, win=.2)
n = int(.2 * SR); t = tt(n)                                                                    # frame drum, edge (dry, short)
add('daf_tek', 'frame drum edge', bp(noise(n), 1700, 6500) * np.exp(-t / .022) + np.sin(2 * np.pi * 410 * t) * np.exp(-t / .035) * .55, None, rms=.09, win=.08)
n = int(5.2 * SR2); t = tt(n, SR2); y = np.zeros(n); g = np.random.default_rng(77)            # gong / tam-tam: inharmonic partials that bloom
for r, a, dc in ((1, 1, 3.4), (1.47, .7, 2.9), (2.09, .6, 2.6), (2.56, .55, 2.2), (3.13, .45, 1.8), (3.9, .35, 1.4), (4.83, .3, 1.0), (5.41, .22, .8), (6.7, .15, .6)):
    f1 = mid2f(40) * r; y += a * np.sin(2 * np.pi * f1 * t * (1 + .0004 * np.sin(2 * np.pi * g.uniform(.3, 1.1) * t)) + g.uniform(0, 6.28)) * np.exp(-t / dc) * fadein(t, .09 + .03 * r, 1.3)
y += bp(noise(n), 300, 2400, 2, SR2) * np.exp(-t / .5) * fadein(t, .12) * .25
add('gong', 'gong', lp(y, 3600, 2, SR2), None, 5, pre=.08, rms=.15, win=.6)
n = int(1.5 * SR); t = tt(n); ph = 2 * np.pi * np.cumsum(41 + 55 * np.exp(-t / .22)) / SR            # sub drop (a low thump for the cuts)
add('drop', 'sub drop', np.sin(ph) * np.exp(-t / .55), None, rms=.2, win=.4)

# ---- plucks: oud (two registers), kora, qanun, a metallophone with a beating pair, a crystal
def oud(m, seed):
    f = mid2f(m); y = ks(f, 1.0, SR, .9965, 3500, seed); y = y + .5 * bp(y, 700, 1600) + hp(noise(len(y)), 3000) * np.exp(-tt(len(y)) / .004) * .22
    return np.tanh(1.3 * y) * np.exp(-tt(len(y)) / .75)
add('oud_lo', 'oud low', oud(43, 21), 43, rms=.10, win=.3); add('oud_hi', 'oud high', oud(55, 22), 55, rms=.10, win=.3)
f = mid2f(69); y = ks(f, 1.5, SR, .9972, 6500, 31); y2 = ks(f * 1.0016, 1.5, SR, .9970, 6000, 32)                                  # kora: two strings per note, glassy
y = (y + .8 * y2) * np.exp(-tt(len(y)) / 1.0); y = y + hp(noise(len(y)), 3500) * np.exp(-tt(len(y)) / .003) * .12
add('kora', 'kora', y, 69, rms=.075, win=.3)
f = mid2f(61); y = ks(f, .8, SR, .9972, 7500, 51) + .3 * ks(f * 2.003, .8, SR, .9965, 7500, 52)                                  # qanun: bright, many strings ringing
add('qanun', 'qanun', y * np.exp(-tt(len(y)) / .55), 61, rms=.075, win=.25)
n = int(2.7 * SR); t = tt(n); f = mid2f(62)                                                                                # metallophone: bars with a detuned pair (the beating)
x = (np.sin(2 * np.pi * f * t) + np.sin(2 * np.pi * f * 1.0065 * t)) * np.exp(-t / 1.15) * .6 + .22 * np.sin(2 * np.pi * f * 2.756 * t) * np.exp(-t / .18) + .08 * np.sin(2 * np.pi * f * 5.4 * t) * np.exp(-t / .05)
add('gender', 'metallophone', x * np.minimum(1, t / .002), 62, rms=.09, win=.3)
n = int(2.6 * SR); t = tt(n); f = mid2f(76)                                                                                # crystal: inharmonic partials, a slow shimmer
x = sum(a * (np.sin(2 * np.pi * f * r * t) + np.sin(2 * np.pi * f * r * 1.004 * t)) * np.exp(-t / dc) for r, a, dc in ((1, 1, 1.5), (2.32, .45, .8), (3.75, .22, .4), (5.9, .1, .2)))
add('crystal', 'crystal', x * np.minimum(1, t / .003), 76, rms=.08, win=.3)

# ---- winds: ney (two registers), duduk
def ney(m, dur, br):
    n = int(dur * SR2); t = tt(n, SR2); f = mid2f(m); fr = vibf(f, t, 5.0, 14, .3, .3, 0, bend=-30, bend_t=.06); w = np.cumsum(fr) * 2 * np.pi / SR2
    x = np.sin(w) + .14 * np.sin(2 * w) + .05 * np.sin(3 * w) + bp(noise(n), 1200, 4500, 2, SR2) * (br * np.exp(-t / .08) + .09)
    return x * fadein(t, .07) * fadeout(t, dur, .35)
add('ney', 'ney', ney(77, 2.0, .35), 77, 5, rms=.15, win=.4); add('ney_lo', 'ney low', ney(65, 2.2, .5), 65, 5, rms=.15, win=.4)
n = int(2.6 * SR2); t = tt(n, SR2); f = mid2f(62); fr = vibf(f, t, 4.7, 13, .45, .5, 1.0)                                  # duduk: a warm double reed, nasal peaks near 1.1 and 2.4 kHz
y = saw_f(fr, SR2) + .8 * saw_f(fr * 1.0012, SR2, 1.7)
y = lp(y, 3000, 2, SR2) + .55 * bp(y, 900, 1400, 2, SR2) + .35 * bp(y, 2000, 2800, 2, SR2) + bp(noise(n), 1500, 4000, 2, SR2) * .07
add('duduk', 'duduk', y * fadein(t, .1) * fadeout(t, 2.6, .4), 62, 5, pre=.03, rms=.15, win=.4)

# ---- bowed strings: ensemble, solo violin(s), cello; a pizzicato
def ensemble(m, dur, att, lpf, cents, seed, vr=5.2, vc=7):
    g = np.random.default_rng(seed); n = int(dur * SR2); t = tt(n, SR2); f = mid2f(m); y = np.zeros(n)
    for c in cents: y += saw_f(vibf(f * 2 ** (c / 1200), t, vr + g.uniform(-.4, .4), vc, .25, .5, g.uniform(0, 6.28)), SR2, g.uniform(0, 6.28))
    y = lp(y / len(cents), lpf, 2, SR2)
    return y * fadein(t, att, 1.3) * fadeout(t, dur, .5)
add('strings', 'strings', ensemble(60, 3.6, .38, 2500, (-11, -5, 0, 6, 12), 5), 60, 5, pre=.28, rms=.12, win=.6)
x = ensemble(69, 2.2, .07, 4200, (-7, 0, 7), 6, 5.8, 9); n = len(x); x = x + bp(noise(n), 2500, 5000, 2, SR2) * .03 * fadein(tt(n, SR2), .05)
add('violins', 'violins', x, 69, 5, pre=.03, rms=.13, win=.4)
add('cello', 'cello', ensemble(48, 2.8, .13, 1700, (-5, 5), 8, 4.8, 8) + .5 * ensemble(48, 2.8, .13, 700, (0,), 9, 4.8, 6), 48, 5, pre=.06, rms=.14, win=.4)
f = mid2f(60); y = ks(f, .7, SR, .987, 4800, 41); y = y + .6 * bp(y, 400, 1100)
add('pizz', 'pizzicato', y * np.exp(-tt(len(y)) / .28), 60, rms=.08, win=.2)

# ---- voices: a choir (oo in two registers, ah), an ondes-like sine
FORM = {'oo': ((310, 70, 1.0), (870, 100, .30), (2250, 160, .05)), 'ah': ((740, 90, 1.0), (1100, 110, .55), (2500, 170, .18))}
def choir(m, vowel, dur, seed):
    g = np.random.default_rng(seed); n = int(dur * SR2); t = tt(n, SR2); f = mid2f(m); y = np.zeros(n)
    for c in (-13, -5, 4, 12): y += saw_f(vibf(f * 2 ** (c / 1200), t, 4.8 + g.uniform(-.5, .5), 9, .3, .6, g.uniform(0, 6.28)), SR2, g.uniform(0, 6.28))
    y = lp(y, 3200, 1, SR2); out = sum(gn * bp(y, max(60, fc - bw), min(fc + bw, SR2 * .45), 2, SR2) for fc, bw, gn in FORM[vowel])
    out = out + bp(noise(n), 1500, 3500, 2, SR2) * .03
    return out * fadein(t, .4, 1.5) * fadeout(t, dur, .6)
add('oo_lo', 'choir oo low', choir(50, 'oo', 3.6, 11), 50, 5, pre=.3, rms=.12, win=.6)
add('oo_hi', 'choir oo high', choir(62, 'oo', 3.6, 12), 62, 5, pre=.3, rms=.12, win=.6)
add('ah_hi', 'choir ah high', choir(64, 'ah', 3.6, 13), 64, 5, pre=.3, rms=.12, win=.6)
n = int(2.8 * SR2); t = tt(n, SR2); f = mid2f(72); fr = vibf(f, t, 5.2, 18, .35, .5, 0, bend=-45, bend_t=.09)                  # ondes: a sine with a little 2nd / 3rd, a scoop, a slow vibrato
w = np.cumsum(fr) * 2 * np.pi / SR2
add('ondes', 'ondes', (np.sin(w) + .22 * np.sin(2 * w) + .07 * np.sin(3 * w)) * fadein(t, .09) * fadeout(t, 2.8, .5), 72, 5, pre=.04, rms=.13, win=.4)

# ---- drones: a glass bowl (singing-bowl partials, slow bloom), a tanpura (a string with the buzz of the bridge)
n = int(5.0 * SR2); t = tt(n, SR2); f = mid2f(69); x = np.zeros(n); g = np.random.default_rng(91)
for r, a, bt in ((1, 1, 0), (2.71, .5, 1.3), (5.06, .2, 2.1)):
    x += a * (np.sin(2 * np.pi * f * r * t) + np.sin(2 * np.pi * f * r * 1.0035 * t + g.uniform(0, 6.28))) * fadein(t, .5 + bt * .15, 1.4)
add('bowl', 'glass bowl', x * np.exp(-t / 2.4) * fadeout(t, 5.0, .8), 69, 5, pre=.38, rms=.1, win=.8)
f = mid2f(53); y = ks(f, 6.0, SR, .99935, 2600, 3); n = len(y); t = tt(n); b = np.tanh(2.5 * y) - np.tanh(2.5 * lp(y, 500))
y = y + .55 * bp(b, 1700, 4300, 2) * np.minimum(1, t / 1.4) * np.exp(-t / 2.6)
add('tanpura', 'tanpura', y, 53, rms=.075, win=1.0)
# (the pad voices and the tanpura are long: they are played a little early, `pre`, and re-struck at phrase ends, like a breath or a bow)

KEYS = ['kick', 'dum', 'tek', 'riq', 'daf', 'daf_tek', 'gong', 'swell', 'drop', 'bass',
        'oud_lo', 'oud_hi', 'kora', 'qanun', 'gender', 'crystal',
        'ney', 'ney_lo', 'duduk', 'strings', 'violins', 'cello', 'pizz',
        'oo_lo', 'oo_hi', 'ah_hi', 'ondes', 'bowl', 'tanpura']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
assert len(KEYS) <= 32

# ================================================================ the score engine
import itertools, bisect
TL = {}          # (row, channel) -> (instrument key, midi or None, volume 1..64, free)   (a later write on the same cell wins)
WARN = []
BARS = []        # every bar, in order (for the checks)
NN = {'C': 0, 'C#': 1, 'Db': 1, 'D': 2, 'D#': 3, 'Eb': 3, 'E': 4, 'F': 5, 'F#': 6, 'Gb': 6, 'G': 7, 'G#': 8, 'Ab': 8, 'A': 9, 'A#': 10, 'Bb': 10, 'B': 11}
def nm(s): return NN[s[:-1]] + 12 * (int(s[-1]) + 1)          # 'F4' = 65, 'C4' = 60
def pc(s): return NN[s]
def near(p, centre):                                          # the pitch with pitch class p nearest to `centre`
    p %= 12; m = centre - ((centre - p) % 12)
    return m if centre - m <= 6 else m + 12
def put(row, ch, key, m=None, vol=40, early=True, free=False):
    d = I[key]; ri = int(round(row))
    if early and d['pre']: row = row - d['pre'] / ROW
    r = max(0, int(round(row)))
    if m is not None and d['gen'] is not None and not -15 <= m - d['gen'] <= 16: WARN.append(('range', key, m, r))
    TL[(r, ch)] = (key, m, max(1, min(64, int(round(vol * VOLK[0] * TRIM.get(key, 1.0))))) if vol > 1 else 1, free, ri)
VOLK = [1.0]     # the level of the module being written (set by build())
TRIM = {'daf': 1.3, 'daf_tek': 1.35, 'dum': 1.2, 'tek': 1.3, 'kora': .85, 'ondes': .85}     # the mix, measured A-weighted against Aim and Shoot
def cut(row, ch, key, m=None):
    """a note-off (the player has none): the same sound restarted at volume 1, which is silent"""
    if (int(round(row)), ch) not in TL: TL[(int(round(row)), ch)] = (key, m, 1, True, int(round(row)))

MODES = dict(min=(0, 2, 3, 5, 7, 8, 10), nah=(0, 2, 3, 5, 7, 8, 11), maj=(0, 2, 4, 5, 7, 9, 11), hij=(0, 1, 4, 5, 7, 8, 10), lyd=(0, 2, 4, 6, 7, 9, 11),
             mix=(0, 2, 4, 5, 7, 9, 10), dor=(0, 2, 3, 5, 7, 9, 10), loc=(0, 1, 3, 5, 6, 8, 10), ukr=(0, 2, 3, 6, 7, 9, 10), phr=(0, 1, 3, 5, 7, 8, 10), mel=(0, 2, 3, 5, 7, 9, 11), kar=(0, 1, 4, 5, 7, 8, 11))
def deg(mode, tonic_m, k):
    """the k-th scale degree (0 = tonic, may be negative or above 6) of `mode` above the midi pitch tonic_m"""
    o, i = divmod(k, 7); return tonic_m + 12 * o + MODES[mode][i]
CH = {'m': (0, 3, 7), 'M': (0, 4, 7), 'm7': (0, 3, 7, 10), 'M7': (0, 4, 7, 11), 'M9': (0, 4, 7, 11, 14), '7': (0, 4, 7, 10), 'sus2': (0, 2, 7),
      'sus4': (0, 5, 7), 'aug': (0, 4, 8), 'dim': (0, 3, 6), 'm9': (0, 3, 7, 10, 14), 'add9': (0, 4, 7, 14), '6': (0, 4, 7, 9), 'mM7': (0, 3, 7, 11),
      'M7s11': (0, 4, 7, 11, 18), 'm11': (0, 3, 7, 10, 14, 17), '5': (0, 7), 'M6': (0, 4, 7, 9), 'm6': (0, 3, 7, 9), 'hij': (0, 4, 7, 13), 'add2': (0, 2, 4, 7)}
def chord(root, kind='M'): r = NN[root] if isinstance(root, str) else root; return [(r + i) % 12 for i in CH[kind]]

class Bar:
    """one bar: start row t0, n rows, P rows per pulse; the local scale (tonic pitch class + mode) and chord (pitch classes), bass pitch class"""
    def __init__(s, t0, n, P, tonic, mode, ch=None, bass=None, extra=(), name=''):
        s.t0, s.n, s.P, s.tonic, s.mode = t0, n, P, NN.get(tonic, tonic) if isinstance(tonic, str) else tonic % 12, mode
        s.ch = list(ch) if ch else []; s.bass = s.tonic if bass is None else (NN[bass] if isinstance(bass, str) else bass % 12); s.name = name
        s.allow = {(s.tonic + i) % 12 for i in MODES[mode]} | set(s.ch) | {s.bass} | {(NN[e] if isinstance(e, str) else e) % 12 for e in extra}
        BARS.append(s)
    @property
    def beats(s): return s.n / s.P
    def r(s, pos): return s.t0 + pos * s.P                  # row of a position given in pulses
    def end(s): return s.t0 + s.n
    def tm(s, centre): return near(s.tonic, centre)         # tonic nearest to a pitch
    def dg(s, centre, k): return deg(s.mode, s.tm(centre), k)
def seq(t0, n, P, specs):
    """specs: (tonic, mode, chord, bass) per bar; returns the bars and the end row"""
    out = []; t = t0
    for sp in specs:
        tonic, mode, ch = sp[0], sp[1], sp[2]; bass = sp[3] if len(sp) > 3 else None; ex = sp[4] if len(sp) > 4 else ()
        b = Bar(t, n, P, tonic, mode, ch, bass, ex); out.append(b); t += n
    return out, t

def voice_lead(prev, pcs, lo, hi, minsp=2):
    """one midi pitch for every pitch class of the chord inside [lo, hi], moving as little as possible from the previous voicing"""
    cands = [[m for m in range(lo, hi + 1) if m % 12 == p % 12] for p in pcs]; best = None
    for combo in itertools.product(*cands):
        c = sorted(combo)
        if len(set(c)) < len(c): continue
        if len(c) > 1 and min(b - a for a, b in zip(c, c[1:])) < minsp: continue
        cost = sum(abs(a - b) for a, b in zip(c, sorted(prev))) if prev else sum(abs(a - (lo + hi) / 2) for a in c)
        cost += .01 * (c[-1] - c[0])
        if best is None or cost < best[0]: best = (cost, c)
    if best is None: return voice_lead(prev, pcs, lo - 3, hi + 3, 0)
    return best[1]

# ---------------------------------------------------------------- the material of the original, as functions
WHEEL = (0, 1, 2, 4, 7, 4, 2, 1)           # F G Ab C F C Ab G: degrees 1 2 3 5 8 5 3 2
def wheel(bars, ch, key, vol, centre, step=.25, fig=WHEEL, phase=0, shift=0.0, oct=0, idx=0, accent=6, keep=None, vols=None, reset=False, rows_off=0):
    """the Wheel in sixteenths (step in pulses), on the local scale of every bar. `phase` continues from bar to bar (returned) unless reset;
       `idx` raises every degree (a 3rd above = 2), `oct` moves it, `shift` delays it (pulses), `keep(k, bar_i)` can drop notes"""
    ph = phase
    for bi, b in enumerate(bars):
        if reset: ph = 0
        k = 0
        while k * step < b.beats - 1e-9:
            i = fig[(ph + k) % len(fig)]
            if keep is None or keep(k, bi):
                m = b.dg(centre, i + idx) + 12 * oct; v = vol * (vols[bi] if vols else 1) + (accent if (ph + k) % len(fig) == 0 else 0)
                put(b.r(k * step + shift) + rows_off, ch, key, m, v)
            k += 1
        ph = (ph + k) % len(fig)
    return ph

# the hook, as scale degrees (index, alter) of the module's tonic scale, with sixteenth positions (a 4/4 bar = 16): H1..H5 and the pickups
HOOK = [dict(p=(0, 2, 3, 4, 6, 15), i=((2, 0), (2, 0), (1, 0), (1, 0), (0, 0), (-3, 0)), end=16),
        dict(p=(0, 2, 3, 5, 6), i=((-1, 0), (-1, 0), (-2, 1), (-2, 1), (-3, 0)), end=16),
        dict(p=(0, 2, 3, 4, 6, 15), i=((-2, 0), (-2, 0), (-1, 0), (-1, 0), (0, 0), (-2, 0)), end=16),
        dict(p=(0, 2, 3, 4, 6), i=((-1, 0), (-1, 0), (0, 0), (0, 0), (1, 0)), end=16),
        dict(p=(0, 1, 3, 5, 15), i=((-1, 1), (-1, 1), (0, 0), (1, 0), (-3, 0)), end=16)]
HOOKSEQ = (0, 1, 0, 1, 2, 3, 2, 3, 4)       # the nine bars of the original's A section
def hook_events(mode, tonic_m, cells=HOOKSEQ, alt=False, u=3, lens=None):
    """list of (bar_index, pos_in_sixteenths, midi, length_in_sixteenths) for the hook on `mode` with its tonic at midi tonic_m. alt: keep the
       original's chromatic alterations (D natural, E natural) -- right on F minor, wrong on anything else"""
    ev = []
    for bi, c in enumerate(cells):
        h = HOOK[c]
        for j, (p, (k, a)) in enumerate(zip(h['p'], h['i'])):
            m = deg(mode, tonic_m, k) + (a if alt else 0); nxt = h['p'][j + 1] if j + 1 < len(h['p']) else h['end']
            ev.append((bi, p, m, nxt - p))
    return ev

# ---------------------------------------------------------------- pads: voice-led chords
def pad(bars, chs, key, vol, lo, hi, hold=1, nv=None, vols=None, minsp=3, prev=None, order=0, fn=None, skip=()):
    """a chord for every `hold` bars on the channels `chs` (one per voice), the voices led smoothly from chord to chord.  Returns the last voicing."""
    pv = prev
    for bi in range(0, len(bars), hold):
        if bi in skip: continue
        b = bars[bi]; pcs = list(b.ch)[:nv or len(chs)] if fn is None else fn(b, bi)
        vc = voice_lead(pv, pcs, lo, hi, minsp); pv = vc
        for j, m in enumerate(vc):
            put(b.t0, chs[j % len(chs)], key, m, (vol * (vols[bi] if vols else 1)) - 2 * j)
    return pv

# ---------------------------------------------------------------- grooves: positions in pulses (a 4/4 bar = 4), volumes 0..64.  inst -> channel
PCH = {'kick': 0, 'gong': 0, 'drop': 0, 'daf': 1, 'daf_tek': 1, 'dum': 2, 'tek': 2, 'riq': 3}
def hits(bar, inst, items, lv=1.0, ch=None, m=None):
    for pos, v in items:
        if 0 <= pos < bar.beats - 1e-9: put(bar.r(pos), ch if ch is not None else PCH[inst], inst, m, v * lv, early=False)
def roll(bar, inst, start, stop, step, v0, v1, ch=None, m0=None, m1=None):
    """a roll / fill: hits from `start` to `stop` (pulses), growing from v0 to v1 (and sliding m0 -> m1 for a tuned sound)"""
    n = max(1, int(round((stop - start) / step)))
    for k in range(n):
        f = k / max(1, n - 1); pos = start + k * step
        if pos < bar.beats - 1e-9: put(bar.r(pos), ch if ch is not None else PCH[inst], inst, None if m0 is None else int(round(m0 + (m1 - m0) * f)), v0 + (v1 - v0) * f, early=False)

# ---------------------------------------------------------------- drones and fx
def tanpura_cycle(t0, t1, tonic_pc, chs=(15, 14), vol=22, period=87, fifth=7, oct=3, jit=0):
    """a tanpura: Pa Sa Sa Sa(low) plucked in turn, every ~0.9 s, each string ringing over the next (two channels alternate); period = rows of a cycle"""
    g = np.random.default_rng(1000 + tonic_pc + jit); t = t0; k = 0
    strings = ((fifth, 48), (0, 53), (0, 53), (0, 41))                                      # Pa, Sa, Sa, low Sa  (around C3 F3 F3 F2)
    while t < t1 - 20:
        for j, (iv, c) in enumerate(strings):
            row = t + j * period / 4 + g.uniform(-1.5, 1.5)
            m = near(tonic_pc + iv, c); m += 12 if m < 40 else 0
            if row < t1 - 10: put(row, chs[(k + j) % 2], 'tanpura', m, vol * (1.0 if j == 0 else .85 + .15 * g.uniform()), early=False)
        t += period; k += 4
    return t
def swell_to(row, ch=3, vol=36): put(row, ch, 'swell', None, vol, early=True)
def gong_at(row, vol=40, ch=0): put(row, ch, 'gong', None, vol, early=True)
def sweep(row, ch, key, mode, tonic_m, k0, k1, step_rows, v0, v1):
    """a run up (k1 > k0) or down the scale, degree by degree, a harp's glissando that stays in the mode"""
    ks_ = list(range(k0, k1 + 1)) if k1 >= k0 else list(range(k0, k1 - 1, -1)); n = len(ks_)
    for j, k in enumerate(ks_): put(row + j * step_rows, ch, key, deg(mode, tonic_m, k), v0 + (v1 - v0) * j / max(1, n - 1), early=False)
    return row + n * step_rows

def bassline(bars, ch, key, vol, centre, pat, approach=0.0, vols=None):
    """the bass: pat = [(pulse, semitones above the bar's bass note, volume factor)] (or a function of the bar index);
       approach = a scale step into the next bar's root, that many pulses before the bar line"""
    for bi, b in enumerate(bars):
        root = near(b.bass, centre); vv = vol * (vols[bi] if vols else 1)
        for pos, iv, vs in (pat(bi) if callable(pat) else pat):
            m = root + iv
            if m % 12 not in b.allow or (iv and b.ch and m % 12 not in b.ch):                  # a fixed interval that leaves the chord: take the nearest chord tone
                m = min((x for x in range(m - 4, m + 5) if x % 12 in (b.ch or b.allow)), key=lambda x: (abs(x - m), x))
            if pos < b.beats - 1e-9: put(b.r(pos), ch, key, m, vv * vs)
        if approach and bi + 1 < len(bars):
            nr = near(bars[bi + 1].bass, centre)
            if nr != root:
                c = [m for m in (nr - 2, nr - 1, nr + 1, nr + 2) if m % 12 in b.allow]
                if c: put(b.r(b.beats - approach), ch, key, min(c, key=lambda m: (abs(m - root), m > nr)), vv * .6)

def g_half(b, lv, g, fill=False, kick=True, riq=True):
    """the half-time takht groove: a soft kick, the frame drum on 3, a quiet maqsum on the darbuka (dum . tek tek dum . tek .), riq eighths"""
    if kick: hits(b, 'kick', [(0, 40)] + [(2.5, 22)] * (g.random() < .7) + [(3.75, 14)] * (g.random() < .3), lv)
    hits(b, 'daf', [(2, 34)], lv); hits(b, 'daf_tek', [(1, 13), (3, 15)] + [(3.5, 10)] * (g.random() < .5), lv)
    hits(b, 'dum', [(0, 26), (2, 21)], lv); hits(b, 'tek', [(1, 20), (1.5, 15), (3, 20)], lv)
    for pos in (.5, 2.5, 3.5, 1.75):
        if g.random() < .45: put(b.r(pos), 2, 'tek', 5, 9 * lv, early=False)            # ka: the light stroke, a fourth higher
    if riq: hits(b, 'riq', [(k * .5, 12 if k % 2 == 0 else 8) for k in range(8)], lv)
    if fill: roll(b, 'tek', 3, 4, .25, 12 * lv, 26 * lv, m0=0, m1=4)

A_SPECS = [('F', 'min', chord('F', 'm')), ('Eb', 'maj', chord('Eb', 'M')), ('F', 'min', chord('F', 'm')), ('Eb', 'maj', chord('Eb', 'M')),
           ('Db', 'maj', chord('Db', 'M')), ('Eb', 'maj', chord('Eb', 'M')), ('Db', 'maj', chord('Db', 'M')), ('Eb', 'maj', chord('Eb', 'M')),
           ('C', 'mix', chord('C', 'add9'))]          # the original's A section: Fm Eb Fm Eb | Db Eb Db Eb | C (add9, E natural)
STAB = (.5, .75, 1.5, 2.5, 2.75, 3.5)                    # the original's stab rhythm (its channel 5: sixteenths 2 3 6 10 11 14)
def sec(s): return s / ROW

# ================================================================ 0. CURTAIN: free time. A tanpura, a glass bowl, the ney alone (a taqsim in F Nahawand that
#    finds the Hijaz on C: C Db E F), then the Wheel on the kora, slow at first, speeding up until it IS the tempo of the next part.
TAQ = [  # seconds, note, length (None = let it ring), volume, instrument
    (2.6, 'F4', 1.5, 30, 'ney_lo'), (4.2, 'G4', .45, 26, 'ney_lo'), (4.65, 'Ab4', 1.3, 30, 'ney_lo'), (6.0, 'G4', .4, 24, 'ney_lo'), (6.4, 'F4', 1.9, 28, 'ney_lo'),
    (9.4, 'Ab4', .55, 28, 'ney_lo'), (9.95, 'Bb4', .4, 26, 'ney_lo'), (10.35, 'C5', 1.7, 32, 'ney_lo'), (12.1, 'Bb4', .35, 26, 'ney_lo'), (12.45, 'Ab4', .35, 26, 'ney_lo'),
    (12.8, 'G4', .5, 28, 'ney_lo'), (13.3, 'Ab4', .4, 26, 'ney_lo'), (13.7, 'G4', .35, 24, 'ney_lo'), (14.05, 'F4', 2.0, 28, 'ney_lo'),
    (17.6, 'C5', .7, 32, 'ney'), (18.3, 'Db5', 1.2, 34, 'ney'), (19.5, 'C5', .35, 28, 'ney'), (19.85, 'Db5', .3, 30, 'ney'), (20.15, 'E5', 1.6, 36, 'ney'),
    (21.8, 'F5', .6, 34, 'ney'), (22.4, 'E5', .4, 30, 'ney'), (22.8, 'Db5', .5, 30, 'ney'), (23.3, 'C5', 1.8, 32, 'ney'),
    (25.8, 'F5', .5, 34, 'ney'), (26.3, 'G5', .4, 34, 'ney'), (26.7, 'Ab5', 1.4, 38, 'ney'), (28.1, 'G5', .3, 32, 'ney'), (28.4, 'F5', .3, 32, 'ney'),
    (28.7, 'E5', .6, 34, 'ney'), (29.3, 'F5', .4, 32, 'ney'), (29.7, 'Db5', .5, 32, 'ney'), (30.2, 'C5', None, 34, 'ney'), (32.1, 'C5', None, 26, 'ney')]
def m_curtain(t0):
    L = 1100
    for k in range(0, L, 100): Bar(t0 + k, min(100, L - k), 12, 'F', 'nah', chord('F', 'm'))
    put(t0 + 2, 0, 'gong', None, 30, early=False)
    tanpura_cycle(t0 + 25, t0 + L, pc('F'), vol=20, period=92)
    for s_, n_, v_ in ((1.0, 'F4', 16), (16.8, 'C5', 18), (29.5, 'F4', 17), (38.0, 'Ab4', 15)): put(t0 + sec(s_), 13, 'bowl', nm(n_), v_)
    for s_, n_, d_, v_, k_ in TAQ:
        put(t0 + sec(s_), 11, k_, nm(n_), v_)
        if d_: cut(t0 + sec(s_ + d_), 11, k_, nm(n_))
    sweep(t0 + sec(16.3), 12, 'qanun', 'nah', nm('F3'), 0, 9, 2, 12, 24)                     # a qanun flourish up through the mode
    sweep(t0 + sec(24.3), 12, 'kora', 'nah', nm('F4'), 11, 4, 2, 20, 9)                      # and one falling, after the Hijaz
    t = t0 + L; d = 3.0; k = 0; marks = []                                                    # the Wheel, accelerating into the tempo
    while True:
        t -= d
        if t < t0 + 830: break
        marks.append((t, k)); d *= 1.13; k += 1
    for t, k in marks:
        put(t, 6, 'kora', deg('min', nm('F4'), WHEEL[(-1 - k) % 8]), 13 + 13 * (1 - k / len(marks)) + (5 if (-1 - k) % 8 == 0 else 0))
        if k % 8 == 7 and t > t0 + 950: put(t, 1, 'daf', None, 10 + 14 * (1 - k / len(marks)), early=False)
    roll(Bar(t0 + L - 96, 96, 12, 'F', 'nah'), 'riq', 0, 8, .25, 3, 14)                       # a riq shimmer opening up
    BARS.pop()
    swell_to(t0 + L, 3, 20)
    return t0 + L

# ================================================================ 1. WHEEL I: the original's A section, as it was (F minor, its D and E naturals), the hook on the ney,
#    the Wheel on the kora, the stab rhythm on the oud, a half-time takht groove.  Two bars of the Wheel alone first.
def m_wheel1(t0):
    pre, t = seq(t0, 48, 12, A_SPECS[:2]); bars, t1 = seq(t, 48, 12, A_SPECS); allb = pre + bars; g = np.random.default_rng(11)
    wheel(allb, 6, 'kora', 22, 65, vols=[.8, .9] + [1.0] * 9)
    tanpura_cycle(t0 + 6, t0 + 48 * 6, pc('F'), vol=14, period=92, jit=1)
    put(bars[0].t0, 13, 'bowl', nm('C5'), 15)
    hits(pre[0], 'riq', [(k * .5, 9 if k % 2 == 0 else 6) for k in range(8)]); hits(pre[0], 'daf', [(0, 24), (2, 16)])
    hits(pre[1], 'riq', [(k * .5, 10 if k % 2 == 0 else 7) for k in range(8)]); hits(pre[1], 'daf', [(0, 26), (2, 20)]); hits(pre[1], 'daf_tek', [(1, 10), (3, 12), (3.5, 9)])
    roll(pre[1], 'tek', 3, 4, .25, 8, 18, m0=0, m1=3)
    bassline(bars, 4, 'bass', 40, 40, [(0, 0, 1), (1.5, 0, .7), (3, 7, .6)], approach=.25)
    for bi, b in enumerate(bars):
        r = b.tm(54)
        for j, pos in enumerate(STAB): put(b.r(pos), 5, 'oud_hi', r + (12 if j in (1, 4) else 0), (24 if j % 3 == 0 else 19) * (.85 + .15 * bi / 8))
        g_half(b, .78 + .22 * bi / 8, g, fill=bi in (3, 7))
    for bi, p, m, l in hook_events('min', nm('F5'), alt=True):
        put(bars[bi].r(p * .25), 11, 'ney', m, 34 + (4 if l >= 6 else 0) - (3 if p % 2 else 0))
    b = bars[-1]; roll(b, 'dum', 2, 4, .5, 14, 30); roll(b, 'tek', 2.25, 4, .5, 12, 24)        # into the second time
    put(t1, 14, 'crystal', nm('C6'), 14); swell_to(t1, 3, 26)
    return t1

# ================================================================ 2. WHEEL II: the same nine bars, all of it moved: the hook low on the duduk (the violins join it an octave
#    up when the tune turns upward), the oud shadowing it a 64th ahead with tremolo, a cello counter-line against it, strings under, the qanun
#    interlocking with the kora (the Wheel a third higher, on the off sixteenths).
CELLO_A = [('C4', 'Ab3'), ('Bb3', 'G3'), ('Ab3', 'F3'), ('G3', 'Bb3'), ('Ab3', 'F3'), ('G3', 'Bb3'), ('Ab3', 'Db4'), ('Bb3', 'G3'), ('E3', 'G3')]
def m_wheel2(t0):
    bars, t1 = seq(t0, 48, 12, A_SPECS); g = np.random.default_rng(12)
    wheel(bars, 6, 'kora', 24, 65)
    wheel(bars, 7, 'qanun', 15, 53, idx=2, keep=lambda k, bi: k % 2 == 1, accent=0)
    pad(bars, (8, 9, 10), 'strings', 22, 53, 70, vols=list(np.linspace(.85, 1.15, 9)))
    for b, (a, c) in zip(bars, CELLO_A): put(b.r(0), 13, 'cello', nm(a), 25); put(b.r(2), 13, 'cello', nm(c), 23)
    bassline(bars, 4, 'bass', 42, 40, lambda bi: [(0, 0, 1), (.75, 0, .5), (1.5, 0, .75), (3, 7, .6)] if bi % 2 else [(0, 0, 1), (1.5, 0, .75), (2.5, 12, .5), (3, 7, .6)], approach=.25)
    for bi, p, m, l in hook_events('min', nm('F4'), alt=True):
        put(bars[bi].r(p * .25), 11, 'duduk', m, 38 + (4 if l >= 6 else 0) - (3 if p % 2 else 0))
        if bi >= 4: put(bars[bi].r(p * .25), 12, 'violins', m + 12, 30 + (3 if l >= 6 else 0))
        r0 = bars[bi].r(p * .25) - 1; put(r0, 5, "oud_hi", m - 12, 21, free=p == 0)          # the oud, a hair ahead
        if l >= 9:
            for k in range(3, l * 3 - 2, 3): put(r0 + k, 5, 'oud_hi', m - 12, 15 - k * .25)    # ...and its tremolo on the long notes
    for bi, b in enumerate(bars): g_half(b, .95 + .1 * bi / 8, g, fill=bi in (3, 7))
    for bi, n_ in ((0, 'C6'), (4, 'F6'), (8, 'E6')): put(bars[bi].t0, 14, 'crystal', nm(n_), 13)
    b = bars[-1]; roll(b, 'dum', 2, 4, .25, 12, 30); roll(b, 'daf_tek', 3, 4, .25, 10, 22)
    swell_to(t1, 3, 30)
    return t1

# ================================================================ 3. LADDER: the original's bridge (two voices in thirds over ii vi V I in Eb) climbing, Wilson-style: Eb, then F,
#    then Ab, every rung another colour (choir alone; strings and the kora; the full choir, the violins, the groove), then a hinge into C Hijaz.
LADDER = [(('Ab4', 'C5'), ('F4', 'Ab4')), (('G4', 'C5'), ('Eb4', 'G4')), (('D4', 'Bb4'), ('F4', 'F4')), (('G4', 'Bb4'), ('Eb4', 'G4'))]
def rung(t):
    t = NN[t] if isinstance(t, str) else t
    return [((t + 2) % 12, 'dor', chord(t + 2, 'm7')), ((t + 9) % 12, 'min', chord(t + 9, 'm7')), ((t + 7) % 12, 'mix', chord(t + 7, 'M'), (t + 11) % 12), (t, 'maj', chord(t, 'M7'))]
def upper(b, bi):
    c = list(b.ch); return c[1:4] if len(c) >= 4 else c[:3]                                 # the chord without its root (the bass has it)
def ladder_voices(bars, shift, parts, rhythm=(0, 1, 2), cells=LADDER):
    """parts: [(channel, instrument, octave, volume, which voice 0/1)]"""
    for b, (v1, v2) in zip(bars, cells):
        for ch, key, o, vol, w in parts:
            a, c = (v1, v2)[w]
            for j, pos in enumerate(rhythm): put(b.r(pos), ch, key, nm(a if j == 0 else c) + shift + 12 * o, vol - (2 if j else 0))
def m_ladder(t0):
    r1, t = seq(t0, 48, 12, rung('Eb')); r2, t = seq(t, 48, 12, rung('F')); r3, t = seq(t, 48, 12, rung('Ab'))
    hinge, t1 = seq(t, 48, 12, [('C', 'hij', chord('F', 'm'), 'C')]); g = np.random.default_rng(13)
    put(t0, 0, 'gong', None, 26, early=False); cut(t0, 13, 'cello')
    # rung 1: the choir alone, the cello walking under it, the kora in eighths
    ladder_voices(r1, 0, [(11, 'oo_hi', 0, 34, 0), (12, 'oo_hi', 0, 31, 1)])
    pv = pad(r1, (8, 9, 10), 'oo_lo', 26, 45, 62, fn=upper)
    bassline(r1 + r2 + r3, 4, 'cello', 30, 41, [(0, 0, 1), (2.5, 7, .7)], approach=.5)
    wheel(r1, 6, 'kora', 18, 65, step=.5, reset=True)
    for b in r1: hits(b, 'riq', [(k * .5, 9 if k % 2 == 0 else 6) for k in range(8)]); hits(b, 'daf', [(0, 20)])
    # rung 2: a whole step up. Strings take the chords, the violins the thirds, the kora in sixteenths, the metallophone interlocks
    ladder_voices(r2, 2, [(11, 'violins', 0, 32, 0), (12, 'violins', 0, 29, 1)], rhythm=(0, 1, 2, 3))
    pv = pad(r2, (8, 9, 10), 'strings', 24, 50, 68, fn=upper, prev=pv)
    wheel(r2, 6, 'kora', 20, 65, reset=True); wheel(r2, 7, 'gender', 14, 55, step=.5, idx=2, accent=0, reset=True)
    for b in r2:
        hits(b, 'kick', [(0, 28), (2.5, 16)]); hits(b, 'daf', [(2, 26)]); hits(b, 'daf_tek', [(1, 11), (3, 13)])
        hits(b, 'riq', [(k * .5, 11 if k % 2 == 0 else 7) for k in range(8)])
    # rung 3: up a minor third. The whole choir (ah) on the chords, the thirds sung and bowed, the takht groove
    ladder_voices(r3, 5, [(11, 'oo_hi', 0, 34, 0), (12, 'oo_hi', 0, 31, 1), (5, 'violins', 0, 26, 0), (13, 'violins', 0, 24, 1)], rhythm=(0, 1, 2.5))
    pv = pad(r3, (8, 9, 10), 'ah_hi', 27, 53, 72, fn=upper, prev=pv)
    wheel(r3, 6, 'kora', 22, 65, reset=True); wheel(r3, 7, 'gender', 15, 55, step=.5, idx=2, accent=0, reset=True)
    for bi, b in enumerate(r3): g_half(b, .9, g, fill=bi == 3)
    for bi, n_ in ((0, 'F6'), (2, 'Eb6')): put(r3[bi].t0, 14, 'crystal', nm(n_), 13)
    # the hinge: Fm over a C pedal (iv of C Hijaz), everything held, a qanun run up the Hijaz
    h = hinge[0]; pad(hinge, (8, 9, 10), 'ah_hi', 26, 53, 72, prev=pv)
    put(h.t0, 4, 'cello', nm('C3'), 32); put(h.t0, 11, 'oo_hi', nm('Ab4'), 30); put(h.t0, 12, 'oo_hi', nm('F4'), 27)
    hits(h, 'daf', [(0, 30)]); hits(h, 'dum', [(0, 26)]); put(h.t0, 0, 'drop', None, 22, early=False)
    sweep(h.r(.5), 13, 'qanun', 'hij', nm('C3'), 0, 14, 3, 12, 30)
    roll(h, 'tek', 3, 4, .25, 10, 24, m0=0, m1=4)
    return t1

# ================================================================ 4. BAZAAR: 7/8 in C Hijaz.  The Wheel without its last note is seven notes = one bar.  The bars lean 3+2+2
#    and 2+2+3 in turn.  The hook in Hijaz (E E Db Db C | Bb Bb Ab Ab G | Ab Ab Bb Bb C | Bb Bb C C Db): the ney, the qanun shadowing it.
#    Then a darbuka solo over the oud and a violin lament, then the hook again, big, and a stop.
G322, G223 = (0, 1.5, 2.5), (0, 1, 2)
def g_seven(b, bi, lv, g, kick=True, busy=0.0):
    gs = G322 if bi % 2 == 0 else G223
    if bi % 2 == 0: hits(b, 'dum', [(0, 34)], lv); hits(b, 'tek', [(1.5, 26), (2.5, 24)], lv); kas = (1.0, 3.0)
    else: hits(b, 'dum', [(0, 34), (2, 26)], lv); hits(b, 'tek', [(1, 24), (3, 20)], lv); kas = (2.5,)
    for pos in kas: put(b.r(pos), 2, 'tek', 5, 11 * lv, early=False)
    for k in range(14):                                                                       # the solo: sixteenths filled in by chance
        pos = k * .25
        if busy and (b.r(pos), 2) not in TL and g.random() < busy: put(b.r(pos), 2, 'tek', int(g.integers(0, 6)), (8 + 10 * g.random()) * lv, early=False)
    hits(b, 'daf', [(0, 30)], lv); hits(b, 'daf_tek', [(p, 15) for p in gs[1:]], lv)
    hits(b, 'riq', [(k * .5, 12 if k * .5 in gs else 7) for k in range(7)], lv)
    if kick: hits(b, 'kick', [(0, 34)], lv)
def m_bazaar(t0):
    H = ['C', 'C', 'C', 'C', 'C', 'Fm', 'C', 'Fm', 'Fm', 'Bbm', 'Fm', 'Bbm', 'Bbm', 'C', 'C', 'C', 'C', 'C', 'Fm', 'Fm', 'Bbm', 'C']
    CHS = {'C': [4, 7, 10], 'Fm': chord('F', 'm'), 'Bbm': chord('Bb', 'm')}                   # C7 (its root is the pedal), Fm, Bbm
    ROT = {'C': ('C', 'hij'), 'Fm': ('F', 'nah'), 'Bbm': ('Bb', 'ukr')}                   # the same seven notes, turned to start on the chord
    bars, t1 = seq(t0, 42, 12, [(ROT[h][0], ROT[h][1], CHS[h], 'C') for h in H]); g = np.random.default_rng(14)
    for bi, b in enumerate(bars):
        gs = G322 if bi % 2 == 0 else G223; last = bi == len(bars) - 1
        lv = (.75, .85, .95, 1)[min(bi, 3)]
        if not last:
            for k in range(7):                                                                 # the oud: the Wheel, one note an eighth
                put(b.r(k * .5), 5, 'oud_hi', b.dg(48, WHEEL[k]), (22 if 13 <= bi <= 16 else 25) + (7 if k * .5 in gs else 0))
            for k, pos in enumerate(gs): put(b.r(pos), 4, 'bass', nm('C2') if k == 0 or g.random() < .6 else nm('G1'), (40, 28, 28)[k])
            g_seven(b, bi, lv if bi < 13 else .9, g, kick=bi >= 4 and not 13 <= bi <= 16, busy=(0, .55)[13 <= bi <= 16])
        if 8 <= bi <= 12 or 17 <= bi <= 20: wheel([b], 6, 'kora', 17, 60, fig=WHEEL[:7], reset=True, accent=3)
    hook = hook_events('hij', nm('C5'))
    for bi, p, m, l in hook:
        b = bars[4 + bi]; pos = min(p, 13) * .25
        put(b.r(pos), 11, 'ney', m, 36 + (4 if l >= 6 else 0))
        r0 = b.r(pos) - 1; put(r0, 12, 'qanun', m - 12, 22, free=p == 0)
        if l >= 6:
            for k in range(3, min(l, 14 - min(p, 13)) * 3 - 1, 3): put(r0 + k, 12, 'qanun', m - 12, 16 - k * .3)
    pad(bars[4:13], (8, 9, 10), 'strings', 18, 55, 72)
    for bi, (n1, n2, p2) in zip(range(13, 17), (('G4', 'Ab4', 2), ('G4', 'F4', 1.5), ('E4', 'F4', 2), ('Db4', 'C4', 2.5))):   # the lament
        put(bars[bi].t0, 11, 'violins', nm(n1), 30); put(bars[bi].r(p2), 11, 'violins', nm(n2), 28)
    for bi, p, m, l in hook_events('hij', nm('C5'), cells=(0, 1, 2, 3)):                         # the hook again, big: violins, duduk an octave down
        b = bars[17 + bi]; pos = min(p, 13) * .25
        put(b.r(pos), 11, 'violins', m, 38); put(b.r(pos), 13, 'duduk', m - 12, 34); put(b.r(pos) - 1, 12, 'qanun', m - 12, 24, free=p == 0)
    pad(bars[17:21], (8, 9, 10), 'strings', 24, 55, 72)
    b = bars[-1]                                                                                  # the stop: the group starts only, then a run and the gong
    for pos in G223: put(b.r(pos), 4, 'bass', nm('C2'), 40); put(b.r(pos), 2, 'dum', None, 34, early=False); put(b.r(pos), 5, 'oud_hi', nm('C3'), 30)
    put(b.t0, 0, 'kick', None, 40, early=False); put(b.t0, 1, 'daf', None, 34, early=False)
    sweep(b.r(2.25), 12, 'qanun', 'hij', nm('C4'), 0, 6, 2, 16, 30)
    roll(bars[-2], 'tek', 2.5, 3.5, .25, 12, 26, m0=0, m1=4)
    put(t1, 15, 'swell', None, 1, early=False)
    return t1

# ================================================================ 4b. DUET: slow (a 24-row pulse), C Hijaz over a tanpura: the oud asks, the ney answers, then they play the hook's
#    head together, and the ney is left holding Db, the Hijaz's second degree, which the next part takes as its home (Db Lydian).
DUET = [(0, 'oud', ((0, 0, 2, 0), (2, 1, 1, 0), (3, 2, 1, 0), (4, 3, 2, 0), (6, 4, 4, 1), (10, 3, 1, 0), (11, 2, 1, 0), (12, 1, 2, 0), (14, 0, 2, 1))),
        (1, 'ney', ((0, 4, 6, 0), (6, 5, 2, 0), (8, 4, 2, 0), (10, 3, 2, 0), (12, 2, 4, 0))),
        (2, 'oud', ((0, 4, 1, 0), (1, 5, 1, 0), (2, 6, 1, 0), (3, 7, 1, 0), (4, 8, 6, 1), (10, 7, 1, 0), (11, 6, 1, 0), (12, 5, 1, 0), (13, 4, 1, 0), (14, 3, 1, 0), (15, 2, 1, 0))),
        (3, 'ney', ((0, 3, 4, 0), (4, 2, 2, 0), (6, 1, 2, 0), (8, 0, 8, 0))),
        (4, 'both', ((0, 2, 2, 0), (2, 2, 1, 0), (3, 1, 1, 0), (4, 1, 2, 0), (6, 0, 10, 1)))]
def m_duet(t0):
    bars, t1 = seq(t0, 96, 24, [('C', 'hij', chord('C', 'M'), 'C')] * 6); g = np.random.default_rng(21)
    tanpura_cycle(t0, t1, pc('C'), vol=20, period=100)
    put(t0, 13, 'bowl', nm('G4'), 18); put(bars[3].t0, 13, 'bowl', nm('C5'), 17)
    for bi, b in enumerate(bars):
        hits(b, 'dum', [(0, 24)] + [(2.5, 14)] * (bi > 0)); hits(b, 'tek', [(1.5, 10), (3, 14), (3.5, 9)] if bi else [])
        hits(b, 'daf', [(0, 20), (2, 12)])
        if bi in (1, 3) and g.random() < .9: put(b.r(2.75), 2, 'tek', 5, 8, early=False)
    def phrase(b, who, notes):
        for p, k, l, trem in notes:
            r = b.r(p * .25)
            if who in ('oud', 'both'):
                m = deg('hij', nm('C3'), k + (7 if who == 'both' else 0)); put(r, 5, 'oud_hi', m, 30)
                if trem:
                    for j in range(3, l * 6 - 2, 3): put(r + j, 5, 'oud_hi', m, 24 - j * .2)
            if who in ('ney', 'both'):
                put(r + (1 if who == 'both' else 0), 11, 'ney_lo' if who == 'ney' else 'ney', deg('hij', nm('C4' if who == 'ney' else 'C5'), k), 36 if l >= 4 else 32)
    for bi, who, notes in DUET: phrase(bars[bi], who, notes)
    for bi in (1, 3): cut(bars[bi].r(3.9), 11, 'ney_lo')
    b = bars[5]                                                                                  # the oud's C fades in tremolo, the ney finds Db
    for j in range(0, 60, 3): put(b.r(0) + j, 5, 'oud_hi', nm('C3'), 26 - j * .3)
    put(b.r(1), 11, 'ney', nm('C5'), 32); put(b.r(2), 11, 'ney', nm('Db5'), 38)
    put(b.r(2), 14, 'crystal', nm('Db6'), 12); swell_to(t1, 15, 18)
    return t1

# ================================================================ 5. FLOATING: 12/8, Db Lydian, half a step above the Bazaar.  The same pulse, cut in three.  The hook in slow
#    motion (each bar of it now two) on the ondes, a crystal echoing its long notes; the Wheel on a metallophone in triplets, eight notes
#    against twelve, so it lands somewhere new every bar, the kora locking into its gaps; choir chords, a bowl, a frame drum's heartbeat.
#    Its last chord, Bbm, is the hinge into A Hijaz (Bb -> A).
FLOAT = [('Db', 'lyd', chord('Db', 'M7')), ('Db', 'lyd', chord('Eb', 'M')), ('Bb', 'dor', chord('Bb', 'm9')), ('Ab', 'maj', chord('Ab', 'M'), 'C'),
         ('Bb', 'dor', chord('Bb', 'm7'), 'F'), ('Db', 'lyd', chord('Db', 'M'), 'F'), ('F', 'min', chord('F', 'm7')), ('Ab', 'maj', chord('Ab', 'M'), 'Eb'),
         ('Db', 'lyd', chord('Db', 'M7s11')), ('Db', 'lyd', chord('Eb', 'M')), ('Bb', 'dor', chord('Bb', 'm9')), ('Ab', 'maj', chord('Ab', 'M'), 'C'),
         ('G', 'loc', [7, 10, 5], 'G', ('Db',)), ('Eb', 'mix', chord('Eb', 'add9')), ('Bb', 'dor', chord('Bb', 'm9')), ('Bb', 'dor', chord('Bb', 'm'), 'Bb', ('A',))]
DUDUK_F = [('F3', 'Ab3'), ('G3', 'Bb3'), ('Db4', 'Bb3'), ('Eb4', 'C4'), ('Bb3', 'F3'), ('G3', 'F3'), ('Db4', 'F4'), ('Db4', 'Bb3')]
def m_floating(t0):
    bars, t1 = seq(t0, 48, 12, FLOAT); g = np.random.default_rng(15)
    third = 1 / 3
    wheel(bars, 6, 'gender', 22, 61, step=third, accent=4)                                    # 8 against 12: the phase runs on over the bar lines
    wheel(bars, 7, 'kora', 13, 63, step=third, shift=1 / 6, idx=2, accent=0, keep=lambda k, bi: k % 2 == 0)
    pad(bars, (8, 9, 10), 'oo_hi', 24, 55, 72, vols=[.85] * 4 + [1.0] * 8 + [1.1] * 4)
    for bi, b in enumerate(bars):
        put(b.t0, 4, 'cello', near(b.bass, 44), 28)
        if bi == len(bars) - 1:                                                            # Bb -> A: the door into A Hijaz (the ondes' Eb rises to E)
            put(b.r(2), 4, 'cello', nm('A2'), 32); put(b.r(2), 11, 'ondes', nm('E5'), 34, free=True); cut(b.r(2), 13, 'duduk')
            for ch, n_ in zip((8, 9, 10), ('A3', 'C#4', 'E4')): put(b.r(2), ch, 'oo_hi', nm(n_), 26, free=True)
        # the heartbeat: frame drum on the dotted beats, tek on the last triplet, riq whispering triplets
        hits(b, 'daf', [(0, 24), (2, 16)] + [(3 + 2 * third, 10)] * (g.random() < .5))
        if bi >= 4:
            hits(b, 'daf_tek', [(1, 10), (3, 12)]); hits(b, 'tek', [(1 + 2 * third, 13), (3 + 2 * third, 15)] + [(2 + 2 * third, 9)] * (g.random() < .4))
            if bi % 2 == 0: hits(b, 'kick', [(0, 22)])
        hits(b, 'riq', [(k * third, (9 if k % 3 == 0 else 5) * (1 if bi >= 2 else .7)) for k in range(12)])
    for bi, p, m, l in hook_events('lyd', nm('Db5'), cells=HOOKSEQ[:8]):                        # the hook, twice as slow
        b = bars[2 * bi]; r = b.t0 + p * 6
        put(r, 11, 'ondes', m, 36 + (3 if l >= 4 else 0))
        if l >= 4: put(r + 4, 12, 'crystal', m + 12, 12 + l * .5)
    for bi, (a, c) in enumerate(DUDUK_F): put(bars[8 + bi].t0, 13, 'duduk', nm(a), 27); put(bars[8 + bi].r(2), 13, 'duduk', nm(c), 25)
    for bi, n_ in ((0, 'Ab4'), (4, 'C5'), (8, 'F4'), (12, 'Db5')): put(bars[bi].t0, 14, 'bowl', nm(n_), 17)
    put(bars[8].t0, 15, 'swell', None, 22); put(bars[8].t0, 0, 'gong', None, 18, early=False)
    return t1

# ================================================================ 6. SLEIGHT OF HAND: A Hijaz, the pulse at 8 rows (half as fast again).  Pizzicato strings play the Wheel
#    (A Bb C# E), the hook goes to the violins, the qanun shadows them.  A trill (the original's A#-C alternation), then the trick: a bar
#    of nothing but a glass bowl.  Then the reveal: gong, a falling crystal run, and the second half of the tune, bigger.
def g_fast(b, lv, g, fill=False):
    hits(b, 'kick', [(0, 34), (2, 30)], lv); hits(b, 'daf', [(1, 24), (3, 26)], lv); hits(b, 'dum', [(0, 26), (2, 22)], lv)
    hits(b, 'tek', [(.75, 20), (1.5, 18), (2.75, 20), (3.5, 18)], lv)
    for pos in (.25, 1.25, 2.25, 3.25):
        if g.random() < .4: put(b.r(pos), 2, 'tek', 5, 9 * lv, early=False)
    hits(b, 'riq', [(k * .5, 12 if k % 2 else 8) for k in range(8)], lv)
    if fill: roll(b, 'tek', 3, 4, .25, 12 * lv, 28 * lv, m0=0, m1=5)
def m_sleight(t0):
    AH = {'A': chord('A', 'M'), 'Gm/A': chord('G', 'm'), 'Dm/A': chord('D', 'm'), 'Bb/A': chord('Bb', 'M')}
    intro, t = seq(t0, 32, 8, [('A', 'hij', AH['A'], 'A')] * 4)
    hh = ['A', 'Gm/A', 'A', 'Gm/A', 'Dm/A', 'Bb/A', 'Dm/A', 'Bb/A', 'A']
    hb, t = seq(t, 32, 8, [('A', 'hij', AH[h], 'A') for h in hh])
    van, t = seq(t, 40, 8, [('A', 'hij', AH['A'], 'A')])
    h2 = ['Dm/A', 'Bb/A', 'Dm/A', 'Bb/A', 'A', 'Gm/A', 'A', 'A']
    rb, t = seq(t, 32, 8, [('A', 'hij', AH[h], 'A') for h in h2])
    end, t1 = seq(t, 32, 8, [('A', 'hij', AH['A'], 'A')] * 2); g = np.random.default_rng(16)
    groove = intro + hb + rb
    wheel(intro + hb, 6, 'pizz', 24, 57, step=.5, vols=[.8, .9, 1, 1] + [1] * 9)
    wheel(rb, 6, 'pizz', 26, 57, step=.5)
    for bi, b in enumerate(groove):
        put(b.t0, 4, 'bass', nm('A1'), 40); put(b.r(2), 4, 'bass', nm('A1'), 30)
        for pos in (.75, 1.5, 2.75, 3.5): put(b.r(pos), 5, 'oud_lo', nm('A2'), 22)
        if b in intro and bi < 1: hits(b, 'dum', [(0, 26), (2, 20)]); continue
        g_fast(b, (.75, .85, .95)[min(bi, 2)] if bi < 3 else 1.0, g, fill=b in (hb[3], hb[7], rb[3]))
    for bi, b in enumerate(hb + rb): put(b.t0, 7, 'gender', near(b.ch[0], 62) + (12 if near(b.ch[0], 62) < 57 else 0), 17)
    for bi, p, m, l in hook_events('hij', nm('A4')):
        put(hb[bi].r(p * .25), 11, 'violins', m, 38); put(hb[bi].r(p * .25) - 1, 12, 'qanun', m - 12, 22, free=p == 0)
    b = hb[-1]                                                                                    # the trill, then the trick
    for k in range(10): put(b.r(2 + k * .2), 12, 'qanun', nm('Bb4') if k % 2 == 0 else nm('A4'), 16 + k * 1.6, early=False)
    v = van[0]
    for ch in (4, 5, 6, 7, 11, 12, 13): cut(v.t0, ch, 'pizz')
    put(v.t0 - 10, 14, 'bowl', nm('A4'), 22, early=False)
    hits(v, 'riq', [(3.5, 5), (4.0, 7), (4.5, 9)])
    r = rb[0]                                                                                     # the reveal
    put(r.t0, 0, 'gong', None, 34, early=False); put(r.t0, 1, 'drop', None, 30, early=False)
    sweep(r.t0, 14, 'crystal', 'hij', nm('A4'), 10, 0, 2, 22, 8)
    for ch, n_ in zip((8, 9, 10), ('A3', 'C#4', 'E4')): put(r.t0, ch, 'ah_hi', nm(n_), 28); cut(rb[1].t0, ch, 'ah_hi')
    cells = (2, 3, 2, 3, 0, 1, 0, 4)
    for bi, p, m, l in hook_events('hij', nm('A4'), cells=cells):
        put(rb[bi].r(p * .25), 11, 'violins', m, 40); put(rb[bi].r(p * .25), 13, 'duduk', m - 12, 32)
        put(rb[bi].r(p * .25) - 1, 12, 'qanun', m - 12, 22, free=p == 0)
    for bi in (1, 3, 5):                                                                          # the original's runs: up the Hijaz in sixteenths
        b = rb[bi]; sweep(b.r(2), 7, 'qanun', 'hij', nm('A3'), 0, 7, 2, 14, 28)
    e = end[0]                                                                                    # the stop: one chord, everyone
    for ch, key, n_, vol in ((4, 'bass', 'A1', 44), (5, 'oud_lo', 'A2', 34), (6, 'pizz', 'A3', 34), (7, 'gender', 'C#5', 24), (11, 'violins', 'A4', 38),
                             (12, 'qanun', 'E4', 30), (13, 'duduk', 'A3', 32), (8, 'ah_hi', 'A4', 28), (9, 'ah_hi', 'C#5', 26), (10, 'ah_hi', 'E5', 24)):
        put(e.t0, ch, key, nm(n_), vol)
    put(e.t0, 0, 'kick', None, 40, early=False); put(e.t0, 2, 'dum', None, 36, early=False); put(e.t0, 1, 'daf', None, 34, early=False)
    put(e.t0 + 2, 3, 'riq', None, 20, early=False); put(end[1].t0, 0, 'gong', None, 22, early=False)
    for ch in (4, 5, 6, 7, 11, 12, 13): cut(e.r(2.5), ch, 'pizz')
    return t1

# ================================================================ 7. VOICES: a cappella, slower (16-row pulse).  Seven parts: a bass, four inner voices, and on top two voices
#    in thirds that move up a third on the second beat, the way the original's bridge does.  F major, then down by fourths through
#    Lydian chords, then C7sus4, C7(b9) = the Hijaz on C, the door back to F minor.
VOX = [('F', 'maj', chord('F', 'M9')), ('D', 'dor', chord('D', 'm11')), ('Bb', 'lyd', chord('Bb', 'M7s11')), ('G', 'dor', chord('G', 'm9')),
       ('Eb', 'lyd', chord('Eb', 'M7s11')), ('Ab', 'maj', chord('Ab', 'M7')), ('Db', 'lyd', chord('Db', 'M7s11')), ('C', 'mix', chord('C', 'sus4') + [10]),
       ('C', 'hij', chord('C', '7') + [1]), ('C', 'hij', chord('C', '7') + [1])]
def m_voices(t0):
    bars, t1 = seq(t0, 64, 16, VOX); pv = None
    TGT = (69, 71, 72, 70, 70, 72, 74, 72, 71, 72)
    for bi, b in enumerate(bars):
        c4 = b.ch[:4]; cr = 1.0 + .25 * max(0, bi - 7)
        cands = sorted(m for m in range(62, 81) if m % 12 in c4); best = None
        for gap in (3, 2, 1):                                                                # thirds if the chord has them, else the closest it has
            for j in range(len(cands) - 2):
                a, c, d = cands[j:j + 3]
                if c - a >= gap and d - c >= gap:
                    sc = abs(a - TGT[bi])
                    if best is None or sc < best[0]: best = (sc, (a, c, d))
            if best: break
        a, c, d = best[1]
        for pos, (lo_, hi_) in ((0, (a, c)), (1, (c, d)), (2.5, (c, d))):
            if bi >= 8 and pos == 2.5: continue
            put(b.r(pos), 12, 'ah_hi', lo_, (28 if pos < 2 else 22) * cr); put(b.r(pos), 11, 'ah_hi', hi_, (31 if pos < 2 else 24) * cr)
        top = {a % 12, c % 12, d % 12}
        inner = [x for x in b.ch[1:5] if all((t_ - x) % 12 != 1 for t_ in top)]
        for x in (b.ch[0], b.ch[2 % len(b.ch)], b.ch[1]):
            if len(inner) < 4: inner.append(x)
        pv = voice_lead(pv, inner[:4], 50, 67, 2)
        for j, m in enumerate(pv): put(b.t0, (8, 9, 10, 13)[j % 4], 'oo_lo' if j < 2 else 'oo_hi', m, (25 - j) * cr)
        put(b.t0, 4, 'oo_lo', near(b.tonic, 42), 30 * cr)
        if bi == 7:                                                                          # the sus resolves inside the bar: F -> E
            for j, m in enumerate(pv):
                if m % 12 == 5: put(b.r(2), (8, 9, 10, 13)[j % 4], 'oo_lo' if j < 2 else 'oo_hi', m - 1, (25 - j) * cr, free=True)
    put(t0 + 4, 14, 'bowl', nm('A4'), 16)
    put(bars[8].t0, 0, 'gong', None, 22, early=False); put(bars[9].t0, 14, 'bowl', nm('C5'), 18)
    put(t1, 15, 'swell', None, 30); put(bars[9].r(3), 1, 'daf', None, 16, early=False); roll(bars[9], 'tek', 3.25, 4, .25, 8, 20, m0=0, m1=3)
    return t1

# ================================================================ 8. RETURN: home, the original's four on the floor at last, F minor, everyone: the hook on violins and ney in
#    octaves, the oud shadowing it, kora and qanun locked together, strings, a new cello line.  Then the whole thing lifts a step to G:
#    duduk and violins, the choir on the chords, a ney descant above.
def g_drive(b, lv, g, fill=False):
    hits(b, 'kick', [(0, 38), (1, 30), (2, 36), (3, 30)], lv); hits(b, 'daf', [(1, 24), (3, 26)], lv)
    hits(b, 'dum', [(0, 18), (2.5, 16)], lv)
    hits(b, 'tek', [(p * .25, 15 if p in (3, 6, 11, 14) else 10) for p in (2, 3, 5, 6, 8, 11, 13, 14)], lv)    # the original's hat pattern
    hits(b, 'riq', [(k + .5, 15) for k in range(4)] + [(k + .25 * j, 6) for k in range(4) for j in (1, 3) if g.random() < .5], lv)
    hits(b, 'daf_tek', [(1.5, 10), (3.5, 12)] if g.random() < .5 else [(3.5, 12)], lv)
    if fill: roll(b, 'tek', 3, 4, .25, 12 * lv, 28 * lv, m0=0, m1=5); hits(b, 'riq', [(3.75, 18)], lv)
def descant(bars, ch, key, vol, t_from, t_to, avoid=None):
    """long notes high above: chord tones nearest a slowly rising target, two a bar, never a semitone from a note of the tune in that half bar"""
    n = len(bars)
    for bi, b in enumerate(bars):
        for pos in (0, 2):
            tg = t_from + (t_to - t_from) * (bi + pos / 4) / n; bad = (avoid or {}).get((bi, pos), set())
            ok = [x for x in range(int(tg) - 6, int(tg) + 7) if x % 12 in b.ch and all((x - y) % 12 not in (1, 11) for y in bad)]
            if ok: put(b.r(pos), ch, key, min(ok, key=lambda x: abs(x - tg)), vol - (3 if pos else 0))
CELLO_R = ['Ab3', 'Bb3', 'C4', 'Bb3', 'Ab3', 'G3', 'F3', 'G3', 'E3']
def bass_drive(bars, vol, centre=40):
    pop = lambda bi: 12 if near(bars[bi].bass, centre) <= 41 else 7                           # the octave pop, or a fifth when the root sits high
    bassline(bars, 4, 'bass', vol, centre, lambda bi: [(0, 0, 1), (.75, 0, .6), (1.5, pop(bi), .55), (2, 0, .9), (2.75, 0, .6), (3.5, 7, .6)], approach=.25)
def m_return(t0):
    fb, t = seq(t0, 48, 12, A_SPECS)
    gb, t1 = seq(t, 48, 12, [((NN[x[0]] + 2) % 12, x[1], [(c + 2) % 12 for c in x[2]]) for x in A_SPECS]); g = np.random.default_rng(17)
    put(t0, 0, 'gong', None, 32, early=False); put(t0, 15, 'drop', None, 26, early=False)
    for bars, ctr in ((fb, 65), (gb, 67)):
        wheel(bars, 6, 'kora', 25, ctr); wheel(bars, 7, 'qanun', 16, ctr - 12, idx=2, keep=lambda k, bi: k % 2 == 1, accent=0)
        bass_drive(bars, 42, 40 + (ctr - 65))
        for bi, b in enumerate(bars): g_drive(b, .95 if bars is fb else 1.0, g, fill=bi in (3, 7))
        for bi, n_ in enumerate(CELLO_R): put(bars[bi].t0, 13, 'cello', nm(n_) + (ctr - 65), 25)
    pad(fb, (8, 9, 10), 'strings', 25, 53, 72)
    for bi, p, m, l in hook_events('min', nm('F4'), alt=True):
        r = fb[bi].r(p * .25); put(r, 11, 'violins', m, 38); put(r, 12, 'ney', m + 12, 33)
        put(r - 1, 5, 'oud_hi', m - 12, 21, free=p == 0)
    pad(gb, (8, 9, 10), 'ah_hi', 27, 55, 74); avoid = {}
    for bi, p, m, l in hook_events('min', nm('G4'), alt=True):
        r = gb[bi].r(p * .25); put(r, 11, 'duduk', m, 40); put(r, 5, 'violins', m + 12, 33)
        avoid.setdefault((bi, 0 if p < 8 else 2), set()).add(m % 12)
    descant(gb, 12, 'ney', 30, 79, 86, avoid)
    put(gb[0].t0, 0, 'gong', None, 30, early=False); put(gb[0].t0, 14, 'crystal', nm('D6'), 15); swell_to(gb[0].t0, 15, 30)
    b = gb[-1]; roll(b, 'dum', 2, 4, .25, 14, 32); roll(b, 'daf_tek', 3, 4, .25, 10, 24)
    return t1

# ================================================================ 9. PROCESSION: 9/8 (2+2+2+3), G Nahawand.  The Wheel with its tonic back on the end is nine notes = one bar.
#    A new tune made of old parts: the hook's rhythm with the Wheel's notes (D D Bb Bb G | A A Bb Bb D | Eb Eb D D Bb | A A G G F#).
PROC = [dict(p=(0, 2, 3, 4, 6, 15), i=((4, 0), (4, 0), (2, 0), (2, 0), (0, 0), (-3, 0)), end=18),
        dict(p=(0, 2, 3, 4, 6), i=((1, 0), (1, 0), (2, 0), (2, 0), (4, 0)), end=18),
        dict(p=(0, 2, 3, 4, 6, 15), i=((5, 0), (5, 0), (4, 0), (4, 0), (2, 0), (1, 0)), end=18),
        dict(p=(0, 2, 3, 4, 6, 10, 12), i=((1, 0), (1, 0), (0, 0), (0, 0), (-1, 0), (0, 0), (1, 0)), end=18)]
WHEEL9 = WHEEL + (0,)
def m_procession(t0):
    PC = [('G', 'nah', chord('G', 'm')), ('G', 'nah', chord('G', 'm')), ('Eb', 'lyd', chord('Eb', 'M'), 'Eb', ('C', 'D', 'F#')), ('D', 'hij', chord('D', 'M'), 'D', ('Bb',))]
    bars, t1 = seq(t0, 54, 12, PC * 4); g = np.random.default_rng(18)
    for bi, b in enumerate(bars):
        lv = .85 + .15 * min(1, bi / 4)
        for k in range(9): put(b.r(k * .5), 5, 'oud_hi', b.dg(55, WHEEL9[k]), 24 + (6 if k in (0, 2, 4, 6) else 0))      # (the chord's own 1-2-3-5)
        hits(b, 'dum', [(0, 32), (2, 24), (3, 22)], lv); hits(b, 'tek', [(1, 22), (3.5, 24), (4, 18)], lv)
        hits(b, 'tek', [(1.5, 10), (2.5, 9)], lv)
        hits(b, 'daf', [(0, 30), (3, 24)], lv); hits(b, 'daf_tek', [(1, 14), (2, 14)], lv)
        hits(b, 'riq', [(k * .5, 12 if k in (0, 2, 4, 6) else 7) for k in range(9)], lv)
        if bi >= 4: hits(b, 'kick', [(0, 34), (2, 24), (3, 26)], lv)
        put(b.t0, 4, 'bass', near(b.ch[0], 40), 40); put(b.r(3), 4, 'bass', near(b.ch[0], 40) + (7 if b.ch[0] != 2 else 0), 30)
        if bi % 4 == 3 and bi < 15: roll(b, 'tek', 4, 4.5, .125, 12, 24, m0=0, m1=4)
        pc0 = b.ch
        if bi >= 4: wheel([b], 6, 'kora', 18, 67, step=.25, fig=WHEEL9, reset=True, accent=3)
    G = nm('G4')
    for bi, b in enumerate(bars):
        c = PROC[bi % 4]
        for j, (p, (k, a)) in enumerate(zip(c['p'], c['i'])):
            m = deg('nah', G, k); r = b.r(p * .25)
            if bi < 12: put(r, 11, 'ney', m + 12, 36 if bi < 8 else 38)
            if 4 <= bi: put(r, 12, 'violins', m, 32 + 4 * (bi >= 12))
            if 8 <= bi: put(r, 13, 'duduk', m - 12 + 12 * (bi >= 12), 30)
    for bi in range(12, 16):                                                                     # the last four: the choir sings it, in thirds, the ney rests
        c = PROC[bi % 4]; b = bars[bi]
        for j, (p, (k, a)) in enumerate(zip(c['p'], c['i'])):
            m = deg('nah', G, k); h = max(x for x in range(m - 9, m - 2) if x % 12 in b.ch)
            put(b.r(p * .25), 11, 'ah_hi', h, 30)
    pad(bars[4:], (8, 9, 10), 'strings', 23, 55, 72, vols=[1] * 4 + [1.15] * 8)
    put(bars[8].t0, 0, 'gong', None, 26, early=False); put(bars[4].t0, 14, 'crystal', nm('G6') - 12, 14); put(bars[12].t0, 14, 'crystal', nm('D6'), 15)
    b = bars[-1]; roll(b, 'dum', 3, 4.5, .25, 14, 32)
    return t1

# ================================================================ 10. CLIMB: the bridge again, now with the band: the thirds sung and bowled over Bb, Bb again (the qanun's runs added),
#    then C, then C sus and C7(b9) with the original's trill, cut off.
def m_climb(t0):
    r1, t = seq(t0, 48, 12, rung('Bb')); r1b, t = seq(t, 48, 12, rung('Bb')); r2, t = seq(t, 48, 12, rung('C'))
    ext, t1 = seq(t, 48, 12, [('C', 'mix', chord('C', 'sus4') + [10]), ('C', 'hij', chord('C', '7') + [1])]); g = np.random.default_rng(19)
    put(t0, 0, 'gong', None, 28, early=False)
    ladder_voices(r1, 7, [(11, 'ah_hi', -1, 34, 0), (12, 'ah_hi', -1, 31, 1)])
    ladder_voices(r1b, 7, [(11, 'ah_hi', -1, 35, 0), (12, 'ah_hi', -1, 32, 1), (5, 'violins', 0, 28, 0), (13, 'violins', 0, 26, 1)], rhythm=(0, 1, 2, 3))
    ladder_voices(r2, -3, [(11, 'ah_hi', 0, 37, 0), (12, 'ah_hi', 0, 34, 1), (5, 'violins', 1, 30, 0), (13, 'violins', 1, 28, 1)], rhythm=(0, 1, 2.5))
    pv = pad(r1 + r1b + r2, (8, 9, 10), 'strings', 26, 53, 72, fn=upper, vols=[1] * 8 + [1.15] * 4)
    bassline(r1 + r1b + r2, 4, 'bass', 42, 40, [(0, 0, 1), (.75, 0, .55), (1.5, 0, .7), (2, 0, .9), (2.75, 0, .55), (3.5, 7, .6)], approach=.25)
    wheel(r1 + r1b + r2, 6, 'kora', 24, 65, reset=True)
    for bi, b in enumerate(r1 + r1b + r2): g_drive(b, .9 + .1 * (bi >= 4), g, fill=bi in (3, 7, 11))
    for b in r1b + r2: sweep(b.r(2), 7, 'qanun', b.mode, b.tm(53), 0, 7, 3, 14, 28)
    put(r2[0].t0, 0, 'gong', None, 34, early=False); put(r2[0].t0, 15, 'drop', None, 28, early=False); swell_to(r2[0].t0, 3, 34)
    descant(r2, 14, 'ney', 30, 79, 84)
    e1, e2 = ext
    pad(ext, (8, 9, 10), 'ah_hi', 30, 55, 74, prev=pv)
    put(e1.t0, 4, 'bass', nm('C2'), 44); put(e2.t0, 4, 'bass', nm('C2'), 44)
    put(e1.t0, 11, 'violins', nm('F5'), 36); put(e1.r(2), 11, 'violins', nm('E5'), 36, free=True); put(e2.t0, 11, 'violins', nm('Db5'), 38)
    put(e1.t0, 12, 'ney', nm('C6'), 30)
    g_drive(e1, 1.0, g)
    for k in range(16): put(e2.r(k * .25), 7, 'qanun', nm('C5') if k % 2 == 0 else nm('Bb4'), 14 + k * 1.3, early=False)   # the trill
    roll(e2, 'dum', 0, 4, .25, 12, 34); roll(e2, 'daf_tek', 2, 4, .25, 10, 28); swell_to(t1, 3, 38)
    return t1

# ================================================================ 11. REVEAL: C7(b9), the Hijaz, held; then F MAJOR (A natural, for the first time).  The Wheel in major (F G A C),
#    and the hook in major, slow, on the ney with the ondes an octave below: A A G G F | E E D D C | D D E E F | E E F F G.
#    It ends the way the original does, on a lone F.
REV = [('C', 'hij', chord('C', '7') + [1]), ('C', 'hij', chord('C', '7') + [1]), ('F', 'maj', chord('F', 'M9')),
       ('F', 'maj', chord('F', 'add9')), ('C', 'mix', chord('C', 'M')), ('F', 'maj', chord('F', 'M')), ('C', 'mix', chord('C', 'M'), 'E'),
       ('D', 'dor', chord('D', 'm7')), ('C', 'mix', chord('C', 'add9')), ('Bb', 'lyd', chord('Bb', 'M7')), ('C', 'mix', chord('C', 'sus4')),
       ('C', 'mix', chord('C', 'M')),
       ('G', 'dor', chord('G', 'm7')), ('D', 'min', chord('D', 'm7')), ('C', 'mix', chord('C', 'M'), 'E'), ('F', 'maj', chord('F', 'M7')),   # the tag
       ('F', 'maj', chord('F', 'M9')), ('F', 'maj', chord('F', 'add9'))]
def m_reveal(t0):
    bars, t1 = seq(t0, 64, 16, REV); g = np.random.default_rng(20)
    b0, b1, b2 = bars[:3]
    put(b0.t0, 0, 'gong', None, 36, early=False); put(b0.t0, 4, 'cello', nm('C2'), 34); put(b1.t0, 4, 'cello', nm('C2'), 30)
    pv = pad([b0, b1], (8, 9, 10), 'ah_hi', 30, 55, 74, fn=lambda b, bi: [4, 10, 1])
    sweep(b0.r(.5), 7, 'kora', 'hij', nm('C5'), 7, 0, 3, 24, 14); sweep(b0.r(2.25), 12, 'qanun', 'hij', nm('C4'), 7, 0, 3, 20, 12)
    put(b1.t0, 11, 'ondes', nm('Db5'), 34); put(b1.r(2), 11, 'ondes', nm('C5'), 32)
    put(b2.t0, 0, 'gong', None, 26, early=False); put(b2.t0, 14, 'crystal', nm('A5'), 20); put(b2.r(.5), 14, 'crystal', nm('C6'), 17); put(b2.r(1), 14, 'crystal', nm('F6'), 15)
    rest = bars[2:]
    pad(rest, (8, 9, 10), 'oo_hi', 26, 55, 72, prev=pv)
    for b in rest: put(b.t0, 4, 'cello', near(b.bass, 43), 30)
    wheel(rest[:-1], 6, 'kora', 19, 65, reset=False, vols=[1.1] + [.9] * (len(rest) - 4) + [.8, .7])
    tag = bars[12:16]
    ladder_voices(tag, 2, [(11, 'oo_hi', 0, 31, 0), (12, 'oo_hi', 0, 28, 1)])                    # the bridge's thirds one last time, in F major
    tanpura_cycle(b2.t0, t1, pc('F'), chs=(15, 3), vol=15, period=110)
    for bi, b in enumerate(rest[1:-2]):
        hits(b, 'daf', [(0, 22), (2.5, 14)]); hits(b, 'daf_tek', [(1, 8), (3, 9)])
    for bi, p, m, l in hook_events('maj', nm('F5')):
        b = bars[3 + bi]; r = b.r(p * .25)
        put(r, 11, 'ney', m, 36 + (3 if l >= 6 else 0)); put(r + 1, 12, 'ondes', m - 12, 26)
    for bi, n_ in ((3, 'C6'), (7, 'A5'), (11, 'G5')): put(bars[bi].t0, 14, 'crystal', nm(n_), 14)
    for b, n_ in ((bars[3], 'A4'), (bars[7], 'F4'), (bars[12], 'C5')): put(b.t0, 13, 'bowl', nm(n_), 16)
    f1, f2 = bars[-2:]                                                                            # F major, held; strings join; a lone F
    put(f1.t0, 11, 'strings', nm('A4'), 26); put(f1.t0, 12, 'strings', nm('E5'), 22); put(f1.t0, 0, 'gong', None, 20, early=False)
    for k, n_ in enumerate(('F5', 'A5', 'C6', 'E6')): put(f1.r(.5 + k * .75), 14, 'crystal', nm(n_), 16 - k)
    put(f2.r(2), 11, 'ondes', nm('F5'), 30); put(f2.r(2), 14, 'bowl', nm('F4'), 16)
    return t1

MODULES = [(m_curtain, 1.9), (m_wheel1, 1.0), (m_wheel2, 1.0), (m_ladder, 1.25), (m_bazaar, 1.0), (m_duet, 1.4), (m_floating, 1.45), (m_sleight, 1.0), (m_voices, 1.7),
           (m_return, 1.0), (m_procession, 1.05), (m_climb, 1.0), (m_reveal, 1.35)]          # (module, its level: the quiet ones are lifted to sit about 6 dB under the grooves)

# ================================================================ assembly, checks, the XM file
TAIL = 160
def build():
    TL.clear(); BARS.clear(); WARN.clear(); t = 0
    for m, k in MODULES: VOLK[0] = k; t = m(t)
    VOLK[0] = 1.0
    return int(round(t))
def check(verbose=True):
    """every pitched note must belong to the scale / chord of its bar (unless marked free); notes out of an instrument's good range"""
    starts = [b.t0 for b in BARS]; bad = []
    for (r, ch), (key, m, vol, free, ri) in sorted(TL.items()):
        if m is None or I[key]['gen'] is None or free or vol <= 1: continue
        i = bisect.bisect_right(starts, ri) - 1
        if i < 0: continue
        b = BARS[i]
        if m % 12 not in b.allow: bad.append((round(ri * ROW, 2), ch, key, m, b.name or (b.tonic, b.mode)))
    if verbose:
        for w in WARN[:20]: print('  WARN', w)
        for x in bad[:40]: print('  OUT OF SCALE', x)
    return bad
def main():
    total = build(); bad = check()
    R = total + TAIL; rows = [[(0, 0, 0)] * NCH for _ in range(R)]
    for (r, ch), (key, m, vol, free, ri) in TL.items():
        if r >= R: continue
        d = I[key]; n = 49 + (m - d['gen']) if (m is not None and d['gen'] is not None) else 49 + (m or 0)
        assert 1 <= n <= 96, (key, m, r)
        rows[r][ch] = (n, INST[key], 0x10 + vol)
    PL = 128; pats = [rows[i:i + PL] for i in range(0, R, PL)]
    F.I.clear(); F.I.update(I)
    uniq, order, seen = [], [], {}
    for P in pats:
        k = F.pat_bytes(P)
        if k not in seen: seen[k] = len(uniq); uniq.append(P)
        order.append(seen[k])
    assert len(uniq) <= 256 and len(order) <= 256, (len(uniq), len(order))
    hdr = b'Extended Module: ' + b'The Magic Act'[:20].ljust(20) + b'\x1a' + b'make_magicact_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(uniq), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in uniq) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    s = R * ROW
    print("wrote %s: %d orders, %d patterns, %d notes, %.1f s (%d:%02d), %d KB, %d out-of-scale" % (OUT, len(order), len(uniq), len(TL), s, s // 60, s % 60, os.path.getsize(OUT) // 1024, len(bad)))
if __name__ == '__main__': main()
