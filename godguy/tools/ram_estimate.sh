#!/bin/bash
# DS ARM9 RAM estimate using the actual hardware compatibility header.
# The host compiler is only a layout proxy; build_hw.sh + hw/arm9.ld perform
# the authoritative ARM link and enforce the 0x02300000 save-buffer boundary.
set -eu
cd "$(dirname "$0")/.."
cap(){ printf '#include "source/gg.h"\nint maxc_is = MAXC;\n' | gcc -E -P -DPCTEST "$1" -I. - | sed -n 's/.*maxc_is = \([0-9]*\);.*/\1/p'; }
DS=$(cap -DNDSHW)
DSI=$(cap -DGG_DSI16)
BIG=$(cap -DGG_BIG)
echo "caps in gg.h: DS $DS  DSi $DSI  3DS/PC $BIG"
[ "$DS" = 12000 ] && [ "$DSI" = 38000 ] && [ "$BIG" = 199999 ] || { echo "unexpected platform cap" >&2; exit 1; }
# This compiles the full ARM9 translation unit with ndshw.h, including its
# 700 KiB save staging buffer. Do not add SVMAX again: it is already in .bss.
gcc -O2 -DNDSHW -DHW_CACHE -Icompat -Isource -Wno-all -c hw/main_hw.c -o /tmp/gg_ram_ds_hw.o 2>/dev/null
used=$(size /tmp/gg_ram_ds_hw.o | awk 'NR==2{print $1+$2+$3}')
limit=$((0x02300000-0x02000800))
spare=$((limit-used))
echo "DS host-layout proxy: $used / $limit bytes (code + data + BSS, raw save buffer included); estimated spare $spare bytes"
if [ "$spare" -lt 0 ]; then echo "DS: proxy exceeds the linker window; the ARM cross-link must be checked" >&2; exit 1; fi
# DSi 16 MB build: its code/rodata live in the low block; static arrays and
# raw save buffer live above 0x02400000, so compare BSS with the big-block cap.
gcc -O2 -DNDSHW -DGG_DSI16 -DHW_CACHE -Icompat -Isource -Wno-all -c hw/main_hw.c -o /tmp/gg_ram_dsi_hw.o 2>/dev/null
dsi_bss=$(size /tmp/gg_ram_dsi_hw.o | awk 'NR==2{print $3}')
dsi_limit=$((0x7F0000-0x10000))
dsi_spare=$((dsi_limit-dsi_bss))
echo "DSi16 host-layout proxy: BSS $dsi_bss / $dsi_limit bytes (raw save included); estimated spare $dsi_spare bytes"
if [ "$dsi_spare" -lt 0 ]; then echo "DSi16: proxy exceeds the big-memory block; the ARM cross-link must be checked" >&2; exit 1; fi
echo "DS final check: run ./build_hw.sh; hw/arm9.ld rejects any real ARM9 overflow."
