#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <user/lib/compat.h>
#include <user/lib/fs/dir.h>
#include <user/lib/fs/misc.h>

bool fork2_n_mount(const char *path) {
	handle_t h;
	if (_syscall_fork(FORK_NEWFS, &h) > 0) { /* parent */
		_syscall_mount(h, path, strlen(path));
		close(h);
		return true;
	}
	return false;
}

static int dir_seglen(const char *path) {
	/* in human terms:
	 * if path contains /, return its position + 1
	 * otherwise, return strlen */
	int len = 0;
	while (path[len]) {
		if (path[len] == '/') {
			len++;
			break;
		}
		len++;
	}
	return len;
}

void forward_open(handle_t reqh, const char *path, long len, int flags) {
	// TODO use threads
	// TODO solve for more complex cases, e.g. fs_union
	/* done in a separate thread/process because open() can block,
	 * but that should only hold the caller back, and not the fs driver.
	 *
	 * for example, running `httpd` in one term would prevent you from doing
	 * basically anything on the second term, because fs_dir_inject would be
	 * stuck on open()ing the socket */
	if (!_syscall_fork(FORK_NOREAP, NULL)) {
		_syscall_fs_respond(reqh, NULL, _syscall_open(path, len, flags), FSR_DELEGATE);
		exit(0);
	}
	close(reqh);
}

