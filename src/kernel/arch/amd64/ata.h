#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ATA_SECTOR 512

void ata_init(void);
bool ata_available(int drive);
size_t ata_size(int drive);
int ata_read(int drive, uint32_t lba, void *buf);
