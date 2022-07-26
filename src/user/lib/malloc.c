#include <shared/flags.h>
#include <shared/syscalls.h>
#include <stdbool.h>
#include <user/lib/malloc.h>

#include <user/lib/stdlib.h>

#define MBLOCK_MAGIC 0x1337BABE

struct mblock {
	uint32_t magic;
	size_t length; // including this struct
	bool used;
	struct mblock *next;
};

static struct mblock *first = NULL, *last = NULL;
static struct mblock *expand(size_t size);

void *malloc(size_t size) {
	struct mblock *iter = first;
	size += sizeof(struct mblock);
	while (iter) {
		if (!iter->used && iter->length >= size)
			break;
		iter = iter->next;
	}

	if (!iter) iter = expand(size);
	if (!iter) return NULL;

	iter->used = true;
	// TODO truncate and split

	return &iter[1];
}

void free(void *ptr) {
	struct mblock *block = ptr - sizeof(struct mblock);
	if (block->magic != MBLOCK_MAGIC) {
		// TODO debug log switch
		printf("didn't find MBLOCK_MAGIC @ 0x%x\n", block);
		return;
	}

	block->used = false;
}

static struct mblock *expand(size_t size) {
	struct mblock *block = _syscall_memflag(0, size, MEMFLAG_PRESENT | MEMFLAG_FINDFREE);
	if (!block) return NULL;

	block->magic = MBLOCK_MAGIC;
	block->length = size;
	block->used = false;
	block->next = NULL;

	if (!first) first = block;
	if (last) last->next = block;
	last = block;

	// TODO collapse

	return block;
}
