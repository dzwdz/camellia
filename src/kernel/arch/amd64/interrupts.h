#pragma once
#include <stdbool.h>
#include <stdint.h>

#define IRQ_COM1 4
#define IRQ_IBASE 0x20
#define IRQ_PIT 0
#define IRQ_PS2KB 1
#define IRQ_PS2MOUSE 12
#define IRQ_RTL8139 11

extern void (*irq_fn[16])(void);
extern const char _isr_stubs;

void irq_init(void);
void irq_eoi(uint8_t line);
void isr_stage3(uint8_t interrupt, uint64_t *stackframe);
