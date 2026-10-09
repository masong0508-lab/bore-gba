#!/usr/bin/env python3
"""Synthesise the skate feel sounds (no ffmpeg needed) -> source/sfx/{pop,land,stick,grind}.adp
pop   = ollie: tail snap on the ground
land  = clean landing: wheels thud + a short roll
stick = a trick landed (perfect / spin / flip): the thud plus a bright two note 'ding'
grind = grabbing a rail: a short metal scrape
usage: python3 tools/make_skate_sfx.py"""
import os, sys
import numpy as np
sys.path.insert(0, os.path.dirname(__file__))
from encode_sfx import encode, RATE

rng = np.random.default_rng(7)
def t(n): return np.arange(int(RATE*n)) / RATE
def env(x, a=0.002, d=0.05):
    e = np.minimum(1, x / a) * np.exp(-x / d); return e
def lp(x, k):   # one pole low pass
    y = np.zeros_like(x); s = 0.0
    for i, v in enumerate(x): s += (v - s) * k; y[i] = s
    return y
def fin(x, peak=0.85):
    x = x / (np.max(np.abs(x)) + 1e-9) * peak
    f = min(len(x), int(RATE*0.02)); x[-f:] *= np.linspace(1, 0, f)
    return (x * 32767).astype(np.int32)

def pop():
    x = t(0.13); n = rng.standard_normal(len(x))
    snap = lp(n, 0.55) * env(x, 0.001, 0.012)                  # the tail slap
    knock = np.sin(2*np.pi*(190*np.exp(-x*18))*x) * env(x, 0.001, 0.03)
    return fin(snap*0.8 + knock*0.9, 0.8)
def land():
    x = t(0.20); n = rng.standard_normal(len(x))
    thud = np.sin(2*np.pi*(95*np.exp(-x*9)+55)*x) * env(x, 0.001, 0.06)
    roll = lp(n, 0.25) * env(x, 0.004, 0.05) * 0.5
    clack = lp(n, 0.6) * env(x, 0.0005, 0.008) * 0.7
    return fin(thud + roll + clack, 0.85)
def stick():
    x = t(0.34); n = rng.standard_normal(len(x))
    thud = np.sin(2*np.pi*(100*np.exp(-x*9)+55)*x) * env(x, 0.001, 0.06)
    clack = lp(n, 0.6) * env(x, 0.0005, 0.008) * 0.6
    ding = np.zeros(len(x))
    for f, st in ((1318.5, 0.05), (1760.0, 0.11)):           # E6 then A6
        m = x >= st; xx = x[m] - st
        ding[m] += np.sin(2*np.pi*f*xx) * np.exp(-xx/0.07) * 0.55
    return fin(thud*0.9 + clack + ding, 0.85)
def grind():
    x = t(0.24); n = rng.standard_normal(len(x))
    rasp = lp(n, 0.7) * (0.6 + 0.4*np.sin(2*np.pi*70*x)) * env(x, 0.003, 0.11)
    ring = np.sin(2*np.pi*1500*x) * np.exp(-x/0.05) * 0.25
    return fin(rasp + ring, 0.75)

out = os.path.join(os.path.dirname(__file__), "..", "source", "sfx")
for name, fn in (("pop", pop), ("land", land), ("stick", stick), ("grind", grind)):
    b = encode(fn()); p = os.path.join(out, name + ".adp")
    open(p, "wb").write(b); print(p, len(b), "bytes")
