# EP6 Phase 0: spike results (PC only)

**What this is:** prototypes in `tools/phase0/` (run `sh tools/phase0/run_phase0.sh`; Termux uses `$TMPDIR`). There is no ARM toolchain or emulator in the session that wrote them, so NOTHING here is a GBA measurement. The hauling prototype counts real work on a PC and turns it into cycles with stated per-operation costs; the drawing figures are a model from the GBA's bus speeds. Replace both with numbers from a cart or Pizza Boy before the go/no-go is final.

## 1. Drawing: the walking giant breaks Plan A, Plan B holds
Budget: 280,896 cycles a frame. The game draws in mode 3 into `fb` (76,800 B EWRAM) and DMAs it to VRAM.
- Presenting the whole fb is about 153,600 cycles (55% of a frame). Shifting the fb inside EWRAM is about 230,400 (82%). Shifting inside VRAM is 76,800 but tears (mode 3 has one buffer and vblank is 83,776).
- **Plan A (software, as the town screen does it) when the whole view scrolls:** about 425k to 467k cycles a frame, i.e. 150 to 166% of a frame, so **30 fps at best, with nothing else running**. Redrawing the whole view each frame is worse (164 to 246%, 20 to 30 fps). Tile-step scrolling keeps 60 fps between steps but hitches (a 2-frame frame) every 8 frames.
- **Plan B (mode 0, hardware tile backgrounds, sprites for the giant and Nubbins):** about 3,100 cycles a frame (1.1%): scroll registers, streaming one new map column and row when the view crosses an 8 px line, about 49 sprites. **60 fps with about 99% of the frame left.**
- **Recommendation: Plan B.** Costs: tile art for the ground and buildings (not drawn in code), 128-sprite limit, menus in mode 3 clobber BG VRAM so leaving a menu reloads about 32 KB of tiles and map (one frame's worth of DMA, once), and the realm gets no `fb`-based effects.
- Constants are in `draw_model.c` (EWRAM 3 cycles a 16-bit access, VRAM 2 a word, software pixel 8 to 14 cycles).

## 2. Hauling: cheap on average, spiky in the worst frame, memory is the cost
Prototype (`haul_proto.c`): 128 x 128 map, road nodes, per-building distance maps (a worker steps to the neighbour with the lowest distance), a request/offer matcher (2 requesters a frame), one distance map rebuilt a frame. A realm day is 600 frames [ASSUMED].
| setup | road nodes | average frame | worst frame | distance maps |
|---|---|---|---|---|
| 10 buildings, 40 workers (the spec) | 341 | 0.5 to 1.3% | 6.5 to 16% | 3.4 KB |
| 24 buildings, 40 workers | 694 | 0.7 to 1.8% | 12 to 29% | 16.7 KB |
| 24 buildings, 80 workers | 694 | 0.7 to 1.9% | 12 to 30% | 16.7 KB |
| 30 buildings, 120 workers | 816 | 1.3 to 3.2% | 16 to 40% | 24.5 KB |
(Cycle ranges use 25 to 60 per matcher step, 30 to 80 per worker step, 20 to 50 per BFS node [ASSUMED].)
- 10,000 days with a road cut, a processor destroyed, and a building walled in: the goods ledger balanced every 1,000 days and at the end, **0 workers stuck**, longest stall 901 frames (the 900 timeout fired once and healed it). Throughput came back to the pre-fault level each time (about 49,500 goods per 1,000 days). A cut road with a loop round it cost about 5%. A destroyed processor stops the castle chain until rebuilt (expected: it needs all four better goods).
- The worst frame is the distance-map rebuild (one whole map, 341 to 816 nodes) plus a full matcher scan. **Fix when built:** rebuild a map in slices of about 64 nodes a frame, which cuts the worst frame to near the average.
- **Memory is the real cost.** Distance maps grow with buildings x road nodes. A plain u16 cell-to-node table would be 32 KB, so the real build needs a hash (about 4 KB) or node numbers kept beside the map. Keep maps only for buildings with open requests, or cap buildings at about 16.

## 3. Saves: already done (Phase 2)
Realms 40 bytes to 7.7 KB, power-cut tested on the real `save.h` on a fake chip, own done-bits block. Not repeated here.

## 4. RAM: not measured, two conflicting figures in the repo
Static limit is 253,952 B (`Makefile`: 262,144 minus the 8 KB stack). `docs/RAM_AUDIT.md` says 247,352 B used (about 6.6 KB free); `docs/RAM_PLAN.md` (22 Oct, CI) says 236,352 B (about 17.6 KB free). **Read the CI "GBA memory" job for the real number.**
A realm needs roughly: map 16 KB (128 x 128) + distance maps 3 to 17 KB + workers, buildings, stocks about 3 KB = 22 to 36 KB. That does not fit in the free space, so it must share scratch with something the life game does not use while a realm is open. Candidates from the source: `hhPool` (about 30 KB, household sprites), `sprBuf` (about 15 KB, player sprites; `tfb` already reuses it as scratch), `flPool` 8 KB, and `fb` itself (76.8 KB, dead while the realm runs in mode 0 but wiped by any mode 3 menu). Whatever is chosen must be rebuilt on leaving the realm.

## Go / no-go (provisional, until a cart run)
- **Go**, with Plan B (hardware tiles) as the drawing plan. Plan A cannot hold 60 fps and probably not 30 with a giant that scrolls the whole view.
- **Go** on hauling at the spec size; slice the distance-map rebuild, keep to about 16 buildings and 40 to 80 workers at first.
- **Open (needs you):** the shared scratch choice, and a real RAM number from CI.
- **Still to do on real hardware:** a mode 0 scrolling test on a cart or in Pizza Boy (this is the prototype I could not build), and the hauling code timed on the ARM.
