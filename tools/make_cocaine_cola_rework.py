#!/usr/bin/env python3
"""Builds tools/cocaine_cola_ii.xm : "Cocaine Cola" (the rework) of the stem transcription tools/cocaine_cola.xm.
The transcription stays in the game as the secret song COCAINE COLA (ORIGINAL) (the title-screen code reveals it).

usage:  python3 tools/make_cocaine_cola_rework.py        (run from the project root; needs numpy + scipy)

What is kept, read out of tools/cocaine_cola.xm bar by bar: the form (220 bars at 120 BPM, every break and drop where it was), the kick /
snare / hat / tom positions and their fills, the bass riff (C# G# B | F# C# E, with its turnarounds) and the melody. In the loud half of the
song the lead is a bent, vocal-like line that the transcription caught slightly differently on every pass: the 8 passes are voted step by
step into ONE 8-bar hook (about 80 % of the steps agree), snapped to the C# / F# scale, and that hook is what you hear.
New: every sound. 808 sub (short + long) with a saw growl on top, punchy kick, snare + clap, metallic hats, shaker, toms, a phonk cowbell
that answers the hook, a detuned hook synth (staccato / medium / long variants so every note ends with its own release), a formant
choir, breathing pads, quartal stabs, glass bells, risers, reverse swells, impacts and crashes. Rows are 1/96 of a bar (speed 1,
XM BPM 120): a 16th is 6 rows, so hat rolls and flams sit on 32nds and triplets.
Channels: 0 kick | 1 808 | 2 growl | 3 snare | 4 clap / rim | 5 hats | 6 shaker | 7 tom / cowbell | 8 lead | 9 echo / bell | 10 11 pad
          12 13 stab | 14 choir | 15 fx.     Uses only one-shot samples, the note column and the volume column.
Re-running gives byte-identical output.
"""
import os, sys, struct, collections
import numpy as np
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xm

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "cocaine_cola.xm")
OUT = os.path.join(HERE, "cocaine_cola_ii.xm")
BPM, SPEED, ROWS, NCH = 120, 1, 96, 16
SR = 22050
rng = np.random.default_rng(120)

# =============================================================== sound design
def tt(n, sr=SR): return np.arange(n) / sr
def noise(n): return rng.uniform(-1, 1, n)
def sos_f(x, kind, fc, order=2, sr=SR):
    w = np.array(fc, float) / (sr / 2)
    return signal.sosfilt(signal.butter(order, np.clip(w, 1e-4, 0.99), kind, output='sos'), x)
def lp(x, fc, order=2, sr=SR): return sos_f(x, 'low', fc, order, sr)
def hp(x, fc, order=2, sr=SR): return sos_f(x, 'high', fc, order, sr)
def bp(x, f1, f2, order=2, sr=SR): return sos_f(x, 'band', [f1, f2], order, sr)
def sweep_lp(x, f0, f1, sr=SR, block=128):
    """low-pass whose cut-off glides exponentially f0 -> f1 over the signal (block-wise, filter state carried)"""
    out = np.zeros_like(x); zi = None; nb = int(np.ceil(len(x) / block))
    for k in range(nb):
        fc = f0 * (f1 / f0) ** (k / max(1, nb - 1)); b, a = signal.butter(2, min(fc, sr * .45) / (sr / 2), 'low')
        seg = x[k * block:(k + 1) * block]
        if zi is None: zi = np.zeros(2)
        y, zi = signal.lfilter(b, a, seg, zi=zi); out[k * block:(k + 1) * block] = y
    return out
def env(n, tau, att=0.002, sr=SR):
    t = tt(n, sr); return np.minimum(1, t / att) * np.exp(-t / tau)
def fade_out(x, ms=12, sr=SR):
    f = max(2, int(ms / 1000 * sr)); x = x.copy(); x[-f:] *= np.linspace(1, 0, f); return x
def mid2f(m): return 440.0 * 2 ** ((m - 69) / 12)
def saw_add(f, n, sr=SR, ph=0.0, fmax=7000, vib=None):
    """band-limited saw (additive); vib = per-sample frequency multiplier"""
    t = tt(n, sr); fm = np.ones(n) if vib is None else vib
    phase = 2 * np.pi * np.cumsum(f * fm) / sr + ph; x = np.zeros(n)
    for k in range(1, max(2, int(min(fmax, sr * .45) / f)) + 1): x += np.sin(k * phase) / k
    return x * (2 / np.pi)
