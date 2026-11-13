@ ARM9 start-up: runs at 0x02000800 (the first 0x800 bytes of the ARM9 image are the decrypted "secure area" filler)
	.section .crt0,"ax"
	.arm
	.global _start
_start:
	msr	cpsr_c, #0xD3			@ supervisor mode, IRQ and FIQ off
	ldr	sp, =0x022F0000
	ldr	r0, =__bss_start
	ldr	r1, =__bss_end
	mov	r2, #0
1:	cmp	r0, r1
	strlo	r2, [r0], #4
	blo	1b
#ifdef GG_DSIBIG
	@ is the RAM above 4 MB really there? (in DS mode 0x02400020 is only a mirror of 0x02000020): caches are still off here, so a plain write and read tell. dsiAlias = 1 means DS mode.
	ldr	r0, =0x02000020
	ldr	r1, =0x02400020
	ldr	r2, [r0]
	mvn	r3, r2
	str	r3, [r1]
	ldr	r4, [r0]
	str	r2, [r0]
	cmp	r4, r3
	moveq	r4, #1
	movne	r4, #0
	ldr	r5, =dsiAlias
	str	r4, [r5]
#endif
#ifdef HW_CACHE
	@ instruction and data caches: region 0 = I/O + VRAM + OAM (never cached), region 1 = main RAM (cached), region 2 = the last megabyte from 0x02300000 (the ARM7 shared block and the
	@ save buffers; never cached, and it wins over region 1)
	mov	r0, #0
	mcr	p15, 0, r0, c7, c5, 0		@ invalidate the instruction cache
	mcr	p15, 0, r0, c7, c6, 0		@ invalidate the data cache
	ldr	r0, =0x04000033
	mcr	p15, 0, r0, c6, c0, 0
#ifdef GG_DSIBIG
	ldr	r0, =0x0200002F			@ DSi mode: all 16 MB of main RAM cached
	mcr	p15, 0, r0, c6, c1, 0
	ldr	r0, =0x02300027
	mcr	p15, 0, r0, c6, c2, 0
	ldr	r0, =0x02C0002B			@ region 3: 0x02C00000, 4 MB, never cached (the packed save the ARM7 reads)
	mcr	p15, 0, r0, c6, c3, 0
#else
	ldr	r0, =0x0200002B
	mcr	p15, 0, r0, c6, c1, 0
	ldr	r0, =0x02300027
	mcr	p15, 0, r0, c6, c2, 0
#endif
	mov	r0, #0x02
	mcr	p15, 0, r0, c2, c0, 0		@ data cacheable: region 1
	mcr	p15, 0, r0, c2, c0, 1		@ instructions cacheable: region 1
	mcr	p15, 0, r0, c3, c0, 0		@ write buffer: region 1
#ifdef GG_DSIBIG
	ldr	r0, =0x00003333
#else
	ldr	r0, =0x00000333
#endif
	mcr	p15, 0, r0, c5, c0, 2		@ data access: full, regions 0..2 (0..3 on the big DSi build)
	mcr	p15, 0, r0, c5, c0, 3		@ instruction access: full
	mrc	p15, 0, r0, c1, c0, 0
	ldr	r1, =0x1005			@ instruction cache, data cache, protection unit
	orr	r0, r0, r1
	mcr	p15, 0, r0, c1, c0, 0
#endif
	bl	hwBoot
	bl	main
2:	b	2b
	.pool
