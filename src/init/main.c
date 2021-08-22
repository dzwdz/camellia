#include <kernel/syscalls.h>
#include <stdint.h>

// takes a cstring and copies it right before a page boundary
const char *multipageify(const char *str) {
	static char buf[0x2000] __attribute__((section("text")));

	char *out = (void*)((uintptr_t)buf & ~0xFFF) + 0xFFF;
	char *cur = out;
	do {
		*cur++ = *str;
	} while (*str++ != '\0');
	return out;
}

#define mount(path) _syscall_mount(path, sizeof(path)-1, 0)

int main() {
	mount("/");
	mount("/some/where");
	mount("/some");

	_syscall_fs_open(
			multipageify("/some/where/file"),
			      sizeof("/some/where/file") - 1);

	_syscall_exit("bye from init! ",
	       sizeof("bye from init! ") - 1);
}
