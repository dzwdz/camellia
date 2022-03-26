#pragma once
#include <stdbool.h>
#include <stdint.h>

void irq_init(void);
void irq_eoi(uint8_t line);

void irq_interrupt_flag(bool flag);
