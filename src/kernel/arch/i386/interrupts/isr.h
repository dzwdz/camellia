#pragma once
#include <stdbool.h>

extern bool isr_test_interrupt_called; // used in the self-test in idt.c
extern const char _isr_stubs;

void isr_stage3(int interrupt);
