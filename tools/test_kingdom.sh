#!/bin/sh
# test_kingdom.sh - PC test of the realm and its save format (source/kingdom.h). From the repo root:   sh tools/test_kingdom.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkdom" tools/test_kingdom.c
"$T/tkdom" | tail -1
