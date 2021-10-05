#include <kernel/arch/i386/interrupts/isr.h>
#include <kernel/arch/io.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <stdbool.h>
#include <stdint.h>

bool isr_test_interrupt_called = false;

/** Kills the process that caused the exception */
_Noreturn static void exception_finish(void) {
	// TODO check if the exception was in the kernel
	process_kill(process_current, 0); // TODO make the return code mean something
	process_switch_any();
}

void isr_stage3(int interrupt) {
	switch (interrupt) {
		case 0x08: // double fault
			tty_const("#DF");
			panic_invalid_state();
		case 0x0D: // general protection fault
			exception_finish();
		case 0x0E: { // page fault
			/*int cr2;
			tty_const("#PF at ");
			asm ("mov %%cr2, %0;" : "=r"(cr2) ::);
			_tty_var(cr2);*/
			exception_finish();
		}

		case 0x34:
			isr_test_interrupt_called = true;
			return;

		default:
			tty_const("unknown interrupt");
			panic_unimplemented();
	}
}
