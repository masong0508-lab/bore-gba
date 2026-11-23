#!/usr/bin/env python3
"""Convert source-rendered P6 400x240 cinema RGB into lossless PNG.
Uses the Python standard library only; no Pillow dependency or SDK."""
from pathlib import Path
import struct
import sys
import zlib

GG_PERSON_ATLAS_W = 32 * 3 * 8
GG_PERSON_ATLAS_H = 48 * 3 * 6

def chunk(name, data):
    payload = name + data
    return (struct.pack(">I", len(data)) + payload +
            struct.pack(">I", zlib.crc32(payload) & 0xFFFFFFFF))

def convert(path):
    with path.open("rb") as f:
        assert f.readline() == b"P6\n"
        wh = f.readline().strip()
        parts = wh.split()
        assert len(parts) == 2, wh
        width, height = (int(x) for x in parts)
        assert 0 < width <= 2048 and 0 < height <= 2048, wh
        # The 400x240 gameplay regression remains strict. The shared
        # DS/DSi/3DS menu UI is natively 256x192, and only the two
        # specifically named restored-globe snapshots use that size.
        # The explicitly named 8-view sprite atlas remains larger.
        expected = ((GG_PERSON_ATLAS_W, GG_PERSON_ATLAS_H)
                    if path.name in ("procedural_people_turntable.ppm",
                                     "people_8_direction_turntable.ppm")
                    else (256, 192)
                    if path.name in ("title_globe_ds.ppm",
                                     "title_globe_new3ds.ppm")
                    else (400, 240))
        assert (width, height) == expected, (path, wh, expected)
        assert f.readline() == b"255\n"
        data = f.read()
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
    # Preserve 15-frame minimum for cinematic regression. A dedicated
    # single-sheet artist review uses --single without bypassing its
    # own strict size and source-format checks.
    single = "--single" in sys.argv[2:]
    if not paths or (len(paths) < 15 and not single):
        raise SystemExit("Missing GodGuy native-resolution reference captures")
    for path in paths:
        convert(path)
    print(f"3DS CAPTURE PNG OK: {len(paths)} source-rendered frame(s) converted")

if __name__=="__main__":
    main()
