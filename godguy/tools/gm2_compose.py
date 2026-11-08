# GodGuy's music, second engine: eleven streams per song (lead, ping-pong echo, counter, harmony, two detuned pads, bass, arpeggio, sparkle, two drum parts),
# long song forms (intro, verse, pre-chorus, chorus, breakdown, second verse, key-lift chorus, bridge in the opposite mode, final chorus, outro)
# and 70 tracks amalgamated from the hooks of BORE's songs (gm_analyze.py reads them). Output: source/gmdata.h.
import os, sys, math
sys.path.insert(0, os.path.dirname(__file__))
import gm_compose as G
from gm_compose import A, MAJ, MIN, hook_notes, chord_for, fold, NOTE, SFX, T, TRACKS as OLD
HERE = os.path.dirname(__file__)
V_LEAD, V_ECHO, V_COUNT, V_HARM, V_PADL, V_PADR, V_BASS, V_ARP, V_SPARK, V_DA, V_DB = range(11)
NV = 11
MAXN = 83          # the highest note the 64-sample waves can play (B5)

class Sc:
    def __init__(s, bars): s.cells = bars * 16; s.v = [[(0, 0)] * s.cells for _ in range(NV)]
    def put(s, vc, c, n, ln):
        if 0 <= c < s.cells:
            if vc < 9:
                n = int(n)
                while n > MAXN: n -= 12
                while 0 < n < 24: n += 12
            s.v[vc][c] = (n, max(1, min(ln, s.cells - c)))

# ------------------------------------------------------------------------------------------------ drums
KICK, SNARE, CLAP, TLO, TMID, THI, RIM = 1, 2, 3, 4, 5, 6, 7        # part A
HAT, OHAT, CRASH, RIDE, SHAKE, HATS = 1, 2, 3, 4, 5, 6             # part B
SOFT = 0x80
def S(d): return d
DR = {
 'house':  dict(A={0: KICK, 4: KICK, 8: KICK, 12: KICK, 4.1: 0}, A2={4: CLAP, 12: CLAP}, B={2: OHAT, 6: OHAT, 10: OHAT, 14: OHAT, 1: HATS | SOFT, 3: HATS | SOFT, 5: HATS | SOFT, 7: HATS | SOFT, 9: HATS | SOFT, 11: HATS | SOFT, 13: HATS | SOFT, 15: HATS | SOFT}),
 'dnb':    dict(A={0: KICK, 10: KICK, 4: SNARE, 12: SNARE, 7: SNARE | SOFT, 15: SNARE | SOFT}, B={c: HAT for c in range(0, 16, 2)}),
 'half':   dict(A={0: KICK, 10: KICK, 8: SNARE}, B={c: HAT for c in range(0, 16, 2)} | {14: OHAT}),
 'shuffle': dict(A={0: KICK, 12: KICK, 8: SNARE, 5: SNARE | SOFT}, B={0: HAT, 3: HAT | SOFT, 6: HAT, 9: HAT | SOFT, 12: HAT, 15: HAT | SOFT}),
 'break':  dict(A={0: KICK, 3: KICK, 10: KICK, 4: SNARE, 12: SNARE, 7: SNARE | SOFT, 9: SNARE | SOFT, 15: SNARE | SOFT}, B={c: HAT for c in range(0, 16, 2)} | {11: HAT | SOFT}),
 'march':  dict(A={0: KICK, 8: KICK, 4: SNARE, 12: SNARE, 6: TMID | SOFT, 14: TLO}, B={0: RIDE, 4: RIDE, 8: RIDE, 12: RIDE}),
 'rock':   dict(A={0: KICK, 10: KICK, 4: SNARE, 12: SNARE, 7: KICK | SOFT}, B={c: HAT for c in range(0, 16, 2)}),
 'dance':  dict(A={0: KICK, 4: KICK, 8: KICK, 12: KICK, 7: SNARE | SOFT, 15: SNARE | SOFT}, A2={4: CLAP, 12: CLAP}, B={c: HAT for c in range(2, 16, 4)} | {c: HATS | SOFT for c in range(1, 16, 2)}),
 'latin':  dict(A={0: KICK, 8: KICK, 3: TMID, 10: TMID, 6: THI, 12: RIM}, B={c: SHAKE for c in range(16)}),
 'tribal': dict(A={0: TLO, 3: TLO, 6: KICK, 8: TMID, 11: KICK, 14: THI}, B={c: SHAKE | (SOFT if c % 2 else 0) for c in range(16)}),
 'soft':   dict(A={0: KICK | SOFT}, B={4: HATS, 12: HATS, 8: RIDE | SOFT}),
 'heart':  dict(A={0: KICK, 3: KICK | SOFT}, B={}),
 'waltz':  dict(A={0: KICK, 8: RIM}, B={4: HATS, 12: HATS}),
 'tick':   dict(A={}, B={0: HATS, 4: HATS, 8: HATS, 12: HATS}),
 'none':   dict(A={}, B={}),
}
def drum_bar(sc, start, style, fill=False, crash=False, ride=False, lite=False):
    d = DR[style]
    for c, n in d['A'].items():
        if n and c == int(c): sc.put(V_DA, start + int(c), n, 1)
    for c, n in d.get('A2', {}).items(): 
        if sc.v[V_DA][start + c][0] == 0: sc.put(V_DA, start + c, n, 1)
    for c, n in d['B'].items():
        if lite and (c % 4): continue
        sc.put(V_DB, start + int(c), RIDE if (ride and n in (HAT, OHAT)) else n, 1)
    if fill and style not in ('none', 'soft', 'tick', 'heart'):
        for c, n in ((12, SNARE | SOFT), (13, SNARE | SOFT), (14, SNARE), (15, SNARE)) if style not in ('latin', 'tribal', 'march') else ((12, THI), (13, TMID), (14, TLO), (15, TLO)):
            sc.put(V_DA, start + c, n, 1)
    if crash: sc.put(V_DB, start, CRASH, 1)

