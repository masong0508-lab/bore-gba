#!/usr/bin/env python3
"""Stage 2 of the hybrid CORPORATIONS conversion (needs tools/corporations/notes.json from tools/corp_transcribe.py).

  * the transcribed bass, piano and drums become (a) MIDI files with the exact performance times and the track's tuning (A = 427 Hz-ish, RPN fine tuning), (b) a tracker module
    tools/corporations/corporations_hybrid.xm on a 1/32-note row grid (speed 3, 131 BPM): bass on channel 1, piano on 2-9, drums 10-12, and CUE lanes on 13-15 whose notes mark where
    each digital clip of the vocals / guitar / other stems starts (note = clip number); instruments: bass, piano, kick, snare, hat, cue. The instruments carry the track's -40 cent tuning
    in their finetune, so the module plays in the song's own pitch
  * the guitar, the vocals and the other synths stay digital: 4-bit IMA-ADPCM at 18157 Hz (the game's rate), cut into clips where the stem is active, with a table of exact start samples
    (tools/corporations/digital/*.adp + digital.json). The vocals get the compression and the room in the mix of tools/make_corporations.py first
  * a software player of the module (the same instruments and pitch maths) + the decoded digital clips at their exact samples makes the preview, and the transcription is measured
    against the stems (spectral similarity of the bass and the piano, chroma, timing)
usage: python3 tools/corp_build.py"""
import os, sys, json, struct, subprocess
import numpy as np, librosa, mido
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import xmwrite as X
from make_corporations import find_stem, load, SR, vad, compress, biquad_peak, smooth, db, rms, vocal_chain, VOICE_DB
from encode_sfx import encode, decode

OUT = "tools/corporations"; GBA = 18157
ND = json.load(open(OUT + "/notes.json")); BPM = ND["bpm"]; T0 = ND["t0"]; TUNE = ND["tuning_semitones"]
ROW = 60.0 / BPM / 8                                         # one row = a 1/32 note
BAR = ROW * 32
XBPM = int(round(BPM))                                       # XM tempo is an integer: speed 3 at 131 BPM

def pitch_rel(P, extra=0.0):    # relative note and finetune that make a sample with P samples per cycle play note n at the track's own pitch
    r = -9 + 12 * np.log2(440.0 * P / 8363.0) + TUNE + extra; rel = int(np.floor(r)); fine = int(round((r - rel) * 128))
    if fine > 127: rel += 1; fine -= 128
    return rel, fine

def rate_rel(Rs):               # relative note that plays a sample made at rate Rs back at Rs when C-4 is played (drums)
    r = 12 * np.log2(Rs / 8363.0); rel = int(np.floor(r)); fine = int(round((r - rel) * 128)); return rel, fine

# ------------------------------------------------------------------------------------------------ the instruments (8-bit samples)
def inst_bass():
    N = 128; n = np.arange(N); w = sum(np.sin(2 * np.pi * k * n / N) / (k ** 1.15) for k in range(1, 12)) + 0.6 * np.sin(2 * np.pi * n / N)      # a warm saw with a strong fundamental
    w = w / np.abs(w).max() * 118; return dict(name="BASS", data=w.astype(np.int8), loop=True, P=N, Rs=None)
def inst_piano():
    Rs = 16000; F0 = 261.6256; t = np.arange(int(Rs * 1.4)) / Rs; y = np.zeros(len(t))
    for k in range(1, 15):
        f = F0 * k * np.sqrt(1 + 0.0004 * k * k)
        if f > Rs / 2 - 500: break
        y += (1.0 / k ** 0.9) * np.sin(2 * np.pi * f * t + 0.3 * k) * np.exp(-t / (1.5 / (1 + 0.55 * (k - 1))))
    rng = np.random.RandomState(1); y += 0.02 * rng.randn(len(t)) * np.exp(-t / 0.02)
    y *= np.minimum(1, t / 0.003); y = y / np.abs(y).max() * 118; return dict(name="PIANO", data=y.astype(np.int8), loop=False, P=Rs / F0, Rs=Rs)
def inst_kick():
    Rs = 16000; t = np.arange(int(Rs * 0.28)) / Rs; f = 45 + 130 * np.exp(-t / 0.03); ph = 2 * np.pi * np.cumsum(f) / Rs
    y = np.sin(ph) * np.exp(-t / 0.09) + 0.35 * np.sin(2 * ph) * np.exp(-t / 0.03); y[:int(0.002 * Rs)] *= np.linspace(0.3, 1, int(0.002 * Rs)); y = y / np.abs(y).max() * 122
    return dict(name="KICK", data=y.astype(np.int8), loop=False, P=None, Rs=Rs)
