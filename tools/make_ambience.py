#!/usr/bin/env python3
"""THE OUTDOOR AMBIENCE BEDS: day (songbirds in the trees, a breeze), night (crickets, frogs, an owl now and then), rain and wind.
Everything is synthesized here, nothing is sampled, so the loops are ours and can be tuned. Each bed is a seamless stereo loop at the
game mixer's own rate (18157 Hz), 8-bit signed, interleaved L R, after a u32 frame count: source/sfx/amb_<name>.raw. The game plays one
bed at a time under the music (main.c: ambMix, ambTick).
usage: python3 tools/make_ambience.py [outdir] [--wav]   (--wav also writes a 16-bit preview of each bed next to it)"""
import sys, os, struct
import numpy as np
from scipy.signal import butter, sosfilt, lfilter

SR = 18157                     # the mixer rate (MUS_N 304 samples a frame, main.c)
rng = np.random.default_rng(20261009)

def lp(x, f, o=2): return sosfilt(butter(o, f/(SR/2), 'low', output='sos'), x)
def hp(x, f, o=2): return sosfilt(butter(o, f/(SR/2), 'high', output='sos'), x)
def bp(x, f0, f1, o=2): return sosfilt(butter(o, [f0/(SR/2), f1/(SR/2)], 'band', output='sos'), x)
def pink(n):   # Voss-ish pink noise by filtering white (Paul Kellet's coefficients)
    w = rng.standard_normal(n)
    b = [0.049922035, -0.095993537, 0.050612699, -0.004408786]; a = [1, -2.494956002, 2.017265875, -0.522189400]
    return lfilter(b, a, w)
