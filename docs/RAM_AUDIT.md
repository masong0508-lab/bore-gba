# RAM audit (ghosts + weather)
The ghosts and weather in `source/fx.h` run on hardware sprites (OAM), so they add no frame buffer and no drawing time.
All their state variables are `EWRAM_BSS` (IWRAM is nearly full). The art is in ROM.
Measure the real numbers with `make size` (or the GitHub Actions job summary) after building; the last measured build before this update was 247,352 B EWRAM (94%).
If EWRAM gets tight, the biggest users are `hhObj` (28 KB), the bake buffer (11 KB), `fb` (76.8 KB) and `spr4` (11 KB).
