// move this to proc.h, maybe?
#pragma once
#include <kernel/types.h>
#include <stddef.h>

size_t pcpy_to(Proc *p, __user void *dst, const void *src, size_t len);
size_t pcpy_from(Proc *p, void *dst, const __user void *src, size_t len);
size_t pcpy_bi(
	Proc *dstp, __user void *dst,
	Proc *srcp, const __user void *src,
	size_t len
);
