#include <init/stdlib.h>
#include <shared/flags.h>
#include <shared/syscalls.h>
#include <stdint.h>

#define BUF_SIZE 64

extern int tty_fd;

// TODO struct tar
static int tar_open(const char *path, int len, void *base, size_t base_len);
static int tar_size(void *sector);
static void *tar_find(const char *path, size_t path_len, void *base, size_t base_len);
static int oct_parse(char *str, size_t len);

void tar_driver(handle_t back, void *base) {
	static char buf[BUF_SIZE];
	struct fs_wait_response res;
	for (;;) {
		switch (_syscall_fs_wait(back, buf, BUF_SIZE, &res)) {
			case VFSOP_OPEN:
				_syscall_fs_respond(NULL, tar_open(buf, res.len, base, ~0));
				break;

			case VFSOP_READ: {
				void *meta = (void*)res.id;
				int size = tar_size(meta);
				if (res.offset < 0 || res.offset > size) {
					// TODO support negative offsets
					_syscall_fs_respond(NULL, -1);
				} else {
					_syscall_fs_respond(meta + 512 + res.offset, size - res.offset);
				}
				break;
			}

			default:
				_syscall_fs_respond(NULL, -1); // unsupported
				break;
		}
	}

	// iterate over all sectors, printing filenames
	while (0 == memcmp(base + 257, "ustar", 5)) {
		int size = oct_parse(base + 124, 12);

		_syscall_write(tty_fd, base, 100, 0);
		_syscall_write(tty_fd, " ", 1, 0);

		base += 512;                 // skip metadata sector
		base += (size + 511) & ~511; // skip file (size rounded up to 512)
		// TODO might pagefault if the last sector was at a page boundary
	}
	_syscall_write(tty_fd, "done.", 5, 0);
}

static int tar_open(const char *path, int len, void *base, size_t base_len) {
	void *ptr;

	if (len <= 1) return -1;
	path += 1; // skip the leading slash
	len  -= 1;

	ptr = tar_find(path, len, base, ~0);
	if (!ptr) return -1;
	return (int)ptr;
}

static int tar_size(void *sector) {
	return oct_parse(sector + 124, 11);
}

static void *tar_find(const char *path, size_t path_len, void *base, size_t base_len) {
	int size;
	if (path_len > 100) return NULL; // illegal path

	for (size_t off = 0; off < base_len;) {
		if (0 != memcmp(base + off + 257, "ustar", 5))
			break; // not a metadata sector
		if (0 == memcmp(base + off, path, path_len))
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
