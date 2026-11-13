#!/bin/bash
# GodGuy's PC tests (the DS build cannot be run in any emulator available here, so the shared game and sound code is tested on the PC)
cd "$(dirname "$0")/.." || exit 1
F="-O2 -DPCTEST -Wno-misleading-indentation -Wno-unused-variable -Wno-unused-function -Wno-parentheses"
set -e
gcc $F -o /tmp/gg_pc pc.c -lm && /tmp/gg_pc 7 3 | tail -3
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_globe pc_globe.c -lm && /tmp/gg_globe | head -2
gcc $F -o /tmp/gg_save pc_save.c -lm && /tmp/gg_save
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_touch_run pc_touch.c -lm && rm -rf /tmp/gg_touch && /tmp/gg_touch_run | head -1   # stylus taps on the menus
gcc $F -w -o /tmp/gg_plague pc_plague.c -lm && /tmp/gg_plague | tail -1
gcc $F -w -o /tmp/gg_laws pc_laws.c -lm && /tmp/gg_laws | tail -1
gcc -O2 -Wall -o /tmp/gg_tcal pc_touchcal.c && /tmp/gg_tcal | tail -1
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_hit pc_hit.c -lm && /tmp/gg_hit | tail -1
gcc $F -w -o /tmp/gg_dis pc_disasters.c -lm && /tmp/gg_dis | tail -1
gcc $F -o /tmp/gg_king pc_kingdom.c -lm && /tmp/gg_king 3 50000 | grep -E "^world|totals"
gcc $F -o /tmp/gg_story pc_story.c -lm && for sz in 128 192 256; do /tmp/gg_story 7 24 $sz | grep -E "planet|END"; done
tools/gm_check.sh /tmp/gm_check 100
# the real DS program, run headless through compat/ndsshim.h with scripted keys and stylus (screenshots in /tmp/gg_ds)
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_tour pc_tour.c -lm && rm -rf /tmp/gg_tour_out && mkdir /tmp/gg_tour_out && /tmp/gg_tour /tmp/gg_tour_out | head -1   # every menu and page, screenshots in /tmp/gg_tour_out
gcc -O2 -DNDSSHIM -DRCHECK -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_ds_run pc_ds.c -lm && rm -rf /tmp/gg_ds && /tmp/gg_ds_run /tmp/gg_ds | head -2
