# GodGuy's five great suites (7 to 22 minutes): each is a medley woven from excerpts of the other tracks of the soundtrack. Excerpts are cut on 8-bar boundaries from tracks near the
# suite's tempo, time-scaled to it exactly and transposed along a slow key plan, so the whole soundtrack is amalgamated into five long journeys. Called from gm2_compose.py.
import random, math

SUITES = [   # id, name, minutes, base bpm, mood (the timbre set), source moods allowed (None = any), home key, key plan (semitones above home, per excerpt)
 dict(id='suite1', name='ONE LONG DAWN', mins=7.5, bpm=90, mood='orch', pick={'orch', 'folk', 'glass', 'minim', 'pop'}, home=2, plan=[0, 0, 5, 0, 7, 5, 0]),
 dict(id='suite2', name='THE WHOLE ROAD', mins=10.0, bpm=116, mood='brass', pick={'brass', 'folk', 'pop', 'rock', 'orch', 'chip'}, home=7, plan=[0, 2, 0, 5, 7, 0, 9, 5]),
 dict(id='suite3', name="A GOD'S AFTERNOON ON THE LONG COUCH", mins=13.5, bpm=98, mood='glass', pick={'glass', 'minim', 'minim2', 'folk', 'orch'}, home=0, plan=[0, 0, 7, 0, 5, 0, 9, 0, 5]),
 dict(id='suite4', name='ALL THE KINGS AND ALL THE STORMS', mins=17.0, bpm=128, mood='rock', pick={'dance', 'rock', 'brass', 'pop', 'dark', 'chip', 'minim2'}, home=9, plan=[0, 0, 3, 5, 0, 7, 10, 0, 2]),
 dict(id='suite6', name='ALL OF IT, ONCE MORE', mins=26.0, bpm=112, mood='dance', pick=None, home=7, plan=[0, 5, 7, 0, 2, 9, 5, 0, 10, 3, 7, 0], allsuites=True, newlayer=True),
 dict(id='suite5', name='THE ENDLESS BORE', mins=22.5, bpm=108, mood='orch', pick=None, home=5, plan=[0, 7, 0, 5, 2, 9, 0, 3, 7, 0, 5, 10]),
]

def excerpt_windows(R, bars):
    # preferred 16-bar windows on 8-bar boundaries (choruses and finals of the standard form), else any 8-aligned window
    if bars == 120: return [24, 72, 96]
    return [b for b in range(8, bars - 24 + 1, 8)] or [0]

