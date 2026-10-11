#!/usr/bin/env python3
"""Synthesise BORTUA FIGHTER's sounds (no recording needed) -> source/sfx/toot.adp
toot = the MOON gone wrong: a buzzy, wobbling brass raspberry that sags in pitch, flutters, and ends in a little squeak
usage: python3 tools/make_bortua_sfx.py"""
import os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from encode_sfx import encode, RATE

rng = np.random.default_rng(3)
def t(n): return np.arange(int(RATE*n)) / RATE
def lp(x, k):   # one pole low pass
    y = np.zeros_like(x); s = 0.0
    for i, v in enumerate(x): s += (v - s) * k; y[i] = s
    return y
def fin(x, peak=0.9):
    x = x / (np.max(np.abs(x)) + 1e-9) * peak
    f = min(len(x), int(RATE*0.02)); x[-f:] *= np.linspace(1, 0, f)
    return (x * 32767).astype(np.int32)

def toot():
    x = t(0.95); n = len(x)
    f0 = 150 * np.exp(-x * 0.9) + 20 + 9 * np.sin(2*np.pi*5.5*x)                    # sags, wobbles
    f0 = f0 * (1 + 0.12 * np.sin(2*np.pi*31*x) * (x > 0.25))                         # then the flutter
    sq = np.where(x > 0.78, (x - 0.78) / 0.17, 0); f0 = f0 + sq**2 * 380              # ... and the squeak at the end
    ph = np.cumsum(2*np.pi*f0 / RATE)
    buzz = np.where((ph % (2*np.pi)) < 2*np.pi*0.18, 1.0, -0.22)                    # a narrow pulse: buzzy, brassy
    buzz = lp(buzz, 0.35)
    air = lp(rng.standard_normal(n), 0.2) * 0.35
    amp = np.minimum(1, x / 0.015) * (0.75 + 0.25*np.abs(np.sin(2*np.pi*13*x))) * np.where(x > 0.85, (0.95 - x) / 0.1, 1)
    return fin((buzz + air) * np.clip(amp, 0, 1), 0.92)

out = os.path.join(os.path.dirname(__file__), "..", "source", "sfx")
for name, fn in (("toot", toot),):
    b = encode(fn()); p = os.path.join(out, name + ".adp")
    open(p, "wb").write(b); print(p, len(b), "bytes")
