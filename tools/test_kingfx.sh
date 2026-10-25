#!/bin/sh
# test_kingfx.sh - PC test of the life on the map (source/kingfx.h). From the repo root:   sh tools/test_kingfx.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkfx" tools/test_kingfx.c
"$T/tkfx"
