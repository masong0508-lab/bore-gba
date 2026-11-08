#!/bin/bash
# builds every target from the one shared source (source/main.c): DS, DS safe (no sound), DSi, 3DS; then runs the tests.   DEVKITPRO=/opt/devkitpro ./build_all.sh
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITPRO
./build_hw.sh >/dev/null; GG_FLAGS=-DGG_SAFE ./build_hw.sh >/dev/null; mv godguy_hw.nds godguy_hw_safe.nds; ./build_hw.sh >/dev/null
./build_dsi.sh >/dev/null
[ -x 3ds/build.sh ] && (3ds/build.sh >/dev/null 2>&1 && echo "3ds ok" || echo "3ds: build skipped/failed (needs libctru)")
tools/run_tests.sh
ls -la godguy_hw.nds godguy_hw_safe.nds godguy_dsi.nds godguy_dsi_mode.nds godguy.3dsx 2>/dev/null
