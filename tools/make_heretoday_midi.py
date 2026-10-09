#!/usr/bin/env python3
"""HERE TODAY (MISSY'S SONG), v5 (voice an octave up, female and emotional; new lyrics written note for note): renders the author's MIDI (tools/here_today.mid: female voice, nylon guitar, string quartet) to source/music/here_today.adp
and re-times the scene-5 lyric beats (source/cutscene.h) and camera shots (source/csshot.h) to its phrases.  Run from the project root: python3 tools/make_heretoday_midi.py"""
import sys, os, re, wave
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import midi_read as M
from encode_sfx import encode
import encode_song as ES
SR = 22050; TRIM = 0.80; FPS = 59.73
div, tr = M.parse('tools/here_today.mid')
tempos = [(e[0], int.from_bytes(e[3], 'big')) for e in tr[0] if e[1] == 'meta' and e[2] == 0x51]
def sec(t):
    s = 0; pt = 0; tp = 500000
    for tk, v in tempos:
        if tk >= t: break
        s += (tk - pt) * tp / div / 1e6; pt = tk; tp = v
    return s + (t - pt) * tp / div / 1e6
def notes(ev):
    on = {}; out = []
    for e in ev:
        if e[1] != 'ev': continue
        k = e[2] >> 4
        if k == 9 and e[4] > 0: on[e[3]] = (e[0], e[4])
        elif k == 8 or (k == 9 and e[4] == 0):
            if e[3] in on: t0, v = on.pop(e[3]); out.append((sec(t0), sec(e[0]), e[3], v))
    return sorted(out)
hz = lambda m: 440 * 2 ** ((m - 69) / 12)
total = sec(90000) + 2.5; N = int(total * SR); out = np.zeros(N)
def put(t0, x, g):
    i = int(t0 * SR); x = x[:N - i]; out[i:i + len(x)] += x * g
def env(n, a, r): e = np.ones(n); k = min(n, int(a * SR)); e[:k] = np.linspace(0, 1, k); k = min(n, int(r * SR)); e[-k:] *= np.linspace(1, 0, k); return e
def additive(f, n, w):
    t = np.arange(n) / SR; return sum(a * np.sin(2 * np.pi * f * (k + 1) * t) for k, a in enumerate(w) if f * (k + 1) < SR * .45)
rng = np.random.default_rng(7)
VOICE_OCT = 12                                         # the melody sits one octave up (a female range, B3 to G5)
def formant(x, fc, bw, g):                             # one vocal-tract resonance
    b, a = signal.butter(2, [(fc - bw / 2) / (SR / 2), (fc + bw / 2) / (SR / 2)], 'band'); return g * signal.lfilter(b, a, x)
vn = notes(tr[1])
for i, (s, e, m, v) in enumerate(vn):                  # voice: a breathy, emotional female vocal ("ah"), glide in, late vibrato, swell
    prev = vn[i - 1] if i else None; legato = prev is not None and s - prev[1] < .12
    n = int((e - s + .2) * SR); t = np.arange(n) / SR; dur = e - s; mt = m + VOICE_OCT; f0 = hz(mt)
    slide = np.zeros(n)                                # scoop up into the note from below (a bigger sigh after a rest), or glide from the last pitch
    st = (prev[2] - m) if legato and abs(prev[2] - m) <= 5 else -1.4
    slide = st * np.exp(-t / .07)
    vd = np.clip((t - min(.35, dur * .45)) / .5, 0, 1)  # vibrato blooms late and widens on long notes, a touch of tremor
    vib = (.011 + .006 * min(1, dur / 1.5)) * np.sin(2 * np.pi * (5.6 + .5 * np.sin(2 * np.pi * .3 * t)) * t) * vd
    f = f0 * 2 ** (slide / 12) * (1 + vib) * (1 + .0012 * rng.standard_normal(n).cumsum() / np.sqrt(n) * 4)
    ph = 2 * np.pi * np.cumsum(f) / SR
    x = sum(np.sin((k + 1) * ph + .3 * k) / (k + 1) ** 1.05 for k in range(24) if f.max() * (k + 1) < SR * .45)   # brighter glottal source
    nz = rng.standard_normal(n); nz = signal.lfilter(*signal.butter(2, 2500 / (SR / 2), 'high'), nz)
    air = (.05 + .09 * np.exp(-t / .12)) * nz * (1 + .4 * np.sin(2 * np.pi * 5.6 * t))   # breath, strongest on the onset
    bright = min(1.0, max(0.0, (mt - 62) / 14))
    x = formant(x, 850 + 150 * bright, 220, 1.5) + formant(x, 1250 + 200 * bright, 300, .9) + formant(x, 2900, 500, .5) + .35 * x + air   # female vowel formants
    swell = 1 + .28 * np.clip(np.sin(np.pi * np.clip(t / max(dur, .3), 0, 1)), 0, 1) * min(1, dur / 1.2)   # long notes swell, then fall away
    put(s, x * env(n, .035 if not legato else .02, .22) * swell, .15 * (.55 + .45 * v / 100) * (1 + .12 * bright))
