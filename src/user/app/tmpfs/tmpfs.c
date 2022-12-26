#include <camellia/flags.h>
#include <camellia/fs/dir.h>
#include <camellia/fs/misc.h>
#include <camellia/fsutil.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct node {
	char *name;
	size_t namelen;
	bool directory;
	char *buf;
	size_t size, capacity;

	size_t open; /* amount of open handles */

	struct node *sibling, *child;
	/* Pointer to the sibling/child field referencing the node. NULL for removed
	 * files. */
	struct node **ref;

	/* allocated by tmpfs_open
	 * freed by node_close when (open == 0 && ref == NULL && self != node_root). */
};

static struct node node_root = {
	.directory = true,
};

/** Responds to open(), finding an existing node, or creating one when applicable. */
static struct node *tmpfs_open(const char *path, struct ufs_request *req);
/** Finds a direct child with the given name. */
static struct node *node_getchild(struct node *parent, const char *name, size_t len);
/** Corresponds to close(); drops a reference. */
static void node_close(struct node *node);
/** Removes a file. It's kept in memory until all the open handles are closed. */
static long node_remove(struct node *node);


static struct node *node_getchild(struct node *parent, const char *name, size_t len) {
	for (struct node *iter = parent->child; iter; iter = iter->sibling) {
		if (iter->namelen == len && !memcmp(name, iter->name, len)) {
			return iter;
		}
	}
	return NULL;
}

static struct node *tmpfs_open(const char *path, struct ufs_request *req) {
	/* *path is not null terminated! */
	struct node *node = &node_root;
	if (req->len == 0) return NULL;
	if (req->len == 1) return node; /* "/" */
	path++;
	req->len--;

	bool more = true;
	size_t segpos = 0, seglen; /* segments end with a slash, inclusive */
	while (more) {
		struct node *parent = node;
		char *slash = memchr(path + segpos, '/', req->len - segpos);
		seglen = (slash ? (size_t)(slash - path + 1) : req->len) - segpos;
		more = segpos + seglen < req->len;

		node = node_getchild(parent, path + segpos, seglen);
		if (!node) {
			if (!more && (req->flags & OPEN_CREATE)) {
				node = calloc(1, sizeof *node);

				node->name = malloc(seglen + 1);
				if (node->name == NULL) {
					free(node);
					return NULL;
				}
				memcpy(node->name, path + segpos, seglen);
				node->name[seglen] = '\0';

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

static void node_close(struct node *node) {
	node->open--;
	if (!node->ref && node != &node_root && node->open == 0) {
		free(node->buf);
		free(node);
	}
}

static long node_remove(struct node *node) {
	if (node == &node_root) return -1;
	if (!node->ref) return -1;
	if (node->child) return -ENOTEMPTY;
	*node->ref = node->sibling;
	node->ref = NULL;
	node_close(node);
	return 0;
}

int main(void) {
	const size_t buflen = 4096;
	char *buf = malloc(buflen);
	if (!buf) return -1;

	for (;;) {
		struct ufs_request req;
		handle_t reqh = ufs_wait(buf, buflen, &req);
		struct node *ptr = req.id;
		if (reqh < 0) break;

		switch (req.op) {
			case VFSOP_OPEN:
				ptr = tmpfs_open(buf, &req);
				_syscall_fs_respond(reqh, ptr, ptr ? 0 : -ENOENT, 0);
				break;

			case VFSOP_READ:
				if (ptr->directory) {
					struct dirbuild db;
					dir_start(&db, req.offset, buf, buflen);
					for (struct node *iter = ptr->child; iter; iter = iter->sibling) {
						dir_appendl(&db, iter->name, iter->namelen);
					}
					_syscall_fs_respond(reqh, buf, dir_finish(&db), 0);
				} else {
					fs_normslice(&req.offset, &req.len, ptr->size, false);
					_syscall_fs_respond(reqh, ptr->buf + req.offset, req.len, 0);
				}
				break;

			case VFSOP_WRITE:
				if (ptr->directory) {
					_syscall_fs_respond(reqh, NULL, -ENOSYS, 0);
					break;
				}

				fs_normslice(&req.offset, &req.len, ptr->size, true);
				if (ptr->capacity <= req.offset + req.len) {
					ptr->capacity = (req.offset + req.len + 511) & ~511;
					ptr->buf = realloc(ptr->buf, ptr->capacity);
				}

				memcpy(ptr->buf + req.offset, buf, req.len);
				if ((req.flags & WRITE_TRUNCATE) || ptr->size < req.offset + req.len) {
					ptr->size = req.offset + req.len;
				}
				_syscall_fs_respond(reqh, NULL, req.len, 0);
				break;

			case VFSOP_GETSIZE:
				if (ptr->directory) {
					// TODO could be cached in ptr->size
					struct dirbuild db;
					dir_start(&db, req.offset, NULL, buflen);
					for (struct node *iter = ptr->child; iter; iter = iter->sibling) {
						dir_append(&db, iter->name);
					}
					_syscall_fs_respond(reqh, NULL, dir_finish(&db), 0);
				} else {
					_syscall_fs_respond(reqh, NULL, ptr->size, 0);
				}
				break;

			case VFSOP_REMOVE:
				_syscall_fs_respond(reqh, NULL, node_remove(ptr), 0);
				break;

			case VFSOP_CLOSE:
				node_close(ptr);
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;

			default:
				_syscall_fs_respond(reqh, NULL, -1, 0);
				break;
		}
	}
	return 1;
}
