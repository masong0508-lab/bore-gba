#!/usr/bin/env python3
"""Convert source-rendered P6 400x240 cinema RGB into lossless PNG.
Uses the Python standard library only; no Pillow dependency or SDK."""
from pathlib import Path
import struct
import sys
import zlib

def chunk(name, data):
    payload = name + data
    return (struct.pack(">I", len(data)) + payload +
            struct.pack(">I", zlib.crc32(payload) & 0xFFFFFFFF))

def convert(path):
    with path.open("rb") as f:
        assert f.readline() == b"P6\n"
        wh = f.readline().strip()
        assert wh == b"400 240", wh
        assert f.readline() == b"255\n"
        data = f.read()
    width, height = 400, 240
    assert len(data) == width * height * 3, path
    rows = b"".join(b"\x00" + data[y * width * 3:(y + 1) * width * 3]
                    for y in range(height))
    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB",width,height,8,2,0,0,0)) +
           chunk(b"IDAT",zlib.compress(rows,6)) +
           chunk(b"IEND",b""))
    path.with_suffix(".png").write_bytes(png)

def main():
    folder = Path(sys.argv[1])
    paths = sorted(folder.glob("*.ppm"))
    if len(paths) < 15:
        raise SystemExit("Missing GodGuy native-resolution reference captures")
    for path in paths:
        convert(path)
    print(f"3DS CAPTURE PNG OK: {len(paths)} source-rendered native frames")

if __name__=="__main__":
    main()
