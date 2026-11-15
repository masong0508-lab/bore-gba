# Builds source/gmvox.h: the VOCAL CHOP kit of the music. 24 one-shots (signed 8 bit, 8 kHz) cut from the GBA game's voice clips (tools/voices_src) and pushed hard:
# 12 TONAL chops (the sung "serenade" clip, one steady vowel found in it, retuned to each of the twelve notes: a vocal pad you can play in any key) and 12 TEXTURE chops
# (stretched into clouds, reversed, stuttered, bit-crushed, shuffled, ring-modulated, pitched into squeaks and growls). The music player fires them from the drum-B stream (ids 16 and up).
import json, numpy as np, wave, os, sys
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
    p90 = float(np.percentile(sm[v], 90))
    if top is not None and abs(top - p90) <= 3.5: off = top - p90; cost = 0.0      # the hook: its highest note is the song's highest note (F#4 in "from the lover that you have loved")
    else:
        best = (1e9, 0.0)
        for o in np.arange(-2.5, 2.51, 0.1):
            c = np.mean([abs(snap_target(p + o, pcw) - (p + o)) for p in sm[v]]) + 0.03 * abs(o)
            if c < best[0]: best = (c, o)
        cost, off = best
    tgt = np.array([snap_target(p + off, pcw) for p in sm])
    if top is not None and cost == 0.0: tgt = np.minimum(tgt, top)      # (nothing sung above the hook's top note)
    corr = np.clip(tgt - mm, -3.0, 3.0)
    k = 2; corr = np.convolve(np.pad(corr, k, mode='edge'), np.ones(2 * k + 1) / (2 * k + 1), 'valid')
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
def add(name, x, vol=100, pan=64, maxs=0.9, rate=R):
    x = norm(fade(x[:int(maxs * rate)]), 0.92); CHOPS.append((name, x, vol, pan)); CRATE.append(rate)
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
# ---- the songs' own voices (tools/extract_song_vox.py cut them out of the CORPORATIONS vocal stem and the Duck & Cover Koala project): each piece gets one of ten treatments
def tapestop(x, end=0.3):
    n = len(x); ratio = np.linspace(1.0, end, n); pos = np.cumsum(ratio); pos = pos[pos < n - 1]; return np.interp(pos, np.arange(n), x)
def treat(x, k):
    x = x / (np.abs(x).max() or 1)
    if k == 0: return room(x, 0.1, 0.4)
    if k == 1: return room(stretch(x, 2.6, 0.07, 0.3), 0.14, 0.5)
    if k == 2: return room(reverse(x), 0.08, 0.4)
    if k == 3: return stutter(x, 0.11, 4, 1.18)
    if k == 4: return crush(resamp(x, 0.67), 5, 2)
    if k == 5: return gate(resamp(x, 2.0), 14, 0.5)
    if k == 6: return shuffle(x, 6)
    if k == 7: return stutter(ring(x, 140), 0.12, 3)
    if k == 8: return room(reverse(stretch(x, 3.5, 0.09, 0.4)), 0.16, 0.55)
    return tapestop(x, 0.28)
