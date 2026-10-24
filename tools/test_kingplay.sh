#!/bin/sh
# test_kingplay.sh - the realm played in hhPool on the REAL flash code (the fake chip of test_kingflash.c). Run from the repo root:   sh tools/test_kingplay.sh
# the chip) and svInit, which test_kingplay.c replaces with a fake 128 KB chip that can lose power at any program or erase. slots.h's real header, scan
# and verify code run on top of it, then kingslot.h. Run from the repo root:   sh tools/test_kingplay.sh
set -e
TD=${TMPDIR:-/tmp}; export ASAN_OPTIONS=detect_leaks=0
S=source/slots.h; V=source/save.h; T=$TD/kp_slots.inc; W=$TD/kp_save.inc
awk '/^static void svInit\(void\)\{/{exit} {print}' $V | sed -e '/^IWRAM_THUMB static/d' -e '/^static void svWaitFrames/d' \
  -e 's|#define SVB ((volatile u8\*)0x0E000000)|#define SVB (fkWin())|' -e 's|static volatile u8\* svBase=(volatile u8\*)0x0E000000;|static volatile u8* svBase=0;|' > $W
{
  grep '^#define SLO(s)' $S
  grep '^#define SLB(s)' $S
  grep '^enum { SLK_ROOM' $S
  grep '^enum { SLE_OK' $S
  grep '^static int slSumOf' $S
  awk '/^typedef struct \{ u8 ok,kind,span/{p=1} /^\/\/ ---------- save \/ load \/ delete/{p=0} p' $S
  awk '/^static int slVerify\(int slot\)\{/{p=1} p{print} p&&/^\}/{exit}' $S
} > $T
cc -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wno-unused-function -Wno-misleading-indentation -DSLOTS_INC="\"$T\"" -DSAVE_INC="\"$W\"" -o $TD/tkp tools/test_kingplay.c
$TD/tkp
