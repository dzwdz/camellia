#include <init/fs/misc.h>
#include <init/stdlib.h>
#include <shared/flags.h>
#include <shared/mem.h>
#include <shared/syscalls.h>
#include <stdbool.h>

bool fork2_n_mount(const char *path) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h) > 0) { /* parent */
		_syscall_mount(h, path, strlen(path));
		_syscall_close(h);
		return true;
	}
	return false;
}

static void fs_respond_delegate(struct fs_wait_response *res, handle_t delegate, const char *og_buf) {
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
	int size;
	int ret;

	switch (res->op) {
		case VFSOP_READ:
			if (_syscall_fork(FORK_NOREAP, NULL)) {
				// handle reads in a child
				// this is a HORRIBLE workaround for making concurrent IO work without proper delegates
				break;
			}
			// TODO instead of truncating the size, allocate a bigger buffer
			size = res->capacity < sizeof(buf) ? res->capacity : sizeof(buf);
			ret = _syscall_read(delegate, buf, size, res->offset);
			_syscall_fs_respond(buf, ret);
			_syscall_exit(0);
			break;

		// TODO proper writing (see above)
		case VFSOP_WRITE:
			ret = _syscall_write(delegate, og_buf, res->len, res->offset);
			_syscall_fs_respond(NULL, ret);
			break;

		case VFSOP_CLOSE:
			_syscall_close(delegate);
			// isn't it kinda weird that i even have to respond to close()s?
			// i suppose it makes the API more consistent
			_syscall_fs_respond(NULL, 0);
			break;

		default:
			/* unsupported / unexpected */
			_syscall_fs_respond(NULL, -1);
			break;
	}
}

void fs_passthru(const char *prefix) {
	struct fs_wait_response res;
	static char buf[1024];
	int ret, prefix_len;
	if (prefix) prefix_len = strlen(prefix);

	while (!_syscall_fs_wait(buf, sizeof(buf), &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				if (prefix) {
					/* special case: rewriting the path */
					if (prefix_len + res.len <= sizeof(buf)) {
						// TODO memmove
						char tmp[64];
						memcpy(tmp, buf, res.len);
						memcpy(buf, prefix, prefix_len);
						memcpy(buf + prefix_len, tmp, res.len);
						ret = _syscall_open(buf, res.len + prefix_len, res.flags);
					} else ret = -1;
				} else {
					ret = _syscall_open(buf, res.len, res.flags);
				}
				_syscall_fs_respond(NULL, ret);
				break;

			default:
				fs_respond_delegate(&res, res.id, buf);
				break;
		}
	}
	_syscall_exit(0);
}


void fs_dir_inject(const char *path) {
	struct fs_dir_handle {
		bool taken;
		int delegate;
		const char *inject;
	};

	const size_t path_len = strlen(path);
	struct fs_wait_response res;
	struct fs_dir_handle handles[16] = {0}; // TODO hardcoded FD_MAX - use malloc instead
	static char buf[1024];
	int ret;

	while (!_syscall_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				int hid = -1;
				for (int hid2 = 0; hid2 * sizeof(*handles) < sizeof(handles); hid2++) {
					if (!handles[hid2].taken) {
						hid = hid2;
						break;
					}
				}
				if (hid < 0) _syscall_fs_respond(NULL, -2); // we ran out of handles

				ret = _syscall_open(buf, res.len, res.flags); /* errors handled in inject handler */
				handles[hid].delegate = ret;
				handles[hid].inject = NULL;
				handles[hid].taken = true;

				if (buf[res.len - 1] == '/' &&
						res.len < path_len && !memcmp(path, buf, res.len)) {
					handles[hid].inject = path + res.len;

					/* if we're making up the opened directory, disallow create */
					if (ret < 0 && (res.flags & OPEN_CREATE)) hid = ret;
				} else {
					/* not injecting, don't allow opening nonexistent stuff */
					if (ret < 0) hid = ret;
				}
				_syscall_fs_respond(NULL, hid);
				break;

			case VFSOP_CLOSE:
				if (handles[res.id].delegate >= 0)
					_syscall_close(handles[res.id].delegate);
				handles[res.id].taken = false;
				_syscall_fs_respond(NULL, 0);
				break;

			case VFSOP_READ:
				if (handles[res.id].inject) {
					if (res.offset > 0) _syscall_fs_respond(NULL, 0); // TODO working offsets
					struct fs_dir_handle h = handles[res.id];

					int out_len = 0;
					while (h.inject[out_len] && h.inject[out_len] != '/')
						out_len++;
					if (h.inject[out_len] == '/')
						out_len++;
					memcpy(buf, h.inject, out_len);
					buf[out_len++] = '\0';

					if (h.delegate >= 0) {
						int to_read = res.capacity < sizeof(buf) ? res.capacity : sizeof(buf);
						to_read -= out_len;
						ret = _syscall_read(h.delegate, buf + out_len, to_read, 0);
						if (ret > 0) out_len += ret;
						// TODO deduplicate entries
					}

					_syscall_fs_respond(buf, out_len);
					break;
				}

			/* fallthrough */

			default: {
				struct fs_dir_handle h = handles[res.id];
				if (h.delegate < 0)
					_syscall_fs_respond(NULL, -1);
				else
					fs_respond_delegate(&res, h.delegate, buf);
				break;
			}
		}
	}
	_syscall_exit(0);
}
