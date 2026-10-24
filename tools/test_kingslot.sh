#!/bin/sh
# test_kingslot.sh - checks source/kingslot.h on a PC, with the REAL slot code of slots.h (header, scan, verify) cut out of it and run on a
# fake flash chip that can lose power at any write. Run from the repo root:   sh tools/test_kingslot.sh
set -e; export ASAN_OPTIONS=detect_leaks=0
S=source/slots.h; T=${TMPDIR:-/tmp}/ks_slots.inc
{
  grep '^#define SLO(s)' $S
  grep '^enum { SLK_ROOM' $S
  grep '^enum { SLE_OK' $S
  grep '^static int slSumOf' $S
  awk '/^typedef struct \{ u8 ok,kind,span/{p=1} /^\/\/ ---------- save \/ load \/ delete/{p=0} p' $S
  awk '/^static int slVerify\(int slot\)\{/{p=1} p{print} p&&/^\}/{exit}' $S
} > $T
gcc -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wno-unused-function -DSLOTS_INC="\"$T\"" -o ${TMPDIR:-/tmp}/tks tools/test_kingslot.c
${TMPDIR:-/tmp}/tks
