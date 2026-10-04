#!/usr/bin/env python3
"""Builds the LOADING tick-tock: source/sfx/tick.adp (4-bit IMA-ADPCM, 6554 Hz mono, same format as the other effects).
usage: python3 tools/make_tick.py [preview.wav]

It is exactly ONE SECOND long and made to be looped (the mixer wraps it, see sfxLoop in main.c): a higher TICK at 0.0 s
and a lower, softer TOCK at 0.5 s. Each one is a woodblock-like click (two damped partials plus a very short noise tick),
and the rest of the second is silence, so the wrap point is silent and the loop never clicks. Nothing here is random:
the same file comes out every time."""
import sys, struct, wave
import numpy as np
sys.path.insert(0, __import__("os").path.dirname(__file__))
from encode_sfx import encode, decode, RATE

N = RATE                      # one second
t = np.arange(N) / RATE

def click(freq, decay_ms, peak, noise=0.10):
    """one clock click: fast attack, two inharmonic partials, a hint of low-passed noise on the attack"""
    env = np.exp(-t / (decay_ms / 1000.0))
    atk = np.minimum(1.0, t / 0.001)                        # 1 ms attack
    body = np.sin(2*np.pi*freq*t) + 0.25*np.sin(2*np.pi*freq*2.41*t)
    rng = np.random.RandomState(7)                          # fixed seed: reproducible
    n = rng.uniform(-1, 1, N)
    n = np.convolve(n, np.ones(5)/5, mode="same")           # a little low-pass (the game's DAC rate is low)
    nenv = np.exp(-t / 0.0012)
    x = (body * env + noise * n * nenv) * atk
    return x / np.max(np.abs(x)) * peak

PRIME = 12   # samples (1.8 ms) of a quick rising burst before each click. IMA-ADPCM starts every click from a tiny step size and
             # cannot follow a loud onset (the error is ~45 %); the burst opens the step size first (error ~15 %) and is part of the "tick".
def prime(amp=0.28):
    k = np.arange(PRIME)
    return amp * ((-1.0) ** k) * (k + 1) / PRIME

x = np.zeros(N)
tick = click(1900.0, 11.0, 1.00)
tock = click(1300.0, 15.0, 0.80)
x[PRIME:] += tick[:N - PRIME]; x[:PRIME] += prime(0.28)
sh = N // 2                                                 # the tock starts at 0.5 s
x[sh:] += tock[:N - sh]; x[sh - PRIME:sh] += prime(0.22)
x[-(RATE // 20):] *= np.linspace(1, 0, RATE // 20)          # belt and braces: the last 50 ms fade to zero
pcm = np.round(x * 32767 * 0.62).astype(np.int32)           # peak 62 %: the mixer scales it by SFX VOLUME anyway

blob = encode(pcm)
open("source/sfx/tick.adp", "wb").write(blob)

back = decode(blob)
err = np.sqrt(np.mean((back.astype(float) - pcm) ** 2)) / max(1.0, np.sqrt(np.mean(pcm.astype(float) ** 2)))
print("source/sfx/tick.adp: %d bytes, %d samples, peak %d, adpcm error %.1f %%, wrap step %d" %
      (len(blob), len(back), int(np.max(np.abs(back))), err * 100, int(abs(back[-1]))))

if len(sys.argv) > 1:         # a listening copy: three turns of the loop
    w = wave.open(sys.argv[1], "wb"); w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE)
    w.writeframes(np.tile(back, 3).astype("<i2").tobytes()); w.close()
    print("preview written:", sys.argv[1])