def finish(x, peak, ms=12, sr=SR):
    x = fade_out(np.asarray(x, float), ms, sr); m = np.abs(x).max(); return x / m * peak if m > 0 else x

I = collections.OrderedDict()      # key -> dict(name, x, gen (midi the sample sounds at, or None), sr)
def add(k, name, x, gen=None, sr=SR): I[k] = dict(name=name, x=x, gen=gen, sr=sr)

def kick(n, f0, f1, tau, drive, tail=0.4, click=0.5):
    t = tt(n); f = f1 + (f0 - f1) * np.exp(-t / 0.026); ph = 2 * np.pi * np.cumsum(f) / SR
    x = np.sin(ph) * env(n, tau, .001) + tail * np.sin(2 * np.pi * f1 * t) * np.exp(-t / (tau * 1.5))
    x += click * hp(noise(n), 1800) * np.exp(-t / .0035)
    return np.tanh(drive * x)
add('kick', 'kick', finish(lp(kick(int(.5 * SR), 190, 44, .15, 1.7), 6500), .95))
add('kick_g', 'kick ghost', finish(lp(kick(int(.22 * SR), 130, 56, .07, 1.3, .2, .3), 5000), .62))

def sub808(n, f0, tau, dive=1.0, drive=2.3):
    t = tt(n); f = f0 * (1 + dive * np.exp(-t / 0.018)); ph = 2 * np.pi * np.cumsum(f) / SR
    x = np.sin(ph) * env(n, tau, .004); x = np.tanh(drive * x) / np.tanh(drive)
    return lp(x + .10 * np.sin(2 * ph) * env(n, tau * .5, .004), 2200)
F808 = mid2f(37)
add('sub_s', '808 short', finish(sub808(int(.30 * SR), F808, .09), .68, 14), 37)
add('sub_l', '808 long', finish(sub808(int(1.0 * SR), F808, .33), .68, 40), 37)
FG = mid2f(49); n_ = int(.55 * SR); t_ = tt(n_)
g = sum(saw_add(FG * 2 ** (c / 1200), n_, ph=p) for c, p in ((-7, 0.0), (7, 1.7))) / 2 + .55 * np.sign(np.sin(2 * np.pi * FG / 2 * t_))
g = np.tanh(2.4 * sweep_lp(g, 2600, 420)) * env(n_, .2, .003)
add('growl', 'growl layer', finish(g, .52, 20), 49)

n_ = int(.42 * SR); t_ = tt(n_)
sn = np.sin(2 * np.pi * np.cumsum(180 + 70 * np.exp(-t_ / .022)) / SR) * np.exp(-t_ / .075) * .8 + bp(noise(n_), 900, 8500) * np.exp(-t_ / .105) + hp(noise(n_), 4200) * np.exp(-t_ / .022) * .55
add('snare', 'snare', finish(np.tanh(1.35 * sn), .82, 20))
n_ = int(.36 * SR); t_ = tt(n_); cl = np.zeros(n_)
for k, d in enumerate((0, .011, .0225, .032)):
    i0 = int(d * SR); m = min(int(.03 * SR), n_ - i0); cl[i0:i0 + m] += bp(noise(m), 900, 3200) * np.exp(-tt(m) / .0065) * (1 - .12 * k)
i0 = int(.032 * SR); cl[i0:] += bp(noise(n_ - i0), 1000, 2600) * np.exp(-tt(n_ - i0) / .085) * .5
add('clap', 'clap', finish(cl, .70, 20))
n_ = int(.11 * SR); t_ = tt(n_)
add('rim', 'rim', finish(np.sin(2 * np.pi * 1750 * t_) * np.exp(-t_ / .008) + .6 * hp(noise(n_), 3000) * np.exp(-t_ / .005), .42, 8))

def metal(n, tau, lo=7200):
    t = tt(n); x = sum(np.sign(np.sin(2 * np.pi * f * t)) for f in (205.3, 304.4, 369.6, 522.7, 540.0, 800.0)) / 6
    return hp(x * 1.0 + .7 * noise(n), lo, 3) * env(n, tau, .0005)
