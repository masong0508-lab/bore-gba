#!/usr/bin/env python3
"""Fast SDK-free port image/manifest selftest (no real ROM distributed)."""
import pathlib
import struct
import tempfile
import zipfile
import hashlib
import json

import package_port_candidates as pack

def nds(name, bad_crc=False):
    data = bytearray(0x8000)
    data[0x0C:0x10] = b"GGDE"
    data[0x12] = 3 if name == "godguy_dsi_mode.nds" else 0
    if data[0x12] == 3:
        struct.pack_into("<I", data, 0x1B0, 0xFFFFFFFF)
        struct.pack_into("<I", data, 0x210, len(data))
    struct.pack_into("<4I", data, 0x20, 0x4000, 0x02000800,
                     0x02000000, 0x1000)
    struct.pack_into("<4I", data, 0x30, 0x6000, 0x02380000,
                     0x02380000, 0x1000)
    struct.pack_into("<H", data, 0x15E, pack.crc16(data[:0x15E]))
    if bad_crc:
        data[0x15E] ^= 1
    return bytes(data)

def dsx():
    data = bytearray(0x1000)
    data[:4] = b"3DSX"
    struct.pack_into("<H", data, 4, 0x20)
    return bytes(data)

def main():
    with tempfile.TemporaryDirectory() as tmp:
        root = pathlib.Path(tmp)
        a = root / "nds"
        b = root / "cinema"
        out = root / "candidate"
        a.mkdir();b.mkdir()
        for name in pack.REQUIRED_DS:
            data = nds(name)
            pack.inspect(data, name)
            (a / name).write_bytes(data)
        (a / pack.GAME_3DSX).write_bytes(dsx())
        (b / pack.GPU_3DSX).write_bytes(dsx())
        for name in (pack.GAME_3DSX,pack.GPU_3DSX):
            pack.inspect(dsx(),name)
        assert pack.main(["--artifact-dir",str(a),
                          "--artifact-dir",str(b),
                          "--output",str(out),
                          "--commit","0123456789abcdef0123456789abcdef01234567"]) == 0
        playable = out/"godguy_0123456789_playable.zip"
        experimental=out/"godguy_0123456789_experimental.zip"
        with zipfile.ZipFile(playable) as z:
            assert "godguy_hw.nds" in z.namelist()
            assert "godguy_new3ds_gpu_probe.3dsx" not in z.namelist()
            assert "godguy_dsi_mode.nds" not in z.namelist()
            assert "README_PORTS.txt" in z.namelist()
            manifest=json.loads(z.read("PORTS_MANIFEST.json"))
            assert manifest["package"] == "playable"
            line=hashlib.sha256(z.read("godguy_hw.nds")).hexdigest()
            assert line in z.read("SHA256SUMS.txt").decode()
        with zipfile.ZipFile(experimental) as z:
            assert "godguy_new3ds_gpu_probe.3dsx" in z.namelist()
            assert "godguy_dsi_mode.nds" in z.namelist()
        for broken,label in (
            (nds("godguy_hw.nds", bad_crc=True),"godguy_hw.nds"),
            (nds("godguy_hw.nds")[:0x6200],"godguy_hw.nds"),
            (b"bad" + dsx(),"godguy.3dsx"),
        ):
            try:
                pack.inspect(broken,label)
            except ValueError:
                pass
            else:
                raise AssertionError("malformed ROM was accepted")
        # Incompatible unit codes and mismatched duplicates fail closed.
        try:
            pack.inspect(nds("godguy_hw.nds"),"godguy_dsi_mode.nds")
        except ValueError:
            pass
        else:
            raise AssertionError("DS image mislabeled as DSi-mode passed validation")
        (b/"godguy_hw.nds").write_bytes(nds("godguy_hw.nds") + b"!")
        try:
            pack.scan([a,b])
        except ValueError as exc:
            assert "Duplicate ROMs differ" in str(exc)
        else:
            raise AssertionError("conflicting port artifact accepted")
    print("GODGUY PORT PACKAGING TEST OK: format checks, ROM labels, deterministic ZIP, checksums and deliberate corruption")

if __name__ == "__main__":
    main()