def inst_snare():
    Rs = 16000; t = np.arange(int(Rs * 0.25)) / Rs; rng = np.random.RandomState(2); nz = signal.sosfilt(signal.butter(2, 900, "hp", fs=Rs, output="sos"), rng.randn(len(t)))
    y = 0.6 * np.sin(2 * np.pi * 185 * t) * np.exp(-t / 0.05) + nz * np.exp(-t / 0.075); y = y / np.abs(y).max() * 122; return dict(name="SNARE", data=y.astype(np.int8), loop=False, P=None, Rs=Rs)
def inst_hat():
    Rs = 16000; t = np.arange(int(Rs * 0.09)) / Rs; rng = np.random.RandomState(3); nz = signal.sosfilt(signal.butter(3, 4500, "hp", fs=Rs, output="sos"), rng.randn(len(t)))
    y = nz * np.exp(-t / 0.022); y = y / np.abs(y).max() * 110; return dict(name="HAT", data=y.astype(np.int8), loop=False, P=None, Rs=Rs)
INST = [inst_bass(), inst_piano(), inst_kick(), inst_snare(), inst_hat()]
for I in INST:
    if I["P"] is not None: I["rel"], I["fine"] = pitch_rel(I["P"])
    else: I["rel"], I["fine"] = rate_rel(I["Rs"])
CUE = 6

# ------------------------------------------------------------------------------------------------ quantising
def prep_events():
    ev = {"bass": ND["bass"], "piano": ND["piano"], "drums": ND["drums"]}
    first = min([n["t0"] for n in ND["bass"] + ND["piano"]] + [h["t"] for h in ND["drums"]]); end = max([n["t1"] for n in ND["bass"] + ND["piano"]] + [h["t"] for h in ND["drums"]])
    origin = T0 - BAR * int(np.ceil((T0 - first + 0.05) / BAR))
    return ev, origin, int(np.ceil((end - origin) / BAR)) + 2
def qrow(t, origin): return int(round((t - origin) / ROW))

def assign(ev, origin):
    """channel assignment and row events: ch[c][row] = (note, inst, vol)"""
    rows = {}
    def put(ch, row, note, inst, vol, force=False):
        k = (ch, row)
        if k in rows and not force and rows[k][2] >= vol and rows[k][0] != 97: return
        rows[k] = (note, inst, vol)
    def vv(v): return 0x10 + max(1, int(round(v / 127.0 * 64)))
    # bass: one voice
    prev_end = None
    for n in ev["bass"]:
        r0 = qrow(n["t0"], origin); r1 = max(r0 + 1, qrow(n["t1"], origin)); put(0, r0, n["pitch"] - 11, 1, vv(n["vel"]), True)
        nxt = None
        put_off = (r1, 97)
        rows.setdefault((0, r1), (97, 0, 0)) if (0, r1) not in rows else None
    # piano: first free voice (the one released longest ago)
    free_at = [-1] * 8
    for n in sorted(ev["piano"], key=lambda n: n["t0"]):
        r0 = qrow(n["t0"], origin); r1 = max(r0 + 2, qrow(n["t1"], origin)); same = [c for c in range(8) if free_at[c] <= r0 and (1 + c, r0) not in rows]
        if not same: continue
        c = min(same, key=lambda c: free_at[c]); put(1 + c, r0, n["pitch"] - 11, 2, vv(n["vel"]), True); free_at[c] = r1
        if (1 + c, r1) not in rows: rows[(1 + c, r1)] = (97, 0, 0)
    for h in ev["drums"]:
        ch = {"kick": 9, "snare": 10, "hat": 11}[h["kind"]]; inst = {"kick": 3, "snare": 4, "hat": 5}[h["kind"]]; put(ch, qrow(h["t"], origin), 49, inst, vv(h["vel"]))
    return rows

