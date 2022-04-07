#include <init/driver/driver.h>
#include <init/fs/misc.h>
#include <init/shell.h>
#include <init/stdlib.h>
#include <init/tar.h>
#include <init/tests/main.h>
#include <shared/flags.h>
#include <shared/syscalls.h>
#include <stdint.h>

// TODO move to shared header file
#define argify(str) str, sizeof(str) - 1

extern char _bss_start; // provided by the linker
extern char _bss_end;
extern char _initrd;

void read_file(const char *path, size_t len);

__attribute__((section(".text.startup")))
int main(void) {
	// allocate bss
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);


	MOUNT("/init", tar_driver(&_initrd));
	MOUNT("/keyboard", ps2_drv());
	MOUNT("/vga_tty", ansiterm_drv());

	MOUNT("/bind", fs_passthru(NULL));

	if (!_syscall_fork()) {
		__tty_fd = _syscall_open(argify("/com1"));
		if (__tty_fd < 0) _syscall_exit(1);

		shell_loop();
		_syscall_exit(1);
	}

	if (!_syscall_fork()) {
		__tty_fd = _syscall_open(argify("/vga_tty"));
		if (__tty_fd < 0) _syscall_exit(1);

		shell_loop();
		_syscall_exit(1);
	}

	// try to find any working output
	__tty_fd = _syscall_open(argify("/com1"));
	if (__tty_fd < 0) __tty_fd = _syscall_open(argify("/vga_tty"));

	for (;;) {
		_syscall_await();
		printf("init: something quit\n");
	}

	_syscall_exit(0);
}
