# GodGuy music, set four: (1) the vocal pass that lays the manipulated voice chops (gm_vox.py) over a track, (2) a MIDI reader for the two newest songs (CORPORATIONS, DUCK & COVER), and
# (3) the FAMILIAR set: the 26 original BORE songs again, this time kept close to what they were (their own key, mode and tempo, their own hooks in the order you know them, a plain intro / verse /
# pre-chorus / chorus / verse / chorus / bridge / last chorus / outro shape), with voices on top. Called from gm2_compose.py.
import os, math, random
VOX = {n: i for i, n in enumerate(['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'])}
VOXT = dict(LAUGH=12, YAHOO_REV=13, YEHA=14, SHRIEK=15, CRY=16, FIGHT=17, FLIRT=18, NAH=19, AGREE=20, SNEEZE=21, GHOST=22, WIN=23)
VOX0 = 16
def vox(sc, V_DB, cell, idx, soft=False):
    if 0 <= cell < sc.cells: sc.v[V_DB][cell] = (VOX0 + idx + (128 if soft else 0), 1)
def vox_pass(sc, V_DB, section_starts, rootpc_at, seed):
    """section_starts: [(name, first cell, bars)]; rootpc_at(cell) -> pitch class of the chord there."""
    r = random.Random(seed); TX = ['LAUGH', 'GHOST', 'CRY', 'FLIRT', 'SNEEZE']; r.shuffle(TX); k = 0
    for sec, c0, n in section_starts:
        for b in range(n):
            c = c0 + b * 16; pc = rootpc_at(c)
            if sec == 'intro' and b == n - 1: vox(sc, V_DB, c + 8, VOXT['YAHOO_REV'])
            elif sec in ('verse', 'verse2') and b % 4 == 3: vox(sc, V_DB, c + 12, pc, soft=True)
            elif sec == 'pre':
                if b == n - 1: vox(sc, V_DB, c + 12, VOXT['YEHA'])
                else: vox(sc, V_DB, c + 8, VOXT['FIGHT'])
            elif sec in ('chorus', 'chorus2', 'final'):
                if b % 2 == 0: vox(sc, V_DB, c, pc); vox(sc, V_DB, c + 8, (pc + 7) % 12, soft=True)
                if b % 4 == 3: vox(sc, V_DB, c + 14, VOXT['AGREE'] if sec != 'final' else VOXT['WIN'])
                if sec == 'final' and b == 0: vox(sc, V_DB, c + 4, VOXT['SHRIEK'])
            elif sec == 'break':
                if b == 0: vox(sc, V_DB, c + 4, VOXT[TX[k % len(TX)]]); k += 1
                if b == n - 1: vox(sc, V_DB, c + 12, VOXT['NAH'])
            elif sec == 'bridge':
                if b % 2 == 0: vox(sc, V_DB, c + 6, VOXT[TX[(k + b) % len(TX)]])
                if b == n - 1: vox(sc, V_DB, c + 14, pc)
            elif sec == 'outro':
                if b == 0: vox(sc, V_DB, c + 8, VOXT['SNEEZE'])
                if b == n - 1: vox(sc, V_DB, c, pc, soft=True)
# ---------------------------------------------------------------------------------------------------- MIDI -> score
def key_of(pcs):      # a rough major / minor guess from a pitch class histogram
    MAJP = [6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88]; MINP = [6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17]; best = (-9, 0, 'maj')
    for k in range(12):
        for nm, pr in (('maj', MAJP), ('min', MINP)):
            a = [pcs[(k + i) % 12] for i in range(12)]; ma = sum(a) / 12; mp = sum(pr) / 12
            num = sum((x - ma) * (y - mp) for x, y in zip(a, pr)); den = math.sqrt(sum((x - ma) ** 2 for x in a) * sum((y - mp) ** 2 for y in pr)) or 1
            if num / den > best[0]: best = (num / den, k, nm)
    return best[1], best[2]
