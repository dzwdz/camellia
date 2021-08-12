#include <kernel/syscalls.h>
#include <stdint.h>

volatile char multi_buf[0x2000] __attribute__((section("text"))) = {0};

inline void _debuglog_hex(const char *buf, size_t len) {
	char hex[2];
	for (size_t i = 0; i < len; i++) {
		hex[0] = (buf[i] & 0xF0) >> 4;
		hex[0] += '0';
		if (hex[0] > '9')
			hex[0] += 'a' - '9' - 1;

		hex[1] = buf[i] & 0xF;
		hex[1] += '0';
		if (hex[1] > '9')
			hex[1] += 'a' - '9' - 1;
		
		_syscall_debuglog(hex, 2);
	}
}
#define _debuglog_var(var) _debuglog_hex((void*)&var, sizeof(var))

// takes a cstring and copies it right before a page boundary
const char *multipageify(const char *str) {
	char *to = ((uintptr_t)multi_buf & ~0xFFF) + 0xFFF;
	int asdf = 0x12345678;
	_debuglog_hex((void*)&asdf, 4);
	_syscall_debuglog(" a2s ", 5);
	do {
		*to++ = *str;
		_syscall_debuglog("!", 1);
	} while (*str++ != '\0');
	_syscall_debuglog(" a3s ", 5);
	return to;
}

int main() {
	// try to print a string crossing page boundaries
	_syscall_debuglog(" a1s ", 5);
	_syscall_debuglog(
		multipageify("I cross pages"),
		      sizeof("I cross pages") - 1);
	_syscall_debuglog(" a4s ", 5);

	_syscall_exit("bye from init! ",
	       sizeof("bye from init! ") - 1);
}
