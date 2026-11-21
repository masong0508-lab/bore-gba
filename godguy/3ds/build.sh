#!/bin/bash
# builds godguy.3dsx with devkitARM + libctru:   DEVKITPRO=/opt/devkitpro 3ds/build.sh
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITARM=$DEVKITPRO/devkitARM; GCC=$DEVKITARM/bin/arm-none-eabi-gcc
# Direct local 3DS builds must generate the same cels as CI.
(cd .. && /usr/bin/python3 tools/make_story_hq.py)
CF="-O2 -mword-relocations -ffunction-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -DGG_NEW3DS_ONLY -D__3DS__ -DARM11 -DNDSSHIM -DSHIM_HQ -DGG_HQVOX -I$DEVKITPRO/libctru/include -I../compat -I../source -Wno-all"
$GCC $CF -c main3ds.c -o main3ds.o
$GCC $CF -c platform3ds.c -o platform3ds.o
$GCC -specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft main3ds.o platform3ds.o -L$DEVKITPRO/libctru/lib -lctru -lm -o godguy3ds.elf
$DEVKITPRO/tools/bin/smdhtool --create "GodGuy New 3DS" "New-series only 3D" "Bore" $DEVKITPRO/libctru/default_icon.png godguy3ds.smdh 2>/dev/null || true
$DEVKITPRO/tools/bin/3dsxtool godguy3ds.elf ../godguy.3dsx --smdh=godguy3ds.smdh 2>/dev/null || $DEVKITPRO/tools/bin/3dsxtool godguy3ds.elf ../godguy.3dsx
echo "GodGuy NEW 3DS ONLY: native 400x240 mesh, adaptive world detail."
ls -la ../godguy.3dsx

# Experimental, SEPARATE GPU app. This cross-compiles the actual PICA200
# shader + Citro3D geometry/depth pipeline. It does NOT replace the
# playable software renderer or its single live-world save state.
if [ "${GG_BUILD_GPU_PROBE:-0}" = "1" ]; then
  command -v picasso >/dev/null 2>&1 || export PATH="$DEVKITPRO/tools/bin:$PATH"
  command -v picasso >/dev/null 2>&1 || { echo "Missing picasso PICA shader assembler" >&2; exit 1; }
  picasso -o gpu_world.shbin gpu_world.v.pica
  /usr/bin/python3 - <<'PY' > gpu_world_shader_data.h
import pathlib, struct
payload = pathlib.Path("gpu_world.shbin").read_bytes()
if not payload:
    raise SystemExit("PICA shader output is empty")
size = len(payload)
payload += b"\0" * (-len(payload) % 4)
words = struct.unpack("<%dI" % (len(payload) // 4), payload)
print("#ifndef GG_GPU_SHADER_DATA_H\n#define GG_GPU_SHADER_DATA_H")
print("#include <stdint.h>")
print("static const uint32_t ggGpuShader[] = {")
for i in range(0, len(words), 8):
    print("    " + ",".join("0x%08xu" % n for n in words[i:i+8]) + ",")
print("};")
print("static const unsigned int ggGpuShaderSize = %d;" % size)
print("#endif")
PY
  $GCC $CF -c gpu_world_probe3ds.c -o gpu_world_probe3ds.o
  $GCC -specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
    gpu_world_probe3ds.o -L$DEVKITPRO/libctru/lib \
    -lcitro3d -lctru -lm -o gpu_world_probe3ds.elf
  $DEVKITPRO/tools/bin/3dsxtool gpu_world_probe3ds.elf \
    ../godguy_new3ds_gpu_probe.3dsx
  test -s ../godguy_new3ds_gpu_probe.3dsx
  echo "Compiled separate New 3DS PICA200 world probe; main game unchanged."
fi
