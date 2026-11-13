#!/bin/bash
# builds godguy_dsi_mode.nds: the DSi build for DSi mode (unit code 3, 16 MB of RAM) with planets up to 768 x 768 and 40,000 creatures. The big arrays sit in their own linker section above 4 MB (hw/arm9dsi.ld); the
# start-up code checks that the RAM is really there and the game stops with a message if it is not (a DS-mode launch). EXPERIMENTAL: written without a DSi to try it on; godguy_dsi.nds is the safe file.
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITARM=$DEVKITPRO/devkitARM; B=$DEVKITARM/bin/arm-none-eabi-; mkdir -p build_dsibig; cd build_dsibig
FLAGS="-march=armv5te -mtune=arm946e-s -marm -mthumb-interwork -O2 -fno-common -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -DNDSHW -DGG_DSI -DGG_DSIBIG ${GG_NOCACHE:--DHW_CACHE} -I../compat -I../source -Wno-all ${GG_FLAGS}"
${B}gcc $FLAGS -c ../hw/main_hw.c -o main_hw.o; ${B}gcc $FLAGS -c ../hw/syscalls.c -o syscalls.o; ${B}gcc $FLAGS -x assembler-with-cpp -c ../hw/crt0.s -o crt0.o
${B}gcc -nostartfiles -march=armv5te -marm -mthumb-interwork -Wl,--gc-sections -Wl,-T,../hw/arm9dsi.ld crt0.o main_hw.o syscalls.o -lm -lc -lgcc -o arm9.elf
${B}objcopy -O binary arm9.elf arm9.body
${B}gcc -march=armv4t -mtune=arm7tdmi -marm -O2 -ffreestanding -fno-unwind-tables -Wno-all -c ../hw/arm7.c -o arm7.o; ${B}gcc -x assembler-with-cpp -march=armv4t -marm -c ../hw/arm7crt.s -o arm7crt.o
${B}gcc -nostartfiles -nostdlib -march=armv4t -marm -Wl,-T,../hw/arm7.ld arm7crt.o arm7.o -lgcc -o arm7.elf; ${B}objcopy -O binary arm7.elf arm7.bin
/usr/bin/python3 - <<'PY'
import struct
pre=(struct.pack('<2I',0xE7FFDEFF,0xE7FFDEFF)*0x40+b'\0'*0x800)[:0x800]
open('arm9.bin','wb').write(pre+open('arm9.body','rb').read())
PY
$DEVKITPRO/tools/bin/ndstool -c ../godguy_dsi_mode.nds -9 arm9.bin -r9 0x2000000 -e9 0x2000800 -7 arm7.bin -r7 0x2380000 -e7 0x2380000 -b ../icon.bmp "Bore: GodGuy;A god game;DSi mode build" -g GGDE 01 GODGUY -h 0x4000 >/dev/null
printf '\x03' | dd of=../godguy_dsi_mode.nds bs=1 seek=18 conv=notrunc 2>/dev/null; $DEVKITPRO/tools/bin/ndstool -f ../godguy_dsi_mode.nds >/dev/null
${B}size -A arm9.elf | grep -E "bss|text"; ls -la ../godguy_dsi_mode.nds
