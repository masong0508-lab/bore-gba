# EP6 Phase 3: the economy and the hauling (PC only, nothing in the game calls it yet)

## Step a: source/kingsim.h, the economy
Pure C on top of `kingdom.h` (every function `static inline`, so an unused one costs no ROM). One struct, `KSim` (about 13 KB), holds the whole simulation; the realm, the building list and the map are the caller's (the ones `kgEncode` / `kgDecode` use).
- Seven building types in a table (`KS_T`): hut, house, woodcutter, quarry, sawmill, mason, storehouse. A building is a blueprint (level 0) until its cost, one good, has been carried to it. Costs, periods and sizes are numbers in the table and are not in a saved realm, so they can be tuned without breaking saves.
- Homes take one Nubbin a day (hut 4, house 8). The founding party is `KS_START` (4) Nubbins with no home: without them a new realm could never build its first hut (the earlier attempt's 30-building soak sat at population 0 and "passed" because nothing checked that it ran).
- A quarter of the Nubbins (and one) stay free to carry. The rest staff gatherers first, then processors (gatherers first so wood and stone never run out of hands).
- Gatherers fell trees and quarry rock in a 14 x 14 reach, one per `period` frames. A working woodcutter plants trees too (a first version only grew a tree beside another, and the forest died out for good; the fix is in the test log). A quarry runs dry.
- Processors turn 2 of their input into 1 output. The storehouse is the way in and out of the stockpile (`R->goods`).
- THE BOOKS: `S->made[g]` is what should exist of each good (gifts and production, minus what is used up or lost at the stockpile's ceiling). `ksLedger` checks it against what does exist (stockpile + buildings + carriers). Every function that moves a good keeps it true.
- In a saved building `stock` is: low byte = goods delivered to it, high byte = goods it holds to send out. `ksInit` clamps and checks everything a damaged save could hold, and refuses overlaps, roads under buildings and more than `KS_MAXB` (40) buildings or `KS_MAXN` (768) road cells.
- Placing, demolishing (what a building holds goes to the stockpile) and roads (`ksRoadSet`, a sorted list of road cells with a binary search) live here too.
- Not run on a cart. The step has no test of its own: `tools/test_kingsim.c` comes in step c.
