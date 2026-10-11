#!/usr/bin/env python3
"""Builds tools/over_my_voice.xm : "Over My Voice", a DayBar hi-NRG / house rework of the Caustic project "Over my voixe. V2"
(tools/over_my_voixe.caustic, read here, and its MIDI export tools/over_my_voixe.mid: 120 BPM, 39 bars, F# minor / A major).
usage: python3 tools/make_overmyvoice_rework.py        (run from the project root; needs numpy + scipy; byte-identical on every run)

THE VOICE: the original is built round four Caustic Vocoders. Here a software singer sings instead (tools/singer.py, a formant singing
synthesizer: a glottal pulse with vibrato through a gliding five-formant vocal tract, real consonants): the four 8-bar vocal lines the
fourth Vocoder holds (never played in the MIDI) are sung with words, as whole phrases rendered at this song's tempo, each doubled a
64th late on the other side; the first Vocoder's one-bar chord riff (G# | D E F# | A B C# ...) becomes the hook, sung as chopped
syllables ("o - ver - my - voice") in three-part harmony; the pads of the second and third Vocoders become a sung "ahh" choir.
  LINE 1  O-ver my voice / can you hear / the sound of my heart / beat-ing / out loud / o-ver my voice
  LINE 2  You can't see / what I feel / deep in-side / ev-ery word's real / turn it up / o-ver my voice
  LINE 3  Oh / I'm sing-ing it loud / to you / lift me a-bove / my voice
  LINE 4  Lift it high-er, high-er / o-ver the noise and the / lights of the crowd / o-ver my voice
EVERYTHING ELSE IN THE FILE, made to fit: the 8BitSynth's melody (the MIDI's) and its unused arpeggio patterns (D, E, E-with-A#),
the Organ (silent in the MIDI): its walking bass line becomes the house organ bass, its D/A chord pattern the stabs, and its three
C major patterns, taken down a minor third into F# minor, give the breakdown its Andalusian fall (F#m E D C#) and its piano; the
second Vocoder's spare 4-bar tune (also down a minor third) is the chime counter-melody; the second BeatBox's unused patterns play
congas and toms over the house kit; the first BeatBox's pattern rides on the hats.
DAYBAR: 127.5 BPM four on the floor, claps on 2 and 4, open hats on the offbeats, an organ bass, brass stabs, orchestra hits, piano,
chimes, a hoover in the dub, risers, snare rolls into every section.
64th-note rows (speed 1, XM BPM 85 = 127.5 BPM), 64 rows a bar, 128-row patterns = 2 bars.
Form (112 bars, about 3:31): intro 8 | verse 16 | pre 8 | chorus 16 | breakdown 16 | verse 16 | pre 8 | chorus 16 | outro 8.
Channels: 0 kick | 1 clap / snare | 2 hat | 3 open hat / congas / toms | 4 organ bass | 5 organ / piano | 6 7 8 choir / chops / brass
          9 vocal | 10 vocal double | 11 8-bit lead | 12 counter-melody / lead echo | 13 arp / chimes | 14 orchestra hit / hoover | 15 fx
"""
import os, sys, struct
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2, HIFI
def rs(x, fo, fi): return x if fo == fi else signal.resample(x, int(round(len(x) * fo / fi)))
def pulse(f, n, duty=.5, sr=SR):
    t = tt(n, sr); y = np.zeros(n)
    for k in range(1, int(min(sr * .45 / f, 30)) + 1): y += np.sin(np.pi * k * duty) * np.cos(2 * np.pi * k * f * t) / k
    return y * (4 / np.pi)
import singer

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "over_my_voixe.caustic")
OUT = os.path.join(HERE, "over_my_voice.xm")
SPEED, BPM, ROWS, NCH, BR, Q = 1, 85, 128, 16, 64, 4      # 16 rows a beat = 127.5 BPM
BEAT = 16 * SPEED * 2.5 / BPM
rng = np.random.default_rng(1275)

