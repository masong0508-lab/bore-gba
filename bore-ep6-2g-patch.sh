#!/bin/sh
# BORE EP6 step 2g (bit e, the last piece): EP6 gets its OWN block of missions-done bits, so the ten-mission campaign fits (16 missions possible).
#   - new source/kingdone.h: 3 bytes in SRAM at 7607..7609 (the free gap after the game totals, before the life stats): 16 bits + a check byte.
#   - expand.h: EP6's missions read and write that block (the old shared bits 60..63 still count as done, so nothing already earned is lost);
#     the "only four missions" assert becomes "at most sixteen". No other block moves, so every old save loads as before.
#   - new tools/test_kingdone.c + tools/test_kingdone.sh (PC test) and a section in docs/EP6_STEP2.md.
# Needs step 2f applied (HEAD "EP6 step 2f"). Run inside ~/bore-gba (Termux: cd ~/bore-gba && sh bore-ep6-2g-patch.sh). Nothing is written unless every anchor matches.
[ -f source/expand.h ] && [ -f source/kingslot.h ] && [ -f docs/EP6_STEP2.md ] && [ -d tools ] || { echo "run this inside ~/bore-gba"; exit 1; }
grep -q "second half, part 1" docs/EP6_STEP2.md || { echo "apply step 2f first (git pull)"; exit 1; }
[ -f source/kingdone.h ] && { echo "already applied"; exit 0; }
command -v python3 >/dev/null || { echo "needs python3 (pkg install python)"; exit 1; }
python3 -I - <<'EOF'
import sys
def rd(p): return open(p).read()
def wr(p,s): open(p,"w").write(s)
def req(src,old,new,label):
    if src.count(old)!=1: print("anchor not found once:",label); sys.exit(1)
    return src.replace(old,new)
E=rd("source/expand.h"); D=rd("docs/EP6_STEP2.md")
E=req(E,'_Static_assert(BD_OFF+4<=BM_OFF&&BM_OFF+4<=LG_OFF,"the base missions\' block sits between the board block and the life stats");',
 '_Static_assert(BD_OFF+4<=BM_OFF&&BM_OFF+4<=LG_OFF,"the base missions\' block sits between the board block and the life stats");\n#include "kingdone.h"   // EP6\'s own missions-done block (KD_OFF, 3 bytes after the game totals; 16 bits)\n_Static_assert(KD_OFF>=LG_OFF+3+4*LG_N&&KD_OFF+KD_LEN<=LS_OFF,"the EP6 done block sits in the gap between the game totals and the life stats");',"include")
E=req(E,'_Static_assert(60+XN(xmKing)<=64,"EP6 has only done bits 60 to 63: more than four missions need a bigger done block (EXD_LEN, docs/EP6_STEP1.md)");',
 '_Static_assert(XN(xmKing)<=KD_MAX,"EP6 has only sixteen done bits (kingdone.h): more missions need a bigger block");',"assert")
E=req(E,"static int exDone(int p,int m){ return p==XP_BASE?(bmDoneBits()>>m)&1:exDoneBit(xpBase[p]+m); }",
 "static int exDone(int p,int m){ if(p==XP_BASE) return (bmDoneBits()>>m)&1; if(p==XP_KING) return kdGet(m)||(m<4&&exDoneBit(xpBase[p]+m)); return exDoneBit(xpBase[p]+m); }   // (EP6: its own block, and the old shared bits 60..63 still count)","exDone")
E=req(E,"if(p==XP_BASE) bmDoneSet(exM); else exDoneSet(xpBase[p]+exM);","if(p==XP_BASE) bmDoneSet(exM); else if(p==XP_KING) kdSet(exM); else exDoneSet(xpBase[p]+exM);","exComplete")
E=req(E,"// (The done bits hold 64: the five older packs use 0 to 59, this one 60 to 63. See the _Static_assert below and docs/EP6_STEP1.md.)",
 "// (The shared done bits hold 64: the five older packs use 0 to 59. EP6 keeps its own block now (kingdone.h, sixteen missions); its old shared bits 60 to 63 still count as done.)","comment")
