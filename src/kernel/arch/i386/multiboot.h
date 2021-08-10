#pragma once
#include <stdint.h>

_Static_assert(sizeof(void*) == 4,
		"this code assumes that pointers have 4 bytes");

struct multiboot_mod {
	void *start;
	void *end;
	const char *str;
	uint32_t _reserved;
} __attribute__((packed));

struct multiboot_info {
	uint32_t flag_mem         : 1;
	uint32_t flag_boot_device : 1;
	uint32_t flag_cmdline     : 1;
	uint32_t flag_mods        : 1;
	uint32_t _flag_other      : 28; // unimplemented
	
	uint32_t mem_lower;
	uint32_t mem_upper;

	uint32_t boot_device;

	const char *cmdline;

	uint32_t mods_count;
	struct multiboot_mod *mods;

	// [...]
} __attribute__((packed));
