#pragma once
#include <stdlib.h>

void thread_creates(int flags, void (*fn)(void*), void *arg, void *stack);

static inline void thread_create(int flags, void (*fn)(void*), void *arg) {
	/* error checking is for WIMPS. */
	thread_creates(flags, fn, arg, malloc(4096) + 4096);
}
