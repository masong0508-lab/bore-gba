# EP6 step 5f: endless play (the caps)

The realm must behave in year 500 as in year 5. Audit of every counter, and what was done:

| Counter | Saved? | Before | Now |
| --- | --- | --- | --- |
| R->day (u32) | yes | stopped at 0xFFFFFFFF already | tested at the top; the screen's day goes through `khDay` (capped at 99999, never negative where a long is 32 bits) |
| stockpile per good | yes | capped at KG_GOODMAX 60000, the books know | tested: planks reach the ceiling at day 18,001 and hold, books balanced to day 24,000 |
| population, buildings, carriers, roads | yes / no | caps (KS_POPCAP, KS_MAXB, KS_MAXW, KS_MAXN) | unchanged, soaked |
| frame clock S->t (u32) | no | wrapped at 2^32, which is not a whole number of days: one day early or late at the wrap | wraps at KS_TMAX, a whole number of days and walk steps; `ksAge` gives the age of a stamp across the wrap |
| map stamps (LRU of distance maps) | no | the oldest-first compare broke across the wrap | compared by `ksAge` |
| delivered, timeouts, aborts (u32) | no | could wrap to 0 | `ksInc` stops them at the top |
| match rotation S->mrr | no | wrapped at 2^32 in the middle of a cycle | reset at 2^30 |
| save size | yes | unknown over long play | measured: a crowned village stays at 380 to 540 bytes over 24,000 days (edits do not pile up) |

Save format unchanged. Test: `tools/test_kingcap.sh` (24,000 day soaks in the 96 and 128 land with the books, stuck carriers, save and load every 2,000 days; the day count at the top of its u32; the frame clock across its wrap; the counters at their ceilings; the rotation; the map stamps; a gift above the stockpile ceiling; the screen's day). Not tested on a cart. Not done: caps for features not written yet (well, tavern), map expansion.
