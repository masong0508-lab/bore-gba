#!/bin/sh
# bss2ewram.sh OBJDUMP OBJCOPY file.o  -  moves the zeroed statics that no IWRAM code touches from IWRAM (.bss) to EWRAM (.sbss), in place.
#
# devkitARM puts every plain global and static into IWRAM, the GBA's 32 KB of fast RAM, which the fast code (the sound mixer, the
# interrupt, the renderer's inner loops: everything IWRAM_CODE / IWRAM_ARM) also has to fit in. Most of those variables are bookkeeping that
# code running from ROM reads a few times a frame, where EWRAM is just as good. The rule, applied to the compiled object (built with
# -fdata-sections, so each variable is a section of its own):
#   a .bss.* section that is referenced from an .iwram* (code) section stays in IWRAM; every other .bss.* section becomes .sbss.* (EWRAM).
# The startup code zeroes .sbss as it does .bss, so nothing else changes. A variable that must stay in IWRAM although only ROM code uses it
# can be put in section ".bss.iw" (IWRAM_BSS in main.c).
OD=$1; OC=$2; O=$3
# symbol -> its section (both for named variables and for section symbols)
$OD -t "$O" | awk 'NF>=6 && $(NF-2) ~ /^\.bss\./ { print $NF, $(NF-2) }' > "$O.syms"
# the .bss sections that IWRAM code refers to
$OD -r "$O" | awk '
  /^RELOCATION RECORDS FOR \[/ { s=$4; sub(/^\[/,"",s); sub(/\]:$/,"",s); hot=(s ~ /^\.iwram/ || s ~ /^\.rel\.iwram/); next }
  hot && NF>=3 { v=$3; sub(/[+-]0x[0-9a-fA-F]+$/,"",v); print v }' | sort -u > "$O.refs"
awk 'NR==FNR { sec[$1]=$2; next } { if ($1 ~ /^\.bss\./) print $1; else if ($1 in sec) print sec[$1] }' "$O.syms" "$O.refs" | sort -u > "$O.keep"
echo ".bss.iw" >> "$O.keep"
ARGS=$($OD -h "$O" | awk '$2 ~ /^\.bss\./ { print $2 }' | sort -u | awk 'NR==FNR { k[$1]=1; next } !($1 in k) { printf " --rename-section %s=.sbss%s", $1, substr($1,5) }' "$O.keep" -)
[ -n "$ARGS" ] && $OC $ARGS "$O"
rm -f "$O.syms" "$O.refs" "$O.keep"
