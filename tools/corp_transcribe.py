#!/usr/bin/env python3
"""Stage 1 of the hybrid CORPORATIONS conversion: transcribe the BASS, the PIANO and the DRUMS of the stems into note lists (seconds, MIDI pitch, velocity).
The guitar, the vocals and the other synths are NOT transcribed: they stay digital (4-bit ADPCM), see tools/corp_digital.py.

  bass   basic-pitch (ONNX) restricted to 38-400 Hz, then checked against an independent pitch tracker (pYIN): every note is compared with the f0 under it and
         corrected by octave / semitone when the tracker clearly disagrees; re-strikes of one pitch are merged; velocity from the stem's loudness under the note
  piano  basic-pitch, polyphonic (max 8 voices kept), short ghost notes dropped, velocity from the model's amplitude
  drums  onset detection on three bands of the drum stem: kick (< 150 Hz), snare (150 Hz - 2.5 kHz, with noise), hats (> 6 kHz)
Then the tempo grid is refined from the bass and kick onsets (BPM and first beat), the key is found (Krumhansl profiles over the bass and piano), and everything is quantised
to 1/32 notes for the tracker, keeping the exact times too (the MIDI files carry the exact performance, the XM carries the grid).
Writes tools/corporations/notes.json.  usage: python3 tools/corp_transcribe.py   (needs numpy scipy librosa mido basic-pitch[onnx] ffmpeg)"""
import os, sys, json, subprocess, collections
import numpy as np, librosa
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_corporations import find_stem, load, SR

OUT = "tools/corporations"
MAJ = np.array([6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88]); MIN = np.array([6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17])
NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

def bp_notes(stem, tune=0.0, **kw):
    """basic-pitch on a stem. tune (semitones, the track's offset from A=440) is removed first by resampling, so the notes sit on the model's semitone centres; the times are mapped back."""
    from basic_pitch.inference import predict, Model
    from basic_pitch import ICASSP_2022_MODEL_PATH
    path = find_stem(stem); r = 1.0
    if abs(tune) > 0.02:
        import soundfile as sf
        x, sr = librosa.load(path, sr=22050, mono=True); r = 2 ** (-tune / 12.0)       # a track 40 cents flat is sped up by 2^(0.4/12)
        y = librosa.resample(x, orig_sr=int(round(sr * r * 1000)), target_sr=sr * 1000); path = "/tmp/_corp_%s_tuned.wav" % stem; sf.write(path, y, sr)
    _, _, ne = predict(path, Model(ICASSP_2022_MODEL_PATH), melodia_trick=True, multiple_pitch_bends=False, **kw)
    return sorted([dict(t0=float(a) * r, t1=float(b) * r, pitch=int(p), amp=float(m)) for a, b, p, m, _ in ne], key=lambda n: (n["t0"], n["pitch"]))

def merge_restrikes(notes, gap=0.05):
    out = []
    for n in notes:
        for o in reversed(out[-6:]):
            if o["pitch"] == n["pitch"] and 0 <= n["t0"] - o["t1"] <= gap and n["amp"] < o["amp"] * 1.15: o["t1"] = n["t1"]; break
        else: out.append(dict(n))
    return out

