ENTRY(_start)

SECTIONS
{
	. = 0;
	.text.early BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.text.early)
		_text_early_len = .;
	}

	. = 1M;
	.text BLOCK(4K) : ALIGN(4K)
	{
		_kern_start = .;
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

		_stack_bottom = .;
		. += 16K;
	}
	_bss_end = (. + 0xFFF) & ~0xFFF; /* aligned to 4K */
}
