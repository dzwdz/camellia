#include <kernel/arch/i386/interrupts/isr.h>
#include <kernel/arch/log.h>
#include <kernel/panic.h>
#include <stdbool.h>
#include <stdint.h>

#define UNUSED __attribute__((unused))

bool isr_test_interrupt_called = false;

__attribute__((interrupt))
void isr_double_fault(UNUSED struct interrupt_frame *frame) {
	log_const("#DF");
	panic();
}

__attribute__((interrupt))
void isr_general_protection_fault(UNUSED struct interrupt_frame *frame) {
	log_const("#GP");
	panic();
}

__attribute__((interrupt))
void isr_page_fault(UNUSED struct interrupt_frame *frame) {
	log_const("#PF");
	panic();
}

__attribute__((interrupt))
void isr_test_interrupt(UNUSED struct interrupt_frame *frame) {
	isr_test_interrupt_called = true;
}
