#include <kernel/arch/amd64/driver/ps2.h>
#include <kernel/arch/amd64/driver/rtl8139.h>
#include <kernel/arch/amd64/driver/serial.h>
#include <kernel/arch/amd64/interrupts/irq.h>
#include <kernel/arch/amd64/interrupts/isr.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/arch/amd64/time.h>
#include <kernel/arch/generic.h>
#include <kernel/mem/alloc.h>
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
			isr_test_interrupt_called = true;
			return;

		case IRQ_IBASE + IRQ_PIT:
			pit_irq();
			irq_eoi(IRQ_PIT);
			return;

		case IRQ_IBASE + IRQ_PS2KB:
		case IRQ_IBASE + IRQ_PS2MOUSE:
			ps2_irq();
			irq_eoi(interrupt - IRQ_IBASE);
			return;

		case IRQ_IBASE + IRQ_COM1:
			serial_irq();
			irq_eoi(IRQ_COM1);
			return;

		case IRQ_IBASE + IRQ_RTL8139:
			rtl8139_irq();
			irq_eoi(interrupt - IRQ_IBASE);
			return;

		default:
			if ((stackframe[1] & 0x3) == 0) {
				mem_debugprint();
				log_interrupt(interrupt, stackframe);
				cpu_halt();
			} else {
				process_kill(process_current, interrupt);
				process_switch_any();
			}
	}
}
