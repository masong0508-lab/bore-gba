#!/bin/sh
# test_kingdone.sh - PC test of EP6's done-bits block. Run from the repo root:   sh tools/test_kingdone.sh
set -e
cc -O1 -g -Wall -o $TMPDIR/tkd tools/test_kingdone.c
$TMPDIR/tkd
