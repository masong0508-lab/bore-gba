# EP6 step 5e: the four giants

One table, `khStatT` in `source/kinghand.h`, holds the four stats of the roadmap's section 4.3 (stride, carry, work, build) for the five giants. All values are [ASSUMED].

| Giant | Stride | Carry | Work | Build |
| --- | --- | --- | --- | --- |
| A the all-rounder | 3 | 5 | 2 | 1 |
| B the runner | 4 | 4 | 2 | 1 |
| C the lifter | 2 | 7 | 2 | 1 |
| D the builder | 3 | 5 | 1 | 2 |
| Your own Bore | 3 | 5 | 2 | 1 (average until Phase 7 sets it from the Sim's skills) |

- `khStride`, `khRun`, `khYield` read the table (B: faster, carries less; C: slower, carries more, the old yield 3 and load 8 are gone so C is "plus on one, minus on one"; D: gathers 1 not 2).
- `khLoad` (kinghold.h) now reads the carry column.
- New: `khBuild`. A site that the giant's own hands finish (the load covers everything the site still asks for) gives back `build - 1` goods through `ksGift`, so the builder gets 1 good back per hand-finished site and the books stay balanced. Nothing else changes: the save format is untouched.
- Test: `tools/test_kinggiants.sh` (the table's shape, none best, bad giant numbers, the rebate for all five giants, no rebate on a part delivery, books balanced). `test_kinghand.c` and `test_kinghold.c` only had range checks loosened (yield >= 1, load >= 4).
- Not done: your own Bore's stats from the Sim's skills (Phase 7), the endless-play caps, well and tavern, map expansion. Not tested on a cart.
