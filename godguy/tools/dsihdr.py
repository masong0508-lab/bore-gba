#!/usr/bin/env python3
"""dsihdr.py ROM.nds : make a DS ROM a DSi-mode ROM (unit code 3). Fills the DSi header fields a loader reads (region flags, title id, total size, no ARM9i/ARM7i), fixes the header CRC.
No RSA signature (homebrew loaders such as TWiLight Menu++ / nds-bootstrap or unlaunch do not need one). EXPERIMENTAL: never run on a real DSi from here."""
import struct, sys
def crc16(d):
    c = 0xFFFF
    for b in d:
        c ^= b
        for _ in range(8): c = (c >> 1) ^ 0xA001 if c & 1 else c >> 1
    return c
def patch(rom):
    r = bytearray(rom)
    if len(r) < 0x4000: r += bytes(0x4000 - len(r))
    r[0x12] = 3
    struct.pack_into('<I', r, 0x1B0, 0xFFFFFFFF)                 # region flags: any region
    for o in (0x1C0, 0x1C4, 0x1C8, 0x1CC, 0x1D0, 0x1D4, 0x1D8, 0x1DC): struct.pack_into('<I', r, o, 0)   # no ARM9i / ARM7i binaries
    struct.pack_into('<I', r, 0x210, len(rom))                   # total ROM size
    struct.pack_into('<II', r, 0x230, (0x00030004 << 0) & 0xFFFFFFFF, 0)
    r[0x230:0x234] = r[0x0C:0x10][::-1]; struct.pack_into('<I', r, 0x234, 0x00030004)   # title id: game code reversed, then 00030004
    struct.pack_into('<H', r, 0x15E, crc16(bytes(r[0:0x15E])))
    return bytes(r)
if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read(); open(sys.argv[1], 'wb').write(patch(d)); print('DSi-mode header written to', sys.argv[1])
