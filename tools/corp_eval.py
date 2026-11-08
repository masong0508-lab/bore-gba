#!/usr/bin/env python3
"""Stage 3: measure the transcription against the stems and render the hybrid preview (needs tools/corp_build.py to have run).
Metrics (stem against the module played back with the same instruments):
  chroma     mean cosine similarity of the CQT chroma frames (tuning removed) where the stem is active: do the right notes sound?
  spectrum   mean cosine similarity of the log-mel spectrum (bass 35-450 Hz / piano 60-4000 Hz / drums in three bands)
  onsets     F1 of the note / hit starts against onsets found independently in the stem's loudness, within 60 ms
  loudness   envelope correlation (50 ms frames)
Writes tools/corporations/accuracy.json and /tmp/corporations_hybrid.wav (the module + the decoded 4-bit digital clips at their exact samples)."""
import os, sys, json, struct
import numpy as np, librosa
from scipy import signal
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_corporations import find_stem, load, SR
from encode_sfx import decode
OUT = "tools/corporations"; GBA = 18157
ND = json.load(open(OUT + "/notes.json")); TUNE = ND["tuning_semitones"]; G = json.load(open(OUT + "/grid.json")); DJ = json.load(open(OUT + "/digital.json"))
S = np.load("/tmp/corp_synth_stems.npy"); synth = dict(bass=S[0], piano=S[1], drums=S[2])

def cos(a, b):
    n = min(a.shape[1], b.shape[1]); a = a[:, :n]; b = b[:, :n]; d = np.linalg.norm(a, axis=0) * np.linalg.norm(b, axis=0) + 1e-9; return (a * b).sum(axis=0) / d
def tuned(x):   # speed the audio up by the track's flatness so a chroma / CQT sees standard pitches
    r = 2 ** (-TUNE / 12.0); return librosa.resample(x, orig_sr=int(round(SR * r * 100)), target_sr=SR * 100)
def active(x, hop=2048): e = librosa.feature.rms(y=x, frame_length=4096, hop_length=hop)[0]; return e > np.percentile(e, 30)
def onset_f1(ref, est, tol=0.06):
    ref = np.array(sorted(ref)); est = np.array(sorted(est)); used = np.zeros(len(est), bool); tp = 0
    for r in ref:
        i = np.searchsorted(est, r); c = [j for j in (i - 1, i) if 0 <= j < len(est) and not used[j] and abs(est[j] - r) <= tol]
        if c: j = min(c, key=lambda j: abs(est[j] - r)); used[j] = True; tp += 1
    p = tp / max(1, len(est)); rc = tp / max(1, len(ref)); return dict(precision=round(p, 3), recall=round(rc, 3), f1=round(2 * p * rc / max(1e-9, p + rc), 3))
def stem_onsets(x, lo, hi):
    y = signal.sosfiltfilt(signal.butter(4, [lo, hi], "bp", fs=SR, output="sos"), x); e = librosa.onset.onset_strength(y=y, sr=SR, hop_length=256)
    return list(librosa.onset.onset_detect(onset_envelope=e, sr=SR, hop_length=256, delta=0.25, wait=8, units="time"))

