#!/usr/bin/env python3
"""STUDIO RENDERS: every tracker song in the game rendered for a soundtrack release (48 kHz, 24-bit), mixed and mastered.

usage:  python3 tools/studio_render.py OUTDIR [song_id ...]      (run from the project root; needs numpy + scipy + ffmpeg; slow)

Same notes, same arrangement, same pan choreography as the game (it reads the songs through tools/xm2gba.py, so the reworked arrangements
the converter makes - the ambient TREE-AGE, Worthless Clouds, the Amiga ending... - are the ones rendered).  What changes is the sound:
  - SOUNDS: a song built by a generator script (tools/make_*.py) has every instrument synthesised again at 4x the rate (BORE_HIFI=4), so
    nothing is cut at the game's 9 kHz and nothing is 8-bit.  Songs built on recorded samples play those samples as they are (no 8-bit
    re-quantising, no down-sampling).  Every note is resampled with a long windowed-sinc filter (no aliasing, no zipper).
  - MIX: each instrument goes to a bus by its role (kick, bass, drums, pads, melodic): low cut where it has no business, kick-keyed
    ducking of the bass and pads on four-on-the-floor songs, pads widened, kick and bass kept mono, one stereo hall for everything with
    a send level per role.  Panning is continuous (the game has 7 positions): the hand-made choreography of each song in xm2gba.py is
    followed exactly where it exists, the rest is placed by a continuous version of the converter's plan.
  - MASTER: glue compression, a gentle tilt towards a common tonal balance, mono below 120 Hz, -14 LUFS integrated, -1 dBTP true peak.
    A song that loops in the game (no written ending) plays once through, goes round again and fades out.
"""
import os, sys, re, io, json, math, subprocess, contextlib, fractions, importlib
import numpy as np
from scipy import signal
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)

FS = 48000          # output rate
HIFI = 4            # generator re-synthesis rate factor
GEN = {             # song id -> the generator script that built it (its sounds are synthesised again at HIFI x the rate)
    'aim_and_shoot': 'make_aimandshoot_rework', 'gottcho_barracho_ii': 'make_barracho_rework', 'closer_to_the_end_old': 'make_closer_old_rework',
    'closer_to_the_end': 'make_closer_rework', 'cocaine_cola_ii': 'make_cocaine_cola_rework', 'condensed_music': 'make_condensed_rework',
    'cynicaller_dnb': 'make_cynicaller_rework', 'emergency_hitech': 'make_emergency_rework', 'excuses_house': 'make_excuses_rework',
    'spanish_flexicode': 'make_flexicode_rework', 'hotdamn_rave': 'make_hotdamn_rework', 'magic_act': 'make_magicact_rework',
    'nursery_time': 'make_nursery_rework', 'staged': 'make_staged_rework', 'sunman_sunrise': 'make_sunman_rework',
    'the_ticking_bomb': 'make_tickingbomb_rework', 'whistler_shuffle': 'make_whistler_rework', 'meltdown_in_mars_house': 'make_meltdown_house',
    # the first versions of two reworks.  WHISTLER MAN (ORIGINAL) is the generator as it was at commit 55e5e50 (it rebuilds that .xm byte for
    # byte); HOT DAMN (ORIGINAL)'s sounds are all still the current generator's, which is checked sound by sound (see sounds())
    'hotdamn_rave_old': 'make_hotdamn_rework', 'whistler_shuffle_old': 'make_whistler_rework@55e5e50',
}

# ================================================================ the generators' sounds at HIFI x the rate (one process per generator)
def dump_bank(modname, out):
    """(runs in its own process, BORE_HIFI set) import the generator - its module code synthesises every instrument - and save them"""
    if os.environ.get('BORE_GENDIR'): sys.path.insert(0, os.environ['BORE_GENDIR'])          # an older version of a generator, from git
    mod = importlib.import_module(modname.split('@')[0])
    I = mod.I; keys = getattr(mod, 'ORDER_KEYS', None) or mod.KEYS
    arrs = {}; meta = []
    for k in keys:
        d = I[k]; x = np.asarray(d['x'], np.float64)
        rate = d['sr'] if 'sr' in d else 8363 * 2 ** (d['rel'] / 12) * HIFI
        arrs['x%d' % len(meta)] = x; meta.append(dict(key=k, name=d['name'], rate=float(rate), rel=int(d.get('rel', 0))))
    np.savez(out, meta=json.dumps(meta), **arrs)

def bank(modname, cache):
    out = os.path.join(cache, modname.replace('@', '_at_') + '.npz')
    if not os.path.exists(out):
        env = dict(os.environ, BORE_HIFI=str(HIFI))
        if '@' in modname:                                                   # 'script@commit': that version of the script, run beside the song files
            name, rev = modname.split('@'); d = os.path.join(cache, 'gen_' + rev); os.makedirs(d, exist_ok=True)
            open(os.path.join(d, name + '.py'), 'w').write(subprocess.run(['git', 'show', '%s:tools/%s.py' % (rev, name)], capture_output=True, text=True, check=True).stdout)
            for f in os.listdir(HERE):
                if f.endswith(('.xm', '.caustic')) and not os.path.exists(os.path.join(d, f)): os.symlink(os.path.join(HERE, f), os.path.join(d, f))
            env['BORE_GENDIR'] = d
        r = subprocess.run([sys.executable, os.path.abspath(__file__), '--bank', modname, out], env=env, capture_output=True, text=True)
        if r.returncode: sys.exit("bank %s failed:\n%s" % (modname, r.stderr[-3000:]))
    z = np.load(out); meta = json.loads(str(z['meta']))
    return [dict(m, x=z['x%d' % j]) for j, m in enumerate(meta)]

def env_of(x, rate, win=0.005):
    """RMS envelope in `win` s frames, for comparing two versions of a sound that differ in rate (and in noise realisation)"""
    w = max(1, int(win * rate)); n = len(x) // w
    return np.sqrt((np.asarray(x[:n * w], float).reshape(n, w) ** 2).mean(1) + 1e-12)

def match(lo, rate_lo, hi, rate_hi):
    """how well a hi-fi sound stands in for the game's: correlation of the loudness envelopes and of the band-limited spectra (0..1)"""
    a = env_of(lo, rate_lo); b = env_of(hi, rate_hi); n = min(len(a), len(b))
    if n < 4: return 0.0
    if abs(len(a) - len(b)) > 0.05 * max(len(a), len(b)) + 3: return 0.0
    ce = np.corrcoef(a[:n], b[:n])[0, 1] if a[:n].std() > 1e-9 and b[:n].std() > 1e-9 else 1.0
    nf = 4096; top = rate_lo * 0.45
    def spec(x, rate):
        f, p = signal.welch(np.asarray(x, float), rate, nperseg=min(len(x), nf)); m = f < top
        bands = np.geomspace(60, top, 24); return np.array([p[(f >= b0) & (f < b1)].mean() if ((f >= b0) & (f < b1)).any() else 0 for b0, b1 in zip(bands, bands[1:])])
    sa = 10 * np.log10(spec(lo, rate_lo) + 1e-12); sb = 10 * np.log10(spec(hi, rate_hi) + 1e-12)
    cs = np.corrcoef(sa, sb)[0, 1] if sa.std() > 1e-6 and sb.std() > 1e-6 else 1.0
    return float(min(ce, cs))


