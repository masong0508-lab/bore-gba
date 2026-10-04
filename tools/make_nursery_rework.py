#!/usr/bin/env python3
"""Builds tools/nursery_time.xm : "Nursery Time", a Singhs rework of "The Dipper Man - Nursery Time" (tools/the_dipper_man_nursery_time.xm).
usage: python3 tools/make_nursery_rework.py        (run from the project root; needs numpy + scipy; re-running gives byte-identical output)

It starts as an innocent little cartoon-techno ditty (a music box, a xylophone, a toy square lead, a bubbly bass, a soft 909, chalk
squeaks and boings), then something goes wrong, and it turns into hard rock, metal and nu-groove that seems to go on forever: a new
riff, a new tempo, a new trick every time, two endings that are not endings.

KEPT from the original (F major / D minor; the tune itself sits in A Phrygian, the metal mode):
  THE CHIME   the music box in sixths: D D D D E D | C C C C D C | Bb Bb Bb Bb C Bb | A A A D F G A  (over Bb/D Am/C Gm/D Am/C)
  THE TUNE    C C C D C | B E E | Bb Bb Bb C Bb | A D | C C C D C | B E E | A A A A Bb | C Bb A G      (over F E Bb A F E Am G)
  THE RUN     G A Bb C D E F G, twice
  THE END     the high F the original closes on
In the metal the tune's chords become power chords (F5 E5 Bb5 A5 ...), the chime is tremolo-picked, sung by a music box over a
nu-metal groove, and played as a stadium anthem; the run becomes a groove riff.  Singhs' colour comes in as Phrygian / Hijaz:
the E chord is the Hijaz on E, a break turns the tune into A Hijaz on an oud with a darbuka, and the guitars answer in kind.
Grid: 0.04 s a row (speed 2, BPM 125); a beat is 12 rows (125 BPM), 8 rows (187.5, the thrash), 16 rows (93.75, the nu-groove) or 24.
Channels: 0 kick | 1 snare / clap | 2 hats / ride | 3 crash / china / riser | 4 bass | 5 toms / woodblock / darbuka | 6 7 guitars left
          and right (the pad in the ditty) | 8 pad / clean guitar | 9 10 music box dyads / oud / xylophone | 11 lead | 12 lead harmony |
          13 scratches / boings / squeaks | 14 music box tune / bells | 15 fx.   tools/xm2gba.py seats them (nursery_pan) and sets the level.
"""
import os, sys, struct, math, bisect, itertools
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
import make_aimandshoot_rework as A
from make_flexicode_rework import tt, lp, hp, bp, mid2f, SR, SR2
import make_magicact_rework as MA
from make_magicact_rework import vibf, saw_f, fadein, fadeout, level, shape_tail, ks, near, deg, MODES, CH, chord, voice_lead, NN, nm, ensemble

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "nursery_time.xm")
SPEED, BPM, NCH = 2, 125, 16
ROW = SPEED * 2.5 / BPM
rng = np.random.default_rng(1977)
def noise(n): return rng.uniform(-1, 1, n)

# ================================================================ instruments
I = {}
RELSR = {12: SR, 5: SR2}
def add(k, name, x, gen=None, rel=12, pre=0.0, rms=.16, win=.3, peak=.95):
    sr = RELSR[rel]; I[k] = dict(name=name, x=shape_tail(level(x, rms, win, sr, peak), sr), gen=gen, rel=rel, pre=pre)
def share(k, d, name=None): I[k] = dict(name=name or d['name'], x=d['x'], gen=d['gen'], rel=d['rel'], pre=0.0)   # another song's sample, stored once

# ---- the ditty: a music box, a xylophone, a toy square lead, a bubbly bass, a soft pad, a 909-ish kit, chalk, boing
n = int(1.4 * SR); t = tt(n); f = mid2f(84)                                                  # music box (C6): a comb tooth, a ping on top
x = np.sin(2 * np.pi * f * t) * np.exp(-t / .55) + .25 * np.sin(2 * np.pi * f * 2.0 * t) * np.exp(-t / .2) + .12 * np.sin(2 * np.pi * f * 4.2 * t) * np.exp(-t / .05)
add('mbox', 'music box', x * np.minimum(1, t / .0015), 84, rms=.10, win=.2)
n = int(.6 * SR); t = tt(n); f = mid2f(76)                                                   # xylophone (E5): hard wood bars
x = np.sin(2 * np.pi * f * t) * np.exp(-t / .16) + .45 * np.sin(2 * np.pi * f * 3.93 * t) * np.exp(-t / .04) + .15 * np.sin(2 * np.pi * f * 9.2 * t) * np.exp(-t / .012)
add('xylo', 'xylophone', x * np.minimum(1, t / .001) + hp(noise(n), 3000) * np.exp(-t / .002) * .2, 76, rms=.10, win=.15)
n = int(.9 * SR); t = tt(n); f = mid2f(72); fr = vibf(f, t, 6.0, 12, .18, .2, 0, bend=60, bend_t=.025)   # toy lead (C5): a narrow pulse, a chirp up into the note
ph = np.cumsum(fr) / SR; pulse = ((ph % 1.0) < .25).astype(float) * 2 - 1
x = lp(pulse - pulse.mean(), 4200, 2) * fadein(t, .006) * (.75 + .25 * np.exp(-t / .12)) * fadeout(t, .9, .12)
add('toy', 'toy lead', x, 72, rms=.12, win=.2)
n = int(.45 * SR); t = tt(n); f = mid2f(36)                                                  # bubbly bass (C2): a sine with a squelch that closes fast
sq = np.sign(np.sin(2 * np.pi * f * t)); env = np.exp(-t / .05)
x = np.sin(2 * np.pi * f * t) + .5 * lp(sq, 300 + 2800 * env.mean(), 2) * env + .25 * bp(sq, 400, 1800) * np.exp(-t / .03)
add('blip', 'bubble bass', np.tanh(1.2 * x) * np.exp(-t / .28) * fadein(t, .003), 36, rms=.2, win=.15)
add('pad', 'soft pad', ensemble(60, 2.6, .12, 2200, (-9, -3, 4, 10), 3, 5.0, 6), 60, 5, pre=.08, rms=.1, win=.6)   # a warm, simple pad
n = int(.35 * SR); t = tt(n)                                                                  # kit: 909-ish kick, clap, hats, woodblock
add('kick9', 'soft kick', np.sin(2 * np.pi * np.cumsum(50 + 110 * np.exp(-t / .03)) / SR) * np.exp(-t / .2) + hp(noise(n), 3000) * np.exp(-t / .002) * .3, None, rms=.24, win=.12)
n = int(.3 * SR); t = tt(n); burst = sum(np.exp(-np.maximum(0, t - d) / .008) * (t >= d) for d in (0, .011, .022)) + .6 * np.exp(-np.maximum(0, t - .033) / .09) * (t >= .033)
add('clap', 'clap', bp(noise(n), 900, 3200) * burst, None, rms=.12, win=.1)
n = int(.06 * SR); t = tt(n); met = sum(np.sign(np.sin(2 * np.pi * fq * t)) for fq in (3140, 4700, 5900, 7200))
add('hat', 'closed hat', hp(met * .3 + noise(n), 6500, 2) * np.exp(-t / .014), None, rms=.07, win=.03)
n = int(.4 * SR); t = tt(n); met = sum(np.sign(np.sin(2 * np.pi * fq * t)) for fq in (3140, 4700, 5900, 7200))
add('ohat', 'open hat', hp(met * .3 + noise(n), 6000, 2) * np.exp(-t / .12), None, rms=.06, win=.1)
n = int(.12 * SR); t = tt(n)
add('wood', 'woodblock', (np.sin(2 * np.pi * 1180 * t) + .5 * np.sin(2 * np.pi * 2730 * t)) * np.exp(-t / .03), None, rms=.1, win=.05)
n = int(.6 * SR); t = tt(n); fr = 320 * 2 ** (1.4 * (1 - np.exp(-t / .06))) * (1 + .12 * np.sin(2 * np.pi * 14 * t) * np.exp(-t / .2))   # boing: a spring
add('boing', 'boing', np.sin(2 * np.pi * np.cumsum(fr) / SR) * np.exp(-t / .22) * fadein(t, .004), None, rms=.12, win=.15)
n = int(.32 * SR); t = tt(n); y = np.zeros(n)                                                # chalk: two strokes of a squeaky bandpass
for s0, s1, f0, f1 in ((0, .13, 2400, 4200), (.17, .3, 3600, 2800)):
    m = (t >= s0) & (t < s1); u = (t[m] - s0) / (s1 - s0); fc = f0 + (f1 - f0) * u
    tone = np.sin(2 * np.pi * np.cumsum(fc * (1 + .02 * np.sin(2 * np.pi * 90 * t[m]))) / SR)
    y[m] = (tone * .7 + .5 * hp(noise(m.sum()), 2500)) * np.sin(np.pi * u) ** .6
