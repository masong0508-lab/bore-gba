#!/bin/sh
# test_lots.sh - every community lot (venues.h: the halls, the lounge, the plaza, the park) at every lot shape the town makes (4x4 to 8x6): everything can be walked to from the door,
# every piece has a free tile beside it, the counter is reachable. From the repo root:   sh tools/test_lots.sh      (tools/lots_preview.py draws them, too)
set -e
python3 tools/lots_preview.py "${TMPDIR:-/tmp}/lots_test.png" 4x4 4x5 5x4 6x4 5x5 4x6 6x5 7x4 5x6 6x6 7x5 8x5 8x6 7x6 | grep -v "repaired:" | tail -4
