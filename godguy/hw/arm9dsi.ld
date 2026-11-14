OUTPUT_ARCH(arm)
ENTRY(_start)
/* DSi mode, 16 MB. Code and read-only data low (the stack grows down from 0x022F0000); 0x02300000..0x023FFFFF is the ARM7 program, the IPC block and the uncached
   scratch block, so the big arrays (bss) start above it; 0x02C00000.. is the uncached packed-save block (crt0.s cache region 3). */
MEMORY { low (rwx) : ORIGIN = 0x02000800, LENGTH = 0x2F0000  big (rw) : ORIGIN = 0x02400000, LENGTH = 0x7F0000 }
SECTIONS {
	.text : { *(.crt0) *(.text .text.*) *(.rodata .rodata.*) *(.data .data.*) . = ALIGN(4); } > low
	.bss (NOLOAD) : { . = ALIGN(4); __bss_start = .; *(.bss .bss.*) *(COMMON) . = ALIGN(4); __bss_end = .; } > big
	__heap_start = ORIGIN(big) + LENGTH(big) - 0x10000; __end__ = __bss_end; __heap_limit = 0x02BFF000;
	/DISCARD/ : { *(.ARM.exidx*) *(.ARM.extab*) *(.comment) *(.note*) }
}
ASSERT(__bss_end <= __heap_start, "GodGuy DSi: the ARM9 data runs into the heap/save block - lower MAXC or the planet size in source/gg.h");
ASSERT(SIZEOF(.text) <= 0x2F0000, "GodGuy DSi: code too big for the low block");