add('chalk', 'chalk', y, None, rms=.08, win=.1)

# ---- the band: guitars (palm-muted chug, open power chord, both in a left and a right take), lead, clean, bass, metal kit, scratch
def amp(x, drive, lo=110, cab=4800, scoop=.3, pres=.45):
    """a high-gain amp and a 4x12: tight lows, hard clipping (a touch asymmetric), scooped mids, presence, nothing above the speaker"""
    x = hp(x, lo, 1); x = x / (np.abs(x).max() + 1e-9)
    y = np.tanh(drive * x + .1 * drive * x * np.abs(x)); y = y - y.mean()
    y = lp(y, cab, 4); y = y + pres * bp(y, 1700, 3200, 2) - scoop * bp(y, 450, 850, 2)
    return hp(y, 70, 2)
def strum(notes, dur, damp, bright, seed, spread=.006):
    out = np.zeros(int(dur * SR))
    for j, m in enumerate(notes):
        y = ks(mid2f(m), dur, SR, damp, bright, seed + j); o = int(j * spread * SR); out[o:] += y[:len(out) - o]
    return out
for side, sd in (('L', 100), ('R', 200)):                                                   # two takes, left and right: a real double-track
    x = amp(strum((33, 40, 45), .45, .975, 1500, sd, .002), 26, lo=90, cab=4200)           # chug: A1 power chord, palm-muted
    add('chug' + side, 'chug ' + side, x * np.exp(-tt(len(x)) / .12) * fadeout(tt(len(x)), .45, .1), 33, rms=.2, win=.08)
    x = amp(strum((40, 47, 52), 2.2, .9993, 5000, sd + 50, .007), 22)                        # power chord: E2 B2 E3, let ring
    add('pow' + side, 'power chord ' + side, x * fadeout(tt(len(x)), 2.2, .5), 40, rms=.18, win=.3)
n = int(2.2 * SR); t = tt(n); f = mid2f(72); fr = vibf(f, t, 5.6, 28, .28, .35)             # lead (C5): a bowed-feeling sustain, vibrato after a moment
x = saw_f(fr, SR) + .5 * saw_f(fr * 2.003, SR, 1.1) + ks(f, 2.2, SR, .999, 7000, 7) * 2.5
add('lead', 'lead guitar', amp(x * fadein(t, .004), 14, lo=200, cab=5200, scoop=.15, pres=.6) * fadeout(t, 2.2, .3), 72, rms=.16, win=.3)
f = mid2f(64); y = ks(f, 1.8, SR, .9985, 6000, 9) + .8 * ks(f * 1.004, 1.8, SR, .9983, 5500, 10)          # clean guitar (E4): a chorus of two
add('clean', 'clean guitar', lp(y, 5000, 2) * np.exp(-tt(len(y)) / .9), 64, rms=.09, win=.25)
f = mid2f(33); y = ks(f, 1.3, SR, .9975, 2600, 11); y = np.tanh(2.2 * y / np.abs(y).max())                 # bass guitar (A1), a little growl
add('bassg', 'bass guitar', lp(y + .3 * bp(y, 600, 1400), 3000, 2) * fadeout(tt(len(y)), 1.3, .2), 33, rms=.22, win=.15)
n = int(.3 * SR); t = tt(n)                                                                   # metal kick: a click and a punch
add('mkick', 'metal kick', np.sin(2 * np.pi * np.cumsum(48 + 140 * np.exp(-t / .012)) / SR) * np.exp(-t / .11) + hp(noise(n), 2500) * np.exp(-t / .0025) * .6
    + np.sin(2 * np.pi * 3000 * t) * np.exp(-t / .003) * .25, None, rms=.26, win=.08)
