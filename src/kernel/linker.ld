ENTRY(_start)

SECTIONS
{
	. = 0;
	.shared BLOCK(4K) : ALIGN(4K)
	{
		*(.multiboot)
		*(.shared)
		_shared_len = .;
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

		. += 16K;
		_isr_big_stack = .;

		. += 16K;
		_stack_top = .;
	}
	_bss_end = (. + 0xFFF) & ~0xFFF; /* aligned to 4K */
}
