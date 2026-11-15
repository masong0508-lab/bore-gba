# GODGUY: What to Add Next, addendum (14 November 2026)

Adds to the roadmap PDF "GODGUY: What to Add Next" (packs A to E). Same rules as there: integer maths only, nothing new in a raw-saved struct, every feature ships with a `pc_*.c` test.

## 1. Built since the roadmap

| Item | What it does | Where | Test |
|---|---|---|---|
| Speed hotkey | B + A cycles 1x / 2x / 4x in play | `main.c` main loop | `pc_ds`, `pc_fix` |
| Map pan | B + stylus drag pans the 2D map, normal and zoomed (same gesture that spins the globe); nothing is painted while B is held | `main.c` main loop | `pc_pan.c` |
| **Pack A1: ruler stats** | Every ruler has STEWARDSHIP, WARFARE, DIPLOMACY, INTELLIGENCE (1 to 10), worked out from the ruler's name seed (`rn1`, `rn2`) plus a growth term from `reign` (+1 at 40 years, +2 at 80, +3 at 120). Nothing is saved, so old saves load unchanged and a new ruler changes the stats for free. | `kdRStat()` in `main.c` | `pc_rulers.c` |
| Pack A5: coronation chronicle | A journal line each time a ruler is crowned | `kingdomTick` | `pc_rulers.c` |

What the stats do now:

- **Stewardship:** city cap +1 per 6 points (`kdMaxCities`); city loyalty target +2 per point above 5, -2 per point below.
- **Warfare:** kingdom power +3% per point above 5 (-3% below); stacks with MARTIAL.
- **Diplomacy:** a kingdom's opinion of every other kingdom +1 per point above 5.
- **Intelligence:** tech cost -2% per point above 5 (+2% below), next to the SCHOLARLY and Education discounts.
- The kingdom card (WORLD > KINGS and Extras > Kingdoms) shows `STEW / WAR / DIP / INT`.

Still open in Pack A: tribute and tax policies, the four new culture traits (Shattered Crown, Equal Heirs, Expansionist, Ethnocentric, Xenophile), and the KdX / GGK1 save trailer they need. The stat effects are small on purpose (about 10 to 15 percent at the extremes), so two kingdoms of one people now differ without any one stat deciding a war.

## 2. Corrections to the roadmap

- **WorldBox version.** The roadmap says 0.51.2. The official changelog now lists **0.51.4 "SteadyBox" (25 May 2026)** as the latest, after 0.51.3 (security and translations). 0.51.2 and 0.51.1 (12 Sept 2025) were fixes plus two new alien rites and a "Metamorphed" trait. The big content update remains 0.51.0 "ArmyBox" (11 Sept 2025).
- **Wiki access.** The Fandom `Special:AllPages` link still fails from here (client error), as it did for the roadmap and for your README. Everything below comes from the official changelog, the 0.51.0 patch notes on SteamDB, and two guides. Numbers marked *(verify)* were not confirmed on the wiki itself.

## 3. New findings and differences, mapped onto GODGUY

### Pack F: Armies and Deeds (medium, low risk)
WorldBox 0.51.0 made **armies** objects of their own, with a **captain** who rallies troops and makes them wait before marching, and army stats that track captains. 0.50.4 and 0.50.5 added **renown** to wars, alliances and kingdoms (war win shown on the banner; new wars need about 50 renown and alliance plots about 60 *(verify)*), and let wars be sorted by renown, length and army size.

- **F1 Captains.** `wgBuild` already makes one record per war group. Name one notable per group its captain (highest kills), show it on the inspect card, and have a captain's death break morale (soldiers of the group flee for a few sweeps). Uses `nb[]`, no new saved bytes.
- **F2 Kings flee.** 0.51.0: a king tries to leave a city before it falls. In `vilConquer`, if the ruler lives in the captured capital, 50% chance the ruler survives and the kingdom keeps a new capital elsewhere; otherwise the kingdom loses its ruler (forces a `NEW RULER`, which now also re-rolls the stats). Journal line either way.
- **F3 Empty cities fall.** 0.50.4: a city attacked with no defenders is destroyed, not captured. Check `vilConquer`: if it hands over a city with no defenders, raze it instead when `pop` is below a threshold, so wars do not give away free cities (not verified in the code yet).
- **F4 Renown.** A `u16` per kingdom in KdX (GGK1 trailer): +1 per battle won, +10 per war won, +5 per alliance formed. Gate war and alliance plots on it, as WorldBox does, so tiny kingdoms stop declaring wars on empires.
- **F5 War records.** A WARS list in the JOURNAL area (winner, length, casualties). Cheap, makes wars readable.

### Pack G: Loot and Legends (medium, low risk)
WorldBox items carry up to five **modifiers** (balance, finesse, health, knowledge, mastery, strength, protection, sharpness, speed, truth) at levels 1 to 5, and a **rarity** from the highest level (Normal, Rare, Epic, Legendary). Special weapons drop from specific monsters (a flame sword from demons, staffs from the mages, a blaster from aliens). GODGUY has 364 table-built items and no loot.