add('hat_c', 'hat closed', finish(metal(int(.13 * SR), .021), .5, 10))
add('hat_o', 'hat open', finish(metal(int(.55 * SR), .17, 6500), .5, 40))
n_ = int(.2 * SR); t_ = tt(n_)
add('shaker', 'shaker', finish(bp(noise(n_), 4300, 9800) * (1 - np.exp(-t_ / .008)) * np.exp(-t_ / .05), .34, 15))
FT = mid2f(52); n_ = int(.5 * SR); t_ = tt(n_)
tm = np.sin(2 * np.pi * np.cumsum(FT * (1 + .7 * np.exp(-t_ / .045))) / SR) * env(n_, .2, .002) + .35 * hp(noise(n_), 1500) * np.exp(-t_ / .006)
add('tom', 'tom', finish(np.tanh(1.4 * tm), .72, 25), 52)
FC = mid2f(80); n_ = int(.5 * SR); t_ = tt(n_)
cb = (np.sign(np.sin(2 * np.pi * FC * t_)) + np.sign(np.sin(2 * np.pi * FC * 1.482 * t_))) * (.55 * np.exp(-t_ / .012) + .6 * np.exp(-t_ / .105))
add('cowbell', 'cowbell', finish(bp(cb, 650, 3400), .75, 30), 80)

def hook(n, f0, att, dec_to, tau_f, rel_ms, vib_depth=0.0035, cut=(4300, 1700)):
    t = tt(n); vib = 1 + vib_depth * np.clip((t - .12) / .2, 0, 1) * np.sin(2 * np.pi * 5.4 * t)
    x = sum(saw_add(f0 * 2 ** (c / 1200), n, ph=p, vib=vib) for c, p in ((-14, 0), (-7, 1.1), (0, 2.2), (7, 3.3), (14, 4.4))) / 5
    x = x + .45 * np.sin(2 * np.pi * np.cumsum(f0 / 2 * vib) / SR)
    x = sweep_lp(x, cut[0], cut[1], block=96) if tau_f else lp(x, cut[1])
    a = np.minimum(1, t / att) * (dec_to + (1 - dec_to) * np.exp(-t / .22))
    r = int(rel_ms / 1000 * SR); a[-r:] *= np.linspace(1, 0, r) ** 1.5
    return np.tanh(1.3 * x * a)