# ================================================================ a song, read the way the game reads it
import xm2gba as X
from xm import parse

def quiet(f, *a, **k):
    with contextlib.redirect_stdout(io.StringIO()): return f(*a, **k)

def _continuous_design_pan():
    """xm2gba.design_pan with the 7-bus rounding taken out: the same plan (classes, fans, ping-pong, mirroring, the hand-made
    choreography), returning the position itself (-0.9 .. 0.9)"""
    import inspect
    src = inspect.getsource(X.design_pan)
    for a, b in [("if flip.get(i, 1) < 0: b = 6 - b", "if flip.get(i, 1) < 0: b = -b"),
                 ("return max(0, min(6, b + shift.get(i, 0)))", "return b"),
                 ("        if not d: return 3\n", "        if not d: return 0.0\n"),
                 ("if q is not None: return int(round(max(-0.9, min(0.9, q)) * 3 / 0.9)) + 3", "if q is not None: return max(-0.9, min(0.9, q))"),
                 ("        return int(round(p * 3 / 0.9)) + 3\n", "        return p\n"),
                 ("ents[i].append((e, pan0(o_, r_, ch, i, n)))", "ents[i].append((e, int(round(pan0(o_, r_, ch, i, n) * 3 / 0.9)) + 3))")]:
        assert src.count(a) == 1, a
        src = src.replace(a, b)
    ns = dict(vars(X)); exec(src.replace('def design_pan(', 'def design_pan_c('), ns); return ns['design_pan_c']
design_pan_c = _continuous_design_pan()

def read_song(sid, path):
    S = parse(path)
    for tab in (X.DANCES, X.TREES, X.POPS, X.CLOUDS, X.ENDINGS):
        if sid in tab: quiet(tab[sid], S)
    used = set(i for o in S['order'] for r in S['pats'][o] for (n, i, v, e, ep) in r if n and n < 97)
    lo = quiet(X.convert_samples, S, used, sid)                      # the game's samples (the pan plan is made from these, as in the game)
    panf = quiet(design_pan_c, S, used, lo, sid)
    pans = {}                                                        # (pattern, row, ch) -> position; asked in the converter's order (ping-pong)
    for pi, p in enumerate(S['pats']):
        for ri, r in enumerate(p):
            for ch, (n, i, v, _, _) in enumerate(r):
                if n and n < 97 and i - 1 < len(lo) and lo[i - 1]: pans[(pi, ri, ch)] = panf(pi, ri, ch, i, n)
    return S, used, lo, panf, pans

# ================================================================ the sounds of a song: hi-fi where a generator made them, else the recorded ones
UPGRADES = os.path.join(HERE, 'sample_upgrades.json')   # better copies of recorded sounds, found by tools/sample_match.py
def sounds(sid, S, used, cache, path=None):
    """instrument number -> dict(x = float samples, rate = rate at C-4 (note 49), src = 'hifi' / 'sample' / 'archive', name)"""
    out = {}; B = bank(GEN[sid], cache) if sid in GEN else []
    try: up = json.load(open(UPGRADES)).get(os.path.basename(path), {}) if path else {}
    except OSError: up = {}
    byname = {}
    for b in B: byname.setdefault(b['name'], b)
    cur = None
    if sid.endswith('_old') and sid[:-4] in dict(X.song_list()) and GEN.get(sid) == GEN.get(sid[:-4]):   # an old version made with the NEW
        # song's generator: only the sounds that are still identical to the new song's
        cur = {ins['name']: ins['samples'][0]['data'] for ins in parse(dict(X.song_list())[sid[:-4]])['insts'] if ins['samples']}
    for i in sorted(used):
        ins = S['insts'][i - 1]
        if not ins['samples']: continue
        s = ins['samples'][0]; x = np.array(s['data'], float); rate = 8363 * 2 ** ((s['rel'] + s['fine'] / 128) / 12)
        b = None
        if len(B) == len(S['insts']) and B[i - 1]['name'] == ins['name']: b = B[i - 1]
        elif ins['name'] in byname: b = byname[ins['name']]
        if b is not None and cur is not None and cur.get(ins['name']) != s['data']: b = None
        if b is not None: out[i] = dict(x=np.asarray(b['x'], float), rate=b['rate'], src='hifi', name=ins['name'], vol=s['vol'])
        else: out[i] = dict(x=x, rate=rate, src='sample', name=ins['name'], vol=s['vol'])
        if out[i]['src'] == 'sample' and str(i) in up:                  # a better copy of this recording in another module: it takes over, lined
            u = up[str(i)]; import sample_match as SM                      # up to the sample and level-matched (sample_match.py worked both out)
            y = np.asarray(SM.read_any(open(u['source'], 'rb').read())[u['sample']]['x'], float); k = int(round(u['start']))
            y = y[k:] if k >= 0 else np.concatenate([np.zeros(-k), y])
            out[i] = dict(x=y * u['gain'], rate=float(u['rate']), src='archive', name=ins['name'], vol=s['vol'])
        if sid in X.FITBAR and i in X.FITBAR[sid][2]:                # Amiga Music's one-bar chord loops, fitted to the song's bar as the game does
            src_bpm, rows, _ = X.FITBAR[sid]
            nts = [n for o in S['order'] for r in S['pats'][o] for (n, ii, v, e, ep) in r if ii == i and 0 < n < 97]
            d = out[i]; fcp = d['rate'] * 2 ** ((int(np.median(nts)) - 49) / 12) if nts else d['rate']
            d['x'] = quiet(X.fit_bar, d['x'], rows * S['tempo'] * 2.5 / src_bpm, rows * S['tempo'] * 2.5 / S['bpm'], fcp)
    return out

# ================================================================ roles: which bus an instrument goes to
ROLE_WORDS = [('boom', r'drop|impact|boom|sub ?hit'), ('kick', r'kick|bass ?drum|\bbd\b'),
              ('perc', r'tom|conga|bongo|timbale|darbuka|\bdum\b|frame|\bdaf\b|wood|clave|perc|tabla|djembe|log'),
              ('snare', r'snare|clap|rim|ghost|crack|\btek\b|chalk|knock|snap'),
              ('hat', r'hat|shaker|ride|crash|cymbal|\bcym|tamb|cowbell|\briq\b|tick|bell tree|chime'),
              ('fx', r'swell|riser|reverse|zap|noise|sweep|whoosh|swoosh|glitch|scratch'),
              ('bass', r'bass|\bsub\b|reese|808|bubble'),
              ('pad', r'pad|string|choir|\booh\b|\bahh?\b|\boo\b|organ|hammond|mellotron|\bair\b|texture|tanpura|drone|ondes|breath|wash')]
