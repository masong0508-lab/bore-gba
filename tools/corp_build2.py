#!/usr/bin/env python3
"""CORPORATIONS, stage 2 (re-analysis): the new bass (corp_bass.py), Rhodes keys (corp_keys.py piano|other) and drum-machine hits (corp_drums.py) become
  * tools/corporations/corporations_v2.xm   a FastTracker II module with ONE ROW PER TRACKER TICK (speed 1, 131 BPM: a row is 19.08 ms = 1/6 of a sixteenth, so the swing of the
                                             off sixteenths, 32 ms late, is on the grid instead of being rounded away); 96 rows = one bar, bar 0 = the first sound
                                             channels: 1 bass | 2-6 Rhodes (piano stem) | 7-10 Rhodes (other stem) | 11 kick | 12 snare+tom | 13 hats | 14 vocals cue | 15 guitar cue | 16 lead cue
  * tools/corporations/corporations_v2.mid  the same notes (tempo 131.004, bass 45 cents flat via RPN fine tuning) and the drums on channel 10
  * the digital stems stay digital: vocals and guitar from tools/corporations/digital.json, the HORN / SLIDE-GUITAR / SAX lead of the other stem as new 4-bit clips (lead.adp), cues on rows
  * tools/corporations/preview_v2.wav   the module played back (the tracker's own pitch maths) + the digital clips at their exact samples
usage: python3 tools/corp_build2.py"""
import os, sys, json, subprocess, struct
import numpy as np, mido, soundfile as sf
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
import xmwrite as X
from encode_sfx import encode, decode

BASS_TUNE = -0.455
LEAD_RANGES = [(75, 111), (118, 123), (130, 134), (142, 196)]     # the bars where the other stem is a horn / slide guitar / sax lead (an instrument tagger and a text-audio model agree)
NCH = 16
def rel_fine(P, tune=0.0):
    r = -9 + 12 * np.log2(440.0 * P / 8363.0) + tune; rel = int(np.floor(r)); fine = int(round((r - rel) * 128))
    if fine > 127: rel += 1; fine -= 128
    return rel, fine
def rate_rel(Rs):
    r = 12 * np.log2(Rs / 8363.0); rel = int(np.floor(r)); return rel, int(round((r - rel) * 128))

def i8(y, peak=0.95): y = np.asarray(y, dtype=np.float64); return np.clip(np.round(y / max(1e-9, np.abs(y).max()) * 127 * peak), -127, 127).astype(np.int8)
def make_instruments():
    I = {}
    # bass: one cycle (a saw with a strong fundamental), looped; the stem is 45 cents flat
    N = 128; n = np.arange(N); w = sum(np.sin(2 * np.pi * k * n / N) / k ** 1.1 for k in range(1, 12)) + 0.6 * np.sin(2 * np.pi * n / N)
    rel, fine = rel_fine(N, BASS_TUNE); I["bass"] = dict(name="BASS", data=i8(w), loop=True, rel=rel, fine=fine, Rs=None, P=N)
    # Rhodes: a tine with a bell partial and a short bark, 16 kHz one-shot
    Rs = 16000; t = np.arange(int(Rs * 1.6)) / Rs; F0 = 261.6256; y = np.zeros(len(t))
    for k, (a, d) in ((1, (1.0, 1.1)), (2, (0.30, 0.55)), (3, (0.10, 0.30)), (5.6, (0.28, 0.10)), (7.2, (0.08, 0.06))): y += a * np.sin(2 * np.pi * F0 * k * t) * np.exp(-t / d)
    y += 0.25 * np.sin(2 * np.pi * F0 * t + 2.0 * np.sin(2 * np.pi * F0 * t)) * np.exp(-t / 0.05); y *= np.minimum(1, t / 0.002)
    r = 12 * np.log2(F0 * Rs / (8363.0 * F0)); rel, fine = rate_rel(Rs); I["rhodes"] = dict(name="RHODES", data=i8(y), loop=False, rel=rel, fine=fine, Rs=Rs, P=None)
    kit = {k: sf.read("%s/kit/%s.wav" % (OUT, k))[0] for k in ("kick", "snare", "tom", "hat", "hat_open")}
    for k, d in kit.items():
        Rs = 22050; y = signal.resample_poly(d, Rs, SR); rel, fine = rate_rel(Rs); I[k] = dict(name=k.upper().replace("_", " "), data=i8(y), loop=False, rel=rel, fine=fine, Rs=Rs, P=None)
    I["cue"] = dict(name="CUE", data=np.array([0], dtype=np.int8), loop=False, rel=0, fine=0, Rs=None, P=None)
    return I

