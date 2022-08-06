#include <assert.h>
#include <kernel/arch/generic.h>

_Noreturn void __badassert(const char *func, const char *file, int line) {
	/* same format as panics */
	kprintf("\nan assert PANIC! at the %s (%s:%u)\n", func, file, line);
	debug_stacktrace();
	cpu_halt();
}
