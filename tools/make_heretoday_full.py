#!/usr/bin/env python3
"""HERE TODAY (FULL VERSION): the longer, fuller, lusher rearrangement of Missy's song, unlocked when the story is finished.
Same MIDI as the cutscene version (tools/here_today.mid) but re-arranged in time and re-orchestrated:
  intro (melody on solo cello over harp) / verse 1 / verse 2 / bridge / chorus / instrumental interlude / chorus again a tone higher / verse 3 /
  the first ending / THE BRIDGE REPRISE (instrumental: solo violin and flute over strings, harp and a heartbeat) / the ending again, fuller than before.
Layers: sung voice (female, octave up) + a chord-tone harmony voice, nylon guitar, string quartet (+ octave violins), harp arpeggios, pad, bass, bells, heartbeat drum, hall reverb.
Writes source/music/here_today_full.adp.  Run from the project root:  python3 tools/make_heretoday_full.py   (needs numpy, scipy, ffmpeg)"""
import sys, os, re, wave
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import midi_read as M
from encode_sfx import encode
import encode_song as ES
SR = 22050; TMP = os.environ.get('TMPDIR', '.')
div, tr = M.parse('tools/here_today.mid')
tempos = [(e[0], int.from_bytes(e[3], 'big')) for e in tr[0] if e[1] == 'meta' and e[2] == 0x51]
def sec(t):
    s = 0; pt = 0; tp = 500000
    for tk, v in tempos:
        if tk >= t: break
        s += (tk - pt) * tp / div / 1e6; pt = tk; tp = v
    return s + (t - pt) * tp / div / 1e6
def tnotes(ev):                                         # (start tick, end tick, midi note, velocity)
    on = {}; out = []
    for e in ev:
        if e[1] != 'ev': continue
        k = e[2] >> 4
        if k == 9 and e[4] > 0: on[e[3]] = (e[0], e[4])
        elif k == 8 or (k == 9 and e[4] == 0):
            if e[3] in on: t0, v = on.pop(e[3]); out.append((t0, e[0], e[3], v))
    return sorted(out)
VOX, GTR, V1, V2, VA, VC = (tnotes(tr[i]) for i in (1, 3, 4, 5, 6, 7))
hz = lambda m: 440 * 2 ** ((m - 69) / 12)
rng = np.random.default_rng(11)
# ---- chords (from the chord-symbol track)
PC = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
QUAL = {'': (0, 4, 7), 'm': (0, 3, 7), 'm7': (0, 3, 7, 10), 'm7b5': (0, 3, 6, 10), 'dim7': (0, 3, 6, 9), '7': (0, 4, 7, 10), 'maj7': (0, 4, 7, 11), '6': (0, 4, 7, 9)}
def chord(sym):
    bass = None
    if '/' in sym: sym, b = sym.split('/'); bass = (PC[b[0]] + (b.count('#') - b.count('b'))) % 12
    r = (PC[sym[0]] + (1 if sym[1:2] == '#' else (-1 if sym[1:2] == 'b' else 0))) % 12
    q = sym[1 + (sym[1:2] in ('#', 'b')):]; return r, QUAL[q], r if bass is None else bass
