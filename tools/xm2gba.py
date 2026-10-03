#!/usr/bin/env python3
"""Turn tracker songs (.xm) into note data + small samples for the GBA jukebox (source/musicdata.h).

usage:  python3 tools/xm2gba.py          (run from the project root)

Which songs get converted is read from source/songs.h - every line like
        SONG_XM(my_song,"MY SONG","tools/my_song.xm")
is converted. The title-screen song (tools/the_dipper_man.xm) is always included, because the title screen plays it.
Needs numpy and scipy   (Termux: pkg install python-numpy python-scipy)

What the player supports: up to 16 channels, up to 32 instruments per song, any pattern length, notes, the volume column
(set volume 0x10-0x50), and a fixed speed/BPM per song. It ignores effect commands, panning, envelopes and note-off, and
this script prints a WARNING when a song uses any of them.
"""
import sys, os, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
from xm import parse

MIXR = 18157                      # the game's music rate (MUS_RATE)
SONGS_H = "source/songs.h"
QREG = {}                         # name -> sample data of every sample stored so far (to spot near-copies)
NEARDUP = {"worthless_clouds"}    # songs whose near-identical samples are merged with earlier ones (the others are left as they were)
SHARED = {}                       # sample data already written for an earlier song: identical samples are stored once in the whole ROM
OUT = "source/musicdata.h"
TITLE = ("the_dipper_man", "tools/the_dipper_man.xm")
GAIN = {"tree_swaying_action": 0.9, "amiga_music": 1.1, "earth_and_the_space_citizens": 2.0, "meltdown_in_mars_house": 1.8, "sunman_sunrise": 1.6, "gottcho_barracho": 1.85, "spanish_flexicode": 1.7, "gottcho_barracho_ii": 1.7, "mi_cora_zone": 1.5, "emergency_hitech": 1.6, "excuses_house": 1.9, "whistler_shuffle": 2.2, "whistler_shuffle_old": 2.2, "worthless_clouds": 1.15}   # louder/quieter per song (default 1.0), so every tracker song sits at a similar level
LOOP_OVERRIDE = {"the_dipper_man": 4, "amiga_music": 0, "emergency_dance_floor": 0, "tree_swaying_action": 0}   # the title song plays its intro once, then loops from order 4 (others loop from the XM restart position)

def make_ending(S):
    """New ending for the Amiga Music song: the song's old tail (orders 15-16) is replaced by a generated breakdown + fade-out.
    Breakdown: pattern 4 (full groove) loses channels one by one (melody+bass, then bass+drum, then bass alone).
    Fade: the same pattern repeated 6 times, every note's volume column stepped down 0x50 -> 0x12, then a short silent tail
    so the last notes ring out before the jukebox moves on. Only patterns and the order list are touched."""
    import copy
    pats = S['pats']; full = pats[4]; keep = S['order'][:15]
    def variant(chs, vol, src=full):
        out = []
        for r in src:
            row = []
            for c, (n, i, v, e, ep) in enumerate(r):
                if c in chs and n and n < 97:
                    rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
                    v = 0x10 + max(1, int(round(64 * rel * vol)))
                    row.append((n, i, min(v, 0x50), 0, 0))
                else: row.append((0, 0, 0, 0, 0))
            out.append(row)
        return out
    plan = [((0, 2, 3, 7), 1.0), ((0, 2, 7), 1.0), ((2, 3), 1.0), ((2,), 1.0)]      # the breakdown: strip the arrangement down
    plan += [((2, 7, 3)[:k], 0.7 * (0.62 ** j)) for j, k in enumerate((3, 3, 2, 2, 1, 1))]   # the fade-out
    order = list(keep)
    for chs, vol in plan:
        pats.append(variant(chs, vol)); order.append(len(pats) - 1)
    pats.append([[(0, 0, 0, 0, 0)] * 4 for _ in range(8)]); order.append(len(pats) - 1)   # silent tail
    S['order'] = order

