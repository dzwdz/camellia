.set MAGIC, 0x1BADB002
.set FLAG_ALIGN,   1<<0 /* align modules on page boundaries */
.set FLAG_MEMINFO, 1<<1 /* memory map */
.set FLAGS,        FLAG_ALIGN | FLAG_MEMINFO
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM
