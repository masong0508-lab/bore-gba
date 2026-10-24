#!/bin/sh
# test_kingover.sh - PC test of the overview (source/kingover.h, step 4i). From the repo root:   sh tools/test_kingover.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkover" tools/test_kingover.c
"$T/tkover"
