/*
   This is a minimal linker script for the ramloaded code.
   Note how it pleasantly just throws everything together. It is
   somewhat inspired from the imxrt1062_t41.ld but not really.
*/

/* The load_addr is changed before executing. */
LOAD_ADDR = 0x20002654;

SECTIONS
{
	.text LOAD_ADDR :
	{
		*(.text)
		*(.text.*)
		
		. = ALIGN(32);
		*(.flashmem*)
		*(.progmem*)
		*(.fastrun)
		
		__preinit_array_start = .;
		KEEP (*(.preinit_array))
		__preinit_array_end = .;
		__init_array_start = .;
		KEEP (*(.init_array))
		__init_array_end = .;
		. = ALIGN(32);
		
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.rodata*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.data*)))
		KEEP(*(.vectorsram))
		
		. = ALIGN(32);

		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
		*(SORT_BY_ALIGNMENT(SORT_BY_NAME(COMMON)))

		. = ALIGN(32);
		KEEP(*(.csf))
	}
}
