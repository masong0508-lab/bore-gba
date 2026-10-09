#!/usr/bin/env python3
"""EVERYDAY SOUNDS for the life game, synthesized (nothing sampled): the doorbell, the phone's dial beeps, a toilet flush, the shower, eating,
the cash register, a page turning, the coffee maker, the TV coming on, a splash, the VR headset booting.
Written as 4-bit IMA-ADPCM at the effect rate (6554 Hz, mono) in the game's .adp format (tools/encode_sfx.py's encoder): source/sfx/<name>.adp.
usage: python3 tools/make_life_sfx.py [outdir] [--wav]"""
import sys, os
import numpy as np
from scipy.signal import butter, sosfilt
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from encode_sfx import encode, decode, RATE as SR

rng = np.random.default_rng(1009)
def lp(x, f, o=2): return sosfilt(butter(o, min(f, SR*0.45)/(SR/2), 'low', output='sos'), x)
def hp(x, f, o=2): return sosfilt(butter(o, f/(SR/2), 'high', output='sos'), x)
def bp(x, f0, f1, o=2): return sosfilt(butter(o, [f0/(SR/2), min(f1, SR*0.45)/(SR/2)], 'band', output='sos'), x)
def t_(secs): return np.arange(int(secs*SR))/SR
def noise(secs): return rng.standard_normal(int(secs*SR))
def edec(secs, tau): return np.exp(-t_(secs)/tau)
def fade(x, a=0.004, r=0.02):
    x = x.copy(); na = int(a*SR); nr = int(r*SR)
    if na: x[:na] *= np.linspace(0, 1, na)
    if nr: x[-nr:] *= np.linspace(1, 0, nr)
    return x
def at(total, parts):   # mix (time, sound) parts into one clip
    y = np.zeros(int(total*SR))
    for t, x in parts: i = int(t*SR); n = min(len(x), len(y)-i); y[i:i+n] += x[:n]
    return y

def chime(f, secs=1.4):   # a struck tubular chime: inharmonic partials, the high ones die first
    t = t_(secs); y = np.zeros(len(t))
    for k, a, tau in ((1.0, 1.0, 0.75), (2.76, 0.35, 0.28), (5.40, 0.12, 0.10), (2.0, 0.15, 0.4)):
        if f*k < SR*0.45: y += a*np.sin(2*np.pi*f*k*t)*np.exp(-t/tau)
    y[:int(0.004*SR)] *= np.linspace(0, 1, int(0.004*SR))
    return y + 0.15*bp(noise(secs), 1500, 3000)*edec(secs, 0.008)   # (the hammer)
def doorbell(): return at(2.0, [(0.0, chime(659.3)), (0.48, chime(523.3))])   # ding... dong

def dtmf(lo, hi, secs=0.11): t = t_(secs); return fade(np.sin(2*np.pi*lo*t)+np.sin(2*np.pi*hi*t), 0.005, 0.01)
def dial(): return at(0.75, [(0.00, dtmf(770, 1336)), (0.18, dtmf(852, 1209)), (0.36, dtmf(697, 1477)), (0.54, dtmf(941, 1336))])

def flush():
    secs = 2.4; t = t_(secs); n = len(t)
    lever = 0.6*bp(noise(0.05), 800, 2500)*edec(0.05, 0.01)
    cut = np.interp(t, [0, 0.25, 1.0, 2.4], [400, 2600, 1100, 500])   # the rush: bright as it opens, duller as it drains
    x = noise(secs); y = np.zeros(n); seg = SR//25
    for i in range(0, n, seg): y[i:i+seg] = lp(x[max(0, i-200):i+seg], cut[i])[-len(y[i:i+seg]):]
    envl = np.interp(t, [0, 0.12, 0.6, 1.6, 2.4], [0, 1, 0.9, 0.35, 0])
    gurgle = np.sin(2*np.pi*np.cumsum(220+120*np.sin(2*np.pi*9*t)+60*rng.standard_normal(n).cumsum()/np.sqrt(np.arange(1, n+1)))/SR)
    gurgle *= np.interp(t, [0, 0.8, 1.4, 2.0, 2.4], [0, 0, 0.35, 0.15, 0]) * (0.5+0.5*np.sign(np.sin(2*np.pi*7*t)))
    return at(secs, [(0, lever), (0.06, y*envl*1.0 + 0.4*lp(gurgle, 900))])

def shower():
    secs = 2.2; t = t_(secs)
    spray = bp(noise(secs), 900, 3100)*(0.75+0.25*rng.random(len(t)))
    drops = np.zeros(len(t))
    for s in rng.uniform(0, secs, 70): i = int(s*SR); k = min(40, len(t)-i); drops[i:i+k] += np.sin(2*np.pi*rng.uniform(1200, 2800)*np.arange(k)/SR)*np.exp(-np.arange(k)/8)*0.5
    return (spray*0.8 + drops)*np.interp(t, [0, 0.15, 1.6, 2.2], [0, 1, 1, 0])

def bite():
    secs = 0.16; c = bp(noise(secs), 700, 3000)*edec(secs, 0.035)
    for s in rng.uniform(0, 0.08, 6): i = int(s*SR); c[i:i+3] += rng.uniform(-1.5, 1.5, len(c[i:i+3]))   # the crackle
    return c
