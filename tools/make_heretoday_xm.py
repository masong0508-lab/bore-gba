#!/usr/bin/env python3
"""HERE TODAY GONE TOMORROW (by Singhs), SEQUENCED: the 12 minute suite as a tracker module for the GBA player (tools/here_today_gone_tomorrow.xm).
The old version was a 6.5 MB streamed recording; this is notes plus about half a megabyte of small synthesised samples (every instrument is rendered here, once).

The song is the HERE TODAY MIDI (tools/here_today.mid) played by a very early (1970) band: 12-string acoustic, mellotron strings and flute, Hammond swell, a felt piano,
fuzzed bass, bells, timpani. Same key and tempo as the song, no key change. The suite has FOUR ACTS and the melody is kept back until the last two minutes:
  I   THE FIELD       wind, 12-string alone, verses that thicken, a bridge, a first modest chorus
  II  WEATHER         hush, builds, bigger choruses, a false ending, a pastoral return
  III THE STORM       tremolo strings, timpani rolls, the biggest chorus, then the unwinding
  IV  HOMECOMING      the 12-string alone again, a tender bridge, then the melody for the first time, on a solo cello over 12-string and a soft Hammond;
                      a final chorus that is warm rather than loud, and an ending that slows, lands on G major and rings out.
Player limits kept: 16 channels, 32 instruments (one 8-bit sample each, no loops, no note-off), fixed speed/BPM (speed 6, 78 BPM = 16th-note rows of 0.192 s),
so every sustained instrument comes in a few LENGTHS (S / M / L) and a note picks the one that ends about when it does.
Run from the project root:  python3 tools/make_heretoday_xm.py      (needs numpy, scipy; a few seconds)
Then:  python3 tools/xm2gba.py   and audition with   python3 tools/preview_xm.py here_today_gone_tomorrow tools/here_today_gone_tomorrow.xm out.wav"""
import sys, os, struct, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import midi_read as M

SR = 32000; SPEED = 6; BPM = 78; NCH = 16; OUT = os.environ.get('OUT_XM', 'tools/here_today_gone_tomorrow.xm')
ROW = SPEED * 2.5 / BPM                                         # seconds per row
rng = np.random.default_rng(1970)
hz = lambda m: 440.0 * 2 ** ((m - 69) / 12)
def lp(x, fc, o=2): return signal.lfilter(*signal.butter(o, min(fc, SR * .45) / (SR / 2)), x)
def bp(x, fc, bw): return signal.lfilter(*signal.butter(2, [(fc - bw / 2) / (SR / 2), min(fc + bw / 2, SR * .45) / (SR / 2)], 'band'), x)
def env(n, a, r):
    e = np.ones(n); k = max(1, min(n, int(a * SR))); e[:k] = .5 - .5 * np.cos(np.pi * np.arange(k) / k)
    k = max(1, min(n, int(r * SR))); e[-k:] *= .5 + .5 * np.cos(np.pi * np.arange(k) / k); return e
def room(x, T=.9, wet=.3, dark=3800, seed=5):                        # a short dark tail baked into a plucked note (the player has no reverb)
    r = np.random.default_rng(seed); m = int(T * SR); t = np.arange(m) / SR; h = r.standard_normal(m) * np.exp(-6.9 * t / T); h = lp(h, dark, 1); h = np.concatenate([np.zeros(int(.02 * SR)), h]); h /= np.sqrt((h ** 2).sum())
    return x + wet * signal.fftconvolve(x, h)[:len(x)]
def fdel(x, dfn):                                                  # fractional delay
    idx = np.arange(len(x), dtype=float); pos = np.clip(idx - dfn(idx / SR) * SR, 0, len(x) - 2); p0 = pos.astype(int); fr = pos - p0
    return x[p0] * (1 - fr) + x[p0 + 1] * fr
def ens(x, seed=0):                                                # string-ensemble chorus: three moving taps (baked in; the player has no effects)
    rg = np.random.default_rng(seed); out = .55 * x
    for d, rate, dep in ((.017, .31, .004), (.024, .47, .006), (.031, .23, .005)):
        ph = rg.uniform(0, 6.28); out = out + .3 * fdel(x, lambda tt, d=d, rate=rate, dep=dep, ph=ph: d + dep * np.sin(2 * np.pi * rate * tt + ph))
    return out
def tt(L): n = int(L * SR); return n, np.arange(n) / SR

# ------------------------------------------------------------------------------------------------------------- the instruments (one note at its root pitch)
def pad(root, L):                                                # a Hammond swell through a Leslie (6.3 Hz wobble), one tone of a chord
    n, t = tt(L); f = hz(root); vib = 1 + .0016 * np.sin(2 * np.pi * 6.3 * t); ph = 2 * np.pi * np.cumsum(f * vib) / SR
    x = np.sin(ph) + .5 * np.sin(2 * ph) + .35 * np.sin(3 * ph) + .2 * np.sin(4 * ph) + .1 * np.sin(6 * ph)
    y = np.sin(2 * np.pi * f * t) + .4 * np.sin(2 * np.pi * f * 2.003 * t) + .15 * np.sin(2 * np.pi * f * 3.001 * t)
    x = lp(x * (1 + .1 * np.sin(2 * np.pi * 6.3 * t + 1.2)), 2600) * .55 + lp(y, 1500) * .8
    x = room(ens(x, int(root)), T=1.0, wet=.25, seed=int(root))
    return x * env(n, .4, 1.0) * (.15 + .85 * np.exp(-t / 1.3))      # the swell eases back like an expression pedal, so a chord that is changed early is cut when it is already quiet
