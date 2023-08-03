#include <assert.h>
#include <camellia/fs/dir.h>
#include <camellia/fs/misc.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

typedef struct Handle {
	int delegate;
	int plen;
	char path[];
} Handle;

static int
dir_seglen(const char *path)
{
	/* if path contains /, return its position + 1
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

static int
find_injects(const char *injects[], const char *path, int plen, struct dirbuild *db)
{
	// TODO deduplicate
	const char *inj;
	int matches = 0;
	assert(plen >= 1);
	assert(path[plen-1] == '/');

	while ((inj = *injects++)) {
		int ilen = strlen(inj);
		if (plen < ilen && memcmp(path, inj, plen) == 0) {
			if (db) {
				/* inj[plen-1] == '/' */
				const char *ent = inj + plen;
				dir_appendl(db, ent, dir_seglen(ent));
			}
			matches++;
		}
	}
	return matches;
}

void
fs_dirinject2(const char *injects[])
{
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	if (!buf) exit(1);

	for (;;) {
		struct ufs_request req;
		hid_t reqh = _sys_fs_wait(buf, buflen, &req);
		if (reqh < 0) break;
		Handle *hndl = req.id;
		switch (req.op) {
		case VFSOP_OPEN: {
			if (buf[req.len - 1] == '/') {
				if (find_injects(injects, buf, req.len, NULL) > 0) {
					/* opening a directory that we're injecting into */
					hndl = malloc(sizeof(Handle) + req.len);
					if (hndl == NULL) {
						_sys_fs_respond(reqh, NULL, -EGENERIC, 0);
						break;
					}
					/* ignore errors from _sys_open */
					hndl->delegate = _sys_open(buf, req.len, req.flags);
					hndl->plen = req.len;
					memcpy(hndl->path, buf, hndl->plen);
					_sys_fs_respond(reqh, hndl, 0, 0);
					break;
				}
			}
			/* default behaviour */
			forward_open(reqh, buf, req.len, req.flags);
			break;
		}

		case VFSOP_CLOSE: {
			if (hndl->delegate >= 0) {
				close(hndl->delegate);
			}
			free(hndl);
			_sys_fs_respond(reqh, NULL, 0, 0);
			break;
		}

		case VFSOP_READ:
		case VFSOP_GETSIZE: {
			struct dirbuild db;
			char *target = NULL;
			if (req.op == VFSOP_READ) {
				target = buf;
			}
			req.capacity = MIN(req.capacity, buflen);

			dir_start(&db, req.offset, target, req.capacity);
			find_injects(injects, hndl->path, hndl->plen, &db);
			if (hndl->delegate >= 0) {
				dir_append_from(&db, hndl->delegate);
			}
			_sys_fs_respond(reqh, target, dir_finish(&db), 0);
			break;
		}

		default: {
			_sys_fs_respond(reqh, NULL, -1, 0);
			break;
		}
		}
	}
	exit(0);
}

void
fs_dirinject(const char *path) {
	fs_dirinject2((const char*[]){ path, NULL });
}
