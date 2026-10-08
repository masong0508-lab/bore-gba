#!/usr/bin/env python3
"""CUTSCENE TUNES: four stripped-down chip renditions of jukebox songs, for the story cutscenes.

Each one is read from the voiced creator-menu loop of the song (the same tracks tools/chip_synth.py stores), then simplified:
  PULSE 1  the tune on an 8th-note grid (one pitch per 8th, held notes joined, every pitch snapped to a semitone, flat level)
  TRIANGLE the bass on a quarter-note grid          NOISE  a plain drum pattern (STYLE: beat / half / sparse / none)  PULSE 2  unused
The loop is a whole number of bars and every beat is exactly BEAT steps (120 steps a second), so the game can read the beat from the player's
step counter and lock the figures' dancing to it (cutscene.h csMusT).   custom_loops() is called by tools/chip_synth.py.
"""
import numpy as np
import make_chiptunes as MC
CR = 120
# key, song id, mood title, drum style, beats in the loop (4 per bar)
TUNES = [
    ("sunman",   "sunman_sunrise",   "CS SUNMAN",   "beat",   32),
    ("whistler", "whistler_shuffle", "WHISTLER MAN  SCENE SCORE", "score", 96, 0, 1),   # also in the creator menu's hidden list (secret code), with its beat clock
    ("cora",     "mi_cora_zone",     "CS CORA",     "sparse", 32),
    ("excuses",  "excuses_house",    "CS EXCUSES",  "sparse", 32),
    ("magic",    "magic_act",        "CS MAGIC",    "half",   24, 16),   # the hopeful one: the opening night scene (16 rows to a beat = a gentle 95 BPM)
]   # (key, song, title, drums, beats, rows per beat (0 = automatic), 1 = also list in the creator menu's hidden list)
def src(sid):
    for _, lst in MC_GROUPS():
        for (s, path, title, opts) in lst:
            if s == sid: return path, opts
    raise KeyError(sid)
def MC_GROUPS(): return [(0, MC.SONGS), (1, MC.SECRET)]
def meta_for(sid, path, opts, full=False):
    import io, contextlib
    with contextlib.redirect_stdout(io.StringIO()):
        S, insts = MC.load(sid, path)
        ev = {}
        for o in S['order']:
            for r in S['pats'][o]:
                for ch, (n, i, v, e, ep) in enumerate(r):
                    if n and n < 97 and insts[i - 1]: ev.setdefault(i, []).append(n)
        info = {i: MC.analyse(insts[i - 1], S['insts'][i - 1]['name'], ev[i], opts.get('v2', False)) for i in ev}
        if not any(a['role'] == 'bass' for a in info.values()):
            low = [i for i, a in info.items() if a['role'] in ('lead', 'pad') and a['f0'] < 330 and len(ev[i]) >= 16]
            if low: info[min(low, key=lambda i: info[i]['f0'])]['role'] = 'bass'
        rowsec = S['tempo'] * 2.5 / S['bpm']
        rows, notes = MC.build(S, insts, info, rowsec)
        tmin, tmax = opts.get('win', (9.0, 20.0))
        score, o0, k, g0, g1, rep, secs = MC.pick_window(S, rows, notes, rowsec, tmin, tmax)
        if full: g0, g1 = 0, len(rows)   # SCORE: the whole song, from its first row, instead of the best looping window
        tracks, NLoop, layout, meta = MC.voice(S, insts, info, rows, notes, g0, g1, rowsec, opts)
    return tracks, NLoop, rowsec, opts.get('duty', (0.25, 0.125))
def snap(f): return 0.0 if f <= 0 else 440.0 * 2 ** (round(12 * np.log2(f / 440.0)) / 12)
def hold(vals, slot):
    """vals: one pitch per slot (0 = rest) -> list of (first slot, slots held, pitch), equal neighbours joined"""
    out = []
    for i, f in enumerate(vals):
        if f <= 0: continue
        if out and out[-1][2] == f and out[-1][0] + out[-1][1] == i: out[-1] = (out[-1][0], out[-1][1] + 1, f)
        else: out.append((i, 1, f))
    return out
