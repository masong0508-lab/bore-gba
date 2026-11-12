#!/usr/bin/env python3
"""CORPORATIONS re-analysis, the KEYS (the Rhodes of the piano stem and of the 'other' stem; an instrument tagger and a text-audio model both call it a Rhodes, not a grand piano or a synth).
Instead of a neural note model (which hears every overtone and echo as another note: the 'cluster') the stem is decomposed harmonically: a semitone spectrum of the stem at the keys' own tuning,
harmonic non-negative factorisation (each note = a fundamental + its octave/twelfth partials, with the weights learned from the stem), then notes = activations that stand out from their neighbourhood.
Per tracker tick: a note starts when its activation rises sharply, stays while it is above a fraction of the loudest note around it; short and weak ones go; at most MAXV voices.
usage: python3 tools/corp_keys.py piano|other      -> tools/corporations/keys_<stem>.json"""
import os, sys, json
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
import corp_hnmf as H

def transcribe(stem, ranges, lo=40, hi=88, tune=-0.04, maxv=5, rel=0.16, absmin=0.05, min_ticks=3):
    x = load(stem).astype(np.float64); notes = []
    for (B0, B1) in ranges:
        t0 = bar_time(B0) - 0.4; t1 = bar_time(B1) + 0.6
        V, dt = H.semispec(x, t0, t1, lo, hi, tune, sr=22050, hop=256)
        Hh, a = H.hnmf(V, hi - lo + 1, iters=60, a0=[1, 0.30, 0.10, 0.04, 0.02, 0.01, 0, 0, 0, 0], learn=False, sparse=0.02 * V.max())      # fixed partial weights: a learned fit puts the notes an octave low
        print("  %s bars %d-%d: partial weights %s" % (stem, B0, B1, np.round(a[:4], 2)))
        nm, T = Hh.shape; ref = Hh.max()
        # a note's strength: its own activation, relative to the loudest voice in a 0.5 s neighbourhood
        k = max(1, int(0.25 / dt)); from scipy.ndimage import maximum_filter1d
        loud = maximum_filter1d(Hh.max(0), size=2 * k + 1)
        act = (Hh >= rel * loud[None, :]) & (Hh >= absmin * ref * 0.1)
        # smooth gaps of one frame
        for m in range(nm):
            row = act[m]; r2 = row.copy(); r2[1:-1] |= row[:-2] & row[2:]; act[m] = r2
        for m in range(nm):
            row = act[m]; i = 0
            while i < T:
                if not row[i]: i += 1; continue
                j = i
                while j < T and row[j]: j += 1
                lev = Hh[m, i:j]; pk = float(lev.max())
                # split a long run at a sharp re-attack (activation more than doubles after a dip)
                cut = [i]
                for u in range(i + 3, j - 3):
                    if lev[u - i] > 2.0 * lev[max(0, u - i - 4):u - i].min() and lev[u - i] >= lev[u - i - 1] and lev[u - i] > 0.6 * pk and lev[u - i - 3:u - i].min() < 0.55 * lev[u - i]: cut.append(u)
                cut.append(j)
                for ca, cb in zip(cut[:-1], cut[1:]):
                    ta = t0 + ca * dt; tb = t0 + cb * dt
                    if ta < bar_time(B0) - 0.05 or ta > bar_time(B1): continue
                    notes.append(dict(t0=ta, t1=tb, pitch=lo + m, amp=float(Hh[m, ca:cb].max() / ref)))
                i = j
    # to ticks, clean
    out = []
    for n in notes:
        g0 = time_to_gtick(n["t0"] + 0.012); g1 = max(g0 + 1, time_to_gtick(n["t1"]))
        if g1 - g0 < min_ticks or n["amp"] < absmin: continue
        out.append(dict(start=g0, end=g1, pitch=n["pitch"], amp=n["amp"]))
    out.sort(key=lambda n: (n["start"], -n["amp"]))
    # voices: at each onset keep the strongest maxv sounding notes; octave ghosts (a note exactly 12 above a louder one sounding with it, much weaker) go
    kept = []; active = []
    for n in out:
        active = [u for u in active if u["end"] > n["start"]]
        if any(u["pitch"] == n["pitch"] - 12 and n["amp"] < 0.5 * u["amp"] and abs(u["start"] - n["start"]) <= 2 for u in active): continue
        if len(active) >= maxv:
            w = min(active, key=lambda u: u["amp"])
            if w["amp"] >= n["amp"]: continue
            w["end"] = min(w["end"], n["start"]); active.remove(w)
        kept.append(n); active.append(n)
    kept = [n for n in kept if n["end"] - n["start"] >= min_ticks]
    # merge same pitch re-strikes closer than 2 ticks
    kept.sort(key=lambda n: (n["pitch"], n["start"])); merged = []
    for n in kept:
        if merged and merged[-1]["pitch"] == n["pitch"] and n["start"] - merged[-1]["end"] <= 1: merged[-1]["end"] = n["end"]; merged[-1]["amp"] = max(merged[-1]["amp"], n["amp"])
        else: merged.append(n)
    merged.sort(key=lambda n: (n["start"], n["pitch"]))
    mx = max(n["amp"] for n in merged)
    for n in merged: n["vel"] = int(np.clip(45 + 80 * (n["amp"] / mx) ** 0.6, 40, 127))
    return merged

RANGES = {
    "piano": [(25, 35), (53, 70), (81, 94), (114, 126), (133, 142), (172, 180)],
    "other": [(0, 10), (21, 26), (37, 75), (110, 142)],      # the Rhodes / vibes / pad parts of the 'other' stem; the horn, slide-guitar and sax lines of the other bars stay digital
}
def main():
    stem = sys.argv[1]; nt = transcribe(stem, RANGES[stem], maxv=5 if stem == "piano" else 4)
    json.dump(dict(stem=stem, notes=nt), open("%s/keys_%s.json" % (OUT, stem), "w"))
    print(stem, len(nt), "notes; mean polyphony per tick %.2f" % (sum(n["end"] - n["start"] for n in nt) / max(1, max(n["end"] for n in nt) - min(n["start"] for n in nt))))
if __name__ == "__main__": main()
