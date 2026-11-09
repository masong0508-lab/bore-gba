#!/usr/bin/env python3
"""DUCK & COVER (DNB REVISION): the Koala Sampler project "Duck & Cover" (145 BPM) transcribed and rebuilt as a drum & bass banger at 174 BPM, as a TRACKER song.

usage: python3 tools/make_duckcover.py path/to/Duck__Cover.koala        (run from the project root; needs numpy + scipy only, so it runs in Termux)

What it does
  * reads the Koala project: sequence.json (6 filled pattern slots played one after the other, 16384 ticks per bar = 4096 per beat; every note: pad, pitch, length, velocity) and
    sampler/*.wav (12 sample pads + the QUOKKA synth pad 1, a filtered saw pluck whose envelope / cutoff / drive settings are copied from sampler.json)
  * NOTES AND ENVELOPES ARE KEPT AS DATA: every note becomes a tracker note (volume column = velocity x pad volume). The tracker player ignores note-off and envelopes, so each
    instrument has its envelope baked in: the pluck's amp envelope (attack / decay / sustain / release) and its falling filter, the sample pads' attack / release, and a note rings
    until the next note on its channel
  * the sampled chops (the vocal bits and the break) are TIME-FITTED to 174 BPM (pitch kept, WSOLA from tools/xm2gba.py): stretched to a whole number of half beats of 174 BPM;
    the pad that is 20 s long is stretched by exactly 145/174. They (and the "unused" chunks) are stored 4-BIT: every sample goes through the game's own 4-bit IMA-ADPCM
    coder (tools/encode_sfx.py) and back, so what the tracker holds is exactly what the 4-bit coder keeps
  * "whatever is unused" is used: the parts of the recordings no pad plays (before / after the pad's start-end window) are cut into 2-beat chunks, the loudest (up to 11; this project
    has only a few that are not silent) become instruments: reverse swells into every section, offbeat stabs in the last section
  * new drum & bass drums (kick, snare, ghost snares, hats, crash, riser) and a heavy bass (the sampled sub + a reese)
  * outputs: tools/duckcover/duck_cover_dnb.xm (16 channels, up to 32 instruments, speed 3 / 174 BPM: 8 rows a beat), duck_cover_dnb.mid (the notes), duck_cover_dnb.wav (a
    preview made from the same note grid), and one SONG_XM line in source/songs.h (then run python3 tools/xm2gba.py)
"""
import os, sys, json, zipfile, io, struct
import numpy as np
from scipy import signal
from scipy.io import wavfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xmwrite as X
from encode_sfx import encode, decode
from xm2gba import wsola_fit

SR = 44100; OLD = 145.0; NEW = 174.0; BEAT = 60.0 / NEW; RATE = 18157
OUT = "tools/duckcover"; ROWB = 8                       # rows per beat (1/32 notes at speed 3, XM BPM 174)
SID, TITLE, XMP = "duck_cover_dnb", "DUCK & COVER", "tools/duckcover/duck_cover_dnb.xm"
VOX_SEMI = -1                                          # the vocal / sampled chops sat a semitone above the song: every chop note is played this many semitones lower (0 = as in Koala)
rng = np.random.default_rng(7)
if len(sys.argv) < 2: sys.exit(__doc__)

# ------------------------------------------------------------------------------------------------------------ the project
z = zipfile.ZipFile(sys.argv[1]); SEQ = json.loads(z.read("sequence.json")); SJ = json.loads(z.read("sampler/sampler.json"))
PADS = {p["pad"]: p for p in SJ["pads"]}

def load_wav(sid):
    r, d = wavfile.read(io.BytesIO(z.read("sampler/%d.wav" % sid)))
    d = d.astype(np.float64)
    d = d / (2.0 ** 31 if d.max() > 2 ** 24 or d.min() < -2 ** 24 else 2.0 ** 15 if np.abs(d).max() > 2.0 else 1.0)
    if d.ndim > 1: d = d.mean(1)
    return r, d
