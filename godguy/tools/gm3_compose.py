# GodGuy's five great suites (7 to 22 minutes): each is a medley woven from excerpts of the other tracks of the soundtrack. Excerpts are cut on 8-bar boundaries from tracks near the
# suite's tempo, time-scaled to it exactly and transposed along a slow key plan, so the whole soundtrack is amalgamated into five long journeys. Called from gm2_compose.py.
import random, math

SUITES = [   # id, name, minutes, base bpm, mood (the timbre set), source moods allowed (None = any), home key, key plan (semitones above home, per excerpt)
 dict(id='suite1', name='ONE LONG DAWN', mins=7.5, bpm=90, mood='orch', pick={'orch', 'folk', 'glass', 'minim', 'pop'}, home=2, plan=[0, 0, 5, 0, 7, 5, 0]),
 dict(id='suite2', name='THE WHOLE ROAD', mins=10.0, bpm=116, mood='brass', pick={'brass', 'folk', 'pop', 'rock', 'orch', 'chip'}, home=7, plan=[0, 2, 0, 5, 7, 0, 9, 5]),
 dict(id='suite3', name="A GOD'S AFTERNOON ON THE LONG COUCH", mins=13.5, bpm=98, mood='glass', pick={'glass', 'minim', 'minim2', 'folk', 'orch'}, home=0, plan=[0, 0, 7, 0, 5, 0, 9, 0, 5]),
 dict(id='suite4', name='ALL THE KINGS AND ALL THE STORMS', mins=17.0, bpm=128, mood='rock', pick={'dance', 'rock', 'brass', 'pop', 'dark', 'chip', 'minim2'}, home=9, plan=[0, 0, 3, 5, 0, 7, 10, 0, 2]),
 dict(id='suite5', name='THE ENDLESS BORE: THE WHOLE SOUNDTRACK', mins=22.5, bpm=108, mood='orch', pick=None, home=5, plan=[0, 7, 0, 5, 2, 9, 0, 3, 7, 0, 5, 10]),
]

def excerpt_windows(R, bars):
    # preferred 16-bar windows on 8-bar boundaries (choruses and finals of the standard form), else any 8-aligned window
    if bars >= 120: return [24, 72, 96]
    return [b for b in range(8, bars - 24 + 1, 8)] or [0]

def build_suites(G, base_tracks, built):
    rnd = random.Random(1234); used = {}; out = []
    for S in SUITES:
        target = int(S['mins'] * 60 * S['bpm'] / 60 * 4 / 16 + 0.5)      # bars of 16 rows at 15/bpm seconds each: bar = 4 beats
        cand = []
        for i, R in enumerate(base_tracks):
            sc, loop_bar, bpm, kt, mt = built[i]
            if R.get('form') in ('jingle', 'sting', 'ambient', 'boss', 'short'): continue
            if sc.cells // 16 < 64: continue
            f = S['bpm'] / bpm
            if not (0.84 <= f <= 1.2): continue
            if S['pick'] is not None and R['mood'] not in S['pick']: continue
            cand.append(i)
        if len(cand) < 12: raise SystemExit('suite %s: too few sources (%d)' % (S['id'], len(cand)))
        # the pieces: an intro, then excerpts, then an outro
        pieces = []; bars = 0; k = 0
        def choose():
            pool = sorted(cand, key=lambda i: (used.get(i, 0), rnd.random())); return pool[0]
        i = choose(); used[i] = used.get(i, 0) + 1; pieces.append((i, 0, 8, S['home'])); bars += 8
        while bars < target - 8:
            i = choose(); used[i] = used.get(i, 0) + 1
            sc = built[i][0]; nb = sc.cells // 16; wins = excerpt_windows(base_tracks[i], nb)
            b0 = rnd.choice(wins); ln = 16 if nb - b0 >= 16 else 8
            key = (S['home'] + S['plan'][k % len(S['plan'])]) % 12; k += 1
            pieces.append((i, b0, ln, key)); bars += ln
        i = choose(); used[i] = used.get(i, 0) + 1; nb = built[i][0].cells // 16; pieces.append((i, nb - 8, 8, S['home'])); bars += 8
        # the weave
        total_cells = 0; segs = []
        for (i, b0, ln, key) in pieces:
            sc, lb, bpm, kt, mt = built[i]; f = S['bpm'] / bpm; n = int(round(ln * 16 * f)); segs.append((i, b0, ln, key, f, n, kt)); total_cells += n
        out_sc = G.Sc((total_cells + 15) // 16 + 1); pos = 0
        for (i, b0, ln, key, f, n, kt) in segs:
            sc = built[i][0]; shift = ((key - kt + 6) % 12) - 6; c0 = b0 * 16; c1 = c0 + ln * 16
            for vc in range(G.NV):
                cells = sc.v[vc]
                for c in range(c0, c1):
                    nt, l = cells[c]
                    if nt <= 0: continue
                    nc = pos + int(round((c - c0) * f)); ne = pos + int(round((min(c + l, c1) - c0) * f)); l2 = max(1, ne - nc)
                    if vc < 9:
                        nt += shift
                        while nt > G.MAXN: nt -= 12
                        while 0 < nt < 24: nt += 12
                    if nc < out_sc.cells: out_sc.v[vc][nc] = (nt, min(l2, out_sc.cells - nc))
            pos += n
        out_sc.cells = pos; 
        for vc in range(G.NV): out_sc.v[vc] = out_sc.v[vc][:pos]
        mins = pos * 15.0 / S['bpm'] / 60
        out.append((S, out_sc, [(base_tracks[i]['name'], b0, ln) for (i, b0, ln, key, f, n, kt) in segs], mins))
    return out
