#include <camellia/syscalls.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define eprintf(fmt, ...) fprintf(stderr, "ethdump: "fmt"\n" __VA_OPT__(,) __VA_ARGS__)

void hexdump(void *vbuf, size_t len) {
	uint8_t *buf = vbuf;
	for (size_t i = 0; i < len; i += 16) {
		printf("%08x  ", i);

		for (size_t j = i; j < i + 8 && j < len; j++)
			printf("%02x ", buf[j]);
		printf(" ");
		for (size_t j = i + 8; j < i + 16 && j < len; j++)
			printf("%02x ", buf[j]);
		printf("\n");
	}
}

int main(void) {
	const char *path = "/kdev/eth";
	handle_t h = _syscall_open(path, strlen(path), 0);
	if (h < 0) {
		eprintf("couldn't open %s", path);
		return 1;
	}

	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	for (;;) {
		long ret = _syscall_read(h, buf, buflen, -1);
		if (ret < 0) break;
		printf("packet of length %u\n", ret);
		hexdump(buf, ret);
	}
}
