Runs the real ARM9 image of the DSi-mode build (build_dsibig/arm9.bin) in the Unicorn CPU emulator (`pip install unicorn`, ARM946 model) with a made-up memory map: 16 MB of RAM (DSi mode) or 4 MB mirrored (DS mode), the I/O ranges as plain RAM and a VCOUNT that counts.
- `nm arm9.elf > syms.txt` first (path as in the scripts), then `python3 boot_probe.py dsi` / `python3 boot_probe.py ds`: DSi mode must reach newWorld and fill the big arrays above 4 MB; DS mode must set dsiAlias, print the message and never reach newWorld.
- `python3 save_path.py`: after boot it calls the game's real doSave with a fake "ARM7" and checks the packed save at 0x02C00000 decodes to the length and magic the header says.
This is not a DSi: no firmware, no real caches or protection-unit checks, no ARM7, no flashcart.
`save_sizes.c` measures packed save sizes (-DPCTEST, optionally -DGG_DSIBIG / -DGG_BIG).
