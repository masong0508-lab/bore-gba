#!/bin/sh
# test_kingmem.sh - PC test of the realm laid out inside the 30,720 byte hhPool. From the repo root:   sh tools/test_kingmem.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkm" tools/test_kingmem.c
"$T/tkm"