n = int(.45 * SR); t = tt(n)                                                                  # snare: a crack, a body, the wires
x = bp(noise(n), 1200, 7500) * np.exp(-t / .14) * .9 + (np.sin(2 * np.pi * 185 * t) + .6 * np.sin(2 * np.pi * 330 * t)) * np.exp(-t / .06)
add('snare', 'snare', np.tanh(1.8 * x), None, rms=.2, win=.1)
n = int(.7 * SR); t = tt(n)                                                                   # tom (tuned by note: hi / mid / floor)
add('tom', 'tom', np.sin(2 * np.pi * np.cumsum(150 * (1 + .5 * np.exp(-t / .03))) / SR) * np.exp(-t / .22) + bp(noise(n), 400, 3000) * np.exp(-t / .01) * .4, None, rms=.2, win=.15)
n = int(2.6 * SR); t = tt(n); g = np.random.default_rng(31)                                  # crash: noise and a cloud of metal partials
x = hp(noise(n), 3200, 2) * .8 + sum(np.sin(2 * np.pi * g.uniform(3000, 7800) * t + g.uniform(0, 6.3)) for _ in range(30)) / 10
add('crash', 'crash', x * np.exp(-t / .9) * fadein(t, .002), None, rms=.13, win=.3)
n = int(1.4 * SR); t = tt(n)                                                                  # ride: a ping, a wash
x = sum(a * np.sin(2 * np.pi * fq * t) * np.exp(-t / d) for fq, a, d in ((2950, 1, .5), (4370, .6, .35), (5210, .4, .3), (6900, .3, .2))) + hp(noise(n), 5000) * np.exp(-t / .25) * .4
add('ride', 'ride', x * fadein(t, .001), None, rms=.08, win=.15)
n = int(.5 * SR); t = tt(n); ph = np.zeros(n); u = t / t[-1]                                 # scratch: a vowel pushed forward and pulled back, the fader chopping it
src = lp(saw_f(np.full(int(.6 * SR), 180.0), SR), 2500) + .3 * bp(noise(int(.6 * SR)), 800, 2500)
pos = (.5 - .5 * np.cos(2 * np.pi * 2 * u)) * .25 * SR; pos = np.clip(pos, 0, len(src) - 2)
x = np.interp(pos, np.arange(len(src)), src) * (np.abs(np.gradient(pos)) > .15) * ((np.sin(2 * np.pi * 15 * t) > -.4).astype(float))
add('scratch', 'scratch', lp(x, 5000, 2), None, rms=.13, win=.12)
# ---- borrowed: The Magic Act's oud, Aim and Shoot's darbuka and air swell (the same samples, so they are stored once in ROM)
share('oud', MA.I['oud_hi']); share('dum', A.I['dum']); share('tek', A.I['tek']); share('swell', A.I['swell']); I['swell']['pre'] = 1.2

KEYS = ['mbox', 'xylo', 'toy', 'blip', 'pad', 'kick9', 'clap', 'hat', 'ohat', 'wood', 'boing', 'chalk',
        'chugL', 'chugR', 'powL', 'powR', 'lead', 'clean', 'bassg', 'mkick', 'snare', 'tom', 'crash', 'ride', 'scratch',
        'oud', 'dum', 'tek', 'swell']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
assert len(KEYS) <= 32

# ================================================================ the score engine (as in make_magicact_rework.py)
TL = {}; WARN = []; BARS = []; VOLK = [1.0]
TRIM = {}
def put(row, ch, key, m=None, vol=40, early=True, free=False):
    d = I[key]; ri = int(round(row))
    if early and d['pre']: row = row - d['pre'] / ROW
    r = max(0, int(round(row)))
    if m is not None and d['gen'] is not None and not -15 <= m - d['gen'] <= 16: WARN.append(('range', key, m, r))
    TL[(r, ch)] = (key, m, max(1, min(64, int(round(vol * VOLK[0] * TRIM.get(key, 1.0))))) if vol > 1 else 1, free, ri)
def cut(row, ch, key='hat', m=None):
    """a note-off: the player has none, so the channel is restarted at volume 1"""
    r = int(round(row))
    if (r, ch) not in TL: TL[(r, ch)] = (key, m, 1, True, r)
class Bar:
    def __init__(s, t0, n, P, tonic, mode, ch=None, bass=None, extra=(), pedal=None):
        s.t0, s.n, s.P = t0, n, P; s.tonic = NN[tonic] if isinstance(tonic, str) else tonic % 12; s.mode = mode
        s.ch = list(ch) if ch else []; s.bass = s.tonic if bass is None else (NN[bass] if isinstance(bass, str) else bass % 12)
        s.pedal = pedal
        s.allow = {(s.tonic + i) % 12 for i in MODES[mode]} | set(s.ch) | {s.bass} | {(NN[e] if isinstance(e, str) else e) % 12 for e in extra}
        BARS.append(s)
    @property
    def beats(s): return s.n / s.P
    def r(s, pos): return s.t0 + pos * s.P
    def tm(s, centre): return near(s.tonic, centre)
    def end(s): return s.t0 + s.n
def seq(t0, n, P, specs, pedal=None):
    out = []; t = t0
    for sp in specs:
        b = Bar(t, n, P, sp[0], sp[1], sp[2] if len(sp) > 2 else None, sp[3] if len(sp) > 3 else None, sp[4] if len(sp) > 4 else (), pedal); out.append(b); t += n
    return out, t
def P(s):                                                                                   # 'D6' or 'D6 F5' -> midi list
    return [nm(w) for w in s.split()]

# ---------------------------------------------------------------- string notation: one character a step, the steps share the bar evenly
DRUM = {'k': ('mkick', 0, None, 40), 'K': ('mkick', 0, None, 50), 's': ('snare', 1, None, 44), 'S': ('snare', 1, None, 54), 'g': ('snare', 1, None, 13),
        'x': ('hat', 2, None, 20), 'X': ('hat', 2, None, 28), 'o': ('ohat', 2, None, 24), 'r': ('ride', 2, None, 24), 'b': ('ride', 2, 5, 32),
        'c': ('crash', 3, None, 44), 'C': ('crash', 3, None, 54), 'n': ('crash', 3, 6, 40), 'h': ('tom', 5, 5, 40), 'm': ('tom', 5, 0, 42), 'f': ('tom', 5, -6, 44),
        'j': ('kick9', 0, None, 40), 'J': ('kick9', 0, None, 48), 'p': ('clap', 1, None, 34), 'P': ('clap', 1, None, 42), 'w': ('wood', 5, None, 26),
        'd': ('dum', 5, None, 36), 't': ('tek', 5, None, 26), 'a': ('tek', 5, 5, 14)}
def drums(bar, s, lv=1.0):
    """s: one or more lines (kick / snare / hats / cymbals ...), each a string of steps; '.' = nothing"""
    for line in s.split():
        n = len(line)
        for i, c in enumerate(line):
            if c == '.' or c == '_': continue
            key, ch, m, v = DRUM[c]; put(bar.r(i * bar.beats / n), ch, key, m, v * lv, early=False)
