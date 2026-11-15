# The upgrade every track of the music goes through before it is packed (gm2_compose.write_header calls upgrade()). It works on the finished score, so it improves the generated
# songs, the remixes, the suites and the retold songs alike, and gm_qc.py measures what it did.
#   1. REPAIRS: notes outside the key (and outside the chord under them) are moved onto the nearest note of it; a bass note a semitone from the lead note sounding with it is moved to a
#      chord-safe neighbour (minor 2nds against the bass are the harshest sound the generators made).
#   2. VARIATION OF REPEATS: a bar that is a repeat of an earlier one (same lead, bass and kick pattern) gets something new each time it comes round (k = how many times before):
#      a neighbour-note turn at the end of long lead notes; a third below the lead on the harmony voice; a bass approach note into the next bar; a counter-line of chord tones on the sparkle voice.
#   3. DRUMS: a fill at the end of every four-bar phrase (toms for the tribal / latin / march kits, snares for the rest), a crash on the first bar of each eight when the song has played
#      it before, an open hat on the last offbeat for the third time round.
#   4. ARC: the fourth pass of a section drops its pads to a single voice for one bar before the next one (a breath), so the sections breathe and the density is not a flat line.
import gm2_compose as g
MAJ_S = [0, 2, 4, 5, 7, 9, 11]; MIN_S = [0, 2, 3, 5, 7, 8, 10]
def scale(kt, mt): return sorted(((kt + x) % 12) for x in (MAJ_S if mt == 'maj' else MIN_S))
def snap(n, pcs, extra=()):
    allowed = set(pcs) | set(extra)
    if n % 12 in allowed: return n
    for d in (1, -1, 2, -2):
        if (n + d) % 12 in allowed: return n + d
    return n
def step(n, k, pcs):
    d = 1 if k > 0 else -1
    for _ in range(abs(k)):
        n += d
        while n % 12 not in pcs: n += d
    return n
def start_of(sc, v, c):
    while c > 0 and sc.v[v][c] == (0, 0): c -= 1
    return c
def set_note(sc, v, c, n, ln):
    s = start_of(sc, v, c)
    if s < c:
        n0, l0 = sc.v[v][s]
        if n0 or l0: sc.v[v][s] = (n0, c - s)
    ln = max(1, min(ln, sc.cells - c))
    for k in range(c + 1, c + ln):
        if sc.v[v][k] != (0, 0): ln = k - c; break
    sc.v[v][c] = (n, ln)
def sounding(sc, v):
    out = [0] * sc.cells; c = 0; cur = 0; left = 0
    for c in range(sc.cells):
        n, ln = sc.v[v][c]
        if n or ln: cur = n; left = ln
        if left > 0: out[c] = cur; left -= 1
        else: out[c] = 0
    return out
def fix_clash(sc, pcs, stats):      # a bass note a semitone (or a major seventh) from the lead note sounding with it is moved to a neighbour that is not
    lead = sounding(sc, 0)
    for c in range(sc.cells):
        n, ln = sc.v[6][c]
        if not n: continue
        span = range(c, min(sc.cells, c + max(1, ln)))
        if any(lead[x] and abs(lead[x] - n) % 12 in (1, 11) for x in span):
            for d in (-12, 12, -2, 2, -1, 1, -7, 7):
                m = n + d
                if 24 <= m <= 83 and m % 12 in pcs and all(not lead[x] or abs(lead[x] - m) % 12 not in (1, 11) for x in span): sc.v[6][c] = (m, ln); stats['bass'] += 1; break
