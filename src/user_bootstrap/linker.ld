ENTRY(main)
OUTPUT_FORMAT("binary")

SECTIONS
{
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
