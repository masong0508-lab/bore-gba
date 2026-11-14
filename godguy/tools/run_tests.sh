#!/bin/bash
# GodGuy's PC tests (the DS build cannot be run in any emulator available here, so the shared game and sound code is tested on the PC)
cd "$(dirname "$0")/.." || exit 1
F="-O2 -DPCTEST -Wno-misleading-indentation -Wno-unused-variable -Wno-unused-function -Wno-parentheses"
set -e
gcc $F -o /tmp/gg_pc pc.c -lm && /tmp/gg_pc 7 3 | tail -3
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_globe pc_globe.c -lm && /tmp/gg_globe | head -2
gcc $F -o /tmp/gg_save pc_save.c -lm && /tmp/gg_save
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_touch_run pc_touch.c -lm && rm -rf /tmp/gg_touch && /tmp/gg_touch_run | head -1   # stylus taps on the menus
gcc $F -o /tmp/gg_king pc_kingdom.c -lm && /tmp/gg_king 3 50000 | grep -E "^world|totals"
gcc $F -o /tmp/gg_story pc_story.c -lm && for sz in 128 192 256; do /tmp/gg_story 7 24 $sz | grep -E "planet|END"; done
tools/gm_check.sh /tmp/gm_check 100
# the real DS program, run headless through compat/ndsshim.h with scripted keys and stylus (screenshots in /tmp/gg_ds)
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_tour pc_tour.c -lm && rm -rf /tmp/gg_tour_out && mkdir /tmp/gg_tour_out && /tmp/gg_tour /tmp/gg_tour_out | head -1   # every menu and page, screenshots in /tmp/gg_tour_out
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_ds_run pc_ds.c -lm && rm -rf /tmp/gg_ds && /tmp/gg_ds_run /tmp/gg_ds | head -2
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_zoom_run pc_zoom.c -lm && rm -rf /tmp/gg_zoom && /tmp/gg_zoom_run /tmp/gg_zoom | tail -1   # zoom: six levels, L+R / X+Y, stylus mapped back to tiles
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_bio_run pc_bio.c -lm && /tmp/gg_bio_run | tail -1   # biomes and atmosphere: painting, spreading, air powers, save and load
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_fix pc_fix.c -lm && for t in "menu" "chap 1" "chap 2" "chap 4" "music 0" "music 1"; do /tmp/gg_fix $t | tail -1; done   # the pause menu does not flicker, chapters move on (paused, 2x, 4x), songs end
# big planets (384 .. 1024), the 100,000 creature cap, the village cap, zoom on a squeezed planet, save and load, and saves crossing between builds (DS-sized <-> big)
gcc -O2 -DNDSSHIM -DGG_BIG -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_big pc_big.c -lm && /tmp/gg_big | tail -1
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_small pc_big.c -lm
N=$(/tmp/gg_small mk /tmp/gg_x.sav | sed -n 's/.*creatures \([0-9]*\) villages \([0-9]*\).*/\1 \2/p'); /tmp/gg_big ld /tmp/gg_x.sav $N | tail -1   # a DS-sized save loads on the big build, creature for creature
# settlements (step 4): unit levels and village mood, docks and boats (and the boat trailer of a save), temples, statues, libraries, crowding
gcc $F -w -o /tmp/gg_life pc_life.c -lm && /tmp/gg_life | tail -1
gcc $F -w -o /tmp/gg_boats pc_boats.c -lm && /tmp/gg_boats | tail -1
gcc $F -w -o /tmp/gg_temples pc_temples.c -lm && /tmp/gg_temples | tail -1
# World Laws (step 5): each law really changes what it names, laws are saved, and the LAWS tab pages and flips them
gcc $F -w -o /tmp/gg_laws pc_laws.c -lm && /tmp/gg_laws | tail -1
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_lawmenu pc_lawmenu.c -lm && /tmp/gg_lawmenu | tail -1
