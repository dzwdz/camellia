#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <camellia/compat.h>
#include <camellia/fs/dir.h>
#include <camellia/fs/misc.h>

bool fork2_n_mount(const char *path) {
	hid_t h;
	if (_sys_fork(FORK_NEWFS, &h) > 0) { /* parent */
		_sys_mount(h, path, strlen(path));
		close(h);
		return true;
	}
	return false;
}

void forward_open(hid_t reqh, const char *path, long len, int flags) {
	// TODO use threads
	// TODO solve for more complex cases, e.g. fs_union
	/* done in a separate thread/process because open() can block,
	 * but that should only hold the caller back, and not the fs driver.
	 *
	 * for example, running `httpd` in one term would prevent you from doing
	 * basically anything on the second term, because fs_dirinject would be
	 * stuck on open()ing the socket */
	if (!_sys_fork(FORK_NOREAP, NULL)) {
		_sys_fs_respond(reqh, NULL, _sys_open(path, len, flags), FSR_DELEGATE);
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
		struct ufs_request res;
		hid_t reqh = _sys_fs_wait(buf, buflen, &res);
		if (reqh < 0) break;
		switch (res.op) {
			case VFSOP_OPEN:
				if (prefix) {
					if (prefix_len + res.len > buflen) {
						_sys_fs_respond(reqh, NULL, -1, 0);
						break;
					}

					memmove(buf + prefix_len, buf, res.len);
					memcpy(buf, prefix, prefix_len);
					res.len += prefix_len;
				}
				forward_open(reqh, buf, res.len, res.flags);
				break;

			default:
				_sys_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	exit(0);
}

void fs_union(const char **list) {
	struct ufs_request res;

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

					ret = _sys_open(path, prefixlen + res.len, res.flags);

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
				hid_t h = _sys_open(prefix, prefixlen, OPEN_READ);
				if (h < 0) continue;
				end = end || dir_append_from(&db, h);
				_sys_close(h);
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

hid_t ufs_wait(char *buf, size_t len, struct ufs_request *req) {
	hid_t reqh;
	for (;;) {
		reqh = _sys_fs_wait(buf, len, req);
		if (reqh < 0) break;
		if (req->op == VFSOP_OPEN) {
			if (req->len == len) {
				_sys_fs_respond(reqh, NULL, -ENAMETOOLONG, 0);
				continue;
			}
			buf[req->len] = '\0';
			// TODO ensure passed paths don't have null bytes in them in the kernel
		}
		break;
	}
	return reqh;
}

bool mount_at(const char *path) {
	// TODO preprocess path - simplify & ensure trailing slash
	if (!fork2_n_mount(path)) {
		/* child -> go into the for body */
		_klogf("%s: impl", path);
		setproctitle("%s", path);
		return true;
	}
	return false; /* continue after the for loop */
}
