#!/bin/bash
# GodGuy's PC tests (the DS build cannot be run in any emulator available here, so the shared game and sound code is tested on the PC)
cd "$(dirname "$0")/.." || exit 1
F="-O2 -DPCTEST -Wno-misleading-indentation -Wno-unused-variable -Wno-unused-function -Wno-parentheses"
set -e
gcc $F -o /tmp/gg_pc pc.c -lm && /tmp/gg_pc 7 3 | tail -3
gcc $F -o /tmp/gg_3d pc3d.c -lm && /tmp/gg_3d | head -1
gcc $F -o /tmp/gg_pick pick_test.c -lm && /tmp/gg_pick | tail -3
gcc $F -o /tmp/gg_save pc_save.c -lm && /tmp/gg_save
gcc $F -o /tmp/gg_story pc_story.c -lm && for sz in 128 192 256; do /tmp/gg_story 7 24 $sz | grep -E "planet|END"; done
tools/gm_check.sh /tmp/gm_check 100
# the real DS program, run headless through compat/ndsshim.h with scripted keys and stylus (screenshots in /tmp/gg_ds)
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_ds_run pc_ds.c -lm && rm -rf /tmp/gg_ds && /tmp/gg_ds_run /tmp/gg_ds | head -2
