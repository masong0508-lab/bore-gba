#!/usr/bin/env python3
"""Check the actual vignette kernel against independent RGB15 channel maths.

Extracts the lighting kernels from csbg.h, compiling the game code on the host. Exhaustive
colors and clipping spans exercise aligned pairs, scalar heads/tails, empty
clips, and framebuffer canaries. Vignette checks cover the full sequence of
mood and edge passes within partial-picture clips. This measures equivalence,
not GBA speed.
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
#define CSG_ALL 0x7FFF
#define CB_BLACK 0
#define CB_RATE 9
enum { SW=240, SH=160, GUARD=8, WORDS=SW*SH+2*GUARD+1 };
_Alignas(4) static u16 storage[WORDS], reference[WORDS];
static u16 *fb, *expected;
static int cX0, cY0;
static unsigned cW, cH;
static int csMood;
static u32 rng=0x5EED1234u;
static unsigned long cases;
static unsigned long glow_cases;
static unsigned long vignette_cases;
'''

tests = r'''
static u16 random_color(void){ rng=rng*1664525u+1013904223u; return (u16)((rng>>8)&0x7FFF); }
static u16 scalar_dark(u16 c,int s){
    unsigned r=c&31u, g=(c>>5)&31u, b=(c>>10)&31u;
    return (u16)((c&0x8000u)|(r-(r>>s))|((g-(g>>s))<<5)|((b-(b>>s))<<10));
}
static u16 scalar_light_mask(u16 c,int s,u16 channels){
    unsigned r=c&31u, g=(c>>5)&31u, b=(c>>10)&31u;
    unsigned dr=((31-r)>>s)&(channels&31u);
    unsigned dg=((31-g)>>s)&((channels>>5)&31u);
    unsigned db=((31-b)>>s)&((channels>>10)&31u);
    return (u16)((c&0x8000u)|(r+dr)|((g+dg)<<5)|((b+db)<<10));
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
static void check_glow(int y,int x0,int x1,int s,u16 channels,int checked_pixels){
    if(y>=12&&y<116&&y>=cY0&&y<cY0+(int)cH){
        int a=x0>cX0?x0:cX0, b=x1<cX0+(int)cW?x1:cX0+(int)cW;
        for(int x=a;x<b;x++) expected[y*SW+x]=scalar_light_mask(expected[y*SW+x],s,channels);
    }
    csGlowRow(x0,x1,y,s,channels); glow_cases++;
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
static void exhaustive_glow_colors(int alignment){
    reset(alignment); cW=8;
    const int y=75;
    static const int span[4][2]={{0,8},{1,7},{0,7},{1,2}};
    static const u16 channels[]={0,0x001F,0x03E0,0x7C00,0x03FF,0x7FE0,0x7FFF,0x5555,0xAAAA};
    for(int s=1;s<=3;s++) for(unsigned color=0;color<32768;color++)
    for(unsigned ch=0;ch<sizeof(channels)/sizeof(*channels);ch++) for(int mode=0;mode<4;mode++){
        for(int x=0;x<8;x++){
            u16 c=((x^mode)&1)?random_color():(u16)color;
            if(mode&2) c|=0x8000u;
            fb[y*SW+x]=expected[y*SW+x]=c;
        }
        check_glow(y,span[mode][0],span[mode][1],s,channels[ch],8);
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
            check_glow(y,-2,SW+2,s,CSG_ALL,SW);
        }
    }
    whole_buffer_unchanged_outside_expected();
    /* Requested spans can also start/end inside the clip, miss it, or reverse. */
    cX0=3; cW=SW-6;
    for(int a=-2;a<=SW+2;a++) for(int b=-2;b<=SW+2;b++) for(int s=1;s<=3;s++){
        paint_row(y); check(y,a,b,s,SW);
        check_glow(y,a,b,s,0x03FF,SW);
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
                check_glow(y,-2,SW+2,s,0x7FE0,SW);
            }
        }
    }
    whole_buffer_unchanged_outside_expected();
}
static void vignette_clips(int alignment){
    static const int clips[][4]={
        {0,0,240,160}, {0,12,240,116}, {24,25,215,103}, {119,63,122,66},
        {0,0,0,0}, {0,0,240,12}, {0,116,240,160}, {0,13,240,14},
        {0,114,240,115}, {3,12,4,116}, {9,12,10,116}, {229,12,230,116},
        {235,12,236,116}, {239,12,240,116}, {24,25,24,103}, {24,25,215,25}
    };
    for(int mood=-1;mood<=2;mood++) for(int bg=0;bg<3;bg++)
    for(unsigned ci=0;ci<sizeof(clips)/sizeof(*clips);ci++){
        reset(alignment);
        for(int y=0;y<SH;y++) paint_row(y);
        cX0=clips[ci][0]; cY0=clips[ci][1];
        cW=(unsigned)(clips[ci][2]-cX0); cH=(unsigned)(clips[ci][3]-cY0);
        csMood=mood;
        int backdrop=bg==0?CB_BLACK:bg==1?CB_RATE:5;
        if(bg==2) for(int y=12;y<116;y++) for(int x=0;x<SW;x++){
            if(x<cX0||x>=cX0+(int)cW||y<cY0||y>=cY0+(int)cH) continue;
            u16 c=expected[y*SW+x];
            if(mood>0) c=scalar_dark(c,mood>1?1:2);
            else if(mood<0) c=scalar_light_mask(c,3,CSG_ALL);
            if((y<14||y>=114)&&x>=10&&x<230) c=scalar_dark(c,3);
            if(x<4||x>=236) c=scalar_dark(c,2);
            else if(x<10||x>=230) c=scalar_dark(c,3);
            expected[y*SW+x]=c;
        }
        csVig(backdrop); vignette_cases++;
        whole_buffer_unchanged_outside_expected();
    }
}
int main(void){
    for(int alignment=0;alignment<2;alignment++){
        exhaustive_colors(alignment);
        random_lanes(alignment);
        exhaustive_glow_colors(alignment);
        clipped_spans(alignment);
        clipped_rows(alignment);
        vignette_clips(alignment);
    }
    printf("BORE cinematic lighting: PASS (%lu kernel calls; all 32768 RGB15 colors, shifts 1..3, packed lanes, clip spans, alignments and canaries)\n",cases);
    printf("BORE cinematic glow: PASS (%lu kernel calls; all RGB15 colors, channel masks, packed lanes, clip spans, alignments and canaries)\n",glow_cases);
    printf("BORE cinematic vignette: PASS (%lu mood/background/clip combinations; pixels outside the active clip unchanged)\n",vignette_cases);
    return 0;
}
'''

with tempfile.TemporaryDirectory(prefix="bore-cslighting-") as tmp:
    harness = Path(tmp) / "lighting.c"
    binary = Path(tmp) / "lighting"
    kernels = "\n".join(function(name) for name in ("csGm", "csGlowRow", "csDk", "csVig"))
    harness.write_text(prefix + kernels + tests, encoding="utf-8")
    subprocess.run([
        os.environ.get("HOST_CC", "cc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
        "-Wno-misleading-indentation", "-fno-strict-aliasing", str(harness), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True)
