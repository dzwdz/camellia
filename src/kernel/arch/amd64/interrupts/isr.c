#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/interrupts/isr.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/arch/generic.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <stdbool.h>
#include <stdint.h>

bool isr_test_interrupt_called = false;

static void log_interrupt(int interrupt, uint64_t *stackframe) {
	kprintf("interrupt %x, rip = k/%08x, cs 0x%x, code 0x%x\n",
			interrupt, stackframe[0], stackframe[1], stackframe[-1]);
	if (interrupt == 0xe) {
		uint64_t addr = 0x69;
		asm("mov %%cr2, %0" : "=r"(addr));
		kprintf("addr 0x%x\n", addr);
	}
}

void isr_stage3(int interrupt, uint64_t *stackframe) {
	if (interrupt == 0xe || interrupt == 0xd) stackframe++;
	switch (interrupt) {
		case 0x34:
			asm("nop" ::: "memory");
			isr_test_interrupt_called = true;
			return;

		case 0x21: // keyboard irq
			ps2_recv(port_in8(0x60));
			irq_eoi(1);
			return;

		case 0x24: // COM1 irq
			serial_irq();
			irq_eoi(1);
			return;

		default:
			if ((stackframe[1] & 0x3) == 0) {
				log_interrupt(interrupt, stackframe);
				cpu_halt();
			} else {
				process_kill(process_current, interrupt);
				process_switch_any();
			}
	}
}