void fs_passthru(const char *prefix) {
	const size_t buflen = 1024;
	char *buf = malloc(buflen);
	int prefix_len = prefix ? strlen(prefix) : 0;
	if (!buf) exit(1);

	for (;;) {
		struct fs_wait_response res;
		handle_t reqh = _syscall_fs_wait(buf, buflen, &res);
		if (reqh < 0) break;
		switch (res.op) {
			case VFSOP_OPEN:
				if (prefix) {
					if (prefix_len + res.len > buflen) {
						_syscall_fs_respond(reqh, NULL, -1, 0);
						break;
					}

					memmove(buf + prefix_len, buf, res.len);
					memcpy(buf, prefix, prefix_len);
					res.len += prefix_len;
				}
				forward_open(reqh, buf, res.len, res.flags);
				break;

			default:
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	exit(0);
}

void fs_whitelist(const char **list) {
	const size_t buflen = 1024;
	char *buf = malloc(buflen);
	if (!buf) exit(1);
	for (;;) {
		struct fs_wait_response res;
		handle_t reqh = _syscall_fs_wait(buf, buflen, &res);
		if (reqh < 0) break;

		char *ipath = res.id;
		size_t blen;
		bool passthru, inject;
		struct dirbuild db;

		switch (res.op) {
			case VFSOP_OPEN:
				passthru = false;
				inject = false;

				for (const char **iter = list; *iter; iter++) {
					size_t len = strlen(*iter);
					bool ro = false;
					if (len >= 3 && !memcmp(*iter + len - 3, ":ro", 3)) {
						ro = true;
						len -= 3;
					}
					if (len <= res.len && !memcmp(buf, *iter, len)) {
						if (ro) res.flags |= OPEN_RO;
						passthru = true;
						break;
					}
					if (res.len < len &&
						buf[res.len - 1] == '/' &&
						!memcmp(buf, *iter, res.len))
					{
						inject = true;
					}
				}
				if (passthru) {
					forward_open(reqh, buf, res.len, res.flags);
				} else if (inject) {
					// TODO all the inject points could be precomputed
					ipath = malloc(res.len + 1);
					memcpy(ipath, buf, res.len);
					ipath[res.len] = '\0';
					_syscall_fs_respond(reqh, ipath, 0, 0);
				} else {
					_syscall_fs_respond(reqh, NULL, -1, 0);
				}
				break;

			case VFSOP_READ:
			case VFSOP_GETSIZE:
				blen = strlen(ipath);
				char *target = res.op == VFSOP_READ ? buf : NULL;
				dir_start(&db, res.offset, target, buflen);
				for (const char **iter = list; *iter; iter++) {
					// TODO could be precomputed too
					size_t len = strlen(*iter); // inefficient, whatever
					if (blen < len && !memcmp(ipath, *iter, blen))
						dir_appendl(&db, *iter + blen, dir_seglen(*iter + blen));
				}
				_syscall_fs_respond(reqh, target, dir_finish(&db), 0);
				break;

			case VFSOP_CLOSE:
				free(ipath);
				_syscall_fs_respond(reqh, NULL, 0, 0);
				break;

			default:
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	exit(0);
}

void fs_union(const char **list) {
	struct fs_wait_response res;

	/* the buffer is split into two halves:
	 *  the second one is filled out with the path by fs_wait
	 *  the first one is used for the prepended paths */
	const size_t buflen = 1024;
	const size_t prelen = 512;
	const size_t postlen = buflen - prelen;
	char *pre = malloc(buflen);
	char *post = pre + prelen;
	long ret;
	struct dirbuild db;
	if (!pre) exit(1);

	while (!c0_fs_wait(post, postlen, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				if (res.len == 1) { /* root directory */
					c0_fs_respond(NULL, 0, 0);
					break;
				}

				ret = -1;
				for (size_t i = 0; ret < 0 && list[i]; i++) {
					const char *prefix = list[i];
					size_t prefixlen = strlen(prefix); // TODO cache
					if (prefixlen > prelen) continue;
					char *path = post - prefixlen;
					memcpy(path, prefix, prefixlen);

					ret = _syscall_open(path, prefixlen + res.len, res.flags);

					post[res.len] = '\0';
				}
				if (ret < 0) ret = -1;
				c0_fs_respond(NULL, ret, FSR_DELEGATE);
				break;

		case VFSOP_READ:
		case VFSOP_GETSIZE:
			if (res.capacity > buflen)
				res.capacity = buflen;
			bool end = false;
			char *target = res.op == VFSOP_READ ? pre : NULL;
			dir_start(&db, res.offset, target, res.capacity);
			for (size_t i = 0; !end && list[i]; i++) {
				const char *prefix = list[i];
				size_t prefixlen = strlen(prefix);
				// TODO only open the directories once
				// TODO ensure trailing slash
				handle_t h = _syscall_open(prefix, prefixlen, 0);
				if (h < 0) continue;
				end = end || dir_append_from(&db, h);
				_syscall_close(h);
			}
			c0_fs_respond(target, dir_finish(&db), 0);
			break;

			default:
				c0_fs_respond(NULL, -1, 0);
				break;
		}
	}
	exit(0);
}


void fs_dir_inject(const char *path) {
	struct fs_dir_handle {
		const char *inject;
		int delegate, inject_len;
	};
	const size_t path_len = strlen(path);
	const size_t buflen = 1024;
	char *buf = malloc(buflen);
	if (!buf) exit(1);

	for (;;) {
		struct fs_wait_response res;
		handle_t reqh = _syscall_fs_wait(buf, buflen, &res);
		if (reqh < 0) break;
		struct fs_dir_handle *data = res.id;
		switch (res.op) {
			struct dirbuild db;
			case VFSOP_OPEN:
				if (buf[res.len - 1] == '/' &&
						res.len < path_len && !memcmp(path, buf, res.len))
				{
					/* opening a directory that we're injecting into */
					data = malloc(sizeof *data);
					data->delegate = _syscall_open(buf, res.len, res.flags);
					data->inject = path + res.len;
					data->inject_len = dir_seglen(data->inject);
					_syscall_fs_respond(reqh, data, 0, 0);
				} else {
					forward_open(reqh, buf, res.len, res.flags);
				}
				break;

			case VFSOP_CLOSE:
				if (data->delegate >= 0)
					close(data->delegate);
				free(data);
				_syscall_fs_respond(reqh, NULL, 0, 0);
				break;

			case VFSOP_READ:
			case VFSOP_GETSIZE:
				if (res.capacity > buflen)
					res.capacity = buflen;
				char *target = res.op == VFSOP_READ ? buf : NULL;
				dir_start(&db, res.offset, target, res.capacity);
				dir_appendl(&db, data->inject, data->inject_len);
				if (data->delegate >= 0)
					dir_append_from(&db, data->delegate);
				_syscall_fs_respond(reqh, target, dir_finish(&db), 0);
				break;

			default:
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	exit(0);
}

bool mount_at_pred(const char *path) {
	// TODO preprocess path - simplify & ensure trailing slash
	if (!fork2_n_mount(path)) {
		/* child -> go into the for body */
		_klogf("%s: impl", path);
		return true;
	}

	if (strcmp("/", path) && !fork2_n_mount("/")) {
		_klogf("%s: dir", path);
		fs_dir_inject(path);
		exit(1);
	}
	return false; /* continue after the for loop */
}