# ------------------------------------------------------------------------------------------------ bass, pads, arps
def bass_bar(sc, start, root, style, mt):
    r = fold(root, 28, 40); f = r + 7; o = r + 12
    third = 3 if mt == 'min' else 4
    pat = {
     'root': [(0, r, 14)], 'sub': [(0, r, 16)], 'drone': [(0, r, 16)],
     'pump': [(c, r if c % 4 != 2 else o, 2) for c in range(0, 16, 2)],
     'five': [(0, r, 4), (4, f, 4), (8, r, 4), (12, o, 4)],
     'walk': [(0, r, 4), (4, r + 2, 4), (8, r + third, 4), (12, f, 2), (14, r + third, 2)],
     'offbeat': [(c, r, 2) for c in (2, 6, 10, 14)],
     'sync': [(0, r, 3), (3, r, 2), (6, f, 2), (10, r, 3), (14, o, 2)],
     'gallop': [(c, r, 1) for c in range(16) if c % 4 != 3],
     'waltz': [(0, r, 4), (4, f, 2), (8, f, 2), (12, r, 4)],
    }[style]
    for c, n, ln in pat: sc.put(V_BASS, start + c, n, ln)
def pads_bar(sc, start, root, mt):
    r = fold(root, 46, 58); third = 3 if mt == 'min' else 4
    sc.put(V_PADL, start, r + third + 12, 16); sc.put(V_PADR, start, r + 7 + 12, 16)
def arp_bar(sc, start, root, style, mt):
    r = fold(root, 48, 58); third = 3 if mt == 'min' else 4
    seqs = {'up': [0, third, 7, 12, 7, third], 'updown': [0, third, 7, 12, 16 if mt == 'maj' else 15, 12, 7, third], 'broken': [0, 7, third, 12, 7, 12, third, 7],
            'sparse': [0, None, 7, None, 12, None, 7, None], 'oct': [0, 12, 0, 12], 'trip': [0, third, 7]}
    sq = seqs[style]
    for c in range(16):
        k = sq[c % len(sq)]
        if k is not None: sc.put(V_ARP, start + c, r + 12 + k, 1)

# ------------------------------------------------------------------------------------------------ hooks into streams
def lead_place(sc, vc, L, ns, start, bars, tr='none', octs=0, maxlen=8, frm=0):
    cells = bars * 16; reps = (cells + L - 1) // L; notes = list(ns)
    if tr == 'inv': base = notes[0][1]; notes = [(g, 2 * base - p) for g, p in notes]
    if tr == 'rev': notes = sorted([(L - 1 - g, p) for g, p in notes])
    if tr == 'half': notes = [(g * 2, p) for g, p in notes]; L *= 2; reps = (cells + L - 1) // L
    placed = []
    for r in range(reps):
        for i, (g, p) in enumerate(notes):
            c = r * L + g
            if c >= cells or c < frm: continue
            nxt = (notes[i + 1][0] if i + 1 < len(notes) else L) - g
            ln = min(max(1, nxt), maxlen); pp = p + 12 * octs
            sc.put(vc, start + c, pp, ln); placed.append((start + c, pp, ln))
    return placed
def harmony_of(p, kt, mt):
    sc_ = MAJ if mt == 'maj' else MIN
    for d in (4, 3, 5):
        if (p - d - kt) % 12 in sc_: return p - d
    return p - 3
def hookset(R, kt, mt):
    a = R['a']; La, na = hook_notes(a, R.get('ra', 0), kt, mt)
    b = R.get('b') or a; Lb, nb = hook_notes(b, R.get('rb', 0), kt, mt) if R.get('b') else (La, na)
    c = R.get('c') or b; Lc, nc = hook_notes(c, R.get('rc', 0), kt, mt, 58, 78) if R.get('c') else (Lb, nb)
    return dict(A=(La, na), B=(Lb, nb), C=(Lc, nc))

# ------------------------------------------------------------------------------------------------ sections
FORMS = {
    'song':   [('intro', 4), ('verse', 8), ('pre', 2), ('chorus', 8), ('break', 4), ('verse2', 8), ('chorus2', 8), ('bridge', 4), ('final', 8), ('outro', 4)],
    'epic':   [('intro', 8), ('verse', 16), ('pre', 4), ('chorus', 16), ('break', 8), ('verse2', 16), ('pre', 4), ('chorus2', 16), ('bridge', 8), ('final', 16), ('outro', 8)],
    'ambient': [('intro', 8), ('verse', 12), ('break', 4), ('chorus', 12), ('outro', 8)],
    'boss':   [('verse', 8), ('pre', 2), ('chorus', 8), ('break', 2), ('verse2', 8), ('chorus2', 8), ('final', 8)],
    'short':  [('verse', 8), ('chorus', 8), ('verse2', 8), ('final', 8)],
    'jingle': [('jingle', 4)],
    'sting':  [('jingle', 2)],
}
FORMS['long'] = FORMS['epic']