ROLES = {   # hpf (Hz), reverb send, kick ducking (dB), widening, pan scale
    'kick':  dict(hpf=24, send=.035, duck=0, widen=0, ps=.5),
    'boom':  dict(hpf=22, send=.08, duck=0, widen=0, ps=.6),
    'bass':  dict(hpf=30, send=.025, duck=3.5, widen=0, ps=.6),
    'perc':  dict(hpf=50, send=.15, duck=0, widen=0, ps=1.0),
    'snare': dict(hpf=95, send=.19, duck=0, widen=0, ps=.85),
    'hat':   dict(hpf=300, send=.09, duck=0, widen=0, ps=.8),
    'fx':    dict(hpf=70, send=.22, duck=0, widen=.25, ps=1.0),
    'pad':   dict(hpf=140, send=.28, duck=1.5, widen=.35, ps=1.0),
    'mel':   dict(hpf=95, send=.16, duck=0, widen=0, ps=1.0),
}
def role_of(name, cls, dur, named):
    """named = the instrument has a real name (the generators name every sound); unnamed ones ('Sound 3') go by the converter's class"""
    nm = name.lower()
    for r, w in ROLE_WORDS:
        if re.search(w, nm): return r
    if named: return 'mel'
    if cls == 'low': return 'kick' if dur < 0.6 else 'bass'
    return {'pad': 'pad', 'hat': 'hat', 'snare': 'snare'}.get(cls, 'mel')

# ================================================================ rendering the notes
def to_rate(x, ratio):
    """resample x by `ratio` (output samples per input sample) with a long Kaiser-windowed sinc: band-limited, no aliasing"""
    fr = fractions.Fraction(ratio).limit_denominator(2000)
    if fr == 1: return np.asarray(x, np.float64)
    return signal.resample_poly(np.asarray(x, np.float64), fr.numerator, fr.denominator, window=('kaiser', 8.0))

def low_edge(x, rate):
    """the frequency below which 2 % of a sound's power lies (to keep a low cut under the sound's own bottom)"""
    x = np.asarray(x, float)[:int(rate * 2)]
    if len(x) < 64: return 1000.0
    f, pw = signal.welch(x, rate, nperseg=min(len(x), 8192)); c = np.cumsum(pw); c /= c[-1] + 1e-30
    return float(f[min(len(f) - 1, np.searchsorted(c, 0.02))]) or 20.0

def song_plan(S, used, lo, panf, pans, snd, sid):
    """every note of the song in play order: (time s, channel, instrument, note, amplitude, pan); plus how the song ends"""
    rowsec = S['tempo'] * 2.5 / S['bpm']; nord = len(S['order'])
    loop = X.LOOP_OVERRIDE.get(sid, S['restart']); loop = 0 if loop >= nord else loop
    def pass_events(orders, t0):
        ev = []; t = t0
        for o in orders:
            for ri, r in enumerate(S['pats'][o]):
                for ch, (n, i, v, _, _) in enumerate(r):
                    if n and n < 97 and i in snd and (o, ri, ch) in pans:
                        rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
                        ev.append((t, ch, i, n, snd[i]['vol'] / 64 * rel, pans[(o, ri, ch)]))
                t += rowsec
        return ev, t
    ev, t_end = pass_events(S['order'], 0.0)
    last = max((e[0] for e in ev), default=0.0)
    return dict(ev=ev, t_end=t_end, last=last, rowsec=rowsec, loop=loop, again=lambda t0: pass_events(S['order'][loop:] * 3, t0))

def make_ir(seed=7, length=3.4, pre=0.021):
    """a stereo hall: decorrelated noise, frequency-dependent decay (lows 2.6 s, mids 2.1 s, highs 0.8 s), early reflections"""
    g = np.random.default_rng(seed); n = int(length * FS); t = np.arange(n) / FS
    bands = [(20, 200, 2.6), (200, 800, 2.3), (800, 2500, 2.0), (2500, 6000, 1.4), (6000, 12000, 0.9), (12000, 20000, 0.55)]
    ir = np.zeros((2, n))
    for c in range(2):
        w = g.standard_normal(n)
        for f0, f1, rt in bands:
            sos = signal.butter(2, [f0, f1], 'band', fs=FS, output='sos'); ir[c] += signal.sosfilt(sos, w) * np.exp(-6.9 * t / rt)
        ir[c] *= np.minimum(1, t / 0.035) ** 1.5                                 # the diffuse field builds up over the first 35 ms
        for k in range(10):                                                     # early reflections, different on each side
            d = int(FS * g.uniform(0.006, 0.075)); ir[c, d] += g.choice([-1, 1]) * 0.9 * np.exp(-d / FS / 0.05)
        ir[c] /= np.sqrt((ir[c] ** 2).sum())
    return np.concatenate([np.zeros((2, int(pre * FS))), ir], axis=1)

