#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void ata_init(void);
bool ata_available(int drive);
size_t ata_size(int drive);
int ata_read(int drive, void *buf, size_t len, size_t off);
int ata_write(int drive, const void *buf, size_t len, size_t off);
