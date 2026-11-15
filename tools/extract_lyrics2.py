#!/usr/bin/env python3
"""Adds the Koala project's remaining sung pads to tools/duckcover/lyrics + lyric_events.json (the first pass took pads 0 2 3 4 5 8 10 11 12 13; these were missed):
pad 6 is a 20 s sung verse (played once, from beat 34.3): cut at its silences into six phrases = pads 60..65; pad 7 is a looped syllable: its long hits are pad 70, its short stabs pad 71.
usage: python3 tools/extract_lyrics2.py <Duck__Cover.koala>"""
import sys, json, io, zipfile, os, numpy as np
from scipy.io import wavfile
BPM = 145.0
z = zipfile.ZipFile(sys.argv[1]); SJ = json.loads(z.read('sampler/sampler.json')); SQ = json.loads(z.read('sequence.json'))
def region(pad):
    p = [q for q in SJ['pads'] if q['pad'] == str(pad)][0]; sid = p.get('sample') or p.get('sampleId')
    sr, d = wavfile.read(io.BytesIO(z.read('sampler/%s.wav' % sid))); d = d.astype(np.float64); d /= (2.0 ** 31 if np.abs(d).max() > 2 ** 24 else 2.0 ** 15)
    if d.ndim > 1: d = d.mean(1)
    return sr, d[p['start']:p['end']]
def save(name, sr, x): wavfile.write('tools/duckcover/lyrics/%s.wav' % name, sr, (x / max(1e-9, np.abs(x).max()) * 30000).astype('<i2'))
base = 0; hits = {6: [], 7: []}
for s in SQ['sequences'][:6]:
    for n in s['pattern']['notes']:
        if n['num'] in hits: hits[n['num']].append((base + n['timeOffset'] / 4096.0, n['length']))
    base += s['pattern']['numBars'] * 4
ev = json.load(open('tools/duckcover/lyric_events.json')); events = [e for e in ev['events'] if e['pad'] < 60]
sr, d = region(6); h = int(.01 * sr); e = np.array([np.sqrt((d[i:i + h] ** 2).mean()) for i in range(0, len(d) - h, h)]); e /= e.max(); sil = 20 * np.log10(e + 1e-6) < -38; segs = []; i = 0
while i < len(sil):
    if not sil[i]:
        j = i
        while j < len(sil) and (not sil[j] or sil[j:j + 12].mean() < 0.9): j += 1
        segs.append((i * .01, j * .01)); i = j
    else: i += 1
b0 = hits[6][0][0]
for k, (a, b) in enumerate(segs):
    x = d[int(max(0, a - .03) * sr):int((b + .06) * sr)]; save('pad_%02d' % (60 + k), sr, x); events.append({'pad': 60 + k, 't': round(b0 + a * BPM / 60, 3), 'beats': round((b - a) * BPM / 60, 3), 'vel': 0.87})
sr, d = region(7)
for pos, ln in hits[7]:
    if ln > 20000: pad, x = 70, d[:int(ln / 48000.0 * sr)]
    else: pad, x = 71, d[:int(0.11 * sr)]
    if not os.path.exists('tools/duckcover/lyrics/pad_%d.wav' % pad): save('pad_%d' % pad, sr, x)
    events.append({'pad': pad, 't': round(pos, 3), 'beats': round(len(x) / sr * BPM / 60, 3), 'vel': 0.87})
events.sort(key=lambda q: q['t']); ev['events'] = events; json.dump(ev, open('tools/duckcover/lyric_events.json', 'w'), indent=0)
print(len(events), 'events;', len(segs), 'pad 6 phrases at beats', [q['t'] for q in events if 60 <= q['pad'] < 70])
