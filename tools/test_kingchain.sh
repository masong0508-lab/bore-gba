#!/bin/sh
# test_kingchain.sh - PC test of the wool and crystal chains (step 5a: kingsim.h's table, kinghand.h's modes, the roofs). From the repo root:   sh tools/test_kingchain.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkchain" tools/test_kingchain.c
"$T/tkchain"
