ENTRY(_start)
OUTPUT_FORMAT("binary")

SECTIONS
{
	. = 0x20000;
	_bss_start = .;
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)
	}
	_bss_end = .;

	. = 2M;
	.text BLOCK(4K) : ALIGN(4K)
	{
		/* the assert needs to be inside of a section or it'll be a syntax error */
		ASSERT(_bss_end <= 2M, "bss too big, needs to be moved down");
		*(.text.startup)
		*(.text)
	}
	.rodata BLOCK(4K) : ALIGN(4K)
	{
		*(.rodata)
	}
	.data BLOCK(4K) : ALIGN(4K)
	{
		*(.data*)
	}
	_initrd = .; /* is just appended onto the end of the binary */
}