def render_mix(plan, snd, panf, sid, extra=None, fade=None):
    """the multitrack mix: every note resampled to 48 kHz, cut by the next note on its channel (4 ms fade), low-cut under its own
    bottom, placed with an equal-power pan, summed into its role's bus; buses get ducking / widening; one hall for everything"""
    ev = list(plan['ev']) + list(extra or [])
    ev.sort(key=lambda e: (e[0], e[1]))
    info = panf.info
    # roles
    role = {}
    for i, d in snd.items():
        dur = len(d['x']) / d['rate']; cls = info.get(i, {}).get('cls', 'mel')
        role[i] = role_of(d['name'], cls, dur, d['src'] == 'hifi' or not re.match(r'\s*(sound|sample|inst)?\s*\d*\s*$', d['name'].lower()))
    # each note's length: until the next note on the same channel
    nxt = {}; cut = [None] * len(ev)
    for k in range(len(ev) - 1, -1, -1):
        ch = ev[k][1]; cut[k] = nxt.get(ch); nxt[ch] = ev[k][0]
    total = (max(e[0] for e in ev) if ev else 0) + 12.0
    if fade: total = min(total, fade[1])
    N = int(total * FS) + FS
    bus = {}; send = np.zeros((2, N), np.float32)
    cache = {}; lowc = {}
    F4 = int(0.004 * FS); fo = (0.5 * (1 + np.cos(np.linspace(0, np.pi, F4)))).astype(np.float32)
    for k, (t, ch, i, n, amp, pan) in enumerate(ev):
        key = (i, n)
        if key not in cache:
            d = snd[i]; rate = d['rate'] * 2 ** ((n - 49) / 12)
            y = to_rate(d['x'], FS / rate)
            if i not in lowc: lowc[i] = low_edge(d['x'], d['rate'])
            hp_f = min(ROLES[role[i]]['hpf'], 0.6 * lowc[i] * 2 ** ((n - 49) / 12))
            if hp_f > 18: y = signal.sosfilt(signal.butter(2, hp_f, 'high', fs=FS, output='sos'), y)
            if abs(y[0]) > 0.03: y[:32] *= np.sin(np.linspace(0, np.pi / 2, 32)) ** 2      # a recorded sample that starts mid-wave
            if len(y) > 192 and abs(y[-96:]).max() > 1e-3: y[-96:] *= np.cos(np.linspace(0, np.pi / 2, 96)) ** 2   # ...or stops mid-wave
            cache[key] = y.astype(np.float32)
        y = cache[key]; s0 = int(round(t * FS))
        if s0 >= N: continue
        L = len(y)
        if cut[k] is not None:
            c = int(round(cut[k] * FS)) - s0
            if c < L:
                L = min(L, c + F4); y = y[:L].copy(); m = L - c
                if m > 0: y[c:] *= fo[:m]
        L = min(L, N - s0); y = y[:L]
        R = ROLES[role[i]]; p = max(-1.0, min(1.0, pan * R['ps']))
        if role[i] in ('mel', 'pad', 'perc', 'fx'): p = math.copysign(abs(p) ** 0.85, p)      # (the plan's in-between places opened out a little)
        th = (p + 1) * np.pi / 4; gl, gr = np.cos(th) * amp, np.sin(th) * amp
        b = bus.get(role[i])
        if b is None: b = bus[role[i]] = np.zeros((2, N), np.float32)
        b[0, s0:s0 + L] += gl * y; b[1, s0:s0 + L] += gr * y
        sl = R['send']
        if sl: send[0, s0:s0 + L] += gl * sl * y; send[1, s0:s0 + L] += gr * sl * y
    # bus processing
    kick = bus.get('kick')
    if kick is not None:                                                            # a key from the kick: fast attack, 140 ms release
        e = np.abs(kick).max(0); blk = 48; nb = len(e) // blk
        eb = e[:nb * blk].reshape(nb, blk).max(1); eb /= (np.percentile(eb[eb > 1e-4], 99) if (eb > 1e-4).any() else 1)
        g = np.zeros(nb); a = 0.0; rel = np.exp(-1 / (0.14 * FS / blk))
        for j in range(nb):
            v = min(1.0, eb[j]); a = v if v > a else a * rel; g[j] = a
        key_env = np.interp(np.arange(N), np.arange(nb) * blk + blk / 2, g).astype(np.float32)
    for r, b in bus.items():
        R = ROLES[r]
        if R['duck'] and kick is not None:
            b *= (10 ** (-R['duck'] * key_env / 20)).astype(np.float32)
        if R['widen']:                                                              # mono-safe width: a delayed, high-passed copy added
            m = (b[0] + b[1]) * 0.5; d = int(0.011 * FS)                            # to one side and taken from the other
            sd = np.zeros_like(m); sd[d:] = m[:-d]; sd = signal.sosfilt(signal.butter(2, 350, 'high', fs=FS, output='sos'), sd).astype(np.float32)
            b[0] += R['widen'] * sd; b[1] -= R['widen'] * sd
    mix = sum(bus.values()) if bus else np.zeros((2, N), np.float32)
    ir = make_ir()
    wet = np.stack([signal.oaconvolve(send[c], ir[c])[:N] for c in range(2)])
    wet = signal.sosfilt(signal.butter(2, 230, 'high', fs=FS, output='sos'), wet)
    wet = signal.sosfilt(signal.butter(2, 8500, 'low', fs=FS, output='sos'), wet)
    mix = mix + wet.astype(np.float32)
    used_roles = {r: sorted(snd[i]['name'] for i in role if role[i] == r) for r in set(role.values())}
    return mix, used_roles

# ================================================================ mastering
def k_weight(x):
    """ITU-R BS.1770 K-weighting at 48 kHz (the two standard biquads)"""
    b1, a1 = [1.53512485958697, -2.69169618940638, 1.19839281085285], [1.0, -1.69065929318241, 0.73248077421585]
    b2, a2 = [1.0, -2.0, 1.0], [1.0, -1.99004745483398, 0.99007225036621]
    return signal.lfilter(b2, a2, signal.lfilter(b1, a1, x, axis=-1), axis=-1)

def lufs(x):
    """integrated loudness (BS.1770-4: 400 ms blocks, 75 % overlap, -70 LUFS absolute and -10 LU relative gates)"""
    blk = int(0.4 * FS); hop = blk // 4
    p = np.cumsum(np.concatenate([[0], sum(k_weight(np.asarray(x[c], np.float64)) ** 2 for c in range(x.shape[0]))]))
    st = np.arange(0, len(p) - 1 - blk, hop); z = (p[st + blk] - p[st]) / blk
    if not len(z): return -70.0
    l = -0.691 + 10 * np.log10(z + 1e-12); z = z[l > -70]
    if not len(z): return -70.0
    rel = -0.691 + 10 * np.log10(z.mean()) - 10; l = -0.691 + 10 * np.log10(z + 1e-12)
    return float(-0.691 + 10 * np.log10(z[l > rel].mean()))

def peaks4(x, chunk=FS * 10, ov=64):
    """per sample: the highest of the 4x-oversampled (inter-sample) peaks around it, both channels; worked in 10 s chunks"""
    x = np.asarray(x, np.float32); n = x.shape[1]; out = np.zeros(n, np.float32)
    for a in range(0, n, chunk):
        lo, hi = max(0, a - ov), min(n, a + chunk + ov)
        y = np.abs(signal.resample_poly(x[:, lo:hi].astype(np.float64), 4, 1, axis=-1)).max(0)
        y = y[:(hi - lo) * 4].reshape(-1, 4).max(1); out[a:min(n, a + chunk)] = y[a - lo:a - lo + min(chunk, n - a)]
    return out

def true_peak(x):
    return float(peaks4(x).max())

def control(x, blk=48):
    """per 1 ms block: the stereo-linked mean square"""
    e = (np.asarray(x, np.float64) ** 2).mean(0); nb = len(e) // blk
    return e[:nb * blk].reshape(nb, blk).mean(1), nb

def glue(x, ratio=1.8, knee=6.0, att=0.02, rel=0.22, max_gr=4.0):
    """bus compressor, stereo-linked, RMS-sensing: threshold just under the loud passages, so it only rides the peaks (1-3 dB)"""
    ms, nb = control(x); blk = 48
    rms = signal.lfilter([1 - np.exp(-1 / (0.012 * FS / blk))], [1, -np.exp(-1 / (0.012 * FS / blk))], ms)
    ldb = 10 * np.log10(rms + 1e-12); act = ldb[ldb > ldb.max() - 40]
    if not len(act): return x, 0.0
    T = np.percentile(act, 88) - 1.0
    over = ldb - T
    gr = np.where(over <= -knee / 2, 0, np.where(over >= knee / 2, over * (1 - 1 / ratio), (1 - 1 / ratio) * (over + knee / 2) ** 2 / (2 * knee)))
    gr = np.minimum(gr, max_gr)
    ka, kr = np.exp(-1 / (att * FS / blk)), np.exp(-1 / (rel * FS / blk)); g = np.zeros(nb); a = 0.0
    for j in range(nb):
        v = gr[j]; a = ka * a + (1 - ka) * v if v > a else kr * a + (1 - kr) * v; g[j] = a
    gl = np.interp(np.arange(x.shape[1]), np.arange(nb) * blk + blk / 2, g)
    return (x * 10 ** (-gl / 20)).astype(np.float32), float(np.percentile(g[g > 0.01], 95)) if (g > 0.01).any() else 0.0

