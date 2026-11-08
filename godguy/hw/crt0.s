@ ARM9 start-up: runs at 0x02000800 (the first 0x800 bytes of the ARM9 image are the decrypted "secure area" filler)
	.section .crt0,"ax"
	.arm
	.global _start
_start:
	msr	cpsr_c, #0xD3			@ supervisor mode, IRQ and FIQ off
	ldr	sp, =0x023EF000
	ldr	r0, =__bss_start
	ldr	r1, =__bss_end
	mov	r2, #0
1:	cmp	r0, r1
	strlo	r2, [r0], #4
	blo	1b
	bl	hwBoot
	bl	main
2:	b	2b
	.pool
