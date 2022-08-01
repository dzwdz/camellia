#pragma once
#include <stdint.h>

struct multiboot_mod {
	uint32_t start;
	uint32_t end;
	uint32_t str;
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

	uint32_t cmdline;

	uint32_t mods_count;
	uint32_t mods;

	uint8_t _padding[60];

	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint8_t framebuffer_bpp;
	uint8_t framebuffer_type;
	uint8_t color_info[6];
} __attribute__((packed));
