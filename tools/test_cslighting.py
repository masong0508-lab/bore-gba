#!/usr/bin/env python3
"""Check the actual vignette kernel against independent RGB15 channel maths.

Extracts csGm/csDk from csbg.h, compiling the game code on the host. Exhaustive
colors and clipping spans exercise aligned pairs, scalar heads/tails, empty
clips, and framebuffer canaries. This test measures equivalence, not GBA speed.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / "source/csbg.h").read_text(encoding="utf-8")


def function(name):
    match = re.search(r"^(?:IWRAM_OVL2\s+)?static [^;\n]+\b" + re.escape(name) + r"\([^;\n]*\)\s*\{", source, re.M)
    if not match:
        raise AssertionError(f"Missing actual lighting function: {name}")
    tokens = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*[\s\S]*?\*/|[{}]')
    depth = 0
    for token in tokens.finditer(source, match.end() - 1):
        if token[0] == "{":
            depth += 1
        elif token[0] == "}":
            depth -= 1
            if depth == 0:
                return source[match.start():token.end()]
    raise AssertionError(f"Unclosed lighting function: {name}")


prefix = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint16_t u16;
typedef uint32_t u32;
#define CS_HOST 1
#define IWRAM_OVL2
enum { SW=240, SH=160, GUARD=8, WORDS=SW*SH+2*GUARD+1 };
_Alignas(4) static u16 storage[WORDS], reference[WORDS];
static u16 *fb, *expected;
static int cX0, cY0;
static unsigned cW, cH;
static u32 rng=0x5EED1234u;
static unsigned long cases;
'''

tests = r'''
static u16 random_color(void){ rng=rng*1664525u+1013904223u; return (u16)((rng>>8)&0x7FFF); }
static u16 scalar_dark(u16 c,int s){
    unsigned r=c&31u, g=(c>>5)&31u, b=(c>>10)&31u;
    return (u16)((c&0x8000u)|(r-(r>>s))|((g-(g>>s))<<5)|((b-(b>>s))<<10));
}
static void reset(int alignment){
    for(int i=0;i<WORDS;i++) storage[i]=reference[i]=(u16)(0x8000u|(i&0x7FFF));
    fb=storage+GUARD+alignment; expected=reference+GUARD+alignment;
    cX0=cY0=0; cW=SW; cH=SH;
}
static void paint_row(int y){
    for(int x=0;x<SW;x++) fb[y*SW+x]=expected[y*SW+x]=random_color();
}
static void whole_buffer_unchanged_outside_expected(void){
    assert(memcmp(storage,reference,sizeof(storage))==0);
}
static void check(int y,int x0,int x1,int s,int checked_pixels){
    if(y>=cY0&&y<cY0+(int)cH){
        int a=x0>cX0?x0:cX0, b=x1<cX0+(int)cW?x1:cX0+(int)cW;
        for(int x=a;x<b;x++) expected[y*SW+x]=scalar_dark(expected[y*SW+x],s);
    }
    csDk(y,x0,x1,s); cases++;
    if(y>=0&&y<SH) assert(memcmp(fb+y*SW,expected+y*SW,(size_t)checked_pixels*sizeof(u16))==0);
    else whole_buffer_unchanged_outside_expected();
}
static void exhaustive_colors(int alignment){
    reset(alignment); cW=8;
    static const int span[4][2]={{0,2},{1,7},{0,7},{1,2}};
    for(int s=1;s<=3;s++) for(unsigned color=0;color<32768;color++) for(int mode=0;mode<4;mode++){
        /* Every RGB15 value reaches both packed lanes and scalar boundaries. */
        for(int x=0;x<8;x++){
            u16 c=((x^mode)&1)?random_color():(u16)color;
            fb[x]=expected[x]=c;
        }
        check(0,span[mode][0],span[mode][1],s,8);
    }
    whole_buffer_unchanged_outside_expected();
}
static void random_lanes(int alignment){
    reset(alignment); cW=8;
    for(int s=1;s<=3;s++) for(int i=0;i<50000;i++){
        for(int x=0;x<8;x++) fb[x]=expected[x]=random_color();
        int a=i&1, b=2+(i%7);
        check(0,a,b,s,8);
    }
    whole_buffer_unchanged_outside_expected();
}
static void clipped_spans(int alignment){
    reset(alignment); const int y=75;
    /* Every possible clip start/end covers odd/even starts and widths,
       including width zero, one pixel, and the last framebuffer column. */
    for(cX0=0;cX0<=SW;cX0++) for(int right=cX0;right<=SW;right++){
        cW=(unsigned)(right-cX0);
        for(int s=1;s<=3;s++){
            paint_row(y); check(y,-2,SW+2,s,SW);
        }
    }
    whole_buffer_unchanged_outside_expected();
    /* Requested spans can also start/end inside the clip, miss it, or reverse. */
    cX0=3; cW=SW-6;
    for(int a=-2;a<=SW+2;a++) for(int b=-2;b<=SW+2;b++) for(int s=1;s<=3;s++){
        paint_row(y); check(y,a,b,s,SW);
    }
    whole_buffer_unchanged_outside_expected();
}
static void clipped_rows(int alignment){
    reset(alignment);
    for(cY0=0;cY0<=SH;cY0++){
        int remaining=SH-cY0;
        const unsigned height[4]={0,(unsigned)(remaining?1:0),(unsigned)(remaining<3?remaining:3),(unsigned)remaining};
        for(int h=0;h<4;h++){
            cH=height[h];
            const int row[4]={cY0-1,cY0,cY0+(int)cH-1,cY0+(int)cH};
            for(int r=0;r<4;r++) for(int s=1;s<=3;s++){
                int y=row[r]; if(y>=0&&y<SH) paint_row(y);
                check(y,-2,SW+2,s,SW);
            }
        }
    }
    whole_buffer_unchanged_outside_expected();
}
int main(void){
    for(int alignment=0;alignment<2;alignment++){
        exhaustive_colors(alignment);
        random_lanes(alignment);
        clipped_spans(alignment);
        clipped_rows(alignment);
    }
    printf("BORE cinematic lighting: PASS (%lu kernel calls; all 32768 RGB15 colors, shifts 1..3, packed lanes, clip spans, alignments and canaries)\n",cases);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-cslighting-") as tmp:
    harness = Path(tmp) / "lighting.c"
    binary = Path(tmp) / "lighting"
    harness.write_text(prefix + function("csGm") + "\n" + function("csDk") + tests, encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-misleading-indentation", "-fno-strict-aliasing", str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