def munch(): return at(1.0, [(0.0, bite()), (0.27, bite()*0.85), (0.52, bite()*0.7), (0.7, 0.25*lp(noise(0.25), 500)*edec(0.25, 0.08))])

def ching(f, secs=0.7):
    t = t_(secs); return sum(a*np.sin(2*np.pi*f*k*t)*np.exp(-t/tau) for k, a, tau in ((1, 1.0, 0.35), (1.5, 0.5, 0.2), (2.2, 0.25, 0.1)) if f*k < SR*0.45)
def cash():   # cha-ching: the drawer clunks out, the bell rings, coins rattle
    clunk = (np.sin(2*np.pi*110*t_(0.12))*edec(0.12, 0.03) + 0.6*lp(noise(0.12), 900)*edec(0.12, 0.02))
    coins = np.zeros(int(0.5*SR))
    for s in rng.uniform(0, 0.35, 9): i = int(s*SR); k = min(300, len(coins)-i); coins[i:i+k] += np.sin(2*np.pi*rng.uniform(1800, 3000)*np.arange(k)/SR)*np.exp(-np.arange(k)/60)*0.35
    return at(1.1, [(0.0, clunk), (0.12, ching(1047)), (0.16, ching(1568)*0.6), (0.2, coins)])

def page():
    secs = 0.35; t = t_(secs); w = bp(noise(secs), 1200, 3100)*np.interp(t, [0, 0.05, 0.15, 0.35], [0, 1, 0.5, 0])
    crk = np.zeros(len(t))
    for s in rng.uniform(0.02, 0.2, 8): i = int(s*SR); crk[i:i+2] += rng.uniform(-1, 1, len(crk[i:i+2]))
    return w + 0.5*crk

def brew():   # the coffee maker: water bubbling up through the filter, a hiss of steam
    secs = 1.6; y = np.zeros(int(secs*SR))
    for s in np.sort(rng.uniform(0.05, 1.45, 26)):
        d = rng.uniform(0.03, 0.07); f0 = rng.uniform(250, 420); n = int(d*SR); tt = np.arange(n)/SR
        b = np.sin(2*np.pi*np.cumsum(np.linspace(f0, f0*2.2, n))/SR)*np.sin(np.pi*tt/d)
        i = int(s*SR); k = min(n, len(y)-i); y[i:i+k] += b[:k]*0.6
    steam = hp(noise(secs), 1800)*np.interp(t_(secs), [0, 0.6, 1.6], [0, 0.25, 0.1])
    return y + steam

def tv():   # a set coming on: the click, the static, then it settles
    click = 0.8*hp(noise(0.02), 500)*edec(0.02, 0.004)
    stat = bp(noise(0.55), 300, 3200)*np.interp(t_(0.55), [0, 0.05, 0.35, 0.55], [0, 0.8, 0.5, 0])
    hum = 0.25*np.sin(2*np.pi*120*t_(0.6))*np.interp(t_(0.6), [0, 0.1, 0.6], [0, 1, 0])
    return at(0.7, [(0, click), (0.03, stat), (0.05, hum)])

def splash():
    plop = np.sin(2*np.pi*np.cumsum(np.linspace(180, 700, int(0.08*SR)))/SR)*np.sin(np.pi*np.linspace(0, 1, int(0.08*SR)))
    spray = bp(noise(0.45), 600, 3000)*np.interp(t_(0.45), [0, 0.02, 0.45], [0, 1, 0])**1.5
    return at(0.55, [(0, plop*0.9), (0.03, spray*0.6)])

def boot():   # the DeadSet 3Thousand VYBE switching on: a rising sweep and a bright two-note chime
    secs = 0.7; t = t_(secs); f = np.interp(t, [0, 0.7], [140, 1500])**1.0
    saw = 2*((np.cumsum(f)/SR) % 1.0) - 1
    sweep = lp(saw, 1800)*np.interp(t, [0, 0.05, 0.6, 0.7], [0, 0.7, 0.5, 0])
    return at(1.3, [(0, sweep), (0.62, ching(1319)*0.6), (0.78, ching(1760)*0.6)])

SOUNDS = (('bell', doorbell), ('dial', dial), ('flush', flush), ('shower', shower), ('munch', munch), ('cash', cash),
          ('page', page), ('brew', brew), ('tv', tv), ('splash', splash), ('boot', boot))

if __name__ == '__main__':
    outdir = next((a for a in sys.argv[1:] if not a.startswith('--')), 'source/sfx'); wav = '--wav' in sys.argv
    os.makedirs(outdir, exist_ok=True); total = 0
    for name, fn in SOUNDS:
        x = fn(); x = fade(x, 0.002, 0.03); x = x/np.max(np.abs(x))*0.89   # (the game sets the level: SFX VOLUME)
        s16 = np.round(x*32767).astype(np.int32); b = encode(s16)
        open(os.path.join(outdir, name+'.adp'), 'wb').write(b); total += len(b)
        y = decode(b); snr = 10*np.log10(np.sum(s16.astype(float)**2)/max(1, np.sum((s16-y).astype(float)**2)))
        print('%-7s %.2f s  %5d bytes  SNR %.1f dB' % (name, len(x)/SR, len(b), snr))
        if wav:
            import wave
            with wave.open(os.path.join(outdir, name+'.wav'), 'wb') as w: w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes(np.clip(y, -32768, 32767).astype(np.int16).tobytes())
    print('total', total, 'bytes')
