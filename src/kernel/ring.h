#pragma once
// TODO merge into driver/util.h
#include <kernel/types.h>
#include <shared/container/ring.h>

size_t ring_to_virt(ring_t *r, Proc *proc, void __user *ubuf, size_t max);
