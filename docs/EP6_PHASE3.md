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

## Step b: source/kinglog.h, the hauling
Needs `kingsim.h`. Carriers are free Nubbins; goods move along the player's roads.
- A building that wants a good asks for a DISTANCE MAP (road steps from the building; its road-side cells are 0). Up to 12 live at once (768 bytes each). One is built at a time, 64 road cells a frame, so the worst frame stays near the average. A road change marks every map not ready. A map still waiting is not taken over for 300 frames (else requesters evict each other for ever).
- The matcher looks at 6 buildings a frame and starts at most 2 jobs. A requester (a blueprint's cost, a processor's input) gets the nearest producer that holds the good, else the nearest storehouse with it in the stockpile, whose road side is joined to the building. A producer's surplus goes to the nearest storehouse the same way (also when it wants something itself: a first version skipped this and the economy stopped).
- The carrier takes the good AT ONCE and appears at the source's road side, so nothing is promised on a shelf and the books always balance. It then takes a step every 4 frames to the neighbour with the lowest distance and hands over at distance 0.
- HEALING: a map not built yet or a cut road just makes it wait. Cut off for 180 frames: the good goes back to the stockpile (counted as an abort). Waiting for anything else for 900 frames: the same (a timeout). A road cell lost under a carrier: it steps aside, else to the nearest road, else it is sent home. A demolition sends every carrier home first, so building numbers can shift safely. `ksPrepSave` does the same before a save: a save never holds a carrier.
- `ksCheck` (books, road list, carriers, claims, map references) is what the PC test runs all the time. A route may be at most 254 road steps.
- Not run on a cart. The test is step c.
