OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY { ram (rwx) : ORIGIN = 0x02000800, LENGTH = 0x3E0000
         bigram (rw) : ORIGIN = 0x02400000, LENGTH = 0x00800000 }
SECTIONS {
	.text : { *(.crt0) *(.text .text.*) *(.rodata .rodata.*) *(.data .data.*) . = ALIGN(4); } > ram
	.bss (NOLOAD) : { . = ALIGN(4); __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > ram
	__heap_start = .; __end__ = .;
	/* the DSi build's big arrays: above the DS's 4 MB, not cleared by the start-up code (the game clears them itself once it knows the RAM is really there) */
	.bigbss (NOLOAD) : { *(.bigbss) *(.bigbss.*) } > bigram
	/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.note*) }
}
