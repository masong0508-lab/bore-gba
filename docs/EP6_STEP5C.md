# EP6 step 5c: the ten missions

The three placeholder missions are replaced by the roadmap's ten (kingmis.h). They are played in your own life and done in the realm.

- `kingq.h` (pure C): task kinds 100..107 (buildings built, a building type, Nubbins, Nubbins at work, a good, castle stage, road cells, loads delivered) and `exkVal`.
- `kingtask.h`: `ktFeed` fills 26 numbers from the running sim (read only).
- `kingmis.h`: `xmKing`, ten missions of two or three tasks.
- `expand.h`: includes the three; `exKingTick(S)` ticks the open mission's realm tasks; `exKv` keeps the last numbers for the task list; life-game `exTkNow` never ticks these kinds. A met task stays met (exTk is saved); the life game completes the mission, pays and opens the next, as before.
- `kingplay.h`: calls `exKingTick` once a second (every 60 passes of the loop) and shows "DONE ..." in the HUD.
- `tools/test_kingmis.sh`: data checks, the feed, and a scripted player who plays all ten in order (done by about day 21).

Changes to know about:
- The old placeholder missions' shared bits 60..62 no longer count as done (the missions are different now), so everyone starts the pack's missions from the first one.
- THE FAR BANK asks for 20 buildings and 2 storehouses until bridges exist.
- TIMBER! now teaches the sawmill and A ROCK AND A HARD PLACE the mason (houses, the weaver, the mine and the castle all cost planks or cut stone).
- Numbers are [ASSUMED]: tune them in kingmis.h. Not compiled for ARM here (no toolchain); the host parse test and all PC tests pass.
