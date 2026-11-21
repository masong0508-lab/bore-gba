# Builds source/gmvox.h: the VOCAL CHOP kit of the music. 24 one-shots (signed 8 bit, 8 kHz) cut from the GBA game's voice clips (tools/voices_src) and pushed hard:
# 12 TONAL chops (the sung "serenade" clip, one steady vowel found in it, retuned to each of the twelve notes: a vocal pad you can play in any key) and 12 TEXTURE chops
# (stretched into clouds, reversed, stuttered, bit-crushed, shuffled, ring-modulated, pitched into squeaks and growls). The music player fires them from the drum-B stream (ids 16 and up).
import json, numpy as np, wave, os, sys
from scipy.signal import resample_poly, butter, sosfilt, fftconvolve, iirnotch, lfilter
from math import gcd
HERE = os.path.dirname(os.path.abspath(__file__)); SRC = os.path.join(HERE, '..', '..', 'tools', 'voices_src'); R = 8000
rng = np.random.RandomState(11)
def load(name):
    import glob
    key = name.lower().replace(' ', '_'); path = [p for p in glob.glob(os.path.join(SRC, '*.wav')) if os.path.basename(p)[:-4].lower().replace(' ', '_') == key][0]
    w = wave.open(path); x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
    if w.getnchannels() == 2: x = x.reshape(-1, 2).mean(1)
    return x
def norm(x, peak=0.9):
    # Preserve a predictable level, but don't amplify whispered noise
    # into a shrill, full-volume fake horn.
    m = np.max(np.abs(x)) if len(x) else 0.0
    return x * min(3.0, peak / max(m, 0.025))
def master_voice(x, sr=R, lead=False):
    """Offline-only vocal mastering: de-rumble, tame the 850-Hz
    nasal/honk band and gently de-ess; no DSP is added to the 3DS mixer.
    The original singer's consonant attacks are preserved."""
    if len(x) < 16: return x
    x = np.asarray(x, dtype=np.float64)
    x = sosfilt(butter(2, 105 if lead else 145, 'highpass', fs=sr, output='sos'), x)
    # Broad, shallow notch rather than destroying the formants.
    b, a = iirnotch(860 if lead else 1050, 1.5, fs=sr)
    x = 0.74*x + 0.26*lfilter(b,a,x)
    x = sosfilt(butter(2, min(6100 if lead else 3000, sr*0.43),
                            'lowpass', fs=sr, output='sos'), x)
    # Mild peak control, not crushing: the consonants still carry rhythm.
    x = np.tanh(x*1.15) / np.tanh(1.15)
    return x
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
SCALE_PCS = (4, 6, 7, 9, 11, 0, 2)      # E natural minor: the key of DUCK & COVER
def f0_track(x, sr, hop=0.01):     # midi pitch per 10 ms hop (nan where unvoiced), by autocorrelation
    n = int(0.045 * sr); h = int(hop * sr); lo = int(sr / 520); hi = int(sr / 80); out = []
    for i in range(0, len(x) - n, h):
        s = x[i:i + n] * np.hanning(n)
        if np.sqrt((s ** 2).mean()) < 0.008: out.append(np.nan); continue
        ac = np.correlate(s, s, 'full')[n - 1:]; k = lo + int(np.argmax(ac[lo:hi]))
        if ac[0] <= 0 or ac[k] / ac[0] < 0.45: out.append(np.nan); continue
        if 0 < k - 1 and k + 1 < len(ac):
            a, b, c = ac[k - 1], ac[k], ac[k + 1]; d = a - 2 * b + c; k = k + (0.5 * (a - c) / d if d else 0)
        out.append(69 + 12 * np.log2(sr / k / 440.0))
    return np.array(out)
def song_notes(pad):      # what DUCK & COVER plays under a lyric (written by gm4_compose.py): melody and bass notes, weighted by how long they sound
    p = os.path.join(HERE, 'duck_lyric_notes.json')
    try: d = json.load(open(p))[str(pad)]
    except Exception: return None
    return {int(n): w for n, w in d['mel'].items()}, {int(n): w for n, w in d['bass'].items()}
