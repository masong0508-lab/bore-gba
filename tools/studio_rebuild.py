#!/usr/bin/env python3
"""Rebuilt sounds for the studio renders (tools/studio_render.py) of the songs built on recorded samples.  The game keeps its own samples.

1. TOP END (every recorded sound): the samples of these songs were recorded at 2-16 kHz, so nothing in them reaches past 1-8 kHz.  bwe()
   gives each one back the octaves above: the top octave it does have is moved up one, two and three octaves (its analytic signal raised
   to the 2nd / 4th / 8th power: frequencies double, a harmonic tone stays harmonic, noise stays noise, the loudness envelope stays the
   same) and laid in above the old ceiling at the level the sound's own spectral slope says it would have had.  Below the old ceiling
   nothing changes, so every sound keeps its character.
2. GOTTCHO BARRACHO (ORIGINAL): its tune is a banda tune (El Menchón by Banda MS) and its pitched sounds were cut from that recording's
   stems ("bass", "piano", "other"), as were six of its drum hits.  Those are rebuilt here as new instruments that play the same notes:
   a tuba, alto horns (the "armonías"), a trumpet and clarinet section, a tambora, a tarola and cymbals, all synthesised at 48 kHz,
   in equal temperament (the stems' own tunings disagreed by up to 40 cents), each level-matched to the sound it replaces.
"""
import numpy as np
from scipy import signal

FS = 48000

# ================================================================ 1. the top end of a recorded sound
def spectral_slope(x, r, lo, hi):
    """dB per octave of the sound's spectrum between lo and hi Hz (a straight line through the log-log Welch spectrum)"""
    f, p = signal.welch(np.asarray(x, float), r, nperseg=min(len(x), 2048))
    m = (f >= lo) & (f <= hi) & (p > 0)
    if m.sum() < 4: return -12.0
    return float(np.polyfit(np.log2(f[m]), 10 * np.log10(p[m]), 1)[0])

def bwe(x, r, top=20000.0, slope_range=(-15.0, -6.0)):
    """x recorded at r Hz -> (y, r2): the same sound at a higher rate with the octaves above r/2 rebuilt (up to `top` Hz, 3 octaves at most)"""
    x = np.asarray(x, float); nyq = r / 2
    if nyq >= 15000 or len(x) < 256: return x, r
    U = 2
    while r * U < min(2 * top + 4000, 192000) and U < 8: U *= 2               # room for the new octaves
    R = r * U; y = signal.resample_poly(x, U, 1, window=('kaiser', 8.0))
    sos = lambda kind, f: signal.butter(6, f, kind, fs=R, output='sos')
    b = signal.sosfiltfilt(signal.butter(4, [nyq * 0.5, nyq * 0.92], 'band', fs=R, output='sos'), y)   # the top octave it has
    if np.sqrt((b * b).mean()) < 10 ** (-48 / 20) * np.sqrt((y * y).mean()) + 1e-9: return y, R       # (nothing up there: a sub, a kick)
    sl = float(np.clip(spectral_slope(x, r, nyq * 0.25, nyq * 0.9), *slope_range))
    z = signal.hilbert(b); a = np.abs(z) + 1e-9; u = z / a; rb = np.sqrt((b * b).mean())
    hb = np.zeros_like(y); k = 1
    for oct_ in (1, 2, 3):
        k *= 2; lo = nyq * 2 ** (oct_ - 1) * 0.92
        if lo >= min(top, R * 0.45): break
        d = np.real(a * u ** k)                                                  # every frequency of the band times k, same envelope
        d = signal.sosfiltfilt(sos('high', lo), d)
        hi = min(top, R * 0.45, nyq * 2 ** oct_ * 0.98)
        if hi <= lo * 1.1: break
        d = signal.sosfiltfilt(sos('low', hi), d)
        rd = np.sqrt((d * d).mean()) + 1e-12
        hb += d * (rb / rd) * 10 ** (sl * oct_ / 20)                             # an octave up = the slope's dB lower
    return y + hb, R

