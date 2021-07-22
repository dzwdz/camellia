#include <kernel/arch/generic.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <kernel/syscalls.h>
#include <stdint.h>

int sc_debuglog(const char *msg, size_t len) {
	struct pagedir *pages = process_current->pages;
	void *phys = pagedir_virt2phys(pages, msg, true, false);

	// page overrun check
	if (((uintptr_t)msg & PAGE_MASK) + len > PAGE_SIZE)
		len = PAGE_SIZE - ((uintptr_t)msg & PAGE_MASK);
	if (((uintptr_t)msg & PAGE_MASK) + len > PAGE_SIZE)
		panic(); // just in case I made an off by 1 error

	log_write(phys, len);
	return len;
}

int syscall_handler(int num, int a, int b, int c) {
	switch (num) {
		case SC_DEBUGLOG:
			return sc_debuglog((void*)a, b);
		default:
			log_const("unknown syscall ");
			panic();
	}
}