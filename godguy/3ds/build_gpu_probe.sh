#!/bin/bash
# Standalone PICA200 proof-of-geometry: no story assets or game compile.
# Produces godguy_new3ds_gpu_probe.3dsx without touching godguy.3dsx.
set -euo pipefail
cd "$(dirname "$0")"
: "${DEVKITPRO:=/opt/devkitpro}"
DEVKITARM="$DEVKITPRO/devkitARM"
GCC="$DEVKITARM/bin/arm-none-eabi-gcc"
if ! command -v picasso >/dev/null 2>&1; then
  export PATH="$DEVKITPRO/tools/bin:$PATH"
fi
command -v picasso >/dev/null 2>&1 || {
  echo "Missing PICA assembler: install 3ds-dev/picasso" >&2
  exit 1
}
test -x "$GCC" || { echo "Missing devkitARM compiler" >&2; exit 1; }
picasso -o gpu_world.shbin gpu_world.v.pica
/usr/bin/python3 - <<'PY' > gpu_world_shader_data.h
import pathlib, struct
data = pathlib.Path("gpu_world.shbin").read_bytes()
if not data:
    raise SystemExit("Empty assembled PICA shader")
original_size = len(data)
data += b"\0" * (-len(data) % 4)
words = struct.unpack("<%dI" % (len(data)//4), data)
print("#ifndef GG_GPU_SHADER_DATA_H\n#define GG_GPU_SHADER_DATA_H")
print("#include <stdint.h>")
print("static const uint32_t ggGpuShader[] = {")
for i in range(0, len(words), 8):
    print("    " + ",".join("0x%08xu" % w for w in words[i:i+8]) + ",")
print("};")
print("static const unsigned int ggGpuShaderSize = %d;" % original_size)
print("#endif")
PY
CF="-O2 -mword-relocations -ffunction-sections -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft -DGG_NEW3DS_ONLY -D__3DS__ -DARM11 -I$DEVKITPRO/libctru/include -I../compat -I../source -Wno-all"
"$GCC" $CF -c gpu_world_probe3ds.c -o gpu_world_probe3ds.o
"$GCC" -specs=3dsx.specs -g -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft \
  gpu_world_probe3ds.o -L"$DEVKITPRO/libctru/lib" \
  -lcitro3d -lctru -lm -o gpu_world_probe3ds.elf
"$DEVKITPRO/tools/bin/3dsxtool" gpu_world_probe3ds.elf \
  ../godguy_new3ds_gpu_probe.3dsx
test -s ../godguy_new3ds_gpu_probe.3dsx
echo "PICA200 / Citro3D probe compiled:"
ls -l ../godguy_new3ds_gpu_probe.3dsx
