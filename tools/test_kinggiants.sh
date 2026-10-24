#!/bin/sh
# test_kinggiants.sh - PC test of the four giants' stats and the builder's rebate (step 5e). From the repo root:   sh tools/test_kinggiants.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkgiants" tools/test_kinggiants.c
"$T/tkgiants"
