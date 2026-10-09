#!/usr/bin/env python3
"""SINGER: a small formant singing synthesizer (no samples, no outside programs): it sings words on notes.

    sing(lines, fs, beat, voice) -> numpy float array
        lines: a list of syllables (phonemes, notes): phonemes is a list like ['V', 'OY', 'S']; notes is a list of
               (start in 16ths, length in 16ths, midi pitch). A syllable that spans several notes is a melisma: the vowel is
               held and slides from note to note; its onset consonants come just before its first note, its coda at the end.
        fs: the sample rate; beat: seconds a beat (a 16th is beat/4); voice: 'diva' (a bright female voice) or 'soft'.

How it sounds: a glottal pulse (Rosenberg) with vibrato that blooms after the attack, a little jitter and breath, through a cascade
of five formant resonators (the vocal tract) whose frequencies glide from phoneme to phoneme; fricatives (S SH F TH V Z) are shaped
noise added beside the tract, stops (P T K B D G) are a closure, a burst and aspiration, nasals (M N NG) a closed, dark tract.
Phonemes are ARPAbet: vowels AA AE AH AO EH ER IH IY OW UH UW and the glides AY AW OY EY; consonants M N NG L R W Y V Z DH F S SH TH HH
P T K B D G.   python3 tools/singer.py OUT.wav sings a test phrase.
"""
import numpy as np
from scipy import signal

# formants (F1 F2 F3, Hz) and bandwidths for a female singing voice; vowels are the targets, glides (AY ...) move between two
V = {'AA': (850, 1220, 2810), 'AE': (860, 2050, 2850), 'AH': (760, 1400, 2780), 'AO': (650, 1000, 2750), 'EH': (600, 2330, 2990),
     'ER': (500, 1350, 1700), 'IH': (430, 2480, 3070), 'IY': (310, 2790, 3310), 'OW': (520, 900, 2700), 'UH': (470, 1160, 2680),
     'UW': (370, 950, 2670)}
GLIDE = {'AY': ('AA', 'IY'), 'AW': ('AA', 'UW'), 'OY': ('AO', 'IY'), 'EY': ('EH', 'IY'), 'OW': ('OW', 'UW')}
# consonants: formants, voicing, frication (amp, band lo, band hi), kind
C = {'M': ((280, 1300, 2400), .45, None, 'nasal'), 'N': ((280, 1700, 2600), .45, None, 'nasal'), 'NG': ((280, 2300, 2750), .45, None, 'nasal'),
     'L': ((360, 1300, 3000), .7, None, 'liquid'), 'R': ((420, 1300, 1600), .7, None, 'liquid'), 'W': ((300, 610, 2200), .7, None, 'liquid'),
     'Y': ((300, 2500, 3200), .7, None, 'liquid'), 'V': ((300, 1500, 2400), .35, (.25, 2500, 7000), 'fric'), 'DH': ((300, 1600, 2600), .35, (.15, 2500, 6000), 'fric'),
     'Z': ((300, 1700, 2600), .3, (.35, 4000, 9000), 'fric'), 'F': ((400, 1500, 2500), 0, (.3, 1500, 8000), 'fric'), 'TH': ((400, 1600, 2600), 0, (.2, 2000, 8000), 'fric'),
     'S': ((400, 1700, 2600), 0, (.6, 4500, 9500), 'fric'), 'SH': ((400, 1800, 2400), 0, (.55, 2200, 6000), 'fric'), 'HH': (None, 0, None, 'asp'),
     'P': ((400, 1000, 2400), 0, (.5, 500, 2500), 'stop'), 'T': ((400, 1700, 2600), 0, (.55, 3000, 8000), 'stop'), 'K': ((400, 1900, 2500), 0, (.5, 1500, 4000), 'stop'),
     'B': ((300, 1000, 2400), .2, (.35, 500, 2500), 'stop'), 'D': ((300, 1700, 2600), .2, (.4, 3000, 7000), 'stop'), 'G': ((300, 1900, 2500), .2, (.35, 1500, 4000), 'stop')}
CDUR = {'nasal': .065, 'liquid': .06, 'fric': .085, 'asp': .06, 'stop': .07}

