// move this to proc.h, maybe?
#pragma once
#include <camellia/types.h>
#include <stddef.h>

struct process;

size_t pcpy_to(struct process *p, __user void *dst, const void *src, size_t len);
size_t pcpy_from(struct process *p, void *dst, const __user void *src, size_t len);
size_t pcpy_bi(
	struct process *dstp, __user void *dst,
	struct process *srcp, const __user void *src,
	size_t len
);
