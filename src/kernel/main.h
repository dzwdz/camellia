#pragma once
#include <stddef.h>

struct kmain_info {
	struct {
		void *at;    // page aligned
		size_t size;
	} init; // a boot module loaded by GRUB, containing the initrd driver
	void *memtop;
};

void kmain(struct kmain_info);
