#include <camellia/fsutil.h>
#include <camellia/syscalls.h>
#include <shared/mem.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <user/lib/fs/dir.h>

struct node {
	const char *name;
	size_t namelen;
	struct node *next;
	char *buf;
	size_t size, capacity;
};

struct node *root = NULL;
static struct node special_root;

static struct node *lookup(const char *path, size_t len) {
	for (struct node *iter = root; iter; iter = iter->next) {
		if (iter->namelen == len && !memcmp(path, iter->name, len))
			return iter;
	}
	return NULL;
}

static struct node *tmpfs_open(const char *path, struct fs_wait_response *res) {
	struct node *node;
	if (res->len == 0) return NULL;
	path++;
	res->len--;

	if (res->len == 0) return &special_root;

	// no directory support (yet)
	if (memchr(path, '/', res->len)) return NULL;

	node = lookup(path, res->len);
	if (!node && (res->flags & OPEN_CREATE)) {
		node = malloc(sizeof *node);
		memset(node, 0, sizeof *node);

		char *namebuf = malloc(res->len + 1);
		memcpy(namebuf, path, res->len);
		namebuf[res->len] = '\0';
		node->name = namebuf;
		node->namelen = res->len;
		node->next = root;
		root = node;
	}
	return node;
}

void tmpfs_drv(void) {
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	struct fs_wait_response res;
	struct node *ptr;
	while (!_syscall_fs_wait(buf, buflen, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				ptr = tmpfs_open(buf, &res);
				_syscall_fs_respond(ptr, ptr ? 0 : -1, 0);
				break;

			case VFSOP_READ:
				ptr = (void*)res.id;
				if (ptr == &special_root) {
					struct dirbuild db;
					dir_start(&db, res.offset, buf, buflen);
					for (struct node *iter = root; iter; iter = iter->next)
						dir_append(&db, iter->name);
					_syscall_fs_respond(buf, dir_finish(&db), 0);
				} else {
					fs_normslice(&res.offset, &res.len, ptr->size, false);
					_syscall_fs_respond(ptr->buf + res.offset, res.len, 0);
					break;
				}
				break;

			case VFSOP_WRITE:
				ptr = (void*)res.id;
				if (ptr == &special_root) {
					_syscall_fs_respond(NULL, -1, 0);
					break;
				}
				if (res.len > 0 && !ptr->buf) {
					ptr->buf = malloc(256);
					if (!ptr->buf) {
						_syscall_fs_respond(NULL, -1, 0);
						break;
					}
					memset(ptr->buf, 0, 256);
					ptr->capacity = 256;
				}

				fs_normslice(&res.offset, &res.len, ptr->size, true);
				if (res.offset + res.len >= ptr->capacity) {
					// TODO expanding files
					_syscall_fs_respond(NULL, -1, 0);
					break;
				}

				memcpy(ptr->buf + res.offset, buf, res.len);
				if ((res.flags & WRITE_TRUNCATE) || ptr->size < res.offset + res.len) {
					ptr->size = res.offset + res.len;
				}
				_syscall_fs_respond(NULL, res.len, 0);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}

	exit(1);
}
