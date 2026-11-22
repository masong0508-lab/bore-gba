#!/usr/bin/env python3
"""Exercise actual personality validation, person serialization and creator edits.

The native UI saved Mamesy's 32-point personality and GOOD BOY's Grow Up lead
Pippa, but the reader rejected both legitimate personas. This extracts the real
encoder, decoder, SRAM personality functions and creator limits. Rendering and
flash I/O are host boundaries; this is not an emulator or UI playthrough.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
main = (ROOT / "source/main.c").read_text(encoding="utf-8")
slots = (ROOT / "source/slots.h").read_text(encoding="utf-8")


def function(name, source):
    match = re.search(r"^static [^;\n]+\b" + re.escape(name) + r"\([^;{}\n]*\)\s*\{", source, re.M)
    assert match, f"Missing actual function: {name}"
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')
    depth = 0
    for token in tokens.finditer(source, match.end() - 1):
        if token[0] == "{":
            depth += 1
        elif token[0] == "}":
            depth -= 1
            if not depth:
                return source[match.start():token.end()]
    raise AssertionError(f"Unclosed function: {name}")


def pick(pattern, source=main):
    match = re.search(pattern, source, re.M)
    assert match, pattern
    return match[0]


definitions = [pick(r"^enum \{ " + start + r",[\s\S]*?\};") for start in (
    "LK_SHAPE", "AG_BABY", "SX_FEMALE", "AS_FORTUNE", "TR_NEAT", "RK_PICK", "PS_ASP",
)]
definitions += [pick(r"^#define " + name + r" .*") for name in (
    "W", "D", "H", "NSW", "NSKIN", "NSHAPE", "NHAIR", "NBROW", "NNOSE", "NCHEEK", "NGLASS",
    "NEYE", "NMOUTH", "NSPR", "BUTT_OFF", "AS_PICK", "LTN", "TR_POINTS", "PERS_OFF", "PERS_LEN",
    *(f"LK_N{i}" for i in range(3, 17)),
)]
definitions += [pick(r"^#define " + name + r" .*", slots) for name in ("CNV", "SLOT_NSPR")]
definitions += [pick(r"^typedef struct \{[^\n]+\} " + name + r";", source) for name, source in (
    ("Row", main), ("SlW", slots), ("SlR", slots),
)]
definitions += [pick(r"^static const u8 signTr\[[\s\S]*?;")]

actual = "\n\n".join(
    [function(name, main) for name in (
        "trLeft", "signOf", "setSign", "persValid", "persSave", "persLoad", "persStep", "decSpr",
    )]
    + [function(name, slots) for name in (
        "slwInit", "slwPut", "slrGet", "slEncPlane", "slEncPerson", "slDecPlane", "slDecPerson",
    )]
)

# Exact format-17 C chunk from the native UI's previously rejected Mamesy save.
# Keep this fixture self-contained so CI can also recover that older save data.
captured = bytes.fromhex(
    "1100010101010201000000000000000000000001000001010100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003000100070509070400"
)
fixture = "static const u8 captured_mamesy[]={" + ",".join(str(v) for v in captured) + "};\n"
# Exact format-17 C chunk from a new GOOD BOY game, saved through the native UI.
# Pippa is an adult whose pre-made persona deliberately retains AS_GROW.
captured_pippa = bytes.fromhex(
    "1100020402010103050200000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003000500040708030300"
)
fixture += "static const u8 captured_pippa[]={" + ",".join(str(v) for v in captured_pippa) + "};\n"

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
'''

boundaries = r'''
static u8 look[LK_N], pAsp, pLtw, pTr[TR_N], stage, ageDays, custom;
static u16 pDna, pUnl;
static u8 SRAM_BASE[PERS_OFF+PERS_LEN];
static u8 vox[H][D][W], slTmp[3][CNV];
static u16 dec[H][D][W];
static int svRd(u32 off){ (void)off; assert(!"Test writer must use its RAM buffer"); return 0; }
static void svWr(u32 off,int byte){ (void)off;(void)byte; assert(!"Test writer must use its RAM buffer"); }
static int sexGuess(const u8 *lk){ (void)lk; return SX_FEMALE; }
static void fixLook(void){}
static void ageSave(void){}
static void buildLook(void){ custom=0; }
static void setColors(void){}
'''

tests = r'''
static const u8 mamesy_traits[TR_N]={7,5,9,7,4};
static int encode(u8 *bytes,int cap){
    SlW w; slwInit(&w,0,cap); w.ram=bytes; slEncPerson(&w);
    assert(!w.over); return w.pos;
}
static int decode(const u8 *bytes,int length,int apply){
    SlR r={(volatile u8*)bytes,0,length,0}; int result=slDecPerson(&r,apply);
    if(result) assert(!r.bad&&r.pos==length); return result;
}
static void clear_person(void){
    memset(look,0,sizeof(look)); memset(pTr,0,sizeof(pTr));
    pAsp=pLtw=ageDays=custom=0; stage=AG_ADULT; pDna=123; pUnl=7;
}
static void round_trip(const u8 *traits,int aspiration,int want,int life_stage){
    clear_person(); pAsp=(u8)aspiration; pLtw=(u8)want; stage=(u8)life_stage; ageDays=7; memcpy(pTr,traits,TR_N);
    u8 encoded[256], second[256]; int length=encode(encoded,sizeof(encoded));
    assert(decode(encoded,length,0));
    memset(pTr,0xff,TR_N); pAsp=pLtw=stage=ageDays=255;
    assert(decode(encoded,length,1));
    assert(pAsp==aspiration&&pLtw==want&&!memcmp(pTr,traits,TR_N));
    assert(stage==life_stage&&ageDays==7);
    assert(encode(second,sizeof(second))==length&&!memcmp(encoded,second,(size_t)length));
    persSave(); memset(pTr,0xff,TR_N); pAsp=pLtw=255; pDna=pUnl=0; persLoad();
    assert(pAsp==aspiration&&pLtw==want&&!memcmp(pTr,traits,TR_N)&&pDna==123&&pUnl==7);
}
static void captured_save(void){
    clear_person();
    assert(sizeof(captured_mamesy)==119);
    assert(decode(captured_mamesy,sizeof(captured_mamesy),0));
    assert(decode(captured_mamesy,sizeof(captured_mamesy),1));
    assert(pAsp==AS_KNOW&&pLtw==0&&!memcmp(pTr,mamesy_traits,TR_N));
    u8 again[256]; int n=encode(again,sizeof(again));
    assert(n==sizeof(captured_mamesy)&&!memcmp(again,captured_mamesy,(size_t)n));
    /* The SRAM personality copy must also retain an already-generated person. */
    memset(pTr,0,TR_N); pAsp=0; persLoad();
    assert(pAsp==AS_KNOW&&!memcmp(pTr,mamesy_traits,TR_N));
}
static void captured_grow_save(void){
    clear_person();
    assert(sizeof(captured_pippa)==119);
    assert(decode(captured_pippa,sizeof(captured_pippa),0));
    assert(decode(captured_pippa,sizeof(captured_pippa),1));
    assert(stage==AG_ADULT&&pAsp==AS_GROW&&pLtw==0&&!memcmp(pTr,signTr[2],TR_N));
    u8 again[256]; int n=encode(again,sizeof(again));
    assert(n==sizeof(captured_pippa)&&!memcmp(again,captured_pippa,(size_t)n));
    memset(pTr,0,TR_N); pAsp=0; persLoad();
    assert(pAsp==AS_GROW&&!memcmp(pTr,signTr[2],TR_N));
}
static void valid_fields(void){
    /* Every independently generated 0..10 combination is a valid saved persona. */
    unsigned count=0;
    for(int a=0;a<=10;a++) for(int b=0;b<=10;b++) for(int c=0;c<=10;c++)
    for(int d=0;d<=10;d++) for(int e=0;e<=10;e++){
        u8 traits[TR_N]={(u8)a,(u8)b,(u8)c,(u8)d,(u8)e};
        for(int asp=0;asp<AS_N;asp++) for(int want=0;want<LTN;want++){
            assert(persValid(asp,want,traits)); count++;
        }
    }
    assert(count==3865224u);
    const u8 zero[TR_N]={0}, high[TR_N]={10,10,10,10,10};
    /* GROW UP belongs to young people and some pre-made adult story leads. */
    for(int st=0;st<AG_N;st++) for(int asp=0;asp<AS_N;asp++) for(int want=0;want<LTN;want++){
        round_trip(zero,asp,want,st); round_trip(high,asp,want,st); round_trip(mamesy_traits,asp,want,st);
        for(int sign=0;sign<12;sign++) round_trip(signTr[sign],asp,want,st);
    }
}
static void invalid_fields(void){
    u8 bytes[256]; memcpy(bytes,captured_mamesy,sizeof(captured_mamesy));
    const int asp=1+LK_N+2, want=asp+1, traits=want+1;
    for(int t=0;t<TR_N;t++) for(int bad=11;bad<=255;bad++){
        u8 tr[TR_N]={7,5,9,7,4}; tr[t]=(u8)bad;
        assert(!persValid(1,0,tr)); bytes[traits+t]=(u8)bad;
        clear_person(); assert(!decode(bytes,sizeof(captured_mamesy),1));
        const u8 zero[TR_N]={0}; assert(!memcmp(pTr,zero,TR_N));
        bytes[traits+t]=captured_mamesy[traits+t];
    }
    for(int bad=AS_N;bad<=255;bad++){
        assert(!persValid(bad,0,mamesy_traits)); bytes[asp]=(u8)bad;
        assert(!decode(bytes,sizeof(captured_mamesy),0));
        clear_person(); assert(!decode(bytes,sizeof(captured_mamesy),1)); assert(pAsp==0);
    }
    bytes[asp]=captured_mamesy[asp];
    for(int bad=LTN;bad<=255;bad++){
        assert(!persValid(1,bad,mamesy_traits)); bytes[want]=(u8)bad;
        assert(!decode(bytes,sizeof(captured_mamesy),0));
    }
    bytes[want]=captured_mamesy[want];
    for(int length=0;length<sizeof(captured_mamesy);length++) assert(!decode(bytes,length,0));
    bytes[0]=18; assert(!decode(bytes,sizeof(captured_mamesy),0));
    bytes[0]=17; bytes[1+LK_N]=AG_N; assert(!decode(bytes,sizeof(captured_mamesy),0));
    /* Corrupt SRAM remains rejected by its original checksum guard. */
    clear_person(); memcpy(pTr,mamesy_traits,TR_N); persSave();
    SRAM_BASE[PERS_OFF+PERS_LEN-1]^=1; memset(pTr,0,TR_N); persLoad();
    const u8 zero[TR_N]={0}; assert(!memcmp(pTr,zero,TR_N));
}
static void creator_budget(void){
    clear_person(); const Row trait={"NEAT",0,RK_TRAIT,TR_NEAT,11};
    memcpy(pTr,signTr[0],TR_N); assert(trLeft()==0);
    persStep(&trait,1); assert(pTr[TR_NEAT]==5&&trLeft()==0);
    persStep(&trait,-1); assert(pTr[TR_NEAT]==4&&trLeft()==1);
    persStep(&trait,1); assert(pTr[TR_NEAT]==5&&trLeft()==0);
    memset(pTr,0,TR_N); persStep(&trait,-1); assert(pTr[TR_NEAT]==0);
    pTr[TR_NEAT]=10; persStep(&trait,1); assert(pTr[TR_NEAT]==10);
    memcpy(pTr,mamesy_traits,TR_N); assert(trLeft()==-7);
    persStep(&trait,1); assert(pTr[TR_NEAT]==7&&trLeft()==-7);
    for(int i=0;i<7;i++) persStep(&trait,-1);
    assert(pTr[TR_NEAT]==0&&trLeft()==0); persStep(&trait,1); assert(pTr[TR_NEAT]==0);
    Row other=trait; other.id=TR_OUT; persStep(&other,-1); assert(trLeft()==1);
    persStep(&trait,1); assert(pTr[TR_NEAT]==1&&trLeft()==0);
    /* Accepting a stored GROW UP persona does not add it to the creator picks. */
    const Row aspiration={"ASPIRATION",0,RK_PERS,PS_ASP,AS_PICK};
    for(int direction=-1;direction<=1;direction+=2){
        pAsp=0;
        for(int step=0;step<AS_PICK*2;step++){ persStep(&aspiration,direction); assert(pAsp<AS_PICK); }
        assert(pAsp==0);
        pAsp=AS_GROW; persStep(&aspiration,direction); assert(pAsp<AS_PICK);
    }
}
int main(void){
    captured_save(); captured_grow_save(); valid_fields(); invalid_fields(); creator_budget();
    puts("BORE person save: PASS (captured Mamesy and Pippa saves, exact person/SRAM round trips across all stages, all 3865224 valid field combinations, malformed bytes and actual creator limits)");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-person-save-") as tmp:
    harness = Path(tmp) / "person.c"
    binary = Path(tmp) / "person"
    harness.write_text("\n".join((prefix, *definitions, boundaries, actual, fixture, tests)), encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-misleading-indentation", "-Wno-sign-compare", "-I", str(ROOT), str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
