# Reads the visible tracks of BORE's music (tools/*.xm of the GBA game) and pulls out what GodGuy's music is amalgamated from:
# the tempo, key and mode of each song, its bass line and its most repeated hooks (one or two bars on a 16th-note grid).
import sys, os, collections, json
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
import xm
TR = os.path.join(os.path.dirname(__file__), '..', '..', 'tools')
SONGS = "earth_and_the_space_citizens amiga_music emergency_hitech tree_swaying_action meltdown_in_mars_house worthless_clouds sunman_sunrise gottcho_barracho_ii spanish_flexicode mi_cora_zone excuses_house whistler_shuffle cynicaller_dnb hotdamn_rave aim_and_shoot magic_act nursery_time tree_fast condensed_music cocaine_cola_ii staged closer_to_the_end the_ticking_bomb quickbullet over_my_voice ohio_hoe".split()
MAXROWS = 6000

def grid_rpb(secrow):
    for rpb in (4, 8, 16, 32, 64, 2, 6, 12, 24, 48):
        bpm = 60.0 / (secrow * rpb)
        if 76 <= bpm <= 176: return rpb, bpm
    return 4, 60.0 / (secrow * 4)
DRUMWORDS = ('kick', 'snare', 'hat', 'ride', 'crash', 'tom', 'clap', 'perc', 'shaker', 'tamb', 'cymbal', 'drum', 'conga', 'bongo', 'rim', 'cowbell', 'noise', 'fx', 'boom', 'darbuka', 'tabla')
def inst_is_drum(ins):
    nm = ins['name'].lower()
    if any(w in nm for w in DRUMWORDS): return True
    if ins['samples']:
        sm = ins['samples'][0]
        if len(sm['data']) < 3500 and sm['ll'] <= 2: return True
    return False

def analyze(name):
    s = xm.parse(os.path.join(TR, name + '.xm'))
    secrow = s['tempo'] * 2.5 / s['bpm']
    rpb, bpm = grid_rpb(secrow)
    step = rpb / 4.0                       # song rows per 16th note
    chans = [dict() for _ in range(s['nch'])]; insuse = [collections.Counter() for _ in range(s['nch'])]
    absr = 0
    for o in s['order']:
        pat = s['pats'][o]
        for r, row in enumerate(pat):
            for c, cell in enumerate(row):
                n = cell[0]
                if 1 <= n <= 96:
                    g = int(round((absr + r) / step))
                    chans[c].setdefault(g, n + 11)      # midi
                    if cell[1]: insuse[c][cell[1] - 1] += 1
        absr += len(pat)
        if absr > MAXROWS: break
    info = dict(name=name, bpm=round(bpm), rpb=rpb, chans=[])
    allp = collections.Counter()
    for c, d in enumerate(chans):
        if len(d) < 12: continue
        ps = list(d.values()); dist = len(set(ps)); mean = sum(ps) / len(ps)
        top = insuse[c].most_common(1); isd = bool(top) and top[0][0] < len(s['insts']) and inst_is_drum(s['insts'][top[0][0]])
        kind = 'drum' if (isd or dist <= 1) else ('bass' if mean < 52 else 'mel')
        info['chans'].append(dict(c=c, n=len(d), dist=dist, mean=round(mean, 1), kind=kind))
        if kind != 'drum':
            for p in ps: allp[p % 12] += 1
    # key: the pitch class that best fits a major or minor triad over the histogram
    best = (-1, 0, 'min')
    for k in range(12):
        for mode, tri in (('maj', (0, 4, 7)), ('min', (0, 3, 7))):
            sc = sum(allp[(k + t) % 12] * (2 if t == 0 else 1) for t in tri) + sum(allp[(k + t) % 12] for t in ((0, 2, 4, 5, 7, 9, 11) if mode == 'maj' else (0, 2, 3, 5, 7, 8, 10))) * 0.5
            if sc > best[0]: best = (sc, k, mode)
    info['key'] = best[1]; info['mode'] = best[2]
    # hooks: windows of 32 cells (two bars) that repeat, in non-drum channels
    hooks = []
    for ch in info['chans']:
        if ch['kind'] == 'drum': continue
        d = chans[ch['c']]; maxg = max(d)
        seen = collections.Counter(); where = {}
        for w0 in range(0, maxg, 16):
            for L in (32, 16):
                cells = [(g - w0, d[g]) for g in range(w0, w0 + L) if g in d]
                if len(cells) < 5: continue
                base = cells[0][1]; key = tuple((g, p - base) for g, p in cells)
                if len(set(p for g, p in cells)) < (2 if ch['kind'] == 'bass' else 3): continue
                seen[(L, key)] += 1; where.setdefault((L, key), (w0, base))
        for (L, key), cnt in seen.items():
            if cnt >= 2: hooks.append(dict(c=ch['c'], kind=ch['kind'], len=L, count=cnt, score=cnt * len(key) * (1.3 if L == 32 else 1.0), base=where[(L, key)][1],
                                          notes=[[g, rel] for g, rel in key]))
    hooks.sort(key=lambda h: -h['score'])
    info['hooks'] = hooks[:40]
    return info

if __name__ == '__main__':
    out = {}
    for n in SONGS:
        a = analyze(n); out[n] = a
        mel = [h for h in a['hooks'] if h['kind'] == 'mel']; nd = sum(1 for c in a['chans'] if c['kind'] == 'drum'); bas = [h for h in a['hooks'] if h['kind'] == 'bass']
        print('%-28s %3d bpm key %2d %s  chans %2d  hooks mel %2d bass %2d drumch %d best %s' % (n, a['bpm'], a['key'], a['mode'], len(a['chans']), len(mel), len(bas), nd, [(h['len'], h['count'], len(h['notes'])) for h in a['hooks'][:3]]))
    json.dump(out, open(os.path.join(os.path.dirname(__file__), 'gm_analysis.json'), 'w'))
