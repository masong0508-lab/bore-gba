#!/usr/bin/env python3
"""A recorded voice clip (m4a, mp3, wav, anything ffmpeg reads) -> tools/voices_src/<name>.wav, in the spec of the clips already there:
8000 Hz, mono, 16 bit, a 70 Hz high-pass against rumble, the silence trimmed off both ends (-42 dB under the peak, 25 ms kept), 8 ms fades, the peak at 29,000.
usage (from the project root):  python3 tools/prep_voice.py <name> <file> [<name> <file> ...]      then:  python3 tools/encode_voices.py
Needs ffmpeg and numpy/scipy. Prints what it did to each clip (a clip whose source clipped is reported)."""
import sys, os, subprocess, wave
import numpy as np
from scipy.signal import butter, sosfilt
SR, PEAK = 8000, 29000
def prep(name, path, out="tools/voices_src"):
    raw = subprocess.run(["ffmpeg", "-v", "error", "-i", path, "-ac", "1", "-ar", str(SR), "-f", "s16le", "-"], capture_output=True, check=True).stdout
    x = np.frombuffer(raw, dtype="<i2").astype(np.float64)
    clipped = int(np.sum(np.abs(x) >= 32766))
    x = sosfilt(butter(2, 70, "hp", fs=SR, output="sos"), x)
    a = np.abs(x); thr = a.max() * 10 ** (-42 / 20); idx = np.where(a > thr)[0]
    keep = int(0.025 * SR); lo, hi = max(0, idx[0] - keep), min(len(x), idx[-1] + keep)
    x = x[lo:hi]; f = int(0.008 * SR); ramp = np.linspace(0, 1, f); x[:f] *= ramp; x[-f:] *= ramp[::-1]
    x = x * (PEAK / np.abs(x).max()); x = np.round(np.clip(x, -32768, 32767)).astype("<i2")
    os.makedirs(out, exist_ok=True)
    with wave.open(f"{out}/{name}.wav", "wb") as w: w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes(x.tobytes())
    print(f"{name}: {len(x)/SR:.2f} s (trimmed {lo/SR:.2f} s from the start, {(len(raw)//2-hi)/SR:.2f} s from the end){'  SOURCE CLIPPED ' + str(clipped) + ' samples' if clipped > 2 else ''}")
if __name__ == "__main__":
    a = sys.argv[1:]
    if len(a) < 2 or len(a) % 2: sys.exit(__doc__)
    for i in range(0, len(a), 2): prep(a[i], a[i + 1])