def alloc(notes, first, nvoice, grid, free):
    """round-robin voices; fills grid[(ch, row)] = (note, inst, vol); a voice is free again at its note's end"""
    for n in sorted(notes, key=lambda n: (n["start"], n["pitch"])):
        ok = [c for c in range(nvoice) if free[first + c] <= n["start"] and (first + c, n["start"]) not in grid]
        if not ok: continue
        c = min(ok, key=lambda c: free[first + c]); ch = first + c
        grid[(ch, n["start"])] = (n["pitch"] - 11, n["inst"], 0x10 + max(1, round(n["vel"] / 127 * 64 * n.get("gain", 1.0))))
        end = max(n["end"], n["start"] + 2)
        if (ch, end) not in grid: grid[(ch, end)] = (97, 0, 0)
        free[ch] = end

def digital_lead(info):
    """the horn / slide-guitar / sax lead of the other stem as 4-bit clips, cut at the quietest moment inside each range"""
    x = load("other").astype(np.float64); blob = bytearray(); clips = []
    for (B0, B1) in LEAD_RANGES:
        ta = bar_time(B0) - 0.04; tb = bar_time(B1) + 0.06
        # long ranges are cut in 40 s pieces at the quietest point (the 4-bit stream has no memory problem, the clip table has)
        t = ta
        while t < tb - 0.1:
            t1 = min(tb, t + 40.0)
            if t1 < tb:
                lo_, hi_ = int((t + 30) * SR), int(t1 * SR); e = env(x[lo_:hi_], 20.0); t1 = (lo_ + int(np.argmin(e))) / SR
            seg = x[int(t * SR):int(t1 * SR)]; pk = float(np.abs(seg).max()) + 1e-9
            pcm = subprocess.run(["ffmpeg", "-v", "error", "-f", "f32le", "-ar", str(SR), "-ac", "1", "-i", "-", "-af", "lowpass=f=7500", "-ar", str(GBA), "-f", "s16le", "-"], input=(seg / pk * 0.95).astype("<f4").tobytes(), capture_output=True, check=True).stdout
            q = np.frombuffer(pcm, dtype="<i2").astype(np.int32); data = encode(q)
            clips.append(dict(i=len(clips), start_s=round(t, 4), end_s=round(t1, 4), gba_sample=int(round(t * GBA)), samples=len(q), offset=len(blob), bytes=len(data), vol_peak=round(pk, 4), cue_tick=time_to_gtick(t)))
            blob += data; t = t1
    open(OUT + "/digital/lead.adp", "wb").write(bytes(blob))
    info["lead"] = dict(clips=clips, total_bytes=len(blob), default_gain=1.0, active_s=round(sum(c["samples"] for c in clips) / GBA, 1), bars=LEAD_RANGES)
    print("digital lead: %d clips, %.0f KB" % (len(clips), len(blob) / 1024))