def rs(y, fo, fi):
    if fo == fi: return y
    from math import gcd
    g = gcd(int(fo), int(fi)); return signal.resample_poly(y, int(fo) // g, int(fi) // g)
def lp(x, f, o=2, sr=SR): return signal.sosfilt(signal.butter(o, min(f, sr * .45), "lp", fs=sr, output="sos"), x)
def hp(x, f, o=2, sr=SR): return signal.sosfilt(signal.butter(o, f, "hp", fs=sr, output="sos"), x)
def bp(x, a, b, o=2, sr=SR): return signal.sosfilt(signal.butter(o, [a, min(b, sr * .45)], "bp", fs=sr, output="sos"), x)
def noise(n): return rng.standard_normal(n)
def tt(n, sr=SR): return np.arange(n) / sr
def fin(y, peak=0.95): m = np.abs(y).max(); return y / m * peak if m > 0 else y

def q4(y, Rs):
    """through the game's 4-bit IMA-ADPCM coder and back (y: floats, -1..1, at Rs Hz): what the tracker keeps is what 4 bits keep"""
    pk = np.abs(y).max() or 1.0; x = np.round(y / pk * 30000).astype(np.int32)
    d = decode(encode(x))[:len(x)].astype(np.float64)
    return d / (np.abs(d).max() or 1.0) * pk * (np.abs(y).max() > 0)

# ------------------------------------------------------------------------------------------------------------ the chops, time-fitted
def fitted(y, sr_in, beats_old, release=0.0):
    """y at sr_in = one pad's window. beats_old = its length in beats of 145 BPM. -> (samples at RATE, length in beats at 174 BPM)"""
    y = rs(y, RATE, sr_in); tb = max(0.5, round(beats_old * 2) / 2)              # a whole number of half beats
    n_out = int(round(tb * BEAT * RATE)); y = wsola_fit(y, n_out, RATE)
    f = min(len(y) // 3, max(24, int(release * RATE))); y[-f:] *= np.linspace(1, 0, f)   # the pad's own release
    return y, tb
CH = {}
for k, p in PADS.items():
    if p["type"] != "sample" or k == "9": continue
    r, w = load_wav(p["sampleId"]); seg = w[p["start"]:p["end"]]
    if p["reverse"] == "true": seg = seg[::-1]
    beats_old = len(seg) / r / (60.0 / OLD)
    if p["pad"] == "6": tb = beats_old * OLD / NEW / 1.0; y = rs(seg, 11025, r); y = wsola_fit(y, int(round(len(seg) / r * OLD / NEW * 11025)), 11025); y = np.concatenate([y[:-200], y[-200:] * np.linspace(1, 0, 200)]); CH[k] = dict(y=q4(y, 11025), beats=tb, sr=11025)   # the 20 s break: exactly 145/174
    else:
        y, tb = fitted(seg, r, beats_old, p["release"])
        if p["looping"] == "true" and p["reverse"] == "true": y = np.concatenate([y] * 2)[:int(2 * BEAT * RATE * round(len(y) / RATE / BEAT / 2 + .49))] if len(y) < BEAT * RATE * 2 else y   # the reversed loop pad: a long swell
        CH[k] = dict(y=q4(y, RATE), beats=len(y) / RATE / BEAT, sr=RATE)
    CH[k]["vol"] = min(p["vol"], 2.0) / 2.0; CH[k]["att"] = p["attack"]

r9, w9 = load_wav(PADS["9"]["sampleId"]); SUB = fin(w9[PADS["9"]["start"]:PADS["9"]["end"]]); SUBVOL = PADS["9"]["vol"]   # the pitched 62 Hz sub (B1)
SUB = SUB * np.minimum(1, (len(SUB) - np.arange(len(SUB))) / 300.0)

# ------------------------------------------------------------------------------------------------------------ the unused parts of the recordings
CB = 2; CHUNK = int(CB * 60.0 / OLD * 48000); cands = []   # 2-beat chunks
for k, p in PADS.items():
    if p["type"] != "sample" or k == "9": continue
    r, w = load_wav(p["sampleId"])
    for a, b in ((0, p["start"]), (p["end"], len(w))):
        for s in range(a, b - CHUNK + 1, CHUNK):
            c = w[s:s + CHUNK]; cands.append((float(np.sqrt((c ** 2).mean())), int(k), s, c))
cands.sort(key=lambda t: -t[0]); EXTRA = []; used_pad = {}
for rms_, k, s, c in cands:                          # the loudest, at most 3 per pad so they do not all come from one recording
    if rms_ < 0.01 or len(EXTRA) >= 11 or used_pad.get(k, 0) >= 2: continue
    used_pad[k] = used_pad.get(k, 0) + 1
    y = wsola_fit(rs(c, RATE, 48000), int(round(CB * BEAT * RATE)), RATE)
    EXTRA.append(dict(src=k, y=y))
for i, e in enumerate(EXTRA):
    y = e["y"]
    if i < 2: y = y[::-1] * np.linspace(0.15, 1, len(y)) ** 1.5; y[-int(.01 * RATE):] *= np.linspace(1, 0, int(.01 * RATE))   # reverse swells
    else: y = y[:int(1.0 * BEAT * RATE)] * np.exp(-tt(int(1.0 * BEAT * RATE), RATE) / 0.18); y[:60] *= np.linspace(0, 1, 60)             # offbeat stabs
    e["y"] = q4(y, RATE); e["rev"] = i < 2
print("pads", sorted(CH, key=int), "| unused chunks used:", [(e["src"], round(len(e["y"]) / RATE / BEAT, 1)) for e in EXTRA])

# ------------------------------------------------------------------------------------------------------------ the synth: QUOKKA pluck, with its envelope baked in
SP = PADS["1"]["synthParams"]
def pluck(m, dur=1.7):
    f = 440.0 * 2 ** ((m - 69) / 12); n = int(dur * SR); t = tt(n)
    ph = np.cumsum(np.full(n, f) / SR); x = (signal.sawtooth(2 * np.pi * ph) + signal.sawtooth(2 * np.pi * ph * 1.004) * .6 + np.sin(2 * np.pi * ph * .5) * .5) / 2.1
    cut = SP["cutoff"] * (1 + 9 * SP["modEnvDepth"] * 4 * np.exp(-t / max(SP["modEnvDecay"], .01)))           # the filter envelope: opens on the hit, falls
    out = np.zeros(n); zi = None; B = 128
    for i in range(0, n, B):
        sos = signal.butter(2, float(np.clip(cut[i], 120, 9000)), "lp", fs=SR, output="sos")
        if zi is None: zi = signal.sosfilt_zi(sos) * x[i]
        out[i:i + B], zi = signal.sosfilt(sos, x[i:i + B], zi=zi)
    a = SP["ampEnvAttack"]; s = SP["ampEnvSustain"]; d = SP["ampEnvDecay"]; rel = SP["ampEnvRelease"]
    env = np.minimum(1, t / max(a, 1e-3)) * (s + (1 - s) * np.exp(-t / (d / 3)))                                # attack, decay to sustain
    env *= np.minimum(1, np.maximum(0, (dur - t) / rel))                                                         # release
    y = np.tanh(out * env * (1 + 3 * SP["drive"])); return hp(y, 98, 2)

# ------------------------------------------------------------------------------------------------------------ the new drums, bass and risers
def kick():
    n = int(.30 * SR); t = tt(n)
    return fin(np.tanh(1.9 * np.sin(2 * np.pi * np.cumsum(46 + 200 * np.exp(-t / .013)) / SR)) * np.exp(-t / .11) + hp(noise(n), 4000) * np.exp(-t / .002) * .6)
def snare():
    n = int(.24 * SR); t = tt(n)
    return fin(np.tanh(2.4 * (bp(noise(n), 1600, 7800) * np.exp(-t / .075) + (np.sin(2 * np.pi * 220 * t) + .5 * np.sin(2 * np.pi * 400 * t)) * np.exp(-t / .03))))
def hat(o=False):
    n = int((.28 if o else .05) * SR); return fin(hp(noise(n) * np.exp(-tt(n) / (.09 if o else .009)), 7500, 4))
def crash(): n = int(1.4 * SR); return fin(hp(noise(n), 4500, 3) * np.exp(-tt(n) / .5))
def riser(beats=16):
    n = int(beats * BEAT * SR); u = np.linspace(0, 1, n); x = np.zeros(n)
    for i in range(0, n, 256): x[i:i + 256] = bp(noise(len(x[i:i + 256])), 250 + 3600 * u[i] ** 2, 600 + 4300 * u[i] ** 2, 2)
    return fin(x * u ** 1.5)
def reese():
    f0 = 440.0 * 2 ** ((47 - 69) / 12); n = int(1.2 * SR11); t = tt(n, SR11)
    x = sum(signal.sawtooth(2 * np.pi * np.cumsum(np.full(n, f0 * 2 ** (c / 1200)) * (1 + .0025 * np.sin(2 * np.pi * 3.1 * t))) / SR11) for c in (-17, 0, 19)) / 3
    y = np.tanh(2.0 * lp(x, 700, 2, SR11)) * np.minimum(1, t / .004) * np.minimum(1, (t[-1] - t) / .06); return fin(y)
SR11 = 11025

# ------------------------------------------------------------------------------------------------------------ the instruments (key -> tracker instrument number)
INS = []; NAMES = {}; SAMP = {}                      # SAMP: key -> (float samples, rate), what the preview plays
def rel_for(Rs): r = 12 * np.log2(Rs / 8363.0); rel = int(np.floor(r)); return rel, int(round((r - rel) * 128))
def i8(y, peak=0.95): y = np.asarray(y, float); m = np.abs(y).max() or 1.0; return np.clip(np.round(y / m * 127 * peak), -127, 127).astype(np.int8)
def addins(key, name, y, Rs, from_sr=None, vol=64):
    y = np.asarray(y, float)
    if from_sr and from_sr != Rs: y = rs(y, Rs, from_sr)
    rel, fine = rel_for(Rs); INS.append(X.instrument(name, i8(y), rel=rel, fine=fine, vol=vol)); NAMES[key] = len(INS); SAMP[key] = (i8(y).astype(float) / 127, Rs)
addins("kick", "KICK", kick(), 22050, SR); addins("snare", "SNARE", snare(), 22050, SR); addins("hat", "HAT", hat(), 22050, SR); addins("ohat", "OPEN HAT", hat(True), 22050, SR)
addins("crash", "CRASH", crash(), 16000, SR)
addins("pluck", "QUOKKA PLUCK B3", pluck(59), RATE, SR); addins("sub", "SUB SAMPLE B1", SUB, 11025, r9)
addins("reese", "REESE B2", reese(), SR11)
for k in sorted(CH, key=int): addins("pad" + k, "PAD %s S%d" % (k, PADS[k]["sampleId"]), CH[k]["y"], CH[k]["sr"])
for i, e in enumerate(EXTRA): addins("x%d" % i, "UNUSED %d.%d" % (e["src"], i), e["y"], RATE)
addins("riser", "RISER 4 BARS", riser(16), 11025, SR)
assert len(INS) <= 32, len(INS)

# ------------------------------------------------------------------------------------------------------------ the arrangement: events in beats
SQ = [s for s in SEQ["sequences"] if s and s.get("pattern") and s["pattern"].get("notes")]
EV = []; sec = []; off = 0.0
for si, s in enumerate(SQ):
    nb = s["pattern"]["numBars"]; sec.append((off, nb)); notes = s["pattern"]["notes"]
    has9 = any(n["num"] == 9 for n in notes)
    for n in notes:
        t = off + (n["timeOffset"] + n["start"]) / 4096.0; g = max(.25, n["length"] / 4096.0 * OLD / NEW); v = n["vel"] / 127.0; p = n["pitch"]; k = n["num"]
        if k == 1:
            EV.append(dict(kind="pluck", t=t, m=59 + p, g=g, v=v))
            if not has9 and abs(((t - off) % 2.0)) < 1e-6: EV.append(dict(kind="bass", t=t, m=35 + p, g=2.0, v=.9))     # no sub pad in these sections: the pluck's root, two beats long
        elif k == 9: EV.append(dict(kind="bass", t=t, m=35 + p, g=g, v=min(1.0, v * SUBVOL)))
        else:
            if str(k) not in CH: continue
            d = CH[str(k)]; EV.append(dict(kind="chop", t=t, pad=str(k), p=p, v=min(1.0, v * (0.55 + d["vol"])), beats=d["beats"]))
    off += nb * 4.0
TOTAL = off
# drums, section by section
LVL = [None] * len(sec)
for si, (o, nb) in enumerate(sec):
    EV.append(dict(kind="crash", t=o, v=.9))
    for b in range(nb):
        t0 = o + b * 4.0; last = b == nb - 1; first_half = si == 0 and b < 4; brk = si == 2 and b < 4; outro = si == len(sec) - 1
        if outro and b >= nb - 2: continue
        for st in range(16):                           # 16ths: kick 1 and the "a" of 2, snare on 2 and 4, ghost snares, hats
            t = t0 + st / 4.0
            if first_half:
                if st % 2 == 0: EV.append(dict(kind="hat", t=t, v=.35 + .25 * (st % 4 == 0)))
                if st == 0 and b % 2 == 0: EV.append(dict(kind="kick", t=t, v=.8))
                continue
            if brk:                                    # half-time breakdown: kick on 1, snare on 3
                if st == 0: EV.append(dict(kind="kick", t=t, v=.9))
                if st == 8: EV.append(dict(kind="snare", t=t, v=.9))
                if st % 4 == 2: EV.append(dict(kind="hat", t=t, v=.35))
                continue
            if st in (0, 10) or (st == 11 and b % 4 == 3): EV.append(dict(kind="kick", t=t, v=1.0 if st == 0 else .85))
            if st in (4, 12): EV.append(dict(kind="snare", t=t, v=1.0))
            if st in (7, 15) and b % 2 == 1: EV.append(dict(kind="snare", t=t, v=.28))      # ghost snares
            if st % 2 == 0: EV.append(dict(kind="hat", t=t, v=.55 if st % 4 == 0 else .38))
            elif st % 4 == 3 and b % 2 == 0: EV.append(dict(kind="hat", t=t, v=.22))
            if st == 14: EV.append(dict(kind="ohat", t=t, v=.45))
        if last and not outro:                          # fill into the next section: a snare roll in the last beat
            for st in range(12, 16): EV.append(dict(kind="snare", t=t0 + st / 4.0 + .125, v=.5 + .12 * (st - 12)))
        if b % 8 == 0 and b and not first_half: EV.append(dict(kind="crash", t=t0, v=.7))
    if si in (1, 3): EV.append(dict(kind="riser", t=o + nb * 4.0 - 16.0, v=1.0))                # a 4-bar riser into the drop
    if si + 1 < len(sec) and EXTRA:                               # a reverse swell into every section (the unused chunks)
        EV.append(dict(kind="extra", t=o + nb * 4.0 - CB, x=si % min(2, len(EXTRA)), v=.8))
    if si == 4 and len(EXTRA) > 2:                      # the longest section: offbeat stabs from the unused recordings
        for b in range(2, nb - 1):
            for q in ((1.5, 3.5) if b % 2 else (2.5,)): EV.append(dict(kind="extra", t=o + b * 4.0 + q, x=2 + (b + int(q)) % (len(EXTRA) - 2), v=.5))
EV.sort(key=lambda e: e["t"])

# ------------------------------------------------------------------------------------------------------------ the note grid (16 channels, 8 rows a beat)
ROWS = int(round(TOTAL * ROWB)) + 32; ROWS = (ROWS + 63) // 64 * 64; NCH = 16
grid = [[None] * NCH for _ in range(ROWS)]; busy = [-1] * NCH
C4 = 49
def put(ch, row, note, inst, v=64):
    if 0 <= row < ROWS and grid[row][ch] is None: grid[row][ch] = (int(note), inst, 0x10 + max(1, min(64, int(v))), 0, 0); return True
    return False
def free(row, lo, hi):
    for c in range(lo, hi):
        if busy[c] < row: return c
    return None
def clampn(n): return max(1, min(96, int(n)))
for e in EV:
    k = e["kind"]; row = int(round(e["t"] * ROWB)); v = e.get("v", 1.0)
    if k in ("kick", "snare"): put(0 if k == "kick" else 1, row, C4, NAMES[k], 64 * v)
    elif k in ("hat", "ohat"): put(2, row, C4, NAMES[k], 40 * v)
    elif k == "crash": put(3, row, C4, NAMES["crash"], 44 * v)
    elif k == "riser": put(3, row, C4, NAMES["riser"], 40)
    elif k == "bass":
        put(4, row, clampn(C4 + (e["m"] - 35)), NAMES["sub"], 64 * v)                                    # the sampled sub (B1 = its own pitch)
        put(5, row, clampn(C4 + (e["m"] + 12 - 47)), NAMES["reese"], 26 * v)                              # the reese, an octave above the sub's pitch name
        gr = max(1, int(round(e["g"] * ROWB)))
        if (row // 8) % 4 == 3 and gr >= 8: put(5, row + gr // 2, clampn(C4 + (e["m"] + 24 - 47)), NAMES["reese"], 22 * v)   # octave jumps
    elif k == "pluck":
        c = free(row, 6, 8)
        if c is None: continue
        if put(c, row, clampn(C4 + (e["m"] - 59)), NAMES["pluck"], 56 * (0.6 + 0.4 * v)): busy[c] = row + max(1, int(round(e["g"] * ROWB))) - 1
    elif k == "chop":
        c = free(row, 8, 13)
        if c is None: continue
        if put(c, row, clampn(C4 + int(round(e["p"])) + VOX_SEMI), NAMES["pad" + e["pad"]], 56 * v): busy[c] = row + int(e["beats"] * ROWB)
    elif k == "extra":
        c = free(row, 13, 16)
        if not EXTRA or c is None or e["x"] >= len(EXTRA): continue
        if put(c, row, C4, NAMES["x%d" % e["x"]], 52 * v): busy[c] = row + int(CB * ROWB if EXTRA[e["x"]]["rev"] else 1 * ROWB)

os.makedirs(OUT, exist_ok=True)
pats = []; orders = []
for p0 in range(0, ROWS, 64):
    pats.append((64, [[X.cell(*(grid[r][c] or (0, 0, 0, 0, 0))) for c in range(NCH)] for r in range(p0, p0 + 64)])); orders.append(len(pats) - 1)
X.write(XMP, "DUCK & COVER", NCH, pats, orders, INS, speed=3, bpm=int(NEW))
print("wrote %s: %d rows, %d patterns, %d instruments, %.0f s" % (XMP, ROWS, len(pats), len(INS), ROWS / ROWB * BEAT))

# ------------------------------------------------------------------------------------------------------------ MIDI of the notes
def vlq(n):
    b = [n & 127]; n >>= 7
    while n: b.append(128 | (n & 127)); n >>= 7
    return bytes(reversed(b))
TPQ = 480; ev = []
GM = {"kick": 36, "snare": 38, "hat": 42, "ohat": 46, "crash": 49, "riser": 51}
for e in EV:
    k = e["kind"]; t = int(round(e["t"] * TPQ)); vel = int(max(1, min(127, 100 * e.get("v", 1))))
    if k in GM: ev += [(t, 0x99, GM[k], vel), (t + 60, 0x89, GM[k], 0)]
    elif k == "bass": ev += [(t, 0x91, e["m"], vel), (t + int(e["g"] * TPQ), 0x81, e["m"], 0)]
    elif k == "pluck": ev += [(t, 0x92, e["m"], vel), (t + int(e["g"] * TPQ), 0x82, e["m"], 0)]
    elif k == "chop": ev += [(t, 0x93, 60 + int(round(e["p"])) + VOX_SEMI, vel), (t + int(e["beats"] * TPQ), 0x83, 60 + int(round(e["p"])) + VOX_SEMI, 0)]
ev.sort(key=lambda x: (x[0], x[1] & 0xF0 == 0x90)); trk = bytearray(b"\x00\xff\x51\x03" + struct.pack(">I", int(60e6 / NEW))[1:]); last = 0
for t, st, a, b in ev: trk += vlq(t - last) + bytes([st, int(a) & 127, int(b)]); last = t
trk += b"\x00\xff\x2f\x00"
open(os.path.join(OUT, "duck_cover_dnb.mid"), "wb").write(b"MThd" + struct.pack(">IHHH", 6, 0, 1, TPQ) + b"MTrk" + struct.pack(">I", len(trk)) + bytes(trk))

# ------------------------------------------------------------------------------------------------------------ the preview, played from the same grid
n_out = int(ROWS / ROWB * BEAT * SR) + SR; mix = np.zeros(n_out); rowt = BEAT / ROWB
for c in range(NCH):
    cur = [(r, grid[r][c]) for r in range(ROWS) if grid[r][c]]
    for j, (r, (note, inst, vol, _, _)) in enumerate(cur):
        key = [k for k, v in NAMES.items() if v == inst][0]; y, Rs = SAMP[key]
        ratio = Rs / SR * 2 ** ((note - C4) / 12.0); end = (cur[j + 1][0] if j + 1 < len(cur) else ROWS) * rowt
        nsrc = min(len(y) - 1, int((end - r * rowt) * SR * ratio)); idx = np.arange(int(nsrc / ratio)) * ratio
        seg = np.interp(idx, np.arange(len(y)), y) * ((vol - 0x10) / 64.0); f = min(len(seg), 200); seg[-f:] *= np.linspace(1, 0, f)
        s0 = int(r * rowt * SR); mix[s0:s0 + len(seg)] += seg[:max(0, n_out - s0)]
mix = np.tanh(mix / (np.abs(mix).max() or 1) * 1.6) / np.tanh(1.6)
wavfile.write(os.path.join(OUT, "duck_cover_dnb.wav"), SR, (mix * 30000).astype(np.int16))
print("preview: peak %.2f, rms %.3f" % (np.abs(mix).max(), np.sqrt((mix ** 2).mean())))

# ------------------------------------------------------------------------------------------------------------ the game: one SONG_XM line
SH = "source/songs.h"
if os.path.exists(SH):
    t = open(SH, encoding="utf-8").read()
    if "SONG_XM(%s," % SID in t: print("  %s already lists %s" % (SH, SID))
    else:
        with open(SH, "a", encoding="utf-8") as f:
            f.write(("" if t.endswith("\n") else "\n") + '// DUCK & COVER: a Koala Sampler project (145 BPM) rebuilt as a 174 BPM drum & bass tracker song (tools/make_duckcover.py): the sampled chops are 4-bit ADPCM-quality samples\n')
            f.write('SONG_XM(%s,"%s","%s")\n' % (SID, TITLE, XMP))
        print("  added to", SH)
    print("now run: python3 tools/xm2gba.py")
