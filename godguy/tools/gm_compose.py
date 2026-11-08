# Bore: GodGuy's music, amalgamated from the visible songs of BORE (the GBA game). gm_analyze.py pulls each song's tempo, key, mode and repeated hooks
# out of the .xm files; this script re-keys, re-moodes and re-times those hooks and builds 38 tracks (four square voices, a bass, a pad and noise drums)
# plus 28 sound-effect motifs cut from the same hooks. Output: source/gmdata.h (a table; the DS player is source/gmplay.h).
import json, os, math, random, sys
HERE = os.path.dirname(__file__)
A = json.load(open(os.path.join(HERE, 'gm_analysis.json')))
MAJ = [0, 2, 4, 5, 7, 9, 11]; MIN = [0, 2, 3, 5, 7, 8, 10]
NOTE = 'C C# D D# E F F# G G# A A# B'.split()

def uniq_hooks(song, kind='mel'):
    seen = set(); out = []
    for h in sorted(A[song]['hooks'], key=lambda h: -h['score']):
        if h['kind'] != kind: continue
        rel = [r for g, r in h['notes']]
        if max(rel) - min(rel) > 31 or len(set(rel)) < 3: continue
        sig = tuple((g, r) for g, r in h['notes'][:10])
        if sig in seen: continue
        seen.add(sig); out.append(h)
    return out
def get_hook(song, rank=0):
    hs = uniq_hooks(song)
    if not hs: hs = uniq_hooks(song, 'bass')
    return hs[rank % len(hs)]
def remap(p, ks, ms, kt, mt):
    sc_s = MAJ if ms == 'maj' else MIN; sc_t = MAJ if mt == 'maj' else MIN
    r = p - ks; o, d = divmod(r, 12)
    if d in sc_s: d2 = sc_t[sc_s.index(d)]
    else:
        # a chromatic note: keep its distance above the scale degree below it
        below = max(x for x in sc_s if x < d); i = sc_s.index(below); d2 = sc_t[i] + (d - below)
    return kt + o * 12 + d2
