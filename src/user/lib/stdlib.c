#include <user/lib/stdlib.h>
#include <shared/printf.h>
#include <shared/syscalls.h>

// TODO oh god this garbage - malloc, actually open, [...]
static libc_file _stdin_null  = { .fd = 0 };
static libc_file _stdout_null = { .fd = 1 };

libc_file *stdin = &_stdin_null, *stdout = &_stdout_null;

static void backend_file(void *arg, const char *buf, size_t len) {
	file_write((libc_file*)arg, buf, len);
}

int printf(const char *fmt, ...) {
	int ret = 0;
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend_file, (void*)stdout);
	va_end(argp);
	return ret;
}

static void backend_buf(void *arg, const char *buf, size_t len) {
	char **ptrs = arg;
	size_t space = ptrs[1] - ptrs[0];
	if (len > space) len = space;
	memcpy(ptrs[0], buf, len);
	ptrs[0] += len;
}

int snprintf(char *str, size_t len, const char *fmt, ...) {
	int ret = 0;
	char *ptrs[2] = {str, str + len};
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend_buf, &ptrs);
	va_end(argp);
	if (ptrs[0] < ptrs[1]) *ptrs[0] = '\0';
	return ret;
}

int _klogf(const char *fmt, ...) {
	// idiotic. however, this hack won't matter anyways
	char buf[256];
	int ret = 0;
	char *ptrs[2] = {buf, buf + sizeof buf};
	va_list argp;
	va_start(argp, fmt);
	ret = __printf_internal(fmt, argp, backend_buf, &ptrs);
	va_end(argp);
	if (ptrs[0] < ptrs[1]) *ptrs[0] = '\0';
	_syscall_debug_klog(buf, ret);
	return ret;
}


libc_file *file_open(const char *path, int flags) {
	handle_t h = _syscall_open(path, strlen(path), flags);
	libc_file *f;
	if (h < 0) return NULL;

	f = malloc(sizeof *f);
	if (!f) {
		_syscall_close(h);
		return NULL;
	}
	f->pos = 0;
	f->eof = false;
	f->fd = h;
	return f;
}

libc_file *file_reopen(libc_file *f, const char *path, int flags) {
	/* partially based on the musl implementation of freopen */
	libc_file *f2;
	if (!path) goto fail;
	f2 = file_open(path, flags);
	if (!f2) goto fail;

	/* shouldn't happen, but if it happens, let's roll with it. */
	if (f->fd == f2->fd) f2->fd = -1;

	if (_syscall_dup(f2->fd, f->fd, 0) < 0) goto fail2;
	f->pos = f2->pos;
	f->eof = f2->eof;
	file_close(f2);
	return f;

fail2:
	file_close(f2);
fail:
	file_close(f);
	return NULL;
}

int file_read(libc_file *f, char *buf, size_t len) {
	if (f->fd < 0) return -1;

	int res = _syscall_read(f->fd, buf, len, f->pos);
	if (res < 0) return res;
	if (res == 0 && len > 0) f->eof = true;
	f->pos += res;
	return res;
}

int file_write(libc_file *f, const char *buf, size_t len) {
	if (f->fd < 0) return -1;

	int res = _syscall_write(f->fd, buf, len, f->pos);
	if (res < 0) return res;
	f->pos += res;
	return res;
}

void file_close(libc_file *f) {
	if (f->fd > 0) _syscall_close(f->fd);
	if (f != &_stdin_null && f != &_stdout_null)
		free(f);
}
