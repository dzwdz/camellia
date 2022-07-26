#pragma once
#include <camellia/types.h>

struct evil_sem {
	handle_t wait, signal;
};

void esem_signal(struct evil_sem *sem);
void esem_wait(struct evil_sem *sem);

struct evil_sem *esem_new(int value);
void esem_free(struct evil_sem *sem);
