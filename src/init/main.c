#include <kernel/syscalls.h>
#include <stddef.h>
#include <stdint.h>

int _syscall(int, int, int, int);

int debuglog(const char *msg, size_t len) {
	return _syscall(SC_DEBUGLOG, (void*)msg, len, 0);
}


int main() {
	debuglog("hello from init! ",
	  sizeof("hello from init! ") - 1);

	// try to mess with kernel memory
	uint8_t *kernel = (void*) 0x100000;
	*kernel = 0; // should segfault
}
