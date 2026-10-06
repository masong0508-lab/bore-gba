# TO_ADD wishlist: status and effort (checked against the source, not just the old notes)

S = under an hour, M = a session, L = several sessions, XL = a new system. "Untested on hardware" means the code was written and syntax-checked
but not run in an emulator: build it (GitHub Actions) and play it before trusting it.

## Done
1. **Rename the Sims-nod tags** (S). OPTIONS tab SIM is SIMU, its sub-page SIMS is BORES, the MASTER page is CHEAT with a descriptor that says what it is.
   (Cosmetic leftovers: a few code comments and the NOTES heading still say MASTER CONTROLLER.)
2. **Belly slider** (S). On a 2 row torso BELLY widens the hip row and CHEST the shoulder row.
3. **Lock most sliders until unlocked** (M). Six packs bought with jenes; Konami code opens all. (Not gated by story or lifetime dreams: a design choice, see NOTES "Slider locks".)
4. **Hide the half-of-story songs** (S-M). CLOSER TO THE END and TREE-AGE IN ACTION unlock at 18 of 35 story chapters (`jbStoryDone`).
5. **DayBar cameo** (S): after a good shift, about 1 in 12 (half of the 1 in 6 cameo chance), with a catchphrase.
6. **Sk9m cameo and catchphrase** (S). The other half of the cameo roll: "SK9M  IM KIND OF A BIG DEAL" then "YEAHHHH" (2 in 5 of his visits), or his name and one of four other lines. `sims.h`, `simCameo`. *Untested on hardware.*
7. **Radio and sound system items** (M). Palette items RADIO (`R`) and SOUND SYSTEM (`A`), appended at the end of the item list, so old saves load. Press R next to one to tune the next station: ALL SONGS FM, DAYBAR FM, SK9M BASS RADIO, DANNY STEELE FM, BRENO FM, SINGHS RADIO, then OFF (normal game music returns). A station plays the unlocked, non-secret songs of one artist. The sound system also gives a small mood lift. Not yet in the default house: place them in the BUILD ROOM item tool. *Untested on hardware.*
8. **Death variants** (M). The dead screen now says why: DIED OF SHOCK, GRAVITY WON, MET A WALL AT SPEED, DIED OF HUNGER, ONE HIT TOO MANY (plain YOU DIED stays as the fallback). Falls scream (SFX_SCREAM); wall deaths use the death sound. Ghosts are NOT done: see below. *Untested on hardware.*
9. **Scripted neighbor arrival at move-in** (M-L). The first neighbor (`twWel`) walks in about 3 seconds after you move to a lot or start a life, even at night, says "<NAME> SAYS WELCOME", and brings a housewarming gift of 25. Needs PRE-MADE SIMS and FREE WILL on, like every visit. *Untested on hardware.*

## Partly done
- **Multi-floor**: 3 floors with stairs, packed in `flPool`. Missing: Sims using stairs, the 3-floor render toggle and focus key.
- **Seasons**: the town map draws four seasons (`nbT.season`). Missing: weather, holidays, any effect while playing.
- **Skate park**: exists as a town lot type (`CT_SKATE`). Missing: AI skaters.
- **Roofs**: only drawn on houses in the town view. Missing: roof options in the house.
- **Neighbors**: visitors walk in, stay and leave (`twTick`) and now one welcomes you at move-in.
- **Relationships viewer**: the RELATIONSHIPS screen is in the pause menu. Missing: the hotkey / lower-bar macro.
- **Yards**: town lots have their own width and height, but the playable map is a fixed 40 x 40, so a lot has no yard size.
- **Voices**: 43 clips. Missing: female voices and a random voice per birth.

## Not started (no code found)
Windows and outside decor, TV channels, mirrors, pets, police and jail, new game+ / generations, `.bps` expansion packs, better air physics, extra animations, a surplus of items for every need, and the rotation decision (`cview` is used in about 27 places).
**Death and ghosts**: the old note said ghost code existed. It does not: the only "ghost" in the code is the creator's cursor preview and a career title. Lingering ghosts are a new system (an NPC-like sprite, a persistence flag, a save bit): L.

## Next, cheapest first
1. Put a RADIO and a SOUND SYSTEM in the default house and the pre-made lots (S, but old saved maps keep their layout).
2. Station display: show the song name under the station name (S).
3. Rotation decision (M): needs a decision first.
4. Radio stations as a jukebox category screen, TV channels (M): item ids plus clips (ROM cost).
5. Windows, outside decor, roof options (M-L).
6. Ghosts that stick around after death (L).
7. Mirrors (L), relationships viewer macro (L), yards (L), air physics (L).
8. Pets (XL), weather and seasons in play (XL), police and jail (XL), AI skate park (XL), new game+ (XL), `.bps` packs (XL, far future).

## RAM warning
IWRAM is nearly full (code + .bss about 30.7 KB of 32 KB; a check inlined into hot drawing code once crashed the game). New code should not be `IWRAM_CODE`. The sound pack adds no IWRAM and about 112 bytes of EWRAM (the item span table), plus about 1.2 KB of ROM for two sprites.
