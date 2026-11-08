#!/bin/bash
# builds godguy.3dsx with devkitARM + libctru:   DEVKITPRO=/opt/devkitpro 3ds/build.sh
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITARM=$DEVKITPRO/devkitARM; GCC=$DEVKITARM/bin/arm-none-eabi-gcc
CF="-O2 -mword-relocations -ffunction-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -D__3DS__ -DARM11 -DNDSSHIM -I$DEVKITPRO/libctru/include -I../compat -I../source -Wno-all"
$GCC $CF -c main3ds.c -o main3ds.o
$GCC $CF -c platform3ds.c -o platform3ds.o
$GCC -specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft main3ds.o platform3ds.o -L$DEVKITPRO/libctru/lib -lctru -lm -o godguy3ds.elf
$DEVKITPRO/tools/bin/smdhtool --create "Bore: GodGuy" "A god game" "Bore" $DEVKITPRO/libctru/default_icon.png godguy3ds.smdh 2>/dev/null || true
$DEVKITPRO/tools/bin/3dsxtool godguy3ds.elf ../godguy.3dsx --smdh=godguy3ds.smdh 2>/dev/null || $DEVKITPRO/tools/bin/3dsxtool godguy3ds.elf ../godguy.3dsx
ls -la ../godguy.3dsx