def rendition(key, sid, title, style, beats, rpb0=0, menu=0):
    path, opts = src(sid)
    score = (style == 'score')
    tr, NL, rowsec, duty = meta_for(sid, path, opts, full=score)
    rowsteps = rowsec * CR; rpb = rpb0 or (4 if 4 * rowsteps >= 42 else 8); BEAT = int(round(rpb * rowsteps)); N = beats * BEAT; q = BEAT // 2; qb = BEAT        # step counts of an 8th and a quarter
    mid = lambda k: (tr[k].f[NL:2 * NL], tr[k].a[NL:2 * NL])
    t = {k: MC.Track(N) for k in ('p1', 'p2', 'tri', 'noi')}
    first = 0
    if score:   # skip the song's silent start: begin on the first beat where the bass or the tune sounds
        act = np.nonzero((mid('p1')[1] > 0) | (mid('tri')[1] > 0))[0]
        first = int(int(act[0]) // (rowsteps * rpb) * (rowsteps * rpb)) if len(act) else 0
    def at(k, j):                                   # the source's pitch at loop step j (the tune is read at its own tempo, then laid on the exact beat grid)
        f, a = mid(k); jj = (int(round(j * (rowsteps * rpb / BEAT))) + first) % NL
        return snap(f[jj]) if a[jj] > 0 else 0.0
    # SCORE automation (12 sections of 8 beats = 2 bars): what plays, how loud, which drums.  The tune builds like a scene does and the last section
    # thins out so the loop comes back round to the quiet start.   section: 0-1 the tune alone (the song's own intro), 2 hats, 3 the bass and the beat enter (as in the song), 6-9 full groove, 10 snare build, 11 breakdown
    LEAD = [8, 8, 9, 10, 11, 11, 12, 12, 12, 12, 12, 10]; DRUM = ['none', 'none', 'hats', 'half', 'half', 'half', 'beat', 'beat', 'beat', 'beat', 'roll', 'none']
    sec = lambda b: min(11, (b // 8) % 12)
    # lead: one pitch per 8th, taken a little after the slot starts
    lead = [at('p1', s * q + 2) or at('p2', s * q + 2) * 0 for s in range(N // q)]
    for (s, n, f) in hold(lead, q):
        j0 = s * q; j1 = j0 + n * q - (3 if n == 1 else 2)
        lv = LEAD[sec(j0 // BEAT)] if score else 10
        if not lv: continue
        for j in range(j0, max(j0 + 2, j1)): t['p1'].f[j] = f; t['p1'].a[j] = min(15, lv + 2) if j - j0 <= 1 else lv
    bass = [at('tri', s * qb + 2) for s in range(N // qb)]
    for (s, n, f) in hold(bass, qb):
        j0 = s * qb; j1 = j0 + n * qb - 6
        for j in range(j0, max(j0 + 2, j1)): t['tri'].f[j] = f; t['tri'].a[j] = 1
    DR = {'kick': (14, 0, 15, 5), 'snare': (8, 0, 13, 9), 'hat': (2, 1, 7, 3)}
    def hit(j0, kind):
        per, mode, lvl, dec = DR[kind]
        for j in range(j0, min(N, j0 + dec * 2)):
            a = lvl * max(0.0, 1 - ((j - j0) * 60 / CR) / dec) * 0.9; qv = MC.quant(a / 15)
            if qv <= 0: break
            t['noi'].a[j] = qv; t['noi'].p[j] = per; t['noi'].m[j] = mode
    for b in range(beats):
        j = b * BEAT; bi = b % 4
        st = DRUM[sec(b)] if score else style
        if st == 'hats': hit(j + (q if bi % 2 else 0), 'hat') if bi != 3 else None
        elif st == 'roll':
            hit(j, 'kick') if bi == 0 else None
            hit(j, 'snare'); hit(j + q, 'snare')
        elif st == 'none': pass
        if st == 'beat':
            if bi in (0, 2): hit(j, 'kick')
            if bi in (1, 3): hit(j, 'snare')
            hit(j + q, 'hat')
        elif st == 'half':
            if bi == 0: hit(j, 'kick')
            if bi == 2: hit(j, 'snare')
            if bi in (1, 3): hit(j, 'hat')
        elif st == 'sparse':
            if bi == 0: hit(j, 'kick')
            if bi == 2: hit(j, 'hat')
    tr3 = {}
    for k in t:                                      # three copies (the loop is periodic: chip_synth keeps the middle one)
        o = MC.Track(3 * N)
        for a in ('f', 'a', 'p', 'm'): setattr(o, a, np.tile(getattr(t[k], a), 3))
        tr3[k] = o
    return dict(secret=menu, menu=menu, sid='cs_' + key, title=title, tr=tr3, NL=N, duty=duty, beat=BEAT, custom=True,
                notes=int((t['p1'].a > 0).sum() > 0))
def custom_loops():
    return [rendition(*x) for x in TUNES]
if __name__ == '__main__':
    for L in custom_loops(): print(L['sid'], L['NL'] / CR, 's', 'beat', L['beat'], 'steps = %.1f BPM' % (60 * CR / L['beat']))
