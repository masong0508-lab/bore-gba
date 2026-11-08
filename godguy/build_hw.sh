#!/bin/bash
# builds godguy_hw.nds with NO libnds and NO calico: our own ARM9 start-up, our own ARM7, direct hardware access.   DEVKITPRO=/opt/devkitpro ./build_hw.sh
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITARM=$DEVKITPRO/devkitARM; B=$DEVKITARM/bin/arm-none-eabi-; mkdir -p build_hw; cd build_hw
FLAGS="-march=armv5te -mtune=arm946e-s -marm -mthumb-interwork -O2 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -DNDSHW -I../compat -I../source -Wno-all ${GG_FLAGS}"
${B}gcc $FLAGS -c ../hw/main_hw.c -o main_hw.o; ${B}gcc $FLAGS -c ../hw/syscalls.c -o syscalls.o; ${B}gcc $FLAGS -x assembler-with-cpp -c ../hw/crt0.s -o crt0.o
${B}gcc -nostartfiles -march=armv5te -marm -mthumb-interwork -Wl,--gc-sections -Wl,-T,../hw/arm9.ld crt0.o main_hw.o syscalls.o -lm -lc -lgcc -o arm9.elf
${B}objcopy -O binary arm9.elf arm9.body
${B}gcc -march=armv4t -mtune=arm7tdmi -marm -O2 -ffreestanding -fno-unwind-tables -Wno-all -c ../hw/arm7.c -o arm7.o; ${B}gcc -x assembler-with-cpp -march=armv4t -marm -c ../hw/arm7crt.s -o arm7crt.o
${B}gcc -nostartfiles -nostdlib -march=armv4t -marm -Wl,-T,../hw/arm7.ld arm7crt.o arm7.o -lgcc -o arm7.elf; ${B}objcopy -O binary arm7.elf arm7.bin
/usr/bin/python3 - <<'PY'
import struct
pre=(struct.pack('<2I',0xE7FFDEFF,0xE7FFDEFF)*0x40+b'\0'*0x800)[:0x800]
open('arm9.bin','wb').write(pre+open('arm9.body','rb').read())
PY
$DEVKITPRO/tools/bin/ndstool -c ../godguy_hw.nds -9 arm9.bin -r9 0x2000000 -e9 0x2000800 -7 arm7.bin -r7 0x2380000 -e7 0x2380000 -b ../icon.bmp "Bore: GodGuy;A god game;no libnds build" -g GGDE 01 GODGUY -h 0x4000 >/dev/null
${B}size arm9.elf | tail -1; ls -la ../godguy_hw.nds