def strings(root, L, bright):                                    # MELLOTRON: three detuned saws with tape wow and flutter, a reedy dull top
    n, t = tt(L); f = hz(root)
    w = .0011 * np.sin(2 * np.pi * (.4 + rng.uniform(0, .25)) * t + rng.uniform(0, 6.3)) + .0006 * np.sin(2 * np.pi * (5.1 + rng.uniform(0, 1)) * t + rng.uniform(0, 6.3))
    ph = 2 * np.pi * np.cumsum(f * (1 + w)) / SR
    x = sum(signal.sawtooth(ph * 2 ** (c / 1200) + rng.uniform(0, 6)) for c in (-7, 0, 7)); x = lp(x, bright); x = x + .35 * bp(x, 900, 500) + .02 * lp(rng.standard_normal(n), 2500, 1)
    x = room(ens(x, int(root) + 1), T=.9, wet=.25, seed=int(root) + 2)
    return x * env(n, min(.07, L * .15), min(.4, L * .3))
def flute(root, L):                                              # the mellotron FLUTE: three detuned voices with their own vibrato, a chiff of breath on the attack, a baked chorus and room
    n, t = tt(L); x = 0
    for cents in (-7, 0, 8):
        f = hz(root) * 2 ** (cents / 1200) * (1 + (.004 + .002 * rng.random()) * np.sin(2 * np.pi * (5.0 + .6 * rng.random()) * t + rng.uniform(0, 6.3)) * np.clip((t - .25) / .5, 0, 1)); ph = 2 * np.pi * np.cumsum(f) / SR
        x = x + np.sin(ph) + .3 * np.sin(2 * ph) + .12 * np.sin(3 * ph) + .05 * np.sin(4 * ph)
    x = x + .05 * bp(rng.standard_normal(n), min(hz(root) * 2, 6000), hz(root)) + .5 * bp(rng.standard_normal(n), 2400, 1800) * np.exp(-t / .07)   # breath and the chiff
    x = lp(x, 4200); x = room(ens(x, int(root) + 3), T=.9, wet=.3, seed=int(root) + 4)
    return x * env(n, min(.08, L * .2), min(.45, L * .3))
def cello(root, L):                                              # the solo cello: a delayed vibrato, a little scoop up into the note, a bow, a swell on long notes
    n, t = tt(L); vd = np.clip((t - .22) / .5, 0, 1)
    f = hz(root) * (1 + .0065 * np.sin(2 * np.pi * 5.0 * t) * vd) * 2 ** ((-.5 * np.exp(-t / .06)) / 12)
    x = signal.sawtooth(2 * np.pi * np.cumsum(f) / SR); x = lp(x, 1700) * 1.3 + .5 * lp(x, 700); x = x + .05 * bp(rng.standard_normal(n), 1200, 900) * np.exp(-t / .12)
    x = room(x, T=1.0, wet=.3, seed=int(root))
    return x * env(n, min(.1, L * .25), min(.4, L * .3)) * (1 + .18 * np.sin(np.pi * np.clip(t / L, 0, 1)) * min(1, L / 1.8))
def guitar(root, L):                                             # 12-string acoustic: octave pairs below, detuned unison pairs above, a loose pick
    n, t = tt(L); f = hz(root)
    def course(fr): return sum(np.sin(2 * np.pi * fr * k * t + .1 * k * k) * np.exp(-t * (1.3 + 1.2 * k)) / k ** .9 for k in range(1, 10) if fr * k < SR * .45)
    x = course(f) + (.75 * course(2 * f * 1.0015) if root < 55 else .85 * course(f * 1.003)); x = x + .03 * np.exp(-t / .006) * rng.standard_normal(n)
    return room(x * env(n, .002, .06), seed=int(root)) * env(n, .001, .25)
def piano(root, L):                                              # a felt piano: slightly stretched partials, two strings beating, a quick decay so the arpeggios stay clean
    n, t = tt(L); f = hz(root); d0 = 1.6 + f / 300; x = 0
    for k in range(1, 12):
        fk = f * k * np.sqrt(1 + .0005 * k * k)
        if fk >= SR * .45: break
        x = x + np.sin(2 * np.pi * fk * t + .3 * k) * np.exp(-t * (d0 + .8 * k)) / k ** 1.05
    x = x + .5 * np.sin(2 * np.pi * f * 1.0012 * t) * np.exp(-t * d0) + .09 * np.exp(-t / .012) * lp(rng.standard_normal(n), 2500, 1)
    return room(lp(x, 5200, 1) * env(n, .002, .12), seed=int(root)) * env(n, .001, .25)
def bass(root, L):                                               # a growling, slightly fuzzed bass that decays like a plucked string
    n, t = tt(L); f = hz(root); x = np.sin(2 * np.pi * f * t) + .35 * np.sin(4 * np.pi * f * t) + .1 * np.sin(6 * np.pi * f * t)
    return lp(np.tanh(1.8 * x), 1100) * env(n, .02, .35) * np.exp(-t * 1.25)
