# Builds source/gmvox.h: the VOCAL CHOP kit of the music. 24 one-shots (signed 8 bit, 8 kHz) cut from the GBA game's voice clips (tools/voices_src) and pushed hard:
# 12 TONAL chops (the sung "serenade" clip, one steady vowel found in it, retuned to each of the twelve notes: a vocal pad you can play in any key) and 12 TEXTURE chops
# (stretched into clouds, reversed, stuttered, bit-crushed, shuffled, ring-modulated, pitched into squeaks and growls). The music player fires them from the drum-B stream (ids 16 and up).
import numpy as np, wave, os, sys
from scipy.signal import resample_poly, butter, sosfilt, fftconvolve
from math import gcd
HERE = os.path.dirname(os.path.abspath(__file__)); SRC = os.path.join(HERE, '..', '..', 'tools', 'voices_src'); R = 8000
rng = np.random.RandomState(11)
def load(name):
    import glob
    key = name.lower().replace(' ', '_'); path = [p for p in glob.glob(os.path.join(SRC, '*.wav')) if os.path.basename(p)[:-4].lower().replace(' ', '_') == key][0]
    w = wave.open(path); x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
    if w.getnchannels() == 2: x = x.reshape(-1, 2).mean(1)
    return x
def norm(x, peak=0.9): m = np.max(np.abs(x)) or 1.0; return x / m * peak
def fade(x, a=0.006, b=0.03):
    n = len(x); fa = int(a * R); fb = int(b * R); x = x.copy()
    if fa: x[:fa] *= np.linspace(0, 1, fa)
    if fb: x[-fb:] *= np.linspace(1, 0, fb)
    return x
def resamp(x, ratio):          # ratio > 1: higher and shorter
    ratio = float(ratio); g = 1000; num = int(round(ratio * g)); return resample_poly(x, g, num)
def stretch(x, factor, grain=0.06, jitter=0.0):   # granular overlap-add: the length changes, the pitch does not
    gl = int(grain * R); hop_out = gl // 2; hop_in = hop_out / factor; win = np.hanning(gl); n_out = int(len(x) * factor) + gl; y = np.zeros(n_out + gl); pos = 0.0; o = 0
    while o < n_out and int(pos) + gl < len(x):
        p = int(pos + (rng.uniform(-jitter, jitter) * gl if jitter else 0)); p = max(0, min(len(x) - gl, p)); y[o:o + gl] += x[p:p + gl] * win; o += hop_out; pos += hop_in
    return y[:o]
def reverse(x): return x[::-1].copy()
def crush(x, bits=4, hold=3):
    q = 2 ** (bits - 1); y = np.round(x * q) / q; return np.repeat(y[::hold], hold)[:len(x)]
def ring(x, f): t = np.arange(len(x)) / R; return x * np.sin(2 * np.pi * f * t) * 0.8 + x * 0.35
def stutter(x, grain, reps, pitch=1.0):
    g = int(grain * R); seg = x[:g]; seg = resamp(seg, pitch) if pitch != 1 else seg; return np.concatenate([fade(seg, 0.002, 0.01) for _ in range(reps)])
def shuffle(x, pieces=8):
    n = len(x) // pieces; parts = [x[i * n:(i + 1) * n] for i in range(pieces)]; order = rng.permutation(pieces); return np.concatenate([fade(parts[i], 0.003, 0.01) for i in order])
def gate(x, rate=12.0, duty=0.5): t = np.arange(len(x)) / R; return x * (((t * rate) % 1.0) < duty)
def room(x, t=0.12, mix=0.35):   # a quick smear: three decaying echoes
    y = x.copy()
    for k, a in ((1, 0.5), (2, 0.3), (3, 0.18)): d = int(t * k * R); y[d:] += x[:len(x) - d] * a * mix * 2
    return y
def pitch_f0(seg):
    seg = seg - seg.mean(); n = len(seg); ac = fftconvolve(seg, seg[::-1])[n - 1:]; lo = R // 500; hi = R // 70; k = lo + int(np.argmax(ac[lo:hi])); a, b, c = ac[k - 1], ac[k], ac[k + 1]; k = k + 0.5 * (a - c) / (a - 2 * b + c); return R / k
