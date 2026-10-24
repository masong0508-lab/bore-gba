#!/bin/sh
# test_kingpos.sh - PC test of the giant's saved position (step 4j: kingdom.h format 2, kinggiant.h kgPlace). From the repo root:   sh tools/test_kingpos.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkpos" tools/test_kingpos.c
"$T/tkpos"
