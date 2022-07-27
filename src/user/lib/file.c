#include "file.h"
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static FILE _stdin_null  = { .fd = 0 };
static FILE _stdout_null = { .fd = 1 };
FILE *const stdin = &_stdin_null, *const stdout = &_stdout_null;


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
	f = fdopen(h, mode);
	if (!f) close(h);
	return f;
}

 FILE *freopen(const char *path, const char *mode, FILE *f) {
	/* partially based on the musl implementation of freopen */
	FILE *f2;
	if (!path) goto fail;
	f2 = fopen(path, mode);
	if (!f2) goto fail;

	if (f->fd == f2->fd) {
		f2->fd = -1;
	} else {
		if (_syscall_dup(f2->fd, f->fd, 0) < 0) goto fail2;
	}
	f->pos = f2->pos;
	f->eof = f2->eof;
	fclose(f2);
	return f;

fail2:
	fclose(f2);
fail:
	fclose(f);
	return NULL;
}

FILE *fdopen(int fd, const char *mode) {
	FILE *f;
	f = malloc(sizeof *f);
	if (!f) return NULL;
	f->pos = mode[0] == 'a' ? -1 : 0;
	f->eof = false;
	f->fd = fd;
	f->error = false;
	return f;
}

FILE *file_clone(const FILE *f) {
	handle_t h = _syscall_dup(f->fd, -1, 0);
	FILE *f2;
	if (h < 0) return NULL;

	f2 = fdopen(h, "r+");
	if (!f2) {
		close(h);
		return NULL;
	}
	f2->pos = f->pos;
	f2->eof = f->eof;
	f2->fd = h;
	return f2;
}

static void fadvance(long amt, FILE *f) {
	bool pos_neg = f->pos < 0;
	f->pos += amt;
	if (pos_neg && f->pos >= 0)
		f->pos = -1;
}

size_t fread(void *restrict ptr, size_t size, size_t nitems, FILE *restrict f) {
	size_t total = size*nitems, pos = 0;
	unsigned char *buf = ptr;

	if (f->fd < 0) {
		errno = EBADF;
		return 0;
	}
	if (size == 0)
		return 0;

	while (pos < total) {
		long res = _syscall_read(f->fd, buf + pos, total - pos, f->pos);
		if (res < 0) {
			f->error = true;
			errno = -res;
			return pos/size;
		} else if (res == 0) {
			f->eof = true;
			return pos/size;
		} else {
			pos += res;
			fadvance(res, f);
		}
	}
	return nitems;
}

size_t fwrite(const void *restrict ptr, size_t size, size_t nitems, FILE *restrict f) {
	size_t total = size*nitems, pos = 0;
	const unsigned char *buf = ptr;

	if (f->fd < 0) {
		errno = EBADF;
		return 0;
	}
	if (size == 0)
		return 0;

	while (pos < total) {
		long res = _syscall_write(f->fd, buf + pos, total - pos, f->pos);
		if (res < 0) {
			f->error = true;
			errno = -res;
			return pos/size;
		} else if (res == 0) {
			f->eof = true;
			return pos/size;
		} else {
			pos += res;
			fadvance(res, f);
		}
	}
	return nitems;
}

char *fgets(char *buf, int size, FILE *f) {
	char c = '\0';
	size_t pos = 0;
	while (pos < (size-1) && c != '\n' && fread(&c, 1, 1, f))
		buf[pos++] = c;
	buf[pos++] = '\0';

	if (f->eof && pos == 1) return NULL;
	if (f->error) return NULL;
	return buf;
}

int fseek(FILE *f, long offset, int whence) {
	if (fflush(f))
		return -1;

	switch (whence) {
		case SEEK_SET:
			f->pos = 0;
			break;
		case SEEK_CUR:
			break;
		case SEEK_END:
			f->pos = -1;
			// TODO doesn't -1 put the cursor before the last byte? i need to fix up the drivers
			break;
		default:
			errno = EINVAL;
			return -1;
	}

	bool pos_neg = f->pos < 0;
	f->pos += offset;
	if (pos_neg && f->pos >= 0) {
		errno = ENOSYS; // TODO
		return -1;
	}
	f->eof = false;
	return 0;
}

int fclose(FILE *f) {
	fflush(f);
	if (f->fd > 0) close(f->fd);
	if (f != &_stdin_null && f != &_stdout_null)
		free(f);
	return 0;
}

int fflush(FILE *f) {
	(void)f;
	return 0;
}

int feof(FILE *f) {
	return f->eof;
}

int ferror(FILE *f) {
	return f->error;
}