def upgrade(sc, R, kt, mt):
    rm = bool(R.get('midi'))
    if rm and not R.get('remix'): return sc      # (the MIDI-read songs are written note for note, except a song flagged 'remix': then the band is only ADDED to, never changed)
    pcs = set(scale(kt, mt)); pl = sorted(pcs); bars = sc.cells // 16; form = R['form']; vox = bool(R.get('vox'))
    bassn = sounding(sc, 6); stats = dict(snapped=0, bass=0, turns=0, harm=0, fills=0, crashes=0, approach=0, spark=0)
    # 1a. off-key notes onto the key (a chord tone of the bass note under them is allowed too)
    for v in (0, 1, 2, 3, 7, 8):
        for c in range(sc.cells if not rm else 0):
            n, ln = sc.v[v][c]
            if not n: continue
            b = bassn[c]; chord = [(b + x) % 12 for x in (0, 3, 4, 7, 10)] if b else []
            if n % 12 in pcs or n % 12 in chord: continue
            m = snap(n, pcs, chord); 
            if m != n: sc.v[v][c] = (m, ln); stats['snapped'] += 1
    if not rm: fix_clash(sc, pcs, stats)
    # identify repeats (against the score as it was)
    keys = []
    for b in range(bars):
        keys.append(tuple(sc.v[0][b * 16 + k] for k in range(16)) + tuple(sc.v[6][b * 16 + k] for k in range(16)) + tuple(sc.v[9][b * 16 + k] for k in range(16)))
    seen = {}; rep = []
    for b in range(bars):
        k = keys[b]; rep.append(seen.get(k, 0)) if any(x[0] for x in k) else rep.append(0); seen[k] = seen.get(k, 0) + 1
    lead = sounding(sc, 0); harm = sounding(sc, 3); spark = sounding(sc, 8)
    toms = False
    da = [sc.v[9][c][0] for c in range(sc.cells)]
    if sum(1 for x in da if x in (4, 5, 6)) > sum(1 for x in da if x == 2): toms = True
    phrases = bars // 4
    for b in range(2, bars - 1):
        if form in ('jingle', 'sting', 'ambient'): break
        k = rep[b]; c0 = b * 16
        if k >= 1 and rm:
            # REMIX additions to the band (the notes of the MIDI are left alone): a third below the lead on the harmony voice, and a rising sparkle on the offbeats of the last bar of a phrase
            if b % 4 in (1, 3):
                for c in range(c0, c0 + 16):
                    n, ln = sc.v[0][c]
                    if n and ln >= 2 and not any(sc.v[3][x][0] or sc.v[3][x][1] for x in range(c, min(sc.cells, c + ln))):
                        h = step(n, -2, pcs)
                        if 24 <= h <= 83: sc.v[3][c] = (h, ln); stats['harm'] += 1
            if k >= 2 and b % 4 == 3:
                r = next((sc.v[6][c0 + q][0] for q in range(16) if sc.v[6][c0 + q][0]), 0)
                if r:
                    t = r + 24
                    while t > 83: t -= 12
                    for j, q in enumerate((1, 5, 9, 13)):
                        if sc.v[8][c0 + q] == (0, 0): set_note(sc, 8, c0 + q, snap(t + (0, 4, 7, 12)[j], pcs), 2); stats['spark'] += 1
        if k >= 1 and not rm:
            # neighbour-note turn on the last cell of every lead note of 4 cells or more
            for c in range(c0, c0 + 16):
                n, ln = sc.v[0][c]
                if n and ln >= 4 and c + ln <= c0 + 16 and (c + ln - 1) % 16 != 0:
                    nb = step(n, 1 if (k + (c >> 2)) % 2 else -1, pcs)
                    if 24 <= nb <= 83: set_note(sc, 0, c, n, ln - 1); set_note(sc, 0, c + ln - 1, nb, 1); stats['turns'] += 1
            # a passing tone between two lead notes a third apart (the first one gives up its last cell), and, from the third time round, the last note of the bar an octave up
            ons = [(c, sc.v[0][c]) for c in range(c0, c0 + 16) if sc.v[0][c][0]]
            for (ca, (na, la)), (cb_, (nb_, lb_)) in zip(ons, ons[1:]):
                if la >= 3 and ca + la == cb_ and abs(nb_ - na) in (3, 4) and (b + ca) % 2 == 0:
                    pt = step(na, 1 if nb_ > na else -1, pcs)
                    if pt % 12 != nb_ % 12 and 24 <= pt <= 83: set_note(sc, 0, ca, na, la - 1); set_note(sc, 0, cb_ - 1, pt, 1); stats['turns'] += 1
            if k >= 2 and ons:
                cl, (nl, ll) = ons[-1]
                if nl + 12 <= 83 and ll >= 2: sc.v[0][cl] = (nl + 12, ll); stats['turns'] += 1
            # the bass jumps an octave on the third beat (the second time round) or takes the fifth (the third time round)
            if sc.v[6][c0 + 8][0] and b % 2 == 0:
                nbv, lbv = sc.v[6][c0 + 8]
                alt = (nbv + 12 if nbv < 48 else nbv - 12) if k % 2 == 1 else snap(nbv + 7, pcs)
                if 24 <= alt <= 83: sc.v[6][c0 + 8] = (alt, lbv); stats['bass'] += 1
            # a third below the lead, on the harmony voice, where there is none
            if b % 4 in (1, 3) and k >= 1:
                for c in range(c0, c0 + 16):
                    n, ln = sc.v[0][c]
                    if n and ln >= 2 and not any(sc.v[3][x][0] or sc.v[3][x][1] for x in range(c, min(sc.cells, c + ln))):
                        h = step(n, -2, pcs); 
                        if 24 <= h <= 83: sc.v[3][c] = (h, ln); stats['harm'] += 1
            # a bass approach note on the last beat into a different bass note of the next bar
            if k >= 1 and b % 2 == 1 and b + 1 < bars:
                nxt = next((sc.v[6][(b + 1) * 16 + q][0] for q in range(0, 4) if sc.v[6][(b + 1) * 16 + q][0]), 0)
                if nxt and sc.v[6][c0 + 14] == (0, 0) and sc.v[6][c0 + 12][0]:
                    ap = step(nxt, 1 if (b % 4) == 1 else -1, pcs)
                    if 24 <= ap <= 83 and ap % 12 != nxt % 12: set_note(sc, 6, c0 + 14, ap, 2); stats['approach'] += 1
            # sparkle: a rising chord-tone arpeggio on the offbeats of the last bar of a phrase, third time round or later
            if k >= 2 and b % 4 == 3:
                r = next((sc.v[6][c0 + q][0] for q in range(16) if sc.v[6][c0 + q][0]), 0)
                if r:
                    t = r + 24
                    while t > 83: t -= 12
                    for j, q in enumerate((1, 5, 9, 13)):
                        if sc.v[8][c0 + q] == (0, 0): set_note(sc, 8, c0 + q, snap(t + (0, 4, 7, 12)[j], pcs), 2); stats['spark'] += 1
        if rm and b % 8 == 0 and b > 0 and sum(1 for q in range(16) if sc.v[9][c0 + q][0]) >= 3:      # a riser into every eighth bar and an impact (with a crash) on it, where the cymbal stream is free
            if sc.v[10][c0 - 12] == (0, 0): sc.v[10][c0 - 12] = (7, 1)
            if sc.v[10][c0] == (0, 0): sc.v[10][c0] = (8 if b % 16 == 0 else 3, 1); stats['crashes'] += 1
        # drums: a fill at the end of each phrase, a crash on the first bar of a repeated eight
        drums = sum(1 for q in range(16) if sc.v[9][c0 + q][0])
        if drums >= 3 and b % 4 == 3 and sum(1 for q in range(12, 16) if sc.v[9][c0 + q][0]) < 2:
            seq = ((12, 6), (13, 5), (14, 4), (15, 4)) if toms else ((12, 2 | 0x80), (13, 2 | 0x80), (14, 2), (15, 2))
            for q, idv in seq: sc.v[9][c0 + q] = (idv, 1)
            stats['fills'] += 1
        if (not vox) and b % 8 == 0 and k >= 1 and drums >= 3 and sc.v[10][c0] == (0, 0): sc.v[10][c0] = (3, 1); stats['crashes'] += 1
    # 4. a breath: the harmony and the arpeggio rest for the last bar of every sixteen, so the next section comes back in bigger
    if form in ('epic', 'suite', 'song') and not vox and not rm:
        for b in range(15, bars - 4, 16):
            for v in (3, 7):
                c0 = b * 16; s = start_of(sc, v, c0)
                if s < c0:
                    n0, l0 = sc.v[v][s]
                    if n0 or l0: sc.v[v][s] = (n0, c0 - s)
                for c in range(c0, c0 + 16): sc.v[v][c] = (0, 0)
            stats['breaths'] = stats.get('breaths', 0) + 1
    # 5. the ARC: the first eight bars of a long piece come in thinner (no counter-line, harmony or sparkle yet), the last four go out thinner, and the second-to-last bar of every sixteen
    #    builds with a soft snare roll into the fill and the breath
    def clear(v, c0, c1):
        s = start_of(sc, v, c0)
        if s < c0:
            n0, l0 = sc.v[v][s]
            if n0 or l0: sc.v[v][s] = (n0, c0 - s)
        for c in range(c0, min(sc.cells, c1)): sc.v[v][c] = (0, 0)
    if form in ('epic', 'suite', 'song') and bars >= 32 and not rm:
        for v in (2, 3, 8): clear(v, 0, 8 * 16)
        for v in (1, 2, 3, 8): clear(v, (bars - 4) * 16, sc.cells)
        stats['arc'] = stats.get('arc', 0) + 1
        for b in range(14, bars - 2, 16):
            c0 = b * 16
            if sum(1 for q in range(16) if sc.v[9][c0 + q][0]) >= 3:
                for q in range(8, 16):
                    if sc.v[9][c0 + q] == (0, 0): sc.v[9][c0 + q] = (2 | 0x80, 1)
    # no bar may end up note for note like an earlier one (lead + bass): the ones that do get one more change, a bass octave on the downbeat or a step on the lead's last note
    seen2 = set(); lead = sounding(sc, 0)
    for b in range(bars):
        c0 = b * 16; key = tuple(sc.v[0][c0 + q] for q in range(16)) + tuple(sc.v[6][c0 + q] for q in range(16))
        if not any(x[0] for x in key): continue
        if key in seen2 and b >= 2 and form not in ('jingle', 'sting', 'ambient') and not rm:
            n6, l6 = sc.v[6][c0]
            if n6:
                alt = n6 + 12 if n6 < 48 else n6 - 12
                if 24 <= alt <= 83: sc.v[6][c0] = (alt, l6); stats['bass'] += 1
            key = tuple(sc.v[0][c0 + q] for q in range(16)) + tuple(sc.v[6][c0 + q] for q in range(16))
            if key in seen2:
                ons = [c for c in range(c0, c0 + 16) if sc.v[0][c][0]]
                if ons:
                    cl = ons[-1]; nl, ll = sc.v[0][cl]; nn = step(nl, 1 if b % 2 else -1, pcs)
                    if 24 <= nn <= 83: sc.v[0][cl] = (nn, ll); stats['turns'] += 1
                key = tuple(sc.v[0][c0 + q] for q in range(16)) + tuple(sc.v[6][c0 + q] for q in range(16))
        seen2.add(key)
    if not rm: fix_clash(sc, pcs, stats)
    g._UPSTATS = getattr(g, '_UPSTATS', {})
    for k_, v_ in stats.items(): g._UPSTATS[k_] = g._UPSTATS.get(k_, 0) + v_
    return sc
