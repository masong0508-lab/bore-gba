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

# Parse every entry instead of silently ignoring invalid anchors or fields.
shot_table = re.search(r"static const CsShot csShots\[\]\s*=\s*\{(.*?)^\};", shots, re.M | re.S)
assert shot_table, "missing camera shot array"
entries = re.findall(r"\{([^{}]*)\}", shot_table.group(1))
assert len(entries) >= 120, ("missing camera shots", len(entries))
seen = set()
count = 0
for entry in entries:
    fields = [field.strip() for field in entry.split(",")]
    assert len(fields) == 11 and all(re.fullmatch(r"-?\d+", x) for x in fields), ("malformed shot entry", entry)
    sc, beat, anchor, frame, duration, z0, x0, y0, z1, x1, y1 = map(int, fields)
    assert 0 <= anchor <= 4, (sc, beat, anchor)
    # Guard camera inputs before they reach signed fixed-point arithmetic on GBA.
    assert 0 <= frame <= 4096 and 0 < duration <= 4096, (sc, beat, frame, duration)
    assert 128 <= z0 <= 1536 and 128 <= z1 <= 1536, (sc, beat, z0, z1)
    assert all(-256 <= x <= 512 for x in (x0, x1)), (sc, beat, x0, x1)
    assert all(-128 <= y <= 256 for y in (y0, y1)), (sc, beat, y0, y1)
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
    "if(stId==STY_TVSHOW&&stCh==3) stmPut(msPressurePack((unsigned)stmGet(),PLUG_START))",
    "static void plugRestore(void)",
    "plugRestore(); int d=ev==SE_BFF?15:10;",
    "plugV=(u8)msPressureEase(plugV,(unsigned)d)",
    "plugRestore(); // restore before any hospital pressure changes",
    "case TQ_PLUG: if(stId==STY_TVSHOW&&stCh==3&&stKidDay!=255) plugRestore(); return plugV<=PLUG_OK;",
    "if(stId==STY_TVSHOW&&stCh==3&&stKidDay!=255){ plugDay=0xFFFF; plugSec=plugTen=0; plugRestore(); }",
    "case STY_TVSHOW: stKid=(u8)hhPUid; break;",
    "int patientUid=(int)msPatientUid((unsigned)stKid,(unsigned)hhPUid);",
    "static int stTvMamesyMember(int patientUid)",
    "kin[patientUid][hhM[k].uid]==KN_SISTER",
    "m=stTvMamesyMember(patientUid);",
    "if(s==STY_TVSHOW&&stTvMamesyMember(hhPUid)>=0) need=0;",
    "stRel(patientUid,hhM[m].uid,70,70,RF_FRIEND|RF_BFF)",
    "int patientMember=patientUid==hhPUid?-1:stMember(patientUid);",
    "if(patientUid!=hhPUid&&patientMember<0)",
    "m=stAddSim(lk,AG_ADULT,patientUid==hhPUid?hhPLast:hhM[patientMember].last)",
    "stKid=(u8)msPatientUid((unsigned)stKid,(unsigned)me)",
    "s==STY_ICE||s==STY_SPARKS||s==STY_TVSHOW)?1:s==STY_FRIEND?2:0;",
    "MAMESY NEEDS A FREE HOUSEHOLD SLOT",
    "AS MAMESY  KEEP PRESSURE 60 OR LESS",
    "msPressureOutcome((unsigned)v,(unsigned)plugNight)==2u",
    "msPressureOutcome((unsigned)v,(unsigned)plugNight)==1u",
    "case TQ_DAYS: return (int)msDaysProgress",
    "toast(\"One more night. One extra day.\")",
    "id==3&&bi==27&&(csStoryMemory&16)",
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
    "m=stTvMamesyMember(patientUid);",
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
assert "static void csWardPatient(" in art
assert "csWardPatient(x,y,who,pose,t,sk,hr)" in art
assert "csCurSc==3&&csCurBi>=16&&csCurBi<34" in art
assert "int talking=awake&&csTalking&&csSpk==who" in art
assert "int blink=((t+37)%180)<5" in face
# Ward anatomy contract: the whole silhouette shares ONE horizontal bed
# plane, hair rests on a pillow, and the face aims UP rather than left.
assert "SUPINE BED POSE:" in art
assert "SUPINE close-up:" in face
assert art.count("csFaceBed(")==1, "patient portrait must render exactly once"
assert "csR(x-53,y-19,40,16" in art, "pillow must support skull"
assert "csR(x-37,y-4,86,3" in art, "spine/blanket rests on mattress"
assert "csR(x-19,y-13-breath,39,13,edge)" in art, "horizontal torso on mattress"
assert "csR(x+8,y-10,41,11,edge)" in art, "blanket starts at the hips"
assert "csR(x-1,y-8-breath-lift,12,3,sh)" in art, "forearm rests on torso"
assert "csD(x-31,y-12-breath,9,edge)" in art, "wide head should not float upright"
assert "csR(x-37,y-22-breath,3,4,sk)" in art, "wide nose faces ceiling"
assert "csFaceBed(x-32,y-12-breath,t,sk,hr,awake)" in art, "closeup shares pose anchor"
assert "csFR(19,6,30,12,shadow)" in face, "reclining neck meets gown"
assert "csFR(-15,-23,6,8,skin)" in face, "closeup nose faces upward"
assert "csFR(-27,3,5,2,shadow)" not in face, "old horizontal-beak profile must not return"
# Geometry test independent of scaling: skull is above the mattress but its
# lower edge rests on the pillow, while the neck/gown reaches the bed.
head_y, head_radius, pillow_y, pillow_height = -12, 9, -19, 16
torso_y, torso_height, blanket_y, blanket_height = -13, 13, -10, 11
assert head_y+head_radius >= pillow_y+7, "head needs actual pillow contact"
assert torso_y+torso_height >= -1, "torso must lie on mattress instead of floating"
assert blanket_y+blanket_height >= 0, "feet must be supported by the bed"
assert "csR(115,90,96,15" in cut, "mattress remains beneath patient/pillow"
assert "csR(118,107,96,2" in cut, "near rail must stay below body"
assert "{3,16,1,  0,100, 560,-12,90, 700,-12,89}" in shots
assert "csFaceBed(x-30,y-15,t,sk,hr,csWardAwake())" not in art
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
qa = Path("tools/qa_missy_visual.sh").read_text(encoding="utf-8")
assert 'game_h=$((win_w*2/3))' in qa, "emulator screenshots must crop to the actual 3:2 game viewport"
assert '-crop "${win_w}x${game_h}+0+${top}"' in qa
assert 'local name="$1"\n  local src="screenshots/$name.xwd"' in qa, "capture path must be assigned after its local label"
assert 'test $((width*2)) -eq $((height*3))' in qa, "visual smoke must reject emulator chrome outside the game frame"
makefile = Path("Makefile").read_text(encoding="utf-8")
assert "preview-missy: bore-missy-preview.gba" in makefile
assert "preview-missy-awake: bore-missy-awake.gba" in makefile
assert "-DCS_PREVIEW=4 -DCS_PREVIEW_BEAT=16" in makefile
assert "if(id==CS_PREVIEW-1&&CS_PREVIEW_BEAT<sc->n)" in cut
assert "-DCS_PREVIEW=$(CS_PREVIEW_SCENE)" in makefile
assert "csShotFind(int t)" in shots and "lastT=-1,nextT=-1" in shots
assert "while(keyNow()&(K_A|K_B|K_START)) vsync()" in cut
print(f"BORE Missy cutscene timing/shot integrity: PASS ({len(scenes)} scenes, {count} camera shots)")