# ================================================================ 2. banda instruments (Gottcho Barracho (Original))
rng = np.random.default_rng(1977)
def mtof(m): return 440.0 * 2 ** ((m - 69) / 12)

def _env(n, att, dur, rel, dec=0.0, sus=1.0):
    t = np.arange(n) / FS
    e = np.minimum(1, t / max(att, 1e-4)) ** 1.5
    if dec: e *= sus + (1 - sus) * np.exp(-np.maximum(0, t - att) / dec)
    e *= np.clip((dur + rel - t) / rel, 0, 1) ** 1.5                            # release from the note's end
    return e

def additive(f0, n, cents, tilt_fc, power=2.0, odd=0.0, formant=None, jitter=0.002):
    """a harmonic tone: harmonic k at k*f(t), its level 1/(1+(k f0/fc(t))^power) (fc may move: brass brightens as it speaks),
    even harmonics `odd` dB down (a clarinet's hollow low register), an optional formant (centre Hz, gain dB, width octaves)"""
    t = np.arange(n) / FS
    f = f0 * 2 ** (cents / 1200); ph = 2 * np.pi * np.cumsum(f) / FS
    K = int(min(19000, FS * 0.45) / f0); y = np.zeros(n)
    for k in range(1, max(2, K) + 1):
        fk = k * f0; lv = 1.0 / (1.0 + (fk / tilt_fc) ** power)
        if odd and k % 2 == 0 and fk < 1800: lv = lv * 10 ** (-odd / 20)
        if formant is not None:
            c, g, w = formant; lv = lv * 10 ** (g / 20 * np.exp(-0.5 * (np.log2(fk / c) / w) ** 2))
        if np.max(lv) < 3e-4: continue                                       # (below -70 dB: not worth the sine)
        y += lv * np.sin(k * ph + rng.uniform(0, 2 * np.pi) + jitter * rng.standard_normal() * k)
    return y

def breath(n, lo, hi, env):
    b = signal.sosfilt(signal.butter(2, [lo, hi], 'band', fs=FS, output='sos'), rng.standard_normal(n)); return b * env

PRESETS = {   # fc: where the harmonics start to fall (Hz) at full blow; bloom: how much darker it starts; att: attack s; vib: vibrato cents
    'tuba':     dict(fc=380, bloom=0.45, att=0.035, rel=0.06, scoop=-20, vib=6, vibr=4.8, power=2.2, breath=0.010, maxlen=0.55, formant=(240, 4, 0.7)),
    'trombone': dict(fc=750, bloom=0.4, att=0.03, rel=0.06, scoop=-20, vib=5, vibr=5.0, power=2.0, breath=0.012, maxlen=0.7, formant=(520, 4, 0.6)),
    'horn':     dict(fc=900, bloom=0.5, att=0.025, rel=0.07, scoop=-12, vib=5, vibr=5.2, power=2.3, breath=0.010, maxlen=0.7, formant=(700, 3, 0.6)),
    'trumpet':  dict(fc=1500, bloom=0.4, att=0.022, rel=0.07, scoop=-25, vib=14, vibr=5.8, power=1.9, breath=0.016, maxlen=1.4, formant=(1300, 5, 0.6)),
    'clarinet': dict(fc=1700, bloom=0.7, att=0.03, rel=0.06, scoop=-8, vib=12, vibr=5.6, power=2.4, breath=0.020, maxlen=1.4, odd=16, formant=(1500, 3, 0.7)),
}
def note(kind, m, dur, vel=1.0, detune=0.0):
    """one note of a banda instrument: MIDI pitch m, held dur seconds (the release comes after), velocity 0..1"""
    p = PRESETS[kind]; dur = max(0.05, min(dur, p['maxlen'])); n = int((dur + p['rel']) * FS); t = np.arange(n) / FS
    f0 = mtof(m)
    cents = detune + p['scoop'] * np.exp(-t / 0.03) + 3 * rng.standard_normal() + 2.0 * np.sin(2 * np.pi * 0.7 * t + rng.uniform(0, 6))
    if dur > 0.28: cents = cents + p['vib'] * np.sin(2 * np.pi * p['vibr'] * t + rng.uniform(0, 6)) * np.clip((t - 0.18) / 0.25, 0, 1)
    fc = p['fc'] * (0.75 + 0.35 * vel) * (1 - p['bloom'] * np.exp(-t / (p['att'] * 1.5)))   # the bloom: dark at the start, then the brass opens
    y = additive(f0, n, cents, fc, p['power'], p.get('odd', 0.0), p['formant'])
    e = _env(n, p['att'], dur, p['rel'], dec=0.6, sus=0.82)
    y = y * e + breath(n, 1500, 7000, e * (0.4 + 0.6 * np.exp(-t / 0.05))) * p['breath'] * 3
    return y.astype(np.float32)