# ---- the tonal source: the loudest steady 0.45 s of the sung serenade
def tonal_source():
    x = load('serenade_good'); win = int(0.45 * R); best = (0, 0)
    for s in range(0, len(x) - win, win // 8):
        seg = x[s:s + win]; rms = np.sqrt(np.mean(seg ** 2)); flat = np.std(np.abs(seg[:win // 2]).mean() - np.abs(seg[win // 2:]).mean()); score = rms - 2.0 * flat
        if score > best[0]: best = (score, s)
    seg = x[best[1]:best[1] + win]; return seg, pitch_f0(seg)
CHOPS = []   # (name, samples, volume 0..127, pan 0..127)
def add(name, x, vol=100, pan=64, maxs=0.9):
    x = norm(fade(x[:int(maxs * R)]), 0.92); CHOPS.append((name, x, vol, pan))
seg, f0 = tonal_source()
NOTES = 'C C# D D# E F F# G G# A A# B'.split()
for pc in range(12):
    midi = 60 + pc if pc < 7 else 48 + pc; target = 440.0 * 2 ** ((midi - 69) / 12.0)       # C4..F#4 and G3..B3: one octave of range around middle C
    y = resamp(seg, target / f0); y = stretch(y, 1.5 if len(y) < 0.5 * R else 1.0, 0.05); y = y[:int(0.55 * R)]
    for _ in range(2):      # measure what came out (the singer wobbles) and nudge it onto the note
        m = pitch_f0(y[len(y) // 5:len(y) // 2 + 1]); y = resamp(y, target / m) if abs(m - target) / target > 0.004 else y
    add('VOX ' + NOTES[pc], room(y, 0.09, 0.3), 90, 40 + (pc * 7) % 48, 0.6)
add('LAUGH CLOUD', room(stretch(load('laughing_2'), 3.0, 0.09, 0.4), 0.14, 0.5), 88, 90)
add('YAHOO BACKWARDS', reverse(load('yahoo')), 96, 30)
add('YEHA STUTTER', stutter(load('yeha'), 0.07, 7, 1.15), 100, 70)
add('SHRIEK CRUSH', crush(load('shriek'), 4, 3), 84, 64)
add('CRY SHUFFLE', shuffle(load('cry'), 10), 90, 100)
add('FIGHT CHOP', gate(resamp(load('lets_fight'), 0.84), 10, 0.55), 100, 50)
add('FLIRT ROBOT', ring(load('flirt'), 95), 90, 80)
add('NAH SUB', stretch(resamp(load('nah'), 0.5), 2.0, 0.05), 110, 64)
add('AGREE STAB', resamp(load('agree'), 1.5)[:int(0.32 * R)], 100, 28)
add('SNEEZE WARP', stretch(resamp(load('sniiize_2'), 0.7), 3.5, 0.08, 0.2), 92, 110)
add('GHOST CHOIR', room(stretch(load('ghost_scared_female_1'), 3.2, 0.1, 0.3) * 0.7 + stretch(resamp(load('ghost_scared_male_2'), 1.19), 2.6, 0.1, 0.3)[:int(0.9 * R)] * 0.6 if False else stretch(load('ghost_scared_female_1'), 3.2, 0.1, 0.3), 0.15, 0.6), 80, 64)
add('WIN STAB', np.concatenate([resamp(load('win_the_fight'), 2.0), np.zeros(int(0.03 * R)), resamp(load('win_the_fight'), 2.0)]), 100, 90)
def write_header(path):
    L = ['#ifndef GMVOX_H', '#define GMVOX_H', '// gmvox.h - GENERATED by godguy/tools/gm_vox.py: the vocal chop kit (signed 8 bit, 8 kHz), cut from the GBA game\'s voice clips and pushed hard. Do not edit.']
    tot = 0
    for i, (nm, x, vol, pan) in enumerate(CHOPS):
        d = np.clip(np.round(x * 127), -127, 127).astype(int); tot += len(d)
        L.append('static const signed char gmVox%d[%d] __attribute__((aligned(4)))={%s};' % (i, len(d), ','.join(map(str, d))))
    L.append('static const GmDrum gmKitC[%d]={%s};' % (len(CHOPS), ','.join('{gmVox%d,%d,%d}' % (i, len(x), R) for i, (nm, x, v, p) in enumerate(CHOPS))))
    L.append('static const unsigned char gmVoxVol[%d]={%s}, gmVoxPan[%d]={%s};' % (len(CHOPS), ','.join(str(v) for _, _, v, _ in CHOPS), len(CHOPS), ','.join(str(p) for _, _, _, p in CHOPS)))
    L.append('#define GM_VOX_N %d' % len(CHOPS)); L.append('#define GM_VOX0 16      // vocal chop id in the drum-B stream = GM_VOX0 + index')
    L.append('static const char*const gmVoxName[%d]={%s};' % (len(CHOPS), ','.join('"%s"' % n for n, _, _, _ in CHOPS))); L.append('#endif')
    open(path, 'w').write('\n'.join(L) + '\n'); return tot
if __name__ == '__main__':
    tot = write_header(os.path.join(HERE, '..', 'source', 'gmvox.h')); print('%d chops, %d bytes, tonal source f0 %.1f Hz' % (len(CHOPS), tot, f0))
    if len(sys.argv) > 1:
        os.makedirs(sys.argv[1], exist_ok=True)
        for nm, x, v, p in CHOPS:
            w = wave.open(os.path.join(sys.argv[1], nm.replace(' ', '_').replace('#', 's') + '.wav'), 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(R); w.writeframes((np.clip(x, -1, 1) * 32000).astype('<i2').tobytes()); w.close()
