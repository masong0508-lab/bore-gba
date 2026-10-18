#!/usr/bin/env python3
"""Builds tools/ohio_hoe.xm : "OHIO HOE", a VaninBlack hyperpop drum & bass song built from three sketches: tools/ohio_hoe.mid (the chord hook:
an E6/9 stack with a C# - D walk under it, 4 bars), tools/hsu1.mid (a 12-bar bass line, Eb minor, played here a semitone up in E) and
tools/woover.mid (a stuttering three-note chord figure and a few melody fragments: the builds).   usage: python3 tools/make_ohio_hoe.py

HYPERPOP D&B at 172 BPM: two-step kick / snare with 16th hats and ghost notes, a reese and a sub on the hsu1 line, the hook on seven detuned
supersaws, a pitched-up "vocal" (a formant synth two octaves above where a voice would sit) singing the hook's top notes, a "HOE" that is a vowel
sliding down an octave at the end of every phrase, bitcrushed 808s and glitch bursts in the breaks, the woover stutters as supersaw stabs in the
builds, FM plucks arpeggiating the chords, risers and snare rolls into every drop.
64th-note rows (speed 1, XM BPM 115 = 16 rows a beat = 172.5 BPM), 64 rows a bar, 128-row patterns = 2 bars.
Channels: 0 kick | 1 snare / clap | 2 ghost / perc | 3 hat | 4 open hat / glitch | 5 sub / 808 | 6 reese | 7 8 9 10 supersaw chord
          11 vox (the hook) | 12 vox echo / HOE | 13 pluck arp | 14 bell / chip | 15 crash / impact / riser.
"""
import os, sys, struct
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import midi_read
import make_flexicode_rework as F
from make_flexicode_rework import tt, noise, lp, hp, bp, saw, square, finish, mid2f, SR, SR2

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "ohio_hoe.xm")
SPEED, BPM, ROWS, NCH, BR, Q = 1, 115, 128, 16, 64, 4       # 16 rows a beat at 172.5 BPM; BR rows a bar; Q rows a 16th
rng = np.random.default_rng(440)
I = {}
def add(k, name, x, gen=None, rel=12): I[k] = dict(name=name, x=x, gen=gen, rel=rel)

