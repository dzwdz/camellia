#pragma once
#include <stddef.h>

struct kmain_info {
	struct {
		void *at; /* page aligned */
		size_t size;
	} init; /* a boot module loaded by GRUB, containing the initrd driver */
	struct {
		void *at;
		size_t size;
	} fb; /* the framebuffer */
	void *memtop;
};

void kmain(struct kmain_info);
void shutdown(void);
