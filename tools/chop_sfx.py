#!/usr/bin/env python3
"""Chop the long voice recordings into one clip per take, and give every kind of hit its own take.

The five recordings in tools/sfx_src (hit, scream, instant, cry, groan; 3 to 16 s each) are recording sessions: several
separate takes with silence between them. The game used to play a whole session for one effect: a punch played all five
hit grunts in a row, a scream, a close call and a death all played the same 16 s file (which, like cry and instant,
opens with the same 3 s take). Here each take becomes its own clip (source/sfx/x_<take>.adp, 4-bit IMA-ADPCM, 6554 Hz,
the same format as every effect) and ROLES says which take(s) each effect plays. An effect with several takes cycles
through them (main.c sfxPlay), so a fight does not repeat the same grunt.

usage (from the project root):
  python3 tools/chop_sfx.py                 -> source/sfx/x_*.adp for the takes ROLES uses + source/sfxtakes.h
  python3 tools/chop_sfx.py --preview DIR   -> also every take (used or not) as DIR/<nn>_<take>_<role>.wav, to listen to
To give an effect another take: change its line in ROLES and run this again (then make)."""
import sys, os, wave
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from encode_sfx import encode, decode, RATE

SRC = "tools/sfx_src"
# Every take: name -> (session, start s, end s). The times are where the voice is (found by a 20 ms loudness scan of each
# session: a take is a loud run, gaps under 160 ms belong to it). cry and instant share takes 1-3 (and instant 4-7) with
# scream, so only their own takes are listed.
TAKES = {
    "hit1": ("hit", 0.00, 0.22), "hit2": ("hit", 1.28, 1.62), "hit3": ("hit", 1.84, 2.04),   # hit3: a quiet low thump
    "hit4": ("hit", 2.22, 2.56), "hit5": ("hit", 3.04, 3.32),                                 # hit1, 2, 4, 5: short voiced grunts
    "scr1": ("scream", 0.18, 3.16),   # a wail that builds (every old scream / cry / death opened with it)
    "scr2": ("scream", 4.14, 5.60),   # breathy, mostly unvoiced
    "scr3": ("scream", 5.80, 7.04),   # a sharp high scream
    "scr4": ("scream", 7.34, 10.64),  # a long moan that swells at the end
    "scr5": ("scream", 11.52, 13.30), # dying groans
    "scr6": ("scream", 13.50, 14.96), # a yell, straight in
    "scr7": ("scream", 15.20, 15.52), # a short yelp
    "ins3": ("instant", 5.80, 6.42),  # a short shriek (instant's own take)
    "cry4": ("cry", 7.34, 9.94),      # sobbing that builds
    "cry5": ("cry", 10.78, 14.22),    # sobbing
    "cry6": ("cry", 14.46, 14.84),    # a short sob / yelp
    "grn1": ("groan", 0.08, 1.34), "grn2": ("groan", 1.58, 3.72), "grn3": ("groan", 4.40, 5.44),   # grn3: noisy, like retching
    "grn4": ("groan", 5.60, 6.48), "grn5": ("groan", 6.68, 9.04), "grn6": ("groan", 9.68, 12.14),
    "grn7": ("groan", 12.32, 12.54),
}
# Which take(s) each effect plays (main.c SFX_*). The first take of a list is also what a cutscene beat plays first.
ROLES = {
    "BONK":    ["hit1"],                          # running into a wall on foot
    "HIT":     ["hit1", "hit2", "hit4", "hit5"],  # a punch or kick lands (on you or on a Sim), a parry, a tackle, a beating
    "SCREAM":  ["scr3"],                          # a fall that kills you (GRAVITY WON)
    "NEARLY":  ["scr6"],                          # CLOSE CALL: a life or death hurt you survive
    "DEATH":   ["scr1"],                          # killed by a wall, or worn out by hits
    "INSTANT": ["ins3"],
    "CRY":     ["cry5"],                          # an accident, the cutscenes' crying
    "GROAN":   ["grn1"],                          # the cutscenes' sick groan
    "YELP":    ["scr7", "cry6", "grn7"],          # a Sim hit by a weapon
    "KO":      ["scr5"],                          # a Sim knocked out
}
PRE, POST = 0.04, 0.12   # seconds kept before and after a take (never into the next one)