def _plan(lines, beat, scale):
    """frame-by-frame targets (2 ms frames): formants, voicing, frication, aspiration, pitch."""
    s16 = beat / 4; end = max(n[0] + n[1] for _, ns in lines for n in ns) * s16 + .6
    fr = .002; N = int(end / fr) + 1
    F = np.zeros((N, 3)); F[:] = V['AH']; av = np.zeros(N); af = np.zeros(N); fl = np.full(N, 4000.); fh = np.full(N, 8000.); ah = np.zeros(N)
    f0 = np.zeros(N); vib = np.zeros(N); burst = np.zeros(N)
    def fi(t): return int(max(0, t) / fr)
    prev_end = 0.0
    for k, (ph, ns) in enumerate(lines):
        t0 = ns[0][0] * s16; t1 = (ns[-1][0] + ns[-1][1]) * s16
        nxt = lines[k + 1][1][0][0] * s16 if k + 1 < len(lines) else t1 + 1
        t1 = min(t1, nxt - .02)
        vi = [i for i, p in enumerate(ph) if p in V or p in GLIDE]
        vi = vi[0] if vi else len(ph)
        onset, nucleus, coda = ph[:vi], ph[vi] if vi < len(ph) else 'AH', ph[vi + 1:]
        # onset consonants just before the note (they steal a little from the gap before)
        t = t0 - sum(CDUR[C[c][3]] for c in onset) * scale
        t = max(t, prev_end + .005)
        for c in onset:
            d = CDUR[C[c][3]] * scale; _cons(c, fi(t), fi(t + d), F, av, af, fl, fh, ah, burst); t += d
        # the vowel, held over all its notes, then the coda at the end
        cd = sum(CDUR[C[c][3]] for c in coda) * scale
        vend = max(t0 + .04, t1 - cd)
        a, b = fi(t0), fi(vend)
        if nucleus in GLIDE:
            v0, v1 = GLIDE[nucleus]; m = a + int((b - a) * .62)
            F[a:m] = V[v0]; tr = np.linspace(0, 1, max(1, b - m))[:, None]; F[m:b] = np.array(V[v0]) * (1 - tr) + np.array(V[v1]) * tr
        else: F[a:b] = V[nucleus]
        av[a:b] = 1.0; ah[a:b] = .035
        t = vend
        for c in coda:
            d = CDUR[C[c][3]] * scale; _cons(c, fi(t), fi(t + d), F, av, af, fl, fh, ah, burst); t += d
        prev_end = t
        # pitch: each note, a quick slide into it; vibrato blooms on long notes
        for (s, l, m) in ns:
            i0, i1 = fi(s * s16), fi((s + l) * s16); f0[max(0, i0 - 8):i1 + 25] = 440 * 2 ** ((m - 69) / 12)
            ln = i1 - i0
            if ln > 90: vib[i0 + 60:i1] = np.minimum(1, np.arange(ln - 60) / 120)
    # fill unvoiced pitch from neighbours; smooth the tracks (articulation takes time)
    nz = np.flatnonzero(f0)
    if len(nz): f0 = np.interp(np.arange(N), nz, f0[nz])
    k = np.ones(9) / 9
    for j in range(3): F[:, j] = np.convolve(F[:, j], k, 'same')
    f0 = np.exp(np.convolve(np.log(np.maximum(f0, 50)), np.ones(13) / 13, 'same'))
    av = np.convolve(av, np.ones(5) / 5, 'same'); vib = np.convolve(vib, np.ones(25) / 25, 'same')
    return fr, N, F, av, af, fl, fh, ah, f0, vib, burst

def _cons(c, a, b, F, av, af, fl, fh, ah, burst):
    form, voi, fric, kind = C[c]
    if form: F[a:b] = form
    if kind == 'asp': ah[a:b] = .5; av[a:b] = 0; return
    if kind == 'stop':   # closure, then a burst and (unvoiced) aspiration in the last third
        m = a + (b - a) * 2 // 3; av[a:m] = voi; av[m:b] = voi * 2
        af[m:m + 5] = fric[0]; fl[m:b] = fric[1]; fh[m:b] = fric[2]; burst[m] = 1
        if voi == 0: ah[m + 4:b] = .3
        return
    av[a:b] = voi
    if fric: af[a:b] = fric[0]; fl[a:b] = fric[1]; fh[a:b] = fric[2]

