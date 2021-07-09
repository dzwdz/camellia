#include <kernel/isr.h>
#include <kernel/tty.h>
#include <stdbool.h>
#include <stdint.h>

bool isr_test_interrupt_called = false;

__attribute__((interrupt))
void isr_double_fault(struct interrupt_frame *frame) {
	tty_const("#DF");
	for(;;);
}

__attribute__((interrupt))
void isr_test_interrupt(struct interrupt_frame *frame) {
	isr_test_interrupt_called = true;
}