def ensemble(parts, m, dur, vel):
    """a section: several players on one note, slightly apart in tuning and time"""
    out = None
    for kind, gain, det, delay in parts:
        y = note(kind, m, dur, vel, det) * gain; d = int(delay * FS)
        y = np.concatenate([np.zeros(d, np.float32), y])
        if out is None: out = y
        else:
            L = max(len(out), len(y)); out = np.pad(out, (0, L - len(out))) + np.pad(y, (0, L - len(y)))
    return out

def instrument(kind):
    """note(m, dur, vel) for an instrument of the band (what studio_render calls for every note of a rebuilt channel)"""
    if kind == 'tuba':
        return lambda m, dur, vel: note('tuba', m, dur, vel)
    if kind == 'horns':      # the armonías: two alto horns; low notes go to a trombone
        return lambda m, dur, vel: (ensemble([('trombone', 0.8, 0, 0)], m, dur, vel) if m < 52 else
                                    ensemble([('horn', 0.6, -4, 0), ('horn', 0.6, 5, 0.006)], m, dur, vel))
    if kind == 'section':    # the tune: two trumpets and a clarinet (a trombone below the trumpets' range)
        return lambda m, dur, vel: (ensemble([('trombone', 0.8, 0, 0)], m, dur, vel) if m < 55 else
                                    ensemble([('trumpet', 0.55, -5, 0), ('trumpet', 0.5, 6, 0.008), ('clarinet', 0.42, 2, 0.004)], m, dur, vel))
    raise KeyError(kind)

# ---------------------------------------------------------------- the drums: tambora, tarola, cymbals (one-shot samples at 48 kHz)
def colour_of(ref, r, top=18000.0):
    """an FIR filter (at 48 kHz) with the recorded hit's own spectrum: its smoothed spectrum up to where the recording stops, then its
    top octave's slope carried on (falling 6 to 12 dB per octave) up to `top`, and a steep fall after"""
    ref = np.asarray(ref, float); f, p = signal.welch(ref, r, nperseg=min(len(ref), 1024)); a = np.sqrt(p + 1e-20)
    lf = np.log2(np.maximum(f, 1.0)); sm = np.array([a[np.abs(lf - v) < 1 / 6].mean() for v in lf])   # 1/3-octave smoothing
    nyq = r / 2; e = nyq * 0.9; sl = float(np.clip(spectral_slope(ref, r, nyq / 4, e), -12.0, -6.0))
    fg = np.linspace(0, FS / 2, 1025); H = np.interp(np.minimum(fg, e), f, sm)
    up = fg > e; H[up] *= 10 ** (sl / 20 * np.log2(fg[up] / e))
    hi = fg > top; H[hi] *= 10 ** (-24 / 20 * np.log2(fg[hi] / top))
    H /= H.max() + 1e-20
    return signal.firwin2(1023, fg / (FS / 2), H)

