"""Reads the textures out of a Sims 2 .package file (DBPF): index -> QFS (RefPack) decompression -> TXTR (cImageData) -> DXT1/3/5 or raw pixels.
Used by tools/make_wallpapers.py.  numpy only."""
import struct
import numpy as np

def qfs(b):   # RefPack / QFS decompression (b starts with the u32 size, then 10 FB, then a 24-bit big-endian output size)
    if b[4:6] != b'\x10\xfb': return b
    n = (b[6] << 16) | (b[7] << 8) | b[8]; out = bytearray(); i = 9
    while i < len(b):
        c0 = b[i]
        if c0 < 0x80:
            c1 = b[i + 1]; i += 2; pl = c0 & 3; ln = ((c0 & 0x1c) >> 2) + 3; off = ((c0 & 0x60) << 3) + c1 + 1
        elif c0 < 0xc0:
            c1, c2 = b[i + 1], b[i + 2]; i += 3; pl = (c1 >> 6) & 3; ln = (c0 & 0x3f) + 4; off = ((c1 & 0x3f) << 8) + c2 + 1
        elif c0 < 0xe0:
            c1, c2, c3 = b[i + 1], b[i + 2], b[i + 3]; i += 4; pl = c0 & 3; ln = ((c0 & 0x0c) << 6) + c3 + 5; off = ((c0 & 0x10) << 12) + (c1 << 8) + c2 + 1
        elif c0 < 0xfc:
            i += 1; pl = ((c0 & 0x1f) << 2) + 4; out += b[i:i + pl]; i += pl; continue
        else:
            i += 1; pl = c0 & 3; out += b[i:i + pl]; break
        out += b[i:i + pl]; i += pl
        for _ in range(ln): out.append(out[-off])
    return bytes(out[:n])

def entries(d):
    cnt, ioff, isz = struct.unpack('<III', d[0x24:0x30]); minor = struct.unpack('<I', d[0x3C:0x40])[0]; es = 24 if minor == 2 else 20
    for k in range(cnt):
        e = d[ioff + k * es: ioff + (k + 1) * es]
        if es == 24: t, g, ih, il, o, s = struct.unpack('<6I', e)
        else: t, g, il, o, s = struct.unpack('<5I', e)
        yield t, d[o:o + s]

def rgb565(c): return np.stack([((c >> 11) & 31) * 255 // 31, ((c >> 5) & 63) * 255 // 63, (c & 31) * 255 // 31], -1).astype(np.int32)
def dxt(data, w, h, fmt):   # -> (h, w, 3) uint8
    img = np.zeros((h, w, 3), np.uint8); bs = 8 if fmt == 4 else 16; p = 0
    for by in range(0, h, 4):
        for bx in range(0, w, 4):
            blk = data[p:p + bs]; p += bs
            cb = blk[bs - 8:]; c0, c1 = struct.unpack('<HH', cb[:4]); bits = struct.unpack('<I', cb[4:8])[0]
            a, b = rgb565(np.array(c0)), rgb565(np.array(c1))
            if fmt == 4 and c0 <= c1: pal = [a, b, (a + b) // 2, np.zeros(3, np.int32)]
            else: pal = [a, b, (2 * a + b) // 3, (a + 2 * b) // 3]
            for k in range(16):
                y, x = by + k // 4, bx + k % 4
                if y < h and x < w: img[y, x] = pal[(bits >> (2 * k)) & 3]
    return img

def textures(path):   # [(name, image)] for every TXTR in the package, the biggest mipmap of each
    d = open(path, 'rb').read(); res = []
    for t, blob in entries(d):
        if t != 0x1C4A276C: continue
        b = qfs(blob)
        k = b.find(b'cImageData')
        if k < 0: continue
        # after "cImageData": block id(4) version(4), then cSGResource: string "cSGResource" (len byte), id(4) version(4), name string
        p = k + len('cImageData') + 8
        sl = b[p]; p += 1 + sl + 8; nl = b[p]; name = b[p + 1:p + 1 + nl].decode('latin1', 'replace'); p += 1 + nl
        w, h, fmt, mips = struct.unpack('<IIII', b[p:p + 16])
        bpp = {4: .5, 5: 1, 8: 1, 1: 4, 2: 3, 7: 4, 9: 3}.get(fmt)
        if not bpp: continue
        need = int(w * h * bpp)
        q = b.rfind(struct.pack('<I', need))   # the biggest mipmap is stored last: its byte count, then its data
        if q < 0 or q + 4 + need > len(b):
            if len(b) < need: continue
            raw = b[-need:]                    # (a single-mipmap texture is just the pixels at the end)
        else: raw = b[q + 4:q + 4 + need]
        if fmt in (4, 5, 8): img = dxt(raw, w, h, fmt)
        elif fmt in (1, 7): img = np.frombuffer(raw, np.uint8).reshape(h, w, 4)[:, :, 2::-1].copy()   # BGRA -> RGB
        else: img = np.frombuffer(raw, np.uint8).reshape(h, w, 3)[:, :, ::-1].copy()
        res.append((name, img))
    return res
