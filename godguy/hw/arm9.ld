OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY { ram (rwx) : ORIGIN = 0x02000800, LENGTH = 0x3E0000 }
SECTIONS {
	.text : { *(.crt0) *(.text .text.*) *(.rodata .rodata.*) *(.data .data.*) . = ALIGN(4); } > ram
	.bss (NOLOAD) : { . = ALIGN(4); __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > ram
	__heap_start = .; __end__ = .;
	/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.note*) }
}
