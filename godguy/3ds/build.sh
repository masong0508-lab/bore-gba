#!/bin/bash
# builds godguy.3dsx with devkitARM + libctru:   DEVKITPRO=/opt/devkitpro 3ds/build.sh
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITARM=$DEVKITPRO/devkitARM; GCC=$DEVKITARM/bin/arm-none-eabi-gcc
# Direct local 3DS builds must generate the same cels as CI.
(cd .. && /usr/bin/python3 tools/make_story_hq.py)
# Keep legacy UI warning noise separate from memory safety: optimized
# out-of-bounds mesh loops must fail the native build, not ship a ROM.
CF="-O2 -mword-relocations -ffunction-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -DGG_NEW3DS_ONLY -D__3DS__ -DARM11 -DNDSSHIM -DSHIM_HQ -DGG_HQVOX -I$DEVKITPRO/libctru/include -I../compat -I../source -Wno-all -Werror=array-bounds -Werror=aggressive-loop-optimizations"
$GCC $CF -c main3ds.c -o main3ds.o
$GCC $CF -c platform3ds.c -o platform3ds.o
$GCC -specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft main3ds.o platform3ds.o -L$DEVKITPRO/libctru/lib -lctru -lm -o godguy3ds.elf
$DEVKITPRO/tools/bin/smdhtool --create "GodGuy New 3DS" "New-series only 3D" "Bore" $DEVKITPRO/libctru/default_icon.png godguy3ds.smdh 2>/dev/null || true
$DEVKITPRO/tools/bin/3dsxtool godguy3ds.elf ../godguy.3dsx --smdh=godguy3ds.smdh 2>/dev/null || $DEVKITPRO/tools/bin/3dsxtool godguy3ds.elf ../godguy.3dsx
echo "GodGuy NEW 3DS ONLY: native 400x240 mesh, adaptive world detail."
ls -la ../godguy.3dsx

# Keep experimental GPU builds separate from the playable main binary.
# The standalone script also runs independently in dedicated, fast CI.
if [ "${GG_BUILD_GPU_PROBE:-0}" = "1" ]; then
  bash ./build_gpu_probe.sh
fi
