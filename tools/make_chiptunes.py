#!/usr/bin/env python3
"""Mini chiptune loops of jukebox songs (for the creator menu).  Needs numpy + scipy (+ ffmpeg for the mp3 previews).

usage:  python3 tools/make_chiptunes.py [out_dir] [song_id ...]       (run from the project root; default out_dir = chiptune_out)

Each song is read from its .xm exactly as the game plays it (the same re-arrangements xm2gba.py applies), then rebuilt for four classic chip voices:
    PULSE 1  the lead           PULSE 2  the harmony, arpeggiated (or an echo of the lead)
    TRIANGLE the bass           NOISE    kick / snare / hats / crash
How a loop is made:
  1. every instrument is classified from its sample (kick, snare, hat, crash, bass, lead, pad ...) and its real sounding pitch is measured
     (the generators write notes relative to each sample's own pitch, so the note numbers alone are not the pitch);
  2. the loop is a run of patterns that comes round again in the song (a section that repeats), 8 to 20 seconds long, with the most going on;
  3. the notes of that run are voiced for the four chip channels (monophonic bass and lead, drums by priority, chords arpeggiated);
  4. everything is rendered NES style (4-bit levels, 60 Hz envelopes, 32-step triangle, LFSR noise, the APU's non-linear mixer) three times in a
     row and the middle copy is kept, so the loop point is seamless.
Output per song:  NN_id.wav (one loop, 48 kHz mono),  NN_id.mp3 (the loop twice, to hear the seam),  NN_id.png (piano roll),  and chiptunes.txt (what was picked).
"""
import sys, os, io, contextlib, wave, subprocess
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import xm2gba as X
from xm import parse

FS = 48000                      # output rate
OS = 4                          # oversampling while the voices are generated (pulse and noise edges are band-limited by the decimation)
CR = 120                        # control rate: voices change 120 times a second; envelopes step at 60 Hz like a real chip driver
SPC = FS * OS // CR             # oversampled samples per control step (1600)
MIXR = X.MIXR

# the ten songs: id, xm file, title.  Options: win = (min, max) loop seconds, duty = (lead, harmony), lead = instrument number to force as the lead
SONGS = [      # the visible set (picked by ear; 9 of 10 so far)
    ("meltdown_in_mars_house", "tools/meltdown_in_mars_house.xm", "MELTDOWN IN MARS",           {}),
    ("sunman_sunrise",         "tools/sunman_sunrise.xm",         "SUNMAN SUNRISE",             {}),
    ("whistler_shuffle",       "tools/whistler_shuffle.xm",       "WHISTLER MAN",               {}),
    ("cynicaller_dnb",         "tools/cynicaller_dnb.xm",         "THE CYNICAL SYNDICATION",    {}),
    ("mi_cora_zone",           "tools/mi_cora_zone.xm",           "MI CORA ZONE",               {}),
    ("excuses_house",          "tools/excuses_house.xm",          "EXCUSES",                    {}),
    ("aim_and_shoot",          "tools/aim_and_shoot.xm",          "AIM AND SHOOT",              {}),
    ("gottcho_barracho_ii",    "tools/gottcho_barracho_ii.xm",    "GOTTCHO BARRACHO",           {'v2': True}),
    ("spanish_flexicode",      "tools/spanish_flexicode.xm",      "AN ODE TO THE SPANISH FLEXICODE", {'v2': True}),
    ("magic_act",              "tools/magic_act.xm",              "THE MAGIC ACT",              {'v2': True}),
]
SECRET = [     # the hidden set: for the title-screen code, like the (ORIGINAL) songs
    ("amiga_music",            "tools/amiga_music.xm",            "AMIGA MUSIC",                {}),
    ("emergency_hitech",       "tools/emergency_hitech.xm",       "EMERGENCY ON THE DANCE FLOOR", {}),
    ("hotdamn_rave",           "tools/hotdamn_rave.xm",           "HOT DAMN",                   {}),
    ("nursery_time",           "tools/nursery_time.xm",           "NURSERY TIME",               {'v2': True}),
    ("closer_to_the_end",      "tools/closer_to_the_end.xm",      "CLOSER TO THE END",          {'v2': True}),
]
CANDIDATES = [ # options for the one visible slot still open (v2 = the better pitch and role analysis)
    ("worthless_clouds",       "tools/worthless_clouds.xm",       "WORTHLESS CLOUDS",           {'v2': True}),
    ("the_dipper_man",         "tools/the_dipper_man.xm",         "THE DIPPER MAN (title song)", {'v2': True}),
    ("tree_swaying_action",    "tools/tree_swaying_action.xm",    "TREE-AGE IN ACTION",        {'v2': True}),
    ("earth_and_the_space_citizens", "tools/earth_and_the_space_citizens.xm", "EARTH AND THE SPACE CITIZENS", {'v2': True}),
]
GROUPS = [('songs', SONGS), ('secret', SECRET), ('candidates', CANDIDATES)]

