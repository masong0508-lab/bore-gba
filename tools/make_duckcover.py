#!/usr/bin/env python3
"""DUCK & COVER (DNB REVISION): a Koala Sampler project ("Duck & Cover", 145 BPM) transcribed and rebuilt as a drum & bass banger at 174 BPM.

usage: python3 tools/make_duckcover.py path/to/Duck__Cover.koala        (run from the project root)

What it does
  * reads the Koala project: sequence.json (6 filled pattern slots, 16384 ticks per bar = 4096 per beat; every note: pad, pitch, length, velocity) and sampler/*.wav (12 sample pads
    + the QUOKKA synth pad 1, a filtered saw pluck: its envelope / cutoff / drive settings are copied from sampler.json)
  * the notes are transcribed to 1/32-note rows (pad 1 = the pluck, pad 9 = a pitched 62 Hz sub sample, everything else = the sampled chops). Pitch 0 of the pluck is B, the same B as the
    sub sample's own pitch (B1): that is the key of E minor the rest of the notes agree on
  * every sampled chop is TIME-FITTED to the new tempo: stretched (pitch kept, phase vocoder) to a whole number of half beats of 174 BPM; the 22 s break (pad 6) is stretched by exactly 145/174
  * "whatever is unused" is used: the parts of the recordings that no pad plays (before / after the pad's start-end window) are cut into 4-beat chunks, the 11 loudest become instruments
    and come back as reverse swells, offbeat stabs and fill-ins; the pads' own tails and the bus-FX (bit-crusher / ring-mod / vocoder) are approximated by light crushing
  * new drum & bass drums (kick, snare, ghost snares, hats, crash, riser) and a heavy bass (the sampled sub + a reese + octave jumps)
  * outputs: tools/duckcover/duck_cover_dnb.xm (the DAW file: 16 channels, 32 instruments, speed 3 / 174 BPM), tools/duckcover/duck_cover_dnb.mid, tools/duckcover/duck_cover_dnb.wav (the
    preview) and the game's stream (source/music/duck_cover.adp + a SONG_ADP line in source/songs.h)
"""
import os, sys, json, zipfile, io, subprocess
import numpy as np, soundfile as sf, librosa, mido
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xmwrite as X
from encode_sfx import encode, decode
import encode_song

SR = 44100; OLD = 145.0; NEW = 174.0; BEAT = 60.0 / NEW; GBA = 18157
OUT = "tools/duckcover"; ROWB = 8                       # rows per beat (1/32 notes)
rng = np.random.default_rng(7)

# ------------------------------------------------------------------------------------------------------------ the project
z = zipfile.ZipFile(sys.argv[1]); SEQ = json.loads(z.read("sequence.json")); SJ = json.loads(z.read("sampler/sampler.json"))
SLOTS = [s["pattern"] for s in SEQ["sequences"] if s and s["pattern"]["notes"]]            # P0..P5
PADS = {str(p["pad"]): p for p in SJ["pads"]}
Q = PADS["1"]["synthParams"]
def wav(i):
    y, sr = sf.read(io.BytesIO(z.read("sampler/%d.wav" % i))); y = y.mean(1) if y.ndim > 1 else y
    return y, sr
def mono44(y, sr): return signal.resample_poly(y, SR, sr).astype(np.float64) if sr != SR else y