rep = {}
for name, lo, hi in (("bass", 35, 450), ("piano", 60, 4000)):
    x = load(find_stem(name)); y = synth[name][:len(x)]; x = x[:len(y)]
    xs = signal.sosfiltfilt(signal.butter(4, [lo, hi], "bp", fs=SR, output="sos"), x); ys = signal.sosfiltfilt(signal.butter(4, [lo, hi], "bp", fs=SR, output="sos"), y)
    # the module starts at the grid origin: line the two up (the origin is where row 0 sits on the stem's clock)
    off = int(round(-G["origin"] * SR)); ys = ys[off:] if off > 0 else ys; n = min(len(xs), len(ys)); xs = xs[:n]; ys = ys[:n]
    cx = librosa.feature.chroma_cqt(y=tuned(xs), sr=SR, hop_length=2048); cy = librosa.feature.chroma_cqt(y=ys, sr=SR, hop_length=2048); act = active(xs)[:min(cx.shape[1], cy.shape[1])]
    c = cos(cx, cy)[:len(act)]
    mx = librosa.feature.melspectrogram(y=xs, sr=SR, n_fft=4096, hop_length=2048, n_mels=64, fmin=lo, fmax=hi); my = librosa.feature.melspectrogram(y=ys, sr=SR, n_fft=4096, hop_length=2048, n_mels=64, fmin=lo, fmax=hi)
    sp = cos(np.log1p(mx * 100), np.log1p(my * 100))[:len(act)]
    ex = librosa.feature.rms(y=xs, frame_length=2205, hop_length=2205)[0]; ey = librosa.feature.rms(y=ys, frame_length=2205, hop_length=2205)[0]; m = min(len(ex), len(ey))
    notes = ND[name]; est = [n["t0"] for n in notes]; ref = [t for t in stem_onsets(xs if False else x, lo, hi)]
    rep[name] = dict(chroma_cosine=round(float(c[act].mean()), 3), spectrum_cosine=round(float(sp[act].mean()), 3), loudness_correlation=round(float(np.corrcoef(ex[:m], ey[:m])[0, 1]), 3), onsets_vs_stem=onset_f1(ref, est), notes=len(notes))
    if name == "bass":      # pitch after rendering: pYIN on the stem against pYIN on the module's bass (both voiced frames), in cents
        sr2 = 16000; hop = 160; xa = load(find_stem("bass"), sr2); ya = librosa.resample(y, orig_sr=SR, target_sr=sr2)[int(round(-G["origin"] * sr2)):]; m_ = min(len(xa), len(ya)); xa = xa[:m_]; ya = ya[:m_]
        fa = librosa.pyin(xa, fmin=30, fmax=450, sr=sr2, frame_length=2048, hop_length=hop)[0]; fb = librosa.pyin(ya, fmin=30, fmax=450, sr=sr2, frame_length=2048, hop_length=hop)[0]
        both = ~np.isnan(fa) & ~np.isnan(fb); cents = 1200 * np.log2(fb[both] / fa[both]); rep[name]["rendered_pitch_vs_stem"] = dict(frames=int(both.sum()), within_50_cents=round(float(np.mean(np.abs(cents) <= 50)), 3), within_a_semitone_octave_ignored=round(float(np.mean(np.abs(((cents + 600) % 1200) - 600) <= 100)), 3), octave_off=round(float(np.mean(np.abs(np.abs(cents) - 1200) <= 100)), 3))
    print(name, rep[name])
# drums: band envelopes
x = load(find_stem("drums")); y = synth["drums"]; off = int(round(-G["origin"] * SR)); y = y[off:] if off > 0 else y; n = min(len(x), len(y)); x = x[:n]; y = y[:n]; rep["drums"] = {}
for nm, (lo, hi) in dict(kick=(30, 150), snare=(150, 2500), hat=(6000, 16000)).items():
    a = signal.sosfiltfilt(signal.butter(4, [lo, hi], "bp", fs=SR, output="sos"), x); b = signal.sosfiltfilt(signal.butter(4, [lo, hi], "bp", fs=SR, output="sos"), y)
    ea = librosa.feature.rms(y=a, frame_length=1102, hop_length=1102)[0]; eb = librosa.feature.rms(y=b, frame_length=1102, hop_length=1102)[0]; m = min(len(ea), len(eb))
    ref = stem_onsets(x, lo, hi); est = [h["t"] for h in ND["drums"] if h["kind"] == nm]
    rep["drums"][nm] = dict(loudness_correlation=round(float(np.corrcoef(ea[:m], eb[:m])[0, 1]), 3), onsets_vs_stem=onset_f1(ref, est), hits=len(est))
print("drums", rep["drums"])
json.dump(rep, open(OUT + "/accuracy.json", "w"), indent=1)
# hybrid preview: the module + the decoded 4-bit clips at their exact samples (rate 18157 -> 44100)
total = len(synth["bass"]); mix = np.zeros(total)
off = int(round(-G["origin"] * SR)); mod = (synth["bass"] + synth["piano"] + synth["drums"]); mod = mod[off:] if off > 0 else mod; mix[:len(mod)] += mod[:total] * 0.55
for st, gain in (("vocals", 1.0), ("guitar", 1.0), ("other", 0.8)):
    blob = open("%s/digital/%s.adp" % (OUT, st), "rb").read()
    for c in DJ["stems"][st]["clips"]:
        pcm = decode(blob[c["offset"]:c["offset"] + c["bytes"]]).astype(np.float64) / 32768 * c["vol_peak"] * gain
        up = signal.resample(pcm, int(round(len(pcm) * SR / GBA))); a = int(round(c["gba_sample"] / GBA * SR)); k = min(len(up), total - a)
        if k > 0: mix[a:a + k] += up[:k]
mix = np.tanh(mix / np.abs(mix).max() * 1.3) / np.tanh(1.3); pcm = (mix * 32767).astype("<i2")
with open("/tmp/corporations_hybrid.wav", "wb") as f: f.write(b"RIFF" + struct.pack("<I", 36 + pcm.nbytes) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, SR, SR * 2, 2, 16) + b"data" + struct.pack("<I", pcm.nbytes) + pcm.tobytes())
print("preview /tmp/corporations_hybrid.wav")
