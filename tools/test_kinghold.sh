#!/bin/sh
# test_kinghold.sh - PC test of pick up and put down (source/kinghold.h, step 4h). From the repo root:   sh tools/test_kinghold.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkhold" tools/test_kinghold.c
"$T/tkhold"
