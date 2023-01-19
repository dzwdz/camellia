#include "ext2/ex_cache.h"
#include "ext2/ext2.h"
#include <assert.h>
#include <camellia/flags.h>
#include <camellia/fs/dir.h>
#include <camellia/fs/misc.h>
#include <camellia/fsutil.h>
#include <camellia/syscalls.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct handle {
	uint32_t n;
	bool dir;
};

static int my_read(void *fp, void *buf, size_t len, size_t off);
static int my_write(void *fp, const void *buf, size_t len, size_t off);
static void do_open(struct ext2 *fs, handle_t reqh, struct ufs_request *req, char *buf);
static void do_read(struct ext2 *fs, handle_t reqh, struct ufs_request *req, char *buf, size_t buflen);
static void do_write(struct ext2 *fs, handle_t reqh, struct ufs_request *req, char *buf);
static void do_getsize(struct ext2 *fs, handle_t reqh, struct ufs_request *req);

static int
my_read(void *fp, void *buf, size_t len, size_t off)
{
	if (fseek(fp, off, SEEK_SET) < 0) {
		return -1;
	} else if (fread(buf, len, 1, fp) == 1) {
		return 0;
	} else {
		return -1;
	}
}

static int
my_write(void *fp, const void *buf, size_t len, size_t off)
{
	if (fseek(fp, off, SEEK_SET) < 0) {
		return -1;
	} else if (fwrite(buf, len, 1, fp) == 1) {
		return 0;
	} else {
		return -1;
	}
}

static void
do_open(struct ext2 *fs, handle_t reqh, struct ufs_request *req, char *buf)
{
	bool is_dir = req->len == 0 || buf[req->len-1] == '/';
	uint32_t n = ext2c_walk(fs, buf, req->len);
	if (n == 0) {
		if (is_dir) {
			_syscall_fs_respond(reqh, NULL, -ENOSYS, 0);
			return;
		}
		/* buf[0] == '/', strrchr != NULL */
		char *name = strrchr(buf, '/') + 1;
		uint32_t dir_n = ext2c_walk(fs, buf, name - buf);
		if (dir_n == 0) {
			_syscall_fs_respond(reqh, NULL, -ENOENT, 0);
			return;
		}
		n = ext2_alloc_inode(fs, 0100700);
		if (n == 0) {
			_syscall_fs_respond(reqh, NULL, -1, 0);
			return;
		}
		if (ext2_link(fs, dir_n, name, n, 1) < 0) {
			_syscall_fs_respond(reqh, NULL, -1, 0);
			return;
		}
	} else {
		struct ext2d_inode *inode = ext2_req_inode(fs, n);
		if (!inode) {
			_syscall_fs_respond(reqh, NULL, -ENOENT, 0);
			return;
		}
		int type = (inode->perms >> 12) & 0xF;
		ext2_dropreq(fs, inode, false);

		if ((type == 0x8 && is_dir) || (type == 0x4 && !is_dir)) {
			_syscall_fs_respond(reqh, NULL, -ENOENT, 0);
			return;
		} else if (type != 0x8 && type != 0x4) {
			_syscall_fs_respond(reqh, NULL, -ENOSYS, 0);
			return;
		}
	}

	struct handle *h = malloc(sizeof *h);
	if (!h) {
		_syscall_fs_respond(reqh, NULL, -1, 0);
		return;
	}
	h->n = n;
	h->dir = is_dir;
	_syscall_fs_respond(reqh, h, 0, 0);
}