# ---------------------------------------------------------------- the .caustic file: every machine's patterns (see make_hotdamn_rework.py)
D = open(SRC, 'rb').read()
def spats():
    o = 0; out = []
    while True:
        o = D.find(b'SPAT', o)
        if o < 0: return out
        lens = struct.unpack('<64i', D[o + 8:o + 264]); cap = struct.unpack('<64i', D[o + 264:o + 520]); p = o + 520; pats = {}
        for i in range(64):
            ns = []
            for k in range(cap[i]):
                r = D[p + 56 * k:p + 56 * k + 56]; ii = struct.unpack('<14i', r); f = struct.unpack('<14f', r)
                if ii[1] == 0 and 0 < ii[6] < 128 and f[2] < 1000: ns.append((round(f[2] * 4, 2), round(f[3] * 4, 2), ii[6]))
            p += 56 * cap[i]
            if ns: pats[i] = (lens[i], sorted(ns))
        out.append(pats); o += 4
ORGN, VC1, VC2, SYN8, VC3, BB1, BB2, VC4, BB3 = spats()          # the rack, in order
def tr(ns, d): return [(s, l, m + d) for s, l, m in ns]
RIFF = {}                                                        # the hook: per 16th, the chord (the C1 carrier pulses dropped)
for s, l, m in VC1[0][1]:
    if m >= 40: RIFF.setdefault(int(s), []).append(m)
ORG_BASS = ORGN[0][1]                                            # 8 bars, A major: A1 ... F#2 G#2
ORG_STAB = ORGN[4][1]                                            # 4 bars: D4/A3 over F#2 E2 G#2
def lowline(ns):   # a played-in part with stuck notes: per 16th, the lowest real note (stuck holds of a bar or more dropped)
    by = {}
    for s, l, m in ns:
        if l <= 8 and m < 45 and (s not in by or m < by[s]): by[s] = m
    return sorted((s, 1, m) for s, m in by.items())
