#include <kernel/arch/i386/interrupts/isr.h>
#include <kernel/arch/log.h>
#include <kernel/panic.h>
#include <stdbool.h>
#include <stdint.h>

#define log_n_panic(x) {log_const(x); panic();}

bool isr_test_interrupt_called = false;

void isr_stage3(int interrupt) {
	switch (interrupt) {
		case 0x08: log_n_panic("#DF"); // double fault
		case 0x0D: log_n_panic("#GP"); // general protection fault
		case 0x0E: log_n_panic("#PF"); // page fault

		case 0x34:
			isr_test_interrupt_called = true;
			return;

		default:
			log_const("unknown interrupt");
			panic();
	}
}
