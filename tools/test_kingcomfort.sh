#!/bin/sh
# test_kingcomfort.sh - PC test of step 5g: the well and the tavern (kingsim.h's comfort, kinghand.h's modes, the roofs). From the repo root:   sh tools/test_kingcomfort.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkcomfort" tools/test_kingcomfort.c
"$T/tkcomfort"
