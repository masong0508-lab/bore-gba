OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY { ram (rwx) : ORIGIN = 0x02000800, LENGTH = 0x3E0000 }
SECTIONS {
	.text : { *(.crt0) *(.text .text.*) *(.rodata .rodata.*) *(.data .data.*) . = ALIGN(4); } > ram
	.bss (NOLOAD) : { . = ALIGN(4); __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > ram
	__heap_start = .; __end__ = .;
	/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.note*) }
}
/* Stack is initialized at 0x02300000 in crt0.s and grows downward.
   Keep >=32 KiB between the end of .bss and that stack top.
   Previously the 20-Nov full build ended at 0x022F1C08, above the
   old 0x022F0000 stack, so every push could corrupt live globals. */
ASSERT(__bss_end <= 0x022F8000, "GodGuy: ARM9 code/data encroaches on the reserved 32 KiB stack; reduce MAXC or static buffers");
