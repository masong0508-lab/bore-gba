#!/usr/bin/env python3
"""HERE TODAY (MISSY'S SONG): a chiptune rendition of the original melody hummed by the game's author (the melody comes from a voice recording, tracked
note by note; the lyrics in source/cutscene.h are original too). Run from the project root:   python3 tools/make_heretoday.py
  1. writes /tmp/here_today.wav   2. encode it with  python3 tools/encode_song.py /tmp/here_today.wav  (or use encode_to_adp below, which does both)
Edit PHRASES to change the tune: each phrase is [start s, end s, [[start s, length s, MIDI note], ...]]. The cutscene (cutscene.h, scene 5) shows one lyric line per
phrase and its beat lengths are worked out from these phrase starts, so if you move a phrase, change that beat's length too (see the comment on the lyric beats).
Sound: a 25 % pulse lead one octave up with a faint echo, a triangle bass an octave below the written note on the longer notes, a soft vibrato on held notes."""
import numpy as np, wave, sys
SR = 44100
PHRASES = [[0.15, 1.35, [[0.15, 1.07, 59],
  [1.22, 0.13, 56]]],
  [2.21, 3.25, [[2.21, 0.22, 56],
  [2.43, 0.25, 59],
  [3.02, 0.23, 60]]],
  [4.91, 7.22, [[4.91, 0.19, 62],
  [5.33, 0.13, 62],
  [5.84, 0.16, 58],
  [6.0, 0.13, 59],
  [6.28, 0.16, 57],
  [6.44, 0.25, 55],
  [6.89, 0.33, 55]]],
  [8.39, 10.03, [[8.39, 0.22, 55],
  [8.81, 0.21, 57],
  [9.25, 0.57, 55],
  [9.82, 0.21, 54]]],
  [11.02, 14.04, [[11.02, 0.16, 54],
  [11.18, 0.88, 55],
  [12.06, 1.07, 53],
  [13.13, 0.14, 51],
  [13.27, 0.77, 50]]],
  [15.09, 16.62, [[15.09, 0.26, 50],
  [15.54, 0.3, 51],
  [16.12, 0.5, 47]]],
  [18.82, 20.73, [[18.82, 0.14, 46],
  [19.17, 0.15, 48],
  [19.66, 0.64, 48],
  [20.57, 0.16, 52]]],
  [22.11, 23.89, [[22.11, 0.33, 50],
  [22.85, 0.5, 53],
  [23.74, 0.15, 50]]],
  [26.09, 27.54, [[26.09, 0.39, 50],
  [26.48, 0.18, 51],
  [26.94, 0.3, 53],
  [27.41, 0.13, 51]]],
  [28.3, 31.96, [[28.3, 0.3, 55],
  [28.79, 0.2, 51],
  [29.23, 0.28, 51],
  [29.88, 0.37, 55],
  [30.64, 0.13, 59],
  [31.15, 0.32, 59],
  [31.63, 0.19, 63],
  [31.82, 0.14, 64]]],
  [32.84, 33.84, [[32.84, 0.37, 67],
  [33.21, 0.2, 66],
  [33.62, 0.22, 67]]],
  [35.56, 38.01, [[35.56, 1.15, 67],
  [36.93, 0.15, 64],
  [37.3, 0.36, 64],
  [37.66, 0.35, 63]]],
  [40.21, 42.59, [[40.21, 0.27, 67],
  [40.48, 0.18, 69],
  [40.91, 0.17, 69],
  [41.08, 0.7, 71],
  [41.78, 0.31, 72],
  [42.35, 0.24, 71]]],
  [43.8, 45.44, [[43.8, 0.25, 69],
  [44.22, 0.2, 71],
  [44.67, 0.41, 72],
  [45.28, 0.16, 74]]],
  [47.64, 48.17, [[47.64, 0.14, 66],
  [47.78, 0.27, 67],
  [48.05, 0.12, 69]]],
  [48.88, 49.51, [[48.88, 0.33, 66],
  [49.37, 0.14, 64]]],
  [50.38, 55.09, [[50.38, 0.17, 65],
  [50.55, 1.19, 67],
  [52.15, 1.2, 65],
  [53.8, 0.62, 67],
  [54.42, 0.13, 65],
  [54.7, 0.39, 65]]],
  [55.6, 57.95, [[55.6, 0.53, 63],
  [56.13, 0.23, 62],
  [56.52, 0.58, 62],
  [57.1, 0.13, 60],
  [57.67, 0.28, 59]]],
  [60.15, 65.07, [[60.15, 0.19, 55],
  [60.62, 1.22, 55],
  [62.21, 1.32, 53],
  [63.99, 1.08, 51]]],
  [65.77, 70.65, [[65.77, 0.84, 50],
  [66.87, 1.09, 48],
  [68.16, 0.5, 47],
  [69.14, 0.23, 48],
  [69.7, 0.13, 47],
  [70.17, 0.48, 47]]]]
def midi_hz(m): return 440.0 * 2 ** ((m - 69) / 12.0)
def render():
    end = PHRASES[-1][1] + 1.0
    out = np.zeros(int(SR * (end + 0.5)))
    for ps, pe, notes in PHRASES:
        for ns, nl, m in notes:
            i0 = int(ns * SR); n = int(max(nl, 0.12) * SR); t = np.arange(n) / SR
            vib = 1 + (0.006 * np.sin(2 * np.pi * 5.5 * t) * np.clip((t - 0.25) * 4, 0, 1) if nl > 0.5 else 0)
            f = midi_hz(m + 12) * vib; ph = np.cumsum(f) / SR; lead = np.where((ph % 1.0) < 0.25, 1.0, -1.0)
            env = np.minimum(t / 0.006, 1.0) * np.exp(-t * 1.6) * 0.7 + 0.3 * np.minimum(t / 0.006, 1.0); env *= np.clip((n / SR - t) / 0.05, 0, 1)
            seg = lead * env * 0.30
            out[i0:i0 + n] += seg[:len(out) - i0]
            j0 = i0 + int(0.23 * SR); out[j0:j0 + n] += (seg * 0.22)[:max(0, len(out) - j0)]   # the echo
            if nl >= 0.4:
                fb = midi_hz(m - 12); pb = np.cumsum(np.full(n, fb)) / SR; tri = 4 * np.abs((pb % 1.0) - 0.5) - 1
                out[i0:i0 + n] += (tri * env * 0.35)[:len(out) - i0]
    out /= max(1e-9, np.abs(out).max()) / 0.9
    return out
def write(path):
    x = render(); w = wave.open(path, 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR)
    w.writeframes((x * 32767).astype('<i2').tobytes()); w.close(); return len(x) / SR
if __name__ == '__main__':
    p = sys.argv[1] if len(sys.argv) > 1 else '/tmp/here_today.wav'
    print('wrote', p, round(write(p), 1), 's')