FL = mid2f(68)
add('lead_s', 'hook short', finish(hook(int(.30 * SR), FL, .004, .0, 1, 130), .92, 5), 68)
add('lead_m', 'hook medium', finish(hook(int(.62 * SR), FL, .005, .5, 1, 170), .92, 5), 68)
add('lead_l', 'hook long', finish(hook(int(1.2 * SR), FL, .006, .72, 1, 280), .92, 5), 68)
n_ = int(2.5 * SR); t_ = tt(n_); vib = 1 + .004 * np.sin(2 * np.pi * 5 * t_ + 1)
ch = sum(saw_add(FL * 2 ** (c / 1200), n_, ph=rng.uniform(0, 6.28), vib=vib, fmax=3500) for c in (-20, -12, -5, 0, 5, 12, 20)) / 7
ch = bp(ch, 300, 700, 2) * 1.0 + bp(ch, 800, 1250, 2) * .7 + lp(ch, 300) * .3
a = np.minimum(1, t_ / .35) * np.minimum(1, (t_[-1] - t_) / .55)
add('choir', 'choir ooh', finish(ch * a, .70, 5), 68)
n_ = int(1.5 * SR); t_ = tt(n_); FB = mid2f(75)
idx = 3.6 * np.exp(-t_ / .22)
bl = np.sin(2 * np.pi * FB * t_ + idx * np.sin(2 * np.pi * FB * 3.51 * t_)) * np.exp(-t_ / .55) + .3 * np.sin(2 * np.pi * FB * 2 * t_) * np.exp(-t_ / .25)
add('bell', 'glass bell', finish(bl, .55, 40), 75)
n_ = int(2.15 * SR); t_ = tt(n_); FPD = mid2f(60)
pad = sum(saw_add(FPD * 2 ** (c / 1200), n_, ph=p, fmax=3000) for c, p in ((-9, 0), (-3, 1), (3, 2), (9, 3))) / 4 + .5 * np.sin(2 * np.pi * FPD / 2 * t_)
pad = sweep_lp(pad, 650, 1250, block=256) * np.minimum(1, t_ / .5) ** 1.3 * np.minimum(1, (t_[-1] - t_) / .45)
add('pad', 'breathing pad', finish(pad, .55, 5), 60)
n_ = int(.7 * SR); t_ = tt(n_); FS = mid2f(64)
st = np.sin(2 * np.pi * FS * t_ + 1.7 * np.exp(-t_ / .11) * np.sin(2 * np.pi * FS * t_)) * np.exp(-t_ / .26) + .18 * np.sin(2 * np.pi * FS * 14 * t_) * np.exp(-t_ / .02)
add('stab', 'quartal stab', finish(lp(st, 4500), .70, 40), 64)
# risers and swells (the riser is stored at 11025 Hz: it holds nothing above 5 kHz)
R2 = 11025; n_ = int(4.0 * R2); t_ = tt(n_, R2); u = t_ / t_[-1]
rs = noise(n_) * (u ** 2.2); rs = sweep_lp(hp(rs, 300, 2, R2), 500, 4800, sr=R2, block=256)
sir = np.sin(2 * np.pi * np.cumsum(180 * 2 ** (u * 3.2)) / R2) * u ** 2.5 * .35
add('riser', 'riser 2 bars', finish(rs + sir, .46, 60, R2), None, R2)
n_ = int(2.0 * SR); t_ = tt(n_); u = t_ / t_[-1]
add('reverse', 'reverse swell', finish(sweep_lp(bp(noise(n_), 1400, 9000, 2), 800, 7500) * u ** 3 + .25 * np.sin(2 * np.pi * np.cumsum(90 + 180 * u) / SR) * u ** 2, .42, 20), None)
n_ = int(1.9 * SR); t_ = tt(n_)
imp = np.sin(2 * np.pi * np.cumsum(46 + 90 * np.exp(-t_ / .09)) / SR) * np.exp(-t_ / .8) + lp(noise(n_), 1800) * np.exp(-t_ / .35) * .6 + hp(noise(n_), 3500) * np.exp(-t_ / .06) * .5
add('impact', 'impact', finish(np.tanh(1.5 * imp), .95, 60), None)
n_ = int(1.9 * SR); t_ = tt(n_)
add('crash', 'crash', finish(hp(metal(n_, .55, 3500), 3000) * 1.0 + .5 * hp(noise(n_), 5000) * np.exp(-t_ / .5), .6, 80), None)
n_ = int(.03 * SR)
add('glitch', 'glitch tick', finish(bp(np.sign(noise(n_)) * np.exp(-tt(n_) / .008), 1500, 7000), .3, 4), None)

KEYS = list(I)
INST = {k: i + 1 for i, k in enumerate(KEYS)}
assert len(KEYS) <= 32

# =============================================================== read the transcription
P0 = {6: 37, 7: 44, 8: 64, 9: 68, 10: 71, 11: 68, 12: 71}       # the pitch each stem-sample sounds at (make_cocaine_cola.py)
src = xm.parse(SRC)
BARS = [o for o in src['order'][:-1]]                            # the last order is the empty tail
NB = len(BARS); assert NB == 220, NB
NSTEP = NB * 16
drum = {k: [set() for _ in range(NB)] for k in 'KkSHT'}           # kick, soft kick, snare, hat, tom : steps in each bar
vel = {k: [{} for _ in range(NB)] for k in 'KkSHT'}
bass_raw = []; lead_raw = {7: [], 8: [], 9: []}
for b, o in enumerate(BARS):
    for r, row in enumerate(src['pats'][o]):
        for c, (n, i, v, e, ep) in enumerate(row):
            if not n: continue
            vv = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
            if c in (0, 1, 2, 3, 5):
                k = {0: 'K', 1: 'k', 2: 'S', 3: 'H', 5: 'T'}[c]; drum[k][b].add(r); vel[k][b][r] = vv
            elif c == 6: bass_raw.append((b * 16 + r, n - 49 + P0[i], vv, v == 0x11))
            elif c in (7, 8, 9): lead_raw[c].append((b * 16 + r, n - 49 + P0[i], vv, v == 0x11))
