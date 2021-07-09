#include <kernel/isr.h>
#include <kernel/tty.h>
#include <stdint.h>

__attribute__((interrupt))
void isr_double_fault(struct interrupt_frame *frame) {
	tty_const("#DF");
	for(;;);
}
