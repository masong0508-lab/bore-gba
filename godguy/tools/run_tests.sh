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
# Pack A1, ruler stats: derived from the name seed, 1..10, feed the city cap / power / loyalty, a coronation is journalled
gcc $F -w -o /tmp/gg_rulers pc_rulers.c -lm && /tmp/gg_rulers | tail -1
gcc $F -w -o /tmp/gg_kingx pc_kingx.c -lm && /tmp/gg_kingx | tail -1
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_meddle pc_meddle.c -lm && /tmp/gg_meddle | tail -1
gcc $F -w -o /tmp/gg_events pc_events.c -lm && /tmp/gg_events | tail -1
gcc $F -w -o /tmp/gg_loot pc_loot.c -lm && /tmp/gg_loot | tail -1
gcc $F -w -o /tmp/gg_wild pc_wild.c -lm && /tmp/gg_wild | tail -1
# B + stylus drag pans the map (normal and zoomed view), nothing is painted while B is held
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_pan pc_pan.c -lm && /tmp/gg_pan | tail -1
# DS (19,999), DSi mode (38,000) and 3DS / PC (199,999) worlds filled to the creature cap: refused at the cap, save and load identical, the raw and packed saves fit their blocks, the loader refuses a save above the cap
# (and tools/ram_estimate.sh: an estimate of the DS and DSi link room, since no ARM linker runs here)
gcc -O2 -DNDSSHIM -DGG_MAXC_TEST=19999 -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_cap_ds pc_cap.c -lm && /tmp/gg_cap_ds | grep -E "FAIL|^CAP " | tail -3 | tee /tmp/gg_cap_ds.out; grep -q "^CAP DS OK" /tmp/gg_cap_ds.out
gcc -O2 -DNDSSHIM -DGG_DSI16 -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_cap_dsi pc_cap.c -lm && /tmp/gg_cap_dsi | grep -E "FAIL|^CAP " | tail -3 | tee /tmp/gg_cap_dsi.out; grep -q "^CAP DSI OK" /tmp/gg_cap_dsi.out
gcc -O2 -DNDSSHIM -DGG_BIG -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_cap_big pc_cap.c -lm && /tmp/gg_cap_big | grep -E "FAIL|^CAP " | tail -3 | tee /tmp/gg_cap_big.out; grep -q "^CAP BIG OK" /tmp/gg_cap_big.out
tools/ram_estimate.sh
# big planets (384 .. 1024), the 199,999 creature cap, the village cap, zoom on a squeezed planet, save and load, and saves crossing between builds (DS-sized <-> big)
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
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_slotscr pc_slotscr.c -lm && /tmp/gg_slotscr | tail -1
gcc -O2 -DNDSSHIM -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_stereo pc_stereo.c -lm && mkdir -p /tmp/gg_stereo_out && /tmp/gg_stereo /tmp/gg_stereo_out | tail -1
# plague (step 6): spread, recovery, heal, the law, zombie bite and rising, village store drain, save
gcc $F -w -o /tmp/gg_plague pc_plague.c -lm && /tmp/gg_plague | tail -1
# planet generator (step 7): main's six shapes unchanged at COASTS / MOUNTAINS normal, the new controls work, SEA the right way round
gcc $F -w -o /tmp/gg_gen pc_gen.c -lm && /tmp/gg_gen | tail -1
# speed (step 8): the kingdom answer cache (every cached answer checked against a fresh one), the globe redrawn only on change, the fast shim renderer equal to the plain one byte for byte, a crowd's flat cost per tick
gcc -O2 -DNDSSHIM -DKD_CHECK -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_speed pc_speed.c -lm && /tmp/gg_speed | tail -1
# the 3DS platform file against a libctru stand-in: tiled screen copy, only changed screens copied, the sound queue, the New 3DS speed-up
gcc -O2 -I3ds/stub -o /tmp/gg_3ds pc_3ds.c 3ds/platform3ds.c -lm && /tmp/gg_3ds | tail -1
# DSi mode (step 9): a 768 x 768 planet saves, packs into the 2 MB block and loads back identical
gcc -O2 -DNDSSHIM -DGG_DSI16 -Icompat -Isource -Wno-all -include unistd.h -o /tmp/gg_vilsave pc_vilsave.c -lm && /tmp/gg_vilsave | tail -1
# the DSi-mode boot probe under Unicorn (only when pip's unicorn and an arm-none-eabi compiler are installed; build_dsi.sh's build_dsi16/arm9.elf is used when it exists)
if python3 -c "import unicorn" 2>/dev/null && [ -f build_dsi16/arm9.elf ]; then python3 tools/dsi_emu/probe.py build_dsi16/arm9.elf build_dsi16/arm9.bin | tail -1; else echo "probe: skipped (no unicorn or no build_dsi16)"; fi
# the cartridge save layer's packer on a fake chip (step 9b): round trip, and a pack bigger than the chip is refused
grep -E "^static u32 rle(Enc|Dec)" compat/ndshw.h > /tmp/gg_rle.inc && gcc -O2 -w -DRLE_INC='"/tmp/gg_rle.inc"' -o /tmp/gg_slots pc_slots.c && /tmp/gg_slots | tail -1
# GodGuy step 10a, identity: permanent ids, derived names and personalities, GGN1 trailer, old saves still load
gcc $F -w -o /tmp/gg_ident pc_ident.c -lm && /tmp/gg_ident | tail -1
# GodGuy step 10b, notables: promotion, hero, scars, grief, births, full-table eviction, stars, save and load
gcc $F -w -o /tmp/gg_notable pc_notable.c -lm && /tmp/gg_notable | tail -1
# GodGuy step 10c, cells: the 8 x 8 cell grid equals a rebuild, cellNearest equals the tile scan, and the game runs exactly as with the old scan (same state hash with -DGG_NOCELL)
gcc $F -w -o /tmp/gg_cells pc_cells.c -lm && /tmp/gg_cells | tail -1 && gcc $F -w -DGG_NOCELL -o /tmp/gg_cells_old pc_cells.c -lm && [ "$(/tmp/gg_cells | grep 'state hash')" = "$(/tmp/gg_cells_old | grep 'state hash')" ] && echo "CELLS same game as the tile scan"
# GodGuy step 10c, war groups: one record per army (target, soldiers, power, centre) equals an independent count, and counting changes nothing
gcc $F -w -o /tmp/gg_wargrp pc_wargrp.c -lm && /tmp/gg_wargrp | tail -1
# GodGuy, the war benchmark (the harness before step 10d): two armies of thousands at war, time per tick, turns per creature per year, casualties, save size. The turn counter changes nothing in the game (same state hash without it)
gcc $F -w -o /tmp/gg_war pc_war.c -lm && /tmp/gg_war 1 3000 20 4096 | tail -1 && gcc $F -w -DWAR_NOSTAT -o /tmp/gg_war_ns pc_war.c -lm && [ "$(/tmp/gg_war 1 1200 20 2048 | grep 'state hash')" = "$(/tmp/gg_war_ns 1 1200 20 2048 | grep 'state hash')" ] && echo "WAR same game with and without the counter"
# GodGuy step 10d, tiers and distant battles: the game against the full per-unit run in war_ref_full.txt (8 seeds, elves against dwarves, camera far and near): casualties within 10% at every checkpoint (this is now a pass or fail)
/tmp/gg_war cmp war_ref_full.txt 10 | tail -1 | grep -q "WAR BENCH OK" && echo "WAR within 10% of the full per-unit run"
# save and load in the middle of a far war, the same state hash; member lists only name their own soldiers; far battles reach the notable table
gcc $F -w -o /tmp/gg_far pc_far.c -lm && /tmp/gg_far | tail -1
# the game as it was before the tiers (-DGG_NOTIER) still builds and plays a war: the benchmark's old side, and the small-world guarantee (below 600 creatures nothing changes)
gcc $F -w -DGG_NOTIER -o /tmp/gg_far_old pc_far.c -lm && /tmp/gg_far_old | tail -1 | grep -q "FAR" && echo "FAR (no tiers) builds"
# GodGuy step 10e, the home lookup of the war groups: the village blocks give the answer vilNear gives for every tile and people (and a crowded block falls back), and a war plays out with the same state hash counted either way
gcc $F -w -o /tmp/gg_home pc_home.c -lm && /tmp/gg_home | tail -1 && gcc $F -w -DGG_SLOWHOME -o /tmp/gg_home_old pc_home.c -lm && [ "$(/tmp/gg_war 1 3000 20 2048 | grep 'state hash')" = "$(gcc $F -w -DGG_SLOWHOME -o /tmp/gg_war_old pc_war.c -lm && /tmp/gg_war_old 1 3000 20 2048 | grep 'state hash')" ] && echo "HOME same war as the old count"
# GodGuy step 10f, the rolled war-group count finds every soldier's home exactly (ends with ROLL OK)
gcc $F -w -o /tmp/gg_roll pc_roll.c -lm && /tmp/gg_roll | tail -1
# GodGuy step 10g, the near-camera war at big sizes: 32,000 (768 planet, DSi caps) and 60,000 (1024 planet, 3DS/PC caps) against their 8-seed full per-unit references. Takes a few minutes, so only with GG_BIGWAR=1 ./tools/run_tests.sh
if [ "$GG_BIGWAR" = "1" ]; then
gcc $F -w -DWAR_PLANET=768 -DWAR_COLS=8 -DWAR_ROWS=8 -DWAR_SIDE=16000 -DGG_DSI16 -o /tmp/gg_war_dsi pc_war.c -lm && /tmp/gg_war_dsi cmp war_ref_dsi.txt 10 | tail -1
gcc $F -w -DWAR_PLANET=1024 -DWAR_COLS=10 -DWAR_ROWS=10 -DWAR_SIDE=30000 -DGG_BIG -o /tmp/gg_war_big pc_war.c -lm && /tmp/gg_war_big cmp war_ref_big.txt 10 | tail -1
fi
