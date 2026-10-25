#!/usr/bin/env python3
"""HERE TODAY (MISSY'S SONG) and HERE TODAY (FULL VERSION), SEQUENCED: tracker modules for the GBA player instead of 1.4 MB and 2.6 MB of streamed ADPCM.
  tools/here_today.xm        Missy's song (the scene-5 cutscene): the author's MIDI (tools/here_today.mid) with the tune SUNG (a female "ah", an octave up), a nylon guitar and the string quartet
  tools/here_today_full.xm   the full version (unlocked with the story): intro on a solo cello, verses, bridge, chorus, an instrumental interlude, the chorus again a tone higher,
                             the first ending, THE BRIDGE REPRISE (violin and strings over a heartbeat), the ending again, fuller than before
Both use the instruments of tools/heretoday_kit.py (the same samples as HERE TODAY GONE TOMORROW, so the ROM stores them once); the sung voice is a few samples of "ah" in
lengths that match the note spacing, so a phrase is a string of short vowels, long notes swell and bloom into vibrato, and the scoop into a note is part of each sample.
The cutscene's lyric beats and camera shots (source/cutscene.h, source/csshot.h) are re-timed to this version's phrases (the tracker is a fixed 78 BPM: the old render had the
MIDI's rubato).  Run from the project root:   python3 tools/make_heretoday_here_xm.py     (needs numpy, scipy; about 15 seconds)
Then:  python3 tools/xm2gba.py"""
import sys, os, re, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import heretoday_kit as K
from heretoday_kit import *

ALL = dict(v1=1.0, v2=.9, va=1.0, vc=1.1)
def S(**k): return {n: v * k.get(n, 1.0) for n, v in ALL.items()}
def SC(w): return {n: v * w for n, v in ALL.items()}
LVF = json.load(open(os.environ['LVFILE'])) if os.environ.get('LVFILE') else {}

# ------------------------------------------------------------------------------------------------------------- MISSY'S SONG
TRIMROWS = 4                                                     # the old render started 0.8 s into the MIDI: the first note comes a moment after the music starts
M_SECS = [('song', (TRIMROWS, 750), dict(lv=0.47, vx=1.0, gk='ny', gt=.9, st=S(v1=.5, v2=.5, va=.55, vc=.6)))]

# ------------------------------------------------------------------------------------------------------------- THE FULL VERSION
F_SECS = [
 ('intro',          V1R,  dict(lv=0.48, ramp=(.65, 1.0), ld=.9, ldk='cello', gk='ny', gt=1.0, st=S(), pn=.55, pnp='arp8', pad=.5)),
 ('verse 1',        V1R,  dict(lv=0.61, vx=1.0, gk='ny', gt=1.0, st=SC(.9), pn=.35, pnp='arp8', pad=.5)),
 ('verse 2',        V2R,  dict(lv=0.55, vx=1.0, gk='ny', gt=1.0, st=SC(1.1), pn=.5, pnp='arp8b', pad=.8, bs=.7, bsp='rf')),
 ('bridge',         BRR,  dict(lv=0.52, vx=1.0, gk='ny', gt=.8, st=SC(1.3), pn=.5, pnp='arp8', pad=1.0, bs=.9, bsp='rf', pc='bar')),
 ('chorus',         CHR,  dict(lv=0.62, vx=1.1, gk='ny', gt=.9, st=SC(1.3), pn=.65, pnp='arp8b', pad=1.0, bs=1.0, bsp='walk', bl=.6, blp='bell4', pc='bar')),
 ('interlude',      V2R,  dict(lv=0.48, ld=1.0, ldk='violin', gk='ny', gt=.8, st=SC(1.3), pn=.65, pnp='arp8c', pad=1.0, bs=.8, bsp='rf', bl=.5, blp='bell4')),
 ('chorus 2',       CHR,  dict(lv=0.69, sh=2, vx=1.15, gk='ny', gt=.9, st=SC(1.5), pn=.7, pnp='arp8', pad=1.2, bs=1.0, bsp='walk', bl=1.0, blp='bell4', pc='bar')),
 ('verse 3',        V3R,  dict(lv=0.51, sh=2, vx=1.1, gk='ny', gt=.8, st=SC(1.4), pn=.65, pnp='arp8c', pad=1.1, bs=.8, bsp='rf', bl=.5, blp='bell4')),
 ('first ending',   OUTR, dict(lv=0.47, sh=2, vx=1.1, gk='ny', gt=.7, st=SC(1.5), pn=.5, pnp='arp8', pad=1.2, bs=.6, bsp='pedal', bl=.7, blp='bell2', gap=8)),
 ('BRIDGE REPRISE', BRR,  dict(lv=0.66, sh=2, ramp=(.45, 1.25), ld=1.0, ldk='violin', st=SC(1.5), pn=.65, pnp='arp8b', pad=1.3, bs=.8, bsp='rf', bl=.4, blp='bell4', pc='heart')),
 ('ending again',   OUTR, dict(lv=0.66, sh=2, vx=1.2, gk='ny', gt=.7, st=SC(1.7), pn=.7, pnp='arp8', pad=1.4, bs=.9, bsp='pedal', bl=1.0, blp='bell4', pc='bar')),
]