CH = sorted((e[0], chord(e[3].decode())) for e in tr[2] if e[1] == 'meta' and e[2] == 1)
def chord_at(t): return [c for tk, c in CH if tk <= t][-1]
# ---- sections: source tick range, key shift, layers
V1R, V2R, BRR, CHR, V3R, OUTR = (0, 13440), (13440, 26880), (26880, 46080), (46080, 67200), (67200, 80640), (80640, 90000)
SECS = [
 ('intro', V1R, 0, dict(lead='cello', gt=1.0, st=.8, hp=.9, pd=.5, ramp=(.65, 1))),
 ('verse 1', V1R, 0, dict(v=1, gt=1.0, st=.9, hp=.5, pd=.5)),
 ('verse 2', V2R, 0, dict(v=1, h=.5, gt=1.0, st=1.1, hp=.8, pd=.8, bs=.7)),
 ('bridge', BRR, 0, dict(v=1, gt=.8, st=1.3, hp=.8, pd=1.0, bs=.9, tp=1)),
 ('chorus', CHR, 0, dict(v=1.1, h=.8, gt=.9, st=1.3, sx=.8, hp=1.0, pd=1.0, bs=1.0, bl=.6, tp=1)),
 ('interlude', V2R, 0, dict(lead='violin', gt=.8, st=1.3, hp=1.0, pd=1.0, bs=.8, bl=.5)),
 ('chorus 2', CHR, 2, dict(v=1.15, h=1.0, gt=.9, st=1.5, sx=1.0, hp=1.1, pd=1.2, bs=1.0, bl=1.0, tp=1)),
 ('verse 3', V3R, 2, dict(v=1.1, h=.7, gt=.8, st=1.4, hp=1.0, pd=1.1, bs=.8, bl=.5)),
 ('first ending', OUTR, 2, dict(v=1.1, h=.8, gt=.7, st=1.5, sx=.8, hp=.8, pd=1.2, bs=.6, bl=.7)),
 ('BRIDGE REPRISE', BRR, 2, dict(lead='violin', lead2='flute', st=1.5, sx=.6, hp=1.0, pd=1.3, bs=.8, bl=.4, tp=2, ramp=(.45, 1.25))),
 ('ending again', OUTR, 2, dict(v=1.2, h=1.0, gt=.7, st=1.7, sx=1.0, hp=1.2, pd=1.4, bs=.9, bl=1.0, tp=1)),
]
GAP = {'first ending': 1.6}                            # a breath of silence after the first ending, before the reprise
TAIL = 7.0
A = np.zeros(int((sum(sec(b) - sec(a) for _, (a, b), _, _ in SECS) + sum(GAP.values()) + TAIL + 2) * SR))   # sustained: voice, strings, pad, leads
B = np.zeros_like(A); C = np.zeros_like(A); VB = np.zeros_like(A)                                                                  # plucked: guitar, harp, bells / low: bass, drum
def put(buf, t0, x, g):
    i = int(t0 * SR)
    if i < 0 or i >= len(buf): return
    x = x[:len(buf) - i]; buf[i:i + len(x)] += x * g
def env(n, a, r): e = np.ones(n); k = min(n, max(1, int(a * SR))); e[:k] = np.linspace(0, 1, k); k = min(n, max(1, int(r * SR))); e[-k:] *= np.linspace(1, 0, k); return e
def bp(x, fc, bw): b, a = signal.butter(2, [(fc - bw / 2) / (SR / 2), (fc + bw / 2) / (SR / 2)], 'band'); return signal.lfilter(b, a, x)
def lp(x, fc, o=2): return signal.lfilter(*signal.butter(o, fc / (SR / 2)), x)
def voice(buf, s, e, m, v, g, legato, prev_m):         # the female "ah": formants, late vibrato, a scoop or glide into the note, a swell on long notes, breath on the onset
    n = int((e - s + .2) * SR); t = np.arange(n) / SR; dur = e - s; f0 = hz(m)
    st = (prev_m - m) if legato and abs(prev_m - m) <= 5 else -1.4
    vd = np.clip((t - min(.35, dur * .45)) / .5, 0, 1)
    vib = (.011 + .006 * min(1, dur / 1.5)) * np.sin(2 * np.pi * (5.6 + .5 * np.sin(2 * np.pi * .3 * t)) * t) * vd
    f = f0 * 2 ** (st * np.exp(-t / .07) / 12) * (1 + vib) * (1 + .0012 * rng.standard_normal(n).cumsum() / np.sqrt(n) * 4); ph = 2 * np.pi * np.cumsum(f) / SR
    x = sum(np.sin((k + 1) * ph + .3 * k) / (k + 1) ** 1.05 for k in range(24) if f.max() * (k + 1) < SR * .45)
    nz = rng.standard_normal(n); nz = nz - lp(nz, 2500, 2)
    air = (.05 + .09 * np.exp(-t / .12)) * nz * (1 + .4 * np.sin(2 * np.pi * 5.6 * t)); br = min(1.0, max(0.0, (m - 62) / 14))
    x = bp(x, 850 + 150 * br, 220) * 1.5 + bp(x, 1250 + 200 * br, 300) * .9 + bp(x, 2900, 500) * .5 + .35 * x + air
    sw = 1 + .28 * np.clip(np.sin(np.pi * np.clip(t / max(dur, .3), 0, 1)), 0, 1) * min(1, dur / 1.2)
    put(buf, s, x * env(n, .035 if not legato else .02, .22) * sw, g * .15 * (.55 + .45 * v / 100) * (1 + .12 * br))
