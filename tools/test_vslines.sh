#!/bin/sh
# test_vslines.sh - the scrolling-text line breaker (main.c vsLines) on a PC, with the game's own font widths, under AddressSanitizer + UBSan.
# It pulls vsLines out of source/main.c itself (so it tests the real code), then runs it on every mission brief in expand.h and kingmis.h and on
# 200,000 random texts of narrow letters (the case that overflowed its 44 byte buffers and hung every expansion pack's page).
# Termux has no writable /tmp: it works in $TMPDIR, or in a folder of its own.
cd "$(dirname "$0")/.." || exit 1
W="${TMPDIR:-.}/vslines_test.$$"; mkdir -p "$W" || exit 1
awk '/^#define VS_L /{p=1} /^static int vsOver/{p=0} p' source/main.c > "$W/vs_real.h"
grep -q "vsLines" "$W/vs_real.h" || { echo "FAILED: could not find vsLines in source/main.c"; rm -rf "$W"; exit 1; }
python3 - "$W" <<'PYX'
import re, sys
src = open('source/expand.h').read() + open('source/kingmis.h').read()
b = ['"%s %s"' % (m.group(2), m.group(3)) for m in re.finditer(r'XM\("((?:[^"\\]|\\.)*)",\s*"((?:[^"\\]|\\.)*)",\s*"((?:[^"\\]|\\.)*)"', src)]
open(sys.argv[1] + '/briefs.h', 'w').write('static const char*B[]={%s};\n#define NB %d\n' % (",".join(b), len(b)))
PYX
cat > "$W/t.c" <<'CEOF'
#include <stdio.h>
#include <string.h>
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#include "fontdata.h"
#include "briefs.h"
static int fIdx(char ch){ const char*q=FNT_CHARS; for(int i=0;*q;q++,i++) if(*q==ch) return i; return -1; }
static int tw(const char*s,int sc){ (void)sc; int w=0; while(*s){ int i=fIdx(*s++); w+=(i<0)?FSP_s:fa_s[i]; } return w; }
#include "vs_real.h"
static int fails;
#define CK(c,m) do{ if(!(c)){ printf("FAIL: %s\n",m); fails++; } }while(0)
static unsigned rs=12345; static unsigned rr(void){ rs=rs*1103515245u+12345u; return rs>>8; }
static void check(const char*s,int w,int expectAll){
    char L[VS_L][44]; int n=vsLines(s,w,L);
    CK(n>=0&&n<=VS_L,"line count in range");
    for(int q=0;q<n;q++){ CK(strlen(L[q])<=42,"a line is at most 42 characters"); CK(L[q][0]!=' '||0,"no leading space"); }
    (void)expectAll;
}
int main(void){
    int ran=0, mx=0;
    for(int i=0;i<NB;i++){ char L[VS_L][44]; check(B[i],216,1); int n=vsLines(B[i],216,L); if(n>mx) mx=n; CK(n<VS_L,"no brief fills all the lines (it would be cut off)"); ran++; }
    CK(ran>=60,"found the missions' briefs");
    const char al[]="iIl.,;:!'ab cdeWMwm ";
    for(int it=0;it<200000;it++){ char buf[300]; int n=(int)(rr()%250); for(int j=0;j<n;j++) buf[j]=al[rr()%(sizeof al-1)]; buf[n]=0; check(buf,60+(int)(rr()%180),0); ran++; }
    { char big[400]; memset(big,'i',sizeof big-1); big[sizeof big-1]=0; check(big,216,0); }   /* one word with no spaces */
    printf("vslines: %d texts, longest brief %d lines: %s, %d failure(s)\n",ran,mx,fails?"FAILED":"ALL PASSED",fails);
    return fails?1:0;
}
CEOF
cc -fsanitize=address,undefined -fno-sanitize-recover=undefined -w -I"$W" -Isource "$W/t.c" -o "$W/t" && ASAN_OPTIONS=detect_leaks=0 "$W/t"; rc=$?
rm -rf "$W"; exit $rc
