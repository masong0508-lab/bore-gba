#!/usr/bin/env python3
"""Packs source/art/bortua_fighters.bin (the pre-rendered fighter frames, 4096 bytes each) into source/art/bortua_fighters.lz, which is what
goes into the ROM (the .bin stays as the source, like the .raw files of the nature beds).

Every frame is its own GBA BIOS LZ77 stream (type 0x10), so the game can unpack one frame straight into OBJ VRAM with the BIOS call
LZ77UnCompReadNormalWrite16bit (SWI 0x12) the moment a fighter changes frame (bortua.h, bfTiles). The VRAM version writes halfwords, so a match may
never point at the byte just before it (displacement 1): this packer only uses displacements of 2 and up. The result is lossless
(--check unpacks every frame the way the BIOS does and compares).

FILE LAYOUT  u32 index[N + 1]: byte offset of frame n's stream from the start of the file (the last entry is the file's length);
             then the streams, each padded to 4 bytes.
usage: python3 tools/pack_fighters.py [--check]      (from anywhere; make_bortua.py runs it after it writes the .bin)"""
import os, sys, struct
from multiprocessing import Pool

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SRC = os.path.join(ROOT, 'source', 'art', 'bortua_fighters.bin')
DST = os.path.join(ROOT, 'source', 'art', 'bortua_fighters.lz')
FRB = 4096
MINOFF = 2   # the BIOS's VRAM version cannot copy from the byte right before the one it writes


def lz77(b):
    n = len(b)
    chains = {}
    best = [(0, 0)] * n
    for i in range(n):
        if i + 3 <= n:
            key = b[i:i + 3]
            lst = chains.get(key)
            bl, bo = 0, 0
            if lst:
                for j in reversed(lst[-96:]):
                    o = i - j
                    if o > 4096: break
                    if o < MINOFF: continue
                    l = 3
                    while l < 18 and i + l < n and b[j + l] == b[i + l]: l += 1
                    if l > bl: bl, bo = l, o
                    if l == 18: break
            best[i] = (bl, bo)
            chains.setdefault(key, []).append(i)
    # optimal parse over bit costs (a literal is 9 bits with its flag, a match 17)
    INF = 1 << 60
    cost = [INF] * (n + 1); how = [None] * (n + 1); cost[0] = 0
    for i in range(n):
        c = cost[i]
        if c + 9 < cost[i + 1]: cost[i + 1] = c + 9; how[i + 1] = (1, 0)
        bl, bo = best[i]
        for l in range(3, bl + 1):
            if c + 17 < cost[i + l]: cost[i + l] = c + 17; how[i + l] = (l, bo)
    toks = []; i = n
    while i > 0:
        l, o = how[i]; toks.append((l, o) if l > 1 else (1, b[i - 1])); i -= l
    toks.reverse()
    out = bytearray(struct.pack('<I', (n << 8) | 0x10))
    for k in range(0, len(toks), 8):
        grp = toks[k:k + 8]; flags = 0; body = bytearray()
        for t, (l, v) in enumerate(grp):
            if l > 1:
                flags |= 0x80 >> t; d = v - 1
                body += bytes([((l - 3) << 4) | (d >> 8), d & 255])
            else:
                body.append(v)
        out.append(flags); out += body
    while len(out) & 3: out.append(0)
    return bytes(out)


def bios_vram_unpack(s):
    """The BIOS's LZ77UnCompReadNormalWrite16bit, byte for byte: output only becomes readable once its halfword is complete."""
    n = struct.unpack_from('<I', s, 0)[0]
    assert s[0] == 0x10, 'not an LZ77 stream'
    n >>= 8; p = 4; out = bytearray()
    while len(out) < n:
        fl = s[p]; p += 1
        for t in range(8):
            if len(out) >= n: break
            if fl & (0x80 >> t):
                l = (s[p] >> 4) + 3; d = (((s[p] & 15) << 8) | s[p + 1]) + 1; p += 2
                assert d >= MINOFF, 'displacement 1 is not allowed in the VRAM version'
                for _ in range(l):
                    pos = len(out); src = pos - d
                    assert 0 <= src < (pos & ~1), 'copy from a halfword the BIOS has not written yet'
                    out.append(out[src])
                    if len(out) >= n: break
            else:
                out.append(s[p]); p += 1
    return bytes(out[:n])


def main():
    data = open(SRC, 'rb').read()
    assert len(data) % FRB == 0
    nfr = len(data) // FRB
    frames = [data[i * FRB:(i + 1) * FRB] for i in range(nfr)]
    with Pool() as pool: packed = pool.map(lz77, frames, chunksize=8)
    base = 4 * (nfr + 1)
    idx = []; blob = bytearray(); off = base
    for p in packed: idx.append(off); blob += p; off += len(p)
    idx.append(off)
    out = struct.pack('<%dI' % (nfr + 1), *idx) + bytes(blob)
    if '--check' in sys.argv:
        for i, p in enumerate(packed):
            assert bios_vram_unpack(p) == frames[i], 'frame %d does not unpack to itself' % i
        print('pack_fighters: all %d frames unpack to the original' % nfr)
    open(DST, 'wb').write(out)
    print('pack_fighters: %d frames, %d B -> %d B (%.1f%%)' % (nfr, len(data), len(out), 100.0 * len(out) / len(data)))


if __name__ == '__main__':
    main()