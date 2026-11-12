"""Scores a note list against its stem: per-beat chroma and per-beat semitone-profile cosine (tuning-corrected), plus onset agreement. Used to compare transcriptions."""
import os, sys, json
import numpy as np, librosa
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
def stem_semis(stem, tune, lo=36, hi=96, sr=22050, hop=512):
    x = load(stem); y = librosa.resample(x.astype(np.float32), orig_sr=SR, target_sr=sr)
    fmin = librosa.midi_to_hz(lo + tune) * 2 ** (-1 / 36)
    C = np.abs(librosa.cqt(y, sr=sr, hop_length=hop, fmin=fmin, n_bins=(hi - lo + 1) * 3, bins_per_octave=36, filter_scale=0.8))
    return np.sqrt((C.reshape(hi - lo + 1, 3, -1) ** 2).sum(1)), hop / sr
def score(notes, stem, tune, ranges, lo=36, hi=96):
    """notes: dicts with start/end (global ticks) and pitch. Compares in windows of one beat (24 ticks) inside the given bar ranges."""
    V, dt = stem_semis(stem, tune, lo, hi); ch = []; sp = []
    for (B0, B1) in ranges:
        for g in range(gtick(B0, 0), gtick(B1, 0), 24):
            ta = gtick_time(g); tb = gtick_time(g + 24); fa = int(ta / dt); fb = max(fa + 1, int(tb / dt))
            s = V[:, fa:fb].mean(1)
            if s.max() < 1e-3: continue
            n = np.zeros(hi - lo + 1)
            for u in notes:
                o = min(u["end"], g + 24) - max(u["start"], g)
                if o > 0 and lo <= u["pitch"] <= hi: n[u["pitch"] - lo] += o
            if n.sum() == 0: sp.append(0); ch.append(0); continue
            def cos(a, b): return float(a @ b / (np.linalg.norm(a) * np.linalg.norm(b) + 1e-12))
            sc = np.array([s[i::12].sum() for i in range(12)]); sc = np.roll(sc, 0); nc = np.array([n[i::12].sum() for i in range(12)])
            # chroma bins: both folded from lo (same alignment)
            ch.append(cos(sc, nc)); sp.append(cos(s ** 0.7, n))
    return dict(chroma=round(float(np.mean(ch)), 3), semitone=round(float(np.mean(sp)), 3), windows=len(ch))
if __name__ == "__main__":
    stem = sys.argv[1]; tune = -0.04
    R = {"piano": [(26, 34), (54, 70), (86, 94)], "other": [(38, 54), (110, 126)]}[stem]
    new = json.load(open("%s/keys_%s.json" % (OUT, stem)))["notes"]
    print("new  :", score(new, stem, tune, R))
    old = json.load(open("%s/notes.json" % OUT))[stem] if os.path.exists(OUT + "/notes.json") else []
    # the old notes are in seconds on the old (early) grid: convert by time
    oldn = [dict(start=time_to_gtick(n["t0"]), end=time_to_gtick(n["t1"]), pitch=n["pitch"]) for n in old]
    print("old  :", score(oldn, stem, tune, R))
