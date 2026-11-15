#!/usr/bin/env python3
"""Cuts the 56 sung phrases of CORPORATIONS (the timing list tools/corporations_vocal_timing.json: start, end, beat) out of the isolated vocal stem as 16 kHz mono wavs
(tools/corporations/lyrics/corp_NN.wav) and writes tools/corporations/lyric_events.json (the phrase's place in beats of the 131 bpm grid the MIDI is on).
usage: python3 tools/extract_corp_lyrics.py <corporations vocals stem .flac>"""
import sys, os, json, subprocess, numpy as np
from scipy.io import wavfile
R = 16000
x = np.frombuffer(subprocess.run(['ffmpeg', '-v', 'error', '-i', sys.argv[1], '-ac', '1', '-ar', str(R), '-f', 's16le', '-'], capture_output=True, check=True).stdout, '<i2').astype(np.float64) / 32768
T = json.load(open('tools/corporations_vocal_timing.json')); os.makedirs('tools/corporations/lyrics', exist_ok=True); ev = []
for p in T['phrases']:
    a = int(max(0, p['start_s'] - 0.04) * R); b = int(min(len(x) / R, p['end_s'] + 0.18) * R); s = x[a:b].copy()
    f = int(0.012 * R); s[:f] *= np.linspace(0, 1, f); g = int(0.12 * R); s[-g:] *= np.linspace(1, 0, g)
    wavfile.write('tools/corporations/lyrics/corp_%02d.wav' % p['i'], R, (s / max(1e-9, np.abs(s).max()) * 30000).astype('<i2'))
    ev.append({'i': p['i'], 'beat': round(p['beat'] - 0.04 * T['bpm'] / 60, 4), 'dur': round(len(s) / R, 3), 'vol': p['vol_q8']})      # (the 40 ms of lead-in moves the start a little earlier)
json.dump({'bpm': T['bpm'], 'events': ev}, open('tools/corporations/lyric_events.json', 'w'), indent=0); print(len(ev), 'phrases,', round(sum(e['dur'] for e in ev), 1), 's')
