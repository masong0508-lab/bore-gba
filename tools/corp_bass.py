#!/usr/bin/python3
"""CORPORATIONS re-analysis, the BASS: one pitch per tracker tick from the stem itself (harmonic-sum f0 on 85 ms windows, highest candidate that explains the spectrum, so no octave-below errors),
the 45-cent flat tuning removed, plucks found by the attack of the 250-1500 Hz band, notes = runs of the same pitch between plucks. Output tools/corporations/bass.json (global ticks)."""
import os, sys, json
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from corp_lib import *
import corp_bass_f0 as F
BT = -0.455
def main():
    x = F.x; full = env(x, 6.0); att = env(bp(x, 250, 1500), 3.0)
    notes = []; last = None
    t_end = len(x) / SR - 1.0
    # attacks over the whole song
    hop = 16; ee = att[::hop]; le = np.log(ee + 1.5e-3); d = np.concatenate([[0] * 5, le[5:] - le[:-5]]); w = int(0.07 * SR / hop); atk = []; i = 0
    while i < len(d):
        if d[i] > 0.75 and d[i] >= d[max(0, i - w):i + w].max() and ee[min(len(ee) - 1, i + 12)] > 0.006: atk.append(i * hop / SR); i += w
        else: i += 1
    atk = np.array(atk); print(len(atk), "attacks")
    cur = None
    g0 = time_to_gtick(0.3); g1 = time_to_gtick(t_end)
    for g in range(g0, g1):
        t = gtick_time(g); r = F.f0_seg(t - 0.0425 + 0.02, t + 0.0425 + 0.02); lev = full[int((t + 0.02) * SR)]
        if r is None or lev < 0.02: m = None
        else: m = int(round(69 + 12 * np.log2(r[0] / 440.0) - BT))
        near = np.any(np.abs(atk - (t - 0.002)) < TICK * 0.55)
        if m is None:
            if cur: cur["end"] = g; notes.append(cur); cur = None
            continue
        if cur is None or m != cur["pitch"] or near:
            if cur: cur["end"] = g; notes.append(cur)
            cur = dict(start=g, pitch=m, lev=float(lev))
        else: cur["lev"] = max(cur["lev"], float(lev))
    if cur: cur["end"] = g1; notes.append(cur)
    notes = [n for n in notes if n["end"] - n["start"] >= 2 and 28 <= n["pitch"] <= 62]
    lo, hi = np.percentile([n["lev"] for n in notes], [5, 98])
    for n in notes: n["vel"] = int(np.clip(50 + 77 * (n["lev"] - lo) / max(1e-6, hi - lo), 40, 127))
    json.dump(dict(tuning_cents=BT * 100, notes=notes), open(OUT + "/bass.json", "w"))
    print(len(notes), "bass notes")
if __name__ == "__main__": main()
