# GodGuy vs WorldBox: what is still different (15 November 2026)

Built from the four documents you attached (status and plans, addendum, "What to Add Next" PDF, the 118-creature xlsx) and the current `main`.
Sizes: S = a few hours, M = a day, L = several days. Risk is to old saves and to the war benchmark.

## 1. Creatures: the xlsx (118 creatures, WorldBox 0.51) against GodGuy

GodGuy has 8 peoples, 24 animal archetypes and 20 monster kinds in 16 colours. The roster gap, by group:

| WorldBox group | In the xlsx | In GodGuy | Gap |
|---|---|---|---|
| Classic civilised races | Human, Elf, Dwarf, Orc | all four | none |
| Special humanoids | Snowman, Evil Mage, White Mage, Necromancer, Druid, Plague Doctor, Cold One, Demon, Angel, Alien | Imp / demon-style monsters only | the mages, Necromancer, Druid, Plague Doctor, Cold One, Angel, Alien, Snowman |
| Normal animals (29) | Cat, Dog, Chicken, Rabbit, Monkey, Fox, Sheep, Cow, Armadillo, Raccoon, Wolf, Bear, Rhino, Buffalo, Hyena, Rat, Alpaca, Capybara, Goat, Penguin, Ostrich, Crab, Scorpion, Turtle, Crocodile, Snake, Frog, Piranha, Seal | 24 archetypes (sheep, wolf, eagle, lizard, fish ...) | about a dozen species have no archetype (Rhino, Buffalo, Hyena, Alpaca, Capybara, Penguin, Ostrich, Armadillo, Raccoon, Seal, Piranha, Monkey) |
| Evolved animals (28) | every normal animal has an evolved form (Meowmorph, Barkfolk, Pecklord ...) | none | the whole Pack D idea |
| Special animals (6) | Flower Bud, Crystal Sword, Bitba, Garl, Smore, Acid Blob and their evolved forms | none | all of them |
| Magical | Fairy, Unicorn (-> Punkcorn) | none | both |
| Insects | Bee, Butterfly, Beetle (-> Dunglord), Grasshopper, Fly | ambient life only (no creatures) | all five as real, tiny, mostly harmless creatures |
| Bosses and special monsters | Crabzilla, Dragon, UFO, Living Plant, Living House | Dragon, Kraken, Hydra, Troll ... | Crabzilla, UFO, Living Plant, Living House |
| Spawner monsters | Tumor Monster, Bioblob, Lil Pumpkin, Assimilator | none | all four (the tumour / biomass system) |
| Undead | Zombie, Skeleton, Ghost, MUSH | Zombie, MUSH | Skeleton, Ghost |
| Misc. monsters | Rude Skull, Fire Skull, Fire Elemental, Greg, Boat | Boat (done) | Skulls, Fire Elemental, Greg |
| Terraformers | God Finger, four Ants, Printer, Sand Spider, Worm | none | all of them (the ants build or eat terrain) |

The cheapest big win is **not** adding 90 sprites. Species in GodGuy are generated from archetype plus colour, so each missing animal is one archetype row: a body, a diet, a speed, a habitat, a size.

## 2. Mechanics still missing (merged from all the documents)

| # | Item | Source | Size | Save impact |
|---|---|---|---|---|
| 1 | Pack A rest: TRIBUTE and TAX policies, four new culture traits (Shattered Crown, Equal Heirs, Expansionist, Ethnocentric, Xenophile), the `KdX` / `GGK1` trailer, 20 more achievements in a u64 trailer | PDF 5 | M | new trailer, old saves default |
| 2 | Pack B: Inspiration, Whisper of War, Discord, Unity, Spite, Friendship, three Dusts, relations map lens, compare view, two-tap powers | PDF 6 | M | none |
| 3 | Interface items 1 to 4: "what is it doing?" line, hall of rulers, minimap zone modes, population by culture / religion / language | addendum | S each | none |
| 4 | Pack F: armies with captains, kings flee, empty cities fall, renown, war records | addendum | M | renown needs KdX |
| 5 | Pack G: loot rarity and modifiers (derived), monster drops, loot rain / corrosive rain | addendum | M | none |
| 6 | Acquired traits: Veteran, Kingslayer, Dragonslayer, Golden Tooth, Energized | addendum | S | `tr2` has room |
| 7 | Drowning (law-gated), snow slow with adaptation immunity, spawned folk inherit neighbours' culture | addendum | S each | none |
| 8 | New laws: Angry Villagers, Bandits, Evolution, Succession, King Stats, Gene Spaghetti, Mutant Box | PDF 9, addendum | S each | needs the law array widened |
| 9 | Pack C: Fire Elemental, Grey Goo, Crabzilla and Crab Bomb, bandits, ghosts of the notable dead, snowmen | PDF 7 | L | hazards in the GGH1 trailer |
| 10 | Pack D: SAPIENT trait, UPLIFT / monolith, animal dens | PDF 8, addendum | L | one trait bit |
| 11 | World events table (12+ weighted events by world age) | PDF 9 | S | none |
| 12 | Kingdom cap raised past 8 (needs a one-off save converter) | PDF 10 | M | converter |
| 13 | Remember zoom and camera in saves; selection history; species icon layer | addendum | S | next save version |
| 14 | Magic costs gold (ENCHANT, BLESS pay from a store) | addendum | S | none |
| 15 | Terraformer creatures (ants, worm, sand spider, printer, God Finger) | xlsx | M | creature kinds only |
| 16 | Tumor Monster, Bioblob, Assimilator, Lil Pumpkin (spawners) and biomass | xlsx | L | none |
| 17 | Insects as real creatures; the missing animals; evolved forms | xlsx | M to L | none |

## 3. Order I would take them in

1. Interface items 1 to 4 and acquired traits (no save changes, visible at once).
2. Pack A rest with the `GGK1` trailer and achievement headroom (everything after it needs the trailer).
3. Pack B (politics you can nudge), then world events and the new laws.
4. Pack G (loot, derived data, can go any time), then Pack F.
5. Missing animals and insects as archetype rows, then Pack C, then Pack D and the spawners.

## 4. Decisions you can change

Bandits and sapient animals ship **off** by default (as WorldBox leaves its harsher laws off); drowning and snow slow ship as **laws that start off**, so old saves play as before; the captain-death morale break needs the war benchmark re-recorded.
