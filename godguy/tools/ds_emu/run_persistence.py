#!/usr/bin/env python3
"""Run a real DS ROM through saving, process exit, cold Continue and autosave."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import select
import shutil
import struct
import subprocess
import time


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def archive(path):
    blob = path.read_bytes()
    if len(blob) < 12 or blob[:3] != b"GGS" or not blob[3]:
        raise ValueError(f"{path.name}: cartridge archive header is missing")
    packed, length = struct.unpack_from("<II", blob, 4)
    if not 0 < packed <= len(blob) - 12 or not 76 <= length <= 16 * 1024 * 1024:
        raise ValueError(f"{path.name}: invalid archive lengths")
    source = memoryview(blob)[12:12 + packed]
    raw = bytearray()
    at = 0
    while at < packed:
        control = source[at]
        at += 1
        if control < 128:
            count = control + 1
            if at + count > packed:
                raise ValueError("truncated literal run")
            raw.extend(source[at:at + count])
            at += count
        else:
            if at == packed:
                raise ValueError("truncated repeated run")
            raw.extend(bytes([source[at]]) * (control - 126))
            at += 1
        if len(raw) > length:
            raise ValueError("archive expands beyond its declared length")
    if len(raw) != length or raw[:4] != b"GGS1":
        raise ValueError("decoded world length or signature is invalid")
    return {
        "archive_bytes": packed + 12,
        "raw_world_bytes": length,
        "save_version": struct.unpack_from("<H", raw, 4)[0],
        "map_width": struct.unpack_from("<H", raw, 6)[0],
        "tick": struct.unpack_from("<I", raw, 12)[0],
        "year": struct.unpack_from("<I", raw, 16)[0],
        "power_uses": struct.unpack_from("<I", raw, 32)[0],
        "world_seed": struct.unpack_from("<I", raw, 44)[0],
        "creatures": struct.unpack_from("<H", raw, 48)[0] | raw[73] << 16,
        "camera": list(struct.unpack_from("<HH", raw, 52)),
        "chapter": raw[56],
        "story_on": raw[58],
        "file_sha256": digest(path),
        "archive_sha256": hashlib.sha256(blob[:packed + 12]).hexdigest(),
    }


def run_session(frontend, core, rom, output, name, script, save_file):
    env = dict(os.environ, GG_EMU_SESSION=name)
    process = subprocess.Popen(
        [str(frontend), str(core), str(rom), str(output)],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT, env=env,
    )
    buffered = bytearray()
    log = (output / f"{name}_status.log").open("w")

    def until(prefix):
        deadline = time.monotonic() + 120
        while True:
            while b"\n" not in buffered:
                left = deadline - time.monotonic()
                if left <= 0 or not select.select([process.stdout], [], [], left)[0]:
                    raise TimeoutError(f"{name}: waiting for {prefix.strip()}")
                part = os.read(process.stdout.fileno(), 4096)
                if not part:
                    raise RuntimeError(f"{name}: frontend stopped before {prefix.strip()}")
                buffered.extend(part)
            line, _, rest = buffered.partition(b"\n")
            buffered[:] = rest
            message = line.decode("utf-8", errors="replace")
            log.write(message + "\n")
            log.flush()
            if message.startswith(prefix):
                return message

    try:
        until("READY ")
        for command in script.read_text().splitlines():
            command = command.strip()
            if not command or command.startswith("#"):
                continue
            words = command.split()
            if words[0] == "quit":
                break
            if len(words) != 6 or words[0] != "run":
                raise ValueError("persistence scripts permit only run and quit; no savestates")
            process.stdin.write((command + "\n").encode())
            process.stdin.flush()
            label = words[2]
            until(f"CAPTURE {label} ")
            if name == "first" and label == "save_complete":
                shutil.copy2(save_file, output / "explicit_save.dsv")
            if name == "restart" and label == "cold_reloaded_world":
                shutil.copy2(save_file, output / "cold_reloaded_snapshot.dsv")
            if label in {"painted_world", "save_complete", "saved_title",
                         "same_session_reload", "cold_reloaded_world",
                         "live_soak_first", "live_soak_second"}:
                print(f"{name}: {label}", flush=True)
        process.stdin.write(b"quit\n")
        process.stdin.close()
        if process.wait(timeout=30):
            raise RuntimeError(f"{name}: frontend returned {process.returncode}")
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--core", type=Path, required=True, help="DeSmuME libretro shared library")
    parser.add_argument("--include", type=Path, required=True, help="directory containing libretro.h")
    parser.add_argument("--rom", type=Path, required=True, help="GodGuy direct-hardware NDS ROM")
    parser.add_argument("--output", type=Path, required=True, help="new, empty evidence directory")
    args = parser.parse_args()
    core = args.core.resolve(strict=True)
    include = args.include.resolve(strict=True)
    rom = args.rom.resolve(strict=True)
    if not (include / "libretro.h").is_file():
        parser.error("--include must contain libretro.h")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).resolve().parent
    frontend = output / "headless_libretro"
    subprocess.run([
        os.environ.get("CC", "cc"), "-O2", "-std=c11", "-Wall", "-Wextra", "-Werror",
        "-I", str(include), str(source / "headless_libretro.c"),
        "-ldl", "-lm", "-o", str(frontend),
    ], check=True)
    for name in ("first.inputs", "restart.inputs"):
        shutil.copy2(source / name, output / name)
    save_file = output / (rom.stem + ".dsv")
    run_session(frontend, core, rom, output, "first", output / "first.inputs", save_file)
    shutil.copy2(save_file, output / "before_cold_restart.dsv")
    run_session(frontend, core, rom, output, "restart", output / "restart.inputs", save_file)
    explicit = archive(output / "explicit_save.dsv")
    before = archive(output / "before_cold_restart.dsv")
    cold = archive(output / "cold_reloaded_snapshot.dsv")
    after = archive(save_file)
    if before["file_sha256"] != cold["file_sha256"]:
        raise AssertionError("cold Continue changed the persisted archive before autosave")
    for key in ("world_seed", "map_width", "chapter", "story_on", "power_uses"):
        if before[key] != after[key]:
            raise AssertionError(f"world identity/progress changed after restart: {key}")
    if before["save_version"] != 14 or before["power_uses"] != 1 or not before["creatures"]:
        raise AssertionError("saved world is missing the scripted first chapter progress")
    if after["tick"] <= before["tick"] or after["archive_sha256"] == before["archive_sha256"]:
        raise AssertionError("world did not advance and autosave after the cold restart")
    steps = [json.loads(line) for line in (output / "restart_steps.jsonl").read_text().splitlines()]
    soak = [step for step in steps if step["label"].startswith("live_soak_")]
    if len(soak) != 2 or any(not step["changed_frames"] or not step["nonzero_audio_samples"] for step in soak):
        raise AssertionError("missing moving video or nonzero audio in a soak interval")
    if any((step["width"], step["height"]) != (256, 384) for step in steps):
        raise AssertionError("unexpected dual-screen output dimensions")
    logs = "\n".join((output / f"{name}_emulator.log").read_text() for name in ("first", "restart"))
    stale_commands = logs.count("Unhandled Backup Memory command 06")
    if stale_commands or list(output.glob("*.state")):
        raise AssertionError("stale SPI commands or emulator savestates found")
    report = {
        "rom_sha256": digest(rom), "core_sha256": digest(core),
        "harness_sha256": digest(source / "headless_libretro.c"),
        "explicit_in_game_save": explicit, "cold_restart_save": before,
        "cold_snapshot_identical": True, "post_soak_autosave": after,
        "archive_and_runtime_checks_pass": True,
        "cold_restart_used_state_files": False,
        "soak_emulated_frames": sum(step["video_frames"] for step in soak),
        "soak_changed_frames": sum(step["changed_frames"] for step in soak),
        "soak_nonzero_audio_samples": sum(step["nonzero_audio_samples"] for step in soak),
        "soak_audio_peak": max(step["audio_peak"] for step in soak),
        "stale_spi_command_errors": stale_commands,
        "visual_review_required": [
            "first_painted_world.ppm", "first_save_complete.ppm",
            "first_saved_title.ppm", "restart_cold_boot_title.ppm",
            "restart_cold_reloaded_world.ppm", "restart_live_soak_second.ppm",
        ],
    }
    (output / "runtime_acceptance.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"PASS: persisted progress, cold Continue, moving video/audio and autosave. Evidence: {output}")
    print("Inspect the listed screenshots before accepting the visual behavior.")


if __name__ == "__main__":
    main()
