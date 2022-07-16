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

void isr_stage3(int interrupt, uint64_t *stackframe) {
	if (interrupt == 0xe) stackframe++;
	kprintf("interrupt %x, rip = k/%08x, cs 0x%x\n", interrupt, stackframe[0], stackframe[1]);
	switch (interrupt) {
		case 0x08: // double fault
			kprintf("#DF");
			panic_invalid_state();

		case 0xe:
			uint64_t addr = 0x69;
			asm("mov %%cr2, %0" : "=r"(addr));
			kprintf("error code 0x%x, addr 0x%x\n", stackframe[-1], addr);
			panic_unimplemented();

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
			// TODO check if the exception was in the kernel
			process_kill(process_current, interrupt);
			process_switch_any();
	}
}
