.set MAGIC, 0x1BADB002

/* 1<<0  - align modules on page boundaries.
   1<<2  - enable graphic mode fields */
.set FLAGS, 1<<0 | 1<<2
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
multiboot_header:
	.long MAGIC
	.long FLAGS
	.long CHECKSUM

	.skip 5 * 4

	.long 0
	.long 0
	.long 0
	.long 32
