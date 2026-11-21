#!/usr/bin/env python3
"""Inspect GodGuy compiled console images and create explicit release candidates.

Packaging is NOT proof of on-device startup. Never promote a passing
cross-compiler to an emulator/hardware certification. No external modules.
"""
import argparse
import hashlib
import json
import pathlib
import struct
import zipfile

REQUIRED_DS = (
    "godguy_hw.nds",
    "godguy_dsi.nds",
    "godguy_dsi_mode.nds",
    "godguy_hw_safe.nds",
    "godguy_hw_audio_only.nds",
    "godguy_hw_save_only.nds",
)
OPTIONAL_DS = ("godguy.nds", "godguy_hw_nocache.nds")
GAME_3DSX = "godguy.3dsx"
GPU_3DSX = "godguy_new3ds_gpu_probe.3dsx"
ALL = REQUIRED_DS + OPTIONAL_DS + (GAME_3DSX, GPU_3DSX)
PACKAGES = {
    "playable": (
        "godguy.3dsx", "godguy_hw.nds", "godguy_dsi.nds", "godguy.nds",
    ),
    "experimental": (
        "godguy_new3ds_gpu_probe.3dsx", "godguy_dsi_mode.nds",
        "godguy_hw_safe.nds", "godguy_hw_audio_only.nds",
        "godguy_hw_save_only.nds", "godguy_hw_nocache.nds",
    ),
}

