#include <kernel/arch/generic.h>
#include <kernel/panic.h>

void syscall_handler() {
	log_const("in a syscall!");
}