def brown(n): x = np.cumsum(rng.standard_normal(n)); return hp(x, 20, 1)
def smooth_rand(n, rate_hz, lo, hi):   # a slow random curve between lo and hi (gusts, swells), eased between its points
    k = max(4, int(n / SR * rate_hz) + 4); pts = rng.uniform(lo, hi, k)
    xs = np.linspace(0, n, k); i = np.arange(n); j = np.minimum((i*(k-1)//n), k-2); u = (i - xs[j])/(xs[j+1]-xs[j]); u = u*u*(3-2*u)
    return pts[j]*(1-u) + pts[j+1]*u

class Bed:   # a stereo bed being built: one-shot events (mixed at a time, with a pan and a distance: quieter, duller, more room) and continuous layers
    def __init__(s, secs):
        s.n = int(secs*SR); s.F = int(1.0*SR); m = s.n + 6*SR
        s.L = np.zeros(m); s.R = np.zeros(m); s.cL = np.zeros(s.n + s.F); s.cR = np.zeros(s.n + s.F)
    def add(s, x, t, pan=0.0, dist=1.0):
        if dist > 1.0:
            x = lp(x, max(1500, 9000/dist)) / dist
            tail = int(0.35*SR); ir = rng.standard_normal(tail) * np.exp(-np.arange(tail)/(0.09*SR)) * 0.035 * min(dist, 4)
            x = np.convolve(x, np.concatenate([[1.0], ir]))[:len(x)+tail]
        i = int(t*SR) % s.n; n = min(len(x), len(s.L) - i)
        gl = np.sqrt(0.5*(1-pan)); gr = np.sqrt(0.5*(1+pan))
        s.L[i:i+n] += x[:n]*gl; s.R[i:i+n] += x[:n]*gr
    def bed(s, x, wide=True):   # a continuous layer, n + 1 s long (left and right decorrelated when wide)
        m = s.n + s.F; y = x(m); s.cL += y; s.cR += x(m) if wide else y
    def loop(s):   # seamless: events that ring past the end wrap onto the start; continuous layers cross over (equal power) in the first second
        out = []; F = s.F; ri = np.sqrt(np.linspace(0, 1, F)); ro = np.sqrt(np.linspace(1, 0, F))
        for ev, c in ((s.L, s.cL), (s.R, s.cR)):
            y = ev[:s.n].copy(); k = s.n
            while k < len(ev): m = min(s.n, len(ev)-k); y[:m] += ev[k:k+m]; k += m
            cc = c[:s.n].copy(); cc[:F] = c[:F]*ri + c[s.n:s.n+F]*ro   # (sample n-1 runs on into c[n], which is where the crossfade starts)
            out.append(y + cc)
        return out

def tone(f_curve, amp_env, harm=((1, 1.0),), vib=(0, 0)):   # a pitched sound from a frequency curve (Hz per sample) and an amplitude envelope
    n = len(f_curve); vr, vd = vib
    if vr: f_curve = f_curve * (1 + vd*np.sin(2*np.pi*vr*np.arange(n)/SR))
    ph = 2*np.pi*np.cumsum(f_curve)/SR; y = np.zeros(n)
    for k, g in harm: y += g*np.sin(k*ph)
    return y*amp_env

def env(n, att=0.006, rel=0.02, shape=1.0):
    a = int(att*SR); r = int(rel*SR); e = np.ones(n)
    if a: e[:a] = np.linspace(0, 1, a)**shape
    if r: e[-r:] *= np.linspace(1, 0, r)**shape
    return e

def sweep(f0, f1, secs, curve=1.0):
    n = max(2, int(secs*SR)); t = np.linspace(0, 1, n)**curve; return f0 + (f1-f0)*t

# ---- birds ----
def syl(f0, f1, secs, curve=1.0, harm2=0.12, vib=(0, 0)):
    f = sweep(f0, f1, secs, curve); n = len(f)
    return tone(f, env(n, 0.005, min(0.03, secs*0.4)) * np.hanning(n)**0.25, ((1, 1.0), (2, harm2)), vib)
def gap(secs): return np.zeros(int(secs*SR))
def phrase(parts): return np.concatenate(parts)

def robin():   # caroling: 3-6 rising and falling whistled syllables
    parts = []
    for _ in range(rng.integers(3, 7)):
        a = rng.uniform(2100, 3300); b = a*rng.uniform(0.75, 1.35); d = rng.uniform(0.09, 0.22)
        parts += [syl(a, b, d, rng.choice([0.6, 1.0, 1.6]), 0.15, (rng.uniform(25, 60), 0.02)), gap(rng.uniform(0.06, 0.14))]
    return phrase(parts)
def chickadee():   # fee-bee: two clear whistles, the second lower
    hi = rng.uniform(3800, 4100); return phrase([syl(hi, hi*0.99, 0.32, 1, 0.05), gap(0.07), syl(hi*0.86, hi*0.83, 0.34, 1, 0.05)])
def sparrow():   # two notes, then a falling trill
    p = [syl(4300, 4500, 0.12), gap(0.08), syl(3500, 3600, 0.15), gap(0.07)]
    rate = rng.uniform(12, 18)
    for _ in range(rng.integers(8, 16)): p += [syl(rng.uniform(5600, 6200), 3900, 0.045, 0.7, 0.2), gap(1/rate-0.045)]
    return phrase(p)
def warbler():   # thin, buzzy, high, quick
    p = []
    for _ in range(rng.integers(5, 9)):
        f = rng.uniform(6200, 7400); p += [syl(f, f*rng.uniform(0.9, 1.05), 0.05, 1, 0.05, (110, 0.05)), gap(0.035)]
    p += [syl(7600, 5200, 0.12, 0.8)]
    return phrase(p)
def finch():   # a bright warble: fast varied notes
    p = []
    for _ in range(rng.integers(10, 18)):
        a = rng.uniform(2800, 5200); p += [syl(a, a*rng.uniform(0.7, 1.4), rng.uniform(0.03, 0.07), rng.choice([0.5, 1, 2]), 0.25), gap(rng.uniform(0.01, 0.04))]
    return phrase(p)
def crow():   # a distant caw: a harsh, nasal, noisy harmonic sound
    n = int(0.32*SR); f = sweep(560, 470, 0.32, 0.8)
    y = tone(f, env(n, 0.03, 0.12), ((1, 0.6), (2, 1.0), (3, 0.8), (4, 0.6), (5, 0.4), (6, 0.25)))
    y = bp(y + 0.6*rng.standard_normal(n)*np.abs(y), 700, 3000)
    return y/np.max(np.abs(y))
def dove():   # mourning dove: a soft, low, hollow coo-OO-oo-oo
    p = [syl(540, 620, 0.25, 1, 0.02), gap(0.05), syl(640, 560, 0.55, 1, 0.02), gap(0.18), syl(520, 500, 0.4, 1, 0.02), gap(0.12), syl(510, 490, 0.4, 1, 0.02)]
    y = phrase(p); return lp(y, 1600)

def day(secs=16.0):
    b = Bed(secs)
    b.bed(lambda n: lp(pink(n), 900) * smooth_rand(n, 0.4, 0.15, 0.55) * 0.05)                   # the breeze
    b.bed(lambda n: hp(rng.standard_normal(n), 3500) * smooth_rand(n, 0.7, 0.0, 1.0)**3 * 0.012)  # leaves stirring in the gusts
    singers = [(robin, -0.6, 1.8, 2.6), (chickadee, 0.55, 2.4, 4.5), (sparrow, 0.2, 3.0, 5.5), (warbler, -0.3, 4.0, 6.0), (finch, 0.8, 2.6, 4.0), (dove, -0.85, 3.2, 7.0)]
    for fn, pan, dist, every in singers:
        t = rng.uniform(0, every)
        while t < secs:
            b.add(fn()*0.32, t, pan + rng.uniform(-0.1, 0.1), dist * rng.uniform(0.9, 1.3))
            t += every * rng.uniform(0.7, 1.4)
    for t in rng.uniform(2, secs-2, 1): b.add(phrase([crow(), gap(0.16), crow()])*0.12, t, 0.9, 6.0)   # far off, once a loop
    return b.loop()

# ---- night ----
def cricket_chirp(f, pulses=3, pul=0.018, gp=0.012):
    p = []
    for _ in range(pulses):
        n = int(pul*SR); p += [tone(np.full(n, f), env(n, 0.002, 0.004), ((1, 1.0), (2, 0.18), (3, 0.06))), gap(gp)]
    return phrase(p)
def frog():   # a spring peeper: a rising peep
    return syl(2550, 2950, 0.14, 0.7, 0.08)
def bullfrog():   # a deep, distant jug-o-rum
    n = int(0.55*SR); f = sweep(140, 115, 0.55)
    y = tone(f, env(n, 0.05, 0.2), ((1, 1.0), (2, 0.7), (3, 0.45), (4, 0.3), (5, 0.18)))
    am = 0.6 + 0.4*np.sin(2*np.pi*28*np.arange(n)/SR); return lp(y*am, 900)
def owl():   # hoo, hoo-hoo
    def hoo(d): n = int(d*SR); return tone(np.full(n, 360.0)*np.linspace(1.03, 0.97, n), env(n, 0.08, 0.15, 1.5), ((1, 1.0), (2, 0.08)))
    return lp(phrase([hoo(0.5), gap(0.35), hoo(0.22), gap(0.12), hoo(0.5)]), 1200)

def night(secs=16.0):
    b = Bed(secs)
    b.bed(lambda n: lp(brown(n), 260) * smooth_rand(n, 0.25, 0.3, 0.7) * 0.0025)                  # a still night: hardly any wind
    b.bed(lambda n: tone(np.full(n, 2950.0), (0.5+0.5*np.sin(2*np.pi*31*np.arange(n)/SR))**2, ((1, 1.0),)) * 0.012)   # a tree cricket's endless trill, far back
    crickets = [(4700, 3, 0.43, -0.5, 1.6), (4450, 4, 0.61, 0.6, 2.2), (5050, 3, 0.37, 0.1, 3.2), (4200, 2, 0.52, -0.85, 3.8), (4900, 3, 0.47, 0.9, 4.5)]
    for f, pulses, period, pan, dist in crickets:
        t = rng.uniform(0, period); per = period
        while t < secs + 1:
            b.add(cricket_chirp(f*rng.uniform(0.995, 1.005), pulses)*0.18, t, pan, dist)
            per = period*rng.uniform(0.97, 1.03); t += per
            if rng.random() < 0.04: t += rng.uniform(0.6, 1.8)   # (now and then one stops for a moment)
    t = 0.5
    while t < secs:   # a chorus of peepers far off
        b.add(frog()*0.10, t, rng.uniform(-1, 1), rng.uniform(4, 7)); t += rng.uniform(0.25, 0.9)
    for t in (3.0, 10.5): b.add(phrase([bullfrog(), gap(0.4), bullfrog()])*0.22, t, -0.4, 4.0)
    b.add(owl()*0.30, 7.2, 0.7, 3.5)
    return b.loop()

# ---- rain ----
def rain(secs=10.0):
    b = Bed(secs)
    b.bed(lambda n: bp(pink(n), 400, 6500) * smooth_rand(n, 0.3, 0.75, 1.0) * 0.06)       # the hiss of it falling everywhere
    b.bed(lambda n: lp(brown(n), 180) * 0.012)                                               # a low roar
    drops = int(secs*260)
    for t in rng.uniform(0, secs, drops):   # drops landing near: a tiny damped ping each
        f = rng.uniform(1200, 5200); d = rng.uniform(0.004, 0.012); n = int(d*SR*4)
        y = np.sin(2*np.pi*f*np.arange(n)/SR) * np.exp(-np.arange(n)/(d*SR)) * rng.uniform(0.02, 0.11)
        b.add(y, t, rng.uniform(-1, 1), rng.uniform(1, 3))
    for t in rng.uniform(0, secs, int(secs*3)):   # heavier drips off the roof
        n = int(0.05*SR); f = rng.uniform(600, 1100)
        y = np.sin(2*np.pi*np.linspace(f*1.4, f, n)*np.arange(n)/SR) * np.exp(-np.arange(n)/(0.012*SR)) * 0.18
        b.add(y, t, rng.uniform(-0.8, 0.8), 1.2)
    return b.loop()

# ---- wind ----
def wind(secs=12.0):
    b = Bed(secs)
    g = smooth_rand(int(secs*SR) + 2*SR, 0.35, 0.15, 1.0)
    def body(n): return lp(brown(n), 500) * g[:n]**1.5 * 0.05
    def whistle(n):   # the wind finding a gap: a band that rises with the gust
        x = rng.standard_normal(n); y = np.zeros(n); seg = SR//20
        for i in range(0, n, seg):
            f = 380 + 520*g[min(i, len(g)-1)]; y[i:i+seg] = bp(x[i:i+seg+256], f*0.93, f*1.07, 2)[:len(y[i:i+seg])]
        return y * g[:n]**2 * 0.05
    b.bed(body); b.bed(whistle); b.bed(lambda n: hp(rng.standard_normal(n), 2500) * g[:n]**3 * 0.02)   # leaves and grit in the gusts
    return b.loop()

def write(name, LR, outdir, wav):
    L, R = LR; peak = max(np.max(np.abs(L)), np.max(np.abs(R)))
    L = L/peak*0.92; R = R/peak*0.92                                   # full scale: the game sets the level
    tpdf = lambda n: (rng.random(n) - rng.random(n))                   # (TPDF dither: 8 bits hiss less than they buzz)
    l8 = np.clip(np.round(L*127 + tpdf(len(L))), -128, 127).astype(np.int8)
    r8 = np.clip(np.round(R*127 + tpdf(len(R))), -128, 127).astype(np.int8)
    st = np.empty(2*len(l8), np.int8); st[0::2] = l8; st[1::2] = r8
    with open(os.path.join(outdir, 'amb_%s.raw' % name), 'wb') as f: f.write(struct.pack('<I', len(l8))); f.write(st.tobytes())
    if wav:
        import wave
        with wave.open(os.path.join(outdir, 'amb_%s.wav' % name), 'wb') as w:
            w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
            s16 = np.empty(2*len(L), np.int16); s16[0::2] = (l8.astype(np.int16)*256); s16[1::2] = (r8.astype(np.int16)*256)
            w.writeframes(np.tile(s16, 2).tobytes())   # (twice round, to hear the seam)
    print('amb_%s: %.1f s, %d KB' % (name, len(l8)/SR, (2*len(l8)+4)//1024))

if __name__ == '__main__':
    outdir = next((a for a in sys.argv[1:] if not a.startswith('--')), 'source/sfx'); wav = '--wav' in sys.argv
    os.makedirs(outdir, exist_ok=True)
    for name, fn in (('day', day), ('night', night), ('rain', rain), ('wind', wind)): write(name, fn(), outdir, wav)
