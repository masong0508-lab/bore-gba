#!/usr/bin/env python3
"""Finds better copies of the sounds of the songs built on recorded samples (the ten "original samples" tracks of the studio renders).

usage:  python3 tools/sample_match.py [--write] SOURCE ...      (run from the project root; needs numpy + scipy)
        SOURCE = module files or folders of them (.xm .mod .s3m .it .mptm, also inside .zip), e.g. the artist's modules from the Mod Archive
        --write  saves the upgrades it finds to tools/sample_upgrades.json, which tools/studio_render.py then uses

Every sample of every source module is read (XM, ProTracker MOD, S3M and IT, compressed IT samples too) and compared with every sound
the ten songs use (as the game's .xm files hold them).  Two copies of one recording are found even when they differ in sample rate,
tuning, bit depth, trimming or loudness:
  1. their spectra are lined up on a log-frequency axis (cycles per sample): the shift that fits best is the rate ratio between them,
  2. the source copy is resampled by that ratio and slid along the song's copy (FFT cross-correlation, both cut to the band they share):
     a correlation near 1 means the same recording.
A match is an UPGRADE when the source copy is the better one: more top end once played at the song's pitch, or 16-bit where the song
has 8-bit.  The report lists, per song and sound, the best match, how sure it is and what it would gain.
"""
import os, sys, re, io, json, struct, zipfile, tempfile, fractions
import numpy as np
from scipy import signal
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, HERE)
import xm

SONGS = ['the_dipper_man', 'earth_and_the_space_citizens', 'amiga_music', 'tree_swaying_action', 'mi_cora_zone',
         'emergency_dance_floor', 'worthless_clouds', 'gottcho_barracho', 'cocaine_cola']   # (TREE-AGE's two versions share one .xm)
OUT = os.path.join(HERE, 'sample_upgrades.json')

# ================================================================ reading samples: dict(name, x = float -1..1, rate = Hz at the module's
# middle C, bits)
def read_xm(d):
    with tempfile.NamedTemporaryFile(suffix='.xm', delete=False) as f: f.write(d); p = f.name
    try: X = xm.parse(p)
    finally: os.remove(p)
    out = []
    for ins in X['insts']:
        for s in ins['samples']:
            x = np.array(s['data'], np.float32)
            if len(x) < 16: continue
            b8 = np.allclose(np.round(x * 128), x * 128, atol=1e-6)
            out.append(dict(name=(ins['name'].strip() + ' / ' + s['name'].strip()).strip(' /'), x=x,
                            rate=8363 * 2 ** ((s['rel'] + s['fine'] / 128) / 12), bits=8 if b8 else 16))
    return out

def read_mod(d):
    if len(d) < 1084: return []
    sig = d[1080:1084]
    nch = {b'M.K.': 4, b'M!K!': 4, b'FLT4': 4, b'4CHN': 4, b'6CHN': 6, b'8CHN': 8, b'FLT8': 8, b'CD81': 8, b'OKTA': 8}.get(sig)
    if nch is None:
        m = re.match(rb'(\d\d)C[HN]', sig); nch = int(m.group(1)) if m else None
    if not nch: return []                                                     # (15-sample Soundtracker modules are not read)
    hdr = []
    for k in range(31):
        h = d[20 + 30 * k:50 + 30 * k]; ln = struct.unpack('>H', h[22:24])[0] * 2; fine = h[24] & 15
        hdr.append((h[:22].split(b'\0')[0].decode('latin1').strip(), ln, fine - 16 if fine > 7 else fine))
    npat = max(d[952:952 + 128]) + 1; p = 1084 + npat * 64 * nch * 4; out = []
    for name, ln, fine in hdr:
        raw = d[p:p + ln]; p += ln
        if len(raw) > 16: out.append(dict(name=name, x=np.frombuffer(raw, np.int8).astype(np.float32) / 128, rate=8287 * 2 ** (fine / 96), bits=8))
    return out

