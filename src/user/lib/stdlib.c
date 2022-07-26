#include <camellia/syscalls.h>
#include <errno.h>
#include <shared/printf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// TODO oh god this garbage - malloc, actually open, [...]
static FILE _stdin_null  = { .fd = 0 };
static FILE _stdout_null = { .fd = 1 };

FILE *const stdin = &_stdin_null, *const stdout = &_stdout_null;

int errno = 0;

static void backend_file(void *arg, const char *buf, size_t len) {
	file_write((FILE*)arg, buf, len);
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


FILE *fopen(const char *path, const char *mode) {
	FILE *f;
	handle_t h;
	int flags = 0;
	if (mode[0] == 'w' || mode[0] == 'a')
		flags |= OPEN_CREATE;
	// TODO truncate on w

	h = _syscall_open(path, strlen(path), flags);
	if (h < 0) {
		errno = -h;
		return NULL;
	}

	f = malloc(sizeof *f);
	if (!f) {
		close(h);
		return NULL;
	}
	f->pos = mode[0] == 'a' ? -1 : 0;
	f->eof = false;
	f->fd = h;
	return f;
}

 FILE *freopen(const char *path, const char *mode, FILE *f) {
	/* partially based on the musl implementation of freopen */
	FILE *f2;
	if (!path) goto fail;
	f2 = fopen(path, mode);
	if (!f2) goto fail;

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

FILE *file_clone(const FILE *f) {
	handle_t h = _syscall_dup(f->fd, -1, 0);
	FILE *f2;
	if (h < 0) return NULL;

	// TODO file_wrapfd
	f2 = malloc(sizeof *f2);
	if (!f2) {
		close(h);
		return NULL;
	}
	f2->pos = f->pos;
	f2->eof = f->eof;
	f2->fd = h;
	return f2;
}

int file_read(FILE *f, char *buf, size_t len) {
	if (f->fd < 0) return -1;

	int res = _syscall_read(f->fd, buf, len, f->pos);
	if (res < 0) return res;
	if (res == 0 && len > 0) f->eof = true;

	bool negative_pos = f->pos < 0;
	f->pos += res;
	if (negative_pos && f->pos >= 0)
		f->pos = -1;

	return res;
}

int file_write(FILE *f, const char *buf, size_t len) {
	if (f->fd < 0) return -1;

	int res = _syscall_write(f->fd, buf, len, f->pos);
	if (res < 0) return res;
	f->pos += res;
	return res;
}

void file_close(FILE *f) {
	if (f->fd > 0) close(f->fd);
	if (f != &_stdin_null && f != &_stdout_null)
		free(f);
}


int fork(void) {
	return _syscall_fork(0, NULL);
}

int close(handle_t h) {
	return _syscall_close(h);
}
