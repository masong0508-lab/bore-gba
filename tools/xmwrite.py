"""A small FastTracker II (.xm) writer: channels, patterns, instruments with one 8-bit sample each. Used by tools/corp_build.py."""
import struct
import numpy as np

def cell(note=0, inst=0, vol=0, fx=0, fxp=0):
    m = 0; out = bytearray()
    if note: m |= 1; out.append(note)
    if inst: m |= 2; out.append(inst)
    if vol: m |= 4; out.append(vol)
    if fx: m |= 8; out.append(fx)
    if fxp: m |= 16; out.append(fxp)
    return bytes([0x80 | m]) + bytes(out) if m != 31 else bytes([0x80 | m]) + bytes(out)

def instrument(name, data=None, loop=False, rel=0, fine=0, vol=64, fadeout=0):
    """data: int8 numpy array of the sample (None = an instrument with no sample)."""
    nm = name.encode("latin-1")[:22].ljust(22, b" ")
    if data is None:
        return struct.pack("<I", 29) + nm + b"\x00" + struct.pack("<H", 0)
    n = len(data); d = np.asarray(data, dtype=np.int8)
    hdr = struct.pack("<I", 263) + nm + b"\x00" + struct.pack("<H", 1)
    hdr += struct.pack("<I", 40) + bytes(96) + bytes(48) + bytes(48) + bytes([0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]) + struct.pack("<H", fadeout) + bytes(22)
    typ = 1 if loop else 0
    sh = struct.pack("<III", n, 0, n if loop else 0) + bytes([vol & 255, fine & 255, typ, 128, rel & 255, 0]) + nm
    delta = np.diff(np.concatenate([[0], d.astype(np.int16)])).astype(np.int8).tobytes()
    return hdr + sh + delta

def write(path, name, channels, patterns, orders, instruments, speed=6, bpm=125, restart=0):
    """patterns: list of (rows, list of rows, each a list of `channels` encoded cells); instruments: list of bytes from instrument()."""
    h = b"Extended Module: " + name.encode("latin-1")[:20].ljust(20, b" ") + b"\x1a" + b"corp_build".ljust(20, b" ") + struct.pack("<H", 0x0104)
    body = struct.pack("<IHHHHHHHH", 276, len(orders), restart, channels, len(patterns), len(instruments), 1, speed, bpm) + bytes(orders) + bytes(256 - len(orders))
    out = bytearray(h + body)
    for rows, data in patterns:
        packed = b"".join(c for r in data for c in r)
        out += struct.pack("<IBHH", 9, 0, rows, len(packed)) + packed
    for ins in instruments: out += ins
    open(path, "wb").write(bytes(out))
