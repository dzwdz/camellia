ENTRY(_start)

SECTIONS
{
	. = 1M;
	.text BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
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
	_data_end = .;
	.bss BLOCK(4K) : ALIGN(4K)
	{
		*(COMMON)
		*(.bss)

		stack_bottom = .;
		. += 16K;
		stack_top    = .;
	}
	_bss_end = (. + 0xFFF) & ~0xFFF; /* aligned to 4K */
}
