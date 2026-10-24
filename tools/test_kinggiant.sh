#!/bin/sh
# test_kinggiant.sh - PC test of the walking giant (source/kinggiant.h). From the repo root:   sh tools/test_kinggiant.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkg" tools/test_kinggiant.c
"$T/tkg"