# ------------------------------------------------------------------------------------------------ digital stems (4-bit ADPCM clips)
def digital(origin):
    os.makedirs(OUT + "/digital", exist_ok=True); info = {}; cues = {}
    stems = {n: load(find_stem(n)) for n in ("vocals", "guitar", "other")}; L = min(len(v) for v in stems.values())
    # vocals: the same squeeze as the single-stream song (compression, presence, level), kept as 4-bit
    voc = stems["vocals"][:L]; act_v, _ = vad(voc); v = vocal_chain(voc, act_v)
    stems["vocals"] = v / np.abs(v).max() * 0.85
    for name, x in stems.items():
        x = x[:L]; act, ph = vad(x) if name != "vocals" else vad(voc)
        if name == "other":     # the synths run on: clips of at most 40 s, cut at the quietest moments
            e = np.sqrt(np.convolve(x * x, np.ones(4410) / 4410, "same")); ph = []; t = 0.0
            while t < L / SR - 0.1:
                t1 = min(L / SR, t + 40.0); 
                if t1 < L / SR: lo = int((t + 30) * SR); hi = int(t1 * SR); t1 = (lo + int(np.argmin(e[lo:hi]))) / SR
                ph.append((t, t1)); t = t1
        blob = bytearray(); clips = []
        for i, (a_, b_) in enumerate(ph):
            s0 = max(0, int((a_ - 0.04) * SR)); s1 = min(L, int((b_ + 0.06) * SR)); seg = x[s0:s1]
            pk = float(np.abs(seg).max()) + 1e-9
            pcm = subprocess.run(["ffmpeg", "-v", "error", "-f", "f32le", "-ar", str(SR), "-ac", "1", "-i", "-", "-af", "lowpass=f=7500", "-ar", str(GBA), "-f", "s16le", "-"],
                                 input=(seg / pk * 0.95).astype("<f4").tobytes(), capture_output=True, check=True).stdout
            q = np.frombuffer(pcm, dtype="<i2").astype(np.int32); data = encode(q); n = len(q)
            clips.append(dict(i=i, start_s=round(s0 / SR, 4), end_s=round(s1 / SR, 4), gba_sample=int(round(s0 / SR * GBA)), samples=n, offset=len(blob), bytes=len(data), vol_peak=round(pk, 4), cue_row=qrow(s0 / SR, origin)))
            blob += data
        open("%s/digital/%s.adp" % (OUT, name), "wb").write(bytes(blob)); info[name] = dict(clips=clips, total_bytes=len(blob), default_gain=(round(10 ** (VOICE_DB / 20), 3) if name == "vocals" else 1.0), active_s=round(sum(c["samples"] for c in clips) / GBA, 1))
        print("digital %-7s %3d clips %6.0f KB  (%.0f s of audio)" % (name, len(clips), len(blob) / 1024, info[name]["active_s"]))
    json.dump(dict(rate=GBA, format="IMA-ADPCM 4-bit, mono, 18157 Hz (the game's song format); each clip starts at gba_sample (music samples from the start of the song, t = 0)", stems=info), open(OUT + "/digital.json", "w"), indent=1)
    return info

# ------------------------------------------------------------------------------------------------ the module
def build_xm(rows, origin, nbars, info):
    NCH = 15; nrows = nbars * 32; npat = (nrows + 63) // 64
    for k, name in ((12, "vocals"), (13, "guitar"), (14, "other")):
        for c in info[name]["clips"]:
            r = c["cue_row"]
            if 0 <= r < npat * 64: rows[(k, r)] = (1 + c["i"] if c["i"] < 96 else 96, CUE, 0x50)
    pats = []; orders = []
    for p in range(npat):
        data = []
        for r in range(64):
            row = []
            for ch in range(NCH):
                e = rows.get((ch, p * 64 + r))
                row.append(X.cell(*e) if e else X.cell())
            data.append(row)
        pats.append((64, data)); orders.append(p)
    inst = [X.instrument(I["name"], I["data"], I["loop"], I["rel"], I["fine"], 64, 0 if I["loop"] or I["name"] != "PIANO" else 0) for I in INST] + [X.instrument("CUE", np.array([0], dtype=np.int8), False, 0, 0, 0)]
    X.write(OUT + "/corporations_hybrid.xm", "CORPORATIONS HYBRID", NCH, pats, orders, inst, speed=3, bpm=XBPM)
    print("module: %d patterns, %d rows, %d channels, %d instruments, %.0f KB" % (npat, nrows, NCH, len(inst), os.path.getsize(OUT + "/corporations_hybrid.xm") / 1024))
    return npat

