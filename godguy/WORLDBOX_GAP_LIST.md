# GodGuy vs WorldBox: what is still different (rewritten 15 November 2026)

## Creature-life batch implemented (18 November 2026)
The first creature-life batch has now been added to the shared game code: the 45-trait codex includes the biome adaptations and the missing traits in this area; the six listed statuses are available from a STATUS divine power; the Grin Mark affects nearby settlement happiness; Handsome Migrant steers people toward a nearby same-race settlement; Burning Feet scorches nearby vegetation and can hurt adjacent creatures. Disease remains in `Cr.sick` bits 0..1 and statuses use its six spare bits, so the DS creature record does not grow. Version 12 adds only a small `GGU1` trailer for high trait-discovery bits; version 11 saves still load. Automated PC-host regression tests exercise the DS, DSi and 3DS record layouts.


The earlier version of this file was out of date: most of what it listed as missing had already been built. This one was checked against the code (keyword search of `source/`, plus the tests) and against the official changelog (https://www.superworldbox.com/changelog, versions 0.21.0 to 0.51.4; about two thirds of the page was read). The Fandom wiki (Special:AllPages and every page tried) answers HTTP 402 from here, so wiki-only pages (the full power, trait and building lists) were NOT read. Anything marked *(unchecked)* was not verified in play.

## Built (do not redo)
- Peoples, 24+29 animals, monsters, terraformers (God Finger, four ants, worm, sand spider, printer), insects, magical animals, evolved forms (57 names), sapient animals with dens, tools, growing tribes and defence, spawner monsters (tumor, bioblob, pumpkin, assimilator), Crabzilla, grey goo, fire elemental, ghosts, snowmen, bandits (law), VR headset (was Greg).
- Kingdoms (up to 16), rulers with stats, ruler hall, tribute and tax, culture traits, religions, languages, clans, alliances, wars, plots, vassals, renown, war records, kings flee, captains and the captain-down rout, loot rarity and modifiers, monster drops, loot and corrosive rain, Pack B powers (inspire, whisper, discord, unity, spite, friendship, dusts), monoliths and uplift, laws (succession, king stats, spaghetti, snow, bandits, evolution, drowning, magic).
- Interface: "what is it doing?", hall of rulers, map tint (kingdom, city, culture, faith, language, relations), map dots by kind, population page, world browser with back, zoom saved, favourites.

## Still missing (from the changelog, 0.21.0 to 0.51.4)
| Area | Missing |
|---|---|
| Traits | species-specific preferred colours and remaining WorldBox-only visual/behaviour variants; the listed adaptation, defense, healing, social and fire traits are now implemented |
| Statuses | rat-mediated plague transmission (ordinary plague spread and Cough status exist); any further one-click status variants not covered by the six new toggles |
| Weather and clouds | ash cloud, magic cloud, rage cloud, heatwave |
| Disasters | alien invasion, dragon from far lands, moon horrors, biomass monstrosity (a big tumor event), ice disasters from dreams, garden surprise |
| Creatures | Navy Seals, planes, hypercube, ghosty ghosts (ghost civilisations), cybercore, super pumpkin, the five fire-elemental kinds, bees with a matriarchy |
| Mechanics | unit possession; teleporting units return to their home city; five culture-based skins per people; species preferred colours; border brush; only xenophobic cultures destroy cities and the rest capture them *(our war code unchecked)*; species loyalty checks and culture/religion/language splits (0.50.3) *(partly: schisms exist, unchecked)*; random traits for new cultures, languages, clans and religions |
| Laws | Glitched Noosphere, Drop of Thoughts, Grin Reaper |
| Powers | the six printer powers (we have the printer creature only), small and medium lightning, clan, alliance, war and plot displays *(partly)* |
| Ages | the changelog lists ten ages (dark, hope, sun, tears, moon, chaos, wonders, ice, ash, despair); the code has "age" strings: *check all ten exist* |
| Interface (0.51.0) | select any meta object (kingdom, city, culture) on the map with its own power tab; multi-toggle for zones; Ctrl+1-9 save and recall selections; save and autosave buttons in the quit menu |

## Known weak spots (not WorldBox gaps)
- DS build has about 13 KB of RAM to spare; the DS holds only 7 of the 56 CORPORATIONS vocal phrases (3DS and DSi mode hold all).
- Old saves (before version 11) do not load.
- Not run on real hardware: DS, DSi mode, 3DS, the new 3DS mixer.
- Audio was measured, not heard: CORPORATIONS timing and tuning, Duck & Cover pitch, the 3DS vocal bus.
- Terraformer rates, healing amounts and sapient tribe caps are first guesses. The 25 new 8 x 8 sprites were looked at once; five redrawn.

## Suggested order
1. Clouds, heatwave and the extra disasters (extend the weather and hazard code).
2. Missing creatures and the printer powers.
3. Possession, skins, preferred colours, teleport home.
4. Interface items (meta selection, zone multi-toggle, save slots in the pause menu).
5. Verify rat-mediated plague transmission and the remaining species-specific behaviour gaps.
6. Paste the wiki AllPages text (or the pages' text) so the unread pages can be compared.
