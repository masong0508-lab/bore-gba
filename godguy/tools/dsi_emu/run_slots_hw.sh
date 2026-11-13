#!/bin/bash
# the DS save-slot code on the real ARM9 and ARM7 images against fake flash / EEPROM chips of several sizes (needs devkitARM and `pip install unicorn`; about half a minute)
set -e; cd "$(dirname "$0")/../.."; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITPRO
GG_FLAGS="-fkeep-static-functions -fno-inline-functions-called-once -fno-inline-small-functions" ./build_hw.sh >/dev/null 2>&1
T=${TMPDIR:-/tmp}/hwtest; mkdir -p $T; cp build_hw/arm9.bin build_hw/arm9.elf build_hw/arm7.bin $T/; $DEVKITPRO/devkitARM/bin/arm-none-eabi-nm $T/arm9.elf > $T/syms.txt
${PYTHON:-python3} tools/dsi_emu/slots_hw.py $T | tail -60
./build_hw.sh >/dev/null 2>&1     # put the normal build back
