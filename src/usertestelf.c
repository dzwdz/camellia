#include <user/lib/syscall.c>

int main(void) {
	_syscall_write(1, "Hello!", 6, 0);
	_syscall_exit(0);
}