def bass(sr=16000):
    """The bass line from the fundamental itself: pYIN gives the f0 of the sub bass (the neural model likes to add the harmonics as extra notes), the track's tuning offset is
    measured and removed, the f0 curve is cut into notes where the pitch really changes and where the stem's energy re-strikes, and basic-pitch is kept as a cross-check."""
    x = load(find_stem("bass"), sr); hop = 160
    f0, vf, vp = librosa.pyin(x, fmin=30, fmax=450, sr=sr, frame_length=2048, hop_length=hop, fill_na=np.nan)
    m = 69 + 12 * np.log2(f0 / 440.0); voiced = ~np.isnan(m)
    frac = ((m[voiced] - np.round(m[voiced]) + 0.5) % 1.0) - 0.5; tune = float(np.angle(np.mean(np.exp(2j * np.pi * frac))) / (2 * np.pi))      # circular mean of the fractional part: the track's tuning in semitones
    q = np.where(voiced, np.round(m - tune), np.nan)
    # median filter of the quantised pitch over 5 frames (50 ms) kills one-frame jumps and the odd octave glitch
    qf = q.copy()
    for i in range(2, len(q) - 2):
        w = q[i - 2:i + 3]; w = w[~np.isnan(w)]
        qf[i] = np.median(w) if len(w) >= 3 else np.nan
    env = librosa.feature.rms(y=x, frame_length=1024, hop_length=hop)[0]; ldb = 20 * np.log10(env + 1e-6)
    on = librosa.onset.onset_detect(y=x, sr=sr, hop_length=hop, units="frames", delta=0.12, wait=4, backtrack=False)
    onset_f = set(int(i) for i in on)
    notes = []; cur = None
    for i in range(len(qf)):
        v = qf[i]
        if np.isnan(v):
            if cur is not None and i - cur["last"] > 3: cur["end"] = cur["last"] + 1; notes.append(cur); cur = None
            continue
        restrike = any((i + d) in onset_f for d in (-1, 0, 1)) and cur is not None and i - cur["start"] > 5 and ldb[i] - ldb[max(0, i - 6)] > 3.0
        if cur is None or v != cur["pitch"] or restrike:
            if cur is not None: cur["end"] = i; notes.append(cur)
            cur = dict(start=i, pitch=int(v), last=i)
        else: cur["last"] = i
    if cur is not None: cur["end"] = cur["last"] + 1; notes.append(cur)
    out = []
    for n in notes:
        t0 = n["start"] * hop / sr; t1 = n["end"] * hop / sr
        if t1 - t0 < 0.06 or not (28 <= n["pitch"] <= 62): continue
        seg = env[n["start"]:n["end"] + 1]; out.append(dict(t0=t0, t1=t1, pitch=n["pitch"], rms=float(np.percentile(seg, 80)), amp=1.0))
    # glue: a note that is the same pitch as the one before and starts within 30 ms of its end was split by an unvoiced dip
    glued = []
    for n in out:
        if glued and glued[-1]["pitch"] == n["pitch"] and n["t0"] - glued[-1]["t1"] < 0.03 and n["t0"] - glued[-1]["t0"] < 0.12: glued[-1]["t1"] = n["t1"]
        else: glued.append(n)
    out = glued
    lo, hi = np.percentile([n["rms"] for n in out], [5, 98])
    for n in out: n["vel"] = int(np.clip(40 + 87 * (np.log(max(n["rms"], 1e-6)) - np.log(max(lo, 1e-6))) / (np.log(hi) - np.log(max(lo, 1e-6))), 30, 127))
    # cross-check with basic-pitch: its notes that land on one of ours (same pitch) or on our fundamental's octave / twelfth (harmonics) are agreement
    bpn = merge_restrikes(bp_notes("bass", tune, onset_threshold=0.45, frame_threshold=0.25, minimum_note_length=70, minimum_frequency=38, maximum_frequency=400))
    ts = np.array([n["t0"] for n in out]); te = np.array([n["t1"] for n in out]); pp = np.array([n["pitch"] for n in out]); same = harm = other = 0
    for b_ in bpn:
        mid = (b_["t0"] + b_["t1"]) / 2; i = np.searchsorted(ts, mid, "right") - 1
        if i >= 0 and mid < te[i]:
            d = b_["pitch"] - pp[i]
            if d == 0: same += 1
            elif d in (12, 19, 24, 28, 31): harm += 1
            else: other += 1
        else: other += 1
    return out, tune, dict(notes=len(out), tuning_cents=round(tune * 100, 1), pyin_voiced_frames=int(voiced.sum()), basic_pitch_notes=len(bpn), bp_same_pitch=same, bp_harmonics_of_ours=harm, bp_other=other)

def piano(tune=0.0):
    # thresholds picked by playing the notes back and comparing the chroma with the stem (chroma cosine 0.635 at 0.5/0.3 -> 0.695 at 0.3/0.2 -> 0.70 at 0.25/0.15 with twice the notes)
    notes = [n for n in bp_notes("piano", tune, onset_threshold=0.3, frame_threshold=0.2, minimum_note_length=60, minimum_frequency=60, maximum_frequency=2200) if n["amp"] >= 0.15 or n["t1"] - n["t0"] > 0.4]
    notes = merge_restrikes(notes, 0.04)
    for n in notes: n["vel"] = int(np.clip(30 + 97 * (n["amp"] - 0.15) / 0.6, 30, 127))
    ev = sorted([(n["t0"], 1, i) for i, n in enumerate(notes)] + [(n["t1"], 0, i) for i, n in enumerate(notes)]); act = []; drop = set(); 
    for t, k, i in ev:
        if k: 
            act.append(i)
            if len(act) > 8: weakest = min(act, key=lambda j: notes[j]["amp"]); act.remove(weakest); drop.add(weakest)
        elif i in act: act.remove(i)
    return [n for i, n in enumerate(notes) if i not in drop], dict(dropped_for_polyphony=len(drop))