static void
do_read(struct ext2 *fs, handle_t reqh, struct ufs_request *req, char *buf, size_t buflen)
{
	struct handle *h = req->id;
	if (!h->dir) {
		struct ext2d_inode *inode = ext2_req_inode(fs, h->n);
		if (!inode) goto err;
		fs_normslice(&req->offset, &req->capacity, inode->size_lower, false);
		ext2_dropreq(fs, inode, false);

		void *b = ext2_req_file(fs, h->n, &req->capacity, req->offset);
		if (!b) goto err;
		_syscall_fs_respond(reqh, b, req->capacity, 0);
		ext2_dropreq(fs, b, false);
	} else {
		struct dirbuild db;
		char namebuf[257];
		if (req->capacity > buflen)
			req->capacity = buflen;
		dir_start(&db, req->offset, buf, buflen);
		for (struct ext2_diriter iter = {0}; ext2_diriter(&iter, fs, h->n); ) {
			if (iter.ent->namelen_lower == 1 && iter.ent->name[0] == '.') {
				continue;
			}
			if (iter.ent->namelen_lower == 2
				&& iter.ent->name[0] == '.'
				&& iter.ent->name[1] == '.')
			{
				continue;
			}
			if (iter.ent->type == 2) { /* dir */
				memcpy(namebuf, iter.ent->name, iter.ent->namelen_lower);
				namebuf[iter.ent->namelen_lower] = '/';
				dir_appendl(&db, namebuf, iter.ent->namelen_lower + 1);
			} else {
				dir_appendl(&db, iter.ent->name, iter.ent->namelen_lower);
			}
		}
		_syscall_fs_respond(reqh, buf, dir_finish(&db), 0);
	}
	return;
err:
	_syscall_fs_respond(reqh, NULL, -1, 0);
}

static void
do_write(struct ext2 *fs, handle_t reqh, struct ufs_request *req, char *buf)
{
	struct handle *h = req->id;
	if (h->dir) goto err;

	struct ext2d_inode *inode = ext2_req_inode(fs, h->n);
	if (!inode) goto err;
	fs_normslice(&req->offset, &req->len, inode->size_lower, true);
	if ((req->flags & WRITE_TRUNCATE) || inode->size_lower < req->offset + req->len) {
		inode->size_lower = req->offset + req->len;
		if (ext2_dropreq(fs, inode, true) < 0) {
			goto err;
		}
	} else {
		ext2_dropreq(fs, inode, false);
	}
	inode = NULL;

	int ret = ext2_write(fs, h->n, buf, req->len, req->offset);
	_syscall_fs_respond(reqh, NULL, ret, 0);
	return;
err:
	_syscall_fs_respond(reqh, NULL, -1, 0);
}

static void
do_getsize(struct ext2 *fs, handle_t reqh, struct ufs_request *req) {
	struct handle *h = req->id;
	if (h->dir) goto err;

	struct ext2d_inode *inode = ext2_req_inode(fs, h->n);
	if (!inode) goto err;
	_syscall_fs_respond(reqh, NULL, inode->size_lower, 0);
	ext2_dropreq(fs, inode, false);
	return;
err:
	_syscall_fs_respond(reqh, NULL, -1, 0);
}

int
main(int argc, char **argv)
{
	intr_set(NULL);

	if (argc < 2) errx(1, "bad usage");
	// TODO pread/pwrite for normal handles
	FILE *disk = fopen(argv[1], "r+");
	if (!disk) err(1, "couldn't open '%s'", argv[1]);

	struct e2device *dev = exc_init(my_read, my_write, (void*)disk);
	if (!dev) errx(1, "exc_init failed");
	struct ext2 *fs = ext2_opendev(dev, exc_req, exc_drop);
	if (!fs) errx(1, "ext2_opendev failed");

	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	struct ufs_request req;
	for (;;) {
		handle_t reqh = ufs_wait(buf, buflen, &req);
		struct handle *h = req.id;
		if (reqh < 0) break;
		switch (req.op) {
		case VFSOP_OPEN:
			do_open(fs, reqh, &req, buf);
			break;
		case VFSOP_READ:
			do_read(fs, reqh, &req, buf, buflen);
			break;
		case VFSOP_WRITE:
			do_write(fs, reqh, &req, buf);
			break;
		case VFSOP_GETSIZE:
			do_getsize(fs, reqh, &req);
			break;
		case VFSOP_CLOSE:
			free(h);
			_syscall_fs_respond(reqh, NULL, -1, 0);
			break;
		default:
			_syscall_fs_respond(reqh, NULL, -1, 0);
			break;
		}
	}
	warnx("cleaning up");

	return 1;
}
