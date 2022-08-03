#pragma once
#include <stdbool.h>
#include <stdint.h>

#define IRQ_IBASE 0x20
#define IRQ_PS2 1
#define IRQ_COM1 4

void irq_init(void);
void irq_eoi(uint8_t line);