def bell(root, L):
    n, t = tt(L); f = hz(root)
    x = sum(a * np.sin(2 * np.pi * f * r * t) * np.exp(-t * d) for r, a, d in ((1, 1, 2.2), (2.76, .35, 4.0), (5.4, .12, 6.0), (.5, .25, 1.8)) if f * r < SR * .45)
    return x * env(n, .002, .25)
def thump(root, L):
    n, t = tt(L); ph = 2 * np.pi * np.cumsum(46 + 70 * np.exp(-t / .05)) / SR
    return (np.sin(ph) * np.exp(-t / .13) + .15 * lp(rng.standard_normal(n), 400) * np.exp(-t / .02)) * env(n, .001, .05)
def timp(root, L):                                               # a timpani: a pitched skin, a short dip in pitch, a long soft body
    n, t = tt(L); f = hz(root); ph = 2 * np.pi * np.cumsum(f * (1 + .35 * np.exp(-t / .04))) / SR
    return (np.sin(ph) * np.exp(-t / .45) + .3 * np.sin(1.5 * ph) * np.exp(-t / .2) + .25 * lp(rng.standard_normal(n), 600) * np.exp(-t / .02)) * env(n, .001, .08)
def swell(root, L):                                              # a cymbal swell: noise that rises for L seconds and stops
    n, t = tt(L); x = bp(rng.standard_normal(n), 5200, 4200) + .4 * lp(rng.standard_normal(n), 900)
    return x * (t / L) ** 2.2 * env(n, .05, .02)

# name, synth, root pitch, seconds, rate code (XM relative note: rate = 8363 * 2^(rel/12) Hz), target loudness (RMS at full volume column), synth extra arg
IN = []
def add(key, fn, root, L, rel, tgt, *a): IN.append(dict(key=key, fn=fn, root=root, L=L, rel=rel, tgt=tgt, a=a))
add('pad_lo', pad, 47, 3.1, 0, .12);  add('pad_hi', pad, 59, 3.1, 0, .16)
# the sustained instruments come in lengths that match the song's own note spacing (4, 8, 12, 16 rows = 0.77, 1.54, 2.31, 3.08 s): each sample finishes with its own
# release just before the next note of that part starts, so a note is never cut off while it is still sounding (a cut at full level is a click)
for reg, root, br, rel in (('lo', 45, 1200, 0), ('hi', 69, 1700, 6)):
    for cl, L in ((('A', .4),) if reg == 'lo' else ()) + (('S', .75), ('M', 1.5), ('L', 2.28), ('X', 3.05)): add('st_%s_%s' % (reg, cl), strings, root, L, rel, .16 if reg == 'lo' else .22, br)
add('fl_S', flute, 86, .75, 6, .13); add('fl_M', flute, 86, 1.5, 6, .13); add('fl_X', flute, 86, 3.05, 6, .13)
for cl, L in (('A', .17), ('B', .36), ('C', .6), ('D', 1.5), ('E', 2.28), ('F', 3.05)): add('ce_' + cl, cello, 58, L, 6, .28)
add('gt_lo', guitar, 46, 3.0, 12, .15);  add('gt_mid', guitar, 57, 2.8, 12, .15);  add('gt_hi', guitar, 66, 2.2, 12, .15)
add('pn_lo', piano, 48, 2.4, 10, .12);  add('pn_mid', piano, 62, 2.2, 12, .12);  add('pn_hi', piano, 74, 1.7, 12, .12)
add('bs_lo', bass, 38, 3.2, 0, .14); add('bs_hi', bass, 44, 3.0, 0, .14)
add('bell', bell, 84, 2.2, 12, .06)
add('thump', thump, 49, .7, 0, .26); add('timp', timp, 40, 1.4, 4, .26); add('swell', swell, 49, 2.4, 10, .2)
II = {d['key']: i + 1 for i, d in enumerate(IN)}
assert len(IN) <= 32, len(IN)
def synth_all():
    for d in IN:
        x = d['fn'](d['root'], d['L'], *d['a']).astype(float)
        rate = int(round(8363 * 2 ** (d['rel'] / 12)))
        n1 = int(round(len(x) * rate / SR)); x = signal.resample(x, n1) if n1 != len(x) else x             # FFT resample: band-limited to the new rate
        pk = np.abs(x).max(); x = x / pk
        act = x[int(.08 * rate):max(int(.08 * rate) + 16, min(int(len(x) * .7), int(.6 * rate)))]; rms = np.sqrt((act ** 2).mean())
        d['x'] = x; d['svol'] = d['tgt'] / rms                                                                   # relative; scaled so the loudest instrument is 64
    top = max(d['svol'] for d in IN if d['key'] != 'swell')                                                   # (the swell's rising noise would otherwise set the scale)
    for d in IN: d['svol'] = max(1, min(64, int(round(64 * d['svol'] / top))))
    IN[II['swell'] - 1]['svol'] = 26                                                                          # (the swell ends at full scale: keep it under the orchestra)
def tn(key, p):                                                  # tracker note that plays pitch p on this instrument
    n = 49 + int(round(p)) - IN[II[key] - 1]['root']; assert 1 <= n <= 96, (key, p); return n