# ---------------------------------------------------------------- the sketches
def bars_of(path, track, tr=0):
    div, TR = midi_read.parse(path); TB = div * 4; T16 = div / 4
    out = {}
    for t0, ln, m, v, c in midi_read.notes(TR[track]): out.setdefault(int(t0 // TB), []).append(((t0 % TB) / T16, max(1, ln / T16), m + tr, v))
    return out
HOOK = bars_of(os.path.join(HERE, "ohio_hoe.mid"), 1)          # 4 bars of chords (E F# G# B C#; C# D walk)
BASS = bars_of(os.path.join(HERE, "hsu1.mid"), 1, +1)          # 12 bars, up a semitone: E
WOO = bars_of(os.path.join(HERE, "woover.mid"), 5)             # bars 4..7: the stutter (E F G#)
WMEL = bars_of(os.path.join(HERE, "woover.mid"), 4)            # bars 0..4: melody fragments
SCALE = {4, 6, 8, 9, 11, 1, 2, 3}                              # E mixolydian with the D# too (the bass walks through both)
def fit(m):                                                    # the woover F into F#, anything else outside the scale up to the next scale note
    while m % 12 not in SCALE: m += 1
    return m
def hook_top(bar):   return sorted(set(round(s) for s, l, m, v in HOOK.get(bar % 4, [])))
def chord_at(bar, s16):
    """The hook's chord sounding at 16th s16 of bar (the 4-bar loop): the notes that started at or before it and still hold."""
    ns = [(s, l, m) for s, l, m, v in HOOK.get(bar % 4, []) if s <= s16 < s + l]
    return sorted(set(m for s, l, m in ns))

# ---------------------------------------------------------------- sounds
n = int(.26 * SR); t = tt(n)                                                               # dnb kick: a click and a short tight thump
add('kick', 'dnb kick', finish(np.tanh(1.9 * np.sin(2 * np.pi * np.cumsum(52 + 190 * np.exp(-t / .012)) / SR)) * np.exp(-t / .09)
                           + hp(noise(n), 4000) * np.exp(-t / .002) * .7, .97))
n = int(.22 * SR); t = tt(n)                                                               # snare: bright, cracking, a little ring
x = bp(noise(n), 1600, 7800) * np.exp(-t / .07) + (np.sin(2 * np.pi * 225 * t) + .5 * np.sin(2 * np.pi * 410 * t)) * np.exp(-t / .03)
add('snare', 'crack snare', finish(np.tanh(2.4 * x), .94))
n = int(.18 * SR); t = tt(n)                                                               # clap: three bursts
cl = sum(bp(noise(n), 900, 5000) * np.exp(-np.maximum(t - d, 0) / .03) * (t >= d) for d in (0, .011, .022))
add('clap', 'clap', finish(cl, .8))
n = int(.06 * SR); t = tt(n)
add('ghost', 'ghost', finish(bp(noise(n), 2000, 7000) * np.exp(-t / .014) + np.sin(2 * np.pi * 230 * t) * np.exp(-t / .01) * .3, .45))
n = int(.04 * SR); add('hat', 'hat', finish(hp(noise(n) * np.exp(-tt(n) / .008), 7600, 4), .36))
n = int(.26 * SR); add('ohat', 'open hat', finish(hp(noise(n) * np.exp(-tt(n) / .08), 7500, 4), .36))
n = int(.9 * SR); add('crash', 'crash', finish(hp(noise(n), 4500, 3) * np.exp(-tt(n) / .35), .5))
n = int(.9 * SR2); t = tt(n, SR2)
add('impact', 'impact', finish(np.tanh(2.2 * np.sin(2 * np.pi * np.cumsum(36 + 70 * np.exp(-t / .07)) / SR2)) * np.exp(-t / .32)
                               + lp(noise(n), 1000, 2, SR2) * np.exp(-t / .22) * .5, .9), None, 5)
n = int(.6 * SR2); t = tt(n, SR2); f = mid2f(28)                                           # sub (E1)
add('sub', 'sub', finish(np.sin(2 * np.pi * f * t) * np.minimum(1, t / .003) * np.minimum(1, (t[-1] - t) / .05), .95), 28, 5)
n = int(1.6 * SR2); t = tt(n, SR2); f = mid2f(28)                                          # 808 (E1): long, driven, bitcrushed a touch
ph = 2 * np.pi * np.cumsum(f * (1 + .4 * np.exp(-t / .025))) / SR2
add('808', 'crushed 808', finish(np.round(np.tanh(1.9 * np.sin(ph)) * 14) / 14 * np.minimum(1, t / .003) * np.exp(-t / 1.1) * np.minimum(1, (t[-1] - t) / .1), .95, .05, SR2), 28, 5)
n = int(.6 * SR2); t = tt(n, SR2); f = mid2f(40)                                           # reese (E2): three saws beating, a slow filter
x = sum(saw(f * 2 ** (c / 1200), n, SR2, ph=rng.uniform(0, 6.28)) for c in (-18, 0, 17)) / 3
seg = [lp(x[i:i + 128], 380 + 900 * (.5 + .5 * np.sin(2 * np.pi * 2.9 * i / SR2)), 2, SR2) for i in range(0, n, 128)]
add('reese', 'reese', finish(np.tanh(2.1 * np.concatenate(seg)[:n]) * np.minimum(1, t / .004) * np.minimum(1, (t[-1] - t) / .05), .82), 40, 5)
def supersaw(dur, cents, sr, hpf=150, lpf=5000, atk=.004, dec=None):
    n = int(dur * sr); t = tt(n, sr); f = mid2f(60)
    x = sum(saw(f * 2 ** (c / 1200), n, sr, ph=rng.uniform(0, 6.28)) for c in cents) / len(cents)
    env = np.minimum(1, t / atk) * (np.exp(-t / dec) if dec else 1) * np.minimum(1, (t[-1] - t) / .08)
    return hp(lp(x, lpf, 2, sr), hpf, 2, sr) * env
add('ssaw', 'supersaw', finish(supersaw(1.2, (-28, -17, -6, 0, 7, 16, 27), SR2, 170, 4200), .6, .05, SR2), 60, 5)        # the hook (C4)
add('stab', 'supersaw stab', finish(supersaw(.22, (-24, -9, 0, 10, 23), SR, 220, 7000, .002, .07), .7), 60)                # the stutters
def vox(dur, f0, vib_hz, sr, formants, glide=0):
    """A vowel: a saw into three band passes, with vibrato; glide = octaves the pitch falls over the note (the HOE)."""
    n = int(dur * sr); t = tt(n, sr)
    fcur = f0 * 2 ** (-glide * np.clip((t - .08) / max(dur - .1, .01), 0, 1)) * (1 + .008 * np.sin(2 * np.pi * vib_hz * t) * np.clip((t - .1) / .2, 0, 1))
    src = saw(f0, n, sr, vib=fcur) * .8 + square(f0, n, sr, vib=fcur) * .25
    out = sum(bp(src, a, b, 2, sr) * g for a, b, g in formants) + lp(src, 2200, 2, sr) * .3
    return out * np.minimum(1, t / .012) * np.minimum(1, (t[-1] - t) / .15)
add('vox', 'hyper vox', finish(vox(1.1, mid2f(84), 6.2, SR, ((700, 1000, .8), (1500, 1900, .8), (2800, 3300, .5))) * np.exp(-tt(int(1.1 * SR)) / 1.5), .72, .05), 84)
add('hoe', 'hoe', finish(vox(.9, mid2f(79), 5.0, SR, ((450, 700, .9), (900, 1250, .7), (2400, 2900, .35)), glide=1) * np.exp(-tt(int(.9 * SR)) / 1.0), .74, .05), 79)
n = int(.16 * SR); t = tt(n); f = mid2f(72)                                                # FM pluck (C5)
add('pluck', 'fm pluck', finish(np.sin(2 * np.pi * f * t + 2.6 * np.exp(-t / .025) * np.sin(2 * np.pi * 2 * f * t)) * np.exp(-t / .055), .62), 72)
n = int(.9 * SR); t = tt(n); f = mid2f(84)                                                 # glass bell (C6)
add('bell', 'bell', finish(np.sin(2 * np.pi * f * t + 1.2 * np.exp(-t / .3) * np.sin(2 * np.pi * 3.01 * f * t)) * np.exp(-t / .4), .55), 84)
n = int(.45 * SR); t = tt(n); f = mid2f(72)                                                # chip lead (C5): 12.5% pulse, bright
x = saw(f, n, SR) - saw(f, n, SR, ph=np.pi / 4)
add('chip', 'chip lead', finish(lp(x, 7000) * np.minimum(1, t / .002) * np.exp(-t / .4), .55), 72)
n = int(1.5 * SR2); t = tt(n, SR2)                                                         # riser
seg = [bp(noise(128), 250 + 3600 * (i / n) ** 2, 600 + 4300 * (i / n) ** 2, 2, SR2) for i in range(0, n, 128)]
add('riser', 'riser', finish(np.concatenate(seg)[:n] * (t / t[-1]) ** 1.4, .5), None, 5)
n = int(.09 * SR); t = tt(n)                                                               # glitch: a bitcrushed noise burst with a tone in it
g = np.round((bp(noise(n), 1200, 6000) + .6 * square(mid2f(76), n)) * 6) / 6
add('glitch', 'glitch', finish(g * np.minimum(1, (t[-1] - t) / .01), .55))
KEYS = ['kick', 'snare', 'clap', 'ghost', 'hat', 'ohat', 'crash', 'impact', 'sub', '808', 'reese', 'ssaw', 'stab', 'vox', 'hoe', 'pluck', 'bell', 'chip', 'riser', 'glitch']
INST = {k: i + 1 for i, k in enumerate(KEYS)}
def xmn(m, key): g = I[key]['gen']; return 49 + (m - g) if g else 49

# ---------------------------------------------------------------- one bar (64 rows)
def build_bar(bi, p, P, base, hb, bb):
    """bar bi of the song into P at row base; hb: which bar of the 4-bar hook, bb: which bar of the 12-bar bass line."""
    def put(r, ch, key, midi=None, vol=64):
        r = int(round(r)) + base
        if base <= r < base + BR and vol > 0: P[r][ch] = (xmn(midi, key) if midi is not None else 49, INST[key], 0x10 + max(1, min(64, int(vol))))
    d = p.get('drums', 0)
    # ---- drums: dnb two-step (kick 1 and the "a" of 2, snare on 2 and 4), 16th hats with accents, ghosts; 'half' = halftime trap
    if d == 1:                                                                                   # four-on-the-floor (the builds)
        for s in range(0, 16, 4): put(s * Q, 0, 'kick', None, 54)
        for s in range(2, 16, 4): put(s * Q, 3, 'hat', None, 22)
    if d == 2 or d == 3:
        put(0, 0, 'kick', None, 64); put(10 * Q, 0, 'kick', None, 54); put(4 * Q, 1, 'snare', None, 64); put(12 * Q, 1, 'snare', None, 64)
        if bi % 2: put(14 * Q + 2, 0, 'kick', None, 44)
        for s in range(16): put(s * Q, 3, 'hat', None, 36 if s % 4 == 2 else 20 if s % 2 else 28)
        put(7 * Q, 2, 'ghost', None, 18); put(15 * Q, 2, 'ghost', None, 16); put(11 * Q + 2, 2, 'ghost', None, 12)
        if d == 3:
            put(6 * Q, 4, 'ohat', None, 20); put(13 * Q, 1, 'clap', None, 30)
            if bi % 4 == 3:
                for k, r in enumerate(range(48, 64, 2)): put(r, 3, 'hat', None, 12 + k * 2)      # a hat roll into the next bar
            if bi % 8 == 7:
                for k, r in enumerate(range(56, 64, 1)): put(r, 1, 'snare', None, 28 + k * 4)    # a 64th snare fill
    if d == 4:                                                                                   # halftime: trap, the snare on 3
        put(0, 0, 'kick', None, 62); put(22, 0, 'kick', None, 48); put(32, 1, 'snare', None, 62); put(32, 1, 'clap', None, 40)
        for s in range(0, 16, 2): put(s * Q, 3, 'hat', None, 28 if s % 4 == 0 else 20)
        for k, r in enumerate(range(48, 64, 2)): put(r, 3, 'hat', None, 10 + k * 2)
    if d == 9:                                                                                   # the break: kick on 1 only
        put(0, 0, 'kick', None, 48)
    if p.get('roll'):                                                                            # a snare roll speeding up to 64ths
        for r in range(BR):
            step = 8 if r < 16 else 4 if r < 32 else 2 if r < 48 else 1
            if r % step == 0: put(r, 1, 'snare', None, p['roll'] * (.3 + .7 * r / BR))
    if p.get('riser'): put(0, 15, 'riser', None, p['riser'])
    for (r, key, vol) in p.get('fx', ()): put(r, 15, key, None, vol)
    if p.get('glitch'):                                                                          # glitch bursts on random 16ths of the second half
        for s in sorted(rng.choice(range(8, 16), 3, replace=False)): put(s * Q, 4, 'glitch', None, p['glitch'])
    # ---- bass (the hsu1 line): reese on every note, the sub on the notes that start a beat; the 808 holds roots in the breaks
    bn = BASS.get(bb, [])
    if p.get('reese'):
        prev = None
        for s, l, m, v in bn:
            if m < 36: continue
            if m != prev or s % 4 == 0: put(s * Q, 6, 'reese', m, p['reese'] - (0 if s % 4 == 0 else 7)); prev = m
    if p.get('sub'):
        prev = None
        for s, l, m, v in bn:
            if m < 36: continue
            if s % 4 == 0 or m != prev: put(s * Q, 5, 'sub', m - 12, p['sub']); prev = m
    if p.get('808'):
        roots = [m for s, l, m, v in bn if s < 1 and m >= 36] or [40]
        put(0, 5, '808', roots[0] - 12, p['808']); put(32, 5, '808', roots[0] - 12, p['808'] - 8)
    # ---- the hook: supersaw chords (four voices), the vox on the top notes, HOE at the phrase's end
    hn = HOOK.get(hb, [])
    if p.get('chords'):
        starts = sorted(set(round(s) for s, l, m, v in hn))
        for s in starts:
            ch = chord_at(hb, s)[-4:]
            for j, m in enumerate(ch): put(s * Q, 7 + j, 'ssaw', m - (12 if p.get('low') else 0), p['chords'] - j * 2)
    if p.get('stabs'):                                                                           # the chords as stabs on the two-step (the "a" of 1, the "and" of 3)
        for s in (0, 3, 8, 11, 14):
            ch = chord_at(hb, s)[-4:]
            for j, m in enumerate(ch): put(s * Q, 7 + j, 'stab', m, p['stabs'] - j * 2 - (6 if s % 4 else 0))
    if p.get('vox'):
        tops = {}
        for s, l, m, v in hn: tops[round(s)] = max(m, tops.get(round(s), 0))
        for k, (s, m) in enumerate(sorted(tops.items())):
            put(s * Q, 11, 'vox', m + 12 + p.get('oct', 0), p['vox'])
            if p.get('echo'): put(s * Q + 12, 12, 'vox', m + 12 + p.get('oct', 0), p['vox'] * .4)
            if p.get('chip'): put(s * Q, 14, 'chip', m + p.get('oct', 0), p['chip'])
        if hb % 2 == 1 and p.get('hoe'): put(12 * Q, 12, 'hoe', 79 + p.get('oct', 0), p['hoe'])   # "HOE": the vowel slides down an octave at the end of the phrase
    if p.get('arp'):                                                                             # FM plucks running up the chord in 16ths
        for s in range(16):
            ch = chord_at(hb, s)
            if ch: put(s * Q, 13, 'pluck', ch[s % len(ch)] + 12, p['arp'] - (0 if s % 4 == 0 else 8))
    if p.get('bell'):
        tops = {}
        for s, l, m, v in hn: tops[round(s)] = max(m, tops.get(round(s), 0))
        for s, m in sorted(tops.items()): put(s * Q, 14, 'bell', m + 12, p['bell'])
    # ---- the woover stutter: supersaw stabs on E F# G# as the sketch plays them (bars 4..7), its fragments on the chip
    if p.get('woo'):
        for s, l, m, v in WOO.get(4 + bi % 4, []): put(s * Q, 7 + (fit(m) % 3), 'stab', fit(m) + 12, p['woo'])
    if p.get('wmel'):
        for s, l, m, v in WMEL.get(bi % 5, []): put(s * Q, 14, 'chip', fit(m) + 12, p['wmel'])

# ---------------------------------------------------------------- arrangement
S = []
def sec(n, **kw):
    for i in range(n): S.append(dict(kw))
sec(8, vox=56, echo=1, bell=40, sub=0, hoe=50)                                             # intro: the hook sung, bells
S[0]['fx'] = ((0, 'crash', 36),); S[4]['drums'] = 1; S[5]['drums'] = 1; S[6]['drums'] = 1; S[7]['drums'] = 1
sec(8, drums=1, woo=52, arp=42, sub=24, chords=46, low=1)                                  # build: the woover stutters, the arp, the chords low
S[-2]['riser'] = 44; S[-1]['roll'] = 40; S[-1]['glitch'] = 30
sec(24, drums=3, reese=54, sub=28, chords=58, vox=62, echo=1, hoe=56, arp=40)               # DROP 1
S[-24]['fx'] = ((0, 'impact', 60), (2, 'crash', 50)); S[-16]['fx'] = ((0, 'crash', 44),); S[-8]['fx'] = ((0, 'crash', 44),); S[-1]['roll'] = 36
sec(8, drums=9, chords=56, vox=60, oct=12, echo=1, hoe=52, glitch=44, **{'808': 36})        # break: 808s, the hook an octave up, glitches
S[-2]['riser'] = 48; S[-1]['roll'] = 50
sec(8, drums=1, woo=54, stabs=46, sub=26, wmel=48)                                         # build 2: stutters, stabs, the woover fragments
S[-2]['riser'] = 50; S[-1]['roll'] = 52; S[-1]['glitch'] = 34
sec(24, drums=3, reese=56, sub=30, chords=60, vox=64, chip=44, hoe=58, arp=42)               # DROP 2: the chip doubles the vox
for k in range(-16, -12): S[k]['drums'] = 4                                                # (four bars of halftime trap in the middle)
S[-24]['fx'] = ((0, 'impact', 62), (2, 'crash', 52)); S[-12]['fx'] = ((0, 'impact', 56),); S[-1]['roll'] = 40
sec(8, drums=4, chords=56, vox=60, echo=1, hoe=54, glitch=40, **{'808': 36})                # bridge: halftime, the hook over 808s
S[-2]['riser'] = 52; S[-1]['roll'] = 54
sec(16, drums=3, reese=58, sub=30, chords=62, stabs=44, vox=64, chip=46, echo=1, hoe=60, arp=44)   # LAST DROP: everything
S[-16]['fx'] = ((0, 'impact', 64), (2, 'crash', 56)); S[-8]['fx'] = ((0, 'crash', 48),)
sec(8, vox=56, echo=1, bell=40, sub=22, hoe=48, chords=46, low=1)                          # outro
S[-8]['fx'] = ((0, 'crash', 40),)

def main():
    F.I.clear(); F.I.update(I)
    pats, order, seen = [], [], {}
    for k in range(0, len(S), 2):
        P = [[(0, 0, 0)] * NCH for _ in range(ROWS)]
        for j in range(2):
            if k + j < len(S): build_bar(k + j, S[k + j], P, j * BR, (k + j) % 4, (k + j) % 12)
        key = F.pat_bytes(P)
        if key not in seen: seen[key] = len(pats); pats.append(P)
        order.append(seen[key])
    end = [[(0, 0, 0)] * NCH for _ in range(ROWS)]                                         # the last hit: an impact, the chord ringing, one HOE
    end[0][15] = (49, INST['impact'], 0x10 + 60); end[0][5] = (xmn(28, '808'), INST['808'], 0x10 + 56)
    for j, m in enumerate((64, 66, 68, 71)): end[0][7 + j] = (xmn(m, 'ssaw'), INST['ssaw'], 0x10 + 40 - j * 2)
    end[8][12] = (xmn(79, 'hoe'), INST['hoe'], 0x10 + 46)
    pats.append(end); order.append(len(pats) - 1)
    hdr = b'Extended Module: ' + b'OHIO HOE'.ljust(20) + b'\x1a' + b'make_ohio_hoe'.ljust(20, b'\0')[:20] + struct.pack('<H', 0x0104)
    hdr += struct.pack('<I', 276) + struct.pack('<8H', len(order), 0, NCH, len(pats), len(KEYS), 1, SPEED, BPM) + bytes(order).ljust(256, b'\0')
    body = b''.join(F.pat_bytes(P) for P in pats) + b''.join(F.inst_bytes(k) for k in KEYS)
    open(OUT, 'wb').write(hdr + body)
    rows = sum(len(pats[o]) for o in order); s = rows * SPEED * 2.5 / BPM
    print("wrote %s: %d bars, %d orders, %d patterns, %.1f s (%d:%02d), %d KB" % (OUT, len(S), len(order), len(pats), s, s // 60, s % 60, os.path.getsize(OUT) // 1024))
if __name__ == '__main__': main()
