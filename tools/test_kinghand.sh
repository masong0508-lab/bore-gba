#!/bin/sh
# test_kinghand.sh - PC test of the hardware tile background logic (source/kinghand.h). From the repo root:   sh tools/test_kinghand.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkh" tools/test_kinghand.c
"$T/tkh"
