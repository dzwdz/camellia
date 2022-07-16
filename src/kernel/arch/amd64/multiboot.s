.set MAGIC, 0x1BADB002

/* 1<<0  - align modules on page boundaries. */
.set FLAGS,        1<<0
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
multiboot_header:
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
