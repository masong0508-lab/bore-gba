# GodGuy vs WorldBox: what is still different (rewritten 15 November 2026)

## Creature-life batch implemented (18 November 2026)
The first creature-life batch has now been added to the shared game code: the 45-trait codex includes the biome adaptations and the missing traits in this area; the six listed statuses are available from a STATUS divine power; the Grin Mark affects nearby settlement happiness; Handsome Migrant steers people toward a nearby same-race settlement; Burning Feet scorches nearby vegetation and can hurt adjacent creatures. Disease remains in `Cr.sick` bits 0..1 and statuses use its six spare bits, so the DS creature record does not grow. Version 12 adds only a small `GGU1` trailer for high trait-discovery bits; version 11 saves still load. Automated PC-host regression tests exercise the DS, DSi and 3DS record layouts.


The earlier version of this file was out of date: most of what it listed as missing had already been built. This one was checked against the code (keyword search of `source/`, plus the tests) and against the official changelog (https://www.superworldbox.com/changelog, versions 0.21.0 to 0.51.4; about two thirds of the page was read). The Fandom wiki (Special:AllPages and every page tried) answers HTTP 402 from here, so wiki-only pages (the full power, trait and building lists) were NOT read. Anything marked *(unchecked)* was not verified in play.

## Built (do not redo)
- Peoples, 24+29 animals, monsters, terraformers (God Finger, four ants, worm, sand spider, printer), insects, magical animals, evolved forms (57 names), sapient animals with dens, tools, growing tribes and defence, spawner monsters (tumor, bioblob, pumpkin, assimilator), Crabzilla, grey goo, fire elemental, ghosts, snowmen, bandits (law), VR headset (was Greg).
- Kingdoms (up to 16), rulers with stats, ruler hall, tribute and tax, culture traits, religions, languages, clans, alliances, wars, plots, vassals, renown, war records, kings flee, captains and the captain-down rout, loot rarity and modifiers, monster drops, loot and corrosive rain, Pack B powers (inspire, whisper, discord, unity, spite, friendship, dusts), monoliths and uplift, laws (succession, king stats, spaghetti, snow, bandits, evolution, drowning, magic).
- Interface: "what is it doing?", hall of rulers, map tint (kingdom, city, culture, faith, language, relations), map dots by kind, population page, world browser with back, zoom saved, favourites.

## Climate batch implemented (18 November 2026)
The four special weather states are now part of the normal weather cycle. Ash clouds add atmospheric dust and dry vegetation; magic clouds heal creatures and encourage forest growth; rage clouds damage creatures and can scorch forests; heatwaves add dust, dry wetlands/vegetation and stress creatures. Their weather IDs use the existing saved weather byte, so no creature or save-header layout expansion is needed. The world map, globe lighting/cloud threshold, weather particles and status label recognize the new states.

## World-disaster batch implemented (18 November 2026)
Six more events now run through the existing `Hz` roaming-hazard table and `GGH1` save trailer: alien invasion (ALIEN/UFO wave), far-land elder dragon (or wyvern fallback), moon horrors (wraiths and a lich), spreading biomass (tumor monsters and bioblobs), dream ice (a moving freeze front that can extinguish nearby fire elementals), and garden surprise (a moving healing and regrowth front). Each has a divine power, world effects, map marker, and a rare natural-event chance when disasters are enabled. The `Hz` and `Cr` record sizes are unchanged. Save v13 adds an optional `GGF1` tail for starred powers 64–79; v11/v12 saves still load and default the new favorite slots to off. Regression tests exercise all six on DS, DSi and 3DS/PC record layouts, including save/load and the actual power path.

## Special-entity event batch implemented (19 November 2026)
The SPECIAL EVENT power now cycles through six additional world encounters: Navy Seal deployment (armed seal units), plane squadron (elite flying units), hypercube anomaly (teleporting terrain/damage pulse), ghost-civilization incursion (wraiths and settlement drain), cybercore (VR-headset/golem defenders with local regeneration), and super pumpkin (boss pumpkin plus recurring minions). The existing saved `Hz` record is reused; the new kinds are accepted by save/load and have map markers. The existing FIRE ELEM. power now cycles five saved variants—ember, magma, inferno, plasma and solar—with distinct pulse, terrain, teleport, damage and regeneration effects.

**Scope note:** these are playable event prototypes built from existing seal/eagle/wraith/monster units and the existing hazard system. They are not yet seven fully independent creature archetypes with dedicated animation sets; a true ghost kingdom with its own culture, cities and diplomacy is still open. The five fire forms are hazard variants, not five separate entries in the species codex. Do not mark the full named-creature roster complete on this basis.

## Remaining-mechanics batch implemented (18 November 2026)
- SEND HOME is a divine power: tapping a citizen returns it to a free walkable tile beside the nearest city of its people.
- BORDER BRUSH paints and toggles a visible perimeter for a city; border markers are owned by that city and are removed if it is razed.
- People use five culture-derived palette skins inside their kingdom; wildlife and monsters favour deterministic species-specific coat colours.
- Same-race soldiers check the kingdoms of their nearby home cities before treating another same-race unit as an enemy; they fight only when those kingdoms are at war.
- New worlds clear temporary captain-rout zones. Save version 14 adds a `GGR2` trailer for the four rout zones and their ring index; v11–v13 saves remain supported.

## Bee matriarchy implemented (18 November 2026)
Bee colonies establish one local queen; workers follow her, and a living queen periodically lays another worker, up to 24 nearby bees. Queen identity uses an existing creature-flag bit and does not enlarge the creature or save record. Regression tests cover queen election and worker spawning on DS, DSi and 3DS/PC layouts.

## Autosave control implemented (18 November 2026)
The pause menu now exposes AUTOSAVE ON/OFF. Turning it off suppresses only automatic year-boundary writes; manual SAVE GAME and save-slot controls remain available. The `autoSaveDue()` decision is covered for enabled, disabled and no-card cases in the remaining-mechanics regression.

## Still missing (from the changelog, 0.21.0 to 0.51.4)
| Area | Missing |
|---|---|
| Traits | remaining WorldBox-only visual/behaviour variants; species-specific preferred colours and five culture skins are now implemented |
| Statuses | rat-mediated plague transmission and the Rat King law are now implemented; further one-click status variants not covered by the six new toggles remain |
| Weather and clouds | Further polish: tune balance and verify on hardware. |
| Disasters | the six listed world events are implemented; remaining work is balancing, visual polish and on-hardware verification |
| Creatures | The six special encounters and five fire-elemental hazard variants now have playable prototypes; dedicated creature archetypes, art/animations and a true ghost kingdom remain open |
| Mechanics | unit possession, SEND HOME, five culture skins, preferred colours, border brush, xenophobic razing, same-race kingdom loyalty and random traits for culture/language/clan/religion creation are implemented. Remaining: culture/religion/language schisms (0.50.3) *(partly: schisms exist, unchecked)* |
| Laws | Glitched Noosphere and Drop of Thoughts are implemented. Grin Reaper was removed in WorldBox 0.51.0, so it is intentionally not a target. Rat King is also implemented as a plague law. |
| Powers | six printer powers and small/medium lightning are implemented; clan, alliance, war and plot displays remain *(partly)* |
| Ages | all ten named ages (dark, hope, sun, tears, moon, chaos, wonders, ice, ash, despair) are present; balance and on-hardware behaviour remain unchecked |
| Interface (0.51.0) | select any meta object (kingdom, city, culture) on the map with its own power tab; multi-toggle for zones; Ctrl+1-9 save and recall selections. Save/load/save slots are in the pause menu; automatic year-boundary saves now have an ON/OFF toggle (manual save remains available). GodGuy now has city selection through Field Tools → Survey and three session map pins in World → Pins. Direct culture and kingdom map selection, region editing and keyboard selection shortcuts remain. |

## Known weak spots (not WorldBox gaps)
- DS build has about 13 KB of RAM to spare; the DS holds only 7 of the 56 CORPORATIONS vocal phrases (3DS and DSi mode hold all).
- Old saves (before version 11) do not load.
- Not run on real hardware: DS, DSi mode, 3DS, the new 3DS mixer.
- Audio was measured, not heard: CORPORATIONS timing and tuning, Duck & Cover pitch, the 3DS vocal bus.
- Terraformer rates, healing amounts and sapient tribe caps are first guesses. The 25 new 8 x 8 sprites were looked at once; five redrawn.

## Suggested order
1. Finish dedicated creature archetypes, art/animations and true ghost-kingdom simulation for the new special-event prototypes.
2. Interface items (direct culture and kingdom selection, region editing and keyboard shortcuts).
3. Verify the new life-cloud and Rat King laws in long-running simulations and on hardware.
4. Paste the wiki AllPages text (or the pages' text) so the unread pages can be compared.
