#!/bin/sh
# test_kingview.sh - PC test of the hardware tile background logic (source/kingview.h). From the repo root:   sh tools/test_kingview.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkv" tools/test_kingview.c
"$T/tkv"
