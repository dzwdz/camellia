#include <kernel/arch/i386/interrupts/isr.h>
#include <kernel/arch/log.h>
#include <kernel/panic.h>
#include <stdbool.h>
#include <stdint.h>

#define log_n_panic(x) {tty_const(x); panic_unimplemented();} // TODO kill the current process instead of panicking

bool isr_test_interrupt_called = false;

void isr_stage3(int interrupt) {
	switch (interrupt) {
		case 0x08: log_n_panic("#DF"); // double fault
		case 0x0D: log_n_panic("#GP"); // general protection fault
		case 0x0E: { // page fault
			int cr2;
			tty_const("#PF at ");
			asm ("mov %%cr2, %0;" : "=r"(cr2) ::);
			_tty_var(cr2);
			panic_unimplemented();
		}

		case 0x34:
			isr_test_interrupt_called = true;
			return;

		default:   log_n_panic("unknown interrupt");
	}
}