# ------------------------------------------------------------------------------------------------------------- the song's notes and chords
div, tr = M.parse('tools/here_today.mid')
tempos = [(e[0], int.from_bytes(e[3], 'big')) for e in tr[0] if e[1] == 'meta' and e[2] == 0x51]
def tnotes(ev):
    on = {}; out = []
    for e in ev:
        if e[1] != 'ev': continue
        k = e[2] >> 4
        if k == 9 and e[4] > 0: on[e[3]] = (e[0], e[4])
        elif k == 8 or (k == 9 and e[4] == 0):
            if e[3] in on: t0, v = on.pop(e[3]); out.append((t0, e[0], e[3], v))
    return sorted(out)
rw = lambda tk: int(round(tk / 120.0))                           # MIDI tick -> row (a 16th note is 120 ticks)
def rows_of(n): return [(rw(a), max(1, rw(b) - rw(a)), m, v) for a, b, m, v in tnotes(n)]
VOX, GTR, V1, V2, VA, VC = (rows_of(tr[i]) for i in (1, 3, 4, 5, 6, 7))
PC = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
QUAL = {'': (0, 4, 7), 'm': (0, 3, 7), 'm7': (0, 3, 7, 10), 'm7b5': (0, 3, 6, 10), 'dim7': (0, 3, 6, 9), '7': (0, 4, 7, 10), 'maj7': (0, 4, 7, 11), '6': (0, 4, 7, 9)}
def chord(sym):
    bass = None
    if '/' in sym: sym, b = sym.split('/'); bass = (PC[b[0]] + (b.count('#') - b.count('b'))) % 12
    r = (PC[sym[0]] + (1 if sym[1:2] == '#' else (-1 if sym[1:2] == 'b' else 0))) % 12
    q = sym[1 + (sym[1:2] in ('#', 'b')):]; iv = QUAL[q]; base = 48 + (r if r < 6 else r - 12)
    return dict(tones=[base + i for i in iv], bass=36 + (r if bass is None else bass) % 12, root=r)
CH = sorted((rw(e[0]), chord(e[3].decode())) for e in tr[2] if e[1] == 'meta' and e[2] == 1)
CHEND = rw(90000)
V1R, V2R, BRR, CHR, V3R, OUTR = (0, 112), (112, 224), (224, 384), (384, 560), (560, 672), (672, 750)
MOTTO = [52, 55, 57, 59]                     # the first four notes of the song's tune (E G A B); kept for two quiet bell quotes, a prelude and the last bars

# ------------------------------------------------------------------------------------------------------------- the grid
MUTE = {int(c) for c in os.environ.get('MUTE', '').split(',') if c}   # (to audition without some channels: MUTE=15,14 OUT_XM=...)
GRID = {}                                                        # row -> {channel: (note, instrument, volume column)}
def put(row, ch, key, p, v):
    if row < 0 or ch in MUTE: return
    v = max(0.0, min(1.0, v))
    if v < .015: return
    GRID.setdefault(row, {})[ch] = (tn(key, p), II[key], 0x10 + max(1, min(64, int(round(64 * v)))))
def hum(v): return v * (1 + .07 * (rng.random() - .5))           # a little life in the touch
def pick(fam, p, rows):                                          # the longest sample of this family that has finished by the time the next note starts (a sample played above its root
    root, cl = FAM[fam]; f = 2 ** ((p - root) / 12.0); sec = rows * ROW                # pitch runs out sooner, one below it later: the length is divided by the pitch ratio)
    best = cl[0][0]
    for suf, Lc in cl:
        if Lc / f <= sec + .03: best = suf
    return fam + '_' + best
FAM = {'st_lo': (45, [('A', .4), ('S', .75), ('M', 1.5), ('L', 2.28), ('X', 3.05)]), 'st_hi': (69, [('S', .75), ('M', 1.5), ('L', 2.28), ('X', 3.05)]),
       'fl': (86, [('S', .75), ('M', 1.5), ('X', 3.05)]), 'ce': (58, [('A', .17), ('B', .36), ('C', .6), ('D', 1.5), ('E', 2.28), ('F', 3.05)])}
def stkey(p, rows): return pick('st_lo' if p < 55 else 'st_hi', p, rows)
def flkey(p, rows): return pick('fl', p, rows)
def cekey(p, rows): return pick('ce', p, rows)
def gtkey(p): return 'gt_lo' if p <= 52 else ('gt_mid' if p <= 62 else 'gt_hi')
def pnkey(p): return 'pn_lo' if p <= 55 else ('pn_mid' if p <= 69 else 'pn_hi')
def bskey(p): return 'bs_lo' if p <= 41 else 'bs_hi'

# channels
PADCH = (0, 1); STCH = {'v1': 4, 'v2': 5, 'va': 6, 'vc': 7}; LEADCH = 8; GTCH = (3, 9, 10, 11); PNCH = (2, 12, 13); BSCH = 14; PCCH = 15
class RR:                                                        # round robin over a few channels (a new note cuts the one that has been ringing longest)
    def __init__(s, chs): s.c = list(chs); s.i = 0
    def next(s): c = s.c[s.i % len(s.c)]; s.i += 1; return c

ARPS = {'arp8': lambda n: [0, 1, 2, n, n + 1, n + 2, n + 1, n],            # up the chord, over the top and back (the original ladder)
        'arp8b': lambda n: [0, n, 1, n + 1, 2, n + 2, 1, n + 1],          # low and high in turn, climbing
        'arp8c': lambda n: [n + 2, n + 1, n, 2, 1, 0, 1, 2]}              # down from the top and back up

