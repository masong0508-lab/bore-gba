# GODGUY special-entity event batch — 19 November 2026

## Player-facing controls

- **SPECIAL EVENT** cycles through six encounters on each use: Navy Seals, Planes, Hypercube, Ghost Civilization, Cybercore and Super Pumpkin.
- **FIRE ELEM.** cycles through Ember, Magma, Inferno, Plasma and Solar forms. The selected variant is stored in the existing hazard `pad` byte, so the form survives a save/load cycle.

## Encounter behaviour

- Navy Seals deploy armed seal units; the event can locally restore their health while they are in water.
- Planes deploy elite eagle units and make a moving bombing pass.
- Hypercube is a moving/teleporting anomaly that damages nearby creatures and occasionally scrambles terrain.
- Ghost Civilization deploys wraiths and drains nearby settlement stores; it also contributes to ghost pressure around cities.
- Cybercore deploys VR-headset and golem defenders, periodically reinforces its machines, and repairs nearby monsters.
- Super Pumpkin deploys a boss pumpkin, periodically adds pumpkin minions, and corrupts nearby ground.
- Ember burns a small radius; Magma can create lava; Inferno has a wider, stronger burn; Plasma jumps and emits lightning/dust; Solar has a gentler burn and locally repairs monsters.

## Compatibility and limits

The event kinds are appended after the existing hazard IDs and reuse the existing fixed-size `Hz` record. The save loader now accepts the new IDs; the record size and creature record size are unchanged. The power catalog grows from 78 to 79 entries, within the existing 80-byte favorite array and the v13 favorite trailer. The DS/DSi/3DS host regression covers the new power path, event spawns, fire variants and save/load.

These are **event prototypes**, not fully separate species-codex entries. Seal/eagle/wraith/monster units reuse existing archetypes and sprites; Hypercube is a hazard, not a creature. A true independently governed ghost civilization, dedicated new creature art/animation, and full standalone archetypes remain unfinished.
