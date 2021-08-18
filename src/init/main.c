#include <kernel/syscalls.h>
#include <stdint.h>

// takes a cstring and copies it right before a page boundary
const char *multipageify(const char *str) {
	static char buf[0x2000] __attribute__((section("text")));

	char *out = ((uintptr_t)buf & ~0xFFF) + 0xFFF;
	char *cur = out;
	do {
		*cur++ = *str;
	} while (*str++ != '\0');
	return out;
}

int main() {
	// try to print a string crossing page boundaries
	_syscall_debuglog(
		multipageify("I cross pages. "),
		      sizeof("I cross pages. ") - 1);

	if (_syscall_fork() > 0) {
		_syscall_debuglog("parent ",
		           sizeof("parent ") - 1);
	} else {
		_syscall_debuglog("child ",
		           sizeof("child ") - 1);
	}

	_syscall_exit("bye from init! ",
	       sizeof("bye from init! ") - 1);
}