def limiter(x, ceiling_db=-1.0, look=0.0015, rel=0.08):
    """true-peak brick wall: 4x oversampled peak sensing, 1.5 ms look-ahead, smooth 80 ms release"""
    c = 10 ** (ceiling_db / 20) * 0.985
    pk = peaks4(x).astype(np.float64)
    need = np.minimum(1.0, c / np.maximum(pk, 1e-9))
    La = int(look * FS)
    from scipy.ndimage import minimum_filter1d, uniform_filter1d
    g = minimum_filter1d(need, 2 * La + 1)                                   # hold each dip across the look-ahead window
    blk = 16; nb = len(g) // blk + 1; gb = np.ones(nb)
    gpad = np.concatenate([g, np.ones(nb * blk - len(g))]).reshape(nb, blk).min(1)
    kr = np.exp(-1 / (rel * FS / blk)); a = 1.0
    for j in range(nb):
        v = gpad[j]; a = v if v < a else kr * a + (1 - kr) * v; gb[j] = a
    h = np.repeat(gb, blk)[:len(g)]                                          # <= g everywhere (instant attack per block, slow release)
    gs = uniform_filter1d(h, La + 1)                                          # the attack ramp: a dip held +-La averaged over La keeps every peak under
    gs = np.minimum(gs, need)                                                 # (a no-op by construction; a guard)
    return (x * gs).astype(np.float32), float(20 * np.log10(gs.min()))

def tilt(x, strength=0.35, lim=2.0, hifi=True):
    """a gentle broad EQ towards a common tonal balance (-4.5 dB per octave above 100 Hz): only part of the way, at most +-2.5 dB,
    never lifting a band the song has nothing in"""
    m = np.asarray(x, np.float64).mean(0); f, pw = signal.welch(m, FS, nperseg=16384)
    cf = 1000 * 2 ** (np.arange(-10, 14) / 3); lv = []
    for c in cf:
        sel = (f >= c / 2 ** (1 / 6)) & (f < c * 2 ** (1 / 6)); lv.append(10 * np.log10(pw[sel].mean() + 1e-20) if sel.any() else -200)
    lv = np.array(lv); ref = np.interp(np.log2(1000), np.log2(cf), lv)
    target = ref - 4.5 * np.log2(cf / 1000)
    d = np.clip((target - lv) * strength, -lim, lim)
    d = np.convolve(np.pad(d, 2, mode='edge'), np.ones(5) / 5, 'valid')
    d[lv < lv.max() - 45] = np.minimum(d[lv < lv.max() - 45], 0)
    if not hifi: d[cf > 9000] = np.minimum(d[cf > 9000], 0)
    d[cf < 60] = np.minimum(d[cf < 60], 0)
    fr = np.concatenate([[0], cf, [FS / 2]]) / (FS / 2); gn = 10 ** (np.concatenate([[d[0]], d, [d[-1]]]) / 20)
    h = signal.firwin2(4097, fr, gn); y = np.stack([signal.oaconvolve(x[c], h)[2048:2048 + x.shape[1]] for c in range(2)])
    return y.astype(np.float32), d

def mono_bass(x, fc=110.0):
    """kick and bass dead centre: the side signal loses everything under fc"""
    mid = (x[0] + x[1]) * 0.5; side = (x[0] - x[1]) * 0.5
    side = signal.sosfilt(signal.butter(4, fc, 'high', fs=FS, output='sos'), side)
    return np.stack([mid + side, mid - side]).astype(np.float32)

def stereo(x, want=0.28, most=1.45):
    """the stereo picture: (1) left / right levels evened out (a mix that leans to one side is trimmed, as the game's converter does);
    (2) a narrow mix is opened up: its side signal above 250 Hz is raised (at most +3.2 dB) until side / mid reaches `want`"""
    l, r = float((x[0].astype(np.float64) ** 2).mean()), float((x[1].astype(np.float64) ** 2).mean())
    k = (l / (r + 1e-20)) ** 0.25; x = np.stack([x[0] / k, x[1] * k])
    mid = (x[0] + x[1]) * 0.5; side = (x[0] - x[1]) * 0.5
    sh = signal.sosfilt(signal.butter(2, 250, 'high', fs=FS, output='sos'), side)
    mh = signal.sosfilt(signal.butter(2, 250, 'high', fs=FS, output='sos'), mid)
    w = np.sqrt((sh ** 2).mean() / ((mh ** 2).mean() + 1e-20)); g = float(np.clip(want / (w + 1e-9), 1.0, most))
    side = side + (g - 1) * sh
    return np.stack([mid + side, mid - side]).astype(np.float32), dict(trim_db=float(20 * np.log10(k)), width_hi=float(w), side_gain_db=float(20 * np.log10(g)))

def master(mix, fade=None, target=-14.0, hifi=True, tail_db=-66):
    rep = {}
    x = mono_bass(mix)
    x, rep['stereo'] = stereo(x)
    x, rep['eq'] = tilt(x, hifi=hifi)
    x, rep['glue_gr'] = glue(x)
    if fade:                                                                     # the fade-out of a song that loops in the game
        a, b = int(fade[0] * FS), int(fade[1] * FS); b = min(b, x.shape[1])
        x[:, a:b] *= (np.linspace(1, 0, b - a) ** 2).astype(np.float32); x = x[:, :b + int(0.5 * FS)]; x[:, b:] = 0
    # loudness: -14 LUFS, unless that would make the limiter work more than ~5 dB on the loudest peak
    L0 = lufs(x); P0 = 20 * np.log10(true_peak(x) + 1e-12)
    g = min(target - L0, -1.0 + 4.0 - P0)
    x = (x * 10 ** (g / 20)).astype(np.float32)
    x, rep['lim_gr'] = limiter(x)
    # trim the silent end
    e = np.abs(x).max(0); thr = 10 ** (tail_db / 20); nz = np.nonzero(e > thr)[0]
    if len(nz): x = x[:, :min(x.shape[1], nz[-1] + int(0.3 * FS))]
    k = int(0.3 * FS); x[:, -k:] *= np.linspace(1, 0, k).astype(np.float32) ** 2
    nz = np.nonzero(np.abs(x).max(0) > 10 ** (-80 / 20))[0]
    if len(nz) and nz[0] > int(0.05 * FS): x = x[:, nz[0] - int(0.05 * FS):]          # (no long silence before the first note)
    rep['lufs'] = lufs(x); rep['tp'] = 20 * np.log10(true_peak(x) + 1e-12); rep['gain'] = g
    l, r = (x[0].astype(np.float64) ** 2).mean(), (x[1].astype(np.float64) ** 2).mean(); rep['balance_db'] = 10 * np.log10((l + 1e-20) / (r + 1e-20))
    m, sd = (x[0] + x[1]) / 2, (x[0] - x[1]) / 2; rep['width'] = float(np.sqrt((sd.astype(np.float64) ** 2).mean() / ((m.astype(np.float64) ** 2).mean() + 1e-20)))
    a, b = x[0].astype(np.float64), x[1].astype(np.float64); rep['corr'] = float((a * b).mean() / np.sqrt((a * a).mean() * (b * b).mean() + 1e-30))
    rep['secs'] = x.shape[1] / FS
    return x, rep