def harmony_voice(buf, s, e, m, v, g, t_ticks):         # a soft "ooh" a third or so above, on a tone of the chord that is sounding
    r, iv, _ = chord_at(t_ticks); tones = {(r + i) % 12 for i in iv}
    for up in (4, 3, 5, 7):
        if (m + up) % 12 in tones: break
    else: return
    n = int((e - s + .3) * SR); t = np.arange(n) / SR; f = hz(m + up) * (1 + .007 * np.sin(2 * np.pi * 5.1 * t + 1.3) * np.clip((t - .3) / .6, 0, 1)); ph = 2 * np.pi * np.cumsum(f) / SR
    x = sum(np.sin((k + 1) * ph) / (k + 1) ** 1.6 for k in range(10) if f.max() * (k + 1) < SR * .45); x = bp(x, 650, 260) * 1.6 + bp(x, 1050, 300) * .8 + .2 * x
    put(buf, s, x * env(n, .25, .4), g * .13 * v / 100)
def lead(buf, kind, s, e, m, v, g):                      # the melody as an instrument
    n = int((e - s + .25) * SR); t = np.arange(n) / SR; dur = e - s
    if kind == 'cello': f = hz(m) * (1 + .006 * np.sin(2 * np.pi * 5.0 * t) * np.clip((t - .25) / .5, 0, 1)); x = signal.sawtooth(2 * np.pi * np.cumsum(f) / SR); x = lp(x, 1700, 2) * 1.3; a, r, gg = .09, .3, .13
    elif kind == 'violin': f = hz(m + 12) * (1 + (.007 + .004 * min(1, dur / 1.2)) * np.sin(2 * np.pi * 5.9 * t) * np.clip((t - .2) / .4, 0, 1)); x = signal.sawtooth(2 * np.pi * np.cumsum(f) / SR); x = lp(x, 4200, 2) + .05 * lp(rng.standard_normal(n), 3000, 1) * np.exp(-t / .1); a, r, gg = .06, .3, .1
    else: f = hz(m + 24) * (1 + .005 * np.sin(2 * np.pi * 5.3 * t) * np.clip((t - .3) / .5, 0, 1)); ph = 2 * np.pi * np.cumsum(f) / SR; x = np.sin(ph) + .25 * np.sin(2 * ph) + .06 * np.sin(3 * ph) + .035 * (rng.standard_normal(n) - lp(rng.standard_normal(n), 3500, 1)) * (1 + np.exp(-t / .1)); a, r, gg = .08, .25, .075
    put(buf, s, x * env(n, a, r) * (1 + .2 * np.sin(np.pi * np.clip(t / max(dur, .3), 0, 1)) * min(1, dur / 1.2)), g * gg * (.6 + .4 * v / 100))
def pluck_gtr(buf, s, e, m, v, g):
    n = int(min(e - s + .4, 3.5) * SR); t = np.arange(n) / SR; f = hz(m)
    x = sum(np.sin(2 * np.pi * f * k * t) * np.exp(-t * (2 + 1.6 * k)) / k for k in range(1, 9) if f * k < SR * .45); put(buf, s, x * env(n, .003, .05), g * .24 * v / 100)
def harp(buf, s, m, g, dur=2.6):
    n = int(dur * SR); t = np.arange(n) / SR; f = hz(m)
    x = sum(np.sin(2 * np.pi * f * k * t + .2 * k) * np.exp(-t * (1.1 + 1.2 * k)) / k ** 1.15 for k in range(1, 8) if f * k < SR * .45); x = x + .05 * np.exp(-t / .01) * rng.standard_normal(n)
    put(buf, s, x * env(n, .002, .08), g * .2)
