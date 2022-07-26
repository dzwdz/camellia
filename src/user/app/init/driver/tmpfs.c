#include <shared/mem.h>
#include <shared/syscalls.h>
#include <stddef.h>
#include <user/lib/malloc.h>

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

		char *namebuf = malloc(res->len);
		memcpy(namebuf, path, res->len);
		node->name = namebuf;
		node->namelen = res->len;
		node->next = root;
		root = node;
	}
	return node;
}

void tmpfs_drv(void) {
	// TODO replace all the static allocations in drivers with mallocs
	static char buf[512];
	struct fs_wait_response res;
	struct node *ptr;
	while (!_syscall_fs_wait(buf, sizeof buf, &res)) {
		switch (res.op) {
			case VFSOP_OPEN:
				ptr = tmpfs_open(buf, &res);
				_syscall_fs_respond(ptr, ptr ? 0 : -1, 0);
				break;

			case VFSOP_READ:
				ptr = (void*)res.id;
				if (ptr == &special_root) {
					size_t buf_pos = 0;
					size_t to_skip = res.offset;

					for (struct node *iter = root; iter; iter = iter->next) {
						if (iter->namelen <= to_skip) {
							to_skip -= iter->namelen;
							continue;
						}

						if (iter->namelen + buf_pos - to_skip >= sizeof(buf)) {
							memcpy(buf + buf_pos, iter->name + to_skip, sizeof(buf) - buf_pos - to_skip);
							buf_pos = sizeof(buf);
							break;
						}
						memcpy(buf + buf_pos, iter->name + to_skip, iter->namelen - to_skip);
						buf_pos += iter->namelen - to_skip;
						buf[buf_pos++] = '\0';
						to_skip = 0;
					}
					_syscall_fs_respond(buf, buf_pos, 0);
				} else {
					// TODO offset
					if (res.offset)
						_syscall_fs_respond(NULL, 0, 0);
					else
						_syscall_fs_respond(ptr->buf, ptr->size, 0);
					break;
				}
				break;

			case VFSOP_WRITE:
				ptr = (void*)res.id;
				if (ptr == &special_root) {
					_syscall_fs_respond(NULL, -1, 0);
					break;
				}
				if (res.len == 0) {
					_syscall_fs_respond(NULL, 0, 0);
					break;
				}
				if (!ptr->buf) {
					ptr->buf = malloc(256);
					if (!ptr->buf) {
						_syscall_fs_respond(NULL, -1, 0);
						break;
					}
					memset(ptr->buf, 0, 256);
					ptr->capacity = 256;
				}

				size_t len = res.len;
				if (len > ptr->capacity - res.offset)
					len = ptr->capacity - res.offset;
				memcpy(ptr->buf + res.offset, buf, len);
				if (ptr->size < res.offset + len)
					ptr->size = res.offset + len;
				_syscall_fs_respond(NULL, len, 0);
				break;

			default:
				_syscall_fs_respond(NULL, -1, 0);
				break;
		}
	}

	_syscall_exit(1);
}
