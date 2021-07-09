#pragma once

struct interrupt_frame;

__attribute__((interrupt))
void isr_double_fault(struct interrupt_frame *frame);
