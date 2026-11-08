#!/bin/sh
# test_kingamb.sh - PC test of the seasons, the weather and the wildlife and the caravan (source/kingamb.h). From the repo root:   sh tools/test_kingamb.sh
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -Wno-unused-function -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tkamb" tools/test_kingamb.c
"$T/tkamb"
