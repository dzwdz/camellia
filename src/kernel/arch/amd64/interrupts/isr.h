#pragma once
#include <stdbool.h>
#include <stdint.h>

extern bool isr_test_interrupt_called; // used in the self-test in idt.c
extern const char _isr_stubs;

void isr_stage3(int interrupt, uint64_t *stackframe);
