#!/usr/bin/env python3
"""The creator-menu chiptune loops as SYNTH DATA: the game plays them live instead of storing a recording of each.

usage:  python3 tools/chip_synth.py [preview_dir]       (run from the project root; needs numpy + scipy; takes a minute)
        writes source/music/chipsyn.bin (wave tables, noise, mixer tables and every loop's note stream),
               source/chipsyn.h (where each piece starts) and source/chips.h (the list of loops),
        and, with preview_dir, a WAV of each loop exactly as the GBA will play it (mono, before the pseudo-stereo).

The loops come from tools/make_chiptunes.py, which voices each song for four NES-style chip voices (PULSE 1 lead, PULSE 2 harmony,
TRIANGLE bass, NOISE drums) as control tracks: pitch and a 4-bit level per voice, 120 times a second. Its renderer turns those into
sound; this script stores the control tracks (a few KB per loop instead of ~130 KB of ADPCM) and the game's mixer does the rendering:
  - each voice reads a 256-step wave table that holds exactly the harmonics below 7.5 kHz for the note's pitch (one table per harmonic
    count, so nothing aliases and nothing is missing: the same band limit the recorded loops were cut to)
  - the noise voice reads a band-limited recording of the NES noise generator at the drum's clock (one 8192-sample buffer per drum sound)
  - two look-up tables apply the NES APU's non-linear mixer, then a 40 Hz one-pole low cut, like the renders
synth_loop() below is an exact integer twin of chipMix() in main.c (same tables, same order of operations), so what it writes is what
the GBA plays. VOL scales every loop (the creator music sits 23 % below the level the loops were first made at).
"""
import sys, os, io, wave, struct, contextlib
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
from scipy import signal
import make_chiptunes as MC
import encode_chip as EC

FS = 18157; CR = 120; BW = 7500.0; TL = 256; NB = 8192
VOL = 0.77
BIN = "source/music/chipsyn.bin"; HDR = "source/chipsyn.h"; CHIPS_H = "source/chips.h"
SHAPES = (0.25, 0.125, 'tri')          # table sets: lead pulse, harmony pulse, triangle
POFF = 3840; UOFF = 4800; NPM = 1920; NTM = 2400; SCALE = 16384   # the tables cover every value the voices can make: no clamping
TRI_MUL = 357                           # triangle level x 15 x 12241/8227 (its weight against the noise in the APU mixer), as x/16
HP_K = 901                              # low cut: lp += (x - lp) * 901 / 65536  (pole 0.98625 = 40 Hz at 18157 Hz)