def riff(bar, s, roots=(), pedal=33, lv=1.0, bass=True, bvol=40, chs=(6, 7)):
    """the guitars, both sides at once.  x X = palm-muted chug on the pedal (X accented), 0-9 a b c = chug that many semitones above the pedal,
       P = power chord on the next root (let ring), p = power chord, stopped on the next step, - = stop, . = let it sound"""
    n = len(s); ri = iter(roots); stop_next = False
    for i, c in enumerate(s):
        r = bar.r(i * bar.beats / n)
        if stop_next and c in '.-': [cut(r, ch) for ch in chs]; stop_next = False
        if c == '.': continue
        if c == '-': [cut(r, ch) for ch in chs]; continue
        stop_next = False
        if c in 'xX0123456789abc':
            m = pedal + (0 if c in 'xX' else int(c, 16)); v = (44 if c == 'X' else 36) * lv
            put(r, chs[0], 'chugL', m, v, early=False); put(r, chs[1], 'chugR', m, v, early=False)
            if bass: put(r, 4, 'bassg', m, bvol * lv, early=False)
        elif c in 'Pp':
            m = next(ri); m = nm(m) if isinstance(m, str) else m
            put(r, chs[0], 'powL', m, 40 * lv, early=False); put(r, chs[1], 'powR', m, 40 * lv, early=False)
            if bass: put(r, 4, 'bassg', m - 12 if m >= 45 else m, bvol * lv, early=False)
            stop_next = c == 'p'
def line(bar, ch, key, events, vol=40, step=.25, oct=0, free=False):
    """events: [(position in steps, 'note' or midi or None = stop)]"""
    for pos, m in events:
        r = bar.r(pos * step)
        if m is None: cut(r, ch, key); continue
        m = nm(m) if isinstance(m, str) else m
        put(r, ch, key, m + 12 * oct, vol, free=free)
def third_below(m, bar):
    """the scale tone a third below m in the bar's mode (twin guitars)"""
    sc = sorted({(bar.tonic + i) % 12 for i in MODES[bar.mode]}); below = [x for x in range(m - 5, m - 2) if x % 12 in sc]
    return below[-1] if below else m - 3

# ================================================================ the original's material
#   (16th positions in a bar, notes)
DYAD = [  # bar by bar: bass dyad, then (pos, top, bottom)
    ('D3 Bb3', [(p, 'D6', 'F5') for p in (0, 2, 4, 6)] + [(8, 'E6', 'G5'), (12, 'D6', 'F5')]),
    ('C3 A3', [(p, 'C6', 'E5') for p in (0, 2, 4, 6)] + [(8, 'D6', 'F5'), (12, 'C6', 'E5')]),
    ('D3 G3', [(p, 'Bb5', 'D5') for p in (0, 2, 4, 6)] + [(8, 'C6', 'E5'), (12, 'Bb5', 'D5')]),
    ('C3 A3', [(p, 'A5', 'C5') for p in (0, 2, 4, 6)] + [(8, 'Bb5', 'D5'), (12, 'A5', 'C5')]),
    ('D3 Bb3', [(p, 'D6', 'F5') for p in (0, 2, 4, 6)] + [(8, 'E6', 'G5'), (12, 'D6', 'F5')]),
    ('C3 A3', [(p, 'C6', 'E5') for p in (0, 2, 4, 6)] + [(8, 'D6', 'F5'), (12, 'C6', 'E5')]),
    ('G3 D3', [(p, 'Bb5', 'D5') for p in range(0, 16, 2)]),
    ('C3 E3', [(p, 'Bb5', 'E5') for p in range(0, 16, 2)])]
CHIME = [('D3 Bb3', [(0, 'D6'), (2, 'D6'), (4, 'D6'), (6, 'D6'), (8, 'E6'), (12, 'D6')]),
         ('C3 A3', [(0, 'C6'), (2, 'C6'), (4, 'C6'), (6, 'C6'), (8, 'D6'), (12, 'C6')]),
         ('D3 G3', [(0, 'Bb5'), (2, 'Bb5'), (4, 'Bb5'), (6, 'Bb5'), (8, 'C6'), (12, 'Bb5')]),
         ('C3 A3', [(0, 'A5'), (2, 'A5'), (4, 'A5'), (6, 'D5'), (10, 'F5'), (12, 'G5'), (14, 'A5')])]
RUN = [(2 * k, n_) for k, n_ in enumerate(('G5', 'A5', 'Bb5', 'C6', 'D6', 'E6', 'F6', 'G6'))]
TUNE = [('F3 C3', [(2, 'C6'), (4, 'C6'), (6, 'C6'), (8, 'D6'), (12, 'C6')]),
        ('E3 B2', [(0, 'B5'), (2, 'E6'), (6, 'E5')]),
        ('Bb2 Eb3', [(2, 'Bb5'), (4, 'Bb5'), (6, 'Bb5'), (8, 'C6'), (12, 'Bb5')]),
        ('A2 D3', [(0, 'A5'), (2, 'D5')]),
        ('F3 C3', [(2, 'C6'), (4, 'C6'), (6, 'C6'), (8, 'D6'), (12, 'C6')]),
        ('E3 B2', [(0, 'B5'), (2, 'E6'), (6, 'E5')]),
        ('A3 E3', [(2, 'A5'), (4, 'A5'), (6, 'A5'), (8, 'A5'), (10, 'Bb5')]),
        ('G3 E3', [(0, 'C6'), (2, 'Bb5'), (4, 'A5'), (6, 'G5')])]
# the bars' harmony (tonic, mode, chord, bass) for the ditty
H_DYAD = [('Bb', 'maj', chord('Bb', 'M'), 'D'), ('A', 'min', chord('A', 'm'), 'C'), ('G', 'dor', chord('G', 'm'), 'D'), ('A', 'min', chord('A', 'm'), 'C'),
          ('Bb', 'maj', chord('Bb', 'M'), 'D'), ('A', 'min', chord('A', 'm'), 'C'), ('G', 'dor', chord('G', 'm')), ('C', 'mix', chord('C', '7'))]
H_CHIME = [('Bb', 'maj', chord('Bb', 'M'), 'D'), ('A', 'min', chord('A', 'm'), 'C'), ('G', 'dor', chord('G', 'm'), 'D'), ('A', 'min', chord('A', 'm'), 'C', ('D', 'F', 'G'))]
H_RUN = [('G', 'dor', chord('G', 'm'), 'G', ('E',)), ('C', 'mix', chord('C', '7'), 'C', ('Bb',))]
H_TUNE = [('F', 'maj', chord('F', 'M')), ('E', 'hij', chord('E', '5')), ('Eb', 'lyd', chord('Eb', 'M'), 'Bb', ('C',)), ('A', 'phr', chord('A', 'sus4')),
          ('F', 'maj', chord('F', 'M')), ('E', 'hij', chord('E', '5')), ('A', 'phr', chord('A', 'm')), ('C', 'mix', chord('C', '7'), 'G')]

# ================================================================ PART ONE: the ditty (125 BPM, a 12-row beat, bars of 48 rows)
def dyads(bars, data, vol=34, chs=(10, 9), key='mbox', every=1):
    for b, (bs, ev) in zip(bars, data):
        for j, (p, hi, lo) in enumerate(ev):
            if j % every == 0: put(b.r(p * .25), chs[0], key, nm(hi), vol); put(b.r(p * .25), chs[1], key, nm(lo), vol - 4)
