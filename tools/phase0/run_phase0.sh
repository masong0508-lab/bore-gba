#!/bin/sh
# run_phase0.sh - EP6 Phase 0 prototypes on the PC. From the repo root:   sh tools/phase0/run_phase0.sh   (Termux: TMPDIR is used)
set -e
T=${TMPDIR:-/tmp}; CC=${CC:-cc}; command -v $CC >/dev/null || CC=gcc
$CC -O2 -Wno-unused -Wno-misleading-indentation -o $T/ep6haul tools/phase0/haul_proto.c
$CC -O1 -o $T/ep6draw tools/phase0/draw_model.c -lm
echo "######## DRAWING MODEL"; $T/ep6draw
echo; echo "######## HAULING: the spec (10 buildings, 40 workers, 10,000 days, with a cut road, a destroyed building and a walled-in one)"; $T/ep6haul 10 40 10000
echo; echo "######## HAULING: stretch (24 buildings, 80 workers, 3,000 days)"; $T/ep6haul 24 80 3000
