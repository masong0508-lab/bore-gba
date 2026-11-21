#!/usr/bin/env python3
"""Static regression: Missy's film may not have dangling camera or memory indices."""
from pathlib import Path
import re

cut = Path("source/cutscene.h").read_text(encoding="utf-8")
shots = Path("source/csshot.h").read_text(encoding="utf-8")
story = Path("source/story.h").read_text(encoding="utf-8")
house = Path("source/house.h").read_text(encoding="utf-8")
art = Path("source/csart.h").read_text(encoding="utf-8")
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
    "plugV=PLUG_START; plugDay=0xFFFF; plugSec=plugTen=plugNight=0;",
    "if(s==STY_TVSHOW) stmPut(0);",
    "replaying this story in the same household",
    "if(stId==STY_TVSHOW&&stCh==4&&stKid!=255",
    "if((cl!=8&&cl!=12)", "slwPut(w,(u8)(stId==STY_TVSHOW?stmGet():0))",
    "SRAM_BASE[STB_OFF+i]=c.p[9+i]",
    "if(csMemOn){ lastSc=-1; return 0; }",
]:
    assert needle in cut + story + house + save + shots, needle
assert cut.count("CF_AUTO,250") == 1, "the original tracker song must start once"
assert cut.count(",251,0,") == 1, "the original tracker song must fade once"
assert cut.index("static const CsBeat csS3") < cut.index("static const CsBeat csS5")
face = Path("source/csface.h").read_text(encoding="utf-8")
assert "static void csFaceBed(" in face
assert "csFaceBed(x-30,y-15,t,sk,hr,csWardAwake())" in art
assert "csCurSc==3&&csCurBi>=16&&csCurBi<34" in art
assert "int talking=awake&&csTalking&&csSpk==who" in art
assert "static void csWardPatient(" in art
assert "csWardPatient(x,y,who,pose,t,sk,hr)" in art
assert "Anatomical neck and shoulder connect the head" in art
assert "Neck and collar FIRST:" in face
assert "{3,3,0," in shots and "{3,20,0," in shots
assert "static void csWardFront(void)" in cut
assert "Mamesy's chair moves toward the bed in the vigil beat" in cut
assert "PERSPECTIVE SITE:" in cut
assert "INSIDE THE CAR:" in cut
assert "qM={TQ_MONEY,8000}" in story, "hospital relief must agree with the actual 8k goal"
assert "csR(123,61,60,4" in cut, "scaffold platform must be at Missy's final foot height"
assert "if(csCurSc==10&&csCurBi>=8)" in cut
assert "backstage doorway" in cut
assert "warm twin footlights" in cut
assert "if((b->bg==CB_HOSP||b->bg==CB_FLAT)&&(b->pb==CP_LIE||b->pb==CP_STIR)) csWardFront()" in cut
assert cut.index("csFtrack=0;") < cut.index("csWardFront();")  # bed rail is an occluder, after patient
makefile = Path("Makefile").read_text(encoding="utf-8")
assert "preview-missy: bore-missy-preview.gba" in makefile
assert "preview-missy-awake: bore-missy-awake.gba" in makefile
assert "-DCS_PREVIEW=4 -DCS_PREVIEW_BEAT=16" in makefile
assert "if(id==CS_PREVIEW-1&&CS_PREVIEW_BEAT<sc->n)" in cut
assert "-DCS_PREVIEW=$(CS_PREVIEW_SCENE)" in makefile
assert "csShotFind(int t)" in shots and "lastT=-1,nextT=-1" in shots
assert "while(keyNow()&(K_A|K_B|K_START)) vsync()" in cut
print(f"BORE Missy cutscene timing/shot integrity: PASS ({len(scenes)} scenes, {count} camera shots)")
