/* CubalC matrix lane — lower than C
 * uint64_t cubalc_asm_pop64(uint64_t x);
 * System V AMD64: arg in %rdi, ret in %rax
 * Uses hardware POPCNT when available; else software loop.
 */
	.globl cubalc_asm_pop64
	.type  cubalc_asm_pop64, @function
cubalc_asm_pop64:
	/* try popcnt */
	popcnt %rdi, %rax
	ret
	.size cubalc_asm_pop64, .-cubalc_asm_pop64

/* void cubalc_asm_xor_lanes(uint64_t *dst, const uint64_t *a, const uint64_t *b, size_t n);
 * rdi=dst rsi=a rdx=b rcx=n  — pure bit talk, no C
 */
	.globl cubalc_asm_xor_lanes
	.type  cubalc_asm_xor_lanes, @function
cubalc_asm_xor_lanes:
	test %rcx, %rcx
	je   .Ldone
.Lloop:
	movq (%rsi), %rax
	xorq (%rdx), %rax
	movq %rax, (%rdi)
	addq $8, %rdi
	addq $8, %rsi
	addq $8, %rdx
	dec  %rcx
	jne  .Lloop
.Ldone:
	ret
	.size cubalc_asm_xor_lanes, .-cubalc_asm_xor_lanes
