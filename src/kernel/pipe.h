#pragma once
#include <kernel/proc.h>
#include <stdbool.h>

/* eventually transitions to PS_RUNNING */
void pipe_joinqueue(struct handle *h, struct process *proc, void __user *pbuf, size_t pbuflen);

void pipe_invalidate_end(struct handle *h);
