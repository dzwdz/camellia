#include <kernel/arch/generic.h>
#include <kernel/panic.h>

int syscall_handler(int a, int b, int c, int d) {
	// verify that the parameters get passed correctly
	if (a != 1) panic();
	if (b != 2) panic();
	if (c != 3) panic();
	if (d != 4) panic();

	log_const("in a syscall!");

	// used to check if the return value gets passed correctly
	return 0x4e;
}
