#pragma once
#include <stdint.h>

#define PCICFG_CMD 0x4

uint8_t pcicfg_r8(uint32_t bdf, uint32_t offset);
uint16_t pcicfg_r16(uint32_t bdf, uint32_t offset);
uint32_t pcicfg_r32(uint32_t bdf, uint32_t offset);

void pcicfg_w16(uint32_t bdf, uint32_t offset, uint32_t value);
void pcicfg_w32(uint32_t bdf, uint32_t offset, uint32_t value);

uint16_t pcicfg_iobase(uint32_t bdf);

void pci_init(void);