def timeline(ev, nsteps):
    """-> per step: (pitch, volume) of the note sounding (a cut ends it), None when silent"""
    out = [None] * nsteps; ev = sorted(ev)
    for k, (s, p, v, cut) in enumerate(ev):
        end = ev[k + 1][0] if k + 1 < len(ev) else nsteps
        if not cut:
            for q in range(s, min(end, nsteps)): out[q] = (p, v)
    return out
bass_tl = timeline(bass_raw, NSTEP)
bass_notes = []                                                  # (step, midi, vel, length in steps)
for s, p, v, cut in sorted(bass_raw):
    if cut: continue
    q = s + 1
    while q < NSTEP and bass_tl[q] == (p, v) and not any(x[0] == q for x in bass_raw): q += 1
    bass_notes.append((s, p, v, q - s))
tl = [timeline(lead_raw[c], NSTEP) for c in (7, 8, 9)]
top = []
for s in range(NSTEP):
    c = [t[s] for t in tl if t[s] is not None]
    top.append(max(c, key=lambda a: a[1])[0] if c else None)

# ---- the hook: vote the 8 loud passes (bars 145-208) step by step into one 8-bar loop
SCALE = {1, 3, 4, 6, 8, 9, 10, 11}                              # C# D# E F# G# A A# B
def snap(p, lo=61, hi=86):
    for d in (0, -1, 1, -2, 2):
        if (p + d) % 12 in SCALE: p += d; break
    while p < lo: p += 12
    while p > hi: p -= 12
    return p
def fold(p, lo=64, hi=76):                                       # the lead lives in E4..E5; anything outside is a harmonic the pitch tracker mistook
    while p < lo: p += 12
    while p > hi: p -= 12
    return p
def mode3(seq):
    out = list(seq)
    for i in range(1, len(seq) - 1):
        w = [x for x in seq[i - 1:i + 2] if x is not None]
        if len(w) >= 2:
            c = collections.Counter(w).most_common(1)[0]
            if c[1] >= 2: out[i] = c[0]
    return out
def runs(seq, minlen=2):
    """seq of pitch|None -> [(start, pitch, length)], runs shorter than minlen dropped"""
    out = []; s = 0
    while s < len(seq):
        e = s
        while e < len(seq) and seq[e] == seq[s]: e += 1
        if seq[s] is not None and e - s >= minlen: out.append((s, seq[s], e - s))
        s = e
    return out
LOOP = 128; H0 = 144 * 16
votes = []
for i in range(LOOP):
    c = collections.Counter(fold(top[H0 + k * LOOP + i]) for k in range(8) if top[H0 + k * LOOP + i] is not None)
    votes.append(c.most_common(1)[0][0] if c and c.most_common(1)[0][1] >= 3 else None)
agree = sum(1 for i in range(LOOP) if votes[i] is not None)
hook_seq = [snap(p, 64, 76) if p is not None else None for p in mode3(votes)]
HOOK = runs(hook_seq, 2)                                          # (step in the 8-bar loop, midi, length)
# ---- the quiet "bell call" (bars 49-64 and 113-144) is already clean: take it as it is
GLINT = set()                                                     # the high sparkle (D#6 and up) the transcription heard at the end of the bell-call bars
low_top = [p if (p is not None and p < 84) else None for p in top]
for s0 in range(NSTEP - 1):
    if top[s0] is not None and top[s0] >= 84 and (s0 == 0 or top[s0 - 1] is None or top[s0 - 1] < 84): GLINT.add(s0)
quiet = [snap(fold(p, 61, 76), 61, 76) if p is not None else None for p in mode3(low_top)]
BELL_CALL = {}
for lo, hi in ((48, 64), (112, 144)):
    for s, p, l in runs(quiet[lo * 16:hi * 16], 2): BELL_CALL[lo * 16 + s] = (p, l)

# =============================================================== arrangement
SECS = [(0, 8, 'intro'), (8, 16, 'g1'), (16, 32, 'main'), (32, 40, 'brk1'), (40, 48, 'bld1'), (48, 64, 'bell1'), (64, 72, 'brk2'),
        (72, 92, 'g2'), (92, 96, 'bld2'), (96, 108, 'bassonly'), (108, 112, 'bld3'), (112, 128, 'bell2'), (128, 144, 'busy'),
        (144, 160, 'hook1'), (160, 176, 'hook2'), (176, 192, 'hook3'), (192, 204, 'hookbrk'), (204, 208, 'hook4'), (208, 220, 'outro')]
