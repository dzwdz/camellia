#include <kernel/mem/virt.h>
#include <kernel/ring.h>

size_t ring_to_virt(ring_t *r, struct pagedir *pages, void __user *ubuf, size_t max) {
	char tmp[32];
	if (max > sizeof tmp) max = sizeof tmp;
	max = ring_get(r, tmp, max);
	// TODO decide how write errors should be handled
	virt_cpy_to(pages, ubuf, tmp, max);
	return max;
}