def midi_score(path, Sc, V, roles, drum_map, bars_max=None, cell_div=4, echo_lead=False):
    import mido
    m = mido.MidiFile(path); tpb = m.ticks_per_beat; tempo = 500000; ev = []
    for tr in m.tracks:
        t = 0
        for msg in tr:
            t += msg.time
            if msg.type == 'set_tempo' and not ev: tempo = msg.tempo
            if msg.type in ('note_on', 'note_off'): ev.append((t, msg.channel, msg.note, msg.velocity if msg.type == 'note_on' else 0))
    tick_cell = tpb / cell_div; ev.sort(); notes = {}; out = []           # per channel: list of (cell, note, len cells)
    on = {}
    for t, ch, n, v in ev:
        if v > 0: on[(ch, n)] = t
        elif (ch, n) in on:
            t0 = on.pop((ch, n)); out.append((ch, n, int(round(t0 / tick_cell)), max(1, int(round((t - t0) / tick_cell)))))
    cells = max(c + l for _, _, c, l in out); bars = (cells + 15) // 16
    if bars_max: bars = min(bars, bars_max)
    sc = Sc(bars); pcs = [0] * 12
    for ch, n, c, l in out:
        if ch != 9 and c < bars * 16: pcs[n % 12] += l
    kt, mt = key_of(pcs)
    bych = {}
    for ch, n, c, l in out:
        if c >= bars * 16: continue
        bych.setdefault(ch, []).append((c, n, l))
    for ch, lst in bych.items():
        if ch == 9:
            for c, n, l in lst:
                k = drum_map.get(n)
                if not k: continue
                part, did = k; stream = V['DA'] if part == 'A' else V['DB']
                cur = sc.v[stream][c]
                if cur[0] == 0 or (part == 'A' and did in (1, 2) and cur[0] not in (1, 2)): sc.put(stream, c, did, 1)
            continue
        spec = roles.get(ch)
        if spec is None: continue
        role, leg, lo = spec if isinstance(spec, tuple) else (spec, 1, 0)
        byc = {}
        for c, n, l in lst: byc.setdefault(c, []).append((n, l))
        cs = sorted(byc); nxt = {c: (cs[i + 1] if i + 1 < len(cs) else bars * 16) for i, c in enumerate(cs)}
        def hold(c, l): return max(l, min(leg, nxt[c] - c)) if leg > 1 else l       # the transcription's notes are short: let each one ring on (to the next onset, up to leg cells) so it can be heard
        def up(n):
            while lo and n < lo: n += 12
            return n
        if role == 'pad':       # chords: low note left, top note right (the middle one doubles as the harmony voice)
            for c, ns in byc.items():
                ns.sort(); L = hold(c, ns[0][1]); sc.put(V['PADL'], c, up(ns[0][0]), L); sc.put(V['PADR'], c, ns[-1][0], L)
                if len(ns) > 2: sc.put(V['HARM'], c, ns[len(ns) // 2][0], L)
        else:
            vc = V[role]
            for c, ns in byc.items():
                n, l = max(ns) if role not in ('BASS',) else min(ns); L = hold(c, l); sc.put(vc, c, up(n), L)
                if role == 'LEAD' and echo_lead and c + 3 < bars * 16: sc.put(V['ECHO'], c + 3, up(n), min(L, 4))
    tempo_bpm = int(round(60e6 / tempo)); return sc, tempo_bpm, kt, mt
GMD = {36: ('A', 1), 35: ('A', 1), 38: ('A', 2), 40: ('A', 2), 39: ('A', 3), 41: ('A', 4), 43: ('A', 4), 45: ('A', 5), 47: ('A', 5), 48: ('A', 6), 50: ('A', 6), 37: ('A', 7), 42: ('B', 1), 44: ('B', 1), 46: ('B', 2),
       49: ('B', 3), 57: ('B', 3), 51: ('B', 4), 59: ('B', 4), 70: ('B', 5), 69: ('B', 5)}
def midi_track_build(R, Sc, V, A):
    sc, bpm, kt, mt = midi_score(R['midi'], Sc, V, R['roles'], GMD, R.get('maxbars'), echo_lead=R.get('echo', False))
    if R.get('bpm'): bpm = R['bpm']      # (a slower tempo than the file's: every note keeps its place on the grid, so the song plays at that pace)
    # voices: a chop on the first beat of every 8 bars (the chord's note), textures between, an opening swell, a stutter before every 32nd bar
    bass = [(c, sc.v[V['BASS']][c][0]) for c in range(sc.cells) if sc.v[V['BASS']][c][0]]
    def rootpc(c):
        best = 0
        for cc, n in bass:
            if cc <= c: best = n
            else: break
        return best % 12 if best else kt
    r = random.Random(R['id']); TX = ['LAUGH', 'GHOST', 'CRY', 'FLIRT', 'SNEEZE', 'SHRIEK', 'FIGHT']
    nb = sc.cells // 16; own = R.get('ownvox')          # own: (first chop index, count) of the song's OWN voice pieces, then the 24 older chops for colour
    for b in range(nb):
        c = b * 16
        if b == 0: vox(sc, V['DB'], c + 8, VOXT['YAHOO_REV'])
        if own:
            f0, cnt = own
            if b % 4 == 0 and b > 0: vox(sc, V['DB'], c + 6, f0 + (b // 4) % cnt)                        # the song's own voice, a different piece every four bars
            if b % 4 == 2: vox(sc, V['DB'], c + 14, f0 + (b // 4 * 5 + 2) % cnt, soft=True)
            if R.get('chatter') and b % 2 == 1: vox(sc, V['DB'], c + 10, f0 + (b * 3) % cnt, soft=True)   # a rhythmic chatter between
        if b % 8 == 0 and b > 0: vox(sc, V['DB'], c, rootpc(c)); vox(sc, V['DB'], c + 8, (rootpc(c) + 7) % 12, soft=True)
        if b % 8 == 4 and not own: vox(sc, V['DB'], c + 6, VOXT[TX[(b // 8) % len(TX)]])
        if b % 16 == 15: vox(sc, V['DB'], c + 12, VOXT['YEHA'])
        if b % 32 == 31: vox(sc, V['DB'], c + 14, VOXT['WIN'])
    if R.get('lyrics'):
        import gm_vox as gv; cell_s = 60.0 / bpm / 4; k0 = len(gv.CHOPS) - len(gv.LYR_PAD)
        durs = [int(math.ceil(len(gv.CHOPS[k0 + j][1]) / gv.CRATE[k0 + j] / cell_s)) + 2 for j in range(len(gv.LYR_PAD))]
        duck_lyrics(sc, V, k0, len(gv.CHOPS), gv.LYR_PAD, durs, 16 + len(gv.CHOPS))
    return sc, 0, bpm, kt, mt
# ---------------------------------------------------------------------------------------------------- the familiar set
# source song -> (drums, bass, mood, arp, unlock)
FAM = [
 ('sunman_sunrise', 'half', 'five', 'folk', 'up'), ('tree_swaying_action', 'shuffle', 'walk', 'folk', 'updown'), ('earth_and_the_space_citizens', 'half', 'five', 'orch', 'updown'),
 ('worthless_clouds', 'soft', 'sub', 'glass', 'sparse'), ('amiga_music', 'break', 'offbeat', 'chip', 'trip'), ('emergency_hitech', 'dance', 'pump', 'dance', 'updown'),
 ('meltdown_in_mars_house', 'house', 'pump', 'dance', 'updown'), ('gottcho_barracho_ii', 'latin', 'gallop', 'brass', 'oct'), ('spanish_flexicode', 'latin', 'sync', 'folk', 'broken'),
 ('mi_cora_zone', 'latin', 'walk', 'pop', 'up'), ('excuses_house', 'house', 'sync', 'pop', 'broken'), ('whistler_shuffle', 'shuffle', 'walk', 'folk', 'up'),
 ('cynicaller_dnb', 'dnb', 'gallop', 'dark', 'broken'), ('hotdamn_rave', 'house', 'pump', 'dance', 'updown'), ('aim_and_shoot', 'rock', 'gallop', 'rock', 'oct'),
 ('magic_act', 'waltz', 'waltz', 'glass', 'broken'), ('nursery_time', 'waltz', 'waltz', 'folk', 'sparse'), ('tree_fast', 'dance', 'pump', 'pop', 'updown'),
 ('condensed_music', 'break', 'offbeat', 'chip', 'trip'), ('cocaine_cola_ii', 'rock', 'sync', 'rock', 'oct'), ('staged', 'half', 'walk', 'brass', 'up'),
 ('closer_to_the_end', 'march', 'five', 'orch', 'up'), ('the_ticking_bomb', 'march', 'gallop', 'dark', 'oct'), ('quickbullet', 'rock', 'gallop', 'rock', 'oct'),
 ('over_my_voice', 'half', 'sub', 'glass', 'sparse'), ('ohio_hoe', 'dance', 'pump', 'brass', 'updown'),
]
def fam_tracks(A):
    out = []
    for n, (src, dr, bs, mood, arp) in enumerate(FAM):
        a = A[src]; nm = src.upper().replace('_', ' ')
        out.append(dict(id='fam%d' % n, name=nm, use='hidden: unlocked in story mode', a=src, ra=0, b=src, rb=1, c=src, rc=2, key=a['key'], mode=a['mode'], bpm=max(70, min(180, a['bpm'])),
                        form='song', bass=bs, drums=dr, mood=mood, arp=arp, lift=0, cup=0, inv=False, vox=True))
    return out

# ---------------------------------------------------------------------------------------------------- DUCK & COVER's lyrics, on top of everything
def duck_lyrics(sc, V, kit_lyr0, kit_n, lyr_pads, lyr_beats_cells, voxe0):
    """The Koala project's vocal pads, each where the project played it (its bar and beat; the song keeps the project's bars, so the vocals land on the same bars at the new tempo), twice:
    the dry voice on one side and, one cell (a sixteenth) later, a copy of it on the other side, quieter. The hats are taken out while a phrase sounds so the voice has the channels to itself."""
    import json
    ev = json.load(open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'tools', 'duckcover', 'lyric_events.json')))['events']
    placed = 0; busy_until = -1
    for e in ev:
        if e['pad'] not in lyr_pads: continue
        j = lyr_pads.index(e['pad']); c = int(round(e['t'] * 4))
        if c < busy_until or c + 2 >= sc.cells: continue          # two phrases never overlap (the player has two voice channels for them)
        dur = lyr_beats_cells[j]
        sc.v[V['DB']][c] = (16 + kit_lyr0 + j, 1); sc.v[V['DB']][c + 1] = (voxe0 + 2 * j + 1, 1)
        dur = max(8, dur - 8)      # (the last second or so is only the plate tail: the next phrase may begin over it)
        for cc in range(c + 2, min(sc.cells, c + 2 + dur)):
            sc.v[V['DB']][cc] = (0, 0)      # (nothing else on the cymbal / voice channels while it sings)
        busy_until = c + 2 + dur; placed += 1
    return placed