def section(name, D, src, **L):
    """Lay one section of the suite at row D; src = rows of the song; returns the next free row. Layers (0 = off), all scaled by lv and the ramp:
      pad  Hammond chord bed          st {v1,v2,va,vc}  mellotron strings (the arranged string parts)     tr  tremolo strings (repeat every 2 rows)
      fl   mellotron flute, the V1 line an octave up     gt  the 12-string's part      gs  a loose strum at each chord
      pn + pnp  piano pattern         bs + bsp  bass pattern          bl + blp  bells                      pc  drum pattern     sw  cymbal swell at the end
      ld   the melody on a solo cello    rit  ritardando (the last stretch gets slower)      gap  silent rows after the section"""
    a, b = src; n = b - a; lv = L.get('lv', 1.0); r0, r1 = L.get('ramp', (1, 1)); rit = L.get('rit', 0)
    def T(x): x = x - a; return D + int(round(x + rit * n * (x / n) ** 2))
    def G(x): return lv * (r0 + (r1 - r0) * (x - a) / n)
    rr_g, rr_p = RR(GTCH), RR(PNCH)
    end = T(b)
    # strings and flute: the class follows the spacing to the next note of the same part
    def spacing(notes, k, r, d): nx = notes[k + 1][0] if k + 1 < len(notes) else r + d; return max(1, min(nx, b) - r)
    for nm, src_notes in (('v1', V1), ('v2', V2), ('va', VA), ('vc', VC)):
        lvl = L.get('st', {}).get(nm, 0)
        if not lvl: continue
        for k, (r, d, p, v) in enumerate(src_notes):
            if not (a <= r < b): continue
            s_, e_ = T(r), T(min(r + d, b)); sp = spacing(src_notes, k, r, d)
            if L.get('tr'):
                for k2 in range(s_, max(e_, s_ + 1), 4): put(k2, STCH[nm], stkey(p, 4), p, hum(lvl * G(r) * (.75 + .25 * (k2 == s_))))
            else: put(s_, STCH[nm], stkey(p, sp), p, hum(lvl * G(r)))
    if L.get('fl'):
        for k, (r, d, p, v) in enumerate(V1):
            if a <= r < b: put(T(r), LEADCH, flkey(p + 12, spacing(V1, k, r, d)), p + 12, hum(L['fl'] * G(r)))
    # melody on the solo cello: same-pitch repeats are tied, a note runs on to the next one; the length class follows the spacing to the next note
    if L.get('ld'):
        ns = [x for x in VOX if a <= x[0] < b]; i = 0; mer = []
        while i < len(ns):
            r, d, p, v = ns[i]
            while i + 1 < len(ns) and ns[i + 1][2] == p and ns[i + 1][0] - (r + d) <= 1: d = ns[i + 1][0] + ns[i + 1][1] - r; i += 1
            mer.append([r, d, p]); i += 1
        for k, (r, d, p) in enumerate(mer):
            nx = mer[k + 1][0] if k + 1 < len(mer) and mer[k + 1][0] - r <= 24 else r + max(d, 12)
            s_, e_ = T(r), T(min(nx, b)); put(s_, LEADCH, cekey(p, max(1, e_ - s_)), p, hum(L['ld'] * G(r)))
    # 12-string
    if L.get('gt'):
        for (r, d, p, v) in GTR:
            if a <= r < b: put(T(r), rr_g.next(), gtkey(p), p, hum(L['gt'] * G(r) * (.55 + .45 * v / 86)))
    # chords: pad bed, bass, strums, piano, bells
    cs = [(max(c0, a), min(c1, b), c) for (c0, c), (c1, _) in zip(CH, CH[1:] + [(CHEND, None)]) if c0 < b and c1 > a]
    bells = []; lastpad = -99
    for (c0, c1, c) in cs:
        tones = c['tones']; nt = len(tones); span = c1 - c0; s = T(c0)
        if L.get('pad') and (span >= 8 or c0 - lastpad >= 12):      # a passing chord shorter than half a bar keeps the pad of the one before it
            sel = [tones[0], tones[1]] if nt > 2 else tones
            for off in range(0, span, 16):                           # a long chord gets a fresh swell every bar
                for j, p in enumerate(sel): put(T(c0 + off), PADCH[j], 'pad_lo' if p < 52 else 'pad_hi', p, hum(1.3 * L['pad'] * G(c0 + off)))
            lastpad = c0 + (span - 1) // 16 * 16
        bsp = L.get('bsp', 'pedal')
        if L.get('bs'):
            bp_ = c['bass']; fifth = bp_ + 7 if bp_ + 7 <= 47 else bp_ - 5
            def B(off, p, vv=1.0): put(T(c0 + off), BSCH, bskey(p), p, hum(L['bs'] * G(c0 + off) * vv))
            if bsp == 'pedal':
                for off in range(0, span, 16): B(off, bp_)
            elif bsp == 'rf':
                for off in range(0, span, 16): B(off, bp_)
                for off in range(8, span, 16): B(off, fifth, .75)
            elif bsp == 'walk':
                nxt = [cc for (cr, cc) in CH if cr >= c1]; tgt = nxt[0]['bass'] if nxt else bp_
                for off in range(0, span, 16): B(off, bp_)
                if span >= 14: B(12, bp_ + (2 if tgt > bp_ else -2) if abs(tgt - bp_) < 6 else fifth, .7)
            elif bsp == 'pulse':
                for off in range(0, span, 4): B(off, bp_, 1.0 if off % 16 == 0 else .8)
            elif bsp == 'octave':
                for off in range(0, span, 8): B(off, bp_ if off % 16 == 0 else (bp_ + 12 if bp_ + 12 <= 53 else bp_), 1.0 if off % 16 == 0 else .7)
        if L.get('gs'):
            for j, p in enumerate(tones[:4]): put(s + j, rr_g.next(), gtkey(p), p, hum(L['gs'] * G(c0) * (.9 - .1 * j)))
        if L.get('pn'):
            pk = L.get('pnp', 'arp8'); lad = tones + [t + 12 for t in tones] + [tones[0] + 24]; q = L['pn']
            def P(off, idx, vv=1.0):
                if off < span: put(T(c0 + off), rr_p.next(), pnkey(lad[idx % len(lad)]), lad[idx % len(lad)], hum(q * G(c0 + off) * vv))
            if pk in ARPS:                                           # the eighth-note ladder (three ways round it) that runs under almost everything
                pat = ARPS[pk](nt)
                for e in range(0, span, 2): P(e, pat[(e // 2) % 8], 1 if e % 8 == 0 else .72)
            elif pk == 'arp4':
                for e in range(0, span, 4): P(e, [0, 2, nt + 1, nt + 2][(e // 4) % 4], 1 if e % 16 == 0 else .75)
            elif pk == 'roll':
                for j, idx in enumerate((0, 1, 2, nt)): P(j, idx, .8)
                P(8, nt + 2, .7); P(12, nt + 1, .6)
            elif pk == 'sparse':
                P(0, nt, .9); P(8, nt + 1, .7)
            elif pk == 'pulse':
                for e in range(0, span, 2): P(e, [nt + 1, nt + 2, nt + 1, nt][(e // 2) % 4], 1 if e % 8 == 0 else .7)
        if L.get('bl'):
            bk = L.get('blp', 'bell4')
            def Bl(off, p, vv=1.0):
                if off < span:
                    fade = 1 - .67 * min(1, (c0 + off - a) * ROW / 6.0) if L.get('bfade') else 1.0         # in verse 1 the bells ease down once it has started
                    bells.append((T(c0 + off) + 1, p, L['bl'] * G(c0 + off) * vv * fade))
            bp2 = [t + 36 - (12 if t > 53 else 0) for t in tones]
            if bk == 'bell4':
                for qn, k in enumerate((2, 3, 4, 3)): Bl(qn * 2, bp2[k % nt])
            elif bk == 'bell1': Bl(0, bp2[2 % nt])
            elif bk == 'bell2': Bl(0, bp2[2 % nt]); Bl(8, bp2[1 % nt], .7)
    # drums: one soft thump on every bar line; a heartbeat (two); or a timpani roll that grows into the end
    pc = L.get('pc')
    if pc in ('bar', 'heart'):
        for (cr, _) in [(r, 0) for r in range(a - a % 16, b, 16) if a <= r < b]:
            put(T(cr), PCCH, 'thump', 49, hum(G(cr) * (.55 if pc == 'bar' else .42)));
            if pc == 'heart': put(T(cr) + 2, PCCH, 'thump', 49, hum(G(cr) * .28))
    if pc == 'roll':
        nb = L.get('rollbars', 2) * 16
        for k in range(b - nb, b):
            x = (k - (b - nb)) / nb; step = 4 if x < .5 else (2 if x < .85 else 1)
            if (k - (b - nb)) % step == 0: put(T(k), PCCH, 'timp', 40 + (0 if ((k - b) // 16) % 2 else 7), .25 + .75 * x * lv * r1)
    for (r_, p_, v_) in bells:                                       # a bell waits for a drum that was hit in the last three rows
        if any(PCCH in GRID.get(k_, {}) and GRID[k_][PCCH][1] != II['bell'] for k_ in range(r_ - 3, r_ + 1)): continue
        put(r_, PCCH, 'bell', p_, hum(v_))
    if L.get('sw'): put(end - int(2.4 / ROW), LEADCH, 'swell', 49, L['sw'] * lv * r1)
    return end + L.get('gap', 0)

# ------------------------------------------------------------------------------------------------------------- the suite
ALL = dict(v1=1.0, v2=.9, va=1.0, vc=1.1)
def S(**k): return {n: v * k.get(n, 1.0) for n, v in ALL.items()}
def SC(w): return {n: v * w for n, v in ALL.items()}
SECS = [
 # ---- I. THE FIELD
 ('wind',        OUTR, dict(lv=1.33, ramp=(.4, .9), pad=.7, fl=.9, pn=.5, pnp='arp8', bl=0.5, blp='bell4')),
 ('prelude',     V1R,  dict(lv=0.88, ramp=(.55, .9), gt=1.0, pad=.3, pn=.55, pnp='arp8b', bl=0.9, blp='bell4')),
 ('intro',       V1R,  dict(fl=.8, lv=0.53, ramp=(.7, 1.0), gt=1.0, pn=.85, pnp='arp8', st={'vc': .8, 'va': .5}, pad=.45, bl=0.9, blp='bell4')),
 ('verse 1',     V1R,  dict(lv=0.6, gt=1.0, pad=.55, st=S(v1=.8, v2=.8), fl=.6, pn=.6, pnp='arp8', bs=.5, bsp='pedal', bl=0.9, blp='bell4', bfade=1)),
 ('verse 2',     V2R,  dict(lv=0.61, gt=1.0, pad=.6, st=SC(.95), fl=.65, pn=.85, pnp='arp8b', bs=.6, bsp='rf', bl=0.3, blp='bell4')),
 ('bridge',      BRR,  dict(lv=0.67, gt=.9, pad=.7, st=SC(1.1), pn=.85, pnp='arp8', bs=.75, bsp='rf', fl=.7, bl=0.35, blp='bell4')),
 ('chorus',      CHR,  dict(lv=0.69, gt=.9, gs=.5, pad=.75, st=SC(1.15), fl=.75, pn=1.0, pnp='arp8', bs=.8, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 # ---- II. WEATHER
 ('interlude 1', V2R,  dict(lv=0.67, gt=.9, st=S(v1=.7, v2=1.1, va=1.1), pn=1.0, pnp='arp8c', bs=.65, bsp='rf', pad=.55, fl=.7, bl=0.45, blp='bell4')),
 ('hush',        V1R,  dict(lv=1.18, ramp=(.7, .4), gt=.8, pad=.5, st={'vc': .5}, pn=.6, pnp='arp8b', gap=10, bl=0.7, blp='bell4')),
 ('build 1',     BRR,  dict(lv=0.85, ramp=(.45, 1.1), gt=.8, pad=.7, st=SC(1.1), fl=.7, pn=.85, pnp='arp8', bs=.75, bsp='pulse', pc='roll', rollbars=2, sw=.9, bl=0.3, blp='bell4')),
 ('chorus 2',    CHR,  dict(lv=0.74, gt=.9, gs=.6, pad=.8, st=SC(1.25), fl=.85, pn=1.1, pnp='arp8b', bs=.9, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 ('verse 3',     V3R,  dict(lv=0.65, gt=1.0, pad=.7, st=S(v1=.9), fl=.65, pn=.95, pnp='arp8', bs=.65, bsp='rf', bl=0.35, blp='bell4')),
 ('first ending', OUTR, dict(lv=0.71, ramp=(1, .5), gt=.9, pad=.8, st=SC(1.0), fl=.7, pn=.8, pnp='arp8c', gap=8, bl=0.4, blp='bell4')),
 ('pastoral',    V2R,  dict(lv=0.79, ramp=(.6, .9), gt=1.0, fl=.9, pn=.9, pnp='arp8', pad=.6, bl=0.6, blp='bell4')),
 ('second verse', V1R, dict(lv=0.58, gt=.9, st=S(v1=.8, v2=.9), fl=.6, pad=.6, pn=.8, pnp='arp8b', bs=.55, bsp='pedal', bl=0.5, blp='bell4')),
 # ---- III. THE STORM
 ('second bridge', BRR, dict(lv=0.78, ramp=(.5, 1.15), gt=.8, pad=.7, st=SC(1.15), fl=.7, pn=1.0, pnp='arp8c', bs=.8, bsp='walk', pc='roll', rollbars=2, sw=1.0, bl=0.35, blp='bell4')),
 ('interlude 2', V3R,  dict(lv=0.72, gt=.9, st=S(v1=.8, va=1.1), fl=.65, pn=1.0, pnp='arp8', pad=.6, bs=.65, bsp='octave', bl=0.4, blp='bell4')),
 ('chorus 3',    CHR,  dict(lv=0.78, ramp=(.9, 1.1), gt=.9, gs=.7, pad=.85, st=SC(1.3), fl=.9, pn=1.1, pnp='arp8', bs=.95, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 ('storm',       V2R,  dict(lv=0.99, ramp=(1.0, 1.2), gt=.9, pad=.8, st=SC(1.2), tr=1, fl=.7, pn=1.0, pnp='arp8c', bs=.9, bsp='pulse', pc='bar', bl=0.4, blp='bell4')),
 ('interlude 3', V2R,  dict(lv=0.71, ramp=(.9, .45), gt=.9, pad=.7, st=S(v1=.7, va=.9), fl=.65, pn=.95, pnp='arp8b', bs=.5, bsp='pedal', bl=0.55, blp='bell4')),
 ('build 2',     BRR,  dict(lv=0.97, ramp=(.4, 1.3), gt=.7, pad=.7, st=SC(1.2), tr=1, fl=.7, pn=.9, pnp='arp8', bs=.85, bsp='pulse', pc='roll', rollbars=3, sw=1.0, bl=0.3, blp='bell4')),
 ('chorus 4',    CHR,  dict(lv=1.16, ramp=(1.05, 1.25), gt=.9, gs=.8, pad=.9, st=SC(1.4), fl=.95, pn=1.2, pnp='arp8b', bs=1.0, bsp='walk', pc='bar', bl=0.5, blp='bell4')),
 ('verse 4',     V3R,  dict(lv=0.88, ramp=(1.0, .6), gt=.9, pad=.7, st=S(v1=.8, v2=.9, va=.9), pn=.9, pnp='arp8c', bs=.6, bsp='rf', fl=.55, bl=0.35, blp='bell4')),
 # ---- IV. HOMECOMING  (the arpeggios stay, softer; the cello has the tune)
 ('ending 1',    OUTR, dict(lv=0.74, ramp=(.9, .5), gt=.8, pad=.8, st=SC(.9), fl=.55, pn=.8, pnp='arp8', gap=10, bl=0.4, blp='bell4')),
 ('quiet again', V1R,  dict(lv=1.01, ramp=(.7, .55), gt=.9, pad=.3, pn=.55, pnp='arp8b', gap=6, bl=0.8, blp='bell4')),
 ('third bridge', BRR, dict(lv=0.86, ramp=(.6, .8), gt=.7, pad=.45, st={'vc': .7, 'va': .55, 'v2': .4}, pn=.8, pnp='arp8', bs=.45, bsp='pedal', bl=0.35, blp='bell4')),
 ('last verse',  V3R,  dict(lv=0.91, ramp=(.7, .85), ld=.85, gt=.8, pad=.4, st={'vc': .55, 'va': .3}, pn=.5, pnp='arp8', bl=0.35, blp='bell4')),
 ('final chorus', CHR, dict(lv=1.0, ramp=(.7, .95), ld=.95, gt=.8, gs=.35, pad=.55, st=S(v1=.45, v2=.5, va=.6, vc=.7), fl=.3, pn=.65, pnp='arp8b', bs=.45, bsp='pedal', pc='heart', bl=0.3, blp='bell4')),
 ('coda',        (672, 726), dict(lv=1.06, ramp=(.9, .45), rit=.5, ld=.9, gt=.7, pad=.6, st=S(v1=.3, v2=.4, va=.5, vc=.6), pn=.45, pnp='arp8', bs=.4, bsp='pedal', bl=0.5, blp='bell4')),
]
LVF = json.load(open(os.environ['LVFILE'])) if os.environ.get('LVFILE') else {}
def compose():
    D = 0; log = []
    for name, src, L in SECS:
        L = dict(L); L['lv'] = L.get('lv', 1.0) * LVF.get(name, 1.0)
        d0 = D; D = section(name, D, src, **L); log.append((name, d0, D))
    # the last G chord rings and fades in four slow Hammond swells; the first four notes of the tune come back once on bells, slower and softer, and the piece is done
    for k, (lvl, p) in enumerate(((.5, 0), (.4, 1), (.3, 0), (.2, 1))):
        for j, t in enumerate((43, 50)): put(D + 10 * k + j, PADCH[j], 'pad_lo' if t < 52 else 'pad_hi', t, lvl * 1.3)
    for i, p in enumerate(MOTTO): put(D + 14 + i * 5 + (i == 3) * 3, PCCH, 'bell', p + 24, .30 - .05 * i)
    D += 16
    end = D + 48
    return log, end

# ------------------------------------------------------------------------------------------------------------- the XM file
def pat_bytes(P):
    b = bytearray()
    for row in P:
        for (n, i, v) in row: b += bytes([n, i, v, 0, 0])
    return struct.pack('<IBHH', 9, 0, len(P), len(b)) + bytes(b)
def inst_bytes(d):
    q = np.clip(np.round(d['x'] * 127), -127, 127).astype(int); delta = np.diff(np.concatenate([[0], q])); dl = bytes((int(v) & 255) for v in delta)
    h = struct.pack('<I', 263) + d['key'].encode('latin1')[:22].ljust(22, b'\0') + b'\0' + struct.pack('<HI', 1, 40) + bytes(96) + bytes(48) + bytes(48)
    h += bytes(14) + struct.pack('<H', 0) + bytes(263 - len(h) - 14 - 2)
    sh = struct.pack('<IIIBbBBbB', len(q), 0, 0, d['svol'], 0, 0, 128, d['rel'], 0) + b'\0' * 22
    assert len(h) == 263 and len(sh) == 40
    return h + sh + dl
def main():
    synth_all()
    log, end = compose()
    nrows = (end + 15) // 16 * 16
    PL = 64; pats = []
    for p0 in range(0, nrows, PL):
        P = []
        for r in range(p0, min(p0 + PL, nrows)):
            cells = GRID.get(r, {}); P.append([cells.get(c, (0, 0, 0)) for c in range(NCH)])
        pats.append(P)
    order = list(range(len(pats)))
    hdr = b'Extended Module: ' + b'HERE TODAY GONE TOMORROW'[:20].ljust(20) + b'\x1a' + b'make_heretoday_xm'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(IN), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(d) for d in IN)
    open(OUT, 'wb').write(hdr + body)
    secs = nrows * ROW
    print('wrote %s: %d patterns of %d rows, %.1f s (%d:%02d), %d instruments, %d KB; samples %d KB' % (OUT, len(pats), PL, secs, secs // 60, secs % 60, len(IN), os.path.getsize(OUT) // 1024, sum(len(d['x']) for d in IN) // 1024))
    for d in IN: print('  %2d %-8s root %2d  %5.2f s  rate %5d  %6d bytes  svol %2d' % (II[d['key']], d['key'], d['root'], d['L'], round(8363 * 2 ** (d['rel'] / 12)), len(d['x']), d['svol']))
    json.dump([(n, round(a * ROW, 1), round(b * ROW, 1)) for n, a, b in log], open(os.environ.get('TMPDIR', '.') + '/ht_sections.json', 'w'))
    for n, a, b in log: print('  %-14s %5.1f - %5.1f s' % (n, a * ROW, b * ROW))
if __name__ == '__main__': main()
