#pragma once
#include <stdbool.h>
#include <stdint.h>

void ata_init(void);
bool ata_available(int drive);
int ata_read(int drive, uint32_t lba, void *buf);