def song_pieces(folder, tag):
    import glob
    for i, f in enumerate(sorted(glob.glob(os.path.join(folder, '*.wav')))):
        w = wave.open(f); sr = w.getframerate(); x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
        g = gcd(R, sr); x = resample_poly(x, R // g, sr // g); x = sosfilt(butter(4, 3300, 'lp', fs=R, output='sos'), x)
        add('%s %02d %s' % (tag, i, ['RAW', 'CLOUD', 'BACKWARDS', 'STUTTER', 'LOW CRUSH', 'SQUEAK GATE', 'SHUFFLE', 'ROBOT', 'GHOST', 'TAPE STOP'][(i + (3 if tag == 'DUCK' else 0)) % 10]), treat(x, (i + (3 if tag == 'DUCK' else 0)) % 10), 96, 20 + (i * 37) % 90, 0.8)
song_pieces(os.path.join(HERE, '..', '..', 'tools', 'voices_corp'), 'CORP')
# ---- DUCK & COVER's lyrics: the ten vocal pads of the Koala project, each stretched (pitch kept) to a whole number of half beats of the new 88 bpm tempo, wrapped in a plate-like tail made of dense
# echoes, at 5.5 kHz. The song plays each one where the project played it (tools/duckcover/lyric_events.json); a second copy follows on the other side (see gmplay.h: the stereo echo).
HQ = bool(os.environ.get('GM_HQ'))      # GM_HQ=1: the 3DS set (source/gmvox_hq.h): the lyrics at 16 kHz instead of 5.5 (the 3DS has the room and a mixer that interpolates)
LYR_PAD = []; LYR_RATE = 16000 if HQ else 5500; LYR_LP = 6500 if HQ else 2700; PLATE_LP = 5500 if HQ else 2400; NEWBPM = 88.0
def plate(x, rate):
    y = np.concatenate([x, np.zeros(int(0.7 * rate))]); out = y.copy()
    for d, a in ((0.019, 0.42), (0.031, 0.36), (0.047, 0.31), (0.071, 0.27), (0.103, 0.22), (0.151, 0.18), (0.233, 0.14), (0.347, 0.1), (0.5, 0.07)):
        n = int(d * rate); out[n:] += y[:len(y) - n] * a
    return sosfilt(butter(2, PLATE_LP, 'lp', fs=rate, output='sos'), out)
def lyric_chops():
    import glob
    for f in sorted(glob.glob(os.path.join(HERE, '..', '..', 'tools', 'duckcover', 'lyrics', 'pad_*.wav'))):
        pad = int(os.path.basename(f)[4:6]); w = wave.open(f); sr = w.getframerate(); x = np.frombuffer(w.readframes(w.getnframes()), dtype='<i2').astype(np.float64) / 32768.0
        x = sosfilt(butter(4, min(LYR_LP, 0.45 * sr), 'lp', fs=sr, output='sos'), x); dur = len(x) / sr; half = 30.0 / NEWBPM
        target = dur * 145.0 / NEWBPM; x = tune_stretch(x, sr, target / dur, pad)      # (the project's bars are kept: its 145 bpm becomes 88, so a phrase lasts the same number of beats)
        g = gcd(LYR_RATE, sr); x = resample_poly(x, LYR_RATE // g, sr // g); x = plate(x, LYR_RATE)
        LYR_PAD.append(pad); add('LYRIC PAD %d' % pad, x, 112, 36 if pad < 70 else 64, 99.0, LYR_RATE)
lyric_chops()
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
def write_header(path):
    L = ['#ifndef GMVOX_H', '#define GMVOX_H', '// gmvox.h - GENERATED by godguy/tools/gm_vox.py: the vocal chop kit, 4-bit IMA ADPCM, cut from the GBA game\'s voice clips, from the CORPORATIONS vocal stem and from the Duck & Cover Koala project, pushed hard. Do not edit.',
         '// A chop is unpacked into a RAM buffer when it is struck (gmVoxLoad). On the real DS the packed data lives in the ARM7 binary (hw/arm7vox.s, loaded at 0x02390000: uncached memory the ARM9 reads) so it',
         '// does not use the ARM9\'s RAM; everywhere else it is an ordinary array.']
    blob = bytearray(); offs = []; tot = 0; mx = 0
    for i, (nm, x, vol, pan) in enumerate(CHOPS):
        pcm = np.clip(np.round(x * 32000), -32768, 32767).astype(int); pk = ima_encode(pcm); offs.append(len(blob)); blob += pk
        if len(blob) & 3: blob += bytes(4 - (len(blob) & 3))
        mx = max(mx, len(x))
    open(os.path.join(os.path.dirname(path), 'gmvox_hq.bin' if HQ else 'gmvox.bin'), 'wb').write(bytes(blob))
    L.append('#define GM_VOX_BLOB 0x02390000u'); L.append('#define GM_VOX_BLOBSIZE %d' % len(blob)); L.append('#ifdef NDSHW'); L.append('#define GM_VOX_PK(o) ((const unsigned char*)(GM_VOX_BLOB+(o)))'); L.append('#else')
    L.append('static const unsigned char gmVoxBlob[%d]={%s};' % (len(blob), ','.join(map(str, blob)))); L.append('#define GM_VOX_PK(o) (gmVoxBlob+(o))'); L.append('#endif')
    L.append('static const unsigned int gmVoxOff[%d]={%s};' % (len(CHOPS), ','.join(map(str, offs))))
    L.append('static const %s gmVoxLen[%d]={%s}; static const unsigned short gmVoxRate[%d]={%s};' % ('unsigned int' if HQ else 'unsigned short', len(CHOPS), ','.join(str(len(x)) for _, x, _, _ in CHOPS), len(CHOPS), ','.join(str(r) for r in CRATE)))
    L.append('static const unsigned char gmVoxVol[%d]={%s}, gmVoxPan[%d]={%s};' % (len(CHOPS), ','.join(str(v) for _, _, v, _ in CHOPS), len(CHOPS), ','.join(str(p) for _, _, _, p in CHOPS)))
    L.append('#define GM_VOX_N %d' % len(CHOPS)); L.append('#define GM_VOX0 16      // vocal chop id in the drum-B stream = GM_VOX0 + index'); L.append('#define GM_VOX_MAX %d' % mx)
    L.append('#define GM_VOX_LYR0 %d      // the first of the Duck & Cover lyric phrases; GM_VOXE0 + 2 * (n) + side is the stereo echo copy of lyric n' % (len(CHOPS) - len(LYR_PAD))); L.append('#define GM_VOX_LYRN %d' % len(LYR_PAD)); L.append('#define GM_VOXE0 (GM_VOX0+GM_VOX_N)')
    L.append('static const unsigned char gmVoxLyrPad[%d]={%s};' % (len(LYR_PAD), ','.join(map(str, LYR_PAD))))
    L.append('static const char*const gmVoxName[%d]={%s};' % (len(CHOPS), ','.join('"%s"' % n for n, _, _, _ in CHOPS)))
    L.append('#ifdef NDSHW'); L.append('#define GM_VOXBUF(b) ((signed char*)(0x023DD000+(b)*0x9800))      /* the real DS: two 38 KB buffers in the uncached block (the ARM7 reads them; ARM9 writes would sit in its data cache) */')
    L.append('#else'); L.append('static signed char gmVoxBuf[2][GM_VOX_MAX] __attribute__((aligned(4)));'); L.append('#define GM_VOXBUF(b) (gmVoxBuf[b])'); L.append('#endif'); L.append('static GmDrum gmVoxDr[2];')
    L.append("static const GmDrum*gmVoxLoad(int idx,int slot){ static const short st[89]={%s}; static const signed char ix[8]={-1,-1,-1,-1,2,4,6,8};" % ','.join(map(str, STEP)))
    L.append("    int b=slot&1, n=gmVoxLen[idx], pred=0, si=0; const unsigned char*p=GM_VOX_PK(gmVoxOff[idx]); signed char*o=GM_VOXBUF(b);")
    L.append("    for(int i=0;i<n;i++){ int c=(p[i>>1]>>((i&1)*4))&15, step=st[si], d=step>>3; if(c&1) d+=step>>2; if(c&2) d+=step>>1; if(c&4) d+=step; pred+=(c&8)?-d:d; if(pred>32767) pred=32767; if(pred<-32768) pred=-32768; si+=ix[c&7]; if(si<0) si=0; if(si>88) si=88; o[i]=(signed char)(pred>>8); }")
    L.append("    gmVoxDr[b].d=o; gmVoxDr[b].n=n; gmVoxDr[b].rate=gmVoxRate[idx]; return &gmVoxDr[b]; }")
    L.append('#endif')
    open(path, 'w').write('\n'.join(L) + '\n'); return len(blob)
if __name__ == '__main__':
    tot = write_header(os.path.join(HERE, '..', 'source', 'gmvox_hq.h' if HQ else 'gmvox.h')); print('%d chops, %d bytes of packed data, max %d samples, tonal source f0 %.1f Hz' % (len(CHOPS), tot, max(len(x) for _, x, _, _ in CHOPS), f0))
    if len(sys.argv) > 1:
        os.makedirs(sys.argv[1], exist_ok=True)
        for nm, x, v, p in CHOPS:
            w = wave.open(os.path.join(sys.argv[1], nm.replace(' ', '_').replace('#', 's') + '.wav'), 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(R); w.writeframes((np.clip(x, -1, 1) * 32000).astype('<i2').tobytes()); w.close()
