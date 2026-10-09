#!/usr/bin/env python3
"""HERE TODAY (MISSY'S SONG), v4 (voice an octave up, female and emotional): renders the author's MIDI (tools/here_today.mid: female voice, nylon guitar, string quartet) to source/music/here_today.adp
and re-times the scene-5 lyric beats (source/cutscene.h) and camera shots (source/csshot.h) to its phrases.  Run from the project root: python3 tools/make_heretoday_midi.py"""
import sys, os, re, wave
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import midi_read as M
from encode_sfx import encode
import encode_song as ES
SR = 22050; TRIM = 0.80; FPS = 59.73
div, tr = M.parse('tools/here_today.mid')
tempos = [(e[0], int.from_bytes(e[3], 'big')) for e in tr[0] if e[1] == 'meta' and e[2] == 0x51]
def sec(t):
    s = 0; pt = 0; tp = 500000
    for tk, v in tempos:
        if tk >= t: break
        s += (tk - pt) * tp / div / 1e6; pt = tk; tp = v
    return s + (t - pt) * tp / div / 1e6
def notes(ev):
    on = {}; out = []
    for e in ev:
        if e[1] != 'ev': continue
        k = e[2] >> 4
        if k == 9 and e[4] > 0: on[e[3]] = (e[0], e[4])
        elif k == 8 or (k == 9 and e[4] == 0):
            if e[3] in on: t0, v = on.pop(e[3]); out.append((sec(t0), sec(e[0]), e[3], v))
    return sorted(out)
hz = lambda m: 440 * 2 ** ((m - 69) / 12)
total = sec(90000) + 2.5; N = int(total * SR); out = np.zeros(N)
def put(t0, x, g):
    i = int(t0 * SR); x = x[:N - i]; out[i:i + len(x)] += x * g
def env(n, a, r): e = np.ones(n); k = min(n, int(a * SR)); e[:k] = np.linspace(0, 1, k); k = min(n, int(r * SR)); e[-k:] *= np.linspace(1, 0, k); return e
def additive(f, n, w):
    t = np.arange(n) / SR; return sum(a * np.sin(2 * np.pi * f * (k + 1) * t) for k, a in enumerate(w) if f * (k + 1) < SR * .45)
rng = np.random.default_rng(7)
VOICE_OCT = 12                                         # the melody sits one octave up (a female range, B3 to G5)
def formant(x, fc, bw, g):                             # one vocal-tract resonance
    b, a = signal.butter(2, [(fc - bw / 2) / (SR / 2), (fc + bw / 2) / (SR / 2)], 'band'); return g * signal.lfilter(b, a, x)
vn = notes(tr[1])
for i, (s, e, m, v) in enumerate(vn):                  # voice: a breathy, emotional female vocal ("ah"), glide in, late vibrato, swell
    prev = vn[i - 1] if i else None; legato = prev is not None and s - prev[1] < .12
    n = int((e - s + .2) * SR); t = np.arange(n) / SR; dur = e - s; mt = m + VOICE_OCT; f0 = hz(mt)
    slide = np.zeros(n)                                # scoop up into the note from below (a bigger sigh after a rest), or glide from the last pitch
    st = (prev[2] - m) if legato and abs(prev[2] - m) <= 5 else -1.4
    slide = st * np.exp(-t / .07)
    vd = np.clip((t - min(.35, dur * .45)) / .5, 0, 1)  # vibrato blooms late and widens on long notes, a touch of tremor
    vib = (.011 + .006 * min(1, dur / 1.5)) * np.sin(2 * np.pi * (5.6 + .5 * np.sin(2 * np.pi * .3 * t)) * t) * vd
    f = f0 * 2 ** (slide / 12) * (1 + vib) * (1 + .0012 * rng.standard_normal(n).cumsum() / np.sqrt(n) * 4)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = sum(np.sin((k + 1) * ph + .3 * k) / (k + 1) ** 1.05 for k in range(24) if f.max() * (k + 1) < SR * .45)   # brighter glottal source
    nz = rng.standard_normal(n); nz = signal.lfilter(*signal.butter(2, 2500 / (SR / 2), 'high'), nz)
    air = (.05 + .09 * np.exp(-t / .12)) * nz * (1 + .4 * np.sin(2 * np.pi * 5.6 * t))   # breath, strongest on the onset
    bright = min(1.0, max(0.0, (mt - 62) / 14))
    x = formant(x, 850 + 150 * bright, 220, 1.5) + formant(x, 1250 + 200 * bright, 300, .9) + formant(x, 2900, 500, .5) + .35 * x + air   # female vowel formants
    swell = 1 + .28 * np.clip(np.sin(np.pi * np.clip(t / max(dur, .3), 0, 1)), 0, 1) * min(1, dur / 1.2)   # long notes swell, then fall away
    put(s, x * env(n, .035 if not legato else .02, .22) * swell, .15 * (.55 + .45 * v / 100) * (1 + .12 * bright))
