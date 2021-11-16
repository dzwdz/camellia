#include <init/fs/misc.h>
#include <init/stdlib.h>
#include <shared/flags.h>
#include <shared/mem.h>
#include <shared/syscalls.h>

bool fork2_n_mount(const char *path) {
	handle_t h = _syscall_fs_fork2();
	if (h) _syscall_mount(h, path, strlen(path));
	return h;
}

void fs_passthru(void) {
	struct fs_wait_response res;
	int buf_size = 64;
	char buf[      64];
	int ret;

	for (;;) {
		switch (_syscall_fs_wait(buf, buf_size, &res)) {
			case VFSOP_OPEN:
				ret = _syscall_open(buf, res.len);
				_syscall_fs_respond(NULL, ret);
				break;

			case VFSOP_READ:
				if (res.capacity > buf_size)
					res.capacity = buf_size; /* don't overflow the buffer */
				ret = _syscall_read(res.id, buf, res.capacity, res.offset);
				_syscall_fs_respond(buf, ret);
				break;

			// temporarily doesn't support writing
			// also TODO closing

			default:
				_syscall_fs_respond(NULL, -1);
				break;
		}
	}
}
