#!/usr/bin/env python3
"""Regression for BORE 1's direct Koala Winner song and debug-only predecessors.

No numpy, scipy or emulator required for this metadata/arrangement check.
"""
import hashlib
import lzma
from pathlib import Path
import re
import struct

ROOT = Path(__file__).resolve().parent.parent
EXPECTED = "b3d5d2cda538f1987c34292f2e94d2218f217c3811bfe0321cd0ab33e61279a4"

def assert_project():
    raw = lzma.decompress((ROOT / "tools/winner_koala.xm.xz").read_bytes())
    assert hashlib.sha256(raw).hexdigest() == EXPECTED
    assert raw.startswith(b"Extended Module: ")
    songlen, restart, nch, npats, ninst, flags, speed, bpm = struct.unpack_from("<8H", raw, 64)
    assert (songlen, restart, nch, npats, ninst, flags, speed, bpm) == (206, 0, 16, 206, 23, 1, 2, 170)
    assert raw[80:80+songlen] == bytes(range(206))

    # Independently decode every tracker event, not merely the XM header.
    pos = 60 + struct.unpack_from("<I", raw, 60)[0]
    notes = 0
    instruments = set()
    for _ in range(npats):
        hlen, packing, nrows, packed = struct.unpack_from("<IBHH", raw, pos)
        assert packing == 0 and nrows == 64
        q, end = pos + hlen, pos + hlen + packed
        for _ in range(nrows * nch):
            byte = raw[q]
            q += 1
            if byte & 128:
                mask = byte & 31
                note, inst = 0, 0
                if mask & 1: note=raw[q];q+=1
                if mask & 2: inst=raw[q];q+=1
                if mask & 4: q+=1
                if mask & 8: q+=1
                if mask & 16: q+=1
            else:
                note, inst = byte, raw[q]
                q += 4
            if note:
                assert 1 <= note <= 96 and 1 <= inst <= 23
                notes += 1
                instruments.add(inst)
        assert q == end, ("bad pattern length", q, end)
        pos = end
    assert notes == 8813, notes
    assert len(instruments) == 23, instruments

    songs = (ROOT / "source/songs.h").read_text(encoding="utf8")
    assert len(re.findall(r"^SONG_(?:XM|ADP)\(", songs, re.M)) <= 64
    assert 'SONG_XM(winner_koala,"CORPORATIONS (YOU\'RE WINNER)","tools/winner_koala.xm")' in songs
    assert 'SONG_ADP(corporations,"CORPORATIONS (ORIGINAL)","source/music/corporations.adp")' in songs
    assert 'SONG_XM(youre_winner,"YOU\'RE WINNER (ORIGINAL)","tools/youre_winner.xm")' in songs
    assert songs.count('SONG_XM(winner_koala,') == 1
    unlocks = (ROOT / "source/unlocks.h").read_text()
    for old in ("CORPORATIONS (ORIGINAL)", "YOU'RE WINNER (ORIGINAL)"):
        assert old not in unlocks, "old song must be secret, not earnable"
    main = (ROOT / "source/main.c").read_text()
    assert 'static int isDbgSong(int i)' in main
    assert 'const char orig[]=" (ORIGINAL)"' in main
    assert '!isDbgSong(i)&&!isLockedSong(i)' in main
    print("Winner original Koala: 8813 notes, 206 bars, 23 sampled instruments, 16 channels, two secret predecessors: PASS")

if __name__ == "__main__":
    assert_project()
