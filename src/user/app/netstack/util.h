#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(fmt, ...) fprintf(stderr, "netstack: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

uint32_t crc32(const uint8_t *buf, size_t len);
uint16_t ip_checksum(const uint8_t *buf, size_t len);
/* 0 on success, negative failure */
int ip_parse(const char *s, uint32_t *ip);

static inline void nput16(void *vbuf, uint16_t n) {
	uint8_t *b = vbuf;
	b[0] = n >> 8;
	b[1] = n >> 0;
}

static inline void nput32(void *vbuf, uint32_t n) {
	uint8_t *b = vbuf;
	b[0] = n >> 24;
	b[1] = n >> 16;
	b[2] = n >>  8;
	b[3] = n >>  0;
}

static inline uint16_t nget16(const void *vbuf) {
	const uint8_t *b = vbuf;
	return (b[0] << 8)
	     | (b[1] << 0);
}

static inline uint32_t nget32(const void *vbuf) {
	const uint8_t *b = vbuf;
	return (b[0] << 24)
	     | (b[1] << 16)
	     | (b[2] <<  8)
	     | (b[3] <<  0);
}
