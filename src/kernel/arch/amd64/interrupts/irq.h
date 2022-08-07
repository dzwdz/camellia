#pragma once
#include <stdbool.h>
#include <stdint.h>

#define IRQ_IBASE 0x20
#define IRQ_PIT 0
#define IRQ_PS2KB 1
#define IRQ_COM1 4
#define IRQ_PS2MOUSE 12

void irq_init(void);
void irq_eoi(uint8_t line);