# ---------------------------------------------------------------- reading a song
def load(sid, path):
    S = parse(path)
    for table in (X.DANCES, X.TREES, X.POPS, X.CLOUDS, X.ENDINGS):
        if sid in table: table[sid](S)
    used = {i for o in S['order'] for r in S['pats'][o] for (n, i, v, e, ep) in r if n and n < 97}
    with contextlib.redirect_stdout(io.StringIO()):
        insts = X.convert_samples(S, used, sid)
    return S, insts

def analyse(I, name, notes, v2=False):
    """what an instrument is: dict(role, f0 = sounding pitch at its median note, med, sec, env = loudness over the sample)"""
    q = I['q'][:-1].astype(float); st = np.array(I['steps']) / 65536.0
    med = int(np.median(notes)); rate = st[med - 1] * MIXR            # sample-frames consumed per second at the median note
    sec = len(q) / rate; distinct = len(set(notes))
    w = q * np.hanning(len(q)); sp = np.abs(np.fft.rfft(w)) ** 2 + 1e-12; fr = np.fft.rfftfreq(len(q), 1 / rate)
    cen = float((sp * fr).sum() / sp.sum())
    band = (fr > 100) & (fr < 8000); flat = float(np.exp(np.log(sp[band]).mean()) / sp[band].mean()) if band.sum() > 3 else 0.0
    # pitch: normalised autocorrelation, lags that correspond to 32..2400 Hz, first strong peak
    s0 = int(0.02 * len(q)); seg = q[s0:s0 + 12000]; seg = seg - seg.mean(); f0 = 0.0; strength = 0.0
    kmin = max(4, int(rate / 2400)); kmax = min(len(seg) // 2, int(rate / 32))
    if kmax > kmin + 4 and np.abs(seg).max() > 0:
        ac = signal.correlate(seg, seg, 'full', method='fft')[len(seg) - 1:]; ac = ac / (ac[0] + 1e-9)
        k = kmin + int(np.argmax(ac[kmin:kmax])); strength = float(ac[k])
        for kk in range(kmin + 1, kmax - 1):
            if ac[kk] > 0.9 * strength and ac[kk] >= ac[kk - 1] and ac[kk] >= ac[kk + 1]: k = kk; break
        if v2 and kmin < k < kmax - 1:                                              # v2: parabolic peak -> sub-sample lag, then snap to the nearest semitone (equal temperament)
            y0, y1, y2 = ac[k - 1], ac[k], ac[k + 1]; den = y0 - 2 * y1 + y2
            if den != 0: k = k + 0.5 * (y0 - y2) / den
        f0 = rate / k
        if v2 and strength >= 0.6: f0 = 440.0 * 2 ** (round(12 * np.log2(f0 / 440.0)) / 12)
    nm = name.lower()
    tonal = strength >= 0.6 and (flat < 0.25 or cen < 400)
    if 'kick' in nm: role = 'kick'
    elif 'snare' in nm or 'clap' in nm: role = 'snare'
    elif 'open hat' in nm: role = 'ohat'
    elif 'hat' in nm: role = 'hat'
    elif 'crash' in nm or 'cymbal' in nm: role = 'crash'
    elif v2 and ('rim' in nm.split() or 'tom' in nm.split() or 'clap' in nm): role = 'snare'
    elif v2 and ('drop' in nm or 'swell' in nm): role = 'fx'
    elif 'riser' in nm or 'impact' in nm or 'laser' in nm or 'zap' in nm or 'sweep' in nm or 'fx' in nm: role = 'fx'
    elif v2 and 'pad' in nm: role = 'pad'
    elif not tonal and cen > 3000 and sec >= 0.6: role = 'crash'
    elif not tonal and cen > 3000: role = 'ohat' if sec > 0.15 else 'hat'
    elif not tonal and cen < 260 and sec < 0.7 and distinct <= 2: role = 'kick'
    elif not tonal and sec < 0.7 and 600 < cen <= 3000: role = 'snare'
    elif not tonal: role = 'fx'
    elif 'bass' in nm or nm.strip().startswith('sub'): role = 'bass'
    elif f0 < 150 and cen < 700 and sec < 1.2 and not any(w in nm for w in ('pad', 'stab', 'chord', 'string', 'organ', 'piano', 'choir', 'vowel', 'lead', 'pluck', 'arp')): role = 'bass'
    elif sec > 1.0 and distinct <= 8: role = 'pad'
    else: role = 'lead'
    # loudness over the sample (16 steps of RMS), played at its natural speed: the chip envelope follows it
    nb = 16; edges = np.linspace(0, len(q), nb + 1).astype(int)
    env = np.array([np.sqrt((q[a:b] ** 2).mean()) if b > a else 0 for a, b in zip(edges[:-1], edges[1:])]); env = env / (env.max() + 1e-9)
    return dict(role=role, f0=f0, strength=strength, med=med, sec=sec, cen=cen, env=env, n=len(q), steps=st, name=name)

# ---------------------------------------------------------------- the song as a list of notes
def build(S, insts, info, rowsec):
    """rows[g] = (order index, row in pattern); notes = list of dict(g, ch, inst, role, freq, vel, dur) for every note of the whole song"""
    rows = []; notes = []; ch_last = {}
    for oi, o in enumerate(S['order']):
        for ri, r in enumerate(S['pats'][o]):
            g = len(rows); rows.append((oi, ri))
            for ch, (n, i, v, e, ep) in enumerate(r):
                if not (n and n < 97) or i not in info: continue
                a = info[i]; I = insts[i - 1]
                rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
                vel = I['svol'] * I['pk'] * rel
                natdur = a['n'] / (a['steps'][n - 1] * MIXR)                       # how long the sample really sounds at this note
                freq = a['f0'] * 2 ** ((n - a['med']) / 12) if a['role'] in ('bass', 'lead', 'pad') else 0.0
                notes.append(dict(g=g, ch=ch, inst=i, role=a['role'], freq=freq, vel=vel, dur=natdur, n=n))
    return rows, notes

def window_score(notes_by_row, g0, g1):
    """how much is going on in rows g0..g1-1: groups present (drums, bass, melody) and their density"""
    groups = {'d': 0, 'b': 0, 'm': 0}; tot = 0
    for g in range(g0, g1):
        for nt in notes_by_row.get(g, ()):
            k = 'd' if nt['role'] in ('kick', 'snare', 'hat', 'ohat', 'crash') else 'b' if nt['role'] == 'bass' else 'm' if nt['role'] in ('lead', 'pad') else None
            if k: groups[k] += 1; tot += 1
    return groups, tot

def pick_window(S, rows, notes, rowsec, tmin, tmax, relax=False):
    """a run of whole patterns (order positions) that repeats in the song, tmin..tmax seconds long, with drums + bass + melody all busy"""
    order = S['order']; npat = [len(S['pats'][o]) for o in order]
    first = {}; acc = 0
    for oi in range(len(order)): first[oi] = acc; acc += npat[oi]
    by_row = {}
    for nt in notes: by_row.setdefault(nt['g'], []).append(nt)
    best = None
    for o0 in range(len(order)):
        for k in range(1, len(order) - o0 + 1):
            nrows = sum(npat[o0:o0 + k]); secs = nrows * rowsec
            if secs > tmax: break
            if secs < tmin: continue
            seq = order[o0:o0 + k]; g0 = first[o0]; g1 = g0 + nrows
            groups, tot = window_score(by_row, g0, g1)
            need = max(2, nrows // 16); have = sum(1 for v in groups.values() if v >= need)
            if have < 3 and not (relax and have >= 2): continue                     # all three layers (drums, bass, melody), or two when the song has no third
            rep = sum(1 for p in range(len(order) - k + 1) if order[p:p + k] == seq)
            nxt = order[o0 + k:o0 + 2 * k] == seq
            dens = tot / secs
            score = dens * (1.0 + 0.6 * (rep >= 2) + 0.4 * nxt) * (1.0 - 0.02 * abs(secs - 15))
            if best is None or score > best[0]: best = (score, o0, k, g0, g1, rep, secs)
    if best is None and not relax: return pick_window(S, rows, notes, rowsec, tmin, tmax, True)
    if best is None:                                                                 # nothing fits: take the busiest stretch of tmax seconds
        nrows = int(tmax / rowsec); bestg = (-1, 0)
        for g0 in range(0, max(1, len(rows) - nrows), 8):
            groups, tot = window_score(by_row, g0, g0 + nrows)
            if tot > bestg[0]: bestg = (tot, g0)
        g0 = bestg[1]; best = (0, rows[g0][0], 0, g0, g0 + nrows, 1, nrows * rowsec)
    return best

# ---------------------------------------------------------------- voicing for four chip channels
def quant(a): return int(max(0, min(15, round(15 * a))))

class Track:
    """a chip voice as control-rate arrays: pitch (Hz), level 0..15 (and noise period / mode)"""
    def __init__(self, n): self.f = np.zeros(n); self.a = np.zeros(n, dtype=int); self.p = np.zeros(n, dtype=int); self.m = np.zeros(n, dtype=int)

def env_at(env, u_norm):
    if u_norm >= 1: return 0.0
    x = u_norm * (len(env) - 1); i = int(x); f = x - i
    return env[i] * (1 - f) + env[min(i + 1, len(env) - 1)] * f

def fold(freq, lo, hi):
    while freq < lo: freq *= 2
    while freq > hi: freq /= 2
    return freq

def voice(S, insts, info, rows, notes, g0, g1, rowsec, opts):
    R = g1 - g0; NLoop = int(round(R * rowsec * CR)); NLoop += NLoop % 2           # whole control steps per loop (even: the decimation lands on whole samples)
    N = 3 * NLoop
    def frame(c, r): return c * NLoop + int(round(r * NLoop / R))                   # control step where row r of copy c starts
    seq = [nt for nt in notes if g0 <= nt['g'] < g1]
    # velocity scale: the loud notes of the section reach full level
    vels = np.array([nt['vel'] for nt in seq]) if seq else np.array([1.0]); vs = max(np.percentile(vels, 95), 1e-6)
    for nt in seq: nt['v'] = min(1.0, nt['vel'] / vs)
    # --- which instruments carry the tune: the lead is the melodic instrument with the busiest, most varied line
    cand = {}
    for nt in seq:
        if nt['role'] == 'lead': cand.setdefault(nt['inst'], []).append(nt)
    def lead_score(i):
        ns = cand[i]; fr = [x['freq'] for x in ns]; return len(ns) ** 0.5 * (1 + 0.15 * len(set(round(12 * np.log2(f / 55)) for f in fr))) * (1.4 if np.median(fr) > 300 else 1.0)
    lead_i = opts.get('lead') or (max(cand, key=lead_score) if cand else None)
    tracks = dict(p1=Track(N), p2=Track(N), tri=Track(N), noi=Track(N))
    layout = []                                                                      # for the piano roll: (voice, t_on, t_off, pitch)
    def note_end(nt, g_on, onsets_after):
        return min(nt['dur'] / rowsec, onsets_after - g_on if onsets_after else 1e9)
    # --- bass (triangle): the lowest bass note of each row, one at a time
    bass_by_row = {}
    for nt in seq:
        if nt['role'] == 'bass': bass_by_row.setdefault(nt['g'], []).append(nt)
    if not bass_by_row:                                                              # no bass instrument: the root under the pads / stabs
        for nt in seq:
            if nt['role'] in ('pad', 'lead') and nt['freq'] > 0: bass_by_row.setdefault(nt['g'], []).append(dict(nt, freq=nt['freq'], role='bass'))
        step = {g: min(v, key=lambda x: x['freq']) for g, v in bass_by_row.items()}
        bass_by_row = {g: [x] for g, x in step.items() if g % 4 == g0 % 4}
    bl = sorted(((g, min(v, key=lambda x: x['freq'])) for g, v in bass_by_row.items()), key=lambda x: x[0])
    if bl:
        med = np.median([np.log2(x['freq']) for g, x in bl]); shift = round(np.log2(82.0) - med)       # the line's middle near E2
        for c in range(3):
            for idx, (g, nt) in enumerate(bl):
                nxt = bl[idx + 1][0] if idx + 1 < len(bl) else (bl[0][0] + R)
                r_on = g - g0; r_off = min(r_on + nt['dur'] / rowsec, nxt - g0, r_on + 16)
                f = fold(nt['freq'] * 2 ** shift, 41.0, 262.0); j0 = frame(c, r_on); j1 = max(j0 + 1, frame(c, r_off) - 1)
                tracks['tri'].f[j0:min(j1, N)] = f; tracks['tri'].a[j0:min(j1, N)] = 15
    # --- lead (pulse 1): one note at a time, the loudest melodic onset of each row (the lead instrument wins), vibrato on held notes
    lead_by_row = {}
    for nt in seq:
        if nt['role'] == 'lead' and (lead_i is None or nt['inst'] == lead_i): lead_by_row.setdefault(nt['g'], []).append(nt)
    ll = sorted(((g, max(v, key=lambda x: (x['v'], x['freq']))) for g, v in lead_by_row.items()), key=lambda x: x[0])
    lead_shift = 0
    if ll:
        med = np.median([np.log2(x['freq']) for g, x in ll]); lead_shift = round(np.log2(660.0) - med)   # the line's middle near E5
        for c in range(3):
            for idx, (g, nt) in enumerate(ll):
                nxt = ll[idx + 1][0] if idx + 1 < len(ll) else (ll[0][0] + R)
                r_on = g - g0; natr = nt['dur'] / rowsec; r_off = min(r_on + natr, nxt - g0, r_on + 12)
                f0 = fold(nt['freq'] * 2 ** lead_shift, 196.0, 2093.0); j0 = frame(c, r_on); j1 = max(j0 + 1, frame(c, r_off)); a_env = info[nt['inst']]['env']
                for j in range(j0, min(j1, N)):
                    u = (j - j0) / CR; a = env_at(a_env, u / max(nt['dur'], 1e-3)); a = max(a, 0.0) ** 0.8
                    q = quant(a * (0.55 + 0.45 * nt['v']) * 1.0)
                    if q <= 0: continue
                    vib = 1.0 + (0.0045 * np.sin(2 * np.pi * 5.6 * (u - 0.18)) if u > 0.18 and (j1 - j0) / CR > 0.3 else 0.0)
                    tracks['p1'].f[j] = f0 * vib; tracks['p1'].a[j] = q
    # --- harmony (pulse 2): every other tonal note sounding, up to 3 of them, arpeggiated at 60 Hz; no harmony -> an echo of the lead
    harm = [nt for nt in seq if nt['role'] in ('pad', 'lead') and nt['freq'] > 0 and nt['inst'] != lead_i]
    if harm:
        med = np.median([np.log2(x['freq']) for x in harm]); hs = round(np.log2(392.0) - med)
        for c in range(3):
            ons = []
            for nt in harm:
                r_on = nt['g'] - g0; r_off = r_on + min(nt['dur'] / rowsec, 16); f = fold(nt['freq'] * 2 ** hs, 130.0, 1760.0)
                ons.append((frame(c, r_on), max(frame(c, r_on) + 1, frame(c, r_off)), f, nt['v'], info[nt['inst']]['env'], nt['dur']))
            for j in range(c * NLoop, min(N, (c + 1) * NLoop + NLoop // 2)):
                act = [(f, v * env_at(en, (j - a) / CR / max(d, 1e-3)), a) for (a, b, f, v, en, d) in ons if a <= j < b]
                act = [x for x in act if x[1] > 0.04]
                if not act: continue
                act.sort(key=lambda x: x[0])
                pcs = {}
                for f, v, a in act: pcs.setdefault(int(round(12 * np.log2(f / 55.0))) % 12, (f, v, a))
                chord = sorted(pcs.values(), key=lambda x: x[0]);
                if len(chord) > 3: chord = [chord[0], chord[len(chord) // 2], chord[-1]]
                k = (j // 2) % len(chord); lvl = min(1.0, max(x[1] for x in chord) * 0.9)
                q = quant(lvl * 0.6)
                if q > 0 and tracks['p2'].a[j] == 0: tracks['p2'].f[j] = chord[k][0]; tracks['p2'].a[j] = q
    elif ll:
        delay = 3
        for c in range(3):
            for idx, (g, nt) in enumerate(ll):
                nxt = ll[idx + 1][0] if idx + 1 < len(ll) else (ll[0][0] + R)
                r_on = g - g0 + 0.75; r_off = min(r_on + min(nt['dur'] / rowsec, 8), nxt - g0 + 0.75)
                f0 = fold(nt['freq'] * 2 ** lead_shift, 196.0, 2093.0); j0 = frame(c, r_on); j1 = max(j0 + 1, frame(c, r_off)); a_env = info[nt['inst']]['env']
                for j in range(j0, min(j1, N)):
                    u = (j - j0) / CR; a = env_at(a_env, u / max(nt['dur'], 1e-3)); q = quant(max(a, 0) * 0.35 * (0.55 + 0.45 * nt['v']))
                    if q > 0: tracks['p2'].f[j] = f0; tracks['p2'].a[j] = q
    # --- drums (noise): the strongest hit of each row: kick > snare > crash > open hat > hat
    prio = {'kick': 5, 'snare': 4, 'crash': 3, 'ohat': 2, 'hat': 1}
    drum_by_row = {}
    for nt in seq:
        if nt['role'] in prio: drum_by_row.setdefault(nt['g'], []).append(nt)
    DR = {'kick': (14, 0, 15, 5, 1.0), 'snare': (8, 0, 13, 9, 1.0), 'hat': (2, 1, 7, 3, 0.9), 'ohat': (3, 0, 8, 10, 0.9), 'crash': (4, 0, 9, 40, 0.8)}   # period idx, mode, level, frames of decay, vel weight
    for c in range(3):
        for g, hits in sorted(drum_by_row.items()):
            nt = max(hits, key=lambda x: (prio[x['role']], x['v'])); per, mode, lvl, dec, vw = DR[nt['role']]
            r_on = g - g0; j0 = frame(c, r_on)
            nxt_rows = [x for x in sorted(drum_by_row) if x > g]; r_next = (nxt_rows[0] - g0) if nxt_rows else R + (min(drum_by_row) - g0)
            j1 = max(j0 + 1, frame(c, r_next))
            for j in range(j0, min(j1, N)):
                k = int((j - j0) * 60 / CR); a = lvl * max(0.0, 1 - k / dec) * (0.55 + 0.45 * nt['v'] * vw)
                q = quant(a / 15)
                if q <= 0: break
                tracks['noi'].a[j] = q; tracks['noi'].p[j] = per; tracks['noi'].m[j] = mode
    for key in ('p1', 'p2', 'tri', 'noi'):                                           # piano roll of the middle copy
        t = tracks[key]; j = NLoop; end = 2 * NLoop
        while j < end:
            if t.a[j] > 0:
                k = j
                if key == 'noi':
                    while k + 1 < end and t.a[k + 1] > 0 and t.p[k + 1] == t.p[j]: k += 1
                    layout.append((key, (j - NLoop) / CR, (k + 1 - NLoop) / CR, {14: 36, 8: 60, 2: 90, 3: 88, 4: 96}.get(int(t.p[j]), 70)))
                else:
                    while k + 1 < end and t.a[k + 1] > 0 and abs(t.f[k + 1] / t.f[j] - 1) < 0.03: k += 1
                    layout.append((key, (j - NLoop) / CR, (k + 1 - NLoop) / CR, t.f[j]))
                j = k + 1
            else: j += 1
    pc = np.zeros(12)
    for key in ('p1', 'p2', 'tri'):
        t = tracks[key]
        for j in range(NLoop, 2 * NLoop):
            if t.a[j] > 0 and t.f[j] > 0: pc[int(round(12 * np.log2(t.f[j] / 440.0))) % 12] += 1
    scale = [0, 2, 4, 5, 7, 9, 11]; fit = max(sum(pc[(r + d) % 12] for d in scale) for r in range(12)) / max(pc.sum(), 1)
    return tracks, NLoop, layout, dict(lead=lead_i, rows=R, nloop=NLoop, keyfit=fit)

# ---------------------------------------------------------------- rendering (NES style)
NOISE_P = [4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068]     # LFSR clock period in CPU cycles
def lfsr(short):
    n = 93 if short else 32767; reg = 1; out = np.zeros(n);
    for i in range(n):
        out[i] = 1.0 if (reg & 1) == 0 else 0.0
        fb = (reg & 1) ^ ((reg >> (6 if short else 1)) & 1); reg = (reg >> 1) | (fb << 14)
    return out
LF_LONG = lfsr(False); LF_SHORT = lfsr(True)
TRI = np.array(list(range(15, -1, -1)) + list(range(0, 16)), dtype=float)

def render(tr, n_ctrl, duty):
    FSo = FS * OS; n = n_ctrl * SPC
    def up(x): return np.repeat(x, SPC)
    out = {}
    for key, d in (('p1', duty[0]), ('p2', duty[1])):
        t = tr[key]; f = up(t.f); a = up(t.a.astype(float)); ph = np.cumsum(f / FSo) % 1.0
        out[key] = (ph < d).astype(float) * a
    t = tr['tri']; f = up(t.f); ph = np.cumsum(f / FSo) % 1.0; out['tri'] = TRI[(ph * 32).astype(int) % 32] * (up(t.a.astype(float)) > 0)
    t = tr['noi']; per = np.array([NOISE_P[min(15, p)] for p in t.p]); fclk = up(1789773.0 / per); pos = np.cumsum(fclk / FSo)
    idx = pos.astype(np.int64); m = up(t.m)
    out['noi'] = np.where(m > 0, LF_SHORT[idx % 93], LF_LONG[idx % 32767]) * up(t.a.astype(float))
    p = out['p1'] + out['p2']; pm = np.where(p > 0, 95.88 / (8128.0 / np.maximum(p, 1e-9) + 100), 0.0)             # the APU's non-linear mixer
    tn = out['tri'] / 8227.0 + out['noi'] / 12241.0; tm = np.where(tn > 0, 159.79 / (1.0 / np.maximum(tn, 1e-12) + 100), 0.0)
    y = pm + tm
    return signal.resample_poly(y, 1, OS, window=('kaiser', 8.0))

def finish(y):
    y = y - y.mean(); b, a = signal.butter(1, 40 / (FS / 2), 'high'); y = signal.lfilter(b, a, y)
    return y / (np.abs(y).max() + 1e-9) * 0.89

# ---------------------------------------------------------------- output helpers
def write_wav(path, y):
    with wave.open(path, 'wb') as w: w.setnchannels(1); w.setsampwidth(2); w.setframerate(FS); w.writeframes((np.clip(y, -1, 1) * 32767).astype('<i2').tobytes())

def piano_roll(path, layout, secs, title):
    from PIL import Image, ImageDraw
    W, H = 1000, 360; im = Image.new('RGB', (W, H), (12, 14, 20)); d = ImageDraw.Draw(im)
    col = {'p1': (255, 214, 64), 'p2': (90, 200, 255), 'tri': (120, 255, 140), 'noi': (255, 110, 110)}
    for v, a, b, f in layout:
        m = 12 * np.log2(max(f, 20.0) / 440.0) + 69 if v != 'noi' else f
        y = H - 10 - (m - 20) / 100 * (H - 30); x0 = 8 + a / secs * (W - 16); x1 = 8 + b / secs * (W - 16)
        d.rectangle([x0, y - 2, max(x1, x0 + 2), y + 2], fill=col[v])
    d.text((10, 4), title + '   (yellow lead, blue harmony, green bass, red drums)', fill=(230, 230, 240)); im.save(path)

def encode_mp3(wav, mp3):
    try: subprocess.run(['ffmpeg', '-y', '-loglevel', 'error', '-i', wav, '-codec:a', 'libmp3lame', '-b:a', '128k', mp3], check=True)
    except Exception as e: print('  (no mp3: %s)' % e)

# ---------------------------------------------------------------- one song
def make(idx, sid, path, title, opts, out_dir, report):
    S, insts = load(sid, path)
    ev = {}
    for o in S['order']:
        for r in S['pats'][o]:
            for ch, (n, i, v, e, ep) in enumerate(r):
                if n and n < 97 and insts[i - 1]: ev.setdefault(i, []).append(n)
    info = {i: analyse(insts[i - 1], S['insts'][i - 1]['name'], ev[i], opts.get('v2', False)) for i in ev}
    if not any(a['role'] == 'bass' for a in info.values()):                          # no bass by pitch: the lowest tonal instrument that plays a lot is the bass
        low = [i for i, a in info.items() if a['role'] in ('lead', 'pad') and a['f0'] < 330 and len(ev[i]) >= 16]
        if low: info[min(low, key=lambda i: info[i]['f0'])]['role'] = 'bass'
    rowsec = S['tempo'] * 2.5 / S['bpm']
    rows, notes = build(S, insts, info, rowsec)
    tmin, tmax = opts.get('win', (9.0, 20.0))
    score, o0, k, g0, g1, rep, secs = pick_window(S, rows, notes, rowsec, tmin, tmax)
    tracks, NLoop, layout, meta = voice(S, insts, info, rows, notes, g0, g1, rowsec, opts)
    duty = opts.get('duty', (0.25, 0.125))
    y = render(tracks, 3 * NLoop, duty); L = NLoop * (FS // CR); y = finish(y)
    loop = y[L:2 * L]
    name = '%02d_%s' % (idx, sid); os.makedirs(out_dir, exist_ok=True)
    wav = os.path.join(out_dir, name + '.wav'); write_wav(wav, loop)
    two = os.path.join(out_dir, name + '_x2.wav'); write_wav(two, np.concatenate([loop, loop])); encode_mp3(two, os.path.join(out_dir, name + '.mp3')); os.remove(two)
    piano_roll(os.path.join(out_dir, name + '.png'), layout, L / FS, '%s' % title)
    roles = {}
    for i, a in info.items(): roles.setdefault(a['role'], []).append(S['insts'][i - 1]['name'].strip())
    lead_name = S['insts'][meta['lead'] - 1]['name'].strip() if meta['lead'] else '-'
    line = '%-30s %5.1f s loop, %3d rows, orders %d..%d (%s), lead = %s, bass = %s' % (title, L / FS, g1 - g0, o0, o0 + k - 1, 'comes round %dx' % rep if rep > 1 else 'once', lead_name, ', '.join(sorted(set(S['insts'][i - 1]['name'].strip() for i, a in info.items() if a['role'] == 'bass'))) or '-')
    print('  ' + line); report.append(line)
    print('  key fit %.0f%%' % (100 * meta['keyfit']))
    return dict(loop=loop, info=info)

if __name__ == '__main__':
    args = sys.argv[1:]; out_dir = 'chiptune_out'
    allids = [x[0] for _, g in GROUPS for x in g]
    if args and args[0] not in allids: out_dir = args.pop(0)
    want = set(args); report = []
    print('chiptune loops -> %s' % out_dir)
    for gname, group in GROUPS:
        for idx, (sid, path, title, opts) in enumerate(group, 1):
            if want and sid not in want: continue
            print('%s %d. %s' % (gname, idx, title)); make(idx, sid, path, title, opts, os.path.join(out_dir, gname), report)
    os.makedirs(out_dir, exist_ok=True); open(os.path.join(out_dir, 'chiptunes.txt'), 'w').write('\n'.join(report) + '\n')