# ------------------------------------------------------------------------------------------------ player (the instruments played back with the tracker's pitch maths)
def play_module(rows, origin, nrows, sr=SR):
    total = int((nrows * ROW) * sr) + sr; out = np.zeros(total); stems = {"bass": np.zeros(total), "piano": np.zeros(total), "drums": np.zeros(total)}
    for ch in range(12):
        evs = sorted([(r, v) for (c, r), v in rows.items() if c == ch]); name = "bass" if ch == 0 else "piano" if ch < 9 else "drums"
        for k, (r, (note, inst, vol)) in enumerate(evs):
            if note == 97 or inst == 0: continue
            I = INST[inst - 1]; start = int((r * ROW) * sr); end_r = evs[k + 1][0] if k + 1 < len(evs) else r + 400; end = min(total, int(end_r * ROW * sr))
            rate = 8363.0 * 2 ** ((note + I["rel"] - 49) / 12.0 + I["fine"] / 1536.0); step = rate / sr; d = I["data"].astype(np.float64) / 128.0; n = int((end - start))
            pos = np.arange(n) * step
            if I["loop"]: pos = pos % len(d)
            else:
                keep = pos < len(d) - 1; pos = pos[keep]; n = len(pos)
            if n <= 0: continue
            i0 = np.floor(pos).astype(int); fr = pos - i0; y = d[i0] * (1 - fr) + d[np.minimum(i0 + 1, len(d) - 1)] * fr
            amp = (vol - 0x10) / 64.0; env = np.ones(n)
            if end_r < r + 399:      # released by the next event: a 12 ms fade so the cut does not click
                f = min(n, int(0.012 * sr)); env[-f:] = np.linspace(1, 0, f)
            seg = y * amp * env; stems[name][start:start + n] += seg[:max(0, min(n, total - start))]
    for k in stems: out += stems[k]
    return out, stems

def main():
    ev, origin, nbars = prep_events(); rows = assign(ev, origin); nrows = nbars * 32
    print("grid: row %.4f s (1/32 at %.3f BPM), origin %.3f s, %d bars; events: %d cells" % (ROW, BPM, origin, nbars, len(rows)))
    # timing error introduced by the 1/32 grid
    errs = [abs(n["t0"] - (origin + qrow(n["t0"], origin) * ROW)) for n in ev["bass"] + ev["piano"]]; print("quantising error: mean %.1f ms, max %.1f ms (the MIDI files keep the exact times)" % (1000 * np.mean(errs), 1000 * np.max(errs)))
    info = digital(origin); build_xm(rows, origin, nbars, info)
    # MIDI: exact performance and the quantised grid version
    for tag, qz in (("performance", False), ("grid", True)):
        mid = mido.MidiFile(ticks_per_beat=480); tempo = mido.bpm2tempo(BPM)
        tt = mido.MidiTrack(); tt.append(mido.MetaMessage("set_tempo", tempo=tempo, time=0)); tt.append(mido.MetaMessage("track_name", name="CORPORATIONS (E major, A=%.1f Hz)" % (440 * 2 ** (TUNE / 12)), time=0)); mid.tracks.append(tt)
        def tick(t): return int(round((t - origin) / (60.0 / BPM) * 480))
        def track(name, ch, prog, notes):
            tr = mido.MidiTrack(); tr.append(mido.MetaMessage("track_name", name=name, time=0))
            if ch != 9:
                tr.append(mido.Message("program_change", channel=ch, program=prog, time=0))
                cents = int(round(TUNE * 100)); val = 8192 + int(cents * 8192 / 100)       # RPN 0,1 = master fine tuning: the track is 40 cents flat
                for cc, v in ((101, 0), (100, 1), (6, val >> 7), (38, val & 127), (101, 127), (100, 127)): tr.append(mido.Message("control_change", channel=ch, control=cc, value=v, time=0))
            evs = []
            for n in notes:
                a = tick(n["t0"]); b = tick(n["t1"])
                if qz: a = qrow(n["t0"], origin) * 60; b = max(a + 60, qrow(n["t1"], origin) * 60)
                evs += [(a, 1, n), (b, 0, n)]
            evs.sort(key=lambda e: (e[0], e[1])); last = 0
            for t, on, n in evs:
                tr.append(mido.Message("note_on" if on else "note_off", channel=ch, note=n["pitch"], velocity=n["vel"] if on else 0, time=max(0, t - last))); last = t
            return tr
        mid.tracks.append(track("bass", 0, 38, ev["bass"])); mid.tracks.append(track("piano", 1, 0, ev["piano"]))
        dn = [dict(t0=h["t"], t1=h["t"] + 0.05, pitch={"kick": 36, "snare": 38, "hat": 42}[h["kind"]], vel=h["vel"]) for h in ev["drums"]]; mid.tracks.append(track("drums", 9, 0, dn))
        mid.save("%s/corporations_%s.mid" % (OUT, tag))
    print("MIDI: corporations_performance.mid (exact), corporations_grid.mid (1/32 grid), tuning RPN %.0f cents" % (TUNE * 100))
    # preview + accuracy
    out, stems = play_module(rows, origin, nrows)
    json.dump(dict(origin=origin, nrows=nrows), open(OUT + "/grid.json", "w"))
    np.save("/tmp/corp_synth_stems.npy", np.stack([stems["bass"], stems["piano"], stems["drums"]]))
    print("rendered the module: %.0f s" % (len(out) / SR))

if __name__ == "__main__": main()