# ---------------------------------------------------------------- pitch: a 16-bit code -> phase increment (integer, as main.c does it)
INC = [int(round(8.1758 * 2 ** (s / 12) * 2 ** 32 / FS)) for s in range(128)]
FINE = [int(round(2 ** (k / 3072) * 32768)) for k in range(256)]
KH = int(BW * 2 ** 32) // FS
def code_of(f): return 0 if f <= 0 else max(1, min(128 * 256 - 1, int(round(3072 * np.log2(f / 8.1758)))))
def inc_of(c): return 0 if c == 0 else (INC[c >> 8] * FINE[c & 255]) >> 15
def harm_of(inc): return 127 if inc == 0 else max(1, min(127, KH // inc))

# ---------------------------------------------------------------- shared tables
def bl_table(fn, H, hi=4096):
    x = np.arange(hi) / hi; C = np.fft.rfft(fn(x)); C[H + 1:] = 0; return np.fft.irfft(C, hi)[::hi // TL]
def shape_fn(s):
    if s == 'tri': return lambda x: MC.TRI[(x * 32).astype(int) % 32] / 15.0
    return lambda x: (x < s).astype(float)
def noise_buf(per, mode):
    OS = 16; fclk = 1789773.0 / MC.NOISE_P[per]; idx = (np.arange(NB * OS) * fclk / (FS * OS)).astype(np.int64)
    raw = MC.LF_SHORT[idx % 93] if mode else MC.LF_LONG[idx % 32767]
    y = signal.resample_poly(raw, 1, OS, window=('kaiser', 8.0)); Y = np.fft.rfft(y); f = np.fft.rfftfreq(len(y), 1 / FS); Y[f > BW] = 0
    return np.fft.irfft(Y, len(y))
def pm(p): return 95.88 * p / (8128.0 + 100 * p)
def tm(tn): return 159.79 * tn / (1 + 100 * tn)
PM = np.array([int(round(SCALE * pm((i * 4 - POFF + 2) / 100.0))) for i in range(NPM)], dtype=np.int64)
TM = np.array([int(round(SCALE * tm((i * 4 - UOFF + 2) / 100.0 / 12241.0))) for i in range(NTM)], dtype=np.int64)

# ---------------------------------------------------------------- a loop: control steps -> byte stream
def per_step(tr, NL, drums):
    """the middle copy's steps: per step (code p1, lvl p1, code p2, lvl p2, code tri, on tri, noise byte)"""
    rows = []
    for j in range(NL, 2 * NL):
        r = []
        for k in ('p1', 'p2'):
            a = int(tr[k].a[j]); r += [code_of(tr[k].f[j]) if a > 0 else -1, a]
        a = int(tr['tri'].a[j]); r += [code_of(tr['tri'].f[j]) if a > 0 else -1, 1 if a > 0 else 0]
        a = int(tr['noi'].a[j])
        if a > 0:
            key = (int(tr['noi'].p[j]), int(tr['noi'].m[j]))
            if key not in drums: drums.append(key)
            r.append(a | (drums.index(key) << 4))
        else: r.append(0)
        rows.append(r)
    return rows
def encode(rows):
    """flags byte per step (bit 0/2/4 pitch of p1/p2/tri, bit 1/3/5 level, bit 6 noise; >= 0x80: this and (b & 0x7F) more steps unchanged).
    A voice that falls silent keeps its pitch (it is not heard), so silence never costs pitch bytes."""
    out = bytearray(); cur = [None] * 7; i = 0; n = len(rows)
    while i < n:
        r = rows[i]; ch = []
        for v in range(3):
            c, l = r[2 * v], r[2 * v + 1]
            if c >= 0 and c != cur[2 * v]: ch.append((2 * v, c))
            if l != cur[2 * v + 1]: ch.append((2 * v + 1, l))
        if r[6] != cur[6]: ch.append((6, r[6]))
        if not ch:
            k = 1
            while i + k < n and k < 128:
                r2 = rows[i + k]
                if any((r2[2 * v] >= 0 and r2[2 * v] != cur[2 * v]) or r2[2 * v + 1] != cur[2 * v + 1] for v in range(3)) or r2[6] != cur[6]: break
                k += 1
            out.append(0x80 | (k - 1)); i += k; continue
        fl = 0
        for b, _ in ch: fl |= 1 << b
        out.append(fl)
        for b, val in sorted(ch):
            if b in (0, 2, 4): out += struct.pack('<H', val)
            else: out.append(val)
            cur[b] = val
        i += 1
    return bytes(out)

# ---------------------------------------------------------------- the integer twin of chipMix()
class Tables:
    def __init__(self, hr):
        self.hr = hr                                                   # per shape: (hmin, hmax)
        self.tab = [{H: np.round(bl_table(shape_fn(s), H) * 100).astype(np.int64) for H in range(hr[i][0], hr[i][1] + 1)} for i, s in enumerate(SHAPES)]
    def get(self, v, inc): lo, hi = self.hr[v]; return self.tab[v][max(lo, min(hi, harm_of(inc)))]

def synth_loop(stream, NL, T, nbufs, G, lp0, passes=1):
    """returns (int8 output of the last pass, pre-gain hp signal of the last pass, lp at the end)"""
    ph = [0, 0, 0]; inc = [0, 0, 0]; tab = [T.get(v, 0) for v in range(3)]; lv = [0, 0, 0]; nl = 0; nb = nbufs[0]; npos = 0
    lp = lp0; acc = 0; outs = []; hps = []
    for ps in range(passes):
        d = 0; rep = 0; o = []; h = []
        for j in range(NL):
            if rep: rep -= 1
            else:
                fl = stream[d]; d += 1
                if fl & 0x80: rep = fl & 0x7F
                else:
                    for v in range(3):
                        if fl & (1 << (2 * v)): c = stream[d] | (stream[d + 1] << 8); d += 2; inc[v] = inc_of(c); tab[v] = T.get(v, inc[v])
                        if fl & (2 << (2 * v)): lv[v] = stream[d]; d += 1
                    if fl & 0x40: b = stream[d]; d += 1; nl = b & 15; nb = nbufs[b >> 4]
            t = acc + FS; n = t // CR; acc = t % CR
            k = np.arange(n, dtype=np.int64)
            seg = [(ph[v] + inc[v] * (k + 1)) & 0xFFFFFFFF for v in range(3)]   # main.c adds the increment before it reads
            for v in range(3): ph[v] = int(seg[v][-1]) if n else ph[v]
            v1 = tab[0][seg[0] >> 24]; v2 = tab[1][seg[1] >> 24]; v3 = tab[2][seg[2] >> 24]
            nz = nb[(npos + k) & (NB - 1)]; npos = (npos + n) & (NB - 1)
            p = v1 * lv[0] + v2 * lv[1]
            u = ((v3 * (TRI_MUL if lv[2] else 0)) >> 4) + nz * nl
            pi = (p + POFF) >> 2; ui = (u + UOFF) >> 2; assert pi.min() >= 0 and pi.max() < NPM and ui.min() >= 0 and ui.max() < NTM
            y = PM[pi] + TM[ui]
            x4 = y << 4
            hp = np.empty(n, dtype=np.int64)
            for i in range(n):                                          # the low cut runs sample by sample
                lp += ((int(x4[i]) - lp) * HP_K) >> 16; hp[i] = int(x4[i]) - lp
            h.append(hp); o.append(np.clip((hp * G) >> 20, -128, 127))
        outs = np.concatenate(o); hps = np.concatenate(h)
    return outs.astype(np.int64), hps, lp

# ---------------------------------------------------------------- driver
def all_loops():
    for secret, lst in ((0, MC.SONGS), (1, MC.SECRET)):
        for idx, (sid, path, title, opts) in enumerate(lst, 1): yield secret, sid, path, title, opts

def tracks_for(sid, path, opts):
    with contextlib.redirect_stdout(io.StringIO()):
        S, insts = MC.load(sid, path)
        ev = {}
        for o in S['order']:
            for r in S['pats'][o]:
                for ch, (n, i, v, e, ep) in enumerate(r):
                    if n and n < 97 and insts[i - 1]: ev.setdefault(i, []).append(n)
        info = {i: MC.analyse(insts[i - 1], S['insts'][i - 1]['name'], ev[i], opts.get('v2', False)) for i in ev}
        if not any(a['role'] == 'bass' for a in info.values()):
            low = [i for i, a in info.items() if a['role'] in ('lead', 'pad') and a['f0'] < 330 and len(ev[i]) >= 16]
            if low: info[min(low, key=lambda i: info[i]['f0'])]['role'] = 'bass'
        rowsec = S['tempo'] * 2.5 / S['bpm']
        rows, notes = MC.build(S, insts, info, rowsec)
        tmin, tmax = opts.get('win', (9.0, 20.0))
        score, o0, k, g0, g1, rep, secs = MC.pick_window(S, rows, notes, rowsec, tmin, tmax)
        tracks, NLoop, layout, meta = MC.voice(S, insts, info, rows, notes, g0, g1, rowsec, opts)
    return tracks, NLoop, opts.get('duty', (0.25, 0.125))

def reference(tr, NL, duty):
    """the loop exactly as make_chiptunes.py + encode_chip.py made it for the game (16-bit, 18157 Hz)"""
    y = MC.render(tr, 3 * NL, duty); L = NL * (MC.FS // MC.CR); y = MC.finish(y); loop = y[L:2 * L]
    loop16 = (np.clip(loop, -1, 1) * 32767).astype('<i2').astype(float) / 32768.0
    return np.round(EC.to_game_rate(loop16, MC.FS) * 32767).astype(np.int64)

def spec_match(ref, y, nf=1024):
    """how closely y's spectrum follows ref's, frame by frame (dB, like an SNR of the magnitudes). Phase is ignored: a free-running
    oscillator is never sample-aligned with the render, and the drums' noise is random, so a plain SNR would say nothing."""
    n = min(len(ref), len(y)); w = np.hanning(nf); F = lambda x: np.abs(np.array([np.fft.rfft(x[i:i + nf] * w) for i in range(0, n - nf, nf // 2)]))
    R = F(ref[:n].astype(float)); Y = F(y[:n].astype(float)); g = np.sum(R * Y) / max(np.sum(Y * Y), 1e-9)
    return 10 * np.log10(np.sum(R ** 2) / max(np.sum((R - g * Y) ** 2), 1e-9))

if __name__ == '__main__':
    prev = sys.argv[1] if len(sys.argv) > 1 else None
    if prev: os.makedirs(prev, exist_ok=True)
    loops = []; drums = []; hmin = [127] * 3; hmax = [1] * 3
    for secret, sid, path, title, opts in all_loops():
        tr, NL, duty = tracks_for(sid, path, opts)
        assert tuple(duty) == SHAPES[:2], 'a loop with other pulse widths needs its own table set'
        rows = per_step(tr, NL, drums)
        for r in rows:
            for v in range(3):
                if r[2 * v] > 0: H = harm_of(inc_of(r[2 * v])); hmin[v] = min(hmin[v], H); hmax[v] = max(hmax[v], H)
        loops.append(dict(secret=secret, sid=sid, title=title, tr=tr, NL=NL, duty=duty, rows=rows, stream=encode(rows)))
    T = Tables([(hmin[v], hmax[v]) for v in range(3)])
    nbufs = [np.round(noise_buf(p, m) * 100).astype(np.int64) for p, m in drums]
    blob = bytearray(); off = {}
    def put(name, b): blob.extend(b'\0' * ((-len(blob)) % 4)); off[name] = len(blob); blob.extend(b)
    put('PM', PM.astype('<i2').tobytes()); put('TM', TM.astype('<i2').tobytes())
    for i, nb in enumerate(nbufs): put('NZ%d' % i, nb.astype('i1').tobytes())
    for v in range(3): put('TAB%d' % v, b''.join(T.tab[v][H].astype('i1').tobytes() for H in range(hmin[v], hmax[v] + 1)))
    total_old = 0; print('%-34s %6s %7s %7s  %s' % ('loop', 'secs', 'old KB', 'new KB', 'spectrum match to the approved render: stored ADPCM / live synth'))
    for L in loops:
        ref = reference(L['tr'], L['NL'], L['duty'])
        _, hp1, lp = synth_loop(L['stream'], L['NL'], T, nbufs, 1, 0)                    # pass 1: the low cut settles
        _, hp, lp2 = synth_loop(L['stream'], L['NL'], T, nbufs, 1, lp)                   # pass 2: the loop as it repeats
        n = min(len(ref), len(hp)); g = np.sqrt(np.mean((ref[:n] / 256.0) ** 2) / np.mean(hp[:n].astype(float) ** 2))   # same loudness as the recording
        G = int(round(g * VOL * (1 << 20)))
        out, _, _ = synth_loop(L['stream'], L['NL'], T, nbufs, G, lp)
        old = os.path.getsize('source/music/chip_%s.adp' % L['sid']) if os.path.exists('source/music/chip_%s.adp' % L['sid']) else 0; total_old += old
        hdr = struct.pack('<HHiI', L['NL'], 0, lp, G)
        put('L_' + L['sid'], hdr + L['stream'])
        b, s0, s1, nib = EC.encode_loop(ref); d, _, _ = EC.dec(nib, *s0)
        L['out'] = out
        print('%-34s %6.1f %7d %7.1f  %5.1f dB / %5.1f dB' % (L['title'][:34], L['NL'] / CR, old // 1024, (len(hdr) + len(L['stream'])) / 1024,
                                                           spec_match(ref, (d.astype(int) >> 8)), spec_match(ref, out / VOL)))
        if prev:
            with wave.open(os.path.join(prev, 'chip_%s.wav' % L['sid']), 'wb') as w:
                w.setnchannels(1); w.setsampwidth(2); w.setframerate(FS); w.writeframes((np.concatenate([out, out]) * 256).astype('<i2').tobytes())
    open(BIN, 'wb').write(blob)
    with open(HDR, 'w') as f:
        f.write('// chipsyn.h - where each piece of source/music/chipsyn.bin starts (made by tools/chip_synth.py, do not edit)\n')
        for k in ('PM', 'TM'): f.write('#define CS_%s %d\n' % (k, off[k]))
        f.write('#define CS_NZ {%s}\n#define CS_NNZ %d\n#define CS_NB %d\n' % (','.join(str(off['NZ%d' % i]) for i in range(len(nbufs))), len(nbufs), NB))
        f.write('#define CS_TAB {%s}\n#define CS_HMIN {%s}\n#define CS_HMAX {%s}\n' % (','.join(str(off['TAB%d' % v]) for v in range(3)), ','.join(map(str, hmin)), ','.join(map(str, hmax))))
        f.write('#define CS_POFF %d\n#define CS_UOFF %d\n#define CS_NPM %d\n#define CS_NTM %d\n#define CS_TRIMUL %d\n#define CS_HPK %d\n#define CS_KH %du\n' % (POFF, UOFF, NPM, NTM, TRI_MUL, HP_K, KH))
        f.write('static const u32 csInc[128]={%s};\n' % ','.join('%du' % x for x in INC))
        f.write('static const u16 csFine[256]={%s};\n' % ','.join(map(str, FINE)))
    with open(CHIPS_H, 'w') as f:
        f.write('// chips.h - the chiptune loops of the creator menu: voiced by tools/make_chiptunes.py, stored as synth data by tools/chip_synth.py (do not edit)\n'
                '//   CHIP(id,"NAME",secret,offset into chipsyn.bin)   secret = 1: only after the title-screen code (UP UP DOWN DOWN LEFT LEFT RIGHT B A START)\n')
        for L in loops: f.write('CHIP(chip_%s,"%s",%d,%d)\n' % (L['sid'], L['title'], L['secret'], off['L_' + L['sid']]))
    print('%d loops: %d KB of ROM in all (was %d KB of ADPCM) -> %s, %s, %s' % (len(loops), len(blob) // 1024, total_old // 1024, BIN, HDR, CHIPS_H))
