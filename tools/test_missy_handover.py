#!/usr/bin/env python3
"""Execute the game's handover and story save functions with host-side fixtures.

The functions are extracted from story.h at test time, not copied into a model.
Household switching, sprite work and UI are test boundaries. This exercises
identity, cast reuse, capacity, retries and UID remapping; it is not a full
interactive campaign playthrough.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "source/story.h").read_text(encoding="utf-8")
house_source = (ROOT / "source/house.h").read_text(encoding="utf-8")


def function(name, source=source):
    match = re.search(r"^static [^;\n]+\b" + re.escape(name) + r"\([^;\n]*\)\s*\{", source, re.M)
    if not match:
        raise AssertionError(f"Missing actual story function: {name}")
    # Ignore braces inside strings/comments while finding the function's end.
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')
    depth = 0
    for token in tokens.finditer(source, match.end() - 1):
        if token[0] == "{":
            depth += 1
        elif token[0] == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():token.end()]
    raise AssertionError(f"Unclosed story function: {name}")


enum = re.search(r"^enum \{ STY_NONE,.*?\};", source, re.M)[0]
functions = "\n\n".join(function(name) for name in (
    "stSum", "stSave", "stLoad", "stUidMap", "stOff", "stMember", "stAddSim",
    "stRel", "stTvUidRemoved", "stTvStoryAbsentUid", "stTvStoryAbsentLabel", "stTvPatientRest",
    "stTvCan", "stTvMamesyMember", "stTvMamesyUid", "stTvControl", "storyJoin",
)) + "\n" + function("hhRemove", house_source)

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "source/missyrules.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t s32;
typedef int money_t;
#define HH_MAX 7
#define HU_N 8
#define RU_N HU_N
#define HH_NM 12
#define LK_N 1
#define TR_N 1
#define AG_ADULT 3
#define AS_GROW 1
#define AS_PICK 2
#define HA_IDLE 0
#define HA_AWAY 7
#define HN_REST 2
#define HN_FUN 5
#define KN_SISTER 1
#define RF_FRIEND 1
#define RF_BFF 2
#define STORY_OFF 0
typedef struct {
    u8 uid, act, use, item, pn, pi, gok;
    int think;
    s32 fx, fy;
    char name[HH_NM], last[HH_NM];
} HhSim;
static HhSim hhM[HH_MAX];
static int hhN, hhPUid, hhExX, hhExY, custom, lnoteT, camSnap, stModal;
static u16 stShown;
static u8 hhUp[HH_MAX], hhFl[HH_MAX], kin[HU_N][HU_N], relF[HU_N][HU_N];
static signed char relD[HU_N][HU_N], relL[HU_N][HU_N];
static struct { int x0, x1; } hhOld[HH_MAX];
static unsigned hhOldSig[HH_MAX];
typedef struct { uintptr_t sm[4], dm[4]; int off, len; } HhSpr;
static HhSpr hhSp[HH_MAX];
static u16 hhPal[HH_MAX][16];
static unsigned hhKey[HH_MAX];
static char hhPName[HH_NM], hhPLast[HH_NM];
static const char *lnote;
static const u8 stMamesyLook[LK_N] = {0};
static u8 stId, stCh, stPart, stKid, stKidDay, stGuest, stTkL, stPending;
static u8 stTvWant, stTvWarn, SRAM_BASE[8];
static int add_calls, switch_calls, switch_blocked, setup_calls, enter_calls;
static const char *last_toast;
static money_t simMoney;
static void toast(const char *s){ last_toast=s; }
static int rnd8(void){ return 1; }
static void hhBakeAll(void){}
static void hhSave(void){}
static void liveInvalidate(void){}
static void famForget(int uid){ (void)uid; }
static void nrForget(int uid){ (void)uid; }
static void hhSprFree(int member){ (void)member; }
static void hhSlotsFree(void){}
static int hhOnOtherFloor(int m){ return hhUp[m]!=0; }
static int hhAdd(const u8*, int, int, int, const u8*);
static void hhSwitchTo(int);
static int storyPick(void);
static void storySetup(int s){ stId=(u8)s; setup_calls++; }
static void stEnter(void){ enter_calls++; }
static int menu(const char *title,const char *const *items,int n){
    (void)title; (void)items; (void)n; return 0;
}
'''

support = r'''
static int storyPick(void){ return STY_TVSHOW; }
static int hhAdd(const u8 *look,int stage,int asp,int ltw,const u8 *traits){
    (void)look; (void)stage; (void)asp; (void)ltw; (void)traits;
    add_calls++;
    if(hhN>=HH_MAX) return -1;
    int uid=0;
    while(uid<HU_N&&(uid==hhPUid||stMember(uid)>=0)) uid++;
    assert(uid<HU_N);
    HhSim *s=&hhM[hhN]; memset(s,0,sizeof(*s)); s->uid=(u8)uid;
    return hhN++;
}
static void hhSwitchTo(int m){
    switch_calls++;
    if(switch_blocked||m<0||m>=hhN||custom) return;
    if(stTvStoryAbsentUid(hhM[m].uid)){
        toast(stTvStoryAbsentLabel(hhM[m].uid)); return;
    }
    if(hhM[m].act==HA_AWAY&&!hhOnOtherFloor(m)) return;
    HhSim selected=hhM[m], old={0}; old.uid=(u8)hhPUid;
    strcpy(old.name,hhPName); strcpy(old.last,hhPLast);
    for(int i=m;i<hhN-1;i++){
        hhM[i]=hhM[i+1]; hhUp[i]=hhUp[i+1]; hhFl[i]=hhFl[i+1];
    }
    hhM[hhN-1]=old; hhUp[hhN-1]=hhFl[hhN-1]=0;
    hhPUid=selected.uid; strcpy(hhPName,selected.name); strcpy(hhPLast,selected.last);
}
static void fixture(int active,int sister,int full){
    memset(hhM,0,sizeof(hhM)); memset(kin,0,sizeof(kin));
    memset(relF,0,sizeof(relF)); memset(relD,0,sizeof(relD)); memset(relL,0,sizeof(relL));
    memset(hhUp,0,sizeof(hhUp)); memset(hhFl,0,sizeof(hhFl)); memset(SRAM_BASE,0,sizeof(SRAM_BASE));
    hhN=0; hhPUid=active; custom=0;
    add_calls=switch_calls=switch_blocked=setup_calls=enter_calls=0;
    last_toast=0; stId=STY_TVSHOW; stCh=3; stKid=0; stPart=stKidDay=255;
    stTvWant=1; stTvWarn=stGuest=stTkL=stPending=0;
    stModal=0; stShown=0; lnote=0; lnoteT=0;
    for(int uid=0;uid<(full?HU_N:3);uid++){
        const char *name=uid==0?"MISSY":uid==1&&sister?"MAMESY":"ROOMMATE";
        const char *last=uid==0?"JEANNE":"OTHER";
        if(uid==active){ strcpy(hhPName,name); strcpy(hhPLast,last); }
        else { HhSim *s=&hhM[hhN++]; s->uid=(u8)uid; strcpy(s->name,name); strcpy(s->last,last); }
    }
    if(sister) kin[0][1]=kin[1][0]=KN_SISTER;
}
static void patient_absent(int patient){
    int m=stMember(patient);
    assert(m>=0&&hhM[m].act==HA_AWAY&&hhUp[m]==0);
    assert(stTvStoryAbsentUid(patient));
    assert(strcmp(stTvStoryAbsentLabel(patient),"IN HOSPITAL")==0);
}
static void remap_household(const u8 *nu){
    u8 old[HU_N][HU_N]; memcpy(old,kin,sizeof(old)); memset(kin,0,sizeof(kin));
    for(int a=0;a<HU_N;a++) for(int b=0;b<HU_N;b++) kin[nu[a]][nu[b]]=old[a][b];
    hhPUid=nu[hhPUid]; for(int m=0;m<hhN;m++) hhM[m].uid=nu[hhM[m].uid];
    stUidMap(nu,HU_N);
}
int main(void){
    /* Start from UID zero; the newly created sister inherits Missy's name. */
    fixture(0,0,0); stTvControl();
    assert(stKid==0&&stPart==hhPUid&&add_calls==1&&stTvWant==0);
    assert(strcmp(hhPLast,"JEANNE")==0); patient_absent(0);
    assert(relF[0][stPart]==(RF_FRIEND|RF_BFF)&&kin[0][stPart]==KN_SISTER);

    /* Another selected housemate must not become the patient or her family. */
    fixture(2,0,0); stTvControl();
    assert(stKid==0&&stPart==hhPUid&&add_calls==1&&stTvWant==0);
    assert(strcmp(hhPLast,"JEANNE")==0&&kin[2][stPart]!=KN_SISTER);
    patient_absent(0); assert(hhM[stMember(2)].act==HA_IDLE);

    /* Reuse an inactive sister, even one parked by a previous story run. */
    fixture(2,1,0); hhM[stMember(1)].act=HA_AWAY; stTvControl();
    assert(stKid==0&&stPart==1&&hhPUid==1&&add_calls==0&&stTvWant==0); patient_absent(0);

    /* Regression: the active Mamesy is outside hhM and must not be duplicated. */
    fixture(1,1,0); int before=hhN; assert(stTvMamesyUid(0)==1); stTvControl();
    assert(stKid==0&&stPart==1&&hhPUid==1&&hhN==before&&add_calls==0&&switch_calls==0);
    assert(stTvWant==0); patient_absent(0);

    /* The same active-sister case is valid with all eight household slots full. */
    fixture(1,1,1); stTvControl();
    assert(stPart==1&&hhPUid==1&&hhN==HH_MAX&&add_calls==0&&stTvWant==0); patient_absent(0);
    fixture(7,1,1); stTvControl();
    assert(stPart==1&&hhPUid==1&&hhN==HH_MAX&&add_calls==0&&stTvWant==0); patient_absent(0);

    /* The same name alone is not proof that a housemate is the story's sister. */
    fixture(2,1,0); kin[0][1]=kin[1][0]=0; stTvControl();
    assert(stPart!=1&&add_calls==1&&kin[0][stPart]==KN_SISTER); patient_absent(0);

    /* A full house without Mamesy remains pending and can retry after room opens. */
    fixture(2,0,1); stTvControl();
    assert(hhPUid==2&&stKid==0&&stPart==255&&stTvWant==1&&last_toast);
    hhN--; stTvControl();
    assert(stPart==hhPUid&&stKid==0&&stTvWant==0); patient_absent(0);

    /* A failed switch must reuse the already-created cast on the next attempt. */
    fixture(2,0,0); switch_blocked=1; stTvControl();
    int cast=stPart; assert(cast!=255&&stTvWant==1&&hhPUid==2&&add_calls==1);
    switch_blocked=0; stTvControl();
    assert(stPart==cast&&hhPUid==cast&&add_calls==1&&stTvWant==0); patient_absent(0);

    /* Reload before handover: household renumbering changes both saved story UIDs. */
    fixture(1,1,1); stPending=1; stSave();
    const u8 selected_map[HU_N]={1,0,2,3,4,5,6,7}; remap_household(selected_map);
    stKid=stPart=stCh=0; stLoad();
    assert(stKid==1&&stPart==255&&stCh==3&&stPending==1&&hhPUid==0);
    stTvControl(); assert(stPart==0&&hhPUid==0&&stKid==1&&add_calls==0); patient_absent(1);
    stPart=stKid=255; stLoad(); assert(stPart==0&&stKid==1);
    stTvWant=1; stTvControl(); assert(add_calls==0&&stTvWant==0); patient_absent(1);
    stCh=4; stTvWant=2; stTvControl();
    assert(hhPUid==1&&stKid==1&&stPart==0&&stTvWant==0);
    assert(stTvStoryAbsentUid(0)&&!stTvStoryAbsentUid(1));

    /* Reload with a different housemate selected still finds the original pair. */
    fixture(2,1,0); stSave();
    const u8 other_map[HU_N]={1,2,0,3,4,5,6,7}; remap_household(other_map);
    stKid=stPart=255; stLoad(); stTvControl();
    assert(stKid==1&&stPart==2&&hhPUid==2&&add_calls==0); patient_absent(1);

    /* Both already-recorded cast IDs follow household UID reassignment. */
    fixture(2,1,0); stTvControl(); stSave(); remap_household(selected_map);
    stKid=stPart=255; stLoad();
    assert(stKid==1&&stPart==0&&hhPUid==0);
    hhSwitchTo(stMember(2)); assert(hhPUid==2); stTvWant=1; stTvControl();
    assert(stKid==1&&stPart==0&&hhPUid==0&&add_calls==0); patient_absent(1);

    /* Legacy saves use the active Sim only when the patient byte is 255. */
    fixture(2,0,0); stKid=255; stTvControl();
    assert(stKid==2&&stPart==hhPUid&&strcmp(hhPLast,"OTHER")==0); patient_absent(2);

    /* Reject invalid patient IDs and removed patients without creating new cast. */
    fixture(2,0,0); stKid=200; stTvControl();
    assert(hhPUid==2&&stTvWant==1&&add_calls==0&&last_toast);
    fixture(2,0,0); stKid=7; stTvControl();
    assert(hhPUid==2&&stTvWant==1&&add_calls==0&&last_toast);

    /* Actual join gate reserves one slot, except when an existing sister can be reused. */
    fixture(0,0,1); storyJoin(); assert(setup_calls==0&&enter_calls==0&&last_toast);
    fixture(0,1,1); storyJoin(); assert(setup_calls==1&&enter_calls==1);
    fixture(0,0,0); storyJoin(); assert(setup_calls==1&&enter_calls==1);

    /* Actual hhRemove must end the unfinished story before a UID can be reused. */
    fixture(2,0,0); stCh=2; stPending=1; stModal=2; stShown=123;
    int removed=stKid; hhRemove(stMember(removed));
    assert(stId==STY_NONE&&stKid==255&&stPart==255&&stPending==0);
    assert(stTvWant==0&&stTvWarn==0&&stModal==0&&stShown==0);
    assert(lnoteT==300&&strcmp(lnote,"MISSY IS GONE. STORY ENDED.")==0);
    int replacement=stAddSim(stMamesyLook,AG_ADULT,"STRANGER");
    assert(replacement>=0&&hhM[replacement].uid==removed);
    stTvWant=1; int adds=add_calls; stTvControl();
    assert(stId==STY_NONE&&hhPUid==2&&stTvWant==0&&add_calls==adds);
    assert(!stTvStoryAbsentUid(removed));
    stId=STY_TVSHOW; stKid=(u8)removed; stPart=1; stCh=3; stLoad();
    assert(stId==STY_NONE&&stKid==255&&stPart==255);
    storyJoin(); assert(setup_calls==1&&enter_calls==1); /* existing free-play restart path */

    /* Removing Mamesy before her part is over also cannot spawn a replacement lead. */
    fixture(2,1,0); stPart=1; stCh=3; hhRemove(stMember(1));
    assert(stId==STY_NONE&&stPart==255&&stKid==255);
    assert(lnoteT==300&&strcmp(lnote,"MAMESY IS GONE. STORY ENDED.")==0);
    replacement=stAddSim(stMamesyLook,AG_ADULT,"STRANGER");
    assert(hhM[replacement].uid==1&&!stTvStoryAbsentUid(1));

    /* Chapter five no longer needs Mamesy, even before the return to Missy. */
    fixture(2,1,0); stCh=4; stPart=1;
    int patient_member=stMember(0); assert(patient_member>=0); hhM[patient_member].act=HA_AWAY;
    hhRemove(stMember(1)); assert(stId==STY_TVSHOW&&stCh==4&&stPart==255&&stKid==0);
    replacement=stAddSim(stMamesyLook,AG_ADULT,"STRANGER");
    assert(hhM[replacement].uid==1&&!stTvStoryAbsentUid(1));
    stPart=1; stLoad(); assert(stPart==255&&stKid==0&&stCh==4);
    stTvWant=2; stTvControl(); assert(hhPUid==0&&stTvWant==0&&stId==STY_TVSHOW);

    /* Unrelated removals, other stories and unknown legacy patients are untouched. */
    fixture(0,0,0); stCh=2; hhRemove(stMember(2));
    assert(stId==STY_TVSHOW&&stCh==2&&stKid==0&&stTvWant==1&&lnote==0);
    fixture(2,0,0); stId=STY_PARENT; hhRemove(stMember(0));
    assert(stId==STY_PARENT&&stKid==0&&stCh==3&&lnote==0);
    fixture(2,0,0); stKid=255; hhRemove(stMember(0));
    assert(stId==STY_TVSHOW&&stKid==255&&stCh==3&&lnote==0);

    /* Completed stories keep their completed state; only stale references clear. */
    fixture(2,1,0); stCh=5; stPart=1; hhRemove(stMember(0));
    assert(stId==STY_TVSHOW&&stCh==5&&stKid==255&&stPart==1&&lnote==0);
    hhRemove(stMember(1)); assert(stId==STY_TVSHOW&&stCh==5&&stPart==255);
    replacement=stAddSim(stMamesyLook,AG_ADULT,"STRANGER");
    assert(!stTvStoryAbsentUid(hhM[replacement].uid));
    stKid=0; stPart=1; stCh=0; stLoad();
    assert(stId==STY_TVSHOW&&stCh==5&&stKid==255&&stPart==255);
    puts("BORE Missy handover: PASS (cast reuse, full house, retries, UID zero, reload, return, legacy, capacity, removal/reuse and completed stories)");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-missy-handover-") as tmp:
    harness = Path(tmp) / "handover.c"
    binary = Path(tmp) / "handover"
    declarations = "\nstatic const u8 stLen[STY_N]={[STY_TVSHOW]=6};\nstatic const char *stNm[STY_N];\n"
    harness.write_text(prefix + enum + declarations + functions + support, encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-misleading-indentation", "-I", str(ROOT), str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
