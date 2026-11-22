#!/usr/bin/env python3
"""Generate song-synchronous original singing over the Koala shuffle.

Use the 56 existing *isolated original vocal* ADPCM clips without resampling
or repitching. Reposition only their onsets onto the source 62% swing grid.
The singer's natural internal timing stays intact (no drunk time stretching).
"""
import json
import math
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
CLIPS=ROOT/"tools/corporations/digital.json"
SOURCE=ROOT/"tools/corporations/digital/vocals.adp"
DEST=ROOT/"source/winner_vocals.h"
KOALA_BPM=127.56512451171875
KOALA_SWING=0.6202292442321777
REFERENCE_BPM=131.005
REFERENCE_DOWNBEAT_S=0.152
ROW_S=5.0/170.0
# The supplied 4.18s normal and 4.22s shuffle vocal takes preserve the same
# overall phrase duration; the shuffle moves attacks later. Retain the real
# recording's pitch, add a modest delay, and apply 62% offbeat swing.
VOCAL_LAG_S=0.045
# STEM-VERIFIED ENTRANCES (2026-11-22): original isolated reference has
# intro singer at 1.49/7.68 s, a short ad-lib at 31.94 s, and its first
# sustained verse at 46.44 s. Drums begin around 18 s, bass around 32.6 s.
# An old +18-bar shift placed the intro clips at the bass entrance (~35 s),
# making vocals appear far too early in the groove. Do NOT shift every line.
# Beat/time remapping from 131.005 to 127.565 BPM preserves relative song form.

def target_seconds(source_seconds):
    beat=(source_seconds-REFERENCE_DOWNBEAT_S)*REFERENCE_BPM/60.0
    whole=math.floor(beat); frac=beat-whole
    phase=frac*KOALA_SWING/.5 if frac<.5 else KOALA_SWING+(frac-.5)*(1-KOALA_SWING)/.5
    return REFERENCE_DOWNBEAT_S+(whole+phase)*60.0/KOALA_BPM+VOCAL_LAG_S

def make():
    clips=json.loads(CLIPS.read_text())["stems"]["vocals"]["clips"]
    assert len(clips)==56
    packed=SOURCE.read_bytes()
    cues=[]
    for i,clip in enumerate(clips):
        assert clip["i"]==i
        off=int(clip["offset"]); size=int(clip["bytes"])
        assert off%4==0 and size>=8 and off+size<=len(packed)
        count=int.from_bytes(packed[off:off+4],"little")
        assert count==int(clip["samples"]), (i,count,clip["samples"])
        assert size>=4+(count+1)//2
        row=round(target_seconds(float(clip["start_s"]))/ROW_S)
        assert 0<=row<206*64 and (not cues or row>cues[-1][0])
        # Keep the live Koala shuffle audible under the singing. The isolated
        # ADPCM vocals are individually normalized, so a full 112/256 mix
        # sounds unnaturally louder than the author's original master.
        # Use a moderate 48/256 center overlay with source dynamics intact.
        gain=max(12,min(48,round(48*min(1.,float(clip["vol_peak"])/.80))))
        cues.append((row,gain,off))
    text=[
        "// GENERATED from tools/corporations/digital.json by tools/winner_voice.py\n",
        "// Original vocal ADPCM is pitch-untouched; starts are shuffled to tracker rows.\n",
        "typedef struct { u16 row; u16 gain; u32 offset; } WinnerVocalCue;\n",
        "static const WinnerVocalCue winnerVocalCues[56]={\n"
    ]
    for row,gain,off in cues:text.append(f"  {{{row},{gain},{off}}},\n")
    text.extend([
        "};\n#define WINNER_VOCAL_CUES 56\n",
        '__asm__(".pushsection .rodata\\n.balign 4\\n.global winnerVocalBlob\\n"\n',
        '        "winnerVocalBlob:\\n.incbin \\"tools/corporations/digital/vocals.adp\\"\\n.balign 4\\n.popsection\\n");\n',
        "extern const u8 winnerVocalBlob[];\n"
    ])
    DEST.write_text("".join(text))
    print("Winner vocals: 56 source-timed phrases (original intro / main verse), rows %d..%d"%(cues[0][0],cues[-1][0]))
    return cues

if __name__=="__main__":make()