for (s, e, m, v) in notes(tr[3]):                      # nylon guitar: plucked partials with their own decays
    n = int(min(e - s + .4, 3.5) * SR); t = np.arange(n) / SR; f = hz(m)
    x = sum(np.sin(2 * np.pi * f * k * t) * np.exp(-t * (2 + 1.6 * k)) / k for k in range(1, 9) if f * k < SR * .45)
    put(s, x * env(n, .003, .05), .24 * v / 100)
for tk, g in ((4, .11), (5, .11), (6, .12), (7, .14)):  # strings: soft detuned saws, slow bloom
    for (s, e, m, v) in notes(tr[tk]):
        n = int((e - s + .5) * SR); t = np.arange(n) / SR; f = hz(m); x = 0
        for c in (-7, 0, 7): x = x + signal.sawtooth(2 * np.pi * f * 2 ** (c / 1200) * t + rng.uniform(0, 6))
        b, a = signal.butter(2, 1800 / (SR / 2)); x = signal.lfilter(b, a, x)
        put(s, x * env(n, .35, .5), g * v / 100)
out = out[int(TRIM * SR):]
out = signal.lfilter(*signal.butter(1, 60 / (SR / 2), 'high'), out); out /= np.abs(out).max() / .9
w = wave.open(__import__('os').environ.get('TMPDIR','.') + '/here_today.wav', 'wb'); w.setnchannels(1); w.setsampwidth(2); w.setframerate(SR); w.writeframes((out * 32767).astype('<i2').tobytes()); w.close()
x = ES.load(__import__('os').environ.get('TMPDIR','.') + '/here_today.wav'); open('source/music/here_today.adp', 'wb').write(encode(x)); print('adp', len(x) / ES.RATE, 's')
# ---- phrases -> lyric beats.  The melody has 20 phrases; each is cut into LINES, and every line starts on the first note it is sung on (the notes count is the
# first number: about one syllable per note, long notes carry the stressed words), so the caption appears with its notes and the syllables fall where the melody does.
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
v = notes(tr[1]); ph = [[v[0]]]
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
open('source/cutscene.h', 'w', encoding='utf-8').write('\n'.join(L).replace("Beats 10+ are the sung lines, one per phrase of the melody (tools/make_heretoday.py); each lasts from its phrase start to the next", "Beats 10+ are the sung lines, written note for note to the melody (LY in tools/make_heretoday_midi.py): each line starts on its first note and lasts to the next line"))
# ---- camera: a push or drift on the singer for each line, tight pushes on the high ones; the last phrase drifts to the empty seat
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
open('source/csshot.h', 'w', encoding='utf-8').write('\n'.join(C).replace('for each sung line (one shot per line of the song, tools/make_heretoday_midi.py)', 'for each sung line').replace('for each sung line', 'for each sung line (one shot per line of the song, tools/make_heretoday_midi.py)'))
print('lines', len(lines), 'beats', bi - 10 + 1, 'last beat', pos['pad2'])