def compose(secs, tail):
    D = 0; log = []
    for name, src, L in secs:
        L = dict(L); L['lv'] = L.get('lv', 1.0) * LVF.get(name, 1.0)
        d0 = D; D = section(name, D, src, **L); log.append((name, d0, D))
    return log, D + tail

# ------------------------------------------------------------------------------------------------------------- the cutscene: lyric beats and camera shots, re-timed to the tracker's phrases
FPS = 59.73; TRIM = 0.0
def sec_of(row): return (row - TRIMROWS) * ROW
def notes_for_retime():
    out = []
    for (a, b, m, v) in tnotes(tr[1]):
        s = sec_of(rw(a)); out.append((s, s + (b - a) / 120.0 * ROW, m, v))
    return out
LY = [
 (0, [(4, "Hey, Mame, it's me.")]),
 (1, [(6, "The room is full tonight,"), (6, "but one chair's left alone.")]),
 (2, [(6, "I'm saving it for you."), (6, "Please stay with me tonight.")]),
 (3, [(4, "Are you out there?")]),
 (4, [(6, "I wrote this in the hall"), (6, "while you sat on the floor.")]),
 (5, [(6, "You whistled in the dark,"), (6, "and that's how I got through.")]),
 (6, [(4, "I was so loud,")]),
 (7, [(5, "and I couldn't see"), (5, "you there beside me,")]),
 (8, [(7, "but you held my hand all night,"), (4, "stayed anyway.")]),
 (9, [(4, "Mame, I'm still here.")]),
 (10, [(4, "Don't say goodbye.")]),
 (11, [(2, "Not yet.")]),
 (12, [(7, "Can you hear me sing it, Mame?"), (12, "I'll hold your note till the whole room knows"), (4, "Don't go quiet.")]),
 (13, [(7, "I felt you in the front row,"), (7, "and the lights stay on for you.")]),
 (14, [(7, "I was so hard to be with,"), (12, "but you stayed through every single worst night"), (4, "you stayed, you stayed.")]),
 (15, [(7, "So this one is for you, Mame,"), (7, "every note, every breath,"), (3, "I miss you.")]),
 (16, [(4, "The song is yours.")]),
 (17, [(6, "Your seat is still so warm"), (6, "like you just left the room.")]),
 (18, [(6, "I won't turn off the light,"), (3, "you were here"), (6, "today, and every day.")]),
 (19, [(6, "I'm here, because you were.")]),
]
def retime_cutscene():
    v = notes_for_retime(); ph = [[v[0]]]
    for n in v[1:]:
        (ph[-1].append(n) if n[0] - ph[-1][-1][1] <= .45 else ph.append([n]))
    assert len(ph) == 20, len(ph); END = ph[-1][-1][1] - TRIM + 1.2
    lines = []                                              # [start s, text, highest note of the line, phrase]
    for pi, chunks in LY:
        assert sum(c[0] for c in chunks) == len(ph[pi]), (pi, sum(c[0] for c in chunks), len(ph[pi]))
        k = 0
        for cn, txt in chunks:
            lines.append([ph[pi][k][0] - TRIM, txt, max(n[2] for n in ph[pi][k:k + cn]), pi]); k += cn
    fs = [round(l[0] * FPS) for l in lines] + [round(END * FPS)]
    PAD1, PAD2 = 40, 41                                     # the short wordless beats before the last phrase and after it (the camera settles on the empty seat)
    last18 = max(i for i, l in enumerate(lines) if l[3] == 18)
    beats = []; bad = 0
    for i, l in enumerate(lines):
        fr = fs[i + 1] - fs[i]
        if i == last18: fr -= PAD1
        if i == len(lines) - 1: fr -= PAD2
        txt = l[1]; ws = txt.split(); rows = 1; cur = ws[0]                # the caption must fit on ONE page (two rows of about 24 letters), or it would wait between pages and drift off the song
        for w in ws[1:]:
            if len(cur) + 1 + len(w) <= 24: cur += ' ' + w
            else: rows += 1; cur = w
        assert rows <= 2, ('3 rows', txt)
        d = max(20, fr - 2 * len(txt) - 1)
        if d * 1 + 2 * len(txt) + 1 > fr + 1: bad += 1; print('TOO LONG for its notes:', repr(txt), 'needs', 2 * len(txt) + 21, 'has', fr)
        beats.append((l, d, d + 2 * len(txt) + 1))
    assert not bad
    L = open('source/cutscene.h', encoding='utf-8').read().split('\n'); s5 = next(i for i, x in enumerate(L) if 'csS5[]' in x)
    idx = [i for i in range(s5 + 1, len(L)) if L[i].startswith(' {')]; first = idx[10]
    end = next(i for i in idx if 'Thanks for coming' in L[i]); assert end > first
    def beat(pose, sfx, d, txt): return ' {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,%s,43,%s,%d,%d,%s,{%s,0,0}},' % (pose, 'CF_AUTO' if txt else '0', sfx, d, '"Missy"' if txt else '0', '"%s"' % txt if txt else '0')
    new = []; shot = []; bi = 10; flip = 0; pos = {}
    for n, (l, d, tot) in enumerate(beats):
        if n == len(beats) - 1:                             # the last phrase: pad beat first, then the sway
            new.append(' {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_SING,43,0,0,%d,0,{0,0,0}},' % PAD1); pos['pad1'] = bi; bi += 1
        pose = 'CP_STAND' if n == 0 else ('CP_SWAY' if n == len(beats) - 1 else 'CP_SING')
        new.append(beat(pose, 250 if n == 0 else 0, d, l[1])); pos[n] = bi
        bi += 1
    new.append(' {CB_BIG,CA_NONE,CP_STAND,0,CA_MISSY,CP_SWAY,43,0,0,%d,0,{0,0,0}},' % PAD2); pos['pad2'] = bi
    L[first:end] = new
    txt = '\n'.join(L)
    open('source/cutscene.h', 'w', encoding='utf-8').write(txt)
    d18 = [(n, tot) for n, (l, d, tot) in enumerate(beats) if l[3] == 18]; T18 = sum(t for _, t in d18); A, B = (330, 172, 80), (568, 127, 95); acc = 0
    for n, (l, d, tot) in enumerate(beats):
        b = pos[n]
        if l[3] == 18:
            f0, f1 = acc / T18, (acc + tot) / T18; acc += tot
            z = lambda f: tuple(round(A[j] + (B[j] - A[j]) * f) for j in range(3))
            shot.append('    {5,%d,0,  0,%d, %d,%d,%d, %d,%d,%d},' % ((b, tot) + z(f0) + z(f1)))
        elif l[3] == 19:
            shot.append('    {5,%d,0,  0,%d, 600,121,97, 296,120,68},' % (pos['pad1'] + 1, tot))
        elif l[2] >= 67: shot.append('    {5,%d,1,  0,%d, 420,0,82, 700,0,82},' % (b, tot))
        else: shot.append('    {5,%d,1,  0,%d, %s,0,84, %s,0,84},' % ((b, tot) + (('330', '450') if flip == 0 else ('450', '330')))); flip ^= 1
    shot.insert(len([x for x in shot if ',0,  0,' in x and x.startswith('    {5,') and int(x.split(',')[1]) < pos['pad1']]), '    {5,%d,0,  0,%d, %d,%d,%d, 600,121,97},' % (pos['pad1'], PAD1, *B))
    shot.append('    {5,%d,0,  0,%d, 296,120,68, 256,120,64},' % (pos['pad2'], PAD2))
    shot.sort(key=lambda x: int(x.split(',')[1]))
    C = open('source/csshot.h', encoding='utf-8').read().split('\n')
    ci = [i for i, x in enumerate(C) if re.match(r'\s*\{5,(\d+),', x) and int(re.match(r'\s*\{5,(\d+),', x).group(1)) >= 10]
    assert ci and ci == list(range(ci[0], ci[-1] + 1)), ci
    C[ci[0]:ci[-1] + 1] = shot
    open('source/csshot.h', 'w', encoding='utf-8').write('\n'.join(C))
    print('cutscene: lines', len(lines), 'beats', bi - 10 + 1, 'last beat', pos['pad2'], 'song %.1f s' % (END))

