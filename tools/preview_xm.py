#!/usr/bin/env python3
"""Renders a tracker song the way the GBA mixer will play it (same samples, steps, volumes, pan buses, 18157 Hz, same clipping) to a stereo WAV.
usage: python3 tools/preview_xm.py song_id tools/song.xm out.wav     (run from the project root)"""
import sys, os, wave
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import xm2gba as X
from xm import parse
sid, path, out = sys.argv[1:4]
S = parse(path)
for table in (X.DANCES, X.TREES, X.POPS, X.CLOUDS, X.ENDINGS):   # the same re-arrangements the converter applies, so the preview matches the ROM
    if sid in table: table[sid](S)
used = {i for o in S['order'] for r in S['pats'][o] for (n, i, v, e, ep) in r if n and n < 97}
insts = X.convert_samples(S, used, sid)
panf = X.design_pan(S, used, insts, sid)                          # the same stereo plan the converter bakes into the ROM
bg = X.bus_gains(panf.trim); GL = [g[0] / 128.0 for g in bg]; GR = [g[1] / 128.0 for g in bg]
rowsec = S['tempo'] * 2.5 / S['bpm']; M = X.MIXR
total = int(sum(len(S['pats'][o]) for o in S['order']) * rowsec * M) + M * 4
accL = np.zeros(total); accR = np.zeros(total); cur = {}; pos = 0.0; clip_rows = 0
def stop(ch, upto):
    if ch in cur:
        start, w, b = cur.pop(ch); n = min(len(w), upto - start)
        if n > 0: accL[start:start + n] += w[:n] * GL[b]; accR[start:start + n] += w[:n] * GR[b]
for o in S['order']:
    for ri, r in enumerate(S['pats'][o]):
        t0 = int(pos)
        for ch, (n, i, v, e, ep) in enumerate(r):
            if n and n < 97 and insts[i - 1]:
                I = insts[i - 1]; rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
                vol = min(127, max(1, int(round(I['svol'] * I['pk'] * rel * X.GAIN.get(sid, 1.0)))))
                stop(ch, t0)
                q = I['q'].astype(float); st = I['steps'][n - 1] / 65536.0
                ln = int((len(q) - 1) / st); idx = np.arange(ln) * st
                w = np.interp(idx, np.arange(len(q)), q) * vol / 64.0
                cur[ch] = (t0, w, panf(o, ri, ch, i, n))
        pos += rowsec * M
for ch in list(cur): stop(ch, total)
y = np.stack([accL, accR], 1) / 4.0                                   # the mixer's >>2, then each side clipped to 8 bits like the FIFOs
peak = np.abs(y).max(); clipped = (np.abs(y) > 127).mean() * 100
y = np.clip(np.round(y), -128, 127)
print("duration %.1f s  peak %.0f/127  rms %.1f  clipped samples %.3f%%  (stereo)" % (len(y) / M, peak, np.sqrt((y ** 2).mean()), clipped))
w = wave.open(out, 'wb'); w.setnchannels(2); w.setsampwidth(2); w.setframerate(M); w.writeframes((y * 256).astype('<i2').tobytes()); w.close()