def build2(R):
    a = A[R['a']]; kt = R.get('key'); mt = R.get('mode') or a['mode']
    if kt is None: kt = a['key']
    bpm = max(60, min(188, R.get('bpm') or a['bpm']))
    form = R.get('form', 'song'); form = FORMS[form]; bars = sum(n for _, n in form)
    sc = Sc(bars); bar = 0
    ds = R.get('drums', 'half'); bs = R.get('bass', 'root'); arps = R.get('arp', 'up'); hooks_cache = {}
    def hooks(k, m):
        if (k, m) not in hooks_cache: hooks_cache[(k, m)] = hookset(R, k, m)
        return hooks_cache[(k, m)]
    omode = 'maj' if mt == 'min' else 'min'
    for si, (sec, n) in enumerate(form):
        s0 = bar * 16; k, m = kt, mt
        if sec == 'chorus2' or sec == 'final': k = (kt + R.get('lift', 2)) % 12
        if sec == 'bridge': m = omode
        H = hooks(k, m)
        # --- which hook leads, and what plays
        P = dict(lead='A', lt='none', lo=0, ctr=None, harm=False, echo=False, arp=False, spark=False, pads=True, bass=bs, drums=ds, fill=True, crash=False, ride=False, lite=False, ml=8)
        if sec == 'intro':   P.update(lead=None, spark=True, bass='sub', drums='none', fill=False)
        elif sec == 'verse': P.update(lead='A', arp=True, ctr=None, lite=True)
        elif sec == 'pre':   P.update(lead='C', ctr='A', arp=True, fill=True, drums=ds, bass=bs, ml=4)
        elif sec == 'chorus': P.update(lead='B', ctr='A', harm=True, echo=True, arp=True, crash=True, ml=10)
        elif sec == 'break': P.update(lead='C', spark=True, bass='sub', drums='soft' if ds not in ('none',) else 'none', fill=False, ml=12, arp=True)
        elif sec == 'verse2': P.update(lead='A', lt='inv' if R.get('inv') else 'none', ctr='C', arp=True, echo=True)
        elif sec == 'chorus2': P.update(lead='B', lo=R.get('cup', 0), ctr='A', harm=True, echo=True, arp=True, crash=True, ml=10)
        elif sec == 'bridge': P.update(lead='C', harm=False, echo=True, spark=True, drums='half' if ds not in ('none', 'soft') else ds, arp=True, bass='five' if bs not in ('sub', 'drone') else bs, ml=12)
        elif sec == 'final': P.update(lead='B', lo=R.get('cup', 0), ctr='A', harm=True, echo=True, arp=True, spark=True, crash=True, ml=10)
        elif sec == 'outro': P.update(lead=None, spark=True, bass='sub', drums='soft' if ds != 'none' else 'none', fill=False)
        elif sec == 'jingle': P.update(lead='A', fill=False, crash=False, drums='tick' if ds != 'none' else 'none', ml=3)
        # --- lead
        lead_cells = []
        if sec == 'outro':
            L, ns = H['A']; lead_cells = lead_place(sc, V_LEAD, L, ns[:max(3, len(ns) // 2)], s0, 2, maxlen=12)
        elif sec == 'intro':
            L, ns = H['A']; lead_cells = lead_place(sc, V_LEAD, L, ns[:max(3, len(ns) // 3)], s0 + 16 * (n - 2), 2, maxlen=8)
        elif sec == 'jingle':
            L, ns = H['A']; lead_cells = lead_place(sc, V_LEAD, L, ns[:R.get('jn', 6)], s0, 1, maxlen=3)
            fin = fold(kt + 72, 64, 76); sc.put(V_LEAD, s0 + (16 * (n - 1) if n > 1 else 12), fin, 16 if n > 1 else 4)
        elif P['lead']:
            L, ns = H[P['lead']]
            lead_cells = lead_place(sc, V_LEAD, L, ns, s0, n, tr=P['lt'], octs=P['lo'], maxlen=P['ml'])
        # --- echo (the lead, three sixteenths later, on the other side) and harmony (a third below)
        if P['echo']:
            for c, p, ln in lead_cells:
                if c + 3 < s0 + n * 16: sc.put(V_ECHO, c + 3, p, min(ln, 4))
        if P['harm']:
            for c, p, ln in lead_cells: sc.put(V_HARM, c, harmony_of(p, k, m), ln)
        # --- counter-melody
        if P['ctr']:
            L2, ns2 = H[P['ctr']]
            lead_place(sc, V_COUNT, L2, ns2, s0, n, octs=-1 if sec in ('pre', 'verse2') else 0, maxlen=2 if sec != 'chorus' else 3, tr='half' if (R.get('chalf') and sec == 'verse2') else 'none')
        # --- sparkle: the hook an octave up, every other note
        if P['spark']:
            L3, ns3 = H['A']; lead_place(sc, V_SPARK, L3, ns3[::2], s0, n, octs=1, maxlen=2)
        # --- the bar-by-bar parts
        use = H[P['lead'] or 'A'][1]; Lu = H[P['lead'] or 'A'][0]
        for b in range(n):
            c0 = (b * 16) % Lu; root = chord_for(use, c0, c0 + 16, k, m)
            if P['bass'] != 'none' and not (sec == 'intro' and b < 1 and R.get('bassfirst') is None and n > 4): bass_bar(sc, s0 + b * 16, root, P['bass'], m)
            if P['pads']: pads_bar(sc, s0 + b * 16, root, m)
            if P['arp']: arp_bar(sc, s0 + b * 16, root, arps, m)
            last = (b % 4 == 3) or b == n - 1
            if P['drums'] != 'none': drum_bar(sc, s0 + b * 16, P['drums'], fill=P['fill'] and last and sec != 'jingle', crash=P['crash'] and b == 0, ride=(sec in ('chorus2', 'final') and R.get('ride', True) and ds in ('rock', 'half', 'march')), lite=P['lite'] and b % 2 == 1 and False)
            elif sec == 'intro' and b == n - 1 and ds != 'none': drum_bar(sc, s0 + b * 16, 'soft', fill=True)
        bar += n
    loop_bar = 0 if R.get('form') in ('boss', 'short') else (2 if R.get('form') in ('song', 'epic', 'long', 'ambient') else 0)
    if R.get('form') in ('song',): loop_bar = 4
    if R.get('form') in ('epic', 'long'): loop_bar = 8
    return sc, loop_bar, bpm, kt, mt

def pack(sc):
    out = []
    for vc in range(NV):
        ev = []; c = 0; cells = sc.cells
        while c < cells:
            n, ln = sc.v[vc][c]
            if n == 0 and ln == 0:
                e = c + 1
                while e < cells and sc.v[vc][e] == (0, 0): e += 1
                ev.append((0, e - c)); c = e
            else:
                e = c + 1
                while e < cells and sc.v[vc][e] == (0, 0) and e - c < ln: e += 1
                ev.append((n, e - c)); c = e
        fixed = []
        for n, ln in ev:
            while ln > 255: fixed.append((n, 255)); ln -= 255
            fixed.append((n, ln))
        out.append(fixed)
    return out


# ------------------------------------------------------------------------------------------------ minimalist pieces (Philip Glass territory): cycling arpeggios, additive process, phasing, 3-against-4, slow chord cycles
PATS = {'a': [0, 1, 2, 3, 2, 1, 0, 1], 'b': [0, 2, 1, 2, 0, 2, 4, 2], 'c': [0, 1, 2, 1, 3, 2, 1, 2], 'd': [0, 3, 1, 4, 2, 5, 3, 1], 'e': [0, 2, 4, 2, 5, 3, 1, 3]}
def build_glass(R):
    a = A[R['a']]; kt = R.get('key', a['key']); mt = R.get('mode') or a['mode']; bpm = R.get('bpm', a['bpm']); N = R['bars']; cb = R.get('cbars', 2)
    sc = Sc(N); P = PATS[R.get('pat', 'a')]
    # the chord cycle comes from the roots of the source hooks
    roots = []
    for src, rk in ((R['a'], R.get('ra', 0)), (R.get('b') or R['a'], R.get('rb', 0))):
        L, ns = hook_notes(src, rk, kt, mt)
        for w in range(0, max(L, 32), 16): roots.append(chord_for(ns, w % max(L, 16), w % max(L, 16) + 16, kt, mt))
    cyc = []
    for r in roots:
        if not cyc or cyc[-1] != r: cyc.append(r)
    cyc = cyc[:R.get('nchords', 4)] or [kt]
    def tones(bar):
        r = cyc[(bar // cb) % len(cyc)]; third = 3 if mt == 'min' else 4; base = fold(r, 55, 66)
        return [base, base + third, base + 7, base + 12, base + 12 + third, base + 19]
    secs = [(0, int(N * .08)), (1, int(N * .22)), (2, int(N * .22)), (3, int(N * .24)), (4, int(N * .16)), (5, N)]
    bounds = []; b0 = 0
    for k, e in secs: bounds.append((k, b0, max(b0, e))); b0 = max(b0, e)
    ptr = 0; addn = 2; addrep = 0
    for k, b0, b1 in bounds:
        for bar in range(b0, b1):
            tn = tones(bar); r = cyc[(bar // cb) % len(cyc)]
            if bar % cb == 0:
                pads_bar_n = 16 * cb
                third = 3 if mt == 'min' else 4; pr = fold(r, 46, 58)
                if k in (0, 3, 4, 5) or k == 2: sc.put(V_PADL, bar * 16, pr + third + 12, pads_bar_n); sc.put(V_PADR, bar * 16, pr + 19, pads_bar_n)
                sc.put(V_BASS, bar * 16, fold(r, 28, 40), 16 * cb if k != 3 else 8); 
                if k == 3: sc.put(V_BASS, bar * 16 + 8, fold(r, 28, 40) + 12, 8 * cb)
            for e8 in range(8):
                c = bar * 16 + e8 * 2
                if k == 0: continue
                if k == 1:   # additive process: the figure grows one note at a time
                    pass
                if k >= 1:
                    # lead: additive in section 1, the whole cycle afterwards (subtractive in section 4)
                    if k == 1:
                        sc.put(V_LEAD, c, tn[P[ptr % addn] % 6], 2); ptr += 1
                        if ptr % addn == 0:
                            addrep += 1
                            if addrep >= 3 and addn < 8: addn += 1; addrep = 0
                    else:
                        ln = 8 if k < 4 else max(3, 8 - (bar - b0) * 5 // max(1, b1 - b0))
                        sc.put(V_LEAD, c, tn[P[(bar * 8 + e8) % ln] % 6], 2)
                if k >= 2:   # phasing: a second voice an octave up that slips one eighth every few bars
                    sh = (bar - bounds[2][1]) // R.get('slip', 4)
                    sc.put(V_HARM, c, tn[P[(bar * 8 + e8 + sh) % 8] % 6] + 12, 2)
            if k >= 2:   # 3 against 4: dotted eighths
                for q in range(0, 16, 3):
                    c = bar * 16 + q
                    sc.put(V_COUNT, c, tn[P[((bar * 16 + q) // 3) % 8] % 6] - 12 + (12 if k == 3 else 0), 3)
            if k == 3:
                for c in range(16): sc.put(V_ARP, bar * 16 + c, tn[0] + 12 + (12 if c % 4 == 2 else 0), 1)
                if bar % 2 == 0: drum_bar(sc, bar * 16, 'heart')
            if k in (3, 4) and bar % 4 == 0:   # the theme, very slowly, on the bell
                L, ns = hook_notes(R['a'], R.get('ra', 0), kt, mt, 70, 88)
                for g, p in ns[::2][:6]: sc.put(V_SPARK, bar * 16 + g * 2 % 64, p, 8)
    return sc, 0, bpm, kt, mt

# ------------------------------------------------------------------------------------------------ instruments by mood
# waves: 0 SAW 1 PULSE 2 TRIANGLE 3 STRINGS 4 ORGAN 5 BELL 6 PLUCK 7 PAD 8 SUB 9 REED 10 FLUTE 11 BRASS 12 CHOIR 13 GLASS 14 SYNTH 15 WOBBLE
MOODS = {   # lead, echo, counter, harmony, padL, padR, bass, arp, sparkle
 'pop':   [14, 14, 6, 3, 7, 7, 2, 6, 5],
 'orch':  [10, 10, 3, 12, 3, 3, 2, 6, 5],
 'chip':  [1, 1, 9, 1, 4, 4, 2, 1, 13],
 'dance': [14, 14, 0, 14, 7, 7, 8, 6, 5],
 'dark':  [9, 9, 4, 12, 7, 7, 8, 13, 5],
 'folk':  [10, 10, 6, 3, 4, 4, 2, 6, 13],
 'rock':  [0, 0, 9, 11, 4, 4, 0, 1, 5],
 'brass': [11, 11, 3, 12, 3, 3, 2, 6, 13],
 'glass': [13, 13, 5, 12, 7, 7, 8, 13, 5],
 'minim': [4, 12, 10, 4, 12, 12, 8, 13, 5],
 'minim2': [14, 12, 6, 3, 7, 7, 2, 13, 5],
}
VOLS = [60, 38, 46, 40, 48, 48, 46, 40, 34]
PANS = [34, 104, 64, 22, 8, 120, 64, 64, 64]
DRUMSTYLE_MOOD = {'house': 'dance', 'dnb': 'dance', 'half': 'pop', 'shuffle': 'folk', 'break': 'dance', 'march': 'brass', 'rock': 'rock', 'dance': 'dance', 'latin': 'folk', 'tribal': 'folk', 'soft': 'orch', 'heart': 'dark', 'waltz': 'folk', 'tick': 'chip', 'none': 'orch'}

# ------------------------------------------------------------------------------------------------ the 38 role tracks (upgraded to long forms) + 32 full songs
U = lambda **k: k
TRACKS = []
FORMUP = {'song': 'song', 'short': 'short', 'ambient': 'ambient', 'boss': 'boss', 'jingle': 'jingle', 'long': 'epic'}
MOODFOR = {'title': 'orch', 'chapters': 'glass', 'newworld': 'orch', 'sphere': 'orch', 'cube': 'chip', 'ring': 'dance', 'morning': 'folk', 'noon': 'pop', 'sunset': 'folk', 'night': 'glass', 'storm': 'rock', 'desert': 'dark',
           'snow': 'glass', 'ocean': 'orch', 'jungle': 'folk', 'swamp': 'dark', 'volcano': 'rock', 'village': 'folk', 'market': 'folk', 'city': 'dance', 'harvest': 'folk', 'festival': 'dance', 'war': 'brass', 'siege': 'dance',
           'monster': 'dark', 'bestiary': 'glass', 'dragon': 'brass', 'demon': 'dark', 'kraken': 'dark', 'lich': 'dark', 'ascend': 'brass', 'unlock': 'glass', 'godhood': 'orch', 'credits': 'orch', 'freeplay': 'pop',
           'gameover': 'dark', 'timelapse': 'dance', 'pause': 'glass'}
for R in OLD:
    r = dict(R); r['form'] = FORMUP[R.get('form', 'song')]; r['mood'] = MOODFOR.get(R['id'], DRUMSTYLE_MOOD[R.get('drums', 'half')]); r['arp'] = r.get('arp') or {'tick': 'sparse', 'sparkle': 'sparse', 'arp': 'updown', 'stab': 'oct', 'pad': 'up', 'none': 'up'}.get(R.get('pad', 'pad'), 'up')
    if r['id'] == 'title': r['form'] = 'epic'
    if r['id'] in ('city', 'godhood', 'credits'): r['form'] = 'epic'
    TRACKS.append(r)
NEW = [
 ('genesis', 'GENESIS', 'earth_and_the_space_citizens', 0, 'sunman_sunrise', 1, 'mi_cora_zone', 0, 2, 'maj', 90, 'epic', 'sub', 'heart', 'orch', 'updown'),
 ('firstsun', 'THE FIRST SUNRISE', 'sunman_sunrise', 0, 'tree_swaying_action', 0, 'worthless_clouds', 1, 7, 'maj', 96, 'epic', 'five', 'soft', 'orch', 'up'),
 ('clay', 'CLAY AND BREATH', 'nursery_time', 0, 'excuses_house', 0, 'magic_act', 1, 9, 'min', 100, 'epic', 'walk', 'waltz', 'folk', 'broken'),
 ('glassriver', 'RIVERS OF GLASS', 'cynicaller_dnb', 0, 'earth_and_the_space_citizens', 1, 'aim_and_shoot', 1, 4, 'min', 128, 'epic', 'sync', 'dnb', 'glass', 'updown'),
 ('worldturns', 'THE WORLD TURNS', 'mi_cora_zone', 0, 'staged', 0, 'emergency_hitech', 1, 0, 'maj', 120, 'epic', 'pump', 'dance', 'dance', 'updown'),
 ('mtnking', 'MOUNTAIN KING', 'closer_to_the_end', 0, 'magic_act', 0, 'the_ticking_bomb', 1, 3, 'min', 112, 'epic', 'gallop', 'rock', 'rock', 'oct'),
 ('caravan', 'CARAVAN OF STARS', 'aim_and_shoot', 0, 'spanish_flexicode', 0, 'earth_and_the_space_citizens', 2, 2, 'min', 108, 'epic', 'waltz', 'latin', 'folk', 'broken'),
 ('hearth', 'HEARTH AND HOME', 'spanish_flexicode', 1, 'nursery_time', 1, 'sunman_sunrise', 3, 5, 'maj', 94, 'epic', 'walk', 'waltz', 'folk', 'up'),
 ('sails', 'SAILS ON THE HORIZON', 'excuses_house', 1, 'whistler_shuffle', 0, 'tree_fast', 0, 7, 'maj', 118, 'epic', 'walk', 'shuffle', 'brass', 'updown'),
 ('ironsalt', 'IRON AND SALT', 'the_ticking_bomb', 0, 'cocaine_cola_ii', 0, 'closer_to_the_end', 1, 6, 'min', 124, 'epic', 'gallop', 'march', 'brass', 'oct'),
 ('midnightp', 'MIDNIGHT PARADE', 'ohio_hoe', 0, 'hotdamn_rave', 0, 'mi_cora_zone', 2, 9, 'min', 134, 'epic', 'pump', 'dance', 'dance', 'updown'),
 ('longwinter', 'THE LONG WINTER', 'tree_swaying_action', 1, 'worthless_clouds', 0, 'nursery_time', 2, 4, 'min', 80, 'epic', 'drone', 'soft', 'glass', 'sparse'),
 ('serpent', "SERPENT'S DANCE", 'aim_and_shoot', 1, 'quickbullet', 0, 'spanish_flexicode', 2, 2, 'min', 122, 'epic', 'sync', 'break', 'dark', 'broken'),
 ('fieldsgold', 'FIELDS OF GOLD', 'gottcho_barracho_ii', 0, 'sunman_sunrise', 2, 'worthless_clouds', 2, 0, 'maj', 102, 'epic', 'five', 'latin', 'folk', 'up'),
 ('clockwork', 'CLOCKWORK ANGELS', 'amiga_music', 0, 'condensed_music', 0, 'magic_act', 2, 10, 'min', 116, 'epic', 'offbeat', 'break', 'chip', 'trip'),
 ('millionlives', 'A MILLION SMALL LIVES', 'worthless_clouds', 1, 'tree_fast', 1, 'sunman_sunrise', 4, 7, 'maj', 106, 'epic', 'five', 'half', 'pop', 'updown'),
 ('thunder', 'THUNDER DOES NOT ASK', 'quickbullet', 0, 'closer_to_the_end', 2, 'cocaine_cola_ii', 1, 2, 'min', 140, 'epic', 'gallop', 'rock', 'rock', 'oct'),
 ('glasscity', 'THE GLASS CITY', 'staged', 1, 'emergency_hitech', 0, 'mi_cora_zone', 1, 11, 'maj', 126, 'epic', 'pump', 'house', 'dance', 'updown'),
 ('oceankeeps', 'WHAT THE OCEAN KEEPS', 'meltdown_in_mars_house', 0, 'excuses_house', 2, 'earth_and_the_space_citizens', 3, 3, 'min', 84, 'epic', 'sub', 'heart', 'glass', 'sparse'),
 ('bloodmoon', 'BLOOD MOON', 'the_ticking_bomb', 1, 'over_my_voice', 0, 'quickbullet', 1, 6, 'min', 130, 'epic', 'sync', 'half', 'dark', 'broken'),
 ('lastbeacon', 'THE LAST BEACON', 'closer_to_the_end', 1, 'sunman_sunrise', 3, 'tree_swaying_action', 2, 9, 'min', 98, 'epic', 'five', 'rock', 'brass', 'up'),
 ('ashes', 'ASHES AND EMBERS', 'cocaine_cola_ii', 0, 'meltdown_in_mars_house', 1, 'worthless_clouds', 3, 5, 'min', 92, 'epic', 'walk', 'half', 'dark', 'sparse'),
 ('skyward', 'SKYWARD', 'tree_fast', 0, 'mi_cora_zone', 3, 'sunman_sunrise', 5, 4, 'maj', 138, 'epic', 'pump', 'dance', 'pop', 'updown'),
 ('dreamingdeep', 'THE DREAMING DEEP', 'worthless_clouds', 2, 'earth_and_the_space_citizens', 0, 'excuses_house', 3, 1, 'min', 74, 'epic', 'drone', 'soft', 'glass', 'sparse'),
 ('crownthorns', 'CROWN OF THORNS', 'emergency_hitech', 2, 'closer_to_the_end', 0, 'the_ticking_bomb', 2, 8, 'min', 118, 'epic', 'gallop', 'march', 'brass', 'oct'),
 ('lanternriver', 'LANTERNS ON THE RIVER', 'nursery_time', 1, 'aim_and_shoot', 2, 'worthless_clouds', 4, 6, 'maj', 90, 'epic', 'waltz', 'waltz', 'glass', 'broken'),
 ('dragontide', 'DRAGON TIDE', 'closer_to_the_end', 2, 'ohio_hoe', 1, 'over_my_voice', 1, 11, 'min', 146, 'epic', 'gallop', 'rock', 'rock', 'oct'),
 ('starfall', 'STARFALL', 'earth_and_the_space_citizens', 1, 'hotdamn_rave', 1, 'tree_fast', 2, 0, 'maj', 112, 'epic', 'five', 'house', 'dance', 'updown'),
 ('greatclock', 'THE GREAT CLOCK', 'condensed_music', 1, 'magic_act', 2, 'amiga_music', 1, 7, 'min', 104, 'epic', 'offbeat', 'half', 'chip', 'trip'),
 ('allkings', 'ALL THE KINGS OF THE EARTH', 'staged', 2, 'gottcho_barracho_ii', 1, 'mi_cora_zone', 3, 3, 'maj', 110, 'epic', 'walk', 'march', 'brass', 'up'),
 ('homeworld', 'HOME IS WHERE THE WORLD IS', 'sunman_sunrise', 4, 'worthless_clouds', 3, 'nursery_time', 3, 2, 'maj', 92, 'epic', 'five', 'half', 'folk', 'up'),
 ('pastedge', 'PAST THE EDGE OF THE MAP', 'earth_and_the_space_citizens', 2, 'excuses_house', 1, 'tree_fast', 3, 9, 'maj', 100, 'epic', 'walk', 'soft', 'orch', 'updown'),
]
for (i, nm, a, ra, b, rb, c, rc, key, mode, bpm, form, bass, drums, mood, arp) in NEW:
    TRACKS.append(dict(id=i, name=nm, use='jukebox: a full song', a=a, ra=ra, b=b, rb=rb, c=c, rc=rc, key=key, mode=mode, bpm=bpm, form=form, bass=bass, drums=drums, mood=mood, arp=arp, cup=1 if mood in ('rock', 'brass', 'dance', 'pop') else 0, lift=2))
GLASS = [  # id, name, source A (ra), source B (rb), key, mode, bpm, bars, pattern, mood, chord change every n bars, chords, slip
 ('accretion', 'ACCRETION', 'earth_and_the_space_citizens', 0, 'sunman_sunrise', 0, 9, 'min', 112, 96, 'a', 'minim', 2, 4, 4),
 ('sandclock', 'SAND CLOCK', 'nursery_time', 0, 'magic_act', 0, 0, 'min', 120, 128, 'b', 'minim', 2, 4, 3),
 ('stonecircles', 'CIRCLES OF STONE', 'closer_to_the_end', 0, 'tree_swaying_action', 0, 4, 'min', 104, 160, 'c', 'minim', 4, 4, 6),
 ('corridor', 'THE LONG CORRIDOR', 'meltdown_in_mars_house', 0, 'worthless_clouds', 0, 2, 'min', 132, 200, 'd', 'minim', 2, 6, 4),
 ('tidalorgan', 'TIDAL ORGAN', 'excuses_house', 0, 'earth_and_the_space_citizens', 1, 7, 'maj', 96, 144, 'a', 'minim', 4, 4, 5),
 ('mirrorstairs', 'MIRROR STAIRS', 'amiga_music', 0, 'condensed_music', 0, 5, 'min', 138, 112, 'e', 'minim2', 2, 4, 3),
 ('stillmotion', 'STILL MOTION', 'worthless_clouds', 1, 'sunman_sunrise', 2, 0, 'maj', 90, 176, 'b', 'minim', 4, 4, 6),
 ('countingstars', 'COUNTING THE STARS', 'tree_fast', 0, 'mi_cora_zone', 1, 2, 'maj', 126, 120, 'c', 'minim2', 2, 4, 4),
 ('slowwheel', 'THE SLOW WHEEL', 'spanish_flexicode', 0, 'aim_and_shoot', 0, 11, 'min', 108, 240, 'd', 'minim', 4, 6, 8),
 ('glassharbor', 'HARBOR OF GLASS', 'staged', 0, 'emergency_hitech', 0, 6, 'maj', 118, 136, 'e', 'minim2', 2, 4, 4),
 ('hourglassnight', 'ONE THOUSAND AND ONE NIGHTS', 'aim_and_shoot', 1, 'meltdown_in_mars_house', 2, 2, 'min', 100, 256, 'a', 'minim', 4, 6, 8),
 ('engineweather', 'ENGINE WEATHER', 'cynicaller_dnb', 0, 'hotdamn_rave', 0, 8, 'min', 144, 128, 'b', 'minim2', 2, 4, 3),
]
for (i, nm, a, ra, b, rb, key, mode, bpm, bars, pat, mood, cb, nch, slip) in GLASS:
    TRACKS.append(dict(id=i, name=nm, use='jukebox: a minimalist piece', a=a, ra=ra, b=b, rb=rb, key=key, mode=mode, bpm=bpm, bars=bars, pat=pat, mood=mood, cbars=cb, nchords=nch, slip=slip, glass=True, form='epic'))
assert len(TRACKS) == 82, len(TRACKS)
# ------------------------------------------------------------------------------------------------ the remixes: every one of the original BORE tracks gets a REMIX (revitalised: new key, groove, sounds, tempo)
# and a REBIRTH (revised: the opposite mode, a quieter or stranger form -- a third of them minimalist -- with a neighbouring song's hooks woven in). Locked until the game is 100% done.
REMIX_START = len(TRACKS)
ORIG = [  # source song, remix name, rebirth name
 ('earth_and_the_space_citizens', 'EARTH AND THE STAR CITIZENS', 'EARTHRISE'), ('amiga_music', 'AMIGA MUSIC 2000', 'AMIGA AMIGO'), ('emergency_hitech', 'EMERGENCY ON THE SKY FLOOR', 'EMERGENCY EXIT'),
 ('tree_swaying_action', 'TREE-AGE IN REACTION', 'TREE-AGE REGROWN'), ('meltdown_in_mars_house', 'MELTUP IN MARS', 'MARS HOUSE REBUILT'), ('worthless_clouds', 'PRICELESS CLOUDS', 'WORTHLESS STORMS'),
 ('sunman_sunrise', 'SUNWOMAN SUNSET', "SUNMAN'S SECOND DAWN"), ('gottcho_barracho_ii', 'GOTTCHO BARRACHO III', 'GOTCHA BORRACHO'), ('spanish_flexicode', 'AN ODE TO THE ITALIAN FLEXICODE', 'A SPANISH FLEXICODA'),
 ('mi_cora_zone', 'MI CORA ZENITH', 'MI CORAZON ZONE'), ('excuses_house', 'EXCUSES, EXCUSES', 'NO MORE EXCUSES'), ('whistler_shuffle', 'WHISTLER WOMAN', "WHISTLER'S SHUFFLE REVISITED"),
 ('cynicaller_dnb', 'THE CYNICAL RENDITION', 'CYNICALLER SERENITY'), ('hotdamn_rave', 'HOTTER DAMN', 'HOT DAMN COOLED'), ('aim_and_shoot', 'AIM AND SHOOT AGAIN', 'AIM AND SPARE'),
 ('magic_act', 'THE MAGIC ENCORE', 'THE MAGIC ACT REVERSED'), ('nursery_time', 'NURSERY NIGHTMARE', 'NURSERY TIME REMEMBERED'), ('tree_fast', 'TREE-AGE IN FASTER ACTION', 'TREE-AGE AT REST'),
 ('condensed_music', 'EXPANDED MUSIC', 'CONDENSED MILK'), ('cocaine_cola_ii', 'CHERRY COLA', 'COLA AFTER MIDNIGHT'), ('staged', 'STAGED THE ENCORE', 'BACKSTAGE'),
 ('closer_to_the_end', 'FURTHER FROM THE END', 'CLOSER TO THE BEGINNING'), ('the_ticking_bomb', 'THE TICKING CLOCK', 'THE DEFUSED BOMB'), ('quickbullet', 'MISTER SLOWBULLET', 'MISS QUICKBULLET'),
 ('over_my_voice', 'OVER MY VOICE AGAIN', 'UNDER MY VOICE'), ('ohio_hoe', 'OHIO HERO', 'OHIO HOMECOMING'),
]
RMOODS = ['dance', 'pop', 'rock', 'brass', 'chip', 'folk', 'glass', 'orch']
RDRUMS = ['dance', 'house', 'rock', 'break', 'half', 'shuffle', 'dnb', 'march']
RBASS = ['pump', 'gallop', 'five', 'sync', 'walk', 'offbeat']
for n, (src, rm, rb) in enumerate(ORIG):
    a = A[src]; nb = ORIG[(n + 1) % len(ORIG)][0]; nc = ORIG[(n + 7) % len(ORIG)][0]; bpm0 = a['bpm']
    bpm = int(bpm0 * 1.18) if bpm0 < 120 else int(bpm0 * 0.62 + 20)
    TRACKS.append(dict(id='remix%d' % n, name=rm, use='remix: unlocked at 100%', a=src, ra=1, b=src, rb=2, c=src, rc=3, key=(a['key'] + 5) % 12, mode=a['mode'], bpm=max(78, min(176, bpm)), form='epic',
                       bass=RBASS[n % 6], drums=RDRUMS[n % 8], mood=RMOODS[(n + 3) % 8], arp=['updown', 'broken', 'up', 'oct'][n % 4], cup=1 if n % 2 == 0 else 0, lift=2, inv=(n % 3 == 0)))
    flip = 'maj' if a['mode'] == 'min' else 'min'
    if n % 3 == 0:
        TRACKS.append(dict(id='rebirth%d' % n, name=rb, use='rebirth: unlocked at 100%', a=src, ra=2, b=nb, rb=0, key=(a['key'] + 7) % 12, mode=flip, bpm=max(84, min(140, int(bpm0 * 0.85))), bars=96 + 16 * (n % 4),
                           pat='abcde'[n % 5], mood='minim' if n % 2 == 0 else 'minim2', cbars=2 + 2 * (n % 2), nchords=4, slip=3 + n % 4, glass=True, form='epic'))
    else:
        TRACKS.append(dict(id='rebirth%d' % n, name=rb, use='rebirth: unlocked at 100%', a=src, ra=2, b=nb, rb=1, c=nc, rc=2, key=(a['key'] + 7) % 12, mode=flip, bpm=max(70, min(132, int(bpm0 * 0.8))), form='epic',
                           bass=['sub', 'drone', 'walk', 'five'][n % 4], drums=['soft', 'heart', 'half', 'waltz'][n % 4], mood=['orch', 'glass', 'folk', 'dark'][n % 4], arp=['sparse', 'broken', 'up', 'trip'][n % 4], cup=0, lift=2))
REMIX_N = len(TRACKS) - REMIX_START
assert REMIX_N == 52, REMIX_N


SUITE_START = len(TRACKS)
def _prebuild():
    return [(build_glass(R) if R.get('glass') else build2(R)) for R in TRACKS]
_BUILT = _prebuild()
import gm3_compose
SUITES_OUT = gm3_compose.build_suites(sys.modules[__name__], list(TRACKS), _BUILT)
for (_S, _sc, _src, _m) in SUITES_OUT:
    TRACKS.append(dict(id=_S['id'], name=_S['name'], use='epic suite: %d min' % int(_m), mood=_S['mood'], form='suite', suite=(_sc, _S['bpm'], _S['home'], getattr(_sc, 'fade', 0))))
SUITE_N = len(SUITES_OUT)

def write_header(path):
    L = ['#ifndef GMDATA_H', '#define GMDATA_H', '// gmdata.h - GENERATED by godguy/tools/gm2_compose.py. Do not edit. %d tracks (11 streams each) and %d sound effects, amalgamated from the visible songs of BORE.' % (len(TRACKS), len(SFX)), '']
    L.append('typedef struct { const char*name, *use; unsigned char bpm; unsigned char inst[9], vol[9], pan[9]; unsigned short rows, loopRow, fade; unsigned short n[11]; const unsigned char*v[11]; } GmSong;')
    L.append('typedef struct { const char*name, *src; unsigned char n; const unsigned char*ev; } GmFx;')
    total = 0; meta = []; info = []
    for i, R in enumerate(TRACKS):
        sc, loop_bar, bpm, kt, mt = ((R['suite'][0], 0, R['suite'][1], R['suite'][2], 'maj') if R.get('suite') else _BUILT[i] if i < SUITE_START else (build_glass(R) if R.get('glass') else build2(R))); pk = pack(sc)
        for vc in range(NV):
            b = []; [b.extend([n, ln]) for n, ln in pk[vc]]
            L.append('static const unsigned char gm%d_%d[]={%s};' % (i, vc, ','.join(map(str, b)) if b else '0,255')); total += len(b)
        inst = MOODS[R['mood']][:]; vol = VOLS[:]
        if R['form'] in ('ambient',): vol[V_LEAD] = 56; vol[V_PADL] = vol[V_PADR] = 40
        if R['form'] in ('jingle', 'sting'): vol[V_LEAD] = 76
        meta.append('{"%s","%s",%d,{%s},{%s},{%s},%d,%d,%d,{%s},{%s}}' % (R['name'], R['use'], bpm, ','.join(map(str, inst)), ','.join(map(str, vol)), ','.join(map(str, PANS)), sc.cells,
                    65535 if R['form'] in ('jingle', 'sting') else loop_bar * 16, R['suite'][3] if R.get('suite') else 0, ','.join(str(len(pk[v]) * 2 if pk[v] else 2) for v in range(NV)), ','.join('gm%d_%d' % (i, v) for v in range(NV))))
        info.append((R['name'], R['use'], bpm, NOTE[kt % 12], mt, sc.cells // 16, sc.cells * 15.0 / bpm, R['mood'], R['form']))
    L.append('#define GM_EPIC0 %d' % SUITE_START); L.append('#define GM_REMIX0 %d' % REMIX_START); L.append('#define GM_REMIXES %d' % REMIX_N)
    L.append('static const GmSong gmSongs[%d]={' % len(TRACKS)); L.append(',\n'.join(meta)); L.append('};'); L.append('#define GM_SONGS %d' % len(TRACKS))
    for i, R in enumerate(TRACKS): L.append('#define GM_%s %d' % (R['id'].upper(), i))
    for i, (nm, evs, src) in enumerate(SFX):
        b = []; [b.extend([e[0], e[1], e[2], e[3], e[4] & 255]) for e in evs]
        L.append('static const unsigned char gmfx%d[]={%s};' % (i, ','.join(map(str, b)))); total += len(b)
    L.append('static const GmFx gmFx[%d]={' % len(SFX)); L.append(',\n'.join('{"%s","%s",%d,gmfx%d}' % (nm, src, len(evs), i) for i, (nm, evs, src) in enumerate(SFX))); L.append('};'); L.append('#define GM_FX %d' % len(SFX))
    for i, (nm, evs, src) in enumerate(SFX): L.append('#define FX_%s %d' % (nm.replace(' ', '_'), i))
    L.append('#endif')
    open(path, 'w').write('\n'.join(L) + '\n')
    return total, info

if __name__ == '__main__':
    tot, info = write_header(os.path.join(HERE, '..', 'source', 'gmdata.h'))
    print('%d bytes of event data, %d tracks, %d effects' % (tot, len(TRACKS), len(SFX)))
    ts = 0
    for (nm, use, bpm, k, m, bars, secs, mood, form) in info:
        ts += secs; print('%-28s %-24s %3d bpm %-2s %-3s %3d bars %4.0fs %-6s %s' % (nm, use, bpm, k, m, bars, secs, mood, form))
    print('total %.1f minutes' % (ts / 60))
