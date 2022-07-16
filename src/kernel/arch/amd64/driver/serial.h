#pragma once
#include <kernel/vfs/request.h>
#include <stdbool.h>
#include <stddef.h>

void serial_preinit(void);
void serial_irq(void);
void serial_write(const char *buf, size_t len);

void serial_init(void);