# ================================================================ one song, start to finish
TARGET = {'tree_swaying_action': -17.0, 'worthless_clouds': -15.0}      # quiet by nature: not pushed to -14
ENDS = {}                                                                # song id -> True / False to override the ending detection

def write_wav(path, x):
    from scipy.io import wavfile
    wavfile.write(path, FS, np.ascontiguousarray(x.T, dtype=np.float32))

def render_song(sid, path, outdir, cache):
    S, used, lo, panf, pans = read_song(sid, path)
    snd = sounds(sid, S, used, cache, path)
    plan = song_plan(S, used, lo, panf, pans, snd, sid)
    ev = plan['ev']; T = plan['t_end']
    act = lambda a, b: sum(e[4] for e in ev if a <= e[0] < b) / max(1e-9, b - a)
    tail_act = act(T - 6, T); avg = act(0, T)
    ends = ENDS.get(sid, True)                                           # (every song has a written ending; ENDS can make one loop + fade)
    extra = None; fade = None
    if not ends:                                                         # loops in the game: once through, round again, fade out
        more, _ = plan['again'](T); extra = [e for e in more if e[0] < T + 15.0]; fade = (T + 3.0, T + 15.0)
    mix, roles = render_mix(plan, snd, panf, sid, extra, fade)
    hifi = sid in GEN
    x, rep = master(mix, fade, TARGET.get(sid, -14.0), hifi=hifi)
    os.makedirs(os.path.join(outdir, 'wav'), exist_ok=True)
    write_wav(os.path.join(outdir, 'wav', sid + '.wav'), x)
    rep.update(sid=sid, ends=bool(ends), hifi=hifi, roles=roles, tail_act=tail_act / (avg + 1e-9),
               sounds={snd[i]['name']: snd[i]['src'] for i in snd}, eq=[round(float(v), 2) for v in rep['eq']])
    json.dump(rep, open(os.path.join(outdir, 'wav', sid + '.json'), 'w'), indent=1, default=float)
    return rep

CHIP_PAN = {'p1': -0.45, 'p2': 0.55, 'tri': 0.0, 'noi': 0.15}     # lead left of centre, harmony right, bass centre, drums just right
CHIP_SEND = {'p1': .12, 'p2': .16, 'tri': .02, 'noi': .07}