def sec(b):
    for lo, hi, n in SECS:
        if lo <= b < hi: return n
def sec_start(b): return any(lo == b for lo, hi, n in SECS)
GROOVE = {'g1', 'main', 'bell1', 'g2', 'bell2', 'busy', 'hook1', 'hook2', 'hook3', 'hook4', 'outro'}
bass_by_bar = collections.defaultdict(list)
for s, p, v, l in bass_notes: bass_by_bar[s // 16].append((s % 16, p, v, l))
def chord_of(b):                                                   # 'C' (C# minor side) or 'F' (F# side) from the bass; loop phase elsewhere
    if bass_by_bar.get(b):
        first = min(bass_by_bar[b], key=lambda x: x[0])
        pcs = [p % 12 for st, p, v, l in bass_by_bar[b]]
        return 'F' if (first[1] % 12 == 6 or pcs.count(6) > pcs.count(1)) else 'C'
    if b < 16: return 'C' if (b // 4) % 2 == 0 else 'F'
    if 192 <= b < 204: return 'C' if ((b - 192) % 8) < 4 else 'F'
    return 'C' if ((b - 16) % 8) < 4 else 'F'
CHORD = [chord_of(b) for b in range(NB)]

grid = [[[(0, 0, 0)] * NCH for _ in range(ROWS)] for _ in range(NB)]
def put(b, step, ch, key, midi=None, v=1.0, sub=0):
    """v 0..1 of full volume; sub = extra rows (0..5) after the 16th"""
    r = step * 6 + sub
    while r >= ROWS: b += 1; r -= ROWS
    if b >= NB: return
    g = I[key]['gen']; n = 49 + (int(midi) - g) if g is not None else 49
    if not 1 <= n <= 96: return
    grid[b][r][ch] = (n, INST[key], 0x10 + int(max(1, min(64, round(64 * v)))))
def putr(b, row, ch, key, midi=None, v=1.0):
    g = I[key]['gen']; n = 49 + (int(midi) - g) if g is not None else 49
    if row < ROWS and 1 <= n <= 96: grid[b][row][ch] = (n, INST[key], 0x10 + int(max(1, min(64, round(64 * v)))))
def free_r(b, row, ch): return grid[b][row][ch][0] == 0
def hv(b, k, r, lo=.55, hi=1.0): return lo + (hi - lo) * vel[k][b].get(r, .8)

def drums(b):
    s = sec(b); K = drum['K'][b]; kk = drum['k'][b]; S = drum['S'][b]; H = drum['H'][b]; T = drum['T'][b]
    groove = s in GROOVE or (s in ('bld1', 'bld2', 'bld3') and len(K) >= 2)
    for r in sorted(K): put(b, r, 0, 'kick', None, hv(b, 'K', r, .72, 1.0))
    for r in sorted(kk): put(b, r, 0, 'kick_g', None, hv(b, 'k', r, .55, .95))
    for r in sorted(S):
        v = hv(b, 'S', r, .6, 1.0); put(b, r, 3, 'snare', None, v)
        if groove and r in (4, 12): put(b, r, 4, 'clap', None, .7 * v)                       # clap on the backbeat
        elif groove: put(b, r, 4, 'rim', None, .5 * v)
    th = sorted(T)
    for j, r in enumerate(th): put(b, r, 7, 'tom', 55 - (j % 3) * 3, hv(b, 'T', r, .6, 1.0))
    for r in sorted(H):
        put(b, r, 5, 'hat_c', None, hv(b, 'H', r, .5, .95))
        if groove and r % 4 == 2 and b % 2 == 1 and r == max(H): put(b, r, 5, 'hat_o', None, .6)         # an open hat now and then
    if groove or len(H) >= 4:                                                                  # ghost hats + shaker around the original hits
        for r in range(16):
            if r not in H and r % 2 == 1 and r not in K and ((r + b) % 4 != 0): put(b, r, 5, 'hat_c', None, .2 + .08 * ((r // 2) % 2))
            if r % 2 == 0 or (r % 4 == 3): put(b, r, 6, 'shaker', None, .42 if r % 4 == 2 else .22)
    if len(S) >= 5 or (b % 4 == 3 and groove and len(S) >= 4):                                 # a 32nd-note roll on the last beat
        for j in range(7): putr(b, 75 + 3 * j, 4, 'rim', None, .28 + .1 * j)
        for j in range(4):
            if free_r(b, 72 + 6 * j, 5): putr(b, 72 + 6 * j, 5, "hat_c", None, .3 + .13 * j)
    if sec_start(b) and s in GROOVE and b > 0: put(b, 0, 9, 'crash', None, .8)
def bass_part(b):
    s = sec(b); notes = sorted(bass_by_bar.get(b, []))
    K = drum['K'][b]
    for st, p, v, l in notes:
        m = p
        while m > 49: m -= 12
        while m < 24: m += 12
        vol = (.62 + .38 * v) * (.82 if st in K else 1.0)
        put(b, st, 1, 'sub_s' if l <= 1 else 'sub_l', m, vol)
        if p > 49: put(b, st, 2, 'growl', p, .5)                                           # the transcription's octave-up accents
        elif s in ('main', 'bell1', 'bell2', 'busy', 'hook1', 'hook2', 'hook3', 'hook4', 'outro', 'g2') and (l >= 2 or st in K):
            put(b, st, 2, 'growl', p + 12, .55 if s in ('main', 'g2', 'outro') else .68)
PADV = {'intro': .42, 'g1': .38, 'main': .30, 'brk1': .62, 'bld1': .48, 'bell1': .34, 'brk2': .62, 'g2': .28, 'bld2': .48, 'bassonly': .62,
        'bld3': .5, 'bell2': .34, 'busy': .26, 'hook1': .24, 'hook2': .26, 'hook3': .28, 'hookbrk': .6, 'hook4': .26, 'outro': .4}
PAD = {'C': (56, 64), 'F': (61, 66)}                           # G#3 E4 | C#4 F#4 : open fifths / fourths, no thirds
STABN = {'C': (59, 64), 'F': (61, 66)}                           # B3 E4 | C#4 F#4
def harmony(b):
    s = sec(b); c = CHORD[b]
    if b >= 2 or s != 'intro':
        a, d = PAD[c]; put(b, 0, 10, 'pad', a, PADV[s]); put(b, 0, 11, 'pad', d, PADV[s] * .85)
    if s in ('main', 'g2', 'bell1', 'bell2', 'busy', 'hook1', 'hook2', 'hook3', 'hook4', 'outro') and (b >= 24 or s != 'main'):
        pat = (3, 9, 14) if b % 4 != 3 else (3, 6, 9, 12, 14)
        for j, st in enumerate(pat):
            vv = (.62 if j == 0 else .48) * (1.0 if s != 'busy' else .8)
            put(b, st, 12, 'stab', STABN[c][0], vv); put(b, st, 13, 'stab', STABN[c][1], vv * .9)
def melody(b):
    s = sec(b); step0 = b * 16
    if s in ('bell1', 'bell2', 'busy'):
        for st in range(16):
            q = step0 + st
            if q in BELL_CALL:
                p, l = BELL_CALL[q]; key = 'lead_l' if l >= 6 else ('lead_m' if l >= 3 else 'lead_s')
                put(b, st, 8, key, p, .42)
                if l >= 6: put(b, st, 14, 'choir', p - 12 if p > 70 else p, .38)
        gl = sorted(q - step0 for q in GLINT if step0 <= q < step0 + 16)
        for j, st in enumerate(gl[:2]): put(b, st, 9, 'bell', 87 + 3 * j, .5 - .08 * j)    # D#6 (and an F#6 answer) where the original glints
        if not gl and b % 2 == 1: put(b, 13, 9, 'bell', 87, .4)
    if s.startswith('hook') or s == 'hookbrk':
        loop = (b - 144) % 8; q0 = loop * 16
        full = s in ('hook2', 'hook3', 'hookbrk', 'hook4')
        for st, p, l in HOOK:
            if q0 <= st < q0 + 16:
                key = 'lead_l' if l >= 6 else ('lead_m' if l >= 3 else 'lead_s')
                put(b, st - q0, 8, key, p, .95 if full else .8)
                if s in ('hook2', 'hook3', 'hookbrk', 'hook4') and st - q0 + 3 < 16:                 # dotted-8th cowbell answer, folded into one octave
                    pc = p
                    while pc < 74: pc += 12
                    while pc > 86: pc -= 12
                    put(b, st - q0 + 3, 7, 'cowbell', pc, .34 if s != 'hook3' else .44)
                if l >= 4 and s in ('hook2', 'hook3', 'hookbrk', 'hook4'): put(b, st - q0, 14, 'choir', p - 12 if p > 74 else p, .5 if s == 'hookbrk' else .4)
        if s in ('hook1',) and loop in (3, 7): put(b, 13, 9, 'bell', 87, .45)
def fx(b):
    s = sec(b)
    drops = {48, 72, 112, 128, 144, 160, 176, 204}
    if (b + 2) in drops: put(b, 0, 15, 'riser', None, .8)                                  # 2-bar riser into a drop
    if (b + 1) in drops: put(b, 0, 9, 'reverse', None, .75)                                # and a reverse swell in the bar before it
    elif (b + 1) in (16, 32, 40): put(b, 0, 15, 'reverse', None, .75)                      # one-bar swell into a new part
    if b in drops and b not in (128,): put(b, 0, 15, 'impact', None, .95)
    if b == 192: put(b, 0, 15, 'impact', None, .8)
    if b in (40, 92): put(b, 0, 15, 'impact', None, .6)
    if b == 219:                                                                             # last bar: a falling hit, then air
        put(b, 0, 15, 'impact', None, .7)
    if s in ('brk1', 'brk2') and b % 8 == 7:
        for j in range(4): put(b, 12 + j, 15, 'glitch', None, .5 - .08 * j)
def build():
    for b in range(NB):
        drums(b); bass_part(b); harmony(b); melody(b); fx(b)
build()

# =============================================================== XM writer (same layout as tools/make_flexicode_rework.py)
def pat_bytes(P):
    b = bytearray()
    for row in P:
        for (n, i, v) in row: b += bytes([n, i, v, 0, 0])
    return struct.pack('<IBHH', 9, 0, len(P), len(b)) + bytes(b)
def inst_bytes(k):
    d = I[k]; q = np.clip(np.round(d['x'] * 127), -127, 127).astype(int)
    delta = np.diff(np.concatenate([[0], q])); dl = bytes((int(v) & 255) for v in delta)
    rate = d['sr']; relf = 12 * np.log2(rate / 8363.0); rel = int(round(relf)); fine = int(round((relf - rel) * 128))
    h = struct.pack('<I', 263) + d['name'].encode('latin1')[:22].ljust(22, b'\0') + b'\0' + struct.pack('<HI', 1, 40) + bytes(96) + bytes(48) + bytes(48)
    h += bytes(14) + struct.pack('<H', 0) + bytes(263 - len(h) - 14 - 2)
    sh = struct.pack('<IIIBbBBbB', len(q), 0, 0, 64, fine, 0, 128, rel, 0) + b'\0' * 22
    assert len(h) == 263 and len(sh) == 40
    return h + sh + dl
def main():
    pats, order, seen = [], [], {}
    for b in range(NB):
        P = grid[b]; key = pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    tail = [[(0, 0, 0)] * NCH for _ in range(ROWS)]; pats.append(tail); order.append(len(pats) - 1)
    assert len(pats) <= 256 and len(order) <= 256, (len(pats), len(order))
    hdr = b'Extended Module: ' + b'COCAINE COLA II'.ljust(20) + b'\x1a' + b'make_cocaine_cola_rework'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(pat_bytes(P) for P in pats) + b''.join(inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    secs = len(order) * ROWS * SPEED * 2.5 / BPM
    print("hook: %d of %d steps voted; %d hook notes; bell-call notes %d; bass notes %d" % (agree, LOOP, len(HOOK), len(BELL_CALL), len(bass_notes)))
    print("wrote %s: %d orders, %d patterns, %.1f s (%d:%02d), %d KB, %d instruments" % (OUT, len(order), len(pats), secs, secs // 60, secs % 60, os.path.getsize(OUT) // 1024, len(KEYS)))
if __name__ == '__main__': main()
