#define ASM_FILE 1
#include "3rdparty/multiboot2.h"

.section .multiboot
.align 8
.set HEADERLEN, multiboot_header_end - multiboot_header
multiboot_header:
	.long MULTIBOOT2_HEADER_MAGIC
	.long MULTIBOOT_ARCHITECTURE_I386
	.long HEADERLEN
	.long -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + HEADERLEN)

	.align 8
	.short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
	.short 0
	.long 20
	.long 0
	.long 0
	.long 32

	.align 8
	.short MULTIBOOT_HEADER_TAG_MODULE_ALIGN
	.short 0
	.long 8

	.align 8
	.short MULTIBOOT_HEADER_TAG_END
	.short 0
	.long 8
multiboot_header_end:
