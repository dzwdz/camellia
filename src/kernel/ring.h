#pragma once
// TODO merge into driver/util.h
#include <shared/container/ring.h>
struct pagedir;

size_t ring_to_virt(ring_t *r, struct pagedir *pages, void __user *ubuf, size_t max);