def fade(y, a=0.004, b=0.02):
    y = y.copy(); na, nb = int(a * SR), int(b * SR); na = min(na, len(y) // 2); nb = min(nb, len(y) // 2)
    if na: y[:na] *= np.linspace(0, 1, na)
    if nb: y[-nb:] *= np.linspace(1, 0, nb)
    return y
def stretch(y, r):           # r > 1 plays faster (shorter), pitch kept
    return librosa.effects.time_stretch(y.astype(np.float32), rate=float(r)).astype(np.float64)
def fit(y, snap=0.5, minb=0.5):   # time-fit a chop: its length becomes a whole number (of `snap` beats) of the NEW tempo
    beats = len(y) / SR * OLD / 60; sb = max(minb, round(beats / snap) * snap); target = int(sb * BEAT * SR)
    r = len(y) / target; r = min(max(r, 0.6), 1.7)
    out = stretch(y, r); out = out[:target] if len(out) >= target else np.concatenate([out, np.zeros(target - len(out))])
    return fade(out), sb
def trim_onset(y, pre=0.006, thr=0.12):
    """cut the silence / breath before the first syllable so the first transient lands exactly on the grid (the pads start 40-136 ms before their sound)"""
    e = np.abs(y); k = max(1, int(0.003 * SR)); e = np.convolve(e, np.ones(k) / k, "same"); i = int(np.argmax(e > thr * e.max()))
    return y[max(0, i - int(pre * SR)):]
def crush(y, bits=13, rate=12400):   # the bus-A BITCOOKER, lightly
    ys = signal.resample_poly(y, rate, SR); q = 2.0 ** (bits - 1); ys = np.round(ys * q) / q
    return signal.resample_poly(ys, SR, rate)[:len(y)] if len(y) else y

CH = {}; EXTRA = []            # pad -> fitted chop ; list of unused chunks
for k, p in PADS.items():
    if p.get("type") == "synth" or "sampleId" not in p: continue
    y, sr = wav(p["sampleId"]); a, b = int(p["start"] * SR / sr), int(p["end"] * SR / sr); y = mono44(y, sr)
    seg = y[a:b] if k == "9" else trim_onset(y[a:b])
    if k == "9": CH[k] = dict(y=fade(seg, 0.001, 0.05), beats=len(seg) / SR * OLD / 60)            # the sub: pitched, not stretched
    elif k == "6": CH[k] = dict(y=fade(stretch(seg, NEW / OLD)), beats=len(seg) / SR * OLD / 60)      # the break: tempo conversion, exact
    else:
        f, sb = fit(seg); CH[k] = dict(y=f, beats=sb)
    # the unused parts of the recording (outside the pad's start/end window), cut in 4-beat chunks
    CH[k]["oneshot"] = bool(p.get("oneshot")); CH[k]["vol"] = float(p.get("vol", 1.0)); CH[k]["rel"] = float(p.get("release", 0))
    if k in ("6", "9"): continue
    for lo, hi in ((0, a), (b, len(y))):
        step = int(4 * 60 / OLD * SR)
        for s0 in range(lo, hi - step // 2, step):
            c = y[s0:min(hi, s0 + step)]
            if len(c) < step // 2 or np.sqrt((c ** 2).mean()) < 0.012: continue
            EXTRA.append(dict(y=c, rms=float(np.sqrt((c ** 2).mean())), src=int(p["sampleId"]), at=s0))
EXTRA.sort(key=lambda e: -e["rms"]); EXTRA = EXTRA[:11]; EXTRA.sort(key=lambda e: (e["src"], e["at"]))
for e in EXTRA: e["y"], e["beats"] = fit(e["y"], 1.0, 1.0)

# ------------------------------------------------------------------------------------------------------------ the synths and drums
def env_ad(n, a, d, s, rel, gate):         # amp envelope of QUOKKA: attack, exponential decay to sustain, release after the gate
    t = np.arange(n) / SR; e = np.minimum(1, t / max(a, 1e-4)) * (s + (1 - s) * np.exp(-t / (d / 4)))
    g = int(gate * SR)
    if g < n:
        r = np.exp(-np.arange(n - g) / SR / (rel / 4)); e[g:] = e[g - 1] * r if g > 0 else 0
    return e
def pluck(m, gate):
    f = 440.0 * 2 ** ((m - 69) / 12); n = int((gate + Q["ampEnvRelease"] * 0.6) * SR); t = np.arange(n) / SR
    y = signal.sawtooth(2 * np.pi * f * t) + 0.6 * signal.sawtooth(2 * np.pi * f * 1.004 * t, 0.3)
    b, a = signal.butter(2, min(0.45, (Q["cutoff"] * 1.1) / (SR / 2)), "low"); y = signal.lfilter(b, a, y)
    y = np.tanh(y * (1 + 2 * Q["drive"])) * env_ad(n, Q["ampEnvAttack"], min(Q["ampEnvDecay"], 0.9), Q["ampEnvSustain"], Q["ampEnvRelease"], gate)
    return fade(y * 0.30, 0.001, 0.01)
def sub(m, gate):                                    # the sampled 62 Hz sub, pitched by resampling; gated
    y = CH["9"]["y"]; ratio = 2 ** ((m - 35) / 12); out = signal.resample(y, int(len(y) / ratio)) if ratio != 1 else y
    n = int((gate + 0.04) * SR)
    out = np.concatenate([out, np.zeros(max(0, n - len(out)))])[:n]
    if gate < len(out) / SR: out[int(gate * SR):] *= np.linspace(1, 0, len(out) - int(gate * SR)) ** 2
    return out
def reese(m, gate):
    f = 440.0 * 2 ** ((m - 69) / 12); n = int((gate + 0.05) * SR); t = np.arange(n) / SR
    y = sum(signal.sawtooth(2 * np.pi * f * (1 + d) * t) for d in (-0.011, -0.004, 0.004, 0.011))
    b, a = signal.butter(2, 650 / (SR / 2), "low"); y = signal.lfilter(b, a, y); y = np.tanh(y * 0.9)
    e = np.ones(n); e[-int(0.05 * SR):] = np.linspace(1, 0, int(0.05 * SR)); e[:int(0.004 * SR)] *= np.linspace(0, 1, int(0.004 * SR))
    return y * e * 0.30
def kick():
    n = int(0.32 * SR); t = np.arange(n) / SR; ph = 2 * np.pi * np.cumsum(46 + 130 * np.exp(-t / 0.028)) / SR
    y = np.sin(ph) * np.exp(-t / 0.16) * 1.1 + np.exp(-t / 0.002) * rng.standard_normal(n) * 0.25
    return np.tanh(y * 1.6)
def snare():
    n = int(0.26 * SR); t = np.arange(n) / SR; nz = rng.standard_normal(n)
    b, a = signal.butter(2, [1500 / (SR / 2), 9000 / (SR / 2)], "band"); nz = signal.lfilter(b, a, nz)
    y = np.sin(2 * np.pi * (185 + 70 * np.exp(-t / 0.02)) * t) * np.exp(-t / 0.05) * 0.8 + nz * np.exp(-t / 0.07) * 0.9 + np.exp(-t / 0.003) * rng.standard_normal(n) * 0.5
    return np.tanh(y * 1.3)
def hat(open_=False):
    n = int((0.30 if open_ else 0.05) * SR); t = np.arange(n) / SR; nz = rng.standard_normal(n)
    b, a = signal.butter(3, 7000 / (SR / 2), "high"); return signal.lfilter(b, a, nz) * np.exp(-t / (0.09 if open_ else 0.012)) * 0.45
def crash():
    n = int(1.6 * SR); t = np.arange(n) / SR; nz = rng.standard_normal(n); b, a = signal.butter(2, 4000 / (SR / 2), "high")
    return signal.lfilter(b, a, nz) * np.exp(-t / 0.5) * 0.5
def riser(beats):
    n = int(beats * BEAT * SR); t = np.arange(n) / SR; x = t / t[-1]; nz = rng.standard_normal(n)
    out = np.zeros(n); k = 6
    for i in range(k):                                  # a noise sweep made from rising band slices
        c = 600 * 2 ** (i * 0.9); b, a = signal.butter(2, [c / (SR / 2), min(0.95, c * 1.5 / (SR / 2))], "band"); out += signal.lfilter(b, a, nz)
    return out * x ** 2 * 0.25 + np.sin(2 * np.pi * np.cumsum(200 + 1800 * x ** 2) / SR) * x ** 2 * 0.12
K, S, HC, HO, CR = kick(), snare(), hat(), hat(True), crash()

# ------------------------------------------------------------------------------------------------------------ the score (beat positions; the arrangement)
EV = []       # dicts: t (beats), kind, + params
def ev(t, kind, **kw): EV.append(dict(t=round(t * ROWB) / ROWB, kind=kind, **kw))
def tb(tick): return tick / 4096.0

def drums(b0, bars, style="a", fill_every=4, kick_on=True):
    for bar in range(bars):
        t0 = b0 + bar * 4; last = (bar % fill_every == fill_every - 1); v = 1.0
        if style == "half":
            ev(t0, "kick", v=1); ev(t0 + 2, "snare", v=1)
            for s in range(0, 16, 2): ev(t0 + s * 0.25, "hat", v=0.6 if s % 4 else 0.85)
        elif style == "ride":
            for s in range(0, 16, 2): ev(t0 + s * 0.25, "hat", v=0.45 if s % 4 else 0.6)
        else:
            ks = [0, 10] if style == "a" else [0, 3, 10, 14]
            if kick_on:
                for s in ks:
                    if not (last and s >= 10 and style == "a"): ev(t0 + s * 0.25, "kick", v=1.0 if s == 0 else 0.9)
            for s in (4, 12): ev(t0 + s * 0.25, "snare", v=1.0)
            for s in (7, 15) if style == "a" else (7, 9, 15): ev(t0 + s * 0.25, "snare", v=0.28)
            for s in range(16):
                if style == "a" and s % 2: continue
                ev(t0 + s * 0.25, "hat", v=(0.8 if s % 4 == 0 else 0.5) * (0.8 if style == "b" and s % 2 else 1))
            if bar % 2 == 1: ev(t0 + 3.5, "ohat", v=0.7)
            if bar == 0: ev(t0, "crash", v=0.9)
            if last:                                                                       # a snare fill on the 4th bar
                for q in range(8): ev(t0 + 2 + q * 0.25, "snare", v=0.5 + 0.06 * q)

def buildup(b0, bars):
    for bar in range(bars):
        t0 = b0 + bar * 4; step = [1, 0.5, 0.5, 0.25][min(3, bar * 4 // max(1, bars))]
        t = 0.0
        while t < 4 - 1e-9:
            ev(t0 + t, "snare", v=0.35 + 0.65 * ((bar * 4 + t) / (bars * 4))); t += step
        ev(t0, "kick", v=1)
    ev(b0, "riser", beats=bars * 4)

def pat_notes(slot, pad):
    return [n for n in SLOTS[slot]["notes"] if str(n["num"]) == str(pad)]
def place_pattern(slot, b0, bars=None, pluck_oct=0, bass=True, samples=True, skip_pads=(), auto_bass=False, bvel=1.0):
    nb = SLOTS[slot]["numBars"]; bars = bars or nb; lim = bars * 4
    for n in SLOTS[slot]["notes"]:
        t = tb(n["timeOffset"]); ln = tb(n["length"]); pad = str(n["num"])
        if t >= lim: continue
        if pad == "1": ev(b0 + t, "pluck", m=59 + n["pitch"] + pluck_oct, g=max(0.12, ln) * BEAT, v=n["vel"] / 127)
        elif pad == "9":
            if bass: ev(b0 + t, "bass", m=35 + n["pitch"], g=max(0.25, ln) * BEAT, v=bvel)
        elif samples and pad not in skip_pads: ev(b0 + t, "chop", pad=pad, g=ln * BEAT, v=n["vel"] / 127, p=n["pitch"])
    if auto_bass:
        pl = sorted(pat_notes(slot, 1), key=lambda n: n["timeOffset"])
        for bar in range(bars):
            for hit in (0.0, 1.5, 2.75):                                                   # a two-step bass phrase on the pluck's own notes
                t = bar * 4 + hit; cand = [n for n in pl if tb(n["timeOffset"]) <= t + 1e-6]
                if cand: ev(b0 + t, "bass", m=35 + (int(cand[-1]["pitch"]) + 12) % 12, g=(1.2 if hit != 2.75 else 0.75) * BEAT, v=bvel * 0.95)

def extras_at(idx, t, rev=False, v=0.7): ev(t, "extra", i=idx % len(EXTRA), rev=rev, v=v)

POS = {}
t = 0.0
# INTRO  (P0, 9 bars): the sampled chops alone, then hats and a riser
POS["intro"] = t
place_pattern(0, t, 9, pluck_oct=0, bass=False)
extras_at(0, t, True, 0.55); extras_at(1, t + 8, True, 0.5)
drums(t + 16, 4, "ride"); buildup(t + 28, 2)
t += 9 * 4 + 8
# DROP 1  (P2, 8 bars, then P3, 14 bars)
POS["drop1"] = t
drums(t, 8, "a"); place_pattern(2, t, 8, pluck_oct=-12)
t += 32
drums(t, 14, "b"); place_pattern(3, t, 14, auto_bass=False)
extras_at(2, t + 24, False, 0.55); extras_at(3, t + 40, False, 0.55)
t += 56
# BREAKDOWN  (P1, 14 bars): half-time, vocal chops and the pluck, no bass until the riser
buildup_len = 2
POS["break"] = t
drums(t, 12, "half"); place_pattern(1, t, 14, bass=False)
for i in range(4): extras_at(4 + i, t + 8 + i * 8, i % 2 == 0, 0.6)
buildup(t + 48, 2)
t += 56 + 8
# DROP 2  (P4, 28 bars): rolling drums, the break layer, bass
POS["drop2"] = t
drums(t, 28, "b"); place_pattern(4, t, 28, pluck_oct=-12)
ev(t, "chop", pad="6", g=0, v=0.55, p=0)
for i in range(28 // 6): ev(t + 4 + i * 24, "chop", pad="6", g=0, v=0.45, p=0)
for i in range(6): extras_at(6 + i, t + 8 + i * 16, i % 3 == 1, 0.55)
t += 112
# DROP 3  (P3 again, up an octave in the pluck, bass octave jumps)
buildup(t, 2); t += 8
POS["drop3"] = t
drums(t, 14, "b"); place_pattern(3, t, 14, pluck_oct=12, bvel=1.0, skip_pads=())
for i in range(14): ev(t + i * 4 + 1.5, "bass", m=47 + (0 if i % 2 else 5) - 12 + 12 * (i % 2), g=0.5 * BEAT, v=0.55)
t += 56
# OUTRO  (P5, 6 bars): the drums fall away
POS["outro"] = t
drums(t, 6, "a", kick_on=True); place_pattern(5, t, 6, pluck_oct=-12)
drums(t + 24, 2, "ride"); ev(t + 24, "crash", v=0.8)
t += 6 * 4 + 8
TOTAL = t
EV.sort(key=lambda e: e["t"])
# the per-section gain
for e in EV:
    e["v"] = float(e.get("v", 1.0))

# ------------------------------------------------------------------------------------------------------------ render
N = int((TOTAL * BEAT + 4) * SR); mix = np.zeros(N)
def add(y, t, g=1.0):
    a = int(round(t * BEAT * SR)); b = min(N, a + len(y))
    if a < N and b > a: mix[a:b] += y[:b - a] * g
def chopaudio(pad, g, p):
    d = CH[pad]; y = d["y"]
    if pad == "9": return y
    if abs(p) > 1e-6: y = librosa.effects.pitch_shift(y.astype(np.float32), sr=SR, n_steps=p).astype(np.float64)
    if g and not d["oneshot"] and g < len(y) / SR: n = int(g * SR + d["rel"] * SR * 0.5); y = fade(y[:n], 0.002, 0.04)
    return y
PCACHE = {}
def hp180(y): return signal.sosfiltfilt(signal.butter(2, 180 / (SR / 2), "hp", output="sos"), y)
def mixlen(a, b):
    n = max(len(a), len(b)); out = np.zeros(n); out[:len(a)] += a; out[:len(b)] += b; return out
for e in EV:
    k = e["kind"]; v = e["v"]
    if k == "kick": add(K, e["t"], 1.0 * v)
    elif k == "snare": add(S, e["t"], 0.85 * v)
    elif k == "hat": add(HC, e["t"], v)
    elif k == "ohat": add(HO, e["t"], v)
    elif k == "crash": add(CR, e["t"], 0.7 * v)
    elif k == "riser": add(riser(e["beats"]), e["t"], 1.0)
    elif k == "pluck": add(pluck(e["m"], e["g"]), e["t"], 1.0 * (0.6 + 0.4 * v))
    elif k == "bass":
        key = (e["m"], round(e["g"], 3))
        if key not in PCACHE: PCACHE[key] = mixlen(sub(e["m"], e["g"]) * 0.70, reese(e["m"] + 12, e["g"]) * 0.55)
        add(PCACHE[key], e["t"], 1.15 * v)
    elif k == "chop":
        key = ("c", e["pad"], round(e["g"], 2), e["p"])
        if key not in PCACHE: PCACHE[key] = hp180(crush(chopaudio(e["pad"], e["g"], e["p"]), 14, 16000))
        add(PCACHE[key], e["t"], 2.2 * CH[e["pad"]]["vol"] * (0.5 + 0.5 * v) * (0.45 if e["pad"] == "6" else 1))
    elif k == "extra":
        y = EXTRA[e["i"]]["y"]; y = y[::-1] if e["rev"] else y
        add(fade(hp180(y), 0.01, 0.05), e["t"], 1.6 * e["v"])
# master: a sidechain duck of everything but the drums under every kick (the "duck" of Duck & Cover), tilt, soft clip, level
duck = np.ones(N)
for e in EV:
    if e["kind"] == "kick":
        a = int(e["t"] * BEAT * SR); n = int(0.16 * SR); c = np.linspace(0.45, 1, n) ** 1.5; duck[a:a + n] = np.minimum(duck[a:a + n], c[:max(0, min(n, N - a))])
dr = np.zeros(N)
def render_drums():
    global mix
    keep = mix.copy(); mix[:] = 0
    for e in EV:
        k = e["kind"]; v = e["v"]
        if k == "kick": add(K, e["t"], v)
        elif k == "snare": add(S, e["t"], 0.85 * v)
        elif k == "hat": add(HC, e["t"], v)
        elif k == "ohat": add(HO, e["t"], v)
        elif k == "crash": add(CR, e["t"], 0.7 * v)
    d = mix.copy(); mix[:] = keep; return d
dr = render_drums(); body = (mix - dr) * duck
y = body * 0.9 + dr * 1.0
b, a = signal.butter(2, 28 / (SR / 2), "high"); y = signal.lfilter(b, a, y)
y = np.tanh(y * 1.5) / np.tanh(1.5) * 0.98
tail = int(2.5 * SR); y[-tail:] *= np.linspace(1, 0, tail)               # the outro's end fades
y = y / np.abs(y).max() * 0.94
os.makedirs(OUT, exist_ok=True); sf.write(OUT + "/duck_cover_dnb.wav", y, SR, subtype="PCM_16")
print("rendered %d bars, %.1f s" % (TOTAL / 4, len(y) / SR))

# ------------------------------------------------------------------------------------------------------------ the game stream
tmp = OUT + "/_gba.wav"; sf.write(tmp, y, SR)
x = encode_song.load(tmp); os.remove(tmp)
b4 = encode(x); yy = decode(b4); snr = 10 * np.log10(np.sum(x.astype(float) ** 2) / max(1, np.sum((x - yy).astype(float) ** 2)))
open("source/music/duck_cover.adp", "wb").write(b4); print("source/music/duck_cover.adp: %.0f s, %d KB, ADPCM SNR %.1f dB" % (len(x) / GBA, len(b4) / 1024, snr))
encode_song.add_line("duck_cover", "DUCK & COVER (DNB REVISION)", "source/music/duck_cover.adp")

# ------------------------------------------------------------------------------------------------------------ the tracker module
RATE = 16000
def rel_for(Rs): r = 12 * np.log2(Rs / 8363.0); rel = int(np.floor(r)); return rel, int(round((r - rel) * 128))
def i8(y, peak=0.95):
    y = np.asarray(y, dtype=np.float64); m = np.abs(y).max() or 1.0; return np.clip(np.round(y / m * 127 * peak), -127, 127).astype(np.int8)
def res(y, Rs): return signal.resample_poly(y, Rs, SR)
INS = []; NAMES = {}
def addins(key, name, y, Rs, loop=False, vol=64):
    rel, fine = rel_for(Rs); INS.append(X.instrument(name, i8(res(y, Rs) if Rs != SR else y), loop=loop, rel=rel, fine=fine, vol=vol)); NAMES[key] = len(INS)
addins("kick", "KICK", K, 22050); addins("snare", "SNARE", S, 22050); addins("hat", "HAT", HC, 22050); addins("ohat", "OPEN HAT", HO, 22050); addins("crash", "CRASH", CR, 16000)
addins("pluck", "QUOKKA PLUCK B3", pluck(59, 0.9), RATE); addins("sub", "SUB SAMPLE B1", CH["9"]["y"], 11025)
f0 = 440.0 * 2 ** ((47 - 69) / 12); P = 256; cyc = int(round(SR / f0)); tt = np.arange(P) / P
rw = sum(signal.sawtooth(2 * np.pi * (tt * 1.0) * 1 + ph) for ph in (0.0, 0.7)); rw = np.tanh(rw * 0.7)
INS.append(X.instrument("REESE (B2 CYCLE)", i8(rw), loop=True, rel=int(np.floor(12 * np.log2(f0 * P / 8363.0))) , fine=int(round((12 * np.log2(f0 * P / 8363.0) - np.floor(12 * np.log2(f0 * P / 8363.0))) * 128)))); NAMES["reese"] = len(INS)
for k in sorted(CH, key=int):
    if k == "9": continue
    addins("pad" + k, "PAD %s S%d" % (k, PADS[k]["sampleId"]), CH[k]["y"], 11025 if k == "6" else RATE)
for i, e in enumerate(EXTRA): addins("x%d" % i, "UNUSED %d.%d" % (e["src"], i), e["y"], RATE)
INS.append(X.instrument("RISER 4 BARS", i8(res(riser(16), 11025)), rel=rel_for(11025)[0], fine=rel_for(11025)[1])); NAMES["riser"] = len(INS)
assert len(INS) <= 32, len(INS)

ROWS = int(round(TOTAL * ROWB)) + 32; NCH = 16
grid = [[None] * NCH for _ in range(ROWS)]; busy = [-1] * NCH
def put(ch, row, note, inst, v=64):
    if 0 <= row < ROWS and grid[row][ch] is None: grid[row][ch] = (int(max(1, min(96, round(note)))) if note != 97 else 97, inst, 0x10 + max(1, min(64, int(v))), 0, 0); return True
    return False
def free(row, lo, hi):
    for c in range(lo, hi):
        if busy[c] < row: return c
    return None
C4 = 49
for e in EV:
    k = e["kind"]; row = int(round(e["t"] * ROWB)); v = e["v"]
    if k in ("kick", "snare"): put(0 if k == "kick" else 1, row, C4, NAMES[k], 64 * v)
    elif k in ("hat", "ohat"): put(2, row, C4, NAMES[k], 40 * v)
    elif k == "crash": put(3, row, C4, NAMES["crash"], 44 * v)
    elif k == "riser": put(3, row, C4, NAMES["riser"], 40)
    elif k == "bass":
        gr = max(1, int(round(e["g"] / BEAT * ROWB)))
        if put(4, row, C4 + (e["m"] - 35), NAMES["sub"], 64 * v): put(4, row + gr, 97, 0, 1) if False else None
        c = 5; put(c, row, C4 + (e["m"] + 12 - 47), NAMES["reese"], 30 * v)
        if row + gr < ROWS and grid[row + gr][5] is None: grid[row + gr][5] = (97, 0, 0, 0, 0)
    elif k == "pluck":
        c = free(row, 6, 8)
        if c is None: continue
        if put(c, row, C4 + (e["m"] - 59), NAMES["pluck"], 56 * (0.6 + 0.4 * v)):
            gr = max(1, int(round(e["g"] / BEAT * ROWB))); busy[c] = row + gr - 1
            if row + gr < ROWS and grid[row + gr][c] is None: grid[row + gr][c] = (97, 0, 0, 0, 0)
    elif k == "chop":
        c = free(row, 8, 13)
        if c is None: continue
        d = CH[e["pad"]]; busy[c] = row + int(d["beats"] * ROWB)
        put(c, row, C4 + int(round(e["p"])), NAMES["pad" + e["pad"]], 56 * v)
    elif k == "extra":
        c = free(row, 13, 16)
        if c is None: continue
        busy[c] = row + int(EXTRA[e["i"]]["beats"] * ROWB); put(c, row, C4, NAMES["x%d" % e["i"]], 50 * e["v"])
PLEN = 256; pats = []; orders = []
for p0 in range(0, ROWS, PLEN):
    rows = min(PLEN, ROWS - p0); rows = rows if rows >= 32 else 32
    rr = [[X.cell(*(grid[p0 + r][c] or (0, 0, 0, 0, 0))) if p0 + r < ROWS else X.cell() for c in range(NCH)] for r in range(rows)]
    pats.append((rows, rr)); orders.append(len(pats) - 1)
X.write(OUT + "/duck_cover_dnb.xm", "DUCK & COVER (DNB REVISION)", NCH, pats, INS and orders, INS, speed=3, bpm=int(NEW))
print("module: %d patterns, %d instruments" % (len(pats), len(INS)))

# ------------------------------------------------------------------------------------------------------------ MIDI
mid = mido.MidiFile(ticks_per_beat=480); tr0 = mido.MidiTrack(); tr0.append(mido.MetaMessage("set_tempo", tempo=mido.bpm2tempo(NEW), time=0)); tr0.append(mido.MetaMessage("track_name", name="DUCK & COVER (DNB REVISION) 174 BPM, E minor", time=0)); mid.tracks.append(tr0)
def track(name, ch, prog, evs):
    tr = mido.MidiTrack(); tr.append(mido.MetaMessage("track_name", name=name, time=0))
    if ch != 9: tr.append(mido.Message("program_change", channel=ch, program=prog, time=0))
    ms = []
    for t0, t1, n, vel in evs: ms.append((int(round(t0 * 480)), 1, n, vel)); ms.append((int(round(t1 * 480)), 0, n, 0))
    ms.sort(key=lambda m: (m[0], m[1])); last = 0
    for tk, on, n, vel in ms: tr.append(mido.Message("note_on" if on else "note_off", channel=ch, note=int(n), velocity=int(vel), time=tk - last)); last = tk
    mid.tracks.append(tr)
G = lambda e: e["g"] / BEAT
track("QUOKKA PLUCK", 0, 81, [(e["t"], e["t"] + G(e), e["m"], 40 + 80 * e["v"]) for e in EV if e["kind"] == "pluck"])
track("SUB + REESE", 1, 38, [(e["t"], e["t"] + G(e), e["m"], 110) for e in EV if e["kind"] == "bass"])
DM = dict(kick=36, snare=38, hat=42, ohat=46, crash=49)
track("DRUMS", 9, 0, [(e["t"], e["t"] + 0.1, DM[e["kind"]], 30 + 97 * e["v"]) for e in EV if e["kind"] in DM])
track("SAMPLE CUES (note = instrument)", 2, 54, [(e["t"], e["t"] + 0.5, 48 + NAMES.get("pad" + e["pad"], 0), 100) for e in EV if e["kind"] == "chop"] + [(e["t"], e["t"] + 0.5, 48 + NAMES["x%d" % e["i"]], 90) for e in EV if e["kind"] == "extra"])
mid.save(OUT + "/duck_cover_dnb.mid")
json.dump(dict(bpm=NEW, original_bpm=OLD, bars=TOTAL / 4, sections=POS, events=len(EV), extras=[dict(src=e["src"], beats=e["beats"]) for e in EXTRA]), open(OUT + "/duck_cover_dnb.json", "w"), indent=1)