ORG_FALL = lowline(tr(ORGN[1][1], -3))                          # 8 bars, C major -> F# minor: the organ's walking fall, F#m E D C# ...
FALL_CH = [(54, 57, 61), (52, 56, 59), (50, 54, 57), (49, 53, 56), (50, 54, 57), (49, 52, 56), (47, 50, 54), (49, 53, 56)]   # its chords, a bar each
PIANO_RH = sorted({int(s) % 16 for s, l, m in ORGN[2][1] if l < 8 and int(s) == s})   # the piano pattern's rhythm (on 16ths)
COUNTER = [x for x in tr(VC2[1][1], -3) if x[2] >= 40]                                     # 4 bars: the spare tune
ARPS = [SYN8[k][1] for k in (2, 3, 4, 5)]                        # the unused one-bar arpeggios
PADS = [(0, (57, 66)), (8, (59, 68)), (14, (61, 69))]            # the second Vocoder's pad: A/F#, B/G#, C#/A
PERC = [BB2[k][1] for k in sorted(BB2) if BB2[k][0] == 1]        # the second BeatBox's one-bar patterns
import midi_read
_div, _tr = midi_read.parse(os.path.join(HERE, "over_my_voixe.mid"))
LEAD8 = {}
for t0, ln, m, v, c in midi_read.notes(_tr[4]): LEAD8.setdefault(t0 // (_div * 4) - 15, []).append(((t0 % (_div * 4)) / (_div / 4), ln / (_div / 4), m))
KIT1 = {}
for t0, ln, m, v, c in midi_read.notes(_tr[6]): KIT1.setdefault(t0 // (_div * 4) - 16, []).append(((t0 % (_div * 4)) / (_div / 4), m))

# ---------------------------------------------------------------- the vocal lines, with words
def mono(ns):
    """The fourth Vocoder's lines are played live: one voice, the top note of a chord, a note cut where the next one starts; notes
    shorter than a 32nd are grace notes (they become part of the next syllable's slide)."""
    ns = sorted([x for x in ns if x[2] >= 40], key=lambda x: (x[0], -x[2])); out = []
    for s, l, m in ns:
        if out and s == out[-1][0]: continue
        if out and out[-1][0] + out[-1][1] > s: out[-1] = (out[-1][0], s - out[-1][0], out[-1][2])
        out.append((s, l, m))
    return [x for x in out if x[1] >= .75]
W = {'o': ['OW'], 'ver': ['V', 'ER'], 'my': ['M', 'AY'], 'voice': ['V', 'OY', 'S'], 'can': ['K', 'AE', 'N'], 'you': ['Y', 'UW'],
     'hear': ['HH', 'IY', 'R'], 'the': ['DH', 'AH'], 'sound': ['S', 'AW', 'N', 'D'], 'of': ['AH', 'V'], 'heart': ['HH', 'AA', 'R', 'T'],
     'beat': ['B', 'IY', 'T'], 'ing': ['IH', 'NG'], 'out': ['AW', 'T'], 'loud': ['L', 'AW', 'D'], "can't": ['K', 'AE', 'N', 'T'],
     'see': ['S', 'IY'], 'what': ['W', 'AH', 'T'], 'i': ['AY'], 'feel': ['F', 'IY', 'L'], 'deep': ['D', 'IY', 'P'], 'in': ['IH', 'N'],
     'side': ['S', 'AY', 'D'], 'ev': ['EH', 'V'], 'ry': ['R', 'IY'], "word's": ['W', 'ER', 'D', 'Z'], 'real': ['R', 'IY', 'L'],
     'turn': ['T', 'ER', 'N'], 'it': ['IH', 'T'], 'up': ['AH', 'P'], 'oh': ['OW'], "i'm": ['AY', 'M'], 'sing': ['S', 'IH', 'NG'],
     'to': ['T', 'UW'], 'lift': ['L', 'IH', 'F', 'T'], 'me': ['M', 'IY'], 'a': ['AH'], 'bove': ['B', 'AH', 'V'], 'high': ['HH', 'AY'],
     'er': ['ER'], 'noise': ['N', 'OY', 'Z'], 'and': ['AE', 'N', 'D'], 'lights': ['L', 'AY', 'T', 'S'], 'crowd': ['K', 'R', 'AW', 'D']}
LYRICS = {   # a syllable per note; '-' holds the last syllable's vowel onto the next note
    1: "o ver my voice - can you hear the sound of my heart beat ing out loud o ver my voice -",
    2: "you can't see what i feel deep in side ev ry word's real turn it up o ver my voice -",
    3: "oh i'm sing ing it loud to you lift me a bove my voice -",
    4: "lift it high er high er o ver the noise and the lights of the crowd o ver my voice"}
OCT = {1: 12, 2: 0, 3: 12, 4: 12}   # each line moved into the singer's middle range (around E4..B4)
def sung(k):
    ns = mono(VC4[k][1]); ws = LYRICS[k].split(); lines = []
    if k == 2: ns = [x for x in ns if x[0] >= 16]                                 # (line 2 opens on a held chord: that is the pad's)
    for i, (s, l, m) in enumerate(ns):
        w = ws[i] if i < len(ws) else '-'
        o = OCT[k]
        if w == '-' and lines: lines[-1][1].append((s, l, m + o))                 # a melisma: the same syllable, the next note
        else: lines.append((list(W[w]), [(s, l, m + o)]))
    return lines
FSV = int(16726 * max(HIFI, 2))
def vox_phrase(k):
    x = singer.sing(sung(k), FSV, BEAT, 'diva', seed=10 + k)
    return rs(x, SR, FSV)

# ---------------------------------------------------------------- sounds
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)
def verb(x, sr, t60, mix, pre=0.015, tone=4200):
    n = int(t60 * sr); t = tt(n, sr)
    ir = noise(n) * np.exp(-6.9 * t / t60); ir = lp(ir, tone, 2, sr); ir[:int(pre * sr)] = 0; ir /= np.sqrt((ir ** 2).sum())
    wet = np.concatenate([signal.fftconvolve(x, ir), [0.0]])
    y = np.concatenate([x, np.zeros(n)]); return y + wet * mix * np.abs(x).max() / (np.abs(wet).max() + 1e-9)
def env(n, att, dec, sr=SR, sus=0.0, rel=None):
    t = tt(n, sr); e = np.minimum(1, t / max(att, 1e-4)) * (sus + (1 - sus) * np.exp(-t / dec))
    if rel: e *= np.minimum(1, (t[-1] - t) / rel)
    return e
for k in (1, 2, 3, 4): add('voc%d' % k, 'sung line %d' % k, finish(verb(vox_phrase(k), SR, 1.2, .22), .9, .05), None)   # played at C-4: its own pitch
for w, base in (('o', 72), ('ver', 72), ('my', 72), ('voice', 72)):                                                    # the chops (C5)
    x = singer.sing([(W[w], [(0, 1.6, base)])], FSV, BEAT, 'diva', seed=3, scale=.7)
    x = rs(x, SR, FSV)
    add('chop_' + w, 'sung ' + w, finish(verb(x, SR, .5, .18), .85, .02), base)
for v, nm in (('AA', 'ahh'), ('UW', 'ooh')):                                                                        # the choir (C4): three singers
    xs = [singer.sing([([v], [(0, 14, 60)])], FSV, BEAT, 'soft', seed=20 + j) for j in range(3)]
    n = min(len(x) for x in xs); x = sum(np.interp(np.arange(n) * 2 ** (c / 1200), np.arange(len(xs[j])), xs[j]) for j, c in enumerate((-9, 0, 8)))
    x = rs(x, SR2, FSV); t = tt(len(x), SR2)
    add('choir_' + nm, 'sung ' + nm, finish(verb(x * np.minimum(1, t / .25), SR2, 1.4, .35)[:int(3.2 * SR2)], .6, .3, SR2), 60, 5)
n = int(.32 * SR); t = tt(n)                                                       # house kick
ph = 2 * np.pi * np.cumsum(50 + 125 * np.exp(-t / .03) + 280 * np.exp(-t / .003)) / SR
add('kick', 'house kick', finish(np.tanh(1.8 * np.sin(ph) * np.exp(-t / .15)), .98))
n = int(.36 * SR); t = tt(n); c = np.zeros(n)
for off, dec, a in ((0, .005, .6), (.010, .005, .8), (.021, .006, .85), (.030, .08, 1.0)):
    i = int(off * SR); m = n - i; c[i:] += noise(m) * np.exp(-tt(m) / dec) * a
add('clap', 'clap', finish(verb(bp(c, 900, 3800, 2), SR, .4, .3), .82))
n = int(.18 * SR); t = tt(n)
add('snare', 'roll snare', finish(bp(noise(n), 1600, 7000, 2) * np.exp(-t / .045) + np.sin(2 * np.pi * 210 * t) * np.exp(-t / .025) * .5, .7))
def metal(n, sr=SR):
    t = tt(n, sr); return sum(np.sign(np.sin(2 * np.pi * f * t + rng.uniform(0, 6))) for f in (205, 304, 369, 437, 532, 812)) / 6
n = int(.05 * SR); t = tt(n); add('hat', 'hat', finish(hp(metal(n) * .5 + noise(n), 7200, 4) * np.exp(-t / .010), .45))
n = int(.3 * SR); t = tt(n); add('ohat', 'open hat', finish(hp(metal(n) * .6 + noise(n), 6000, 4) * np.exp(-t / .11), .45))
n = int(.28 * SR); t = tt(n); f = mid2f(57)                                        # conga (A3)
add('conga', 'conga', finish(np.sin(2 * np.pi * f * t * (1 + .25 * np.exp(-t / .01))) * np.exp(-t / .12) + .2 * bp(noise(n), 300, 2000) * np.exp(-t / .01), .7), 57)
n = int(.5 * SR); t = tt(n); f = mid2f(45) * (1 + 1.2 * np.exp(-t / .045))         # Simmons tom (A2)
add('tom', 'simmons tom', finish(np.sin(2 * np.pi * np.cumsum(f) / SR) * np.exp(-t / .2) + bp(noise(n), 1500, 5000) * np.exp(-t / .015) * .4, .8), 45)
def organ(f, n, sr, perc=1):
    t = tt(n, sr); x = sum(a * (np.sin(2 * np.pi * f * h * t) + np.sin(2 * np.pi * f * h * 1.0025 * t + 1)) for h, a in ((1, 1), (2, .8), (3, .45), (4, .4), (6, .15)))
    return x * np.minimum(1, t / .002) * (np.exp(-t / .12) * perc + .3)
n = int(.3 * SR); t = tt(n)                                                        # the house organ bass (A1): the classic M1 organ, low
add('obass', 'organ bass', finish(np.tanh(1.3 * lp(organ(mid2f(33), n, SR), 1800) + np.sin(2 * np.pi * mid2f(33) * t)) * env(n, .002, .2, sus=.3, rel=.03), .95), 33)
n = int(.6 * SR); add('organ', 'organ stab', finish(verb(lp(organ(mid2f(60), n, SR) * np.exp(-tt(n) / .22), 4200), SR, .5, .22), .78), 60)
n = int(1.6 * SR); t = tt(n); f = mid2f(60)                                        # piano (C4)
pn = np.sin(2 * np.pi * f * t + 1.6 * np.exp(-t / .25) * np.sin(2 * np.pi * f * t)) + .35 * np.sin(4 * np.pi * f * t) * np.exp(-t / .3)
add('piano', 'piano', finish(verb(pn * np.exp(-t / .7) * np.minimum(1, t / .002), SR, .9, .3), .7, .05), 60)
n = int(.26 * SR2); t = tt(n, SR2); f = mid2f(60)                                  # brass stab (C4)
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6)) for c in (-9, 0, 9)) / 3
add('brass', 'brass stab', finish(verb(lp(x, 3200, 2, SR2) * np.exp(-t / .12) * np.minimum(1, t / .01), SR2, .5, .3), .62, .02, SR2), 60, 5)
n = int(.6 * SR2); t = tt(n, SR2); f = mid2f(60)                                   # orchestra hit (C4)
oh = sum(a * saw(f * r, n, SR2, ph=rng.uniform(0, 6)) for r, a in ((0.5, .8), (1, 1), (1.5, .6), (2, .7), (3, .3)))
add('orch', 'orchestra hit', finish(verb(lp(oh, 4000, 2, SR2) * np.exp(-t / .16) + bp(noise(n), 800, 4000, 2, SR2) * np.exp(-t / .03) * .7, SR2, .8, .3), .8, .05, SR2), 60, 5)
n = int(.45 * SR); t = tt(n); f = mid2f(72)                                        # the 8BitSynth (C5): a pulse with a fast arp-free attack
add('chip', '8bit lead', finish(lp(pulse(f, n, .25) * .8 + pulse(f * 1.003, n, .5) * .3, 6000) * env(n, .002, .3, sus=.4, rel=.05), .6), 72)
n = int(1.2 * SR2); t = tt(n, SR2); f = mid2f(84)                                  # chimes (C6)
add('chime', 'chime', finish(np.sin(2 * np.pi * f * t) * np.exp(-t / .5) + .5 * np.sin(2 * np.pi * f * 2.76 * t) * np.exp(-t / .18), .55, .05, SR2), 84, 5)
n = int(.14 * SR); t = tt(n); f = mid2f(72)                                        # arp pluck (C5)
add('pluck', 'arp pluck', finish(np.sin(2 * np.pi * f * t + 2.0 * np.exp(-t / .03) * np.sin(2 * np.pi * 2 * f * t)) * np.exp(-t / .06), .6), 72)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(48) * (1 - .25 * np.exp(-t / .07))   # hoover (C3)
hv = sum(saw(mid2f(48) * 2 ** (c / 1200), n, SR2, vib=f * 2 ** (c / 1200)) for c in (-25, -12, 0, 12, 25)) / 5
add('hoover', 'hoover', finish(lp(np.tanh(2 * hv), 3000, 2, SR2) * np.minimum(1, t / .02) * np.minimum(1, (t[-1] - t) / .4), .6, .05, SR2), 48, 5)
n = int(2 * 4 * BEAT * SR2); t = tt(n, SR2); u = t / t[-1]; nz = noise(n)          # riser (2 bars), crash, impact
add('riser', 'riser', finish((bp(nz, 250, 900, 2, SR2) * (1 - u) + bp(nz, 1200, 3600, 2, SR2) * u) * u ** 2, .5, .01, SR2), None, 5)
n = int(1.4 * SR2); t = tt(n, SR2); add('crash', 'crash', finish(hp(metal(n, SR2) * .4 + noise(n), 3200, 3, SR2) * np.exp(-t / .55), .5, .05, SR2), None, 5)
n = int(1.3 * SR2); t = tt(n, SR2); ph = 2 * np.pi * np.cumsum(30 + 80 * np.exp(-t / .08)) / SR2
add('impact', 'impact', finish(np.sin(ph) * np.exp(-t / .5) + hp(noise(n), 1500, 2, SR2) * np.exp(-t / .25) * .45, .9, .05, SR2), None, 5)
KEYS = ['kick', 'clap', 'snare', 'hat', 'ohat', 'conga', 'tom', 'obass', 'organ', 'piano', 'brass', 'orch', 'chip', 'chime', 'pluck', 'hoover',
        'riser', 'crash', 'impact', 'choir_ahh', 'choir_ooh', 'chop_o', 'chop_ver', 'chop_my', 'chop_voice', 'voc1', 'voc2', 'voc3', 'voc4']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