# ------------------------------------------------------------------------------------------------------------- go
def build(which):
    K.GRID.clear()
    if which == 'missy': log, end = compose(M_SECS, 20); path, title = os.environ.get('OUT_M', 'tools/here_today.xm'), "HERE TODAY (MISSY'S SONG)"
    else:
        K.FAM['st_lo'] = (45, [('S', .75), ('M', 1.5), ('L', 2.28), ('X', 3.05)])                    # the full version needs the sung voice's ten samples, so the rest is trimmed to stay under
        K.FAM['ce'] = (58, [('B', .36), ('D', 1.5), ('F', 3.05)])                                    # the player's 32 instruments: no short low-string class, three solo-cello lengths, two piano registers
        K.pnkey = lambda p: 'pn_lo' if p <= 55 else 'pn_mid'
        log, end = compose(F_SECS, 24); path, title = os.environ.get('OUT_F', 'tools/here_today_full.xm'), 'HERE TODAY (FULL VERSION)'
    IN, nrows = write_xm(path, title, end, 'make_heretoday_here_xm')
    for d in IN: print('  %-8s %5.2f s %6d bytes svol %2d' % (d['key'], d['L'], len(d['x']), d['svol']))
    json.dump([(n, round(a * ROW, 1), round(b * ROW, 1)) for n, a, b in log], open(os.environ.get('TMPDIR', '.') + '/ht_sections.json', 'w'))
    return log
if __name__ == '__main__':
    which = sys.argv[1] if len(sys.argv) > 1 else 'both'
    if which in ('missy', 'both'): build('missy')
    if which in ('full', 'both'): build('full')
    if which in ('missy', 'both') and not os.environ.get('NO_RETIME'): retime_cutscene()
