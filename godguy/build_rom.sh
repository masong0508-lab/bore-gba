#!/bin/bash
# builds godguy.nds in the layout DraStic and melonDS accept: ARM9 loaded at 0x02000000 with a 2 KB decrypted "secure area" first (E7FFDEFF filler) and the code entry at 0x02000800,
# the same as the BORE2 and wordPass ROMs.   DEVKITPRO=/opt/devkitpro ./build_rom.sh [extra make flags such as GG_FLAGS=-DGG_SAFE]
set -e; cd "$(dirname "$0")"; : ${DEVKITPRO:=/opt/devkitpro}; export DEVKITPRO DEVKITARM=$DEVKITPRO/devkitARM
sed "s#ORIGIN = 0x2000000, LENGTH = 0x380000#ORIGIN = 0x2000800, LENGTH = 0x37f800#" $DEVKITPRO/calico/lib/ds9-legacy.ld > ds9-800.ld
printf '%%include <sync-none.specs>\n\n*link:\n+ -dT %s/ds9-800.ld --gc-sections --nmagic --no-warn-rwx-segments --use-blx\n\n*startfile:\ncrti%%O%%s crtbegin%%O%%s --require-defined=main\n' "$PWD" > ds9-800.specs
make "$@" 2>&1 | grep -E "error|undefined" || true
$DEVKITARM/bin/arm-none-eabi-objcopy -O binary godguy.elf godguy9.body
/usr/bin/python3 - <<'PY'
import struct
body=open('godguy9.body','rb').read(); pre=struct.pack('<2I',0xE7FFDEFF,0xE7FFDEFF)*0x40   # 0x200 bytes of marker then zeros up to 0x800
pre=(pre+b'\0'*0x800)[:0x800]
open('godguy9.bin','wb').write(pre+body)
PY
$DEVKITPRO/tools/bin/ndstool -c godguy.nds -9 godguy9.bin -r9 0x2000000 -e9 0x2000800 -7 $DEVKITPRO/calico/bin/ds7_maine.elf -b icon.bmp "Bore: GodGuy;A god game;Bore spinoff" -g GGDE 01 GODGUY -h 0x4000 >/dev/null
rm -f godguy9.body godguy9.bin; $DEVKITPRO/tools/bin/ndstool -i godguy.nds | sed -n 4,16p
