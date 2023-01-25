#pragma once
#include <kernel/proc.h>
#include <stdbool.h>

/* eventually transitions to PS_RUNNING */
void pipe_joinqueue(Handle *h, Proc *proc, void __user *pbuf, size_t pbuflen);

void pipe_invalidate_end(Handle *h);