def drums(sr=SR):
    x = load(find_stem("drums"), sr); hop = 256; hits = []; res = {}
    bands = dict(kick=((30, 150), 0.09, 0.11), snare=((150, 2500), 0.10, 0.09), hat=((6000, 16000), 0.04, 0.07))
    for name, ((lo, hi), tau, wait_s) in bands.items():
        y = signal.sosfiltfilt(signal.butter(4, [lo, hi], "bp", fs=sr, output="sos"), x); e = librosa.feature.rms(y=y, frame_length=1024, hop_length=hop)[0]; le = np.log(e + 1e-5)
        nov = le - np.convolve(le, np.ones(41) / 41, "same"); kern = np.exp(-np.arange(int(5 * tau * sr / hop)) / (tau * sr / hop)); best = None
        for delta in (0.5, 0.7, 0.9, 1.1, 1.3, 1.6, 2.0):      # the detector threshold is chosen by how well the hits, each with its decay, rebuild the band's own loudness curve
            pk = librosa.util.peak_pick(nov, pre_max=4, post_max=4, pre_avg=10, post_avg=10, delta=delta, wait=int(wait_s * sr / hop))
            imp = np.zeros(len(e)); imp[pk] = e[pk]; rec = np.convolve(imp, kern)[:len(e)]; c = np.corrcoef(rec, e)[0, 1]
            if best is None or c > best[0] + 0.01: best = (c, delta, pk)
        c, delta, pk = best; res[name] = dict(hits=int(len(pk)), threshold=delta, envelope_correlation=round(float(c), 3))
        am = np.array([e[i:i + 6].max() for i in pk]); lo_, hi_ = np.percentile(am, [5, 98])
        for i, a in zip(pk, am): hits.append(dict(t=float(i * hop / sr), kind=name, amp=float(a), vel=int(np.clip(45 + 82 * (a - lo_) / max(1e-9, hi_ - lo_), 35, 127))))
    hits.sort(key=lambda h: h["t"])
    return hits, res

def refine_grid(onsets, bpm0, t0_0):
    best = (1e9, bpm0, t0_0); on = np.array(onsets)
    for bpm in np.arange(bpm0 - 1.5, bpm0 + 1.5, 0.005):
        g = 60.0 / bpm / 8
        for t0 in np.linspace(t0_0 - g, t0_0 + g, 41):
            r = ((on - t0 + g / 2) % g) - g / 2; s = np.mean(np.abs(r))
            if s < best[0]: best = (s, bpm, t0)
    return best[1], best[2], best[0] * 1000

def find_key(bass_n, piano_n):
    h = np.zeros(12)
    for n in bass_n + piano_n: h[n["pitch"] % 12] += (n["t1"] - n["t0"]) * (1.0 if n in bass_n else 0.7)
    best = max(((np.corrcoef(np.roll(MAJ, k), h)[0, 1], k, "major") for k in range(12)), default=None); bm = max(((np.corrcoef(np.roll(MIN, k), h)[0, 1], k, "minor") for k in range(12)))
    return dict(best_major=[NOTE[best[1]], round(best[0], 3)], best_minor=[NOTE[bm[1]], round(bm[0], 3)], histogram={NOTE[i]: round(float(h[i] / h.sum()), 3) for i in range(12)})

def main():
    os.makedirs(OUT, exist_ok=True)
    b, tune, binfo = bass(); print("bass:", len(b), "notes", binfo)
    p, pinfo = piano(tune); print("piano:", len(p), "notes", pinfo)
    d, dinfo = drums(); print("drums:", len(d), "hits", dinfo)
    kick = [h["t"] for h in d if h["kind"] == "kick"]; bpm, t0, err = refine_grid([n["t0"] for n in b] + kick, 131.0, 0.344)
    g16 = 60.0 / bpm / 4; pos = collections.Counter(int(round((t - t0) / g16)) % 16 for t in kick); down = max(range(0, 16, 2), key=lambda q: pos[q])      # the downbeat: the even sixteenth where the kick lands most
    t0 += down * g16; t0 -= int(t0 / (16 * g16)) * 16 * g16
    print("grid: %.3f BPM, downbeat at %.3f s (kick per sixteenth of the bar: %s), mean onset distance to the 1/32 grid %.1f ms" % (bpm, t0, [pos[(q + down) % 16] for q in range(16)], err))
    key = find_key(b, p); print("key:", key["best_major"], key["best_minor"])
    json.dump(dict(tuning_semitones=tune, tuning_cents=tune * 100, bpm=bpm, t0=t0, grid_err_ms=err, key=key, bass_info=binfo, piano_info=pinfo, drum_info=dinfo, bass=b, piano=p, drums=d), open(OUT + "/notes.json", "w"))

if __name__ == "__main__": main()
