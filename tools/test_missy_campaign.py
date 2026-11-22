#!/usr/bin/env python3
"""Run Missy's actual campaign state functions against controlled host fixtures.

Story setup, goals, clock/events, rewards, pressure, SRAM serialization, chapter
dispatch and handover functions are extracted from source/story.h. Household
switch admission is also the real house.h function; sprite transfer, world time,
input, rendering, audio and external reward storage are fixture boundaries.
This is a source-driven scenario test, not an interactive campaign playthrough.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "source/story.h").read_text(encoding="utf-8")
house = (ROOT / "source/house.h").read_text(encoding="utf-8")


def function(name, text=source):
    match = re.search(r"^static [^;\n]+\b" + re.escape(name) + r"\([^;\n]*\)\s*\{", text, re.M)
    assert match, f"Missing actual function: {name}"
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')
    depth = 0
    for token in tokens.finditer(text, match.end() - 1):
        if token[0] == "{":
            depth += 1
        elif token[0] == "}":
            depth -= 1
            if not depth:
                return text[match.start():token.end()]
    raise AssertionError(f"Unclosed function: {name}")


def declaration(name):
    match = re.search(r"^static const [^;\n]*\b" + re.escape(name) + r"\b[^;]*;", source, re.M)
    assert match, name
    return match[0]


enums = "\n".join(re.search(pattern, source, re.M)[0] for pattern in (
    r"^enum \{ STY_NONE,[\s\S]*?\};",
    r"^enum \{ SG_FRIEND,[\s\S]*?\};",
    r"^enum \{ TQ_CLEAN,[\s\S]*?\};",
))
tables = "\n".join(declaration(name) for name in ("stTv", "stLen", "stqT", "stqN"))
constants = "\n".join(re.search(r"^#define " + name + r" .*", source, re.M)[0] for name in (
    "PLUG_START", "PLUG_BILL", "PLUG_OK", "STM_OFF", "STB_OFF", "SM_PER",
))
functions = "\n\n".join(function(name) for name in (
    "stSum", "stSave", "stLoad", "stUidMap", "stOff", "stMember", "stAddSim", "stRel",
    "stmGet", "stmPut", "stmSlips", "stbGet", "stbSet", "stbEarned",
    "plugRemember", "plugRestore", "stmSlip", "stqDay", "stqStartDay", "stqDays", "stqVal", "stqOk", "stqDone", "stqAll",
    "stTvEvent", "stValue", "stDone", "stRew", "stComplete",
    "stTvStoryAbsentUid", "stTvStoryAbsentLabel", "stTvPatientRest", "stTvCan",
    "stTvMamesyMember", "stTvMamesyUid", "stTvControl", "plugTick", "stTick0", "stTick", "stEnter",
    "stPlugLose", "stRunModal0", "stRunModal", "storySetup",
)) + "\n" + function("hhSwitchTo", house)

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "source/missyrules.h"
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef int64_t money_t;
#define EWRAM_BSS
#define HH_MAX 7
#define HU_N 8
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
#define RF_LOVE 4
#define RF_STEADY 8
#define RF_CRUSH 16
#define RF_KISSED 32
#define STORY_OFF 64
#define TW_N 2
#define MS_HAPPY 2
#define SE_FRIEND 1
#define SE_BFF 2
#define SE_PROMO 3
#define SE_PIPE 4
#define SE_FAINT 5
#define SE_PASSOUT 6
#define K_A 1
#define K_B 2
#define K_START 4
#define GOLD 1
#define WHITE 2
#define RGB(r,g,b) ((r)|((g)<<5)|((b)<<10))
#define UL_HERETODAY 1
#define RW_N 1
#define NCAT 1
typedef struct { const char *nm; u8 goal; u32 arg; } StCh;
typedef struct { u8 k; u32 n; } StQ;
typedef struct { const char *br[2]; } StB;
typedef struct { int unused; } StCast;
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
static char hhPName[HH_NM], hhPLast[HH_NM];
static const char *lnote, *last_toast;
static const u8 stMamesyLook[LK_N] = {0};
static u8 stId, stCh, stPart, stKid, stKidDay, stGuest, stTkL, stPending;
static u8 stTvWant, stTvWarn, SRAM_BASE[8192], twOn[TW_N];
static int simDay, moods[HU_N], camera_count, gate_count;
static money_t simMoney;
static unsigned jobLvl;
static int skillLvl, simTricks, simDone, bdOwn, jobNone;
static int sgPid, sgDiscard, sgDirty, key_calls;
static u8 csStoryMemory, stGotN;
static const char *stGotP[2], *slkNm[1]={"GIFT"}, *palNm[1]={"REWARD"};
static const int rwNeed[1]={1000}, catItems[1][1]={{0}};
static int films[100], film_count, unlock_count, reward_mask, dna;
static int add_calls, switch_calls, prison_blocks;

/* Boundaries: the actual story and selection functions below call these. */
static void toast(const char *s){ last_toast=s; }
static int rnd8(void){ return 1; }
static void hhBakeAll(void){}
static void hhSave(void){}
static void liveInvalidate(void){}
static int hhOnOtherFloor(int m){ return hhUp[m]!=0; }
static int prSwitchHook(int m){ (void)m; return prison_blocks; }
static int hhAdd(const u8*, int, int, int, const u8*);
static void hhSwitchTo(int);
static void hhSwitchFrom(int);
static int simMoneyI(void){ return (int)simMoney; }
/* This campaign fixture has no town friends; test_story_friends.py covers real town synchronization. */
static int nrFriends(int u){ (void)u; return 0; }
static void simMoneyAdd(money_t n){ simMoney+=n; }
static int moodState(void){ return moods[hhPUid]; }
static int secCount(char kind){ return kind=='n'?camera_count:gate_count; }
static void dnaAdd(int n){ dna+=n; }
static void persSave(void){}
static void simsSave(void){}
static void simQPush(const char *s){ (void)s; }
static int rwTotal(void){ return 0; }
static int jbStoryCount(int story){ (void)story; return __builtin_popcount((unsigned)reward_mask); }
static int jbStoryDone(int story,int chapter){ (void)story; reward_mask|=1<<chapter; return 0; }
static int slkGift(int story){ (void)story; return -1; }
static void stGotAdd(const char *s){ (void)s; }
static char *slCat(char *out,const char *s){ while((*out=*s++)) out++; return out; }
static char *simCat(char *out,const char *s){ return slCat(out,s); }
static char *slNum(char *out,int n){ return out+sprintf(out,"%d",n); }
static void stAnnounce(void){}
static void stFix(void){}
static void stTkTick(void){}
static void stKidHome(void){ assert(!"Non-TV child goal reached"); }
static int stTkN(const StB *b){ (void)b; return 0; }
static const StB *stBkNow(void){ return 0; }
static void papTick(void){}
static void stqBeatTick(void){}
static void stPlaces(void){}
static void bdSave(void){}
static void stSeeds(void){}
static int stCastSim(const StCast *c){ (void)c; assert(!"Non-TV cast setup reached"); return -1; }
static void csPlay(int scene){ assert(film_count<100); films[film_count++]=scene; }
static void jbUnlock(int id){ assert(id==UL_HERETODAY); unlock_count++; }
static u16 keyNow(void){ return key_calls++&1?K_A:0; }
static void present(void){ assert(!"Fixture confirmation should close card before drawing"); }
static void stBack(const char *s,int n){ (void)s; (void)n; }
static void s2rr(int x,int y,int w,int h,u16 c){ (void)x;(void)y;(void)w;(void)h;(void)c; }
static void rect(int x,int y,int w,int h,u16 c){ (void)x;(void)y;(void)w;(void)h;(void)c; }
static void s2grad(int x,int y,int w,int h,int r,int g,int b,int r1,int g1,int b1){ (void)x;(void)y;(void)w;(void)h;(void)r;(void)g;(void)b;(void)r1;(void)g1;(void)b1; }
static void stIcon(int id,int x,int y,int sc,u16 c){ (void)id;(void)x;(void)y;(void)sc;(void)c; }
static void text(int x,int y,const char *s,u16 c,int sc){ (void)x;(void)y;(void)s;(void)c;(void)sc; }
static void s2pill(int x,int y,int w,const char *s){ (void)x;(void)y;(void)w;(void)s; }
static int tw(const char *s,int sc){ return (int)strlen(s)*6*sc; }
static void stSparkle(u32 n,int x,int y,int w,int h){ (void)n;(void)x;(void)y;(void)w;(void)h; }
static int stTkPlace(int *ct){ (void)ct; return 0; }
static void stTkList(int x,int y,int dy){ (void)x;(void)y;(void)dy; }
static const char *stqWhyNow(void){ return ""; }
static int stProg(const StCh *c,char *b){ (void)c;(void)b; return 0; }
static void stqList(int x,int y,int dy){ (void)x;(void)y;(void)dy; }
'''

declarations = r'''
static const StCh *const stChs[STY_N]={[STY_TVSHOW]=stTv};
static const char *stNm[STY_N], *stTag[STY_N];
static const StCast stCast[STY_N][2]={0};
static u8 plugV=PLUG_START, plugSec, plugTen, plugNight;
static u16 plugDay=0xffff;
'''

scenarios = r'''
static int hhAdd(const u8 *look,int stage,int asp,int ltw,const u8 *traits){
    (void)look;(void)stage;(void)asp;(void)ltw;(void)traits;
    add_calls++; if(hhN>=HH_MAX) return -1;
    int uid=0; while(uid<HU_N&&(uid==hhPUid||stMember(uid)>=0)) uid++;
    assert(uid<HU_N); HhSim *s=&hhM[hhN]; memset(s,0,sizeof(*s)); s->uid=(u8)uid;
    strcpy(s->name,"ROOMMATE"); strcpy(s->last,"OTHER"); return hhN++;
}
static void hhSwitchFrom(int m){
    switch_calls++; assert(m>=0&&m<hhN);
    HhSim selected=hhM[m], old={0}; old.uid=(u8)hhPUid;
    strcpy(old.name,hhPName); strcpy(old.last,hhPLast);
    for(int i=m;i<hhN-1;i++){ hhM[i]=hhM[i+1]; hhUp[i]=hhUp[i+1]; hhFl[i]=hhFl[i+1]; }
    hhM[hhN-1]=old; hhUp[hhN-1]=hhFl[hhN-1]=0;
    hhPUid=selected.uid; strcpy(hhPName,selected.name); strcpy(hhPLast,selected.last);
}
static void ticks(int n){ while(n-->0) stTick(); }
static void modal(void){ assert(stModal); key_calls=0; stRunModal(); assert(!stModal); }
static void reload(void){
    /* The saved SRAM bytes survive; transient chapter/pressure UI state does not. */
    stId=stCh=stPending=stGuest=stTkL=0; stPart=stKid=stKidDay=255;
    stModal=stShown=0; stTvWant=stTvWarn=0;
    plugV=PLUG_START; plugDay=0xffff; plugSec=plugTen=plugNight=0;
    stEnter();
}
static void fixture(void){
    memset(SRAM_BASE,0,sizeof(SRAM_BASE)); memset(hhM,0,sizeof(hhM));
    memset(hhUp,0,sizeof(hhUp)); memset(hhFl,0,sizeof(hhFl)); memset(kin,0,sizeof(kin));
    memset(relF,0,sizeof(relF)); memset(relD,0,sizeof(relD)); memset(relL,0,sizeof(relL));
    memset(moods,0,sizeof(moods)); memset(twOn,0,sizeof(twOn));
    hhN=2; hhPUid=0; strcpy(hhPName,"MISSY"); strcpy(hhPLast,"JEANNE");
    for(int i=0;i<hhN;i++){ hhM[i].uid=(u8)(i+1); strcpy(hhM[i].name,"ROOMMATE"); }
    simDay=10; simMoney=20000; custom=prison_blocks=0; camera_count=gate_count=0;
    sgPid=sgDiscard=sgDirty=key_calls=0; film_count=unlock_count=reward_mask=dna=0;
    add_calls=switch_calls=0; last_toast=lnote=0; lnoteT=0;
    storySetup(STY_TVSHOW);
}
static void friend_of(int who,int friend){ relF[who][friend]=relF[friend][who]=RF_FRIEND; }
static void earn(int amount){ simMoney=stbGet()+amount; }
static void films_since(int before,const int *want,int n){
    assert(film_count==before+n); for(int i=0;i<n;i++) assert(films[before+i]==want[i]);
}
static void chapter_clock(void){
    assert(stKidDay==255); ticks(60); assert(stKidDay!=255&&stqDays()==0);
}
static void hospital(void){
    fixture(); stCh=3; stKid=0; stPart=255; stKidDay=10; stPending=0;
    plugV=50; plugDay=(u16)simDay; plugSec=plugTen=plugNight=0; plugRemember(plugV);
    stTvWant=1; stTvControl(); assert(hhPUid==stPart&&stKid==0); stSave();
}
static void full_campaign(void){
    fixture(); stEnter(); modal();
    assert(stCh==0&&stKid==0&&stKidDay==simDay&&films[0]==7&&film_count==1);
    /* Chapter one: all three live goals, payment, then reload before its films. */
    friend_of(0,1); twOn[0]=2; earn(8000); ticks(60);
    assert(stCh==1&&stPending&&stModal==2&&dna==25&&reward_mask==1);
    money_t cash=simMoney; stSave(); reload(); assert(stPending&&stModal==2);
    int before=film_count; modal(); const int ch1[]={12,0,8}; films_since(before,ch1,3);
    assert(!stPending&&simMoney==cash); chapter_clock();
    /* Chapter two: a relapse resets only the clean streak, and survives reload. */
    friend_of(0,2); moods[0]=MS_HAPPY; simDay+=6; ticks(60); assert(stCh==1);
    stTvEvent(SE_PIPE); assert(stqDays()==0&&stmSlips()==1); stSave(); reload(); modal();
    assert(stqDays()==0&&stmSlips()==1); simDay+=6; ticks(60); assert(stCh==1);
    simDay++; ticks(60); assert(stCh==2&&stPending&&reward_mask==3);
    before=film_count; modal(); const int ch2[]={1,9}; films_since(before,ch2,2); chapter_clock();
    /* Chapter three: camera/gate mix matters; faint/pass-out reset its clean days. */
    camera_count=4; gate_count=0; earn(16000); simDay+=4; ticks(60); assert(stCh==2);
    stTvEvent(SE_FAINT); assert(stqDays()==0&&stmSlips()==2);
    simDay+=2; stTvEvent(SE_PASSOUT); assert(stqDays()==0&&stmSlips()==3);
    camera_count=gate_count=2; simDay+=4; ticks(60); assert(stCh==3&&stPending&&stKid==0);
    /* Ordinary selection and a pending-film reload cannot replace Missy's identity. */
    hhSwitchTo(stMember(2)); assert(hhPUid==2); stSave(); reload();
    assert(stCh==3&&stKid==0&&stPending&&stModal==2);
    before=film_count; modal(); const int ch3[]={2}; films_since(before,ch3,1);
    assert(stPart!=255&&hhPUid==stPart&&stKid==0&&add_calls==1);
    assert(stTvStoryAbsentUid(0)); hhSwitchTo(stMember(0)); assert(hhPUid==stPart);
    chapter_clock(); int sister=stPart;
    hhSwitchTo(stMember(1)); assert(hhPUid==1); stSave(); reload(); modal();
    assert(hhPUid==sister&&stKid==0&&stPart==sister&&add_calls==1);
    /* Five bedside days with support; a friend/promotion immediately after reload helps. */
    friend_of(sister,1); friend_of(sister,2); moods[sister]=MS_HAPPY; earn(8000+5*PLUG_BILL);
    unsigned old=plugV; stTvEvent(SE_FRIEND); assert(plugV==msPressureEase(old,10));
    for(int d=0;d<4;d++){ simDay++; ticks(600); assert(stCh==3); }
    simDay++; ticks(60); assert(stCh==4&&stPending&&reward_mask==15);
    before=film_count; stSave(); reload(); modal(); const int ch4[]={3,10,4}; films_since(before,ch4,3);
    assert(hhPUid==0&&stKid==0&&stPart==sister&&stTvStoryAbsentUid(sister));
    hhSwitchTo(stMember(sister)); assert(hhPUid==0); chapter_clock();
    /* Finale: the absent sister contributes neither a friend nor a living housemate. */
    while(hhN<5) assert(stAddSim(stMamesyLook,AG_ADULT,"OTHER")>=0);
    friend_of(0,4); friend_of(0,5); moods[0]=MS_HAPPY; earn(18000);
    simDay+=9; ticks(60); assert(stCh==4);
    stTvEvent(SE_PASSOUT); assert(stqDays()==0&&stmSlips()==3);
    simDay+=9; ticks(60); assert(stCh==4); simDay++; ticks(60);
    assert(stCh==5&&stPending&&reward_mask==31&&dna==125);
    stSave(); reload(); assert(stCh==5&&stPending&&stModal==2);
    before=film_count; modal(); const int finale[]={11,5}; films_since(before,finale,2);
    assert(unlock_count==1&&!stPending);
    cash=simMoney; stSave(); reload(); modal(); ticks(1200);
    assert(stCh==5&&simMoney==cash&&dna==125&&unlock_count==1&&film_count==before+2);
}
static void reprieve_and_terminal(void){
    hospital(); simDay=15; plugDay=15; plugV=99; plugTen=9; plugSec=59;
    int start=stKidDay; money_t cash=simMoney; plugTick();
    assert(plugV==50&&plugNight==1&&msNightUsed(stmGet())&&simMoney==cash-1000);
    assert(stKidDay==start&&stqDays()==5&&stqVal(&stqT[3][0])==4);
    friend_of(hhPUid,1); friend_of(hhPUid,2); moods[hhPUid]=MS_HAPPY; earn(8000);
    ticks(60); assert(stCh==3); stSave(); reload(); modal();
    assert(msNightUsed(stmGet())&&stKidDay==start&&stqVal(&stqT[3][0])==4);
    simDay++; ticks(60); assert(stqVal(&stqT[3][0])==5);
    stTvEvent(SE_BFF); earn(8000); ticks(60); assert(stCh==4); /* precisely one extra day */

    hospital(); stmPut(msPressurePack(3|MS_ONE_MORE_NIGHT,98));
    plugNight=1; plugV=98; plugDay=(u16)simDay; plugSec=59; plugTen=9;
    plugTick(); assert(plugV==100&&stModal==3);
    modal(); assert(films[film_count-1]==6&&sgDiscard==0&&stCh==3);
    assert(stKidDay==stqDay()&&stqDays()==0);
    /* Regression: an unsaved retry must reset the persisted pressure AND reprieve. */
    assert((stmGet()&15)==3&&!msNightUsed(stmGet()));
    plugRestore(); assert(plugV<=PLUG_START&&!plugNight);
    ticks(60); assert(stCh==3&&!stModal);

    hospital(); sgPid=4; plugV=100; plugNight=1;
    stmPut(msPressurePack(MS_ONE_MORE_NIGHT,100)); stModal=3;
    modal(); assert(sgDiscard==2&&films[film_count-1]==6);
    assert(msNightUsed(stmGet())); /* the player-file rollback owns this path */
}
static void day_boundary(void){
    /* Every possible day byte gets a zero-day start and exact ten-day streak. */
    for(int day=0;day<258;day++){
        fixture(); simDay=day; stCh=1; stKidDay=255; stSave();
        chapter_clock(); assert(stqDays()==0);
        assert(!!(stTkL&MS_DAY_255)==((day&255)==255));
        stSave(); reload(); assert(stqDays()==0);
        for(int d=1;d<=10;d++){ simDay=day+d; assert(stqDays()==d); }
        stTvEvent(SE_PIPE); assert(stqDays()==0); stSave(); reload(); assert(stqDays()==0);
        simDay++; assert(stqDays()==1);
    }
}
static void marker_lifecycle(void){
    /* The marker survives saving but never leaks into the next chapter. */
    for(int chapter=0;chapter<5;chapter++){
        fixture(); stCh=(u8)chapter; simDay=255; stqStartDay();
        assert(stKidDay==254&&(stTkL&MS_DAY_255)); stSave(); reload();
        assert(stCh==chapter&&stqDays()==0&&(stTkL&MS_DAY_255));
        stComplete(); assert(stCh==chapter+1&&stKidDay==255&&stTkL==0);
        stLoad(); assert(stCh==chapter+1&&stKidDay==255&&stTkL==0);
    }
    /* Every relapse kind replaces, rather than accumulates, the previous marker. */
    const int chapters[]={1,2,4}, events[]={SE_PIPE,SE_FAINT,SE_PASSOUT};
    for(int c=0;c<3;c++) for(int e=0;e<3;e++) for(int day=254;day<=256;day++){
        fixture(); stCh=(u8)chapters[c]; simDay=255; stqStartDay(); simDay=day;
        stTvEvent(events[e]); assert(stqDays()==0);
        assert(!!(stTkL&MS_DAY_255)==(day==255)); stSave(); reload(); assert(stqDays()==0);
        simDay++; assert(stqDays()==1);
    }
    fixture(); simDay=255; stqStartDay(); stOff();
    assert(stId==STY_NONE&&stTkL==0&&stKidDay==255); stLoad(); assert(stTkL==0);
    storySetup(STY_TVSHOW); assert(stTkL==0&&stKidDay==255&&stmGet()==0);

    /* Old day-254 saves and the old 255="not started" sentinel keep their meaning. */
    fixture(); stCh=1; stKidDay=254; stTkL=0; simDay=255; stSave(); reload();
    assert(stqDays()==1); stKidDay=255; stTkL=MS_DAY_255; stSave(); reload();
    assert(stqDays()==0); ticks(60); assert(stKidDay==254&&stqDays()==0);

    /* Other stories own all three task bits; TV's clock helper cannot alter them. */
    for(int tasks=0;tasks<8;tasks++){
        fixture(); stId=STY_ROOM; stTkL=(u8)tasks; simDay=255; stqStartDay();
        assert(stTkL==tasks); stSave(); stTkL=0; stLoad(); assert(stTkL==tasks);
        assert(stqDays()==1); /* non-TV date interpretation remains unchanged */
    }
    /* A fresh unsaved hospital retry on the reserved day also starts at zero. */
    hospital(); simDay=255; plugV=100; plugNight=1;
    stmPut(msPressurePack(3|MS_ONE_MORE_NIGHT,100)); stModal=3; modal();
    assert(stKidDay==254&&(stTkL&MS_DAY_255)&&stqDays()==0&&!msNightUsed(stmGet()));
    stSave(); reload(); assert(stqDays()==0); simDay=256; assert(stqDays()==1);
}
int main(void){
    full_campaign(); reprieve_and_terminal(); day_boundary(); marker_lifecycle();
    puts("BORE Missy campaign scenarios: PASS (actual-source five chapters, selection/reload, relapse, final night, terminal retry, finale and day boundaries; not an interactive playthrough)");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-missy-campaign-") as tmp:
    harness = Path(tmp) / "campaign.c"
    binary = Path(tmp) / "campaign"
    harness.write_text("\n".join((prefix, enums, constants, tables, declarations, functions, scenarios)), encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-unused-function", "-Wno-misleading-indentation", "-Wno-sign-compare",
        "-I", str(ROOT), str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