def read_s3m(d):
    if d[0x2C:0x30] != b'SCRM': return []
    ordnum, insnum = struct.unpack('<HH', d[0x20:0x24]); ffi = struct.unpack('<H', d[0x2A:0x2C])[0]; out = []
    for k in range(insnum):
        o = struct.unpack('<H', d[0x60 + ordnum + 2 * k:0x62 + ordnum + 2 * k])[0] * 16; h = d[o:o + 0x50]
        if len(h) < 0x50 or h[0] != 1: continue
        so = ((h[0x0D] << 16) | struct.unpack('<H', h[0x0E:0x10])[0]) * 16; ln = struct.unpack('<I', h[0x10:0x14])[0]
        fl = h[0x1F]; c2 = struct.unpack('<I', h[0x20:0x24])[0]; w = 2 if fl & 4 else 1
        raw = d[so:so + ln * w]
        if fl & 4: x = np.frombuffer(raw[:len(raw) // 2 * 2], '<u2' if ffi == 2 else '<i2').astype(np.float32); x = (x - 32768) / 32768 if ffi == 2 else x / 32768
        else: x = np.frombuffer(raw, np.uint8 if ffi == 2 else np.int8).astype(np.float32); x = (x - 128) / 128 if ffi == 2 else x / 128
        if len(x) > 16: out.append(dict(name=h[0x30:0x4C].split(b'\0')[0].decode('latin1').strip(), x=x, rate=c2 or 8363, bits=16 if fl & 4 else 8))
    return out

def it_unpack(d, p, n, b16, it215):
    """Impulse Tracker 2.14 / 2.15 sample compression: blocks of a variable-width delta bitstream"""
    out = np.zeros(n, np.float32); pos = 0; top = 17 if b16 else 9; full = 16 if b16 else 8; mask = (1 << full) - 1
    while pos < n and p + 2 <= len(d):
        blen = d[p] | d[p + 1] << 8; blk = d[p + 2:p + 2 + blen]; p += 2 + blen
        acc = int.from_bytes(blk, 'little'); bp = 0; nbits = len(blk) * 8
        def take(k):
            nonlocal bp
            v = (acc >> bp) & ((1 << k) - 1); bp += k; return v
        width = top; d1 = d2 = 0; i = 0; bl = min(n - pos, 0x4000 if b16 else 0x8000)
        while i < bl and bp + width <= nbits:
            v = take(width)
            if width < 7:
                if v == 1 << (width - 1):
                    nw = take(4 if b16 else 3) + 1; width = nw if nw < width else nw + 1; continue
            elif width < top:
                border = (mask >> (top - width)) - (8 if b16 else 4)
                if border < v <= border + (16 if b16 else 8):
                    v -= border; width = v if v < width else v + 1; continue
            else:
                if v & (1 << full): width = (v + 1) & 0xFF; continue
            if width < full: sh = full - width; v = (v << sh) & mask; v = (v - (1 << full) if v >> (full - 1) else v) >> sh
            else: v &= mask; v = v - (1 << full) if v >> (full - 1) else v
            d1 = (d1 + v) & mask; d2 = (d2 + d1) & mask; r = d2 if it215 else d1
            out[pos + i] = (r - (1 << full) if r >> (full - 1) else r) / (1 << (full - 1)); i += 1
        pos += bl
    return out

def read_it(d):
    if d[:4] != b'IMPM': return []
    ordnum, insnum, smpnum = struct.unpack('<HHH', d[0x20:0x26]); off = 0xC0 + ordnum + insnum * 4; out = []
    for k in range(smpnum):
        o = struct.unpack('<I', d[off + 4 * k:off + 4 * k + 4])[0]; h = d[o:o + 0x50]
        if h[:4] != b'IMPS' or not h[0x12] & 1: continue
        fl, cvt = h[0x12], h[0x2E]; ln = struct.unpack('<I', h[0x30:0x34])[0]; c5 = struct.unpack('<I', h[0x3C:0x40])[0]; ptr = struct.unpack('<I', h[0x48:0x4C])[0]
        b16 = bool(fl & 2)
        if ln < 16: continue
        if fl & 8: x = it_unpack(d, ptr, ln, b16, bool(cvt & 4))
        else:
            raw = d[ptr:ptr + ln * (2 if b16 else 1)]; sg = cvt & 1
            if b16: x = np.frombuffer(raw[:len(raw) // 2 * 2], '<i2' if sg else '<u2').astype(np.float32); x = x / 32768 if sg else (x - 32768) / 32768
            else: x = np.frombuffer(raw, np.int8 if sg else np.uint8).astype(np.float32); x = x / 128 if sg else (x - 128) / 128
        out.append(dict(name=h[0x14:0x2E].split(b'\0')[0].decode('latin1').strip(), x=x, rate=c5 or 8363, bits=16 if b16 else 8))
    return out

def read_any(d):
    if d[:17] == b'Extended Module: ': return read_xm(d)
    if d[:4] == b'IMPM': return read_it(d)
    if len(d) > 0x30 and d[0x2C:0x30] == b'SCRM': return read_s3m(d)
    return read_mod(d)

def sources(paths):
    """every sample of every module under paths: (where, index, sample)"""
    for p in paths:
        if os.path.isdir(p):
            yield from sources(sorted(os.path.join(dp, f) for dp, dn, fs in os.walk(p) for f in fs)); continue
        try: d = open(p, 'rb').read()
        except OSError: continue
        if d[:2] == b'PK':
            with zipfile.ZipFile(p) as z:
                for nm in z.namelist():
                    try: smp = read_any(z.read(nm))
                    except Exception: smp = []
                    for k, s in enumerate(smp): yield '%s#%s' % (p, nm), k, s
            continue
        try: smp = read_any(d)
        except Exception: smp = []
        for k, s in enumerate(smp): yield p, k, s

# ================================================================ comparing two sounds
GRID = 2 ** np.arange(-11, -1 + 1e-9, 1 / 48)        # cycles per sample, 1/48 octave apart (2^-11 .. 0.5)
def trim(x):
    a = np.abs(x); nz = np.nonzero(a > 2e-3)[0]
    return x[nz[0]:nz[-1] + 1] if len(nz) else x[:0]

def logspec(x):
    """log power against log cycles-per-sample, from the loudest part of the sound"""
    n = len(x); seg = min(n, 1 << 15)
    if n > seg:
        e = np.convolve(x.astype(np.float64) ** 2, np.ones(1024), 'same'); c = int(np.argmax(e)); a = max(0, min(n - seg, c - seg // 2)); x = x[a:a + seg]
    f, p = signal.welch(np.asarray(x, np.float64), 1.0, nperseg=min(len(x), 4096))
    lp = 10 * np.log10(np.interp(GRID, f, p) + 1e-14)
    return lp

def shift_fit(lg, la, top_g, max_oct=4):
    """the shift (in 1/48 octaves) of a's spectrum that best lines it up with g's, over the bins where g has something (log power, Pearson)"""
    best = []; ok_g = lg > lg.max() - 55
    for s in range(-48 * max_oct, 48 * max_oct + 1):
        j0, j1 = max(0, -s), min(len(GRID), len(GRID) - s)
        if j1 - j0 < 60: continue
        g = lg[j0:j1]; a = la[j0 + s:j1 + s]; m = ok_g[j0:j1] & (GRID[j0:j1] < top_g)
        if m.sum() < 40: continue
        gg, aa = g[m] - g[m].mean(), a[m] - a[m].mean(); den = np.sqrt((gg * gg).sum() * (aa * aa).sum())
        if den > 0: best.append(((gg * aa).sum() / den, s))
    best.sort(reverse=True); return best[:3]

def resample(x, ratio):
    fr = fractions.Fraction(ratio).limit_denominator(1000)
    return signal.resample_poly(np.asarray(x, np.float64), fr.numerator, fr.denominator, window=('kaiser', 8.0)) if fr != 1 else np.asarray(x, np.float64)

def band_edge(x):
    """the highest frequency (cycles per sample) still within 50 dB of the loudest part of the spectrum"""
    f, p = signal.welch(np.asarray(x, np.float64), 1.0, nperseg=min(len(x), 4096)); db = 10 * np.log10(p + 1e-20)
    k = np.nonzero(db > db.max() - 50)[0]; return float(f[k[-1]]) if len(k) else 0.0

def compare(g, a, shifts=None):
    """how well source sound a stands for song sound g: dict(score 0..1, ratio, lag) or None.  ratio: a must be stretched by this to line
    up with g sample for sample; lag: where g starts in the stretched a.  shifts: the spectral fits, if already known"""
    xg, xa = g['_x'], a['_x']
    if len(xg) < 64 or len(xa) < 64: return None
    if shifts is None: shifts = shift_fit(g['_spec'], a['_spec'], 0.45)
    best = None
    for c, s in shifts:
        if c < 0.6: continue
        ratio = 2 ** (s / 48)                                                # a's cycles/sample are 2^(s/48) times g's: stretch a by that
        for fine in (0.0, -1 / 96, 1 / 96):
            if fine and (best is None or best['score'] < 0.5): continue      # (the fine steps only for a real candidate)
            r = ratio * 2 ** fine; y = resample(xa, r)
            if len(y) < 32 or len(y) > 40 * len(xg): continue
            cut = min(0.45, 0.45 * r) if r < 1 else 0.45                      # both cut to the band they share
            sos = signal.butter(6, max(0.01, min(0.49, cut)), 'low', fs=1.0, output='sos')
            gg = signal.sosfiltfilt(sos, xg) if len(xg) > 60 else xg; yy = signal.sosfiltfilt(sos, y) if len(y) > 60 else y
            n = len(gg) + len(yy); N = 1 << (n - 1).bit_length()
            cc = np.fft.irfft(np.fft.rfft(yy, N) * np.conj(np.fft.rfft(gg, N)), N)
            # normalised by the energy of the overlapping parts (so a longer or shorter copy still scores)
            lag = int(np.argmax(np.abs(cc))); L = lag if lag < N // 2 else lag - N
            g0, y0 = max(0, -L), max(0, L); m = min(len(gg) - g0, len(yy) - y0)
            if m < 32: continue
            sg, sy = gg[g0:g0 + m], yy[y0:y0 + m]; den = np.sqrt((sg * sg).sum() * (sy * sy).sum())
            sc = float(abs((sg * sy).sum()) / den) if den > 0 else 0.0
            sc *= min(1.0, m / len(gg)) ** 0.5                                # (a copy that covers only part of the sound counts less)
            if best is None or sc > best['score']: best = dict(score=sc, ratio=r, lag=L)
    return best

# ================================================================ the songs' sounds, and the search
def song_sounds(sid, path):
    X = xm.parse(path); used = set(i for o in X['order'] for r in X['pats'][o] for (n, i, v, e, ep) in r if n and n < 97); out = []
    for i in sorted(used):
        ins = X['insts'][i - 1]
        if not ins['samples']: continue
        s = ins['samples'][0]; x = np.array(s['data'], np.float32)
        out.append(dict(inst=i, name=ins['name'].strip(), x=x, rate=8363 * 2 ** ((s['rel'] + s['fine'] / 128) / 12),
                        bits=8 if np.allclose(np.round(x * 128), x * 128, atol=1e-6) else 16))
    return out

def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]; write = '--write' in sys.argv
    if not args: sys.exit(__doc__)
    table = dict((a, c) for a, b, c in re.findall(r'^SONG_XM\((\w+),"([^"]+)","([^"]+)"\)', open('source/songs.h').read(), re.M))
    src = list(sources(args)); print('%d samples in %d sources' % (len(src), len(set(w for w, k, s in src))))
    for w, k, s in src:
        s['_x'] = trim(s['x']); s['_spec'] = logspec(s['_x']) if len(s['_x']) >= 64 else None
    found = {}
    for sid in SONGS:
        print('\n' + sid)
        for g in song_sounds(sid, table[sid]):
            gx = g['_x'] = trim(g['x'])
            if len(gx) < 64: continue
            g['_spec'] = logspec(gx)
            pre = []                                                          # stage 1: the spectra (cheap), stage 2: the waveforms of the best 8
            for w, k, s in src:
                if s['_spec'] is None or os.path.abspath(w.split('#')[0]) == os.path.abspath(table[sid]): continue    # (not the song's own file)
                sh = shift_fit(g['_spec'], s['_spec'], 0.45)
                if sh and sh[0][0] >= 0.6: pre.append((sh[0][0], w, k, s, sh))
            pre.sort(key=lambda r: -r[0]); res = []
            for c, w, k, s, sh in pre[:8]:
                m = compare(g, s, sh)
                if m and m['score'] > 0.5: res.append((m['score'], w, k, s, m))
            res.sort(key=lambda r: -r[0])
            if not res: print('  inst %2d %-22s  no match' % (g['inst'], g['name'][:22])); continue
            sc, w, k, s, m = res[0]
            # what the source copy would give: its top end once played at the song's pitch (Hz), against the song's own
            top_g = band_edge(gx) * g['rate']; rate_new = g['rate'] / m['ratio']; top_a = band_edge(trim(s['x'])) * rate_new
            better = sc >= 0.85 and (top_a > top_g * 1.15 or (s['bits'] == 16 and g['bits'] == 8))
            print('  inst %2d %-22s  %.2f  %-40s #%-2d %-20s  %5d Hz %2d-bit -> %5d Hz %2d-bit  top end %5.0f -> %5.0f Hz  %s' % (
                g['inst'], g['name'][:22], sc, os.path.basename(w)[:40], k, s['name'][:20], g['rate'], g['bits'], rate_new * len(s['x']) / max(1, len(s['x'])),
                s['bits'], top_g, top_a, 'UPGRADE' if better else ''))
            if better:
                found.setdefault(sid, {})[str(g['inst'])] = dict(source=os.path.relpath(w.split('#')[0]) + ('#' + w.split('#')[1] if '#' in w else ''),
                    sample=k, name=s['name'], score=round(sc, 3), rate=rate_new, start=max(0, int(round(m['lag'] / m['ratio']))), top_from=round(top_g), top_to=round(top_a))
    print('\nupgrades found: %d sounds in %d songs' % (sum(len(v) for v in found.values()), len(found)))
    if write:
        json.dump(found, open(OUT, 'w'), indent=1); print('wrote', OUT)

if __name__ == '__main__': main()
