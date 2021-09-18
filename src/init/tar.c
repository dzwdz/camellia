#include <init/stdlib.h>
#include <shared/syscalls.h>
#include <stdint.h>

extern int tty_fd;

static int oct_parse(char *str, size_t len);

void tar_driver(void *base) {
	// iterate over all sectors, printing filenames
	while (0 == memcmp(base + 257, "ustar", 5)) {
		int size = oct_parse(base + 124, 12);

		_syscall_write(tty_fd, base, 100);
		_syscall_write(tty_fd, " ", 1);

		base += 512;                 // skip metadata sector
		base += (size + 511) & ~511; // skip file (size rounded up to 512)
		// TODO might pagefault if the last sector was at a page boundary
	}
	_syscall_write(tty_fd, "done.", 5);
}

static int oct_parse(char *str, size_t len) {
	int res = 0;
	for (size_t i = 0; i < len; i++) {
		res *= 8;
		res += str[i] - '0'; // no format checking
	}
	return res;
}