def build_suites(G, base_tracks, built):
    rnd = random.Random(1234); used = {}; out = []; base_tracks = list(base_tracks); built = list(built)
    for S in sorted(SUITES, key=lambda x: x['id']):
        target = int(S['mins'] * 60 * S['bpm'] / 60 * 4 / 16 + 0.5)      # bars of 16 rows at 15/bpm seconds each: bar = 4 beats
        cand = []
        for i, R in enumerate(base_tracks):
            sc, loop_bar, bpm, kt, mt = built[i]
            if R.get('form') in ('jingle', 'sting', 'ambient', 'boss', 'short'): continue
            if R.get('form') == 'suite' and not S.get('allsuites'): continue
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
        i = choose(); used[i] = used.get(i, 0) + 1; nb = built[i][0].cells // 16; pieces.append((i, max(0, nb - 16), 16, S['home'], 'end'))
        # the weave
        def ramp(C):    # time-stretch ramp for the ending: the music slows to three times its length
            st = [0.0]; acc = 0.0
            for c in range(C): acc += 1.0 + 2.0 * (float(c) / C) ** 1.5; st.append(acc)
            return st
        segs = []; total_cells = 0
        for pc in pieces:
            i, b0, ln, key = pc[:4]; kind = pc[4] if len(pc) > 4 else 'x'
            sc, lb, bpm, kt, mt = built[i]; f = S['bpm'] / bpm
            if kind == 'end': n = int(ramp(ln * 16)[-1] * f) + 12 * 16 * 3 // 2 + 16
            else: n = int(round(ln * 16 * f))
            segs.append((i, b0, ln, key, f, n, kt, kind)); total_cells += n
        out_sc = G.Sc((total_cells + 15) // 16 + 1); pos = 0; segno = 0
        KICK_, SNARE_, CRASH_, HATSOFT = 1, 2, 3, 6 | 0x80
        for (i, b0, ln, key, f, n, kt, kind) in segs:
            sc = built[i][0]; shift = ((key - kt + 6) % 12) - 6; c0 = b0 * 16; c1 = c0 + ln * 16; C = ln * 16
            st = ramp(C) if kind == 'end' else None
            def mp(c):   # source cell -> output offset
                return (st[c - c0] * f) if st else (c - c0) * f
            for vc in range(G.NV):
                cells = sc.v[vc]
                for c in range(c0, c1):
                    nt, l = cells[c]
                    if nt <= 0: continue
                    nc = pos + int(round(mp(c))); ne = pos + int(round(mp(min(c + l, c1)))); l2 = max(1, ne - nc)
                    if kind == 'end':
                        pr = float(c - c0) / C   # voices leave one by one as the ending draws out
                        if (vc in (9, 10) and pr > 0.2) or (vc == 8 and pr > 0.35) or (vc == 7 and pr > 0.3) or (vc == 3 and pr > 0.55) or (vc == 2 and pr > 0.5) or (vc == 1 and pr > 0.7) or (vc == 6 and pr > 0.8): continue
                        if vc == 0 and pr > 0.9: continue
                    if vc < 9:
                        nt += shift
                        while nt > G.MAXN: nt -= 12
                        while 0 < nt < 24: nt += 12
                    if nc < out_sc.cells: out_sc.v[vc][nc] = (nt, min(l2, out_sc.cells - nc))
            if kind == 'x' or kind == 'i':
                seg_end = pos + n
                # spice: a drum fill and crash at every join, sometimes a drum-less build, an octave-up echo, or the lead laying out for a while
                r = rnd.random()
                if kind == 'x':
                    if r < 0.28:
                        for c in range(pos, min(pos + 64, seg_end)): out_sc.v[10][c] = (0, 0); out_sc.v[9][c] = (0, 0)
                    elif r < 0.5 and not S.get('newlayer'):
                        for c in range(pos, seg_end):
                            nt, l = out_sc.v[0][c]
                            if nt > 0 and ((c - pos) // 64) % 2 == 1: out_sc.v[8][c] = (min(nt + 12, G.MAXN), max(1, l // 2))
                    elif r < 0.66:
                        for c in range(pos + 32, min(pos + 96, seg_end)): out_sc.v[0][c] = (0, 0)
                for k_, d_ in enumerate((SNARE_ | 0x80, SNARE_ | 0x80, SNARE_, SNARE_)):
                    if seg_end - 4 + k_ >= 0 and seg_end - 4 + k_ < out_sc.cells and segno + 1 < len(segs): out_sc.v[9][seg_end - 4 + k_] = (d_, 1)
                if segno + 1 < len(segs) and seg_end < out_sc.cells: out_sc.v[10][seg_end] = (CRASH_, 1)
            if S.get('newlayer') and kind != 'end':   # a brand new line of our own over every excerpt: a pentatonic motif that is reborn each time
                sc8 = out_sc.v[8]
                for c in range(pos, pos + n): sc8[c] = (0, 0)
                pent = [0, 2, 4, 7, 9]; root = 60 + key % 12; motif = [(rnd.choice(pent) + 12 * rnd.choice([0, 0, 1]), rnd.choice([2, 2, 3, 4])) for _ in range(5)]
                c = pos; j = 0
                while c < pos + n - 2:
                    d, l = motif[j % len(motif)]; nt = root + d
                    while nt > G.MAXN: nt -= 12
                    if rnd.random() < 0.78: sc8[c] = (nt, min(l, pos + n - c))
                    c += l * 2; j += 1
            if kind == 'end': out_sc.fade = pos + int(0.25 * n)   # the master volume falls away from here to the last row
            if kind == 'end':   # the tail: held chords, a few bell notes further and further apart, then silence
                t0 = pos + int(ramp(C)[-1] * f); R_ = 48 + S['home'] % 12; T = 12 * 16 * 3 // 2
                for vc, nt in ((4, R_ + 4 + 12), (5, R_ + 7 + 12), (6, R_)): out_sc.v[vc][t0] = (nt, min(T - 16, out_sc.cells - t0))
                gap = 8; c = t0 + 4
                for q in range(6):
                    if c < out_sc.cells: out_sc.v[0][c] = (min(R_ + 24 + (0, 7, 4, 12, 7, 0)[q], G.MAXN), 24)
                    c += gap; gap = gap * 2 - 2
            pos += n; segno += 1
        out_sc.cells = pos
        for vc in range(G.NV): out_sc.v[vc] = out_sc.v[vc][:pos]
        mins = pos * 15.0 / S['bpm'] / 60
        base_tracks.append(dict(name=S['name'], mood=S['mood'], form='suite')); built.append((out_sc, 0, S['bpm'], S['home'], 'maj'))
        out.append((S, out_sc, [(base_tracks[i]['name'], b0, ln) for (i, b0, ln, key, f, n, kt, kind) in segs], mins))
    return out
