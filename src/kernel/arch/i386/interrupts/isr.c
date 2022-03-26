#include <kernel/arch/i386/interrupts/irq.h>
#include <kernel/arch/i386/interrupts/isr.h>
#include <kernel/arch/i386/port_io.h>
#include <kernel/arch/i386/tty/keyboard.h>
#include <kernel/arch/io.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <stdbool.h>
#include <stdint.h>

bool isr_test_interrupt_called = false;

void isr_stage3(int interrupt) {
	switch (interrupt) {
		case 0x08: // double fault
			tty_const("#DF");
			panic_invalid_state();
		case 0x34:
			isr_test_interrupt_called = true;
			return;

		case 0x21:; // keyboard irq
			keyboard_recv(port_in8(0x60));
			irq_eoi(1);
			return;

		default:
			// TODO check if the exception was in the kernel
			process_kill(process_current, interrupt);
			process_switch_any();
	}
}