def render_chip(sid, path, title, opts, outdir, loops=4):
    """a creator-menu chiptune, from the same control tracks the game plays, by make_chiptunes.py's own 48 kHz renderer (4x oversampled,
    no 7.5 kHz band limit) - but with every voice on its own place in the stereo field: the APU's two non-linear mixers are kept, and
    each one's output is shared between its voices by their levels at that instant.  A small room; plays 3 times and fades on the 4th."""
    import chip_synth as CS, make_chiptunes as MC
    tr, NL, duty = CS.tracks_for(sid, path, opts)
    import copy                                                                    # the control tracks: the first loop, then the steady second
    tr = {k: copy.copy(t) for k, t in tr.items()}                                  # loop again and again, so every voice's phase runs on
    for t in tr.values():                                                          # across the joins (no seam, as one long render)
        for at in ('f', 'a', 'p', 'm'):
            if hasattr(t, at) and isinstance(getattr(t, at), np.ndarray) and len(getattr(t, at)) >= 2 * NL:
                arr = getattr(t, at); setattr(t, at, np.concatenate([arr[:NL]] + [arr[NL:2 * NL]] * (loops - 1)))
    n_ctrl = loops * NL; FSo = MC.FS * MC.OS
    def up(v): return np.repeat(v, MC.SPC)
    v = {}
    for key, d in (('p1', duty[0]), ('p2', duty[1])):
        t = tr[key]; f = up(t.f); a = up(t.a.astype(float)); ph = np.cumsum(f / FSo) % 1.0; v[key] = (ph < d).astype(float) * a
    t = tr['tri']; f = up(t.f); ph = np.cumsum(f / FSo) % 1.0; v['tri'] = MC.TRI[(ph * 32).astype(int) % 32] * (up(t.a.astype(float)) > 0)
    t = tr['noi']; per = np.array([MC.NOISE_P[min(15, q)] for q in t.p]); fclk = up(1789773.0 / per); pos = np.cumsum(fclk / FSo)
    idx = pos.astype(np.int64); m = up(t.m); v['noi'] = np.where(m > 0, MC.LF_SHORT[idx % 93], MC.LF_LONG[idx % 32767]) * up(t.a.astype(float))
    pp = v['p1'] + v['p2']; pm = np.where(pp > 0, 95.88 / (8128.0 / np.maximum(pp, 1e-9) + 100), 0.0)
    tn = v['tri'] / 8227.0 + v['noi'] / 12241.0; tm = np.where(tn > 0, 159.79 / (1.0 / np.maximum(tn, 1e-12) + 100), 0.0)
    out = {'p1': pm * np.where(pp > 0, v['p1'] / np.maximum(pp, 1e-9), 0), 'p2': pm * np.where(pp > 0, v['p2'] / np.maximum(pp, 1e-9), 0),
           'tri': tm * np.where(tn > 0, v['tri'] / 8227.0 / np.maximum(tn, 1e-12), 0), 'noi': tm * np.where(tn > 0, v['noi'] / 12241.0 / np.maximum(tn, 1e-12), 0)}
    L = NL * (MC.FS // MC.CR); hp = signal.butter(1, 40, 'high', fs=FS, output='sos')
    mix = np.zeros((2, L * loops)); send = np.zeros((2, L * loops))
    for key, y in out.items():
        y = signal.resample_poly(y, 1, MC.OS, window=('kaiser', 8.0)); y = signal.sosfilt(hp, y - y[:L].mean())[:L * loops]
        th = (CHIP_PAN[key] + 1) * np.pi / 4
        mix[0] += np.cos(th) * y; mix[1] += np.sin(th) * y; send[0] += np.cos(th) * y * CHIP_SEND[key]; send[1] += np.sin(th) * y * CHIP_SEND[key]
    ir = make_ir(seed=11, length=1.6, pre=0.012)                                  # (a smaller room than the songs' hall)
    ir = ir[:, :int(1.6 * FS)] * np.exp(-np.arange(int(1.6 * FS)) / FS / 0.35)
    for c in range(2): ir[c] /= np.sqrt((ir[c] ** 2).sum())
    pad = int(2.0 * FS); mix = np.concatenate([mix, np.zeros((2, pad))], 1); send = np.concatenate([send, np.zeros((2, pad))], 1)
    wet = np.stack([signal.oaconvolve(send[c], ir[c])[:mix.shape[1]] for c in range(2)])
    wet = signal.sosfilt(signal.butter(2, 300, 'high', fs=FS, output='sos'), wet)
    mix = (mix + wet).astype(np.float32)
    fade = ((loops - 1) * L / FS, loops * L / FS)
    x, rep = master(mix, fade, -15.0, hifi=True)
    os.makedirs(os.path.join(outdir, 'wav'), exist_ok=True)
    write_wav(os.path.join(outdir, 'wav', 'chip_' + sid + '.wav'), x)
    rep.update(sid='chip_' + sid, title=title, loop_secs=L / FS, eq=[round(float(q), 2) for q in rep['eq']])
    json.dump(rep, open(os.path.join(outdir, 'wav', 'chip_' + sid + '.json'), 'w'), indent=1, default=float)
    return rep

def chip_table():
    import make_chiptunes as MC
    return [(secret, sid, path, title, opts) for secret, lst in ((0, MC.SONGS), (1, MC.SECRET)) for (sid, path, title, opts) in lst]

def _chip(args):
    secret, sid, path, title, opts, outdir = args
    try:
        r = render_chip(sid, path, title, opts, outdir)
        return 'chip_' + sid, 'ok %5.1f s (loop %.1f s) %6.2f LUFS %5.2f dBTP  width %.2f corr %.2f' % (r['secs'], r['loop_secs'], r['lufs'], r['tp'], r['width'], r['corr'])
    except Exception:
        import traceback; return 'chip_' + sid, 'FAILED\n' + traceback.format_exc()

def song_table():
    """(id, NAME, xm path) of every tracker song, in songs.h order (the title song first)"""
    return [(a, b, c) for a, b, c in re.findall(r'^SONG_XM\((\w+),"([^"]+)","([^"]+)"\)', open('source/songs.h').read(), re.M)]

def _one(args):
    sid, path, outdir, cache = args
    try:
        r = render_song(sid, path, outdir, cache)
        return sid, 'ok %5.1f s %6.2f LUFS %5.2f dBTP  glue %.1f  lim %.1f dB  bal %+.2f  width %.2f (side %+.1f dB) corr %.2f %s' % (
            r['secs'], r['lufs'], r['tp'], r['glue_gr'], -r['lim_gr'], r['balance_db'], r['width'], r['stereo']['side_gain_db'], r['corr'], '' if r['ends'] else 'LOOPS -> fade')
    except Exception:
        import traceback; return sid, 'FAILED\n' + traceback.format_exc()

# ================================================================ the release: cover, tags, MP3 (LAME V0) + FLAC (24-bit), a track list
ALBUM = 'BORE (Original Soundtrack) - Studio Edition'
SMALL = {'a', 'an', 'the', 'to', 'in', 'on', 'of', 'and', 'at', 'for'}
def title_case(t):
    w = t.lower().split(' ')
    return ' '.join((x if (k and x in SMALL) else '-'.join(y[:1].upper() + y[1:] for y in x.split('-'))) if not x.startswith('(') else '(' + x[1:2].upper() + x[2:] for k, x in enumerate(w))

def make_cover(out):
    from PIL import Image, ImageDraw, ImageFont, ImageFilter
    W = 1500; logo = Image.open('assets/preview/title_logo.png').convert('RGB'); lw, lh = logo.size; a = np.array(logo).astype(float)
    y = np.linspace(0, 1, W)[:, None, None]; bg = a[2, 2] * (1 - 0.55 * y) + np.array([40, 18, 22]) * 0.55 * y
    bg = np.broadcast_to(bg, (W, W, 3)).copy(); yy, xx = np.mgrid[0:W, 0:W]
    bg += np.exp(-(((xx - W / 2) / 620) ** 2 + ((yy - 610) / 330) ** 2))[..., None] * np.array([60, 38, 10])
    img = Image.fromarray(np.clip(bg, 0, 255).astype(np.uint8))
    k = 2; L = logo.resize((lw * k, lh * k), Image.NEAREST); la = np.array(L).astype(float)
    mask = (np.abs(la - np.median(la[:, :40, :], axis=1, keepdims=True)).sum(2) > 60).astype(np.uint8) * 255
    M = Image.fromarray(mask).filter(ImageFilter.MaxFilter(3)); x0 = (W - lw * k) // 2; y0 = 610 - lh * k // 2
    sh = Image.new('L', img.size, 0); sh.paste(M, (x0 + 10, y0 + 14)); sh = sh.filter(ImageFilter.GaussianBlur(14))
    img = Image.composite(Image.new('RGB', img.size, (20, 8, 10)), img, sh.point(lambda v: v * 0.6)); img.paste(L, (x0, y0), M)
    d = ImageDraw.Draw(img)
    f1 = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf', 74); f2 = ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf', 44)
    def centred(t, f, yy, col, sp=0):
        x = (W - d.textlength(t, font=f) - sp * (len(t) - 1)) / 2
        for ch in t: d.text((x + 4, yy + 5), ch, font=f, fill=(25, 10, 12)); d.text((x, yy), ch, font=f, fill=col); x += d.textlength(ch, font=f) + sp
    centred('ORIGINAL SOUNDTRACK', f1, 1000, (255, 214, 80), 6); d.line([(470, 1106), (1030, 1106)], fill=(255, 214, 80), width=3)
    centred('STUDIO EDITION', f2, 1126, (245, 230, 210), 18); img.save(out)

def album(outdir):
    """the track list: disc 1 the jukebox (the title song first), disc 2 the secret and locked songs, disc 3 the creator chiptunes"""
    artist = dict(re.findall(r'^ARTIST\("([^"]+)","([^"]+)"\)', open('source/artists.h', encoding='utf-8').read(), re.M))
    locked = set(re.findall(r'^UNLOCK\("([^"]+)"', open('source/unlocks.h', encoding='utf-8').read(), re.M))
    d1, d2, d3 = [], [], []
    for sid, name, path in song_table():
        a = artist.get(name, 'The Dipper Man')
        if name.endswith(' (ORIGINAL)'): d2.append((sid, name, a, 'secret: the first version'))
        elif name in locked: d2.append((sid, name, a, 'locked until a lifetime want is met'))
        else: d1.append((sid, name, a, 'title screen' if sid == 'the_dipper_man' else ''))
    for secret, sid, path, title, opts in chip_table():
        d3.append(('chip_' + sid, title + ' (CHIPTUNE)', artist.get(title, 'BORE'), 'creator menu loop' + (', secret' if secret else '')))
    return [('Disc 1 - Jukebox', d1), ('Disc 2 - Secrets and Originals', d2), ('Disc 3 - Creator Chiptunes', d3)]

