#!/usr/bin/env python3
"""Turn tracker songs (.xm) into note data + small samples for the GBA jukebox (source/musicdata.h).

usage:  python3 tools/xm2gba.py          (run from the project root)

Which songs get converted is read from source/songs.h - every line like
        SONG_XM(my_song,"MY SONG","tools/my_song.xm")
is converted. The title-screen song (tools/the_dipper_man.xm) is always included, because the title screen plays it.
Needs numpy and scipy   (Termux: pkg install python-numpy python-scipy)

What the player supports: up to 10 channels, up to 32 instruments per song, any pattern length, notes, the volume column
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
OUT = "source/musicdata.h"
TITLE = ("the_dipper_man", "tools/the_dipper_man.xm")
GAIN = {"amiga_music": 1.1, "earth_and_the_space_citizens": 2.0}   # louder/quieter per song (default 1.0), so every tracker song sits at a similar level
LOOP_OVERRIDE = {"the_dipper_man": 4, "amiga_music": 0}   # the title song plays its intro once, then loops from order 4 (others loop from the XM restart position)

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

ENDINGS = {"amiga_music": make_ending}
POPS = {"amiga_music": make_pop}

def song_list():
    text = open(SONGS_H).read() if os.path.exists(SONGS_H) else ""
    found = re.findall(r'^\s*SONG_XM\(\s*(\w+)\s*,\s*"[^"]*"\s*,\s*"([^"]+)"\s*\)', text, re.M)
    return [TITLE] + [s for s in found if s[0] != TITLE[0]]

def convert_samples(S, used):
    """one entry per instrument: dict(q = samples incl. guard, pk, svol, steps) - or None for unused instruments"""
    insts = []; total = 0
    for k, I in enumerate(S['insts']):
        if (k + 1) not in used or not I['samples']:
            insts.append(None); continue
        s = I['samples'][0]; x = np.array(s['data'])
        if s['type'] != 0: print("  WARNING: instrument %d has a looping sample (loops are not played)" % (k + 1))
        period = 7680 - (0 + s['rel']) * 64 - s['fine'] / 2 - 48 * 64; fc4 = 8363 * 2 ** ((4608 - period) / 768)
        X = np.abs(np.fft.rfft(x)) ** 2; fr = np.fft.rfftfreq(len(x), 1 / fc4); ds = 1
        for d in (4, 2):
            if X[fr > 0.4 * fc4 / d].sum() / X.sum() < 0.004: ds = d; break
        y = signal.resample_poly(x, 1, ds) if ds > 1 else x.copy()
        pk = abs(y).max(); y = y / pk * 0.98 if pk > 0 else y     # normalise; the gain is folded into each note's volume
        q = np.round(y * 127).astype(int); fade = min(48, len(q) // 4); q[-fade:] = (q[-fade:] * np.linspace(1, 0, fade)).astype(int)
        q = np.append(q, 0)                                       # guard sample for interpolation
        base = fc4 / MIXR / ds
        steps = [int(round(base * 2 ** ((n - 49) / 12) * 65536)) for n in range(1, 97)]
        insts.append(dict(q=q, pk=pk, svol=s['vol'], steps=steps)); total += len(q)
        print("  inst %2d  ds %d  len %6d  vol %2d  fc4 %.0f" % (k + 1, ds, len(q), s['vol'], fc4))
    print("  sample bytes", total)
    return insts

def convert(sid, path):
    print("%s  <-  %s" % (sid, path))
    S = parse(path)
    if sid in POPS: POPS[sid](S)
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
    if maxch >= 10: sys.exit("  ERROR: notes on channel %d, the player has channels 1-10" % (maxch + 1))
    if max(used, default=0) > 32: sys.exit("  ERROR: instrument numbers go up to 32")
    if eff: print("  WARNING: %d effect commands are ignored by the player" % eff)
    if offs: print("  WARNING: %d note-off keys are ignored by the player" % offs)
    insts = convert_samples(S, used)
    ninst = len(insts)
    # note events: u32  ch(4) | inst(5)<<4 | note(7)<<9 | volume(7)<<16   (volume = final voice volume, 64 = full sample level, up to 127 with GAIN)
    ev = []; off = []; rows = []
    for p in S['pats']:
        off.append(len(ev)); rows.append(len(p))
        for r in p:
            e = []
            for ch, (n, i, v, _, _) in enumerate(r):
                if n and n < 97 and i - 1 < ninst and insts[i - 1]:
                    I = insts[i - 1]
                    rel = (v - 0x10) / 64 if 0x10 <= v <= 0x50 else 1.0       # the volume column scales the sample's own volume
                    vol = min(127, max(1, int(round(I['svol'] * I['pk'] * rel * GAIN.get(sid, 1.0)))))
                    e.append(ch | ((i - 1) << 4) | ((n - 1) << 9) | (vol << 16))
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
    arr('u32', P + 'len', [(len(I['q']) - 1) if I else 0 for I in insts])
    seen = {}; names = []
    for k, I in enumerate(insts):
        if not I: names.append('0'); continue
        key = I['q'].tobytes()
        if key in seen: names.append(seen[key]); continue            # identical sample data is stored once
        nm = P + 'S%d' % k; seen[key] = nm; names.append(nm)
        arr('s8', nm, [int(v) for v in I['q']], 32)
    o.append('static const s8* const %sdata[%d]={%s};\n' % (P, ninst, ','.join(names)))
    o.append('static const XmSong xm_%s={%sorder,%srows,%spatOff,%sev,%sstep,%slen,%sdata,%d,%d,%d,%d};\n' % (sid, P, P, P, P, P, P, P, nord, loop, rowN, rfr))
    return ''.join(o)

if __name__ == "__main__":
    songs = song_list()
    head = ['// generated by tools/xm2gba.py from the .xm songs listed in source/songs.h - do not edit\n',
            '// needs before it: u8 u16 u32 s8, and the XmSong struct (main.c)\n',
            '#define MUS_RATE %d\n' % MIXR]
    body = [convert(sid, p) for sid, p in songs]
    open(OUT, 'w').write(''.join(head) + ''.join(body))
    print("wrote %s  (%d KB of source)" % (OUT, os.path.getsize(OUT) // 1024))
