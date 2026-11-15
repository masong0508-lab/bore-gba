#!/bin/bash
# Estimates (PC symbol sizes, not a real ARM link) of how much room the DS and DSi builds have for the creature caps in source/gg.h.
# It adds up the game's own static arrays in a PC build with the same limits, adds the raw save buffer the hardware build keeps, and compares the total
# with the room the linker scripts give (hw/arm9.ld, hw/arm9dsi.ld). The real answer is build_hw.sh / build_dsi.sh: the linker stops if the data does not fit.
cd "$(dirname "$0")/.." || exit 1
G="-O2 -DNDSSHIM -Icompat -Isource -w -include unistd.h"
bss(){ nm -S -t d "$1" | awk '$3 ~ /^[bB]$/ && $4 !~ /^(shim|BG_PAL|SPRITE_PAL|__)/ {s+=$2} END{print s+0}'; }
# the caps the hardware builds really get: the preprocessor on gg.h with the hardware defines
cap(){ printf '#include "source/gg.h"\nint maxc_is = MAXC;\n' | gcc -E -P -DPCTEST $1 -I. - | sed -n 's/.*maxc_is = \([0-9]*\);.*/\1/p'; }
DS=$(cap -DNDSHW); DSI=$(cap -DGG_DSI16); BIG=$(cap -DGG_BIG)
echo "caps in gg.h: DS $DS  DSi $DSI  3DS/PC $BIG"
[ "$DS" = 19999 ] && [ "$DSI" = 38000 ] && [ "$BIG" = 199999 ] || echo "NOTE: not the 19,999 / 38,000 / 199,999 of the cap raise"
gcc $G -DGG_MAXC_TEST=$DS -o /tmp/gg_ram_ds pc_cap.c -lm 2>/dev/null; gcc $G -DGG_DSI16 -o /tmp/gg_ram_dsi pc_cap.c -lm 2>/dev/null
d=$(bss /tmp/gg_ram_ds); t=$(size /tmp/gg_ram_ds | awk 'NR==2{print $1+$2}'); sv=$((700*1024)); lim=$((0x02300000-0x02000800))
echo "DS : game data $d + code and read-only data about $t + raw save buffer $sv = $((d+t+sv)) of $lim bytes -> spare about $((lim-d-t-sv)) (the code figure is the PC's; ARM code differs)"
d=$(bss /tmp/gg_ram_dsi); sv=$((3*1024*1024)); lim=$((0x7F0000-0x10000))
echo "DSi: game data $d + raw save buffer $sv = $((d+sv)) of $lim bytes -> spare about $((lim-d-sv))"
if [ $((lim-d-sv)) -lt 0 ]; then echo "DSi: OVER by about $((d+sv-lim)) bytes (an estimate): expect the link to stop; about $(( (d+sv-lim)/29+1 )) creatures fewer would fit, so try MAXC $((DSI-(d+sv-lim)/29-1000)) or lower in the GG_DSI16 line of source/gg.h"; fi
