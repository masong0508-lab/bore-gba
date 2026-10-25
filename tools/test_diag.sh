#!/bin/sh
# test_diag.sh - PC test of the DIAGONALS option (8 WAY / 4 WAY, source/opts.h). From the repo root:   sh tools/test_diag.sh
set -e
T=${TMPDIR:-/tmp}
cc -O1 -g -Wall -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tdiag" tools/test_diag.c
"$T/tdiag"