def main():
    I = make_instruments(); names = ["bass", "rhodes", "kick", "snare", "tom", "hat", "hat_open", "cue"]; ID = {k: i + 1 for i, k in enumerate(names)}
    bass = json.load(open(OUT + "/bass.json"))["notes"]; kp = json.load(open(OUT + "/keys_piano.json"))["notes"]; ko = json.load(open(OUT + "/keys_other.json"))["notes"]
    dr = json.load(open(OUT + "/drums.json")); ev = dr["events"]
    dj = json.load(open(OUT + "/digital.json")); info = dj["stems"]; info.pop("other", None)
    if os.path.exists(OUT + "/digital/other.adp"): os.remove(OUT + "/digital/other.adp")
    digital_lead(info)
    grid = {}; free = [-1] * NCH
    for n in bass: n["inst"] = ID["bass"]
    for n in kp: n["inst"] = ID["rhodes"]
    for n in ko: n["inst"] = ID["rhodes"]; n["gain"] = 0.85
    # the bass is one voice (notes cannot overlap: the next note ends the one before)
    bass.sort(key=lambda n: n["start"])
    for a, b in zip(bass[:-1], bass[1:]): a["end"] = min(a["end"], b["start"])
    alloc(bass, 0, 1, grid, free); alloc(kp, 1, 5, grid, free); alloc(ko, 6, 4, grid, free)
    # drums
    chmap = dict(kick=(10, "kick"), snare=(11, "snare"), tom=(11, "tom"), hat=(12, "hat"), hat_open=(12, "hat_open"))
    nd = 0
    for v in ev:
        ch, nm = chmap[v["k"]]; g = time_to_gtick(v["s"] / SR - (0.0 if v["k"] != "kick" else 0.0) - 0.002)
        vol = max(6, min(64, int(round(40 * min(1.6, max(0.15, v["g"])) ** 0.8))))
        if (ch, g) in grid and grid[(ch, g)][2] >= 0x10 + vol: continue
        grid[(ch, g)] = (49, ID[nm], 0x10 + vol); nd += 1
    # cues
    for k, (name, ch) in enumerate((("vocals", 13), ("guitar", 14), ("lead", 15))):
        for c in info[name]["clips"]:
            g = c.get("cue_tick", time_to_gtick(c["start_s"])); c["cue_tick"] = g
            grid[(ch, g)] = (1 + c["i"] if c["i"] < 96 else 96, ID["cue"], 0x50)
    last = max(g for (_, g) in grid); nbars = last // TPB + 2
    pats = []; orders = []
    for B in range(nbars):
        rows = [[X.cell(*grid[(ch, B * TPB + r)]) if (ch, B * TPB + r) in grid else X.cell() for ch in range(NCH)] for r in range(TPB)]
        pats.append((TPB, rows)); orders.append(B)
    inst = [X.instrument(I[k]["name"], I[k]["data"], I[k]["loop"], I[k]["rel"], I[k]["fine"], 64, 0) for k in names]
    X.write(OUT + "/corporations_v2.xm", "CORPORATIONS V2", NCH, pats[:255], orders[:255], inst, speed=1, bpm=131)
    print("module: %d bars (%d patterns of 96 rows), %d drum hits, %d bass notes, %d + %d Rhodes notes, %.0f KB" % (nbars, len(pats), nd, len(bass), len(kp), len(ko), os.path.getsize(OUT + "/corporations_v2.xm") / 1024))
    dj["stems"] = info; dj["note"] = "clips start at gba_sample (music samples from t = 0); cue_tick = row of the module (1 row = 1 tracker tick = 19.08 ms)"
    json.dump(dj, open(OUT + "/digital.json", "w"), indent=1)
    # ---- MIDI
    mid = mido.MidiFile(ticks_per_beat=480); tt = mido.MidiTrack(); tt.append(mido.MetaMessage("set_tempo", tempo=mido.bpm2tempo(BPM), time=0)); tt.append(mido.MetaMessage("track_name", name="CORPORATIONS V2 (E major; keys A=%.1f Hz, bass 45 cents flat)" % 440.0, time=0)); mid.tracks.append(tt)
    TK = 480 / 96 * 4 / 4        # midi ticks per tracker tick: a bar is 4 beats = 1920 midi ticks = 96 tracker ticks
    TK = 1920 / 96
    def track(name, ch, prog, notes, cents=0):
        tr = mido.MidiTrack(); tr.append(mido.MetaMessage("track_name", name=name, time=0))
        if ch != 9:
            tr.append(mido.Message("program_change", channel=ch, program=prog, time=0))
            if cents:
                val = 8192 + int(cents * 8192 / 100)
                for cc, v in ((101, 0), (100, 1), (6, val >> 7), (38, val & 127), (101, 127), (100, 127)): tr.append(mido.Message("control_change", channel=ch, control=cc, value=v, time=0))
        evs = []
        for n in notes: evs += [(int(round(n["start"] * TK)), 1, n), (int(round(max(n["end"], n["start"] + 1) * TK)), 0, n)]
        evs.sort(key=lambda e: (e[0], e[1])); last = 0
        for t, on, n in evs: tr.append(mido.Message("note_on" if on else "note_off", channel=ch, note=n["pitch"], velocity=n["vel"] if on else 0, time=max(0, t - last))); last = t
        return tr
    mid.tracks.append(track("bass (45 cents flat)", 0, 38, bass, cents=-45.5)); mid.tracks.append(track("rhodes (piano stem)", 1, 4, kp)); mid.tracks.append(track("rhodes (other stem)", 2, 4, ko))
    GM = dict(kick=36, snare=38, tom=45, hat=42, hat_open=46)
    dn = [dict(start=time_to_gtick(v["s"] / SR), end=time_to_gtick(v["s"] / SR) + 2, pitch=GM[v["k"]], vel=int(np.clip(40 + 70 * min(1.4, v["g"]), 30, 127))) for v in ev]
    mid.tracks.append(track("drums", 9, 0, dn)); mid.save(OUT + "/corporations_v2.mid")
    # ---- preview
    render(grid, I, ID, info, nbars)

