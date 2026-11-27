BORE 1 SNES — Milestone 5
Prototype, not the full game.

Build from supplied SPC payloads: python3 build_milestone5.py bore1-m5.sfc
Run tests: python3 test_milestone5.py
Audio test: python3 verify_audio.py (requires numpy, soundfile and libgme.so.0)
The source builder includes earlier milestones. No external assembler is needed.

Controls: D-pad move, A use/strike, START save (outside combat), SELECT cycle base/four packs, X select companion campaign, Y add family, B remove family or leave arena, L/R switch the two native tracks. Home/town door at lower centre. Town mission desk at lower right.

SPC audio playback is independently tested in libgme. ROM instructions and modeled peripherals were tested, not a full SNES emulator. M4/M5 SRAM uses new record slots; old prototype saves are left intact, but not imported. No external PCM/MSU is required. The native music bank has eight-voice scheduling limits; simultaneous retriggers can coalesce, and Ticking Bomb has 18 dropped notes.

GBA V14 rollback baseline SHA256: adc952e998e08ec112164d68fc454d207d3b40a0083e52fe64fae54db1406493. All previous GBA content remains untouched by this SNES prototype.