def hook_notes(song, rank, kt, mt, lo=62, hi=80):
    """-> (length in cells, [(cell, midi)]) re-keyed to the target key and mode, the octave set so it sits between lo and hi."""
    h = get_hook(song, rank); a = A[song]
    ns = [(g, remap(h['base'] + r, a['key'], a['mode'], kt, mt)) for g, r in h['notes']]
    med = sorted(p for g, p in ns)[len(ns) // 2]
    while med < (lo + hi) // 2 - 6: ns = [(g, p + 12) for g, p in ns]; med += 12
    while med > (lo + hi) // 2 + 6: ns = [(g, p - 12) for g, p in ns]; med -= 12
    return h['len'], ns

class Score:
    def __init__(self, bars): self.cells = bars * 16; self.v = [[(0, 0)] * self.cells for _ in range(5)]   # (note, len) at onset cells; drums use note = drum id
    def put(self, vc, cell, note, ln):
        if 0 <= cell < self.cells: self.v[vc][cell] = (note, max(1, min(ln, self.cells - cell)))
    def clear(self, vc, c0, c1):
        for c in range(c0, min(c1, self.cells)): self.v[vc][c] = (0, 0)

def place_hook(sc, vc, L, ns, start, bars, tr='none', octs=0, maxlen=8, step=1):
    cells = bars * 16; reps = (cells + L - 1) // L
    notes = list(ns)
    if tr == 'inv': base = notes[0][1]; notes = [(g, 2 * base - p) for g, p in notes]
    if tr == 'rev': notes = [(L - 1 - g, p) for g, p in reversed(notes)]; notes = sorted(notes)
    if tr == 'half': notes = [(g * 2, p) for g, p in notes]; L *= 2; reps = (cells + L - 1) // L
    if tr == 'stac': maxlen = 1
    for r in range(reps):
        for i, (g, p) in enumerate(notes):
            c = r * L + g
            if c >= cells: continue
            nxt = (notes[i + 1][0] if i + 1 < len(notes) else L) - g
            sc.put(vc, start + c, max(24, min(100, p + 12 * octs)), min(max(1, nxt), maxlen) * step)

def chord_for(ns, c0, c1, kt, mt):
    sc = MAJ if mt == 'maj' else MIN; w = [0] * 12
    for g, p in ns:
        if c0 <= g < c1: w[p % 12] += 1 + (1 if g == c0 else 0)
    best = (-1, kt)
    for deg in range(7):
        r = (kt + sc[deg]) % 12; tri = [(r + sc[(deg + k) % 7] - sc[deg]) % 12 for k in (0, 2, 4)]
        s = sum(w[t] * (2 if t == r else 1) for t in tri) + (0.3 if deg in (0, 3, 4) else 0)
        if s > best[0]: best = (s, r)
    return best[1]

def fold(p, lo, hi):
    while p < lo: p += 12
    while p > hi: p -= 12
    return p

def bass_bar(sc, start, root, style, kt, mt, hard=False):
    r = fold(root, 28, 39); f = r + 7; o = r + 12
    pat = {
     'root': [(0, r, 14)], 'sub': [(0, r, 16)],
     'pump': [(c, r if c % 4 != 2 else o, 2) for c in range(0, 16, 2)],
     'five': [(0, r, 4), (4, f, 4), (8, r, 4), (12, o, 4)],
     'walk': [(0, r, 4), (4, r + 2 if mt == 'maj' else r + 3, 4), (8, f, 4), (12, r + 5, 2), (14, r + 4 if mt == 'maj' else r + 3, 2)],
     'offbeat': [(c, r, 2) for c in (2, 6, 10, 14)],
     'sync': [(0, r, 3), (3, r, 2), (6, f, 2), (10, r, 3), (14, o, 2)],
     'gallop': [(c, r, 1) for c in range(16) if c % 4 != 3],
     'drone': [(0, r, 16)],
     'waltz': [(0, r, 4), (4, f, 2), (8, f, 2), (12, r, 4)],
    }[style]
    for c, n, ln in pat: sc.put(2, start + c, n, ln)

def pad_bar(sc, start, root, style, kt, mt):
    r = fold(root, 48, 59); third = 3 if mt == 'min' else 4
    if style == 'pad': sc.put(3, start, r + 7 if (start // 16) % 2 else r + third, 16)
    elif style == 'stab':
        for c in (3, 7, 11): sc.put(3, start + c, r + third + 12, 2)
    elif style == 'arp':
        seq = [r, r + third, r + 7, r + 12, r + 7, r + third]
        for c in range(16): sc.put(3, start + c, seq[c % 6] + 12, 1)
    elif style == 'tick':
        for c in range(0, 16, 4): sc.put(3, start + c, r + 7 + 12, 1)
    elif style == 'sparkle':
        for c in (0, 5, 10): sc.put(3, start + c, r + (0, third, 7)[c // 5] + 24, 2)
    # 'none' puts nothing

KICK, SNARE, HAT, OHAT, TOM, CRASH = 1, 2, 3, 4, 5, 6
def drum_bar(sc, start, style, fill=False, crash=False):
    d = {
     'house': {0: KICK, 4: KICK, 8: KICK, 12: KICK, 2: OHAT, 6: OHAT, 10: OHAT, 14: OHAT, 4.5: 0},
     'dnb': {0: KICK, 10: KICK, 4: SNARE, 12: SNARE, 2: HAT, 6: HAT, 8: HAT, 14: HAT},
     'half': {0: KICK, 8: SNARE, 4: HAT, 12: HAT, 14: HAT, 10: KICK},
     'shuffle': {0: KICK, 8: SNARE, 3: HAT, 6: HAT, 9: HAT, 12: KICK, 14: HAT},
     'break': {0: KICK, 3: KICK, 4: SNARE, 7: HAT, 10: KICK, 12: SNARE, 14: HAT, 15: SNARE},
     'march': {0: KICK, 4: SNARE, 8: KICK, 12: SNARE, 6: TOM, 14: TOM},
     'rock': {0: KICK, 2: HAT, 4: SNARE, 6: HAT, 8: KICK, 10: KICK, 12: SNARE, 14: HAT},
     'dance': {0: KICK, 2: HAT, 4: KICK, 6: HAT, 8: KICK, 10: HAT, 12: KICK, 14: HAT, 7: SNARE},
     'latin': {0: KICK, 3: TOM, 6: HAT, 8: KICK, 10: TOM, 12: SNARE, 14: HAT},
     'tribal': {0: TOM, 3: TOM, 6: KICK, 8: TOM, 11: KICK, 14: TOM},
     'soft': {0: HAT, 8: HAT, 12: HAT},
     'heart': {0: KICK, 3: KICK, 8: 0},
     'none': {},
     'waltz': {0: KICK, 4: HAT, 8: HAT},
     'tick': {c: HAT for c in range(0, 16, 4)},
    }[style]
    for c, n in d.items():
        if n and c == int(c): sc.put(4, start + int(c), n, 1)
    if fill and style not in ('none', 'soft'):
        for c in (12, 13, 14, 15): sc.put(4, start + c, SNARE if c > 12 else TOM, 1)
    if crash: sc.put(4, start, CRASH, 1)

def build(R):
    """R: recipe dict -> (score, loop bar, bpm)"""
    a = A[R['a']]; kt = R.get('key'); mt = R.get('mode') or a['mode']
    if kt is None: kt = a['key']
    bpm = R.get('bpm') or a['bpm']; bpm = max(60, min(190, bpm))
    La, nsa = hook_notes(R['a'], R.get('ra', 0), kt, mt)
    Lb, nsb = hook_notes(R['b'], R.get('rb', 0), kt, mt, 58, 76) if R.get('b') else (La, nsa)
    Lc, nsc = hook_notes(R['c'], R.get('rc', 0), kt, mt, 50, 70) if R.get('c') else (Lb, nsb)
    form = R.get('form', 'song'); bs, ps, ds = R.get('bass', 'root'), R.get('pad', 'pad'), R.get('drums', 'half')
    forms = {
        'song':    [('intro', 4), ('A', 8), ('B', 8), ('C', 8), ('out', 4)],
        'short':   [('A', 4), ('B', 4), ('C', 4), ('A', 4)],
        'ambient': [('intro', 4), ('A', 6), ('B', 6), ('out', 4)],
        'boss':    [('A', 8), ('B', 8), ('C', 8)],
        'jingle':  [('J', 4)],
        'sting':   [('J', 2)],
        'long':    [('intro', 4), ('A', 8), ('B', 8), ('A', 8), ('C', 8), ('B', 8), ('out', 4)],
    }[form]
    bars = sum(n for _, n in forms); sc = Score(bars); bar = 0; loop_bar = 0
    for si, (sec, n) in enumerate(forms):
        s0 = bar * 16
        hook_lead = {'A': (La, nsa), 'B': (Lb, nsb), 'C': (La, nsa), 'intro': (La, nsa), 'out': (La, nsa), 'J': (La, nsa)}[sec]
        L, ns = hook_lead
        # lead
        if sec == 'intro':
            place_hook(sc, 0, L, ns[:max(3, len(ns) // 3)], s0 + 16 * (n - 2), 2, octs=0)
        elif sec == 'out':
            place_hook(sc, 0, L, ns[:max(3, len(ns) // 2)], s0, 2, octs=0, maxlen=12)
        elif sec == 'C':
            place_hook(sc, 0, Lb, nsb, s0, n, octs=R.get('cup', 0))
        elif sec == 'J':
            place_hook(sc, 0, L, ns[:R.get('jn', 6)], s0, 1, maxlen=3)
            root = (kt) % 12; fin = fold(root + 12 * 6, 64, 76)
            sc.put(0, s0 + 16 * (n - 1) if n > 1 else s0 + 12, fin, 16 if n > 1 else 4)
        else:
            place_hook(sc, 0, L, ns, s0, n, tr=('inv' if (R.get('inv') and si > 2) else 'none'), maxlen=R.get('maxlen', 8))
        # counter voice (arp or the other hook an octave down)
        if sec in ('B', 'C'):
            Lq, nq = (Lc, nsc) if sec == 'C' else (La, nsa)
            place_hook(sc, 1, Lq, nq, s0, n, octs=-1 if sec == 'B' else 0, maxlen=R.get('cmax', 2), tr='half' if R.get('chalf') and sec == 'B' else 'none')
        elif sec == 'A' and R.get('counterA'):
            place_hook(sc, 1, Lc, nsc, s0 + 16 * (n // 2), n - n // 2, octs=0, maxlen=2)
        # bass, pad, drums bar by bar
        use = {'A': nsa, 'B': nsb, 'C': nsa, 'intro': nsa, 'out': nsa, 'J': nsa}[sec]
        Lu = {'A': La, 'B': Lb, 'C': La, 'intro': La, 'out': La, 'J': La}[sec]
        for b in range(n):
            c0 = (b * 16) % Lu; root = chord_for(use, c0, c0 + 16, kt, mt)
            if sec in ('J',) or R.get('bassfirst') or sec != 'intro' or b >= 1:
                bass_bar(sc, s0 + b * 16, root, 'sub' if sec in ('out',) or (sec == 'intro') else bs, kt, mt)
            if ps != 'none': pad_bar(sc, s0 + b * 16, root, 'pad' if sec in ('intro', 'out') else ps, kt, mt)
            if sec in ('A', 'B', 'C') or (sec == 'J' and form != 'sting'):
                last = (b == n - 1 and sec != 'J')
                drum_bar(sc, s0 + b * 16, ds, fill=last and sec != 'C', crash=(b == 0 and sec in ('B', 'C')))
            elif sec == 'intro' and b >= n - 1 and ds not in ('none',): drum_bar(sc, s0 + b * 16, 'soft')
        bar += n
        if sec == 'A' and loop_bar == 0 and si == 0 and form in ('short', 'boss'): loop_bar = 0
    if form in ('song', 'long', 'ambient'): loop_bar = 4
    return sc, loop_bar, bpm, kt, mt

def pack(sc):
    out = []
    for vc in range(5):
        ev = []; c = 0; cells = sc.cells
        while c < cells:
            n, ln = sc.v[vc][c]
            if n == 0 and ln == 0:
                e = c + 1
                while e < cells and sc.v[vc][e] == (0, 0): e += 1
                ev.append((0, e - c)); c = e
            else:
                ln = min(ln, 255)
                # the note rings for ln cells; a gap up to the next onset is a rest
                e = c + 1
                while e < cells and sc.v[vc][e] == (0, 0) and e - c < ln: e += 1
                ev.append((n, e - c)); c = e
        # split long rests
        fixed = []
        for n, ln in ev:
            while ln > 255: fixed.append((n, 255)); ln -= 255; n = n if n == 0 else n
            fixed.append((n, ln))
        out.append(fixed)
    return out

# ---------------------------------------------------------------------------------------- the 38 tracks: (id, name, where it plays, recipe)
T = lambda **k: k
TRACKS = [
 T(id='title', name='GODGUY DAWN', use='title screen', a='sunman_sunrise', ra=0, b='earth_and_the_space_citizens', rb=0, c='tree_swaying_action', rc=0, key=2, mode='maj', bpm=84, form='song', bass='sub', pad='pad', drums='heart', cup=0),
 T(id='chapters', name='THE SEVEN DAYS', use='story chapter select', a='magic_act', ra=0, b='nursery_time', rb=1, c='amiga_music', rc=0, key=0, mode='maj', bpm=100, form='short', bass='walk', pad='sparkle', drums='tick'),
 T(id='newworld', name='LET THERE BE LAND', use='making a new planet', a='tree_swaying_action', ra=1, b='excuses_house', rb=0, key=5, mode='maj', bpm=70, form='ambient', bass='drone', pad='pad', drums='none'),
 T(id='sphere', name='ORBIT', use='sphere world', a='earth_and_the_space_citizens', ra=1, b='mi_cora_zone', rb=2, c='sunman_sunrise', rc=2, key=7, mode='maj', bpm=96, form='song', bass='five', pad='arp', drums='soft'),
 T(id='cube', name='CORNERS', use='cube world', a='amiga_music', ra=1, b='hotdamn_rave', rb=0, c='condensed_music', rc=1, key=9, mode='min', bpm=124, form='song', bass='sync', pad='tick', drums='break'),
 T(id='ring', name='THE LOOP', use='ring world', a='cynicaller_dnb', ra=1, b='aim_and_shoot', rb=0, c='cynicaller_dnb', rc=3, key=11, mode='min', bpm=132, form='song', bass='gallop', pad='arp', drums='dnb'),
 T(id='morning', name='MORNING MEADOW', use='daybreak', a='sunman_sunrise', ra=3, b='worthless_clouds', rb=2, key=7, mode='maj', bpm=92, form='song', bass='five', pad='pad', drums='soft'),
 T(id='noon', name='HIGH NOON', use='midday', a='mi_cora_zone', ra=1, b='condensed_music', rb=2, c='tree_fast', rc=1, key=0, mode='maj', bpm=118, form='song', bass='pump', pad='stab', drums='dance'),
 T(id='sunset', name='LONG SHADOWS', use='evening', a='excuses_house', ra=1, b='spanish_flexicode', rb=1, key=9, mode='min', bpm=88, form='song', bass='walk', pad='pad', drums='shuffle'),
 T(id='night', name='STARLIT FIELDS', use='night', a='meltdown_in_mars_house', ra=2, b='worthless_clouds', rb=4, key=4, mode='min', bpm=76, form='ambient', bass='sub', pad='sparkle', drums='soft'),
 T(id='storm', name='THUNDERHEAD', use='storm weather', a='quickbullet', ra=0, b='cocaine_cola_ii', rb=0, c='closer_to_the_end', rc=1, key=2, mode='min', bpm=136, form='song', bass='gallop', pad='tick', drums='rock'),
 T(id='desert', name='SEVEN DUNES', use='desert', a='aim_and_shoot', ra=1, b='spanish_flexicode', rb=2, key=2, mode='min', bpm=104, form='song', bass='waltz', pad='sparkle', drums='latin'),
 T(id='snow', name='WHITE SILENCE', use='snow', a='nursery_time', ra=2, b='tree_swaying_action', rb=2, key=4, mode='min', bpm=72, form='ambient', bass='drone', pad='sparkle', drums='none'),
 T(id='ocean', name='THE DEEP BLUE', use='ocean', a='excuses_house', ra=0, b='earth_and_the_space_citizens', rb=2, key=3, mode='maj', bpm=82, form='ambient', bass='sub', pad='pad', drums='heart'),
 T(id='jungle', name='GREEN CANOPY', use='jungle', a='whistler_shuffle', ra=1, b='tree_fast', rb=0, key=7, mode='maj', bpm=112, form='song', bass='walk', pad='stab', drums='tribal'),
 T(id='swamp', name='BOG LIGHTS', use='swamp', a='meltdown_in_mars_house', ra=0, b='over_my_voice', rb=3, key=1, mode='min', bpm=84, form='song', bass='sub', pad='sparkle', drums='heart'),
 T(id='volcano', name='FIRE MOUNTAIN', use='lava lands', a='closer_to_the_end', ra=0, b='quickbullet', rb=1, c='ticking_bomb' if False else 'the_ticking_bomb', rc=1, key=4, mode='min', bpm=128, form='boss', bass='gallop', pad='stab', drums='march'),
 T(id='village', name='FIRST HEARTH', use='a village is founded', a='nursery_time', ra=0, b='spanish_flexicode', rb=0, key=7, mode='maj', bpm=104, form='song', bass='walk', pad='arp', drums='waltz'),
 T(id='market', name='MARKET DAY', use='thriving villages', a='whistler_shuffle', ra=0, b='gottcho_barracho_ii', rb=0, c='emergency_hitech', rc=2, key=5, mode='maj', bpm=116, form='song', bass='walk', pad='stab', drums='shuffle'),
 T(id='city', name='THE GREAT CITY', use='level 3 halls', a='staged', ra=0, b='mi_cora_zone', rb=0, c='emergency_hitech', rc=0, key=2, mode='maj', bpm=126, form='long', bass='pump', pad='arp', drums='dance'),
 T(id='harvest', name='GOLDEN HARVEST', use='farms and plenty', a='spanish_flexicode', ra=1, b='sunman_sunrise', rb=1, key=0, mode='maj', bpm=98, form='song', bass='waltz', pad='pad', drums='latin'),
 T(id='festival', name='LANTERN FESTIVAL', use='celebration', a='ohio_hoe', ra=1, b='hotdamn_rave', rb=1, c='mi_cora_zone', rc=3, key=9, mode='maj', bpm=140, form='song', bass='pump', pad='stab', drums='dance'),
 T(id='war', name='WAR DRUMS', use='wars between peoples', a='the_ticking_bomb', ra=0, b='closer_to_the_end', rb=2, key=6, mode='min', bpm=122, form='boss', bass='gallop', pad='tick', drums='march', inv=True),
 T(id='siege', name='THE SIEGE', use='a village burns', a='cynicaller_dnb', ra=0, b='staged', rb=1, c='the_ticking_bomb', rc=2, key=7, mode='min', bpm=150, form='boss', bass='sync', pad='stab', drums='dnb'),
 T(id='monster', name='SOMETHING STIRS', use='first monsters', a='quickbullet', ra=1, b='meltdown_in_mars_house', rb=3, key=8, mode='min', bpm=92, form='ambient', bass='sub', pad='sparkle', drums='heart'),
 T(id='bestiary', name='THE BESTIARY', use='bestiary screen', a='magic_act', ra=2, b='amiga_music', rb=2, key=2, mode='min', bpm=90, form='short', bass='walk', pad='sparkle', drums='tick'),
 T(id='dragon', name='ELDER DRAGON', use='boss: dragon', a='closer_to_the_end', ra=2, b='the_ticking_bomb', rb=3, c='over_my_voice', rc=0, key=3, mode='min', bpm=144, form='boss', bass='gallop', pad='stab', drums='rock', cup=1),
 T(id='demon', name='THE DEMON LORD', use='boss: demon', a='cocaine_cola_ii', ra=1, b='hotdamn_rave', rb=2, key=5, mode='min', bpm=138, form='boss', bass='pump', pad='arp', drums='break', inv=True),
 T(id='kraken', name='KRAKEN DEEP', use='boss: kraken', a='cynicaller_dnb', ra=2, b='excuses_house', rb=2, key=11, mode='min', bpm=110, form='boss', bass='sync', pad='pad', drums='half'),
 T(id='lich', name='THE LICH KING', use='boss: lich', a='meltdown_in_mars_house', ra=1, b='magic_act', rb=1, c='quickbullet', rc=2, key=0, mode='min', bpm=100, form='boss', bass='walk', pad='arp', drums='heart'),
 T(id='ascend', name='ASCENSION', use='chapter complete', a='staged', ra=2, b='sunman_sunrise', rb=4, key=7, mode='maj', bpm=112, form='jingle', bass='five', pad='pad', drums='tick', jn=8),
 T(id='unlock', name='GIFTS OF THE GODS', use='items unlocked', a='mi_cora_zone', ra=2, b='ohio_hoe', rb=0, key=0, mode='maj', bpm=126, form='jingle', bass='pump', pad='sparkle', drums='tick', jn=8),
 T(id='godhood', name='GODHOOD', use='the finale', a='emergency_hitech', ra=1, b='tree_fast', rb=2, c='earth_and_the_space_citizens', rc=3, key=4, mode='maj', bpm=132, form='long', bass='pump', pad='arp', drums='dance', cup=1),
 T(id='credits', name='THE LONG WAY HOME', use='credits', a='worthless_clouds', ra=0, b='sunman_sunrise', rb=5, c='nursery_time', rc=3, key=2, mode='maj', bpm=88, form='long', bass='five', pad='pad', drums='soft'),
 T(id='freeplay', name='SANDBOX OF THE GODS', use='free play', a='amiga_music', ra=3, b='tree_fast', rb=3, c='sunman_sunrise', rc=6, key=7, mode='maj', bpm=108, form='song', bass='walk', pad='stab', drums='shuffle'),
 T(id='gameover', name='THE LAST SUNSET', use='the world ends', a='meltdown_in_mars_house', ra=4, b='worthless_clouds', rb=5, key=1, mode='min', bpm=64, form='jingle', bass='sub', pad='pad', drums='none', jn=7),
 T(id='timelapse', name='A THOUSAND YEARS', use='fast forward', a='condensed_music', ra=0, b='hotdamn_rave', rb=3, key=8, mode='min', bpm=160, form='short', bass='gallop', pad='arp', drums='dance'),
 T(id='pause', name='THE QUIET HOUR', use='paused', a='worthless_clouds', ra=3, b='excuses_house', rb=3, key=5, mode='maj', bpm=68, form='ambient', bass='drone', pad='pad', drums='none'),
]
assert len(TRACKS) == 38, len(TRACKS)

# ---------------------------------------------------------------------------------------- sound effects: (name, hook song, rank, how)
def frag(song, rank, n, key, mode, lo=64, hi=84, order=None):
    L, ns = hook_notes(song, rank, key, mode, lo, hi); ps = [p for g, p in ns]
    seen = []; [seen.append(p) for p in ps if p not in seen]
    if order == 'up': seen = sorted(seen)
    if order == 'down': seen = sorted(seen, reverse=True)
    k = 0
    while len(seen) < n and seen:      # a short hook: climb on through its own notes an octave up (or down)
        seen.append(seen[k] + (-12 if order == 'down' else 12)); k += 1
    return seen[:n]
# event: (note, frames, volume 0..127, kind 0..3 = duty 12/25/50/75, 4 = noise, slide: semitones*8 per ~frame as s8)
def ev(n, f, v=100, k=2, sl=0): return (int(n), int(f), int(v), int(k), int(sl))
SFX = []
def sfx(name, evs, src): SFX.append((name, evs, src))
def seq(notes, f, v=100, k=2, gap=0):
    out = []
    for n in notes:
        out.append(ev(n, f, v, k))
        if gap: out.append(ev(0, gap, 0, k))
    return out
sfx('MENU MOVE', seq(frag('the_ticking_bomb', 0, 1, 8, 'maj', 70, 80), 2, 70), 'THE TICKING BOMB')
sfx('MENU OK', seq(frag('mi_cora_zone', 0, 2, 7, 'min', 66, 84, 'up'), 3, 90), 'MI CORA ZONE')
sfx('MENU BACK', seq(frag('mi_cora_zone', 0, 2, 7, 'min', 66, 84, 'down'), 3, 80), 'MI CORA ZONE')
sfx('PAINT', [ev(0, 1, 0, 4)] + [ev(48, 2, 50, 4)] + seq(frag('amiga_music', 0, 1, 0, 'maj', 60, 72), 2, 60, 1), 'AMIGA MUSIC')
sfx('SPAWN', seq(frag('magic_act', 0, 4, 5, 'maj', 66, 90, 'up'), 3, 90, 1), 'THE MAGIC ACT')
sfx('RAIN', [ev(100, 12, 40, 4)] + seq(frag('nursery_time', 0, 4, 0, 'maj', 78, 96), 3, 60, 0, 2), 'NURSERY TIME')
sfx('FIRE', [ev(90, 14, 90, 4, 0)] + [ev(frag('closer_to_the_end', 0, 1, 3, 'min', 52, 64)[0], 14, 70, 2, -6)], 'CLOSER TO THE END')
sfx('LIGHTNING', [ev(110, 3, 127, 4)] + [ev(frag('quickbullet', 0, 1, 8, 'min', 90, 100)[0], 10, 110, 3, -40)] + [ev(60, 6, 70, 4)], 'MISTER QUICKBULLET')
sfx('METEOR', [ev(frag('closer_to_the_end', 2, 1, 3, 'min', 80, 92)[0], 30, 100, 2, -30), ev(70, 14, 127, 4)], 'CLOSER TO THE END')
sfx('SMITE', seq([66, 63, 66], 5, 120, 2) + [ev(80, 8, 127, 4)], 'THE TICKING BOMB')
sfx('HEAL', seq(frag('sunman_sunrise', 0, 4, 2, 'maj', 70, 88, 'up'), 4, 90, 1), 'SUNMAN SUNRISE')
sfx('FEED', seq(frag('spanish_flexicode', 0, 3, 5, 'maj', 68, 86, 'up'), 3, 85, 2), 'AN ODE TO THE SPANISH FLEXICODE')
sfx('SETTLE', seq(frag('staged', 0, 5, 2, 'maj', 62, 86, 'up'), 4, 100, 2), 'STAGED THE FULL PERFORMANCE')
sfx('WAR', [ev(40, 5, 110, 4), ev(40, 5, 110, 4), ev(40, 5, 120, 4)] + seq(frag('closer_to_the_end', 1, 2, 6, 'min', 48, 60), 8, 100, 3), 'CLOSER TO THE END')
sfx('PEACE', seq(frag('worthless_clouds', 0, 4, 0, 'maj', 66, 86, 'down'), 5, 80, 2), 'WORTHLESS CLOUDS')
sfx('BLESS', seq(frag('earth_and_the_space_citizens', 0, 6, 0, 'maj', 72, 96, 'up'), 3, 75, 1), 'EARTH AND THE SPACE CITIZENS')
sfx('GIFT', seq(frag('whistler_shuffle', 0, 4, 7, 'maj', 66, 90, 'up'), 3, 90, 2), 'WHISTLER MAN')
sfx('INSPECT', seq(frag('condensed_music', 0, 2, 5, 'min', 76, 90, 'up'), 2, 70, 1), 'CONDENSED MUSIC')
sfx('BIRTH', seq(frag('tree_fast', 0, 3, 4, 'maj', 78, 96, 'up'), 3, 80, 0), 'TREE-AGE IN ACTION (FAST)')
sfx('MONSTER SEEN', seq([58, 64, 58], 7, 100, 2) + [ev(46, 12, 90, 3, -2)], 'MELTDOWN IN MARS')
sfx('MONSTER SLAIN', [ev(70, 4, 110, 4)] + seq(frag('over_my_voice', 0, 3, 9, 'maj', 66, 90, 'up'), 3, 95, 2), 'OVER MY VOICE')
sfx('VILLAGE FOUNDED', seq(frag('emergency_hitech', 0, 5, 7, 'min', 62, 86, 'up'), 4, 95, 2), 'EMERGENCY ON THE DANCE FLOOR')
sfx('VILLAGE FALLS', seq(frag('cocaine_cola_ii', 0, 4, 0, 'min', 52, 72, 'down'), 6, 90, 3) + [ev(60, 14, 100, 4)], 'COCAINE COLA')
sfx('CHAPTER DONE', seq(frag('mi_cora_zone', 2, 8, 0, 'maj', 64, 92, 'up'), 5, 100, 2), 'MI CORA ZONE')
sfx('UNLOCK', seq(frag('ohio_hoe', 0, 6, 0, 'maj', 72, 98, 'up'), 3, 90, 1, 1), 'OHIO HOE')
sfx('SAVE', seq(frag('hotdamn_rave', 0, 2, 0, 'maj', 70, 86, 'up'), 3, 80, 2), 'HOT DAMN')
sfx('ERROR', [ev(34, 6, 100, 3, 0), ev(31, 8, 100, 3, 0)], 'GODGUY')
sfx('STEP', [ev(70, 1, 35, 4)], 'GODGUY')
sfx('YEAR', seq(frag('sunman_sunrise', 2, 3, 2, 'maj', 70, 84), 4, 70, 2), 'SUNMAN SUNRISE')
assert len(SFX) >= 28, len(SFX)

def write_header(path):
    L = ['// gmdata.h - GENERATED by godguy/tools/gm_compose.py. Do not edit. 38 tracks and %d sound effects, amalgamated from the visible songs of BORE.' % len(SFX), '']
    L.append('typedef struct { const char*name, *use; unsigned char bpm; unsigned char duty[4], vol[4]; unsigned short rows, loopRow; unsigned short n[5]; const unsigned char*v[5]; } GmSong;')
    L.append('typedef struct { const char*name, *src; unsigned char n; const unsigned char*ev; } GmFx;')
    total = 0; names = []
    meta = []
    for i, R in enumerate(TRACKS):
        sc, loop_bar, bpm, kt, mt = build(R); pk = pack(sc)
        for vc in range(5):
            b = []; [b.extend([n, ln]) for n, ln in pk[vc]]
            L.append('static const unsigned char gm%d_%d[]={%s};' % (i, vc, ','.join(map(str, b)))); total += len(b)
        duty = R.get('duty') or [2, 0, 1, 0]; vol = R.get("vol") or [31, 18, 29, 12]
        if R.get('form') in ('ambient',): vol = [28, 15, 28, 14]
        meta.append('{"%s","%s",%d,{%s},{%s},%d,%d,{%s},{%s}}' % (R['name'], R['use'], bpm, ','.join(map(str, duty)), ','.join(map(str, vol)), sc.cells, (65535 if R.get('form') in ('jingle', 'sting') else loop_bar * 16),
                    ','.join('%d' % (len(pack(sc)[v]) * 2) for v in range(5)), ','.join('gm%d_%d' % (i, v) for v in range(5))))
    L.append('static const GmSong gmSongs[%d]={' % len(TRACKS)); L.append(',\n'.join(meta)); L.append('};')
    L.append('#define GM_SONGS %d' % len(TRACKS))
    for i, (nm, evs, src) in enumerate(SFX):
        b = []; [b.extend([e[0], e[1], e[2], e[3], e[4] & 255]) for e in evs]
        L.append('static const unsigned char gmfx%d[]={%s};' % (i, ','.join(map(str, b)))); total += len(b)
    L.append('static const GmFx gmFx[%d]={' % len(SFX)); L.append(',\n'.join('{"%s","%s",%d,gmfx%d}' % (nm, src, len(evs), i) for i, (nm, evs, src) in enumerate(SFX))); L.append('};')
    L.append('#define GM_FX %d' % len(SFX))
    for i, (nm, evs, src) in enumerate(SFX): L.append('#define FX_%s %d' % (nm.replace(' ', '_'), i))
    open(path, 'w').write('\n'.join(L) + '\n')
    return total

if __name__ == '__main__':
    tot = write_header(os.path.join(HERE, '..', 'source', 'gmdata.h'))
    print('wrote %d bytes of music data, %d tracks, %d effects' % (tot, len(TRACKS), len(SFX)))
    for R in TRACKS:
        sc, lb, bpm, kt, mt = build(R); secs = sc.cells * 15.0 / bpm
        print('%-20s %-22s %3d bpm %s %-3s %2d bars %3.0fs  from %s' % (R['name'], R['use'], bpm, NOTE[kt % 12], mt, sc.cells // 16, secs, '+'.join(x for x in (R['a'], R.get('b'), R.get('c')) if x)))
