#include <camellia/fsutil.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <shared/mem.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <user/lib/fs/dir.h>

struct node {
	const char *name;
	bool directory;
	size_t namelen;
	char *buf;
	size_t size, capacity;
	size_t open; /* amount of open handles */

	struct node *sibling, *child;
	/* each node except special_root has exacly one sibling or child reference to it
	 * on remove(), it gets replaced with *sibling. */
	struct node **ref;
};

static struct node special_root = {
	.directory = true,
	.size = 0,
};

static struct node *lookup(struct node *parent, const char *path, size_t len) {
	for (struct node *iter = parent->child; iter; iter = iter->sibling) {
		if (iter->namelen == len && !memcmp(path, iter->name, len))
			return iter;
	}
	return NULL;
}

static struct node *tmpfs_open(const char *path, struct fs_wait_response *res) {
	struct node *node = &special_root;
	if (res->len == 0) return NULL;
	if (res->len == 1) return node;
	path++;
	res->len--;

	bool more = true;
	size_t segpos = 0, seglen; /* segments end with a slash, inclusive */
	while (more) {
		struct node *const parent = node;
		char *slash = memchr(path + segpos, '/', res->len);
		seglen = (slash ? (size_t)(slash - path + 1) : res->len) - segpos;
		more = segpos + seglen < res->len;

		node = lookup(parent, path + segpos, seglen);
		if (!node) {
			if (!more && (res->flags & OPEN_CREATE)) {
				node = malloc(sizeof *node);
				memset(node, 0, sizeof *node);

				char *namebuf = malloc(seglen + 1);
				memcpy(namebuf, path + segpos, seglen);
				namebuf[seglen] = '\0';
				node->name = namebuf;
				node->directory = slash;
				node->namelen = seglen;

				if (parent->child) {
					parent->child->ref = &node->sibling;
					*parent->child->ref = parent->child;
				}
				node->ref = &parent->child;
				*node->ref = node;
			} else {
				return NULL;
			}
		}
		segpos += seglen;
	}
	node->open++;
	return node;
}

static void handle_down(struct node *node) {
	node->open--;
	if (!node->ref && node != &special_root && node->open == 0) {
		free(node->buf);
		free(node);
	}
}

static long remove_node(struct node *node) {
	if (node == &special_root) return -1;
	if (!node->ref) return -1;
	if (node->child) return -ENOTEMPTY;
	*node->ref = node->sibling;
	node->ref = NULL;
	handle_down(node);
	return 0;
}

int main(void) {
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
				if (ptr->directory) {
					struct dirbuild db;
					dir_start(&db, res.offset, buf, buflen);
					for (struct node *iter = ptr->child; iter; iter = iter->sibling)
						dir_append(&db, iter->name);
					_syscall_fs_respond(buf, dir_finish(&db), 0);
				} else {
					fs_normslice(&res.offset, &res.len, ptr->size, false);
					_syscall_fs_respond(ptr->buf + res.offset, res.len, 0);
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
				if (ptr->capacity <= res.offset + res.len) {
					size_t newcap = 1;
					while (newcap && newcap <= res.offset + res.len)
						newcap *= 2;
					if (!newcap) { /* overflow */
						_syscall_fs_respond(NULL, -1, 0);
						break;
					}
					ptr->capacity = newcap;
					ptr->buf = realloc(ptr->buf, ptr->capacity);
				}

				memcpy(ptr->buf + res.offset, buf, res.len);
				if ((res.flags & WRITE_TRUNCATE) || ptr->size < res.offset + res.len) {
					ptr->size = res.offset + res.len;
				}
				_syscall_fs_respond(NULL, res.len, 0);
				break;

			case VFSOP_GETSIZE:
				ptr = (void*)res.id;
				if (ptr->directory) {
					// TODO could be cached in ptr->size
					struct dirbuild db;
					dir_start(&db, res.offset, NULL, buflen);
					for (struct node *iter = ptr->child; iter; iter = iter->sibling)
						dir_append(&db, iter->name);
					_syscall_fs_respond(NULL, dir_finish(&db), 0);
				} else {
					_syscall_fs_respond(NULL, ptr->size, 0);
				}
				break;

			case VFSOP_REMOVE:
				ptr = (void*)res.id;
				_syscall_fs_respond(NULL, remove_node(ptr), 0);
				break;

			case VFSOP_CLOSE:
				ptr = (void*)res.id;
				handle_down(ptr);
				_syscall_fs_respond(NULL, -1, 0);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}
	return 1;
}
