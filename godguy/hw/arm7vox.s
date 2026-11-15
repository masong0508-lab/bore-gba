@ The packed vocal chops (source/gmvox.bin, made by tools/gm_vox.py) ride in the ARM7 binary: it is loaded to 0x02380000, so this section lands at 0x02390000 (hw/arm7.ld), in uncached main RAM that the
@ ARM9 reads when a chop is struck. That keeps about 200 KB of audio out of the ARM9's own RAM (the DS has 4 MB and the game uses nearly all of it).
	.section .vox,"a"
	.balign 4
	.incbin "../source/gmvox.bin"
