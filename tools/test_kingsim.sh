#!/bin/sh
# test_kingsim.sh - PC test of the realm's economy and hauling. From the repo root:   sh tools/test_kingsim.sh          (about a minute)
#                                                                                      sh tools/test_kingsim.sh soak 10000   (10,000 days, longer)
# (Leak detection is off: LeakSanitizer crashes at exit on Termux/Android, and the test frees nothing it needs to.)
set -e
T=${TMPDIR:-/tmp}
export ASAN_OPTIONS=detect_leaks=0
cc -O1 -g -Wall -Wno-misleading-indentation -fsanitize=address,undefined -fno-sanitize-recover=undefined -o "$T/tks" tools/test_kingsim.c
"$T/tks" "$@"
