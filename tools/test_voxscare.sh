#!/bin/sh
# test_voxscare.sh - PC test of the ghost scare voices (source/voxscare.h). From the repo root:   sh tools/test_voxscare.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-unused-variable -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tvs" tools/test_voxscare.c
"$T/tvs"
