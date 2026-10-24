# EP6 step 5d: bridges

A bridge is a road laid over water: terrain value 7 (KT_BRIDGE; KT_N is now 8). Still bits 0..2 of a cell, so the save format is unchanged (a run of value 7 is a run like any other; old saves load as before, a save with a bridge needs this ROM).

- `kingsim.h`: `ksRoadV(v)` (road or bridge), `KS_BRIDGE_N` (1 cut stone a cell, [ASSUMED]), `KSE_POOR`. `ksRoadSet` lays a bridge on water if the stockpile has the cut stone (it leaves R->goods and S->made together, so `ksCheck` balances); lifting a bridge gives water back and the stone stays sunk. Carriers walk bridges like roads (kinglog.h, node list). No building goes on a bridge (ksInit refuses one in a save).
- `kinghand.h`: the ROAD mode lays a bridge on water; new result KHR_NOCUT, "A BRIDGE NEEDS CUT STONE" (KHR_N 31); terrain name BRIDGE.
- `kingview.h`: a bridge is drawn as the road tile for now (tile 7 is the off-map deep water; real art is Phase 6). `kingover.h`: a brown overview colour.
- `kingq.h`, `kingtask.h`: QK_KBRIDGE (108), EXK_BRIDGE; `kingmis.h`: THE FAR BANK is now 4 bridge cells, 2 storehouses, 16 buildings.
- Tests: test_kingmis (bridge section, a river in the village, mission 8 played), test_kinghand and test_kingview updated.
- Not done: expanding the map to new ground (the second half of the roadmap's "bridges and expansion"), bridge art.