for (s, e, m, v) in notes(tr[3]):                      # nylon guitar: plucked partials with their own decays
    n = int(min(e - s + .4, 3.5) * SR); t = np.arange(n) / SR; f = hz(m)
    x = sum(np.sin(2 * np.pi * f * k * t) * np.exp(-t * (2 + 1.6 * k)) / k for k in range(1, 9) if f * k < SR * .45)
    put(s, x * env(n, .003, .05), .24 * v / 100)
for tk, g in ((4, .11), (5, .11), (6, .12), (7, .14)):  # strings: soft detuned saws, slow bloom
    for (s, e, m, v) in notes(tr[tk]):
        n = int((e - s + .5) * SR); t = np.arange(n) / SR; f = hz(m); x = 0
        for c in (-7, 0, 7): x = x + signal.sawtooth(2 * np.pi * f * 2 ** (c / 1200) * t + rng.uniform(0, 6))
        b, a = signal.butter(2, 1800 / (SR / 2)); x = signal.lfilter(b, a, x)
        put(s, x * env(n, .35, .5), g * v / 100)
out = out[int(TRIM * SR):]
out = signal.lfilter(*signal.butter(1, 60 / (SR / 2), 'high'), out); out /= np.abs(out).max() / .9
w = wave.open(__import__('os').environ.get('TMPDIR','.') + '/here_today.wav', 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((out * 32767).astype('<i2').tobytes()); w.close()
x = ES.load(__import__('os').environ.get('TMPDIR','.') + '/here_today.wav'); open('source/music/here_today.adp', 'wb').write(encode(x)); print('adp', len(x) / ES.RATE, 's')
# ---- phrases -> lyric beats
v = notes(tr[1]); ph = [[v[0]]]
for n in v[1:]:
    (ph[-1].append(n) if n[0] - ph[-1][-1][1] <= .45 else ph.append([n]))
P = [p[0][0] - TRIM for p in ph]; END = ph[-1][-1][1] - TRIM + 1.2; assert len(P) == 20, len(P)
L = open('source/cutscene.h', encoding='utf-8').read().split('\n'); s = next(i for i, l in enumerate(L) if 'csS5[]' in l)
idx = [i for i in range(s + 1, len(L)) if L[i].startswith(' {')]; lyr = list(range(10, 29)) + [30]; lens = {}
for k, bi in enumerate(lyr):
    gap = (P[k + 1] - P[k]) if k < 19 else (END - P[19]); fr = round(gap * FPS)
    if bi == 28: fr -= 40
    if bi == 30: fr -= 41
    line = L[idx[bi]]; txt = re.findall(r'\{"([^"]*)"', line)[0]; d = max(20, fr - 2 * len(txt) - 1)
    L[idx[bi]] = re.sub(r'(CF_AUTO,\d+,)\d+(,"Missy")', lambda m: m.group(1) + str(d) + m.group(2), line); lens[bi] = d + 2 * len(txt) + 1
lens[29] = 40; lens[31] = 41
L = '\n'.join(L).replace('u8 bg, a, pa, ax, b, pb, bx, fx, sfx, dur;', 'u8 bg, a, pa, ax, b, pb, bx, fx, sfx; u16 dur;')
open('source/cutscene.h', 'w', encoding='utf-8').write(L)
C = open('source/csshot.h', encoding='utf-8').read().split('\n')
for i, l in enumerate(C):
    m = re.match(r'(\s*\{5,(\d+),\d+,\s*0,\s*)\d+(,.*)', l)
    if m and int(m.group(2)) in lens: C[i] = m.group(1) + str(lens[int(m.group(2))]) + m.group(3)
open('source/csshot.h', 'w', encoding='utf-8').write('\n'.join(C)); print('retimed', lens)
