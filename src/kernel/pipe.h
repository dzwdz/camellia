#pragma once
#include <kernel/proc.h>
#include <stdbool.h>

/* returns false on failure */
bool pipe_joinqueue(struct handle *h, bool wants_write,
		struct process *proc, void __user *pbuf, size_t pbuflen);

void pipe_trytransfer(struct handle *h);

void pipe_invalidate_end(struct handle *h);