wr("source/expand.h",E)
D=D.rstrip("\n")+"""

---
# Bit e (second half, part 2): EP6's own missions-done block (step 2g)

`source/kingdone.h`: the shared done block (`EXD_OFF`, 8 bits of 64 per pack slot range) was full, and the one spare byte before the arcade record meant a block had to move. Nothing moves instead: EP6 gets its OWN 3 bytes in the free gap at SRAM 7607..7609 (after the game totals, which end at 7606, before the life stats at 7610; an assert in expand.h holds both ends). Layout: bits 0..7, bits 8..15, a check byte (`0x4B ^ lo ^ 3*hi`, so a blank 0x00 or an erased 0xFF block reads as invalid, i.e. nothing done). That is room for 16 missions (`KD_MAX`); the old compile-time stop at 4 missions is now a stop at 16.
- Compatibility: no existing block moved or changed size, so every old save loads as before. The three placeholder missions already done sit in the old shared bits 60..62; `exDone` for EP6 reads the new block OR those four old bits, so nothing earned is lost. New completions go to the new block only.
- A damaged block reads as nothing done and is rebuilt from zero by the next completion (the old shared bits still show for missions 1 to 4).
- Why not the kingdom's own save: the done bits are per cart, like the other packs'; a realm save is deleted with its realm, which would un-finish the campaign.
- Test: `sh tools/test_kingdone.sh` (blank, erased, set one by one, all 16, every single-bit and every single-byte damage is refused, a set keeps the others, the old bits 60..63 still count).
- Not run on a cart; host parse baseline unchanged by this step (14 errors all `target("arm")`, 131 warnings), but re-check it on your machine.

Phase 2 is now finished apart from a devkitARM build and an emulator look. Next is Phase 0 (the drawing and hauling prototypes).
"""
wr("docs/EP6_STEP2.md",D)
EOF
[ $? -eq 0 ] || exit 1
cat > source/kingdone.h <<'EOF'
// kingdone.h - EP6's own "missions done" block. The shared block (EXD_OFF, expand.h) holds 64 bits and the older packs use 0 to 59, so EP6
// keeps its own: 3 bytes in SRAM after the game totals (LG_OFF block ends at 7606) and before the life stats (LS_OFF 7610).
//   KD_OFF+0 done bits 0..7   KD_OFF+1 done bits 8..15   KD_OFF+2 check byte: 0x4B ^ lo ^ (3*hi)   (blank 0x00 and erased 0xFF read as invalid)
// A damaged block reads as nothing done; the next kdSet starts it again. Needs before it: u8, SRAM_BASE.
#define KD_OFF 7607
#define KD_LEN 3
#define KD_MAX 16
static u8 kdSumOf(u8 lo,u8 hi){ return (u8)(0x4B^lo^(u8)(hi*3)); }
static int kdBitsOf(volatile const u8*m){ if(m[2]!=kdSumOf(m[0],m[1])) return 0; return m[0]|(m[1]<<8); }
static int kdGet(int b){ if(b<0||b>=KD_MAX) return 0; return (kdBitsOf(SRAM_BASE+KD_OFF)>>b)&1; }
static void kdSet(int b){ if(b<0||b>=KD_MAX) return; volatile u8*m=SRAM_BASE+KD_OFF; int v=kdBitsOf(m)|(1<<b);
    u8 lo=(u8)v, hi=(u8)(v>>8); m[0]=lo; m[1]=hi; m[2]=kdSumOf(lo,hi); }
EOF
cat > tools/test_kingdone.c <<'EOF'
// test_kingdone.c - EP6's own done-bits block (source/kingdone.h) on a fake SRAM. Run: sh tools/test_kingdone.sh
#include <stdio.h>
#include <string.h>
typedef unsigned char u8;
static u8 fake[9000]; static u8*const SRAM_BASE_P=fake;
#define SRAM_BASE SRAM_BASE_P
#include "../source/kingdone.h"
static int fails;
#define CHK(c,msg) do{ if(!(c)){ printf("FAIL: %s\n",msg); fails++; } }while(0)
int main(void){
    memset(fake,0x00,sizeof fake); for(int b=0;b<16;b++) CHK(!kdGet(b),"blank block reads nothing done");
    memset(fake,0xFF,sizeof fake); for(int b=0;b<16;b++) CHK(!kdGet(b),"erased block reads nothing done");
    memset(fake,0,sizeof fake);
    for(int b=0;b<16;b++){ kdSet(b); for(int c=0;c<16;c++) CHK(kdGet(c)==(c<=b),"bits set one by one keep the others"); }
    CHK(kdGet(-1)==0&&kdGet(16)==0,"out of range reads 0"); kdSet(16); kdSet(-1); for(int c=0;c<16;c++) CHK(kdGet(c),"out of range sets change nothing");
    u8 good[3]; memcpy(good,fake+KD_OFF,3);
    for(int i=0;i<3;i++) for(int bit=0;bit<8;bit++){ memcpy(fake+KD_OFF,good,3); fake[KD_OFF+i]^=(u8)(1<<bit);
        int any=0; for(int c=0;c<16;c++) any|=kdGet(c); CHK(!any,"every single bit of damage is refused"); }
    for(int i=0;i<3;i++) for(int v=0;v<256;v++){ memcpy(fake+KD_OFF,good,3); if(v==good[i]) continue; fake[KD_OFF+i]=(u8)v;
        int any=0; for(int c=0;c<16;c++) any|=kdGet(c); CHK(!any,"every damaged byte value is refused"); }
    memcpy(fake+KD_OFF,good,3); fake[KD_OFF+1]=0x12; kdSet(3); CHK(kdGet(3)&&!kdGet(0),"a damaged block starts again from nothing");
    memset(fake,0,sizeof fake); kdSet(0); kdSet(9); CHK(kdGet(0)&&kdGet(9)&&!kdGet(1)&&!kdGet(8)&&!kdGet(10),"low and high byte bits");
    // nothing outside the block is touched
    memset(fake,0xA5,sizeof fake); memset(fake+KD_OFF,0,3); kdSet(5); kdSet(12);
    for(int i=0;i<(int)sizeof fake;i++) if(i<KD_OFF||i>=KD_OFF+KD_LEN) CHK(fake[i]==0xA5,"no byte outside the block changes");
    if(fails){ printf("%d FAILED\n",fails); return 1; } printf("test_kingdone: all checks passed\n"); return 0;
}
EOF
cat > tools/test_kingdone.sh <<'EOF'
#!/bin/sh
# test_kingdone.sh - PC test of EP6's done-bits block. Run from the repo root:   sh tools/test_kingdone.sh
set -e
gcc -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wall -o /tmp/tkd tools/test_kingdone.c
/tmp/tkd
EOF
sh tools/test_kingdone.sh || { echo "the PC test failed: nothing is wrong with your repo, but check the output above"; exit 1; }
echo "EP6 step 2g applied: EP6's own done bits (16 missions). Now commit it:  git add -A && git commit -m 'EP6 step 2g: own done-bits block for EP6'"
