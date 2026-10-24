#!/bin/sh
# test_kingcastle.sh - PC test of the castle (step 5b: kingsim.h's castle type and its four stages, the hand's and the text layer's rules). From the repo root:   sh tools/test_kingcastle.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkcastle" tools/test_kingcastle.c
"$T/tkcastle"
