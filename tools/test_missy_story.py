#!/usr/bin/env python3
"""Static regression: Missy's film may not have dangling camera or memory indices."""
from pathlib import Path
import re

cut = Path("source/cutscene.h").read_text(encoding="utf-8")
shots = Path("source/csshot.h").read_text(encoding="utf-8")
story = Path("source/story.h").read_text(encoding="utf-8")
house = Path("source/house.h").read_text(encoding="utf-8")
save = Path("source/savegame.h").read_text(encoding="utf-8")
scenes = {}
for match in re.finditer(r"static const CsBeat csS(\d+)\[\]=\{([\s\S]*?)^\};", cut, re.M):
    n = int(match.group(1))
    beats = [line for line in match.group(2).splitlines() if re.match(r"\s*\{CB_", line)]
    scenes[n] = len(beats)
assert len(scenes) == 13 and set(scenes) == set(range(13)), scenes
assert all(0 < n < 255 for n in scenes.values()), scenes
assert scenes[3] == 35 and scenes[4] == 32 and scenes[5] == 52
assert scenes[10] == 11 and scenes[11] == 22 and scenes[12] == 10

seen = set()
count = 0
for m in re.finditer(r"^\s*\{(\d+),(\d+),([0-4]),\s*(-?\d+),\s*(-?\d+),", shots, re.M):
    sc, beat, anchor, frame, duration = map(int, m.groups())
    assert sc in scenes and 0 <= beat < scenes[sc], (sc, beat, scenes.get(sc))
    assert frame >= 0 and duration > 0, (sc, beat, frame, duration)
    key = (sc, beat, frame)
    assert key not in seen, ("two shots at identical start obscure the first", key)
    seen.add(key)
    count += 1
assert count >= 120, count

# The canonical film and five-story sequence must remain playable.
for needle in [
    "static const CsScene csScenes[13]",
    "static const s8 csTune[13]",
    "{12,0,8,255}", "{1,9,255}", "{2,255}", "{3,10,4,255}", "{11,5,255}",
    "csStoryMemory=(u8)stmGet()",
    "stPending=(u8)(stId==STY_TVSHOW",
    "stModal=2;", "if(stPending){ stPending=0; stSave(); }",
    "plugNight=(u8)msNightUsed", "plugRemember(v)",
    "if(stCh==3&&(stPart==255||hhPUid!=(int)stPart))",
    "static int stTvStoryAbsentUid(int uid)",
    "if(stTvStoryAbsentUid(s->uid))",
    "stTvStoryAbsentLabel(hhM[m].uid)",
    "IN HOSPITAL",
    "REMEMBERED",
    "static void stTvPatientRest(int m)",
    "if(stCh>=4&&stPart!=255&&hhPUid!=(int)stPart",
    "if(stCh==4&&stPart!=255&&stMember(stPart)>=0) n--;",
    "stShown=0; stModal=0; stTvWant=stTvWarn=0;",
    "if(s==STY_TVSHOW&&custom)",
    "if(stId==STY_TVSHOW&&stCh==4&&stKid!=255",
    "if((cl!=8&&cl!=12)", "slwPut(w,(u8)(stId==STY_TVSHOW?stmGet():0))",
    "SRAM_BASE[STB_OFF+i]=c.p[9+i]",
    "if(csMemOn){ lastSc=-1; return 0; }",
]:
    assert needle in cut + story + house + save + shots, needle
assert cut.count("CF_AUTO,250") == 1, "the original tracker song must start once"
assert cut.count(",251,0,") == 1, "the original tracker song must fade once"
assert cut.index("static const CsBeat csS3") < cut.index("static const CsBeat csS5")
assert "csShotFind(int t)" in shots and "lastT=-1,nextT=-1" in shots
assert "while(keyNow()&(K_A|K_B|K_START)) vsync()" in cut
print(f"BORE Missy cutscene timing/shot integrity: PASS ({len(scenes)} scenes, {count} camera shots)")
