#!/usr/bin/env python3
"""Makes the two weather / ghost sound effects from scratch (no recordings): thunder.adp and ghost.adp.
usage: python3 tools/make_fx_sfx.py    (needs numpy; run from the repo root)  -> source/sfx/thunder.adp, source/sfx/ghost.adp
Same 6554 Hz mono 4-bit IMA-ADPCM as every other effect (tools/encode_sfx.py)."""
import sys, os
import numpy as np
sys.path.insert(0, os.path.join(os.path.dirname(__file__)))
from encode_sfx import encode, RATE

rng = np.random.default_rng(7)

def lowpass(x, k):                       # one-pole low pass, k in 0..1 (smaller = darker)
    y = np.zeros_like(x); a = 0.0
    for i, v in enumerate(x):
        a += k * (v - a); y[i] = a
    return y

def thunder():
    n = int(RATE * 2.4); t = np.arange(n) / RATE
    crack = rng.standard_normal(n) * np.exp(-t * 28)                      # the sharp first crack
    roll = lowpass(rng.standard_normal(n), 0.05)                          # the rumble: dark noise ...
    roll *= (0.55 + 0.45 * np.sin(2 * np.pi * 3.1 * t + 1.0)) * np.exp(-t * 1.35) * np.minimum(1, t * 14)   # ... that rolls and dies away
    x = 0.55 * crack + 1.0 * roll * 6.0
    x = lowpass(x, 0.5)
    x = x / np.max(np.abs(x)) * 0.85
    return (x * 32767).astype(np.int32)

def ghost():
    n = int(RATE * 1.5); t = np.arange(n) / RATE
    f = 330 + 150 * np.sin(np.pi * t / 1.5) - 60 * t + 14 * np.sin(2 * np.pi * 5.5 * t)   # a wavering OOOH: up, over, down
    ph = 2 * np.pi * np.cumsum(f) / RATE
    x = np.sin(ph) + 0.35 * np.sin(2 * ph) + 0.12 * np.sin(3 * ph)
    env = np.minimum(1, t / 0.25) * np.minimum(1, (1.5 - t) / 0.45)
    x = x * env * (0.8 + 0.2 * np.sin(2 * np.pi * 7 * t))
    echo = np.zeros_like(x); d = int(RATE * 0.21); echo[d:] = x[:-d] * 0.45
    x = x + echo
    x = x / np.max(np.abs(x)) * 0.8
    return (x * 32767).astype(np.int32)

out = os.path.join(os.path.dirname(__file__), "..", "source", "sfx")
for name, fn in (("thunder", thunder), ("ghost", ghost)):
    b = encode(fn())
    with open(os.path.join(out, name + ".adp"), "wb") as f: f.write(b)
    print(name, len(b), "bytes")
