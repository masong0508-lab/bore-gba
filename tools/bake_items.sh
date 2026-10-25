#!/bin/sh
# Regenerates source/itemrom.h (all item sprites, stored in ROM as opaque spans). Run after changing item art (source/itembake.h). Needs a C compiler (gcc / clang / cc; Termux: pkg install clang).
cd "$(dirname "$0")" || exit 1
CC=${CC:-$(command -v cc || command -v gcc || command -v clang)}
[ -n "$CC" ] || { echo "no C compiler found (Termux: pkg install clang)"; exit 1; }
T=${TMPDIR:-/tmp}
$CC -O1 -o "$T/bake_items" bake_items.c && "$T/bake_items" ../source/itemrom.h && echo "wrote source/itemrom.h"
