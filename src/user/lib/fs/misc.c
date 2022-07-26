#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <stdbool.h>
#include <user/lib/fs/misc.h>
#include <user/lib/stdlib.h>

bool fork2_n_mount(const char *path) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h) > 0) { /* parent */
		_syscall_mount(h, path, strlen(path));
		close(h);
		return true;
	}
	return false;
}

void fs_passthru(const char *prefix) {
	struct fs_wait_response res;
	const size_t buf_len = 1024;
	char *buf = malloc(buf_len);
	int prefix_len = prefix ? strlen(prefix) : 0;
	if (!buf) _syscall_exit(1);

	while (!_syscall_fs_wait(buf, buf_len, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				if (prefix) {
					if (prefix_len + res.len > buf_len) {
						_syscall_fs_respond(NULL, -1, 0);
						break;
					}

					// TODO memmove
					char tmp[64];
					memcpy(tmp, buf, res.len);
					memcpy(buf + prefix_len, tmp, res.len);
					memcpy(buf, prefix, prefix_len);
					res.len += prefix_len;
				}
				_syscall_fs_respond(NULL, _syscall_open(buf, res.len, res.flags), FSR_DELEGATE);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}
	_syscall_exit(0);
}

void fs_whitelist(const char **list) {
	struct fs_wait_response res;
	const size_t buf_len = 1024;
	char *buf = malloc(buf_len);
	bool allow;
	if (!buf) _syscall_exit(1);

	while (!_syscall_fs_wait(buf, buf_len, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				allow = false;
				// TODO reverse dir_inject
				for (const char **iter = list; *iter; iter++) {
					size_t len = strlen(*iter); // inefficient, whatever
					if (len <= res.len && !memcmp(buf, *iter, len)) {
						allow = true;
						break;
					}
				}
				_syscall_fs_respond(NULL, allow ? _syscall_open(buf, res.len, res.flags) : -1, FSR_DELEGATE);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}
	_syscall_exit(0);
}


void fs_dir_inject(const char *path) {
	struct fs_dir_handle {
		const char *inject;
		int delegate, inject_len;
	};

	const size_t path_len = strlen(path);
	struct fs_wait_response res;
	struct fs_dir_handle *data;
	const size_t buf_len = 1024;
	char *buf = malloc(buf_len);
	int ret, inject_len;

	if (!buf) _syscall_exit(1);

	while (!_syscall_fs_wait(buf, buf_len, &res)) {
		data = res.id;
		switch (res.op) {
			case VFSOP_OPEN:
				if (buf[res.len - 1] == '/' &&
						res.len < path_len && !memcmp(path, buf, res.len))
				{
					/* opening a directory that we're injecting into */

					data = malloc(sizeof *data);
					data->delegate = _syscall_open(buf, res.len, res.flags);
					data->inject = path + res.len;

					/* inject up to the next slash */
					inject_len = 0;
					while (data->inject[inject_len] && data->inject[inject_len] != '/')
						inject_len++;
					if (data->inject[inject_len] == '/')
						inject_len++;
					data->inject_len = inject_len;

					_syscall_fs_respond(data, 0, 0);
				} else {
					_syscall_fs_respond(NULL, _syscall_open(buf, res.len, res.flags), FSR_DELEGATE);
				}
				break;

			case VFSOP_CLOSE:
				if (data->delegate >= 0)
					close(data->delegate);
				_syscall_fs_respond(NULL, 0, 0);
				break;

			case VFSOP_READ:
				if (res.offset > 0) _syscall_fs_respond(NULL, 0, 0); // TODO working offsets

				int out_len = data->inject_len;
				memcpy(buf, data->inject, out_len);
				buf[out_len++] = '\0';

				if (data->delegate >= 0) {
					int to_read = res.capacity < buf_len ? res.capacity : buf_len;
					to_read -= out_len;
					ret = _syscall_read(data->delegate, buf + out_len, to_read, 0);
					if (ret > 0) out_len += ret;
					// TODO deduplicate entries
				}

				_syscall_fs_respond(buf, out_len, 0);
				break;

			case VFSOP_WRITE:
				if (data->delegate >= 0)
					ret = _syscall_write(data->delegate, buf, res.len, res.offset);
				else
					ret = -1;
				_syscall_fs_respond(NULL, ret, 0);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}
	_syscall_exit(0);
}
