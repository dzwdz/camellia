ENTRY(main)
OUTPUT_FORMAT("binary")

SECTIONS
{
	/* Not an ELF, but we need this to link with elfreloc.c.
	 * Not that we need to link with it, it's just easier that way */
	_DYNAMIC = 0;
	_image_base = 0;

	. = 2M;
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.text.startup)
		*(.text)
	}
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data)
	}

	. = ALIGN(32); /* seems to be necessary? */
	_initrd = .; /* is just appended onto the end of the binary */
	. += 2M;

	_bss_start = .;
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	_bss_end = .;
}
