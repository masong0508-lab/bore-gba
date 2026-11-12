"""Compares the module's preview (tracked notes + digital clips) with the original mix of the six stems: timing (cross-correlation of the onset envelopes, per section), beat-wise chroma and
mel-spectrum similarity, and per instrument: drum hit F1 within 20 ms, bass pitch agreement. Writes tools/corporations/accuracy_v2.json. Optional argument: another preview wav to score the same way."""
import os, sys, json
import numpy as np, librosa, soundfile as sf
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
def reference():
    p = os.path.join(CACHE, "ref_mix.npy")
    if not os.path.exists(p): np.save(p, sum(load(s) for s in STEMS).astype(np.float32))
    return np.load(p)
def feats(y, sr=22050):
    y = librosa.resample(y.astype(np.float32), orig_sr=SR, target_sr=sr)
    S = librosa.feature.melspectrogram(y=y, sr=sr, n_fft=2048, hop_length=441, n_mels=64, fmin=40, fmax=9000); M = np.log(S + 1e-5)
    ch = librosa.feature.chroma_cqt(y=y, sr=sr, hop_length=441, tuning=0.0)
    on = librosa.onset.onset_strength(y=y, sr=sr, hop_length=441, lag=1, max_size=3); return M, ch, on
def cosr(a, b): return float(np.mean(np.sum(a * b, 0) / (np.linalg.norm(a, axis=0) * np.linalg.norm(b, axis=0) + 1e-9)))
def evaluate(path, label):
    ref = reference(); y = sf.read(path)[0]
    if y.ndim > 1: y = y.mean(1)
    n = min(len(ref), len(y)); ref = ref[:n]; y = y[:n]
    ref = ref / np.abs(ref).max(); y = y / np.abs(y).max()
    Mr, Cr, Or = feats(ref); My, Cy, Oy = feats(y); fps = 22050 / 441; res = dict(label=label)
    sections = {"intro (0-17)": (0, 17), "verse (17-70)": (17, 70), "chorus (70-110)": (70, 110), "bridge (110-142)": (110, 142), "outro (142-196)": (142, 196)}
    sec = {}
    for name, (b0, b1) in sections.items():
        a = int(bar_time(b0 if b0 else 1) * fps); b = int(bar_time(b1) * fps); a = max(0, a); b = min(b, Mr.shape[1], My.shape[1])
        orr = Or[a:b] - Or[a:b].mean(); oy = Oy[a:b] - Oy[a:b].mean()
        c = np.correlate(orr, oy, "full") / (np.linalg.norm(orr) * np.linalg.norm(oy) + 1e-9); lags = np.arange(-len(orr) + 1, len(orr)); w = np.abs(lags) <= int(0.12 * fps)
        k = np.argmax(np.where(w, c, -9)); sec[name] = dict(onset_corr=round(float(c[k]), 3), lag_ms=round(float(lags[k] / fps * 1000), 1), chroma=round(cosr(Cr[:, a:b], Cy[:, a:b]), 3), mel=round(cosr(Mr[:, a:b] - Mr[:, a:b].mean(0), My[:, a:b] - My[:, a:b].mean(0)), 3))
    res["sections"] = sec
    res["overall"] = {k: round(float(np.mean([s[k] for s in sec.values()])), 3) for k in ("onset_corr", "chroma", "mel")}; res["overall"]["mean_abs_lag_ms"] = round(float(np.mean([abs(s["lag_ms"]) for s in sec.values()])), 1)
    return res
def drum_f1(ev, tol=0.02):
    """hit onsets of the module's drums against the onsets found in the drum stem (independent detector: spectral flux), kick+snare+tom (>150 Hz and <110 Hz bands)"""
    x = load("drums").astype(np.float64)
    on = librosa.onset.onset_detect(y=x.astype(np.float32), sr=SR, hop_length=128, units="time", backtrack=False, delta=0.08, wait=4)
    ours = np.array(sorted(v["s"] / SR for v in ev)); on = np.array(on)
    # one-to-one matching
    used = set(); tp = 0
    for t in ours:
        j = np.searchsorted(on, t); cand = [i for i in (j - 1, j) if 0 <= i < len(on) and i not in used and abs(on[i] - t) <= tol]
        if cand: i = min(cand, key=lambda i: abs(on[i] - t)); used.add(i); tp += 1
    p = tp / max(1, len(ours)); r = len(used) / max(1, len(on)); return dict(precision=round(p, 3), recall=round(r, 3), f1=round(2 * p * r / max(1e-9, p + r), 3), hits=len(ours), stem_onsets=len(on))
if __name__ == "__main__":
    out = {}
    out["v2"] = evaluate(OUT + "/preview_v2.wav", "v2 (new)")
    for o in sys.argv[1:]: out[os.path.basename(o)] = evaluate(o, os.path.basename(o))
    out["drums_v2"] = drum_f1(json.load(open(OUT + "/drums.json"))["events"])
    json.dump(out, open(OUT + "/accuracy_v2.json", "w"), indent=1)
    for k, v in out.items(): print(k, json.dumps(v if k.startswith("drums") else dict(overall=v["overall"], sections=v["sections"]))[:900])
