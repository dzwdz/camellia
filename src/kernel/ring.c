#include <kernel/mem/virt.h>
#include <kernel/ring.h>

size_t ring_to_virt(ring_t *r, struct process *proc, void __user *ubuf, size_t max) {
	char tmp[32];
	if (max > sizeof tmp) max = sizeof tmp;
	max = ring_get(r, tmp, max);
	return pcpy_to(proc, ubuf, tmp, max);
}