def bell(buf, s, m, g):
    n = int(3.2 * SR); t = np.arange(n) / SR; f = hz(m); x = sum(a * np.sin(2 * np.pi * f * r * t) * np.exp(-t * d) for r, a, d in ((1, 1, 1.1), (2.76, .35, 2.2), (5.4, .12, 3.5), (.5, .25, .9)) if f * r < SR * .45)
    put(buf, s, x * env(n, .002, .3), g * .09)
def strings(buf, s, e, m, v, g, bright=1800):
    n = int((e - s + .55) * SR); t = np.arange(n) / SR; f = hz(m); x = 0
    for c in (-8, -3, 3, 8): x = x + signal.sawtooth(2 * np.pi * f * 2 ** (c / 1200) * (1 + .0015 * np.sin(2 * np.pi * (.4 + .1 * c % 1) * t)) * t + rng.uniform(0, 6))
    x = lp(x, bright, 2); put(buf, s, x * env(n, .4, .55), g * .11 * v / 100)
def pad(buf, s, e, ms, g):
    n = int((e - s + .9) * SR); t = np.arange(n) / SR; x = 0
    for m in ms: f = hz(m); x = x + np.sin(2 * np.pi * f * t) + .4 * np.sin(2 * np.pi * f * 2.003 * t) + .15 * np.sin(2 * np.pi * f * 3.001 * t)
    put(buf, s, lp(x, 1500, 2) * env(n, .9, .9), g * .035)
def bass(buf, s, e, m, g):
    n = int((e - s + .3) * SR); t = np.arange(n) / SR; f = hz(m); x = np.sin(2 * np.pi * f * t) + .35 * np.sin(4 * np.pi * f * t) + .1 * np.sin(6 * np.pi * f * t); put(buf, s, x * env(n, .06, .35), g * .085)
def thump(buf, s, g):
    n = int(.7 * SR); t = np.arange(n) / SR; ph = 2 * np.pi * np.cumsum(46 + 70 * np.exp(-t / .05)) / SR; x = np.sin(ph) * np.exp(-t / .22) + .15 * lp(rng.standard_normal(n), 400, 2) * np.exp(-t / .02); put(buf, s, x, g * .36)
