# TO_ADD wishlist: effort ranking (smallest to biggest)

Sized from reading the code, not from building it. S = under an hour, M = a session, L = several sessions, XL = a new system.

## Done in this patch
1. **Rename the Sims-nod tags** (S). OPTIONS tab SIM is now SIMU, its sub-page SIMS is now BORES, and the MASTER page is now CHEAT with a descriptor that says what it is (debug cheat console: extreme body sliders).
2. **Belly slider** (S). Root cause: the belly (and chest) rows only existed for a torso of 3+ rows, but `bodyPlan()` gives almost every body a 2 row torso, so the slider changed nothing. On a 2 row torso, BELLY now widens the hip row and CHEST the shoulder row.

## Next, cheapest first
3. Sk9m catchphrase / DayBar and Sk9m as cameos (S-M): artist names exist already (artists.h); a cameo is a line of text on a job event.
4. Hide the half-of-story songs (S-M): the unlock plumbing exists (unlocks.h, jbUnlock). It currently keys off lifetime dreams (UL_CLOSER / UL_TREE); it needs a story-mission counter to key off instead.
5. Lock most sliders until unlocked (M): needs a per-row lock mask in the creator plus unlock bits saved next to the jukebox flags.
6. Rotation decision (M): `cview` is used in about 27 places; removing it frees code but touches drawing everywhere. Needs a decision first.
7. Death types and lingering ghosts (M): ghost and death code exist; add variants and a persistence flag.
8. Radio / sound system items and TV channels (M): new item ids plus a jukebox hook; TV clips cost ROM.
9. Windows, outside decor, roof options (M-L): new item ids and wall-slot drawing.
10. More voices and sound effects, random voice per birth (M): encode_voices.py pipeline exists; each clip costs ROM.
11. Neighbors visiting at move-in (M-L): visitor code exists in house.h; needs a scripted arrival.
12. Mirrors that change style/body (L): reuse the creator screens in a limited mode; gender, random and age stay behind debug mode.
13. Better multi-floor system and 3-floor render toggle with focus key (L): draw cost on mode 3 is the limit.
14. In-game relationships/careers viewer macro (L).
15. Yards of varying size (L): lot sizes touch the neighborhood and save formats.
16. Better air physics (L): needs play-testing, easy to break tricks.
17. Pets: dogs, cats, birds, beetles, guinea pigs (XL): new baked sprite sets (11 KB each in EWRAM), AI, needs.
18. Seasons, weather, four holidays (XL): none of it exists (zero weather code); touches palette, time, items.
19. Police and jail (XL): no code exists; needs NPCs and a state machine.
20. Skate park with AI skaters (XL): NPC skaters, level data, the draw-time budget.
21. New game+ / generational carryover (XL): save format change.
22. Extra animations (open-ended), more baked character models (RAM bound), .bps expansion packs (XL, far future).