def mel(bars, data, ch, key, vol=36, oct=0, shift=0):
    for b, (bs, ev) in zip(bars, data):
        for p, n_ in ev: put(b.r(p * .25) + shift, ch, key, nm(n_) + 12 * oct, vol - (3 if p % 4 else 0))
def run(bars, ch, key, vol, oct=0, step=2):
    for b in bars:
        for p, n_ in RUN: put(b.r(p * .25 * step / 2), ch, key, nm(n_) + 12 * oct, vol)
def bounce(bars, data, vol=40):
    """the bubble bass: the dyad's lower note, bouncing root / octave in eighths"""
    for b, (bs, ev) in zip(bars, data):
        root = near(min(P(bs)) % 12, 38)
        for k in range(8): put(b.r(k * .5), 4, 'blip', root + (12 if k % 2 else 0), vol - (8 if k % 2 else 0))
def pads(bars, vol=22, prev=None):
    pv = prev
    for b in bars:
        pv = voice_lead(pv, b.ch[:3], 55, 72, 3)
        for j, m in enumerate(pv): put(b.t0, (6, 7, 8)[j], 'pad', m, vol - 2 * j)
    return pv
def kit(b, kind, lv=1.0, fill=False):
    K = {'four': 'j...j...j...j... ....p.......p... ..x...x...x...x.',
         'full': 'j...j...j...j..j ....p.......p... xxoxxxoxxxoxxxox',
         'bounce': 'j...j..jj...j... ....p..p....p... ..x.oxx...x.oxx. w.....w...w.....'}
    drums(b, K[kind], lv)
    if fill: drums(b, '............PPPP', lv * .7)