def pack(outdir, only=None):
    """the whole album, or (only = some song ids) just those tracks again: their files are replaced and so are the zip parts holding them"""
    import zipfile, shutil
    rel = os.path.join(outdir, 'BORE Studio Edition')
    if not only: shutil.rmtree(rel, ignore_errors=True)
    os.makedirs(rel, exist_ok=True)
    cover = os.path.join(rel, 'cover.png'); make_cover(cover)
    lines = [ALBUM, '48 kHz / 24-bit masters (FLAC) and LAME V0 MP3s.  Loudness -14 LUFS (quiet pieces lower), true peak -1 dBTP.', '']
    flac_dir = os.path.join(outdir, 'BORE Studio Edition (FLAC)')
    if not only: shutil.rmtree(flac_dir, ignore_errors=True)
    changed = []
    for dn, (disc, items) in enumerate(album(outdir), 1):
        lines += ['', disc]; os.makedirs(os.path.join(rel, disc), exist_ok=True); os.makedirs(os.path.join(flac_dir, disc), exist_ok=True)
        for tn, (sid, name, art, note) in enumerate(items, 1):
            wav = os.path.join(outdir, 'wav', sid + '.wav'); rep = json.load(open(os.path.join(outdir, 'wav', sid + '.json')))
            t = title_case(name); base = '%d-%02d %s' % (dn, tn, re.sub(r'[\\/:*?"<>|]', '', t))
            meta = ['-metadata', 'title=' + t, '-metadata', 'artist=' + art, '-metadata', 'album=' + ALBUM, '-metadata', 'album_artist=BORE',
                    '-metadata', 'track=%d/%d' % (tn, len(items)), '-metadata', 'disc=%d/3' % dn, '-metadata', 'genre=Soundtrack', '-metadata', 'date=2026']
            if not only or sid in only:
              changed.append(os.path.join(rel, disc, base + '.mp3'))
              subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', wav, '-i', cover, '-map', '0:a', '-map', '1:v', '-c:a', 'libmp3lame', '-q:a', '0',
                            '-compression_level', '0', '-c:v', 'mjpeg', '-vf', 'scale=600:600', '-disposition:v', 'attached_pic', '-id3v2_version', '3']
                           + meta + [os.path.join(rel, disc, base + '.mp3')], check=True)
              subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', wav, '-i', cover, '-map', '0:a', '-map', '1:v', '-c:a', 'flac', '-sample_fmt', 's32',
                            '-bits_per_raw_sample', '24', '-compression_level', '8', '-c:v', 'png', '-disposition:v', 'attached_pic']
                           + meta + [os.path.join(flac_dir, disc, base + '.flac')], check=True)
            m, sec = divmod(int(round(rep['secs'])), 60)
            na = sum(1 for v in rep.get('sounds', {}).values() if v == 'archive')
            how = 'hi-fi re-synthesis' if rep.get('hifi') else ('chip voices in stereo' if sid.startswith('chip_') else 'original samples' + (', %d from better copies' % na if na else ''))
            lines.append('  %d-%02d  %-42s %-22s %2d:%02d  %s%s' % (dn, tn, t, art, m, sec, how, ('  (' + note + ')') if note else ''))
            print(lines[-1], flush=True)
    open(os.path.join(rel, 'TRACKLIST.txt'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
    shutil.copy(cover, flac_dir); shutil.copy(os.path.join(rel, 'TRACKLIST.txt'), flac_dir)
    if not only: return zip_parts(outdir)
    import glob                                                              # just the parts holding a changed track, rewritten in place
    out = []
    for zn in sorted(glob.glob(os.path.join(outdir, 'BORE Studio Edition part * of *.zip'))):
        with zipfile.ZipFile(zn) as z: names = z.namelist()
        if not any(os.path.relpath(c, outdir) in names for c in changed + [os.path.join(rel, 'TRACKLIST.txt')]): continue
        with zipfile.ZipFile(zn, 'w', zipfile.ZIP_STORED) as z:
            for n in names: z.write(os.path.join(outdir, n), n)
        out.append(zn)
    return out

def zip_parts(outdir, most=23e6):
    """the MP3 album in zip parts small enough to send (at most ~23 MB each, whole tracks only, packed first-fit by size); unzipped
    into one folder they make the album again"""
    import zipfile, glob
    for old in glob.glob(os.path.join(outdir, 'BORE Studio Edition part * of *.zip')): os.remove(old)
    rel = os.path.join(outdir, 'BORE Studio Edition')
    files = sorted((os.path.join(dp, f) for dp, dn_, fs in os.walk(rel) for f in fs), key=lambda f: -os.path.getsize(f))
    bins = []
    for f in files:
        z = os.path.getsize(f)
        for b in bins:
            if b[0] + z <= most: b[0] += z; b[1].append(f); break
        else: bins.append([z, [f]])
    bins.sort(key=lambda b: min(b[1]))                                   # (part 1 holds the earliest tracks)
    names = []
    for k, (z, part) in enumerate(bins, 1):
        zn = os.path.join(outdir, 'BORE Studio Edition part %d of %d.zip' % (k, len(bins))); names.append(zn)
        with zipfile.ZipFile(zn, 'w', zipfile.ZIP_STORED) as zf:
            for f in sorted(part): zf.write(f, os.path.relpath(f, outdir))
    return names

if __name__ == '__main__' and len(sys.argv) > 1 and sys.argv[1] == '--bank':
    dump_bank(sys.argv[2], sys.argv[3]); sys.exit(0)

if __name__ == '__main__' and len(sys.argv) > 2 and sys.argv[1] == '--zip':
    for z in zip_parts(sys.argv[2]): print(z, os.path.getsize(z) // 1048576, 'MB')
    sys.exit(0)

if __name__ == '__main__' and len(sys.argv) > 2 and sys.argv[1] == '--pack':
    for z in pack(sys.argv[2], set(sys.argv[3:]) or None): print(z, os.path.getsize(z) // 1048576, 'MB')
    sys.exit(0)

if __name__ == '__main__':
    import concurrent.futures as cf
    outdir = sys.argv[1]; want = set(sys.argv[2:]); cache = os.path.join(outdir, 'bank'); os.makedirs(cache, exist_ok=True)
    jobs = [(sid, path, outdir, cache) for sid, name, path in song_table() if not want or sid in want or 'songs' in want]
    for m in sorted(set(GEN[j[0]] for j in jobs if j[0] in GEN)): bank(m, cache)          # (the banks first, so parallel songs share them)
    jobs.sort(key=lambda j: -os.path.getsize(j[1]))
    chips = [c + (outdir,) for c in chip_table() if not want or 'chip_' + c[1] in want or 'chips' in want]
    with cf.ProcessPoolExecutor(int(os.environ.get('STUDIO_JOBS', '3'))) as ex:
        for sid, msg in ex.map(_one, jobs): print('%-30s %s' % (sid, msg), flush=True)
        for sid, msg in ex.map(_chip, chips): print('%-30s %s' % (sid, msg), flush=True)