# ---- lay the sections out in time
D = 0.0; log = []
for name, (a, b), shift, L in SECS:
    ramp = L.get('ramp', (1, 1)); span = sec(b) - sec(a); sc = lambda tk: D + sec(tk) - sec(a)
    gain = lambda s_: ramp[0] + (ramp[1] - ramp[0]) * (s_ - D) / span
    v = L.get('v', 0); h = L.get('h', 0); ld = L.get('lead'); ld2 = L.get('lead2')
    prev = None
    for (t0, t1, m, vel) in VOX:
        if not (a <= t0 < b): continue
        s_, e_ = sc(t0), sc(min(t1, b)); gg = gain(s_)
        if v: voice(VB, s_, e_, m + 12 + shift, vel, v * gg, prev is not None and s_ - prev[0] < .12, prev[1] if prev else 0)
        prev = (e_, m + 12 + shift)
        if h: harmony_voice(VB, s_, e_, m + 12 + shift, vel, h * gg, t0)
        if ld: lead(A, ld, s_, e_, m + shift, vel, gg)
        if ld2: lead(A, ld2, s_, e_, m + shift, vel, gg * .8)
    if L.get('gt'):
        for (t0, t1, m, vel) in GTR:
            if a <= t0 < b: s_ = sc(t0); pluck_gtr(B, s_, s_ + 1, m + shift, vel, L['gt'] * gain(s_))
    for trk, sg, br_ in ((V1, 1.0, 1800), (V2, .9, 1800), (VA, 1.0, 1500), (VC, 1.1, 1200)):
        if not L.get('st'): break
        for (t0, t1, m, vel) in trk:
            if a <= t0 < b: s_ = sc(t0); strings(A, s_, sc(min(t1, b)), m + shift, vel, L['st'] * sg * gain(s_), br_)
    if L.get('sx'):
        for (t0, t1, m, vel) in V1:
            if a <= t0 < b: s_ = sc(t0); strings(A, s_, sc(min(t1, b)), m + 12 + shift, vel, L['sx'] * gain(s_) * .55, 3400)
    # chord layers
    segs = [(max(tk, a), min(nxt, b), c) for (tk, c), (nxt, _) in zip(CH, CH[1:] + [(10 ** 9, None)]) if tk < b and nxt > a]
    for (c0, c1, (r, iv, bs_)) in segs:
        s_, e_ = sc(c0), sc(c1); gg = gain(s_); base = 48 + (r if r < 6 else r - 12) + shift; tones = [base + i for i in iv]
        if L.get('pd'): pad(A, s_, e_, tones, L['pd'] * gg)
        if L.get('bs'): bass(C, s_, e_, 36 + ((bs_ + shift) % 12) , L['bs'] * gg)
        if L.get('hp'):
            ladder = tones + [t_ + 12 for t_ in tones] + [tones[0] + 24]; pat = [0, 1, 2, len(tones), len(tones) + 1, len(tones) + 2, len(tones) + 1, len(tones)]
            e8 = 0; tk = c0
            while tk < c1:
                if int((tk - c0) // 240) < 64: harp(B, sc(tk), ladder[pat[e8 % len(pat)] % len(ladder)], L['hp'] * gain(sc(tk)) * (1 if e8 % 4 == 0 else .75))
                tk += 240; e8 += 1
        if L.get('bl'):
            for q, k in enumerate((2, 3, 4, 3)): bell(B, s_ + q * .38 + .02, tones[k % len(tones)] + 36 - (12 if tones[k % len(tones)] > 53 else 0), L['bl'] * gg)
    if L.get('tp'):                                      # the drum: one soft thump on every bar line; the reprise has a heartbeat (two thumps)
        for bt, tx in [(e[0], e[3]) for e in tr[0] if e[1] == 'meta' and e[2] == 6]:
            if a <= bt < b:
                s_ = sc(bt); thump(C, s_, .55 * gain(s_) if L['tp'] == 1 else .42 * gain(s_))
                if L['tp'] == 2: thump(C, s_ + .42, .3 * gain(s_))
    log.append((name, round(D, 1), round(D + span, 1))); D += span + GAP.get(name, 0)
# ---- hall reverb and mix
def reverb(x, T, dark):
    n = int(T * SR); t = np.arange(n) / SR; h = rng.standard_normal(n) * np.exp(-6.9 * t / T); h = lp(h, dark, 1); h = np.concatenate([np.zeros(int(.025 * SR)), h]); h /= np.sqrt((h ** 2).sum())
    return signal.fftconvolve(x, h)[:len(x)]
A = A + VB
out = A + .55 * reverb(A, 2.6, 3200) + B + .5 * reverb(B, 2.2, 4200) + C + .12 * reverb(C, 1.6, 900)
last = int(np.nonzero(np.abs(out) > 1e-3 * np.abs(out).max())[0][-1]) + int(.5 * SR); out = out[:last]
out = signal.lfilter(*signal.butter(1, 55 / (SR / 2), 'high'), out); out *= np.linspace(1, 1, len(out)); fo = int(2.5 * SR); out[-fo:] *= np.linspace(1, 0, fo)
out /= np.abs(out).max() / .9
w = wave.open(TMP + '/here_today_full.wav', 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((out * 32767).astype('<i2').tobytes()); w.close()
x = ES.load(TMP + '/here_today_full.wav'); open('source/music/here_today_full.adp', 'wb').write(encode(x))
for nm_, a_, b_ in [(l[0], l[1], l[2]) for l in log]:
    i0, i1 = int(a_ * SR), int(b_ * SR); r = lambda z: float(np.sqrt((z[i0:i1] ** 2).mean()) + 1e-9)
    if r(VB) > 1e-6: print('   voice vs rest', nm_, round(20 * np.log10(r(VB) / r(A - VB)), 1), 'dB')
print('adp', round(len(x) / ES.RATE, 1), 's'); [print(' ', *l) for l in log]
