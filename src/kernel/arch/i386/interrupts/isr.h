#pragma once
#include <stdbool.h>

struct interrupt_frame;

extern bool isr_test_interrupt_called; // used in the self-test in idt.c

__attribute__((interrupt))
void isr_double_fault(struct interrupt_frame *frame);

__attribute__((interrupt))
void isr_general_protection_fault(struct interrupt_frame *frame);

__attribute__((interrupt))
void isr_page_fault(struct interrupt_frame *frame);

__attribute__((interrupt))
void isr_test_interrupt(struct interrupt_frame *frame);
