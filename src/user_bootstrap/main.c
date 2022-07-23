#include <shared/flags.h>
#include <shared/mem.h>
#include <shared/syscalls.h>
#include <user_bootstrap/elfload.h>

extern char _bss_start;
extern char _bss_end;
extern char _initrd;

/* libc stubs */
int printf(const char *fmt, ...) {(void)fmt; return 0;}
void *malloc(size_t size) {(void)size; _syscall_exit(1);}
void free(void *ptr) {(void)ptr;}
int file_read(libc_file *f, char *buf, size_t len) {(void)f; (void)buf; (void)len; _syscall_exit(1);}


static void *tar_find(void *tar, const char *path);
static int oct_parse(char *str, size_t len);

static void *tar_find(void *tar, const char *path) {
	size_t path_len = strlen(path);
	int size;
	if (path_len > 100) return NULL;

	for (size_t off = 0;;) {
		if (0 != memcmp(tar + off + 257, "ustar", 5))
			return NULL; // not a metadata sector, end of archive

		size = oct_parse(tar + off + 124, 11);
		if (0 == memcmp(tar + off, path, path_len) &&
				*(char*)(tar + off + path_len) == '\0')
		{
			return tar + off + 512;
		}
		off += 512;
		off += (size + 511) & ~511;
	}
}

static int oct_parse(char *str, size_t len) {
	int res = 0;
	for (size_t i = 0; i < len; i++) {
		res *= 8;
		res += str[i] - '0'; // no format checking
	}
	return res;
}


__attribute__((section(".text.startup")))
int main(void) {
	_syscall_memflag(&_bss_start, &_bss_end - &_bss_start, MEMFLAG_PRESENT);

	void *init = tar_find(&_initrd, "init.elf");
	void (*entry)(void*) = elf_partialexec(init);
	if (entry) {
		// TODO dynamically link initrd
		entry(&_initrd);
	} else {
		_syscall_debug_klog("bootstrap failed", sizeof("bootstrap failed"));
	}
	_syscall_exit(0);
}
