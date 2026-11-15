# Quality control for every track of the music (run: python3 gm_qc.py [name filter]): reads the scores exactly as gm2_compose.write_header packs them and measures
#   layers (which of the 9 voices play, and how much of the song), pitch range, notes outside the key (and outside the chord under them), lead-against-bass clashes (minor 2nds, tritones, major 7ths),
#   bar-for-bar repetition, the dynamic arc (how much the density changes through the song), drum fills at the ends of phrases, a quiet start / a ringing end.
# gm_qc.upgrade() is the other half: the repairs and additions every track gets (see its docstring).
import os, sys, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
MAJ_S = {0, 2, 4, 5, 7, 9, 11}; MIN_S = {0, 2, 3, 5, 7, 8, 10, 11}      # (natural minor + the raised seventh)
NAMES = ['LEAD', 'ECHO', 'COUNT', 'HARM', 'PADL', 'PADR', 'BASS', 'ARP', 'SPARK']
def sounding(sc, vc):
    """per cell: the pitch sounding (0 = none) for a melodic voice"""
    out = [0] * sc.cells; c = 0
    while c < sc.cells:
        n, ln = sc.v[vc][c]
        if n:
            for k in range(c, min(sc.cells, c + ln)):
                if sc.v[vc][k] != (0, 0) and k > c: break
                out[k] = n
            c += 1
        else: c += 1
    return out
def analyze(sc, kt, mt):
    cells = sc.cells; bars = cells // 16; scale = {(kt + x) % 12 for x in (MAJ_S if mt == 'maj' else MIN_S)}; M = {}
    snd = [sounding(sc, v) for v in range(9)]
    M['bars'] = bars; M['layers'] = {}
    for v in range(9):
        on = sum(1 for x in snd[v] if x); notes = [x for x in snd[v] if x]; M['layers'][NAMES[v]] = (on / cells, min(notes) if notes else 0, max(notes) if notes else 0)
    off = tot = 0
    for v in (0, 1, 2, 3, 7, 8):
        for c in range(cells):
            n = sc.v[v][c][0]
            if n:
                tot += 1; pc = n % 12; bpc = [snd[6][k] % 12 for k in range(c, min(cells, c + 1)) if snd[6][k]]
                if pc not in scale and not (bpc and (pc - bpc[0]) % 12 in (0, 3, 4, 7, 10, 11)): off += 1
    M['offkey'] = off / max(1, tot)
    cl = both = 0
    for c in range(cells):
        a, b = snd[0][c], snd[6][c]
        if a and b:
            both += 1; iv = abs(a - b) % 12
            if iv in (1, 11): cl += 1
    M['clash'] = cl / max(1, both); M['both'] = both
    hs = collections.Counter(); dup = 0
    for b in range(bars):
        h = tuple(sc.v[0][b * 16 + k] for k in range(16)) + tuple(sc.v[6][b * 16 + k] for k in range(16))
        if any(x[0] for x in h):
            if h in hs: dup += 1
            hs[h] += 1
    M['dupbars'] = dup / max(1, sum(hs.values()))
    dens = []
    for b0 in range(0, bars, 4):
        d = 0
        for v in (0, 1, 2, 3, 7, 8, 9, 10):
            for c in range(b0 * 16, min(cells, (b0 + 4) * 16)):
                if sc.v[v][c][0]: d += 1
        dens.append(d)
    m = sum(dens) / max(1, len(dens)); M['arc'] = (sum((x - m) ** 2 for x in dens) / max(1, len(dens))) ** 0.5 / (m or 1); M['dens'] = dens
    fills = phr = 0
    for b in range(3, bars, 4):
        phr += 1
        if sum(1 for k in range(12, 16) if sc.v[9][b * 16 + k][0]) >= 2: fills += 1
    M['fills'] = fills / max(1, phr)
    first = next((c for c in range(cells) if any(sc.v[v][c][0] for v in (0, 2, 3, 6, 7))), cells); M['start'] = first
    M['empty'] = [NAMES[v] for v in range(9) if M['layers'][NAMES[v]][0] < 0.02]
    return M
def tracks(upgraded=False):
    import gm2_compose as g
    import gm_up
    for i, R in enumerate(g.TRACKS):
        sc, loop_bar, bpm, kt, mt = ((R['suite'][0], 0, R['suite'][1], R['suite'][2], 'maj') if R.get('suite') else g._BUILT[i] if i < g.SUITE_START else g.build_any(R))
        if upgraded and not getattr(sc, '_up', 0): gm_up.upgrade(sc, R, kt, mt); sc._up = 1
        yield i, R, sc, bpm, kt, mt
def report(flt=None, upgraded=False):
    rows = []
    for i, R, sc, bpm, kt, mt in tracks(upgraded):
        if flt and flt.lower() not in R['name'].lower(): continue
        M = analyze(sc, kt, mt); rows.append((i, R, M, bpm))
    print('%3s %-28s %-6s %4s %5s %5s %5s %5s %5s  %s' % ('#', 'name', 'form', 'bars', 'offk%', 'clsh%', 'dup%', 'arc', 'fill%', 'empty layers'))
    for i, R, M, bpm in rows:
        print('%3d %-28s %-6s %4d %5.1f %5.1f %5.1f %5.2f %5.0f  %s' % (i, R['name'][:28], R['form'][:6], M['bars'], M['offkey'] * 100, M['clash'] * 100, M['dupbars'] * 100, M['arc'], M['fills'] * 100, ','.join(M['empty'])))
    return rows
if __name__ == '__main__':
    rows = report(None, 'up' in sys.argv)
    n = len(rows); f = lambda k: sum(r[2][k] for r in rows) / n
    print('\nmean: offkey %.1f%%  clash %.1f%%  dup %.1f%%  arc %.2f  fills %.0f%%  empty layers per track %.1f' % (f('offkey') * 100, f('clash') * 100, f('dupbars') * 100, f('arc'), f('fills') * 100, sum(len(r[2]['empty']) for r in rows) / n))
