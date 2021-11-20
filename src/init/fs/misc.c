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

static void fs_respond_delegate(struct fs_wait_response *res, handle_t delegate) {
	/* The idea behind this function is that many fs implementations (e.g. for
	 * overlay fs) will want to forward received requests to the original fs
	 * implementation.
	 *
	 * Having some special support for this in the kernel makes sense - it would
	 * avoid a lot of unnecessary copies. However, it wouldn't be necessary for
	 * a working kernel - and since I want the "base" of this system to be a small
	 * as possible, I want to have an alternative implementation of this in the
	 * userland which works in exactly the same way. The libc would then choose
	 * the best available implementation.
	 *
	 * (note: this wouldn't be an issue if i treated everything like functions
	 *        in the ruby prototype)
	 *
	 * This function was supposed to avoid unnecessary copies on delegated reads.
	 * It is being executed /after/ the write read, though. I don't currently see
	 * a good way to fix that. Here are a few loose ideas:
	 *   - a fs_wait flag which requires a separate syscall to get the write copy
	 *   - a way to "mark" returned handles
	 *     - no write copy mark
	 *     - mark with delegate handle
	 *         every vfs op would go directly to that delegate handle
	 *         probably the fastest way
	 *   - treat open() just like in the ruby prototype
	 *       Instead of just returning a handle, the driver would return a list
	 *       of functions. Here's how it would work, in pseudocode:
	 *          ```
	 *          # for full delegation
	 *          respond_new_handle({
	 *          	read:  {DELEGATE, handle}, // extract the read function from `handle`
	 *          	write: {DELEGATE, handle},
	 *          	close: {DELEGATE, handle}
	 *          });
	 *          ```
	 */
	static char buf[1024];
	int buf_size  = 1024;
	int size;
	int ret;

	switch (res->op) {
		case VFSOP_READ:
			// TODO instead of truncating the size, allocate a bigger buffer
			size = res->capacity < buf_size ? res->capacity : buf_size;
			ret = _syscall_read(delegate, buf, size, res->offset);
			_syscall_fs_respond(buf, ret);
			break;

		// TODO writing (see above)

		default:
			/* unsupported / unexpected */
			_syscall_fs_respond(NULL, -1);
			break;
	}
}

void fs_passthru(const char *prefix) {
	struct fs_wait_response res;
	int buf_size = 64;
	char buf[      64];
	int ret, prefix_len;
	if (prefix) prefix_len = strlen(prefix);

	while (!_syscall_fs_wait(buf, buf_size, &res)) {
		if (prefix && res.op == VFSOP_OPEN) {
			/* the only special case: rewriting the path */
			if (prefix_len + res.len <= buf_size) {
				// TODO memmove
				char tmp[64];
				memcpy(tmp, buf, res.len);
				memcpy(buf, prefix, prefix_len);
				memcpy(buf + prefix_len, tmp, res.len);
				ret = _syscall_open(buf, res.len + prefix_len);
			} else ret = -1;
			_syscall_fs_respond(NULL, ret);
		} else {
			fs_respond_delegate(&res, res.id);
		}
	}
	_syscall_exit(0);
}