def make_pop(S):
    """POP pass for the Amiga Music song (adds layers on the unused channels 5-9, nothing from the original is removed):
      ch5 four-on-the-floor kick (inst 10), ch6 backbeat clap (inst 15, an octave up so it snaps), ch7 bright off-beat hats with
      ghost 16ths (inst 14), ch8 octave-up shimmer doubling the lead (inst 5), ch9 crash on every groove downbeat (inst 13).
      The 4th pass of the intro becomes a build (kick doubling up + rising snare roll) that drops into the groove, the last groove
      bar before the B section gets a snare-roll fill, and the tempo goes 120 -> 126 BPM. Channel numbers below are 0-based."""
    pats = S['pats']; S['bpm'] = 126
    for p in pats:
        for r in p: r.extend([(0, 0, 0, 0, 0)] * (10 - len(r)))
    def put(p, r, ch, n, i, vol): p[r][ch] = (n, i, 0x10 + max(1, min(64, vol)), 0, 0)
    def groove(p, hats=True, crash=False):
        for r in range(32):
            if r % 4 == 0: put(p, r, 4, 49, 10, 64)                       # kick on every beat
            if r % 8 == 4: put(p, r, 5, 61, 15, 50)                       # clap on 2 and 4
            if hats:
                if r % 4 == 2: put(p, r, 6, 61, 14, 40)                   # off-beat hat
                elif r % 2 == 1: put(p, r, 6, 61, 14, 14)                 # ghost 16th
            n, i, v, _, _ = p[r][2]
            if n and n < 85 and i == 5: put(p, r, 7, n + 12, 5, 26)       # shimmer an octave up
        if crash: put(p, 0, 8, 61, 13, 40)
    def roll(p, start):
        for r in range(start, 32):
            step = 1 if r >= 24 else 2
            if (r - start) % step == 0: put(p, r, 5, 61, 15, 22 + (r - start) * 42 // (32 - start))
            if r % 2 == 0 and r >= 24: put(p, r, 4, 49, 10, 60)
    import copy
    for k in (4, 5): groove(pats[k], crash=(k == 4))
    build = copy.deepcopy(pats[3])                                          # intro bar 4: tension build
    for r in range(0, 32, 8): put(build, r, 4, 49, 10, 64)
    for r in range(16, 32, 4): put(build, r, 4, 49, 10, 64)
    for r in range(8, 32, 4): put(build, r, 6, 61, 14, 30)
    roll(build, 16)
    fill = copy.deepcopy(pats[4]); roll(fill, 24)                           # fill into the B section
    pats.append(build); pats.append(fill)
    order = S['order']; order[7] = len(pats) - 2; order[12] = len(pats) - 1

def make_dance(S):
    """Rework of Emergency On The Dance Floor: the 36 s original (18 orders) becomes a ~105 s song with a real arrangement.
    Layers added on the free channels: ch8 off-beat hats + ghost 16ths (inst 14), ch9 octave-up shimmer on the lead (inst 9);
    ch8 also plays the snare risers. Arrangement: intro -> drop -> breakdown + riser -> second drop (new bar order) ->
    breakdown + riser -> final drop -> breakdown, then a volume fade-out and a silent tail. Only patterns and the order list change.
    Channels are 0-based: ch0/1 bass (inst 8), ch2 lead (9), ch3/5 stabs, ch4 pulse, ch6 snare (12), ch7 kick (11)."""
    import copy
    pats = S['pats']
    for p in pats:
        for r in p: r.extend([(0, 0, 0, 0, 0)] * (10 - len(r)))
    def put(p, r, ch, n, i, vol): p[r][ch] = (n, i, 0x10 + max(1, min(64, vol)), 0, 0)
    def mix(src, chs, vol=1.0):               # keep only some channels, scale their volume
        out = []
        for r in src:
            row = []
            for c, (n, i, v, e, ep) in enumerate(r):
                if c in chs and n and n < 97:
                    rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
                    row.append((n, i, 0x10 + min(64, max(1, int(round(64 * rel * vol)))), 0, 0))
                else: row.append((0, 0, 0, 0, 0))
            out.append(row)
        return out
    def groove(k):
        p = copy.deepcopy(pats[k])
        for r in range(24):
            if r % 4 == 2: put(p, r, 8, 61, 14, 34)
            elif r % 2 == 1: put(p, r, 8, 61, 14, 12)
            n, i, v, _, _ = p[r][2]
            if n and n < 85 and i == 9: put(p, r, 9, n + 12, 9, 14)
        return p
    def riser(src, vol=1.0):                  # build bar: stripped groove + rising snare roll (every 2 rows, then every row)
        p = mix(src, (0, 1, 2, 5), vol)
        for r in range(24):
            if r >= 12 and (r % 2 == 0 or r >= 18): put(p, r, 8, 61, 15, 16 + (r - 12) * 48 // 12)
        for r in range(0, 24, 6): put(p, r, 7, 61, 11, 64)
        return p
    base = len(pats)
    G2, G3 = groove(2), groove(3)
    pats += [G2, G3, mix(pats[3], (0, 1, 2, 3, 5)), mix(pats[1], (0, 1, 5)), riser(pats[1]), riser(pats[3])]
    g2, g3, br3, br1, rs1, rs3 = range(base, base + 6)
    sil = [[(0, 0, 0, 0, 0)] * 10 for _ in range(24)]
    order = [0, 0, 1, 1, 0, 0, 1, 1]                                  # intro (original)
    order += [g2, g2, g3, g3, g2, g2, g3, g3]                         # drop 1
    order += [br3, br3, br1, br1, rs1, rs1]                           # breakdown + riser
    order += [g3, g3, g2, g2, g3, g3, g2, g2]                         # drop 2 (chorus first)
    order += [br1, br1, br3, rs3, rs3]                                # shorter breakdown + riser
    order += [g2, g3, g3, g2, g3, g3, g3, g3]                         # final drop
    order += [br3, br3]                                               # last breakdown
    for j, k in enumerate((3, 3, 2, 2, 1, 1)):                        # fade-out: thinning layers, volume falling
        pats.append(mix(G3, (2, 9, 0, 1)[:k + 1], 0.7 * 0.62 ** j)); order.append(len(pats) - 1)
    pats.append(sil); order.append(len(pats) - 1)                     # silent tail
    S['order'] = order

DANCES = {"emergency_dance_floor": make_dance}
def make_tree(S):
    """Rework of Tree Swaying Action: 67 s -> about 4 minutes, cooler, a KEY CHANGE (+2 semitones) for the last section and a hard
    stop (stinger) instead of a fade.  The XM uses 12 channels but only 7 carry notes, so they are packed into channels 0-6 and the
    free channels 7-9 take the new layers (0-based): ch7 backbeat snare / snare risers / crash, ch8 off-beat hats + ghost 16ths,
    ch9 octave-up shimmer on the melody.  Existing channels after packing: 0/1 pad (inst 1, 2), 2/3 bass (inst 7), 4 pluck bass
    (inst 3), 5 melody (inst 3), 6 drums (inst 11, 12, 16).  The key change moves the pitched instruments only (1, 2, 3, 7);
    drums stay put.  Tempo 50 -> 62 BPM.  Only patterns and the order list change."""
    import copy
    pats = S['pats']; S['bpm'] = 62
    pack = {0: 0, 1: 1, 2: 2, 4: 3, 6: 4, 8: 5, 10: 6}
    for k, p in enumerate(pats):
        for j, r in enumerate(p):
            new = [(0, 0, 0, 0, 0)] * 10
            for c, cell in enumerate(r):
                if cell[0]: new[pack[c]] = cell
            p[j] = new
    PITCHED = (1, 2, 3, 7)
    def put(p, r, ch, n, i, vol): p[r][ch] = (n, i, 0x10 + max(1, min(64, vol)), 0, 0)
    def mix(src, chs, vol=1.0):
        out = []
        for r in src:
            row = []
            for c, (n, i, v, e, ep) in enumerate(r):
                if c in chs and n and n < 97:
                    rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
                    row.append((n, i, 0x10 + min(64, max(1, int(round(64 * rel * vol)))), 0, 0))
                else: row.append((0, 0, 0, 0, 0))
            out.append(row)
        return out
    def transp(src, semis):                                                  # the key change
        return [[(n + semis, i, v, e, ep) if n and n < 97 and i in PITCHED else (n, i, v, e, ep) for (n, i, v, e, ep) in r] for r in src]
    def groove(k, crash=False, shim=True):
        p = copy.deepcopy(pats[k])
        for r in range(32):
            if r % 8 == 4: put(p, r, 7, 61, 13, 44)                          # backbeat snare
            if r % 4 == 2: put(p, r, 8, 61, 15, 30)                          # off-beat hat
            elif r % 2 == 1: put(p, r, 8, 61, 15, 12)                        # ghost 16th
            n, i, v, _, _ = p[r][5]
            if shim and n and n < 85 and i == 3: put(p, r, 9, n + 12, 3, 14) # shimmer an octave above the melody
        if crash: put(p, 0, 7, 61, 14, 36)
        return p
    def riser(src):                                                          # pad + rising snare roll + kick on the beat
        p = mix(src, (0, 1), 1.0)
        for r in range(16, 32):
            if r % 2 == 0 or r >= 26: put(p, r, 7, 61, 13, 14 + (r - 16) * 50 // 16)
        for r in range(0, 32, 8): put(p, r, 6, 80, 12, 64)
        for r in range(24, 32, 2): put(p, r, 6, 80, 12, 64)
        return p
    base = len(pats)
    g1, g2, g3 = groove(1), groove(2), groove(3, crash=True)
    bd = mix(pats[3], (0, 1, 4, 5), 0.9)                                     # breakdown: pads + pluck + melody, no kick / bass
    for r in range(32):
        n, i, v, _, _ = bd[r][5]
        if n and n < 85: put(bd, r, 9, n + 12, 3, 16)
    rise = riser(pats[0])
    t3 = transp(g3, 2); rt = transp(rise, 2)
    st = [[(0, 0, 0, 0, 0)] * 10 for _ in range(16)]                         # the stinger: one big hit in the new key, then silence
    for ch, n, i, v in ((2, 30, 7, 80), (3, 35, 7, 80), (4, 47, 3, 80), (5, 54, 3, 70), (9, 59, 3, 64), (8, 66, 3, 50), (6, 80, 11, 80), (7, 61, 14, 64)):
        put(st, 0, ch, n, i, v)
    pats += [g1, g2, g3, bd, rise, t3, rt, st]
    G1, G2, G3, BD, RS, T3, RT, ST = range(base, base + 8)
    order = [0, 0, 1, G1, 2, G2]                                             # A: intro, layers arrive
    order += [G3] * 4                                                        # B: drop 1
    order += [BD, BD, RS]                                                    # C: breakdown + riser
    order += [G3] * 4                                                        # D: drop 2
    order += [G2, G2, BD, RS]                                                # E: bridge + riser
    order += [G3] * 4                                                        # F: drop 3
    order += [4, RS]                                                         # G: last breakdown (the soft outro bar) + riser
    order += [T3] * 4                                                        # H: KEY CHANGE, final drop
    order += [RT, ST]                                                        # I: riser, then the hard stop
    S['order'] = order

TREES = {"tree_swaying_action": make_tree}
ENDINGS = {"amiga_music": make_ending}
POPS = {"amiga_music": make_pop}

def make_clouds(S):
    """FUNK REWORK of WORTHLESS CLOUDS: the 65-order original (271 s) becomes a ~9.8 minute edit (115 BPM) that no longer loops its two
    verse patterns: the song is rebuilt from the real harmony sections (patterns 10-24, in their original order), played about three times
    with a different energy, drum feel and bass line every time round.  The tune is untouched: the lead (inst 7), the arpeggios (1, 9), the
    pad (5) and the riff (4) are the original notes.  What is new:
      * a syncopated FUNK BASS (ch12, a louder copy of inst 3 = instrument 20, one octave under the pad) that follows the chord roots (read
        from the pad; root / 5th / b7 / octave only, so it can never clash), seven grooves, a turnaround on the last half bar of every pattern
      * off-beat 'shell' STABS (ch13/14, a copy of inst 9 = instrument 17): the 5th and b7 of the chord, four rhythm styles
      * funk KICKS (three syncopated patterns, sub pulse on the same hits) instead of four-on-the-floor; the peaks (energy 4) keep the
        original house kick so the drops still pump; backbeat SNARE with GHOST notes; 16th HATS with accents and open hats
        (copies of inst 14 / 15 = instruments 18 / 19)
      * ENERGY 0-4 per pattern: 0 pad + melodies + a soft bass, 1 + soft hats, 2 + backbeat and stabs, 3 + ghost notes and full stabs,
        4 = house kick on top of everything + crash.  Snare-roll FILLS lead into every louder part, and one layer sits out now and then.
      * SWING: the song runs at 6 rows per 16th (speed 1) so off-beat 16ths sit 58 % late and the snare lays back a hair.
    Everything is picked by seeded random numbers, so the build is repeatable.  Edit RUNS to change the arc: (kind, patterns)
    with S soft / M mid / L loud."""
    import copy, random
    P = S['pats']; O = list(S['order']); Z = (0, 0, 0, 0, 0)
    for p in P:
        for r in p: r.extend([Z] * (16 - len(r)))
    for k in (9, 14, 15, 3):                                   # copies of existing instruments: the new layers get their own pan / mix identity
        S['insts'].append(dict(S['insts'][k - 1]))             # 17 stabs (copy of 9)  18 hats (14)  19 open hat / crash (15)  20 bass (3)
    S['insts'][19]['samples'] = [dict(S['insts'][19]['samples'][0], vol=100)]   # the bass copy plays louder (identical sample data, stored once)
    BASSI, STABI, HATI, OPENI = 20, 17, 18, 19
    def put(p, r, ch, n, i, vol): p[r][ch] = (n, i, 0x10 + max(1, min(64, int(round(vol)))), 0, 0)
    def rel(v): return (v - 0x10) / 64.0 if 0x10 <= v <= 0x50 else 1.0
    def role(c):
        n, i = c[0], c[1]
        if not n or n >= 97: return None
        return {7: 'lead', 1: 'arp', 9: 'ctr', 5: 'pad', 4: 'riff', 12: 'tick', 13: 'snare', 11: 'kick', 14: 'fill', 15: 'fill', 16: 'fill'}.get(i, ('pulse' if n == 61 else 'fill') if i == 10 else 'other')
    def strip(p, roles, r0=0, r1=32):
        for r in range(r0, r1):
            for ch in range(16):
                if role(p[r][ch]) in roles: p[r][ch] = Z
    def scale(p, roles, f):
        for r in range(32):
            for ch in range(16):
                n, i, v, _, _ = p[r][ch]
                if role(p[r][ch]) in roles: p[r][ch] = (n, i, 0x10 + max(1, min(64, int(round(64 * rel(v) * f)))), 0, 0)
    def roots(p):                                              # chord root (pitch class) of each half bar = the lowest pad note
        out = []
        for s in range(4):
            pad = [c[0] for r in range(s * 8, s * 8 + 8) for c in p[r] if 0 < c[0] < 97 and c[1] == 5]
            out.append((min(pad) - 1) % 12 if pad else None)
        last = next((x for x in out if x is not None), 4)      # no pad (the drum / riff patterns): the song's home note, E
        for s in range(4):
            if out[s] is None: out[s] = last
            last = out[s]
        return out
    def bnote(pc):                                             # bass register: the note with this pitch class nearest to note 45
        best = None
        for n in range(36, 58):
            if (n - 1) % 12 == pc and (best is None or abs(n - 45) <= abs(best - 45)): best = n
        return best
    # (row in the half bar, semitones above the root, volume, length in rows)
    BT = [[(0, 0, 64, 3), (3, 0, 42, 1), (6, 12, 50, 1)],
          [(0, 0, 64, 2), (2, 0, 44, 1), (3, 7, 54, 1), (5, 10, 52, 2), (7, 7, 44, 1)],
          [(0, 0, 64, 4), (5, 0, 48, 1), (6, 7, 54, 2)],
          [(0, 0, 64, 1), (1, 12, 38, 1), (3, 0, 56, 1), (4, 0, 64, 2), (7, 7, 44, 1)],
          [(0, 0, 64, 2), (3, 0, 50, 2), (6, 0, 56, 1), (7, 12, 40, 1)],
          [(0, 0, 64, 6)],
          [(0, 0, 64, 2), (2, 7, 50, 1), (4, 0, 60, 2), (7, 10, 48, 1)],
          [(0, 0, 64, 1), (2, 12, 48, 1), (3, 0, 52, 1), (5, 7, 50, 1), (6, 0, 60, 1)]]
    TURN = [(0, 0, 60, 1), (2, 0, 46, 1), (4, 7, 56, 1), (5, 10, 52, 1), (6, 12, 58, 1)]
    GROOVES = [(0, 4), (2, 6), (3, 1), (7, 0), (4, 3), (1, 2), (5, 5)]   # first / second half bar of each bar; the last is the sparse whole-note root
    SPARSE = [6, 6, 2, 6, 5, 6]                                  # the quiet levels: mostly the long root, sometimes a groove
    def add_bass(p, rts, g, vol):
        a, b = GROOVES[g]
        for seg in range(4):
            tpl = TURN if seg == 3 else BT[a if seg % 2 == 0 else b]
            rn = bnote(rts[seg]) - 12; base = seg * 8
            for j, (r, iv, v, ln) in enumerate(tpl):
                put(p, base + r, 12, rn + iv, BASSI, v * vol)
                end = base + (tpl[j + 1][0] if j + 1 < len(tpl) else 8); cut = min(base + r + ln, end)
                if cut < 32 and (j + 1 == len(tpl) or cut < base + tpl[j + 1][0]) and cut > base + r: put(p, cut, 12, rn + iv, BASSI, 1)   # cut tick
    KF = [(0, 7, 10, 16, 19, 26), (0, 3, 10, 14, 16, 22, 26), (0, 6, 10, 16, 18, 23, 26, 30)]
    def add_kick(p, kch, k):
        for r in KF[k]: put(p, r, kch, 49, 11, 64); put(p, r, 11, 61, 10, 38)
    GHOST = [(7, 15, 23, 31), (3, 7, 9, 15, 25, 31), (7, 9, 15, 21, 23, 31)]
    def add_snare(p, sch, rows, vol, ghost, gi):
        for r in rows: put(p, r, sch, 56, 13, vol)
        if ghost:
            for r in GHOST[gi]:
                if r not in rows: put(p, r, sch, 56, 13, 11)
    HACC = [(34, 10, 24, 10, 30, 10, 24, 12), (30, 0, 22, 12, 30, 0, 22, 12), (36, 12, 0, 16, 28, 12, 20, 16)]
    HOPEN = [(14, 30), (6, 22, 30), (15, 31)]
    def add_hats(p, h, level):
        taken = {r for r in range(32) if any(role(c) == 'fill' and c[1] in (14, 15) for c in p[r])}
        for r in range(32):
            if r in taken: continue
            if level == 3:
                if r % 4 == 2: put(p, r, 15, 49, HATI, 16)
            elif level == 1:
                if r % 2 == 0: put(p, r, 15, 49, HATI, 22 if r % 4 == 0 else 13)
            else:
                a = HACC[h][r % 8]
                if a: put(p, r, 15, 56 if a > 22 else 49, HATI, a)
        if level == 2:
            for r in HOPEN[h]: put(p, r, 15, 56, OPENI, 30)
    STAB = [(3, 6, 10, 14), (2, 5, 7, 10, 13), (6, 7, 14, 15), (0, 3, 8, 11, 14)]
    def add_stabs(p, rts, st, level):
        for bar in (0, 1):
            rows = STAB[st] if level >= 2 else STAB[st][:2]
            for j, r in enumerate(rows):
                gr = bar * 16 + r; rn = bnote(rts[gr // 8]); v = 34 if j % 2 == 0 else 24
                put(p, gr, 13, rn + 7, STABI, v); put(p, gr, 14, rn + 10, STABI, v - 4)
    def add_fill(p):
        strip(p, ('kick', 'pulse', 'tick', 'fill', 'snare'), 24, 32)
        for r in range(24, 32):
            for ch in (13, 14, 15): p[r][ch] = Z
            if r < 28 and r % 2: continue
            put(p, r, 8, 56, 13, 24 + (r - 24) * 5)
    LV = {0: dict(strip=('kick', 'pulse', 'snare', 'tick', 'fill'), bass=0.55, hats=0, stab=0, snare=0, ghost=0, lead=0.85),
          1: dict(strip=('kick', 'pulse', 'snare', 'tick'), bass=0.8, hats=1, stab=0, snare=0, ghost=0, lead=1.0),
          2: dict(strip=('snare',), bass=0.9, hats=2, stab=1, snare=44, ghost=0, lead=1.0),
          3: dict(strip=('snare',), bass=1.0, hats=2, stab=2, snare=56, ghost=1, lead=1.0),
          4: dict(strip=('snare',), bass=1.0, hats=2, stab=2, snare=60, ghost=1, lead=1.0)}
    cache = {}
    def variant(src, e, pk, fill, crash, nolead, octv=False):
        feel = 'house' if e == 4 else 'funk'
        key = (src, e, feel, tuple(sorted(pk.items())), fill, crash, nolead, octv)
        if key in cache: return cache[key]
        p = copy.deepcopy(P[src]); lv = LV[e]; rts = roots(p)
        snares = [(r, ch, p[r][ch]) for r in range(32) for ch in range(16) if role(p[r][ch]) == 'snare']
        kicks = [(r, ch) for r in range(32) for ch in range(16) if role(p[r][ch]) == 'kick']
        sch = snares[0][1] if snares else 8; kch = kicks[0][1] if kicks else 10
        strip(p, lv['strip'])
        if e >= 2 and feel == 'funk': strip(p, ('kick', 'pulse')); add_kick(p, kch, pk['K'])
        if lv['snare']: add_snare(p, sch, sorted({r for r, _, _ in snares}) or [4, 12, 20, 28], lv['snare'], lv['ghost'], pk['GH'])
        if lv['lead'] < 1: scale(p, ('lead',), lv['lead'])
        if nolead: strip(p, ('lead',))
        add_bass(p, rts, pk['G'] if e else SPARSE[pk['G']], lv['bass'])
        hl = lv['hats']
        if e == 0 and pk['H'] == 2: hl = 3                          # the quietest level still gets soft off-beat hats now and then
        if e == 1 and pk['H'] == 1: hl = 3                          # level 1: eighth-note hats or off-beat hats only
        if hl: add_hats(p, pk['H'], hl)
        if e == 1 and pk['K'] == 1:                                 # level 1: a light kick on 1 and the 'and' of 3 now and then
            for r in (0, 10, 16, 26): put(p, r, kch, 49, 11, 36); put(p, r, 11, 61, 10, 22)
        stab = lv['stab'] if e >= 2 else (1 if (e == 1 and pk['S'] == 3) else 0)
        if stab and (e >= 3 or pk['S'] % 2 == 1 or e == 1): add_stabs(p, rts, pk['S'], stab)
        if pk['D']: strip(p, (pk['D'],))                            # one layer sits out this pattern
        if fill: add_fill(p)
        if octv:                                                    # the finale: the lead doubled an octave up on a channel this pattern leaves free
            used = [any(0 < p[r][ch][0] < 97 for r in range(32)) for ch in range(16)]
            free = [ch for ch in (2, 4, 5, 7, 10, 6) if not used[ch]]
            if free:
                for r in range(32):
                    for ch in range(16):
                        c = p[r][ch]
                        if role(c) == 'lead' and c[0] + 12 < 97: put(p, r, free[0], c[0] + 12, c[1], 64 * rel(c[2]) * 0.42)
        if crash: put(p, 0, 15, 49, OPENI, 48)
        P.append(p); cache[key] = len(P) - 1
        return cache[key]
    BODY = O[12:54]; END = O[54:65]
    RUNS = [('S', 26), ('L', 4), ('S', 6), ('M', 1), ('L', 29), ('M', 5), ('L', 9), ('M', 1), ('S', 3), ('L', 9), ('M', 6), ('L', 6), ('M', 2), ('S', 10),
            ('M', 4), ('L', 12), ('M', 2)]   # (the second act: a build, the FINALE with the lead doubled an octave up, a breath)
    FINALE = len(RUNS) - 2
    seq = [(8, 1), (8, 1), (8, 2), (8, 2), (9, 2), (9, 2), (9, 2), (9, 3)]   # opening: the drum loop, then the riff + lead, funk groove coming in
    nrun = []                                                   # (pattern, energy, position in its run, run length)
    pi = 0
    for ri, (kind, cnt) in enumerate(RUNS):
        for k in range(cnt):
            if kind == 'S': e = 0 if (ri == 0 and k < 4) else (0 if (ri == len(RUNS) - 1 and k >= cnt - 2) else 1)
            elif kind == 'M': e = 2
            else: e = 4 if (cnt >= 6 and k >= cnt - 4) else 3
            nrun.append((BODY[pi % len(BODY)], e, k, cnt, ri == FINALE)); pi += 1
    es = [e for _, e in seq] + [x[1] for x in nrun] + [3] * 4          # the 4 patterns of the run-in to the ending (25-28) are energy 3
    order = list(O[0:4])
    allp = [(p_, e_, None, None, False) for p_, e_ in seq] + nrun
    for idx, (p_, e_, k, cnt, octv) in enumerate(allp):
        rng = random.Random(1150 + idx // 4 * 7919)             # one set of picks per 4 patterns, so a phrase hangs together and the next one differs
        g0, k0, s0, h0, gh0 = rng.randrange(6), rng.randrange(3), rng.randrange(4), rng.randrange(3), rng.randrange(3)
        q = idx % 4                                              # place in the phrase: bass groove / kick pairs, stab / ghost styles walk on every pattern
        pk = dict(G=(g0 + q // 2 + (q % 2 if e_ <= 1 else 0)) % 6, K=(k0 + q // 2) % 3, S=(s0 + q) % 4, H=(h0 + q % 2) % 3, GH=(gh0 + q) % 3)
        dr = random.Random(31 * idx + 5)
        pk['D'] = dr.choice(('riff', 'ctr', 'arp')) if (e_ <= 1 and dr.random() < 0.4) else (dr.choice(('riff', 'ctr', 'arp', 'tick')) if (e_ in (2, 3) and dr.random() < 0.2) else '')
        nxt = es[idx + 1] if idx + 1 < len(es) else 3
        fill = (e_ >= 1 and nxt > e_) or (e_ >= 2 and idx % 4 == 3)
        crash = idx > 0 and e_ >= 3 and (es[idx - 1] < e_ or (idx - 1) % 4 == 3 and es[idx - 1] >= 2)
        rr = random.Random(77 + idx)
        nolead = e_ in (1, 2, 3) and k is not None and 0 < k < cnt - 1 and rr.random() < (0.2 if e_ == 1 else 0.12) and not fill
        order.append(variant(p_, e_, pk, fill, crash, nolead, octv))
    # ---- the EXPANDED ENDING: the run-in (25-28), then the outro riff (29) four times stepping down in energy (house, funk, groove,
    # hats only), a soft AFTERGLOW (the first four body patterns at the quietest level: pad, arps and the lead), the original coda
    # (31-33), and a last HIT: the outro's first row (lead, riff, pad) with a crash and the bass, ringing out ----
    def pk_(seed): rng = random.Random(seed); return dict(G=rng.randrange(6), K=rng.randrange(3), S=rng.randrange(4), H=rng.randrange(3), GH=rng.randrange(3), D='')
    for j, p_ in enumerate((25, 26, 27, 28)): order.append(variant(p_, 3, pk_(555 + j), j == 3, j == 0, False))
    for j, e_ in enumerate((4, 3, 2, 1)): order.append(variant(29 if j < 3 else 30, e_, pk_(600 + j), e_ >= 2 and j == 2, j == 0, False, j == 0))
    for j in range(4): order.append(variant(BODY[j], 0, pk_(700 + j), False, False, False))
    order.extend([31, 32, 33])
    hit = [[Z] * 16 for _ in range(32)]
    for ch in range(16):
        c = P[29][0][ch]
        if c[0] and c[0] < 97 and role(c) != 'kick': hit[0][ch] = c
    put(hit, 0, 15, 49, OPENI, 56); put(hit, 0, 12, bnote(4) - 12, BASSI, 64)
    P.append(hit); order.append(len(P) - 1)
    # ---- timing: 6 rows per 16th (speed 1); every odd 16th sits 1 row late (58 % swing), the snare lays back 1 row ----
    SUB, SWING = 6, 1
    for pi_, p in enumerate(P):
        q = [[Z] * 16 for _ in range(len(p) * SUB)]
        for r, row in enumerate(p):
            base = r * SUB + (SWING if r % 2 else 0)
            for ch, c in enumerate(row):
                if c[0]: q[base + (1 if c[1] == 13 and ch in (3, 8) else 0)][ch] = c
        P[pi_] = q
    S['tempo'] = 1; S['order'] = order

CLOUDS = {"worthless_clouds": make_clouds}

# ---- STEREO: every note carries a pan bus 0..6 (3 = centre); the player mixes each voice into left and right with that bus' gains ----
NBUS = 7
BUSPAN = [-0.9, -0.6, -0.3, 0.0, 0.3, 0.6, 0.9]           # pan position of each bus (-1 left .. +1 right)
PANK = 1.6                                                  # overall pan gain; the law below is the "-4.5 dB" compromise (constant power x linear),
                                                            # so a panned part keeps its loudness on stereo AND sums well on the mono speaker
def bus_gains(trim=(1.0, 1.0)):
    g = []
    for p in BUSPAN:
        th = (p + 1) * np.pi / 4
        l = PANK * np.sqrt(np.cos(th) * (1 - p) / 2); r = PANK * np.sqrt(np.sin(th) * (1 + p) / 2)
        g.append((int(round(min(1.9, l * trim[0]) * 128)), int(round(min(1.9, r * trim[1]) * 128))))
    return g                                                # (left, right) in 1/128 units

# ---- hand-made pan choreography (overrides the automatic plan for the notes it returns a position for; None = leave it to the plan) ----
def title_pan(pat, row, ch, i, n):
    """THE DIPPER MAN.  Third drop (patterns 20-22, the one where the extra drums come in) is the widest part of the song:
       toms (inst 8) sweep across the field in opposite directions and cross in the middle of every bar, the swoosh (inst 9) throws
       high notes right and low notes left, the click (inst 11) answers on the opposite side, the double hits (inst 16) ping-pong
       on every row, the stab chords swap sides every half bar.  Elsewhere: gentler versions of the same ideas, the lead (inst 3)
       auto-pans in a slow sine, and the bass pair stays close to the middle."""
    d3 = pat in (20, 21, 22); W = 0.9 if d3 else 0.6
    if i == 4: return -0.25 if ch == 0 else 0.25                          # bass pair: a little apart, still centred
    if i == 1:                                                            # stab chords
        side = -1 if ch == 2 else 1
        if d3 and (row // 4) % 2: side = -side                            # drop 3: swap sides every half bar
        return side * W
    if i == 3: return 0.8 * np.sin(2 * np.pi * (row / 16.0 + pat * 0.37))  # lead: each note lands further round the circle
    if i == 8:                                                            # drop-3 toms: two voices sweep L->R and R->L, crossing mid bar
        x = 0.9 * np.cos(np.pi * row / 15.0); return -x if ch == 8 else x
    if i == 9: return 0.7 if n >= 78 else -0.7                            # swoosh: pitch decides the side
    if i == 11: return 0.85 if (row // 2) % 2 == 0 else -0.85             # click answers on the opposite side
    if i == 16:
        if ch == 4: return (-1 if row % 2 == 0 else 1) * W                # double hits ping-pong on every row
        return 0.25 * (-1 if (row // 4) % 2 else 1)
    if i == 15:
        base = {5: -0.55, 6: 0.55, 7: (0.35 if row % 2 else -0.35)}.get(ch, 0.0)
        return base * (W / 0.9 if not d3 else 1.0)
    if i == 14: return -W if row < 8 else W                               # hats cross the field once per bar
    if i == 13: return -0.35 if row < 8 else 0.35
    return None
def clouds_pan(pat, row, ch, i, n):
    """WORTHLESS CLOUDS: the funk bass (instrument 20, a copy of 3) stays in the middle; everything else follows the automatic plan."""
    return 0.0 if i == 20 else None
def flexicode_pan(pat, row, ch, i, n):
    """AN ODE TO THE SPANISH FLEXICODE (tools/make_flexicode_rework.py lays the channels out): the flute sits just left of centre and its echo
       ping-pongs hard left/right on every note, the two nylon voices sit apart, the pads spread left / centre / right, the marimba sweeps
       across the bar, vibes right, bell left, the clave answers the shaker from the right; kick, sub and rim stay in the middle."""
    if ch in (0, 1, 4, 15): return 0.0
    if ch == 2: return -0.5 if row % 4 < 2 else 0.2
    if ch == 3: return 0.55
    if ch in (5, 6, 7): return (-0.8, 0.0, 0.8)[ch - 5]
    if ch == 8: return -0.45
    if ch == 9: return 0.45
    if ch == 10: return -0.12
    if ch == 11: return 0.85 if (row // 3) % 2 == 0 else -0.85
    if ch == 12: return 0.7 * np.sin(2 * np.pi * row / 24.0)
    if ch == 13: return 0.5
    if ch == 14: return -0.5
    return None
def barracho_pan(pat, row, ch, i, n):
    """GOTTCHO BARRACHO (tools/make_barracho_rework.py): the Flexicode layout, except channel 2 is a hat that ping-pongs on every hit
       and channel 3 a shaker on the right."""
    if ch == 2: return 0.6 if (row // 4) % 2 else -0.6
    if ch == 3: return 0.55
    return flexicode_pan(pat, row, ch, i, n)
def hitech_pan(pat, row, ch, i, n):
    """EMERGENCY ON THE DANCE FLOOR (hi-tech, tools/make_emergency_rework.py): kick, snare, bass and sub in the middle; the hat and
       the glitch ticks ping-pong; the lead just left with its octave / echo flying the other way; the stabs spread wide; the arp and
       the data blips sweep across the bar; zaps and fx swap sides."""
    if ch in (0, 1, 4, 5): return 0.0
    if ch == 2: return 0.55 if (row // 2) % 2 else -0.55
    if ch == 3: return -0.8 if (row // 3) % 2 else 0.8
    if ch == 6: return -0.15
    if ch == 7: return 0.85 if (row // 6) % 2 == 0 else -0.85
    if ch == 8: return 0.75 * np.sin(2 * np.pi * row / 48.0)
    if ch in (9, 10, 11): return (-0.8, 0.0, 0.8)[ch - 9]
    if ch == 12: return 0.0
    if ch == 13: return 0.7 if (pat + row // 24) % 2 else -0.7
    if ch == 15: return 0.85 * np.cos(2 * np.pi * row / 24.0)
    return None
def excuses_pan(pat, row, ch, i, n):
    """EXCUSES (tools/make_excuses_rework.py): kick, clap and bass centred; hats and shaker on opposite sides; the stab voices and the
       three pad voices spread wide; the lead a little left with its echo ping-ponging; the arp sweeping slowly across two bars."""
    if ch in (0, 1, 4): return 0.0
    if ch == 2: return -0.45
    if ch == 3: return 0.5
    if ch in (5, 6, 7): return (-0.6, 0.1, 0.7)[ch - 5]
    if ch in (8, 9, 10): return (-0.85, 0.0, 0.85)[ch - 8]
    if ch == 11: return -0.15
    if ch == 12: return 0.85 if (row // 6) % 2 == 0 else -0.85
    if ch == 13: return 0.75 * np.sin(2 * np.pi * row / 64.0)
    if ch == 15: return 0.4
    return None
def whistler_pan(pat, row, ch, i, n):
    """WHISTLER MAN (tools/make_whistler_rework.py): a live band on a stage: kick, snare and bass in the middle, hats right, ghosts a
       touch left, the Rhodes voices spread, clav left, the two horns either side, the whistle centre-left with its echo / harmony
       right, the guitar right, the ride left."""
    if ch in (0, 1, 5): return 0.0
    if ch == 2: return -0.2
    if ch == 3: return 0.45
    if ch == 4: return 0.6
    if ch in (6, 7, 8): return (-0.55, 0.0, 0.55)[ch - 6]
    if ch == 9: return -0.65
    if ch == 10: return -0.4
    if ch == 11: return 0.4
    if ch == 12: return -0.1
    if ch == 13: return 0.6
    if ch == 14: return 0.7
    if ch == 15: return -0.5
    return None
OVERRIDES = {"the_dipper_man": title_pan, "excuses_house": excuses_pan, "whistler_shuffle": whistler_pan, "whistler_shuffle_old": whistler_pan, "emergency_hitech": hitech_pan, "worthless_clouds": clouds_pan, "spanish_flexicode": flexicode_pan, "gottcho_barracho_ii": barracho_pan}

def design_pan(S, used, insts, sid=None):
    """Pan plan for one song. Returns pan(pat, row, ch, inst, note) -> bus.  Rules (a small 'mix engineer'):
      kick / sub / bass (low and short, or low notes)  -> centre.
      long low pads (two or more of them)               -> alternate hard-ish left / right, so pad pairs become a wide bed.
      bright short hits (hats, shakers, ticks)          -> ping-pong left/right on every hit (two such instruments play opposite phases).
      mid noisy hits (snares, claps)                    -> just off centre, alternating sides by instrument.
      melodic instruments on several channels (chords)  -> channels fanned across the field, lowest channel left.
      single-channel melodic instruments                -> take the next slot of  -0.55 +0.55 -0.35 +0.35 -0.75 +0.75, brighter = wider,
                                                           plus a gentle drift with pitch (low notes left, high notes right).
    """
    ev = {}                                                  # inst -> list of (ch, note)
    for o in S['order']:
        for r in S['pats'][o]:
            for ch, (n, i, v, e, ep) in enumerate(r):
                if n and n < 97 and i in used: ev.setdefault(i, []).append((ch, n, v))
    info = {}
    for i in sorted(ev):
        I = insts[i - 1]
        if not I: continue
        q = I['q'].astype(float); st = I['steps'][48] / 65536; fc = st * MIXR
        sec = len(q) / fc; sp = np.abs(np.fft.rfft(q * np.hanning(len(q)))) ** 2; fr = np.fft.rfftfreq(len(q), 1 / fc)
        cen = (sp * fr).sum() / (sp.sum() + 1e-9); zc = (np.diff(np.sign(q)) != 0).mean()
        notes = [n for c, n, v in ev[i]]; chs = sorted(set(c for c, n, v in ev[i])); med = float(np.median(notes))
        cen_eff = cen * 2 ** ((med - 49) / 12.0)                                 # spectral centroid at the pitch the song actually plays it
        if cen_eff < 120: cls = 'low'                                                 # kicks and subs (also long sub-bass: low notes stay in the middle)
        elif med <= 45 and zc < 0.3: cls = 'low'                                  # bass notes
        elif cen < 200 and sec > 2.0: cls = 'pad'                                 # long low-centroid pads
        elif cen > 2000 and sec < 1.0: cls = 'hat'
        elif zc > 0.3 and sec < 0.8: cls = 'snare'
        else: cls = 'mel'
        info[i] = dict(cls=cls, chs=chs, med=med, cen=cen, n=len(notes))
    pos = {}                                                 # (inst, ch) -> base pan
    pads = [i for i in info if info[i]['cls'] == 'pad']
    for k, i in enumerate(pads): pos[(i, None)] = (-0.85 if k % 2 == 0 else 0.85)
    slots = [-0.55, 0.55, -0.35, 0.35, -0.75, 0.75, -0.2, 0.2]; si = 0; sn = 0; ht = 0
    for i in sorted(info, key=lambda k: (-info[k]['n'], k)):
        d = info[i]
        if d['cls'] == 'low': pos[(i, None)] = 0.0
        elif d['cls'] == 'snare': pos[(i, None)] = (0.25 if sn % 2 == 0 else -0.25); sn += 1
        elif d['cls'] == 'hat': pos[(i, 'pp')] = ht; ht += 1
        elif d['cls'] == 'mel':
            if len(d['chs']) >= 2:
                w = 0.8; m = len(d['chs'])
                for k, c in enumerate(d['chs']): pos[(i, c)] = -w + 2 * w * k / (m - 1)
            else:
                pos[(i, None)] = slots[si % len(slots)] * (1.0 + min(0.4, d['cen'] / 4000)); si += 1
    toggle = {}; flip = {}; shift = {}
    def pan(pat, row, ch, i, note):
        b = pan0(pat, row, ch, i, note)
        if flip.get(i, 1) < 0: b = 6 - b
        return max(0, min(6, b + shift.get(i, 0)))
    ov = OVERRIDES.get(sid)
    def pan0(pat, row, ch, i, note):
        d = info.get(i)
        if not d: return 3
        if ov:
            q = ov(pat, row, ch, i, note)
            if q is not None: return int(round(max(-0.9, min(0.9, q)) * 3 / 0.9)) + 3
        if (i, 'pp') in pos:                                 # hats: ping-pong
            k = toggle.get(i, 0); toggle[i] = k + 1
            side = -1 if (k + pos[(i, 'pp')]) % 2 == 0 else 1
            p = 0.8 * side
        else:
            p = pos.get((i, ch), pos.get((i, None), 0.0))
            if d['cls'] == 'mel': p += max(-0.18, min(0.18, (note - d['med']) * 0.012))
        p = max(-0.9, min(0.9, p))
        return int(round(p * 3 / 0.9)) + 3
    # --- balance: weigh how much energy each instrument puts on each side, then mirror whole instruments (never single notes)
    # where that evens the two ears out. The player is stereo, so a song that leans 3 dB left is audible on headphones.
    gl = [g[0] / 128.0 for g in bus_gains()]; gr = [g[1] / 128.0 for g in bus_gains()]
    rowN = S['tempo'] * 2.5 / S['bpm'] * MIXR; seq = []; t = 0           # the song in play order; a new note on a channel cuts the old one
    for o in S['order']:
        for ri, r in enumerate(S['pats'][o]):
            for ch, (n, i, v, e_, ep) in enumerate(r):
                if n and n < 97 and i in info: seq.append([t, ch, i, n, v, o, ri])
            t += 1
    nxt = {}
    for k in range(len(seq) - 1, -1, -1):
        t0, ch = seq[k][0], seq[k][1]; seq[k].append((nxt.get(ch, t + 8) - t0) * rowN); nxt[ch] = t0
    ents = {i: [] for i in info}; etot = 1e-9; cums = {}
    for t0, ch, i, n, v, o_, r_, room in seq:
        I = insts[i - 1]; q = I['q'].astype(float); st = I['steps'][n - 1] / 65536
        rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0
        cs = cums.get(i)
        if cs is None: cs = cums[i] = np.cumsum(q ** 2)
        e = (I['svol'] * I['pk'] * rel) ** 2 * cs[min(len(q) - 1, int(room * st))] / st      # energy of the part of the sample that actually plays
        ents[i].append((e, pan0(o_, r_, ch, i, n))); etot += e
    def contrib(i, f, sh):
        c = 0.0
        for e, b0 in ents[i]:
            b = max(0, min(6, (6 - b0 if f < 0 else b0) + sh)); c += e * (gl[b] ** 2 - gr[b] ** 2)
        return c
    movable = [] if ov else [i for i in info if info[i]['cls'] != 'low']   # (a hand-made plan is not mirrored; the trim below balances it)
    cur = {i: contrib(i, 1, 0) for i in movable}; D = sum(cur.values())
    # balance: weigh how much energy each instrument puts on each side and mirror whole instruments (left <-> right) where that evens
    # the two ears out. The player is stereo, so a song that leans 2-3 dB to one side is plainly audible on headphones.
    for i in sorted(movable, key=lambda k: -abs(cur[k])):
        if abs(D - 2 * cur[i]) < abs(D): flip[i] = -1; D -= 2 * cur[i]; cur[i] = -cur[i]
    toggle.clear()
    # whatever imbalance is left is removed with a small per-song trim on the bus gains (the pan layout itself stays as designed)
    tot = sum(e * (gl[b] ** 2 + gr[b] ** 2) for i in info for e, b in ents[i])      # total energy over both ears
    ratio = max(0.5, min(2.0, (tot + D) / (tot - D + 1e-9)))     # left energy / right energy
    pan.trim = (ratio ** -0.25, ratio ** 0.25)                      # amplitude factors for the left and right gains
    pan.balance_db = 10 * np.log10(ratio)
    pan.info = info
    return pan

def song_list():
    text = open(SONGS_H).read() if os.path.exists(SONGS_H) else ""
    found = re.findall(r'^\s*SONG_XM\(\s*(\w+)\s*,\s*"[^"]*"\s*,\s*"([^"]+)"\s*\)', text, re.M)
    return [TITLE] + [s for s in found if s[0] != TITLE[0]]

# ---- BAR FIT: long chord / loop samples that were recorded as exactly one bar at some tempo ----
# Amiga Music's three chord samples (instruments 1-3) are one-bar loops recorded at 120 BPM (exactly 4.000 s when played at C-5), but the
# pop pass raises the song to 126 BPM (a bar is 3.81 s).  Played as they were, every chord hit ran 5 % slow against the drums and was
# cut off 0.2 s early by the next bar's hit, mid-pulse.  So the converter time-stretches the first bar of each of those samples to the
# song's real bar length (pitch unchanged), and keeps any real decaying tail that rings past the bar line.
#   song -> (tempo the samples were recorded at, rows per bar, the instruments)
FITBAR = {"amiga_music": (120.0, 32, (1, 2, 3))}

def wsola_fit(x, n_out, fs, win_ms=36.0, tol_ms=6.0):
    """Shrink / stretch x to exactly n_out samples WITHOUT changing its pitch (waveform-similarity overlap-add).  fs = the rate x is
    played at (only used to size the window).  The attack at the very start is kept untouched (the first frame is not faded in), and
    the frames are placed so that every pulse of the original lands at the same relative position in the result."""
    x = np.asarray(x, float); n_in = len(x); win = int(fs * win_ms / 1000) // 4 * 4; ha = win // 2; tol = int(fs * tol_ms / 1000)
    ratio = n_out / n_in; w = np.hanning(win + 1)[:win]; w1 = w.copy(); w1[:ha] = 1.0           # frame 0: no fade-in
    nfr = int(np.ceil((n_out - win) / ha)) + 2
    pad = np.concatenate([np.zeros(tol), x, np.zeros(win + 2 * tol + ha)])
    out = np.zeros(nfr * ha + win); norm = np.zeros_like(out); prev = 0                         # prev = analysis start of the last frame
    for k in range(nfr):
        nom = int(round(k * ha / ratio))
        if k == 0: a = 0
        else:
            nat = pad[tol + prev + ha: tol + prev + ha + win]                                   # what would naturally follow the last frame
            best, a = -1e30, nom
            lo, hi = max(-tol, -nom), tol
            cand = np.arange(lo, hi + 1)
            idx = tol + nom + cand[:, None] + np.arange(0, win, 2)[None, :]                     # every 2nd sample is plenty for the match
            seg = pad[np.clip(idx, 0, len(pad) - 1)]; ref = nat[::2]
            sc = (seg * ref).sum(1) / (np.sqrt((seg ** 2).sum(1) * (ref ** 2).sum()) + 1e-9)
            a = nom + int(cand[int(np.argmax(sc))])
        fr = pad[tol + a: tol + a + win] * (w1 if k == 0 else w)
        out[k * ha: k * ha + win] += fr; norm[k * ha: k * ha + win] += (w1 if k == 0 else w); prev = a
    norm[norm < 1e-6] = 1.0
    return (out / norm)[:n_out]

def fit_bar(x, bar_src, bar_dst, fc, keep_tail=0.1):
    """x = one instrument's sample, fc = the rate (Hz) it plays at.  The first bar_src seconds are stretched to bar_dst seconds; what is
    left after the bar line is kept as it is if it is a real tail (longer than keep_tail s), otherwise dropped (it is only the first
    few ms of the next bar's hit, which the next hit replaces anyway)."""
    n_src = int(round(bar_src * fc)); n_dst = int(round(bar_dst * fc))
    if n_src > len(x): n_src = len(x)
    bar = wsola_fit(x[:n_src], int(round(n_dst * n_src / int(round(bar_src * fc)))), fc)
    rest = np.asarray(x[n_src:], float)
    if len(rest) > keep_tail * fc:                                                                  # a real ring-out: join it on with a short crossfade
        L = int(0.006 * fc); a = bar[-L:]; tol = int(0.004 * fc); best, d = -2, 0
        for dd in range(-tol, tol + 1):
            lo = n_src - L + dd
            if lo < 0 or lo + L > len(x): continue
            b = x[lo: lo + L]; sc = (a * b).sum() / (np.sqrt((a * a).sum() * (b * b).sum()) + 1e-9)
            if sc > best: best, d = sc, dd
        j = n_src + d; fo = np.linspace(1, 0, L)
        return np.concatenate([bar[:-L], bar[-L:] * fo + x[j - L: j] * (1 - fo), x[j:]])
    F = int(0.008 * fc); bar[-F:] *= 0.5 * (1 + np.cos(np.linspace(0, np.pi, F)))                   # tiny fade so the retrigger can not click
    return bar

def convert_samples(S, used, sid=None):
    """one entry per instrument: dict(q = samples incl. guard, pk, svol, steps) - or None for unused instruments"""
    insts = []; total = 0
    for k, I in enumerate(S['insts']):
        if (k + 1) not in used or not I['samples']:
            insts.append(None); continue
        s = I['samples'][0]; x = np.array(s['data'])
        if s['type'] != 0: print("  WARNING: instrument %d has a looping sample (loops are not played)" % (k + 1))
        period = 7680 - (0 + s['rel']) * 64 - s['fine'] / 2 - 48 * 64; fc4 = 8363 * 2 ** ((4608 - period) / 768)
        if sid in FITBAR and (k + 1) in FITBAR[sid][2]:               # one-bar chord loop: stretch it to this song's bar (see FITBAR)
            src_bpm, rows, _ = FITBAR[sid]
            nts = [n for o in S['order'] for r in S['pats'][o] for (n, i, v, e, ep) in r if i == k + 1 and 0 < n < 97]
            fcp = fc4 * 2 ** ((int(np.median(nts)) - 49) / 12) if nts else fc4                        # the rate it is actually played at
            bar_src = rows * S['tempo'] * 2.5 / src_bpm; bar_dst = rows * S['tempo'] * 2.5 / S['bpm']
            n0 = len(x); x = fit_bar(x, bar_src, bar_dst, fcp)
            print("  inst %2d fitted to the bar: %.3f s -> %.3f s (%d -> %d samples)" % (k + 1, bar_src, bar_dst, n0, len(x)))
        X = np.abs(np.fft.rfft(x)) ** 2; fr = np.fft.rfftfreq(len(x), 1 / fc4); ds = 1
        for d in (4, 2):
            if X[fr > 0.4 * fc4 / d].sum() / X.sum() < 0.004: ds = d; break
        y = signal.resample_poly(x, 1, ds) if ds > 1 else x.copy()
        pk = abs(y).max(); y = y / pk * 0.98 if pk > 0 else y     # normalise; the gain is folded into each note's volume
        q = np.round(y * 127).astype(int); fade = min(48, len(q) // 4); q[-fade:] = (q[-fade:] * np.linspace(1, 0, fade)).astype(int)
        k = len(q)
        while k > 16 and abs(q[k - 1]) <= 1: k -= 1               # cut the inaudible tail (values -1..1 after the fade)
        q = np.append(q[:k], 0)                                   # guard sample for interpolation
        base = fc4 / MIXR / ds
        steps = [int(round(base * 2 ** ((n - 49) / 12) * 65536)) for n in range(1, 97)]
        insts.append(dict(q=q, pk=pk, svol=s['vol'], steps=steps)); total += len(q)
        print("  inst %2d  ds %d  len %6d  vol %2d  fc4 %.0f" % (k + 1, ds, len(q), s['vol'], fc4))
    print("  sample bytes", total)
    return insts

def convert(sid, path):
    print("%s  <-  %s" % (sid, path))
    S = parse(path)
    if sid in DANCES: DANCES[sid](S)
    if sid in TREES: TREES[sid](S)
    if sid in POPS: POPS[sid](S)
    if sid in CLOUDS: CLOUDS[sid](S)
    if sid in ENDINGS: ENDINGS[sid](S)
    d = open(path, 'rb').read()
    flags = int.from_bytes(d[74:76], 'little')
    if not flags & 1: sys.exit("  ERROR: %s uses Amiga frequencies; save it with linear frequencies" % path)
    nord = len(S['order'])
    used = set(); maxch = 0; eff = 0; offs = 0     # what the order list really uses
    for o in S['order']:
        for r in S['pats'][o]:
            for ch, (n, i, v, e, ep) in enumerate(r):
                if e or ep: eff += 1
                if n == 97: offs += 1
                if n and n < 97:
                    if not i: sys.exit("  ERROR: a note without an instrument number in pattern %d (put the instrument in every note)" % o)
                    used.add(i); maxch = max(maxch, ch)
    if maxch >= 16: sys.exit("  ERROR: notes on channel %d, the player has channels 1-16" % (maxch + 1))
    if max(used, default=0) > 32: sys.exit("  ERROR: instrument numbers go up to 32")
    if eff: print("  WARNING: %d effect commands are ignored by the player" % eff)
    if offs: print("  WARNING: %d note-off keys are ignored by the player" % offs)
    insts = convert_samples(S, used, sid)
    ninst = len(insts)
    panf = design_pan(S, used, insts, sid)
    # note events: u32  ch(4) | inst(5)<<4 | note(7)<<9 | volume(7)<<16 | pan bus(3)<<23   (volume = final voice volume, 64 = full sample level, up to 127 with GAIN)
    ev = []; off = []; rows = []
    for pi, p in enumerate(S['pats']):
        off.append(len(ev)); rows.append(len(p))
        for ri, r in enumerate(p):
            e = []
            for ch, (n, i, v, _, _) in enumerate(r):
                if n and n < 97 and i - 1 < ninst and insts[i - 1]:
                    I = insts[i - 1]
                    rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0       # the volume column scales the sample's own volume
                    vol = min(127, max(1, int(round(I['svol'] * I['pk'] * rel * GAIN.get(sid, 1.0)))))
                    e.append(ch | ((i - 1) << 4) | ((n - 1) << 9) | (vol << 16) | (panf(pi, ri, ch, i, n) << 23))
            ev.append(len(e)); ev.extend(e)
    rowsec = S['tempo'] * 2.5 / S['bpm']                       # speed ticks per row, 2.5/bpm seconds per tick
    rowN = int(rowsec * MIXR); rfr = int(round((rowsec * MIXR - rowN) * 256))
    loop = LOOP_OVERRIDE.get(sid, S['restart'])
    if loop >= nord: loop = 0
    secs = sum(rows[o] for o in S['order']) * rowsec
    print("  %d orders, %d patterns, %.0f s, row %.3f s, loops from order %d" % (nord, len(S['pats']), secs, rowsec, loop))
    o = []
    def arr(t, nm, vals, w=24):
        o.append('static const %s %s[%d]={' % (t, nm, len(vals)))
        for j in range(0, len(vals), w): o.append(','.join(str(v) for v in vals[j:j + w]) + ',\n')
        o.append('};\n')
    P = 'xm_%s_' % sid
    o.append('// ---- %s  (from %s) ----\n' % (sid, path))
    arr('u8', P + 'order', S['order'], 30); arr('u16', P + 'rows', rows, 30); arr('u32', P + 'patOff', off); arr('u32', P + 'ev', ev, 12)
    arr('u32', P + 'step', [s for I in insts for s in (I['steps'] if I else [0] * 96)], 8)
    seen = SHARED; names = []; fresh = []
    for k, I in enumerate(insts):
        if not I: names.append('0'); continue
        key = I['q'].tobytes()
        if key in seen: names.append(seen[key]); continue            # identical sample data is stored once
        if sid in NEARDUP:                                           # a near-copy of an earlier sample (same sound, trimmed a few samples
            q = I['q']; hit = None                                   # differently) points at that sample instead of being stored again
            for nm0, w in QREG.items():
                n = min(len(q), len(w))
                if n < 64 or abs(len(q) - len(w)) > 0.02 * max(len(q), len(w)): continue
                if np.corrcoef(q[:n - 1], w[:n - 1])[0, 1] > 0.985: hit = (nm0, n); break
            if hit:
                nm0, n = hit; I['q'] = np.append(q[:n - 1], 0); names.append(nm0)
                print("  inst %2d is a copy of %s - reusing it" % (k + 1, nm0)); continue
        nm = P + 'S%d' % k; seen[key] = nm; QREG[nm] = I['q']; names.append(nm); fresh.append((nm, I['q']))
    arr('u32', P + 'len', [(len(I['q']) - 1) if I else 0 for I in insts])
    for nm, q in fresh: arr('s8', nm, [int(v) for v in q], 32)
    o.append('static const s8* const %sdata[%d]={%s};\n' % (P, ninst, ','.join(names)))
    bg = bus_gains(panf.trim); print('  stereo balance %.2f dB (L-R) before trim' % panf.balance_db)
    arr('u8', P + 'busL', [g[0] for g in bg]); arr('u8', P + 'busR', [g[1] for g in bg])   # pan bus gains, 128 = 1.0
    o.append('static const XmSong xm_%s={%sorder,%srows,%spatOff,%sev,%sstep,%slen,%sdata,%sbusL,%sbusR,%d,%d,%d,%d};\n' % (sid, P, P, P, P, P, P, P, P, P, nord, loop, rowN, rfr))
    return ''.join(o)

if __name__ == "__main__":
    songs = song_list()
    head = ['// generated by tools/xm2gba.py from the .xm songs listed in source/songs.h - do not edit\n',
            '// needs before it: u8 u16 u32 s8, and the XmSong struct (main.c)\n',
            '#define MUS_RATE %d\n' % MIXR]
    body = [convert(sid, p) for sid, p in songs]
    open(OUT, 'w').write(''.join(head) + ''.join(body))
    print("wrote %s  (%d KB of source)" % (OUT, os.path.getsize(OUT) // 1024))
