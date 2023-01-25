#include <kernel/arch/amd64/interrupts.h>
#include <kernel/arch/amd64/port_io.h>
#include <kernel/arch/generic.h>
#include <kernel/panic.h>
#include <kernel/proc.h>
#include <stdbool.h>
#include <stdint.h>

void (*irq_fn[16])(void) = {0};

static void log_interrupt(int interrupt, uint64_t *stackframe) {
	kprintf("interrupt %d, rip = k/%08x, cs 0x%x, code 0x%x\n",
			interrupt, stackframe[0], stackframe[1], stackframe[-1]);
	if ((stackframe[1] & 0x3) == 0) {
		uint64_t *stack = (void*)stackframe[3];
		kprintf("kernel rip = %p, *rip = %p\n", stack, *stack);
	}
	if (interrupt == 0xe) {
		uint64_t addr = 0x69;
		asm("mov %%cr2, %0" : "=r"(addr));
		kprintf("addr 0x%x\n", addr);
	}
}

void isr_stage3(uint8_t interrupt, uint64_t *stackframe) {
	uint8_t irqn = interrupt - IRQ_IBASE;
	if (irqn < 16) {
		if (irq_fn[irqn]) {
			irq_fn[irqn]();
			irq_eoi(irqn);
			return;
		}
	}

	if (interrupt == 0xe || interrupt == 0xd) {
		stackframe++;
	}

	if ((stackframe[1] & 0x3) == 0) { /* in kernel */
		log_interrupt(interrupt, stackframe);
		cpu_halt();
	} else { /* in user */
		proc_kill(proc_cur, interrupt);
		proc_switch_any();
	}
}