# ---------------------------------------------------------------- one bar
def build_bar(p, P, base, bi):
    def put(r, ch, key, midi=None, vol=64):
        r = int(round(r)) + base
        if base <= r < base + BR and vol > 0: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, int(vol))))
    k = p.get('k', 0)                                    # this bar's place in its section (0, 1, 2 ...)
    d = p.get('drums', 0)
    if d >= 1:
        for b in range(4): put(b * 16, 0, 'kick', None, 60 if b % 2 == 0 else 56)
    if d >= 2:
        put(16, 1, 'clap', None, 54); put(48, 1, 'clap', None, 56)
        for b in range(4): put(b * 16 + 8, 3, 'ohat', None, 30)
        for st in range(16):
            if st % 4 and st % 4 != 2: put(st * Q, 2, 'hat', None, 20)
        for st, m in KIT1.get(k % 16, []):                                                       # the first BeatBox's pattern on the hats
            if int(st) == st and st % 2 == 0 and st % 4: put(st * Q, 2, 'hat', None, 12 if m in (52, 54) else 8)
    if p.get('perc'):                                                                            # the second BeatBox: congas and toms
        for st, l, m in PERC[(bi // 2) % len(PERC)]:
            if m in (48, 50): put(st * Q, 13, 'conga', 57 + (m - 48) * 2, 26)
            elif m in (51, 52, 53): put(st * Q, 13, 'tom', 45 + (m - 51) * 3, 22)
    if d == 1 and p.get('half'): put(32, 1, 'clap', None, 40)
    if p.get('roll'):
        for r in range(BR):
            step = 8 if r < 16 else 4 if r < 32 else 2 if r < 48 else 1
            if r % step == 0: put(r, 1, 'snare', None, p['roll'] * (.35 + .65 * r / BR))
    if p.get('riser'): put(0, 15, 'riser', None, p['riser'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    # ---- the organ: its walking bass, its stabs, the fall; the piano
    if p.get('obass'):
        for st, l, m in ORG_BASS:
            if k % 8 * 16 <= st < (k % 8 + 1) * 16: put((st - k % 8 * 16) * Q, 4, 'obass', m, p['obass'] - (0 if st % 4 == 0 else 8))
    if p.get('stabs'):
        for st, l, m in ORG_STAB:
            if m >= 50 and k % 4 * 16 <= st < (k % 4 + 1) * 16: put((st - k % 4 * 16) * Q, 5, 'organ', m, p['stabs'] - (0 if st % 2 == 0 else 10))
    if p.get('fall'):
        for st, l, m in ORG_FALL:
            if k % 8 * 16 <= st < (k % 8 + 1) * 16: put((st - k % 8 * 16) * Q, 4, 'obass', m, p['fall'] - (0 if st % 4 == 0 else 10))
    if p.get('piano'):
        ch = FALL_CH[k % 8]
        for st in PIANO_RH: put(st * Q, 5, 'piano', ch[(st // 3) % 3] + 12, p['piano'] - (0 if st % 4 == 0 else 8))
    # ---- the voice
    if p.get('choir'):
        for s, (a, b) in PADS:
            put(s * Q, 6, 'choir_ahh', a, p['choir']); put(s * Q, 7, 'choir_ahh', b, p['choir'] - 4); put(s * Q, 8, 'choir_ooh', b - 12, p['choir'] - 8)
    if p.get('chops'):                                                                          # the hook: the first Vocoder's riff, sung in syllables
        syl = ('o', 'ver', 'my', 'voice')
        for h, st in enumerate(sorted(RIFF)):
            for j, m in enumerate(sorted(RIFF[st])[-3:]): put(st * Q, 6 + j, 'chop_' + syl[h % 4], m - 12, p['chops'] - j * 4)
    if p.get('brass'):                                                                          # brass doubles the riff's two big chords
        for st in (1, 8): put(st * Q, 5, 'brass', max(RIFF[st]) - 12, p['brass'])
    if p.get('voc') and k % 8 == 0:                                                             # a whole sung line (8 bars), doubled a 64th late
        put(0, 9, 'voc%d' % p['voc'], None, p.get('vv', 60)); put(1, 10, 'voc%d' % p['voc'], None, p.get('vv', 60) * .55)
    # ---- the 8BitSynth, the arps, the counter-melody, the hits
    if p.get('lead'):
        for s, l, m in LEAD8.get(p.get('lk', k) % 22, []): put(s * Q, 11, 'chip', m + p.get('lo', 0), p['lead']); put(s * Q + 12, 12, 'chip', m + p.get('lo', 0), p['lead'] * .35)
    if p.get('arp'):
        for s, l, m in ARPS[k % 4]: put(s * Q, 13, 'pluck', m + 12, p['arp'] - (0 if s % 4 == 0 else 8))
    if p.get('counter'):
        for s, l, m in COUNTER:
            if k % 4 * 16 <= s < (k % 4 + 1) * 16: put((s - k % 4 * 16) * Q, 12, 'chime', m + 12, p['counter'])
    if p.get('orch') and k % 2 == 0: put(0, 14, 'orch', 57, p['orch'])
    if p.get('hoover') and k % 4 == 0: put(0, 14, 'hoover', 42, p['hoover'])

# ---------------------------------------------------------------- arrangement
S = []
def sec(n, **kw):
    for i in range(n): d = dict(kw); d['k'] = i; S.append(d)
sec(8, choir=40, chops=26, drums=0)                                                         # intro: the choir, the hook sung softly
for i in range(4, 8): S[-8 + i]['drums'] = 1
S[-8]['fx'] = ((0, 'impact', 44),); S[-2]['riser'] = 44; S[-1]['roll'] = 40
def verse(l1, l2):
    sec(16, drums=2, obass=54, choir=34, perc=1, stabs=0)
    S[-16]['voc'] = l1; S[-8]['voc'] = l2; S[-16]['fx'] = ((0, 'crash', 46),)
def pre():
    sec(8, drums=2, obass=50, arp=40, stabs=40, choir=30)
    S[-8]['fx'] = ((0, 'crash', 40),); S[-2]['riser'] = 50; S[-1]['roll'] = 52
def chorus():
    sec(16, drums=2, obass=58, chops=50, brass=40, lead=46, orch=50, perc=1)
    S[-16]['voc'] = 3; S[-8]['voc'] = 4; S[-16]['fx'] = ((0, 'impact', 60), (2, 'crash', 52))
    for i in range(16): S[-16 + i]['lk'] = i + (2 if i >= 8 else 0)
verse(1, 2); pre(); chorus()
sec(16, choir=40, fall=46, piano=40, counter=38)                                            # breakdown: the Andalusian fall, piano, chimes, the voice
S[-16]['voc'] = 1; S[-16]['fx'] = ((0, 'impact', 50),)
for i in range(8, 16): S[-16 + i].update(drums=1, half=1)
S[-8]['hoover'] = 44; S[-4]['hoover'] = 44; S[-2]['riser'] = 54; S[-1]['roll'] = 56
verse(2, 1); pre(); chorus()
sec(8, chops=40, drums=1, choir=34, counter=30)                                              # outro
S[-8]['voc'] = 1; S[-8]['vv'] = 50; S[-8]['fx'] = ((0, 'crash', 44),)
for i in range(4, 8): S[-8 + i]['drums'] = 0

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for k in range(0, len(S), 2):
        P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
        for j in range(2):
            if k + j < len(S): build_bar(S[k + j], P, j * BR, k + j)
        key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    end = [[(0, 0, 0)] * NCH for _ in range(ROWS)]                                         # the last chord, sung
    end[0][15] = (49, INST['impact'], 0x10 + 56); end[0][4] = (xmn(30, 'obass'), INST['obass'], 0x10 + 54)
    for j, m in enumerate((57, 61, 66)): end[0][6 + j] = (xmn(m, 'choir_ahh'), INST['choir_ahh'], 0x10 + 44)
    end[0][9] = (xmn(78, 'chop_voice'), INST['chop_voice'], 0x10 + 50)
    pats.append(end); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'Over My Voice'[:20].ljust(20) + b'\x1a' + b'make_overmyvoice'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d bars, %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(S), len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
    for k in (1, 2, 3, 4): print('  line %d: %d syllables over %d notes' % (k, len(sung(k)), sum(len(n) for _, n in sung(k))))
if __name__ == '__main__': main()
