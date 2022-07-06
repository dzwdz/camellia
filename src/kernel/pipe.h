#pragma once
#include <kernel/proc.h>

void pipe_joinqueue(struct handle *h, bool wants_write,
		struct process *proc, void __user *pbuf, size_t pbuflen);

void pipe_trytransfer(struct handle *h);
