"""Shared pieces of the CORPORATIONS re-analysis: the stems, the song's real bar grid (with its two splices), tick maths, small DSP helpers.

The grid (measured from the drum machine's hits, which repeat as identical waveforms):
  * 131.004 BPM, sixteenths 114.50 ms apart; the snare is on beats 2 and 4 and the four-bar phrases (fills on every 4th / 8th bar) fix the bar line: bar B starts at
    PH0 + (11 + 16 * (B - 1)) sixteenths. (The first version of this conversion had the bar line an eighth note early and put the notes on straight 1/32 rows.)
  * the off sixteenths are SWUNG: they land about 32 ms late (64 % swing), so the transcription keeps time in tracker ticks (1 tick = 1/6 of a sixteenth = 19.08 ms)
  * the master has two edits: from bar 158 everything is 0.905 of a sixteenth late (104 ms), from bar 174 it is 0.17 of a sixteenth late (19 ms). The tracker timeline follows them
    with +5 ticks and +1 tick."""
import os, sys, subprocess
import numpy as np
from scipy import signal

SR = 44100
SRC = os.environ.get("STEM_DIR", "/root/.claude/uploads/2b9164b2-7e8c-59d8-8b67-0e06180c3de2")
CACHE = os.environ.get("CORP_CACHE", "/tmp/corp_cache")
STEMS = ["bass", "drums", "other", "guitar", "piano", "vocals"]
OUT = "tools/corporations"
GBA = 18157

BPM = 131.004; S16 = 60.0 / BPM / 4; PH0 = 0.0299
TICK = S16 / 6.0                      # tracker tick: speed 3, 32 rows per bar, 96 ticks per bar
TPB = 96                              # ticks per bar
SPLICE = [(11 + 16 * 157 + 8, 0.905 * S16, 5), (11 + 16 * 173, 0.17 * S16, 1)]      # (first sixteenth index, true offset in seconds, offset on the tracker timeline in ticks): bar 158 beat 3, bar 174
FIRST_BAR = 0                         # the module starts at bar 0 (0.54 s before the first sound)
NOTE = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

def nn(m): return "%s%d" % (NOTE[int(round(m)) % 12], int(round(m)) // 12 - 1)

def find_stem(name):
    for f in os.listdir(SRC):
        if f.endswith("_%s_mvsep.com.flac" % name): return os.path.join(SRC, f)
    raise SystemExit("stem not found: " + name)

def load(name, stereo=False):
    os.makedirs(CACHE, exist_ok=True); p = os.path.join(CACHE, "%s_%s.npy" % (name, "st" if stereo else "mono"))
    if not os.path.exists(p):
        raw = subprocess.run(["ffmpeg", "-v", "error", "-i", find_stem(name), "-ac", "2", "-ar", str(SR), "-f", "f32le", "-"], capture_output=True, check=True).stdout
        x = np.frombuffer(raw, dtype="<f4").reshape(-1, 2)
        np.save(os.path.join(CACHE, "%s_st.npy" % name), x); np.save(os.path.join(CACHE, "%s_mono.npy" % name), x.mean(axis=1).astype(np.float32))
    return np.load(p)

# ---------------------------------------------------------------------------------------------- the grid
def _offs(k):
    sec, tk = 0.0, 0
    for k0, s_, t_ in SPLICE:
        if k >= k0 - 1e-6: sec, tk = s_, t_
    return sec, tk
def bar_time(B, slot=0.0):
    """true time of sixteenth `slot` (may be fractional) of bar B"""
    k = 11 + 16 * (B - 1) + slot; return PH0 + k * S16 + _offs(k)[0]
def pos_of(t):
    """true time -> (bar, tick in the bar as a float 0..96)"""
    k = (t - PH0) / S16
    for k0, sec, _ in reversed(SPLICE):
        kk = (t - sec - PH0) / S16
        if kk >= k0 - 0.5: k = kk; break
    k -= 11; B = int(np.floor(k / 16.0)) + 1; return B, (k - 16 * (B - 1)) * 6.0
def gtick(B, tick):
    """global tick on the tracker timeline (uniform ticks; the splices are whole ticks)"""
    return TPB * (B - FIRST_BAR) + int(tick) + _offs(11 + 16 * (B - 1) + tick / 6.0)[1]
T_ORIGIN = PH0 + (11 + 16 * (FIRST_BAR - 1)) * S16          # time of global tick 0
def gtick_time(g): return T_ORIGIN + g * TICK
def time_to_gtick(t):
    B, tk = pos_of(t); tk = int(round(tk))
    if tk >= TPB: B += 1; tk -= TPB
    return gtick(B, tk)
def swing_ticks(tk):
    """a tick position in the bar as (sixteenth, ticks late)"""
    return int(tk // 6), int(tk % 6)

# ---------------------------------------------------------------------------------------------- DSP helpers
def bp(y, lo, hi, order=4): return signal.sosfiltfilt(signal.butter(order, [lo, hi], "bp", fs=SR, output="sos"), y)
def lp(y, f, order=4): return signal.sosfiltfilt(signal.butter(order, f, "lp", fs=SR, output="sos"), y)
def hp(y, f, order=4): return signal.sosfiltfilt(signal.butter(order, f, "hp", fs=SR, output="sos"), y)
def env(y, ms=8.0):
    n = max(1, int(ms / 1000.0 * SR)); return np.sqrt(np.convolve(y * y, np.ones(n) / n, "same"))
def db(v): return 20 * np.log10(max(1e-9, float(v)))
def rms(v): return float(np.sqrt(np.mean(np.asarray(v, dtype=np.float64) ** 2) + 1e-18))
def fade(y, a=0.002, b=0.01):
    y = np.array(y, dtype=np.float64); na = min(int(a * SR), len(y) // 2); nb = min(int(b * SR), len(y) // 2)
    if na: y[:na] *= np.linspace(0, 1, na)
    if nb: y[-nb:] *= np.linspace(1, 0, nb)
    return y