def session(name):
    return decode(open(f"{SRC}/{name}_session.adp", "rb").read()).astype(np.float64)

def cut(name):
    s, a, b = TAKES[name]; x = session(s)
    # the room around the take: up to PRE / POST, but not past the middle of the gap to a neighbouring take
    nb = sorted((t[1], t[2]) for t in TAKES.values() if t[0] == s)
    i = nb.index((a, b)); lo = max(0.0, a - PRE); hi = b + POST
    if i > 0: lo = max(lo, (nb[i-1][1] + a) / 2)
    if i + 1 < len(nb): hi = min(hi, (b + nb[i+1][0]) / 2)
    y = x[int(lo * RATE):min(len(x), int(hi * RATE))].copy()
    y -= np.mean(y)
    fi, fo = int(0.006 * RATE), int(0.04 * RATE)
    y[:fi] *= np.linspace(0, 1, fi); y[-fo:] *= np.linspace(1, 0, fo)
    pk = np.max(np.abs(y)) or 1.0
    y *= 29000.0 / pk   # every take at the same peak (-1 dB): a quiet take is as loud as the rest
    return np.round(np.clip(y, -32768, 32767)).astype(np.int32)

def main():
    prev = sys.argv[sys.argv.index("--preview") + 1] if "--preview" in sys.argv else None
    used = []
    for r in ROLES.values():
        for t in r:
            if t not in used: used.append(t)
    os.makedirs("source/sfx", exist_ok=True); total = 0
    for t in used:
        b = encode(cut(t)); open(f"source/sfx/x_{t}.adp", "wb").write(b); total += len(b)
        print(f"x_{t}: {len(cut(t))/RATE:.2f}s {len(b)} bytes  ->", ", ".join(k for k, v in ROLES.items() if t in v))
    if prev:
        os.makedirs(prev, exist_ok=True)
        for n, t in enumerate(TAKES):
            y = decode(encode(cut(t))).astype(np.int16)   # (what the game plays: through the ADPCM)
            role = "+".join(k for k, v in ROLES.items() if t in v) or "spare"
            with wave.open(f"{prev}/{n+1:02d}_{t}_{role}.wav", "wb") as w:
                w.setnchannels(1); w.setsampwidth(2); w.setframerate(RATE); w.writeframes(y.tobytes())
    L = " \\\n".join(f"    X({t})" for t in used)
    roles = "\n".join(f"#define TK_{k} " + ",".join(f"x_{t}" for t in v) + f"\n#define TK_{k}_N {len(v)}\n#define TK_{k}_FIRST x_{v[0]}" for k, v in ROLES.items())
    with open("source/sfxtakes.h", "w") as h:
        h.write(f"""// sfxtakes.h - GENERATED by tools/chop_sfx.py (do not edit by hand): the takes cut out of the long recordings in tools/sfx_src.
// Each is source/sfx/x_<take>.adp (4-bit IMA-ADPCM, 6554 Hz). TK_<EFFECT> lists the take(s) an effect plays (main.c sfxTab / sfxPlay);
// an effect with several cycles through them. To change which take an effect plays, edit ROLES in tools/chop_sfx.py and run it again.
#ifndef SFXTAKES_H
#define SFXTAKES_H
#define TAKE_LIST(X) \\
{L}
#define TK_ASM(n) ".global x_" #n "\\nx_" #n ":\\n.incbin \\"source/sfx/x_" #n ".adp\\"\\n.balign 4\\n"
__asm__(".pushsection .rodata\\n.balign 4\\n" TAKE_LIST(TK_ASM) ".popsection\\n");
#define TK_EXT(n) extern const u8 x_##n[];
TAKE_LIST(TK_EXT)
{roles}
#endif
""")
    print("total", total, "bytes,", len(used), "takes used of", len(TAKES))

if __name__ == "__main__":
    main()
