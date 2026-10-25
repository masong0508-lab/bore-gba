#!/bin/sh
# test_kingadv.sh - PC test of the advisor (source/kingadv.h). From the repo root:   sh tools/test_kingadv.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkadv" tools/test_kingadv.c
"$T/tkadv"