- **G1 Rarity and modifiers, derived.** Hash the item index to give each item a rarity and up to three modifiers. Nothing saved; the GIFT picker and inspect card show `+STR +SPD` style tags. A legendary sword of the same material is just better, so gifts matter more.
- **G2 Monster drops.** A slain boss or elite drops its signature item in the nearest village's store (or on the ground as a pickup); the 20 monster kinds each get one. Uses the stock field, no new arrays.
- **G3 Loot rain power.** WorldBox has rains that add or remove items (0.51.0: the corrosive one strips cursed items). GODGUY: LOOT RAIN gifts a weapon or armour piece to creatures in a brush; CORROSIVE strips gear. Fits the Wrath and Gift tabs.

### Acquired traits from deeds (small)
In WorldBox traits are earned: *Veteran* at 11 kills, *Kingslayer*, *Dragonslayer*, *Wise* (30% at age 40), *Golden Tooth* (5% for a king at 45), *Energized* (20% if struck by lightning), *Infected* by a zombie. GODGUY's notables already count kills and titles; add the slayer and veteran traits as notable titles and let lightning have a small chance to give a spark trait. No new fields (`tr2` has room).

### World mechanics (small to medium; put the harsh ones behind LAWS)
- **Drowning.** 0.51.0 added a drowning status. Non-swimmers pushed into deep water by a TSUNAMI or a flood die at once today; a short drowning timer (a few ticks, helped by a boat or by landing) makes floods something people can run from. Law-gated.
- **Snow slows, adaptation helps.** Snow and permafrost slow units; desert, swamp and permafrost adaptation traits grant immunity. GODGUY has the traits and the biomes, so add the slow in `effSpd` for creatures without the matching trait (the fast path already exists).
- **Spawned folk inherit the neighbours' culture.** 0.51.0: spawned units take the culture, language and religion of nearby same-species units. GODGUY's SPAWN and SETTLE power should do this, or new villages start without a culture. A playability fix more than a feature.
- **New world laws** from 0.50.4: *Gene Spaghetti* (more mutations), *Mutant Box* (traits filtered), *Glitched Noosphere* and *Drop of Thoughts* (odd mind effects). GODGUY maps cleanly to GENE SPAGHETTI (mutation rate x3) and MUTANT BOX (only chill traits). There are six law slots, five already claimed in roadmap E3: a law-array resize goes with them.
- **Magic costs gold.** 0.50.4 made rites cost more gold. For the ENCHANT and BLESS powers: a village or kingdom that uses a rite pays from its store. Optional.

### Pack D gets stronger evidence
0.50.5 and 0.51.0 let **monoliths** evolve bipeds, ghosts form civilizations, and bees default to matriarchy. So evolving beyond the first eight peoples is a core WorldBox idea, not an extra. Suggestion for D1: make UPLIFT a **MONOLITH building** a village can raise (a temple variant) that, over years, gives the SAPIENT trait to animals near it, instead of only a power. A world-shaping structure fits the temple code and the gifts and avoids a ninth people.

### Interface ideas worth taking (small each, mostly no sim risk)
From the 0.51.0 interface list, ranked by value on a DS:

1. **"What is it doing?" line** on the inspect card (the unit task display). Players cannot tell why a creature stands still.
2. **Hall of rulers**: past rulers of a kingdom (WorldBox keeps past leaders and past names). Free given `rn1`, `rn2` and the journal.
3. **Minimap zone modes**: borders by kingdom, city, culture, religion or language (the zone toggle). The legend already exists; add a mode key.
4. **Population breakdown** by culture, religion and language on the kingdom card.
5. **Selection history / back** for the WORLD browser (the browser already jumps the camera).
6. **Remember zoom and camera in saves** (WorldBox saves them). `camX`, `camY` are in the save header; I did not find the zoom level there (verify). Fits the next save-version bump.
7. **Species icons layer** on the minimap.

## 4. Updated build order

1. Finish Pack A (policies, culture traits, KdX trailer) as `godguy-port-rulers-crowns`, with E1 (achievement headroom).
2. Pack B (divine meddling), as in the roadmap.
3. **Interface items 1 to 4 above** (no save changes, one patch).
4. **Pack F** (needs KdX for renown).
5. **Pack G** (derived data, so it can ship before F if you prefer).
6. Pack E leftovers, Pack C, then D1 (monolith) and D2.

## 5. Questions

1. Should rulers' stats be visible only on the kingdom card, or also on the inspect card of the ruler's creature (it adds a line)?
2. Pack G: items with random-looking rarity make gifting less predictable. Fine, or show rarity only when inspected?
3. Drowning and snow slow change balance in every existing save. On by default, or two new LAWS that start off?
4. Pack F's captain-death morale break changes the near-camera war numbers; it needs the war benchmark re-recorded (8 seeds). OK to do that?

## 6. Sources

- WorldBox changelog (0.51.4 to 0.50.4): https://superworldbox.com/changelog
- WorldBox 0.51.0 ArmyBox patch notes (SteamDB): https://steamdb.info/patchnotes/19939104
- WorldBox traits guide (Destructoid): https://www.destructoid.com/how-to-unlock-all-traits-in-worldbox/
- WorldBox tools, weapons and armour guide: https://worldboxguide.blogspot.com/2024/03/tools-weapons-and-armor-explained-in.html
- Your roadmap PDF and `godguy/` source (`main.c`, `ggx.h`, `ggid.h`, `ggmenu.h`).
