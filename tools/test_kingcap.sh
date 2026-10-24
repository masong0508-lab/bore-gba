#!/bin/sh
# test_kingcap.sh - PC test of endless play (step 5f): 24,000 day soaks and every counter at its limit. From the repo root:   sh tools/test_kingcap.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkcap" tools/test_kingcap.c
"$T/tkcap"
