#include "file.h"
#include <bits/panic.h>
#include <camellia.h>
#include <camellia/syscalls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

static FILE _stdin_null  = { .fd = STDIN_FILENO };
static FILE _stdout_null = { .fd = STDOUT_FILENO };
static FILE _stderr_null = { .fd = STDERR_FILENO };
FILE *const stdin = &_stdin_null;
FILE *const stdout = &_stdout_null;
FILE *const stderr = &_stderr_null;


FILE *fopen(const char *path, const char *mode) {
	FILE *f;
	hid_t h;
	int flags = 0;
	if (!path) {
		errno = 1;
		return NULL;
	} else if (path[0] == '!') {
		/* special handling for "!files" */
		path++;
		if (!strcmp(path, "stdin"))  return file_clone(stdin, mode);
		if (!strcmp(path, "stdout")) return file_clone(stdout, mode);
		if (!strcmp(path, "stderr")) return file_clone(stderr, mode);
		errno = ENOENT;
		return NULL;
	}

	if (strchr(mode, 'e')) {
		/* camellia extension: open as executable */
		flags |= OPEN_EXEC;
	} else if (strchr(mode, 'r')) {
		flags |= OPEN_READ;
		if (strchr(mode, '+'))
			flags |= OPEN_WRITE;
	} else {
		flags |= OPEN_WRITE | OPEN_CREATE;
	}

	h = camellia_open(path, flags);
	if (h < 0) return NULL;

	if (mode[0] == 'w')
		_sys_write(h, NULL, 0, 0, WRITE_TRUNCATE);

	f = fdopen(h, mode);
	if (!f) close(h);
	setvbuf(f, NULL, _IOFBF, 0);
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
		if (_sys_dup(f2->fd, f->fd, 0) < 0) goto fail2;
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
	f = calloc(1, sizeof *f);
	if (f) {
		f->fd = fd;
		f->pos = mode[0] == 'a' ? -1 : 0;
	}
	return f;
}

FILE *file_clone(const FILE *f, const char *mode) {
	hid_t h = _sys_dup(f->fd, -1, 0);
	FILE *f2;
	if (h < 0) return NULL;

	f2 = fdopen(h, mode);
	if (!f2) {
		close(h);
		return NULL;
	}
	f2->pos = f->pos;
	f2->eof = f->eof;
	f2->fd = h;
	return f2;
}

// TODO popen / pclose
FILE *popen(const char *cmd, const char *mode) {
	(void)cmd; (void)mode;
	errno = ENOSYS;
	return NULL;
}

int pclose(FILE *f) {
	(void)f;
	errno = ENOSYS;
	return -1;
}

// TODO tmpfile()
FILE *tmpfile(void) {
	errno = ENOSYS;
	return NULL;
}


int fextflags(FILE *f, int extflags) {
	int old = f->extflags;
	f->extflags = extflags;
	return old;
}

int setvbuf(FILE *restrict f, char *restrict buf, int type, size_t size) {
	if (type == _IONBF) {
		free(f->readbuf);
		f->readbuf = NULL;
		return 0;
	} else if (type == _IOFBF && buf == NULL) {
		(void) size;
		f->rblen = 0;
		f->rbcap = BUFSIZ;
		f->readbuf = malloc(f->rbcap);
		return f->readbuf ? 0 : -1;
	} else {
		return errno = ENOSYS, -1;
	}
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
	if (size == 0) {
		return 0;
	}

	while (pos < total) {
		long res = 0;
		if (f->readbuf) {
			if (0 == f->rblen && total - pos < (f->rbcap >> 1)) {
				res = _sys_read(f->fd, f->readbuf, f->rbcap, f->pos);
				if (res < 0) {
					f->error = true;
					errno = -res;
					break;
				} else if (res == 0) {
					f->eof = true;
					break;
				} else {
					f->rblen = res;
				}
			}
			if (0 < f->rblen) {
				res = MIN(total - pos, f->rblen);
				memcpy(buf + pos, f->readbuf, res);
				f->rblen -= res;
				memmove(f->readbuf, f->readbuf + res, f->rblen);
			}
		}
		if (res == 0) {
			/* no cache hit */
			res = _sys_read(f->fd, buf + pos, total - pos, f->pos);
			if (res < 0) {
				f->error = true;
				errno = -res;
				break;
			} else if (res == 0) {
				f->eof = true;
				break;
			}
		}
		pos += res;
		fadvance(res, f);
		if (f->extflags & FEXT_NOFILL) break;
	}
	return pos == total ? nitems : (pos/size);
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
		long res = _sys_write(f->fd, buf + pos, total - pos, f->pos, 0);
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

int fputs(const char *s, FILE *f) {
	return fprintf(f, "%s\n", s);
}

// TODO! c file buffering
char *fgets(char *buf, int size, FILE *f) {
	int pos, c;
	for (pos = 0; pos < size-1; pos++) {
		c = fgetc(f);
		if (c == EOF) break;
		buf[pos] = c;
		if (c == '\n') break;
	}
	if (pos == 0 || f->error) {
		return NULL;
	} else {
		buf[pos] = '\0';
		return buf;
	}
}

int fgetc(FILE *f) {
	char c;
	size_t ret = fread(&c, 1, 1, f);
	return ret ? c : EOF;
}
int getc(FILE *f) { return fgetc(f); }

int fputc(int c, FILE *f) {
	return fwrite(&c, 1, 1, f) ? c : EOF;
}
int putc(int c, FILE *f) { return fputc(c, f); }

// TODO ungetc
int ungetc(int c, FILE *f) {
	(void)c; (void)f;
	__libc_panic("unimplemented");
}

int fseek(FILE *f, long offset, int whence) {
	return fseeko(f, offset, whence);
}

int fseeko(FILE *f, off_t offset, int whence) {
	if (fflush(f))
		return -1;

	long base;
	switch (whence) {
		case SEEK_SET:
			base = 0;
			break;
		case SEEK_CUR:
			base = f->pos;
			// TODO untested
			if (f->readbuf) {
				base -= f->rblen;
			}
			break;
		case SEEK_END:
			base = _sys_getsize(f->fd);
			if (base < 0)
				base = -1;
			break;
		default:
			errno = EINVAL;
			return -1;
	}
	f->rblen = 0;

	if (base >= 0 && base + offset < 0) {
		/* underflow */
		errno = EINVAL;
		return -1;
	} else if (base < 0 && base + offset >= 0) {
		/* overflow - went from a negative offset (relative to EOF)
		 *            to a positive offset (from start of file).
		 *            can only happen when getsize() is unsupported */
		errno = EINVAL;
		return -1;
	}
	f->pos = base + offset;
	f->eof = false;
	return 0;
}

long ftell(FILE *f) {
	return ftello(f);
}

off_t ftello(FILE *f) {
	return f->pos;
}

int fclose(FILE *f) {
	fflush(f);
	if (f->fd > 0) close(f->fd);
	free(f->readbuf);
	if (f != &_stdin_null && f != &_stdout_null && f != &_stderr_null)
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

void clearerr(FILE *f) {
	f->error = false;
	f->eof = false;
}
