#!/bin/sh
# test_helptext.sh - every hand-wrapped help line fits its screen in the game's own font (tools/check_helptext.py).
cd "$(dirname "$0")/.." || exit 1
python3 tools/check_helptext.py