def crc16(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    return crc

def inspect_nds(data, name):
    if len(data) < 0x4000:
        raise ValueError(f"{name}: truncated NDS header ({len(data)} bytes)")
    if data[0x0C:0x10] != b"GGDE":
        raise ValueError(f"{name}: unexpected Nintendo DS game code")
    unit = data[0x12]
    expected = 3 if name == "godguy_dsi_mode.nds" else 0
    if unit != expected:
        raise ValueError(f"{name}: unit-code {unit}, expected {expected}")
    if unit == 3:
        # dsihdr.py intentionally produces unsigned DSi homebrew. A
        # mislabeled 4 MiB DS image must not be sold as 16 MiB DSi mode.
        region = struct.unpack_from("<I", data, 0x1B0)[0]
        total = struct.unpack_from("<I", data, 0x210)[0]
        if region != 0xFFFFFFFF or total != len(data):
            raise ValueError(f"{name}: invalid DSi homebrew region/length")
        if any(data[0x1C0:0x1E0]):
            raise ValueError(f"{name}: unexpected ARM9i/ARM7i segments")
    claimed_crc = struct.unpack_from("<H", data, 0x15E)[0]
    if crc16(data[:0x15E]) != claimed_crc:
        raise ValueError(f"{name}: Nintendo DS header CRC mismatch")
    records = {}
    for label, off in (("arm9", 0x20), ("arm7", 0x30)):
        rom, entry, address, size = struct.unpack_from("<4I", data, off)
        if rom < 0x200 or not 0 < size <= 24*1024*1024:
            raise ValueError(f"{name}: {label} segment offset/size invalid")
        if rom + size > len(data) or not entry or not address:
            raise ValueError(f"{name}: {label} segment outside ROM")
        if not address <= entry < address + size:
            # Some loaders have separate or unconventional entry points.
            # Still require a valid address range, avoiding false claims.
            raise ValueError(f"{name}: {label} entry lies outside RAM segment")
        records[label + "_size"] = size
    if name != "godguy_dsi_mode.nds" and len(data) > 8*1024*1024:
        raise ValueError(f"{name}: DS-compatible image unexpectedly oversized")
    return {"type": "Nintendo DS ROM", "unit_code": unit, **records}

def inspect_3dsx(data, name):
    if len(data) < 0x40 or data[:4] != b"3DSX":
        raise ValueError(f"{name}: invalid or truncated 3DSX executable")
    header_size = struct.unpack_from("<H", data, 4)[0]
    if not 0x20 <= header_size <= 0x200:
        raise ValueError(f"{name}: invalid 3DSX header size")
    if len(data) > 64*1024*1024:
        raise ValueError(f"{name}: image exceeds candidate budget")
    return {"type": "New-series Nintendo 3DS homebrew", "header_size": header_size}

def inspect(data, name):
    if name.endswith(".nds"):
        return inspect_nds(data, name)
    if name.endswith(".3dsx"):
        return inspect_3dsx(data, name)
    raise ValueError(f"Unsupported ROM format: {name}")

def scan(roots, alternate_builds=None):
    available = {}
    for root in roots:
        root = pathlib.Path(root)
        if not root.is_dir():
            raise ValueError(f"Artifact directory not found: {root}")
        for path in sorted(root.rglob("*")):
            if not path.is_file() or path.name not in ALL:
                continue
            data = path.read_bytes()
            if path.name in available:
                if available[path.name] != data:
                    if path.name != GAME_3DSX:
                        raise ValueError(f"Duplicate ROMs differ: {path.name}")
                    # Independent 3DS CI jobs can compile with different
                    # package installation sequences. Keep the main mixed
                    # console artifact canonical; record the SECOND
                    # fully-validated executable rather than silently
                    # mixing it into the download or blocking DS bundles.
                    inspected = inspect(data, path.name)
                    if alternate_builds is not None:
                        alternate_builds.append({
                            "file": path.name,
                            "role": "separate 3DS cinema job, not distributed",
                            "sha256": hashlib.sha256(data).hexdigest(),
                            "bytes": len(data),
                            **inspected,
                        })
                continue
            available[path.name] = data
    return available

def readme(kind, commit, package_files, alternates=None):
    lines = [
        f"GodGuy {kind.upper()} ports candidate",
        f"Git commit: {commit}",
        "",
        "IMPORTANT: GitHub CI verifies compiler, ROM structure, generated",
        "assets and host regressions. It does NOT certify console/emulator",
        "startup, gameplay, cartridge save access or frame rate.",
        "",
    ]
    if kind == "playable":
        lines += [
            "godguy.3dsx: requires New 3DS, New 3DS XL or New 2DS XL.",
            "  This is the existing playable SOFTWARE renderer, not the",
            "  experimental PICA200 GPU prototype.",
            "godguy_hw.nds: direct-hardware DS build; cartridge/save-state",
            "  behavior must be tested on the target emulator/flashcart.",
            "godguy_dsi.nds: DSi-tuned ROM running in compatible DS mode.",
            "godguy.nds (if present): experimental libnds/Calico build.",
        ]
    else:
        lines += [
            "godguy_new3ds_gpu_probe.3dsx: separate PICA200 world test;",
            "  NOT the live GodGuy game; New 3DS/New 2DS XL only.",
            "godguy_dsi_mode.nds: EXPERIMENTAL 16 MiB DSi mode with",
            "  unsigned homebrew header. Do not run on original DS.",
            "godguy_hw_safe.nds: no sound or save diagnostics.",
            "godguy_hw_audio_only.nds: saves disabled diagnostic.",
            "godguy_hw_save_only.nds: audio disabled diagnostic.",
            "godguy_hw_nocache.nds: optional memory-cache diagnostic.",
        ]
    if alternates:
        lines += [
            "",
            "REPRODUCIBILITY NOTE: The separately compiled 3DS cinema job",
            "produced a valid but byte-different executable. The main",
            "mixed-port build is canonical. Both SHA-256 digests and",
            "sizes are recorded in PORTS_MANIFEST.json for investigation.",
        ]
    lines += ["", "Included files:", *(" - " + x for x in package_files),
              "", "SHA256SUMS.txt lists the exact binaries inside this ZIP.",
              "Treat this as a release candidate until hardware QA passes."]
    return "\n".join(lines) + "\n"

def write_zip(path, files, readme_text, manifest):
    # Stable ordering, timestamps and permissions prevent ZIP drift.
    with zipfile.ZipFile(path, "w", compression=zipfile.ZIP_DEFLATED,
                         compresslevel=6) as z:
        blobs = {name: files[name] for name in files}
        blobs["README_PORTS.txt"] = readme_text.encode("utf-8")
        blobs["PORTS_MANIFEST.json"] = (json.dumps(manifest, indent=2,
                                            sort_keys=True) + "\n").encode()
        checksums = "".join(
            hashlib.sha256(blobs[name]).hexdigest() + "  " + name + "\n"
            for name in sorted(blobs))
        blobs["SHA256SUMS.txt"] = checksums.encode()
        for name in sorted(blobs):
            info = zipfile.ZipInfo(name, date_time=(2026, 11, 21, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            z.writestr(info, blobs[name], compress_type=zipfile.ZIP_DEFLATED)

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifact-dir", action="append", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--commit", required=True)
    args = parser.parse_args(argv)
    if len(args.commit) < 7 or not all(c in "0123456789abcdef" for c in args.commit.lower()):
        parser.error("--commit must be a Git SHA")
    alternates = []
    image = scan(args.artifact_dir, alternates)
    mandatory = set(REQUIRED_DS + (GAME_3DSX, GPU_3DSX))
    missing = sorted(mandatory - set(image))
    if missing:
        raise ValueError("Required compiled images absent: " + ", ".join(missing))
    records = {}
    for name, data in sorted(image.items()):
        fields = inspect(data, name)
        records[name] = {
            "sha256": hashlib.sha256(data).hexdigest(),
            "bytes": len(data),
            **fields,
        }
        print(f"VERIFIED IMAGE {name}: {len(data)} bytes")
    destination = pathlib.Path(args.output)
    destination.mkdir(parents=True, exist_ok=True)
    manifest = {
        "git_commit": args.commit,
        "qualification": "automated-build-and-format-only; hardware unverified",
        "artifacts": records,
        "alternate_builds": alternates,
    }
    stem = f"godguy_{args.commit[:10]}"
    for kind, names in PACKAGES.items():
        selected = {name:image[name] for name in names if name in image}
        submanifest = {**manifest,"package":kind,
                       "contents":sorted(selected)}
        out = destination / f"{stem}_{kind}.zip"
        write_zip(out, selected,
                  readme(kind,args.commit,sorted(selected),alternates),
                  submanifest)
        print(f"CREATED {out} ({out.stat().st_size} compressed bytes)")
    (destination / "PORTS_MANIFEST.json").write_text(
        json.dumps(manifest,indent=2,sort_keys=True)+"\n",
        encoding="utf-8")
    print("GODGUY PORT IMAGES OK: verified NDS/3DSX headers and made labeled candidate bundles")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
