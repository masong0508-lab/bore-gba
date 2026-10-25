#!/bin/sh
# test_bortua3d.sh - PC test of BORTUA FIGHTER's 3D mode core (source/bortua3dcore.h). From the repo root:   sh tools/test_bortua3d.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
sed -n '/^static const short sinQ/,/^static inline int fcos/p' source/feel.h > "$T/fsin.h"
cc -O1 -g -Wall -Wno-unused-variable -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -I"$T" -o "$T/tbz" tools/test_bortua3d.c
"$T/tbz"
