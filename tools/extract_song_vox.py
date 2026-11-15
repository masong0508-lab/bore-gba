#!/usr/bin/env python3
"""Cuts the sung / sampled vocal material of CORPORATIONS and DUCK & COVER into small mono wavs for godguy/tools/gm_vox.py (which pushes them hard and bakes the chop kit).
usage: python3 tools/extract_song_vox.py <corporations vocals stem .flac> <Duck__Cover.koala>
  CORPORATIONS: the isolated vocal stem is split at its quiet gaps / onsets (inside the lines in tools/corporations_vocal_timing.json) into 0.25 - 0.9 s pieces; the 14 loudest, spread over the song, are kept.
  DUCK & COVER: every sample pad's play window in the Koala project, cut in the same way; the 14 loudest are kept."""
import sys, os, io, json, zipfile, subprocess
import numpy as np
from scipy.io import wavfile
R = 11025
def pieces(x, sr, lo=0.25, hi=0.9, want=14, spread=True):
    hop = int(sr * 0.01); n = len(x) // hop; env = np.array([np.sqrt(np.mean(x[i * hop:(i + 1) * hop] ** 2)) for i in range(n)]); thr = max(env.max() * 0.08, 1e-4)
    # onsets: rises of the envelope; a piece runs from an onset to the next one (or until it gets too long / too quiet)
    on = [i for i in range(2, n - 2) if env[i] > thr and env[i] > env[i - 2] * 1.6 and env[i - 1] <= env[i]]; cut = []; last = -99
    for i in on:
        if i - last >= int(lo * 100): cut.append(i); last = i
    out = []
    for a, b in zip(cut, cut[1:] + [min(n, cut[-1] + int(hi * 100))] if cut else []):
        b = min(b, a + int(hi * 100)); seg = x[a * hop:b * hop]
        if len(seg) / sr >= lo * 0.8 and np.sqrt(np.mean(seg ** 2)) > thr: out.append((float(np.sqrt(np.mean(seg ** 2))), a, seg))
    out.sort(key=lambda t: -t[0]); keep = out[:want * 3]
    if spread: keep = sorted(keep, key=lambda t: t[1]); keep = [keep[i] for i in np.linspace(0, len(keep) - 1, min(want, len(keep))).astype(int)]
    return keep
def save(path, seg, sr):
    seg = seg / (np.abs(seg).max() or 1) * 0.9; fa = int(0.004 * sr); fb = int(0.02 * sr); seg = seg.copy(); seg[:fa] *= np.linspace(0, 1, fa); seg[-fb:] *= np.linspace(1, 0, fb)
    wavfile.write(path, sr, (seg * 32000).astype('<i2'))
if __name__ == '__main__':
    stem, koala = sys.argv[1], sys.argv[2]
    os.makedirs('tools/voices_corp', exist_ok=True); os.makedirs('tools/duckcover/vox_src', exist_ok=True)
    raw = subprocess.run(['ffmpeg', '-v', 'error', '-i', stem, '-ac', '1', '-ar', str(R), '-f', 's16le', '-'], capture_output=True, check=True).stdout; x = np.frombuffer(raw, '<i2').astype(np.float64) / 32768
    T = json.load(open('tools/corporations_vocal_timing.json'))['phrases']; sel = np.zeros(len(x), bool)
    for p in T: sel[int(p['start_s'] * R):int(p['end_s'] * R)] = True
    for i, (e, a, seg) in enumerate(pieces(x * sel, R)): save('tools/voices_corp/corp_%02d.wav' % i, seg, R); print('corp', i, round(a / 100, 1), 's', round(len(seg) / R, 2))
    z = zipfile.ZipFile(koala); SJ = json.loads(z.read('sampler/sampler.json')); allp = []
    for p in SJ['pads']:
        sid = p.get('sample') or p.get('sampleId')
        if not sid: continue
        sr, d = wavfile.read(io.BytesIO(z.read('sampler/%s.wav' % sid))); d = d.astype(np.float64); d /= (2.0 ** 31 if np.abs(d).max() > 2 ** 24 else 2.0 ** 15)
        if d.ndim > 1: d = d.mean(1)
        s0, s1 = p.get('start') or 0, p.get('end') or len(d); d = d[s0:s1]
        from scipy.signal import resample_poly
        from math import gcd
        g = gcd(R, sr); d = resample_poly(d, R // g, sr // g)
        for e, a, seg in pieces(d, R, want=6, spread=False): allp.append((e, p['pad'], a, seg))
    allp.sort(key=lambda t: -t[0])
    for i, (e, pad, a, seg) in enumerate(allp[:14]): save('tools/duckcover/vox_src/duck_%02d_pad%s.wav' % (i, pad), seg, R); print('duck', i, 'pad', pad, round(len(seg) / R, 2))
