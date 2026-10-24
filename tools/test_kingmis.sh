#!/bin/sh
# test_kingmis.sh - PC test of the ten missions (step 5c: kingmis.h, kingq.h, kingtask.h). From the repo root:   sh tools/test_kingmis.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkmis" tools/test_kingmis.c
"$T/tkmis"
