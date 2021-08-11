#include <kernel/syscalls.h>

#define STR_64  "This string has exactly 64 characters. It'll be repeated a bunch"
#define STR_256 STR_64  STR_64  STR_64  STR_64
#define STR_1K  STR_256 STR_256 STR_256 STR_256
#define STR_4K  STR_1K  STR_1K  STR_1K  STR_1K
#define STR_LNG "start " STR_4K STR_4K " finished! "

int main() {
	// try to print a string which is longer than a page
	_syscall_debuglog(STR_LNG,
	           sizeof(STR_LNG) - 1);

	_syscall_exit("bye from init! ",
	       sizeof("bye from init! ") - 1);
}