def m_ditty(t0):
    g = np.random.default_rng(1)
    b1, t = seq(t0, 48, 12, H_DYAD)                                                            # the chalkboard: the music box alone, then the beat
    dyads(b1, DYAD, 36)
    put(b1[0].r(.5), 13, 'chalk', None, 30, early=False); put(b1[1].r(2), 13, 'chalk', 3, 26, early=False); put(b1[3].r(3.5), 13, 'boing', None, 26, early=False)
    for b in b1[4:]: kit(b, 'four', .85)
    bounce(b1[4:], DYAD[4:], 34); put(b1[6].t0, 13, 'chalk', None, 24, early=False); kit(b1[7], 'four', .9, fill=True)
    b2, t = seq(t, 48, 12, H_TUNE)                                                             # the tune: the toy lead, the xylophone an octave up
    mel(b2, TUNE, 11, 'toy', 38, oct=-1); mel(b2, TUNE, 12, 'xylo', 28)
    pv = pads(b2, 20); bounce(b2, TUNE, 38)
    for j, b in enumerate(b2):
        kit(b, 'full', .95, fill=j == 7)
        if j % 2 == 0: put(b.t0, 10, 'mbox', near(b.ch[0], 86), 26); put(b.t0, 9, 'mbox', near(b.ch[1 % len(b.ch)], 80), 22)
    put(b2[2].t0, 13, 'boing', None, 22, early=False); put(b2[6].r(3), 13, 'chalk', None, 22, early=False)
    b3, t = seq(t, 48, 12, H_CHIME + H_CHIME[:2] + H_RUN)                                     # the chime: music box and xylophone, the toy a sixth under
    CH6 = CHIME + CHIME[:2]
    mel(b3[:6], CH6, 14, 'mbox', 38); mel(b3[:6], CH6, 12, 'xylo', 28, oct=-1)
    for b, (bs, ev) in zip(b3[:6], CH6):
        for p, n_ in ev: put(b.r(p * .25), 11, 'toy', third_below(third_below(nm(n_), b), b) - 12, 24)     # a sixth below, an octave down
    run(b3[6:], 14, 'mbox', 34); run(b3[6:], 11, 'toy', 30, oct=-1)
    for b in b3[6:]:
        for k in range(16): put(b.r(k * .25), 12, 'xylo', nm(RUN[k % 8][1]) + (12 if k >= 8 else 0) - 12, 20 + k)
    pv = pads(b3, 20, pv); bounce(b3, CHIME + CHIME[:2] + [('G3 D3', []), ('C3 E3', [])], 36)
    for j, b in enumerate(b3):
        kit(b, 'bounce', .95)
        if j in (3, 5): put(b.r(3.5), 13, 'boing', 2, 24, early=False)
    b4, t = seq(t, 48, 12, H_TUNE)                                                             # the tune again, toy and xylophone taking turns
    put(b4[0].t0, 14, 'mbox', nm('F6'), 40)
    for j, (b, (bs, ev)) in enumerate(zip(b4, TUNE)):
        who = ('toy', 11, -1) if (j // 2) % 2 == 0 else ('xylo', 12, 0)
        for p, n_ in ev: put(b.r(p * .25), who[1], who[0], nm(n_) + 12 * who[2], 38 if who[0] == 'toy' else 34)
        kit(b, 'full' if j % 2 else 'bounce', 1.0, fill=j in (3, 7))
        if j in (2, 6): put(b.t0, 13, 'chalk', None, 26, early=False)
    pv = pads(b4, 22, pv); bounce(b4, TUNE, 40)
    b5, t = seq(t, 48, 12, H_CHIME + H_CHIME[:2] + H_RUN)                                     # everything at once: the chime, its dyads, the tune's toy
    dyads(b5[:6], DYAD[:6], 30); mel(b5[:6], CH6, 14, 'mbox', 36, oct=0); mel(b5[:6], CH6, 11, 'toy', 34, oct=-1); mel(b5[:6], CH6, 12, 'xylo', 26, oct=-1)
    run(b5[6:], 11, 'toy', 34, oct=-1); run(b5[6:], 14, 'mbox', 34)
    dyads(b5[6:], DYAD[6:], 30)
    pv = pads(b5, 24, pv); bounce(b5, CHIME + CHIME[:2] + [('G3 D3', []), ('C3 E3', [])], 42)
    for j, b in enumerate(b5): kit(b, 'full', 1.05, fill=j == 7)
    # ---- something goes wrong
    tw, t = seq(t, 48, 12, [('C', 'mix', chord('C', '7')), ('F', 'maj', chord('F', 'M'), 'F', ('E', 'Eb', 'D', 'Db', 'C', 'B', 'Bb', 'A', 'Ab', 'G', 'Gb')), ('A', 'phr', None, 'A')])
    a, b, c = tw
    dyads([a], DYAD[7:8], 30); put(a.t0, 0, 'kick9', None, 40, early=False); put(a.r(2), 6, 'chugL', nm('A1'), 16, early=False); put(a.r(2), 7, 'chugR', nm('A1'), 16, early=False)
    put(a.r(3), 13, 'boing', -7, 26, early=False); put(a.r(1), 2, 'hat', None, 14, early=False)
    r = b.t0; gap = 7.0; m = nm('F6'); k = 0                                                   # the music box sticks on its last F and runs down like a tape
    while r < b.t0 + b.n - 1:
        put(r, 14 if k % 2 == 0 else 10, 'mbox', m, 40 - k * .5, free=True); r += max(1.0, gap); gap *= .86; k += 1
        if k % 2 == 0: m -= 1
    for j in range(16): put(b.r(j * .25), 11, 'toy', nm('C5') - j, 30 - j, free=True)           # the toy slides down a semitone a step
    for j, (pos, mm) in enumerate(((0, 0), (1.25, -2), (2.75, -5), (3.75, -9))): put(b.r(pos), 0, 'kick9', mm, 40 - 4 * j, early=False)
    for ch in (4, 6, 7, 8, 9, 10, 11, 12, 14): cut(c.t0, ch)
    put(c.t0, 13, 'scratch', None, 40, early=False)
    for j, row in enumerate((12, 21, 27, 31, 34, 37, 39, 41, 43, 45, 46, 47)):
        put(c.t0 + row, 6, 'chugL', nm('A1'), 24 + j * 1.6, early=False); put(c.t0 + row, 7, 'chugR', nm('A1'), 24 + j * 1.6, early=False)
    for j in range(16): put(c.r(j * .25), 1, 'snare', None, 10 + j * 2.6, early=False)
    put(c.t0 + c.n, 3, 'swell', None, 40)
    sil, t1 = seq(t, 12, 12, [('A', 'phr', None, 'A')])                                       # one beat of nothing
    for ch in range(16): cut(sil[0].t0, ch)
    return t1

# ================================================================ PART TWO: the band
H_M = [('F', 'lyd', chord('F', '5')), ('E', 'hij', chord('E', '5')), ('Bb', 'lyd', chord('Bb', '5')), ('A', 'phr', chord('A', '5')),
       ('F', 'lyd', chord('F', '5')), ('E', 'hij', chord('E', '5')), ('A', 'phr', chord('A', '5')), ('G', 'dor', chord('G', '5'))]
ROOT = {'F': 41, 'E': 40, 'Bb': 34, 'A': 33, 'G': 31, 'D': 38, 'C': 36, 'B': 35}
def twin(bars, data, vol=36, oct=-1, harm=True, step=.25):
    """the tune on the lead guitar, a second guitar a third under it (twin leads)"""
    for b, (bs, ev) in zip(bars, data):
        for p, n_ in ev:
            m = nm(n_) + 12 * oct; r = b.r(p * step)
            put(r, 11, 'lead', m, vol, free=True)
            if harm: put(r, 12, 'lead', third_below(m, b), vol - 6, free=True)
def trem(b, notes, lv=1.0, step=.25, octs=-24):
    """tremolo picking: every step a chug on the note that is sounding (notes: [(pos, 'note')])"""
    cur = None; it = sorted(notes); j = 0; n = int(round(b.beats / step))
    for k in range(n):
        while j < len(it) and it[j][0] <= k: cur = nm(it[j][1]) + octs; j += 1
        if cur is None: continue
        v = (40 if k % 4 == 0 else 33) * lv
        put(b.r(k * step), 6, 'chugL', cur, v, early=False); put(b.r(k * step), 7, 'chugR', cur, v, early=False)
        if k % 2 == 0: put(b.r(k * step), 4, 'bassg', cur - (12 if cur >= 45 else 0), 38 * lv, early=False)
def kick_with(b, s, lv=1.0):
    """the kick in unison with a riff string"""
    drums(b, ''.join('K' if c in 'XP' else 'k' if c in 'xp0123456789abc' else '.' for c in s), lv)

def m_metal(t0):
    g = np.random.default_rng(2)
    # ---- 1. IMPACT: the tune's chords as one huge stab after another
    b, t = seq(t0, 48, 12, H_M[:4])
    for bi, (bar, s, r) in enumerate(zip(b, ('P.p.p.p.P...P...', 'P.P...P.........', 'P.p.p.p.P...P...', 'P.P....-XXXXXXXX'), 'F E Bb A'.split())):
        riff(bar, s, [ROOT[r]] * 8); kick_with(bar, s); drums(bar, 'C............... ....S.......S...' if bi < 3 else 'C............... ........hhmmffff')
    # ---- 2. THE RIFF: chugs on low A, the tune's chords punched in; the second time round the twin leads play the tune
    b, t = seq(t, 48, 12, H_M * 2)
    R2 = ['P..x.xx.X.xxP.x.', 'P..x.xx.X.xxP.x.', 'P..x.xx.X.xxP.x.', 'X.xxX.xxX.xxXXXX']
    for bi, bar in enumerate(b):
        r = H_M[bi % 8][0]; s = R2[bi % 4] if bi % 8 != 7 else 'P...p.p.p.P.----'
        riff(bar, s, [ROOT[r]] * 8)
        dk = 'K.k.K.k.K.k.K.k.' if bi < 8 else 'kkkkkkkkkkkkkkkk'
        drums(bar, dk + ' ....S.......S... ' + ('x.x.x.x.x.x.x.x.' if bi < 8 else 'r.r.b.r.r.r.b.r.') + (' C...............' if bi % 4 == 0 else ''))
    twin(b[8:], TUNE, 38)
    put(b[0].t0, 11, 'lead', nm('E6'), 34, free=True); put(b[4].t0, 11, 'lead', nm('A5'), 34, free=True)
    # ---- 3. THRASH: twice as fast (8-row beat), the chime tremolo-picked, a skank beat, then a blast with the chime on the lead
    b, t = seq(t, 32, 8, (H_CHIME + H_CHIME) * 2)
    for bi, bar in enumerate(b):
        bs, ev = CHIME[bi % 4]; trem(bar, ev, 1.0, octs=-48)
        if bi < 8: drums(bar, 'k...k...k...k... ..s...s...s...s. x.x.x.x.x.x.x.x.' + (' C...............' if bi % 4 == 0 else ''))
        else:
            drums(bar, 'kkkkkkkkkkkkkkkk .s.s.s.s.s.s.s.s r.r.r.r.r.r.r.r.' + (' C...............' if bi % 2 == 0 else ''), .9)
            for p, n_ in ev: put(bar.r(p * .25), 11, 'lead', nm(n_) - 12, 38, free=True)
    drums(b[-1], '............ffff', 1.1)
    # ---- 4. NU-GROOVE: down to a 16-row beat; drop-A chugs with holes in them, scratches, and the music box singing the chime over it
    b, t = seq(t, 64, 16, [('A', 'phr', chord('A', '5'))] * 16)
    NU = ['X..x..X.-.X.x...', 'X..x..X.-.1.1.-.', 'X..x..X.-.X.x...', 'X.X..1..X.-.X.1-']
    for bi, bar in enumerate(b):
        if 8 <= bi < 10:
            drums(bar, '........ x.x.x.x.x.x.x.x.', .7); [cut(bar.t0, ch) for ch in (4, 6, 7)]
        else:
            s = NU[bi % 4]; riff(bar, s, pedal=33); kick_with(bar, s, .9)
            drums(bar, '....S.......S... x.x.x.xox.x.x.xo' + (' g......g.....g..' if bi % 2 else ''))
            if bi % 2 == 1: put(bar.r(3.5), 13, 'scratch', None, 38, early=False); put(bar.r(3.75), 13, 'scratch', 5, 34, early=False)
        bs, ev = CHIME[bi % 4]
        if bi >= 4: [put(bar.r(p * .25), 14, 'mbox', nm(n_), 34) for p, n_ in ev]
        if bi >= 10 and bi % 2 == 0: put(bar.t0, 11, 'lead', nm('E6') if bi % 4 == 2 else nm('F6'), 30, free=True)
    for j, n_ in enumerate(('D5', 'A4', 'F4', 'A4', 'D5', 'A4', 'F4', 'A4')): put(b[10].r(j * .5), 8, 'clean', nm(n_), 26)
    put(b[10].t0, 3, 'crash', None, 50, early=False)
    # ---- 5. HIJAZ: the tune in A Hijaz on the oud over a darbuka, then the band answers in kind (quiet / loud / quiet / both)
    HT = [('A3', [(2, 'C#5'), (4, 'C#5'), (6, 'C#5'), (8, 'D5'), (12, 'C#5')]), ('A3', [(0, 'Bb4'), (2, 'E5'), (6, 'E4')]),
          ('A3', [(2, 'Bb4'), (4, 'Bb4'), (6, 'Bb4'), (8, 'C#5'), (12, 'Bb4')]), ('A3', [(0, 'A4'), (2, 'D5')])]
    b, t = seq(t, 48, 12, [('A', 'hij', chord('A', 'M'), 'A')] * 12)
    for bi, bar in enumerate(b):
        bs, ev = HT[bi % 4]; loud = bi in (4, 5) or bi >= 8
        for p, n_ in ev: put(bar.r(p * .25), 9, 'oud', nm(n_) - 12, 40)
        drums(bar, 'd..t..d.t.d.t...' if not loud else 'd...t.t.d...t...', .9)
        if loud:
            trem(bar, ev, .95, octs=-36); drums(bar, ('kkkkkkkkkkkkkkkk' if bi >= 8 else 'K...k.k.K...k.k.') + ' ....S.......S... r.r.r.r.r.r.r.r.' + (' C...............' if bi in (4, 8) else ''))
            if bi >= 8: [put(bar.r(p * .25), 11, 'lead', nm(n_), 34, free=True) for p, n_ in ev]
        else:
            put(bar.t0, 4, 'bassg', nm('A1'), 30, early=False); [cut(bar.t0, ch) for ch in (6, 7)]
    sweep_notes = [deg('hij', nm('A4'), k) for k in range(15)]
    for j, m in enumerate(sweep_notes): put(b[-1].r(2 + j * 2 / 15), 11, 'lead', m, 30 + j, free=True, early=False)
    # ---- 6. BREAKDOWN: half-time, the kick locked to the guitars, A against B flat
    b, t = seq(t, 48, 12, [('A', 'phr', chord('A', '5'))] * 8)
    BD = ['X..X..1.X..X1...', 'X..X..1.X..X1.1.', 'X..X..1.X..X1...', 'X.....1.....X.--', 'X..X..1.X..X1...', 'X..X..1.X..X1.1.', 'X.....X.....X...', 'X..1..X..1..XXXX']
    for bi, bar in enumerate(b):
        riff(bar, BD[bi], pedal=33); kick_with(bar, BD[bi]); drums(bar, '........S....... ........n....... b...b...b...b...')
    # ---- 7. SOLO: Am G F E (the Andalusian fall, E as the Hijaz on E): bends, runs, sweeps, tapping, then two guitars in harmony
    AND = [('A', 'min', chord('A', '5')), ('G', 'mix', chord('G', '5')), ('F', 'lyd', chord('F', '5')), ('E', 'hij', chord('E', '5'))]
    b, t = seq(t, 48, 12, AND * 4)
    for bi, bar in enumerate(b):
        r = AND[bi % 4][0]; riff(bar, 'P.x.x.x.P.x.x.x.' if bi % 4 != 3 else 'P.x.x.x.P.p.p.p.', [ROOT[r] if r != 'A' else 45] * 8, pedal=ROOT[r] if r != 'F' else 41)
        drums(bar, ('k.....k.k.....k.' if bi < 12 else 'kkkkkkkkkkkkkkkk') + ' ....s.......s... x.x.x.x.x.x.x.x.' + (' c...............' if bi % 4 == 0 else ''))
        top = bar.tm(76); sc = [deg(bar.mode, top - 12, k) for k in range(15)]
        if bi < 4:                                                                                       # bends: long notes from the tune
            for p, m in ((0, top), (6, top + (2 if bar.mode != 'hij' else 1)), (8, top), (12, deg(bar.mode, top, -1))): put(bar.r(p * .25), 11, 'lead', m, 38, free=True)
        elif bi < 8:                                                                                     # runs: sixteenths down the scale in fours
            k0 = 13
            for j in range(16): put(bar.r(j * .25), 11, 'lead', sc[k0 - (j // 4) - (j % 4)], 34, free=True)
        elif bi < 12:                                                                                    # sweeps: the chord up and down, six a beat
            arp = [near(c, 64) for c in bar.ch] + [near(c, 64) + 12 for c in bar.ch] + [near(bar.tonic, 64) + 24]
            arp = sorted(arp); path = arp + arp[-2:0:-1]
            for j in range(24): put(bar.r(j / 6), 11, 'lead', path[j % len(path)], 32, free=True)
        else:                                                                                            # tapping the chime, two guitars
            bs, ev = CHIME[bi % 4]
            for p, n_ in ev:
                m = nm(n_) - 12; put(bar.r(p * .25), 11, 'lead', m, 36, free=True); put(bar.r(p * .25), 12, 'lead', third_below(m, bar), 30, free=True)
    for j in range(12): put(b[-1].r(2 + j / 6), 11, 'lead', nm('E6') - 2 * j, 34 - j, free=True, early=False)      # a whammy dive
    # ---- 8. FLASHBACK: two bars of the ditty, cut off by the drums
    b, t = seq(t, 48, 12, H_TUNE[:2] + [('A', 'phr')])
    for ch in (4, 6, 7, 11, 12): cut(b[0].t0, ch)
    mel(b[:2], TUNE[:2], 11, 'toy', 38, oct=-1); mel(b[:2], TUNE[:2], 12, 'xylo', 28); kit(b[0], 'full'); kit(b[1], 'bounce'); bounce(b[:2], TUNE[:2], 36)
    for ch in (4, 11, 12): cut(b[2].t0, ch)
    drums(b[2], 'C...............  hhhhmmmmffffSSSS', 1.1)
    # ---- 9. ANTHEM: half-time (16-row beat), open chords, the chime as a stadium tune on twin leads; then up a whole step; then a fake ending
    AN = [('Bb', 'lyd', chord('Bb', '5')), ('A', 'phr', chord('A', '5')), ('G', 'dor', chord('G', '5')), ('A', 'hij', chord('A', '5'))]
    b, t = seq(t, 64, 16, AN * 2 + [((NN[x[0]] + 2) % 12,) + x[1:2] + ([(c + 2) % 12 for c in x[2]],) for x in AN] * 2)
    for bi, bar in enumerate(b):
        up = 2 if bi >= 8 else 0; r = ROOT[AN[bi % 4][0]] + up
        riff(bar, 'P.......P...P.x.' if bi % 4 != 3 else 'P...P...P.p.p.p.', [r] * 8, pedal=r)
        drums(bar, ('k.......k.k.....' if bi % 4 != 3 else 'k.k.k.k.kkkkkkkk') + ' ........S....... r.r.r.r.r.r.r.r.' + (' C...............' if bi % 2 == 0 else ''))
        bs, ev = CHIME[bi % 4]
        for p, n_ in ev:
            m = nm(n_) - 12 + up; put(bar.r(p * .25), 11, 'lead', m, 38, free=True); put(bar.r(p * .25), 12, 'lead', third_below(m, bar), 32, free=True)
    end, t = seq(t, 64, 16, [('B', 'phr', chord('B', '5'))])
    riff(end[0], 'P...............', [35]); drums(end[0], 'K............... S............... C...............')
    put(end[0].t0, 11, 'lead', nm('B5'), 36, free=True)
    gap, t = seq(t, 64, 16, [('A', 'phr')])                                                           # ...and nothing, as if it were over
    for ch in range(16): cut(gap[0].r(2.5), ch)
    # ---- 10. GROOVE: it was not over. The clean guitar plays the run, then the run becomes a groove riff
    b, t = seq(t, 48, 12, [('A', 'phr', chord('A', '5'))] * 10)
    for bi in range(2):
        for p, n_ in RUN: put(b[bi].r(p * .25), 8, 'clean', nm(n_) - 12, 32)
    GR = ['X..3.5X..7.8a.X.', 'X..3.5X..7.85.3.', 'X..3.5X..7.8a.X.', 'X.X.X.--a.8.7.5.']
    for bi, bar in enumerate(b[2:]):
        s = GR[bi % 4]; riff(bar, s, pedal=33); kick_with(bar, s)
        drums(bar, '....S.......S... x.x.x.x.x.x.x.x.' + (' n...............' if bi % 2 == 0 else ''))
    # ---- 11. FINALE: the 8-row beat again, the tune's chords tremolo-picked, the tune on twin leads; gallop, then blast; stop hits
    b, t = seq(t, 32, 8, H_M * 2)
    for bi, bar in enumerate(b):
        r = H_M[bi % 8][0]; trem(bar, [(0, {41: 'F3', 40: 'E3', 34: 'Bb2', 33: 'A2', 31: 'G2'}[ROOT[r]])], 1.0, octs=-12)
        drums(bar, ('k.kkk.kkk.kkk.kk' if bi < 8 else 'kkkkkkkkkkkkkkkk') + ' ....S.......S... ' + ('r.r.r.r.r.r.r.r.' if bi < 8 else 'C.......C.......'))
    twin(b, TUNE * 2, 38)
    hit, t = seq(t, 48, 12, [('F', 'lyd', chord('F', '5')), ('A', 'phr', chord('A', '5'))])
    riff(hit[0], '--p.p.p.p...p...', [41] * 5); kick_with(hit[0], '--p.p.p.p...p...', 1.2); drums(hit[0], '..S.S.S.S...S...')
    riff(hit[1], 'P...............', [33]); drums(hit[1], 'K............... S............... C...............')
    for ch in (11, 12): cut(hit[0].t0, ch)
    put(hit[1].t0, 11, 'lead', nm('E6'), 36, free=True)
    gap, t = seq(t, 48, 12, [('F', 'maj')] * 2)
    for ch in range(16): cut(gap[0].r(2), ch)
    # ---- CODA: the music box, alone, winding down; the original's last F
    coda, t1 = seq(t, 96, 24, [('Bb', 'maj', chord('Bb', 'M'), 'D'), ('A', 'min', chord('A', 'm'), 'C'), ('F', 'maj', chord('F', 'M'))])
    k = 0; extra = 0.0
    for bi, (bs, ev) in enumerate(CHIME[:2]):
        for p, n_ in ev:
            put(coda[bi].r(p * .25) + extra, 14, 'mbox', nm(n_), 36 - k); extra += k * .8; k += 1
    put(coda[2].r(1), 14, 'mbox', nm('F6'), 34); put(coda[2].r(1), 13, 'boing', -12, 14, early=False)
    return t1

MODULES = [(m_ditty, 1.0), (m_metal, 1.0)]

# ================================================================ assembly, checks, the XM file
TAIL = 150
def build():
    TL.clear(); BARS.clear(); WARN.clear(); t = 0
    for m, k in MODULES: VOLK[0] = k; t = m(t)
    VOLK[0] = 1.0; return int(round(t))
def check(verbose=True):
    starts = [b.t0 for b in BARS]; bad = []
    for (r, ch), (key, m, vol, free, ri) in sorted(TL.items()):
        if m is None or I[key]['gen'] is None or free or vol <= 1: continue
        i = bisect.bisect_right(starts, ri) - 1
        if i >= 0 and m % 12 not in BARS[i].allow: bad.append((round(ri * ROW, 2), ch, key, m))
    if verbose:
        for w in WARN[:15]: print('  WARN', w)
        for x in bad[:30]: print('  OUT OF SCALE', x)
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
    for Pt in pats:
        k = F.pat_bytes(Pt)
        if k not in seen: seen[k] = len(uniq); uniq.append(Pt)
        order.append(seen[k])
    assert len(uniq) <= 256 and len(order) <= 256, (len(uniq), len(order))
    hdr = b'Extended Module: ' + b'Nursery Time'[:20].ljust(20) + b'\x1a' + b'make_nursery_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(uniq), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(Pt) for Pt in uniq) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    s = R * ROW
    print("wrote %s: %d orders, %d patterns, %d notes, %.1f s (%d:%02d), %d KB, %d out-of-scale" % (OUT, len(order), len(uniq), len(TL), s, s // 60, s % 60, os.path.getsize(OUT) // 1024, len(bad)))
if __name__ == '__main__': main()
