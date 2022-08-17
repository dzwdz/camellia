#pragma once
#include <stdint.h>
void rtl8139_init(uint32_t bdf);
void rtl8139_irq(void);
