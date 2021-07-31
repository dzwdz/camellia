#include <kernel/syscalls.h>

int main() {
	_syscall_debuglog("hello from init! ",
	           sizeof("hello from init! ") - 1);

	_syscall_fork();

	_syscall_debuglog("i got forked. ",
	           sizeof("i got forked. ") - 1);

	_syscall_exit("bye from init! ",
	       sizeof("bye from init! ") - 1);
}
