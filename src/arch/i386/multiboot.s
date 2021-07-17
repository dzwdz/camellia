.set MAGIC, 0x1BADB002
/* TODO set bss_end_addr, so the init module doesn't get overriden by the stack */

/* 1<<0  - align modules on page boundaries.
   1<<16 - enable manual addressing */
.set FLAGS,        1<<0 | 1<<16
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
multiboot_header:
	.long MAGIC
	.long FLAGS
	.long CHECKSUM
	.long multiboot_header // header_addr
	.long multiboot_header // load_addr
	.long _data_end        // load_end_addr
	.long _kernel_end      // bss_end_addr
	.long _start           // entry_addr
