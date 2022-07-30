#include "tar.h"
#include <camellia/flags.h>
#include <camellia/fsutil.h>
#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <stdint.h>
#include <unistd.h>
#include <user/lib/fs/dir.h>

#define BUF_SIZE 64

static void *tar_open(const char *path, int len, void *base, size_t base_len);
static void tar_read(struct fs_wait_response *res, void *base, size_t base_len);
static int tar_size(void *sector);
static int oct_parse(char *str, size_t len);


static const char *root_fakemeta = ""; /* see comment in tar_open */


void tar_driver(void *base) {
	static char buf[BUF_SIZE];
	struct fs_wait_response res;
	void *ptr;
	while (!_syscall_fs_wait(buf, BUF_SIZE, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				ptr = tar_open(buf, res.len, base, ~0);
				_syscall_fs_respond(ptr, ptr ? 0 : -1, 0);
				break;

			case VFSOP_READ:
				tar_read(&res, base, ~0);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0); // unsupported
				break;
		}
	}
	exit(0);
}

static void *tar_open(const char *path, int len, void *base, size_t base_len) {
	if (len <= 0) return NULL;
	path += 1; // skip the leading slash
	len  -= 1;

	/* TAR archives don't (seem to) contain an entry for the root dir, so i'm
	 * returning a fake one. this isn't a full entry because i'm currently too
	 * lazy to create a full one - thus, it has to be special cased in tar_read */
	if (len == 0)
		return (void*)root_fakemeta;

	return tar_find(path, len, base, base_len);
}

static void tar_read(struct fs_wait_response *res, void *base, size_t base_len) {
	void *meta =  (void*)res->id;
	char  type = *(char*)(meta + 156);
	size_t meta_len;
	int size;

	static char buf[BUF_SIZE]; // TODO reuse a single buffer

	if (meta == root_fakemeta) type = '5'; /* see comment in tar_open() */

	switch (type) {
		case '\0':
		case '0': /* normal files */
			size = tar_size(meta);
			fs_normslice(&res->offset, &res->len, size, false);
			_syscall_fs_respond(meta + 512 + res->offset, res->len, 0);
			break;

		case '5': /* directory */
			struct dirbuild db;
			dir_start(&db, res->offset, buf, sizeof buf);

			meta_len = strlen(meta);
			for (size_t off = 0; off < base_len;) {
				if (0 != memcmp(base + off + 257, "ustar", 5))
					break; // not a metadata sector

				/* check if prefix matches */
				if (0 == memcmp(base + off, meta, meta_len)
					&& *(char*)(base + off + meta_len) != '\0') {
					char *suffix = base + off + meta_len;

					/* check if the path contains any non-trailing slashes */
					char *next = suffix;
					// TODO strchr
					while (*next && *next != '/') next++;
					if (*next == '/') next++;

					if (*next == '\0') {
						if (dir_append(&db, suffix)) break;
					}
				}

				size = tar_size(base + off);
				off += 512;                 // skip this metadata sector
				off += (size + 511) & ~511; // skip the data sectors
			}

			_syscall_fs_respond(buf, dir_finish(&db), 0);
			break;

		default:
			_syscall_fs_respond(NULL, -1, 0);
			break;
	}
}

static int tar_size(void *sector) {
	return oct_parse(sector + 124, 11);
}

void *tar_find(const char *path, size_t path_len, void *base, size_t base_len) {
	int size;
	if (path_len > 100) return NULL; // illegal path

	for (size_t off = 0; off < base_len;) {
		if (0 != memcmp(base + off + 257, "ustar", 5))
			break; // not a metadata sector
		if (0 == memcmp(base + off, path, path_len) &&
				*(char*)(base + off + path_len) == '\0')
			return base + off; // file found, quit

		size = tar_size(base + off);
		off += 512;                 // skip this metadata sector
		off += (size + 511) & ~511; // skip the data sectors
	}
	return NULL;
}

static int oct_parse(char *str, size_t len) {
	int res = 0;
	for (size_t i = 0; i < len; i++) {
		res *= 8;
		res += str[i] - '0'; // no format checking
	}
	return res;
}
