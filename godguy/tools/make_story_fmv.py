#!/usr/bin/env python3
"""Render GodGuy's story clips and pack their indexed scanlines for the DS."""
from itertools import groupby
from math import cos, sin, sqrt
from pathlib import Path
from random import Random

from PIL import Image, ImageDraw


WIDTH, HEIGHT, FRAMES, FRAME_TICKS = 128, 72, 48, 5
PALETTE = (
    (9, 9, 24), (18, 18, 43), (35, 28, 60), (58, 41, 75),
    (98, 53, 83), (154, 71, 89), (219, 105, 96), (250, 168, 106),
    (255, 226, 163), (174, 214, 209), (86, 158, 159), (45, 99, 111),
    (26, 59, 82), (26, 42, 69), (62, 113, 89), (108, 149, 103),
)
STARS = list(zip(Random(37).sample(range(WIDTH), 46),
                 Random(82).choices(range(50), k=46)))


def sky(draw, colors):
    for y, color in enumerate(colors):
        draw.line((0, y, WIDTH - 1, y), fill=color)


def stars(draw, t, limit=1):
    for i, (x, y) in enumerate(STARS):
        if i < len(STARS) * limit:
            draw.point(((x - t // 4) % WIDTH, y), fill=9 if (i + t // 5) % 9 == 0 else 3)
    x = 104 - t // 4
    draw.line((x - 2, 11, x + 2, 11), fill=3)
    draw.line((x, 9, x, 13), fill=3)
    draw.point((x, 11), fill=9)


def orbit(frame, draw, t):
    sky(draw, [0] * 46 + [1] * 9 + [2] * 10 + [1] * 7)
    stars(draw, t)
    cx, cy, radius = 67 - t // 5, 43 + t // 4, 19 + t // 2
    draw.ellipse((cx - radius - 1, cy - radius - 1,
                  cx + radius + 1, cy + radius + 1), fill=12)
    pixels = frame.load()
    ca, sa = cos(t / 34), sin(t / 34)
    for y in range(max(0, cy - radius), min(HEIGHT, cy + radius + 1)):
        v = (y - cy) / radius
        for x in range(cx - radius, cx + radius + 1):
            u = (x - cx) / radius
            if u*u + v*v > 1:
                continue
            z = sqrt(max(0, 1 - u*u - v*v))
            light = -0.67*u - 0.5*v + 0.35*z
            a, b = u*ca + z*sa, z*ca - u*sa
            land = sin(a*8 + v*3) + sin(v*9 - b*4) + 0.6*sin(a*19 + v*11)
            cloud = sin(v*29 + 3*sin(a*7) + b*4) > 0.96 and z > 0.45
            if light < -0.08:
                color = 0 if light < -0.4 else 13
            elif cloud and light > 0.40:
                color = 9 if light > 0.71 else 10
            elif land > 0.42:
                color = 15 if light > 0.58 else 14 if light > 0.28 else 11
            else:
                color = 10 if light > 0.68 else 11 if light > 0.28 else 12
            if z < 0.27 and light > 0.47:
                color = 9 if z < 0.16 else 10
            pixels[x, y] = color


RIDGES = (
    ((0, 52), (9, 44), (15, 47), (28, 38), (36, 45), (43, 43), (58, 53),
     (72, 47), (86, 39), (96, 48), (108, 36), (123, 48), (139, 41)),
    ((-12, 57), (4, 49), (12, 46), (21, 48), (34, 45), (42, 51), (64, 58),
     (83, 55), (95, 49), (104, 52), (120, 46), (133, 48), (149, 44)),
)


def ridge(draw, peaks, offset, color):
    draw.polygon([(x - offset, y) for x, y in peaks] +
                 [(WIDTH + 30, 59), (-30, 59)], fill=color)


def house(draw, x, y, size, lit):
    draw.rectangle((x, y - size, x + 2*size, y), fill=13)
    draw.polygon(((x - 2, y - size), (x + size, y - 2*size - 1),
                  (x + 2*size + 2, y - size)), fill=2)
    draw.line((x + size, y - 2*size, x + 2*size + 1, y - size), fill=4)
    draw.rectangle((x + size - 1, y - 3, x + size, y - 2), fill=lit)


def coast(draw, t, settled):
    dawn = t if settled else t + 5
    band = (1, 2, 3, 4, 5, 6, 7, 8)
    sky(draw, [band[min(7, max(0, (y + dawn - 14) // 8))] for y in range(HEIGHT)])
    if dawn < 21:
        stars(draw, t, (21 - dawn) / 21)
    sun_y = 57 - dawn // 2
    draw.ellipse((51, sun_y - 10, 71, sun_y + 10), fill=8)
    for i in range(4):
        x = (t + i * 37) % 145 - 12
        y = 17 + i * 7
        draw.line((x, y, x + 16 + i * 2, y), fill=3 + (dawn > 19))
        draw.line((x + 4, y + 1, x + 26, y + 1), fill=3 + (dawn > 24))
    ridge(draw, RIDGES[0], t // 6, 4)
    ridge(draw, RIDGES[1], t // 4, 11 if dawn >= 18 else 12)
    draw.rectangle((0, 56, 127, 71), fill=12)
    for i in range(9):
        y = 56 + (i*7 + t // 2) % 16
        x = 61 + (i*11 % 17 - 8)
        draw.line((x - i - 2, y, x + i + 2, y), fill=7 if i % 2 else 10)
        x = (i*31 + t) % 127
        draw.line((x, y, x + i + 3, y), fill=11)
    offset = t // 3
    draw.polygon([(x - offset, y) for x, y in
                  ((-20, 56), (6, 54), (28, 56), (40, 60), (47, 65),
                   (43, 67), (56, 71), (-20, 71))], fill=1)
    draw.polygon([(x - offset, y) for x, y in
                  ((94, 72), (97, 65), (105, 59), (119, 57), (131, 52),
                   (150, 51), (150, 72))], fill=1)
    if settled:
        for i, (x, y, size) in enumerate(((5, 62, 4), (20, 58, 5), (34, 61, 4),
                                          (47, 66, 2), (106, 64, 4), (122, 59, 5),
                                          (136, 57, 7))):
            house(draw, x - offset, y, size, 8 if (t // 5 + i) % 7 else 6)
        x = 23 - offset
        drift = t % 11
        draw.point((x + drift // 3, 46 - drift // 2), fill=9)
        draw.point((x + 3 + drift // 2, 43 - drift // 4), fill=10)
    else:
        for x, y, size in ((5, 62, 9), (21, 58, 10), (28, 60, 5), (122, 63, 13)):
            x -= offset
            draw.line((x, y - size, x, y), fill=0)
            for j in range(3):
                draw.polygon(((x, y - size + j*2), (x - j - 2, y - size + 4 + j*2),
                              (x + j + 2, y - size + 4 + j*2)), fill=13)
    if dawn > 14:
        for i in range(3):
            x, y = 42 + t + i*9, 35 - t//5 - (i % 2)*4
            wing = 1 if (t + i) % 4 < 2 else -1
            draw.line((x - 2, y - wing, x, y, x + 2, y - wing), fill=1)


def render(ending, t):
    frame = Image.new("P", (WIDTH, HEIGHT))
    frame.putpalette([channel for rgb in PALETTE for channel in rgb])
    draw = ImageDraw.Draw(frame)
    if ending:
        coast(draw, t, True)
    elif t < 26:
        orbit(frame, draw, t)
    else:
        coast(draw, t - 26, False)
    return frame


def encode_row(row):
    # One byte per run: high nibble is length minus one, low nibble is color.
    code = bytearray()
    for color, pixels in groupby(row):
        length = sum(1 for _ in pixels)
        while length:
            run = min(16, length)
            code.append(((run - 1) << 4) | color)
            length -= run
    return bytes(code)


def c_array(out, name, ctype, values, width):
    out.write(f"static const {ctype} {name}[] = {{\n")
    for i in range(0, len(values), width):
        out.write("    " + ",".join(map(str, values[i:i+width])) + ",\n")
    out.write("};\n")


def main():
    rows, codes, clips = {}, bytearray(), {}
    for name, ending in (("opening", False), ("finale", True)):
        clip = []
        for t in range(FRAMES):
            pixels = render(ending, t).tobytes()
            for y in range(HEIGHT):
                row = pixels[y*WIDTH:(y+1)*WIDTH]
                if row not in rows:
                    rows[row] = len(codes)
                    codes.extend(encode_row(row))
                clip.append(rows[row])
        clips[name] = clip
    if len(codes) > 65536:
        raise ValueError("Scanline offsets exceed 16 bits")

    target = Path(__file__).resolve().parents[1] / "source" / "story_fmv_data.h"
    with target.open("w", encoding="ascii", newline="\n") as out:
        out.write("// Generated by tools/make_story_fmv.py.\n")
        out.write(f"enum {{ FMV_W = {WIDTH}, FMV_H = {HEIGHT}, FMV_TICKS = {FRAME_TICKS}, FMV_FRAMES = {FRAMES} }};\n")
        c_array(out, "fmvPalette", "u16",
                [(r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10) for r, g, b in PALETTE], 16)
        c_array(out, "fmvRows", "u8", codes, 24)
        for name, clip in clips.items():
            c_array(out, f"fmv_{name}", "u16", clip, 16)
    print(f"Two {FRAMES * FRAME_TICKS / 60:g}s clips, {len(codes) + 4 * FRAMES * HEIGHT + 32:,} bytes")


if __name__ == "__main__":
    main()
