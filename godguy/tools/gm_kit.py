# Builds source/gmkit.h: the single-cycle waves (64 samples, 8-bit signed) the music voices loop, and the drum kit (8-bit one-shots) -- all synthesised here, no recordings.
import math, random, os
random.seed(7)
HERE = os.path.dirname(__file__)
N = 64
def add(h, n=N, phase=None):
    out = []
    for i in range(n):
        t = 2 * math.pi * i / n; v = 0.0
        for k, a in h:
            if k * 2 < n: v += a * math.sin(k * t + (phase or {}).get(k, 0))
        out.append(v)
    return out
def norm(w, peak=0.94):
    m = max(abs(x) for x in w) or 1; return [int(round(max(-127, min(127, x / m * 127 * peak)))) for x in w]
def saw(n=N, p=1.0, top=30): return add([(k, (1.0 / k ** p) * (1 if k % 2 else 1)) for k in range(1, top)], n)
WAVES = []
names = []
def wave(nm, w): names.append(nm); WAVES.append(norm(w))
wave('SAW', saw())
wave('PULSE', add([(k, math.sin(math.pi * k * 0.25) / k) for k in range(1, 31)]))
wave('TRIANGLE', add([(k, (1.0 / k ** 2) * (1 if (k // 2) % 2 == 0 else -1)) for k in range(1, 31, 2)]))
wave('STRINGS', saw(p=1.35, top=26))
wave('ORGAN', add([(1, 1.0), (2, 0.8), (3, 0.55), (4, 0.5), (6, 0.35), (8, 0.25)]))
wave('BELL', [math.sin(2 * math.pi * i / N + 2.4 * math.sin(2 * math.pi * 3.0 * i / N)) for i in range(N)])
wave('PLUCK', saw(p=1.7, top=20))
wave('PAD', add([(1, 1.0), (2, 0.5), (3, 0.32), (4, 0.16), (5, 0.1), (7, 0.05)]))
wave('SUB', add([(1, 1.0), (2, 0.18)]))
wave('REED', add([(k, math.sin(math.pi * k * 0.37) / k) for k in range(1, 31)]))
wave('FLUTE', add([(1, 1.0), (2, 0.25), (3, 0.1)]))
wave('BRASS', add([(1, 1), (2, .9), (3, .8), (4, .62), (5, .46), (6, .3), (7, .2), (8, .12), (9, .07)]))
wave('CHOIR', add([(1, 1.0), (2, 0.35), (3, 0.6), (4, 0.2), (5, 0.38), (6, 0.1)], phase={3: 1.3, 5: 2.1}))
wave('GLASS', [math.sin(2 * math.pi * i / N + 1.4 * math.sin(2 * math.pi * 2.0 * i / N)) for i in range(N)])
wave('SYNTH', add([(k, 1.0 / k ** 0.9) for k in range(1, 28)], phase={k: (k * 0.7) for k in range(1, 28)}))
wave('WOBBLE', add([(1, 1.0), (2, .7), (3, .45), (4, .3), (5, .5), (6, .2), (7, .35)]))

def drum(nm, rate, secs, fn):
    n = int(rate * secs); w = [fn(i / rate, i, n) for i in range(n)]; m = max(abs(x) for x in w) or 1
    return nm, rate, [int(round(max(-127, min(127, x / m * 120)))) for x in w]
def nz(): return random.uniform(-1, 1)
class HP:      # one-pole high pass on noise
    def __init__(s, a): s.a = a; s.y = 0; s.x = 0
    def __call__(s, x): s.y = s.a * (s.y + x - s.x); s.x = x; return s.y
class LP:
    def __init__(s, a): s.a = a; s.y = 0
    def __call__(s, x): s.y += s.a * (x - s.y); return s.y
def mk_kick():
    ph = [0.0]
    def f(t, i, n):
        fr = 45 + 140 * math.exp(-t * 34); ph[0] += 2 * math.pi * fr / 16000; return (math.sin(ph[0]) * math.exp(-t * 9) + 0.25 * nz() * math.exp(-t * 90))
    return drum('KICK', 16000, 0.30, f)
def mk_snare():
    hp = HP(0.62); ph = [0.0]
    def f(t, i, n):
        ph[0] += 2 * math.pi * (190 - 60 * t) / 22050; return 0.55 * math.sin(ph[0]) * math.exp(-t * 28) + 0.9 * hp(nz()) * math.exp(-t * 17)
    return drum('SNARE', 22050, 0.22, f)
def mk_clap():
    hp = HP(0.7)
    def f(t, i, n):
        e = sum(math.exp(-((t - d) * 160)) if t >= d else 0 for d in (0, 0.011, 0.024)) * 0.5 + math.exp(-t * 20) * 0.5; return hp(nz()) * e
    return drum('CLAP', 22050, 0.22, f)
def mk_hat(nm, secs, dec):
    hp = HP(0.86)
    return drum(nm, 22050, secs, lambda t, i, n: hp(nz()) * math.exp(-t * dec))
def mk_tom(nm, f0):
    ph = [0.0]
    def f(t, i, n):
        ph[0] += 2 * math.pi * (f0 * (1 + 0.5 * math.exp(-t * 25))) / 16000; return math.sin(ph[0]) * math.exp(-t * 11)
    return drum(nm, 16000, 0.32, f)
def mk_rim():
    return drum('RIM', 22050, 0.05, lambda t, i, n: (math.sin(2 * math.pi * 1700 * t) + 0.5 * nz()) * math.exp(-t * 120))
def mk_crash():
    hp = HP(0.8)
    return drum('CRASH', 22050, 0.95, lambda t, i, n: hp(nz()) * (math.exp(-t * 4.2) * 0.85 + 0.15 * math.exp(-t * 14)))
def mk_ride():
    hp = HP(0.75)
    def f(t, i, n):
        return hp(nz()) * 0.35 * math.exp(-t * 7) + 0.6 * math.sin(2 * math.pi * 3100 * t) * math.exp(-t * 11) + 0.4 * math.sin(2 * math.pi * 4930 * t) * math.exp(-t * 9)
    return drum('RIDE', 22050, 0.55, f)
def mk_shaker():
    hp = HP(0.8); lp = LP(0.7)
    return drum('SHAKER', 22050, 0.09, lambda t, i, n: lp(hp(nz())) * (math.sin(math.pi * min(1, t / 0.09)) ** 1.5))
def mk_riser():      # a reversed-cymbal swell with the room opening up: noise through a band-pass (a biquad, stable at any frequency) whose centre climbs from 500 Hz to 5 kHz while it grows, and a low swell underneath
    rate = 16000; secs = 1.3; st = [0.0, 0.0, 0.0, 0.0]; co = [0, 0, 0, 0, 0]; ph = [0.0]
    def f(t, i, n):
        p = t / secs
        if i % 32 == 0:
            fc = 500 * (10 ** p); w0 = 2 * math.pi * min(fc, 7000) / rate; al = math.sin(w0) / (2 * 1.1); a0 = 1 + al
            co[:] = [al / a0, 0.0, -al / a0, -2 * math.cos(w0) / a0, (1 - al) / a0]
        x = nz(); y = co[0] * x + co[2] * st[1] - co[3] * st[2] - co[4] * st[3]; st[1] = st[0]; st[0] = x; st[3] = st[2]; st[2] = y
        ph[0] += 2 * math.pi * (50 + 60 * p) / rate
        env = p ** 2.2 * (1 - 0.35 * max(0, (p - 0.93) / 0.07))
        return (y * 3.0 + 0.35 * math.sin(ph[0]) * p) * env
    return drum('RISER', rate, secs, f)
def mk_impact():      # the downbeat after it: a sub boom dropping from 95 to 38 Hz, a dark thump of noise, and a long low-passed tail that falls away
    rate = 11025; secs = 1.0; lp1 = LP(0.30); lp2 = LP(0.12); ph = [0.0]
    def f(t, i, n):
        ph[0] += 2 * math.pi * (38 + 57 * math.exp(-t * 7)) / rate; boom = math.sin(ph[0]) * math.exp(-t * 3.2)
        thump = lp1(nz()) * math.exp(-t * 26) * 0.9; a = 0.30 - 0.22 * min(1, t / 0.9); lp2.a = a; tail = lp2(nz()) * math.exp(-t * 3.6) * 1.1
        return boom * 1.2 + thump + tail
    return drum('IMPACT', rate, secs, f)
KITA = [mk_kick(), mk_snare(), mk_clap(), mk_tom('TOMLO', 85), mk_tom('TOMMID', 120), mk_tom('TOMHI', 170), mk_rim()]
KITB = [mk_hat('HAT', 0.05, 90), mk_hat('OPENHAT', 0.30, 11), mk_crash(), mk_ride(), mk_shaker(), mk_hat('HATSOFT', 0.035, 140), mk_riser(), mk_impact()]

def cl(w): return ','.join(str(x) for x in w)
if __name__ == '__main__':
    L = ['#ifndef GMKIT_H', '#define GMKIT_H', '// gmkit.h - GENERATED by godguy/tools/gm_kit.py: single-cycle waves (64 samples, signed 8 bit) and the drum kit (signed 8 bit one-shots). All synthesised.', '#define GM_WAVES %d' % len(WAVES)]
    for i, w in enumerate(WAVES): L.append('static const signed char gmWave%d[64] __attribute__((aligned(4)))={%s};' % (i, cl(w)))
    L.append('static const signed char*const gmWaveT[GM_WAVES]={%s};' % ','.join('gmWave%d' % i for i in range(len(WAVES))))
    L.append('static const char*const gmWaveNm[GM_WAVES]={%s};' % ','.join('"%s"' % n for n in names))
    tot = 0
    for tag, kit in (('A', KITA), ('B', KITB)):
        for i, (nm, rate, w) in enumerate(kit):
            L.append('static const signed char gmDr%s%d[%d] __attribute__((aligned(4)))={%s};' % (tag, i, len(w), cl(w))); tot += len(w)
        L.append('#ifdef GG_HQVOX\ntypedef struct { const signed char*d; unsigned int n; unsigned short rate; } GmDrum;      // (the 3DS\'s long chops: more than 65535 samples)\n#else\ntypedef struct { const signed char*d; unsigned short n, rate; } GmDrum;\n#endif') if tag == 'A' else None
        L.append('static const GmDrum gmKit%s[%d]={%s};' % (tag, len(kit), ','.join('{gmDr%s%d,%d,%d}' % (tag, i, len(w), rate) for i, (nm, rate, w) in enumerate(kit))))
    L.append('#endif')
    open(os.path.join(HERE, '..', 'source', 'gmkit.h'), 'w').write('\n'.join(L) + '\n')
    print('kit: %d waves, drum bytes %d' % (len(WAVES), tot))
