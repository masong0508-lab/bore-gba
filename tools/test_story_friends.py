#!/usr/bin/env python3
"""Check story friendship goals against actual town relationship functions.

The campaign, generic chapter and checklist predicates come from story.h.
Guest synchronization, remembered-friend counting and name validation come
from townrel.h. Fixture boundaries provide a small town, banks, mood and cash;
this is source-level integration coverage, not an interactive playthrough.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
story = (ROOT / "source/story.h").read_text(encoding="utf-8")
town = (ROOT / "source/townrel.h").read_text(encoding="utf-8")
house = (ROOT / "source/house.h").read_text(encoding="utf-8")


def function(name, source):
    match = re.search(r"^static [^;\n]+\b" + re.escape(name) + r"\([^;{}\n]*\)\s*\{", source, re.M)
    assert match, name
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')
    depth = 0
    for token in tokens.finditer(source, match.end() - 1):
        if token[0] == "{":
            depth += 1
        elif token[0] == "}":
            depth -= 1
            if not depth:
                return source[match.start():token.end()]
    raise AssertionError(name)


def pick(pattern, source):
    match = re.search(pattern, source, re.M)
    assert match, pattern
    return match[0]


definitions = [pick(r"^enum \{ " + start + r",[\s\S]*?\};", story) for start in (
    "STY_NONE", "SG_FRIEND", "TQ_CLEAN", "QK_NONE",
)]
definitions += [pick(r"^typedef struct \{[^\n]+\} " + name + r";", source) for name, source in (
    ("StCh", story), ("StQ", story), ("StT", story), ("NrE", house),
)]
definitions += [pick(r"^#define " + name + r" .*", source) for name, source in (
    ("PLUG_START", story), ("PLUG_BILL", story), ("PLUG_OK", story), ("STM_OFF", story),
    ("NR_N", house), ("NR_MARK", house), ("NR_LOST", house),
)]
definitions += [pick(r"^enum \{ RF_CRUSH[^\n]+\};", house)]

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "source/missyrules.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
'''

boundaries = r'''
#define HH_MAX 7
#define HU_N (HH_MAX+1)
#define TW_N 6
#define GU0 HU_N
#define RU_N (HU_N+TW_N)
#define TW_V(k) (HH_MAX-1-(k))
#define HH_NM 12
#define HH_NFAM 2
#define NB_LOTS 4
#define SX_NB 2
#define SX_N 3
#define LK_N15 100
#define LK_SEX 99
#define BK_HDR 40
#define MS_HAPPY 2
typedef struct { const char *name; int sex; } HhPre;
typedef struct { int n; HhPre m[3]; } HhFam;
typedef struct { u16 key; int cur,home; } Town;
static const HhFam hhFams[HH_NFAM]={
    {3,{{"DEX",0},{"ROWAN",1},{"JUNIE",2}}},
    {2,{{"BRANNET",0},{"KIARA",1}}},
};
static struct { u8 uid; } hhM[HH_MAX];
static Town nbT;
static int nbOk, live_lot[NB_LOTS], family_lot[NB_LOTS];
static int hhN, hhPUid, stId, stCh, stPart, stKid, stKidDay, stTkL, stGuest;
static u8 twHas[TW_N], twLot[TW_N], twMem[TW_N], twNh[TW_N];
static signed char relD[RU_N][RU_N], relL[RU_N][RU_N];
static u8 relF[RU_N][RU_N], SRAM_BASE[8192];
static NrE nrT[NR_N]; static u8 nrN; static u16 nrKey;
static u8 plugV, plugSec, plugTen, plugNight; static u16 plugDay;
static int simDay, happy, earned, jobLvl, skillLvl, simTricks, simDone, sRoom, prDays;
static long long simMoney;
static int lnoteT, stModal; static const char *lnote;
static u16 nbKey(const Town *t){ return t->key; }
static int nbLives(int lot){ return live_lot[lot]; }
static int nbFamOf(const Town *t,int lot){ (void)t; return family_lot[lot]; }
static int bkFind(int key,int lot){ (void)key;(void)lot; return -1; }
static void bkName(int bank,int mode,char *name){ (void)bank;(void)mode; name[0]=0; }
static u32 bkOff(int bank){ (void)bank; return 0; }
static int svRd(u32 offset){ (void)offset; return 0; }
static int stbEarned(void){ return earned; }
static int simMoneyI(void){ return (int)simMoney; }
static int moodState(void){ return happy; }
static int secCount(char c){ (void)c; return 0; }
static int trvHere(int kind){ (void)kind; return 0; }
static int skLvl(int kind){ (void)kind; return 0; }
static int prIn(void){ return 0; }
static int rnd8(void){ return 0; }
static void toast(const char *s){ (void)s; }
'''

actual = "\n\n".join([function(name, town) for name in (
    "nrHash", "nrCheckTown", "nrFind", "nrAbs", "nrNew", "nrGuestOk", "nrSync", "nrWho", "nrFriends",
)] + [function(name, story) for name in (
    "stMember", "stmGet", "stmPut", "plugRemember", "plugRestore", "stqDays", "stqVal", "stqOk", "stValue", "stTkNow", "plugTick",
)])

tests = r'''
static void fixture(void){
    memset(hhM,0,sizeof(hhM)); memset(nrT,0,sizeof(nrT));
    memset(twHas,0,sizeof(twHas)); memset(twLot,255,sizeof(twLot));
    memset(twMem,0,sizeof(twMem)); memset(twNh,0,sizeof(twNh));
    memset(relD,0,sizeof(relD)); memset(relL,0,sizeof(relL)); memset(relF,0,sizeof(relF));
    memset(SRAM_BASE,0,sizeof(SRAM_BASE));
    for(int i=0;i<NB_LOTS;i++){ live_lot[i]=1; family_lot[i]=i&1; }
    nbOk=1; nbT.key=nrKey=123; nbT.cur=nbT.home=0; nrN=0;
    hhPUid=0; hhN=2; hhM[0].uid=1; hhM[1].uid=2;
    stId=STY_TVSHOW; stCh=0; stPart=stKid=255; stKidDay=simDay=10; stTkL=stGuest=0;
    simMoney=0; earned=happy=0; plugV=20; plugDay=10; plugSec=plugTen=plugNight=0;
    stModal=0; lnote=0; lnoteT=0;
}
static int remembered(int own,int lot,int member,unsigned flags){
    int family=family_lot[lot]; int n=nrN++;
    assert(n<NR_N); nrT[n]=(NrE){(u8)lot,(u8)member,nrHash(hhFams[family].m[member].name),(u8)own,60,60,60,60,(u8)flags,0};
    return n;
}
static int tv_count(void){ const StQ q={TQ_FRIENDS,1}; return stqVal(&q); }
static int chapter_count(void){ const StCh c={"FRIENDS",SG_FRIENDS,1}; return stValue(&c); }
static int checklist(int count){ const StT t={QK_FRIENDS,0,(u32)count,"MAKE FRIENDS"}; return stTkNow(&t); }
static void all_paths(int count){
    assert(tv_count()==count); assert(chapter_count()==count);
    assert(checklist(count)); assert(!checklist(count+1));
}
static void remembered_and_identity(void){
    fixture(); all_paths(0);
    remembered(0,0,0,RF_FRIEND); assert(nrFriends(0)==1); all_paths(1); /* a real neighbor alone satisfies one friend */
    relF[0][1]=RF_FRIEND; all_paths(2);
    remembered(0,1,1,RF_FRIEND); all_paths(3);
    remembered(1,0,1,RF_FRIEND); all_paths(3); /* another household member's friend */
    int other=remembered(0,2,2,0); nrT[other].f2=RF_FRIEND; all_paths(3); /* one-way is not yours */
    int stale=remembered(0,3,0,RF_FRIEND); nrT[stale].nh^=1; all_paths(3);
    remembered(NR_MARK,0,2,RF_FRIEND); all_paths(3);
    live_lot[1]=0; all_paths(2); /* no longer lives there */
    hhPUid=1; all_paths(1); /* ownership follows the selected Sim */
}
static void active_guest_and_departure(void){
    fixture(); int k=0,g=GU0+k;
    twHas[k]=1; hhM[TW_V(k)].uid=(u8)g; twLot[k]=0; twMem[k]=0; twNh[k]=nrHash("DEX");
    relD[0][g]=relL[0][g]=60; relF[0][g]=RF_FRIEND;
    all_paths(1); assert(nrN==1); /* sync a just-made visitor friend into remembered state */
    all_paths(1); assert(nrN==1); /* no duplicate while visiting */
    relF[0][g]=0; all_paths(0); /* current relationship overrides an older saved flag */
    relF[0][g]=RF_FRIEND; all_paths(1);
    twHas[k]=0; memset(relF[0]+GU0,0,TW_N); all_paths(1); /* remembered after departure */
    relF[0][GU0+1]=RF_FRIEND; all_paths(1); /* unused visitor row is not a person */
    hhN=HH_MAX; for(int m=0;m<hhN;m++) hhM[m].uid=(u8)(m+1);
    all_paths(1); /* a full house does not erase known town friends */
}
static void cast_exclusion_and_support(void){
    fixture(); remembered(0,0,0,RF_FRIEND); relF[0][1]=RF_FRIEND; relF[0][2]=RF_FRIEND;
    stCh=4; stPart=1; stKid=0; assert(tv_count()==2); /* remembered Mamesy is not comeback support */
    assert(chapter_count()==3&&checklist(3)); /* ordinary story predicates have no TV exclusions */
    hhPUid=1; stCh=3; stPart=1; stKid=0;
    relF[1][0]=relF[1][2]=RF_FRIEND; remembered(1,1,0,RF_FRIEND);
    assert(tv_count()==2); /* hospital patient excluded, town friend included */
    relF[1][2]=0; remembered(1,1,1,RF_FRIEND); assert(tv_count()==2);
    plugSec=59; plugTen=9; plugTick();
    assert(plugV==21&&stModal==0); /* two town friends supply the same hospital support as household friends */
    happy=MS_HAPPY; earned=8000; plugSec=59; plugTen=9; plugTick(); assert(plugV==20);
}
int main(void){
    remembered_and_identity(); active_guest_and_departure(); cast_exclusion_and_support();
    puts("BORE story friends: PASS (actual town sync/name checks, all story goal paths, visiting/departed friends, ownership, cast exclusions and hospital support)");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-story-friends-") as tmp:
    source = Path(tmp) / "friends.c"
    binary = Path(tmp) / "friends"
    source.write_text("\n".join((prefix, *definitions, boundaries, actual, tests)), encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-misleading-indentation", "-Wno-sign-compare", "-I", str(ROOT), str(source), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
