#include <camellia/flags.h>
#include <camellia/syscalls.h>
#include <stdlib.h>
#include <string.h>
#include <user/lib/fs/dir.h>
#include <user/lib/fs/misc.h>

static int dir_seglen2(const char *path, size_t len) {
	/* returns the length of the first segment of the path, including the trailing / (if any). */
	for (size_t i = 0; i < len; i++) {
		if (path[i] == '/')
			return i + 1;
	}
	return len;
}

/** @return the length of the path w/o suffixes */
static size_t suffix_parse(const char *path, size_t len, bool *ro_ptr) {
	bool ro = false;
	if (len >= 3 && !memcmp(path + len - 3, ":ro", 3)) {
		ro = true;
		len -= 3;
	}
	if (ro_ptr) *ro_ptr = ro;
	return len;
}

/** Check if a path is a prefix of another path. */
// TODO move to libc; tests
static bool prefix_match(const char *prefix, size_t plen, const char *full, size_t flen) {
	if (flen < plen) return false;
	if (flen == plen)
		return memcmp(full, prefix, flen) == 0;
	return plen >= 1
		&& prefix[plen - 1] == '/' /* prefixes must point to one of the parent directories */
		&& memcmp(full, prefix, plen) == 0;
}

void fs_whitelist(const char **whitelist) {
	const size_t buflen = 1024;
	char *buf = malloc(buflen);
	if (!buf) exit(1);
	for (;;) {
		struct ufs_request res;
		handle_t reqh = _syscall_fs_wait(buf, buflen, &res);
		if (reqh < 0) break;

		char *ipath = res.id; /* the path of the open()ed directory */

		switch (res.op) {
			case VFSOP_OPEN: {
				bool passthru = false;
				bool inject = false;

				for (const char **entry = whitelist; *entry; entry++) {
					bool ro = false;
					size_t entry_len = suffix_parse(*entry, strlen(*entry), &ro);
					/* If *entry is a prefix of the opened path, pass the open() through. */
					if (prefix_match(*entry, entry_len, buf, res.len)) {
						if (ro) res.flags |= OPEN_RO;
						passthru = true;
						break;
					}
					/* If the path is a prefix of *entry, we might need to inject a directory. */
					if (prefix_match(buf, res.len, *entry, entry_len)) {
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
			}
			case VFSOP_READ:
			case VFSOP_GETSIZE: {
				struct dirbuild db;
				size_t ilen = strlen(ipath);
				char *target = res.op == VFSOP_READ ? buf : NULL;
				dir_start(&db, res.offset, target, buflen);
				for (const char **entry = whitelist; *entry; entry++) {
					// TODO could be precomputed too
					size_t elen = suffix_parse(*entry, strlen(*entry), NULL);
					if (ilen < elen && !memcmp(ipath, *entry, ilen))
						dir_appendl(&db, *entry + ilen, dir_seglen2(*entry + ilen, elen - ilen));
				}
				_syscall_fs_respond(reqh, target, dir_finish(&db), 0);
				break;
			}
			case VFSOP_CLOSE: {
				free(ipath);
				_syscall_fs_respond(reqh, NULL, 0, 0);
				break;
			}
			default: {
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
			}
		}
	}
	exit(0);
}