def snap_target(m, pcw=None):      # the nearest note of the song's own harmony (or of the key, when the song gives none): the notes it plays there, the heavy ones preferred
    pcw = pcw or {pc: 1.0 for pc in SCALE_PCS}; best = None; bc = 1e9
    for o in range(int(m) - 2, int(m) + 3):
        for pc, wgt in pcw.items():
            for t in (o // 12 * 12 + pc - 12, o // 12 * 12 + pc, o // 12 * 12 + pc + 12):
                c = abs(t - m) - 0.12 * np.log2(1 + wgt)
                if c < bc: bc = c; best = t
    return best
def tune_curve(x, sr, pad=None):      # semitones of correction per hop: the pitch, moved as a whole so the phrase's top note lands on the song's top note there, then each note onto the song's harmony; held through unvoiced bits and eased so it never steps
    m = f0_track(x, sr); v = ~np.isnan(m)
    if v.sum() < 3: return np.zeros(len(m)), 0.0
    idx = np.arange(len(m)); mm = np.interp(idx, idx[v], m[v])
    sm = np.array([np.median(mm[max(0, i - 3):i + 4]) for i in range(len(mm))]); sn = song_notes(pad) if pad is not None else None
    pcw = {pc: 0.5 for pc in SCALE_PCS}; top = None
    if sn:
        mel, bas = sn
        for n, w in mel.items(): pcw[n % 12] = pcw.get(n % 12, 0) + w
        for n, w in bas.items(): pcw[n % 12] = pcw.get(n % 12, 0) + w * 0.5
        heavy = [n for n, w in mel.items() if w >= 8]
        if heavy and max(heavy) >= 63: top = max(heavy)
    # The Koala project is two semitones above the new song (its hook's top note, G#4, is the song's F#4): every phrase is moved down a whole tone, then each note onto the song's harmony
    off = -2.0; cost = 0.0
    tgt = np.array([snap_target(p + off, pcw) for p in sm])
    if top is not None: tgt = np.minimum(tgt, top)      # (nothing sung above the hook's top note)
    # Avoid wobbling/robot honks from snapping every 10-ms syllable to
    # a new pitch. Gentle, slow correction retains natural vowel formants.
    corr = np.clip((tgt - mm)*0.55, -1.35, 1.35)
    k = 5; corr = np.convolve(np.pad(corr, k, mode='edge'), np.ones(2 * k + 1) / (2 * k + 1), 'valid')
    return corr, float(cost)
def tune_stretch(x, sr, factor, pad=None, grain=0.04):   # WSOLA stretch (each grain is slid to the place that continues the last one, so voiced sound stays coherent instead of beating) with every grain's pitch pushed onto the key
    corr, _ = tune_curve(x, sr, pad); gl = int(grain * sr); hop_out = gl // 2; hop_in = hop_out / factor; win = np.hanning(gl); S = int(0.012 * sr)
    n_out = int(len(x) * factor) + gl; y = np.zeros(n_out + 2 * gl); nom = 0.0; o = 0; prev = None; rprev = 1.0
    xp = np.concatenate([x, np.zeros(6 * gl)])
    while o < n_out and nom < len(x):
        ci = min(len(corr) - 1, int((nom + gl / 2) / (0.01 * sr))); r = 2 ** (corr[ci] / 12.0); need = int(gl * r) + 2
        p = int(nom)
        if prev is not None:
            ref = xp[prev + int(hop_out * rprev):prev + int(hop_out * rprev) + gl]
            lo = max(0, p - S); best = -1e18; bp = p
            for q in range(lo, p + S + 1, 2):
                c = float(np.dot(ref[:gl:2], xp[q:q + gl:2]))
                if c > best: best = c; bp = q
            p = bp
        seg = xp[p:p + need]; gseg = np.interp(np.arange(gl) * r, np.arange(len(seg)), seg); y[o:o + gl] += gseg * win
        prev = p; rprev = r; o += hop_out; nom += hop_in
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
    for k, a in ((1, 0.5), (2, 0.3), (3, 0.18)):
        d = int(t * k * R)
        if d < len(x): y[d:] += x[:len(x) - d] * a * mix * 2
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
CRATE = []   # each chop's own sample rate
def balance_vocal(x, lead):
    """Match audible syllable RMS without crushing the singer's phrasing.
    One constant gain per *phrase* (not a peak-by-peak compressor).
    Ignore near-silent tails to avoid boosting hiss or reverb noise."""
    if len(x)<32:return x
    peak=float(np.max(np.abs(x)))
    if peak<1e-4:return x
    active=np.abs(x)>max(0.012,peak*0.11)
    if not np.any(active):return x
    rms=float(np.sqrt(np.mean(np.square(x[active]))))
    target=0.20 if lead else 0.135
    gain=float(np.clip(target/max(rms,0.025),0.72,1.8))
    if peak*gain>0.89:gain=0.89/peak
    return x*gain
def add(name, x, vol=100, pan=64, maxs=0.9, rate=R):
    x = np.asarray(x[:int(maxs * rate)], dtype=np.float64)
    # Character chops get warm EQ and softened splices; full sung lyric
    # phrases instead get their own gentler, intelligibility-first chain.
    lyric = name.startswith('LYRIC PAD') or name.startswith('CORP LYRIC')
    x = master_voice(x, rate, lead=lyric)
    x = fade(x, 0.005 if lyric else 0.013,
             0.025 if lyric else 0.055)
    if lyric:
        # Crucial: do not peak-normalize AGAIN after matching RMS, which
        # would undo the loudness adjustment and make quiet words leap
        # back to full volume. Limit genuine peaks instead.
        x = balance_vocal(x, lead=True)
        top = float(np.max(np.abs(x))) if len(x) else 0.
        if top>0.90:x = x*(0.90/top)
    else:
        x = balance_vocal(x, lead=False)
        top = float(np.max(np.abs(x))) if len(x) else 0.
        if top>0.78:x = x*(0.78/top)
    CHOPS.append((name, x, vol, pan)); CRATE.append(rate)
seg, f0 = tonal_source()
NOTES = 'C C# D D# E F F# G G# A A# B'.split()
for pc in range(12):
    midi = 60 + pc if pc < 7 else 48 + pc; target = 440.0 * 2 ** ((midi - 69) / 12.0)       # C4..F#4 and G3..B3: one octave of range around middle C
    # Tuning is bounded to one octave and the source's natural formant
    # movement is protected from repeated pitch-detection/resampling.
    y = resamp(seg, target / f0)
    y = stretch(y, 1.12 if len(y)<0.48*R else 1.0, 0.065)
    y = y[:int(0.52*R)]
    add('VOX ' + NOTES[pc], room(y, 0.09, 0.14), 66,
        48 + (pc * 5) % 32, 0.58)
# Keep the familiar 12 texture indices but turn novelty honks into
# actual production textures: breathy swells, double-tracked consonants,
# short gated replies, and restrained reverse tails.
add('LAUGH ROOM', room(load('laughing_2'), 0.11, 0.17), 65, 82)
add('YAHOO REVERSE SWELL', room(reverse(load('yahoo')), 0.09, 0.12), 58, 44)
add('YEHA BACKBEAT', stutter(load('yeha'), 0.105, 3), 61, 68)
add('SHRIEK DISTANT AIR', room(load('shriek')*0.6, 0.18, 0.12), 42, 64)
add('CRY HALFTIME', stretch(load('cry'), 1.25, 0.075), 58, 88)
add('FIGHT SYNCOPATION', gate(load('lets_fight'), 8, 0.75), 63, 51)
add('FLIRT WARM DOUBLE', room(load('flirt'), 0.065, 0.22), 62, 77)
add('NAH LOW RESPONSE', room(resamp(load('nah'), 0.9), 0.1, 0.12), 62, 64)
add('AGREE PLUCK', load('agree')[:int(0.29*R)], 64, 46)
add('SNEEZE AIRY FILL', room(load('sniiize_2')*0.65, 0.14, 0.12), 45, 86)
add('GHOST SOFT CHOIR', room(stretch(load('ghost_scared_female_1'), 1.4, 0.075), 0.12, 0.14), 58, 64)
add('WIN GENTLE CADENCE', room(load('win_the_fight')[:int(0.34*R)], 0.08, 0.12), 65, 85)
# ---- the songs' own voices (tools/extract_song_vox.py cut them out of the CORPORATIONS vocal stem and the Duck & Cover Koala project): each piece gets one of ten treatments
def tapestop(x, end=0.3):
    n = len(x); ratio = np.linspace(1.0, end, n); pos = np.cumsum(ratio); pos = pos[pos < n - 1]; return np.interp(pos, np.arange(n), x)
def treat(x, k):
    """Warm club-style vocal cuts: rhythmic, not a novelty SFX reel.
    No severe formant shifts, 4-bit crusher, ring modulation or squeak."""
    x = x / (np.abs(x).max() or 1)
    if k == 0: return room(x, 0.065, 0.13)          # dry upfront hook
    if k == 1: return room(stretch(x, 1.18, 0.065), 0.1, 0.17)
    if k == 2: return room(reverse(x), 0.075, 0.13)
    if k == 3: return stutter(x, 0.12, 3)           # coherent triplet
    if k == 4: return room(resamp(x, 0.93), 0.09, 0.11)
    if k == 5: return gate(x, 8, 0.8)               # rhythmic but legible
    if k == 6: return room(x, 0.13, 0.23)           # dub-space reply
    if k == 7: return stutter(x, 0.13, 2)           # two-beat pickup
    if k == 8: return room(reverse(x), 0.12, 0.18)
    return tapestop(x, 0.78)                        # tasteful turnaround
def song_pieces(folder, tag):
    import glob
    for i, f in enumerate(sorted(glob.glob(os.path.join(folder, '*.wav')))):
        w = wave.open(f); sr = w.getframerate(); x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
        g = gcd(R, sr); x = resample_poly(x, R // g, sr // g); x = sosfilt(butter(4, 3300, 'lp', fs=R, output='sos'), x)
        add('%s %02d %s' % (tag, i, ['DRY', 'WARM ECHO', 'REVERSE', 'TRIPLET', 'LOW DUB', 'GATED', 'ROOM', 'DOUBLE', 'REVERSED TAIL', 'TAPE TURN'][(i + (3 if tag == 'DUCK' else 0)) % 10]), treat(x, (i + (3 if tag == 'DUCK' else 0)) % 10), 96, 20 + (i * 37) % 90, 0.8)
song_pieces(os.path.join(HERE, '..', '..', 'tools', 'voices_corp'), 'CORP')
# ---- DUCK & COVER's lyrics: the ten vocal pads of the Koala project, each stretched (pitch kept) to a whole number of half beats of the new 88 bpm tempo, wrapped in a plate-like tail made of dense
# echoes, at 5.5 kHz. The song plays each one where the project played it (tools/duckcover/lyric_events.json); a second copy follows on the other side (see gmplay.h: the stereo echo).
FULL = bool(os.environ.get('GM_FULL'))      # GM_FULL=1: source/gmvox_full.h, the DSi-mode set: DS rates, ALL of CORPORATIONS' voice, the packed data an ordinary array in the ARM9 program (16 MB of RAM)
HQ = bool(os.environ.get('GM_HQ'))      # GM_HQ=1: the 3DS set (source/gmvox_hq.h): the lyrics at 16 kHz instead of 5.5 (the 3DS has the room and a mixer that interpolates)
LYR_PAD = []; LYR_RATE = 16000 if HQ else 5500; LYR_LP = 6500 if HQ else 2700; PLATE_LP = 5500 if HQ else 2400; NEWBPM = 88.0
def plate(x, rate, wet=1.0, tail=0.7):
    y = np.concatenate([x, np.zeros(int(tail * rate))]); out = y.copy()
    for d, a in ((0.019, 0.42), (0.031, 0.36), (0.047, 0.31), (0.071, 0.27), (0.103, 0.22), (0.151, 0.18), (0.233, 0.14), (0.347, 0.1), (0.5, 0.07)):
        n = int(d * rate); out[n:] += y[:len(y) - n] * a * wet
    return sosfilt(butter(2, min(PLATE_LP, 0.45 * rate), 'lp', fs=rate, output='sos'), out)
def lyric_chops():
    import glob
    for f in sorted(glob.glob(os.path.join(HERE, '..', '..', 'tools', 'duckcover', 'lyrics', 'pad_*.wav'))):
        pad = int(os.path.basename(f)[4:6]); w = wave.open(f); sr = w.getframerate(); x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
        x = sosfilt(butter(4, min(LYR_LP, 0.45 * sr), 'lp', fs=sr, output='sos'), x); dur = len(x) / sr; half = 30.0 / NEWBPM
        target = dur * 145.0 / NEWBPM; x = tune_stretch(x, sr, target / dur, pad, grain=0.055)      # (the project's bars are kept: its 145 bpm becomes 88, so a phrase lasts the same number of beats)
        g = gcd(LYR_RATE, sr); x = resample_poly(x, LYR_RATE // g, sr // g); x = plate(x, LYR_RATE, 0.42)
        LYR_PAD.append(pad); add('LYRIC PAD %d' % pad, x, 102, 64, 99.0, LYR_RATE)
lyric_chops()
# ---- CORPORATIONS' own voice, accurately: the 56 sung phrases of the song (tools/corporations/lyrics, cut from the isolated vocal stem by tools/extract_corp_lyrics.py) sounded where the song sang them,
# at the song's own tempo (131 bpm, the MIDI's grid: no stretching), on the song's own pitch (only the whole voice nudged to A = 440 when the stem is a few cents off), with a light plate tail.
# The remix feel comes from the manipulated chops of the same voice (CORP 00..13), the stereo placement and the ducking of the band under the line.
# Standard DS: adaptive 4.5–5.2 kHz lyric rate improves sibilants,
# short vowels and consonants without overflowing the 38-KiB streaming
# decode slot. Better use of memory than a fixed 4.5-kHz voice and a
# half-second mostly-silent reverb tail on EVERY one of 56 phrases.
CLYR_RATE = 12000 if HQ else 5200
CLYR_LP = 5200 if HQ else 2200
CLYR_TAIL = 0.7 if HQ else 0.33
CLYR = []
def corp_lyric_chops():
    import json
    ev = json.load(open(os.path.join(HERE, '..', '..', 'tools', 'corporations', 'lyric_events.json'))); cell_s = 60.0 / ev['bpm'] / 4; srcs = []
    for e in ev['events']:
        w = wave.open(os.path.join(HERE, '..', '..', 'tools', 'corporations', 'lyrics', 'corp_%02d.wav' % e['i'])); srcs.append((e, np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0, w.getframerate()))
    CORP_CENTS = -39.84; fix = 2 ** (CORP_CENTS / 1200.0)      # the recording is tuned to A = 430 Hz (notes.json): the voice is raised 40 cents to the MIDI's A = 440
    for e, x, sr in srcs:
        x = sosfilt(butter(4, min(CLYR_LP, 0.45 * sr), 'lp', fs=sr, output='sos'), x)
        cell = int(round(e['beat'] * 4)); frac = e['beat'] * 4 - cell      # the sixteenth it is placed on, and how far past it the phrase really starts (cells)
        sh = int(round(abs(frac) * cell_s * sr)); x = np.concatenate([np.zeros(sh), x]) if frac > 0 else x[sh:]
        x = resamp(x, 1.0 / fix)
        # Long sentences need slightly lower sample rates than short
        # syllables to fit 0x9800 PCM bytes. The pitch/timing is unchanged:
        # only each phrase's sample rate and PCM length differ together.
        rate = CLYR_RATE
        if not HQ:
            secs = len(x) / sr
            rate = min(rate, int((0x9800-256)/(secs+CLYR_TAIL+0.02)))
            if rate<4400:
                raise RuntimeError('DS recorded lyric is too long: %s needs %d Hz'
                                   % (e['i'],rate))
        g_ = gcd(rate,sr)
        x = resample_poly(x,rate//g_,sr//g_)
        x = plate(x,rate,0.27,tail=CLYR_TAIL)
        if not HQ and len(x)>0x9800:
            raise RuntimeError('DS lyric decode buffer exceeded: %s has %d samples'
                               % (e['i'],len(x)))
        CLYR.append(e)
        add('CORP LYRIC %02d' % e['i'],x,105,
            54 if e['i']%2==0 else 74,99.0,rate)
corp_lyric_chops()
LYR0 = next(i for i, c in enumerate(CHOPS) if c[0].startswith('LYRIC PAD')); CLYR0 = next(i for i, c in enumerate(CHOPS) if c[0].startswith('CORP LYRIC'))
# The first bank stays below the ARM7 voice decoding scratch buffers.
# Remaining complete phrases are loaded by the ARM9 at 0x02300000,
# reclaimed from save compression by ggsave_stream.h.
BLOB_MAX = 0x5B000 - 1024
HIGH_BLOB_MAX = 0x80000
STEP = [7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767]
IDX = [-1, -1, -1, -1, 2, 4, 6, 8]
def ima_encode(x16):      # 4-bit IMA ADPCM, two samples to a byte (low nibble first); the same coder the game's songs use
    pred, idx, out, nib = 0, 0, [], []
    for sm in x16:
        step = STEP[idx]; d = int(sm) - pred; n = 0
        if d < 0: n = 8; d = -d
        if d >= step: n |= 4; d -= step
        if d >= step >> 1: n |= 2; d -= step >> 1
        if d >= step >> 2: n |= 1
        diff = step >> 3
        if n & 1: diff += step >> 2
        if n & 2: diff += step >> 1
        if n & 4: diff += step
        pred += -diff if n & 8 else diff; pred = max(-32768, min(32767, pred)); idx = max(0, min(88, idx + IDX[n & 7])); nib.append(n)
    if len(nib) & 1: nib.append(0)
    return bytes(nib[i] | (nib[i + 1] << 4) for i in range(0, len(nib), 2))
def write_header(path, compact=False):
    L = ['#ifndef GMVOX_H', '#define GMVOX_H', '// gmvox.h - GENERATED by godguy/tools/gm_vox.py: the vocal chop kit, 4-bit IMA ADPCM, cut from the GBA game\'s voice clips, from the CORPORATIONS vocal stem and from the Duck & Cover Koala project, pushed hard. Do not edit.',
         '// A chop is unpacked into a RAM buffer when it is struck (gmVoxLoad). On the real DS the packed data lives in the ARM7 binary (hw/arm7vox.s, loaded at 0x02390000: uncached memory the ARM9 reads) so it',
         '// does not use the ARM9\'s RAM; everywhere else it is an ordinary array.']
    inc = [True] * len(CHOPS)
    # Compact bank remains for optional libnds/Calico ROMs, where the
    # standard linker cannot use the reclaimed ARM9 high-memory bank.
    # Hardware DS/DSi-compatible ROMs now store every recorded phrase.
    if compact:
        size=lambda x:(len(x)+1)//2+3
        base=sum(size(x) for i,(_,x,_,_) in enumerate(CHOPS)
                 if i<CLYR0)
        room=BLOB_MAX-3072-base-8*len(CHOPS)
        order=sorted(range(len(CLYR)),
                     key=lambda k:(-CLYR[k]['vol'],-CLYR[k]['dur']))
        inc2=[False]*len(CLYR)
        for k in order:
            sz=size(CHOPS[CLYR0+k][1])
            if sz<=room:inc2[k]=True;room-=sz
        for k in range(len(CLYR)):inc[CLYR0+k]=inc2[k]
        print('libnds compact fallback: %d/%d CORPORATIONS phrases'%
              (sum(inc2),len(CLYR)))
    CH=[(nm,x if inc[i] else x[:0],vol,pan)
        for i,(nm,x,vol,pan) in enumerate(CHOPS)]
    blob = bytearray(); offs = []; tot = 0; mx = 0
    for i, (nm, x, vol, pan) in enumerate(CH):
        pcm = np.clip(np.round(x * 32000), -32768, 32767).astype(int); pk = ima_encode(pcm); offs.append(len(blob)); blob += pk
        if len(blob) & 3: blob += bytes(4 - (len(blob) & 3))
        mx = max(mx, len(x))
    split=len(blob);hi=b''
    if not HQ and not FULL and not compact:
        # Split only BEFORE a whole phrase, never midway through ADPCM.
        for i,off in enumerate(offs):
            end=offs[i+1] if i+1<len(offs) else len(blob)
            if end>BLOB_MAX: split=off;break
        hi=bytes(blob[split:])
        if split>BLOB_MAX or len(hi)>HIGH_BLOB_MAX or not hi:
            raise RuntimeError('Normal DS dual bank overflow: '
                               '%d bytes ARM7 + %d bytes ARM9'%
                               (split,len(hi)))
        if any(not len(CH[CLYR0+k][1]) for k in range(len(CLYR))):
            raise RuntimeError('Missing normal DS vocal phrase')
        print('NORMAL DS: %d/%d CORPORATIONS lyrics + %d DUCK pads; '
              'ARM7 %d B, ARM9 %d B'%
              (len(CLYR),len(CLYR),len(LYR_PAD),split,len(hi)))
    suffix='_hq' if HQ else '_full' if FULL else '_compact' if compact else ''
    open(os.path.join(os.path.dirname(path),'gmvox'+suffix+'.bin'),'wb').write(bytes(blob[:split]))
    if not HQ and not FULL and not compact:
        open(os.path.join(os.path.dirname(path),'gmvox_hi.bin'),'wb').write(hi)
    L.append('#define GM_VOX_BLOB 0x02382000u')
    L.append('#define GM_VOX_HIBLOB 0x02300000u')
    L.append('#define GM_VOX_BLOBSIZE %d' % len(blob))
    L.append('#define GM_VOX_SPLIT %d' % split)
    L.append('#define GM_VOX_HIBYTES %d' % len(hi))
    L.append('#if defined(NDSHW) && %d' % (0 if FULL or HQ or compact else 1))
    L.append('#define GM_VOX_PK(o) ((const unsigned char*)(((o)<GM_VOX_SPLIT)?(GM_VOX_BLOB+(o)):(GM_VOX_HIBLOB+(o)-GM_VOX_SPLIT)))')
    L.append('#else')
    L.append('static const unsigned char gmVoxBlob[%d]={%s};' % (len(blob), ','.join(map(str, blob)))); L.append('#define GM_VOX_PK(o) (gmVoxBlob+(o))'); L.append('#endif')
    L.append('static const unsigned int gmVoxOff[%d]={%s};' % (len(CHOPS), ','.join(map(str, offs))))
    L.append('static const %s gmVoxLen[%d]={%s}; static const unsigned short gmVoxRate[%d]={%s};' % ('unsigned int' if HQ else 'unsigned short', len(CHOPS), ','.join(str(len(x)) for _, x, _, _ in CH), len(CHOPS), ','.join(str(r) for r in CRATE)))
    L.append('static const unsigned char gmVoxVol[%d]={%s}, gmVoxPan[%d]={%s};' % (len(CHOPS), ','.join(str(v) for _, _, v, _ in CHOPS), len(CHOPS), ','.join(str(p) for _, _, _, p in CHOPS)))
    L.append('#define GM_VOX_N %d' % len(CHOPS)); L.append('#define GM_VOX0 16      // vocal chop id in the drum-B stream = GM_VOX0 + index'); L.append('#define GM_VOX_MAX %d' % mx)
    L.append('#define GM_VOX_LYR0 %d      // the first of the Duck & Cover lyric phrases (a lyric id with the soft bit set is its stereo echo copy)' % LYR0); L.append('#define GM_VOX_LYRN %d' % len(LYR_PAD)); L.append('#define GM_VOX_CLYR0 %d      // the first of the CORPORATIONS phrases' % CLYR0); L.append('#define GM_VOX_CLYRN %d' % len(CLYR)); L.append('#define GM_VOXE0 (GM_VOX0+GM_VOX_N)')
    L.append('static const unsigned char gmVoxLyrPad[%d]={%s};' % (len(LYR_PAD), ','.join(map(str, LYR_PAD))))
    L.append('static const char*const gmVoxName[%d]={%s};' % (len(CHOPS), ','.join('"%s"' % n for n, _, _, _ in CHOPS)))
    if not HQ:
        L.append('#ifdef NDSHW'); L.append('#define GM_VOXBUF(b) ((signed char*)(0x023DD000+(b)*0x9800))      /* the real DS: two 38 KB buffers in the uncached block (the ARM7 reads them; ARM9 writes would sit in its data cache) */')
        L.append('#else'); L.append('static signed char gmVoxBuf[2][GM_VOX_MAX] __attribute__((aligned(4)));'); L.append('#define GM_VOXBUF(b) (gmVoxBuf[b])'); L.append('#endif'); L.append('static GmDrum gmVoxDr[2];')
        L.append("static const GmDrum*gmVoxLoad(int idx,int slot){ static const short st[89]={%s}; static const signed char ix[8]={-1,-1,-1,-1,2,4,6,8};" % ','.join(map(str, STEP)))
        L.append("    int b=slot&1, n=gmVoxLen[idx], pred=0, si=0; const unsigned char*p=GM_VOX_PK(gmVoxOff[idx]); signed char*o=GM_VOXBUF(b);")
        L.append("    for(int i=0;i<n;i++){ int c=(p[i>>1]>>((i&1)*4))&15, step=st[si], d=step>>3; if(c&1) d+=step>>2; if(c&2) d+=step>>1; if(c&4) d+=step; pred+=(c&8)?-d:d; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768; si+=ix[c&7]; if(si<0) si=0; if(si>88) si=88; o[i]=(signed char)(pred>>8); }")
        L.append("    gmVoxDr[b].d=o; gmVoxDr[b].n=n; gmVoxDr[b].rate=gmVoxRate[idx]; return &gmVoxDr[b]; }")
    if HQ:      # the 3DS: every chop is unpacked ONCE, at start-up (gmVoxWarm), into one pool: a strike then costs nothing (no decode hitch in a frame) and no two phrases can share a buffer
        L.append('static signed char gmVoxPool[%d]; static GmDrum gmVoxDr[%d]; static int gmVoxWarmed;' % (sum(len(x) for _, x, _, _ in CH), len(CHOPS)))
        L.append("static void gmVoxWarm(void){ static const short st[89]={%s}; static const signed char ix[8]={-1,-1,-1,-1,2,4,6,8}; signed char*o=gmVoxPool; if(gmVoxWarmed) return; gmVoxWarmed=1;" % ','.join(map(str, STEP)))
        L.append("    for(int k=0;k<GM_VOX_N;k++){ int n=gmVoxLen[k], pred=0, si=0; const unsigned char*p=GM_VOX_PK(gmVoxOff[k]);")
        L.append("        for(int i=0;i<n;i++){ int c=(p[i>>1]>>((i&1)*4))&15, step=st[si], d=step>>3; if(c&1) d+=step>>2; if(c&2) d+=step>>1; if(c&4) d+=step; pred+=(c&8)?-d:d; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768; si+=ix[c&7]; if(si<0) si=0; if(si>88) si=88; o[i]=(signed char)(pred>>8); }")
        L.append("        gmVoxDr[k].d=o; gmVoxDr[k].n=n; gmVoxDr[k].rate=gmVoxRate[k]; o+=n; } }")
        L.append("static const GmDrum*gmVoxLoad(int idx,int slot){ (void)slot; gmVoxWarm(); return &gmVoxDr[idx]; }")
    L.append('#endif')
    open(path, 'w').write('\n'.join(L) + '\n'); return len(blob)
if __name__ == '__main__':
    tot=write_header(os.path.join(HERE,'..','source',
                     'gmvox_hq.h' if HQ else 'gmvox_full.h' if FULL else 'gmvox.h'))
    if not HQ and not FULL:
        write_header(os.path.join(HERE,'..','source','gmvox_compact.h'),compact=True)
    print('%d chops, %d total bytes, max %d samples, source f0 %.1f Hz'%
          (len(CHOPS),tot,max(len(x) for _,x,_,_ in CHOPS),f0))
    if len(sys.argv) > 1:
        os.makedirs(sys.argv[1], exist_ok=True)
        for nm, x, v, p in CHOPS:
            w = wave.open(os.path.join(sys.argv[1], nm.replace(' ', '_').replace('#', 's') + '.wav'), 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(R); w.writeframes((np.clip(x, -1, 1) * 32000).astype('<i2').tobytes()); w.close()