def sing(lines, fs, beat, voice='diva', seed=7, scale=1.0):
    rng = np.random.default_rng(seed)
    fr, N, F, av, af, fl, fh, ah, f0, vib, burst = _plan(lines, beat, scale)
    n = int(N * fr * fs); t = np.arange(n) / fs; fi = np.minimum((t / fr).astype(int), N - 1)
    # glottal source: Rosenberg pulses at f0 with vibrato (5.6 Hz, up to a third of a semitone) and a little jitter
    vibr = 2 ** (np.sin(2 * np.pi * 5.6 * t) * vib[fi] * .33 / 12)
    jit = 1 + .004 * np.convolve(rng.standard_normal(n), np.ones(200) / 200, 'same') * 14
    ph = np.cumsum(f0[fi] * vibr * jit / fs) % 1.0
    oq = .62 if voice == 'diva' else .7
    g = np.where(ph < oq * .65, .5 * (1 - np.cos(np.pi * ph / (oq * .65))), np.where(ph < oq, np.cos(np.pi * (ph - oq * .65) / (2 * oq * .35)), 0.0))
    src = np.diff(np.concatenate([[0], g])) * fs / 2000                         # the radiation at the lips: the pulse's slope
    src = src * np.interp(t / fr, np.arange(N), av) + rng.standard_normal(n) * .12 * np.interp(t / fr, np.arange(N), ah)
    # the vocal tract: five resonators in cascade, coefficients changed every frame
    shift = 1.0 if voice == 'diva' else .93
    out = np.zeros(n); spf = max(1, int(fr * fs))
    zs = [np.zeros(2) for _ in range(5)]
    BW = (80, 100, 140, 220, 300)
    for i in range(0, n, spf):
        j = min(i // spf, N - 1); x = src[i:i + spf]
        fs_ = list(F[j] * shift) + [3600 * shift, 4400 * shift]
        for k in range(5):
            f, bw = min(fs_[k], fs * .45), BW[k] + (40 if k == 0 else 0)
            r = np.exp(-np.pi * bw / fs); a1 = 2 * r * np.cos(2 * np.pi * f / fs); a2 = -r * r
            x, zs[k] = signal.lfilter([1 - a1 - a2], [1, -a1, -a2], x, zi=zs[k])
        out[i:i + spf] = x
    # frication beside the tract, and the stop bursts
    nz = rng.standard_normal(n); fric = np.zeros(n)
    for i in range(0, n, spf * 4):
        j = min(i // spf, N - 1)
        if af[j:j + 4].max() <= 0: continue
        lo, hi = fl[j], min(fh[j], fs * .45)
        b_, a_ = signal.butter(2, [lo / (fs / 2), hi / (fs / 2)], 'band')
        seg = nz[max(0, i - 400):i + spf * 4]; y = signal.lfilter(b_, a_, seg)[-len(nz[i:i + spf * 4]):]
        fric[i:i + spf * 4] = y * np.interp(np.arange(i, i + len(y)) / fs / fr, np.arange(N), af)
    out = out / (np.abs(out).max() + 1e-9) + fric * .8
    # breath: a soft high shelf of air, and a gentle high-pass
    b_, a_ = signal.butter(2, 90 / (fs / 2), 'high'); out = signal.lfilter(b_, a_, out)
    return out / (np.abs(out).max() + 1e-9)

if __name__ == '__main__':
    import sys, wave
    L = [(['OW'], [(0, 2, 66)]), (['V', 'ER'], [(2, 2, 64)]), (['M', 'AY'], [(4, 2, 66)]), (['V', 'OY', 'S'], [(6, 6, 69), (12, 4, 68)])]
    fs = 33452; x = sing(L, fs, .47)
    w = wave.open(sys.argv[1] if len(sys.argv) > 1 else 'sing.wav', 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(fs)
    w.writeframes((x * 30000).astype('<i2').tobytes()); w.close()
