#!/bin/sh
# budget.sh ELF - the three budgets of the build (docs/SIZE_ROADMAP.md, section 4), printed after every link, so they cannot creep up unseen:
#   EWRAM statics (the top 8 KB is the stack), IWRAM (.bss + .data + .iwram + the biggest overlay), and the BASE GAME's ROM (everything before the first expansion pack's slot).
# A NOTE line appears above 92% (EWRAM, IWRAM) or 95% (ROM). The hard limits stay in the Makefile and in tools/packs.ld; the ROM one fails here: an oversized song or picture stops the build with
# a message about the base game, not a confusing slot overlap. Needs arm-none-eabi-size and arm-none-eabi-objdump on the PATH.
ELF=${1:-bore.elf}
EW_MAX=$((262144-8192)); IW_MAX=${IWRAM_MAX:-32512}
SLOT0=$(awk '/XP_AT0/{gsub("u","",$3); print $3; exit}' source/packslots.h 2>/dev/null)
arm-none-eabi-size -A "$ELF" | awk -v ew="$EW_MAX" -v iw="$IW_MAX" '
  $1==".sbss"||$1==".ewram"{e+=$2}
  $1==".bss"||$1==".data"||$1==".iwram"{i+=$2}
  $1~/^\.iwram[0-9]$/{ if($2>o) o=$2 }
  END{ i+=o;
    printf("BUDGET EWRAM %d of %d B (%.1f%%, %d B free)\n",e,ew,100*e/ew,ew-e); if(e*100>ew*92) printf("NOTE: EWRAM is over 92%% (%d B left): before adding a static array, see docs/RAM_PLAN.md (ROM first, a pool second, EWRAM last)\n",ew-e);
    printf("BUDGET IWRAM %d of %d B (%.1f%%, %d B free)\n",i,iw,100*i/iw,iw-i); if(i*100>iw*92) printf("NOTE: IWRAM is over 92%% (%d B left): evict code to an overlay or to ROM, do not grow it\n",iw-i) }'
if [ -n "$SLOT0" ]; then
  arm-none-eabi-objdump -h "$ELF" | awk -v slot="$SLOT0" '
    function hex(s,  i,c,v){ v=0; s=tolower(s); for(i=1;i<=length(s);i++){ c=index("0123456789abcdef",substr(s,i,1))-1; v=v*16+c } return v }
    /^ *[0-9]+ /{ n=$2; sz=hex($3); va=hex($4); if(n ~ /^\.xp[0-9]/||n ~ /^\.debug/||n=="\.comment") next; if(va>=134217728&&va<167772160){ end=va+sz; if(end>top) top=end } }
    END{ base=top-134217728; room=slot-134217728; s=sprintf("%d",base);
      printf("BUDGET ROM   base game %d of %d B (%.1f%%, %d B free before the first pack slot at %#x)\n",base,room,100*base/room,room-base,slot);
      if(base>room){ printf("ERROR: the base game (%d B) runs into the expansion packs'"'"' slots (they start at %d B). Cut ROM data (see docs/SIZE_ROADMAP.md) or move the slots in tools/make_packs.py.\n",base,room); exit 1 }
      if(base*100>room*95) printf("NOTE: the base game is over 95%% of its ROM (%d B left): a big song or picture will not fit\n",room-base) }'
fi