def drum(kind, ref, r):
    """a new drum hit shaped like the recorded one (ref at r Hz): the same length and loudness envelope, a full-band body of its own"""
    ref = np.asarray(ref, float); dur = len(ref) / r; n = int(dur * FS) + 1; t = np.arange(n) / FS
    w = max(1, int(r * 0.004)); envr = np.sqrt(np.convolve(ref ** 2, np.ones(w) / w, 'same'))      # the recorded hit's envelope
    env = np.interp(t, np.arange(len(ref)) / r, envr / (envr.max() + 1e-12))
    noise = rng.standard_normal(n)
    if kind == 'tambora':        # the big bass drum: a falling boom, the beater, a wooden shell
        f0 = _pitch(ref, r, 35, 200) or 58.0
        ph = 2 * np.pi * np.cumsum(f0 * (1 + 0.9 * np.exp(-t / 0.018))) / FS
        y = np.sin(ph) * np.exp(-t / 0.22) + 0.35 * np.sin(2 * ph * 1.48) * np.exp(-t / 0.06)
        y += 0.5 * signal.sosfilt(signal.butter(2, [900, 5000], 'band', fs=FS, output='sos'), noise) * np.exp(-t / 0.006)
        y *= np.maximum(env, np.exp(-t / 0.25) * 0.15)
    elif kind in ('tarola', 'tarola_hi'):   # the snare: a tight high head and the wires, in the recorded hit's colour
        head = 310 if kind == 'tarola' else 380
        y = 0.6 * np.sin(2 * np.pi * head * t) * np.exp(-t / 0.035) + 0.3 * np.sin(2 * np.pi * head * 1.6 * t) * np.exp(-t / 0.02)
        y = y + signal.oaconvolve(noise, colour_of(ref, r), 'same') * 5.0
        y *= env
    else:                        # cymbals: a metallic cluster of square-wave partials and noise, in the recorded hit's colour
        fr = [313, 517, 785, 1043, 1395, 1739, 2213, 3049] if kind != 'tick' else [2400, 3700, 5300, 7900]
        sq = sum(np.sign(np.sin(2 * np.pi * f * t + rng.uniform(0, 6))) for f in fr) / len(fr)
        y = signal.oaconvolve(0.5 * sq + 0.5 * noise, colour_of(ref, r), 'same')
        y *= env
    y *= np.sqrt((ref ** 2).mean() / ((np.interp(np.arange(len(ref)) / r, t, y) ** 2).mean() + 1e-20))   # the recorded hit's loudness
    if kind in ('tick', 'platillo', 'crash'): y *= 10 ** (-2 / 20)   # (their new top octaves: sat 2 dB back so the cymbals do not lead the mix)
    return y.astype(np.float32)

def _pitch(x, r, fmin, fmax):
    x = np.asarray(x, float)[:int(r * 0.25)]
    if len(x) < int(r / fmin) * 2: return None
    ac = signal.correlate(x, x, 'full', method='fft')[len(x) - 1:]; ac /= ac[0] + 1e-12
    lo, hi = int(r / fmax), int(r / fmin); k = lo + int(np.argmax(ac[lo:hi])); return r / k if ac[k] > 0.3 else None

# which sounds of which song are rebuilt: instrument number -> a band instrument (pitched, played per note) or a drum (a one-shot)
REBUILD = {'gottcho_barracho.xm': {1: ('tuba', 'tuba bass'), 2: ('horns', 'alto horns'), 3: ('section', 'trumpets and clarinet'),
                                    4: ('drum:tambora', 'tambora'), 9: ('drum:tarola', 'tarola'), 10: ('drum:tarola_hi', 'tarola rim'),
                                    13: ('drum:tick', 'cymbal tick'), 19: ('drum:platillo', 'platillo'), 20: ('drum:crash', 'crash cymbal')}}
NODUCK = {'gottcho_barracho'}   # (a banda is not pumped by its bass drum)