def render(grid, I, ID, info, nbars):
    total = int(gtick_time(nbars * TPB) * SR) + SR; mix = np.zeros(total); inv = {v: k for k, v in ID.items()}
    chev = {}
    for (ch, g), v in grid.items(): chev.setdefault(ch, []).append((g, v))
    for ch, lst in chev.items():
        lst.sort()
        for j, (g, (note, inst, vol)) in enumerate(lst):
            if note == 97 or inst == 0 or inv[inst] == "cue": continue
            ins = I[inv[inst]]; start = int(gtick_time(g) * SR); nxt = lst[j + 1][0] if j + 1 < len(lst) else g + 400
            end = min(total, int(gtick_time(nxt) * SR))
            if end <= start or start < 0: continue
            rate = 8363.0 * 2 ** ((note + ins["rel"] - 49) / 12.0 + ins["fine"] / 1536.0); step = rate / SR
            d = ins["data"].astype(np.float64) / 128.0; n = end - start; pos = np.arange(n) * step
            if ins["loop"]: pos %= len(d)
            else: keep = pos < len(d) - 1; pos = pos[keep]; n = len(pos)
            if n <= 0: continue
            i0 = np.floor(pos).astype(int); fr = pos - i0; y = d[i0] * (1 - fr) + d[np.minimum(i0 + 1, len(d) - 1)] * fr
            env_ = np.ones(n); f = min(n, int(0.012 * SR)); env_[-f:] = np.linspace(1, 0, f)
            seg = y * ((vol - 0x10) / 64.0) * env_; mix[start:start + n] += seg[:max(0, min(n, total - start))]
    tracked = mix.copy()
    dig = np.zeros(total)
    for name, gain in (("vocals", 10 ** (-3.5 / 20)), ("guitar", 1.0), ("lead", 1.0)):
        blob = open("%s/digital/%s.adp" % (OUT, name), "rb").read()
        for c in info[name]["clips"]:
            pcm = decode(blob[c["offset"]:c["offset"] + c["bytes"]]).astype(np.float64) / 32768.0 * c["vol_peak"]
            y = signal.resample_poly(pcm, SR, GBA); a = int(c["start_s"] * SR); n = min(len(y), total - a)
            if n > 0: dig[a:a + n] += y[:n] * gain
    y = tracked * 0.9 + dig; y = np.tanh(y * 1.2) / np.tanh(1.2); y = y / max(1e-9, np.abs(y).max()) * 0.9
    sf.write(OUT + "/preview_v2.wav", y.astype(np.float32), SR); np.save(os.path.join(CACHE, "v2_tracked.npy"), tracked.astype(np.float32))
    print("preview_v2.wav: %.0f s" % (len(y) / SR))
if __name__ == "__main__": main()
