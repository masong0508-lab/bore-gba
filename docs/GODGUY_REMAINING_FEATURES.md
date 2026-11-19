# GODGUY: Remaining Features Batch

**Implementation note — 18 November 2026**
This document describes the follow-up mechanics batch. It supplements, rather than replaces, the original *What to Add Next* roadmap and [`GODGUY_NEXT_ADDENDUM.md`](GODGUY_NEXT_ADDENDUM.md). The current status checklist is [`../godguy/WORLDBOX_GAP_LIST.md`](../godguy/WORLDBOX_GAP_LIST.md).

## Player-facing changes

### Bee colonies

Bees now form local colonies. A colony elects a queen; workers follow the queen when they stray, and a living queen periodically produces another worker. Colony growth is limited to 24 nearby bees. The queen marker uses an existing creature-flag extension rather than enlarging the raw creature record.

### Autosave switch

The pause menu exposes **AUTOSAVE ON/OFF**. When OFF, the game skips automatic saves at year boundaries. This does not disable manual **SAVE** or the save-slot controls. The automatic-save gate also refuses to attempt a write when storage is unavailable.

### Divine powers

- **SEND HOME**: select a citizen and use the power to move it to a free, walkable tile beside the nearest city belonging to that citizen's people. If no suitable home city or tile is available, the power reports that it could not send the citizen home.
- **BORDER BRUSH**: use the power on a city to paint its visible perimeter; use it again to clear the perimeter. Border markers are associated with the city and are removed when that city is razed.

### Culture and species appearance

People use five palette variants derived from their culture family. Wildlife and monsters use deterministic species-family colour preferences. These choices are derived at runtime and do not add per-creature fields to the save format.

### Laws and world ages

- **GLITCHED NOOSPHERE** increases the number of randomly assigned traits when new cultures, languages, clans and religions are created.
- **DROP OF THOUGHTS** allows a life cloud to create a sapient creature.
- **RAT KING** allows a large mouse swarm to infect nearby citizens with plague.
- The age-name table contains ten names: **DARK, HOPE, SUN, TEARS, MOON, CHAOS, WONDERS, ICE, ASH, DESPAIR**. The presence of the labels does not by itself establish that every age balance has been playtested.

## Save compatibility

Save version 14 adds a `GGR2` trailer containing the four temporary captain-rout zones and their ring index. Versions 11, 12 and 13 remain accepted. The rout data is temporary simulation state; preserving it prevents a save/load boundary from changing the currently active rout zones. The bee queen marker uses the existing creature-flag extension, and the culture/species palette choices are derived rather than separately saved.

## Regression coverage

The new `pc_remaining.c` test exercises culture palette selection, SEND HOME, BORDER BRUSH, Drop of Thoughts, Rat King, bee queen election/worker spawning, autosave gating and captain-rout save/load. `pc_laws.c` also isolates starvation behaviour so the HUNGER-law assertion measures starvation rather than unrelated predation or movement. The test runner includes the remaining-feature regression alongside the existing law, weather, disaster and creature regressions.

These are host-side regression tests and layout checks. They are not a substitute for a fresh devkitARM hardware link, emulator boot, or real-console test.

## Still open

This batch does not claim to finish GODGUY. The remaining named creature work includes Navy Seals, planes, hypercube, ghost civilizations, cybercore, super pumpkin and the five fire-elemental kinds. Interface work still includes meta-object selection, zone multi-toggle and Ctrl+1–9 selection shortcuts. Further culture/religion/language schism behaviour and long-running balance checks also remain open. Consult the gap list for the current breakdown before planning another implementation batch.
