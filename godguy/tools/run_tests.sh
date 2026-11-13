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
# zoom: the six levels (planet, very far, far, normal, close, micro) driven by L+R / X+Y, panned, and the stylus mapped back to tiles (screenshots in /tmp/gg_zoom)
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_zoom_run pc_zoom.c -lm && rm -rf /tmp/gg_zoom && /tmp/gg_zoom_run /tmp/gg_zoom | tail -1
# biomes (corruption, infernal, enchantment) and the atmosphere (gases, greenhouse, dust, climate)
gcc $F -w -o /tmp/gg_biome pc_biome.c -lm && /tmp/gg_biome | tail -1
gcc $F -w -o /tmp/gg_atmo pc_atmo.c -lm && /tmp/gg_atmo | tail -1
# big planets (the 3DS and PC layout: 16-bit coordinates, up to 1024 x 1024 tiles and 100,000 creatures)
gcc $F -DGG_BIG -w -o /tmp/gg_big pc_big.c -lm && /tmp/gg_big 1024 3000 | tail -1
# the planet generator: land masses follow the LAND choice, the sea level controls the land share, same seed same planet
gcc $F -w -o /tmp/gg_gen pc_gen.c -lm && /tmp/gg_gen | tail -1
# saves keep every village across builds with different village counts (write as a 40-village build, read as 64 and as 200)
gcc $F -DGG_VILN=40 -DVS_WRITE -w -o /tmp/gg_vsw pc_vilsave.c -lm && /tmp/gg_vsw >/dev/null && gcc $F -w -o /tmp/gg_vsr pc_vilsave.c -lm && /tmp/gg_vsr | tail -1 && gcc $F -DGG_BIG -w -o /tmp/gg_vsr2 pc_vilsave.c -lm && /tmp/gg_vsr2 | tail -1
# save slots (the 3DS / PC file layout): nine files, listed from their headers, the furthest world is the one the game starts on
gcc $F -w -o /tmp/gg_slotsrun pc_slots.c -lm && /tmp/gg_slotsrun | tail -1
# unit levels and village mood
gcc $F -w -o /tmp/gg_life pc_life.c -lm && /tmp/gg_life | tail -1
# boats and docks
gcc $F -w -o /tmp/gg_boats pc_boats.c -lm && /tmp/gg_boats | tail -1
# temples and statues
gcc $F -w -o /tmp/gg_temples pc_temples.c -lm && /tmp/gg_temples | tail -1
