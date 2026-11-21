@ Additional complete Corporations vocal phrases for regular 4-MiB
@ DS and DSi-compatible ROMs. Bank is preloaded at 0x02300000 by
@ the ROM bootloader and never overwritten by saving.
    .section .voxhi,"a"
    .balign 4
    .incbin "../source/gmvox_hi.bin"
