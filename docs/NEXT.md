# What to build next (notes from playing THPS3 / THPS4 on GBA)

**Status (checked against the source, updated with the combo string):** the SPECIAL meter (`lspec`, top bar under the score), the SKATE letters (`lskl`, `sktAward`), SWITCH tricks (`lsw`), manuals, wall taps and the combo chain (`lcN`) exist, and the top bar now shows the **combo string**: the names of the tricks in the running chain, newest last, with the multiplier (`lcAdd` in main.c keeps the names, `hudComboStr` in hud.h fits them in the middle of the top bar). **Jobs:** PRO SKATER (track 8, `JT_SKATE` in sims.h) is the default job and the ONLY one paid in trick points (the SKATER QUOTA option scales it). Every other track is a normal job: its quota is work minutes (`jobQuotaOf`; a minute counts 1 while you are up and about, 2 while STOKED), and promotions want a life skill (`jobSk`) instead of SKATING. **Collectibles are in** (`clPlace` / `clTick` / `clDraw` in main.c): the letters S K A T E and one hidden tape float over empty floor on any lot that has something to skate; they are scattered again every run, drawn in code, saved nowhere. A letter feeds `sktAward` (five = SKATE bonus), the tape pays 1000. **VIEW TRICKS** is in the pause menu under WANTS (`tricksScreen`, skills.h). Not built yet: wants for collecting ("COLLECT S-K-A-T-E"), a tape count saved per map, halfpipe, timed run, bail flicker, a goals list per map. What does exist and helps: story mode (`story.h`) hands out chapter goals like THPS4's, neighbors visit and welcome you (`twTick`), a radio and sound system play the jukebox in the room, and the aspiration wants and fears act as a goal list. For the Sims-side wishlist see `WISHLIST_EFFORT.md`.

Observed by playing the carts in an emulator; nothing was copied from them. These are design targets for BORE, sized for the GBA.

## What the THPS GBA games do that BORE can match
| Seen in THPS3 GBA | BORE today | Next step (fits the GBA) |
|---|---|---|
| Fixed isometric camera that scrolls smoothly with the skater | Same, already | Keep |
| HUD: `Score` top left with a **special meter** bar under it, a big red **2:00 run timer** top centre | Score, cash, clock | Add a SPECIAL meter (fills with tricks, drains slowly; full = bonus points and special tricks). Optional TIMED RUN mode (2:00) beside the free life mode |
| **Level intro flyover**: the camera pans to each goal with a banner ("FIND THE HIDDEN TAPE", "WALLRIDE THE MAGMA FALLS") | none | A goals intro: pan the camera to 3 to 5 map spots with a text banner. Needs a goal table per map (id, tile, text) |
| **Collectibles in the level**: floating S-K-A-T-E letters, a hidden tape, coins | none | Item tiles that spin (2 to 4 frame sprites), collected on touch; feed wants ("COLLECT SKATE") and DNA |
| **Career goals list** (pause menu: VIEW GOALS) | wants and fears | Per-map goal list saved per slot: high score, SKATE, tape, gap, wallride. Completing goals unlocks parts or maps |
| **Pause menu**: CONTINUE, RETRY, VIEW GOALS, VIEW TRICKS, SOUND, END RUN | similar | Add VIEW TRICKS (the trick list and controls) |
| **SWITCH stance** badge (skateboard icon, top right) | none | Track stance after a 180; switch tricks score more |
| **Bail**: the skater flickers (dithered) while getting up | stun + sounds | Flicker the sprite during `lstun` |
| Halfpipes and quarter pipes everywhere, wallrides | kicker, quarter pipe, launch | Wallride on walls (hold the grind key against a wall) and a HALFPIPE item (two quarter pipes back to back) |
| THPS4: free roam, no timer, goals handed out by people in the level | life mode is free roam | **NPCs that give goals**: the freed 124 KB of RAM (see README, RAM budget) holds about 10 baked characters. An NPC = baked sprite set + a tile + a goal id; talk with R |

## Order to build it in
1. **Special meter + trick names on screen + combo string** (cheap, all HUD; biggest "feels like THPS" win).
2. ~~**Collectibles** (SKATE letters, tape)~~ done as runtime pickups; still to do: hook them into wants ("COLLECT S-K-A-T-E") and keep a tape count per map.
3. **NPCs**: bake 2 to 4 extra sprite sets from creator looks (reuse `bakeSprites`), place them on the map, simple idle/wander, R to talk: they give a goal (THPS4 style) and count as SOCIAL for the Sims side.
4. **Goals list per map** + level intro flyover.
5. **Timed run mode** (2:00, high score / pro score / sick score).
6. Wallride, switch stance, manuals on the MANUAL PAD.

## GBA limits to respect
- EWRAM 256 KB (113 KB when this was written; the 32 x 60 sprites and household tiles added about 25 KB since: see NOTES), IWRAM 32 KB (about 30.7 KB used by code + .bss; the stack lives in the rest, so new code must stay out of IWRAM). One baked character = 11 KB at 16 bits per pixel; 8 bits plus a palette halves that.
- The game draws in mode 3 (one 240x160 bitmap, the CPU draws everything). Each extra character on screen costs draw time: keep NPC sprites small and cull anything off screen. Check DRAW COST in OPTIONS > VIDEO after each addition.
- The ROM is about 3 MB now; carts go to 32 MB, so ROM space is not the problem, RAM and draw time are.
