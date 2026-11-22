#!/usr/bin/env python3
"""Exercise the actual flash polling/writer functions against an explicit I/O fixture.

An observed native finale save wrote 0x5F at 0x5969, then read busy status 0x5F
instead of erased 0xFF at 0x596A and rejected the next byte, 0x61. The previous
player save survived that first reproduction. A later cold-start reproduction
also damaged a neighboring slot while the partial-sector erase copied it back.
This test extracts the real save.h polling, byte-write and partial-sector erase
functions plus slots.h's writer/checksum. Only accesses to the memory-mapped
flash bus are replaced with fixture calls.

The fixture models the observed 650-cycle busy window with six abstract ticks
per read, the minimum cadence of the compiled Thumb read loop. It tests ordering
and failure handling; it is not an emulator or a hardware timing measurement.
Native mGBA traces separately establish the observed status value and timing.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--save-header", type=Path, default=ROOT / "source/save.h",
                    help="Use an isolated candidate save.h for a before/after check")
parser.add_argument("--preservation-only", action="store_true",
                    help="Isolate the neighboring-slot regression for a before/after check")
args = parser.parse_args()
save = args.save_header.read_text(encoding="utf-8")
slots = (ROOT / "source/slots.h").read_text(encoding="utf-8")


def pick(pattern, source):
    match = re.search(pattern, source, re.M)
    assert match, pattern
    return match[0]


def function(name, source):
    match = re.search(r"^(?:IWRAM_THUMB )?static [^;\n]+\b" + re.escape(name)
                      + r"\([^;{}\n]*\)\s*\{", source, re.M)
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


def flash_io(source):
    # Preserve every branch, argument and loop. Replace only physical bus reads
    # and writes, including the volatile reads in the short settling guard.
    source = re.sub(r"SVB\[([^\]]+)\]\s*=(?!=)\s*([^;]+);",
                    r"flashWrite(\1,\2);", source)
    source = re.sub(r"SVB\[([^\]]+)\]", r"flashRead(\1)", source)
    assert "SVB[" not in source
    return source


definitions = [pick(r"^enum \{ SV_SRAM[^\n]+", save)]
definitions += [pick(r"^#define " + name + r" .*", save)
                for name in ("SV_SEC", "SV_LOW0", "SV_LOWN", "SV_BK", "SV_JN")]
definitions += [pick(r"^typedef struct \{[^\n]+\} SlW;", slots)]
actual = "\n\n".join(
    [flash_io(function(name, save)) for name in (
        "flCmd", "flWait", "flProgRaw", "flEraseRaw", "flBankRaw", "svBankTo", "svChip", "svRd", "svBk", "svWr",
        "svEraseSec", "svJWrite", "svErase",
    )]
    + [function(name, slots) for name in ("slwInit", "slwPut", "slwSum", "slSumOf")]
)

prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
#define IWRAM_THUMB
'''

boundaries = r'''
static u8 svType, svErr, svLow[SV_LOWN];
static u32 svSize, svScr;
static int svBank;
static u8 chip[131072];
static unsigned ticks, readyAt, busyTicks, reads, writes, resets, programs;
static unsigned command, unlockStep, hardwareBank, busySector;
static int stuckBusy, failNextProgram;

/* Explicit flash-bus boundary. The data byte is programmed immediately, while
 * reads of its sector return the observed busy value until the interval ends.
 * Erases use the same bounded interval here; this is not erase timing data. */
static u8 flashRead(u32 address){
    ticks+=6; reads++;
    if(svType==SV_SRAM) return chip[address&0x7FFF];
    u32 physical=(hardwareBank<<16)|(address&0xFFFF);
    if(stuckBusy||(ticks<readyAt&&(physical&~(SV_SEC-1))==busySector)) return 0x5F;
    return chip[physical];
}
static void flashWrite(u32 address,int value){
    u8 v=(u8)value; ticks++; writes++;
    if(svType==SV_SRAM){ chip[address&0x7FFF]=v; return; }
    if(command==0xA0){
        if(failNextProgram){ stuckBusy=1; failNextProgram=0; }
        u32 physical=(hardwareBank<<16)|(address&0xFFFF);
        if(!stuckBusy) chip[physical]&=v;
        busySector=physical&~(SV_SEC-1);
        readyAt=ticks+busyTicks; programs++; command=unlockStep=0; return;
    }
    if(command==0x80){
        if(unlockStep==0){ assert(address==0x5555&&v==0xAA); unlockStep=1; return; }
        if(unlockStep==1){ assert(address==0x2AAA&&v==0x55); unlockStep=2; return; }
        assert(v==0x30&&(address&(SV_SEC-1))==0);
        busySector=(hardwareBank<<16)|address;
        memset(chip+busySector,0xFF,SV_SEC); readyAt=ticks+busyTicks;
        command=unlockStep=0; return;
    }
    if(command==0xB0){ assert(address==0); hardwareBank=v&1; command=unlockStep=0; return; }
    if(address==0x5555&&v==0xF0){ resets++; readyAt=0; stuckBusy=0; command=unlockStep=0; return; }
    if(unlockStep==0){ assert(address==0x5555&&v==0xAA); unlockStep=1; return; }
    if(unlockStep==1){ assert(address==0x2AAA&&v==0x55); unlockStep=2; return; }
    assert(address==0x5555&&(v==0xA0||v==0xB0||v==0x80)); command=v; unlockStep=0;
}
static void fixture(unsigned latency){
    memset(chip,0xFF,sizeof(chip)); memset(svLow,0xFF,sizeof(svLow));
    svType=SV_FLASH; svErr=0; svSize=sizeof(chip); svScr=svSize-SV_SEC; svBank=0;
    ticks=readyAt=reads=writes=resets=programs=command=unlockStep=hardwareBank=busySector=0;
    busyTicks=latency; stuckBusy=failNextProgram=0;
}
'''

tests = r'''
static void transition(int first,int next,unsigned latency){
    fixture(latency);
    SlW w; slwInit(&w,0x5969,2); slwPut(&w,first); slwPut(&w,next);
    if(svErr||chip[0x5969]!=first||chip[0x596A]!=next){
        fprintf(stderr,"flash transition %02X -> %02X, busy %u: stored %02X %02X, svErr %u\n",
                first,next,latency,chip[0x5969],chip[0x596A],svErr);
        assert(!"actual flash writer failed a valid byte transition");
    }
    assert(!w.over&&w.pos==2&&slwSum(&w)==slSumOf(chip+0x5969,2));
}
static void transitions(void){
    transition(0x5F,0x61,650); /* exact native failure */
    transition(0x5F,0x5F,650); /* busy status must not make the next write look done */
    for(int next=0;next<256;next++){
        transition(0x5F,next,0);
        transition(0x5F,next,650);
        transition(0x42,next,650); /* ordinary polling still waits for the data */
    }
}
static void banks_and_ram(void){
    fixture(650);
    assert(svWr(0xFFFF,0x5F)); assert(svWr(0x10000,0x61));
    assert(!svErr&&chip[0xFFFF]==0x5F&&chip[0x10000]==0x61&&hardwareBank==1);
    assert(svRd(0xFFFF)==0x5F&&svBank==0&&hardwareBank==0);
    assert(svRd(0x10000)==0x61&&svBank==1&&hardwareBank==1);
    unsigned busWrites=writes;
    assert(svWr(SV_LOW0+17,0x5F)&&svRd(SV_LOW0+17)==0x5F);
    assert(writes==busWrites&&chip[SV_LOW0+17]==0xFF); /* deferred low-page RAM stays deferred */
    fixture(650); svType=SV_SRAM; svSize=32768;
    assert(svWr(0x5969,0x5F)&&svWr(0x596A,0x61));
    assert(chip[0x5969]==0x5F&&chip[0x596A]==0x61&&!programs&&!resets);
}
static void rejection_and_timeout(void){
    fixture(650); chip[0x596A]=0;
    assert(!svWr(0x596A,0x61)&&svErr&&chip[0x596A]==0&&!writes);
    fixture(650);
    assert(!svWr(sizeof(chip),0x61)&&svErr&&!writes); /* never program outside the chip */
    fixture(0); chip[0x596A]=0x61;
    assert(flWait(0x596A,0x61,1)&&reads==1&&!resets);
    fixture(650); stuckBusy=1;
    assert(!flWait(0x596A,0x61,7)&&reads==7&&resets==1&&!stuckBusy);
    /* A failed program remains a failure; the original bounded poll resets the
     * chip and svWr propagates its error instead of accepting busy status. */
    fixture(650); failNextProgram=1;
    assert(!svWr(0x596A,0x61)&&svErr&&chip[0x596A]==0xFF);
    assert(reads==60001&&resets==1&&programs==1&&!stuckBusy);
}
static void partial_sector_preservation(void){
    for(int bank=0;bank<2;bank++) for(int erasedHalf=0;erasedHalf<2;erasedHalf++){
        fixture(650);
        u32 sector=(u32)(bank*65536+0x5000), erased=sector+(u32)erasedHalf*2048;
        u32 kept=sector+(u32)(1-erasedHalf)*2048;
        u8 expected[2048];
        /* All 256 successors of 0x5F occur four times in the neighboring slot,
         * including 0x5F again and the native failure's 0x61. */
        for(unsigned i=0;i<sizeof(expected);i++) expected[i]=(i&1)?(u8)(i/2):0x5F;
        memcpy(chip+kept,expected,sizeof(expected)); memset(chip+erased,0xA5,2048);
        int checksum=slSumOf(expected,sizeof(expected));
        svErase(erased,2048); /* actual scratch park, journal, erase, copy-back */
        unsigned changed=0; for(unsigned i=0;i<sizeof(expected);i++) if(chip[kept+i]!=expected[i]) changed++;
        if(svErr||changed){
            fprintf(stderr,"partial-sector erase %05X: neighboring slot %05X changed %u bytes, svErr %u\n",
                    erased,kept,changed,svErr);
            assert(!"actual partial-sector erase damaged its neighboring slot");
        }
        assert(slSumOf(chip+kept,2048)==checksum);
        for(unsigned i=0;i<2048;i++) assert(chip[erased+i]==0xFF);
        /* The parked copy is exact and the completed journal is void. */
        assert(!memcmp(chip+svScr+(kept-sector),expected,sizeof(expected)));
        assert(chip[svScr+(erased-sector)]==0);
    }
}
int main(int argc,char**argv){
    (void)argv;
    if(argc==1){ transitions(); banks_and_ram(); rejection_and_timeout(); }
    partial_sector_preservation();
    if(argc>1) puts("BORE flash polling: PASS (actual partial-sector erase/scratch copy-back, both halves and banks, all 256 successors of 0x5F; host I/O fixture, not an emulator)");
    else puts("BORE flash polling: PASS (actual helpers, 0x5F transitions and checksums, all 256 successors, neighboring-slot preservation, banks/SRAM, bit preservation and timeout reset; host I/O fixture, not an emulator)");
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-flash-poll-") as tmp:
    harness = Path(tmp) / "flash.c"
    binary = Path(tmp) / "flash"
    harness.write_text("\n".join((prefix, *definitions, boundaries, actual, tests)), encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-misleading-indentation", str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary), *(["preservation"] if args.preservation_only else [])], check=True)
